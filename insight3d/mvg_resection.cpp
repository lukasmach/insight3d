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

#include "mvg_resection.h"

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
	int * samples /*= NULL*/,
	int ns /*= -1*/
)
{
	// if we're not picking subset of correspondences, then the number of 
	// vertices is the same as the number of columns of the two input matrices;
	// otherwise we use supplied value 
	const int n = samples ? ns : projected->cols;
	const bool homogeneous = vertices->rows == 4;

	// we must have at least 5.5 correspondences to constrain the solution 
	if (!vertices || !projected || !P) return false; 
	if (n < 6) return false;
	if (vertices->cols != projected->cols) return false;

	// as always when using DLT, we'll fill the matrix A and then 
	// find p such that |Ap| is minimized; we then proceed to build 
	// the projection matrix from the values of the vector p

	// * initialize computation *

	// allocate memory 
	CvMat * A = cvCreateMat(2 * n, 12, CV_64F);

	// construct A
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

		OPENCV_ELEM(A, 2 * j, 0)  = 0; 
		OPENCV_ELEM(A, 2 * j, 1)  = 0; 
		OPENCV_ELEM(A, 2 * j, 2)  = 0; 
		OPENCV_ELEM(A, 2 * j, 3)  = 0;

		OPENCV_ELEM(A, 2 * j, 4)  = -1 * OPENCV_ELEM(vertices, 0, i); 
		OPENCV_ELEM(A, 2 * j, 5)  = -1 * OPENCV_ELEM(vertices, 1, i); 
		OPENCV_ELEM(A, 2 * j, 6)  = -1 * OPENCV_ELEM(vertices, 2, i); 
		OPENCV_ELEM(A, 2 * j, 7)  = -1 * (homogeneous ? OPENCV_ELEM(vertices, 3, i) : 1);

		OPENCV_ELEM(A, 2 * j, 8)  = OPENCV_ELEM(projected, 1, i) * OPENCV_ELEM(vertices, 0, i); 
		OPENCV_ELEM(A, 2 * j, 9)  = OPENCV_ELEM(projected, 1, i) * OPENCV_ELEM(vertices, 1, i); 
		OPENCV_ELEM(A, 2 * j, 10) = OPENCV_ELEM(projected, 1, i) * OPENCV_ELEM(vertices, 2, i); 
		OPENCV_ELEM(A, 2 * j, 11) = OPENCV_ELEM(projected, 1, i) * (homogeneous ? OPENCV_ELEM(vertices, 3, i) : 1);

		// normalize this row
		if (normalize_A)
		{
			double norm = 0; 
			for (int k = 0; k < 12; k++) { norm += OPENCV_ELEM(A, 2 * j, k) * OPENCV_ELEM(A, 2 * j, k); }
			norm = 1 / norm;
			for (int k = 0; k < 12; k++) { OPENCV_ELEM(A, 2 * j, k) *= norm; }
		}

		OPENCV_ELEM(A, 2 * j + 1,  0) = OPENCV_ELEM(vertices, 0, i); 
		OPENCV_ELEM(A, 2 * j + 1,  1) = OPENCV_ELEM(vertices, 1, i); 
		OPENCV_ELEM(A, 2 * j + 1,  2) = OPENCV_ELEM(vertices, 2, i); 
		OPENCV_ELEM(A, 2 * j + 1,  3) = (homogeneous ? OPENCV_ELEM(vertices, 3, i) : 1);

		OPENCV_ELEM(A, 2 * j + 1,  4) = 0;
		OPENCV_ELEM(A, 2 * j + 1,  5) = 0;
		OPENCV_ELEM(A, 2 * j + 1,  6) = 0;
		OPENCV_ELEM(A, 2 * j + 1,  7) = 0;

		OPENCV_ELEM(A, 2 * j + 1,  8) = -1 * OPENCV_ELEM(projected, 0, i) * OPENCV_ELEM(vertices, 0, i); 
		OPENCV_ELEM(A, 2 * j + 1,  9) = -1 * OPENCV_ELEM(projected, 0, i) * OPENCV_ELEM(vertices, 1, i); 
		OPENCV_ELEM(A, 2 * j + 1, 10) = -1 * OPENCV_ELEM(projected, 0, i) * OPENCV_ELEM(vertices, 2, i); 
		OPENCV_ELEM(A, 2 * j + 1, 11) = -1 * OPENCV_ELEM(projected, 0, i) * (homogeneous ? OPENCV_ELEM(vertices, 3, i) : 1);

		// again normalize
		if (normalize_A)
		{
			double norm = 0; 
			for (int k = 0; k < 12; k++) { norm += OPENCV_ELEM(A, 2 * j + 1, k) * OPENCV_ELEM(A, 2 * j + 1, k); }
			norm = 1 / norm;
			for (int k = 0; k < 12; k++) { OPENCV_ELEM(A, 2 * j + 1, k) *= norm; }
		}
	}

	// * calculate projection matrix *

	// find x = argmin(|Ax|) subject to |x| == 1
	CvMat * W = cvCreateMat(12, 1, CV_64F), * V_transposed = cvCreateMat(12, 12, CV_64F);
	cvSVD(A, W, NULL, V_transposed, CV_SVD_MODIFY_A | CV_SVD_V_T); // todo check W for numerical stability

	// build projection matrix from the last row of V_transposed
	for (int i = 0; i < 12; i++) 
	{
		OPENCV_ELEM(P, i / 4, i % 4) = OPENCV_ELEM(V_transposed, 11, i);
	}

	// release temporary variables
	cvReleaseMat(&A);
	cvReleaseMat(&W);
	cvReleaseMat(&V_transposed);

	// * optionally decompose projection matrix *
	if (R || T || K)
	{
		if (!R || !T || !K) return false;

		// call decomposition routine 
		bool finite = mvg_finite_projection_matrix_decomposition(P, K, R, T);
		/*printf("\n----\n\n");
		opencv_debug("P", P);
		opencv_debug("R", R);
		opencv_debug("K", K);
		opencv_debug("T", T);
		printf("Det(R) = %f\n", cvDet(R));

		mvg_assemble_projection_matrix(K, R, T, P); 
		opencv_debug("Again P", P); 

		// call it again just for fun 
		for (int i = 0; i < 3; i++) 
		{
			for (int j = 0; j < 4; j++) 
			{
				OPENCV_ELEM(P, i, j) = -1 * OPENCV_ELEM(P, i, j);
			}
		}

		// oki
		finite = mvg_finite_projection_matrix_decomposition(P, K, R, T);
		printf("\n----\n\n");
		opencv_debug("P", P);
		opencv_debug("R", R);
		opencv_debug("K", K);
		opencv_debug("T", T);
		printf("Det(R) = %f\n", cvDet(R));

		mvg_assemble_projection_matrix(K, R, T, P); 
		opencv_debug("Again P", P); */

		return finite;
	}
	else
	{
		return true;
	}
}

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
//   - sufficiently large consensus set is found 
//
bool mvg_resection_RANSAC(
	const CvMat * const vertices, 
	const CvMat * const projected, 
	CvMat * const P, 
	CvMat * const K, 
	CvMat * const R, 
	CvMat * const T, 
	bool normalize_A /*= false*/,
	const int trials /*= 500*/, 
	const double threshold /*= 3.0*/,
	bool * inliers /*= NULL*/
)
{
	// transform input
	const int n = vertices->cols;
	const double threshold_sq = threshold * threshold;
	const bool homogeneous = vertices->rows == 4;

	// allocate
	// CvMat * best_P = opencv_create_matrix(3, 4);
	bool * best_status = ALLOC(bool, n), * status = ALLOC(bool, n);
	int best_inliers_count = -1;

	// fail immediately if the solution is underdetermined 
	if (n < 6)
	{
		return false;
	}

	// do this many times 
	for (int i = 0; i < trials; i++) 
	{
		// pick randomly 6 correspondences 
		memset(status, 0, sizeof(bool) * n);
		int samples[6];
		for (int count = 0; count < 6;)
		{
			const int pick = rand() % n; 
			if (!status[pick])
			{
				status[pick] = true; 
				samples[count] = pick;
				count++;
			}
		}

		// calculate using mvg_resection_SVD 
		if (!mvg_resection_SVD(vertices, projected, P, NULL, NULL, NULL, normalize_A, samples, 6)) continue;

		// count and mark the inliers 
		int inliers_count = 0; 
		for (int j = 0; j < n; j++) 
		{
			double reprojection[2]; 
			// note that here we're using "dirty" projection method which is suitable only for visualization; on the upside, it shouldn't matter because 
			// if a point is projected as infinite, there's something wrong with it anyway
			if (homogeneous)
			{
				opencv_vertex_projection_visualization(
					P, 
					OPENCV_ELEM(vertices, 0, j),
					OPENCV_ELEM(vertices, 1, j),
					OPENCV_ELEM(vertices, 2, j),
					OPENCV_ELEM(vertices, 3, j),
					reprojection
				);
			}
			else
			{
				opencv_vertex_projection_visualization(
					P, 
					OPENCV_ELEM(vertices, 0, j), 
					OPENCV_ELEM(vertices, 1, j), 
					OPENCV_ELEM(vertices, 2, j), 
					reprojection
				);
			}

			const double 
				dx = reprojection[0] - OPENCV_ELEM(projected, 0, j), 
				dy = reprojection[1] - OPENCV_ELEM(projected, 1, j);
			
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

		// check for best sample 
		if (inliers_count > best_inliers_count)
		{
			bool * temp = best_status; 
			best_status = status; 
			status = temp;
			best_inliers_count = inliers_count; 
			// cvCopy(P, best_P);
		}

		// debug 
		// printf("%d ", inliers_count);
	}

	FREE(status);
	if (best_inliers_count < 6)
	{
		FREE(best_status);
		return false;
	}

	// calculate camera calibration using only inliers
	int * samples = ALLOC(int, best_inliers_count);
	int j = 0; 
	for (int i = 0; i < n; i++)
	{
		if (best_status[i]) 
		{
			samples[j] = i;
			j++; 
		}

		if (inliers) 
		{
			inliers[i] = best_status[i];
		}
	}

	bool ok = mvg_resection_SVD(vertices, projected, P, K, R, T, normalize_A, samples, best_inliers_count);
	if (!ok) printf("FAIL "); 
	FREE(samples);
	FREE(best_status);

	// cvCopy(best_P, P);

	return ok; 
}

// clamps down some values in internal calibration matrix
bool mvg_restrict_calibration_matrix(CvMat * const K, const bool zero_skew, const bool square_pixels) 
{
	if (!K || OPENCV_ELEM(K, 2, 2) != 1) 
	{
		return false;
	}

	const double 
		ratio = OPENCV_ELEM(K, 0, 0) / OPENCV_ELEM(K, 1, 1), 
		average_scale = 0.5 * (OPENCV_ELEM(K, 0, 0) + OPENCV_ELEM(K, 1, 1)),
		skew = OPENCV_ELEM(K, 0, 1);

	if (zero_skew) 
	{
		OPENCV_ELEM(K, 0, 1) = 0;
	}
	
	if (square_pixels) 
	{
		OPENCV_ELEM(K, 0, 0) = average_scale; 
		OPENCV_ELEM(K, 1, 1) = average_scale; 
	}

	return true;
}
