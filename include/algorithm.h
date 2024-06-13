#ifndef _ALGORITHM_H
#define _ALGORITHM_H
typedef struct {
	unsigned char *data;
	int width;
	int height;
} rgb_image;

int mjpeg2rgb(unsigned char* mjpeg_data, long size, unsigned char* rgb_data, int width, int height);
void yuyv2rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height);
#endif
