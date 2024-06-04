#include <stdio.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "libcamera.h"

int main(void)
{
	int fd, ret;
	struct v4l2_capability cap;

	fd = open(VIDEO_DEV, O_RDWR);
	if (fd < 0) {
		perror("open:");
		return fd;
	}
	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (fd < 0) {
		perror("open:");
		goto failed;
	}

	print_cap(&cap);

failed:
	close(fd);
	return 0;
}
