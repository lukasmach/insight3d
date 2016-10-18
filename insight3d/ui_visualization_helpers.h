/*

  insight3d - image based 3d modelling software
  Copyright (C) 2007-2008  Lukas Mach
                           email: lukas.mach@gmail.com 
                           web: http://mach.matfyz.cz/

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
  
*/

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
