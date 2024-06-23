#ifndef _VENCODE_H
#define _VENCODE_H
#include <stdint.h>
#include "x264.h"

x264_t* init_x264_encoder(x264_param_t *param, x264_picture_t *pic_in, unsigned int width, unsigned int height);

#endif
