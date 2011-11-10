/*
 * GUI PNG files processing
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

#include "png.h"
#include "qemu-common.h"
#include "gui_common.h"
#include "gui_png.h"

/*
int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;
*/

typedef struct
{
    int width;
    int height;
    void* image4c; /* in 4 channels */
} png_image_data_t;

static void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

#if 0
static void read_png_file_low(const char* file_name, png_image_data_t *png_image_data)
{
    png_structp png_ptr;
    int number_of_passes;
	char header[8];	// 8 is the maximum size that can be checked
    int y;

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp)
		abort_("[read_png_file] File %s could not be opened for reading", file_name);
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr)
		abort_("[read_png_file] png_create_read_struct failed");

	png_image_data->info_ptr = png_create_info_struct(png_ptr);
	if (!png_image_data->info_ptr)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");
	png_init_io(png_ptr, fp);

	png_set_sig_bytes(png_ptr, 8); /* 8 == sig_read ? */

	png_read_info(png_ptr, png_image_data->info_ptr);

    /* set transformations */
/*
	if ((png_image_data->info_ptr->color_type & PNG_COLOR_TYPE_RGB) == 0)
        transforms |= PNG_TRANSFORM_BGR;
*/
   if (png_get_valid(png_ptr, png_image_data->info_ptr, PNG_INFO_sBIT))
   {
      png_color_8p sig_bit;

      png_get_sBIT(png_ptr, png_image_data->info_ptr, &sig_bit);
      printf("sig_bit: %d\n", sig_bit);
      png_set_shift(png_ptr, sig_bit);
   }

    /* TODO DFG: is this needed? */
	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, png_image_data->info_ptr);





	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during read_image");

#if 1
	png_image_data->row_pointers = (png_bytep*) qemu_malloc(sizeof(png_bytep) * png_image_data->info_ptr->height);
	for (y=0; y < png_image_data->info_ptr->height; y++)
		png_image_data->row_pointers[y] = (png_byte*) qemu_malloc(png_image_data->info_ptr->rowbytes);

	png_read_image(png_ptr, png_image_data->row_pointers);
#endif
    /* DFG TODO: Cleanup */

/*    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL); */

    fclose(fp);
}
#endif

static void read_png_file(const char* file_name, png_image_data_t *png_image_data, int max_width, int max_height)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
	char header[8];	// 8 is the maximum size that can be checked
    int y;
    char* dest;

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp)
		abort_("[read_png_file] File %s could not be opened for reading", file_name);
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr)
		abort_("[read_png_file] png_create_read_struct failed");

    if (max_width > 0 && max_height > 0)
        png_set_user_limits(png_ptr, max_width, max_height);

	info_ptr = png_create_info_struct(png_ptr);

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");
	png_init_io(png_ptr, fp);

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");
   png_set_sig_bytes(png_ptr, sizeof(header));

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");
   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_ALPHA, png_voidp_NULL);

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");

    row_pointers = png_get_rows(png_ptr, info_ptr);

    png_image_data->height = png_get_image_height(png_ptr, info_ptr);
    png_image_data->width = png_get_image_width(png_ptr, info_ptr);

    png_image_data->image4c = (void*)qemu_malloc(png_image_data->width * png_image_data->height * 4);

    dest = (char*)png_image_data->image4c;

    /* transform this from 3 channels to a (fake for now) 4 channels */
	for (y=0; y < png_image_data->height; y++) {
        char* src = row_pointers[y];
        int x;
        for (x = 0; x < png_image_data->width; x++) {
            *dest = *src; dest++, src++;
            *dest = *src; dest++, src++;
            *dest = *src; dest++, src++;
            *dest = 0; dest++; /* alpha channel ignored */ 
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

    fclose(fp);
}



void gui_load_image_png(const char *filename, gui_image_t *image_data)
{
    const int area_height = GET_GUI_AREA_HEIGHT(&image_data->area);
    const int area_width  = GET_GUI_AREA_WIDTH(&image_data->area);
    png_image_data_t png_image_data;

    read_png_file(filename, &png_image_data, area_width, area_height);

    /* see if area has to be resized, because of any of these reasons:
        a) the image is smaller than the area
        b) the area is 0 (auto size for the background)
    */
    if (area_height > png_image_data.height || area_height <= 0)
        SET_GUI_AREA_HEIGHT(&image_data->area, png_image_data.height);
    if (area_width > png_image_data.width || area_width <= 0)
        SET_GUI_AREA_WIDTH(&image_data->area, png_image_data.width);

    image_data->image = png_image_data.image4c;
}

