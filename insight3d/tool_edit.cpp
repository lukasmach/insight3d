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

#include "tool_edit.h"

// tool's state structure 
struct Tool_Edit
{ 
};

static Tool_Edit tool_edit;

void tool_edit_overview_mode()
{
	ui_state.mode = UI_MODE_OVERVIEW; 
}

void tool_edit_inspection_mode()
{
	ui_switch_to_inspection_mode();
}

void tool_edit_shot_mode()
{
	ui_switch_to_shot_mode();
}

void tool_edit_select_points_on_current_shot()
{
	if (INDEX_IS_SET(ui_state.current_shot))
	{
		ui_empty_selection_list();
		ui_select_points_on_shot(ui_state.current_shot);
	}
}

void tool_edit_reconstructed_points()
{
	ui_empty_selection_list();
	ui_select_points_with_reconstructed_vertices();
}

void tool_edit_select_all_points()
{
	ui_empty_selection_list(); 
	ui_select_all_points();
}

// select points corresponding to vertex which has at least one of it's corresponding points selected 
void tool_edit_all_corresponding_points()
{
	for ALL(ui_state.selection_list, i) 
	{
		const Selected_Item * const item = ui_state.selection_list.data + i; 

		if (item->item_type == GEOMETRY_POINT) 
		{
			ASSERT_IS_SET(shots, item->shot_id); 
			ASSERT_IS_SET(shots.data[item->shot_id].points, item->item_id);

			// obtain vertex id 
			const size_t vertex_id = shots.data[item->shot_id].points.data[item->item_id].vertex;
			ASSERT_IS_SET(vertices_incidence, vertex_id);

			// go through all points of this vertex and select the remaining unselected 
			for ALL(vertices_incidence.data[vertex_id].shot_point_ids, j)
			{
				Double_Index * index = vertices_incidence.data[vertex_id].shot_point_ids.data + j; 
				ASSERT_IS_SET(shots, index->primary); 
				ASSERT_IS_SET(shots.data[index->primary].points, index->secondary); 

				// if this point is unselected, select it 
				if (!shots.data[index->primary].points.data[index->secondary].selected) 
				{
					ui_add_point_to_selection(index->primary, index->secondary);
				}
			}
		}
	}
}

void tool_edit_deselect_all()
{
	ui_empty_selection_list();
}

void tool_edit_erase_selected_points()
{
	// copy the indices of points 
	size_t * ids = ALLOC(size_t, 2 * ui_state.selection_list.count);
	size_t count = 0;
	for ALL(ui_state.selection_list, i) 
	{
		Selected_Item * item = ui_state.selection_list.data + i; 
		if (item->item_type == GEOMETRY_POINT)
		{
			ids[2 * count + 0] = item->item_id; 
			ids[2 * count + 1] = item->shot_id;
			count++;
		}
	}

	// prepare for deletition 
	ui_prepare_for_deletition(true, true, true, false, false);

	// delete the points 
	for (size_t i = 0; i < 2 * count; i += 2)
	{
		const size_t vertex_id = shots.data[ids[i + 1]].points.data[ids[i]].vertex;
		geometry_delete_point(ids[i + 1], ids[i]);

		// was this the last point of that vertex? 
		bool last = true;
		for ALL(vertices_incidence.data[vertex_id].shot_point_ids, j) 
		{
			last = false; 
			break; 
		}

		// if it was last, delete the vertex
		if (last) 
		{
			geometry_delete_vertex(vertex_id);
		}
	}

	// release resources 
	FREE(ids);
}

void tool_edit_erase_current_polygon()
{
	if (!INDEX_IS_SET(ui_state.processed_polygon)) return;

	size_t id = ui_state.processed_polygon; 
	ui_prepare_for_deletition(false, false, true, false, false);
	geometry_delete_polygon(id);
}

void tool_edit_erase_all_vertices_and_points()
{
	ui_prepare_for_deletition(true, true, true, false, false);

	// delete all polygons 
	DYN_FREE(polygons);

	// delete all points
	for ALL(shots, i) 
	{
		Shot * const shot = shots.data + i;

		// erase all points
		DYN_FREE(shot->points);
	}

	// delete all vertices
	DYN_FREE(vertices);

	// delete the incidence structures 
	for ALL(vertices_incidence, i) 
	{
		DYN_FREE(vertices_incidence.data[i].shot_point_ids);
	}

	DYN_FREE(vertices_incidence);

	// go through all calibrations and release all triangulated points 
	for ALL(calibrations, i) 
	{
		Calibration * calibration = calibrations.data + i; 
		DYN_FREE(calibration->Xs);
	}

	ui_list_update();
}

void tool_edit_create() 
{
	tool_register_menu_function("Main menu|Edit|Mode...|Overview mode|", tool_edit_overview_mode);
	tool_register_menu_function("Main menu|Edit|Mode...|Inspection mode|", tool_edit_inspection_mode);
	tool_register_menu_function("Main menu|Edit|Mode...|Shot mode|", tool_edit_shot_mode);
	tool_register_menu_function("Main menu|Edit|Select...|Points on current shot|", tool_edit_select_points_on_current_shot);
	tool_register_menu_function("Main menu|Edit|Select...|Reconstructed points|", tool_edit_reconstructed_points);
	tool_register_menu_function("Main menu|Edit|Select...|Add corresponding points|", tool_edit_all_corresponding_points);
	tool_register_menu_function("Main menu|Edit|Select...|All points|", tool_edit_select_all_points);
	tool_register_menu_function("Main menu|Edit|Deselect all|", tool_edit_deselect_all);
	tool_register_menu_function("Main menu|Edit|Erase selected points|", tool_edit_erase_selected_points);
	tool_register_menu_function("Main menu|Edit|Erase current polygon|", tool_edit_erase_current_polygon);
	tool_register_menu_function("Main menu|Edit|Erase...|All vertices and points|", tool_edit_erase_all_vertices_and_points);
}
