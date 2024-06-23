#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "x264.h"
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

// YUYV to I420 (YUV420P) conversion function
void yuyv_to_i420(const unsigned char *yuyv, unsigned char *i420, unsigned int width, unsigned int height) {
	unsigned int frame_size = width * height;
	unsigned char *y = i420;
	unsigned char *u = i420 + frame_size;
	unsigned char *v = i420 + frame_size + frame_size / 4;
	unsigned int j;
	unsigned int i;

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i += 2) {
			unsigned int yuyv_index = (j * width + i) * 2;
			unsigned int i420_index = j * width + i;

			y[i420_index] = yuyv[yuyv_index];
			y[i420_index + 1] = yuyv[yuyv_index + 2];

			if (j % 2 == 0 && i % 2 == 0) {
				unsigned int uv_index = (j / 2) * (width / 2) + (i / 2);
				u[uv_index] = yuyv[yuyv_index + 1];
				v[uv_index] = yuyv[yuyv_index + 3];
			}
		}
	}
}

void yuyv2h264(unsigned char *yuyv, unsigned char *h264, unsigned int width, unsigned int height) {
    // Initialize x264 encoder
    x264_param_t param;
    x264_t *encoder;
    x264_picture_t pic_in, pic_out;
    int frame_size = width * height;
    int csp = X264_CSP_I420;

    x264_param_default_preset(&param, "medium", "zerolatency");
    param.i_width = width;
    param.i_height = height;
    param.i_csp = csp;
    param.rc.i_rc_method = X264_RC_CRF;
    param.rc.f_rf_constant = 25;
    param.rc.f_rf_constant_max = 35;
    param.i_fps_num = 30;
    param.i_fps_den = 1;
    param.b_vfr_input = 0;

    if (x264_param_apply_profile(&param, "high") < 0) {
        fprintf(stderr, "Failed to set profile\n");
        return;
    }

    encoder = x264_encoder_open(&param);
    if (!encoder) {
        fprintf(stderr, "Failed to open encoder\n");
        return;
    }

    x264_picture_alloc(&pic_in, csp, width, height);
    pic_in.img.i_csp = csp;
    pic_in.img.i_plane = 3;

    // Convert YUYV to I420
    unsigned char *i420_frame = (unsigned char *)malloc(frame_size * 3 / 2);
    if (!i420_frame) {
        fprintf(stderr, "Failed to allocate I420 frame\n");
        return;
    }

    yuyv_to_i420(yuyv, i420_frame, width, height);

    memcpy(pic_in.img.plane[0], i420_frame, frame_size);          // Y
    memcpy(pic_in.img.plane[1], i420_frame + frame_size, frame_size / 4);  // U
    memcpy(pic_in.img.plane[2], i420_frame + frame_size + frame_size / 4, frame_size / 4);  // V

    free(i420_frame);

    // Encode frame
    x264_nal_t *nals;
    int i_nals;
    int frame_size_out = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
    if (frame_size_out < 0) {
        fprintf(stderr, "Failed to encode frame\n");
        return;
    }

    // Copy encoded frame to output buffer
    memcpy(h264, nals[0].p_payload, frame_size_out);

    // Clean up
    x264_picture_clean(&pic_in);
    x264_encoder_close(encoder);
}
