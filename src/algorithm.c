#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "algorithm.h"

rgb_image* decode_jpeg(const unsigned char *jpeg_data, size_t jpeg_size) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpeg_data, jpeg_size);

	int rc = jpeg_read_header(&cinfo, TRUE);
	if (rc != 1) {
		fprintf(stderr, "file does not seem to be a normal jpeg\n");
		return NULL;
	}

	jpeg_start_decompress(&cinfo);

	int width = cinfo.output_width;
	int height = cinfo.output_height;
	int pixel_size = cinfo.output_components;

	unsigned long bmp_size = width * height * pixel_size;
	unsigned char *bmp_buffer = (unsigned char*) malloc(bmp_size);

	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * width * pixel_size;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	rgb_image *image = (rgb_image *)malloc(sizeof(rgb_image));
	image->data = bmp_buffer;
	image->width = width;
	image->height = height;

	return image;
}

void free_rgb_image(rgb_image *image) {
	if (image) {
		free(image->data);
		free(image);
	}
}
