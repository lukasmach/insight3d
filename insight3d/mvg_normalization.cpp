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

#include "mvg_normalization.h"

// normalize image points so that their centroid and typical magnitude of 
// the vector is (1, 1)
bool mvg_normalize_points(CvMat * points, CvMat * H, double * output_scale) 
{
	double sum_x = 0, sum_y = 0;

	// initialize normalizing homography 
	cvZero(H); 
	OPENCV_ELEM(H, 0, 0) = 1; 
	OPENCV_ELEM(H, 1, 1) = 1;
	OPENCV_ELEM(H, 2, 2) = 1;

	// find centroid
	for (size_t i = 0; i < points->cols; i++) 
	{
		sum_x += OPENCV_ELEM(points, 0, i); 
		sum_y += OPENCV_ELEM(points, 1, i); 
	}
	
	const double avg_x = sum_x / (double)points->cols, avg_y = sum_y / (double)points->cols;

	// move centroid to origin
	double sum = 0;
	for (size_t i = 0; i < points->cols; i++)
	{
		OPENCV_ELEM(points, 0, i) -= avg_x;
		OPENCV_ELEM(points, 1, i) -= avg_y;
		const double a = OPENCV_ELEM(points, 0, i), b = OPENCV_ELEM(points, 1, i);
		sum += sqrt(a * a + b * b);
	}

	// calculate variance
	const double avg = sum / (double)points->cols;

	double scale = 1;
	if (avg < -1.0e-6 || avg > 1.0e-6)
	{
		scale = sqrt((double)2) / avg;
	}

	for (size_t i = 0; i < points->cols; i++) 
	{
		OPENCV_ELEM(points, 0, i) *= scale;
		OPENCV_ELEM(points, 1, i) *= scale;
	}

	// translation 
	OPENCV_ELEM(H, 0, 2) = -avg_x; 
	OPENCV_ELEM(H, 1, 2) = -avg_y; 

	// scale 
	if (output_scale) *output_scale = scale;
	CvMat * S = opencv_create_I_matrix(3); 
	OPENCV_ELEM(S, 0, 0) = scale; 
	OPENCV_ELEM(S, 1, 1) = scale;

	// compute final normalizing homography, release and return
	cvMatMul(S, H, H);
	cvReleaseMat(&S);

	return true;
}
