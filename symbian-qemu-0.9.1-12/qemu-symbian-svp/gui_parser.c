/*
 * GUI XML parser
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

#include "stdio.h"
#include "string.h"
#include "stddef.h"
#include "qemu-common.h"
#include "gui_common.h"
#include "gui_parser.h"
#include "expat.h"

typedef enum {
    WAITING_GUI_START,
    WAITING_VT_START,
    WAITING_VT_CONTENT,
    WAITING_BUTTON_END,
    WAITING_PAREA_END,  /* Pointer Area */
    WAITING_DAREA_END,  /* Display Area */
    /* leave this at the end */
    N_PARSER_STATES
} parser_state_t;

typedef struct
{
    XML_Parser parser;
    parse_result_t result;
    parser_state_t state;
    gui_xml_tree_t *gui_xml_tree;

    vt_node_t           *last_vt_node;
    /* the following data points to the last vt node */
    image_node_t        *last_image_node;
    displayarea_node_t  *last_displayarea_node;
    pointerarea_node_t  *last_pointerarea_node;
    button_node_t       *last_button_node;

} parser_data_t;

typedef struct
{
    void (*start)(parser_data_t *pdata, const char *element, const char **attr);
    void (*end)(parser_data_t *pdata, const char *element);
} parser_state_handler_t;

typedef struct
{
    gui_area_t area;
    int height;
    int width;
} tmp_area_t;

#define HANDLER_NAME_START(state)       parser_##state##_start_handler
#define HANDLER_START_SIGNATURE         (parser_data_t *pdata, const char *element, const char **attr)
#define START_HANDLER_DECL(state)       static void HANDLER_NAME_START(state) HANDLER_START_SIGNATURE

#define HANDLER_NAME_END(state)         parser_##state##_end_handler
#define HANDLER_END_SIGNATURE           (parser_data_t *pdata, const char *element)
#define END_HANDLER_DECL(state)         static void HANDLER_NAME_END(state) HANDLER_END_SIGNATURE

#define FORWARD_DECL_STATE(state)   \
    START_HANDLER_DECL(state);      \
    END_HANDLER_DECL(state)

#define HANDLER_PAIR(state) &HANDLER_NAME_START(state), &HANDLER_NAME_END(state)

FORWARD_DECL_STATE(waiting_gui_start);
FORWARD_DECL_STATE(waiting_vt_start);
FORWARD_DECL_STATE(waiting_vt_content);
FORWARD_DECL_STATE(waiting_button_end);
FORWARD_DECL_STATE(waiting_parea_end);
FORWARD_DECL_STATE(waiting_darea_end);

static const parser_state_handler_t state_handler[N_PARSER_STATES] = {
    { HANDLER_PAIR(waiting_gui_start)  },
    { HANDLER_PAIR(waiting_vt_start)   },
    { HANDLER_PAIR(waiting_vt_content) },
    { HANDLER_PAIR(waiting_button_end) },
    { HANDLER_PAIR(waiting_parea_end)  },
    { HANDLER_PAIR(waiting_darea_end)  }
};

/**************************/

const char* parsing_location_error(void)
{
    return "";
}

static inline int attribute_is(const char *attr, const char *name)
{
    return (strcmp(attr, name) == 0);
}

static void clear_tarea(tmp_area_t *tarea)
{
    memset(tarea, 0, sizeof(tmp_area_t));
}

static int assign_bool_value(const char *value, int* res)
{
    int is_bool_value = 1;

    if (strcmp(value, "1") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "YES") == 0)
        *res = 1;
    else if (strcmp(value, "0") == 0 || strcmp(value, "no") == 0 || strcmp(value, "NO") == 0)
        *res = 0;
    else {
        is_bool_value = 0;
        fprintf(stderr, "Warning: unrecognized bool value '%s'\n", value);
    }

    return is_bool_value;
}

static int assign_tarea_attribute(const char *full_attribute, const char *value, tmp_area_t *tarea, const char *attribute_prefix)
{
    int is_area_attribute = 1;
    const char *attribute;
    int int_value = atol(value);
    int was_negative = 0;

    if (int_value < 0) {
        was_negative = 1;
        int_value = 0;
    }

    if (attribute_prefix != NULL) {
        const int prefix_len = strlen(attribute_prefix);
        if (strncmp(attribute_prefix, full_attribute, prefix_len) == 0)
            attribute = full_attribute + prefix_len;
        else
            return 0;
    } else
        attribute = full_attribute;


    if (attribute_is(attribute, "x0") || attribute_is(attribute, "x"))
        tarea->area.x0 = int_value;
    else if (attribute_is(attribute, "x1"))
        tarea->area.x1 = int_value;
    else if (attribute_is(attribute, "y0") || attribute_is(attribute, "y"))
        tarea->area.y0 = int_value;
    else if (attribute_is(attribute, "y1"))
        tarea->area.y1 = int_value;
    else if (attribute_is(attribute, "width"))
        tarea->width = int_value;
    else if (attribute_is(attribute, "height"))
        tarea->height = int_value;
    else
        is_area_attribute = 0;

    if (was_negative && is_area_attribute) {        
        fprintf(stderr, "Warning: attribute '%s' has a negative value. Made it 0.\n", attribute);
    }

    return is_area_attribute;
}

static void assign_tarea_to_area(const tmp_area_t *tarea, gui_area_t *area)
{
    *area = tarea->area;
    if (tarea->height != 0) {
        if (area->y1 == 0)
            area->y1 = area->y0 + tarea->height;
        else
            fprintf(stderr, "Warning: both y1 and height specified. 'height' ignored.\n");
    }
    if (tarea->width != 0) {
        if (area->x1 == 0)
            area->x1 = area->x0 + tarea->width;
        else
            fprintf(stderr, "Warning: both x1 and width specified. 'width' ignored.\n");
    }
}

/* this is not for the first image, since it is the background */
static int allocate_image_node(parser_data_t *pdata)
{
    const int ret = pdata->last_vt_node->n_images;

    image_node_t* const node = (image_node_t*)qemu_mallocz(sizeof(image_node_t));

    pdata->last_image_node->next = node;
    pdata->last_image_node = node;

    pdata->last_vt_node->n_images++;

    return ret;
}

static void add_new_vt(parser_data_t *pdata, const char *element, const char **attr)
{
    vt_node_t *new_node;
    int attr_number = 0;
    image_node_t background_img;
    const char *value;

    new_node = (vt_node_t*)qemu_mallocz(sizeof(vt_node_t));

    if (new_node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        pdata->result = OUT_OF_MEMORY;
        return;
    }

    if (pdata->gui_xml_tree->first_vt_node == NULL)
        pdata->gui_xml_tree->first_vt_node = new_node;
    else
        pdata->last_vt_node->next = new_node;

    pdata->last_vt_node = new_node;
    pdata->gui_xml_tree->n_vts++;

    memset(&background_img, 0, sizeof(image_node_t));

    /* process attributes */
    while (attr[attr_number] != NULL) {
        value = attr[attr_number + 1];

        if (attribute_is(attr[attr_number], "background")) {
            strncpy(background_img.filename, value, sizeof(background_img.filename)-1);
            background_img.filename[sizeof(background_img.filename)-1] = 0;
        } else if (attribute_is(attr[attr_number], "width")) {
            SET_GUI_AREA_WIDTH(&background_img.area, atol(value));
        } else if (attribute_is(attr[attr_number], "height")) {
            SET_GUI_AREA_HEIGHT(&background_img.area, atol(value));
        } else {
            fprintf(stderr, "Unrecognized vt attribute '%s'.\n", attr[attr_number]);
        }

        attr_number += 2;
    }

    if (background_img.filename[0] != 0) {
        new_node->first_image_node = (image_node_t*)qemu_malloc(sizeof(image_node_t));
        *(new_node->first_image_node) = background_img;
        new_node->n_images = 1;
        pdata->last_image_node = new_node->first_image_node;
    } else {
        fprintf(stderr, "Error: background image missing for vt.\n");
        pdata->result = UNEXPECTED_EOF;
    }
}

static void add_new_button(parser_data_t *pdata, const char *element, const char **attr)
{
    button_node_t *new_node = (button_node_t*)qemu_mallocz(sizeof(button_node_t));
    tmp_area_t tarea;
    tmp_area_t tarea_pressed;
    image_node_t *pressed_img = NULL;
    const char *attribute, *value;
    int attr_number = 0;

    if (new_node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        pdata->result = OUT_OF_MEMORY;
        return;
    }

    if (pdata->last_vt_node->first_button_node == NULL)
        pdata->last_vt_node->first_button_node = new_node;
    else
        pdata->last_button_node->next = new_node;

    pdata->last_button_node = new_node;
    pdata->last_vt_node->n_buttons++;

#define NOT_ASSIGNED    -1

    clear_tarea(&tarea);
    clear_tarea(&tarea_pressed);
    tarea_pressed.area.x0 = tarea_pressed.area.y0 = NOT_ASSIGNED;

    /* process attributes */
    while (attr[attr_number] != NULL) {
        attribute = attr[attr_number];
        value = attr[attr_number + 1];

        if (attribute_is(attribute, "pressedimg")) {
            if (pressed_img == NULL) {
                new_node->pressed_img_id = allocate_image_node(pdata);
                pressed_img = pdata->last_image_node;
            }
            strncpy(pressed_img->filename, value, sizeof(pressed_img->filename));
            pressed_img->filename[sizeof(pressed_img->filename)-1] = 0;
        } else if (attribute_is(attribute, "action")) {
            strncpy(new_node->action, value, 15);
            new_node->action[15] = 0;
        } else if (attribute_is(attribute, "parameter")) {
            strncpy(new_node->parameter, value, 31);
            new_node->parameter[31] = 0;
        } else if (assign_tarea_attribute(attribute, value, &tarea_pressed, "pressedimg_")) {
            if (pressed_img == NULL) {
                new_node->pressed_img_id = allocate_image_node(pdata);
                pressed_img = pdata->last_image_node;
            }
        } else if (!assign_tarea_attribute(attribute, value, &tarea, NULL)) {
            fprintf(stderr, "Warning: unrecognized button attribute '%s'\n", attribute);
        }

        attr_number += 2;
    }
 
    assign_tarea_to_area(&tarea, &new_node->area);

    if (pressed_img != NULL) {
        if (tarea_pressed.area.x0 == NOT_ASSIGNED)
            tarea_pressed.area.x0 = tarea.area.x0;

        if (tarea_pressed.area.y0 == NOT_ASSIGNED)
            tarea_pressed.area.y0 = tarea.area.y0;

        assign_tarea_to_area(&tarea_pressed, &pressed_img->area);        
    }

#undef  NOT_ASSIGNED
}

static void add_new_parea(parser_data_t *pdata, const char *element, const char **attr)
{
    pointerarea_node_t *new_node = (pointerarea_node_t*)qemu_mallocz(sizeof(pointerarea_node_t));
    tmp_area_t tarea;
    const char *attribute, *value;
    int attr_number = 0;

    if (new_node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        pdata->result = OUT_OF_MEMORY;
        return;
    }

    if (pdata->last_vt_node->first_pointerarea_node == NULL)
        pdata->last_vt_node->first_pointerarea_node = new_node;
    else
        pdata->last_pointerarea_node->next = new_node;

    pdata->last_pointerarea_node = new_node;
    pdata->last_vt_node->n_pointerareas++;

    clear_tarea(&tarea);

    /* process attributes */
    while (attr[attr_number] != NULL) {
        attribute = attr[attr_number];
        value = attr[attr_number + 1];

        if (attribute_is(attribute, "devid")) {
            strncpy(new_node->devid, value, sizeof(devid_t)-1);
            new_node->devid[sizeof(devid_t)-1] = 0;
        } else if (attribute_is(attribute, "grabonclick")) {
            assign_bool_value(value, &new_node->grab_on_click);
        } else if (attribute_is(attribute, "absolute")) {
            assign_bool_value(value, &new_node->absolute);
        } else if (attribute_is(attribute, "grabbingcursor")) {
            assign_bool_value(value, &new_node->show_cursor_while_grabbing);
        } else if (!assign_tarea_attribute(attribute, value, &tarea, NULL)) {
            fprintf(stderr, "Warning: unrecognized displayarea attribute '%s'\n", attribute);
        }

        attr_number += 2;
    }

    assign_tarea_to_area(&tarea, &new_node->area);    
}

static void add_new_darea(parser_data_t *pdata, const char *element, const char **attr)
{
    displayarea_node_t *new_node = (displayarea_node_t*)qemu_mallocz(sizeof(displayarea_node_t));
    tmp_area_t tarea;
    const char *attribute, *value;
    int attr_number = 0;

    if (new_node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        pdata->result = OUT_OF_MEMORY;
        return;
    }

    if (pdata->last_vt_node->first_displayarea_node == NULL)
        pdata->last_vt_node->first_displayarea_node = new_node;
    else
        pdata->last_displayarea_node->next = new_node;

    pdata->last_displayarea_node = new_node;
    pdata->last_vt_node->n_displayareas++;

    clear_tarea(&tarea);

    /* process attributes */
    while (attr[attr_number] != NULL) {
        attribute = attr[attr_number];
        value = attr[attr_number + 1];

        if (attribute_is(attribute, "devid")) {
            strncpy(new_node->devid, value, sizeof(devid_t)-1);
            new_node->devid[sizeof(devid_t)-1] = 0;
        } else if (!assign_tarea_attribute(attribute, value, &tarea, NULL)) {
            fprintf(stderr, "Warning: unrecognized displayarea attribute '%s'\n", attribute);
        }

        attr_number += 2;
    }

    assign_tarea_to_area(&tarea, &new_node->area);

}

START_HANDLER_DECL(waiting_gui_start)
{
    if (strcmp(element, "gui") == 0) {
        pdata->state = WAITING_VT_START;
    } else {
        fprintf(stderr, "Unexpected element '%s' in line %ld; expected <gui>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

END_HANDLER_DECL(waiting_gui_start)
{
    fprintf(stderr, "Unexpected closing element '%s' in line %ld\n",
            element, 
            XML_GetCurrentLineNumber(pdata->parser));
}

START_HANDLER_DECL(waiting_vt_start)
{
    if (strcmp(element, "vt") == 0) {
        pdata->state = WAITING_VT_CONTENT;
        add_new_vt(pdata, element, attr);
    } else {
        fprintf(stderr, "Unexpected element '%s' in line %ld; expected <vt>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

END_HANDLER_DECL(waiting_vt_start)
{
    if (strcmp(element, "gui") == 0) {
        /* OK! */
    } else {
        fprintf(stderr, "Unexpected closing element '%s' in line %ld; expected </gui>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

START_HANDLER_DECL(waiting_vt_content)
{
    if (strcmp(element, "button") == 0) {
        pdata->state = WAITING_BUTTON_END;
        add_new_button(pdata, element, attr);
    } else if (strcmp(element, "displayarea") == 0) {
        pdata->state = WAITING_DAREA_END;
        add_new_darea(pdata, element, attr);
    } else if (strcmp(element, "pointerarea") == 0) {
        pdata->state = WAITING_PAREA_END;
        add_new_parea(pdata, element, attr);
    } else {
        fprintf(stderr, "Unexpected element '%s' in line %ld; expected <button>, <displayarea>, or <pointerarea>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

END_HANDLER_DECL(waiting_vt_content)
{
    if (strcmp(element, "vt") == 0) {
        pdata->state = WAITING_VT_START;
    } else {
        fprintf(stderr, "Unexpected closing element '%s' in line %ld; expected </vt>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

START_HANDLER_DECL(waiting_button_end)
{
    fprintf(stderr, "Unexpected element '%s' in line %ld; expected closing </button>\n",
            element, 
            XML_GetCurrentLineNumber(pdata->parser));
}

END_HANDLER_DECL(waiting_button_end)
{
    if (strcmp(element, "button") == 0) {
        pdata->state = WAITING_VT_CONTENT;
    } else {
        fprintf(stderr, "Unexpected closing element '%s' in line %ld; expected </button>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

START_HANDLER_DECL(waiting_parea_end)
{
    fprintf(stderr, "Unexpected element '%s' in line %ld; expected closing </pointerarea>\n",
            element, 
            XML_GetCurrentLineNumber(pdata->parser));
}

END_HANDLER_DECL(waiting_parea_end)
{
    if (strcmp(element, "pointerarea") == 0) {
        pdata->state = WAITING_VT_CONTENT;
    } else {
        fprintf(stderr, "Unexpected closing element '%s' in line %ld; expected </pointerarea>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

START_HANDLER_DECL(waiting_darea_end)
{
    fprintf(stderr, "Unexpected element '%s' in line %ld; expected closing </displayarea>\n",
            element, 
            XML_GetCurrentLineNumber(pdata->parser));
}

END_HANDLER_DECL(waiting_darea_end)
{
    if (strcmp(element, "displayarea") == 0) {
        pdata->state = WAITING_VT_CONTENT;
    } else {
        fprintf(stderr, "Unexpected closing element '%s' in line %ld; expected </displayarea>\n",
                element, 
                XML_GetCurrentLineNumber(pdata->parser));
    }
}

/**************************/

static void parser_start_hndl(void *data, const char *element, const char **attr)
{
    parser_data_t *pdata = (parser_data_t*)data;

    state_handler[pdata->state].start(pdata, element, attr);
}


static void parser_end_hndl(void *data, const char *element)
{
    parser_data_t *pdata = (parser_data_t*)data;

    state_handler[pdata->state].end(pdata, element);
}

static void test_start_hndl(void *data, const char *element, const char **attr)
{
    fprintf(stderr, "start '%s'\n", element);
}


static void test_end_hndl(void *data, const char *element)
{
    fprintf(stderr, "end '%s'\n", element);
}

parse_result_t parse_gui_xml(const char *xml_file, gui_xml_tree_t *gui_xml_tree)
{
    parser_data_t parser_data;
    FILE *xml;
    int done = 0;
    char buffer[128];

    parser_data.parser = XML_ParserCreate(NULL);
    parser_data.result = PARSE_OK;
    parser_data.state = WAITING_GUI_START;
    parser_data.gui_xml_tree = gui_xml_tree;

    memset(gui_xml_tree, 0, sizeof(gui_xml_tree_t));

    if (parser_data.parser == NULL) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        return OUT_OF_MEMORY;
    }

    xml = fopen(xml_file, "r");
    if (xml == NULL) {
        fprintf(stderr, "XML file not found\n");
        XML_ParserFree(parser_data.parser);
        return FILE_NOT_FOUND;
    }

    XML_SetUserData(parser_data.parser, &parser_data);
    XML_SetElementHandler(parser_data.parser, parser_start_hndl, parser_end_hndl);

    do {
        if (fgets(buffer, sizeof(buffer), xml) == NULL)
            done = 1;
        else {
            done = feof(xml);
            if (! XML_Parse(parser_data.parser, buffer, strlen(buffer), done)) {
                fprintf(stderr, "Parse error at line %ld:\n%s\n",
                  XML_GetCurrentLineNumber(parser_data.parser),
                  XML_ErrorString(XML_GetErrorCode(parser_data.parser)));
                parser_data.result = UNEXPECTED_EOF;
            }
        }
    }while(!done && parser_data.result == PARSE_OK);

    if (done && !feof(xml)) {
        parser_data.result = UNEXPECTED_EOF;
    }

    fclose(xml);
    XML_ParserFree(parser_data.parser);

    return parser_data.result;
}

