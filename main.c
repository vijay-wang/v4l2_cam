#include <stdio.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include "video.h"
#include "level_log.h"
#include "algorithm.h"
#include "fb_screen.h"

#define BUFFER_COUNT	4




static int main_run = 0;
void sighandler(int sig)
{
	main_run = 0;
}

int main(int argc, char *argv[])
{
	int fd, ret;
	unsigned char *rgb_frame;
	unsigned int width, height;
	struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	struct v4l2_buffer mbuffer;
	struct fb_info fb_info;
	struct opt_args args;

	memset(bufs, 0, sizeof(bufs));
	memset(&fmt, 0, sizeof(struct v4l2_format));
	memset(&reqbuffer, 0, sizeof(struct v4l2_requestbuffers));
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	memset(&fb_info, 0, sizeof(struct fb_info));
	memset(&args, 0, sizeof(struct opt_args));

	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGSTOP, sighandler);
	signal(SIGHUP, sighandler);

	fd = camera_open(VIDEO_DEV);
	if (fd < 0) {
		LOG_ERROR("camera_open failed\n");
		return fd;
	} else
		LOG_DEBUG("camera_open success\n");

	parse_args(fd, &args, argc, argv);
	width = atoi(args.width);
	height = atoi(args.height);

	/* set format */
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	if (!args.pixel_format) {
		LOG_ERROR("Pixel format not been set\n");
		exit(EXIT_FAILURE);
	} else if (!strcmp(args.pixel_format, "yuyv"))
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	else if (!strcmp(args.pixel_format, "rgb565le"))
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	else {
		LOG_ERROR("Unkown pixel format\n");
		exit(EXIT_FAILURE);
	}
	ret = camera_set_format(fd, &fmt);
	if (ret < 0)
		LOG_ERROR("camera_set_format failed\n");
	 else
		LOG_DEBUG("camera_set_format success\n");

	/* request buffers */
	reqbuffer.count = BUFFER_COUNT;
	reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuffer.memory = V4L2_MEMORY_MMAP;
	ret = camera_request_buffers(fd, &reqbuffer);
	if (ret < 0)
		LOG_ERROR("camera_request_buffers failed\n");
	 else
		LOG_DEBUG("camera_request_buffers success\n");

	/* query buffers, map buffers, enqueue buffers */
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	int i;
	for (i = 0; i < BUFFER_COUNT; ++i) {
		mbuffer.index = i;
		mbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = camera_query_buffer(fd, &mbuffer);
		if (ret < 0)
			LOG_ERROR("camera_query_buffers failed\n");
		ret = camera_map_buffer(fd, &mbuffer, &bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_map_buffers failed\n");
		ret = camera_qbuffer(fd, &mbuffer);
		if (ret < 0)
			LOG_ERROR("camera_qbuffer failed\n");
	}

	/* stream on */
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = camera_streamon(fd, &type);
	if (ret < 0)
		LOG_ERROR("camera_streamon failed\n");
	 else
		LOG_DEBUG("camera_streamon success\n");

	rgb_frame = (unsigned char *)camera_alloc_rgb(width, height);

	if (!args.display_mode) {
		LOG_ERROR("Display mode not been set\n");
		exit(EXIT_FAILURE);
	} else if (!strcmp(args.display_mode, "fb")) {
		fb_set_info(&fb_info, width, height, FB_PATH);
		ret = fb_init(&fb_info);
		if (ret < 0)
			LOG_ERROR("fb_init failed\n");
	} else {
		LOG_ERROR("Unkown display_mode\n");
		exit(EXIT_FAILURE);
	}

	main_run = 100;
	while (main_run) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		struct timeval tv = {0};
		tv.tv_sec = 5;
		int r = select(fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == r) {
			LOG_WARNING("Waiting for Frame time out 5s\n");
			//break;
			continue;
		}

		if (camera_dqbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Retrieving Frame failed\n");
			continue;
			//return 1;
		}

		int ret;
		char path[32] = { 0 };
		sprintf(path, "./yuyv.%d", main_run);
		int fd_yuv;

		fd_yuv = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
		if (fd_yuv == -1) {
			perror("open");
			return 1;
		}

		ret = write(fd_yuv, bufs[mbuffer.index].pbuf, width * height *2);
		if (fd_yuv == -1) {
			perror("write");
			return 1;
		}

		close(fd_yuv);

		//if (!strcmp(args.pixel_format, "yuyv"))
		//	yuyv2rgb(bufs[mbuffer.index].pbuf, rgb_frame, width, height);
		//if (!strcmp(args.pixel_format, "rgb565le"))
		//	rgb565le2rgb888(bufs[mbuffer.index].pbuf, rgb_frame, width, height);

		//if (!strcmp(args.display_mode, "fb"))
		//	fb_display_rgb_frame(&fb_info, rgb_frame);

		if (camera_qbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Queue Buffer failed\n");
			continue;
			//break;
		}
		main_run--;
	}

	if (!strcmp(args.display_mode, "fb")) {
		ret = fb_deinit(&fb_info);
		if (ret < 0)
			LOG_ERROR("fb_deinit failed\n");
	}
	camera_free_rgb(rgb_frame);
		
	/* stream off */
	ret = camera_streamoff(fd, &type);
	if (ret < 0)
		LOG_ERROR("camera_streamoff failed\n");
	 else
		LOG_DEBUG("camera_streamoff success\n");

	/* unmap buffers */
	for (i = 0; i < BUFFER_COUNT; ++i) {
		ret = camera_munmap_buffer(&bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_munmap_buffer failed\n");
	}
	camera_close(fd);
	return 0;
}
