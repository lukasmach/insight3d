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

#ifndef __UI_VISUALIZATION
#define __UI_VISUALIZATION

#include "interface_opengl.h"
#include "core_constants.h"
#include "core_math_routines.h"
#include "ui_constants.h"
#include "ui_core.h"
#include "geometry_structures.h"
#include "geometry_queries.h"

// visualization state value 
struct Visualization_State { 

	// statistics
	double shots_T_mean[3], shots_T_deviation[3], max_dev;    // mean and variance of the cameras' positions
	size_t finite_shots_count; 

	// inspection mode 
	double T[3];    // user camera position in space
	double R[3];    // user camera orientation (radians)
	double point_of_focus[3];    // position of the point on which user camera is focused 

	// continuous image loading 
	double continuous_loading_alpha; 

	// inspection mode - orientation using ground plane
	bool ground_mode;    // ground inspection is on/off
	double * ground;     // homogeneous coordinates of ground plane (inhomogeneous part, i.e. first 3 values, 
	                     // is always normalized and thus forms unit length normal vector of the plane)
	                     // note maybe check that it always points 'upwards'
	double ground_POI[3];    // point of interest 
	double ground_axis_x[3], ground_axis_y[3], ground_axis_z[3];    // perpendicular unit vectors lying on ground plane and specifying 
                                                                    // (together with plane's normal vector) coordinate frame
	double ground_alpha, ground_phi, ground_distance; // distance and angles used to orient in space while in ground plane inspection

	// opengl state variables
	GLdouble opengl_modelview[16], opengl_projection[16]; 
	int opengl_viewport[4];

};

extern Visualization_State visualization_state; 

// initialize visualization
bool visualization_initialize();

// save opengl transformation matrices into visualization state structure
void visualization_export_opengl_matrices();

// prepare inspection projection 
void visualization_prepare_inspection_projection(double fovx);

// calculate the view_zoom factor in x axis // todo write better description 
const double visualization_get_zoom_x();

// calculate the viewport coordinates on OpenGL clipping plane (using current zooming and scrolling settings) 
void visualization_viewport_in_shot_coordinates(double & x1, double & y1, double & x2, double & y2);

// prepare drawing using perspective projection
void visualization_prepare_projection();

// prepare drawing in image space 
void visualization_prepare_planar_drawing();

// end drawing in image space 
void visualization_end_planar_drawing();

// processes data so that they can be eventually nicely displayed (mean and deviance of camera centers, etc.)
bool visualization_process_data_cameras(Shots shots);
bool visualization_process_data_vertices(Vertices vertices);
void visualization_process_data(Vertices vertices, Shots shots);

// normalize coordinate 
double visualization_normalize(const double x, const Core_Axes axis);
double visualization_normalize_linear(const double x, const Core_Axes axis);

// denormalize coordiante 
double visualization_denormalize(const double x, const Core_Axes axis);

// denormalize 3-vector 
void visualization_denormalize_vector(double * point);

// calculate distance between two points in shot coordinates in screen pixels 
// unused because query_nearest_point is flexible enough; might be useful someday though
double visualization_screen_distance_sq(const double x1, const double y1, const double x2, const double y2);

// find nearest point on shot 
double visualization_nearest_point(const size_t shot_id, const double x, const double y, size_t & point_id);

// show selection box 
void visualization_selection_box(double x1, double y1, double x2, double y2);

// displays camera centers as points in normalized space
// conditions: visualization_process_data has to be run before the first time calling this function on modified (or newly constructed) data
void visualization_cameras(const Shots shots, const double world_scale = 1);

// display vertices using normalization from visualization_state
void visualization_vertices(const Vertices & vertices, double world_scale = 1);

// display reconstructed polygons 
void visualization_polygons(const Polygons_3d & polygons, const double world_scale = 1);

// display 3d reconstruction contours (which are 2d shots' polygons)
void visualization_contours(const Shots & shots, const Vertices & vertices, const double world_scale = 1);

// place user camera (image plane mode)
void visualization_shot_user_camera(const double world_scale = 1);

// show shot
void visualization_shot_image(Shot & shot);

// show dualview 
void visualization_show_dualview(size_t current_shot_id, size_t dual_shot_id, const double x, const double y);

// draw rectangle with shadow
void visualization_rect_with_shadow(double left, double top, double right, double bottom, double texture_left, double texture_top, double texture_right, double texture_bottom, double alpha);

// show polygons in shot mode 
void visualization_shot_polygons(const size_t shot_id);

// show all of the contours
void visualization_shot_contours(const size_t shot_id);

// place user camera (inspection mode)
void visualization_inspection_user_camera(double world_scale = 1);

// fit image to viewport
void visualization_fit_to_viewport();

// move image into viewport - if the user scrolls zooms or scrolls out of image, we want to contrain him (that's what she said!)
void visualization_move_into_viewport(bool x_axis = true, bool y_axis = true);

#endif

