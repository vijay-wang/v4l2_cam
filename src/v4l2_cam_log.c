#include "v4l2_cam_log.h"

// 将日志级别转换为字符串
const char* log_level_to_string(int level) {
	switch (level) {
		case LOG_LEVEL_ERROR:   return "V4L2_CAM_LOG ERROR";
		case LOG_LEVEL_WARNING: return "V4L2_CAM_LOG WARNING";
		case LOG_LEVEL_INFO:    return "V4L2_CAM_LOG INFO";
		case LOG_LEVEL_DEBUG:   return "V4L2_CAM_LOG DEBUG";
		default:                return "V4L2_CAM_LOG UNKNOWN";
	}
}
