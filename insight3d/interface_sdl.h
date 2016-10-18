/*

  Image-based 3d modelling software
  Copyright (C) 2007-2008  Lukas Mach
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __INTERFACE_SDL
#define __INTERFACE_SDL

#include "SDL.h"

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
