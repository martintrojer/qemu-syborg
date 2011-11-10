/*
 * Virtio PCI bindings
 *
 * Copyright IBM, Corp. 2007
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#include "virtio-pci.h"
#include "virtio-blk.h"
#include "virtio-net.h"
#include "virtio-balloon.h"

/* from Linux's linux/virtio_pci.h */

/* A 32-bit r/o bitmask of the features supported by the host */
#define VIRTIO_PCI_HOST_FEATURES        0

/* A 32-bit r/w bitmask of features activated by the guest */
#define VIRTIO_PCI_GUEST_FEATURES       4

/* A 32-bit r/w PFN for the currently selected queue */
#define VIRTIO_PCI_QUEUE_PFN            8

/* A 16-bit r/o queue size for the currently selected queue */
#define VIRTIO_PCI_QUEUE_NUM            12

/* A 16-bit r/w queue selector */
#define VIRTIO_PCI_QUEUE_SEL            14

/* A 16-bit r/w queue notifier */
#define VIRTIO_PCI_QUEUE_NOTIFY         16

/* An 8-bit device status register.  */
#define VIRTIO_PCI_STATUS               18

/* An 8-bit r/o interrupt status register.  Reading the value will return the
 * current contents of the ISR and will also clear it.  This is effectively
 * a read-and-acknowledge. */
#define VIRTIO_PCI_ISR                  19

#define VIRTIO_PCI_CONFIG               20

/* Virtio ABI version, if we increment this, we break the guest driver. */
#define VIRTIO_PCI_ABI_VERSION          0

/* How many bits to shift physical queue address written to QUEUE_PFN.
 * 12 is historical, and due to x86 page size. */
#define VIRTIO_PCI_QUEUE_ADDR_SHIFT    12

/* Proxy device to link the PCIDevice to the VirtIODevice.  */
typedef struct {
    PCIDevice pci_dev;
    VirtIODevice *vdev;
} VirtIOPCIProxy;

/* Device independent interface code.  */

static VirtIODevice *to_virtio_device(PCIDevice *pci_dev)
{
    VirtIOPCIProxy *proxy = (VirtIOPCIProxy *)pci_dev;
    return proxy->vdev;
}

static void virtio_update_irq_pci(VirtIODevice *vdev)
{
    PCIDevice *pci_dev = (PCIDevice *)vdev->binding_dev;
    qemu_set_irq(pci_dev->irq[0], vdev->isr & 1);
}

static void virtio_ioport_write(void *opaque, uint32_t addr, uint32_t val)
{
    VirtIODevice *vdev = to_virtio_device(opaque);
    target_phys_addr_t pa;

    addr -= vdev->addr;

    switch (addr) {
    case VIRTIO_PCI_GUEST_FEATURES:
        if (vdev->set_features)
            vdev->set_features(vdev, val);
        vdev->features = val;
        break;
    case VIRTIO_PCI_QUEUE_PFN:
        pa = (target_phys_addr_t)val << VIRTIO_PCI_QUEUE_ADDR_SHIFT;
        virtio_set_vring_addr(vdev, vdev->queue_sel, pa);
    case VIRTIO_PCI_QUEUE_SEL:
        if (val < VIRTIO_PCI_QUEUE_MAX)
            vdev->queue_sel = val;
        break;
    case VIRTIO_PCI_QUEUE_NOTIFY:
        virtio_kick(vdev, val);
        break;
    case VIRTIO_PCI_STATUS:
        vdev->status = val & 0xFF;
        if (vdev->status == 0)
            virtio_reset(vdev);
        break;
    }
}

static uint32_t virtio_ioport_read(void *opaque, uint32_t addr)
{
    VirtIODevice *vdev = to_virtio_device(opaque);
    uint32_t ret = 0xFFFFFFFF;

    addr -= vdev->addr;

    switch (addr) {
    case VIRTIO_PCI_HOST_FEATURES:
        ret = vdev->get_features(vdev);
        ret |= (1 << VIRTIO_F_NOTIFY_ON_EMPTY);
        break;
    case VIRTIO_PCI_GUEST_FEATURES:
        ret = vdev->features;
        break;
    case VIRTIO_PCI_QUEUE_PFN:
        ret = virtio_get_vring_pa(vdev, vdev->queue_sel)
              >> VIRTIO_PCI_QUEUE_ADDR_SHIFT;
        break;
    case VIRTIO_PCI_QUEUE_NUM:
        ret = virtio_get_vring_num(vdev, vdev->queue_sel);
        break;
    case VIRTIO_PCI_QUEUE_SEL:
        ret = vdev->queue_sel;
        break;
    case VIRTIO_PCI_STATUS:
        ret = vdev->status;
        break;
    case VIRTIO_PCI_ISR:
        /* reading from the ISR also clears it. */
        ret = vdev->isr;
        vdev->isr = 0;
        virtio_update_irq(vdev);
        break;
    default:
        break;
    }

    return ret;
}

static uint32_t virtio_pci_config_readb(void *opaque, uint32_t addr)
{
    VirtIODevice *vdev = opaque;

    addr -= vdev->addr + VIRTIO_PCI_CONFIG;
    return virtio_config_readb(vdev, addr);
}

static uint32_t virtio_pci_config_readw(void *opaque, uint32_t addr)
{
    VirtIODevice *vdev = opaque;

    addr -= vdev->addr + VIRTIO_PCI_CONFIG;
    return virtio_config_readw(vdev, addr);
}

static uint32_t virtio_pci_config_readl(void *opaque, uint32_t addr)
{
    VirtIODevice *vdev = opaque;

    addr -= vdev->addr + VIRTIO_PCI_CONFIG;
    return virtio_config_readl(vdev, addr);
}

static void virtio_pci_config_writeb(void *opaque, uint32_t addr, uint32_t data)
{
    VirtIODevice *vdev = opaque;

    addr -= vdev->addr + VIRTIO_PCI_CONFIG;
    virtio_config_writeb(vdev, addr, data);
}

static void virtio_pci_config_writew(void *opaque, uint32_t addr, uint32_t data)
{
    VirtIODevice *vdev = opaque;

    addr -= vdev->addr + VIRTIO_PCI_CONFIG;
    virtio_config_writew(vdev, addr, data);
}

static void virtio_pci_config_writel(void *opaque, uint32_t addr, uint32_t data)
{
    VirtIODevice *vdev = opaque;

    addr -= vdev->addr + VIRTIO_PCI_CONFIG;
    virtio_config_writel(vdev, addr, data);
}

static void virtio_map(PCIDevice *pci_dev, int region_num,
                       uint32_t addr, uint32_t size, int type)
{
    VirtIODevice *vdev = to_virtio_device(pci_dev);
    int i;

    vdev->addr = addr;
    for (i = 0; i < 3; i++) {
        register_ioport_write(addr, 20, 1 << i, virtio_ioport_write, vdev);
        register_ioport_read(addr, 20, 1 << i, virtio_ioport_read, vdev);
    }

    if (vdev->config_len) {
        register_ioport_write(addr + 20, vdev->config_len, 1,
                              virtio_pci_config_writeb, vdev);
        register_ioport_write(addr + 20, vdev->config_len, 2,
                              virtio_pci_config_writew, vdev);
        register_ioport_write(addr + 20, vdev->config_len, 4,
                              virtio_pci_config_writel, vdev);
        register_ioport_read(addr + 20, vdev->config_len, 1,
                             virtio_pci_config_readb, vdev);
        register_ioport_read(addr + 20, vdev->config_len, 2,
                             virtio_pci_config_readw, vdev);
        register_ioport_read(addr + 20, vdev->config_len, 4,
                             virtio_pci_config_readl, vdev);

        vdev->get_config(vdev, vdev->config);
    }
}

static void virtio_save_pci(VirtIODevice *vdev, QEMUFile *f)
{
    pci_device_save((PCIDevice *)vdev->binding_dev, f);
}

static void virtio_load_pci(VirtIODevice *vdev, QEMUFile *f)
{
    pci_device_load((PCIDevice *)vdev->binding_dev, f);
}

/* FIXME: PCI bindings should go elsewhere.  */
typedef struct {
    PCIBus *bus;
    int devfn;
    uint16_t vendor;
    uint16_t device;
    uint16_t class_code;
    uint16_t subclass_code;
    uint8_t pif;
} VirtIOPCIArgs;

static VirtIODevice *virtio_init_pci(void *args, const char *name,
                                     uint16_t vendor, uint16_t device,
                                     size_t config_size, size_t struct_size)
{
    VirtIODevice *vdev;
    PCIDevice *pci_dev;
    VirtIOPCIArgs *p = args;
    uint8_t *config;
    uint32_t size;

    pci_dev = pci_register_device(p->bus, name, sizeof(VirtIOPCIProxy),
                                  p->devfn, NULL, NULL);
    if (!pci_dev)
        return NULL;

    vdev = virtio_init_common(name, config_size, struct_size);
    ((VirtIOPCIProxy *)pci_dev)->vdev = vdev;

    vdev->binding_dev = pci_dev;
    vdev->update_irq = virtio_update_irq_pci;
    vdev->save_binding = virtio_save_pci;
    vdev->load_binding = virtio_load_pci;

    config = pci_dev->config;
    config[0x00] = p->vendor & 0xFF;
    config[0x01] = (p->vendor >> 8) & 0xFF;
    config[0x02] = p->device & 0xFF;
    config[0x03] = (p->device >> 8) & 0xFF;

    config[0x08] = VIRTIO_PCI_ABI_VERSION;

    config[0x09] = p->pif;
    config[0x0a] = p->subclass_code;
    config[0x0b] = p->class_code;
    config[0x0e] = 0x00;

    config[0x2c] = vendor & 0xFF;
    config[0x2d] = (vendor >> 8) & 0xFF;
    config[0x2e] = device & 0xFF;
    config[0x2f] = (device >> 8) & 0xFF;

    config[0x3d] = 1;

    size = 20 + config_size;
    if (size & (size-1))
        size = 1 << qemu_fls(size);

    pci_register_io_region(pci_dev, 0, size, PCI_ADDRESS_SPACE_IO,
                           virtio_map);

    return vdev;
}


/* Device specific init routines.  */

void virtio_blk_init_pci(PCIBus *bus, BlockDriverState *bs)
{
    VirtIOPCIArgs args;
    args.bus = bus;
    args.devfn = -1;
    args.vendor = PCI_VENDOR_ID_REDHAT_QUMRANET;
    args.device = PCI_DEVICE_ID_VIRTIO_BLOCK;
    args.class_code = 0x01;
    args.subclass_code = 0x80;
    args.pif = 0x00;
    virtio_blk_init(virtio_init_pci, &args, bs);
}

void virtio_net_init_pci(PCIBus *bus, NICInfo *nd, int devfn)
{
    VirtIOPCIArgs args;
    args.bus = bus;
    args.devfn = devfn;
    args.vendor = PCI_VENDOR_ID_REDHAT_QUMRANET;
    args.device = PCI_DEVICE_ID_VIRTIO_NET;
    args.class_code = 0x02;
    args.subclass_code = 0x00;
    args.pif = 0x00;
    virtio_net_init(virtio_init_pci, &args, nd);
}

void virtio_balloon_init_pci(PCIBus *bus)
{
    VirtIOPCIArgs args;
    args.bus = bus;
    args.devfn = -1;
    args.vendor = PCI_VENDOR_ID_REDHAT_QUMRANET;
    args.device = PCI_DEVICE_ID_VIRTIO_BALLOON;
    args.class_code = 0x05;
    args.subclass_code = 0x00;
    args.pif = 0x00;
    virtio_balloon_init(virtio_init_pci, &args);
}
