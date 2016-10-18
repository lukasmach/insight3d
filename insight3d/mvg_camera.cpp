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

#include "mvg_camera.h"

// computes projective depth of a point X with respect to finite camera P
double mvg_projective_depth(CvMat * P, CvMat * X) 
{
	// check if X is represented by a 4-vector or 3-vector 
	const double T = X->rows == 4 ? OPENCV_ELEM(X, 3, 0) : 1;

	// retrieve 3x3 submatrix 
	CvMat * M = cvCreateMat(3, 3, CV_64F);
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			OPENCV_ELEM(M, j, i) = OPENCV_ELEM(P, j, i);
		}
	}

	// calculate the sign(det(M)) and w 
	double d = cvDet(M); 
	if (d != 0) d = d > 0 ? 1 : -1; else d = 0;
	const double w = 
		OPENCV_ELEM(X, 0, 0) * OPENCV_ELEM(P, 2, 0) +  
		OPENCV_ELEM(X, 1, 0) * OPENCV_ELEM(P, 2, 1) +  
		OPENCV_ELEM(X, 2, 0) * OPENCV_ELEM(P, 2, 2) +  
		T * OPENCV_ELEM(P, 2, 3);
	 
	// and finally |M_3| 
	double M3_norm = 0; 
	for (int i = 0; i < 3; i++) 
	{
		const double a = OPENCV_ELEM(M, 2, i); 
		M3_norm += a * a; 
	}
	cvReleaseMat(&M);
	M3_norm = cvSqrt(M3_norm);

	// check for division by zero 
	double denominator = T * M3_norm; 
	if (denominator == 0) return 1; // todo check if this happens iff P is camera at infini

	// return result 
	return (d * w) / denominator; 
}

// returns true if the point is in front of finite camera 
bool mvg_point_in_front_of_camera(CvMat * P, CvMat * X) 
{
	// which is iff the projective depth is non-negative
	return mvg_projective_depth(P, X) >= 0; 
}

// create canonical camera 
void mvg_canonical_P(CvMat * P) 
{
	cvZero(P); 

	for (size_t i = 0; i < 3; i++) 
	{
		OPENCV_ELEM(P, i, i) = 1;
	}
}

// extracts canonical pair of projective matrices from fundamental matrix
bool mvg_extract_Ps_from_F(CvMat * F, CvMat * P1, CvMat * P2)
{
	// first extract left null vector of F 
	CvMat * e_prime = opencv_left_null_vector(F);
	/*CvMat * e_prime_transposed = opencv_create_matrix(1, 3); 
	cvTranspose(e_prime, e_prime_transposed);
	CvMat * r = opencv_create_matrix(1, 3); 
	cvMatMul(e_prime_transposed, F, r);
	opencv_debug("Should be zero!", r);*/
	mvg_canonical_P(P1);
	CvMat * Ex = opencv_create_cross_product_matrix(e_prime);
	CvMat * ExF = opencv_create_matrix(3, 3); 
	cvMatMul(Ex, F, ExF);

	// assemble P2 
	for (size_t i = 0; i < 3; i++) 
	{
		for (size_t j = 0; j < 3; j++) 
		{
			OPENCV_ELEM(P2, i, j) = OPENCV_ELEM(ExF, i, j);
		}

		OPENCV_ELEM(P2, i, 3) = OPENCV_ELEM(e_prime, i, 0);
	}

	// release resources 
	cvReleaseMat(&e_prime); 
	cvReleaseMat(&Ex); 
	cvReleaseMat(&ExF); 

	return true;
}

// calculates fundamental matrix for a pair of views given their projective matrices 
bool mvg_calculate_F_from_Ps(CvMat * P1, CvMat * P2, CvMat * F) 
{
	// find center of the first camera
	CvMat * C = opencv_right_null_vector(P1); 

	// project it using P2 to find epipole e_prime 
	CvMat * e_prime = opencv_create_matrix(3, 1);
	cvMatMul(P2, C, e_prime);

	// create cross product matrix for epipole 
	CvMat * Ex = opencv_create_cross_product_matrix(e_prime);

	// calculate pseudoinverse of P1
	CvMat * P1_inverse = opencv_create_matrix(4, 3); 
	cvInvert(P1, P1_inverse, CV_SVD);

	// F = Ex * P2 * P1_inverse 
	CvMat * ExP2 = opencv_create_matrix(3, 4); 
	cvMatMul(Ex, P2, ExP2); 
	cvMatMul(ExP2, P1_inverse, F);

	// release memory
	cvReleaseMat(&C);
	cvReleaseMat(&e_prime);
	cvReleaseMat(&Ex);
	cvReleaseMat(&P1_inverse);
	cvReleaseMat(&ExP2);

	return true;
}
