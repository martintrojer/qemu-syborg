/*
 * GUI implementation
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
 *
 * Contributors:
 * NTT DOCOMO, INC. -- Clicking a QEMU skin button causes screen flicker on the display area
 *
 */

#include "hw/hw.h"
#include "sysemu.h"
#include "stdio.h"
#include "qemu-timer.h"
#include "hw/gui.h"
#include "gui_host.h"
#include "gui_common.h"
#include "gui_parser.h"
#define HOST_ONLY_DEFS /* DFG: FIXME */
#include "hw/fb_render_engine.h"
#include "gui_png.h"

#define MAX_CONSOLES 12

typedef void (*button_action_t)(void *parameter);

struct button_actions_s
{
    button_action_t down;
    button_action_t up;
};

enum button_actions_e
{
    BUTTON_ACTION_NOACTION = 0,
    BUTTON_ACTION_SENDKEY = 1,
};

typedef enum
{
    CA_POINTERAREA,
    CA_BUTTON
} clickarea_type;

typedef struct
{
    devid_t devid;
    int absolute;
    int grab_on_click;
    int show_cursor_while_grabbing;
    QEMUPutMouseEvent *callback;
    void *opaque;
} pointer_area_t;

typedef struct
{
    ImageID pressed_img_id;
    enum button_actions_e actions;
    void* parameter;
} button_t;

typedef struct
{
    gui_area_t area;
    clickarea_type type;
    int id;
} clickable_area_t;

typedef struct
{
    /* TODO: Optimize with a geometrical index */
    clickable_area_t *areas;
    unsigned int n_areas;
    clickable_area_t *currently_grabbing; /* NULL if none */
} clickable_map_t;

typedef struct
{
    devid_t devid;
    DisplayState ds;
    vga_hw_update_ptr update;
    vga_hw_invalidate_ptr invalidate;
    vga_hw_screen_dump_ptr screen_dump;
    /*vga_hw_text_update_ptr text_update;*/
    void *opaque;
} ds_data_t;

typedef struct
{
    ds_data_t *ds_data;
    unsigned int n_ds;

    int order_priority; /* This is used to place this VT in the key sequence (i.e. skinned VTs first) */

    pointer_area_t *pointerareas;
    unsigned int n_pointerareas;

    KeyCallback key_callback;
    void *key_opaque;

    /* skin data */
    int has_skin;

    /* If has skin, the following data applies.
       Maybe all should be moved to skin_data_t.
    */
    DisplayState gui_ds; /* just for skin and host images */


    void *background_buffer;
    unsigned int background_row_size;
    render_data *rdata;
    int full_update;

    gui_image_t *images;
    unsigned int n_images;
    clickable_map_t clickable_map;
    button_t *buttons;
    unsigned int n_buttons;

} vt_t;

enum gui_input_state_t
{
    INPUT_STATE_GUI_UNLOADED, /* Compatible with old (gui-less) machines */
    INPUT_STATE_GUI_LOADED
};

typedef struct
{
    void (*gui_notify_toggle_fullscreen)(void);
    void (*gui_notify_toggle_grabmode)(void);
    void (*gui_notify_mouse_motion)(int dx, int dy, int dz, int x, int y, int state);
    void (*gui_notify_mouse_button)(int dz, int x, int y, int state);
    void (*gui_notify_mouse_warp)(int x, int y, int on);
    void (*gui_notify_new_guest_cursor)(void);
    void (*gui_notify_input_focus_lost)(void);
    void (*gui_notify_app_focus)(int gain);
} gui_input_table_t;

struct gui_data_t
{
    gui_host_callbacks_t host_callbacks;
    vt_t vts[MAX_CONSOLES];
    unsigned int n_vts;
    vt_t *current_vt;
    VtID ordered_vts[MAX_CONSOLES]; /* Vts sorted by order_priority */
    screen_data_t screen_data;

    struct QEMUTimer *gui_timer;
    uint64_t gui_timer_interval;
    int idle; /* there is nothing to update (window invisible), set by vnc/sdl */

    int updating; /* to prevent recursion */

    /* Input stuff */
    KeyCallback dev_key_callback;
    void *dev_key_opaque;

    int gui_fullscreen;

    const gui_input_table_t* input_table;
    int guest_cursor;
    int guest_x;
    int guest_y;
    int last_vm_running;
    struct {
        int absolute_enabled;
        int gui_grab; /* if true, all keyboard/mouse events are grabbed */
        int gui_saved_grab;
        int gui_fullscreen_initial_grab;
    } unloaded_state_data;

    int loaded;
};

static struct gui_data_t* gui_data = NULL;
/*******************************************/


/* Internal Functions ==================== */
static parse_result_t load_gui_xml(const char* xml_file);
static void index_map(clickable_map_t *map);
static void initialize_display_areas(VtID vtid);
static void destroy_clickable_map(clickable_map_t *map);
static void destroy_images(VtID vtid);
static void gui_update_guest_display(vt_t *vt);
static void init_background(VtID vtid);
static void gui_set_input_state(enum gui_input_state_t input_state);
static void gui_update_caption(void);
static void gui_update_ds_data(DisplayState *ds, int in_skinned_vt);

static void gui_loaded_grab_end(void);
static void gui_update_timer(int64_t ticks);

/* ======================================= */

int ds_get_width(const DisplayState *ds)
{
    return ds->width;
}

int ds_get_height(const DisplayState *ds)
{
    return ds->height;
}

int ds_get_bits_per_pixel(const DisplayState *ds)
{
    return ds->depth;
}

int ds_get_bgr(const DisplayState *ds)
{
    return ds->bgr;
}

int ds_get_linesize(const DisplayState *ds)
{
    return ds->linesize;
}

uint8_t *ds_get_data(const DisplayState *ds)
{
    return ds->data;
}

static inline int vt_enabled(void)
{
    return gui_data->current_vt != NULL;
}

static inline VtID current_vt_id(void)
{
    return gui_data->current_vt - &gui_data->vts[0];
}

static inline int is_current_vt(VtID vtid)
{
    return gui_data->current_vt == &gui_data->vts[vtid];
}

static inline void invalidate_ds(const ds_data_t* ds_data)
{
    if (ds_data->invalidate != NULL)
        ds_data->invalidate(ds_data->opaque);
}

static inline void update_ds(const ds_data_t* ds_data)
{
    if (ds_data->update != NULL)
        ds_data->update(ds_data->opaque);
}

static inline int get_vt_width(const vt_t *vt)
{
    if (vt->has_skin)
        return GET_GUI_AREA_WIDTH(&vt->images[DEF_BACKGROUND_IMAGE].area);
    else
        return vt->ds_data[0].ds.width;
}

static inline int get_vt_height(const vt_t *vt)
{
    if (vt->has_skin)
        return GET_GUI_AREA_HEIGHT(&vt->images[DEF_BACKGROUND_IMAGE].area);
    else
        return vt->ds_data[0].ds.height;
}

void dpy_update(DisplayState *s, int x, int y, int w, int h)
{
    s->dpy_update(s, x, y, w, h);
}

void dpy_cursor(DisplayState *s, int x, int y)
{
    if (s->dpy_text_cursor)
        s->dpy_text_cursor(s, x, y);
}

static int accepts_kbd(const vt_t *vt)
{
    return (vt->key_callback != NULL);
#if 0
    DFG FIXME: manage multiple text consoles
    int accepts = 0;

    if (vt->n_ds_data > 0) {
        int vtid = 0;
        do {
            accepts = (vt->ds_data[vtid++]->);
        } while (!accepts && vtid < vt->n_ds_data);
    }

    return accepts;
#endif
}

void gui_init(gui_host_callbacks_t* callbacks)
{
    gui_data = (struct gui_data_t*)qemu_mallocz(sizeof(struct gui_data_t));

    gui_set_input_state(INPUT_STATE_GUI_UNLOADED);
    if (callbacks != NULL) {
        gui_data->host_callbacks = *callbacks;
        gui_data->host_callbacks.set_screen_size(640, 480, 0);
        gui_data->host_callbacks.get_screen_data(&gui_data->screen_data);
    }

    gui_update_caption();
}

void gui_destroy(void)
{
    if (gui_data != NULL) {
        if (gui_data->loaded)
            gui_unload();
        qemu_free(gui_data);
        gui_data = NULL;
    }
}

int gui_load(const char *xml_file)
{
    parse_result_t res;
    int vt;
    
    res = load_gui_xml(xml_file);
    if (res == PARSE_OK) {
        gui_data->loaded = 1;

        for (vt = 0; vt < gui_data->n_vts; vt++) {
            index_map(&gui_data->vts[vt].clickable_map);
            if (gui_data->vts[vt].has_skin) {
                gui_data->vts[vt].rdata = create_render_data();
                init_background(vt);
                gui_show_image(vt, DEF_BACKGROUND_IMAGE);
            }
            initialize_display_areas(vt);
        }
        gui_set_input_state(INPUT_STATE_GUI_LOADED);
        gui_data->current_vt = &gui_data->vts[0];
        /*gui_notify_console_select(0); Done in vl.c */
    } else {
        if (parsing_location_error() != NULL)
            fprintf(stderr, "GUI parser error %d at %s\n", res, parsing_location_error());
    }

    return res == PARSE_OK;
}

void gui_unload(void)
{
    int vt;
    if (gui_data->loaded) {
        for (vt = 0; vt < gui_data->n_vts; vt++) {
            destroy_images(vt);
            destroy_clickable_map(&gui_data->vts[vt].clickable_map);
            destroy_render_data(gui_data->vts[vt].rdata);
            qemu_free(gui_data->vts[vt].background_buffer);
            qemu_free(gui_data->vts[vt].ds_data);
        }
        gui_set_input_state(INPUT_STATE_GUI_UNLOADED);
        gui_data->loaded = 0;
    }
}

int gui_needs_timer(void)
{
    return (gui_data->host_callbacks.process_events != NULL);
}

void gui_set_timer(struct QEMUTimer *timer)
{
    gui_data->gui_timer = timer;
}

static void init_background(VtID vtid)
{
    const int background_width = GET_GUI_AREA_WIDTH(&gui_data->vts[vtid].images[DEF_BACKGROUND_IMAGE].area);
    const int background_height = GET_GUI_AREA_HEIGHT(&gui_data->vts[vtid].images[DEF_BACKGROUND_IMAGE].area);

    gui_data->vts[vtid].gui_ds.width = background_width;
    gui_data->vts[vtid].gui_ds.height = background_height;
    gui_data->host_callbacks.init_ds(&gui_data->vts[vtid].gui_ds);
    gui_update_ds_data(&gui_data->vts[vtid].gui_ds, 1);

    gui_data->vts[vtid].background_row_size = background_width * 4;

    gui_data->vts[vtid].background_buffer = (void*)qemu_malloc(
        gui_data->vts[vtid].background_row_size * background_height);

    set_fb_base_from_host(gui_data->vts[vtid].rdata, 
                          gui_data->vts[vtid].background_buffer);

    set_cols(gui_data->vts[vtid].rdata, background_width);
    set_rows(gui_data->vts[vtid].rdata, background_height);

    /* FIXME: these data should be taken from the image_data,
       rather than hardcode this here. */
        /*set_pixel_order(gui_data->rdata, PO_BE);*/
        set_byte_order(gui_data->vts[vtid].rdata, BO_LE);
        set_color_order(gui_data->vts[vtid].rdata, CO_RGB);
        set_src_bpp(gui_data->vts[vtid].rdata, BPP_SRC_32);
    /* **** */

    gui_update_guest_display(&gui_data->vts[vtid]);
}

static void gui_update_guest_display(vt_t *vt)
{
    if (!nographic && vt->gui_ds.depth != 0) {
        render(&vt->gui_ds, vt->rdata, vt->full_update);
        vt->full_update = 0;
    }
}

enum what_buffer_t {
    FROM_BG_BUFFER,
    FROM_DEF_BG_IMAGE
};

static inline char* calc_bg_area_address(VtID vtid, const gui_area_t *area, enum what_buffer_t what_buffer)
{
    return (what_buffer == FROM_BG_BUFFER ? 
                (char*)gui_data->vts[vtid].background_buffer :
                (char*)gui_data->vts[vtid].images[DEF_BACKGROUND_IMAGE].image) +
        area->y0 * gui_data->vts[vtid].background_row_size +
        area->x0 * 4;
}

static void paint_rectangle(VtID vtid, gui_area_t *area, const void* source, int src_row_size)
{
    const int area_height = GET_GUI_AREA_HEIGHT(area);
    const int area_row_size = src_row_size > 0 ? src_row_size : GET_GUI_AREA_WIDTH(area) * 4;
    char* dest = calc_bg_area_address(vtid, area, FROM_BG_BUFFER);
    const char* src = (const char*)source;
    int y = 0;

    for (y=0; y < area_height; y++) {
        memcpy(dest, src, area_row_size);
        dest += gui_data->vts[vtid].background_row_size;
        src += area_row_size;
    }
}

void gui_show_image(VtID vtid, ImageID id)
{
    if (id == DEF_BACKGROUND_IMAGE) {
        /* background is the whole DS */
        memcpy(gui_data->vts[vtid].background_buffer,
               gui_data->vts[vtid].images[id].image, 
                   gui_data->vts[vtid].background_row_size *
                   GET_GUI_AREA_HEIGHT(&gui_data->vts[vtid].images[id].area));

    } else {
        paint_rectangle(vtid,
                        &gui_data->vts[vtid].images[id].area, 
                        gui_data->vts[vtid].images[id].image, 
                        0);
    }

    gui_data->vts[vtid].images[id].visible = 1;

    gui_data->vts[vtid].full_update = 1;    /* DFG TODO: IMPROVE! */
}

void gui_hide_image(VtID vtid, ImageID id)
{
    /* restore the background there */

    paint_rectangle(vtid,
                    &gui_data->vts[vtid].images[id].area, 
                    calc_bg_area_address(vtid,
                                         &gui_data->vts[vtid].images[id].area,
                                         FROM_DEF_BG_IMAGE),
                    gui_data->vts[vtid].background_row_size);

    gui_data->vts[vtid].images[id].visible = 0;

    gui_data->vts[vtid].full_update = 1;    /* DFG TODO: IMPROVE! */
}

int gui_register_mouse_event_handler(QEMUPutMouseEvent *func,
                                     void *opaque, int absolute,
                                     const char *devname)
{
    if (gui_data->loaded) {
        int found = 0;
        VtID vtid = 0;
        int parea;

        while (!found && vtid < gui_data->n_vts) {
            parea = 0;
            while (!found && parea < gui_data->vts[vtid].n_pointerareas) {
                found = (strcmp(gui_data->vts[vtid].pointerareas[parea].devid, devname) == 0);
                if (!found)
                    parea++;
            }
            if (!found)
                vtid++;
        }

        if (found) {
            gui_data->vts[vtid].pointerareas[parea].callback = func;
            gui_data->vts[vtid].pointerareas[parea].absolute = absolute;
            gui_data->vts[vtid].pointerareas[parea].opaque = opaque;
        } else {
            fprintf(stderr, "Warning: pointer device %s not accepted by the gui. Device ignored.\n", devname);
        }

        return found;
    } else {
        qemu_add_mouse_event_handler(func, opaque, absolute, devname);
        return 1;
    }
}

void gui_set_paint_callbacks(DisplayState *ds,
                             vga_hw_update_ptr update,
                             vga_hw_invalidate_ptr invalidate,
                             vga_hw_screen_dump_ptr screen_dump,
                             void *opaque)
{
    ds_data_t * const ds_data = &gui_data->vts[ds->vtid].ds_data[ds->dispid];
    ds_data->update = update;
    ds_data->invalidate = invalidate;
    ds_data->screen_dump = screen_dump;
    ds_data->opaque = opaque;
}

DisplayState *gui_get_graphic_console(const char *devname,
                                      vga_hw_update_ptr update,
                                      vga_hw_invalidate_ptr invalidate,
                                      vga_hw_screen_dump_ptr screen_dump,
                                      void *opaque)
{
    DisplayState *ret = NULL;

    if (gui_data->loaded && devname != NULL) {
        int found = 0;
        VtID vtid = 0;
        DisplayID dispid;

        while (!found && vtid < gui_data->n_vts) {
            dispid = 0;
            while (!found && dispid < gui_data->vts[vtid].n_ds) {
                found = (strcmp(gui_data->vts[vtid].ds_data[dispid].devid, devname) == 0);
                if (!found)
                    dispid++;
            }
            if (!found)
                vtid++;
        }

        if (found)
            ret = &gui_data->vts[vtid].ds_data[dispid].ds;
        else
            fprintf(stderr, "Warning: frame buffer '%s' not found in the GUI, assigning a skinless VT to it.\n", devname);
    }

    if (ret == NULL)  /* Allocate a new (skinless) VT */
        ret = gui_new_vt(SKINLESS_GRAPHIC_VT_PRIORITY_ORDER);

    if (ret != NULL)
        gui_set_paint_callbacks(ret, update, invalidate, screen_dump, opaque);

    return ret;
}

void gui_register_dev_key_callback(KeyCallback cb, void *opaque)
{
    gui_data->dev_key_callback = cb;
    gui_data->dev_key_opaque = opaque;
}

void gui_register_vt_key_callback(DisplayState *ds, KeyCallback cb, void *opaque)
{
    gui_data->vts[ds->vtid].key_callback = cb;
    gui_data->vts[ds->vtid].key_opaque = opaque;
}

/* only for skinless vts */
int gui_resize_vt(DisplayState *ds, int width, int height)
{
    const VtID vtid = ds->vtid;

    if (!gui_data->vts[vtid].has_skin) {
        ds_data_t * const ds_data = &gui_data->vts[vtid].ds_data[0]; /* assume ds->dispid == 0 */

        if (is_current_vt(vtid)) { 
            gui_data->host_callbacks.set_screen_size(width, 
                                                     height,
                                                     gui_data->gui_fullscreen);

            gui_data->host_callbacks.get_screen_data(&gui_data->screen_data);
            gui_update_ds_data(&ds_data->ds, 0);
        }

        ds_data->ds.width = width;
        ds_data->ds.height = height;

        if (!gui_data->updating) {
            invalidate_ds(ds_data);

            if (is_current_vt(vtid))
                update_ds(ds_data);
        }

        return 1;
    }
    else {
        fprintf(stderr, "Error: cannot resize a skinned gui. Specify the dimensions in the XML file.\n");
        return 0;
    }
}

static VtID insert_new_vt(int order_priority)
{
    int position = 0;
    VtID new_vtid;

    while(position < gui_data->n_vts && gui_data->vts[gui_data->ordered_vts[position]].order_priority <= order_priority)
        position++;

    if (position < gui_data->n_vts) {
        /* insert a place, shift all the remaining elements 1 position. */
        int i;
        for (i = gui_data->n_vts; i > position; i--)
            gui_data->ordered_vts[i] = gui_data->ordered_vts[i-1];
    } /* else : new one */

    new_vtid = gui_data->n_vts++;
    gui_data->ordered_vts[position] = new_vtid;
    gui_data->vts[new_vtid].order_priority = order_priority;

    return new_vtid;
}

DisplayState *gui_new_vt(int order_priority)
{
    int vtid = -1;

    if (gui_data->n_vts < MAX_CONSOLES-1) {
        /* find where to insert this according to order_priority */
        vtid = insert_new_vt(order_priority);

        /* allocate and initialize */
        gui_data->vts[vtid].has_skin = 0; /* redundant, but to be clear */
        gui_data->vts[vtid].n_ds = 1;
        gui_data->vts[vtid].ds_data = (ds_data_t*)qemu_mallocz(sizeof(ds_data_t)*1);
        gui_data->host_callbacks.init_ds(&gui_data->vts[vtid].ds_data[0].ds);
        gui_update_ds_data(&gui_data->vts[vtid].ds_data[0].ds, 0);
        gui_data->vts[vtid].ds_data->ds.width = 640;
        gui_data->vts[vtid].ds_data->ds.height = 480;
        gui_data->vts[vtid].ds_data->ds.vtid = vtid;
        gui_data->vts[vtid].ds_data->ds.dispid = 0;
        return &gui_data->vts[vtid].ds_data->ds;
    } else {
        fprintf(stderr, "Error: too many consoles.\n");
        return NULL;
    }
}

/************ Button Actions **************/
static void gui_button_noaction(void *parameter)
{
}

static void gui_button_sendkey_dn(void *parameter)
{
    const int key = (int)(long int)parameter;

    gui_notify_dev_key(key);
}

static void gui_button_sendkey_up(void *parameter)
{
    const int key = (int)(long int)parameter;

    gui_notify_dev_key(key | 0x80);
}

struct button_actions_s button_actions[] =
{
    { gui_button_noaction, gui_button_noaction },
    { gui_button_sendkey_dn, gui_button_sendkey_up },
};

/************ Internal Functions Definition **************/

enum AreaAdjustment
{
    AREA_CONTAINED_OK,
    AREA_NOT_CONTAINED,
    AREA_DIMENSIONS_ADJUSTED
};

static enum AreaAdjustment adjust_area_to_bg(VtID vtid, gui_area_t *area)
{
    enum AreaAdjustment ret = AREA_CONTAINED_OK;

    const int bg_width = get_vt_width(&gui_data->vts[vtid]);
    const int bg_height = get_vt_height(&gui_data->vts[vtid]);

    if (area->x0 > bg_width || area->y0 > bg_height)
        ret = AREA_NOT_CONTAINED;
    else {
        const int area_width = GET_GUI_AREA_WIDTH(area);
        const int area_height = GET_GUI_AREA_HEIGHT(area);

        if ((area->x0 + area_width) > bg_width || area_width <= 0) {
            SET_GUI_AREA_WIDTH(area, bg_width - area->x0);
            if (area_width > 0)
                ret = AREA_DIMENSIONS_ADJUSTED;
        }

        if ((area->y0 + area_height) > bg_height || area_height <= 0) {
            SET_GUI_AREA_HEIGHT(area, bg_height - area->y0);
            if (area_height > 0)
                ret = AREA_DIMENSIONS_ADJUSTED;
        }
    }

    return ret;
}

static void load_gui_image(VtID vtid, ImageID id, const image_node_t *image_node)
{
    gui_data->vts[vtid].images[id].area = image_node->area;

    if (id == DEF_BACKGROUND_IMAGE) {
        /* Ensure x0,y0 are 0,0*/
        if (gui_data->vts[vtid].images[id].area.x0 != 0 ||
            gui_data->vts[vtid].images[id].area.y0 != 0) {

            fprintf(stderr, "Warning: invalid (x0,y0) for background. They should be (0,0).\n");
            gui_data->vts[vtid].images[id].area.x0 = 0;
            gui_data->vts[vtid].images[id].area.y0 = 0;
        }
    } else {
        /* if it's not the background, ensure it is contained in it */
        switch(adjust_area_to_bg(vtid, &gui_data->vts[vtid].images[id].area)) {
        case AREA_CONTAINED_OK:
            break; /* OK */
        case AREA_NOT_CONTAINED:
            fprintf(stderr, "Warning: image id %d of vt %d outside background boundaries. Image ignored.\n", id, vtid);
            break;
        case AREA_DIMENSIONS_ADJUSTED:
            fprintf(stderr, "Warning: image id %d of vt %d outside background boundaries. Image cropped.\n", id, vtid);
            break;
        }
    }

    /* Support png only for now */
    gui_load_image_png(image_node->filename, &gui_data->vts[vtid].images[id]);
}

static void load_gui_displayarea(VtID vtid, DisplayID id, displayarea_node_t *displayarea_node)
{
    if (adjust_area_to_bg(vtid, &displayarea_node->area) == AREA_CONTAINED_OK) {
        DisplayState * const ds = &gui_data->vts[vtid].ds_data[id].ds;
        ds->vtid = vtid;
        ds->dispid = id;
        ds->x0 = displayarea_node->area.x0;
        ds->y0 = displayarea_node->area.y0;
        ds->width = GET_GUI_AREA_WIDTH(&displayarea_node->area);
        ds->height = GET_GUI_AREA_HEIGHT(&displayarea_node->area);
        gui_data->host_callbacks.init_ds(ds);
        gui_update_ds_data(ds, 1);
        strcpy(gui_data->vts[vtid].ds_data[id].devid, displayarea_node->devid);
    } else
        fprintf(stderr, "Warning: displayarea id %d of vt %d outside background boundaries. Display ignored.", id, vtid);
}

static void load_gui_button(VtID vtid, int nbutton, const button_node_t *button_node)
{
    const int area_id = gui_data->vts[vtid].clickable_map.n_areas++;

    /* fill button data */
    gui_data->vts[vtid].buttons[nbutton].pressed_img_id = button_node->pressed_img_id;

    if (button_node->action[0] == 0) {
        gui_data->vts[vtid].buttons[nbutton].actions = BUTTON_ACTION_NOACTION;
    } else if (strcmp(button_node->action, "sendkey") == 0) {
        gui_data->vts[vtid].buttons[nbutton].actions = BUTTON_ACTION_SENDKEY;
        gui_data->vts[vtid].buttons[nbutton].parameter = (void*)atol(button_node->parameter);
    } else {
        gui_data->vts[vtid].buttons[nbutton].actions = BUTTON_ACTION_NOACTION;
        fprintf(stderr, "Warning: unknown button action '%s'\n", button_node->action);
    }

    /* fill clickarea data */
    gui_data->vts[vtid].clickable_map.areas[area_id].area = button_node->area;
    gui_data->vts[vtid].clickable_map.areas[area_id].type = CA_BUTTON;
    gui_data->vts[vtid].clickable_map.areas[area_id].id = nbutton;
}

static void load_gui_pointerarea(VtID vtid, int nparea, const pointerarea_node_t *parea_node)
{
    const int area_id = gui_data->vts[vtid].clickable_map.n_areas++;

    /* fill pointerarea data */
    strcpy(gui_data->vts[vtid].pointerareas[nparea].devid, parea_node->devid);
    gui_data->vts[vtid].pointerareas[nparea].absolute = parea_node->absolute;
    gui_data->vts[vtid].pointerareas[nparea].grab_on_click = parea_node->grab_on_click;
    gui_data->vts[vtid].pointerareas[nparea].show_cursor_while_grabbing = parea_node->show_cursor_while_grabbing;

    /* fill clickarea data */
    gui_data->vts[vtid].clickable_map.areas[area_id].area = parea_node->area;
    gui_data->vts[vtid].clickable_map.areas[area_id].type = CA_POINTERAREA;
    gui_data->vts[vtid].clickable_map.areas[area_id].id = nparea;
}

#define FLATTEN(type,first_node,function)   \
    {                                       \
        type *node = first_node;            \
        type *next;                         \
        unsigned int i = 0;                 \
        while(node != NULL) {               \
            function(i, node);              \
            next = node->next;              \
            qemu_free(node);                \
            node = next;                    \
            i++;                            \
        }                                   \
    }

#define VT_FLATTEN(type,vtid,first_node,function)   \
    {                                               \
        type *node = first_node;                    \
        type *next;                                 \
        unsigned int i = 0;                         \
        while(node != NULL) {                       \
            function(vtid, i, node);                \
            next = node->next;                      \
            qemu_free(node);                        \
            node = next;                            \
            i++;                                    \
        }                                           \
    }

#define ALLOC_ARRAY(type,n) (type*)qemu_mallocz((n)*sizeof(type))


static void load_gui_vt(int nvt, const vt_node_t *vt_node)
{
    const VtID vt = insert_new_vt(SKINNED_VT_PRIORITY_ORDER); /* nvt + gui_data->n_vts;*/
    int n_clickareas = 0;

    gui_data->vts[vt].has_skin = 1;

    /* allocate memory first */
    gui_data->vts[vt].n_images = vt_node->n_images;
    if (vt_node->n_images > 0)
        gui_data->vts[vt].images = ALLOC_ARRAY(gui_image_t, vt_node->n_images);

    n_clickareas = vt_node->n_buttons + vt_node->n_pointerareas;
    if (n_clickareas > 0)
        gui_data->vts[vt].clickable_map.areas = ALLOC_ARRAY(clickable_area_t, n_clickareas);
    gui_data->vts[vt].clickable_map.n_areas = 0; /* will be incremented while
                                                    loading buttons and pointerareas */

    gui_data->vts[vt].n_buttons = vt_node->n_buttons;
    if (vt_node->n_buttons > 0)
        gui_data->vts[vt].buttons = ALLOC_ARRAY(button_t, vt_node->n_buttons);

    gui_data->vts[vt].n_pointerareas = vt_node->n_pointerareas;
    if (vt_node->n_pointerareas > 0)
        gui_data->vts[vt].pointerareas = ALLOC_ARRAY(pointer_area_t, vt_node->n_pointerareas);

    gui_data->vts[vt].n_ds = vt_node->n_displayareas;
    if (vt_node->n_displayareas > 0)
        gui_data->vts[vt].ds_data = ALLOC_ARRAY(ds_data_t, vt_node->n_displayareas);

    /* flatten subtrees */
    VT_FLATTEN(image_node_t,       vt, vt_node->first_image_node,       load_gui_image);
    VT_FLATTEN(button_node_t,      vt, vt_node->first_button_node,      load_gui_button);
    VT_FLATTEN(pointerarea_node_t, vt, vt_node->first_pointerarea_node, load_gui_pointerarea);
    VT_FLATTEN(displayarea_node_t, vt, vt_node->first_displayarea_node, load_gui_displayarea);
}

static parse_result_t load_gui_xml(const char* xml_file)
{
    gui_xml_tree_t gui_xml_tree;
    parse_result_t ret;

    /* 1: parse the file */
    ret = parse_gui_xml(xml_file, &gui_xml_tree);
    if (ret != PARSE_OK)
        return ret;

    /* 2: allocate memory */
    if (gui_xml_tree.n_vts == 0) {
        fprintf(stderr, "GUI Error: at least one VT shall be defined\n");
        return UNEXPECTED_EOF;
    }

    FLATTEN(vt_node_t, gui_xml_tree.first_vt_node, load_gui_vt);

    return ret;
}

#undef ALLOC_ARRAY
#undef FLATTEN
#undef VT_FLATTEN

static void index_map(clickable_map_t *map)
{
    /* TODO: no geometrical index implemented yet */
}

static void initialize_display_areas(VtID vtid)
{
    int nds;
    for(nds = 0; nds < gui_data->vts[vtid].n_ds; nds++) {
        gui_data->host_callbacks.init_ds(&gui_data->vts[vtid].ds_data[nds].ds);
        gui_update_ds_data(&gui_data->vts[vtid].ds_data[nds].ds, gui_data->vts[vtid].has_skin);
    }
}

static void destroy_images(VtID vtid)
{
    int i;
    for (i=0; i<gui_data->vts[vtid].n_images; i++)
        qemu_free(gui_data->vts[vtid].images[i].image);

    qemu_free(gui_data->vts[vtid].images);
}

static void destroy_clickable_map(clickable_map_t *map)
{
    qemu_free(map->areas);
}

static inline int area_contains(const gui_area_t *area, int x, int y)
{
#define IN_RANGE(min,value,max) ((min)<=(value) && (value)<=(max))
    return IN_RANGE(area->x0, x, area->x1) && IN_RANGE(area->y0, y, area->y1);
#undef IN_RANGE
}

/* TODO: Optimize with a geometrical index! */
static clickable_area_t *find_clickarea(const vt_t *vt, int x, int y, AreaID *id_found)
{
    AreaID id = 0;
    int found = 0;
    clickable_area_t * carea;

    while (id < vt->clickable_map.n_areas && !found) {
        carea = &vt->clickable_map.areas[id++];
        found = area_contains(&carea->area, x, y);
    }

    if (found) {
        *id_found = id-1;
        return carea;
    } else
        return NULL;
}

static inline int grabbing_mouse(void)
{
    if (vt_enabled())
        return gui_data->current_vt->clickable_map.currently_grabbing != NULL;
    else
        return 0;
}

static inline int gui_grabbing(void)
{
    return grabbing_mouse() /*TODO when implementing VNC: || grabbing_kbd()*/;
}

static pointer_area_t *current_parea(void)
{
    if (vt_enabled())
        if (gui_data->current_vt->clickable_map.currently_grabbing != NULL)
            return &gui_data->current_vt->pointerareas[
                gui_data->current_vt->clickable_map.currently_grabbing->id
            ];
        else
            return NULL;
    else
        return NULL;
}

/************************* Input Host Functions ****************************/

/* Wrappers and functions to common states */

static void gui_update_caption(void)
{
    if(gui_data->host_callbacks.set_caption != NULL) {
        char buf[1024];
        const char *status = "";
        int grabbing;

        if (gui_data->loaded)
            grabbing = (gui_data->current_vt->clickable_map.currently_grabbing != NULL);
        else
            grabbing = gui_data->unloaded_state_data.gui_grab;


        if (!vm_running)
            status = " [Stopped]";
        else if (grabbing) {
            if (!alt_grab)
                status = " - Press Ctrl-Alt to exit grab";
            else
                status = " - Press Ctrl-Alt-Shift to exit grab";
        }

        if (qemu_name)
            snprintf(buf, sizeof(buf), "QEMU (%s)%s", qemu_name, status);
        else
            snprintf(buf, sizeof(buf), "QEMU%s", status);

        gui_data->host_callbacks.set_caption(buf, "QEMU");
    }
}

int gui_allows_fullscreen(void)
{
    return (gui_data->host_callbacks.set_screen_size != NULL);
}

void gui_notify_toggle_fullscreen(void)
{
    gui_data->input_table->gui_notify_toggle_fullscreen();
}

void gui_notify_mouse_motion(int dx, int dy, int dz, int x, int y, int state)
{
    gui_data->input_table->gui_notify_mouse_motion(dx, dy, dz, x, y, state);
}

void gui_notify_mouse_button(int dz, int x, int y, int state)
{
    gui_data->input_table->gui_notify_mouse_button(dz, x, y, state);
}

void gui_notify_mouse_warp(int x, int y, int on)
{
    gui_data->input_table->gui_notify_mouse_warp(x, y, on);
}

void gui_notify_term_key(int keysym)
{
    if (vt_enabled()) {
        if (gui_data->current_vt->key_callback != NULL)
            gui_data->current_vt->key_callback(
                gui_data->current_vt->key_opaque,
                keysym);
    }
}

void gui_notify_dev_key(int keysym)
{
    if (vt_enabled()) {
        if (gui_data->dev_key_callback != NULL)
            gui_data->dev_key_callback(
                gui_data->dev_key_opaque,
                keysym);
    }
}

static void gui_notify_console_select_by_vtid(VtID console)
{
    if (console < gui_data->n_vts) {
        const int vt_width = get_vt_width(&gui_data->vts[console]);
        const int vt_height = get_vt_height(&gui_data->vts[console]);
        int disp;
        gui_data->current_vt = &gui_data->vts[console];
        if (vt_width != gui_data->screen_data.width ||
            vt_height != gui_data->screen_data.height) {

            gui_data->host_callbacks.set_screen_size(vt_width, vt_height, gui_data->gui_fullscreen);
            gui_data->host_callbacks.get_screen_data(&gui_data->screen_data);
        }

        if (gui_data->vts[console].has_skin)
            gui_update_ds_data(&gui_data->vts[console].gui_ds, 1);
        gui_data->current_vt->full_update = 1;

        for (disp=0; disp < gui_data->current_vt->n_ds; disp++) {
            gui_update_ds_data(&gui_data->current_vt->ds_data[disp].ds, gui_data->vts[console].has_skin);
            if (gui_data->current_vt->ds_data[disp].invalidate != NULL)
                gui_data->current_vt->ds_data[disp].invalidate(
                    gui_data->current_vt->ds_data[disp].opaque
                );
        }

        /* determine kbd terminal mode */
        if (gui_data->host_callbacks.set_kbd_terminal_mode != NULL)
            gui_data->host_callbacks.set_kbd_terminal_mode(gui_data->current_vt->key_callback != NULL);
    } else
        gui_data->current_vt = NULL;

/*    if (!gui_is_graphic_console()) { */
    if (gui_data->current_vt != NULL && accepts_kbd(gui_data->current_vt)) {
        /* display grab if going to a text console */
        if (gui_grabbing())
            gui_loaded_grab_end();
    }

}

static VtID gui_get_console_order_vtid(int console_order)
{
    return gui_data->ordered_vts[console_order];
}

void gui_notify_console_select(int console_order)
{
    if (console_order < gui_data->n_vts)
        gui_notify_console_select_by_vtid(gui_get_console_order_vtid(console_order));
}

void gui_notify_activate_display(DisplayState *ds)
{
    gui_notify_console_select_by_vtid(ds->vtid);
}

void gui_notify_toggle_grabmode(void)
{
    gui_data->input_table->gui_notify_toggle_grabmode();
}

void gui_notify_new_guest_cursor(void)
{
    gui_data->input_table->gui_notify_new_guest_cursor();
}

void gui_notify_input_focus_lost(void)
{
    gui_data->input_table->gui_notify_input_focus_lost();
}

void gui_notify_update_tick(int64_t ticks)
{
    if (vt_enabled()) {
        int disp;

        if (gui_data->current_vt->full_update) {
            /* update the background */
            if (gui_data->current_vt->has_skin)
                gui_update_guest_display(gui_data->current_vt);

            for (disp=0; disp < gui_data->current_vt->n_ds; disp++)
                invalidate_ds(&gui_data->current_vt->ds_data[disp]);
        }

        gui_data->updating = 1;

        for (disp=0; disp < gui_data->current_vt->n_ds; disp++)
            update_ds(&gui_data->current_vt->ds_data[disp]);

        gui_data->updating = 0;
    }

    gui_data->host_callbacks.process_events();

    gui_update_timer(ticks);
}

void gui_notify_app_focus(int gain)
{
    gui_data->input_table->gui_notify_app_focus(gain);
}

void gui_notify_idle(int idle)
{
    gui_data->idle = idle;
}

void gui_notify_repaint_screen(void)
{
    if (vt_enabled()) {
        int disp;

        /* update the background */
        if (gui_data->current_vt->has_skin) {
            dpy_update(&gui_data->current_vt->gui_ds,
                       0,
                       0,
                       gui_data->current_vt->gui_ds.width,
                       gui_data->current_vt->gui_ds.height
                      );
        }

        for (disp=0; disp < gui_data->current_vt->n_ds; disp++) {
            dpy_update(&gui_data->current_vt->ds_data[disp].ds,
                       0,
                       0,
                       gui_data->current_vt->ds_data[disp].ds.width,
                       gui_data->current_vt->ds_data[disp].ds.height
                      );
        }
    }
}

void gui_notify_screen_dump(const char *filename)
{
    if (vt_enabled()) {
        if (gui_data->current_vt->n_ds > 0 &&
            gui_data->current_vt->ds_data[0].screen_dump != NULL) {
            gui_data->current_vt->ds_data[0].screen_dump(
                gui_data->current_vt->ds_data[0].opaque,
                filename);
        }
    }
}

int gui_is_display_active(DisplayState *ds)
{
    return (gui_data->current_vt - gui_data->vts) == ds->vtid;
}

void gui_refresh_caption(void)
{
    if (gui_data->last_vm_running != vm_running) {
        gui_data->last_vm_running = vm_running;
        gui_update_caption();
    }
}

static void gui_update_timer(int64_t ticks)
{
    qemu_mod_timer(gui_data->gui_timer,
        (gui_data->gui_timer_interval ?
	    gui_data->gui_timer_interval :
	    GUI_REFRESH_INTERVAL)
	+ ticks);
}

static inline int col_to_bytes(int bpp, int col)
{
    switch(bpp) {
    case 32:
        return col * 4;
    case 24:
        return col * 3;
    case 16:
    case 15:
        return col * 2;
    case 8:
        return col;
    case 4:
        return col >> 1;
    case 2:
        return col >> 2;
    case 1:
        return col >> 3;
    default:
        return 0;
    }
}

static void gui_update_ds_data(DisplayState *ds, int in_skinned_vt)
{
#if 0
    DFG: TODO: see why this comes wrong
    ds->linesize = gui_data->screen_data.linesize;
#endif
    ds->depth = gui_data->screen_data.depth;
    ds->bgr = gui_data->screen_data.bgr;
    if (in_skinned_vt) {
        ds->linesize = col_to_bytes(ds->depth, gui_data->screen_data.width);
        /* calc offset */
        ds->data = gui_data->screen_data.data + ds->y0 * ds->linesize + col_to_bytes(ds->depth, ds->x0);
    } else {
        ds->linesize = gui_data->screen_data.linesize;
        ds->data = gui_data->screen_data.data;
    }

}

/* ************ GUI UNLOADED VERSION ********************/

static void gui_unloaded_hide_cursor(void)
{
    if (!cursor_hide)
        return;

    if (kbd_mouse_is_absolute()) {
        gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_HIDDEN);
    } else {
        gui_data->host_callbacks.turn_cursor_off();
    }
}

static void gui_unloaded_show_cursor(void)
{
    gui_cursor_type_t cursor_type;

    if (!cursor_hide)
        return;

    if (!kbd_mouse_is_absolute()) {
        if (gui_data->guest_cursor &&
                (gui_data->unloaded_state_data.gui_grab || 
                 kbd_mouse_is_absolute() || 
                 gui_data->unloaded_state_data.absolute_enabled))
            cursor_type = GUI_CURSOR_GUEST_SPRITE;
        else
            cursor_type = GUI_CURSOR_NORMAL;
        
        gui_data->host_callbacks.turn_cursor_on(cursor_type);
    }
}

static void gui_unloaded_grab_start(void)
{
    if (gui_data->guest_cursor) {
        gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_GUEST_SPRITE);
        gui_data->host_callbacks.mouse_warp(gui_data->guest_x, 
                                            gui_data->guest_y);
    } else
        gui_unloaded_hide_cursor();

    gui_data->host_callbacks.grab_input_on();
    gui_data->unloaded_state_data.gui_grab = 1;
    gui_update_caption();
}

static void gui_unloaded_grab_end(void)
{
    gui_data->host_callbacks.grab_input_off();
    gui_data->unloaded_state_data.gui_grab = 0;
    gui_unloaded_show_cursor();
    gui_update_caption();
}

static void gui_unloaded_send_mouse_event(int dx, int dy, int dz, int x, int y, int state)
{
    if (kbd_mouse_is_absolute()) {
        if (!gui_data->unloaded_state_data.absolute_enabled) {
            gui_unloaded_hide_cursor();
            if (gui_data->unloaded_state_data.gui_grab) {
	            gui_unloaded_grab_end();
            }
            gui_data->unloaded_state_data.absolute_enabled = 1;
        }

        dx = x * 0x7FFF / (gui_data->screen_data.width - 1);
        dy = y * 0x7FFF / (gui_data->screen_data.height - 1);
    } else if (gui_data->unloaded_state_data.absolute_enabled) {
        gui_unloaded_show_cursor();
        gui_data->unloaded_state_data.absolute_enabled = 0;
    } else if (gui_data->guest_cursor) {
        /* calc relative coords 
            dx = x - guest_x
            dy = y - guest_y
        */
        x -= gui_data->guest_x;
        y -= gui_data->guest_y;
        gui_data->guest_x += x;
        gui_data->guest_y += y;
        dx = x;
        dy = y;
    }

    kbd_mouse_event(dx, dy, dz, state);
}

static void gui_unloaded_notify_toggle_fullscreen(void)
{
    gui_data->gui_fullscreen = !gui_data->gui_fullscreen;

    if (gui_data->gui_fullscreen) {
        gui_data->unloaded_state_data.gui_saved_grab = 
            gui_data->unloaded_state_data.gui_grab;
        gui_unloaded_grab_start();
    } else {
        if (!gui_data->unloaded_state_data.gui_saved_grab)
            gui_unloaded_grab_end();
    }

    gui_data->host_callbacks.set_screen_size(
        gui_data->screen_data.width,
        gui_data->screen_data.height,
        gui_data->gui_fullscreen);

    gui_data->host_callbacks.get_screen_data(&gui_data->screen_data);

    gui_notify_console_select_by_vtid(current_vt_id());
}

static void gui_unloaded_notify_mouse_motion(int dx, int dy, int dz, int x, int y, int state)
{
    if (gui_data->unloaded_state_data.gui_grab || 
        kbd_mouse_is_absolute() ||
        gui_data->unloaded_state_data.absolute_enabled) {
        gui_unloaded_send_mouse_event(dx, dy, dz, x, y, state);
    }
}

static void gui_unloaded_notify_mouse_button(int dz, int x, int y, int state)
{
    if (gui_data->unloaded_state_data.gui_grab || kbd_mouse_is_absolute()) {
        gui_unloaded_send_mouse_event(0, 0, dz, x, y, state);
    } else {
        if (state == MOUSE_EVENT_LBUTTON) {
            /* start grabbing all events */
            gui_unloaded_grab_start();
        }
    }
}

static void gui_unloaded_notify_mouse_warp(int x, int y, int on)
{
    if (on) {
        if (!gui_data->guest_cursor)
            gui_unloaded_show_cursor();
        if (gui_data->unloaded_state_data.gui_grab || 
            kbd_mouse_is_absolute() || 
            gui_data->unloaded_state_data.absolute_enabled) {
            gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_GUEST_SPRITE);
            gui_data->host_callbacks.mouse_warp(gui_data->guest_x, 
                                                gui_data->guest_y);
        }
    } else if (gui_data->unloaded_state_data.gui_grab)
        gui_unloaded_hide_cursor();
    gui_data->guest_cursor = on;
    gui_data->guest_x = x;
    gui_data->guest_y = y;
}

static void gui_unloaded_notify_toggle_grabmode(void)
{
    if (!gui_data->unloaded_state_data.gui_grab) {
        int app_active = 1;

        /* if the application is not active,
           do not try to enter grab state. It
           prevents
           'SDL_WM_GrabInput(SDL_GRAB_ON)'
           from blocking all the application
           (SDL bug). */
        if (gui_data->host_callbacks.is_app_active != NULL)
            app_active = gui_data->host_callbacks.is_app_active();

        if (app_active)
            gui_unloaded_grab_start();
    } else {
        gui_unloaded_grab_end();
    }
}

static void gui_unloaded_notify_new_guest_cursor(void)
{
    /* DFG: DANGER, this call also invokes show_cursor() */
    if (gui_data->guest_cursor &&
            (gui_data->unloaded_state_data.gui_grab || 
             kbd_mouse_is_absolute() || 
             gui_data->unloaded_state_data.absolute_enabled))
        gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_GUEST_SPRITE);
}

static void gui_unloaded_notify_input_focus_lost(void)
{
    if (gui_data->unloaded_state_data.gui_grab && 
        !gui_data->unloaded_state_data.gui_fullscreen_initial_grab)
        gui_unloaded_grab_end();
}

static void gui_unloaded_notify_app_focus(int gain)
{
    if (gain) {
        /* Back to default interval */
        gui_data->gui_timer_interval = 0;
        gui_data->idle = 0;
    } else {
        /* Sleeping interval */
        gui_data->gui_timer_interval = 500;
        gui_data->idle = 1;
    }
}

/******************* GUI LOADED VERSION *********************/

static void gui_loaded_hide_cursor(void)
{
    if (!cursor_hide)
        return;

    gui_data->host_callbacks.turn_cursor_off();
}

static void gui_loaded_show_cursor(void)
{
    gui_cursor_type_t cursor_type;

    if (!cursor_hide)
        return;

    if (gui_grabbing()) {
        if (gui_data->guest_cursor && current_parea() != NULL &&
            current_parea()->show_cursor_while_grabbing) {
            cursor_type = GUI_CURSOR_GUEST_SPRITE;
        }else
            cursor_type = GUI_CURSOR_NORMAL;
    } else
        cursor_type = GUI_CURSOR_NORMAL;

    gui_data->host_callbacks.turn_cursor_on(cursor_type);
}

static void gui_loaded_grab_start(void)
{
    if (gui_data->guest_cursor) {
        gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_GUEST_SPRITE);
        gui_data->host_callbacks.mouse_warp(gui_data->guest_x, 
                                            gui_data->guest_y);
    } else
        gui_loaded_hide_cursor();

    gui_data->host_callbacks.grab_input_on();
    gui_update_caption();
}

static void gui_loaded_grab_end(void)
{
    gui_data->host_callbacks.grab_input_off();
    gui_data->current_vt->clickable_map.currently_grabbing = NULL;
    gui_loaded_show_cursor();
    gui_update_caption();
}

static void gui_loaded_send_mouse_event(clickable_area_t *carea, int dx, int dy, int dz, int x, int y, int state)
{
    pointer_area_t * const parea = &gui_data->current_vt->pointerareas[carea->id];

    if (parea->absolute) {
        dx = x * 0x7FFF / (GET_GUI_AREA_WIDTH(&carea->area)- 1);
        dy = y * 0x7FFF / (GET_GUI_AREA_HEIGHT(&carea->area) - 1);
    } else if (gui_data->guest_cursor) {
        /* calc relative coords 
            dx = x - guest_x
            dy = y - guest_y
        */
        x -= gui_data->guest_x;
        y -= gui_data->guest_y;
        gui_data->guest_x += x;
        gui_data->guest_y += y;
        dx = x;
        dy = y;
    }

    parea->callback(parea->opaque, dx, dy, dz, state);
}

static void gui_loaded_notify_toggle_fullscreen(void)
{
    gui_data->gui_fullscreen = !gui_data->gui_fullscreen;

    gui_data->host_callbacks.set_screen_size(
        gui_data->screen_data.width,
        gui_data->screen_data.height,
        gui_data->gui_fullscreen);

    gui_data->host_callbacks.get_screen_data(&gui_data->screen_data);

    gui_notify_console_select_by_vtid(current_vt_id());

}

static void gui_loaded_notify_mouse_motion(int dx, int dy, int dz, int x, int y, int state)
{
    if (vt_enabled()) {
        if (gui_grabbing() /*|| kbd_mouse_is_absolute() ||
            absolute_enabled*/) {
            gui_loaded_send_mouse_event(gui_data->current_vt->clickable_map.currently_grabbing, dx, dy, dz, x, y, state);
        }
    }
}

static void gui_loaded_notify_mouse_button(int dz, int x, int y, int state)
{
    if (vt_enabled()) {
        clickable_area_t *forward_area = NULL;

        /* If we're not grabbing, check if we should start due to click on a pointerarea */

        if (grabbing_mouse())
            forward_area = gui_data->current_vt->clickable_map.currently_grabbing;
        else {
            AreaID id;
            clickable_area_t *carea = find_clickarea(gui_data->current_vt, x, y, &id);

            if (carea != NULL) {
                if (carea->type == CA_BUTTON) {
                    if (state == MOUSE_EVENT_LBUTTON) {
                        if (gui_data->current_vt->buttons[carea->id].pressed_img_id > 0)
							gui_show_image(current_vt_id(),
                                           gui_data->current_vt->buttons[carea->id].pressed_img_id);

                        button_actions[gui_data->current_vt->buttons[carea->id].actions].down(gui_data->current_vt->buttons[carea->id].parameter);
                    } else {
                        if (gui_data->current_vt->buttons[carea->id].pressed_img_id > 0)
							gui_hide_image(current_vt_id(),
											gui_data->current_vt->buttons[carea->id].pressed_img_id);
                        button_actions[gui_data->current_vt->buttons[carea->id].actions].up(gui_data->current_vt->buttons[carea->id].parameter);
                    }
                } else { /* pointerarea*/
                    /* get the associated pointerarea */
                    pointer_area_t * const parea = &gui_data->current_vt->pointerareas[carea->id];
                    if (parea->grab_on_click) {
                        if (state == MOUSE_EVENT_LBUTTON) {
                            gui_data->current_vt->clickable_map.currently_grabbing = carea;
                            gui_loaded_grab_start();
                        } /* else: ignore */
                    } else
                        forward_area = carea;
                }
            }
        }

        if (forward_area != NULL)
            gui_loaded_send_mouse_event(forward_area, 0, 0, dz, x, y, state);
    }
}

static void gui_loaded_notify_mouse_warp(int x, int y, int on)
{
    if (on) {
        if (!gui_data->guest_cursor)
            gui_loaded_show_cursor();
        if (gui_grabbing() /*|| kbd_mouse_is_absolute() || absolute_enabled*/) {
            gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_GUEST_SPRITE);
            gui_data->host_callbacks.mouse_warp(gui_data->guest_x,
                                                gui_data->guest_y);
        }
    } else if (gui_grabbing())
        gui_loaded_hide_cursor();
    gui_data->guest_cursor = on;
    gui_data->guest_x = x;
    gui_data->guest_y = y;
}

static void gui_loaded_notify_toggle_grabmode(void)
{
    if (vt_enabled()) {
        if (!gui_grabbing()) {
#if 0
            /* TODO DFG : distinguish between mouse and kbd grabbing. Do this when
               implementing VNC support. */
            int app_active = 1;

            /* if the application is not active,
               do not try to enter grab state. It
               prevents
               'SDL_WM_GrabInput(SDL_GRAB_ON)'
               from blocking all the application
               (SDL bug). */
            if (gui_data->host_callbacks.is_app_active != NULL)
                app_active = gui_data->host_callbacks.is_app_active();

            if (app_active)
                gui_grab_start();
#endif
        } else {
            gui_loaded_grab_end();
        }
    }
}

static void gui_loaded_notify_new_guest_cursor(void)
{
    /* DFG: DANGER, this call also invokes show_cursor() */
    if (gui_data->guest_cursor &&
            (gui_grabbing() /*|| kbd_mouse_is_absolute() || absolute_enabled*/))
        gui_data->host_callbacks.turn_cursor_on(GUI_CURSOR_GUEST_SPRITE);
}

static void gui_loaded_notify_input_focus_lost(void)
{
    if (gui_grabbing())
        gui_loaded_grab_end();
}

static void gui_loaded_notify_app_focus(int gain)
{
    gui_unloaded_notify_app_focus(gain);
}


/**************************************************************************/

const gui_input_table_t input_tables[2] = {
    {
        gui_unloaded_notify_toggle_fullscreen,
        gui_unloaded_notify_toggle_grabmode,
        gui_unloaded_notify_mouse_motion,
        gui_unloaded_notify_mouse_button,
        gui_unloaded_notify_mouse_warp,
        gui_unloaded_notify_new_guest_cursor,
        gui_unloaded_notify_input_focus_lost,
        gui_unloaded_notify_app_focus,
    },
    {
        gui_loaded_notify_toggle_fullscreen,
        gui_loaded_notify_toggle_grabmode,
        gui_loaded_notify_mouse_motion,
        gui_loaded_notify_mouse_button,
        gui_loaded_notify_mouse_warp,
        gui_loaded_notify_new_guest_cursor,
        gui_loaded_notify_input_focus_lost,
        gui_loaded_notify_app_focus,
    },
};

static void gui_set_input_state(enum gui_input_state_t input_state)
{
    gui_data->input_table = &input_tables[input_state];
}

