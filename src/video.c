#include <stdio.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "video.h"
#include "level_log.h"

void camera_query_capability(int fd)
{
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
		LOG_ERROR("video query error");
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

int camera_open(char *path)
{
	return open(path, O_RDWR);
}

int camera_close(int fd)
{
	return close(fd);
}

int camera_list_fmt(int fd)
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
			printf(INDENT"fomate [%d]:\n", desc.index);
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

int camera_set_format(int fd, struct v4l2_format *fmt)
{
	return ioctl(fd, VIDIOC_S_FMT, fmt);
}

int camera_request_buffers(int fd, struct v4l2_requestbuffers *buffers)
{
	return ioctl(fd, VIDIOC_REQBUFS, buffers);
}

int camera_query_buffer(int fd, struct v4l2_buffer *mbuffer)
{
	return ioctl(fd, VIDIOC_QUERYBUF, mbuffer);
}

int camera_map_buffer(int fd, struct v4l2_buffer *mbuffer, struct mbuf *buf)
{
	buf->length = mbuffer->length;
	buf->pbuf = mmap(NULL, mbuffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mbuffer->m.offset );
	if (buf->pbuf < 0)
		return -1;
	return 0;
}

int camera_munmap_buffer(struct mbuf *buf)
{
	return munmap(buf->pbuf, buf->length);
}

/* 将空闲的内存加入可捕获视频的队列 */
int camera_qbuffer(int fd, struct v4l2_buffer *mbuffer)
{
	return ioctl(fd, VIDIOC_QBUF, mbuffer);
}

/* 将已经捕获好视频的内存拉出已捕获视频的队列 */
int camera_dqbuffer(int fd, struct v4l2_buffer *mbuffer)
{
	return ioctl(fd, VIDIOC_DQBUF, mbuffer);
}

int camera_streamon(int fd, enum v4l2_buf_type *type)
{
	return ioctl(fd, VIDIOC_STREAMON, type);
}

int camera_streamoff(int fd, enum v4l2_buf_type *type)
{
	return ioctl(fd, VIDIOC_STREAMOFF, type);
}

unsigned char *camera_alloc_rgb(const unsigned int width, const unsigned int height)
{
	return (unsigned char *)malloc(width * height * 3);
}

void camera_free_rgb(unsigned char *rgb)
{
	free(rgb);
}
