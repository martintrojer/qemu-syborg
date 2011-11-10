/*
 * Virtio Syborg bindings
 *
 * Copyright (c) 2008 CodeSourcery
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "virtio.h"
#include "syborg.h"
#include "devtree.h"
#include "virtio-net.h"
#include "virtio-audio.h"
#include "sysemu.h"

//#define DEBUG_SYBORG_VIRTIO

#ifdef DEBUG_SYBORG_VIRTIO
#define DPRINTF(fmt, args...) \
do { printf("syborg_virtio: " fmt , ##args); } while (0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_virtio: error: " fmt , ##args); exit(1);} while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_virtio: error: " fmt , ##args);} while (0)
#endif

enum {
    SYBORG_VIRTIO_ID             = 0,
    SYBORG_VIRTIO_DEVTYPE        = 1,
    SYBORG_VIRTIO_HOST_FEATURES  = 2,
    SYBORG_VIRTIO_GUEST_FEATURES = 3,
    SYBORG_VIRTIO_QUEUE_BASE     = 4,
    SYBORG_VIRTIO_QUEUE_NUM      = 5,
    SYBORG_VIRTIO_QUEUE_SEL      = 6,
    SYBORG_VIRTIO_QUEUE_NOTIFY   = 7,
    SYBORG_VIRTIO_STATUS         = 8,
    SYBORG_VIRTIO_INT_ENABLE     = 9,
    SYBORG_VIRTIO_INT_STATUS     = 10
};

#define SYBORG_VIRTIO_CONFIG 0x100

/* Device independent interface.  */

typedef struct {
    QEMUDevice *qdev;
    VirtIODevice *vdev;
    uint32_t int_enable;
    qemu_irq irq;
    int id;
} syborg_virtio_state;

static void syborg_virtio_update_irq(VirtIODevice *vdev)
{
    syborg_virtio_state *s = vdev->binding_dev;
    int level;

    level = s->int_enable & vdev->isr;
    DPRINTF("IRQ %d\n", level);
    qemu_set_irq(s->irq, level != 0);
}

static uint32_t syborg_virtio_readl(void *opaque, target_phys_addr_t offset)
{
    syborg_virtio_state *s = (syborg_virtio_state *)opaque;
    VirtIODevice *vdev = s->vdev;
    uint32_t ret;

    DPRINTF("readl 0x%x\n", (int)offset);
    offset &= 0xfff;
    if (offset >= SYBORG_VIRTIO_CONFIG) {
        return virtio_config_readl(vdev, offset - SYBORG_VIRTIO_CONFIG);
    }
    switch(offset >> 2) {
    case SYBORG_VIRTIO_ID:
        ret = SYBORG_ID_VIRTIO;
        break;
    case SYBORG_VIRTIO_DEVTYPE:
        ret = s->id;
        break;
    case SYBORG_VIRTIO_HOST_FEATURES:
        ret = vdev->get_features(vdev);
        ret |= (1 << VIRTIO_F_NOTIFY_ON_EMPTY);
        break;
    case SYBORG_VIRTIO_GUEST_FEATURES:
        ret = vdev->features;
        break;
    case SYBORG_VIRTIO_QUEUE_BASE:
        ret = virtio_get_vring_pa(vdev, vdev->queue_sel);
        break;
    case SYBORG_VIRTIO_QUEUE_NUM:
        ret = virtio_get_vring_num(vdev, vdev->queue_sel);
        break;
    case SYBORG_VIRTIO_QUEUE_SEL:
        ret = vdev->queue_sel;
        break;
    case SYBORG_VIRTIO_STATUS:
        ret = vdev->status;
        break;
    case SYBORG_VIRTIO_INT_ENABLE:
        ret = s->int_enable;
        break;
    case SYBORG_VIRTIO_INT_STATUS:
        ret = vdev->isr;
        break;
    default:
        BADF("Bad read offset 0x%x\n", (int)offset);
        return 0;  
    }
    return ret;
}

static void syborg_virtio_writel(void *opaque, target_phys_addr_t offset,
                                 uint32_t value)
{
    syborg_virtio_state *s = (syborg_virtio_state *)opaque;
    VirtIODevice *vdev = s->vdev;

    DPRINTF("writel 0x%x = 0x%x\n", (int)offset, value);
    offset &= 0xfff;
    if (offset >= SYBORG_VIRTIO_CONFIG) {
        return virtio_config_writel(vdev, offset - SYBORG_VIRTIO_CONFIG,
                                    value);
    }
    switch (offset >> 2) {
    case SYBORG_VIRTIO_GUEST_FEATURES:
        if (vdev->set_features)
            vdev->set_features(vdev, value);
        vdev->features = value;
        break;
    case SYBORG_VIRTIO_QUEUE_BASE:
        virtio_set_vring_addr(vdev, vdev->queue_sel, value);
        break;
    case SYBORG_VIRTIO_QUEUE_SEL:
        if (value < VIRTIO_PCI_QUEUE_MAX)
            vdev->queue_sel = value;
        break;
    case SYBORG_VIRTIO_QUEUE_NOTIFY:
        virtio_kick(vdev, value);
        break;
    case SYBORG_VIRTIO_STATUS:
        vdev->status = value & 0xFF;
        if (vdev->status == 0)
            virtio_reset(vdev);
        break;
    case SYBORG_VIRTIO_INT_ENABLE:
        s->int_enable = value;
        virtio_update_irq(vdev);
        break;
    case SYBORG_VIRTIO_INT_STATUS:
        vdev->isr &= ~value;
        virtio_update_irq(vdev);
        break;
    default:
        BADF("Bad write offset 0x%x\n", (int)offset);
        break;
    }
}

static uint32_t syborg_virtio_readw(void *opaque, target_phys_addr_t offset)
{
    syborg_virtio_state *s = (syborg_virtio_state *)opaque;
    VirtIODevice *vdev = s->vdev;

    DPRINTF("readw 0x%x\n", (int)offset);
    offset &= 0xfff;
    if (offset >= SYBORG_VIRTIO_CONFIG) {
        return virtio_config_readw(vdev, offset - SYBORG_VIRTIO_CONFIG);
    }
    BADF("Bad halfword read offset 0x%x\n", (int)offset);
    return -1;
}

static void syborg_virtio_writew(void *opaque, target_phys_addr_t offset,
                                 uint32_t value)
{
    syborg_virtio_state *s = (syborg_virtio_state *)opaque;
    VirtIODevice *vdev = s->vdev;

    DPRINTF("writew 0x%x = 0x%x\n", (int)offset, value);
    offset &= 0xfff;
    if (offset >= SYBORG_VIRTIO_CONFIG) {
        return virtio_config_writew(vdev, offset - SYBORG_VIRTIO_CONFIG,
                                    value);
    }
    BADF("Bad halfword write offset 0x%x\n", (int)offset);
}

static uint32_t syborg_virtio_readb(void *opaque, target_phys_addr_t offset)
{
    syborg_virtio_state *s = (syborg_virtio_state *)opaque;
    VirtIODevice *vdev = s->vdev;

    DPRINTF("readb 0x%x\n", (int)offset);
    offset &= 0xfff;
    if (offset >= SYBORG_VIRTIO_CONFIG) {
        return virtio_config_readb(vdev, offset - SYBORG_VIRTIO_CONFIG);
    }
    BADF("Bad byte read offset 0x%x\n", (int)offset);
    return -1;
}

static void syborg_virtio_writeb(void *opaque, target_phys_addr_t offset,
                                 uint32_t value)
{
    syborg_virtio_state *s = (syborg_virtio_state *)opaque;
    VirtIODevice *vdev = s->vdev;

    DPRINTF("writeb 0x%x = 0x%x\n", (int)offset, value);
    offset &= 0xfff;
    if (offset >= SYBORG_VIRTIO_CONFIG) {
        return virtio_config_writeb(vdev, offset - SYBORG_VIRTIO_CONFIG,
                                    value);
    }
    BADF("Bad byte write offset 0x%x\n", (int)offset);
}

static CPUReadMemoryFunc *syborg_virtio_readfn[] = {
     syborg_virtio_readb,
     syborg_virtio_readw,
     syborg_virtio_readl
};

static CPUWriteMemoryFunc *syborg_virtio_writefn[] = {
     syborg_virtio_writeb,
     syborg_virtio_writew,
     syborg_virtio_writel
};

static void syborg_virtio_save_binding(VirtIODevice *vdev, QEMUFile *f)
{
    /* Nothing to do.  */
}

static void syborg_virtio_load_binding(VirtIODevice *vdev, QEMUFile *f)
{
    /* Nothing to do.  */
}

typedef struct {
    QEMUDevice *qdev;
}  VirtIOSyborgArgs;

static VirtIODevice *syborg_virtio_create(void *args, const char *name,
                                          uint16_t vendor, uint16_t type,
                                          size_t config_size,
                                          size_t struct_size)
{
    VirtIODevice *vdev;
    VirtIOSyborgArgs *p = args;
    syborg_virtio_state *s;

    s = (syborg_virtio_state *)qemu_mallocz(sizeof(syborg_virtio_state));
    s->qdev = p->qdev;
    qdev_set_opaque(s->qdev, s);
    s->id = ((uint32_t)vendor << 16) | type;
    qdev_get_irq(s->qdev, 0, &s->irq);

    vdev = virtio_init_common(name, config_size, struct_size);
    s->vdev = vdev;

    vdev->binding_dev = s;
    vdev->update_irq = syborg_virtio_update_irq;
    vdev->save_binding = syborg_virtio_save_binding;
    vdev->load_binding = syborg_virtio_load_binding;

    return vdev;
}

/* Device specific bindings.  */

static void syborg_virtio_net_create(QEMUDevice *dev)
{
    int nic = nb_nics;
    VirtIOSyborgArgs args;
    args.qdev = dev;
    /* FIXME: add net options.  */
    if (net_client_init("nic", "") < 0)
        return;
    nd_table[nic].model = "virtio-net";
    virtio_net_init(syborg_virtio_create, &args, &nd_table[nic]);
}

static AudioState *audio_state;

static void syborg_virtio_audio_create(QEMUDevice *dev)
{
    VirtIOSyborgArgs args;
    args.qdev = dev;
    if (!audio_state)
        audio_state = AUD_init();
    virtio_audio_init(syborg_virtio_create, &args, audio_state);
}

static QEMUDeviceClass *syborg_virtio_register_common(const char *name,
                                                      QDEVCreateFn create)
{
    QEMUDeviceClass *dc;
    dc = qdev_new(name, create, 1);
    qdev_add_registers(dc, syborg_virtio_readfn, syborg_virtio_writefn,
                       0x1000);
    return dc;
}

void syborg_virtio_register(void)
{
    QEMUDeviceClass *dc;
    dc = syborg_virtio_register_common("syborg,virtio-net",
                                       syborg_virtio_net_create);
    /* FIXME */
    //qdev_add_netdev(dc);
    dc = syborg_virtio_register_common("syborg,virtio-audio",
                                       syborg_virtio_audio_create);
}
