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

#ifndef __MVG_RESECTION
#define __MVG_RESECTION

#include "core_debug.h"
#include "interface_opencv.h"
#include "mvg_decomposition.h"

// computes projection matrix P given 3d points X and their projections x = PX
//
// computation is done using direct linear transform and SVD
// 
// arguments: 
// 
//   vertices   - 3 x n or 4 x n, matrix with i-th column representing the i-th 
//                3d vertex in homogeneous or inhomogeneous coordinates respectively
//   projected  - 2 x n matrix with i-th column representing the 2d point 
//                on which the i-th 3d vertex is projected 
//   P, K, R, T - allocated containers for the resulting matrices 
//   sample     - array of ns indices values specifying which correspondences 
//                should be used 
//   ns         - number of samples
//
// returned value: 
// 
//   true  - if P has been successfully estimated
//   false - when it fails 
// 
// fails when: 
//
//   - vertices, projected, P are not allocated 
//   - one of K, R, T is allocated, but not all of them
//   - the problem is underdetermined 
//   - the size of matrices is inconsistent
//   - decomposition is required and the camera is infinite
//
bool mvg_resection_SVD(
	const CvMat * const vertices,
	const CvMat * const projected,
	CvMat * const P,
	CvMat * const K,
	CvMat * const R,
	CvMat * const T,
	bool normalize_A,
	int * samples = NULL,
	int ns = -1
);

// robustly computes projection matrix P given 3d points X and their projections x = PX
//
// computation is done using RANSAC applied to mvg_resection_SVD
// 
// arguments: 
// 
//   vertices  - 3 x n matrix with i-th column representing the coordinates 
//               of the i-th 3d vertex 
//   projected - 2 x n matrix with i-th column representing the 2d point 
//               on which the i-th 3d vertex is projected 
//   trials    - number of trials/iterations to do
//   threshold - maximum value of reprojection error with which the vertex is 
//               still considered inlier
//   inliers   - (optional) array of n booleans used to mark which points 
//               were considered to be inliers
//
// returned value: 
// 
//   3 x 4 projection matrix
// 
// fails when: 
//
//   - whenever mvg_resection_SVD fails 
//   - sufficiently large consensus set is not found 
//
bool mvg_resection_RANSAC(
	const CvMat * const vertices, 
	const CvMat * const projected, 
	CvMat * const P, 
	CvMat * const K, 
	CvMat * const R, 
	CvMat * const T, 
	bool normalize_A = false,
	const int trials = 500, 
	const double threshold = 4.0,
	bool * inliers = NULL
);

// clamps down some values in internal calibration matrix
bool mvg_restrict_calibration_matrix(CvMat * const K, const bool zero_skew, const bool square_pixels);

#endif
