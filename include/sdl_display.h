#ifndef _SDL_DISPLAY_H
#define _SDL_DISPLAY_H
#include <SDL2/SDL.h>

struct sdl_info {
	unsigned int width;
	unsigned int height;
	struct {
		SDL_Window *window;
		SDL_Renderer *renderer;
		SDL_Texture *texture;
	} sdl_handle;
};

int sdl_init(struct sdl_info *sdl_info, unsigned int width, unsigned int height);
void sdl_deinit(struct sdl_info *sdl_info);
int sdl_dislpay(struct sdl_info *sdl_info, unsigned char *rgb_frame);

#endif
