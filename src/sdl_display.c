#include <stddef.h>
#include "sdl_display.h"
#include "level_log.h"

int sdl_init(struct sdl_info *sdl_info, unsigned int width, unsigned int height)
{
	sdl_info->width = width;
	sdl_info->height = height;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		LOG_ERROR("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	sdl_info->sdl_handle.window = SDL_CreateWindow("Video Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sdl_info->width, sdl_info->height, 0);
	if (!sdl_info->sdl_handle.window) {
		LOG_ERROR("Could not create window - %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	sdl_info->sdl_handle.renderer = SDL_CreateRenderer(sdl_info->sdl_handle.window, -1, 0);
	if (!sdl_info->sdl_handle.renderer) {
		LOG_ERROR("Could not create renderer - %s\n", SDL_GetError());
		SDL_DestroyWindow(sdl_info->sdl_handle.window);
		SDL_Quit();
		return -1;
	}

	sdl_info->sdl_handle.texture = SDL_CreateTexture(sdl_info->sdl_handle.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, sdl_info->width, sdl_info->height);
	if (!sdl_info->sdl_handle.texture) {
		LOG_ERROR("Could not create texture - %s\n", SDL_GetError());
		SDL_DestroyRenderer(sdl_info->sdl_handle.renderer);
		SDL_DestroyWindow(sdl_info->sdl_handle.window);
		SDL_Quit();
		return -1;
	}
	return 0;
}

void sdl_deinit(struct sdl_info *sdl_info)
{
	SDL_DestroyTexture(sdl_info->sdl_handle.texture);
	SDL_DestroyRenderer(sdl_info->sdl_handle.renderer);
	SDL_DestroyWindow(sdl_info->sdl_handle.window);
	SDL_Quit();
}

int sdl_dislpay(struct sdl_info *sdl_info, unsigned char *rgb_frame)
{
	// Update the texture with the new frame
	SDL_UpdateTexture(sdl_info->sdl_handle.texture, NULL, rgb_frame, sdl_info->width * 3);

	// Clear the renderer
	SDL_RenderClear(sdl_info->sdl_handle.renderer);

	// Copy the texture to the renderer
	//SDL_RenderCopy(renderer, texture, NULL, NULL);
	if (SDL_RenderCopy(sdl_info->sdl_handle.renderer, sdl_info->sdl_handle.texture, NULL, NULL) != 0) {
		LOG_ERROR("SDL_RenderCopy error: %s\n", SDL_GetError());
		sdl_deinit(sdl_info);
		return -1;
	}

	// Present the rendered image to the window
	SDL_RenderPresent(sdl_info->sdl_handle.renderer);
	return 0;
}

