//
// Copyright (c) 2008 Symbian Ltd. All rights reserved.
//
// dummy peripheral used for logging peripheral accesses

#include "hw.h"

typedef struct {
  uint32_t base;
  char tag[32];
} syborg_dummy_state;

static const unsigned int syborg_dummy_id = 0xbabe0007;

static uint32_t syborg_dummy_read(void *opaque, target_phys_addr_t offset)
{
  syborg_dummy_state *s = (syborg_dummy_state *)opaque;
  offset -= s->base;

  printf("[%s] dummy_read b:%x o:%x\n", s->tag, s->base, offset);

  return 0;
}

static void syborg_dummy_write(void *opaque, target_phys_addr_t offset, uint32_t value)
{
  syborg_dummy_state *s = (syborg_dummy_state *)opaque;
  offset -= s->base;

  printf("[%s] dummy_write b:%x o:%x v:%x\n", s->tag, s->base, offset, value);
}

static CPUReadMemoryFunc *syborg_dummy_readfn[] = {
   syborg_dummy_read,
   syborg_dummy_read,
   syborg_dummy_read
};

static CPUWriteMemoryFunc *syborg_dummy_writefn[] = {
   syborg_dummy_write,
   syborg_dummy_write,
   syborg_dummy_write
};

void syborg_dummy_init(uint32_t base, char *tag)
{
  syborg_dummy_state *s;
  int iomemtype;

  s = (syborg_dummy_state *)qemu_mallocz(sizeof(syborg_dummy_state));
  iomemtype = cpu_register_io_memory(0, syborg_dummy_readfn, syborg_dummy_writefn, s);
  cpu_register_physical_memory(base, 0x00001000, iomemtype);
  s->base = base;
  memcpy(s->tag, tag, 32);
  s->tag[31] = 0;
}

