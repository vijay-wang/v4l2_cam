#include <stdio.h>
#include <linux/videodev2.h>
#include "libcamera.h"

void print_cap(struct v4l2_capability *cap)
{
	printf("driver  name: %s\n", cap->driver);
	printf("card    name: %s\n", cap->card);
	printf("bus     info: %s\n", cap->bus_info);
	printf("krn  version: %d.%d.%d\n", cap->version >> 16, (cap->version >> 8) &0xff, cap->version & 0xff);
	printf("capabilities: 0x%x\n", cap->capabilities);
	printf("node    caps: 0x%x\n", cap->device_caps);
}
