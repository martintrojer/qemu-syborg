/*
 * QEMU SDL display driver
 *
 * Copyright (c) 2003 Fabrice Bellard
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
#include "qemu-common.h"
#include "console.h"
#include "sysemu.h"
#include "gui_host.h"

#include <SDL.h>

#ifndef _WIN32
#include <signal.h>
#endif

static SDL_Surface *screen;
static int gui_noframe;
static int gui_key_modifier_pressed;
static int special_key_combination;
static int kbd_in_terminal_mode;
static int gui_grab_code = KMOD_LALT | KMOD_LCTRL;
static uint8_t modifiers_state[256];
static int width, height;
static SDL_Cursor *sdl_cursor_normal;
static SDL_Cursor *sdl_cursor_hidden;
static SDL_Cursor *guest_sprite = 0;

static void sdl_update(DisplayState *ds, int x, int y, int w, int h)
{
    //    printf("updating x=%d y=%d w=%d h=%d\n", x, y, w, h);
    SDL_UpdateRect(screen, ds->x0 + x, ds->y0 + y, w, h);
}

static void sdl_gui_set_screen_size(int w, int h, int fullscreen_on)
{
    int flags;

    //    printf("resizing to %d %d\n", w, h);

    flags = SDL_HWSURFACE|SDL_ASYNCBLIT|SDL_HWACCEL;
    if (fullscreen_on)
        flags |= SDL_FULLSCREEN;
    if (gui_noframe)
        flags |= SDL_NOFRAME;

    width = w;
    height = h;

 again:
    screen = SDL_SetVideoMode(width, height, 0, flags);
    if (!screen) {
        fprintf(stderr, "Could not open SDL display\n");
        exit(1);
    }
    if (!screen->pixels && (flags & SDL_HWSURFACE) && (flags & SDL_FULLSCREEN)) {
        flags &= ~SDL_HWSURFACE;
        goto again;
    }

    if (!screen->pixels) {
        fprintf(stderr, "Could not open SDL display\n");
        exit(1);
    }
}

static void sdl_gui_get_screen_data(screen_data_t *new_screen_data)
{
    new_screen_data->data = screen->pixels;
    new_screen_data->linesize = screen->pitch;
    new_screen_data->width = screen->w;
    new_screen_data->height = screen->h;
    new_screen_data->depth = screen->format->BitsPerPixel;
    /* SDL BitsPerPixel never indicates any values other than
       multiples of 8, so we need to check for strange depths. */
    if (new_screen_data->depth == 16) {
        uint32_t mask;

        mask = screen->format->Rmask;
        mask |= screen->format->Gmask;
        mask |= screen->format->Bmask;
        if ((mask & 0x8000) == 0)
            new_screen_data->depth = 15;
    }
    if (new_screen_data->depth == 32 && screen->format->Rshift == 0) {
        new_screen_data->bgr = 1;
    } else {
        new_screen_data->bgr = 0;
    }
}


/* generic keyboard conversion */

#include "sdl_keysym.h"
#include "keymaps.c"

static kbd_layout_t *kbd_layout = NULL;

static uint8_t sdl_keyevent_to_keycode_generic(const SDL_KeyboardEvent *ev)
{
    int keysym;
    /* workaround for X11+SDL bug with AltGR */
    keysym = ev->keysym.sym;
    if (keysym == 0 && ev->keysym.scancode == 113)
        keysym = SDLK_MODE;
    /* For Japanese key '\' and '|' */
    if (keysym == 92 && ev->keysym.scancode == 133) {
        keysym = 0xa5;
    }
    return keysym2scancode(kbd_layout, keysym);
}

/* specific keyboard conversions from scan codes */

#if defined(_WIN32)

static uint8_t sdl_keyevent_to_keycode(const SDL_KeyboardEvent *ev)
{
    return ev->keysym.scancode;
}

#else

static uint8_t sdl_keyevent_to_keycode(const SDL_KeyboardEvent *ev)
{
    int keycode;

    keycode = ev->keysym.scancode;

    if (keycode < 9) {
        keycode = 0;
    } else if (keycode < 97) {
        keycode -= 8; /* just an offset */
    } else if (keycode < 212) {
        /* use conversion table */
        keycode = _translate_keycode(keycode - 97);
    } else {
        keycode = 0;
    }
    return keycode;
}

#endif

static void reset_keys(void)
{
    int i;
    for(i = 0; i < 256; i++) {
        if (modifiers_state[i]) {
            if (i & 0x80)
                gui_notify_dev_key(0xe0);
            gui_notify_dev_key(i | 0x80);
            modifiers_state[i] = 0;
        }
    }
}

static void sdl_process_key(SDL_KeyboardEvent *ev)
{
    int keycode, v;

    if (ev->keysym.sym == SDLK_PAUSE) {
        /* specific case */
        v = 0;
        if (ev->type == SDL_KEYUP)
            v |= 0x80;
        gui_notify_dev_key(0xe1);
        gui_notify_dev_key(0x1d | v);
        gui_notify_dev_key(0x45 | v);
        return;
    }

    if (kbd_layout) {
        keycode = sdl_keyevent_to_keycode_generic(ev);
    } else {
        keycode = sdl_keyevent_to_keycode(ev);
    }

    switch(keycode) {
    case 0x00:
        /* sent when leaving window: reset the modifiers state */
        reset_keys();
        return;
    case 0x2a:                          /* Left Shift */
    case 0x36:                          /* Right Shift */
    case 0x1d:                          /* Left CTRL */
    case 0x9d:                          /* Right CTRL */
    case 0x38:                          /* Left ALT */
    case 0xb8:                         /* Right ALT */
        if (ev->type == SDL_KEYUP)
            modifiers_state[keycode] = 0;
        else
            modifiers_state[keycode] = 1;
        break;
    case 0x45: /* num lock */
    case 0x3a: /* caps lock */
        /* SDL does not send the key up event, so we generate it */
        gui_notify_dev_key(keycode);
        gui_notify_dev_key(keycode | 0x80);
        return;
    }

    /* now send the key code */
    if (keycode & 0x80)
        gui_notify_dev_key(0xe0);
    if (ev->type == SDL_KEYUP)
        gui_notify_dev_key(keycode | 0x80);
    else
        gui_notify_dev_key(keycode & 0x7f);
}

static int sdl_state_to_qemu(int sdl_state)
{
    int buttons = 0;
    if (sdl_state & SDL_BUTTON(SDL_BUTTON_LEFT))
        buttons |= MOUSE_EVENT_LBUTTON;
    if (sdl_state & SDL_BUTTON(SDL_BUTTON_RIGHT))
        buttons |= MOUSE_EVENT_RBUTTON;
    if (sdl_state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
        buttons |= MOUSE_EVENT_MBUTTON;

    return buttons;
}

static void sdl_gui_process_events(void)
{
    SDL_Event ev1, *ev = &ev1;
    int mod_state;
    int buttonstate = SDL_GetMouseState(NULL, NULL);

    gui_refresh_caption();

    /*vga_hw_update();*/
    SDL_EnableUNICODE(kbd_in_terminal_mode);

    while (SDL_PollEvent(ev)) {
        switch (ev->type) {
        case SDL_VIDEOEXPOSE:
            gui_notify_repaint_screen();
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (ev->type == SDL_KEYDOWN) {
                if (!alt_grab) {
                    mod_state = (SDL_GetModState() & gui_grab_code) ==
                                gui_grab_code;
                } else {
                    mod_state = (SDL_GetModState() & (gui_grab_code | KMOD_LSHIFT)) ==
                                (gui_grab_code | KMOD_LSHIFT);
                }
                gui_key_modifier_pressed = mod_state;
                if (gui_key_modifier_pressed) {
                    int keycode;
                    keycode = sdl_keyevent_to_keycode(&ev->key);
                    switch(keycode) {
                    case 0x21: /* 'f' key on US keyboard */
                        if (gui_allows_fullscreen()) {
                            gui_notify_toggle_fullscreen();
                            special_key_combination = 1;
                        }
                        break;
                    case 0x02 ... 0x0a: /* '1' to '9' keys */
                        /* Reset the modifiers sent to the current console */
                        reset_keys();
                        gui_notify_console_select(keycode - 0x02);
                        special_key_combination = 1;
                        break;
                    default:
                        break;
                    }
                } else if (kbd_in_terminal_mode) {
                    int keysym;
                    keysym = 0;
                    if (ev->key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
                        switch(ev->key.keysym.sym) {
                        case SDLK_UP: keysym = QEMU_KEY_CTRL_UP; break;
                        case SDLK_DOWN: keysym = QEMU_KEY_CTRL_DOWN; break;
                        case SDLK_LEFT: keysym = QEMU_KEY_CTRL_LEFT; break;
                        case SDLK_RIGHT: keysym = QEMU_KEY_CTRL_RIGHT; break;
                        case SDLK_HOME: keysym = QEMU_KEY_CTRL_HOME; break;
                        case SDLK_END: keysym = QEMU_KEY_CTRL_END; break;
                        case SDLK_PAGEUP: keysym = QEMU_KEY_CTRL_PAGEUP; break;
                        case SDLK_PAGEDOWN: keysym = QEMU_KEY_CTRL_PAGEDOWN; break;
                        default: break;
                        }
                    } else {
                        switch(ev->key.keysym.sym) {
                        case SDLK_UP: keysym = QEMU_KEY_UP; break;
                        case SDLK_DOWN: keysym = QEMU_KEY_DOWN; break;
                        case SDLK_LEFT: keysym = QEMU_KEY_LEFT; break;
                        case SDLK_RIGHT: keysym = QEMU_KEY_RIGHT; break;
                        case SDLK_HOME: keysym = QEMU_KEY_HOME; break;
                        case SDLK_END: keysym = QEMU_KEY_END; break;
                        case SDLK_PAGEUP: keysym = QEMU_KEY_PAGEUP; break;
                        case SDLK_PAGEDOWN: keysym = QEMU_KEY_PAGEDOWN; break;
                        case SDLK_BACKSPACE: keysym = QEMU_KEY_BACKSPACE; break;
                        case SDLK_DELETE: keysym = QEMU_KEY_DELETE; break;
                        default: break;
                        }
                    }
                    if (keysym) {
                        gui_notify_term_key(keysym);
                    } else if (ev->key.keysym.unicode != 0) {
                        gui_notify_term_key(ev->key.keysym.unicode);
                    }
                }
            } else if (ev->type == SDL_KEYUP) {
                if (!alt_grab) {
                    mod_state = (ev->key.keysym.mod & gui_grab_code);
                } else {
                    mod_state = (ev->key.keysym.mod &
                                 (gui_grab_code | KMOD_LSHIFT));
                }
                if (!mod_state) {
                    if (gui_key_modifier_pressed) {
                        gui_key_modifier_pressed = 0;
                        if (!special_key_combination) {
                            /* exit/enter grab if pressing Ctrl-Alt */
                            gui_notify_toggle_grabmode();
                            /* SDL does not send back all the
                               modifiers key, so we must correct it */
                            reset_keys();
                            break;
                        }
                        special_key_combination = 0;
                    }
                }
            }
            if (!kbd_in_terminal_mode && !special_key_combination)
                sdl_process_key(&ev->key);
            break;
        case SDL_QUIT:
            if (!no_quit)
                qemu_system_shutdown_request();
            break;
        case SDL_MOUSEMOTION:
            gui_notify_mouse_motion(ev->motion.xrel, ev->motion.yrel, 0,
                                    ev->motion.x, ev->motion.y, 
                                    sdl_state_to_qemu(ev->motion.state));
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            {
                SDL_MouseButtonEvent *bev = &ev->button;
                int dz = 0;

                if (ev->type == SDL_MOUSEBUTTONDOWN) {
                    buttonstate |= SDL_BUTTON(bev->button);
                } else {
                    buttonstate &= ~SDL_BUTTON(bev->button);
                }
#ifdef SDL_BUTTON_WHEELUP
                if (bev->button == SDL_BUTTON_WHEELUP && ev->type == SDL_MOUSEBUTTONDOWN) {
                    dz = -1;
                } else if (bev->button == SDL_BUTTON_WHEELDOWN && ev->type == SDL_MOUSEBUTTONDOWN) {
                    dz = 1;
                }
#endif
                gui_notify_mouse_button(dz, bev->x, bev->y, sdl_state_to_qemu(buttonstate));
            }
            break;
        case SDL_ACTIVEEVENT:
            if (ev->active.state == SDL_APPINPUTFOCUS && !ev->active.gain) {
                gui_notify_input_focus_lost();
            } else if (ev->active.state & SDL_APPACTIVE) {
                gui_notify_app_focus(ev->active.gain);
            }
            break;
        default:
            break;
        }
    }
}

static void sdl_gui_set_kbd_terminal_mode(int on)
{
    kbd_in_terminal_mode = on;
}

static void sdl_fill(DisplayState *ds, int x, int y, int w, int h, uint32_t c)
{
    SDL_Rect dst = { x, y, w, h };
    SDL_FillRect(screen, &dst, c);
}

static void sdl_mouse_warp(int x, int y, int on)
{
    gui_notify_mouse_warp(x, y, on);
}

static void sdl_mouse_define(int width, int height, int bpp,
                             int hot_x, int hot_y,
                             uint8_t *image, uint8_t *mask)
{
    uint8_t sprite[256], *line;
    int x, y, dst, bypl, src = 0;
    if (guest_sprite)
        SDL_FreeCursor(guest_sprite);

    memset(sprite, 0, 256);
    bypl = ((width * bpp + 31) >> 5) << 2;
    for (y = 0, dst = 0; y < height; y ++, image += bypl) {
        line = image;
        for (x = 0; x < width; x ++, dst ++) {
            switch (bpp) {
            case 24:
                src = *(line ++); src |= *(line ++); src |= *(line ++);
                break;
            case 16:
            case 15:
                src = *(line ++); src |= *(line ++);
                break;
            case 8:
                src = *(line ++);
                break;
            case 4:
                src = 0xf & (line[x >> 1] >> ((x & 1)) << 2);
                break;
            case 2:
                src = 3 & (line[x >> 2] >> ((x & 3)) << 1);
                break;
            case 1:
                src = 1 & (line[x >> 3] >> (x & 7));
                break;
            }
            if (!src)
                sprite[dst >> 3] |= (1 << (~dst & 7)) & mask[dst >> 3];
        }
    }
    guest_sprite = SDL_CreateCursor(sprite, mask, width, height, hot_x, hot_y);

    gui_notify_new_guest_cursor();
}

static void sdl_cleanup(void)
{
    if (guest_sprite)
        SDL_FreeCursor(guest_sprite);
    SDL_Quit();
}

/******** GUI CALLBACKS **********/
static void sdl_gui_turn_cursor_on(gui_cursor_type_t cursor_type)
{
    SDL_ShowCursor(1);

    switch(cursor_type) {
    case GUI_CURSOR_NORMAL:
        SDL_SetCursor(sdl_cursor_normal);
        break;
    case GUI_CURSOR_GUEST_SPRITE:
        SDL_SetCursor(guest_sprite);
        break;
    case GUI_CURSOR_HIDDEN:
        SDL_SetCursor(sdl_cursor_hidden);
        break;
    case GUI_CURSOR_GRABBING:
        if (guest_sprite)
            SDL_SetCursor(guest_sprite);
        else
            SDL_SetCursor(sdl_cursor_normal);
        break;
    }
}

static void sdl_gui_turn_cursor_off(void)
{
    SDL_ShowCursor(0);
}

static void sdl_gui_mouse_warp(int x, int y)
{
    SDL_WarpMouse(x, y);
}

static void sdl_gui_grab_input_on(void)
{
    SDL_WM_GrabInput(SDL_GRAB_ON);
}

static void sdl_gui_grab_input_off(void)
{
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}

static void sdl_gui_set_caption(const char* title, const char* icon)
{
    SDL_WM_SetCaption(title, icon);
}

static int sdl_gui_is_app_active(void)
{
    return (SDL_GetAppState() & SDL_APPACTIVE);
}

static void sdl_init_ds(DisplayState *ds)
{
    ds->dpy_update = sdl_update;
    ds->dpy_fill = sdl_fill;
    ds->mouse_set = sdl_mouse_warp;
    ds->cursor_define = sdl_mouse_define;
}

/*********************************/


void sdl_display_init(int full_screen, int no_frame)
{
    int flags;
    uint8_t data = 0;
    gui_host_callbacks_t gui_callbacks;

#if defined(__APPLE__)
    /* always use generic keymaps */
    if (!keyboard_layout)
        keyboard_layout = "en-us";
#endif
    if(keyboard_layout) {
        kbd_layout = init_keyboard_layout(keyboard_layout);
        if (!kbd_layout)
            exit(1);
    }

    if (no_frame)
        gui_noframe = 1;

    flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
    if (SDL_Init (flags)) {
        fprintf(stderr, "Could not initialize SDL - exiting\n");
        exit(1);
    }

    memset(&gui_callbacks, 0, sizeof(gui_host_callbacks_t));
    gui_callbacks.turn_cursor_on = &sdl_gui_turn_cursor_on;
    gui_callbacks.turn_cursor_off = &sdl_gui_turn_cursor_off;
    gui_callbacks.mouse_warp = &sdl_gui_mouse_warp;
    gui_callbacks.grab_input_on = &sdl_gui_grab_input_on;
    gui_callbacks.grab_input_off = &sdl_gui_grab_input_off;
    gui_callbacks.set_caption = &sdl_gui_set_caption;
    gui_callbacks.set_screen_size = &sdl_gui_set_screen_size;
    gui_callbacks.get_screen_data = &sdl_gui_get_screen_data;
    gui_callbacks.is_app_active = &sdl_gui_is_app_active;
    gui_callbacks.init_ds = &sdl_init_ds;
    gui_callbacks.process_events = &sdl_gui_process_events;
    gui_callbacks.set_kbd_terminal_mode = &sdl_gui_set_kbd_terminal_mode;
    gui_init(&gui_callbacks);

    if (full_screen && gui_allows_fullscreen()) {
        gui_notify_toggle_fullscreen();
    }

    SDL_EnableKeyRepeat(250, 50);

    sdl_cursor_hidden = SDL_CreateCursor(&data, &data, 8, 1, 0, 0);
    sdl_cursor_normal = SDL_GetCursor();

    atexit(sdl_cleanup);

}

