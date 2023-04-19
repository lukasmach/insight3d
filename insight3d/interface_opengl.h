#ifndef __INTERFACE_OPENGL
#define __INTERFACE_OPENGL

#ifdef _MSC_VER
#include "windows.h"
#endif

#include "SDL/SDL.h" 
#include "SDL/SDL_opengl.h"
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
