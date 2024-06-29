#ifndef _LEVEL_LOG_H
#define _LEVEL_LOG_H
#include <stdio.h>

// 定义日志级别
#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4

// 当前日志级别
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// 定义日志宏
#define LOG(level, fmt, ...) \
	do {\
		if (level <= CURRENT_LOG_LEVEL) {\
			if (level == LOG_LEVEL_ERROR) \
				printf("\033[31m[%s] " fmt "\033[0m", log_level_to_string(level), ##__VA_ARGS__); \
			else if (level == LOG_LEVEL_WARNING) \
				printf("\033[33m[%s] " fmt "\033[0m", log_level_to_string(level), ##__VA_ARGS__); \
			else if (level == LOG_LEVEL_INFO) \
				printf("\033[32m[%s] " fmt "\033[0m", log_level_to_string(level), ##__VA_ARGS__); \
			else \
				printf("[%s] " fmt, log_level_to_string(level), ##__VA_ARGS__); \
		} \
	} while (0)

// 日志宏的快捷定义
#define LOG_ERROR(fmt, ...)   LOG(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) LOG(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    LOG(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   LOG(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define CHECK_RET(function) \
		LOG_DEBUG("%ld %s: return %d\n", __LINE__, __func__, function)

const char* log_level_to_string(int level);
#endif
