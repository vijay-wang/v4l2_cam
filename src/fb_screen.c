#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
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
unsigned char *fb_map_framebuffer(int fb_fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	long screensize = vinfo->yres_virtual * finfo->line_length;
	return (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
}
