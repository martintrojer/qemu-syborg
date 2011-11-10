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

/* Vertical flip happens after rotation.  */
enum fb_rotation
{
    ROT_0 = 0,
    ROT_90,
    ROT_180,
    ROT_270,
    ROT_0_V,
    ROT_90_V,
    ROT_180_V,
    ROT_270_V
};

enum fb_src_bpp_mode
{
    BPP_SRC_1,
    BPP_SRC_2,
    BPP_SRC_4,
    BPP_SRC_8,
    BPP_SRC_15,
    BPP_SRC_16,
    BPP_SRC_24,
    BPP_SRC_32
};

enum fb_byte_order
{
    BO_LE,
    BO_BE
};

enum fb_pixel_order
{
    PO_LE,
    PO_BE
};

enum fb_color_order
{
    CO_BGR,
    CO_RGB
};

struct render_data_t;
typedef struct render_data_t render_data;

/* constructor / destructor */
render_data *create_render_data(void);
void destroy_render_data(render_data *rd);

/* getters */
uint32_t get_cols(const render_data *rd);
uint32_t get_rows(const render_data *rd);
enum fb_rotation get_orientation(const render_data *rd);
void* get_fb_base_in_host(const render_data *rd);
#ifndef HOST_ONLY_DEFS
target_phys_addr_t get_fb_base_in_target(const render_data *rd);
#endif
uint32_t get_blank_mode(const render_data *rd);
enum fb_color_order get_color_order(const render_data *rd);
enum fb_byte_order get_byte_order(const render_data *rd);
enum fb_pixel_order get_pixel_order(const render_data *rd);
enum fb_src_bpp_mode get_src_bpp(const render_data *rd);
uint32_t get_row_pitch(const render_data *rd);
uint32_t get_palette_value(const render_data *rd, uint32_t n);

/* setters */
void set_cols(render_data *rd, uint32_t cols);
void set_rows(render_data *rd, uint32_t rows);
void set_orientation(render_data *rd, enum fb_rotation orientation);
void set_fb_base_from_host(render_data *rd, void* base);
#ifndef HOST_ONLY_DEFS
void set_fb_base_from_target(render_data *rd, target_phys_addr_t base);
#endif
void set_blank_mode(render_data *rd, int on_off);
void set_pixel_order(render_data *rd, enum fb_pixel_order pixel_order);
void set_byte_order(render_data *rd, enum fb_byte_order byte_order);
void set_color_order(render_data *rd, enum fb_color_order color_order);
void set_src_bpp(render_data *rd, enum fb_src_bpp_mode src_bpp_mode);
void set_row_pitch(render_data *rd, uint32_t pitch);
void set_palette_value(render_data *rd, uint32_t n, uint32_t value);

/* This function is used to render the screen on a DisplayState */
void render(DisplayState *ds, render_data * rd, int full_update);

/* Save/restore */
void qemu_put_render_data(QEMUFile *f, const render_data *s);
void qemu_get_render_data(QEMUFile *f, render_data *s);

