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

/* This file is included from fb_render_def.h
    At the moment of this inclusion, the following 
    preprocessor parameters are defined:
        DEST_BPP            { 8,15,16,24,32 }
        FB_SRC_COLOR_ORDER  { FB_CO_RGB, FB_CO_BGR }
        FB_SRC_BYTE_ORDER   { FB_LITTLE_ENDIAN, FB_BIG_ENDIAN }
        FB_SRC_PIXEL_ORDER  { FB_LITTLE_ENDIAN, FB_BIG_ENDIAN }
*/


/* The COPY_PIXEL macro does two things:
    1) Copies all the bytes involved in the pixel (depending on the DEST_BPP)
    2) Leaves the 'dest' parameter pointing to the first byte of next pixel.
        This depends also by the rotation parameter. The rotation modifies the
        'increment' bytes between pixels.
*/

#if DEST_BPP == 8
#	define COPY_PIXEL(to, from, increment) { *to = from; to += increment; }
#elif DEST_BPP == 15 || DEST_BPP == 16
#	define COPY_PIXEL(to, from, increment) { *(uint16_t *)to = from; to += increment; }
#elif DEST_BPP == 24
#	define COPY_PIXEL(to, from, increment) \
  		{ *to = from; to += increment; *to = (from) >> 8; to += increment; *to = (from) >> 16; to += increment; }
#elif DEST_BPP == 32
#	define COPY_PIXEL(to, from, increment) *(uint32_t *)to = from; to += increment;
#else
#	error unknown bit depth
#endif


/* Define the infixes of the functions name */

#if FB_SRC_COLOR_ORDER == FB_CO_RGB
#   define  SRC_COLOR_ORDER_INFIX   rgb
#else
#   define  SRC_COLOR_ORDER_INFIX   bgr
#endif

#if FB_SRC_BYTE_ORDER == FB_LITTLE_ENDIAN
#   define  SRC_BYTE_ORDER_INFIX    lb
#else
#   define  SRC_BYTE_ORDER_INFIX    bb
#endif

#if FB_SRC_PIXEL_ORDER == FB_LITTLE_ENDIAN
#   define  SRC_PIXEL_ORDER_INFIX    lp
#else
#   define  SRC_PIXEL_ORDER_INFIX    bp
#endif

#define MAKE_FN_SUFFIX(dest_bpp,color_order,byte_order,pixel_order)	\
    byte_order##pixel_order##_##color_order##dest_bpp

#define FN_SUFFIX   glue(SRC_BYTE_ORDER_INFIX,glue(SRC_PIXEL_ORDER_INFIX,glue(_,glue(SRC_COLOR_ORDER_INFIX,DEST_BPP))))

#ifdef WORDS_BIGENDIAN
#   define  FB_DEST_BYTE_ORDER  FB_BIG_ENDIAN
#else
#   define  FB_DEST_BYTE_ORDER  FB_LITTLE_ENDIAN
#endif


#if FB_SRC_BYTE_ORDER != FB_DEST_BYTE_ORDER
#   define  SWAP_WORDS  1
#endif

#if FB_SRC_BYTE_ORDER != FB_SRC_PIXEL_ORDER
#   define  SWAP_PIXELS 1
#endif


#define FN_2(x, y) FN(x, y) FN(x+1, y)
#define FN_4(x, y) FN_2(x, y) FN_2(x+2, y)
#define FN_8(y) FN_4(0, y) FN_4(4, y)


static void glue(fb_draw_line1_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    while (width > 0) {
        data = *(uint32_t *)src;
#ifdef SWAP_PIXELS
#   define FN(x, y) COPY_PIXEL(d, palette[(data >> (y + 7 - (x))) & 1], increment);
#else
#   define FN(x, y) COPY_PIXEL(d, palette[(data >> ((x) + y)) & 1], increment);
#endif
#ifdef SWAP_WORDS
        FN_8(24)
        FN_8(16)
        FN_8(8)
        FN_8(0)
#else
        FN_8(0)
        FN_8(8)
        FN_8(16)
        FN_8(24)
#endif
#undef FN
        width -= 32;
        src += 4;
    }
}

static void glue(fb_draw_line2_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    while (width > 0) {
        data = *(uint32_t *)src;
#ifdef SWAP_PIXELS
#define FN(x, y) COPY_PIXEL(d, palette[(data >> (y + 6 - (x)*2)) & 3], increment);
#else
#define FN(x, y) COPY_PIXEL(d, palette[(data >> ((x)*2 + y)) & 3], increment);
#endif
#ifdef SWAP_WORDS
        FN_4(0, 24)
        FN_4(0, 16)
        FN_4(0, 8)
        FN_4(0, 0)
#else
        FN_4(0, 0)
        FN_4(0, 8)
        FN_4(0, 16)
        FN_4(0, 24)
#endif
#undef FN
        width -= 16;
        src += 4;
    }
}

static void glue(fb_draw_line4_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    while (width > 0) {
        data = *(uint32_t *)src;
#ifdef SWAP_PIXELS
#   define FN(x, y) COPY_PIXEL(d, palette[(data >> (y + 4 - (x)*4)) & 0xf], increment);
#else
#   define FN(x, y) COPY_PIXEL(d, palette[(data >> ((x)*4 + y)) & 0xf], increment);
#endif
#ifdef SWAP_WORDS
        FN_2(0, 24)
        FN_2(0, 16)
        FN_2(0, 8)
        FN_2(0, 0)
#else
        FN_2(0, 0)
        FN_2(0, 8)
        FN_2(0, 16)
        FN_2(0, 24)
#endif
#undef FN
        width -= 8;
        src += 4;
    }
}

static void glue(fb_draw_line8_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    while (width > 0) {
        data = *(uint32_t *)src;
#define FN(x) COPY_PIXEL(d, palette[(data >> (x)) & 0xff], increment);
#ifdef SWAP_WORDS
        FN(24)
        FN(16)
        FN(8)
        FN(0)
#else
        FN(0)
        FN(8)
        FN(16)
        FN(24)
#endif
#undef FN
        width -= 4;
        src += 4;
    }
}

static void glue(fb_draw_line15_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    unsigned int r, g, b;
    while (width > 0) {
        data = *(uint32_t *)src;
#ifdef SWAP_WORDS
        data = bswap32(data);
#endif
#if FB_SRC_COLOR_ORDER == FB_CO_RGB
#   define LSB r
#   define MSB b
#else
#   define LSB b
#   define MSB r
#endif

        /* byte swapping done by bswap32 */
        LSB = (data & 0x1f) << 3;
        data >>= 5;
        g = (data & 0x1f) << 3;
        data >>= 5;
        MSB = (data & 0x1f) << 3;
        data >>= 6;
        COPY_PIXEL(d, glue(rgb_to_pixel,DEST_BPP)(r, g, b), increment);

        LSB = (data & 0x1f) << 3;
        data >>= 5;
        g = (data & 0x1f) << 3;
        data >>= 5;
        MSB = (data & 0x1f) << 3;
        COPY_PIXEL(d, glue(rgb_to_pixel,DEST_BPP)(r, g, b), increment);
#undef MSB
#undef LSB
        width -= 2;
        src += 4;
    }
}

static void glue(fb_draw_line16_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    unsigned int r, g, b;
    while (width > 0) {
        data = *(uint32_t *)src;
#ifdef SWAP_WORDS
        data = bswap32(data);
#endif
#if FB_SRC_COLOR_ORDER == FB_CO_RGB
#   define LSB r
#   define MSB b
#else
#   define LSB b
#   define MSB r
#endif

        /* byte swapping done by bswap32 */
        LSB = (data & 0x1f) << 3;
        data >>= 5;
        g = (data & 0x3f) << 2;
        data >>= 6;
        MSB = (data & 0x1f) << 3;
        data >>= 5;
        COPY_PIXEL(d, glue(rgb_to_pixel,DEST_BPP)(r, g, b), increment);

        LSB = (data & 0x1f) << 3;
        data >>= 5;
        g = (data & 0x3f) << 2;
        data >>= 6;
        MSB = (data & 0x1f) << 3;
        data >>= 5;
        COPY_PIXEL(d, glue(rgb_to_pixel,DEST_BPP)(r, g, b), increment);
#undef MSB
#undef LSB
        width -= 2;
        src += 4;
    }
}

static void glue(fb_draw_line24_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    unsigned int r, g, b;
    while (width > 0) {
#if FB_SRC_COLOR_ORDER == FB_CO_RGB
#   define LSB r
#   define MSB b
#else
#   define LSB b
#   define MSB r
#endif
        /* byte swapping not implemented in 24 bpp */
        LSB = *src; src++;
        g   = *src; src++;
        MSB = *src; src++;

        COPY_PIXEL(d, glue(rgb_to_pixel,DEST_BPP)(r, g, b), increment);
#undef MSB
#undef LSB
        width--;
    }
}

static void glue(fb_draw_line32_,FN_SUFFIX)(uint32_t *palette, uint8_t *d, const uint8_t *src, int width, int increment)
{
    uint32_t data;
    unsigned int r, g, b;
    while (width > 0) {
        data = *(uint32_t *)src;
#if FB_SRC_COLOR_ORDER == FB_CO_RGB
#   define LSB r
#   define MSB b
#else
#   define LSB b
#   define MSB r
#endif
#ifndef SWAP_WORDS
        LSB = data & 0xff;
        g = (data >> 8) & 0xff;
        MSB = (data >> 16) & 0xff;
#else
        LSB = (data >> 24) & 0xff;
        g = (data >> 16) & 0xff;
        MSB = (data >> 8) & 0xff;
#endif
        COPY_PIXEL(d, glue(rgb_to_pixel,DEST_BPP)(r, g, b), increment);
#undef MSB
#undef LSB
        width--;
        src += 4;
    }
}

/* Undefine all the local definitions, so they don't collide with the next time
   this file is included in the tree */

#undef COPY_PIXEL
#undef SWAP_PIXELS
#undef SWAP_WORDS
#undef FN_SUFFIX
#undef FB_DEST_BYTE_ORDER
#undef SRC_COLOR_ORDER_INFIX
#undef SRC_BYTE_ORDER_INFIX
#undef SRC_PIXEL_ORDER_INFIX


