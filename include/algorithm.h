#ifndef _ALGORITHM_H
#define _ALGORITHM_H
#include <stdint.h>
#include "x264.h"
typedef struct {
	unsigned char *data;
	int width;
	int height;
} rgb_image;

typedef unsigned short rgb565le;
typedef unsigned char rgb888[3];
void yuyv2rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height);
void rgb565le2rgb888(rgb565le *prgb565le, rgb888 *prgb888, unsigned int width, unsigned int height);
void yuyv_to_yuv420p(unsigned char *yuyv, x264_picture_t *pic, unsigned int width, unsigned int height);
#endif
