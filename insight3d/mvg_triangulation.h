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

#ifndef __MVG_TRIANGULATION
#define __MVG_TRIANGULATION

#include "core_debug.h"
#include "interface_opencv.h"
#include "mvg_thresholds.h"

// triangulates 3d position of a point given projection matrix of each camera 
// and the coordinates where the point is visible on each camera image
// 
// computation is done using direct linear transform and SVD, assuming that 
// the point is finite; this triangulation method is fairly suitable if we're 
// in affine space
// 
// arguments: 
// 
//   projection_matrices - array of n projection matrices
//   projected_points    - 2 x n matrix with i-th column representing the 
//                         coordinates on which the triangulated vertex 
//                         is visible on i-th camera
//   normalize_A         - flag denotes if the matrix A and vector b should be 
//                         normalized
//   min_points          - the minimum number of points required for triangulation 
//   samples             - pointer to indices of points that should be used, if NULL is 
//                         supplied, all of them are used
//   ns                  - number of samples
//
// returned value: 
// 
//   3-dimensional column vector holding inhomogeneous coordinates of 
//   the reconstructed point
//
CvMat * mvg_triangulation_SVD_affine(
	const CvMat * projection_matrices[], 
	const CvMat * projected_points, 
	bool normalize_A = false, 
	const unsigned int min_points = 2,
	int * samples = NULL, 
	int ns = -1
);

// triangulates 3d position of a point given projection matrix of each camera 
// and the coordinates where the point is visible on each camera image
// 
// computation is done using direct linear transform and SVD, reconstructed 
// vertices are allowed to be infinite
// 
// arguments: 
// 
//   projection_matrices - array of n projection matrices
//   projected_points    - 2 x n matrix with i-th column representing the 
//                         coordinates on which the triangulated vertex 
//                         is visible on i-th camera
//   normalize_A         - flag denotes if the matrix A should be normalized // todo
//   min_points          - the minimum number of points required for triangulation 
//   samples             - pointer to indices of points that should be used, if NULL is 
//                         supplied, all of them are used
//   ns                  - number of samples
//
// returned value: 
// 
//   4-dimensional column vector holding homogeneous coordinates of 
//   the reconstructed point
//
CvMat * mvg_triangulation_SVD(
	const CvMat * projection_matrices[], 
	const CvMat * projected_points, 
	bool normalize_A = false, 
	const unsigned int min_points = 2,
	int * samples = NULL, 
	int ns = -1
);

// robustly estimates the 3d position of a point given projection matrix of 
// each camera and the coordinates where the point is visible on each camera image
// 
// computation is done via RANSAC applied to mvg_triangulation_SVD_affine
// or mvg_triangulation_SVD
// 
// arguments: 
// 
//   projection_matrices - array of n projection matrices
//   projected_points    - 2 x n matrix with i-th column representing the 
//                         coordinates on which the triangulated vertex 
//                         is visible on i-th camera
//   affine              - true if affine triangulation should be used
//   normalize_A         - apply normalization of matrix A (and, if applicable, 
//                         of vector b)
//   min_inliers_to_reconstruct - minimum number of inliers to reliably 
//                                reconstruct the vertex (used in final 
//                                triangulation)
//   trials              - number of trials/iterations to do
//   threshold           - maximum value of reprojection error with which the 
//                         point is still considered to be inlier
//   inliers             - array of n bool values used to mark which points 
//                         are considered to be inliers
//
// returned value: 
// 
//   3- or 4-dimensional column vector holding (in)homogeneous coordinates of 
//   the reconstructed point; inhomogeneous coordinates are used iff 
//   the parameter affine was false
//
CvMat * mvg_triangulation_RANSAC(
	const CvMat * projection_matrices[],
	const CvMat * projected_points,
	const bool affine,
	const bool normalize_A = false, 
	const int min_inliers_to_triangulate = MVG_MIN_INLIERS_TO_TRIANGULATE,
	const int min_inliers_to_triangulate_weaker = MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER,
	const int trials = MVG_RANSAC_TRIANGULATION_TRIALS,
	const double threshold = MVG_MEASUREMENT_THRESHOLD, 
	bool * inliers = NULL
);

#endif
