#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
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
