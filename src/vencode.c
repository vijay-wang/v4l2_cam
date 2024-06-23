#include "vencode.h"
#include "level_log.h"

x264_t* init_x264_encoder(x264_param_t *param, x264_picture_t *pic_in, unsigned int width, unsigned int height)
{
	int csp = X264_CSP_I420;

	x264_param_default_preset(param, "placebo", "zerolatency");
	param->i_width = width;
	param->i_height = height;
	param->i_fps_num = 30;
	param->i_fps_den = 1;
	param->i_csp = X264_CSP_I420;


	if (x264_param_apply_profile(param, "baseline") < 0) {
		LOG_ERROR("Failed to set profile\n");
		return;
	}

	//open encoder
	x264_t *encoder = x264_encoder_open(param);
	if (!encoder) {
		LOG_ERROR("Failed to open encoder\n");
		return;
	}
	x264_picture_alloc(pic_in, param->i_csp, param->i_width, param->i_height);
	return encoder;
}
