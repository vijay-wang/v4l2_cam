#ifndef _ALGORITHM_H
#define _ALGORITHM_H
typedef struct {
	unsigned char *data;
	int width;
	int height;
} rgb_image;

void yuyv2rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height);
#endif
