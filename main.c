#include <stdio.h>
#include <SDL2/SDL.h>
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
#define WIDTH		1280
#define HEIGHT		720

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
	unsigned char *fb_ptr;
	unsigned char *rgb_frame;
	struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	struct v4l2_buffer mbuffer;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

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

	///* dequeu buffer */
	//mbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//ret = camera_dqbuffer(fd, &mbuffer);
	//if (ret < 0)
	//	perror("camera_dqbuffer");
	// else
	//	printf("camera_dqbuffer success\n");
	//

	// Open the framebuffer device
	int fb_fd = fb_open(FB_PATH);
	if (fb_fd == -1) {
		perror("Opening framebuffer device");
		return -1;
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("Video Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
	if (!window) {
		fprintf(stderr, "Could not create window - %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Could not create renderer - %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	if (!texture) {
		fprintf(stderr, "Could not create texture - %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	rgb_frame = (unsigned char *)camera_alloc_rgb(WIDTH, HEIGHT);

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

		// Update the texture with the new frame
		SDL_UpdateTexture(texture, NULL, rgb_frame, WIDTH * 3);

		// Clear the renderer
		SDL_RenderClear(renderer);

		// Copy the texture to the renderer
		//SDL_RenderCopy(renderer, texture, NULL, NULL);
		if (SDL_RenderCopy(renderer, texture, NULL, NULL) != 0) {
			fprintf(stderr, "SDL_RenderCopy error: %s\n", SDL_GetError());
			SDL_DestroyTexture(texture);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
			break;
		}

		// Present the rendered image to the window
		SDL_RenderPresent(renderer);

		//fb_display_rgb_frame(WIDTH, HEIGHT, &vinfo, rgb_frame, fb_ptr);

		if (camera_qbuffer(fd, &mbuffer) == -1) {
			perror("Queue Buffer");
			return -1;
		}
	}
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	camera_free_rgb(rgb_frame);
#if 0
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
#endif
		
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
