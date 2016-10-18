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

#ifndef __TOOL_PLANE_EXTRACTION
#define __TOOL_PLANE_EXTRACTION

#include "interface_opencv.h"
#include "core_math_routines.h"
#include "geometry_structures.h"

// find major plane in point cloud using RANSAC
// todo time bounds
// todo normalization 
// todo refactor code (look at RANSAC implementation from OpenCV)
// todo check that we have at least 3 _reconstructed_ vertices
// todo optimize this (sqrt...)
double * tool_plane_extraction(Vertices & vertices, double threshold = 0.5, bool flatten_inliers = false, size_t group = 0);

// version using only a subset of vertices
double * tool_plane_extraction_subset(Vertices & vertices, size_t * ids, size_t count);

#endif
