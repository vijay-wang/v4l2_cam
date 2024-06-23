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
#include <stdint.h>
#include "video.h"
#include "level_log.h"
#include "algorithm.h"
#include "fb_screen.h"
#include "x264.h"
#include "vencode.h"

#define BUFFER_COUNT	4

struct opt_args {
	char *pixel_format;
	char *display_mode;
	char *width;
	char *height;
};

void usage(void)
{
	printf(	"v4l2_cam usage:\n"
		"	-f pixel format, yuyv, rgb565le\n"
		"	-W set pixel width\n"
		"	-H set pixel height\n"
		"	-d set display way\n"
		"	-l list the resolutions supported by the camera\n"
		"	-q query the basic infomation about the driver and camera\n"
		"	-h help\n");
}

void parse_args(int fd, struct opt_args *args, int argc, char **argv)
{
	int ch;

	if (argc < 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	while ((ch = getopt(argc, argv, "d:f:W:H:lqh")) != -1)  {
		switch (ch) {
		case 'd':
			args->display_mode = optarg;
			break;
		case 'f':
			args->pixel_format = optarg;
			break;
		case 'W':
			args->width = optarg;
			break;
		case 'H':
			args->height = optarg;
			break;
		case 'q':
			camera_query_capability(fd);
			break;
		case 'l':
			camera_list_fmt(fd);
			break;

		case 'h':
			usage();
			camera_close(fd);
			exit(EXIT_SUCCESS);
		default:
			usage();
			camera_close(fd);
			exit(EXIT_FAILURE);
		}
	}

	int i;
	for (i = optind; i < argc; i++) {
		LOG_ERROR("Error: Invalid argument '%s'\n", argv[i]);
		usage();
		exit(EXIT_FAILURE);
	}
}

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

	unsigned char *h264 = (unsigned char *)malloc(width * height);  // H.264输出缓冲区
	if (!h264) {
		LOG_ERROR("Failed to allocate h264 frame\n");
		return -1;
	}
	unsigned char *i420_frame = (unsigned char *)malloc(width * height * 3 / 2);
	if (!i420_frame) {
		LOG_ERROR("Failed to allocate I420 frame\n");
		return -1;
	}

	// Initialize x264 encoder
	x264_picture_t pic_in, pic_out;
	x264_param_t param;
	x264_t *encoder;
	x264_nal_t *nals;
	int num_nals;

	encoder = init_x264_encoder(&param, &pic_in, width, height);

	main_run = 1;
	int flag = 0;
	int save_file = 0;
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

		if (!(flag % 10)) {
		yuyv_to_yuv420p(bufs[mbuffer.index].pbuf, &pic_in, width, height);
		if (x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out) < 0) {
			LOG_ERROR("Error encoding frame\n");
			break;
		}



			int file;
			char filename[32];
			sprintf(filename, "h2264.%d", save_file);
			file = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
			if (fd == -1) {
				perror("open");
				return 1;
			}

			write(file, h264, width * height);


			// 关闭文件
			close(file);
			save_file++;
		}


		if (camera_qbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Queue Buffer failed\n");
			continue;
			//break;
		}
		flag++;
	}

	x264_encoder_close(encoder);
	free(i420_frame);
	free(h264);

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
