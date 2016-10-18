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

#include "tool_coordinates.h"

// note that this small tool doesn't follow naming conventions of tool subsystem
// and there's no real reason for this

// create UI elements for changing coordinates
void tool_coordinates_create()
{
	tool_create(UI_MODE_UNSPECIFIED, "Coordinate frames", "Changing world-space coordinate frames");
	tool_register_menu_function("Main menu|Modelling|Coordinate frame...|Reorient|", coordinates_reorient_using_current_camera);
}

// rotate the world-space so that the coordinate frame is orientet in the same direction as current camera 
// obtains LOCK_RW
void coordinates_reorient_using_current_camera()
{
	if (INDEX_IS_SET(ui_state.current_shot) && shots.data[ui_state.current_shot].calibrated)
	{
		coordinates_rotate_all_cameras(ui_state.current_shot);
		visualization_process_data(vertices, shots);
	}
}

// rotate space so that the coordinate frame coincides with chosen camera orientation
bool coordinates_rotate_all_cameras(size_t shot_id) 
{
	ASSERT(validate_shot(shot_id), "invalid shot supplied when transforming calibration into camera's coordinate frame"); 
	const Shot * const shot = shots.data + shot_id; 
	CvMat * H; 

	LOCK_RW(opencv)
	{
		H = cvCreateMat(4, 4, CV_64F); 
		CvMat * I = cvCreateMat(4, 4, CV_64F);
		cvZero(I);
		cvZero(H); 

		OPENCV_ELEM(I, 0, 0) = -1;
		OPENCV_ELEM(I, 1, 1) = 1;
		OPENCV_ELEM(I, 2, 2) = 1;
		OPENCV_ELEM(I, 3, 3) = 1;
		OPENCV_ELEM(H, 3, 3) = 1;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				OPENCV_ELEM(H, i, j) = OPENCV_ELEM(shot->rotation, j, i);
			}
		}

		cvMatMul(H, I, H);
		cvReleaseMat(&I);
	}
	UNLOCK_RW(opencv);

	printf("Coordinate frame aligned with camera.\n");

	return coordinates_apply_homography_to_cameras(H);
	// todo release H
}

// applies homography to all calibrated cameras
bool coordinates_apply_homography_to_cameras(CvMat * H)
{
	for ALL(shots, i) 
	{
		// consider only calibrated shots 
		const Shot * const shot = shots.data + i;
		if (!shot->calibrated) continue;

		// multiply the projection matrix 
		ATOMIC_RW(opencv, cvMatMul(shot->projection, H, shot->projection); );
		
		// we need to decompose projection matrix to remain consistent 
		geometry_calibration_from_P(i);
	}

	return true;
}
