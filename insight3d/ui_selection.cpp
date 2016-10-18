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

#include "ui_selection.h"

// check which shots user selected in GUI 
void ui_get_selection()
{
	bool current_shot_unselected = false; 
	size_t first_selected_shot = 0;
	bool first_selected_shot_set = false; 

	for ALL(shots, i)
	{
		UI_Shot_Meta * const meta = ui_check_shot_meta(i);

		// check if this shot is selected 
		if (/*AG_TableRowSelected(ui_state.list, meta->list_id)*/0 == 1) // T
		{
			meta->selected = true;

			if (!first_selected_shot_set) 
			{
				first_selected_shot = i;
				first_selected_shot_set = true;
			}
		}
		else
		{
			meta->selected = false;
		}

		if (INDEX_IS_SET(ui_state.current_shot) && ui_state.current_shot == i)
		{
			current_shot_unselected = !meta->selected;
		}
	}

	if (current_shot_unselected && first_selected_shot_set) 
	{
		ui_workflow_select_shot(first_selected_shot);
	}
}

// get ids of the first n selected shots
size_t ui_selected_shots_n(size_t * const selected, const size_t count) 
{
	size_t c = 0;

	for ALL(shots, i) 
	{
		const UI_Shot_Meta * const meta = ui_check_shot_meta(i);
		
		if (meta->selected) 
		{
			selected[c++] = i; 
			if (c >= count) break;
		}
	}

	return c;
}

// throw all items out of selection list
void ui_empty_selection_list()
{
	// go through all items in selection list 
	for ALL(ui_state.selection_list, i)
	{
		Selected_Item * const selected_item = ui_state.selection_list.data + i; 

		// which type of item is this 
		switch (selected_item->item_type) 
		{
			case GEOMETRY_POINT: 
				// mark point as not selected
				ASSERT_IS_SET(shots, selected_item->shot_id);
				ASSERT_IS_SET(shots.data[selected_item->shot_id].points, selected_item->item_id);
				shots.data[selected_item->shot_id].points.data[selected_item->item_id].selected = false; 
				break; 
			case GEOMETRY_VERTEX:
				// mark vertex as not selected
				ASSERT_IS_SET(vertices, selected_item->item_id);
				vertices.data[selected_item->item_id].selected = false; 
				break; 
		}
	}

	ui_state.selection_list.count = 0;
}

// add vertex to selection list
bool ui_add_vertex_to_selection(const size_t vertex_id) 
{
	// consistency check 
	ASSERT_IS_SET(vertices, vertex_id);

	// add vertex
	ADD(ui_state.selection_list);
	LAST(ui_state.selection_list).set = true; 
	LAST(ui_state.selection_list).item_id = vertex_id; 
	LAST(ui_state.selection_list).item_type = GEOMETRY_VERTEX;
	vertices.data[vertex_id].selected = true; 
	return true; // todo true only if the addition succeeded 
}

// remove vertex from selection box
void ui_remove_vertex_from_selection(const size_t selection_id)
{
	// consistency check 
	ASSERT_IS_SET(ui_state.selection_list, selection_id);

	// retrieve vertex id
	const size_t vertex_id = ui_state.selection_list.data[selection_id].item_id;
	ASSERT_IS_SET(vertices, vertex_id);

	// remove vertex
	ui_state.selection_list.data[selection_id].set = false; 
	vertices.data[vertex_id].selected = false;
}

// add point to selection box 
bool ui_add_point_to_selection(const size_t shot_id, const size_t point_id) 
{
	// consistency check
	ASSERT_IS_SET(shots, shot_id);
	ASSERT_IS_SET(shots.data[shot_id].points, point_id);

	// if it's already selected, we're done
	if (shots.data[shot_id].points.data[point_id].selected) return true;

	// add point 
	ADD(ui_state.selection_list);
	LAST(ui_state.selection_list).set = true;
	LAST(ui_state.selection_list).shot_id = shot_id;
	LAST(ui_state.selection_list).item_id = point_id;
	LAST(ui_state.selection_list).item_type = GEOMETRY_POINT;
	shots.data[shot_id].points.data[point_id].selected = true;
	return true; // todo true only if the addition succeeded 
}

// remove point from selection 
void ui_remove_point_from_selection(const size_t selection_id) 
{
	// consistency check
	ASSERT_IS_SET(ui_state.selection_list, selection_id); 

	// retrieve ids
	const size_t shot_id = ui_state.selection_list.data[selection_id].shot_id;
	const size_t point_id = ui_state.selection_list.data[selection_id].item_id;
	ASSERT_IS_SET(shots, shot_id);
	ASSERT_IS_SET(shots.data[shot_id].points, point_id);

	// remove point 
	ui_state.selection_list.data[selection_id].set = false;
	shots.data[shot_id].points.data[point_id].selected = false;
}

// select all points on shot 
void ui_select_points_on_shot(const size_t shot_id) 
{
	ASSERT_IS_SET(shots, shot_id);
	const Shot * const shot = shots.data + shot_id;

	// go through all points on this shot 
	for ALL(shot->points, i) 
	{
		// and select them 
		ui_add_point_to_selection(shot_id, i);
	}
}

// select all points (everywhere) 
void ui_select_all_points() 
{
	for ALL(shots, i)
	{
		ui_select_points_on_shot(i);
	}
}

// select points with reconstructed vertices 
void ui_select_points_with_reconstructed_vertices()
{
	if (INDEX_IS_SET(ui_state.current_calibration))
	{
		// go through all vertices reconstructed in this calibration 
		for ALL(calibrations.data[ui_state.current_calibration].Xs, i) 
		{
			Calibration_Vertex * vertex = calibrations.data[ui_state.current_calibration].Xs.data + i; 
			
			// go through all points with this vertex 
			ASSERT(validate_vertex(vertex->vertex_id), "invalid vertex reconstructed"); 
			for ALL(vertices_incidence.data[vertex->vertex_id].shot_point_ids, j) 
			{
				Double_Index * id = vertices_incidence.data[vertex->vertex_id].shot_point_ids.data + j;
				ui_add_point_to_selection(id->primary, id->secondary);
			}
		}
	}
	else
	{
		for ALL(shots, shot_id) 
		{
			const Shot * const shot = shots.data + shot_id; 

			for ALL(shot->points, point_id) 
			{
				const Point * const point = shot->points.data + point_id; 

				ASSERT_IS_SET(vertices, point->vertex); 
				if (vertices.data[point->vertex].reconstructed) 
				{
					ui_add_point_to_selection(shot_id, point_id);
				}
			}
		}
	}
}

// finds point on current shot under position [x,y] (if there is any)
// uses UI_FOCUS_PIXEL_DISTANCE to determine if the point is close enough
bool ui_selection_get_point_by_position(const double x, const double y, size_t & point_id)
{
	ASSERT(validate_shot(ui_state.current_shot), "finding nearest point on invalid shot");
	const size_t shot_id = ui_state.current_shot;
	const Shot * const shot = shots.data + ui_state.current_shot;

	// take the nearest point 
	double distance = query_nearest_point(ui_state.current_shot, x, y, point_id, option_hide_automatic);

	// recalculate distance to screen pixels
	distance = visualization_calc_screen_distance_sq(distance);

	// if it's close enough
	if (inside_interval(distance, 0, UI_FOCUS_PIXEL_DISTANCE_SQ))
	{
		// return it 
		distance = sqrt(distance);
		return true;
	}
	else
	{
		// otherwise no point was found
		point_id = SIZE_MAX;
		return false;
	}
}

// does this vertex belong to an invisible group? 
bool ui_vertex_invisible(size_t vertex_id)
{
	ASSERT_IS_SET(vertices, vertex_id);
	ASSERT_IS_SET(ui_state.groups, vertices.data[vertex_id].group); 
	return ui_state.groups.data[vertices.data[vertex_id].group].hidden;
}

// perform 3d selection of vertices
void ui_3d_selection_box(double x1, double y1, double x2, double y2, Selection_Type operation) 
{
	// what to do with the previous selection 
	if (operation == SELECTION_TYPE_REPLACEMENT) 
	{
		// we're selecting new set of points, deselect all 
		ui_empty_selection_list();
	}
	else if (operation == SELECTION_TYPE_INTERSECTION)
	{
		// if we're selecting subset of current selection, we'll clear 
		// the selection list (without removing 'selected flag' from the items)
		ui_state.selection_list.count = 0;
	}

	// y axis points downwards in opengl
	y1 = gui_get_height(ui_state.gl) - y1; // note corrected from width during the transition to new gui
	y2 = gui_get_height(ui_state.gl) - y2;

	// save transformation matrices
	visualization_export_opengl_matrices();

	// selection is done by 4 planes (forming a pyramid) which are calculated 
	// from 8 vertices (the 9th vertex is used to choose which side is 
	// the "inner" side of the pyramid for each plane) 
	double unprojected[9][3];

	// go through all corners of selection box and reconstruct 2 points in different depth levels
	for (char i = 0; i < 8; i++) 
	{
		gluUnProject(
			i % 2 == 0 ? x1 : x2, i / 2 % 2 == 0 ? y1 : y2, i / 4, 
			visualization_state.opengl_modelview, visualization_state.opengl_projection, visualization_state.opengl_viewport, 
			unprojected[i], unprojected[i] + 1, unprojected[i] + 2
		); 

		// denormalize to obtain 3d position of the corner in vertices' reference plane
		visualization_denormalize_vector(unprojected[i]);
	}

	// also calculate position of some point inside the pyramid 
	gluUnProject(
		average_value(x1, x2), average_value(y1, y2), 0.5,
		visualization_state.opengl_modelview, visualization_state.opengl_projection, visualization_state.opengl_viewport, 
		unprojected[8], unprojected[8] + 1, unprojected[8] + 2
	); 

	// also denormalize
	visualization_denormalize_vector(unprojected[8]);

	// calculate plane coordinates
	double plane[4][4];
	plane_from_three_points(unprojected[0], unprojected[4], unprojected[5], plane[0]);
	plane_from_three_points(unprojected[1], unprojected[5], unprojected[7], plane[1]);
	plane_from_three_points(unprojected[3], unprojected[7], unprojected[6], plane[2]);
	plane_from_three_points(unprojected[2], unprojected[6], unprojected[4], plane[3]);

	// calculate signs of inequity (we want to select the inside part of the pyramid)
	signed char sgns[4]; 
	for (int i = 0; i < 4; i++) 
	{
		sgns[i] = sgn(dot_3(plane[i], unprojected[8]) + plane[i][3]);
	}
	
	// * update selection box *

	if (operation != SELECTION_TYPE_REMOVE)
	{
		// iterate over all vertices and decide which one should be in new selection
		ASSERT_IS_SET(shots, ui_state.current_shot);
		for ALL(vertices, i) 
		{
			Vertex * const vertex = vertices.data + i; 

			// 3d selection box applies only to vertices with reconstructed 3d position
			if (!vertex->reconstructed || ui_vertex_invisible(i))
			{
				// clear 'selected flag'
				if (operation == SELECTION_TYPE_INTERSECTION) 
				{
					vertex->selected = false;
				}

				continue;
			}

			// decide if this point lies in or out of selection pyramid 
			const double vertex_coords[3] = { vertex->x, vertex->y, vertex->z };
			bool inside = true; 
			for (int j = 0; j < 1; j++)
			{
				inside &= sgns[j] == sgn(dot_3(plane[j], vertex_coords) + plane[j][3]);
			}

			// add it (or remove) from new selection
			if (inside) 
			{
				switch (operation) 
				{
					case SELECTION_TYPE_REPLACEMENT: 
						ui_add_vertex_to_selection(i);
						break; 

					case SELECTION_TYPE_UNION: 
						if (!vertex->selected) 
						{
							ui_add_vertex_to_selection(i);
						}
						break; 

					case SELECTION_TYPE_INTERSECTION: 
						if (vertex->selected) 
						{
							ui_add_vertex_to_selection(i);
						}
				}
			}
			else if (operation == SELECTION_TYPE_INTERSECTION)
			{
				// finally remove the 'selected flag' when selecting subset of previous selection 
				vertex->selected = false;
			}
		}
	}
	else 
	{
		// remove vertices inside selection box from current selection (i.e. 'remove operation')
		for ALL(ui_state.selection_list, i)
		{
			const Selected_Item * selected_item = ui_state.selection_list.data + i; 

			// we're only interested in vertices 
			if (selected_item->item_type != GEOMETRY_VERTEX) continue; 

			// consistency check
			ASSERT_IS_SET(vertices, selected_item->item_id);
			const Vertex * vertex = vertices.data + selected_item->item_id;

			// 3d selection box applies only to vertices with reconstructed 3d position 
			if (!vertex->reconstructed || ui_vertex_invisible(selected_item->item_id)) continue; 

			// decide if this point lies in or out of selection pyramid // note small code duplicity (see above)
			const double vertex_coords[3] = { vertex->x, vertex->y, vertex->z };
			bool inside = true; 
			for (int j = 0; j < 4; j++)
			{
				inside &= sgns[j] == sgn(dot_3(plane[j], vertex_coords) + plane[j][3]);
			}

			// if it's inside, remove it 
			if (inside) 
			{
				ui_remove_vertex_from_selection(i);
			}
		}
	}
}

// perform 2d selection of points 
void ui_2d_selection_box(double x1, double y1, double x2, double y2, Selection_Type operation)
{
	// what to do with the previous selection 
	if (operation == SELECTION_TYPE_REPLACEMENT) 
	{
		// we're selecting new set of points, deselect all 
		ui_empty_selection_list();
	}
	else if (operation == SELECTION_TYPE_INTERSECTION)
	{
		// if we're selecting subset of current selection, we'll clear 
		// the selection list (without removing 'selected flag' from the items)
		ui_state.selection_list.count = 0;
	}

	// top left and bottom right corner of box in shot coordinates 
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_down_x, ui_state.mouse_down_y, x1, y1); 
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_x, ui_state.mouse_y, x2, y2); 

	// add points to selection
	// if (operation != SELECTION_TYPE_REMOVE)
	{
		ASSERT_IS_SET(shots, ui_state.current_shot);
		for ALL(shots.data[ui_state.current_shot].points, i) 
		{
			Point * point = shots.data[ui_state.current_shot].points.data + i; 

			// add it (or remove) from new selection
			if (
				inside_2d_interval(
					shots.data[ui_state.current_shot].points.data[i].x, 
					shots.data[ui_state.current_shot].points.data[i].y, 
					x1, y1, x2 ,y2
				) 
				&& (!option_hide_automatic || vertices.data[shots.data[ui_state.current_shot].points.data[i].vertex].vertex_type != GEOMETRY_VERTEX_AUTO)
			)
			{
				switch (operation) 
				{
					case SELECTION_TYPE_REPLACEMENT: 
						ui_add_point_to_selection(ui_state.current_shot, i); 
						break; 

					case SELECTION_TYPE_UNION: 
						if (!point->selected) 
						{
							ui_add_point_to_selection(ui_state.current_shot, i); 
						}
						break; 

					case SELECTION_TYPE_INTERSECTION: 
						if (point->selected) 
						{
							point->selected = false; // just to keep application invariant intact
							ui_add_point_to_selection(ui_state.current_shot, i);
						}
				}
			}
			else if (operation == SELECTION_TYPE_INTERSECTION)
			{
				// finally remove the 'selected flag' when selecting subset of previous selection 
				point->selected = false;
			}
		}
	}
	/* // todo else
	{
		// remove points inside selection box from current selection (i.e. 'remove operation')
		for ALL_SET_ITEMS(visualization_state.selection_list, i)
		{
			const Selected_Item * selected_item = visualization_state.selection_list.data + i; 

			// we're only interested in points 
			if (selected_item->item_type != GEOMETRY_POINT) continue; 

			// consistency check
			ASSERT_IS_SET(shots.data[visualization_state.current_shot].points, selected_item->item_id);
			const Point * point = shots.data[visualization_state.current_shot].points.data + selected_item->item_id;

			// decide if this point lies in or out of selection pyramid // note small code duplicity (see above)
			const double vertex_coords[3] = { vertex->x, vertex->y, vertex->z };
			bool inside = true; 
			for (int i = 0; i < 4; i++)
			{
				inside &= sgns[i] == sgn(dot_3(plane[i], vertex_coords) + plane[i][3]);
			}

			// if it's inside, remove it 
			if (inside) 
			{
				ui_remove_vertex_from_selection(i);
			}
		}
	}*/
}

// delete selected points (only on current shot)
void ui_delete_selected_points(bool dont_restrict_to_current_shot /*= false*/)
{
	for ALL(ui_state.selection_list, i) 
	{
		if (
			(ui_state.selection_list.data[i].item_type == GEOMETRY_POINT) &&
			(dont_restrict_to_current_shot || ui_state.selection_list.data[i].shot_id == ui_state.current_shot)
		)
		{
			geometry_delete_point(ui_state.selection_list.data[i].shot_id, ui_state.selection_list.data[i].item_id);
			ui_state.selection_list.data[i].set = false;
		}
	}
}

// show selection box 
void ui_selection_box()
{
	double x1, y1, x2, y2;
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_down_x, ui_state.mouse_down_y, x1, y1); 
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_x, ui_state.mouse_y, x2, y2);
	visualization_selection_box(x1, y1, x2, y2);
}
