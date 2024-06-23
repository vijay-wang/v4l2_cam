#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


// 将5位颜色分量转换为8位
static inline unsigned convert_5bit_to_8bit(unsigned color) {
	return (color << 3) | (color >> 2);
}

// 将6位颜色分量转换为8位
static inline unsigned convert_6bit_to_8bit(unsigned color) {
	return (color << 2) | (color >> 4);
}

// 将RGB565_LE格式转换为RGB888格式
void rgb565le2rgb888(rgb565le *prgb565le, rgb888 *prgb888, unsigned int width, unsigned int height)
{
	unsigned int num_pixels = width * height;
	int i;

	for (i = 0; i < num_pixels; ++i) {
		rgb565le pixel = prgb565le[i];

		// 提取红色、绿色和蓝色分量
		unsigned r5 = (pixel >> 11) & 0x1F;
		unsigned g6 = (pixel >> 5) & 0x3F;
		unsigned b5 = pixel & 0x1F;

		// 转换为8位颜色分量
		unsigned r8 = convert_5bit_to_8bit(r5);
		unsigned g8 = convert_6bit_to_8bit(g6);
		unsigned b8 = convert_5bit_to_8bit(b5);

		// 将结果存储到RGB888缓冲区
		prgb888[i][0] = r8;
		prgb888[i][1] = g8;
		prgb888[i][2] = b8;
	}
}

void yuyv_to_yuv420p(unsigned char *yuyv, x264_picture_t *pic, unsigned int width, unsigned int height)
{
	unsigned char *y = pic->img.plane[0];
	unsigned char *u = pic->img.plane[1];
	unsigned char *v = pic->img.plane[2];
	int j, i;

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i += 2) {
			int index = j * width + i;
			int y_index = j * width + i;
			int u_index = (j >> 2) * (width >> 2) + (i >> 2);
			int v_index = (j >> 2) * (width >> 2) + (i >> 2);

			y[y_index] = yuyv[index << 2];
			y[y_index + 1] = yuyv[index << 2 + 2];
			u[u_index] = yuyv[index << 2 + 1];
			v[v_index] = yuyv[index << 2 + 3];
		}
	}
}
