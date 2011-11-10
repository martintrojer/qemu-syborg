/*
 * Syborg platform device
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

#include "hw.h"
#include "syborg.h"
#include "devtree.h"

//#define DEBUG_SYBORG_PLATFORM

#ifdef DEBUG_SYBORG_PLATFORM
#define DPRINTF(fmt, args...) \
do { printf("syborg_platform: " fmt , ##args); } while (0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_platform: error: " fmt , ##args); exit(1);} while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_platform: error: " fmt , ##args);} while (0)
#endif

enum {
    PLATFORM_ID         = 0,
    PLATFORM_TREE_START = 1
};

#define PLATFORM_TREE_OFFSET  0x1000

typedef struct {
    QEMUDevice *qdev;
} syborg_platform_state;

static uint32_t syborg_platform_readl(void *opaque, target_phys_addr_t offset)
{
    offset &= 0xffffff;
    if (offset >= PLATFORM_TREE_OFFSET) {
        offset -= PLATFORM_TREE_OFFSET;
        if (offset >= machine_devtree_size)
            return 0;
        return ldl_p((char *)machine_devtree + offset);
    }
    DPRINTF("read 0x%x\n", (int)offset);
    switch(offset >> 2) {
    case PLATFORM_ID:
        return SYBORG_ID_PLATFORM;
    case PLATFORM_TREE_START:
        return PLATFORM_TREE_OFFSET;
    default:
        cpu_abort(cpu_single_env, "syborg_platform_read: Bad offset %x\n",
                  (int)offset);
        return 0;  
    }
}

static uint32_t syborg_platform_readw(void *opaque, target_phys_addr_t offset)
{
    offset &= 0xffffff;
    if (offset >= PLATFORM_TREE_OFFSET) {
        offset -= PLATFORM_TREE_OFFSET;
        if (offset >= machine_devtree_size)
            return 0;
        return lduw_p((char *)machine_devtree + offset);
    }
    cpu_abort(cpu_single_env, "syborg_platform_readw: Bad offset %x\n",
              (int)offset);
}

static uint32_t syborg_platform_readb(void *opaque, target_phys_addr_t offset)
{
    offset &= 0xffffff;
    if (offset >= PLATFORM_TREE_OFFSET) {
        offset -= PLATFORM_TREE_OFFSET;
        if (offset >= machine_devtree_size)
            return 0;
        return ldub_p((char *)machine_devtree + offset);
    }
    cpu_abort(cpu_single_env, "syborg_platform_readb: Bad offset %x\n",
              (int)offset);
}

static void syborg_platform_write(void *opaque, target_phys_addr_t offset,
                                  uint32_t value)
{
    offset &= 0xffffff;
    cpu_abort(cpu_single_env, "syborg_platform_write: Bad offset %x\n",
              (int)offset);
}

static CPUReadMemoryFunc *syborg_platform_readfn[] = {
     syborg_platform_readb,
     syborg_platform_readw,
     syborg_platform_readl
};

static CPUWriteMemoryFunc *syborg_platform_writefn[] = {
     syborg_platform_write,
     syborg_platform_write,
     syborg_platform_write
};

static void syborg_platform_create(QEMUDevice *dev)
{
    syborg_platform_state *s;
    s = (syborg_platform_state *)qemu_mallocz(sizeof(syborg_platform_state));
    s->qdev = dev;
    qdev_set_opaque(dev, s);
    /* Device has no state, so savevm not needed.  */
}

void syborg_platform_register(void)
{
    QEMUDeviceClass *dc;
    dc = qdev_new("syborg,platform", syborg_platform_create, 0);
    qdev_add_registers(dc, syborg_platform_readfn, syborg_platform_writefn,
                       0x1000000);
}
