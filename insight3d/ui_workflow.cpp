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

#include "ui_workflow.h"

// cancels processing of a vertex, this usually results in a new vertex being created
void ui_workflow_no_vertex()
{
	ui_state.processed_vertex_stage = 2;
	INDEX_CLEAR(ui_state.processed_vertex);
}

// next vertex to be processed
void ui_workflow_next_vertex()
{
	ASSERT(validate_shot(ui_state.current_shot), "tried to proceed to next vertex on invalid shot");

	// if we're at the end, we can't go to the next vertex 
	if (ui_state.processed_vertex_stage == 2) return;

	// find the next one without correspondence on this image
	size_t vertex_id, point_id, from;
	bool found;

	// we want to go through manually entered vertices first
	if (INDEX_IS_SET(ui_state.processed_vertex))
	{
		from = ui_state.processed_vertex + 1; 
	}
	else
	{
		from = 0;
	}

	GEOMETRY_VERTEX_TYPE vertex_type = ui_state.processed_vertex_stage == 0 ? GEOMETRY_VERTEX_USER : GEOMETRY_VERTEX_AUTO;

	LAMBDA_FIND_FROM(
		vertices, 
		from,
		vertex_id, 
		found, 
		vertices.data[vertex_id].vertex_type == vertex_type
		&& !query_find_point_on_shot_by_vertex_id(ui_state.current_shot, vertex_id, point_id)
	);

	if (found)
	{
		INDEX_SET(ui_state.processed_vertex, vertex_id);
	}
	else
	{
		if (ui_state.processed_vertex_stage == 0) 
		{
			if (!option_hide_automatic)
			{
				ui_state.processed_vertex_stage = 1;
				INDEX_CLEAR(ui_state.processed_vertex);
				ui_workflow_next_vertex();
			}
			else
			{
				ui_workflow_no_vertex();
			}
		}
		else if (ui_state.processed_vertex_stage == 1)
		{
			ui_workflow_no_vertex();
		}
		else
		{
			ASSERT(false, "invalid stage index in workflow module");
		}
	}
}

// next vertex to be processed
void ui_workflow_prev_vertex()
{
	ASSERT(validate_shot(ui_state.current_shot), "tried to proceed to next vertex on invalid shot");

	// find the previous one without correspondence on this image
	size_t vertex_id, point_id, from;
	bool found = false;

	// if we were at the end, start from automatically generated vertices 
	if (ui_state.processed_vertex_stage == 2) 
	{
		ui_state.processed_vertex_stage = !option_hide_automatic ? 1 : 0;

		ASSERT(!INDEX_IS_SET(ui_state.processed_vertex), "processed_vertex index should be cleared whenever stage 2 is reached");
	}

	// we want to go through manually entered vertices first
	bool begin = false;
	if (INDEX_IS_SET(ui_state.processed_vertex))
	{
		if (ui_state.processed_vertex == 0)
		{
			begin = true;
		}
		else
		{
			from = ui_state.processed_vertex - 1; 
		}
	}
	else
	{
		if (vertices.count == 0)
		{
			begin = true;
		}
		else 
		{
			from = vertices.count - 1;
		}
	}

	GEOMETRY_VERTEX_TYPE vertex_type = ui_state.processed_vertex_stage == 0 ? GEOMETRY_VERTEX_USER : GEOMETRY_VERTEX_AUTO;

	if (!begin)
	{
		for (vertex_id = from; true; vertex_id--)
		{
			if (!IS_SET(vertices, vertex_id))
			{
				if (vertex_id == 0) break; else continue;
			}

			if (
				vertices.data[vertex_id].vertex_type == vertex_type 
				&& !query_find_point_on_shot_by_vertex_id(ui_state.current_shot, vertex_id, point_id)
			)
			{
				found = true; 
				break;
			}
			
			if (vertex_id == 0) break; 
		}
	}

	if (found)
	{
		INDEX_SET(ui_state.processed_vertex, vertex_id);
	}
	else
	{
		if (ui_state.processed_vertex_stage == 1)
		{
			ui_state.processed_vertex_stage = 0;
			INDEX_CLEAR(ui_state.processed_vertex);
			ui_workflow_prev_vertex();
		}
		else if (ui_state.processed_vertex_stage == 0)
		{
			if (!INDEX_IS_SET(ui_state.processed_vertex)) 
			{
				ui_workflow_no_vertex();
			}
		}
		else if (ui_state.processed_vertex_stage != 2)
		{
			ASSERT(false, "invalid stage index in workflow module");
		}
	}
}

// first vertex to be processed 
void ui_workflow_first_vertex() 
{
	ASSERT(validate_shot(ui_state.current_shot), "tried to find first unprocessed vertex on invalid shot"); 

	// we're processing manually entered vertices first 
	INDEX_CLEAR(ui_state.processed_vertex);
	ui_state.processed_vertex_stage = 0;

	ui_workflow_next_vertex();
}

// prev polygon to be processed
void ui_workflow_next_polygon()
{
	if (INDEX_IS_SET(ui_state.processed_polygon))
	{
		for (size_t id = ui_state.processed_polygon + 1; id < polygons.count; id++) 
		{
			if (polygons.data[id].set)
			{
				INDEX_SET(ui_state.processed_polygon, id);
				return; 
			}
		}

		INDEX_CLEAR(ui_state.processed_polygon);
	}
}

// next polygon to be processed
void ui_workflow_prev_polygon()
{
	size_t id = INDEX_IS_SET(ui_state.processed_polygon) ? ui_state.processed_polygon : polygons.count;

	while (id != 0) 
	{
		id--;

		if (polygons.data[id].set) 
		{
			INDEX_SET(ui_state.processed_polygon, id);
			return;
		}
	}
}

// sets focused point 
void ui_workflow_set_focused_point(const size_t point_id)
{
	ASSERT(validate_point(ui_state.current_shot, point_id), "focus set to point which cannot be found on current shot"); 
	INDEX_SET(ui_state.focused_point, point_id);
}

// unsets focused point 
void ui_workflow_unset_focused_point()
{
	INDEX_CLEAR(ui_state.focused_point);
}

// switches to another image
void ui_workflow_select_shot(size_t shot_id)
{
	ASSERT(validate_shot(shot_id), "selecting invalid shot");

	// end currently selected tool 
	if (tools_state.tools[tools_state.current].end)
	{
		tools_state.tools[tools_state.current].end();
	}

	// remove request for the image that is being replaced 
	if (INDEX_IS_SET(ui_state.current_shot)) 
	{
		ASSERT_IS_SET(shots, ui_state.current_shot); 
		if (image_loader_nonempty_handle(shots.data[ui_state.current_shot].image_loader_request))
		{
			image_loader_cancel_request(&shots.data[ui_state.current_shot].image_loader_request);
		}
	}

	// update state value 
	INDEX_SET(ui_state.current_shot, shot_id);

	// deselect focused items
	ui_workflow_unset_focused_point();

	// set default zoom and scrolling if necessary
	if (!ui_viewport_set(shot_id)) visualization_fit_to_viewport();

	// update cursor of unprocessed items 
	ui_workflow_first_vertex();

	// begin working with current tool again 
	if (tools_state.tools[tools_state.current].begin)
	{
		tools_state.tools[tools_state.current].begin();
	}
}

// set current shot to default value (if necessary)
// this function is called after any modification to the image sequence (i.e., addition or 
// removal of images)
void ui_workflow_default_shot()
{
	// if current shot was (for example) removed, we clear the value of current_shot
	if (INDEX_IS_SET(ui_state.current_shot) && !validate_shot(ui_state.current_shot))
	{
		INDEX_CLEAR(ui_state.current_shot);
	}

	// scan the sequence of images and try to find a shot without meta info 
	size_t shot_wo_meta_id; 
	bool shot_wo_meta_found; 
	LAMBDA_FIND(shots, shot_wo_meta_id, shot_wo_meta_found, shots.data[shot_wo_meta_id].info_status < GEOMETRY_INFO_DEDUCED);
	if (shot_wo_meta_found) 
	{
		ui_workflow_select_shot(shot_wo_meta_id);
		return;
	}

	// if current shot is not set
	if (!INDEX_IS_SET(ui_state.current_shot))
	{
		// find first valid shot in sequence
		size_t shot;
		bool found;
		LAMBDA_FIND(shots, shot, found, validate_shot(shot));

		if (found) 
		{
			// set it as current shot
			ui_workflow_select_shot(shot);
		}
	}

	// we probably want to ensure that the treeview shows 
	// the list of pictures and the user can see the change 
	// ui_treeview_unfold_pictures();
	// todo remove the above comment 
}
