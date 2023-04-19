#ifndef __INTERFACE_SDL
#define __INTERFACE_SDL

#include "SDL/SDL.h"

// SDL initialization 
// note unused when using AgarGUI, but might be useful someday
void sdl_initialize(int window_width, int window_height);

// reset keys status to false
void sdl_clear_keys(Uint8 * keys, bool * clear_keys, size_t keys_length);

// map rgb color defined as vector 
Uint32 sdl_map_rgb_vector(const SDL_PixelFormat * const format, const double color[3]);

// button is mouse wheel 
bool sdl_wheel_button(int button);

// get modifiers 
bool sdl_shift_pressed();
bool sdl_ctrl_pressed();
bool sdl_alt_pressed();

#endif
