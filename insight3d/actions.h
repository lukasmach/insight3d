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
