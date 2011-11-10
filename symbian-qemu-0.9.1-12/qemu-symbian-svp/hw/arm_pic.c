/*
 * Generic ARM Programmable Interrupt Controller support.
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licenced under the LGPL
 */

#include "hw.h"
#include "arm-misc.h"
#include "devtree.h"

/* Stub functions for hardware that doesn't exist.  */
void pic_info(void)
{
}

void irq_info(void)
{
}


/* Input 0 is IRQ and input 1 is FIQ.  */
static void arm_pic_cpu_handler(void *opaque, int irq, int level)
{
    CPUState *env = (CPUState *)opaque;
    switch (irq) {
    case ARM_PIC_CPU_IRQ:
        if (level)
            cpu_interrupt(env, CPU_INTERRUPT_HARD);
        else
            cpu_reset_interrupt(env, CPU_INTERRUPT_HARD);
        break;
    case ARM_PIC_CPU_FIQ:
        if (level)
            cpu_interrupt(env, CPU_INTERRUPT_FIQ);
        else
            cpu_reset_interrupt(env, CPU_INTERRUPT_FIQ);
        break;
    default:
        cpu_abort(env, "arm_pic_cpu_handler: Bad interrput line %d\n", irq);
    }
}

/* FIXME: This is obsolete.  */
qemu_irq *arm_pic_init_cpu(CPUState *env)
{
    return qemu_allocate_irqs(arm_pic_cpu_handler, env, 2);
}

/* Override default CPU settings with user supplied values.  */
void arm_cpu_reset_dev(CPUState *env)
{
    QEMUDevice *dev = env->qdev;
    char buf[64];
    uint32_t v;
    int i;

    if (!dev)
        return;

    v = qdev_get_property_int(dev, "cp15,ctr");
    if (v != 0xffffffffu)
        env->cp15.c0_cachetype = v;
    v = qdev_get_property_int(dev, "cp15,clid");
    if (v != 0xffffffffu)
        env->cp15.c0_clid = v;
    for (i = 0; i < 7; i++) {
        sprintf(buf, "cp15,ccsid%d", i);
        v = qdev_get_property_int(dev, buf);
        if (v != 0xffffffffu)
            env->cp15.c0_ccsid[i << 1] = v;
        sprintf(buf, "cp15,ccsid%di", i);
        v = qdev_get_property_int(dev, buf);
        if (v != 0xffffffffu)
            env->cp15.c0_ccsid[(i << 1) + 1] = v;
    }

}

static void arm_cpu_create(QEMUDevice *dev)
{
    CPUState *env;
    const char *name;
    char model[64];
    int i;

    name = qdev_get_name(dev);
    if (strncmp(name, "ARM,", 4) == 0)
        name += 4;

    i = 0;
    while (name[i] && name[i] != '@' && i < 63) {
        model[i] = tolower(name[i]);
        i++;
    }
    model[i] = 0;

    env = cpu_arm_init(model, dev);

    if (!env) {
        fprintf(stderr, "Unable to find CPU definition %s\n", model);
        exit(1);
    }

    qdev_create_interrupts(dev, arm_pic_cpu_handler, env, 2);
}

void cpu_device_register(void)
{
    int i;
    char buf[13];

    cpu_device_class = qdev_new("cpu", arm_cpu_create, 0);
    qdev_add_property_int(cpu_device_class, "cp15,ctr", 0xffffffffu);
    qdev_add_property_int(cpu_device_class, "cp15,clid", 0xffffffffu);
    for (i = 0; i < 7; i++) {
        sprintf(buf, "cp15,ccsid%d", i);
        qdev_add_property_int(cpu_device_class, buf, 0xffffffffu);
        sprintf(buf, "cp15,ccsid%di", i);
        qdev_add_property_int(cpu_device_class, buf, 0xffffffffu);
    }
}
