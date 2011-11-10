/*
 * GUI XML parser interface
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

typedef struct images_node_s
{
    struct images_node_s *next;
    char filename[128];
    gui_area_t area;
} image_node_t;

typedef char devid_t[64];

typedef struct button_node_s
{
    struct button_node_s *next;
    gui_area_t area;
    int pressed_img_id;
    char action[16];
    char parameter[32];
} button_node_t;

typedef struct displayarea_node_s
{
    struct displayarea_node_s *next;
    gui_area_t area;
    devid_t devid;
} displayarea_node_t;

typedef struct pointerarea_node_s
{
    struct pointerarea_node_s *next;
    gui_area_t area;
    devid_t devid;
    int absolute;
    int grab_on_click;
    int show_cursor_while_grabbing;
} pointerarea_node_t;

typedef struct vt_node_s
{
    struct vt_node_s    *next;
    image_node_t        *first_image_node;
    displayarea_node_t  *first_displayarea_node;
    pointerarea_node_t  *first_pointerarea_node;
    button_node_t       *first_button_node;
    unsigned int n_images;
    unsigned int n_displayareas;
    unsigned int n_pointerareas;
    unsigned int n_buttons;
} vt_node_t;


typedef struct
{
    vt_node_t    *first_vt_node;
    unsigned int n_vts;
} gui_xml_tree_t;

typedef enum {
    PARSE_OK = 0,
    UNRECOGNIZED_ELEMENT,
    UNEXPECTED_EOF,
    FILE_NOT_FOUND,
    INVALID_ID,
    OUT_OF_MEMORY,
} parse_result_t;

parse_result_t parse_gui_xml(const char *xml_file, gui_xml_tree_t *gui_xml_tree);
const char* parsing_location_error(void);


