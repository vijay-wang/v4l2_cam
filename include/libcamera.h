#ifndef _LIBCAMERA_H
#define _LIBCAMERA_H

#define VIDEO_DEV	"/dev/video0"
#define INDENT		"  "

struct mbuf {
	char *pbuf;
	size_t length;
};

void camera_query_capability(int fd);
int camera_open(char *path);
int camera_close(int fd);
int camera_list_fmt(int fd);
int camera_list_fmt(int fd);
int camera_set_format(int fd, struct v4l2_format *fmt);
int camera_request_buffers(int fd, struct v4l2_requestbuffers *buffers);
int camera_query_buffer(int fd, struct v4l2_buffer *mbuffer);
int camera_map_buffer(int fd, struct v4l2_buffer *mbuffer, struct mbuf *buf);
int camera_munmap_buffer(struct mbuf *buf);
int camera_qbuffer(int fd, struct v4l2_buffer *mbuffer);
int camera_dqbuffer(int fd, struct v4l2_buffer *mbuffer);
int camera_streamon(int fd, enum v4l2_buf_type *type);
int camera_streamoff(int fd, enum v4l2_buf_type *type);
unsigned char *camera_alloc_rgb(const unsigned int width, const unsigned int height);
void camera_free_rgb(unsigned char *rgb);
#endif
