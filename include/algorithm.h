#ifndef _ALGORITHM_H
#define _ALGORITHM_H
typedef struct {
	unsigned char *data;
	int width;
	int height;
} rgb_image;

rgb_image* mjpeg2rgb(const unsigned char *jpeg_data, size_t jpeg_size);
void free_rgb_image(rgb_image *image);

void yuyv2rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height);
#endif
