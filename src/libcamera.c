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
	struct v4l2_frmsizeenum frmsize;
	struct v4l2_frmivalenum frmival;
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
			printf(INDENT"index: %d\n", desc.index);
			printf(INDENT"type: %d\n", desc.type);
			printf(INDENT"flag: %d\n", desc.flags);
			printf(INDENT"description: %s\n", desc.description);
			printf(INDENT"pixelformat: %c%c%c%c\n", d[0], d[1], d[2], d[3]);

			memset(&frmsize, 0, sizeof(frmsize));
			frmsize.pixel_format = desc.pixelformat;
			frmsize.index = 0;
			while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
				if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
					printf(INDENT INDENT"Discrete: %dx%d\n", frmsize.discrete.width, frmsize.discrete.height);
				} else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
					printf(INDENT INDENT"Stepwise: %dx%d - %dx%d with steps %dx%d\n",
							frmsize.stepwise.min_width, frmsize.stepwise.min_height,
							frmsize.stepwise.max_width, frmsize.stepwise.max_height,
							frmsize.stepwise.step_width, frmsize.stepwise.step_height);
				}
				memset(&frmival, 0, sizeof(frmival));
				frmival.pixel_format = desc.pixelformat;
				frmival.width = frmsize.discrete.width;
				frmival.height = frmsize.discrete.height;
				frmival.index = 0;

				while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
					if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
						printf(INDENT INDENT INDENT"Interval: %.3f s (%.3f fps)\n", 
								(double)frmival.discrete.numerator / frmival.discrete.denominator, 
								(double)frmival.discrete.denominator / frmival.discrete.numerator);
					} else if (frmival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS || frmival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
						printf(INDENT INDENT INDENT"Continuous: %.3f - %.3f s (%.3f - %.3f fps)\n",
								(double)frmival.stepwise.min.numerator / frmival.stepwise.min.denominator,
								(double)frmival.stepwise.max.numerator / frmival.stepwise.max.denominator,
								(double)frmival.stepwise.max.denominator / frmival.stepwise.max.numerator,
								(double)frmival.stepwise.min.denominator / frmival.stepwise.min.numerator);
						break; // Typically, there's only one range for continuous/stepwise intervals
					}
					frmival.index++;
				}
				frmsize.index++;
			}
		}
		desc.index++;
	}
	return 0;
}

int get_camera_ext_ctrl(int fd)
{
//	int ret;
//	//struct v4l2_fmtdesc desc;
//	struct v4l2_query_ext_ctrl ext_ctrl;
//	memset(&ext_ctrl, 0, sizeof(struct v4l2_query_ext_ctrl));
//	printf("\next ctrl:\n");
//	for (;;) {
//		/* index 必须初始化,这里从0开始循环 */
//		//desc.index = 0;
//		desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//		ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
//		if (ret < 0)
//			return ret;
//		else {
//			char *d = (char *)&desc.pixelformat;
//			printf(INDENT"description %d:\n", desc.index);
//			printf(INDENT INDENT"index: %d\n", desc.index);
//			printf(INDENT INDENT"type: %d\n", desc.type);
//			printf(INDENT INDENT"flag: %d\n", desc.flags);
//			printf(INDENT INDENT"description: %s\n", desc.description);
//			printf(INDENT INDENT"pixelformat: %c%c%c%c\n", d[0], d[1], d[2], d[3]);
//		}
//		desc.index++;
//	}
	return 0;
}
