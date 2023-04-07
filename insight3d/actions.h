#ifndef __ACTIONS
#define __ACTIONS

#include "geometry_structures.h"
#include "geometry_publish.h"
#include "geometry_routines.h"
#include "mvg_thresholds.h"
#include "mvg_triangulation.h"
#include "mvg_resection.h"

// triangulates vertices given current projection matrices and vertex projections
bool action_triangulate_vertices(bool * shots_to_use = NULL, const int min_inliers = MVG_MIN_INLIERS_TO_TRIANGULATE, const int min_inliers_weaker = MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER, const bool only_manual = false, const double measurement_threshold = MVG_MEASUREMENT_THRESHOLD);

// compute projection matrix of current camera from 3d to 2d correspondences
bool action_camera_resection(size_t shot_id, const bool enforce_square_pixels, const bool enforce_zero_skew);

#endif
