#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "algorithm.h"

rgb_image* mjpeg2rgb(const unsigned char *jpeg_data, size_t jpeg_size) {
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

void yuyv_to_rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height)
{
	int frame_size = width * height * 2;
	int rgb_index = 0;

	for (int i = 0; i < frame_size; i += 4) {
		int y1 = yuyv[i];
		int u = yuyv[i + 1] - 128;
		int y2 = yuyv[i + 2];
		int v = yuyv[i + 3] - 128;

		int c1 = y1 - 16;
		int c2 = y2 - 16;
		int d = u;
		int e = v;
		//int d = 0;
		//int e = 0;

		int r1 = (298 * c1 + 409 * e + 128) >> 8;
		int g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
		int b1 = (298 * c1 + 516 * d + 128) >> 8;

		int r2 = (298 * c2 + 409 * e + 128) >> 8;
		int g2 = (298 * c2 - 100 * d - 208 * e + 128) >> 8;
		int b2 = (298 * c2 + 516 * d + 128) >> 8;

		rgb[rgb_index++] = r1 > 255 ? 255 : r1 < 0 ? 0 : r1;
		rgb[rgb_index++] = g1 > 255 ? 255 : g1 < 0 ? 0 : g1;
		rgb[rgb_index++] = b1 > 255 ? 255 : b1 < 0 ? 0 : b1;

		rgb[rgb_index++] = r2 > 255 ? 255 : r2 < 0 ? 0 : r2;
		rgb[rgb_index++] = g2 > 255 ? 255 : g2 < 0 ? 0 : g2;
		rgb[rgb_index++] = b2 > 255 ? 255 : b2 < 0 ? 0 : b2;
	}
}
