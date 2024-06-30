#include "vencode.h"
#include "level_log.h"

x264_t* init_x264_encoder(x264_param_t *param, x264_picture_t *pic_in, unsigned int width, unsigned int height)
{
	int csp = X264_CSP_I420;

	//可用的预设从快到慢依次为：ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, placebo。
	x264_param_default_preset(param, "ultrafast", "zerolatency");
	param->rc.i_rc_method = X264_RC_CRF; //使用恒定质量（CRF）模式可以简化比特率控制，并通常提供更好的编码效率。CRF值越低，质量越好，但文件大小和编码时间增加。
	param->rc.f_rf_constant = 23;  // 调整此值以控制质量，范围为0-51，越小质量越好
	param->i_width = width;
	param->i_height = height;
	param->i_fps_num = 10;
	param->i_fps_den = 1;
	param->i_csp = X264_CSP_I420;
	param->i_bframe = 0;


	if (x264_param_apply_profile(param, "baseline") < 0) {
		LOG_ERROR("Failed to set profile\n");
		return NULL;
	}

	//open encoder
	x264_t *encoder = x264_encoder_open(param);
	if (!encoder) {
		LOG_ERROR("Failed to open encoder\n");
		return NULL;
	}
	x264_picture_alloc(pic_in, param->i_csp, param->i_width, param->i_height);
	return encoder;
}
