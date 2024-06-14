#ifndef _FB_SCREEN_H
#define _FB_SCREEN_H
#include <linux/fb.h>

struct fb_handle {
	int fb_fd;
	unsigned char *fb_ptr;
};

unsigned char *fb_mmap_framebuffer(int fb_fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo);
int fb_init(char *fb_path, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, struct fb_handle *fb_handle);
int fb_deinit(struct fb_handle *fb_handle, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo);
void fb_display_rgb_frame(const unsigned int width, const unsigned int height, struct fb_var_screeninfo *vinfo, unsigned char *rgb_frame, unsigned char *fb_ptr);
int fb_get_vscreen_info(int fb_fd, struct fb_var_screeninfo *vinfo);
int fb_get_fscreen_info(int fb_fd, struct fb_fix_screeninfo *finfo);
int fb_open(const char *path);
int fb_close(int fb_fd);
#endif
