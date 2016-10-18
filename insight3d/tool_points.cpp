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

#include "tool_points.h"

// tool's state structure 
struct Tool_Points 
{ 
	bool new_point_created;
	size_t context_vertex;
	bool context_vertex_set;
};

static Tool_Points tool_points;

// create tool 
void tool_points_create() 
{
	tool_create(UI_MODE_SHOT, "Points creator", "Creates new points and moves already existing ones"); 

	tool_set_process_events_function(tool_requires_current_shot);
	tool_set_key_pressed_event_handler(tool_points_key);
	tool_set_mouse_down_handler(tool_points_mouse_down);
	tool_set_click_handler(tool_points_click);
	tool_set_move_handler(tool_points_move);
	tool_set_dragging_handler(tool_points_dragging);
	tool_set_dragging_done_handler(tool_points_dragging_done);
	tool_set_begin_handler(tool_points_begin);
	tool_set_end_handler(tool_points_end);

	tool_register_toolbar_button("Points creator");
}

// refresh lists in UI containing information modified by this tool // todo remove 
void tool_points_refresh_UI()
{
}

// fill the context menu 
void points_fill_context_menu(bool enforce_recretion = false)
{
	// if there is processed vertex, show it's correspondences in the context popup
	size_t vertex_id; 
	bool vertex_id_set = false; 

	if (INDEX_IS_SET(ui_state.focused_point))
	{
		vertex_id = shots.data[ui_state.current_shot].points.data[ui_state.focused_point].vertex; 
		vertex_id_set = true;
	}
	else if (INDEX_IS_SET(ui_state.processed_vertex))
	{
		vertex_id = ui_state.processed_vertex;
		vertex_id_set = true;
	}

	if (vertex_id_set && (!INDEX_IS_SET(tool_points.context_vertex) || tool_points.context_vertex != vertex_id || enforce_recretion))
	{
		ui_context_clear();
		INDEX_SET(tool_points.context_vertex, vertex_id);

		// show detail of the place under cursor
		UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
		if (meta->view_zoom > 0.20) 
		{
			ui_context_add_zoom(350, 350, UI_CONTEXT_CROSSHAIR);
		}

		// if this vertex has some points, fill the context popup
		if (query_count_points_by_vertex(vertex_id) > 0)
		{
			// sort the thumbnails by shot_id
			Vertex_Incidence * const incidence = vertices_incidence.data + vertex_id;
			geometry_sort_double_indices_primary_desc(&incidence->shot_point_ids);

			// fill it with thumbnails
			for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i) 
			{
				Double_Index * const id = vertices_incidence.data[vertex_id].shot_point_ids.data + i; 
				if (id->primary == ui_state.current_shot) continue;

				ui_context_add_thumbnail(id->primary, shots.data[id->primary].points.data[id->secondary].x, shots.data[id->primary].points.data[id->secondary].y, 350, 234, UI_CONTEXT_CROSSHAIR);
			}
		}
	}
	else if (!vertex_id_set) 
	{
		ui_context_clear();
		INDEX_CLEAR(tool_points.context_vertex);

		// show detail of the place under cursor
		UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
		if (meta->view_zoom > 0.20) 
		{
			ui_context_add_zoom(350, 350, UI_CONTEXT_CROSSHAIR);
		}
	}

}

// tool activated
void tool_points_begin() 
{
	ui_context_set_delay(0);
	ui_context_show();
	memset(&tool_points, 0, sizeof(tool_points));

	// points_fill_context_menu();
}

// tool deactivated 
void tool_points_end()
{
	ui_context_hide();
}

// create new point on current image 
bool tool_points_new(const double x, const double y, size_t & point_id)
{
	const size_t shot_id = ui_state.current_shot; 
	bool ui_needs_refresh = false;
	ASSERT(validate_shot(ui_state.current_shot), "invalid shot set as current shot");

	// if there's vertex to be processed, take it; 
	// otherwise create new vertex and set it as being processed
	size_t vertex_id;
	if (INDEX_IS_SET(ui_state.processed_vertex))
	{
		vertex_id = ui_state.processed_vertex;
	}
	else
	{
		ui_needs_refresh = true;
		geometry_new_vertex(vertex_id);
	}
	
	// assign new point to this vertex 
	geometry_new_point(point_id, x, y, shot_id, vertex_id);

	// refresh UI listing points 
	if (ui_needs_refresh) tool_points_refresh_UI();

	// ok, we're happy 
	return true;
}

// user pressed mouse button
bool tool_points_mouse_down(double x, double y, int button)
{
	tool_points.new_point_created = false;

	// there are events we don't care about and want to pass them to other tools
	if (sdl_wheel_button(button) || button == SDL_BUTTON_RIGHT) 
	{
		bool result = tool_selection_mouse_down(x, y, button); 
		if (button == SDL_BUTTON_WHEELUP || button == SDL_BUTTON_WHEELDOWN) points_fill_context_menu(true);
		return result;
	}
	
	// if there is no focused point, create new one
	if (!INDEX_IS_SET(ui_state.focused_point))
	{
		size_t new_point; 
		if (tool_points_new(x, y, new_point))
		{
			ui_workflow_set_focused_point(new_point);
			ui_workflow_next_vertex();

			// save whether user clicked on point or on an empty space
			tool_points.new_point_created = true;
		}
		// {}
	}

	// we'll let the user start dragging
	return true;
}

// user pressed key 
void tool_points_key()
{
	// move to next vertex 
	if (ui_state.key_state[SDLK_RETURN] || ui_state.key_state[SDLK_PAGEDOWN]) 
	{
		ui_workflow_next_vertex();
		points_fill_context_menu();
		ui_clear_key(SDLK_RETURN);
		ui_clear_key(SDLK_PAGEDOWN);
	}

	// user wants to create new vertex 
	if (ui_state.key_state[SDLK_n] || ui_state.key_state[SDLK_END])
	{
		ui_workflow_no_vertex();
		points_fill_context_menu();
		ui_clear_key(SDLK_n);
		ui_clear_key(SDLK_END);
	}

	// move to previous vertex 
	if (ui_state.key_state[SDLK_BACKSPACE] || ui_state.key_state[SDLK_PAGEUP])
	{
		ui_workflow_prev_vertex();
		points_fill_context_menu();
		ui_clear_key(SDLK_BACKSPACE);
		ui_clear_key(SDLK_PAGEUP);
	}

	// move to previous vertex 
	if (ui_state.key_state[SDLK_b] || ui_state.key_state[SDLK_HOME])
	{
		ui_workflow_first_vertex();
		points_fill_context_menu();
		ui_clear_key(SDLK_b);
		ui_clear_key(SDLK_HOME);
	}

	// pass to selection tool
	TOOL_PASS_KEY(selection);
}

// user moves mouse - set focused point 
void tool_points_move(double x, double y)
{
	size_t point_id;
	bool found = ui_selection_get_point_by_position(x, y, point_id);

	if (found)
	{
		ui_workflow_set_focused_point(point_id);
	}
	else if (INDEX_IS_SET(ui_state.focused_point))
	{
		ui_workflow_unset_focused_point();
	}

	points_fill_context_menu();
}

// user can move focused point when dragging (also, focused point will not change until dragging is over)
void tool_points_dragging(double x, double y, int button)
{
	// there are events we don't care about and want to pass them to other tools
	if (sdl_wheel_button(button)) 
	{
		TOOL_PASS_DRAGGING(selection); 
	}

	if (INDEX_IS_SET(ui_state.focused_point))
	{
		ASSERT(validate_shot(ui_state.current_shot), "invalid shot"); 
		ASSERT(validate_point(ui_state.current_shot, ui_state.focused_point), "dragging invalid point"); 

		// change coordinates
		geometry_point_xy(ui_state.current_shot, ui_state.focused_point, x, y);
	}
}

// user finished dragging 
void tool_points_dragging_done(double x1, double y1, double x2, double y2, int button)
{
	TOOL_PASS_DRAGGING_DONE(selection);
}

// mouse click
void tool_points_click(double x, double y, int button) 
{
	if (!tool_points.new_point_created)
	{
		// pass this event to selection tool
		TOOL_PASS_CLICK(selection); 
	}
	else
	{
		ui_empty_selection_list();
	}
}

/*	if (INDEX_IS_SET(ui_state.focused_point))
	{
		if (modifier SHIFT pressed) 
		{
			// if there is focused point under user's cursor, select it
		}
		else if (modifier CTRL pressed)
		{
			// 
		}
	}
	else
	{
		// if there's nothing, create new point 
		// set it as focused and selected 
	}

	// either way, subsequent dragging 
	return true;

	// if there is polygon being processed 
	size_t polygon_id;
	if (INDEX_IS_SET(ui_state.processed_polygon))
	{
		// use it
		ASSERT_IS_SET(polygons, ui_state.processed_polygon);
		polygon_id = ui_state.processed_polygon;
	}
	else 
	{
		// create new 3d polygon
		geometry_new_polygon(polygon_id); // {}
		INDEX_SET(ui_state.processed_polygon, polygon_id);
	}

	Polygon_3d * const polygon = polygons.data + polygon_id;

	// find the first vertex of current polygon that doesn't have corresponding point on this shot
	size_t point_id, vertex_no, vertex_id;
	bool vertex_found;

	LAMBDA_FIND(
		polygon->vertices,
		vertex_no, 
		vertex_found,
		!query_find_point_on_shot_by_vertex_id(shot_id, polygon->vertices.data[vertex_no].value, point_id)
	);

	// if there is one, then use it; otherwise create new vertex
	if (vertex_found)
	{
		vertex_id = polygon->vertices.data[vertex_no].value;
	}
	else
	{
		geometry_new_vertex(vertex_id); // {}
		geometry_polygon_add_vertex(polygon_id, vertex_id);
	}

	// new point values 
	size_t id = 0; 
	double x, y; 
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_x, ui_state.mouse_y, x, y);

	// add 2d point 
	geometry_new_point(id, x, y, ui_state.current_shot, vertex_id);}

	return true;
}*/
