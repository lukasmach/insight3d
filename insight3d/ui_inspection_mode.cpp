#include "ui_inspection_mode.h"

// mode switching 
void ui_switch_to_inspection_mode()
{
	if (INDEX_IS_SET(ui_state.current_shot))
	{
		// we were in shot mode, set assign the most recently used shot camera position to user camera position
		visualization_state.T[X] = shots.data[ui_state.current_shot].visualization_T[X];
		visualization_state.T[Y] = shots.data[ui_state.current_shot].visualization_T[Y];
		visualization_state.T[Z] = shots.data[ui_state.current_shot].visualization_T[Z];
		visualization_state.R[X] = shots.data[ui_state.current_shot].R_euler[X];
		visualization_state.R[Y] = shots.data[ui_state.current_shot].R_euler[Y];
		visualization_state.R[Z] = shots.data[ui_state.current_shot].R_euler[Z];
	}

	// finally change the mode
	ui_state.mode = UI_MODE_INSPECTION;
}

// initialize "ground" inspection mode
bool ui_inspection_ground_initialization()
{
	// quit immediately if ground hasn't been estimated
	if (visualization_state.ground == NULL) return false;

	// initialize camera orientation in this mode
	visualization_state.ground_alpha = 0;
	visualization_state.ground_phi = CORE_PI / 4.0;

	// find nearest point on ground to the current camera location 
	nearest_point_on_plane(visualization_state.ground, visualization_state.T, visualization_state.ground_POI);

	// specify reference coordinate frame using three perpendicular vectors 
	double axis_x[3], axis_y[3], axis_z[3], direction[3];
	direction[0] = 1; 
	direction[0] = 1; 
	direction[0] = 1;

	// calculate distance of current camera and point of interest 
	double camera_relative_to_POI[3]; 
	sub_3(visualization_state.T, visualization_state.ground_POI, camera_relative_to_POI); 
	visualization_state.ground_distance = vector_norm_3(camera_relative_to_POI);

	// z axis of ground's reference frame should point towards current camera 
	memcpy(axis_z, visualization_state.ground, 3 * sizeof(double));
	const double projected_distance = dot_3(camera_relative_to_POI, axis_z);
	if (nearly_zero(projected_distance))
	{
		return false;
	}
	else if (projected_distance < 0) 
	{
		mul_3(-1, axis_z, axis_z);
	}

	// re-check that this vector is unit vector
	ASSERT(nearly_zero(vector_norm_3(axis_z) - 1), "inhomogeneous part of vector not normalized");

	// finalize camera direction vector
	normalize_vector(direction, 3);

	// axis x lies on ground plane
	double d = dot_3(axis_z, direction);
	add_mul_3(direction, -d, axis_z, axis_x);
	normalize_vector(axis_x, 3);

	// calculate cross product to obtain last perpendicular vector 
	cross_3(axis_z, axis_x, axis_y);

	// copy result 
	memcpy(visualization_state.ground_axis_x, axis_x, 3 * sizeof(double));
	memcpy(visualization_state.ground_axis_y, axis_y, 3 * sizeof(double));
	memcpy(visualization_state.ground_axis_z, axis_z, 3 * sizeof(double));

	return true;
}

// process user input (in inspection mode) 
void ui_update_inspection(const Uint32 delta_time)
{
	// shift changes behavior of cursor keys (translation vs. rotation)
	const double 
		camera_step = ui_state.inspection_camera_movement_speed * (delta_time / 1000.0),
		camera_rot = ui_state.inspection_camera_rotation_speed * (delta_time / 1000.0);

	// direction of forward motion
	double fx, fy, fz = 1;
	const bool no_revert = false;
	// const bool no_revert = inside_interval(normalize_angle(visualization_state.R[Z]), 0.5 * CORE_PI, 1.5 * CORE_PI);
	// no_revert ? printf("T %f\n", visualization_state.R[Z]) : printf("F %f\n", visualization_state.R[Z]);
	const double rx = visualization_state.R[X], ry = visualization_state.R[Y];
	fy = sin(rx);
	fx = cos(rx) * sin(-ry);
	fz = cos(rx) * cos(-ry);

	// change camera position and/or rotation
	if (no_revert)
	{
		if (ui_state.keys[SDLK_LEFT]) visualization_state.R[Y] += camera_rot;
		if (ui_state.keys[SDLK_RIGHT]) visualization_state.R[Y] -= camera_rot;
	}
	else
	{
		if (ui_state.keys[SDLK_LEFT]) visualization_state.R[Y] -= camera_rot;
		if (ui_state.keys[SDLK_RIGHT]) visualization_state.R[Y] += camera_rot;
	}
	if (!sdl_shift_pressed())
	{
		if (ui_state.key_state[SDLK_UP]) 
		{
			visualization_state.T[X] -= fx; 
			visualization_state.T[Y] -= fy; 
			visualization_state.T[Z] -= fz; 
		}
		if (ui_state.key_state[SDLK_DOWN])
		{
			visualization_state.T[X] += fx; 
			visualization_state.T[Y] += fy; 
			visualization_state.T[Z] += fz; 
		}
	}
	else
	{
		if (ui_state.keys[SDLK_UP]) visualization_state.R[X] -= camera_rot;
		if (ui_state.keys[SDLK_DOWN]) visualization_state.R[X] += camera_rot;
	}

	/*if (!sdl_shift_pressed())
	{
		const double camera_step = ui_state.inspection_camera_movement_speed * (delta_time / 1000.0);

		// direction of forward motion
		double fx, fy, fz = 1;
		const double rx = visualization_state.R[X], ry = visualization_state.R[Y];
		fy = sin(rx); 
		fx = cos(rx) * sin(-ry); 
		fz = cos(rx) * cos(-ry);
	
		// change the user camera position
		if (ui_state.key_state[SDLK_LEFT]) visualization_state.T[X] += camera_step;
		if (ui_state.key_state[SDLK_RIGHT]) visualization_state.T[X] -= camera_step;
		if (ui_state.key_state[SDLK_UP]) visualization_state.T[Y] += camera_step;
		if (ui_state.key_state[SDLK_DOWN]) visualization_state.T[Y] -= camera_step;
		if (ui_state.key_state[SDLK_HOME])
		{
			printf("%f %f %f\n", fx, fy, fz);
			visualization_state.T[X] += fx; 
			visualization_state.T[Y] += fy; 
			visualization_state.T[Z] += fz; 
		}
		if (ui_state.key_state[SDLK_END]) 
		{
			printf("%f %f %f\n", fx, fy, fz);
			visualization_state.T[X] -= fx; 
			visualization_state.T[Y] -= fy; 
			visualization_state.T[Z] -= fz; 
		}
	}
	else
	{
		const double camera_step = ui_state.inspection_camera_rotation_speed * (delta_time / 1000.0);

		if (ui_state.keys[SDLK_LEFT]) visualization_state.R[Y] -= camera_step;
		if (ui_state.keys[SDLK_RIGHT]) visualization_state.R[Y] += camera_step;
		if (ui_state.keys[SDLK_UP]) visualization_state.R[X] -= camera_step;
		if (ui_state.keys[SDLK_DOWN]) visualization_state.R[X] += camera_step;
	}*/

	if (ui_state.key_state[SDLK_g])
	{
		if (!visualization_state.ground_mode) 
		{
			ui_inspection_ground_initialization();
			if (visualization_state.ground) 
			{
				FREE(visualization_state.ground);
			}
			visualization_state.ground = tool_plane_extraction(vertices, 0.5, false, 1);
		}

		// switch to and from ground inspection 
		visualization_state.ground_mode = !visualization_state.ground_mode;

		ui_clear_key(SDLK_g);
	}
}

// respond to mouse click in inspection mode 
void ui_inspection_mouse_click()
{
	// empty selection list 
	if (ui_state.mouse_button == SDL_BUTTON_LEFT)
	{
		ui_empty_selection_list();
	}
}

// respond to selection box in inspection mode 
void ui_inspection_mouse_selection()
{
	if (!INDEX_IS_SET(ui_state.current_shot)) return;

	// obtain coordinates // note currently code duplicity
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
		if (!ui_state.keys[SDLK_LCTRL] && !ui_state.keys[SDLK_RCTRL]) 
		{
			operation = SELECTION_TYPE_UNION;
		}
		else
		{
			operation = SELECTION_TYPE_REMOVE;
		}
	}
	else if (ui_state.keys[SDLK_LCTRL] || ui_state.keys[SDLK_RCTRL]) 
	{
		operation = SELECTION_TYPE_INTERSECTION;
	}

	// perform selection
	ui_3d_selection_box(x1, y1, x2, y2, operation);
}

// handle mouse button down event in inspection mode 
void ui_inspection_mouse_button_down()
{
	if (visualization_state.ground_mode)
	{
		switch (ui_state.mouse_button) 
		{
			case SDL_BUTTON_RIGHT: 
				ui_state.ground_phi_dragging_start = visualization_state.ground_phi; 
				ui_state.ground_alpha_dragging_start = visualization_state.ground_alpha; 
				break; 
			case SDL_BUTTON_WHEELDOWN:
				visualization_state.ground_distance += ui_state.ground_camera_movement_speed;
				break;
			case SDL_BUTTON_WHEELUP:
				visualization_state.ground_distance = max_value(visualization_state.ground_distance - ui_state.ground_camera_movement_speed, CORE_PRECISION);
				break;
			case SDL_BUTTON_MIDDLE:
				memcpy(ui_state.ground_POI_dragging_start, visualization_state.ground_POI, 3 * sizeof(double));
				break;
		}
	}
}

// handle mouse movement in inspection mode
void ui_inspection_mouse_move()
{
	// is mouse being dragged? 
	if (ui_state.mouse_down)
	{
		// in which mode are we? 
		if (visualization_state.ground_mode)
		{
			// ground mode 
			if (ui_state.mouse_button == SDL_BUTTON_RIGHT)
			{
				visualization_state.ground_phi = ui_state.ground_phi_dragging_start + 0.005 * (ui_state.mouse_down_y - ui_state.mouse_y);
				visualization_state.ground_alpha = ui_state.ground_alpha_dragging_start - 0.005 * (ui_state.mouse_down_x - ui_state.mouse_x);
				//if (visualization_state.ground_phi > CORE_PI * 0.49) visualization_state.ground_phi = CORE_PI * 0.49;
				//if (visualization_state.ground_phi < 0) visualization_state.ground_phi = 0;
			}
			else if (ui_state.mouse_button == SDL_BUTTON_MIDDLE)
			{
				double forward[3], sideways[3];
				linear_combination_23(
					visualization_state.ground_axis_x, 
					visualization_state.ground_axis_y,
					cos(visualization_state.ground_alpha), 
					sin(visualization_state.ground_alpha),
					forward
				);
				linear_combination_23(
					visualization_state.ground_axis_x, 
					visualization_state.ground_axis_y,
					cos(visualization_state.ground_alpha - CORE_PI / 2.0), 
					sin(visualization_state.ground_alpha - CORE_PI / 2.0),
					sideways
				);
				add_mul_3(ui_state.ground_POI_dragging_start, -0.05 * (ui_state.mouse_down_x - ui_state.mouse_x), sideways, visualization_state.ground_POI);
				add_mul_3(visualization_state.ground_POI, 0.05 * (ui_state.mouse_down_y - ui_state.mouse_y), forward, visualization_state.ground_POI); 
			}
		}
	}
}
