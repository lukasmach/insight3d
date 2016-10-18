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

#include "mvg_decomposition.h"
#include "cv_extensions.h"

// decomposition of projection matrix into rotation, translation and internal calibration 
bool mvg_finite_projection_matrix_decomposition(CvMat * const P, CvMat * const K, CvMat * const R, CvMat * const T)
{
	// extract camera center, i.e. calculate the right null vector of P 
	CvMat * W = cvCreateMat(4, 1, CV_64F), * V_transposed = cvCreateMat(4, 4, CV_64F);
	cvSVD(P, W, NULL, V_transposed, CV_SVD_V_T); // todo check singular values of P for numerical stability

	// check for camera at infinity 
	const double w = OPENCV_ELEM(V_transposed, 3, 3);
	if (w == 0)
	{
		// deallocate and fail
		cvReleaseMat(&W); 
		cvReleaseMat(&V_transposed);
		return false; 
	}

	// camera center is the last row of V_transposed
	OPENCV_ELEM(T, 0, 0) = OPENCV_ELEM(V_transposed, 3, 0) / w;
	OPENCV_ELEM(T, 1, 0) = OPENCV_ELEM(V_transposed, 3, 1) / w;
	OPENCV_ELEM(T, 2, 0) = OPENCV_ELEM(V_transposed, 3, 2) / w;

	// fill in data for RQ decomposition 
	CvMat * M = cvCreateMat(3, 3, CV_64F);
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++)
		{
			OPENCV_ELEM(M, i, j) = OPENCV_ELEM(P, i, j);
		}
	}

	// allocate structures
	CvPoint3D64f euler = cvPoint3D64f(0, 0, 0); 

	// perform decomposition 
	CvMat * Qx = cvCreateMat(3, 3, CV_64F), * Qy = cvCreateMat(3, 3, CV_64F), * Qz = cvCreateMat(3, 3, CV_64F);
	cvComputeRQDecomposition(M, K, R, Qx, Qy, Qz, &euler);

	// multiply the calibration matrix so that [3, 3] entry is 1
	const double K33 = OPENCV_ELEM(K, 2, 2); 
	if (K33 == 0) 
	{ 
		// todo probably inifinite camera, we could handle this (and we've probably already done so, 
		// since we check for inifinite cameras in the code above)
		cvReleaseMat(&V_transposed);
		cvReleaseMat(&W);
		cvReleaseMat(&M);
		cvReleaseMat(&Qx);
		cvReleaseMat(&Qy);
		cvReleaseMat(&Qz);
		return false; 
	}
	const double lambda = 1 / K33; 
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			OPENCV_ELEM(K, i, j) *= lambda;
		}
	}
	OPENCV_ELEM(K, 2, 2) = 1;

	// release allocated data 
	cvReleaseMat(&V_transposed);
	cvReleaseMat(&W);
	cvReleaseMat(&M);
	cvReleaseMat(&Qx);
	cvReleaseMat(&Qy);
	cvReleaseMat(&Qz);
	return true;
}

// assemble projection matrix
void mvg_assemble_projection_matrix(CvMat * internal_calibration, CvMat * rotation, CvMat * translation, CvMat * projection)
{
	// compute temporary matrices
	CvMat * M = cvCreateMat(3, 3, CV_64F);
	cvMatMul(internal_calibration, rotation, M); 
	CvMat * minusC = cvCreateMat(3, 1, CV_64F);
	cvCopy(translation, minusC);
	OPENCV_ELEM(minusC, 0, 0) *= -1; 
	OPENCV_ELEM(minusC, 1, 0) *= -1; 
	OPENCV_ELEM(minusC, 2, 0) *= -1; 
	CvMat * minusMC = cvCreateMat(3, 1, CV_64F); 
	cvMatMul(M, minusC, minusMC); 

	// save values 
	for (size_t j = 0; j < 3; j++) 
	{
		for (size_t k = 0; k < 3; k++) 
		{
			OPENCV_ELEM(projection, j, k) = OPENCV_ELEM(M, j, k); 
		}

		OPENCV_ELEM(projection, j, 3) = OPENCV_ELEM(minusMC, j, 0);
	}

	// release temporary matrices
	cvReleaseMat(&M); 
	cvReleaseMat(&minusC); 
	cvReleaseMat(&minusMC); 
}
