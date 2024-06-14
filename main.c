#include <stdio.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "libcamera.h"
#include "algorithm.h"
#include "fb_screen.h"

#define BUFFER_COUNT	4
//#define WIDTH		640
//#define HEIGHT	480
//#define WIDTH		1280
//#define HEIGHT		720
#define WIDTH		320
#define HEIGHT		180



#define FB_PATH	"/dev/fb0"

enum {
	MODE_YUYV,
	MODE_MJPEG,
};

#define MODE	MODE_MJPEG
//#define MODE	MODE_YUYV

int main(void)
{
	int fd, ret;
	unsigned char *rgb_frame;
	struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	struct v4l2_buffer mbuffer;
	struct fb_info fb_info;

	fd = camera_open(VIDEO_DEV);
	if (fd < 0) {
		perror("camera_open");
		return fd;
	} else
		printf("camera_open success\n");

	camera_query_capability(fd);
	camera_list_fmt(fd);

	/* set format */
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	if (MODE == MODE_YUYV)
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	if (MODE == MODE_MJPEG)
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	ret = camera_set_format(fd, &fmt);
	if (ret < 0)
		perror("camera_set_format");
	 else
		printf("camera_set_format success\n");

	/* request buffers */
	reqbuffer.count = BUFFER_COUNT;
	reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuffer.memory = V4L2_MEMORY_MMAP;
	ret = camera_request_buffers(fd, &reqbuffer);
	if (ret < 0)
		perror("camera_request_buffers");
	 else
		printf("camera_request_buffers success\n");

	/* query buffers, map buffers, enqueue buffers */
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	for (int i = 0; i < BUFFER_COUNT; ++i) {
		mbuffer.index = i;
		mbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = camera_query_buffer(fd, &mbuffer);
		if (ret < 0)
			perror("camera_query_buffers");
		ret = camera_map_buffer(fd, &mbuffer, &bufs[i]);
		if (ret < 0)
			perror("camera_map_buffers");
		ret = camera_qbuffer(fd, &mbuffer);
		if (ret < 0)
			perror("camera_qbuffer");
	}

	/* stream on */
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = camera_streamon(fd, &type);
	if (ret < 0)
		perror("camera_streamon");
	 else
		printf("camera_streamon success\n");

	rgb_frame = (unsigned char *)camera_alloc_rgb(WIDTH, HEIGHT);
	fb_set_info(&fb_info, WIDTH, HEIGHT, FB_PATH);
	ret = fb_init(&fb_info);
	if (ret < 0)
		perror("fb_init failed\n");

	while (1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		struct timeval tv = {0};
		tv.tv_sec = 5;
		int r = select(fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == r) {
			perror("Waiting for Frame");
			return 1;
		}

		if (camera_dqbuffer(fd, &mbuffer) == -1) {
			perror("Retrieving Frame");
			continue;
			//return 1;
		}

		if (MODE == MODE_YUYV)
			yuyv2rgb(bufs[mbuffer.index].pbuf, rgb_frame, WIDTH, HEIGHT);
		if (MODE == MODE_MJPEG) {
			int result = mjpeg2rgb(bufs[mbuffer.index].pbuf, mbuffer.length, rgb_frame, WIDTH, HEIGHT);
			if (result != 0) {
				printf("mjpeg decode failed\n");
				break;
			}
		}

		fb_display_rgb_frame(&fb_info, rgb_frame);

		if (camera_qbuffer(fd, &mbuffer) == -1) {
			perror("Queue Buffer");
			return -1;
		}
	}
	ret = fb_deinit(&fb_info);
	if (ret < 0)
		perror("fb_deinit failed");
	camera_free_rgb(rgb_frame);
		
	/* stream off */
	ret = camera_streamoff(fd, &type);
	if (ret < 0)
		perror("camera_streamoff");
	 else
		printf("camera_streamoff success\n");

	/* unmap buffers */
	for (int i = 0; i < BUFFER_COUNT; ++i) {
		ret = camera_munmap_buffer(&bufs[i]);
		if (ret < 0)
			perror("camera_munmap_buffer");
	}
	camera_close(fd);
	return 0;
}
