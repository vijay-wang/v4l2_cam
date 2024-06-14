#include <stdio.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "video.h"
#include "v4l2_cam_log.h"
#include "algorithm.h"
#include "fb_screen.h"
#include "sdl_display.h"

#define BUFFER_COUNT	4
//#define WIDTH		640
//#define HEIGHT	480
//#define WIDTH		1280
//#define HEIGHT		720
#define WIDTH		320
#define HEIGHT		180

#define FB_PATH	"/dev/fb0"

struct opt_args {
	char *pixel_format;
	char *display_mode;
};

void usage(void)
{
	printf(	"v4l2_cam usage:\n"
		"	-d display mode, sdl or fb\n"
		"	-f pixel format, yuyv or mjpeg\n"
		"	-h help\n");
}

void parse_args(struct opt_args *args, int argc, char **argv)
{
	char ch;

	if (argc < 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	while ((ch = getopt(argc, argv, "d:f:h")) != -1)  {
		switch (ch) {
		case 'd':
			args->display_mode = optarg;
			break;
		case 'f':
			args->pixel_format = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}

	for (int i = optind; i < argc; i++) {
		LOG_ERROR("Error: Invalid argument '%s'\n", argv[i]);
		usage();
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	int fd, ret;
	unsigned char *rgb_frame;
	struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	struct v4l2_buffer mbuffer;
	struct fb_info fb_info;
	struct sdl_info sdl_info;
	struct opt_args args;

	memset(bufs, 0, sizeof(bufs));
	memset(&fmt, 0, sizeof(struct v4l2_format));
	memset(&reqbuffer, 0, sizeof(struct v4l2_requestbuffers));
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	memset(&fb_info, 0, sizeof(struct fb_info));
	memset(&sdl_info, 0, sizeof(struct opt_args));
	memset(&args, 0, sizeof(struct opt_args));

	parse_args(&args, argc, argv);

	fd = camera_open(VIDEO_DEV);
	if (fd < 0) {
		LOG_ERROR("camera_open failed");
		return fd;
	} else
		LOG_INFO("camera_open success\n");

	camera_query_capability(fd);
	camera_list_fmt(fd);

	/* set format */
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	if (!args.pixel_format) {
		LOG_ERROR("Pixel format not been set\n");
		exit(EXIT_FAILURE);
	} else if (!strcmp(args.pixel_format, "yuyv"))
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	else if (!strcmp(args.pixel_format, "mjpeg"))
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	else {
		LOG_ERROR("Unkown pixel format\n");
		exit(EXIT_FAILURE);
	}
	ret = camera_set_format(fd, &fmt);
	if (ret < 0)
		LOG_ERROR("camera_set_format failed");
	 else
		LOG_INFO("camera_set_format success\n");

	/* request buffers */
	reqbuffer.count = BUFFER_COUNT;
	reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuffer.memory = V4L2_MEMORY_MMAP;
	ret = camera_request_buffers(fd, &reqbuffer);
	if (ret < 0)
		LOG_ERROR("camera_request_buffers failed");
	 else
		LOG_INFO("camera_request_buffers success\n");

	/* query buffers, map buffers, enqueue buffers */
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	for (int i = 0; i < BUFFER_COUNT; ++i) {
		mbuffer.index = i;
		mbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = camera_query_buffer(fd, &mbuffer);
		if (ret < 0)
			LOG_ERROR("camera_query_buffers failed");
		ret = camera_map_buffer(fd, &mbuffer, &bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_map_buffers failed");
		ret = camera_qbuffer(fd, &mbuffer);
		if (ret < 0)
			LOG_ERROR("camera_qbuffer failed");
	}

	/* stream on */
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = camera_streamon(fd, &type);
	if (ret < 0)
		LOG_ERROR("camera_streamon failed");
	 else
		LOG_INFO("camera_streamon success\n");

	rgb_frame = (unsigned char *)camera_alloc_rgb(WIDTH, HEIGHT);

	if (!args.display_mode) {
		LOG_ERROR("Display mode not been set\n");
		exit(EXIT_FAILURE);
	} else if (!strcmp(args.display_mode, "fb")) {
		fb_set_info(&fb_info, WIDTH, HEIGHT, FB_PATH);
		ret = fb_init(&fb_info);
		if (ret < 0)
			LOG_ERROR("fb_init failed\n");
	} else if (!strcmp(args.display_mode, "sdl")) {
		ret = sdl_init(&sdl_info, WIDTH, HEIGHT);
		if (ret < 0)
			LOG_ERROR("sdl_init");
	} else {
		LOG_ERROR("Unkown display_mode\n");
		exit(EXIT_FAILURE);
	}


	while (1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		struct timeval tv = {0};
		tv.tv_sec = 5;
		int r = select(fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == r) {
			LOG_WARNING("Waiting for Frame time out 5s");
			//break;
			continue;
		}

		if (camera_dqbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Retrieving Frame failed");
			continue;
			//return 1;
		}

		if (!strcmp(args.pixel_format, "yuyv"))
			yuyv2rgb(bufs[mbuffer.index].pbuf, rgb_frame, WIDTH, HEIGHT);
		else if (!strcmp(args.pixel_format, "mjpeg")) {
			int result = mjpeg2rgb(bufs[mbuffer.index].pbuf, mbuffer.length, rgb_frame, WIDTH, HEIGHT);
			if (result != 0) {
				LOG_WARNING("mjpeg decode failed\n");
				continue;
				//break;
			}
		}

		if (!strcmp(args.display_mode, "fb"))
			fb_display_rgb_frame(&fb_info, rgb_frame);
		else if (!strcmp(args.display_mode, "sdl"))
			sdl_dislpay(&sdl_info, rgb_frame);

		if (camera_qbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Queue Buffer failed");
			continue;
			//break;
		}
	}

	if (!strcmp(args.display_mode, "fb")) {
		ret = fb_deinit(&fb_info);
		if (ret < 0)
			LOG_ERROR("fb_deinit failed");
	} else if (!strcmp(args.display_mode, "sdl"))
		sdl_deinit(&sdl_info);
		
	camera_free_rgb(rgb_frame);
		
	/* stream off */
	ret = camera_streamoff(fd, &type);
	if (ret < 0)
		LOG_ERROR("camera_streamoff failed");
	 else
		LOG_INFO("camera_streamoff success\n");

	/* unmap buffers */
	for (int i = 0; i < BUFFER_COUNT; ++i) {
		ret = camera_munmap_buffer(&bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_munmap_buffer failed");
	}
	camera_close(fd);
	return 0;
}
