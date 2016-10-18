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

#include "ui_events.h"

// call visualization routines compatible with current application mode
void ui_event_redraw()
{
	static double angle = 0; // debug
	static int frame_count = 0;
	frame_count++;

	// OpenGL settings 
	ATOMIC(opengl, opengl_push_attribs(); );

	// if (pthread_mutex_trylock(&geometry_mutex) == 0)
	{
		LOCK(tools);

		if (tools_state.progressbar_show == false) 
		{
			UNLOCK(tools);
			LOCK(geometry);
			LOCK(opengl);

			// which mode we're in? 
			switch (ui_state.mode)
			{
				// overview mode
				case UI_MODE_OVERVIEW: 

					// init opengl 
					visualization_prepare_inspection_projection(45);

					// place user camera
					LOCK(opengl)
					{
						angle += 0.7; //min_value(delta_time / 20, 20);
						if (angle >= 720.0) angle -= 720.0;
						glTranslated(0, 0, -3.2);
						glRotated(angle / 2 + 140, 0, 1, 0);
					}
					UNLOCK(opengl);

					// visualize data
					visualization_cameras(shots, 0.5); 
					visualization_vertices(vertices, 0.5);
					visualization_polygons(polygons, 0.5);
					visualization_helper_cube();

				break; 

				// inspection mode
				case UI_MODE_INSPECTION:

					// init opengl
					visualization_prepare_inspection_projection(45);

					// place user camera
					visualization_inspection_user_camera();

					// visualize data
					visualization_vertices(vertices);
					visualization_cameras(shots);
					// visualization_contours(shots, vertices); // specific (more or less)
					visualization_polygons(polygons);

				break;

				// shot mode
				case UI_MODE_SHOT:

					// if there is no current photo, we don't have anything to do
					if (!INDEX_IS_SET(ui_state.current_shot)) break; 
					ASSERT(validate_shot(ui_state.current_shot), "invalid shot set as current");

					// load the image if needed
					// visualization_prepare_image(shots.data[ui_state.current_shot]);

					// request image 
					if (!image_loader_nonempty_handle(shots.data[ui_state.current_shot].image_loader_request))
					{
						shots.data[ui_state.current_shot].image_loader_request = image_loader_new_request(
							ui_state.current_shot, 
							shots.data[ui_state.current_shot].image_filename, 
							IMAGE_LOADER_CONTINUOUS_LOADING
						);
					}

					UNLOCK(opengl)
					{
						image_loader_upload_to_opengl(shots.data[ui_state.current_shot].image_loader_request);
					}
					LOCK(opengl);

					// initialize projection compatible with this view
					visualization_prepare_projection();

					// show image
					visualization_prepare_planar_drawing();
					UNLOCK(opengl)
					{
						visualization_shot_image(shots.data[ui_state.current_shot]);
					}
					LOCK(opengl); 
					visualization_shot_polygons(ui_state.current_shot);

					// if meta info about this image is loaded and display widget has positive size, we can display geometry 
					if (shots.data[ui_state.current_shot].info_status >= GEOMETRY_INFO_DEDUCED && gui_get_width(ui_state.gl) > 0 && gui_get_height(ui_state.gl) > 0)
					{
						// if dualview is enabled and 2 shots are selected, show it
						bool dualview_displayed = false;
						size_t selected[2], selected_count; 
						// selected_count = ui_selected_shots_n(selected, 2);
						// if (INDEX_IS_SET(ui_state.current_shot) 

						// select current shot and the next one 
						selected[0] = ui_state.current_shot; 
						size_t next_shot = (selected[0] + 1) % shots.count;
						while (next_shot != selected[0]) 
						{
							if (IS_SET(shots, next_shot)) 
							{
								selected[1] = next_shot; 
								selected_count = 2;
								break;
							}

							next_shot = (next_shot + 1) % shots.count;
						}

						// printf(next_shot_set ? "true\n" : "false\n");
						
						if (option_show_dualview && selected_count > 0 && (selected[0] != ui_state.current_shot || selected_count == 2))
						{
							if (selected[0] == ui_state.current_shot)
							{
								swap_size_t(selected[0], selected[1]);
							}

							// release previous dualview
							if (INDEX_IS_SET(ui_state.dualview) && ui_state.dualview != selected[0])
							{
								ui_release_dualview();
							}

							// set dualview of new shot and send request for it's image
							if (!INDEX_IS_SET(ui_state.dualview)) 
							{
								INDEX_SET(ui_state.dualview, selected[0]);
								shots.data[selected[0]].image_loader_request = image_loader_new_request(
									selected[0], shots.data[selected[0]].image_filename, IMAGE_LOADER_CONTINUOUS_LOADING
								);
							}
							
							// show it
							image_loader_upload_to_opengl(shots.data[selected[0]].image_loader_request);
							dualview_displayed = true;
							visualization_show_dualview(ui_state.current_shot, selected[0], ui_state.tool_x, ui_state.tool_y);
						}
						else 
						{
							ui_release_dualview();
						}

						// visualization_shot_polygons(ui_state.current_shot);
						if (!dualview_displayed) 
						{
							visualization_shot_points(); // note this function can render only current shot, because other shots have undefined focused point and selected points...
							/*if (INDEX_IS_SET(ui_state.focused_point))
							{
								ui_epipolars_display(ui_state.current_shot, ui_state.focused_point);
							}*/
							if (ui_state.mouse_over) ui_context_display(ui_state.tool_x, ui_state.tool_y);
						}
					}
					else
					{
						ui_release_dualview();
					}

					if (ui_state.mouse_down && !ui_state.mouse_no_dragging && ui_state.mouse_button == SDL_BUTTON_RIGHT)
					{
						ui_selection_box();
					}

					visualization_end_planar_drawing();

					// place the user camera into the same position as current camera
					visualization_shot_user_camera();
					// now we can draw some 3d visualization (if we'd want)
					// visualization_vertices(vertices, 1);
					
				break; 

				// calculation
				case UI_MODE_CALCULATION: 

					// note that this mode probably won't ever be used

				break; 
			}

			// display visualizations common for all modes 
			if (INDEX_IS_SET(ui_state.current_shot))
			{
				if (ui_state.mouse_down && (
						ui_state.mode == UI_MODE_SHOT && ui_state.mouse_button == SDL_BUTTON_RIGHT /*|| 
						ui_state.mode == UI_MODE_INSPECTION && ui_state.mouse_button == SDL_BUTTON_LEFT*/
					)
				)
				{
					// draw selection box 
					visualization_prepare_planar_drawing();
					ui_selection_box();
					visualization_end_planar_drawing();
				}
			}

			UNLOCK(opengl);
			UNLOCK(geometry);
		}
		else
		{
			LOCK(opengl); 

			glBegin(GL_POLYGON);
				glColor3f(0.2, 0.2, 0.2);

				glVertex3f(-0.3, -0.07, 0);
				glVertex3f(0.3, -0.07, 0);
				glVertex3f(0.3, 0.07, 0);
				glVertex3f(-0.3, 0.07, 0);
			glEnd();

			const double border = 0.01;
			const double opacity = 0.1 * sin(frame_count / 50.0);
			glBegin(GL_POLYGON);
				glColor3f(0.4 + opacity, 0.4 + opacity, 0.4 + opacity);
				glVertex3f(-0.3 + border, 0.07 - border, 0);
				glVertex3f(-0.3 + border, -0.07 + border, 0);
				glVertex3f(-0.3 + tools_state.progressbar_percentage * (0.6 - 3 * border) + 2 * border, -0.07 + border, 0);
				glVertex3f(-0.3 + tools_state.progressbar_percentage * (0.6 - 3 * border) + 2 * border, 0.07 - border, 0);
			glEnd();

			UNLOCK(opengl);
			UNLOCK(tools);
		}

		// restore OpenGL settings
		glPopAttrib();

		UNLOCK(opengl);
	}
}

// dispatch mouse button down events
void ui_event_mouse_button_down(Uint8 button, Uint16 x, Uint16 y)
{
	// save information about mouse event
	ui_state.mouse_button = button;
	ui_state.mouse_down_x = x;
	ui_state.mouse_down_y = y;
	ui_state.mouse_x = x;
	ui_state.mouse_y = y; 
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_down_x, ui_state.mouse_down_y, ui_state.tool_down_x, ui_state.tool_down_y);
	ui_state.tool_x = ui_state.tool_down_x; 
	ui_state.tool_y = ui_state.tool_down_y; 
	ui_state.mouse_down = true; 
	ui_state.mouse_down_ticks = SDL_GetTicks();
	ui_state.mouse_dragging_ticks = SDL_GetTicks();

	// if the currently selected tool is applicable in current application mode 
	const size_t current_tool = tools_state.current;
	if (tools_state.tools[current_tool].mode_affinity == ui_state.mode || tools_state.tools[current_tool].mode_affinity == UI_MODE_UNSPECIFIED)
	{
		// invoke mouse down event handler 
		if (
			tools_state.tools[current_tool].mouse_down && 
			(!tools_state.tools[current_tool].process_events || tools_state.tools[current_tool].process_events())
		)
		{
			ui_state.mouse_no_dragging = !tools_state.tools[current_tool].mouse_down(ui_state.tool_down_x, ui_state.tool_down_y, button);
		}
		else
		{
			ui_state.mouse_no_dragging = false;
		}
	}
	else if (ui_state.mode == UI_MODE_INSPECTION)
	{
		// currently there are no tools in inspection mode // todo
		ui_inspection_mouse_button_down();
	}
}

// event translation 
void ui_event_agar_button_down(const GUI_Event_Descriptor event)
{
	// T
	/*AG_Widget * const widget = (AG_Widget *)AG_SELF();
	const int button = AG_INT(1), x = AG_INT(2), y = AG_INT(3);
	const int abs_x = widget->cx + x, abs_y = widget->cy + y; 

	if (AG_WidgetArea(widget, abs_x, abs_y))
	{
		ui_event_mouse_button_down(button, x, y);
	}*/

	const int 
		x = event.sdl_event.button.x, 
		y = event.sdl_event.button.y
	;

	if (inside_2d_interval(
		x, y, 
		ui_state.gl->effective_x1, ui_state.gl->effective_y1, 
		ui_state.gl->effective_x2, ui_state.gl->effective_y2
	))
	{
		ui_event_mouse_button_down(
			event.sdl_event.button.button, 
			x - ui_state.gl->effective_x1, 
			y - ui_state.gl->effective_y1
		);
	}
}

// handle mouse movement
void ui_event_mouse_move(Uint16 x, Uint16 y)
{
	// update mouse position 
	ui_state.mouse_x = x; 
	ui_state.mouse_y = y;
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_x, ui_state.mouse_y, ui_state.tool_x, ui_state.tool_y);

	// if the currently selected tool is applicable in current application mode 
	const size_t current_tool = tools_state.current;
	if (tools_state.tools[current_tool].mode_affinity == ui_state.mode || tools_state.tools[current_tool].mode_affinity == UI_MODE_UNSPECIFIED)
	{
		// decide which event (dragging or mouse move) we want to fire
		if (!ui_state.mouse_down)
		{
			// user only moves the mouse
			if (
				tools_state.tools[current_tool].move && 
				(!tools_state.tools[current_tool].process_events || tools_state.tools[current_tool].process_events())
			)
			{
				tools_state.tools[current_tool].move(ui_state.tool_x, ui_state.tool_y);
			}
		}
		else if (!ui_state.mouse_no_dragging)
		{
			// dragging with current tool 
			if (
				tools_state.tools[current_tool].dragging && 
				(!tools_state.tools[current_tool].process_events || tools_state.tools[current_tool].process_events())
			)
			{
				tools_state.tools[current_tool].dragging(ui_state.tool_x, ui_state.tool_y, ui_state.mouse_button);
			}
		}
		/*else
		{
			// current tool doesn't handle dragging - we could use some default
		}*/
	}
	else if (ui_state.mode == UI_MODE_INSPECTION)
	{
		// currently there are no tools in inspection mode
		ui_inspection_mouse_move();
	}
}

// event translation 
void ui_event_agar_motion(const GUI_Event_Descriptor event)
{
	// T
	/*AG_Widget * const widget = (AG_Widget *)AG_SELF(); 
	const int x = AG_INT(1), y = AG_INT(2);
	const int abs_x = widget->cx + x, abs_y = widget->cy + y; 

	if (AG_WidgetArea(widget, abs_x, abs_y))
	{
		ui_state.mouse_over = true;

		// ensure that the widget is focused
		AG_WindowFocus(AG_WidgetParentWindow(widget));
		AG_WidgetFocus(widget);

		// fire mouse move event 
		ui_event_mouse_move(x, y);
	}
	else
	{
		ui_state.mouse_over = false;
		// unfocus if the mouse is not in the working area
		// AG_WidgetUnfocus(widget);
	}*/

	const int 
		x = event.sdl_event.motion.x, 
		y = event.sdl_event.motion.y
	;

	ui_state.mouse_over = true; 
	ui_event_mouse_move(x - ui_state.gl->effective_x1, y - ui_state.gl->effective_y1);
}

void ui_event_mouse_out(const GUI_Event_Descriptor event)
{
	ui_state.mouse_over = false;
}

// dispatch mouse button up events 
void ui_event_mouse_button_up(Uint8 button, Uint16 x, Uint16 y)
{
	// update mouse position
	ui_state.mouse_x = x;
	ui_state.mouse_y = y;
	ui_convert_xy_from_screen_to_shot(ui_state.mouse_x, ui_state.mouse_y, ui_state.tool_x, ui_state.tool_y);
	const size_t current_tool = tools_state.current;

	if (!ui_state.mouse_down) 
	{
		// mouse button was pressed outside of our window, we don't care about this event 
		return; 
	}

	// decide if it is mouse click or dragging
	if (
		SDL_GetTicks() - ui_state.mouse_down_ticks < UI_CLICK_DELAY &&
		sqr_value(ui_state.mouse_down_x - ui_state.mouse_x) + sqr_value(ui_state.mouse_down_y - ui_state.mouse_y) < UI_CLICK_DISTANCE_SQ
	)
	{
		// * click *

		// if the currently selected tool is applicable in current application mode
		if (tools_state.tools[current_tool].mode_affinity == ui_state.mode || tools_state.tools[current_tool].mode_affinity == UI_MODE_UNSPECIFIED)
		{
			// invoke mouse click event handler 
			if (
				tools_state.tools[current_tool].click && 
				(!tools_state.tools[current_tool].process_events || tools_state.tools[current_tool].process_events())
			)
			{
				tools_state.tools[current_tool].click(ui_state.tool_down_x, ui_state.tool_down_y, button);
			}
		}
		else if (ui_state.mode == UI_MODE_INSPECTION)
		{
			// currently there are no tools in inspection mode // todo
			ui_inspection_mouse_click();
		}
	}
	else
	{
		// * dragging *
		
		// normalize coordinates 
		double 
			x1 = min_value(ui_state.tool_down_x, ui_state.tool_x), 
			y1 = min_value(ui_state.tool_down_y, ui_state.tool_y), 
			x2 = max_value(ui_state.tool_down_x, ui_state.tool_x),
			y2 = max_value(ui_state.tool_down_y, ui_state.tool_y);

		// (surprise surprise...) if the currently selected tool is applicable in current application mode
		if (tools_state.tools[current_tool].mode_affinity == ui_state.mode || tools_state.tools[current_tool].mode_affinity == UI_MODE_UNSPECIFIED)
		{
			// if the tool reacts to mouse dragging
			if (!ui_state.mouse_no_dragging)
			{
				// invoke tool's mouse dragging done event handler 
				if (
					tools_state.tools[current_tool].dragging_done && 
					(!tools_state.tools[current_tool].process_events || tools_state.tools[current_tool].process_events())
				)
				{
					tools_state.tools[current_tool].dragging_done(ui_state.tool_down_x, ui_state.tool_down_y, ui_state.tool_down_x, ui_state.tool_down_y, button);
				}
			}
			/*else
			{
				// otherwise do the default for this application mode 
			}*/
		}
		else if (ui_state.mode == UI_MODE_INSPECTION)
		{
			/*if (button == SDL_BUTTON_LEFT)
			{
				ui_inspection_mouse_selection();
			}*/
		}
	}

	// update ui state
	ui_state.mouse_down = false;
}

// event translation 
void ui_event_agar_button_up(const SDL_Event * const event)
{
	// T
	/*AG_Widget * const widget = (AG_Widget *)AG_SELF(); 
	const int button = AG_INT(1), x = AG_INT(2), y = AG_INT(3);
	const int abs_x = widget->cx + x, abs_y = widget->cy + y; 

	if (AG_WidgetArea(widget, abs_x, abs_y))
	{
		ui_event_mouse_button_up(button, x, y);
	}*/

	const int 
		x = event->button.x, 
		y = event->button.y
	;

	// removed to prevent the mousedown state from handing when 
	// the user presses the mouse down and then moves cursor
	// out of panel's area
	/*if (inside_2d_interval(
		x, y, 
		ui_state.gl->effective_x1, ui_state.gl->effective_y1, 
		ui_state.gl->effective_x2, ui_state.gl->effective_y2
	))
	{*/
		ui_event_mouse_button_up(
			event->button.button,
			x - ui_state.gl->effective_x1, 
			y - ui_state.gl->effective_y1
		);
	//}
}

// process user input
void ui_event_update(const Uint32 delta_time)
{
	// note obsolete code
	// negative time 
	/*if (delta_time <= 0) return; // todo possibly report this - also report when framerate is less then 4 fps

	// act in different application modes accordingly
	switch (ui_state.mode)
	{
		case UI_MODE_OVERVIEW: break;
		case UI_MODE_INSPECTION: ui_update_inspection(delta_time); break; 
		case UI_MODE_SHOT: ui_update_shot(delta_time); break; 
		case UI_MODE_CALCULATION: break; 
		default: ASSERT(false, "unknown ui mode");
	}

	// handle global events (exiting application, saving, ...) 
	ui_update_global(delta_time);

	// clear keyboard state 
	switch (ui_state.mode)
	{
		case UI_MODE_OVERVIEW: sdl_clear_keys(ui_state.keys, ui_state.overview_clear_keys, ui_state.keys_length); break;
		case UI_MODE_INSPECTION: sdl_clear_keys(ui_state.keys, ui_state.inspection_clear_keys, ui_state.keys_length); break; 
		case UI_MODE_SHOT: sdl_clear_keys(ui_state.keys, ui_state.shot_clear_keys, ui_state.keys_length); break; 
		case UI_MODE_CALCULATION: break; // todo 
		default: ASSERT(false, "unknown ui mode");
	}*/

	// also call update routines
	switch (ui_state.mode) 
	{
		case UI_MODE_SHOT: ui_event_update_shot(delta_time); break; 
		case UI_MODE_INSPECTION: ui_update_inspection(delta_time); break;
	}
}

// update routine for shot mode 
void ui_event_update_shot(const Uint32 delta_time)
{
	// first process "key pressed" events
	const size_t current_tool = tools_state.current;
	if (tools_state.tools[current_tool].mode_affinity == UI_MODE_SHOT || tools_state.tools[current_tool].mode_affinity == UI_MODE_UNSPECIFIED)
	{
		// invoke update handler 
		if (
			tools_state.tools[current_tool].key_pressed && 
			(!tools_state.tools[current_tool].process_events || tools_state.tools[current_tool].process_events())
		)
		{
			tools_state.tools[current_tool].key_pressed();
		}
	}

	return; 
}

// resize window
// note this is not optimally done due to some issues with SDL
void ui_event_resize()
{
	image_loader_flush_texture_ids();
 
	if (ui_state.mode == UI_MODE_SHOT && INDEX_IS_SET(ui_state.current_shot))
	{
		visualization_move_into_viewport();
	}
}
