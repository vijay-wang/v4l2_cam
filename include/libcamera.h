#ifndef _LIBCAMERA_H
#define _LIBCAMERA_H

#define VIDEO_DEV	"/dev/video0"

void print_camera_info(int fd);
int open_camera(char *path);
#endif
