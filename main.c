#include <stdio.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "libcamera.h"

int main(void)
{
	int fd, ret;

	fd = open_camera(VIDEO_DEV);
	if (fd < 0) {
		perror("open_camera");
		return fd;
	}

	print_camera_info(fd);
	ret = get_camera_fmt(fd);
	if (ret < 0)
		perror("get_camera_fmt");
	close(fd);
	return 0;
}
