/*
 * Syborg NAND flash controller
 *
 * Copyright (c) 2009 CodeSourcery
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
#include "block.h"
#include "flash.h"

//#define DEBUG_SYBORG_NAND

#ifdef DEBUG_SYBORG_NAND
#define DPRINTF(fmt, args...) \
do { printf("syborg_nand: " fmt , ##args); } while (0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_nand: error: " fmt , ##args); exit(1);} while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "syborg_nand: error: " fmt , ##args);} while (0)
#endif

enum {
    SNAND_ID            = 0,
    SNAND_DATA          = 1,
    SNAND_CTL           = 2,
    SNAND_ECC_COUNT     = 3,
    SNAND_ECC_CP        = 4,
    SNAND_ECC_LP        = 5
};

#define SNAND_CTL_CLE   0x01
#define SNAND_CTL_ALE   0x02
#define SNAND_CTL_CE    0x04
#define SNAND_CTL_WP    0x08
#define SNAND_CTL_RB    0x10
#define SNAND_CTL_IN_MASK \
  (SNAND_CTL_CLE | SNAND_CTL_ALE | SNAND_CTL_CE | SNAND_CTL_WP)

typedef struct {
    QEMUDevice *qdev;
    struct nand_flash_s *nand;
    struct ecc_state_s ecc;
    uint32_t ctl;
} syborg_nand_state;

static uint32_t syborg_nand_ecc_lp(syborg_nand_state *s)
{
    uint32_t v0;
    uint32_t v1;

    v0 = s->ecc.lp[0];
    v1 = s->ecc.lp[1];

#define ODD(n) ((v0 << n) & (1 << (2 * n)))
#define EVEN(n) ((v1 << (n + 1)) & (1 << (2 * n + 1)))
    /* Interleave parity bits.  */
    return ODD( 0) | ODD( 1) | ODD( 2) | ODD( 3)
         | ODD( 4) | ODD( 5) | ODD( 6) | ODD( 7)
         | ODD( 8) | ODD( 9) | ODD(10) | ODD(11)
         | ODD(12) | ODD(13) | ODD(14) | ODD(15)
         | EVEN( 0) | EVEN( 1) | EVEN( 2) | EVEN( 3)
         | EVEN( 4) | EVEN( 5) | EVEN( 6) | EVEN( 7)
         | EVEN( 8) | EVEN( 9) | EVEN(10) | EVEN(11)
         | EVEN(12) | EVEN(13) | EVEN(14) | EVEN(15);
#undef ODD
#undef EVEN
}

static uint32_t syborg_nand_read(void *opaque, target_phys_addr_t offset)
{
    syborg_nand_state *s = (syborg_nand_state *)opaque;
    
    offset &= 0xfff;
    DPRINTF("read 0x%x\n", (int)offset);
    switch(offset >> 2) {
    case SNAND_ID:
        return SYBORG_ID_NAND;
    case SNAND_DATA:
        return ecc_digest(&s->ecc, nand_getio(s->nand));
    case SNAND_CTL:
        {
            int rb;
            nand_getpins(s->nand, &rb);
            return s->ctl | (rb ? SNAND_CTL_RB : 0);
        }
    case SNAND_ECC_COUNT:
        return s->ecc.count;
    case SNAND_ECC_CP:
        return s->ecc.cp;
    case SNAND_ECC_LP:
        return syborg_nand_ecc_lp(s);
    default:
        BADF("Bad read offset 0x%x\n", (int)offset);
        return 0;  
    }
}

static void syborg_nand_write(void *opaque, target_phys_addr_t offset,
                              uint32_t value)
{
    syborg_nand_state *s = (syborg_nand_state *)opaque;
    
    offset &= 0xfff;
    DPRINTF("Write 0x%x=0x%x\n", (int)offset, value);
    switch (offset >> 2) {
    case SNAND_DATA:
        nand_setio(s->nand, ecc_digest(&s->ecc, value & 0xff));
        break;
    case SNAND_CTL:
        s->ctl = value & SNAND_CTL_IN_MASK;
        nand_setpins(s->nand,
                     (value & SNAND_CTL_CLE) != 0,
                     (value & SNAND_CTL_ALE) != 0,
                     (value & SNAND_CTL_CE) != 0,
                     (value & SNAND_CTL_WP) != 0,
                     0);
        break;
    case SNAND_ECC_COUNT:
        if (value != 0) {
            BADF("Bad write to ECC count\n");
        }
        ecc_reset(&s->ecc);
        break;
    default:
        BADF("Bad write offset 0x%x\n", (int)offset);
        break;
    }
}

static CPUReadMemoryFunc *syborg_nand_readfn[] = {
     syborg_nand_read,
     syborg_nand_read,
     syborg_nand_read
};

static CPUWriteMemoryFunc *syborg_nand_writefn[] = {
     syborg_nand_write,
     syborg_nand_write,
     syborg_nand_write
};

static void syborg_nand_save(QEMUFile *f, void *opaque)
{
    syborg_nand_state *s = opaque;

    qemu_put_be32(f, s->ctl);
    ecc_put(f, &s->ecc);
}

static int syborg_nand_load(QEMUFile *f, void *opaque, int version_id)
{
    syborg_nand_state *s = opaque;

    if (version_id != 1)
        return -EINVAL;

    s->ctl = qemu_get_be32(f);
    ecc_get(f, &s->ecc);

    return 0;
}

static const struct
{
    int size;
    int id;
} nand_chips[] =
{
    {1, 0x6e},
    {2, 0x64},
    {4, 0x6b},
    {8, 0xd6},
    {16, 0x33},
    {32, 0x35},
    {64, 0x36},
    {128, 0x78},
    {256, 0x71},
    {512, 0xa2},
    {1024, 0xa1},
    {2048, 0xaa},
    {4096, 0xac},
    {8192, 0xa3},
    {16384, 0xa5},
    {0, 0}
};

static void syborg_nand_create(QEMUDevice *dev)
{
    int n;
    int size;
    syborg_nand_state *s;
    s = (syborg_nand_state *)qemu_mallocz(sizeof(syborg_nand_state));
    s->qdev = dev;
    qdev_set_opaque(dev, s);

    size = qdev_get_property_int(dev, "size");
    n = 0;
    while (nand_chips[n].size && nand_chips[n].size != size) {
        n++;
    }
    if (nand_chips[n].size == 0) {
        BADF("Unsuppored chip size: %d\n", size);
        exit(1);
    }
    s->nand = nand_init(NAND_MFR_SAMSUNG, nand_chips[n].id);
    ecc_reset(&s->ecc);
}

void syborg_nand_register(void)
{
    QEMUDeviceClass *dc;
    dc = qdev_new("syborg,nand", syborg_nand_create, 0);
    qdev_add_registers(dc, syborg_nand_readfn, syborg_nand_writefn, 0x1000);
    qdev_add_property_int(dc, "size", 0);
    qdev_add_savevm(dc, 1, syborg_nand_save, syborg_nand_load);
}
