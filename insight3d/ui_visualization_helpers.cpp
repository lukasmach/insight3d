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

#include "ui_visualization_helpers.h"

// draw helper cuber
void visualization_helper_cube()
{
	// draw cube
	opengl_drawing_style(UI_STYLE_HELPERS);
	glBegin(GL_LINES); 
		glColor3f(1, 0, 0); 
		glVertex3f(-1, -1, -1); glVertex3f(1, -1, -1); 
		glColor3f(0, 1, 0); 
		glVertex3f(-1, -1, -1); glVertex3f(-1, 1, -1); 
		glColor3f(0, 0, 1);
		glVertex3f(-1, -1, -1); glVertex3f(-1, -1, 1); 

		glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1); glVertex3f(-1, 1, 1); 
		glVertex3f(1, 1, 1); glVertex3f(1, -1, 1); 
		glVertex3f(1, 1, 1); glVertex3f(1, 1, -1); 
		glVertex3f(-1, -1, 1); glVertex3f(-1, -1, -1); 
		glVertex3f(-1, -1, 1); glVertex3f(-1, 1, 1); 
		glVertex3f(-1, -1, 1); glVertex3f(1, -1, 1); 
		glVertex3f(-1, 1, -1); glVertex3f(1, 1, -1); 
		glVertex3f(-1, 1, -1); glVertex3f(-1, -1, -1); 
		glVertex3f(-1, 1, -1); glVertex3f(-1, 1, 1); 
		glVertex3f(1, -1, -1); glVertex3f(-1, -1, -1); 
		glVertex3f(1, -1, -1); glVertex3f(1, 1, -1); 
		glVertex3f(1, -1, -1); glVertex3f(1, -1, 1); 
	glEnd(); 
}

// square vertices 
void visualization_helper_square(double x, double y, double side)
{
	// draw square 
	glVertex3d(x - side, y - side, -1); 
	glVertex3d(x + side, y - side, -1); 
	glVertex3d(x + side, y + side, -1); 
	glVertex3d(x - side, y + side, -1); 
}

// convert x axis distance from pixels to opengl coordinates
double visualization_calc_dx(const double screen_distance_x)
{
	ASSERT(INDEX_IS_SET(ui_state.current_shot), "cannot calculate coordinates with respect to image size when current shot is undefined");
	ASSERT_IS_SET(shots, ui_state.current_shot);
	ASSERT(shots.data[ui_state.current_shot].info_status >= GEOMETRY_INFO_DEDUCED, "image proportions must be known when computing coordinates with respect to image size");

	UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
	// return screen_distance_x * ( * meta->view_zoom / ui_state.glview->wid.w);
	return 2 * screen_distance_x * ((shots.data[ui_state.current_shot].height / (double)shots.data[ui_state.current_shot].width) * 2 * meta->view_zoom / gui_get_height(ui_state.gl));
}

// convert y axis distance from pixels to opengl coordinates
double visualization_calc_dy(const double screen_distance_y)
{
	ASSERT(INDEX_IS_SET(ui_state.current_shot), "cannot calculate coordinates with respect to image size when current shot is undefined");
	ASSERT_IS_SET(shots, ui_state.current_shot);
	ASSERT(shots.data[ui_state.current_shot].info_status >= GEOMETRY_INFO_DEDUCED, "image proportions must be known when computing coordinates with respect to image size");

	UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
	return 2 * screen_distance_y * (2 * meta->view_zoom / gui_get_height(ui_state.gl));
}

// absolute version of visualization_calc_dx
double visualization_calc_x(const double shot_x, const double screen_distance_x)
{
	return shot_x + visualization_calc_dx(screen_distance_x);
}

// absolute version of visualization_calc_dy
double visualization_calc_y(const double shot_y, const double screen_distance_y)
{
	return shot_y + visualization_calc_dy(screen_distance_y);
}

// recalculate squared distance from image pixels to screen pixels 
double visualization_calc_screen_distance_sq(double distance) 
{
	ASSERT(INDEX_IS_SET(ui_state.current_shot), "cannot calculate image distance current shot is undefined");
	ASSERT_IS_SET(shots, ui_state.current_shot);
	ASSERT(shots.data[ui_state.current_shot].info_status >= GEOMETRY_INFO_DEDUCED, "image proportions must be known when computing coordinates with respect to image size");

	UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
	return sqr_value(sqrt(distance) * (gui_get_height(ui_state.gl) / (2 * meta->view_zoom * shots.data[ui_state.current_shot].height)));
}
