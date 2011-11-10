/*
 * Syborg snapshot device
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
#include "qemu-char.h"
#include "syborg.h"
#include "devtree.h"
#include "sysemu.h"

//#define DEBUG_SYBORG_SNAPSHOT

#ifdef DEBUG_SYBORG_SNAPSHOT
#define DPRINTF(fmt, args...) \
do { printf("syborg_snapshot: " fmt , ##args); } while (0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_snapshot: error: " fmt , ##args); exit(1);} while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_snapshot: error: " fmt , ##args);} while (0)
#endif

enum {
    SNAPSHOT_ID           = 0,
    SNAPSHOT_ADDRESS      = 1,
    SNAPSHOT_LENGTH       = 2,
    SNAPSHOT_TRIGGER      = 3
};

typedef struct {
    QEMUDevice *qdev;
    uint32_t address;
    uint32_t length;
} syborg_snapshot_state;

static void syborg_snapshot_trigger(syborg_snapshot_state *s, uint32_t value)
{
    char *buf;

    buf = qemu_malloc(s->length + 1);
    if (!buf)
        return;
    cpu_physical_memory_read(s->address, (void *)buf, s->length);
    buf[s->length] = 0;
    switch (value) {
    case 1:
        DPRINTF("Snapshotting to %s\n", buf);
        qemu_snapshot_request(buf);
        break;
    case 2:
        DPRINTF("Restoring from %s\n", buf);
        qemu_snapshot_request_restore(buf);
        break;
    default:
        break;
    }
    qemu_free(buf);
}

static uint32_t syborg_snapshot_read(void *opaque, target_phys_addr_t offset)
{
    syborg_snapshot_state *s = (syborg_snapshot_state *)opaque;
    
    offset &= 0xfff;
    DPRINTF("read 0x%x\n", (int)offset);
    switch(offset >>2) {
    case SNAPSHOT_ID:
        return SYBORG_ID_SNAPSHOT;
    case SNAPSHOT_ADDRESS:
        return s->address;
    case SNAPSHOT_LENGTH:
        return s->length;
    default:
        BADF("Bad read offset %x\n", (int)offset);
        return 0;  
    }
}

static void syborg_snapshot_write(void *opaque, target_phys_addr_t offset,
                                  uint32_t value)
{
    syborg_snapshot_state *s = (syborg_snapshot_state *)opaque;
    
    offset &= 0xfff;
    DPRINTF("Write 0x%x=0x%x\n", (int)offset, value);
    switch (offset >> 2) {
    case SNAPSHOT_ADDRESS:
        s->address = value;
        break;
    case SNAPSHOT_LENGTH:
        s->length = value;
        break;
    case SNAPSHOT_TRIGGER:
        syborg_snapshot_trigger(s, value);
        break;
    default:
        BADF("Bad write offset %x\n", (int)offset);
        break;
    }
}

static CPUReadMemoryFunc *syborg_snapshot_readfn[] = {
     syborg_snapshot_read,
     syborg_snapshot_read,
     syborg_snapshot_read
};

static CPUWriteMemoryFunc *syborg_snapshot_writefn[] = {
     syborg_snapshot_write,
     syborg_snapshot_write,
     syborg_snapshot_write
};

static void syborg_snapshot_save(QEMUFile *f, void *opaque)
{
    syborg_snapshot_state *s = opaque;

    qemu_put_be32(f, s->address);
    qemu_put_be32(f, s->length);
}

static int syborg_snapshot_load(QEMUFile *f, void *opaque, int version_id)
{
    syborg_snapshot_state *s = opaque;

    if (version_id != 1)
        return -EINVAL;

    s->address = qemu_get_be32(f);
    s->length = qemu_get_be32(f);

    return 0;
}

static void syborg_snapshot_create(QEMUDevice *dev)
{
    syborg_snapshot_state *s;
    s = (syborg_snapshot_state *)qemu_mallocz(sizeof(syborg_snapshot_state));
    s->qdev = dev;
    qdev_set_opaque(dev, s);
}

void syborg_snapshot_register(void)
{
    QEMUDeviceClass *dc;
    dc = qdev_new("syborg,snapshot", syborg_snapshot_create, 0);
    qdev_add_registers(dc, syborg_snapshot_readfn, syborg_snapshot_writefn,
                       0x1000);
    qdev_add_savevm(dc, 1, syborg_snapshot_save, syborg_snapshot_load);
}
