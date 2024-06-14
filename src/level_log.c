#include "level_log.h"

// 将日志级别转换为字符串
const char* log_level_to_string(int level) {
	switch (level) {
		case LOG_LEVEL_ERROR:   return "LEVEL_LOG ERROR";
		case LOG_LEVEL_WARNING: return "LEVEL_LOG WARNING";
		case LOG_LEVEL_INFO:    return "LEVEL_LOG INFO";
		case LOG_LEVEL_DEBUG:   return "LEVEL_LOG DEBUG";
		default:                return "LEVEL_LOG UNKNOWN";
	}
}
