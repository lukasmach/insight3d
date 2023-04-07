#include "tool_polygons.h"

// tool's state structure 
struct Tool_Polygons
{ 
};

static Tool_Polygons tool_polygons;

// create tool 
void tool_polygons_create() 
{
	tool_create(UI_MODE_SHOT, "Polygons creator", "Let's you join vertices into polygons"); 

	tool_set_process_events_function(tool_requires_current_shot);
	tool_set_key_pressed_event_handler(tool_polygons_key);
	tool_set_mouse_down_handler(tool_polygons_mouse_down);
	tool_set_click_handler(tool_polygons_click);
	tool_set_move_handler(tool_polygons_move);
	tool_set_dragging_handler(tool_polygons_dragging);
	tool_set_dragging_done_handler(tool_polygons_dragging_done);
	tool_set_begin_handler(tool_polygons_begin);
	tool_set_end_handler(tool_polygons_end);

	tool_register_toolbar_button("Polygons creator");
}

// tool activated
void tool_polygons_begin() 
{
	memset(&tool_polygons, 0, sizeof(tool_polygons));
}

// tool deactivated 
void tool_polygons_end()
{
}

// user pressed mouse button
bool tool_polygons_mouse_down(double x, double y, int button)
{
	// there are events we don't care about and want to pass them to other tools
	if (sdl_wheel_button(button) || button == SDL_BUTTON_RIGHT) 
	{
		TOOL_PASS_MOUSE_DOWN(selection);
	}

	if (sdl_ctrl_pressed())
	{
		return true;
	}

	// if there is a focused point, add it to the polygon
	if (INDEX_IS_SET(ui_state.focused_point))
	{
		// if no polygon is being processed, create a new one 
		size_t polygon_id;
		if (!INDEX_IS_SET(ui_state.processed_polygon))
		{
			geometry_new_polygon(polygon_id);
			INDEX_SET(ui_state.processed_polygon, polygon_id);
		}
		else
		{
			polygon_id = ui_state.processed_polygon;
		}

		// ok, check if this polygon already contains this vertex 
		const size_t vertex_id = shots.data[ui_state.current_shot].points.data[ui_state.focused_point].vertex; 
		ASSERT_IS_SET(vertices, vertex_id);

		bool found;
		size_t polygon_iter; 
		LAMBDA_FIND(polygons.data[polygon_id].vertices, polygon_iter, found, 
			polygons.data[polygon_id].vertices.data[polygon_iter].value == vertex_id);

		// if it is there and shift is pressed, remove it 
		if (found)
		{
			if (sdl_shift_pressed()) // strong TODO!!! this can't be used here!
			{
				polygons.data[polygon_id].vertices.data[polygon_iter].set = false;
			}
		}
		else
		{
			// otherwise, add it to the polygon
			ADD(polygons.data[polygon_id].vertices); 
			LAST(polygons.data[polygon_id].vertices).value = vertex_id;
		}
	}
	/*else // if there is no focused point, pass the event to points tool 
	{
		TOOL_PASS_MOUSE_DOWN(points);
	}*/

	return true;
}

// user pressed key 
void tool_polygons_key()
{
	if (ui_state.key_state[SDLK_RETURN] || ui_state.key_state[SDLK_PAGEDOWN])
	{
		ui_workflow_next_polygon();
		ui_state.key_state[SDLK_RETURN] = false;
		ui_state.key_state[SDLK_PAGEDOWN] = false;
	}

	if (ui_state.key_state[SDLK_BACKSPACE] || ui_state.key_state[SDLK_PAGEUP])
	{
		ui_workflow_prev_polygon();
		ui_state.key_state[SDLK_BACKSPACE] = false;
		ui_state.key_state[SDLK_PAGEUP] = false;
	}

	TOOL_PASS_KEY(selection);
}

// user moves mouse - set focused point 
void tool_polygons_move(double x, double y)
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
}

// user can move focused point when dragging (also, focused point will not change until dragging is over)
void tool_polygons_dragging(double x, double y, int button)
{
	// there are events we don't care about and want to pass them to other tools
	if (sdl_wheel_button(button)) 
	{
		TOOL_PASS_DRAGGING(selection); 
	}

	if (sdl_ctrl_pressed())
	{
		TOOL_PASS_DRAGGING(points);
	}
}

// user finished dragging 
void tool_polygons_dragging_done(double x1, double y1, double x2, double y2, int button)
{
	if (sdl_ctrl_pressed())
	{
		TOOL_PASS_DRAGGING_DONE(points);
	}

	TOOL_PASS_DRAGGING_DONE(selection);
}

// mouse click
void tool_polygons_click(double x, double y, int button)
{
	/*if (!tool_points.new_point_created)
	{
		// pass this event to selection tool
		TOOL_PASS_CLICK(selection);
	}
	else
	{
		ui_empty_selection_list();
	}*/
}
