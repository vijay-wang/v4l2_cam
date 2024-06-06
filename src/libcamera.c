#include <stdio.h>
#include <linux/videodev2.h>
#include "libcamera.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

void print_camera_info(int fd)
{
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
		perror("video query error:");
	else {
		printf("driver  name: %s\n", cap.driver);
		printf("card    name: %s\n", cap.card);
		printf("bus     info: %s\n", cap.bus_info);
		printf("krn  version: %d.%d.%d\n", cap.version >> 16, (cap.version >> 8) &0xff, cap.version & 0xff);
		printf("capabilities: 0x%x\n", cap.capabilities);
		printf("node    caps: 0x%x\n", cap.device_caps);
	}
}

int open_camera(char *path)
{
	return open(path, O_RDWR);
}
