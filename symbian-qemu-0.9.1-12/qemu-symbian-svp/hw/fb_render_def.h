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


/* This file builds the definition tree. */

/*
    Tree levels:
        DEFINING_DEST_BPP
        DEFINING_COLOR_ORDER
        DEFINING_BYTE_ORDER
        DEFINING_PIXEL_ORDER
*/

/* this is for the first time definitions */
#ifndef DEFINING_WHAT

#   define FB_LITTLE_ENDIAN 0
#   define FB_BIG_ENDIAN    1

#   define FB_CO_RGB    0
#   define FB_CO_BGR    1

#   define DEFINING_DEST_BPP        1
#   define DEFINING_SRC_COLOR_ORDER 2
#   define DEFINING_SRC_BYTE_ORDER  3
#   define DEFINING_SRC_PIXEL_ORDER 4
#   define DEFINING_FUNCTIONS       5   /* Tree leaf */

#   define DEFINING_WHAT    DEFINING_DEST_BPP   /* first definition level */
#endif


/* Begin tree:
    Each #if DEFINING_WHAT == XXX has one block per option of that level.
    For example, DEFINING_DEST_BPP will have 5 blocks (8,15,16,24,32).
    Each of these blocks has the same format:
        undef DEFINING_WHAT
        define DEFINING_WHAT (next level)
        define option
        include "fb_render_def.h"
        undef option
    So, at the end of each block, the only remaining definition is DEFINING_WHAT.

    New combining factors (i.e. more parameters for DEST, such as COLOR ORDER,
    BYTE ORDER, etc.) will have to be added as new blocks.
*/


#if DEFINING_WHAT == DEFINING_DEST_BPP

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_COLOR_ORDER
#   define DEST_BPP 8
#   include "fb_render_def.h"
#   undef DEST_BPP

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_COLOR_ORDER
#   define DEST_BPP 15
#   include "fb_render_def.h"
#   undef DEST_BPP

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_COLOR_ORDER
#   define DEST_BPP 16
#   include "fb_render_def.h"
#   undef DEST_BPP

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_COLOR_ORDER
#   define DEST_BPP 24
#   include "fb_render_def.h"
#   undef DEST_BPP

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_COLOR_ORDER
#   define DEST_BPP 32
#   include "fb_render_def.h"
#   undef DEST_BPP

#elif DEFINING_WHAT == DEFINING_SRC_COLOR_ORDER

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_BYTE_ORDER
#   define FB_SRC_COLOR_ORDER   FB_CO_BGR
#   include "fb_render_def.h"
#   undef FB_SRC_COLOR_ORDER

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_BYTE_ORDER
#   define FB_SRC_COLOR_ORDER   FB_CO_RGB
#   include "fb_render_def.h"
#   undef FB_SRC_COLOR_ORDER

#elif DEFINING_WHAT == DEFINING_SRC_BYTE_ORDER

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_PIXEL_ORDER
#   define FB_SRC_BYTE_ORDER    FB_LITTLE_ENDIAN
#   include "fb_render_def.h"
#   undef FB_SRC_BYTE_ORDER

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_SRC_PIXEL_ORDER
#   define FB_SRC_BYTE_ORDER    FB_BIG_ENDIAN
#   include "fb_render_def.h"
#   undef FB_SRC_BYTE_ORDER

#elif DEFINING_WHAT == DEFINING_SRC_PIXEL_ORDER

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_FUNCTIONS
#   define FB_SRC_PIXEL_ORDER    FB_LITTLE_ENDIAN
#   include "fb_render_def.h"
#   undef FB_SRC_PIXEL_ORDER

#   undef DEFINING_WHAT
#   define DEFINING_WHAT    DEFINING_FUNCTIONS
#   define FB_SRC_PIXEL_ORDER    FB_BIG_ENDIAN
#   include "fb_render_def.h"
#   undef FB_SRC_PIXEL_ORDER

#elif DEFINING_WHAT == DEFINING_FUNCTIONS
#   undef DEFINING_WHAT
    /* End of recursion. Include the template now */
#   include "fb_render_template.h"
#endif



