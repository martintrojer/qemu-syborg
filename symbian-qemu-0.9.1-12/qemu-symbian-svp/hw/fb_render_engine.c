/*
 *  Render Engine for framebuffer devices
 *
 *  Copyright (c) 2008 CodeSourcery
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "hw.h"
//#include "console.h"
#include "gui.h"
#include "fb_render_engine.h"
#include "pixel_ops.h"

typedef void (*row_draw_fn)(uint32_t *, uint8_t *, const uint8_t *, int, int);

enum fb_dest_bpp_mode
{
    BPP_DEST_8,
    BPP_DEST_15,
    BPP_DEST_16,
    BPP_DEST_24,
    BPP_DEST_32
};

struct render_data_t {
    int base_is_in_target;
    union {
        void* base_host;
        target_phys_addr_t base_target;
    };
    uint8_t *dest;
    uint32_t cols;
    uint32_t rows;
    uint32_t row_pitch;
    /* The following attributes refer to the guest side.
       They should be renamed once they are also variable
       at the host side.
    */
    enum fb_pixel_order pixel_order;
    enum fb_byte_order  byte_order;
    enum fb_color_order color_order;
    enum fb_src_bpp_mode  src_bpp_mode;

    enum fb_dest_bpp_mode dest_bpp_mode;
    /* pending:
        pixel_order, byte_order, and color_order, for the dest (host).
    */
    enum fb_rotation orientation;
    uint32_t blank : 1;
    uint32_t need_internal_update : 1;
    uint32_t raw_palette[256];

    /* ====== cached internal data ===== */
    uint32_t inter_src_row_gap;
    uint32_t bytes_per_src_row; /* Does not include padding.  */
    row_draw_fn fn;
    /* rotation info */
    uint32_t dest_start_offset;
    uint32_t bytes_per_dest_row;
    int      dest_row_step;  /* in bytes */
    int      dest_col_step;  /* in bytes */
    int      swap_width_height;
    /* color info */
    uint32_t palette[256];

};


#define NOT_ASSIGNED -1

#include "fb_render_def.h"
#include "fb_render_decl.h"

/* getters */
uint32_t get_cols(const render_data *rd)
{
    return rd->cols;
}

uint32_t get_rows(const render_data *rd)
{
    return rd->rows;
}

uint32_t get_screen_width(const render_data *rd)
{
    return (!rd->swap_width_height) ? rd->cols : rd->rows;
}

uint32_t get_screen_height(const render_data *rd)
{
    return (!rd->swap_width_height) ? rd->rows : rd->cols;
}

enum fb_rotation get_orientation(const render_data *rd)
{
    return rd->orientation;
}

void* get_fb_base_in_host(const render_data *rd)
{
    return rd->base_is_in_target ? 0 : rd->base_host;
}

target_phys_addr_t get_fb_base_in_target(const render_data *rd)
{
    return rd->base_is_in_target ? rd->base_target : 0;
}

uint32_t get_blank_mode(const render_data *rd)
{
    return rd->blank;
}

enum fb_color_order get_color_order(const render_data *rd)
{
    return rd->color_order;
}

enum fb_byte_order get_byte_order(const render_data *rd)
{
    return rd->byte_order;
}

enum fb_pixel_order get_pixel_order(const render_data *rd)
{
    return rd->pixel_order;
}

enum fb_src_bpp_mode get_src_bpp(const render_data *rd)
{
    return rd->src_bpp_mode;
}

uint32_t get_row_pitch(const render_data *rd)
{
    return rd->row_pitch;
}

uint32_t get_palette_value(const render_data *rd, uint32_t n)
{
    return rd->raw_palette[n];
}


/* setters */
void set_cols(render_data *rd, uint32_t cols)
{
    rd->cols = cols; rd->need_internal_update = 1;
}

void set_rows(render_data *rd, uint32_t rows)
{
    rd->rows = rows; rd->need_internal_update = 1;
}

void set_orientation(render_data *rd, enum fb_rotation rotation)
{
    rd->orientation = rotation; rd->need_internal_update = 1;
}

void set_fb_base_from_host(render_data *rd, void* base)
{
    rd->base_is_in_target = 0;
    rd->base_host = base; rd->need_internal_update = 1;
}

void set_fb_base_from_target(render_data *rd, target_phys_addr_t base)
{
    rd->base_is_in_target = 1;
    /* ensure alignment */
    rd->base_target = (base & (~3)); rd->need_internal_update = 1;
}

void set_blank_mode(render_data *rd, int on_off)
{
    rd->blank = on_off; rd->need_internal_update = 1;
}

void set_pixel_order(render_data *rd, enum fb_pixel_order pixel_order)
{
    rd->pixel_order = pixel_order; rd->need_internal_update = 1;
}

void set_byte_order(render_data *rd, enum fb_byte_order byte_order)
{
    rd->byte_order = byte_order; rd->need_internal_update = 1;
}

void set_color_order(render_data *rd, enum fb_color_order color_order)
{
    rd->color_order = color_order; rd->need_internal_update = 1;
}

void set_src_bpp(render_data *rd, enum fb_src_bpp_mode src_bpp_mode)
{
    rd->src_bpp_mode = src_bpp_mode; rd->need_internal_update = 1;
}

void set_row_pitch(render_data *rd, uint32_t pitch)
{
    rd->row_pitch = pitch & ~3; rd->need_internal_update = 1;
}

static void set_dest_bpp(render_data *rd, enum fb_dest_bpp_mode dest_bpp_mode)
{
    rd->dest_bpp_mode = dest_bpp_mode; rd->need_internal_update = 1;
}

static void set_dest_bpp_from_ds(render_data *rd, const DisplayState *ds)
{
    enum fb_dest_bpp_mode new_dest_bpp_mode;

    switch (ds_get_bits_per_pixel(ds)) {
    case  8: new_dest_bpp_mode = BPP_DEST_8;  break;
    case 15: new_dest_bpp_mode = BPP_DEST_15; break;
    case 16: new_dest_bpp_mode = BPP_DEST_16; break;
    case 24: new_dest_bpp_mode = BPP_DEST_24; break;
    case 32: new_dest_bpp_mode = BPP_DEST_32; break;
    default:
        fprintf(stderr, "fb_render_engine: Bad color depth\n");
        exit(1);
    }

    if (rd->dest_bpp_mode != new_dest_bpp_mode)
        set_dest_bpp(rd, new_dest_bpp_mode);
}

static void set_dest_data_from_ds(render_data *rd, const DisplayState *ds)
{
    set_dest_bpp_from_ds(rd, ds);

    /* calculate row size */
    rd->bytes_per_dest_row = ds_get_linesize(ds);
}

/* this function rounds down odd widths */
static uint32_t calc_bytes_per_src_row(enum fb_src_bpp_mode bpp, uint32_t cols)
{
    uint32_t bytes_per_row = cols;

    switch (bpp) {
    case BPP_SRC_1:
        bytes_per_row >>= 3;
        break;

    case BPP_SRC_2:
        bytes_per_row >>= 2;
        break;

    case BPP_SRC_4:
        bytes_per_row >>= 1;
        break;

    case BPP_SRC_8:
        break;

    case BPP_SRC_15:
    case BPP_SRC_16:
        bytes_per_row <<= 1;
        break;

    case BPP_SRC_24:
        bytes_per_row *= 3;
        break;

    case BPP_SRC_32:
        bytes_per_row <<= 2;
        break;
    }

    if (bpp != BPP_SRC_24 && (bytes_per_row % 4) != 0) {
        fprintf(stderr, "Warning: setting an invalid width/depth combination, rounding down.\n");
        bytes_per_row &= ~3;
    }

    return bytes_per_row;
}

static uint32_t calc_bytes_per_dest_row(enum fb_dest_bpp_mode bpp, uint32_t cols)
{
    uint32_t bytes_per_row = cols;

    switch (bpp) {
    case BPP_DEST_8:
        break;
    case BPP_DEST_15:
    case BPP_DEST_16:
        bytes_per_row <<= 1;
        break;
    case BPP_DEST_24:
        bytes_per_row *= 3;
        break;
    case BPP_DEST_32:
        bytes_per_row <<= 2;
        break;
    }

    return bytes_per_row;
}

static row_draw_fn get_draw_fn(const render_data * rd)
{
    return fb_draw_fn[rd->dest_bpp_mode][rd->color_order][rd->byte_order][rd->pixel_order][rd->src_bpp_mode];
}

inline static void* calc_src_row_address_host(const render_data * rd, uint32_t row)
{
    return (void*)((char*)(rd->base_host) + row * (rd->bytes_per_src_row + rd->inter_src_row_gap));
}

inline static ram_addr_t calc_src_row_address_target(const render_data * rd, uint32_t row)
{
    target_phys_addr_t phys;
    phys = rd->base_target + row * (rd->bytes_per_src_row + rd->inter_src_row_gap);
    return get_ram_offset_phys(phys);
}

inline static uint8_t *calc_dest_row_address(const render_data * rd, int row)
{
    return rd->dest + rd->dest_start_offset + row * rd->dest_col_step;
}

static int is_dirty_row(const render_data * rd, uint32_t row)
{
    int dirty;
    uint32_t addr;
    uint32_t end;

    addr = calc_src_row_address_target(rd, row);
    end = (addr + rd->bytes_per_src_row - 1) & TARGET_PAGE_MASK;
    addr &= TARGET_PAGE_MASK;

    /* Iterate over all the pages belonging to this row */

    do {
        dirty = cpu_physical_memory_get_dirty(addr, VGA_DIRTY_FLAG);
        addr += TARGET_PAGE_SIZE;
    } while (!dirty && addr <= end);

    return dirty;
}

static void draw_blank_row(uint8_t *dest, uint32_t bytes)
{
    memset(dest, 0, bytes);
}


static void decode_rgb_for_palette(uint32_t data, unsigned int *r, unsigned int *g, unsigned int *b)
{
    *r = (data >> 16) & 0xff;
    *g = (data >> 8) & 0xff;
    *b = data & 0xff;
}

static void update_complete_palette(render_data * rd)
{
    int n, i;
    uint32_t raw;
    unsigned int r, g, b;

    switch (rd->src_bpp_mode) {
    case BPP_SRC_1: n = 2; break;
    case BPP_SRC_2: n = 4; break;
    case BPP_SRC_4: n = 16; break;
    case BPP_SRC_8: n = 256; break;
    default: return;
    }

    for (i=0; i<n; i++) {
        raw = rd->raw_palette[i];
        decode_rgb_for_palette(raw, &r, &g, &b);
        switch (rd->dest_bpp_mode) {
        case BPP_DEST_8:
            rd->palette[i] = rgb_to_pixel8(r, g, b);
            break;
        case BPP_DEST_15:
            rd->palette[i] = rgb_to_pixel15(r, g, b);
            break;
        case BPP_DEST_16:
            rd->palette[i] = rgb_to_pixel16(r, g, b);
            break;
        case BPP_DEST_24:
        case BPP_DEST_32:
            rd->palette[i] = rgb_to_pixel32(r, g, b);
            break;
        }
    }
}

/* Mapping from guest (row/col) to host (x/y) coordinates.  */ 
typedef struct {
    int row_x;
    int row_y;
    int col_x;
    int col_y;
} rotation_data;

static const rotation_data rotations[8] = {
      { 1,  0,  0,  1},
      { 0, -1,  1,  0},
      {-1,  0,  0, -1},
      { 0,  1, -1,  0},
      { 1,  0,  0, -1},
      { 0,  1,  1,  0},
      {-1,  0,  0,  1},
      { 0, -1, -1,  0},
};

static void update_rotation_data(render_data *rd)
{
    uint32_t bytes_per_dest_pixel;
    const rotation_data *r;

    r = &rotations[rd->orientation];

    /*rd->swap_width_height = rd->orientation & 1;*/

#if 0
    TODO DFG
    rd->bytes_per_dest_row = calc_bytes_per_dest_row(rd->dest_bpp_mode, get_screen_width(rd));
#endif
    bytes_per_dest_pixel = calc_bytes_per_dest_row(rd->dest_bpp_mode, get_screen_width(rd)) / get_screen_width(rd);

    /* Offset the start position if we are rendering backwards.  */
    rd->dest_start_offset = 0;
    if (r->row_x + r->col_x < 0)
        rd->dest_start_offset += bytes_per_dest_pixel * (get_screen_width(rd) - 1);
        /*rd->dest_start_offset += rd->bytes_per_dest_row - bytes_per_dest_pixel;*/

    if (r->row_y + r->col_y < 0)
        rd->dest_start_offset += rd->bytes_per_dest_row * (get_screen_height(rd) - 1);

    rd->dest_row_step = rd->bytes_per_dest_row * r->row_y
                        + bytes_per_dest_pixel * r->row_x;
    rd->dest_col_step = rd->bytes_per_dest_row * r->col_y
                        + bytes_per_dest_pixel * r->col_x;
}

static void update_render_data(render_data *rd)
{
    if (rd->need_internal_update) {
        rd->fn = get_draw_fn(rd);
        rd->bytes_per_src_row = calc_bytes_per_src_row(rd->src_bpp_mode,
                                                       rd->cols);
        update_complete_palette(rd);
        if (rd->row_pitch == 0)
            rd->inter_src_row_gap = 0;
        else
            rd->inter_src_row_gap = rd->row_pitch - rd->bytes_per_src_row;
        update_rotation_data(rd); /* updates bytes_per_dest_row too */
        rd->need_internal_update = 0;
    }
}


void set_palette_value(render_data *rd, uint32_t n, uint32_t value)
{
    rd->raw_palette[n] = value;
    rd->need_internal_update = 1;
}

static void render_blank_screen(render_data * rdata)
{
    int i;
    uint8_t* addr = rdata->dest;
    const uint32_t rows = get_screen_height(rdata);

    for (i = 0; i < rows; i++) {
        draw_blank_row(addr, rdata->bytes_per_dest_row);
        addr += rdata->bytes_per_dest_row;
    }
}

static void render_from_host(DisplayState *ds, const render_data *rdata)
{
    int i;

    for (i = 0; i < rdata->rows; i++) {
        rdata->fn(rdata->palette, 
                  calc_dest_row_address(rdata, i),
                  /*(uint8_t *)*/calc_src_row_address_host(rdata, i),
                  rdata->cols, 
                  rdata->dest_row_step);
    }

    if (!rdata->swap_width_height)
        dpy_update(ds, 0, 0, rdata->cols, rdata->rows);
    else
        dpy_update(ds, 0, 0, rdata->rows, rdata->cols);

}

static void render_from_target(DisplayState *ds, const render_data *rdata, int full_update)
{
    int first_dirty_row = NOT_ASSIGNED, last_dirty_row = 0;
    int i;

    for (i = 0; i < rdata->rows; i++) {
        if (full_update || is_dirty_row(rdata, i)) {
            /* FIXME: This is broken if it spans multiple RAM regions.  */
            rdata->fn(rdata->palette, 
                      calc_dest_row_address(rdata, i),
                      host_ram_addr(calc_src_row_address_target(rdata, i)),
                      rdata->cols, 
                      rdata->dest_row_step);

            if (first_dirty_row == NOT_ASSIGNED)
                first_dirty_row = i;
            last_dirty_row = i;
        }
    }

    if (first_dirty_row != NOT_ASSIGNED) {
        cpu_physical_memory_reset_dirty(
            calc_src_row_address_target(rdata, first_dirty_row), /* first row byte */
            calc_src_row_address_target(rdata, last_dirty_row + 1) - 1, /* last row byte */
            VGA_DIRTY_FLAG);

        if (!rdata->swap_width_height)
            dpy_update(ds, 0, first_dirty_row, rdata->cols, last_dirty_row - first_dirty_row + 1);
        else
            dpy_update(ds, first_dirty_row, 0, last_dirty_row - first_dirty_row + 1, rdata->cols);
    }
}

static int prepare_ds_for_rendering(DisplayState *ds, render_data *rdata)
{
    rdata->swap_width_height = rdata->orientation & 1;

    if (ds_get_width(ds) == get_screen_width(rdata) && 
        ds_get_height(ds) == get_screen_height(rdata))
        return 1;
    else
        return gui_resize_vt(ds, get_screen_width(rdata), get_screen_height(rdata));
}

void render(DisplayState *ds, render_data * rdata, int full_update)
{
    if (prepare_ds_for_rendering(ds, rdata)) {

        rdata->dest = ds_get_data(ds);
        
        set_dest_data_from_ds(rdata, ds);

        rdata->need_internal_update |= full_update;
        update_render_data(rdata);

        if (rdata->blank) {
            render_blank_screen(rdata);
        } else {
            if (rdata->base_is_in_target)
                render_from_target(ds, rdata, full_update);
            else
                render_from_host(ds, rdata);
        }
    }
}

render_data *create_render_data()
{
   render_data *rdata = qemu_mallocz(sizeof(render_data));

   /* set some defaults */

   return rdata;
}

void destroy_render_data(render_data *rd)
{
    qemu_free(rd);
}

void qemu_put_render_data(QEMUFile *f, const render_data *s)
{
    int i;

    qemu_put_be32(f, s->base_is_in_target);
    if (s->base_is_in_target)
        qemu_put_be64(f, s->base_target);
    qemu_put_be32(f, s->cols);
    qemu_put_be32(f, s->rows);
    qemu_put_be32(f, s->row_pitch);
    qemu_put_be32(f, s->pixel_order);
    qemu_put_be32(f, s->byte_order);
    qemu_put_be32(f, s->color_order);
    qemu_put_be32(f, s->src_bpp_mode);
    qemu_put_be32(f, s->orientation);
    qemu_put_be32(f, s->blank);
    for (i = 0; i < 256; i++)
        qemu_put_be32(f, s->raw_palette[i]);
}

void qemu_get_render_data(QEMUFile *f, render_data *s)
{
    int i;

    s->base_is_in_target = qemu_get_be32(f);
    if (s->base_is_in_target)
        s->base_target = qemu_get_be64(f);
    s->cols = qemu_get_be32(f);
    s->rows = qemu_get_be32(f);
    s->row_pitch = qemu_get_be32(f);
    s->pixel_order = qemu_get_be32(f);
    s->byte_order = qemu_get_be32(f);
    s->color_order = qemu_get_be32(f);
    s->src_bpp_mode = qemu_get_be32(f);
    s->orientation = qemu_get_be32(f);
    s->blank = qemu_get_be32(f);
    for (i = 0; i < 256; i++)
        s->raw_palette[i] = qemu_get_be32(f);
    s->need_internal_update = 1;
}
