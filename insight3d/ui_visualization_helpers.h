#ifndef __UI_VISUALIZATION_HELPERS
#define __UI_VISUALIZATION_HELPERS

#include "interface_opengl.h"
#include "core_debug.h"
#include "core_math_routines.h"
#include "ui_constants.h"
#include "ui_state.h"
#include "ui_core.h"

// draw helper cuber
void visualization_helper_cube();

// square vertices 
void visualization_helper_square(double x, double y, double side);

// convert x axis distance from pixels to opengl coordinates
double visualization_calc_dx(const double screen_distance_x);

// convert y axis distance from pixels to opengl coordinates
double visualization_calc_dy(const double screen_distance_y);

// absolute version of visualization_calc_dx
double visualization_calc_x(const double shot_x, const double screen_distance_x);

// absolute version of visualization_calc_dy
double visualization_calc_y(const double shot_y, const double screen_distance_y);

// recalculate squared distance from image pixels to screen pixels 
double visualization_calc_screen_distance_sq(double distance);

#endif
