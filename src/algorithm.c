#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "algorithm.h"

RGBImage* decodeJPEG(const unsigned char *jpegData, size_t jpegSize) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpegData, jpegSize);

	int rc = jpeg_read_header(&cinfo, TRUE);
	if (rc != 1) {
		fprintf(stderr, "File does not seem to be a normal JPEG\n");
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

	RGBImage *image = (RGBImage *)malloc(sizeof(RGBImage));
	image->data = bmp_buffer;
	image->width = width;
	image->height = height;

	return image;
}

void freeRGBImage(RGBImage *image) {
	if (image) {
		free(image->data);
		free(image);
	}
}
