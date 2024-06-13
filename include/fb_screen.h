#ifndef _FB_SCREEN_H
#define _FB_SCREEN_H
#include <linux/fb.h>
int fb_get_vscreen_info(int fb_fd, struct fb_var_screeninfo *vinfo);
int fb_get_fscreen_info(int fb_fd, struct fb_fix_screeninfo *finfo);
int fb_open(const char *path);
int fb_close(int fb_fd);
#endif
