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

#include "ui_visualization.h"

Visualization_State visualization_state; 

// initialize visualization
bool visualization_initialize()
{
	memset(&visualization_state, 0, sizeof(Visualization_State));
	DYN(ui_state.groups, 0);
	DYN(ui_state.groups, 1);
	return true;
}

// save opengl transformation matrices into visualization state structure
void visualization_export_opengl_matrices()
{
	glGetDoublev(GL_PROJECTION_MATRIX, visualization_state.opengl_projection);
	glGetDoublev(GL_MODELVIEW_MATRIX, visualization_state.opengl_modelview);
	glGetIntegerv(GL_VIEWPORT, visualization_state.opengl_viewport);
}

// prepare inspection projection 
void visualization_prepare_inspection_projection(double fovx) 
{
	LOCK(opengl) 
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		const double ratio = gui_get_width(ui_state.gl) / (double)gui_get_height(ui_state.gl);
		const double fovy = fovx / ratio;
		gluPerspective(fovx, ratio, 0.1, 1000);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glEnable(GL_DEPTH_TEST);
	}
	UNLOCK(opengl);
}

// calculate the view_zoom factor in x axis // todo write better description 
const double visualization_get_zoom_x() 
{
	const double
		shot_ratio = shots.data[ui_state.current_shot].width / (double)shots.data[ui_state.current_shot].height,
		window_ratio = gui_get_width(ui_state.gl) / (double)gui_get_height(ui_state.gl);
	const UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);

	return meta->view_zoom * window_ratio / shot_ratio;
}

// calculate the viewport coordinates on OpenGL clipping plane (using current zooming and scrolling settings) 
void visualization_viewport_in_shot_coordinates(double & x1, double & y1, double & x2, double & y2) 
{
	const double
		shot_ratio = shots.data[ui_state.current_shot].width / (double)shots.data[ui_state.current_shot].height,
		window_ratio = gui_get_width(ui_state.gl) / (double)gui_get_height(ui_state.gl);
	UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
	y1 = (1 - meta->view_center_y) - meta->view_zoom;
	y2 = (1 - meta->view_center_y) + meta->view_zoom;
	x1 = meta->view_center_x - meta->view_zoom * window_ratio / shot_ratio;
	x2 = meta->view_center_x + meta->view_zoom * window_ratio / shot_ratio;
}

// prepare drawing using perspective projection
void visualization_prepare_projection()
{
	// prepare opengl for rendering
	glShadeModel(GL_SMOOTH);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// setup projection
	double x1, y1, x2, y2;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	visualization_viewport_in_shot_coordinates(x1, y1, x2, y2);

	const double t = tan(0.5 * shots.data[ui_state.current_shot].fovx / 180.0 * 3.14159265358979323846);
	const double q = 0.1 * t;
	x1 = q * (x1 * 2 - 1); 
	x2 = q * (x2 * 2 - 1); 
	y1 = q * (y1 * 2 - 1);
	y2 = q * (y2 * 2 - 1);

	glFrustum(x1, x2, y1, y2, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// prepare drawing in image space 
void visualization_prepare_planar_drawing() 
{
	double x1, y1, x2, y2;
	visualization_viewport_in_shot_coordinates(x1, y1, x2, y2);
	opengl_2d_mode(x1, y1, x2, y2);
}

// end drawing in image space 
void visualization_end_planar_drawing()
{
	opengl_end_2d_mode();
}

// processes data so that they can be eventually nicely displayed (using mean and deviance of camera centers)
bool visualization_process_data_cameras(Shots shots)
{
	// compute average and variance of all cameras' positions
	double sum[3] = {0, 0, 0};
	size_t finite_shots_count = 0; // number of cameras which are not located on the plane at infinity 

	// compute sums (and also computes inhomogeneous coordinates of camera centers) 
	// also compute the vectors to visualize field of view
	// note maybe move the inhomogenization out of visualization (into geometry) 
	for (size_t i = 0; i < shots.count; i++)
	{
		if (!nearly_zero(shots.data[i].T[W]))
		{
			shots.data[i].visualization_T[X] = shots.data[i].T[X] / shots.data[i].T[W];
			shots.data[i].visualization_T[Y] = shots.data[i].T[Y] / shots.data[i].T[W];
			shots.data[i].visualization_T[Z] = shots.data[i].T[Z] / shots.data[i].T[W];

			sum[X] += shots.data[i].visualization_T[X];
			sum[Y] += shots.data[i].visualization_T[Y]; 
			sum[Z] += shots.data[i].visualization_T[Z];

			finite_shots_count++;

			// compute vectors to visualize field of view

			/*
			// variant that clamps down some values in internal calibration
			CvMat * P_corrected = opencv_create_matrix(3, 4); 
			CvMat * internal_calibration = cvCloneMat(shots.data[i].internal_calibration);
			opencv_debug("original internal calibration", internal_calibration); 
			mvg_restrict_calibration_matrix(internal_calibration, true, true);
			opencv_debug("restricted internal calibration", internal_calibration); 

			mvg_assemble_projection_matrix(internal_calibration, shots.data[i].rotation, shots.data[i].translation, P_corrected);

			opencv_debug("original projection matrix", shots.data[i].projection);
			opencv_debug("restricted projection matrix", P_corrected);
			*/

			CvMat * P_pseudoinverse = opencv_create_matrix(4, 3); 
			cvInvert(shots.data[i].projection, P_pseudoinverse, CV_SVD);

			/* 
			// debug code 
			CvMat * x = opencv_create_matrix(3, 1);
			OPENCV_ELEM(x, 0, 0) = 2000; 
			OPENCV_ELEM(x, 1, 0) = 1000; 
			OPENCV_ELEM(x, 2, 0) = 1;
			CvMat * X = opencv_create_matrix(4, 1); 
			cvMatMul(P_pseudoinverse, x, X);
			cvMatMul(shots.data[i].projection, X, x); 
			opencv_normalize_homogeneous(x);
			opencv_debug("reprojected X", x); */

			const double
				normalize_10 = OPENCV_ELEM(P_pseudoinverse, 3, 2) + OPENCV_ELEM(P_pseudoinverse, 3, 0) * shots.data[i].width,
				normalize_01 = OPENCV_ELEM(P_pseudoinverse, 3, 2) + OPENCV_ELEM(P_pseudoinverse, 3, 1) * shots.data[i].height,
				normalize_11 = 
					OPENCV_ELEM(P_pseudoinverse, 3, 2) + 
					OPENCV_ELEM(P_pseudoinverse, 3, 0) * shots.data[i].width + 
					OPENCV_ELEM(P_pseudoinverse, 3, 1) * shots.data[i].height
			;

			for (size_t k = 0; k < 3; k++) 
			{
				shots.data[i].visualization_pyr_00[k] = 
					OPENCV_ELEM(P_pseudoinverse, k, 2) / OPENCV_ELEM(P_pseudoinverse, 3, 2);

				shots.data[i].visualization_pyr_10[k] = 
					(OPENCV_ELEM(P_pseudoinverse, k, 2) + shots.data[i].width * OPENCV_ELEM(P_pseudoinverse, k, 0))
					/ normalize_10;

				shots.data[i].visualization_pyr_01[k] = 
					(OPENCV_ELEM(P_pseudoinverse, k, 2) + shots.data[i].height * OPENCV_ELEM(P_pseudoinverse, k, 1))
					/ normalize_01;

				shots.data[i].visualization_pyr_11[k] = 
					(OPENCV_ELEM(P_pseudoinverse, k, 2) + shots.data[i].width * OPENCV_ELEM(P_pseudoinverse, k, 0) + shots.data[i].height * OPENCV_ELEM(P_pseudoinverse, k, 1))
					/ normalize_11;
			}

			sub_3(shots.data[i].visualization_pyr_00, shots.data[i].visualization_T, shots.data[i].visualization_pyr_00);
			sub_3(shots.data[i].visualization_pyr_10, shots.data[i].visualization_T, shots.data[i].visualization_pyr_10);
			sub_3(shots.data[i].visualization_pyr_01, shots.data[i].visualization_T, shots.data[i].visualization_pyr_01);
			sub_3(shots.data[i].visualization_pyr_11, shots.data[i].visualization_T, shots.data[i].visualization_pyr_11);

			normalize_vector(shots.data[i].visualization_pyr_00, 3);
			normalize_vector(shots.data[i].visualization_pyr_10, 3);
			normalize_vector(shots.data[i].visualization_pyr_11, 3);
			normalize_vector(shots.data[i].visualization_pyr_01, 3);

			// check if it's in front of the camera 
			CvMat * point = opencv_create_matrix(3, 1);
			OPENCV_ELEM(point, 0, 0) = shots.data[i].visualization_pyr_00[X]; 
			OPENCV_ELEM(point, 1, 0) = shots.data[i].visualization_pyr_00[Y]; 
			OPENCV_ELEM(point, 2, 0) = shots.data[i].visualization_pyr_00[Z]; 
			const double projective_depth = mvg_projective_depth(shots.data[i].projection, point);
			cvReleaseMat(&point);

			// and reverse if necessary 
			if (projective_depth <= 0)
			{
				mul_3(-1, shots.data[i].visualization_pyr_00, shots.data[i].visualization_pyr_00); 
				mul_3(-1, shots.data[i].visualization_pyr_01, shots.data[i].visualization_pyr_01); 
				mul_3(-1, shots.data[i].visualization_pyr_10, shots.data[i].visualization_pyr_10); 
				mul_3(-1, shots.data[i].visualization_pyr_11, shots.data[i].visualization_pyr_11); 
			}

			/*
			// debug check reprojection
			double reprojection[2]; 
			opencv_vertex_projection_visualization(
				P_corrected,
				shots.data[i].visualization_pyr_01[X], 
				shots.data[i].visualization_pyr_01[Y], 
				shots.data[i].visualization_pyr_01[Z],
				reprojection
			);*/

			cvReleaseMat(&P_pseudoinverse);
		}
	}

	// compute average and variance
	double avg[3] = {0, 0, 0}; 
	double dev[3] = {0, 0, 0}; 
	double max_dev = 0;

	if (finite_shots_count)
	{
		avg[X] = sum[X] / finite_shots_count;
		avg[Y] = sum[Y] / finite_shots_count; 
		avg[Z] = sum[Z] / finite_shots_count; 

		for (size_t i = 0; i < shots.count; i++)
		{
			if (!nearly_zero(shots.data[i].T[W]))
			{
				dev[X] += sqr_value(avg[X] - shots.data[i].visualization_T[X]);
				dev[Y] += sqr_value(avg[Y] - shots.data[i].visualization_T[Y]); 
				dev[Z] += sqr_value(avg[Z] - shots.data[i].visualization_T[Z]);
			}
		}

		dev[X] = sqrt(dev[X] / finite_shots_count); 
		dev[Y] = sqrt(dev[Y] / finite_shots_count); 
		dev[Z] = sqrt(dev[Z] / finite_shots_count);
		max_dev = max_value(dev[X], max_value(dev[Y], dev[Z])); 
	}

	// save the values into application state structure 
	visualization_state.shots_T_mean[0] = avg[0]; 
	visualization_state.shots_T_mean[1] = avg[1]; 
	visualization_state.shots_T_mean[2] = avg[2]; 
	visualization_state.shots_T_deviation[0] = dev[0]; 
	visualization_state.shots_T_deviation[1] = dev[1]; 
	visualization_state.shots_T_deviation[2] = dev[2]; 
	visualization_state.max_dev = max_dev; 
	visualization_state.finite_shots_count = finite_shots_count; 

	// initialize user's camera to be the first finite camera in our space
	visualization_state.T[X] = 0; 
	visualization_state.T[Y] = 0; 
	visualization_state.T[Z] = 0; 

	for (size_t i = 0; i < shots.count; i++)
	{
		if (!nearly_zero(shots.data[i].T[4]))
		{
			visualization_state.T[X] = shots.data[i].visualization_T[X];
			visualization_state.T[Y] = shots.data[i].visualization_T[Y];
			visualization_state.T[Z] = shots.data[i].visualization_T[Z];
			break;
		}
	}

	return true;
}

// processes data so that they can be eventually nicely displayed (mean and deviance of the point cloud, etc.)
bool visualization_process_data_vertices(Vertices vertices) 
{
	size_t all_count = 0, manual_count = 0; 
	
	// first check if we have enough points 
	for ALL(vertices, i) 
	{
		const Vertex * const vertex = vertices.data + i;

		if (vertex->reconstructed)
		{
			all_count++; 

			if (vertex->vertex_type == GEOMETRY_VERTEX_USER)
			{
				manual_count++;
			}

			if (all_count > 2 && manual_count > 2) break; 
		}
	}

	// decide what points to use 
	bool use_man = manual_count > 2; 
	if (all_count <= 2) return false;

	// calculate the mean and variance 
	double mean[3], var[3], sum[3];
	size_t count = 0;

	sum[X] = 0; 
	sum[Y] = 0; 
	sum[Z] = 0;

	for ALL(vertices, i)
	{
		const Vertex * const vertex = vertices.data + i;

		if (vertex->reconstructed)
		{
			if (use_man && vertex->vertex_type != GEOMETRY_VERTEX_USER) continue;

			sum[X] += vertex->x;
			sum[Y] += vertex->y;
			sum[Z] += vertex->z;
			count++;
		}
	}

	mean[X] = sum[X] / count;
	mean[Y] = sum[Y] / count;
	mean[Z] = sum[Z] / count;

	sum[X] = 0;
	sum[Y] = 0;
	sum[Z] = 0;

	// we have the mean, we can calculate variance
	for ALL(vertices, i)
	{
		const Vertex * const vertex = vertices.data + i;

		if (vertex->reconstructed)
		{
			if (use_man && vertex->vertex_type != GEOMETRY_VERTEX_USER) continue;

			sum[X] += sqr_value(mean[X] - vertex->x);
			sum[Y] += sqr_value(mean[Y] - vertex->y);
			sum[Z] += sqr_value(mean[Z] - vertex->z);
		}
	}

	var[X] = sqrt(sum[X] / count);
	var[Y] = sqrt(sum[Y] / count);
	var[Z] = sqrt(sum[Z] / count);

	const double max_var = max_value(var[X], max_value(var[Y], var[Z]));

	visualization_state.shots_T_mean[0] = mean[0]; 
	visualization_state.shots_T_mean[1] = mean[1]; 
	visualization_state.shots_T_mean[2] = mean[2]; 
	visualization_state.shots_T_deviation[0] = var[0]; 
	visualization_state.shots_T_deviation[1] = var[1]; 
	visualization_state.shots_T_deviation[2] = var[2]; 
	visualization_state.max_dev = max_var; 

	return true;
}

// processes data so that they can be eventually nicely displayed (will decide whether to use point cloud or cameras)
void visualization_process_data(Vertices vertices, Shots shots) 
{
	visualization_process_data_cameras(shots); // todo this routine must be called to calculate inhomogeneous coordinate, etc.; should be changed
	visualization_process_data_vertices(vertices); // this routine will overwrite the normalization transformation if there's enough points
}

// normalize coordinate 
double visualization_normalize(const double x, const Core_Axes axis)
{
	if (visualization_state.max_dev == 0) return x;
	return (x - visualization_state.shots_T_mean[axis]) / visualization_state.max_dev; 
}

// normalize coordinate - no translation
double visualization_normalize_linear(const double x, const Core_Axes axis)
{
	if (visualization_state.max_dev == 0) return x;
	return x / visualization_state.max_dev; 
}

// denormalize coordiante 
double visualization_denormalize(const double x, const Core_Axes axis) 
{
	return x * visualization_state.max_dev + visualization_state.shots_T_mean[axis];
}

// denormalize 3-vector 
void visualization_denormalize_vector(double * point)
{
	point[0] = visualization_denormalize(point[0], X); 
	point[1] = visualization_denormalize(point[1], Y); 
	point[2] = visualization_denormalize(point[2], Z); 
}

// calculate distance between two points in shot coordinates in screen pixels 
// unused because query_nearest_point is flexible enough; might be useful someday though
double visualization_screen_distance_sq(const double x1, const double y1, const double x2, const double y2) 
{
	return sqr_value((x1 - x2) * (double)(ui_state.gl->effective_x2 - ui_state.gl->effective_x1)); //(double)(AG_WIDGET(ui_state.glview))) + sqr((y1 - y2) * (double)(AG_WIDGET(ui_state.glview)->y));
}

// find nearest point on shot 
double visualization_nearest_point(const size_t shot_id, const double x, const double y, size_t & point_id)
{
	ASSERT(validate_shot(shot_id), "invalid shot when finding nearest point");

	// find nearest points and it's squared distance in image pixels 
	double distance = query_nearest_point(shot_id, x, y, point_id);

	if (distance >= 0.0) 
	{
		// todo convert shot distance to screen pixels 
		return sqrt(distance);
	}
	else
	{
		return distance;
	}
}

// show selection box 
void visualization_selection_box(double x1, double y1, double x2, double y2)
{
	// normalize order and convert to opengl coordinates
	if (x1 > x2) swap_double(x1, x2);
	if (y1 > y2) swap_double(y1, y2);
	ui_convert_xy_from_shot_to_opengl(x1, y1, x1, y1);
	ui_convert_xy_from_shot_to_opengl(x2, y2, x2, y2);

	// show box
	// show border
	// opengl_drawing_style(UI_STYLE_SELECTION_BORDER);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0, 0, 0, 0.5);
	/* glBegin(GL_POLYGON); 
		glVertex3d(x1, y1, -1);
		glVertex3d(x1, y2, -1);
		glVertex3d(x2, y2, -1);
		glVertex3d(x2, y1, -1);
	glEnd(); */ 
	glBegin(GL_POLYGON); 
		glVertex3d(-1, 1, -1); 
		glVertex3d(1, 1, -1); 
		glVertex3d(1, y1, -1); 
		glVertex3d(-1, y1, -1);
	glEnd();
	glBegin(GL_POLYGON); 
		glVertex3d(-1, -1, -1); 
		glVertex3d(1, -1, -1); 
		glVertex3d(1, y2, -1); 
		glVertex3d(-1, y2, -1);
	glEnd();
	glBegin(GL_POLYGON); 
		glVertex3d(-1, y1, -1); 
		glVertex3d(x1, y1, -1); 
		glVertex3d(x1, y2, -1); 
		glVertex3d(-1, y2, -1);
	glEnd();
	glBegin(GL_POLYGON); 
		glVertex3d(1, y1, -1); 
		glVertex3d(x2, y1, -1); 
		glVertex3d(x2, y2, -1); 
		glVertex3d(1, y2, -1);
	glEnd();
	glDisable(GL_BLEND);
}

// displays camera centers as points in normalized space
// conditions: visualization_process_data has to be run before the first time calling this function on modified (or newly constructed) data
void visualization_cameras(const Shots shots, const double world_scale /*= 1*/)
{
	LOCK(opengl)
	{
		// if we have something (finite, of course) to show 
		if (visualization_state.finite_shots_count)
		{
			// we'll display all the camera centers as points
			// idea what about using clustering or ransac to group cameras?
			opengl_drawing_style(UI_STYLE_CAMERA);
			const double camera_size = 6;

			for (size_t i = 0; i < shots.count; i++)
			{
				const bool current = (INDEX_IS_SET(ui_state.current_shot) && ui_state.current_shot == i);

				if (!nearly_zero(shots.data[i].T[W]))
				{
					if (!current) glColor3f(0.9, 0.9, 0.9); else glColor3f(0.9, 0.45, 0.45);
					glBegin(GL_POLYGON);
					
					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_00[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_00[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_00[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_10[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_10[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_10[Z], Z)
					);

					glEnd();

					if (!current) glColor3f(0.75, 0.75, 0.75); else glColor3f(0.75, 0.375, 0.374);
					glBegin(GL_POLYGON);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_10[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_10[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_10[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_11[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_11[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_11[Z], Z)
					);

					glEnd();

					glBegin(GL_POLYGON);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_11[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_11[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_11[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_01[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_01[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_01[Z], Z)
					);

					glEnd();

					if (!current) glColor3f(0.8, 0.8, 0.8); else glColor3f(0.8, 0.4, 0.4);
					glBegin(GL_POLYGON);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_01[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_01[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_01[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_00[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_00[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_00[Z], Z)
					);

					glEnd();

					/*if (!current) glColor3f(1, 1, 1); else glColor3f(1, 0.5, 0.5);
					glBegin(GL_POLYGON);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_00[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_00[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_00[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_01[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_01[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_01[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_11[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_11[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_11[Z], Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(shots.data[i].visualization_T[X] + camera_size * shots.data[i].visualization_pyr_10[X], X),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Y] + camera_size * shots.data[i].visualization_pyr_10[Y], Y),
						world_scale * visualization_normalize(shots.data[i].visualization_T[Z] + camera_size * shots.data[i].visualization_pyr_10[Z], Z)
					);

					glEnd();*/
				}
			}

			glEnd();
		}
		else
		{
			// there are either no cameras or all are at infinity 
		}
	}
	UNLOCK(opengl);
}

// display vertices using normalization from visualization_state
void visualization_vertices(const Vertices & vertices, double world_scale /*= 1*/)
{
	LOCK(opengl)
	{
		opengl_drawing_style(UI_STYLE_VERTEX);

		glDisable(GL_BLEND);
		glBegin(GL_POINTS);

		// go through all vertices
		for ALL(vertices, i)
		{
			// process only reconstructed vertices
			if (!vertices.data[i].reconstructed) continue;

			// don't display hidden vertices
			if (vertices.data[i].group)
			{
				ASSERT_IS_SET(ui_state.groups, vertices.data[i].group);
				if (ui_state.groups.data[vertices.data[i].group].hidden) continue;
			}
		
			// optionally skip generated vertices 
			if (option_hide_automatic && vertices.data[i].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

			// pick appropriate style 
			if (!vertices.data[i].selected)
			{
				opengl_drawing_style(UI_STYLE_VERTEX);

				if (vertices.data[i].color[0] > 0 || vertices.data[i].color[1] > 0 || vertices.data[i].color[2] > 0)
				{
					glColor4f(vertices.data[i].color[0], vertices.data[i].color[1], vertices.data[i].color[2], 1);
				}
				else if (vertices.data[i].group)
				{
					if (vertices.data[i].group == 1) 
					{
						glColor3d(0.52, 0.83, 0.52);
					}
				}
			}
			else
			{
				opengl_drawing_style(UI_STYLE_SELECTED_VERTEX);
			}

			// display them in normalized space
			glVertex3d(
				world_scale * visualization_normalize(vertices.data[i].x, X),
				world_scale * visualization_normalize(vertices.data[i].y, Y),
				world_scale * visualization_normalize(vertices.data[i].z, Z)
			);
		}

		glEnd(); 

		const double normal_len = 0.1;

		// glColor4f(1, 1, 1, 0.3);
		glBegin(GL_LINES);

		// go through all vertices
		for ALL(vertices, i)
		{
			// process only reconstructed vertices
			if (!vertices.data[i].reconstructed) continue;

			// don't display hidden vertices
			if (vertices.data[i].group)
			{
				ASSERT_IS_SET(ui_state.groups, vertices.data[i].group);
				if (ui_state.groups.data[vertices.data[i].group].hidden) continue;
			}
		
			// optionally skip generated vertices 
			if (option_hide_automatic && vertices.data[i].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

			// pick appropriate style 
			if (!vertices.data[i].selected)
			{
				opengl_drawing_style(UI_STYLE_VERTEX);

				if (vertices.data[i].color[0] > 0 || vertices.data[i].color[1] > 0 || vertices.data[i].color[2] > 0)
				{
					glColor4f(vertices.data[i].color[0], vertices.data[i].color[1], vertices.data[i].color[2], 1.0);
				}
				else if (vertices.data[i].group)
				{
					if (vertices.data[i].group == 1) // debug ?
					{
						glColor3d(0.52, 0.83, 0.52);
					}
				}
			}
			else
			{
				opengl_drawing_style(UI_STYLE_SELECTED_VERTEX);
			}

			// * display the direction of the normal *

			glVertex3d(
				world_scale * (visualization_normalize(vertices.data[i].x, X) + normal_len * vertices.data[i].nx),
				world_scale * (visualization_normalize(vertices.data[i].y, Y) + normal_len * vertices.data[i].ny),
				world_scale * (visualization_normalize(vertices.data[i].z, Z) + normal_len * vertices.data[i].nz)
			);

			glVertex3d(
				world_scale * visualization_normalize(vertices.data[i].x, X), 
				world_scale * visualization_normalize(vertices.data[i].y, Y),
				world_scale * visualization_normalize(vertices.data[i].z, Z)
			);
		}

		glEnd();

		// go through all detected edges 
		/*glLineWidth(1.0);
		glBegin(GL_LINES);
		glColor3d(1, 1, 1);
		for (std::map<int, std::map<int, unsigned int> >::iterator edge_i1 = detected_edges.begin(); edge_i1 != detected_edges.end(); ++edge_i1)
		{
			for (std::map<int, unsigned int>::iterator edge_i2 = edge_i1->second.begin(); edge_i2 != edge_i1->second.end(); ++edge_i2) 
			{
				if (edge_i2->second >= 1)
				{
					size_t 
						e1 = edge_i1->first,
						e2 = edge_i2->first
					;

					glVertex3d(
						world_scale * visualization_normalize(vertices.data[e1].x, X), 
						world_scale * visualization_normalize(vertices.data[e1].y, Y), 
						world_scale * visualization_normalize(vertices.data[e1].z, Z)
					);

					glVertex3d(
						world_scale * visualization_normalize(vertices.data[e2].x, X),
						world_scale * visualization_normalize(vertices.data[e2].y, Y), 
						world_scale * visualization_normalize(vertices.data[e2].z, Z)
					);
				}
			}
		}
		glEnd();*/
	}
	UNLOCK(opengl);
}

// display reconstructed polygons 
void visualization_polygons(const Polygons_3d & polygons, const double world_scale /*= 1*/)
{
	// set drawing style
	ATOMIC(opengl, opengl_drawing_style(UI_STYLE_POLYGON); );

	// go through all polygons 
	for ALL(polygons, i) 
	{
		const Polygon_3d * const polygon = polygons.data + i; 

		// draw only reconstruted polygons
		if (!query_is_polygon_reconstructed(*polygon, vertices)) continue;

		// check if we have texture for this polygon 
		bool texture_ready = false; 
		GLuint texture_id;
		double tx, ty, sx, sy;
		if (image_loader_nonempty_handle(polygon->image_loader_request))
		{
			image_loader_upload_to_opengl(polygon->image_loader_request);
			texture_ready = image_loader_opengl_upload_ready(polygon->image_loader_request, &texture_id, &tx, &ty, &sx, &sy);
		}
		
		// prepare texture mapping
		LOCK(opengl)
		{
			if (texture_ready) 
			{
				glBindTexture(GL_TEXTURE_2D, texture_id);
				glBegin(GL_POLYGON);
			}
			else
			{
				glBegin(GL_LINE_STRIP);
			}
		
			// go through all of it's vertices
			size_t first = 0;
			bool first_set = false;
			size_t v = 0;
			for ALL(polygon->vertices, j)
			{
				const size_t vertex_id = polygon->vertices.data[j].value;
				ASSERT_IS_SET(vertices, vertex_id); 
				if (!first_set)
				{
					first_set = true; 
					first = vertex_id;
				}

				if (texture_ready)
				{
					glTexCoord2d(tx + polygon->texture_coords[2 * v + 0] * (sx - tx), ty + polygon->texture_coords[2 * v + 1] * (sy - ty));
				}
				
				glVertex3d(
					world_scale * visualization_normalize(vertices.data[vertex_id].x, X), 
					world_scale * visualization_normalize(vertices.data[vertex_id].y, Y), 
					world_scale * visualization_normalize(vertices.data[vertex_id].z, Z)
				);

				v++;
			}

			if (first_set)
			{
				if (texture_ready)
				{
					glTexCoord2d(tx + polygon->texture_coords[0] * (sx - tx), ty + polygon->texture_coords[1] * (sy - ty));
				}

				glVertex3d(
					world_scale * visualization_normalize(vertices.data[first].x, X), 
					world_scale * visualization_normalize(vertices.data[first].y, Y), 
					world_scale * visualization_normalize(vertices.data[first].z, Z)
				);
			}

			glEnd();
			
			if (texture_ready) 
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		UNLOCK(opengl);
	}
}

// display 3d reconstruction contours (which are 2d shots' polygons)
void visualization_contours(const Shots & shots, const Vertices & vertices, const double world_scale /*= 1*/)
{
	opengl_drawing_style(UI_STYLE_CONTOUR);

	// go through all shots
	for ALL(shots, i)
	{
		const Shot * const shot = shots.data + i;

		// go through all polygons on this shot 
		for ALL(shot->contours, j)
		{
			const Contour * const contour = shot->contours.data + j; 

			// draw reconstructed polygon in 3d 
			bool drawing_state = false;    // will be true if previous vertex was drawn

			// check that every point on this polygon is reconstructed
			if (!query_is_contour_reconstructed(*contour, shot->points, vertices)) continue;

			// go through all 2d vertices of this polygon
			for ALL(contour->vertices, k) 
			{
				const size_t point_id = contour->vertices.data[k].value;
				ASSERT_IS_SET(shot->points, point_id); 
				const size_t vertex_id = shot->points.data[point_id].vertex; 
				ASSERT_IS_SET(vertices, vertex_id);

				// is this vertex reconstructed in 3d? 
				if (vertices.data[vertex_id].reconstructed)
				{
					if (!drawing_state) 
					{
						glBegin(GL_LINE_LOOP);
					}

					glVertex3d(
						world_scale * visualization_normalize(vertices.data[vertex_id].x, X), 
						world_scale * visualization_normalize(vertices.data[vertex_id].y, Y), 
						world_scale * visualization_normalize(vertices.data[vertex_id].z, Z)
					); 

					drawing_state = true; 
				}
				else if (drawing_state)
				{
					// not reconstructed
					glEnd(); 
					drawing_state = false;
				}
			}
			
			// close the polygon (if not already closed);
			if (drawing_state) 
			{
				glEnd();
			}
		}
	}
}

// load shot image into memory
/*void visualization_prepare_image(Shot & shot) 
// todo should probably receive shot_id
// todo add locking
// note this is replaced by image_loader subsystem
{
	// check if the image is loaded
	if (shot.image == NULL)
	{
		// deallocate all loaded images 
		// todo this is not nice, do this differently; also check effectiveness 
		// note on the other hand it's nice, that we deallocate smartly (take 
		// a look at recently visited shots, common points, ...) 
		for ALL(shots, i) 
		{
			Shot * const shot_iterator = shots.data + i; 

			if (shot_iterator->image) 
			{
				cvReleaseImage(&shot_iterator->image);
				shot_iterator->image = NULL;
				// shot_iterator->ready = false;
				if (shot_iterator->gl_texture_id_set) 
				{
					glDeleteTextures(1, &shot_iterator->gl_texture_id);
					shot_iterator->gl_texture_id_set = false; 
				}
			}
		}

		// if not, load it 
		// todo image caching 
		// todo resize while preserving aspect ratio
		IplImage * orig = cvLoadImage(shot.image_filename); // {}
		shot.width = orig->width;
		shot.height = orig->height;
		shot.image = cvCreateImage(cvSize(3000, 3000), orig->depth, orig->nChannels); // shot.image = opencv_downsize(orig, 2048);
		cvResize(orig, shot.image, CV_INTER_AREA);
		cvReleaseImage(&orig);
		shot.gl_texture_id_set = false;
		// shot.ready = true;
	}

	// if loaded successfully, register it as an opengl texture
	if (shot.image && (!shot.gl_texture_id_set || !glIsTexture(shot.gl_texture_id)))
	{
		// upload to opengl texture
		glGenTextures(1, &(shot.gl_texture_id));
		glBindTexture(GL_TEXTURE_2D, shot.gl_texture_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shot.image->width, shot.image->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, shot.image->imageData);
		shot.gl_texture_id_set = true;
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}*/

// place user camera (image plane mode)
void visualization_shot_user_camera(const double world_scale /*= 1*/)
{
	// current camera is visualization_state.current_camera
	// it is guaranteed that it exists and is finite 
	// note remove the later requirement by supplying opengl directly with the projection matrix

	glRotated(rad2deg(-shots.data[ui_state.current_shot].R_euler[X]), 1, 0, 0); 
	glRotated(rad2deg(-shots.data[ui_state.current_shot].R_euler[Y]), 0, 1, 0); 
	glRotated(rad2deg(-shots.data[ui_state.current_shot].R_euler[Z]), 0, 0, 1); 

	glTranslated(
		-world_scale * visualization_normalize(shots.data[ui_state.current_shot].T[X], X), 
		-world_scale * visualization_normalize(shots.data[ui_state.current_shot].T[Y], Y), 
		-world_scale * visualization_normalize(shots.data[ui_state.current_shot].T[Z], Z)
	);

	// save projection matrices 
	visualization_export_opengl_matrices();
}

// show shot
void visualization_shot_image(Shot & shot)
{
	LOCK(opengl) 
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	UNLOCK(opengl);

	// display the image as an OpenGL texture over the whole window
	if (
		image_loader_nonempty_handle(shot.image_loader_request) && 
		image_loader_request_ready(shot.image_loader_request)
	)
	{
		int loaded_width, loaded_height; 
		image_loader_get_original_dimensions(shot.image_loader_request, &loaded_width, &loaded_height);

		// if the meta data about this image aren't completely reliable, replace them with loaded values 
		if (shot.info_status < GEOMETRY_INFO_LOADED) 
		{
			shot.width = loaded_width; 
			shot.height = loaded_height;
			shot.info_status = GEOMETRY_INFO_LOADED;

			// it might be necessary to propagate this information to other shots
			for ALL(shots, i) 
			{
				Shot * const s = shots.data + i; 
				if (s->info_status == GEOMETRY_INFO_NOT_LOADED)
				{
					s->info_status = GEOMETRY_INFO_DEDUCED; 
					s->width = loaded_width; 
					s->height = loaded_height;
				}
			}
		}
		// otherwise simply verify stored values
		else if (loaded_width != shot.width || loaded_height != shot.height)
		{
			// note that this branch is very unlikely to be triggered - maybe we could even 
			// consider warning the user about this...
			debug("refreshed image dimensions do not match the ones we've remember from previous loading");
			shot.width = loaded_width; 
			shot.height = loaded_height;
		}

		// if the texture was uploaded
		GLuint full_texture, low_texture; 
		if (image_loader_opengl_upload_ready_dual(shot.image_loader_request, &full_texture, &low_texture))
		{
			LOCK(opengl)
			{
				if (!full_texture) 
				{
					visualization_state.continuous_loading_alpha = 1.0; 
					glColor4f(1, 1, 1, 1);
					glBindTexture(GL_TEXTURE_2D, low_texture);
					glBegin(GL_POLYGON);
						glTexCoord2f(0, 1); glVertex3f(-1, -1, -1);
						glTexCoord2f(1, 1); glVertex3f(1, -1, -1);
						glTexCoord2f(1, 0); glVertex3f(1, 1, -1);
						glTexCoord2f(0, 0); glVertex3f(-1, 1, -1);
					glEnd();
				}
				else if (visualization_state.continuous_loading_alpha >= 0.001) 
				{
					visualization_state.continuous_loading_alpha -= 0.1; 
					if (visualization_state.continuous_loading_alpha < 0) visualization_state.continuous_loading_alpha = 0;

					glColor4f(1, 1, 1, 1);
					glBindTexture(GL_TEXTURE_2D, full_texture);
					glBegin(GL_POLYGON); 
						glTexCoord2f(0, 1); glVertex3f(-1, -1, -1);
						glTexCoord2f(1, 1); glVertex3f(1, -1, -1);
						glTexCoord2f(1, 0); glVertex3f(1, 1, -1);
						glTexCoord2f(0, 0); glVertex3f(-1, 1, -1);
					glEnd();

					if (low_texture) 
					{
						glColor4d(1, 1, 1, visualization_state.continuous_loading_alpha);
						glBindTexture(GL_TEXTURE_2D, low_texture);
						glBegin(GL_POLYGON); 
							glTexCoord2f(0, 1); glVertex3f(-1, -1, -1);
							glTexCoord2f(1, 1); glVertex3f(1, -1, -1);
							glTexCoord2f(1, 0); glVertex3f(1, 1, -1);
							glTexCoord2f(0, 0); glVertex3f(-1, 1, -1);
						glEnd();
					}
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, full_texture);
					glColor4f(1, 1, 1, 1);
					glBegin(GL_POLYGON); 
						glTexCoord2f(0, 1); glVertex3f(-1, -1, -1);
						glTexCoord2f(1, 1); glVertex3f(1, -1, -1);
						glTexCoord2f(1, 0); glVertex3f(1, 1, -1);
						glTexCoord2f(0, 0); glVertex3f(-1, 1, -1);
					glEnd();
				}

				glBindTexture(GL_TEXTURE_2D, 0);
			}
			UNLOCK(opengl);
		}
	}

	ATOMIC(opengl, glDisable(GL_BLEND); );
}

// show dualview 
void visualization_show_dualview(size_t current_shot_id, size_t dual_shot_id, const double x, const double y) 
{
	const Shot * const current_shot = shots.data + current_shot_id;
	const Shot * const dual_shot = shots.data + dual_shot_id;

	// decoide, where to display it
	double tx, ty; 
	if (current_shot->width > current_shot->height) 
	{
		tx = 0;
		ty = -2; 
	}
	else
	{
		tx = 2; 
		ty = 0;
	}

	glTranslated(tx, ty, 0);
	visualization_shot_image(shots.data[dual_shot_id]);
	glTranslated(-tx, -ty, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// show the corresponsences
	for ALL(current_shot->points, i) 
	{
		const Point * const point = current_shot->points.data + i; 
		const size_t vertex_id = point->vertex; 
		size_t corresponding_point_id = 0;

		bool highlighted = point->selected;

		if (query_find_point_on_shot_by_vertex_id(dual_shot_id, vertex_id, corresponding_point_id))
		{
			const Point * const corresponding_point = dual_shot->points.data + corresponding_point_id;

			// calculate distance to cursor 
			const double distance = sqrt(distance_sq_2(
				x * current_shot->width, y * current_shot->height, 
				point->x * current_shot->width, point->y * current_shot->height
			)) / (double)current_shot->width;

			double weight = 2 - distance * 20; 
			if (weight < 0) weight = 0;
			else if (weight > 1) weight = 1;

			// calculate color (just some random-like mapping from points to colors should do)
			double c = sqrt(i + sqr_value(point->x) + sqr_value(point->y));
			double r = 1000 * c, g = 10000 * c, b = 100000 * c;
			r = r - (int)r;
			g = g - (int)g;
			b = b - (int)b;

			// draw correspondence
			glLineWidth((float)(1 + 1)); //weight));
			if (true) //!highlighted) 
			{
				/*if (weight > 0)*/ glColor4d(r, g, b, 0.6); // 0.2 + 0.8 * weight); // else glColor4d(1, 1, 1, 0.3);
			}
			else
			{
				glColor4d(1, 0, 1, 1);
			}
			glBegin(GL_LINES); 
				double x, y; 
				ui_convert_xy_from_shot_to_opengl(point->x, point->y, x, y);
				glVertex3d(x, y, -1);
				double x_prime, y_prime;
				ui_convert_xy_from_shot_to_opengl(corresponding_point->x, corresponding_point->y, x_prime, y_prime);
				glVertex3d(x_prime + tx, y_prime + ty, -1);
			glEnd();

			glPointSize((float)(4 + weight));
			if (!highlighted) 
			{
				if (weight > 0) glColor4d(r, g, b, 0.4 + 0.6 * weight); else glColor4d(1, 1, 1, 0.4);
			}
			else
			{
				glColor4d(1, 0, 1, 1);
			}
			glColor4d(1, 1, 1, 1);
			glBegin(GL_POINTS);
				glVertex3d(x, y, -1);
				glVertex3d(x_prime + tx, y_prime + ty, -1);
			glEnd();
		}
	}

	glDisable(GL_BLEND);
}

// draw rectangle with shadow
void visualization_rect_with_shadow(double left, double top, double right, double bottom, double texture_left, double texture_top, double texture_right, double texture_bottom, double alpha)
{
	const int shadow_precision = 8; 

	for (double size = UI_SHADOW_SIZE / UI_SHADOW_PRECISION; size <= UI_SHADOW_SIZE; size += UI_SHADOW_SIZE / UI_SHADOW_PRECISION)
	{
		for (double angle = 0; angle < 2 * CORE_PI; angle += CORE_PI / (shadow_precision / 2))
		{
			glColor4d(0.2, 0.2, 0.2, alpha * UI_SHADOW_ALPHA);
			glBegin(GL_POLYGON);
			  glVertex3d(left + UI_SHADOW_DISTANCE + size * cos(angle), top - UI_SHADOW_DISTANCE + size * sin(angle), -1);
			  glVertex3d(right + UI_SHADOW_DISTANCE + size * cos(angle), top - UI_SHADOW_DISTANCE + size * sin(angle), -1); 
			  glVertex3d(right + UI_SHADOW_DISTANCE + size * cos(angle), bottom - UI_SHADOW_DISTANCE + size * sin(angle), -1);
			  glVertex3d(left + UI_SHADOW_DISTANCE + size * cos(angle), bottom - UI_SHADOW_DISTANCE + size * sin(angle), -1);
			glEnd();
		}
	}

	glColor4d(1, 1, 1, alpha);
	glBegin(GL_POLYGON);
	  glTexCoord2d(texture_left, texture_top); glVertex3d(left, top, -1);
	  glTexCoord2d(texture_right, texture_top); glVertex3d(right, top, -1); 
	  glTexCoord2d(texture_right, texture_bottom); glVertex3d(right, bottom, -1); 
	  glTexCoord2d(texture_left, texture_bottom); glVertex3d(left, bottom, -1); 
	glEnd();
}

// show thumbnail of point in a different shot 
// note unused
/*void visualization_shot_thumb(int row, bool left_side, size_t shot_id, size_t point_id, double shot_x, double shot_y, double radius_x, double alpha = 1)
{
	// consistency check
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT(row >= 0, "illegal row number"); 
	ASSERT(inside_interval(shot_x, 0, 1), "x coordinate out of range"); 
	ASSERT(inside_interval(shot_y, 0, 1), "y coordinate out of range"); 
	ASSERT(radius_x > 0, "radius_x must be positive");

	double left, right, top, bottom, texture_left, texture_right, texture_top, texture_bottom;
	Shot * const shot = shots.data + shot_id;

	// calculate and constrain size of displayed neighbourhood
	radius_x = min(0.5, radius_x);
	double radius_y = radius_x * UI_THUMB_HEIGHT / UI_THUMB_WIDTH;
	if (radius_y > 0.5) 
	{
		radius_x *= 0.5 / radius_y;
		radius_y = 0.5; 
	}
	
	// calculate thumbnail coordinates
	if (left_side) 
	{
		left = -1 + UI_THUMB_BORDER;
		right = left + UI_THUMB_WIDTH; 
		top = 1 - UI_THUMB_BORDER - row * (UI_THUMB_HEIGHT + UI_THUMB_BORDER); 
		bottom = top - UI_THUMB_HEIGHT;
	}

	// calculate texture coordinates 
	texture_left = shot_x - radius_x; 
	texture_right = shot_x + radius_x; 
	texture_top = shot_y - radius_y; 
	texture_bottom = shot_y + radius_y; 

	double shift_x = 0, shift_y = 0; 

	// avoid displaying out of picture area
	if (texture_left < 0) 
	{
		shift_x = -texture_left; 
	}
	else if (texture_right > 1) 
	{
		shift_x = 1 - texture_right; 
	}

	if (texture_top < 0) 
	{
		shift_y = -texture_top; 
	}
	else if (texture_bottom > 1) 
	{
		shift_y = 1 - texture_bottom; 
	}

	texture_left += shift_x; 
	texture_right += shift_x; 
	texture_top += shift_y; 
	texture_bottom += shift_y;

	// load shot's texture into memory (if it hasn't been already loaded) 
	// visualization_prepare_image(*shot);

	// prepare texturing
	opengl_push_attribs();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, shot->gl_texture_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shot->image->width, shot->image->height, GL_BGR_EXT, GL_UNSIGNED_BYTE, shot->image->imageData); // note this is done more than once in our sorce code, maybe create inline function for this

	// display thumbnail 
	visualization_rect_with_shadow(left, top, right, bottom, texture_left, texture_top, texture_right, texture_bottom, alpha);

	// mark point location
	opengl_drawing_style(UI_STYLE_POINT);
	glBegin(GL_POINTS);
	glVertex3d((left + right) / 2.0, (top + bottom) / 2.0, -1); // todo breaks when displaying points near picture edge
	glEnd();

	// restore opengl settings
	glPopAttrib();
}*/

// show polygons in shot mode 
void visualization_shot_polygons(const size_t shot_id)
{
	ASSERT_IS_SET(shots, shot_id);
	const Shot * const shot = shots.data + shot_id;

	// go through all polygons 
	for ALL(polygons, i)
	{
		const Polygon_3d * const polygon = polygons.data + i; 

		opengl_drawing_style(UI_STYLE_POLYGON_JOINING_POINTS);

		// begin drawing
 		glBegin(GL_LINE_LOOP);

		// go through all vertices of this polygon
		for ALL(polygon->vertices, j)
		{
			const size_t vertex_id = polygon->vertices.data[j].value;
			size_t point_id; 

			// check if the vertex is marked on this shot
			if (query_find_point_on_shot_by_vertex_id(shot_id, vertex_id, point_id)) 
			{
				// * if it is marked on this shot, we'll display it *
				
				// get point coordinates 
				double x = geometry_get_point_x(shot_id, point_id);
				double y = geometry_get_point_y(shot_id, point_id); 

				// convert to opengl coordinates and draw
				ui_convert_xy_from_shot_to_opengl(x, y, x, y); 
				glVertex3d(x, y, -1);
			}
			/*else
			{
				// it's not visible, too bad... maybe we'll find a way to visualize this
			}*/
		}

		glEnd();
	}

	// display the processed polygon
	if (INDEX_IS_SET(ui_state.processed_polygon))
	{
		const Polygon_3d * const polygon = polygons.data + ui_state.processed_polygon;

		opengl_drawing_style(UI_STYLE_POLYGON_JOINING_POINTS);
		glColor3d(1, 0, 1);

		// begin drawing 
 		glBegin(GL_LINE_LOOP);

		// go through all vertices of this polygon
		for ALL(polygon->vertices, j)
		{
			const size_t vertex_id = polygon->vertices.data[j].value;
			size_t point_id; 

			// check if the vertex is marked on this shot
			if (query_find_point_on_shot_by_vertex_id(shot_id, vertex_id, point_id)) 
			{
				// * if it is marked on this shot, we'll display it *
				
				// get point coordinates 
				double x = geometry_get_point_x(shot_id, point_id);
				double y = geometry_get_point_y(shot_id, point_id); 

				// convert to opengl coordinates and draw
				ui_convert_xy_from_shot_to_opengl(x, y, x, y); 
				glVertex3d(x, y, -1);
			}
		}

		glEnd();
	}
}

// show all of the contours
void visualization_shot_contours(const size_t shot_id) 
{
	// drawing options 
	opengl_drawing_style(UI_STYLE_CONTOUR);

	// assertions 
	ASSERT(validate_shot(shot_id), "invalid shot"); 

	// go through all 2d polygons on this shot
	for ALL(shots.data[shot_id].contours, i)
	{
		// setup polygon
		glBegin(GL_LINE_LOOP); 

		// go through all vertices of this polygon
		for ALL(shots.data[shot_id].contours.data[i].vertices, j) 
		{
			// retrieve shot coordiates
			double x = geometry_get_point_x(shot_id, shots.data[shot_id].contours.data[i].vertices.data[j].value);
			double y = geometry_get_point_y(shot_id, shots.data[shot_id].contours.data[i].vertices.data[j].value); 
			
			// convert them to opengl coordinates and draw
			ui_convert_xy_from_shot_to_opengl(x, y, x, y); 
			glVertex3d(x, y, -1); 
		}

		glEnd();
	}
}

// place user camera (inspection mode)
void visualization_inspection_user_camera(double world_scale /*= 1*/)
{
	// are we in normal mode or ground inspection? 
	if (!visualization_state.ground_mode) 
	{
		// * normal mode *

		glRotated(rad2deg(visualization_state.R[X]), 1, 0, 0); 
		glRotated(rad2deg(visualization_state.R[Y]), 0, 1, 0); 
		// glRotated(rad2deg(visualization_state.R[Z]), 0, 0, 1); 

		glTranslated(
			-world_scale * visualization_normalize(visualization_state.T[X], X),
			-world_scale * visualization_normalize(visualization_state.T[Y], Y),
			-world_scale * visualization_normalize(visualization_state.T[Z], Z)
		);

		visualization_export_opengl_matrices();
	}
	else
	{
		// * ground mode * 

		// find camera center position 
		double center[3], up[3];

		// note could be precalculated
		linear_combination_43(
			visualization_state.ground_POI,
			visualization_state.ground_axis_x,
			visualization_state.ground_axis_y,
			visualization_state.ground_axis_z,
			1,
			visualization_state.ground_distance * cos(visualization_state.ground_alpha) * cos(visualization_state.ground_phi),
			visualization_state.ground_distance * sin(visualization_state.ground_alpha) * cos(visualization_state.ground_phi),
			visualization_state.ground_distance * sin(visualization_state.ground_phi),
			center
		);

		linear_combination_33(
			visualization_state.ground_axis_x,
			visualization_state.ground_axis_y,
			visualization_state.ground_axis_z,
			cos(visualization_state.ground_alpha) * cos(visualization_state.ground_phi + CORE_PI / 2.0),
			sin(visualization_state.ground_alpha) * cos(visualization_state.ground_phi + CORE_PI / 2.0),
			sin(visualization_state.ground_phi + CORE_PI / 2.0),
			up
		);

		// opengl transformation
		gluLookAt(
			world_scale * visualization_normalize(center[X], X),
			world_scale * visualization_normalize(center[Y], Y),
			world_scale * visualization_normalize(center[Z], Z),
			world_scale * visualization_normalize(visualization_state.ground_POI[X], X),
			world_scale * visualization_normalize(visualization_state.ground_POI[Y], Y),
			world_scale * visualization_normalize(visualization_state.ground_POI[Z], Z),
			up[X],
			up[Y], 
			up[Z]
		);
	}
}

// fit image to viewport
void visualization_fit_to_viewport()
{
	ASSERT_IS_SET(shots, ui_state.current_shot);
	ASSERT(validate_shot(ui_state.current_shot), "fitting invalid image to viewport");

	const Shot * const shot = shots.data + ui_state.current_shot; 
	UI_Shot_Meta * const meta = ui_check_shot_meta(ui_state.current_shot);

	meta->view_center_x = 0.5;
	meta->view_center_y = 0.5;
	meta->view_zoom = 0.5;
}

// move image into viewport - if the user scrolls zooms or scrolls out of image, we want to contrain her
void visualization_move_into_viewport(bool x_axis /*= true*/, bool y_axis /*= true*/) 
{
	ASSERT_IS_SET(shots, ui_state.current_shot);
	ASSERT(validate_shot(ui_state.current_shot), "moving invalid image to viewport");

	const Shot * const shot = shots.data + ui_state.current_shot; 
	UI_Shot_Meta * const meta = ui_check_shot_meta(ui_state.current_shot);
	const double view_zoom_x = visualization_get_zoom_x();
	double overlap;

	if (y_axis) 
	{
		if (meta->view_zoom >= 0.5) 
		{
			meta->view_center_y = 0.5;
		}
		else if ((overlap = meta->view_center_y - meta->view_zoom) < 0) 
		{
			meta->view_center_y -= overlap;
		}
		else if ((overlap = meta->view_center_y + meta->view_zoom - 1) > 0)
		{
			meta->view_center_y -= overlap;
		}
	}

	if (x_axis)
	{
		if (view_zoom_x >= 0.5) 
		{
			meta->view_center_x = 0.5; 
		}
		else if ((overlap = meta->view_center_x - view_zoom_x) < 0) 
		{
			meta->view_center_x -= overlap; 
		}
		else if ((overlap = meta->view_center_x + view_zoom_x - 1) > 0)
		{
			meta->view_center_x -= overlap;
		}
	}
}
