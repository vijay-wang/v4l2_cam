#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <error.h>
#include <sys/mman.h>
#include "fb_screen.h"
#include "level_log.h"

// Get variable screen information
static int fb_get_vscreen_info(int fb_fd, struct fb_var_screeninfo *vinfo)
{
	return ioctl(fb_fd, FBIOGET_VSCREENINFO, vinfo);
}

// Get fixed screen information
static int fb_get_fscreen_info(int fb_fd, struct fb_fix_screeninfo *finfo)
{
	return ioctl(fb_fd, FBIOGET_FSCREENINFO, finfo);
}

// Open the framebuffer device
static int fb_open(const char *path)
{
	return open(path, O_RDWR);
}

// Close the framebuffer device
static int fb_close(int fb_fd)
{
	return close(fb_fd);
}

// Display rgb frame
void fb_display_rgb_frame(struct fb_info *fb_info, unsigned char *rgb_frame)
{
	int y, x;
	for (y = 0; y < fb_info->height; y++) {
		for (x = 0; x < fb_info->width; x++) {
			int fb_index = (y * fb_info->vinfo.xres + x) * (fb_info->vinfo.bits_per_pixel / 8);
			int rgb_index = (y * fb_info->width + x) * 3;

			if (fb_info->vinfo.bits_per_pixel == 32) {
				fb_info->fb_handle.fb_ptr[fb_index + 0] = rgb_frame[rgb_index + 2]; // Blue
				fb_info->fb_handle.fb_ptr[fb_index + 1] = rgb_frame[rgb_index + 1]; // Green
				fb_info->fb_handle.fb_ptr[fb_index + 2] = rgb_frame[rgb_index + 0]; // Red
				fb_info->fb_handle.fb_ptr[fb_index + 3] = 0;                        // No transparency
			} else {
				// Assuming 16bpp
				int b = rgb_frame[rgb_index + 2] >> 3;
				int g = rgb_frame[rgb_index + 1] >> 2;
				int r = rgb_frame[rgb_index + 0] >> 3;
				unsigned short int t = r << 11 | g << 5 | b;
				*((unsigned short int*)(fb_info->fb_handle.fb_ptr + fb_index)) = t;
			}
		}
	}
}

// Map framebuffer to user space
static unsigned char *fb_mmap_framebuffer(int fb_fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	long screensize = vinfo->yres_virtual * finfo->line_length;
	return (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
}

static int fb_munmap_framebuffer(unsigned char *addr, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	long screensize = vinfo->yres_virtual * finfo->line_length;
	return munmap(addr, screensize);
}

int fb_init(struct fb_info *fb_info)
{
	// Open the framebuffer device
	fb_info->fb_handle.fb_fd = fb_open(fb_info->fb_path);
	if (fb_info->fb_handle.fb_fd == -1) {
		LOG_ERROR("Opening framebuffer device");
		return -1;
	}

	// Get variable screen information
	if (fb_get_vscreen_info(fb_info->fb_handle.fb_fd, &fb_info->vinfo)) {
		LOG_ERROR("Reading variable information");
		return -1;
	}

	// Get fixed screen information
	if (fb_get_fscreen_info(fb_info->fb_handle.fb_fd, &fb_info->finfo)) {
		LOG_ERROR("Reading fixed information");
		return -1;
	}

	fb_info->fb_handle.fb_ptr = fb_mmap_framebuffer(fb_info->fb_handle.fb_fd, &fb_info->vinfo, &fb_info->finfo);
	if (fb_info->fb_handle.fb_ptr == MAP_FAILED) {
		LOG_ERROR("Error mapping framebuffer device to memory");
		return -1;
	}
	return 0;
}

int fb_deinit(struct fb_info *fb_info)
{
	int ret;
	ret |= fb_munmap_framebuffer(fb_info->fb_handle.fb_ptr, &fb_info->vinfo, &fb_info->finfo);
	ret = fb_close(fb_info->fb_handle.fb_fd);
	return ret;
}

void fb_set_info(struct fb_info *fb_info, unsigned int width, unsigned int height, char *fb_path)
{
	fb_info->width = width;
	fb_info->height = height;
	fb_info->fb_path = fb_path;
}
