/*
 *  Dynamic device configuration and creation.
 *
 *  Copyright (c) 2008 CodeSourcery
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* FIXME: check all malloc/strdup exit coeds.  Or better still have
   malloc/strdup abort.  */

#include "qemu-common.h"
#include "sysemu.h"
#include "devtree.h"
#include "hw/boards.h"
#include "libfdt/libfdt.h"
#include "qemu-char.h"

#define BADF(fmt, args...) \
do { fprintf(stderr, "error: " fmt , ##args); exit(1);} while (0)

/* Assume no device will ever need more than 4 register windows.  */
#define MAX_DEV_REGS 4

enum QEMUDeicePropetyType {
    QDEV_PROP_INT,
    QDEV_PROP_STRING
};

typedef struct QEMUDeviceProperty {
    const char *name;
    enum QEMUDeicePropetyType type;
    union {
        int i;
        char *string;
    } value;
    struct QEMUDeviceProperty *next;
} QEMUDeviceProperty;

struct QEMUDeviceClass {
    struct QEMUDeviceClass *next;
    const char *name;
    void *opaque;
    QEMUDeviceProperty *properties;
    int num_irqs;
    int num_regs;
    CPUReadMemoryFunc **mem_read[MAX_DEV_REGS];
    CPUWriteMemoryFunc **mem_write[MAX_DEV_REGS];
    target_phys_addr_t mem_size[MAX_DEV_REGS];
    QDEVCreateFn create;
    SaveStateHandler *save_state;
    LoadStateHandler *load_state;
    int savevm_version;
    unsigned has_chardev:1;
};

struct QEMUDevice {
    QEMUDevice *next;
    QEMUDeviceClass *dc;
    QEMUDeviceProperty *properties;
    qemu_irq **irqp;
    qemu_irq *irq;
    CharDriverState *chardev;
    qemu_irq *irq_sink;
    void *mem_opaque[MAX_DEV_REGS];
    void *opaque;
    int irq_sink_count;
    const void *dt;
    int node_offset;
    uint32_t phandle;
};

const void *machine_devtree;
int machine_devtree_size;

devtree_ram_region *devtree_ram_map;
int devtree_ram_map_size;

/* Device (class) registration.  */

QEMUDeviceClass *cpu_device_class;

static QEMUDeviceClass *all_dc;

QEMUDeviceClass *qdev_new(const char *name, QDEVCreateFn create, int nirq)
{
    QEMUDeviceClass *dc = qemu_mallocz(sizeof(*dc));

    dc->num_irqs = nirq;
    dc->create = create;
    dc->name = qemu_strdup(name);

    dc->next = all_dc;
    all_dc = dc;

    return dc;
}

void qdev_add_chardev(QEMUDeviceClass *dc)
{
    if (dc->has_chardev) {
        BADF("Device class %s already has a chardev\n", dc->name);
    }
    dc->has_chardev = 1;
}

void qdev_add_property_string(QEMUDeviceClass *dc, const char *name,
                              const char *def)
{
    QEMUDeviceProperty *p = qemu_mallocz(sizeof(*p));

    p->name = qemu_strdup(name);
    p->type = QDEV_PROP_STRING;
    if (def)
        p->value.string = qemu_strdup(def);
    p->next = dc->properties;
    dc->properties = p;
}

void qdev_add_property_int(QEMUDeviceClass *dc, const char *name, int def)
{
    QEMUDeviceProperty *p = qemu_mallocz(sizeof(*p));

    p->name = qemu_strdup(name);
    p->type = QDEV_PROP_INT;
    p->value.i = def;
    p->next = dc->properties;
    dc->properties = p;
}

void qdev_add_registers(QEMUDeviceClass *dc, CPUReadMemoryFunc **mem_read,
                        CPUWriteMemoryFunc **mem_write,
                        target_phys_addr_t mem_size)
{
    if (dc->num_regs == MAX_DEV_REGS) {
        BADF("too many regs");
        return;
    }

    dc->mem_read[dc->num_regs] = mem_read;
    dc->mem_write[dc->num_regs] = mem_write;
    dc->mem_size[dc->num_regs] = mem_size;
    dc->num_regs++;
}

void qdev_add_class_opaque(QEMUDeviceClass *dc, void *opaque)
{
    dc->opaque = opaque;
}

void qdev_add_savevm(QEMUDeviceClass *dc, int ver,
                     SaveStateHandler *save_state,
                     LoadStateHandler *load_state)
{
    dc->savevm_version = ver;
    dc->save_state = save_state;
    dc->load_state = load_state;
}

static QEMUDeviceProperty *qdev_copy_properties(QEMUDeviceProperty *src)
{
    QEMUDeviceProperty *first;
    QEMUDeviceProperty **p;
    QEMUDeviceProperty *dest;

    first = NULL;
    p = &first;
    while (src) {
        dest = qemu_mallocz(sizeof(*dest));
        dest->name = src->name;
        dest->type = src->type;
        switch (src->type) {
        case QDEV_PROP_INT:
            dest->value.i = src->value.i;
            break;
        case QDEV_PROP_STRING:
            if (src->value.string)
                dest->value.string = qemu_strdup(src->value.string);
            break;
        }
        src = src->next;
        *p = dest;
        p = &dest->next;
    }
    return first;
}


/* Device manipulation.  */

static QEMUDevice *first_device;

static QEMUDevice *qdev_create(QEMUDeviceClass *dc, const void *dt,
                               int node_offset)
{
    QEMUDevice *dev = qemu_mallocz(sizeof(*dc));

    dev->dc = dc;
    dev->properties = qdev_copy_properties(dc->properties);
    if (dc->num_irqs) {
        dev->irqp = qemu_mallocz(dc->num_irqs * sizeof(qemu_irq *));
        dev->irq = qemu_mallocz(dc->num_irqs * sizeof(qemu_irq));
    }
    dev->node_offset = node_offset;
    dev->dt = dt;
    dev->phandle = fdt_get_phandle(dt, node_offset);

    dev->next = first_device;
    first_device = dev;

    return dev;
}

/* IRQs are not created/linked until all devices have been created.
   This function take a pointer to a qemu_irq object, which will be
   populated later.  */
/* FIXME: Should we just have qdev_irq_{raise,lower}?  */
void qdev_get_irq(QEMUDevice *dev, int n, qemu_irq *p)
{
    if (n >= dev->dc->num_irqs)
        BADF("Bad IRQ %d (%d)\n", n, dev->dc->num_irqs);
    dev->irqp[n] = p;
}

CharDriverState *qdev_get_chardev(QEMUDevice *dev)
{
    return dev->chardev;
}

void qdev_create_interrupts(QEMUDevice *dev, qemu_irq_handler handler, 
                            void *opaque, int n)
{
    dev->irq_sink = qemu_allocate_irqs(handler, opaque, n);
    dev->irq_sink_count = n;
}

int qdev_get_property_int(QEMUDevice *dev, const char *name)
{
    QEMUDeviceProperty *p;

    for (p = dev->properties; p; p = p->next) {
        if (strcmp(name, p->name) == 0) {
            if (p->type != QDEV_PROP_INT)
                abort();
            return p->value.i;
        }
    }
    abort();
}

const char *qdev_get_property_string(QEMUDevice *dev, const char *name)
{
    QEMUDeviceProperty *p;

    for (p = dev->properties; p; p = p->next) {
        if (strcmp(name, p->name) == 0) {
            if (p->type != QDEV_PROP_STRING)
                abort();
            return p->value.string;
        }
    }
    abort();
}

const char *qdev_get_name(QEMUDevice *dev)
{
    return fdt_get_name(dev->dt, dev->node_offset, NULL);
}

void *qdev_get_class_opaque(QEMUDevice *dev)
{
    return dev->dc->opaque;
}

void qdev_set_opaque(QEMUDevice *dev, void *opaque)
{
    dev->opaque = opaque;
}

void qdev_set_region_opaque(QEMUDevice *dev, int n, void *opaque)
{
    dev->mem_opaque[n] = opaque;
}

void qdev_set_irq_level(QEMUDevice *dev, int n, int level)
{
    if (n < 0 || n > dev->dc->num_irqs)
        return;

    qemu_set_irq(dev->irq[n], level);
}

/* FDT handling.  */

static void invalid_devtree(QEMUDevice *dev, const char *msg)
{
    fprintf(stderr, "devtree: %s: %s\n", dev->dc->name, msg);
    exit(1);
}

static const char *fdt_getprop_string(const void *dt, int node,
                                      const char * name)
{
    const char *p;
    int len;

    p = fdt_getprop(dt, node, name, &len);
    if (!p || len == 0)
        return NULL;
    /* Check string is properly terminated.  If the wrong kind of property
       is used then this may not be true.  */
    if (p[len - 1] != 0)
        return NULL;
    return p;
}

static void find_properties(QEMUDevice *dev)
{
    const struct fdt_property *p;
    QEMUDeviceProperty *dp;
    int len;

    for (dp = dev->properties; dp; dp = dp->next) {
        p = fdt_get_property(dev->dt, dev->node_offset, dp->name, &len);
        if (!p)
            continue;
        switch (dp->type) {
        case QDEV_PROP_INT:
            if (len != 4) {
                invalid_devtree(dev, "Bad integer property");
                break;
            }
            dp->value.i = fdt32_to_cpu(*(uint32_t *)p->data);
            break;
        case QDEV_PROP_STRING:
            if (len == 0 || p->data[len - 1]) {
                invalid_devtree(dev, "Bad string property");
                break;
            }
            if (dp->value.string)
                qemu_free(dp->value.string);
            dp->value.string = qemu_strdup((const char *)p->data);
            break;
        }
    }
}

/* We currently assume a fixed address/size.  Enforce that here.  */
static void check_cells(const void *dt, int node, int address, int size)
{
    const struct fdt_property *p;
    int parent;
    int len;
    int n;

    parent = fdt_parent_offset(dt, node);
    if (node < 0) {
        fprintf(stderr, "missing parent node for %s\n",
                fdt_get_name(dt, node, NULL));
        exit(1);
    }
    p = fdt_get_property(dt, parent, "#address-cells", &len);
    if (!p || len != 4) {
        fprintf(stderr,
                "Invalid or missing #address-cells for %s\n",
                fdt_get_name(dt, node, NULL));
        exit(1);
    }
    n = fdt32_to_cpu(*(uint32_t *)p->data);
    if (n != address) {
        fprintf(stderr,
                "Incorrect #address-cells for %s (expected %d got %d)\n",
                fdt_get_name(dt, node, NULL), address, n);
        exit(1);
    }
    p = fdt_get_property(dt, parent, "#size-cells", &len);
    if (!p || len != 4) {
        fprintf(stderr,
                "Invalid or missing #size-cells for %s\n",
                fdt_get_name(dt, node, NULL));
        exit(1);
    }
    n = fdt32_to_cpu(*(uint32_t *)p->data);
    if (n != size) {
        fprintf(stderr,
                "Incorrect #size-cells for %s (expected %d got %d)\n",
                fdt_get_name(dt, node, NULL), size, n);
        exit(1);
    }
}

static void create_from_node(QEMUDeviceClass *dc, const void *dt, int node)
{
    QEMUDevice *d;
    const char *propstr;
    int i;

    d = qdev_create(dc, dt, node);
    if (dc->has_chardev) {
        int n;
        propstr = fdt_getprop_string(dt, node, "chardev");
        if (propstr) {
            i = sscanf(propstr, "serial%d", &n);
            if (i == 1 && n >= 0 && n < MAX_SERIAL_PORTS)
            {
                if (!serial_hds[n])
                {
                    const char* target = fdt_getprop_string(dt, node, "target");
                    if (target)
                        serial_hds[n] = qemu_chr_open(propstr, target);
                }
                d->chardev = serial_hds[n];
            }
        }
    }
    find_properties(d);
    d->dc->create(d);
    if (dc->savevm_version) {
        register_savevm(dc->name, -1, dc->savevm_version,
                        dc->save_state, dc->load_state, d->opaque);
    }
    if (dc->num_regs) {
        const struct fdt_property *p;
        uint32_t base;
        uint32_t *data;
        void *opaque;
        int iomemtype;
        int len;

        check_cells(dt, node, 1, 0);
        p = fdt_get_property(dt, node, "reg", &len);
        if (!p || len != dc->num_regs * 4) {
            invalid_devtree(d, "Missing reg");
            return;
        }
        data = (uint32_t *)p->data;
        for (i = 0; i < dc->num_regs; i++) {
            base = fdt32_to_cpu(*data);
            data++;
            opaque = d->mem_opaque[i];
            if (!opaque)
                opaque = d->opaque;
            iomemtype = cpu_register_io_memory(0, dc->mem_read[i],
                                               dc->mem_write[i], opaque);
            cpu_register_physical_memory(base, dc->mem_size[i], iomemtype);
        }
    }
}

static void scan_devtree(const void *dt)
{
    QEMUDeviceClass *dc;
    int node;

    for (dc = all_dc; dc; dc = dc->next) {
        node = -1;
        while (1) {
            node = fdt_node_offset_by_compatible(dt, node, dc->name);
            if (node < 0)
                break;
            create_from_node(dc, dt, node);
        }
    }
}

/* Create CPU devices.  These are devices so that they can have interrupts.  */
static void create_cpus(const void *dt)
{
    int node = -1;

    while (1) {
        node = fdt_node_offset_by_prop_value(dt, node, "device_type",
                                             "cpu", 4);
        if (node < 0)
            break;
        create_from_node(cpu_device_class, dt, node);
    }
}

/* Add RAM.  */
static void create_ram(const void *dt)
{
    int node = -1;
    const struct fdt_property *p;
    int len;
    uint32_t base;
    uint32_t size;
    uint32_t *data;
    ram_addr_t offset;

    while (1) {
        node = fdt_node_offset_by_prop_value(dt, node, "device_type",
                                             "memory", 7);
        if (node < 0)
            break;


        check_cells(dt, node, 1, 1);
        p = fdt_get_property(dt, node, "reg", &len);
        if (!p || (len % 8) != 0) {
            fprintf(stderr, "bad memory section %s\n",
                    fdt_get_name(dt, node, NULL));
            exit(1);
        }
        data = (uint32_t *)p->data;
        while (len) {
            base = fdt32_to_cpu(data[0]);
            size = fdt32_to_cpu(data[1]);
            data += 2;
            len -= 8;
            /* Ignore zero size regions.  */
            if (size == 0)
                continue;
            offset = qemu_ram_alloc(size);
            cpu_register_physical_memory(base, size, offset | IO_MEM_RAM);

            devtree_ram_map_size++;
            devtree_ram_map = qemu_realloc(devtree_ram_map,
                devtree_ram_map_size * sizeof(devtree_ram_region));
            devtree_ram_map[devtree_ram_map_size - 1].base = base;
            devtree_ram_map[devtree_ram_map_size - 1].size = size;
        }
    }
    /* FIXME: Merge and sort memory map entries.  */
    /* Technically there's no reason we have to have RAM.  However in
       practice it indicates a busted machine description.  */
    if (!devtree_ram_map) {
        fprintf(stderr, "No memory regions found\n");
        exit(1);
    }
}

static QEMUDevice *find_device_by_phandle(uint32_t phandle)
{
    QEMUDevice *dev;
    for (dev = first_device; dev; dev = dev->next) {
        if (dev->phandle == phandle)
            return dev;
    }
    return NULL;
}

/* We currently assume #interrupt-cells is 1.  */
static void check_interrupt_cells(QEMUDevice *dev)
{
    const struct fdt_property *p;
    int len;

    p = fdt_get_property(dev->dt, dev->node_offset, "#interrupt-cells", &len);
    /* Allow a missing value.  Useful for devices that are pointed to by
       a qemu,interrupts property.  */
    if (!p)
        return;
    if (len != 4) {
        invalid_devtree(dev, "Invalid #interrupt-cells");
    }
    if (fdt32_to_cpu(*(uint32_t *)p->data) != 1) {
        invalid_devtree(dev, "#interrupt-cells must be 1");
    }
}

static QEMUDevice *find_interrupt_parent(QEMUDevice *dev)
{
    const struct fdt_property *p;
    QEMUDevice *parent;
    uint32_t phandle;
    int len;

    p = fdt_get_property(dev->dt, dev->node_offset, "interrupt-parent", &len);
    if (!p)
        return NULL;
    if (len != 4) {
        invalid_devtree(dev, "bad/missing interrupt-parent");
        return NULL;
    }
    phandle = fdt32_to_cpu(*(uint32_t *)p->data);

    parent = find_device_by_phandle(phandle);
    if (!parent) {
        invalid_devtree(dev, "interrupt-parent not found");
    }
    check_interrupt_cells(parent);
    return parent;
}

static void fixup_irqs(void)
{
    QEMUDevice *dev;
    QEMUDevice *parent;
    const struct fdt_property *prop;
    int len;
    int i;
    qemu_irq parent_irq;
    int is_qemu_irq = 0;
    uint32_t *data;

    for (dev = first_device; dev; dev = dev->next) {
        if (dev->dc->num_irqs) {
            parent = find_interrupt_parent(dev);
            if (!parent) {
                prop = fdt_get_property(dev->dt, dev->node_offset,
                                        "qemu,interrupts", &len);
                if (!prop) {
                    invalid_devtree(dev, "missing interrupt-parent");
                    continue;
                }
                if (len != dev->dc->num_irqs * 8) {
                    invalid_devtree(dev, "bad interrupts");
                    continue;
                }
                is_qemu_irq = 1;
            } else {
                prop = fdt_get_property(dev->dt, dev->node_offset,
                                        "interrupts", &len);
                if (!prop || len != dev->dc->num_irqs * 4) {
                    invalid_devtree(dev, "bad/missing interrupts");
                    continue;
                }
                is_qemu_irq = 0;
            }
            data = (uint32_t *)prop->data;
            /* FIXME: Need to handle interrupt remapping.  */
            for (i = 0; i < dev->dc->num_irqs; i++) {
                uint32_t parent_irq_num;
                if (is_qemu_irq) {
                    parent = find_device_by_phandle(fdt32_to_cpu(*data));
                    data++;
                    if (!parent) {
                        invalid_devtree(dev, "bad qemu,interrupts");
                    }
                    check_interrupt_cells(parent);
                }
                parent_irq_num = fdt32_to_cpu(*data);
                data++;
                if (parent_irq_num >= parent->irq_sink_count) {
                    invalid_devtree(dev, "bad interrupt number");
                    continue;
                }
                parent_irq = parent->irq_sink[parent_irq_num];
                dev->irq[i] = parent_irq;
                if (dev->irqp[i])
                    *(dev->irqp[i]) = parent_irq;
            }
        }
    }
}

static void parse_devtree(const char *filename)
{
    FILE *f;
    void *dt;

    f = fopen(filename, "rb");
    if (!f)
        goto err;
    fseek(f, 0, SEEK_END);
    machine_devtree_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    dt = qemu_malloc(machine_devtree_size);
    if (!dt)
        goto err_close;
    machine_devtree = dt;
    if (fread(dt, machine_devtree_size, 1, f) != 1)
        goto err_close;
    if (fdt_check_header(dt))
        goto err_close;

    create_cpus(dt);

    create_ram(dt);

    scan_devtree(dt);

    fixup_irqs();

    fclose(f);
    return;

err_close:
    fclose(f);
err:
    fprintf(stderr, "Failed to load device tree\n");
    exit(1);
}

int devtree_get_config_int(const char *name, int def)
{
    const struct fdt_property *p;
    int len;
    int node;

    node = fdt_path_offset(machine_devtree, "/chosen");
    if (node < 0)
        return def;
    p = fdt_get_property(machine_devtree, node, name, &len);
    if (!p)
        return def;
    if (len != 4) {
        fprintf(stderr, "Expected integer for /chosen/%s\n", name);
        exit(1);
    }
    return fdt32_to_cpu(*(uint32_t *)p->data);
}

static void devtree_machine_init(ram_addr_t ram_size, int vga_ram_size,
                            const char *boot_device, DisplayState *ds,
                            const char *kernel_filename, const char *kernel_cmdline,
                            const char *initrd_filename, const char *cpu_model)
{
    cpu_device_register();
    register_devices();
    parse_devtree(devtree_machine.name);
    /* FIXME: Get these values from device tree.  */
    cpu_bootstrap(kernel_filename, kernel_cmdline, initrd_filename);
}

QEMUMachine devtree_machine = {
    .name = "",
    .desc = "Device tree",
    .init = devtree_machine_init,
    .max_cpus = 1,
};
