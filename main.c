#include <stdio.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libcamera.h"
#include "algorithm.h"

#define BUFFER_COUNT	4

int main(void)
{
	int fd, ret;
	struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	struct v4l2_buffer mbuffer;

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
	fmt.fmt.pix.width = 1280;
	fmt.fmt.pix.height = 720;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
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

	/* dequeu buffer */
	mbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = camera_dqbuffer(fd, &mbuffer);
	if (ret < 0)
		perror("camera_dqbuffer");
	 else
		printf("camera_dqbuffer success\n");

	/* save image */
	int img_file = open("./image", O_RDWR | O_CREAT, 0666);
	if (img_file < 0)
		perror("open");
	else {
		ret = write(img_file, bufs[mbuffer.index].pbuf, mbuffer.length);
		if (ret < 0)
			perror("write");
		close(img_file);
	}

	unsigned char *mjpeg_frame = bufs[mbuffer.index].pbuf;
	size_t frame_size = mbuffer.length;
	rgb_image *image = decode_jpeg(mjpeg_frame, frame_size);
	free_rgb_image(image);

	/* enqueue buffer */
	ret = camera_qbuffer(fd, &mbuffer);
	if (ret < 0)
		perror("camera_qbuffer");
	 else
		printf("camera_qbuffer success\n");
		
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
