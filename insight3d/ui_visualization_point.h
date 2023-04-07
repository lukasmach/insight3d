#ifndef __UI_VISUALIZATION_POINT
#define __UI_VISUALIZATION_POINT

#include "interface_opengl.h"
#include "ui_visualization_helpers.h"

extern const double VISUALIZATION_POINT_SIZE;
extern const double VISUALIZATION_FOCUSED_POINT_SIZE;
extern const unsigned int VISUALIZATION_DEFAULT; 
extern const unsigned int VISUALIZATION_FOCUSED; 
extern const unsigned int VISUALIZATION_SELECTED; 
extern const unsigned int VISUALIZATION_PROCESSED;
extern const unsigned int VISUALIZATION_OUTLIER;
extern const unsigned int VISUALIZATION_AUTO;

// draw point 
void visualization_point(double x, double y, unsigned int style);

// draw line segment visualizing reprojection error in image 
void visualization_reprojection(const double x, const double y, const double rx, const double ry, const bool outlier = false);

// show reprojection of point's vertex 
void visualization_point_reprojection(const size_t shot_id, const size_t point_id);

// show reprojection of point's vertex in chosen calibration
void visualization_point_reprojection_calibration(
	const size_t shot_id, const size_t point_id, const size_t calibration_id, 
	const size_t P_id, const size_t X_id, const bool outlier
);

// show 2d points in shot mode
void visualization_shot_points();

#endif
