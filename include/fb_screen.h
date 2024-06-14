#ifndef _FB_SCREEN_H
#define _FB_SCREEN_H
#include <linux/fb.h>

#define FB_PATH	"/dev/fb0"

struct fb_handle {
	int fb_fd;
	unsigned char *fb_ptr;
};

struct fb_info {
	char *fb_path;
	unsigned int width;
	unsigned int height;
	struct fb_handle fb_handle;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
};

void fb_set_info(struct fb_info *fb_info, unsigned int width, unsigned int height, char *fb_path);
int fb_init(struct fb_info *fb_info);
int fb_deinit(struct fb_info *fb_info);
void fb_display_rgb_frame(struct fb_info *fb_info, unsigned char *rgb_frame);
//unsigned char *fb_mmap_framebuffer(int fb_fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo);
//static int fb_munmap_framebuffer(unsigned char *addr, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
//int fb_get_vscreen_info(int fb_fd, struct fb_var_screeninfo *vinfo);
//int fb_get_fscreen_info(int fb_fd, struct fb_fix_screeninfo *finfo);
//int fb_open(const char *path);
//int fb_close(int fb_fd);
#endif
