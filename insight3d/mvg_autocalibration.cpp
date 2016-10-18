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

#include "mvg_autocalibration.h"

double sq(const double x) { return x * x; }

// perform autocalibration using absolute quadric
bool mvg_autocalibration(CvMat ** Ps, double * principal_points, const size_t n, CvMat ** Xs, const size_t m)
{
	if (n < 3)
	{
		return false;
	}

	printf("*****************************************\n");
	opencv_debug("First camera before transformation", Ps[0]);

	// move the principal point to the origin for every camera
	// and use canonical first camera
	CvMat * T = opencv_create_matrix(3, 3);
	CvMat * S = opencv_create_matrix(3, 3);
	CvMat * G = opencv_create_matrix(4, 4);	
	CvMat * H = opencv_create_matrix(4, 4);
	for (size_t i = 0; i < n; i++) 
	{
		// set up translation matrix 
		cvZero(T);
		OPENCV_ELEM(T, 0, 0) = 1;
		OPENCV_ELEM(T, 1, 1) = 1;
		OPENCV_ELEM(T, 2, 2) = 1;
		OPENCV_ELEM(T, 0, 2) = -principal_points[2 * i + 0]; 
		OPENCV_ELEM(T, 1, 2) = -principal_points[2 * i + 1]; 

		// apply it to the projection matrix
		cvMatMul(T, Ps[i], Ps[i]);

		// also scale 
		cvZero(S);
		OPENCV_ELEM(S, 0, 0) = 0.001; 
		OPENCV_ELEM(S, 1, 1) = 0.001; 
		OPENCV_ELEM(S, 2, 2) = 1; 
		cvMatMul(S, Ps[i], Ps[i]);

		// calculate the world-space homography which transforms P_1 to [I_3x3 | 0]
		if (i == 0) 
		{
			cvZero(G);
			OPENCV_ELEM(G, 3, 3) = 1;
			for (int i = 0; i < 3; i++)  
			{
				for (int j = 0; j < 4; j++) 
				{
					OPENCV_ELEM(G, i, j) = OPENCV_ELEM(Ps[0], i, j);
				}
			}
			cvInvert(G, H, CV_SVD);
		}

		// apply the homography to every camera
		cvMatMul(Ps[i], H, Ps[i]);
	}

	// also apply inverse homography to all the points
	for (size_t i = 0; i < m; i++)
	{
		cvMatMul(G, Xs[i], Xs[i]);
	}

	// debug
	opencv_debug("First camera", Ps[0]);
	opencv_debug("Transformed using this transformation", H);

	printf("*****************************************\n");
	printf("List of all cameras:\n");
	for (size_t i = 0; i < n; i++) 
	{
		opencv_debug("Camera", Ps[i]);
	}

	cvReleaseMat(&S);
	cvReleaseMat(&T); 
	cvReleaseMat(&H);
	cvReleaseMat(&G);
	
	// construct system of linear equations 
	CvMat * W = opencv_create_matrix(4 * (n - 1), 5), * b = opencv_create_matrix(4 * (n - 1), 1);
	for (size_t i = 1; i < n; i++) 
	{
		const double
			p11 = OPENCV_ELEM(Ps[i], 0, 0),
			p12 = OPENCV_ELEM(Ps[i], 0, 1),
			p13 = OPENCV_ELEM(Ps[i], 0, 2),
			p14 = OPENCV_ELEM(Ps[i], 0, 3),

			p21 = OPENCV_ELEM(Ps[i], 1, 0),
			p22 = OPENCV_ELEM(Ps[i], 1, 1),
			p23 = OPENCV_ELEM(Ps[i], 1, 2),
			p24 = OPENCV_ELEM(Ps[i], 1, 3),

			p31 = OPENCV_ELEM(Ps[i], 2, 0),
			p32 = OPENCV_ELEM(Ps[i], 2, 1),
			p33 = OPENCV_ELEM(Ps[i], 2, 2),
			p34 = OPENCV_ELEM(Ps[i], 2, 3)
		;

		const double
			p11_2 = sq(p11),
			p12_2 = sq(p12),
			p21_2 = sq(p21), 
			p22_2 = sq(p22), 
			p14_2 = sq(p14),
			p24_2 = sq(p24),
			p13_2 = sq(p13),
			p23_2 = sq(p23)
		;

		OPENCV_ELEM(W, (i - 1) * 4 + 0, 0) = p11_2 + p12_2 - p21_2 - p22_2;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 1) = 2 * p11 * p14 - 2 * p21 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 2) = 2 * p12 * p14 - 2 * p22 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 3) = 2 * p13 * p14 - 2 * p23 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 4) = p14_2 - p24_2;
		OPENCV_ELEM(b, (i - 1) * 4 + 0, 0) = -(p13_2 - p23_2);

		OPENCV_ELEM(W, (i - 1) * 4 + 1, 0) = p11 * p21 + p12 * p22;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 1) = p14 * p21 + p11 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 2) = p14 * p22 + p12 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 3) = p14 * p23 + p13 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 4) = p14 * p24;
		OPENCV_ELEM(b, (i - 1) * 4 + 1, 0) = -(p13 * p23);

		OPENCV_ELEM(W, (i - 1) * 4 + 2, 0) = p11 * p31 + p12 * p32;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 1) = p14 * p31 + p11 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 2) = p14 * p32 + p12 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 3) = p14 * p33 + p13 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 4) = p14 * p34;
		OPENCV_ELEM(b, (i - 1) * 4 + 2, 0) = -(p13 * p33);

		OPENCV_ELEM(W, (i - 1) * 4 + 3, 0) = p21 * p31 + p22 * p32;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 1) = p24 * p31 + p21 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 2) = p24 * p32 + p22 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 3) = p24 * p33 + p23 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 4) = p24 * p34;
		OPENCV_ELEM(b, (i - 1) * 4 + 3, 0) = -(p23 * p33);
	}

	opencv_debug("Autocalibrating equations", W);
	
	CvMat * solution = opencv_create_matrix(5, 1);
	cvSolve(W, b, solution, CV_SVD);

	// if first row in solution is not positive, replace W by -W
	/*if (OPENCV_ELEM(solution, 0, 0) <= 0) 
	{
		printf("--- !!! --- Multiplying the matrix by -1 --- !!! ---\n");
		for (int i = 0; i < W->rows; i++) 
		{
			for (int j = 0; j < W->cols; j++) 
			{
				OPENCV_ELEM(W, i, j) *= -1;
			}
		}

		cvSolve(W, b, solution, CV_SVD);
	}*/

	opencv_debug("Solution", solution);
	cvReleaseMat(&W); 
	cvReleaseMat(&b);

	// compute f_1, K_1 and w (the focal length of the first camera, calibration matrix 
	// of the first camera and the plane at infinity)
	double f_1 = OPENCV_ELEM(solution, 0, 0);
	if (f_1 < 0) 
	{
		printf("--- Multiplying f^2 by -1 ---\n");
		f_1 *= -1;
	}

	f_1 = sqrt(f_1);
	// f_1 *= 1000.0;
	printf("f_1 = %f\n\n", f_1);
	CvMat * K_1 = opencv_create_I_matrix(3); 
	OPENCV_ELEM(K_1, 0, 0) = f_1; 
	OPENCV_ELEM(K_1, 1, 1) = f_1;
	CvMat * w = opencv_create_matrix(3, 1); 
	OPENCV_ELEM(w, 0, 0) = OPENCV_ELEM(solution, 1, 0) / f_1;
	OPENCV_ELEM(w, 1, 0) = OPENCV_ELEM(solution, 2, 0) / f_1;
	OPENCV_ELEM(w, 2, 0) = OPENCV_ELEM(solution, 3, 0);

	// check with the last value of the solution vector, which contains 
	// the scalar product w_transposed * w
	const double w_Tw = sq(OPENCV_ELEM(w, 0, 0)) + sq(OPENCV_ELEM(w, 1, 0)) + sq(OPENCV_ELEM(w, 2, 0));

	// debug 
	opencv_debug("K_1", K_1);
	opencv_debug("w", w); 
	printf("difference between calculated and recomputed w_T * w = %f - %f = %f\n", OPENCV_ELEM(solution, 4, 0), w_Tw, OPENCV_ELEM(solution, 4, 0) - w_Tw);

	// contruct rectifying homography 
	CvMat * H_metric = opencv_create_matrix(4, 4);
	cvZero(H_metric);
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			OPENCV_ELEM(H_metric, i, j) = OPENCV_ELEM(K_1, i, j); 
		}
	}

	CvMat * H_metric_4_prime = opencv_create_matrix(1, 3);
	cvTranspose(w, w);
	cvMatMul(w, K_1, H_metric_4_prime);

	for (int j = 0; j < 3; j++)
	{
		OPENCV_ELEM(H_metric, 3, j) = OPENCV_ELEM(H_metric_4_prime, 0, j);
	}

	OPENCV_ELEM(H_metric, 3, 3) = 1;
	cvInvert(H_metric, H_metric);

	// apply rectifying homography to all points
	for (size_t i = 0; i < m; i++) 
	{
		cvMatMul(H_metric, Xs[i], Xs[i]);
	}

	// release resources 
	cvReleaseMat(&K_1); 
	cvReleaseMat(&w);
	cvReleaseMat(&solution);

	return true;
}

/*double sq(const double x) { return x * x; }

// perform autocalibration using absolute quadric
void mvg_autocalibration(const size_t n, CvMat ** Ps, double * principal_points) 
{
	if (n < 3) 
	{
		return; 
	}

	opencv_debug("First camera before transformation", Ps[0]);

	// move the principal point to the origin for every camera 
	// and use canonical first camera
	CvMat * T = opencv_create_matrix(3, 3);
	CvMat * H = opencv_create_matrix(4, 4);
	for (size_t i = 0; i < n; i++) 
	{
		// set up translation matrix 
		cvZero(T);
		OPENCV_ELEM(T, 0, 0) = 1;
		OPENCV_ELEM(T, 1, 1) = 1;
		OPENCV_ELEM(T, 2, 2) = 1;
		OPENCV_ELEM(T, 0, 2) = -principal_points[2 * i + 0]; 
		OPENCV_ELEM(T, 1, 2) = -principal_points[2 * i + 1]; 

		// apply it to the projection matrix
		cvMatMul(T, Ps[i], Ps[i]);

		// calculate the world-space homography which transforms P_1 to [I_3x3 | 0]
		if (i == 0) 
		{
			CvMat * G = opencv_create_matrix(4, 4);	
			cvZero(G); 
			OPENCV_ELEM(G, 3, 3) = 1;
			for (int i = 0; i < 3; i++)  
			{
				for (int j = 0; j < 4; j++) 
				{
					OPENCV_ELEM(G, i, j) = OPENCV_ELEM(Ps[0], i, j);
				}
			}
			cvInvert(G, H, CV_SVD);
			cvReleaseMat(&G);
		}

		// apply the homography to every camera
		cvMatMul(Ps[i], H, Ps[i]);
	}

	// debug
	opencv_debug("First camera", Ps[0]);
	opencv_debug("Transformed using this transformation", H);
	
	// construct system of linear equations 
	CvMat * W = opencv_create_matrix(4 * (n - 1), 5), * b = opencv_create_matrix(4 * (n - 1), 1);
	for (size_t i = 1; i < n; i++) 
	{
		const double
			p11 = OPENCV_ELEM(Ps[i], 0, 0),
			p12 = OPENCV_ELEM(Ps[i], 0, 1),
			p13 = OPENCV_ELEM(Ps[i], 0, 2),
			p14 = OPENCV_ELEM(Ps[i], 0, 3),

			p21 = OPENCV_ELEM(Ps[i], 1, 0),
			p22 = OPENCV_ELEM(Ps[i], 1, 1),
			p23 = OPENCV_ELEM(Ps[i], 1, 2),
			p24 = OPENCV_ELEM(Ps[i], 1, 3),

			p31 = OPENCV_ELEM(Ps[i], 2, 0),
			p32 = OPENCV_ELEM(Ps[i], 2, 1),
			p33 = OPENCV_ELEM(Ps[i], 2, 2),
			p34 = OPENCV_ELEM(Ps[i], 2, 3)
		;

		const double
			p11_2 = sq(p11),
			p12_2 = sq(p12),
			p21_2 = sq(p21), 
			p22_2 = sq(p22), 
			p14_2 = sq(p14),
			p24_2 = sq(p24),
			p13_2 = sq(p13),
			p23_2 = sq(p23)
		;

		OPENCV_ELEM(W, (i - 1) * 4 + 0, 0) = p11_2 + p12_2 - p21_2 - p22_2;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 1) = 2 * p11 * p14 - 2 * p21 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 2) = 2 * p12 * p14 - 2 * p22 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 3) = 2 * p13 * p14 - 2 * p23 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 4) = p14_2 - p24_2;
		OPENCV_ELEM(b, (i - 1) * 4 + 0, 0) = -(p13_2 - p23_2);

		OPENCV_ELEM(W, (i - 1) * 4 + 1, 0) = p11 * p21 + p12 * p22;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 1) = p14 * p21 + p11 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 2) = p14 * p22 + p12 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 3) = p14 * p23 + p13 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 4) = p14 * p24;
		OPENCV_ELEM(b, (i - 1) * 4 + 1, 0) = -(p13 * p23);

		OPENCV_ELEM(W, (i - 1) * 4 + 2, 0) = p11 * p31 + p12 * p32;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 1) = p14 * p31 + p11 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 2) = p14 * p32 + p12 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 3) = p14 * p33 + p13 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 4) = p14 * p34;
		OPENCV_ELEM(b, (i - 1) * 4 + 2, 0) = -(p13 * p33);

		OPENCV_ELEM(W, (i - 1) * 4 + 3, 0) = p21 * p31 + p22 * p32;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 1) = p24 * p31 + p21 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 2) = p24 * p32 + p22 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 3) = p24 * p33 + p23 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 4) = p24 * p34;
		OPENCV_ELEM(b, (i - 1) * 4 + 3, 0) = -(p23 * p33);
	}

	opencv_debug("Autocalibrating equations", W);
	
	CvMat * solution = opencv_create_matrix(5, 1);
	cvSolve(W, b, solution, CV_SVD);
	opencv_debug("Solution", solution);

	cvReleaseMat(&T);
}*/

// get the coefficient in autocalibrating equations
double q(CvMat * P, int i, int j, int c)
{
	i--; 
	j--;
	int ci, cj; 
	if (c < 4) 
	{
		ci = 0; 
		cj = c;
	}
	else if (c < 7)
	{
		ci = 1; 
		cj = c - 4 + 1;
	}
	else if (c < 9) 
	{
		ci = 2;
		cj = c - 7 + 2;
	}
	else if (c == 9) 
	{
		ci = 3;
		cj = 3;
	}
	else
	{
		printf("error, invalid index\n");
	}

	if (ci != cj) 
	{
		return OPENCV_ELEM(P, i, ci) * OPENCV_ELEM(P, j, cj) + OPENCV_ELEM(P, i, cj) * OPENCV_ELEM(P, j, ci);
	}
	else
	{
		return OPENCV_ELEM(P, i, ci) * OPENCV_ELEM(P, j, cj);
	}
}

// perform autocalibration using absolute quadric
bool mvg_autocalibration_2(CvMat ** Ps, double * principal_points, const size_t n, CvMat ** Xs, const size_t m, CvMat ** pi_infinity /*= NULL*/, bool affine /*= false*/)
{
	if (n < 3)
	{
		printf("at least three views must be selected\n");
		return false;
	}

	// create deep copy of the input data
	CvMat ** Ps_orig = ALLOC(CvMat *, n);
	for (size_t i = 0; i < n; i++)
	{
		Ps_orig[i] = Ps[i];
		Ps[i] = opencv_create_matrix(3, 4);
		cvCopy(Ps_orig[i], Ps[i]);
	}

	// move the principal point to the origin for every camera
	CvMat * T = opencv_create_matrix(3, 3);
	CvMat * S = opencv_create_matrix(3, 3);

	for (size_t i = 0; i < n; i++)
	{
		// set up the translation matrix
		cvZero(T);
		OPENCV_ELEM(T, 0, 0) = 1;
		OPENCV_ELEM(T, 1, 1) = 1;
		OPENCV_ELEM(T, 2, 2) = 1;
		OPENCV_ELEM(T, 0, 2) = -principal_points[2 * i + 0];
		OPENCV_ELEM(T, 1, 2) = -principal_points[2 * i + 1];

		// apply it to the projection matrix
		cvMatMul(T, Ps[i], Ps[i]);

		// also normalize scale
		cvZero(S);
		OPENCV_ELEM(S, 0, 0) = 0.001;
		OPENCV_ELEM(S, 1, 1) = 0.001;
		OPENCV_ELEM(S, 2, 2) = 1.0;
		cvMatMul(S, Ps[i], Ps[i]);
	}

	cvReleaseMat(&T); 
	cvReleaseMat(&S);

	// RANSAC paradigm state 
	const size_t samples = 3;
	int best_inliers_count = 0; 
	size_t best_inliers_ids[samples];
	bool 
		* best_inliers_marked = ALLOC(bool, n), 
		* inliers_marked = ALLOC(bool, n), 
		* sample_marked = ALLOC(bool, n)
	;
	memset(best_inliers_marked, 0, n * sizeof(bool));
	memset(best_inliers_ids, 0, samples * sizeof(size_t));

	// result
	CvMat 
		* pi_inf = NULL, 
		* H_rectify = NULL, 
		* H_rectify_inv = NULL, 
		* solution = NULL, 
		* W = NULL
	;

	// --- begin --- RANSAC paradigm iterator
	const int total = 500;
	for (int tries = 0; tries <= total; tries++) 
	{
	// --- end   --- RANSAC paradigm iterator

	// if this is the last RANSAC iteration, we'll use all inliers from the best sample
	memset(sample_marked, 0, n * sizeof(bool));
	
	if (tries == total) 
	{
		if (best_inliers_count >= 3) 
		{
			// use all inliers 
			memcpy(sample_marked, best_inliers_marked, n * sizeof(bool));
			printf("Last iteration, using the following sample: ");
			for (size_t i = 0; i < n; i++) { printf(sample_marked[i] ? "T" : "F"); }
			printf("\n");
		}
		else
		{
			// fallback method - use all shots
			printf("Failed to find consistent sample when autocalibrating! Using all shots!\n");
			memset(sample_marked, ~0, n * sizeof(bool));
			best_inliers_count = n;
		}
	}
	else
	{
		// generate sample 
		memset(sample_marked, 0, n * sizeof(bool)); 
		for (int count = 0; count < samples;)
		{
			const int pick = rand() % n; 
			if (!sample_marked[pick]) 
			{
				sample_marked[pick] = true;
				count++;
			}
		}
	}

	// fill the matrix W containing linear equations determining Q
	W = opencv_create_matrix(4 * (tries == total ? best_inliers_count : samples), 10);
	cvZero(W);
	int i = 0;
	for (size_t j = 0; j < n; j++)
	{
		if (!sample_marked[j]) continue;

		// shortcut for P 
		CvMat * const P = Ps[j];

		// (P * Omega * P_t)_12 = 0
		OPENCV_ELEM(W, i, 0) = q(P, 1, 2, 0);
		OPENCV_ELEM(W, i, 1) = q(P, 1, 2, 1);
		OPENCV_ELEM(W, i, 2) = q(P, 1, 2, 2);
		OPENCV_ELEM(W, i, 3) = q(P, 1, 2, 3);
		OPENCV_ELEM(W, i, 4) = q(P, 1, 2, 4);
		OPENCV_ELEM(W, i, 5) = q(P, 1, 2, 5);
		OPENCV_ELEM(W, i, 6) = q(P, 1, 2, 6);
		OPENCV_ELEM(W, i, 7) = q(P, 1, 2, 7);
		OPENCV_ELEM(W, i, 8) = q(P, 1, 2, 8);
		OPENCV_ELEM(W, i, 9) = q(P, 1, 2, 9);
		i++;

		// (P * Omega * P_t)_13 = 0
		OPENCV_ELEM(W, i, 0) = q(P, 1, 3, 0);
		OPENCV_ELEM(W, i, 1) = q(P, 1, 3, 1);
		OPENCV_ELEM(W, i, 2) = q(P, 1, 3, 2);
		OPENCV_ELEM(W, i, 3) = q(P, 1, 3, 3);
		OPENCV_ELEM(W, i, 4) = q(P, 1, 3, 4);
		OPENCV_ELEM(W, i, 5) = q(P, 1, 3, 5);
		OPENCV_ELEM(W, i, 6) = q(P, 1, 3, 6);
		OPENCV_ELEM(W, i, 7) = q(P, 1, 3, 7);
		OPENCV_ELEM(W, i, 8) = q(P, 1, 3, 8);
		OPENCV_ELEM(W, i, 9) = q(P, 1, 3, 9);
		i++;

		// (P * Omega * P_t)_23 = 0
		OPENCV_ELEM(W, i, 0) = q(P, 2, 3, 0);
		OPENCV_ELEM(W, i, 1) = q(P, 2, 3, 1);
		OPENCV_ELEM(W, i, 2) = q(P, 2, 3, 2);
		OPENCV_ELEM(W, i, 3) = q(P, 2, 3, 3);
		OPENCV_ELEM(W, i, 4) = q(P, 2, 3, 4);
		OPENCV_ELEM(W, i, 5) = q(P, 2, 3, 5);
		OPENCV_ELEM(W, i, 6) = q(P, 2, 3, 6);
		OPENCV_ELEM(W, i, 7) = q(P, 2, 3, 7);
		OPENCV_ELEM(W, i, 8) = q(P, 2, 3, 8);
		OPENCV_ELEM(W, i, 9) = q(P, 2, 3, 9);
		i++;

		// (P * Omega * P_t)_11 - (P * Omega * P_t)_22 = 0
		OPENCV_ELEM(W, i, 0) = q(P, 1, 1, 0) - q(P, 2, 2, 0);
		OPENCV_ELEM(W, i, 1) = q(P, 1, 1, 1) - q(P, 2, 2, 1);
		OPENCV_ELEM(W, i, 2) = q(P, 1, 1, 2) - q(P, 2, 2, 2);
		OPENCV_ELEM(W, i, 3) = q(P, 1, 1, 3) - q(P, 2, 2, 3);
		OPENCV_ELEM(W, i, 4) = q(P, 1, 1, 4) - q(P, 2, 2, 4);
		OPENCV_ELEM(W, i, 5) = q(P, 1, 1, 5) - q(P, 2, 2, 5);
		OPENCV_ELEM(W, i, 6) = q(P, 1, 1, 6) - q(P, 2, 2, 6);
		OPENCV_ELEM(W, i, 7) = q(P, 1, 1, 7) - q(P, 2, 2, 7);
		OPENCV_ELEM(W, i, 8) = q(P, 1, 1, 8) - q(P, 2, 2, 8);
		OPENCV_ELEM(W, i, 9) = q(P, 1, 1, 9) - q(P, 2, 2, 9);
		i++;
	}

	// solve the system
	solution = opencv_right_null_vector(W);
	cvReleaseMat(&W);

	// construct Q 
	CvMat * Q_temp = opencv_create_matrix(4, 4);
	int si = 0;
	for (int i = 0; i < 4; i++) 
	{
		for (int j = i; j < 4; j++) 
		{
			OPENCV_ELEM(Q_temp, i, j) = OPENCV_ELEM(solution, si, 0);
			if (i != j) OPENCV_ELEM(Q_temp, j, i) = OPENCV_ELEM(solution, si, 0);
			si++;
		}
	}

	cvReleaseMat(&solution);

	// SVD decomposition 
	CvMat * D = opencv_create_matrix(4, 4), * V = opencv_create_matrix(4, 4), * U = opencv_create_matrix(4, 4);
	cvSVD(Q_temp, D, U, V, CV_SVD_V_T);
	cvReleaseMat(&Q_temp);
	OPENCV_ELEM(D, 3, 3) = 0; // Q has rank 3
	CvMat * Q_star_inf = opencv_create_matrix(4, 4);
	cvZero(Q_star_inf);
	cvMatMul(U, D, Q_star_inf);
	cvMatMul(Q_star_inf, V, Q_star_inf);

	// now find the pi_inf
	pi_inf = opencv_right_null_vector(Q_star_inf);
	H_rectify = opencv_create_I_matrix(4);
	H_rectify_inv = NULL;

	if (!affine) 
	{
		// full metric reconstruction
		CvMat * E_U = opencv_create_matrix(4, 4), * E_V = opencv_create_matrix(4, 4), * E_D = opencv_create_matrix(4, 1);
		cvSVD(Q_star_inf, E_D, E_U, E_V, CV_SVD_V_T);
		for (int j = 0; j < 3; j++)
		{
			for (int i = 0; i < 4; i++) 
			{
				OPENCV_ELEM(E_U, i, j) *= sqrt(OPENCV_ELEM(E_D, j, 0));
			}
		}
		cvInvert(E_U, H_rectify);
		H_rectify_inv = E_U;
		cvReleaseMat(&E_V);
		cvReleaseMat(&E_D);
	}
	else
	{
		// affine 
		for (int i = 0; i < 4; i++) { OPENCV_ELEM(H_rectify, 3, i) = OPENCV_ELEM(pi_inf, i, 0); }
		cvInvert(H_rectify, H_rectify_inv);
	}

	if (tries == total) 
	{
		// apply it to all points
		for (int i = 0; i < m; i++) 
		{
			cvMatMul(H_rectify, Xs[i], Xs[i]);
		}
	
		// apply it to original matrices 
		for (int i = 0; i < n; i++)
		{
			cvMatMul(Ps_orig[i], H_rectify_inv, Ps_orig[i]);
		}
	}

	// now calculate principal points of all the matrices and count inliers
	CvMat 
		* rectified_K = opencv_create_matrix(3, 3),
		* rectified_R = opencv_create_matrix(3, 3),
		* rectified_T = opencv_create_matrix(3, 1)
	;

	memset(inliers_marked, 0, n * sizeof(bool));
	size_t inliers_count = 0;
	CvMat * P_temp = opencv_create_matrix(3, 4); 

	printf("[");
	for (int i = 0; i < n; i++) 
	{
		// at the end of the final iteration, the matrices Ps_orig are already rectified
		if (tries < total) 
		{
			cvMatMul(Ps_orig[i], H_rectify_inv, P_temp);
		}
		else
		{
			cvCopy(Ps_orig[i], P_temp);
		}

		const bool decomposed = mvg_finite_projection_matrix_decomposition(P_temp, rectified_K, rectified_R, rectified_T);

		if (decomposed) 
		{
			const double PP_distance = sqrt( 
				sq(OPENCV_ELEM(rectified_K, 0, 2) - principal_points[2 * i + 0]) + 
				sq(OPENCV_ELEM(rectified_K, 1, 2) - principal_points[2 * i + 1])
			); 

			if (PP_distance < 100) 
			{
				inliers_count++;
				inliers_marked[i] = true;
			}
			else
			{
				inliers_marked[i] = false;
			}
			// test
			// inliers_marked[i] = sample_marked[i];
			printf("%f ", PP_distance);
		}
		else
		{
			inliers_marked[i] = false;
			printf("failed to extract PP\n");
		}
	}

	cvReleaseMat(&P_temp);

	// debug output
	printf("] ");
	for (size_t i = 0; i < n; i++) { printf(inliers_marked[i] ? "T" : "F"); }
	printf(" %d ", inliers_count); 
	printf("\n");

	// set the best result 
	if (inliers_count > best_inliers_count) 
	{
		memcpy(best_inliers_marked, inliers_marked, n * sizeof(bool)); 
		best_inliers_count = inliers_count; 
	}

	// TODO release allocated matrices 
	cvReleaseMat(&rectified_K);
	cvReleaseMat(&rectified_R);
	cvReleaseMat(&rectified_T);

	// --- begin --- RANSAC paradigm iterator close
	}
	printf("\n");
	// --- end   --- RANSAC paradigm iterator close

	// release memory
	if (pi_infinity) 
	{
		*pi_infinity = pi_inf;
	}
	else
	{
		cvReleaseMat(&pi_inf);
	}

	for (size_t i = 0; i < n; i++) 
	{
		cvReleaseMat(Ps + i);
	}
	
	cvReleaseMat(&H_rectify);
	cvReleaseMat(&H_rectify_inv);
	cvReleaseMat(&solution);
	cvReleaseMat(&W);

	return true;
}

/*double sq(const double x) { return x * x; }

// perform autocalibration using absolute quadric 
void mvg_autocalibration(const size_t n, CvMat ** Ps, double * principal_points) 
{
	if (n < 3) 
	{
		return; 
	}

	opencv_debug("First camera before transformation", Ps[0]);

	// move the principal point to the origin for every camera 
	// and use canonical first camera
	CvMat * T = opencv_create_matrix(3, 3);
	CvMat * H = opencv_create_matrix(4, 4);
	for (size_t i = 0; i < n; i++) 
	{
		// set up translation matrix 
		cvZero(T);
		OPENCV_ELEM(T, 0, 0) = 1;
		OPENCV_ELEM(T, 1, 1) = 1;
		OPENCV_ELEM(T, 2, 2) = 1;
		OPENCV_ELEM(T, 0, 2) = -principal_points[2 * i + 0]; 
		OPENCV_ELEM(T, 1, 2) = -principal_points[2 * i + 1]; 

		// apply it to the projection matrix
		cvMatMul(T, Ps[i], Ps[i]);

		// calculate the world-space homography which transforms P_1 to [I_3x3 | 0]
		if (i == 0) 
		{
			CvMat * G = opencv_create_matrix(4, 4);	
			cvZero(G); 
			OPENCV_ELEM(G, 3, 3) = 1;
			for (int i = 0; i < 3; i++)  
			{
				for (int j = 0; j < 4; j++) 
				{
					OPENCV_ELEM(G, i, j) = OPENCV_ELEM(Ps[0], i, j);
				}
			}
			cvInvert(G, H, CV_SVD);
			cvReleaseMat(&G);
		}

		// apply the homography to every camera
		cvMatMul(Ps[i], H, Ps[i]);
	}

	// debug
	opencv_debug("First camera", Ps[0]);
	opencv_debug("Transformed using this transformation", H);
	
	// construct system of linear equations 
	CvMat * W = opencv_create_matrix(4 * (n - 1), 5), * b = opencv_create_matrix(4 * (n - 1), 1);
	for (size_t i = 1; i < n; i++) 
	{
		const double
			p11 = OPENCV_ELEM(Ps[i], 0, 0),
			p12 = OPENCV_ELEM(Ps[i], 0, 1),
			p13 = OPENCV_ELEM(Ps[i], 0, 2),
			p14 = OPENCV_ELEM(Ps[i], 0, 3),

			p21 = OPENCV_ELEM(Ps[i], 1, 0),
			p22 = OPENCV_ELEM(Ps[i], 1, 1),
			p23 = OPENCV_ELEM(Ps[i], 1, 2),
			p24 = OPENCV_ELEM(Ps[i], 1, 3),

			p31 = OPENCV_ELEM(Ps[i], 2, 0),
			p32 = OPENCV_ELEM(Ps[i], 2, 1),
			p33 = OPENCV_ELEM(Ps[i], 2, 2),
			p34 = OPENCV_ELEM(Ps[i], 2, 3)
		;

		const double
			p11_2 = sq(p11),
			p12_2 = sq(p12),
			p21_2 = sq(p21), 
			p22_2 = sq(p22), 
			p14_2 = sq(p14),
			p24_2 = sq(p24),
			p13_2 = sq(p13),
			p23_2 = sq(p23)
		;

		OPENCV_ELEM(W, (i - 1) * 4 + 0, 0) = p11_2 + p12_2 - p21_2 - p22_2;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 1) = 2 * p11 * p14 - 2 * p21 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 2) = 2 * p12 * p14 - 2 * p22 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 3) = 2 * p13 * p14 - 2 * p23 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 0, 4) = p14_2 - p24_2;
		OPENCV_ELEM(b, (i - 1) * 4 + 0, 0) = -(p13_2 - p23_2);

		OPENCV_ELEM(W, (i - 1) * 4 + 1, 0) = p11 * p21 + p12 * p22;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 1) = p14 * p21 + p11 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 2) = p14 * p22 + p12 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 3) = p14 * p23 + p13 * p24;
		OPENCV_ELEM(W, (i - 1) * 4 + 1, 4) = p14 * p24;
		OPENCV_ELEM(b, (i - 1) * 4 + 1, 0) = -(p13 * p23);

		OPENCV_ELEM(W, (i - 1) * 4 + 2, 0) = p11 * p31 + p12 * p32;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 1) = p14 * p31 + p11 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 2) = p14 * p32 + p12 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 3) = p14 * p33 + p13 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 2, 4) = p14 * p34;
		OPENCV_ELEM(b, (i - 1) * 4 + 2, 0) = -(p13 * p33);

		OPENCV_ELEM(W, (i - 1) * 4 + 3, 0) = p21 * p31 + p22 * p32;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 1) = p24 * p31 + p21 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 2) = p24 * p32 + p22 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 3) = p24 * p33 + p23 * p34;
		OPENCV_ELEM(W, (i - 1) * 4 + 3, 4) = p24 * p34;
		OPENCV_ELEM(b, (i - 1) * 4 + 3, 0) = -(p23 * p33);
	}

	opencv_debug("Autocalibrating equations", W);
	
	CvMat * solution = opencv_create_matrix(5, 1);
	cvSolve(W, b, solution, CV_SVD);
	opencv_debug("Solution", solution);

	cvReleaseMat(&T);
}*/
