#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <error.h>
#include <sys/mman.h>
#include "fb_screen.h"

// Get variable screen information
int fb_get_vscreen_info(int fb_fd, struct fb_var_screeninfo *vinfo)
{
	return ioctl(fb_fd, FBIOGET_VSCREENINFO, vinfo);
}

// Get fixed screen information
int fb_get_fscreen_info(int fb_fd, struct fb_fix_screeninfo *finfo)
{
	return ioctl(fb_fd, FBIOGET_FSCREENINFO, finfo);
}

// Open the framebuffer device
int fb_open(const char *path)
{
	return open(path, O_RDWR);
}

// Close the framebuffer device
int fb_close(int fb_fd)
{
	return close(fb_fd);
}

// Display rgb frame
void fb_display_rgb_frame(const unsigned int width, const unsigned int height, struct fb_var_screeninfo *vinfo, unsigned char *rgb_frame, unsigned char *fb_ptr)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int fb_index = (y * vinfo->xres + x) * (vinfo->bits_per_pixel / 8);
			int rgb_index = (y * width + x) * 3;

			if (vinfo->bits_per_pixel == 32) {
				fb_ptr[fb_index + 0] = rgb_frame[rgb_index + 2]; // Blue
				fb_ptr[fb_index + 1] = rgb_frame[rgb_index + 1]; // Green
				fb_ptr[fb_index + 2] = rgb_frame[rgb_index + 0]; // Red
				fb_ptr[fb_index + 3] = 0;                        // No transparency
			} else {
				// Assuming 16bpp
				int b = rgb_frame[rgb_index + 2] >> 3;
				int g = rgb_frame[rgb_index + 1] >> 2;
				int r = rgb_frame[rgb_index + 0] >> 3;
				unsigned short int t = r << 11 | g << 5 | b;
				*((unsigned short int*)(fb_ptr + fb_index)) = t;
			}
		}
	}
}

// Map framebuffer to user space
unsigned char *fb_mmap_framebuffer(int fb_fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	long screensize = vinfo->yres_virtual * finfo->line_length;
	return (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
}

int fb_munmap_framebuffer(unsigned char *addr, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	long screensize = vinfo->yres_virtual * finfo->line_length;
	return munmap(addr, screensize);
}

int fb_init(char *fb_path, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, struct fb_handle *fb_handle)
{
	// Open the framebuffer device
	fb_handle->fb_fd = fb_open(fb_path);
	if (fb_handle->fb_fd == -1) {
		perror("Opening framebuffer device");
		return -1;
	}

	// Get variable screen information
	if (fb_get_vscreen_info(fb_handle->fb_fd, vinfo)) {
		perror("Reading variable information");
		return -1;
	}

	// Get fixed screen information
	if (fb_get_fscreen_info(fb_handle->fb_fd, finfo)) {
		perror("Reading fixed information");
		return -1;
	}

	fb_handle->fb_ptr = fb_mmap_framebuffer(fb_handle->fb_fd, vinfo, finfo);
	if (fb_handle->fb_ptr == MAP_FAILED) {
		perror("Error mapping framebuffer device to memory");
		return -1;
	}
	return 0;
}

int fb_deinit(struct fb_handle *fb_handle, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	int ret;
	ret = fb_close(fb_handle->fb_fd);
	ret |= fb_munmap_framebuffer(fb_handle->fb_ptr, vinfo, finfo);
	return ret;
}
