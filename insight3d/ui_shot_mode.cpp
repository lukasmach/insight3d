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

#include "ui_shot_mode.h"

// mode switching 
void ui_switch_to_shot_mode() 
{
	// if current shot isn't set, we'll pick the first one
	// todo switch to the nearest camera (or better to a camera with a similar view, seeing similar set of features, etc)
	if (!INDEX_IS_SET(ui_state.current_shot))
	{
		const size_t first_shot = dyn_first(shots); 
		
		if (dyn_found(first_shot))
		{
			INDEX_SET(ui_state.current_shot, first_shot);
		}
	}

	// finally change the mode 
	ui_state.mode = UI_MODE_SHOT;
}

// process mouse click events in shot mode
void ui_shot_mouse_click()
{
}

// respond to selection box in shot mode
void ui_shot_mouse_selection()
{
	// if no shot is currently displayed, we don't have anything to do 
	if (!validate_shot(ui_state.current_shot)) return;

	// obtain coordinates
	double x1, y1, x2, y2;
	x1 = ui_state.mouse_down_x; 
	y1 = ui_state.mouse_down_y;
	x2 = ui_state.mouse_x; 
	y2 = ui_state.mouse_y; 
	if (x1 > x2) swap_double(x1, x2); 
	if (y1 > y2) swap_double(y1, y2); 

	// selection box modifiers 
	Selection_Type operation = SELECTION_TYPE_REPLACEMENT; 

	if (ui_state.keys[SDLK_LSHIFT] || ui_state.keys[SDLK_RSHIFT]) 
	{
		operation = SELECTION_TYPE_UNION;
	}
	else if (ui_state.keys[SDLK_LCTRL] || ui_state.keys[SDLK_RCTRL]) 
	{
		operation = SELECTION_TYPE_INTERSECTION;
	}

	// empty selection list 
	if (operation == SELECTION_TYPE_REPLACEMENT)
	{
		ui_empty_selection_list(); 
	}

	// perform selection
	#ifdef LANG_CORRECTION
	ui_3d_selection_box(x1, y1, x2, y2, operation);
	#else
	ui_2d_selection_box(x1, y1, x2, y2, operation); 
	#endif 
}

// switch current shot (when the user works in shot mode and decides to switch to another image/camera)
bool ui_switch_shot(size_t shot_id)
{
	// first check this shot actually exists 
	if (!validate_shot(shot_id)) return false;

	// switch to it
	ui_state.current_shot = shot_id;

	return true;
}

// process user input (in shot mode) 
void ui_update_shot(const Uint32 delta_time)
{
	// shift changes behavior of cursor keys (translation vs. rotation)
	if (!ui_state.keys[SDLK_LSHIFT] && !ui_state.keys[SDLK_RSHIFT])
	{
		// * deleting selected points * 
		if (ui_state.keys[SDLK_DELETE])
		{
			// ui_delete_selected_points();
		}

		// * switching shots * 

		// find the shot wanted by the user
		// size_t shot_id = 0; 
		// bool exists = false; 

		// if it exists, switch to it
		// if (exists) ui_switch_shot(shot_id);
	}
}

// displays small thumbnails of areas around currently selected/created vertex 
// note unused 
/*void ui_shot_thumbs()
{
	// * we're going to display helpers only for the first selected point *

	// obtain vertex id 
	size_t vertex_id = 0; 
	bool vertex_found = false;
	
	if (INDEX_IS_SET(ui_state.processed_polygon))
	{
		const size_t polygon_id = ui_state.processed_polygon;

		// todo obsolete code, use LAMBDA_FIND
		for ALL(polygons.data[polygon_id].vertices, i) 
		{
			const size_t polygons_vertex_id = polygons.data[polygon_id].vertices.data[i].value;

			// check if this vertex is localised on this shot
			size_t point_id; 
			if (!query_find_point_on_shot_by_vertex_id(ui_state.current_shot, polygons_vertex_id, point_id))
			{
				// this vertex is not marked on current shot yet
				vertex_found = true; 
				vertex_id = polygons_vertex_id; 
				break; 
			}
		}
	}
	else
	{
		for ALL(ui_state.selection_list, i) 
		{
			if (ui_state.selection_list.data[i].item_type == GEOMETRY_POINT)
			{
				// obtain ids
				size_t point_id = ui_state.selection_list.data[i].item_id; 
				size_t shot_id = ui_state.selection_list.data[i].shot_id;

				// consistency check 
				ASSERT_IS_SET(shots, shot_id); 
				ASSERT_IS_SET(shots.data[shot_id].points, point_id); 
				
				// obtain vertex id
				vertex_id = shots.data[shot_id].points.data[point_id].vertex;
				vertex_found = true;
				break;
			}
		}
	}

	// if no point is selected, we don't have anything to show 
	if (!vertex_found) return;

	// go through all shots on which this vertex was marked 
	size_t j = 0; 
	ASSERT_IS_SET(vertices_incidence, vertex_id); 
	for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i) 
	{
		// for every occurence of this vertex on some shot, obtain shot's id
		const size_t shot_id = vertices_incidence.data[vertex_id].shot_point_ids.data[i].primary; 
		const size_t point_id = vertices_incidence.data[vertex_id].shot_point_ids.data[i].secondary;
		ASSERT_IS_SET(shots, shot_id);
		ASSERT_IS_SET(shots.data[shot_id].points, point_id);
		const Shot * const shot = shots.data + shot_id;

		// we don't want to display thumbnail for current shot, since it's visible anyway 
		if (shot_id == ui_state.current_shot) continue; 

		// we won't display points located (for some strange reason) outside of image plane
		if (!inside_2d_interval(shot->points.data[point_id].x, shot->points.data[point_id].y, 0, 0, 1, 1)) continue; 

		// we won't display points, which aren't credible 
		if (shot->points.data[point_id].data_origin < 0) continue; // note this has to be thought about...
		
		// display thumbnail 
		visualization_shot_thumb(j, true, shot_id, point_id, shot->points.data[point_id].x, shot->points.data[point_id].y, 0.04);

		if (++j >= 11) break;
	}
}*/
