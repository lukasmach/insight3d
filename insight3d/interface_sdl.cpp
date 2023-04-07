#include "interface_sdl.h"

// SDL initialization 
// note unused when using AgarGUI, but might be useful someday
void sdl_initialize(int window_width, int window_height)
{	
	const SDL_VideoInfo * info = NULL;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) 
	{
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit(1);
	}

	atexit(SDL_Quit);

	info = SDL_GetVideoInfo();
	if (!info)
	{
		fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int bpp = info->vfmt->BitsPerPixel;
	int flags = SDL_OPENGL;// | SDL_RESIZABLE; // debug

	if (SDL_SetVideoMode(window_width, window_height, bpp, flags) == 0)
	{
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("OpenGL window", NULL);
}

// reset keys status to false
void sdl_clear_keys(Uint8 * keys, bool * clear_keys, size_t keys_length)
 {
	 for (size_t i = 0; i < keys_length; i++) 
		 if (clear_keys[i]) 
			 keys[i] = 0;
 }

// map rgb color defined as vector 
Uint32 sdl_map_rgb_vector(const SDL_PixelFormat * const format, const double color[3])
{
	return SDL_MapRGB(format, (Uint32)(255 * color[0]), (Uint32)(255 * color[1]), (Uint32)(255 * color[2]));
}

// button is mouse wheel 
bool sdl_wheel_button(int button) 
{
	return button == SDL_BUTTON_WHEELUP || button == SDL_BUTTON_MIDDLE || button == SDL_BUTTON_WHEELDOWN;
}

// get modifiers 
bool sdl_shift_pressed() 
{
	SDLMod mod = SDL_GetModState();
	return (bool)(mod & KMOD_SHIFT);
}

bool sdl_ctrl_pressed() 
{
	SDLMod mod = SDL_GetModState();
	return (bool)(mod & KMOD_CTRL);
}
bool sdl_alt_pressed() 
{
	SDLMod mod = SDL_GetModState();
	return (bool)(mod & KMOD_ALT);
}
