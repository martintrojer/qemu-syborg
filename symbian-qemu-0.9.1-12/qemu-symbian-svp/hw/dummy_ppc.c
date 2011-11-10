/*
 * Dummy board with just RAM and CPU for use as an ISS.
 *
 * Copyright (c) 2007 CodeSourcery.
 *
 * This code is licenced under the GPL
 */

#include "hw.h"
#include "sysemu.h"
#include "boards.h"
#include "ppc.h"

/* Board init.  */

static void dummy_ppc_init(ram_addr_t ram_size, int vga_ram_size,
                     const char *boot_device, DisplayState *ds,
                     const char *kernel_filename, const char *kernel_cmdline,
                     const char *initrd_filename, const char *cpu_model)
{
    CPUState *env;
    int kernel_size;
    uint64_t elf_entry;
    target_ulong entry;

    if (!cpu_model)
        cpu_model = "default";
    env = cpu_init(cpu_model);
    if (!env) {
        fprintf(stderr, "Unable to find ppc CPU definition\n");
        exit(1);
    }

    cpu_ppc_tb_init(env, 100UL * 1000UL * 1000UL);
    /* RAM at address zero */
    cpu_register_physical_memory(0x10000000, ram_size,
        qemu_ram_alloc(ram_size) | IO_MEM_RAM);

    /* Load kernel.  */
    if (kernel_filename) {
        kernel_size = load_elf(kernel_filename, 0, &elf_entry, NULL, NULL);
        entry = elf_entry;
        if (kernel_size < 0) {
            kernel_size = load_uimage(kernel_filename, &entry, NULL, NULL);
        }
        if (kernel_size < 0) {
            kernel_size = load_image(kernel_filename, phys_ram_base);
            entry = 0;
        }
        if (kernel_size < 0) {
            fprintf(stderr, "qemu: could not load kernel '%s'\n",
                    kernel_filename);
            exit(1);
        }
    } else {
        entry = 0;
    }
    env->nip = entry;
    if (semihosting_enabled) {
        /* Setup initial stack.  */
        target_ulong *p;
        p = (target_ulong *)(phys_ram_base + ram_size);
        *(--p) = 0;
        *(--p) = 0;
        env->gpr[1] = 0x10000000 + ram_size - 2 * sizeof(target_ulong);
        /* Enable FPU.  */
        env->msr |= (1 << MSR_FP);
    }
}

QEMUMachine dummy_ppc_machine = {
    .name = "dummy",
    .desc = "Dummy board",
    .init = dummy_ppc_init,
    .max_cpus = 1
};
