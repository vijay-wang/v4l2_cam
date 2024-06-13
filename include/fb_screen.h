#ifndef _FB_SCREEN_H
#define _FB_SCREEN_H
#include <linux/fb.h>
unsigned char *fb_map_framebuffer(int fb_fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo);
void fb_display_rgb_frame(const unsigned int width, const unsigned int height, struct fb_var_screeninfo *vinfo, unsigned char *rgb_frame, unsigned char *fb_ptr);
int fb_get_vscreen_info(int fb_fd, struct fb_var_screeninfo *vinfo);
int fb_get_fscreen_info(int fb_fd, struct fb_fix_screeninfo *finfo);
int fb_open(const char *path);
int fb_close(int fb_fd);
#endif
