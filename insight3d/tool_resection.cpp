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

#include "tool_resection.h"

// register tool 
void tool_resection_create() 
{
	tool_create(UI_MODE_UNSPECIFIED, "Resection", "Estimate cameras using correspondence between reconstructred 3d vertices and their projection");
	tool_register_menu_function("Main menu|Calibration|Camera resection|", tool_resection_current_camera);
	tool_register_menu_function("Main menu|Calibration|More resection...|Uncalibrated, lattice test|", tool_resection_all_lattice);
	tool_register_menu_function("Main menu|Calibration|More resection...|Uncalibrated, enough points|", tool_resection_all_enough);
	tool_register_menu_function("Main menu|Calibration|More resection...|Uncalibrated cameras|", tool_resection_all_uncalibrated);
	tool_register_menu_function("Main menu|Calibration|More resection...|All cameras|", tool_resection_all);
	tool_register_menu_function("Main menu|Calibration|More resection...|Sydney|", tool_resection_sydney);
}

// refresh GUI 
void resection_refresh()
{
	ui_list_update();
	visualization_process_data(vertices, shots);
}

// resection of current camera
void tool_resection_current_camera()
{
	if (!INDEX_IS_SET(ui_state.current_shot))
	{
		return;
	}

	// call resectioning routine
	if (action_camera_resection(ui_state.current_shot, false, false)) 
	{
		shots.data[ui_state.current_shot].resected = true;
	}

	resection_refresh();
}

// resection of all uncalibrated cameras 
void tool_resection_all_uncalibrated()
{
	for ALL(shots, i) 
	{
		// check if this shot is uncalibrated 
		if (!shots.data[i].calibrated) 
		{
			// try camera calibration (will harmlessly fail when there are less than 6 points on the image) 
			if (action_camera_resection(i, false, false))
			{
				shots.data[i].resected = true;
			}
		}
	}

	resection_refresh();
}

// resection of all cameras
void tool_resection_all()
{
	for ALL(shots, i) 
	{
		// note just for now 
		// if (!shots.data[i].resected) continue;

		// try camera calibration (will harmlessly fail when there are less than 6 points on the image) 
		if (action_camera_resection(i, false, false)) 
		{
			shots.data[i].resected = true;
		}
	}

	resection_refresh();
}

void tool_resection_sydney()
{
	// calculate the average focal length 
	double avg_focal_length = 0; 
	int shots_count = 0; 

	for ALL(shots, i) 
	{
		avg_focal_length += shots.data[i].f; 
		shots_count++; 
	}

	avg_focal_length /= shots_count;
	printf("%f\n", avg_focal_length);

	// adjust the focal length of the cameras 
	/*for ALL(shots, i) 
	{
		shots.data[i].f = avg_focal_length; 
		geometry_calibration_from_P
	}*/
}

// resection of all uncalibrated cameras with at least 12 (=2*6) points
void tool_resection_all_enough()
{
	for ALL(shots, i) 
	{
		if (!shots.data[i].calibrated && query_count_reconstructed_points_on_shot(i) >= 12) 
		{
			if (action_camera_resection(i, false, false)) // todo set .resection inside action_camera (in all ui_event_menu_resection_*)
			{
				shots.data[i].resected = true;
			}
		}
	}

	resection_refresh();
}

// resection of all uncalibrated cameras satisfying lattice test
void tool_resection_all_lattice()
{
	for ALL(shots, i) 
	{
		// check if this shot is uncalibrated and satisfies lattice test
		if (!shots.data[i].calibrated && geometry_lattice_test(i))
		{
			if (action_camera_resection(i, false, false))
			{
				shots.data[i].resected = true;
			}
		}
	}

	resection_refresh();
}

