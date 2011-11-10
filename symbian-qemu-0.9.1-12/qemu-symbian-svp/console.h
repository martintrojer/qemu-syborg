#ifndef CONSOLE_H
#define CONSOLE_H

#include "qemu-char.h"
#include "hw/gui.h"

/* keysym is a unicode code except for special keys (see QEMU_KEY_xxx
   constants) */
#define QEMU_KEY_ESC1(c) ((c) | 0xe100)
#define QEMU_KEY_BACKSPACE  0x007f
#define QEMU_KEY_UP         QEMU_KEY_ESC1('A')
#define QEMU_KEY_DOWN       QEMU_KEY_ESC1('B')
#define QEMU_KEY_RIGHT      QEMU_KEY_ESC1('C')
#define QEMU_KEY_LEFT       QEMU_KEY_ESC1('D')
#define QEMU_KEY_HOME       QEMU_KEY_ESC1(1)
#define QEMU_KEY_END        QEMU_KEY_ESC1(4)
#define QEMU_KEY_PAGEUP     QEMU_KEY_ESC1(5)
#define QEMU_KEY_PAGEDOWN   QEMU_KEY_ESC1(6)
#define QEMU_KEY_DELETE     QEMU_KEY_ESC1(3)

#define QEMU_KEY_CTRL_UP         0xe400
#define QEMU_KEY_CTRL_DOWN       0xe401
#define QEMU_KEY_CTRL_LEFT       0xe402
#define QEMU_KEY_CTRL_RIGHT      0xe403
#define QEMU_KEY_CTRL_HOME       0xe404
#define QEMU_KEY_CTRL_END        0xe405
#define QEMU_KEY_CTRL_PAGEUP     0xe406
#define QEMU_KEY_CTRL_PAGEDOWN   0xe407

/* consoles */

typedef unsigned long console_ch_t;
static inline void console_write_ch(console_ch_t *dest, uint32_t ch)
{
    cpu_to_le32wu((uint32_t *) dest, ch);
}

typedef void (*vga_hw_text_update_ptr)(void *, console_ch_t *);

int is_fixed_size_console(const TextConsole *console);

CharDriverState *text_console_init(DisplayState *ds, const char *p);
CharDriverState *new_text_console(const char *p);

void console_color_init(DisplayState *ds);
void text_console_copy(TextConsole *console, int src_x, int src_y,
                int dst_x, int dst_y, int w, int h);

void text_console_resize(TextConsole *console, int width, int height);

/* sdl.c */
void sdl_display_init(int full_screen, int no_frame);

/* cocoa.m */
void cocoa_display_init(DisplayState *ds, int full_screen);

/* vnc.c */
void vnc_display_init(DisplayState *ds);
void vnc_display_close(DisplayState *ds);
int vnc_display_open(DisplayState *ds, const char *display);
int vnc_display_password(DisplayState *ds, const char *password);
void do_info_vnc(void);

/* curses.c */
void curses_display_init(DisplayState *ds, int full_screen);

/* x_keymap.c */
extern uint8_t _translate_keycode(const int key);

/* FIXME: term_printf et al should probably go elsewhere so everything
   does not need to include console.h  */
/* monitor.c */
void monitor_init(CharDriverState *hd, int show_banner);
void term_puts(const char *str);
void term_vprintf(const char *fmt, va_list ap);
void term_printf(const char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void term_print_filename(const char *filename);
void term_flush(void);
void term_print_help(void);
void monitor_readline(const char *prompt, int is_password,
                      char *buf, int buf_size);
void monitor_suspend(void);
void monitor_resume(void);

/* readline.c */
typedef void ReadLineFunc(void *opaque, const char *str);

extern int completion_index;
void add_completion(const char *str);
void readline_handle_byte(int ch);
void readline_find_completion(const char *cmdline);
const char *readline_get_history(unsigned int index);
void readline_start(const char *prompt, int is_password,
                    ReadLineFunc *readline_func, void *opaque);

#endif
