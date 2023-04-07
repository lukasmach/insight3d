#include "ui_visualization_point.h"

const double VISUALIZATION_POINT_SIZE = 5;
const double VISUALIZATION_FOCUSED_POINT_SIZE = 7.5;
const unsigned int VISUALIZATION_DEFAULT = 0; 
const unsigned int VISUALIZATION_FOCUSED = 1; 
const unsigned int VISUALIZATION_SELECTED = 2; 
const unsigned int VISUALIZATION_PROCESSED = 4;
const unsigned int VISUALIZATION_OUTLIER = 8;
const unsigned int VISUALIZATION_AUTO = 16;

// draw point 
void visualization_point(double x, double y, unsigned int style)
{
	if (style & VISUALIZATION_AUTO)
	{
		const double size = style & VISUALIZATION_FOCUSED ? 10 : 8;
		const double size_x = visualization_calc_dx(size / 2), size_y = visualization_calc_dy(size / 2);

		if (style & VISUALIZATION_SELECTED) glColor4d(0.8, 0, 0.8, 1); 
		else if (style & VISUALIZATION_FOCUSED) glColor4d(1, 1, 1, 1);
		else glColor4d(1, 1, 1, 0.7);

		glPointSize((float)size);
		glBegin(GL_POINTS);
		glVertex3d(x, y, -1);
		glEnd();

		if (style & VISUALIZATION_FOCUSED) glColor3d(0, 0, 0); else glColor4d(0, 0, 0, 0.7);
		glPointSize((float)(size - 3));
		glBegin(GL_POINTS);
		glVertex3d(x, y, -1);
		glEnd();
	}
	else
	{
		double size = style & VISUALIZATION_FOCUSED ? VISUALIZATION_FOCUSED_POINT_SIZE : VISUALIZATION_POINT_SIZE;
		size += 1.5;

		double size_x = visualization_calc_dx(size), size_y = visualization_calc_dy(size);
		glLineWidth(4);
		if (style & VISUALIZATION_SELECTED) /*glColor3d(226 / 255.0, 122 / 255.0, 1.0);*/ glColor3d(0.8, 0, 0.8); else glColor3d(1, 1, 1);
		glBegin(GL_LINES);
		glVertex3d(x, y, -1);
		glVertex3d(x + size_x, y, -1);
		glVertex3d(x, y, -1);
		glVertex3d(x - size_x, y, -1);
		glVertex3d(x, y, -1);
		glVertex3d(x, y + size_y, -1);
		glVertex3d(x, y, -1);
		glVertex3d(x, y - size_y, -1);
		glEnd();

		size -= 1.5;
		size_x = visualization_calc_dx(size), size_y = visualization_calc_dy(size);
		glLineWidth(1.0);
		glColor3d(0.0, 0.0, 0.0);
		glBegin(GL_LINES);
		glVertex3d(x, y, -1); 
		glVertex3d(x + size_x, y, -1);
		glVertex3d(x, y, -1); 
		glVertex3d(x - size_x, y, -1);
		glVertex3d(x, y, -1); 
		glVertex3d(x, y + size_y, -1);
		glVertex3d(x, y, -1); 
		glVertex3d(x, y - size_y, -1);
		glEnd();
	}
}

// draw line segment visualizing reprojection error in image 
void visualization_reprojection(const double x, const double y, const double rx, const double ry, const bool outlier /*= false*/)
{
	if (!outlier) glColor4d(0, 0.8, 0, 0.6); else glColor4d(0.7, 0, 0, 0.6);
	glLineWidth(1.0);
	glBegin(GL_LINES);
	glVertex3d(x, y, -1);
	glVertex3d(rx, ry, -1);
	glEnd();

	if (!outlier) glColor4d(0, 0.2, 0, 1); else glColor4d(0, 0.2, 0, 1);
	glPointSize(5);
	glBegin(GL_POINTS);
	glVertex3d(rx, ry, -1); 
	glEnd();

	if (!outlier) glColor4d(0, 1, 0, 1); else glColor4d(1, 0, 0, 1);
	glPointSize(3);
	glBegin(GL_POINTS); 
	glVertex3d(rx, ry, -1); 
	glEnd();
}

// show reprojection of point's vertex
void visualization_point_reprojection(const size_t shot_id, const size_t point_id)
{
	// calculate the reprojection 
	double reprojection[2]; 
	const size_t vertex_id = shots.data[shot_id].points.data[point_id].vertex;
	opencv_vertex_projection_visualization(
		shots.data[shot_id].projection, 
		vertices.data[vertex_id].x,
		vertices.data[vertex_id].y,
		vertices.data[vertex_id].z,
		reprojection
	);

	// convert coordinates to percentage
	reprojection[0] /= shots.data[shot_id].width; 
	reprojection[1] /= shots.data[shot_id].height;

	// convert to opengl coordinates 
	double x, y;
	ui_convert_xy_from_shot_to_opengl(geometry_get_point_x(shot_id, point_id), geometry_get_point_y(shot_id, point_id), x, y);
	ui_convert_xy_from_shot_to_opengl(reprojection[0], reprojection[1], reprojection[0], reprojection[1]);

	// draw it 
	visualization_reprojection(x, y, reprojection[0], reprojection[1]);
}

// show reprojection of point's vertex in chosen calibration
void visualization_point_reprojection_calibration(
	const size_t shot_id, const size_t point_id, const size_t calibration_id, 
	const size_t P_id, const size_t X_id, const bool outlier
)
{
	ASSERT_IS_SET(calibrations, calibration_id);
	Calibration * const calibration = calibrations.data + calibration_id;

	// calculate the reprojection 
	double reprojection[2]; 
	opencv_vertex_projection_visualization(
		calibration->Ps.data[P_id].P,
		calibration->Xs.data[X_id].X,
		reprojection
	);

	// convert coordinates to percentage
	reprojection[0] /= shots.data[shot_id].width; 
	reprojection[1] /= shots.data[shot_id].height;

	// convert to opengl coordinates 
	double x, y;
	ui_convert_xy_from_shot_to_opengl(geometry_get_point_x(shot_id, point_id), geometry_get_point_y(shot_id, point_id), x, y);
	ui_convert_xy_from_shot_to_opengl(reprojection[0], reprojection[1], reprojection[0], reprojection[1]);

	// draw it 
	visualization_reprojection(x, y, reprojection[0], reprojection[1], outlier);
}

// show 2d points in shot mode
void visualization_shot_points()
{
	// assertions
	ASSERT(INDEX_IS_SET(ui_state.current_shot), "rendering points on current shot and no shot is set as current");
	ASSERT(validate_shot(ui_state.current_shot), "invalid shot set as current shot");

	const size_t shot_id = ui_state.current_shot;
	const Shot * shot = shots.data + ui_state.current_shot;

	// check if there is current calibration selected 
	Calibration * const calibration = calibrations.data + ui_state.current_calibration;
	size_t P_id;
	bool use_calibration = false, P_found = false;

	if (INDEX_IS_SET(ui_state.current_calibration))
	{
		// if partial calibration was picked, show it 
		ASSERT_IS_SET(calibrations, ui_state.current_calibration);
		Calibration * const calibration = calibrations.data + ui_state.current_calibration;
		use_calibration = true;

		// find id of this shot 
		LAMBDA_FIND(calibration->Ps, P_id, P_found, calibration->Ps.data[P_id].shot_id == shot_id);
	}

	// go through all unselected and unfocused 2d points on this shot
	for ALL(shot->points, i)
	{
		// skipping automatic points if the user wishes so 
		if (option_hide_automatic && vertices.data[shot->points.data[i].vertex].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

		// fetch point data and skip selected and focused
		const Point * point = shot->points.data + i;
		if (INDEX_IS_SET(ui_state.focused_point) && ui_state.focused_point == i || point->selected) continue;

		// check if this is outlier 
		const bool outlier = P_found && IS_SET(calibration->Ps.data[P_id].points_meta, i) && calibration->Ps.data[P_id].points_meta.data[i].inlier == 0;
		const bool automatic = vertices.data[point->vertex].vertex_type == GEOMETRY_VERTEX_AUTO;

		// draw it 
		double x, y;
		ui_convert_xy_from_shot_to_opengl(geometry_get_point_x(shot_id, i), geometry_get_point_y(shot_id, i), x, y);
		visualization_point(x, y, VISUALIZATION_DEFAULT | (outlier ? VISUALIZATION_OUTLIER : 0) | (automatic ? VISUALIZATION_AUTO : 0));
	}

	// now display the selected ones 
	for ALL(shot->points, i)
	{
		// fetch point data and skip unselected or focused
		const Point * point = shot->points.data + i;
		if (!point->selected || INDEX_IS_SET(ui_state.focused_point) && ui_state.focused_point == i) continue;

		// skipping automatic points if the user wishes so 
		if (option_hide_automatic && vertices.data[shot->points.data[i].vertex].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

		// check if this is outlier 
		const bool outlier = P_found && IS_SET(calibration->Ps.data[P_id].points_meta, i) && calibration->Ps.data[P_id].points_meta.data[i].inlier == 0;
		const bool automatic = vertices.data[point->vertex].vertex_type == GEOMETRY_VERTEX_AUTO;

		// draw it
		double x, y; 
		ui_convert_xy_from_shot_to_opengl(geometry_get_point_x(shot_id, i), geometry_get_point_y(shot_id, i), x, y);
		visualization_point(x, y, VISUALIZATION_SELECTED | (outlier ? VISUALIZATION_OUTLIER : 0) | (automatic ? VISUALIZATION_AUTO : 0));
	}

	// and finally the focused point is displayed
	if (INDEX_IS_SET(ui_state.focused_point))
	{
		ASSERT_IS_SET(shot->points, ui_state.focused_point);
		const Point * const focused_point = shot->points.data + ui_state.focused_point;

		// draw focused unselected point
		double x, y;
		ui_convert_xy_from_shot_to_opengl(
			geometry_get_point_x(shot_id, ui_state.focused_point), 
			geometry_get_point_y(shot_id, ui_state.focused_point), 
			x, y
		);

		// check if this is outlier 
		const bool outlier = P_found && IS_SET(calibration->Ps.data[P_id].points_meta, ui_state.focused_point) 
		                     && calibration->Ps.data[P_id].points_meta.data[ui_state.focused_point].inlier == 0;
		const bool automatic = vertices.data[focused_point->vertex].vertex_type == GEOMETRY_VERTEX_AUTO;

		int style = VISUALIZATION_FOCUSED;
		if (focused_point->selected)
		{
			style |= VISUALIZATION_SELECTED;
		}

		visualization_point(x, y, style | (outlier ? VISUALIZATION_OUTLIER : 0) | (automatic ? VISUALIZATION_AUTO : 0));
	}

	// show reprojection errors of individual points
	if (use_calibration && P_found)
	{
		// go through all points
		for ALL(shot->points, i) 
		{
			// skipping automatic points if the user wishes so 
			if (option_hide_automatic && vertices.data[shot->points.data[i].vertex].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

			const Point * const point = shot->points.data + i;
			const size_t vertex_id = point->vertex;

			bool found; 
			size_t X_id = 0; 
			LAMBDA_FIND(calibration->Xs, X_id, found, calibration->Xs.data[X_id].vertex_id == vertex_id);

			// check if this is outlier 
			const bool outlier = P_found && IS_SET(calibration->Ps.data[P_id].points_meta, i) && calibration->Ps.data[P_id].points_meta.data[i].inlier == 0;

			if (found) 
			{
				// draw it
				visualization_point_reprojection_calibration(
					shot_id, i, ui_state.current_calibration, 
					P_id, X_id, outlier
				);
			}
		}
	}
	else if (shot->calibrated)
	{
		ASSERT(shot->projection, "calibrated shot doesn't have projection matrix");

		for ALL(shot->points, i)
		{
			// skipping automatic points if the user wishes so 
			if (option_hide_automatic && vertices.data[shot->points.data[i].vertex].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

			// fetch point data and skip selected and focused
			const Point * point = shot->points.data + i;

			// check if it's reconstructed
			if (!vertices.data[point->vertex].reconstructed) continue;

			// draw it
			visualization_point_reprojection(shot_id, i);
		}
	}
}
