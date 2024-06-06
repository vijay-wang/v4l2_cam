#ifndef _LIBCAMERA_H
#define _LIBCAMERA_H

#define VIDEO_DEV	"/dev/video0"
#define INDENT		"	"

void print_camera_info(int fd);
int open_camera(char *path);
int get_camera_fmt(int fd);
int get_camera_ext_ctrl(int fd);
#endif
