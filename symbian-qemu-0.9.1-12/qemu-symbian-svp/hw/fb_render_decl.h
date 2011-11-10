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

#define N_DEST_BPP_MODES 5       /* 8, 15, 16, 24, 32 */
#define N_COLOR_ORDERS   2
#define N_BYTE_ORDERS    2
#define N_PIXEL_ORDERS   2
#define N_SRC_BPP_MODES  8

#define declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,src_bpp)	\
    fb_draw_line##src_bpp##_##byte_order##pixel_order##_##color_order##dest_bpp

#define declare_pixel_order(dest_bpp,color_order,byte_order,pixel_order)    \
{                                                                           \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,1),    \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,2),    \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,4),    \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,8),    \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,15),   \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,16),   \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,24),   \
	declare_src_bpp_mode(dest_bpp,color_order,byte_order,pixel_order,32),   \
}

#define declare_byte_order(dest_bpp,color_order,byte_order)     \
{                                                               \
	declare_pixel_order(dest_bpp,color_order,byte_order,lp),    \
	declare_pixel_order(dest_bpp,color_order,byte_order,bp)     \
}

#define declare_color_order(dest_bpp,color_order)   \
{                                                   \
	declare_byte_order(dest_bpp,color_order,lb),    \
	declare_byte_order(dest_bpp,color_order,bb)     \
}

#define declare_dest_bpp_mode(dest_bpp) \
{                                       \
    declare_color_order(dest_bpp,bgr),  \
    declare_color_order(dest_bpp,rgb)   \
}

static row_draw_fn fb_draw_fn[N_DEST_BPP_MODES][N_COLOR_ORDERS][N_BYTE_ORDERS][N_PIXEL_ORDERS][N_SRC_BPP_MODES] =
{
    declare_dest_bpp_mode(8),
    declare_dest_bpp_mode(15),
    declare_dest_bpp_mode(16),
    declare_dest_bpp_mode(24),
    declare_dest_bpp_mode(32)
};

