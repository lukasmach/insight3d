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

#ifndef __INTERFACE_OPENGL
#define __INTERFACE_OPENGL

#ifdef _MSC_VER
#include "windows.h"
#endif

#include "SDL.h" 
#include "SDL_opengl.h"
#include "pthread.h"

extern pthread_mutex_t opengl_mutex;

// #include "GL/gl.h"
// #include "GL/glu.h"

// drawing options (for drawing polygons, correspodences, etc.) 
struct Drawing_Style { 
	float color[3], line_width, point_size, opacity;
};

// go to 2d mode 
void opengl_2d_mode(double x1, double y1, double x2, double y2);

// restore saved matrices
void opengl_end_2d_mode();

// set drawing style
void opengl_drawing_style(const Drawing_Style & style);

// saves settings of some common OpenGL attributes
void opengl_push_attribs();

#endif
