/*
 * GUI Host side interface
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

#ifndef GUI_HOST_H
#define GUI_HOST_H

#define SKINNED_VT_PRIORITY_ORDER           10
#define SKINLESS_GRAPHIC_VT_PRIORITY_ORDER  20
#define SKINLESS_TEXT_VT_PRIORITY_ORDER     30

typedef unsigned int DisplayID;
typedef unsigned int VtID;
typedef unsigned int ImageID;
typedef unsigned int AreaID;


/* DFG FIXME: move the DisplayState definition to gui.c (as internal implementation) */

struct DisplayState {
    /* FIXME: Move away this from here (now in screen_data_t) */
    uint8_t *data;
    int linesize;
    int depth;
    int bgr; /* BGR color order instead of RGB. Only valid for depth == 32 */
    /*******************/
    int x0;
    int y0;
    int width;
    int height;
    VtID vtid;
    DisplayID dispid;
    void *opaque;

    void (*dpy_update)(struct DisplayState *s, int x, int y, int w, int h);
    void (*dpy_copy)(struct DisplayState *s, int src_x, int src_y,
                     int dst_x, int dst_y, int w, int h);
    void (*dpy_fill)(struct DisplayState *s, int x, int y,
                     int w, int h, uint32_t c);
    void (*dpy_text_cursor)(struct DisplayState *s, int x, int y);
    void (*mouse_set)(int x, int y, int on);
    void (*cursor_define)(int width, int height, int bpp, int hot_x, int hot_y,
                          uint8_t *image, uint8_t *mask);
};

/****************************************************/

/* in ms */
#define GUI_REFRESH_INTERVAL 30

/************ HOST SIDE INTERFACE ************/

void kbd_mouse_event(int dx, int dy, int dz, int buttons_state);
int kbd_mouse_is_absolute(void);
void do_info_mice(void);
void do_mouse_set(int index);

/* to be used by sdl.c or vnc.c */

typedef enum
{
    GUI_CURSOR_NORMAL,
    GUI_CURSOR_GRABBING,
    GUI_CURSOR_GUEST_SPRITE,
    GUI_CURSOR_HIDDEN, /* What is this for? */
} gui_cursor_type_t;

typedef struct {
    /* this was in DisplayState */
    uint8_t *data;
    int linesize;
    int depth;
    int bgr; /* BGR color order instead of RGB. Only valid for depth == 32 */
    int height;
    int width;
} screen_data_t;

typedef struct
{
    void (*turn_cursor_on)(gui_cursor_type_t);
    void (*turn_cursor_off)(void);
    void (*grab_input_on)(void);
    void (*grab_input_off)(void);
    void (*mouse_warp)(int x, int y);
    void (*set_caption)(const char* title, const char* icon);
    int  (*is_app_active)(void);
    void (*set_screen_size)(int width, int height, int on);
    void (*get_screen_data)(screen_data_t *new_screen_data);
    void (*init_ds)(struct DisplayState *ds);
    void (*process_events)(void);
    void (*set_kbd_terminal_mode)(int on);
}gui_host_callbacks_t;

void gui_init(gui_host_callbacks_t* callbacks);
int gui_load(const char *xml_file);
void gui_unload(void);
DisplayState *gui_new_vt(int order_priority);
int gui_needs_timer(void);
void gui_set_timer(struct QEMUTimer *timer);
void gui_refresh_caption(void);
void gui_destroy(void);
void gui_set_paint_callbacks(DisplayState *ds,
                             vga_hw_update_ptr update,
                             vga_hw_invalidate_ptr invalidate,
                             vga_hw_screen_dump_ptr screen_dump,
                             void *opaque);
void gui_show_image(VtID vtid, ImageID id);
void gui_hide_image(VtID vtid, ImageID id);

/* Queries */
int gui_allows_fullscreen(void); /* returns 0 or 1 */
int gui_is_display_active(DisplayState *ds);

/* Events */
void gui_notify_toggle_fullscreen(void);
void gui_notify_toggle_grabmode(void);
void gui_notify_mouse_motion(int dx, int dy, int dz, int x, int y, int state);
void gui_notify_mouse_button(int dz, int x, int y, int state);
void gui_notify_mouse_warp(int x, int y, int on);
void gui_notify_dev_key(int key);
void gui_notify_term_key(int key);
void gui_notify_console_select(int console); /* console is not the VtID, but the ordinal # */
void gui_notify_activate_display(DisplayState *ds);
void gui_notify_new_guest_cursor(void);
void gui_notify_input_focus_lost(void);
void gui_notify_app_focus(int gain);
void gui_notify_idle(int idle);
void gui_notify_update_tick(int64_t ticks);
void gui_notify_repaint_screen(void);
void gui_notify_screen_dump(const char *filename);

#define DEF_BACKGROUND_IMAGE (ImageID)0  /* Default Background is always 0 */

#endif

