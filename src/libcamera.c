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
		perror("video query error");
	else {
		printf("video query capability:\n");
		printf("	driver  name: %s\n", cap.driver);
		printf("	card    name: %s\n", cap.card);
		printf("	bus     info: %s\n", cap.bus_info);
		printf("	krn  version: %d.%d.%d\n", cap.version >> 16, (cap.version >> 8) &0xff, cap.version & 0xff);
		printf("	capabilities: 0x%x\n", cap.capabilities);
		printf("	node    caps: 0x%x\n", cap.device_caps);
	}
}

int open_camera(char *path)
{
	return open(path, O_RDWR);
}

int get_camera_fmt(int fd)
{
	int ret;
	struct v4l2_fmtdesc desc;
	desc.index = 0;
	desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
	if (ret < 0)
		return ret;
	else {
		char *d = (char *)&desc.pixelformat;
		printf("format description:");
		printf("	index: %d\n", desc.index);
		printf("	type: %d\n", desc.type);
		printf("	flag: %d\n", desc.flags);
		printf("	description: %s\n", desc.description);
		printf("	pixelformat: %c%c%c%c\n", d[0], d[1], d[2], d[3]);
	}
}
