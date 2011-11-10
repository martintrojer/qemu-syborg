//
// Copyright (c) 2008 Symbian Ltd. All rights reserved.
//

#ifndef _SYBORG_H
#define _SYBORG_H

enum TSyborgInterruptId
{
  EIntTimer0 = 0,
  EIntTimer1 = 1,
  EIntKeyboard = 2,
  EIntMouse = 3,
  EIntFb = 4,
  EIntSerial0 = 5,
  EIntSerial1 = 6,
  EIntSerial2 = 7,
  EIntSerial3 = 8
};

void syborg_dummy_init(uint32_t base, char *tag);
void syborg_fb_init(DisplayState *ds, uint32_t base, qemu_irq irq, int old);
qemu_irq *syborg_interrupt_init(uint32_t base, qemu_irq parent_irq, int n);
void syborg_keyboard_init(uint32_t base, qemu_irq irq);
void syborg_touchscreen_init(uint32_t base, qemu_irq irq);
void syborg_mouse_init(uint32_t base, qemu_irq irq);
void syborg_serial_init(uint32_t base, qemu_irq irq, CharDriverState *chr);
void syborg_timer_init(uint32_t base, qemu_irq irq, uint32_t freq);
void syborg_rtc_init(uint32_t base);
/*FIXME: obsolete.  */
void syborg_oldtimer_init(uint32_t base, qemu_irq irq, uint32_t freq);
qemu_irq *syborg_old_interrupt_init(uint32_t base, qemu_irq parent_irq);
void syborg_old_keyboard_init(uint32_t base, qemu_irq irq);
void syborg_old_mouse_init(uint32_t base, qemu_irq irq);
void syborg_old_serial_init(uint32_t base, qemu_irq irq, CharDriverState *chr);
void syborg_old_timer_init(uint32_t base, qemu_irq irq, uint32_t freq);

#define SYBORG_ID_PLATFORM    0xc51d1000
#define SYBORG_ID_INT         0xc51d0000
#define SYBORG_ID_SERIAL      0xc51d0001
#define SYBORG_ID_KEYBOARD    0xc51d0002
#define SYBORG_ID_TIMER       0xc51d0003
#define SYBORG_ID_RTC         0xc51d0004
#define SYBORG_ID_MOUSE       0xc51d0005
#define SYBORG_ID_TOUCHSCREEN 0xc51d0006
#define SYBORG_ID_FRAMEBUFFER 0xc51d0007
#define SYBORG_ID_HOSTFS      0xc51d0008
#define SYBORG_ID_SNAPSHOT    0xc51d0009
#define SYBORG_ID_VIRTIO      0xc51d000a
#define SYBORG_ID_NAND        0xc51d000b

#endif
