#ifndef _ALGORITHM_H
#define _ALGORITHM_H
typedef struct {
	unsigned char *data;
	int width;
	int height;
} RGBImage;

RGBImage* decodeJPEG(const unsigned char *jpegData, size_t jpegSize);
void freeRGBImage(RGBImage *image);
#endif
