#include <stdio.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define VIDEO_DEV	"/dev/video0"

void print_cap(struct v4l2_capability *cap)
{
	printf("driver  name: %s\n", cap->driver);
	printf("card    name: %s\n", cap->card);
	printf("bus     info: %s\n", cap->bus_info);
	printf("krn  version: %d.%d.%d\n", cap->version >> 16, (cap->version >> 8) &0xff, cap->version & 0xff);
	printf("capabilities: 0x%x\n", cap->capabilities);
	printf("node    caps: 0x%x\n", cap->device_caps);
}

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
