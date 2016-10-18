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

#include "mvg_triangulation.h"

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
	bool normalize_A /*= false*/, 
	const unsigned int min_points /*= 2*/,
	int * samples /*= NULL*/, 
	int ns /*= -1*/
)
{
	// if we're not picking subset of correspondences, then the number of 
	// points is the same as the number of columns of the meassurement matrices;
	// otherwise we use supplied value 
	const int n = samples ? ns : projected_points->cols;

	// we can't reconstruct anything unless we see it from at least two shots 
	if (n < min_points) return NULL;

	// we want to find x which minimizes |Ax - b|
	// we'll build the matrix A and vector b and then 
	// use SVD to find unknown vector x 

	// allocate data structures
	CvMat * x = opencv_create_matrix(3, 1);        // allocate structure to store the result
	CvMat * A = opencv_create_matrix(2 * n, 3);    // allocate space for matrix A
	CvMat * b = opencv_create_matrix(2 * n, 1);    // allocate space for column vector b

	// fill the matrix A and column vector b
	double norm;
	for (int j = 0; j < n; j++)
	{
		int i;
		if (samples)
		{
			i = samples[j]; 
		}
		else
		{
			i = j;
		}

		// set the two new values of column vector b, since we assume $$X_4 = 1$$
		OPENCV_ELEM(b, 2 * j, 0) = -1.0 * OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 3) + OPENCV_ELEM(projection_matrices[i], 0, 3);
		OPENCV_ELEM(b, 2 * j + 1, 0) = -1.0 * OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 3) + OPENCV_ELEM(projection_matrices[i], 1, 3);

		// enter values into two rows to matrix A
		OPENCV_ELEM(A, 2 * j, 0) = OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 0) - OPENCV_ELEM(projection_matrices[i], 0, 0);
		OPENCV_ELEM(A, 2 * j, 1) = OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 1) - OPENCV_ELEM(projection_matrices[i], 0, 1);
		OPENCV_ELEM(A, 2 * j, 2) = OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 2) - OPENCV_ELEM(projection_matrices[i], 0, 2);

		// normalize the row
		if (normalize_A)
		{
			for (int k = norm = 0; k < 3; k++) { norm += OPENCV_ELEM(A, 2 * j, k) * OPENCV_ELEM(A, 2 * j, k); }
			norm += OPENCV_ELEM(b, 2 * j, 0) * OPENCV_ELEM(b, 2 * j, 0);
			norm = 1 / sqrt(norm);
			for (int k = 0; k < 3; k++) { OPENCV_ELEM(A, 2 * j, k) *= norm; }
			OPENCV_ELEM(b, 2 * j, 0) *= norm;
		}

		OPENCV_ELEM(A, 2 * j + 1, 0) = OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 0) - OPENCV_ELEM(projection_matrices[i], 1, 0);
		OPENCV_ELEM(A, 2 * j + 1, 1) = OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 1) - OPENCV_ELEM(projection_matrices[i], 1, 1);
		OPENCV_ELEM(A, 2 * j + 1, 2) = OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 2) - OPENCV_ELEM(projection_matrices[i], 1, 2);

		// again normalize
		if (normalize_A)
		{
			for (int k = norm = 0; k < 3; k++) { norm += OPENCV_ELEM(A, 2 * j + 1, k) * OPENCV_ELEM(A, 2 * j + 1, k); }
			norm += OPENCV_ELEM(b, 2 * j + 1, 0) * OPENCV_ELEM(b, 2 * j + 1, 0);
			// printf("%f %f\n", OPENCV_ELEM(b, 2 * j, 0) * OPENCV_ELEM(b, 2 * j, 0), OPENCV_ELEM(b, 2 * j + 1, 0) * OPENCV_ELEM(b, 2 * j + 1, 0));
			norm = 1 / sqrt(norm);
			for (int k = 0; k < 3; k++) { OPENCV_ELEM(A, 2 * j + 1, k) *= norm; }
			OPENCV_ELEM(b, 2 * j + 1, 0) *= norm;
		}
	}

	// use SVD to find our x
	cvSolve(A, b, x, CV_SVD);

	// release memory
	cvReleaseMat(&A); 
	cvReleaseMat(&b); 

	return x; 
}

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
	bool normalize_A /*= false*/, 
	const unsigned int min_points /*= 2*/,
	int * samples /*= NULL*/, 
	int ns /*= -1*/
)
{
	// if we're not picking subset of correspondences, then the number of 
	// points is the same as the number of columns of the meassurement matrices;
	// otherwise we use supplied value 
	const int n = samples ? ns : projected_points->cols;

	// we can't reconstruct anything unless we see it from at least two shots 
	if (n < min_points) return NULL;

	// we want to find x which minimizes |Ax - b|
	// we'll build the matrix A and vector b and then 
	// use SVD to find unknown vector x 

	// allocate data structures
	CvMat * A = opencv_create_matrix(2 * n, 4);    // allocate space for matrix A

	// fill the matrix A and column vector b
	double norm;
	for (int j = 0; j < n; j++)
	{
		int i;
		if (samples)
		{
			i = samples[j]; 
		}
		else
		{
			i = j;
		}

		// enter values into two rows to matrix A
		OPENCV_ELEM(A, 2 * j, 0) = (OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 0) - OPENCV_ELEM(projection_matrices[i], 0, 0));
		OPENCV_ELEM(A, 2 * j, 1) = (OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 1) - OPENCV_ELEM(projection_matrices[i], 0, 1));
		OPENCV_ELEM(A, 2 * j, 2) = (OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 2) - OPENCV_ELEM(projection_matrices[i], 0, 2));
		OPENCV_ELEM(A, 2 * j, 3) = (OPENCV_ELEM(projected_points, 0, i) * OPENCV_ELEM(projection_matrices[i], 2, 3) - OPENCV_ELEM(projection_matrices[i], 0, 3));

		// normalize the row
		if (normalize_A)
		{
			for (int k = norm = 0; k < 4; k++) { norm += OPENCV_ELEM(A, 2 * j, k) * OPENCV_ELEM(A, 2 * j, k); }
			norm = 1 / sqrt(norm);
			for (int k = 0; k < 4; k++) { OPENCV_ELEM(A, 2 * j, k) *= norm; }
		}

		OPENCV_ELEM(A, 2 * j + 1, 0) = (OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 0) - OPENCV_ELEM(projection_matrices[i], 1, 0));
		OPENCV_ELEM(A, 2 * j + 1, 1) = (OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 1) - OPENCV_ELEM(projection_matrices[i], 1, 1));
		OPENCV_ELEM(A, 2 * j + 1, 2) = (OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 2) - OPENCV_ELEM(projection_matrices[i], 1, 2));
		OPENCV_ELEM(A, 2 * j + 1, 3) = (OPENCV_ELEM(projected_points, 1, i) * OPENCV_ELEM(projection_matrices[i], 2, 3) - OPENCV_ELEM(projection_matrices[i], 1, 3));

		// again normalize
		if (normalize_A)
		{
			for (int k = norm = 0; k < 4; k++) { norm += OPENCV_ELEM(A, 2 * j + 1, k) * OPENCV_ELEM(A, 2 * j + 1, k); }
			norm = 1 / sqrt(norm);
			for (int k = 0; k < 4; k++) { OPENCV_ELEM(A, 2 * j + 1, k) *= norm; }
		}
	}

	// use SVD to find our x
	// todo check this
	CvMat * X = opencv_right_null_vector(A);

	// release memory
	cvReleaseMat(&A); 

	return X;
}

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

CvMat * mvg_triangulation_RANSAC(
	const CvMat * projection_matrices[],
	const CvMat * projected_points,
	const bool affine,
	const bool normalize_A /*= false*/, 
	const int min_inliers_to_triangulate /*= MVG_MIN_INLIERS_TO_TRIANGULATE*/,
	const int min_inliers_to_triangulate_weaker /*= MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER*/,
	const int trials /*= MVG_RANSAC_TRIANGULATION_TRIALS*/,
	const double threshold /*= MVG_MEASUREMENT_THRESHOLD*/, 
	bool * inliers /*= NULL*/
)
{
	// printf("Triangulating: ");

	// transform input 
	const int n = projected_points->cols;
	const double threshold_sq = threshold * threshold;

	// fail immediately if the solution is underdetermined 
	if (n < 2)
	{
		return false;
	}

	// allocate
	bool * best_status = ALLOC(bool, n), * status = ALLOC(bool, n);
	int best_inliers_count = -1;

	// do this many times 
	for (int i = 0; i < trials; i++) 
	{
		// pick randomly 2 points
		memset(status, 0, sizeof(bool) * n);
		int samples[2];
		for (int count = 0; count < 2;)
		{
			const int pick = rand() % n; 
			if (!status[pick])
			{
				status[pick] = true; 
				samples[count] = pick;
				count++;
			}
		}

		// calculate using mvg_triangulation_SVD_affine
		CvMat * X = 
			affine ? mvg_triangulation_SVD_affine(projection_matrices, projected_points, normalize_A, 2, samples, 2)
			       : mvg_triangulation_SVD(projection_matrices, projected_points, normalize_A, 2, samples, 2);
		if (!X)
		{
			continue;
		}

		// count and mark the inliers
		int inliers_count = 0;
		for (int j = 0; j < n; j++)
		{
			double reprojection[2];
			// note that here we're using "dirty" projection method which is suitable only for visualization; on the upside, it shouldn't matter because
			// if a point is projected on pi_infinity, there's something wrong with it anyway
			if (affine) 
			{
				opencv_vertex_projection_visualization(projection_matrices[j], OPENCV_ELEM(X, 0, 0), OPENCV_ELEM(X, 1, 0), OPENCV_ELEM(X, 2, 0), reprojection);
			}
			else
			{
				opencv_vertex_projection_visualization(projection_matrices[j], OPENCV_ELEM(X, 0, 0), OPENCV_ELEM(X, 1, 0), OPENCV_ELEM(X, 2, 0), OPENCV_ELEM(X, 3, 0), reprojection);
			}

			const double
				dx = reprojection[0] - OPENCV_ELEM(projected_points, 0, j), 
				dy = reprojection[1] - OPENCV_ELEM(projected_points, 1, j);
			
			if (dx * dx + dy * dy <= threshold_sq)
			{
				inliers_count++; 
				status[j] = true; 
			}
			else
			{
				status[j] = false;
			}
		}

		cvReleaseMat(&X);

		// check for the best sample 
		if (inliers_count > best_inliers_count)
		{
			bool * temp = best_status; 
			best_status = status; 
			status = temp;
			best_inliers_count = inliers_count; 
		}

		// debug 
		// printf("{%d/%d} ", inliers_count, n);
	}

	FREE(status);
	if (best_inliers_count < 2)
	{
		FREE(best_status);
		return false;
	}

	// calculate camera calibration using only inliers
	int * samples = ALLOC(int, best_inliers_count);
	int j = 0;
	int outliers = 0, min = min_inliers_to_triangulate; 
	for (int i = 0; i < n; i++)
	{
		if (best_status[i]) 
		{
			samples[j++] = i;
		}
		else
		{
			outliers++;
		}
	}

	if (outliers == 0) min = min_inliers_to_triangulate_weaker;
	CvMat * X = affine ? mvg_triangulation_SVD_affine(projection_matrices, projected_points, true, min, samples, best_inliers_count)
	                   : mvg_triangulation_SVD(projection_matrices, projected_points, true, min, samples, best_inliers_count);
	if (!X)
	{
		// printf("failed to estimate\n"); 
		if (inliers) 
		{
			memset(inliers, 0, sizeof(bool) * n);
		}
	}
	else if (inliers)
	{
		memcpy(inliers, best_status, sizeof(bool) * n);
	}

	// release resources
	FREE(samples);
	FREE(best_status);

	// printf("\n");
	return X;
}
