#include <stdio.h>
#include <stdlib.h>
#include "algorithm.h"

void yuyv2rgb(unsigned char *yuyv, unsigned char *rgb, int width, int height)
{
	int frame_size = width * height * 2;
	int rgb_index = 0;
	int i;

	for (i = 0; i < frame_size; i += 4) {
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
