#include <stdio.h>
#include <linux/videodev2.h>
#include "libcamera.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

void print_camera_info(int fd)
{
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
		perror("video query error");
	else {
		printf("\nvideo query capability:\n");
		printf(INDENT"driver  name: %s\n", cap.driver);
		printf(INDENT"card    name: %s\n", cap.card);
		printf(INDENT"bus     info: %s\n", cap.bus_info);
		printf(INDENT"krn  version: %d.%d.%d\n", cap.version >> 16, (cap.version >> 8) &0xff, cap.version & 0xff);
		printf(INDENT"capabilities: 0x%x\n", cap.capabilities);
		printf(INDENT"node    caps: 0x%x\n", cap.device_caps);
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
	memset(&desc, 0, sizeof(struct v4l2_fmtdesc));
	printf("\nformat description:\n");
	for (;;) {
		/* index 必须初始化,这里从0开始循环 */
		//desc.index = 0;
		desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
		if (ret < 0)
			return ret;
		else {
			char *d = (char *)&desc.pixelformat;
			printf(INDENT"description %d:\n", desc.index);
			printf(INDENT INDENT"index: %d\n", desc.index);
			printf(INDENT INDENT"type: %d\n", desc.type);
			printf(INDENT INDENT"flag: %d\n", desc.flags);
			printf(INDENT INDENT"description: %s\n", desc.description);
			printf(INDENT INDENT"pixelformat: %c%c%c%c\n", d[0], d[1], d[2], d[3]);
		}
		desc.index++;
	}
	return 0;
}

int get_camera_ext_ctrl(int fd)
{
	int ret;
	//struct v4l2_fmtdesc desc;
	struct v4l2_query_ext_ctrl ext_ctrl;
	memset(&ext_ctrl, 0, sizeof(struct v4l2_query_ext_ctrl));
	printf("\nformat description:\n");
	for (;;) {
		/* index 必须初始化,这里从0开始循环 */
		//desc.index = 0;
		desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
		if (ret < 0)
			return ret;
		else {
			char *d = (char *)&desc.pixelformat;
			printf(INDENT"description %d:\n", desc.index);
			printf(INDENT INDENT"index: %d\n", desc.index);
			printf(INDENT INDENT"type: %d\n", desc.type);
			printf(INDENT INDENT"flag: %d\n", desc.flags);
			printf(INDENT INDENT"description: %s\n", desc.description);
			printf(INDENT INDENT"pixelformat: %c%c%c%c\n", d[0], d[1], d[2], d[3]);
		}
		desc.index++;
	}
	return 0;
}
VIDIOC_QUERY_EXT_CTRL
