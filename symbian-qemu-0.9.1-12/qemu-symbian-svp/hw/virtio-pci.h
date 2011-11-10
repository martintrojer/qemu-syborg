/*
 * Virtio PCI bindings
 *
 * Copyright 2008 CodeSourcery
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#include "virtio.h"
#include "pci.h"

void virtio_blk_init_pci(PCIBus *bus, BlockDriverState *bs);
void virtio_net_init_pci(PCIBus *bus, NICInfo *nd, int devfn);
void virtio_balloon_init_pci(PCIBus *bus);
