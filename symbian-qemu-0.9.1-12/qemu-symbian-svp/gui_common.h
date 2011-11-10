/*
 * GUI Common internal definitions
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

#define X1_Y1

typedef struct
{
    int x0;
    int y0;
#if defined X1_Y1
    int x1;
    int y1;
#else
    int width;
    int height;
#endif
} gui_area_t;


#if defined X1_Y1
#   define GET_GUI_AREA_WIDTH(area_ptr)    ((area_ptr)->x1 - (area_ptr)->x0)
#   define GET_GUI_AREA_HEIGHT(area_ptr)   ((area_ptr)->y1 - (area_ptr)->y0)
#   define GET_GUI_AREA_X1(area_ptr)       (area_ptr)->x1)
#   define GET_GUI_AREA_Y1(area_ptr)       (area_ptr)->y1)
#   define SET_GUI_AREA_WIDTH(area_ptr,w)  (area_ptr)->x1 = (area_ptr)->x0 + (w)
#   define SET_GUI_AREA_HEIGHT(area_ptr,h) (area_ptr)->y1 = (area_ptr)->y0 + (h)
#   define SET_GUI_AREA_X0(area_ptr,x0)    (area_ptr)->x0 = (x0)
#   define SET_GUI_AREA_Y0(area_ptr,y0)    (area_ptr)->y0 = (y0)
#   define SET_GUI_AREA_X1(area_ptr,x1)    (area_ptr)->x1 = (x1)
#   define SET_GUI_AREA_Y1(area_ptr,y1)    (area_ptr)->y1 = (y1)
#elif defined WIDTH_HEIGHT
#   define GET_GUI_AREA_WIDTH(area_ptr)    (area_ptr)->width
#   define GET_GUI_AREA_HEIGHT(area_ptr)   (area_ptr)->height
#   define GET_GUI_AREA_X1(area_ptr)       ((area_ptr)->x0 + (area_ptr)->width)
#   define GET_GUI_AREA_Y1(area_ptr)       ((area_ptr)->y0 + (area_ptr)->height)
#   define SET_GUI_AREA_WIDTH(area_ptr,w)  (area_ptr)->width = (w)
#   define SET_GUI_AREA_HEIGHT(area_ptr,h) (area_ptr)->height = (h)
#   define SET_GUI_AREA_X0(area_ptr,x0)    { (area_ptr)->width += (area_ptr)->x0 - (x0); (area_ptr)->x0 = (x0); }
#   define SET_GUI_AREA_Y0(area_ptr,y0)    { (area_ptr)->height += (area_ptr)->y0 - (y0); (area_ptr)->y0 = (y0); }
#   define SET_GUI_AREA_X1(area_ptr,x1)    (area_ptr)->width = (x1) - (area_ptr)->x0
#   define SET_GUI_AREA_Y1(area_ptr,y1)    (area_ptr)->height = (y1) - (area_ptr)->y0
#else
#   error BAD DEFINITIONS IN GUI_COMMON.H
#endif

