#include "ui_core.h"

// initialize user interface
bool ui_initialize()
{ 
	// note why aren't we memsetting the whole thing to 0s?

	// if we're on linux, we use gtk for dialogs (as every sane person should do)  
#ifdef LINUX
	gtk_init(NULL, NULL);
#endif

	// set default application mode to "shot mode"
	ui_state.mode = UI_MODE_SHOT; 
	ui_state.previous_mode = UI_MODE_INSPECTION;

	// user interface state initialization
	ui_state.key_state = ALLOC(Uint8, SDLK_LAST);
	memset(ui_state.key_state, 0, sizeof(Uint8) * SDLK_LAST);
	ui_state.keys = SDL_GetKeyState(&ui_state.keys_length);
	ui_state.shot_clear_keys = ALLOC(bool, ui_state.keys_length); 
	ui_state.inspection_clear_keys = ALLOC(bool, ui_state.keys_length); 
	ui_state.overview_clear_keys = ALLOC(bool, ui_state.keys_length);
	ui_state.mouse_x = ui_state.mouse_y = ui_state.mouse_down_x = ui_state.mouse_down_y = 0; 
	ui_state.mouse_down_ticks = 0;
	ui_state.mouse_down = false;

	// todo options
	// todo rewrite

	// shot clear keys 
	memset(ui_state.shot_clear_keys, 0, ui_state.keys_length * sizeof(bool));
	ui_state.shot_clear_keys[SDLK_LEFT] = ui_state.shot_clear_keys[SDLK_RIGHT] = ui_state.shot_clear_keys[SDLK_UP] 
	= ui_state.shot_clear_keys[SDLK_DOWN] = ui_state.shot_clear_keys[SDLK_PAGEDOWN] = ui_state.shot_clear_keys[SDLK_PAGEUP] 
	= ui_state.shot_clear_keys[SDLK_HOME] = ui_state.shot_clear_keys[SDLK_END]
	= true;

	// inspection clear keys 
	memset(ui_state.inspection_clear_keys, 0, ui_state.keys_length * sizeof(bool));
	ui_state.inspection_clear_keys[SDLK_g] = true; 

	// overview clear keys
	memset(ui_state.overview_clear_keys, 0, ui_state.keys_length * sizeof(bool));
	
	// keys cleared in all states 
	ui_state.overview_clear_keys[SDLK_TAB] = ui_state.overview_clear_keys[SDLK_F10] = true; 
	ui_state.shot_clear_keys[SDLK_TAB] = ui_state.shot_clear_keys[SDLK_F10] = true; 
	ui_state.inspection_clear_keys[SDLK_TAB] = ui_state.inspection_clear_keys[SDLK_F10] = true; 
	ui_state.inspection_clear_keys[SDLK_0] = ui_state.shot_clear_keys[SDLK_0] = ui_state.overview_clear_keys[SDLK_0] = true; 
	ui_state.inspection_clear_keys[SDLK_1] = ui_state.shot_clear_keys[SDLK_1] = ui_state.overview_clear_keys[SDLK_1] = true; 
	ui_state.inspection_clear_keys[SDLK_2] = ui_state.shot_clear_keys[SDLK_2] = ui_state.overview_clear_keys[SDLK_2] = true; 
	ui_state.inspection_clear_keys[SDLK_3] = ui_state.shot_clear_keys[SDLK_3] = ui_state.overview_clear_keys[SDLK_3] = true; 
	ui_state.inspection_clear_keys[SDLK_4] = ui_state.shot_clear_keys[SDLK_4] = ui_state.overview_clear_keys[SDLK_4] = true; 
	ui_state.inspection_clear_keys[SDLK_5] = ui_state.shot_clear_keys[SDLK_5] = ui_state.overview_clear_keys[SDLK_5] = true; 

	// create at least five object groups (and the default group 0)
	for (int i = 0; i <= 5; i++) 
	{
		DYN(ui_state.groups, 0);
	}

	// initialize indices 
	INDEX_CLEAR(ui_state.current_shot); 
	INDEX_CLEAR(ui_state.processed_polygon); 
	INDEX_CLEAR(ui_state.processed_vertex); 
	INDEX_CLEAR(ui_state.focused_point);

	// initialize tools // note it's necessary to create at least one tool...
	memset(&tools_state, 0, sizeof(Tools_State));
	tools_state.count = 0;
	tools_state.current = 0; 
	DYN_INIT(tools_state.menu_items);
	tools_state.finalized = false;

	return true;
}

// release allocated memory (called when program terminates)
void ui_release()
{
	FREE(ui_state.shot_clear_keys);
	FREE(ui_state.inspection_clear_keys);
	FREE(ui_state.overview_clear_keys);
}

// create new GUI item structure 
UI_Meta * ui_create_meta(UI_Item_Type type, size_t index)
{
	UI_Meta * meta = ALLOC(UI_Meta, 1);
	if (!meta)
	{
		core_state.error = CORE_ERROR_OUT_OF_MEMORY;
		return NULL;
	}
	memset(meta, 0, sizeof(UI_Meta));
	meta->index = index; 
	meta->type = type;
	return meta; 
}

// create new GUI item structure for section
UI_Section_Meta * ui_check_section_meta(UI_Section_Meta * & meta)
{
	if (!meta) 
	{
		meta = ALLOC(UI_Section_Meta, 1);
		meta->type = UI_ITEM_SECTION;
		meta->unfolded = true;
	}

	return meta;
}

// create new GUI item structure for shot 
UI_Shot_Meta * ui_check_shot_meta(size_t shot_id) 
{
	// check if this shot already has meta structure
	ASSERT(validate_shot(shot_id), "invalid shot supplied when checking for meta structure");

	if (!shots.data[shot_id].ui)
	{
		UI_Shot_Meta * meta = ALLOC(UI_Shot_Meta, 1);
		if (!meta) 
		{
			core_state.error = CORE_ERROR_OUT_OF_MEMORY; 
			return NULL; 
		}
		memset(meta, 0, sizeof(UI_Shot_Meta));
		meta->index = shot_id; 
		meta->type = UI_ITEM_SHOT;
		meta->view_center_x = 0.5; 
		meta->view_center_y = 0.5; 
		meta->view_zoom = -1;
		shots.data[shot_id].ui = (void *)meta;
	}

	return (UI_Shot_Meta *)shots.data[shot_id].ui;
}

// initialize GUI library
bool ui_library_initialization()
{
	const int width = 1024, height = 732; 

	gui_initialize();
	gui_set_size(width, height);
	gui_set_title("insight3d");

	return true; 
}

// OpenGL settings 
// note is this still needed? 
bool ui_opengl_initialization()
{
	LOCK_RW(opengl)
	{
		glShadeModel(GL_SMOOTH);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		// glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	}
	UNLOCK_RW(opengl);

	return true; 
}

// create dialogs for opening and saving files 
bool ui_create_file_dialogs()
{
	// note deprecated
	return true;
}

// create additional dialogs
bool ui_create_dialogs() 
{
	return true;
}

// create main window with glview and friends
bool ui_create_main_window()
{
	// define the user interface
	ui_state.root_panel = gui_get_root_panel();
	ui_state.top = gui_new_panel(ui_state.root_panel, NULL, gui_top_hfill);
	ui_state.side = gui_new_panel(ui_state.root_panel, ui_state.top, gui_below_left_vfill);
	ui_state.gl = gui_new_panel(ui_state.root_panel, ui_state.side, gui_below_on_the_right_fill);
	gui_set_height(ui_state.top, 30);
	gui_set_width(ui_state.side, 250);
	gui_set_style(ui_state.top, gui_style_background); 
	gui_make_menu(ui_state.top);
	gui_make_glview(ui_state.gl, ui_event_redraw);

	// left tab
	ui_state.side_top = gui_new_panel(ui_state.side, NULL, gui_top_hfill);
	ui_state.side_bottom = gui_new_panel(ui_state.side, ui_state.side_top, gui_below_fill);
	gui_set_height(ui_state.side_top, 180);
	gui_set_style(ui_state.side_top, gui_style_background);
	gui_set_style(ui_state.side_bottom, gui_style_background);
	ui_state.side_top_last = gui_new_label(ui_state.side_top, NULL, "Tools you can use:");

	// tabs
	ui_state.tabs = gui_new_tabs(ui_state.side_bottom, NULL, gui_fill);

	// register events 
	GUI_EVENT_HANDLER(ui_state.gl, mousemove) = ui_event_agar_motion; 
	GUI_EVENT_HANDLER(ui_state.gl, mousedown) = ui_event_agar_button_down; 
	// note we're now sending all mousebuttonup events to the our routine, 
	// shouldn't we change the name (remove _agar_)?
	// GUI_EVENT_HANDLER(ui_state.gl, mouseup) = ui_event_agar_button_up;
	GUI_EVENT_HANDLER(ui_state.gl, unfocus) = ui_event_mouse_out;

	return true;
}

// create main application menu
bool ui_create_menu()
{
	tools_state.application_menu = ui_state.top;

	return true;
}

// create tools 
bool ui_register_tools() 
{
	// create root elements for tools 
	tool_register_menu_void("Main menu|"); 

	// menu 
	tool_file_create();
	tool_edit_create();

	// toolbar tools 
	tool_selection_create();
	tool_points_create();
	tool_polygons_create();

	// note move this somewhere
	// add controls to change current image
	ui_state.side_top_last = gui_new_label(ui_state.side_top, ui_state.side_top_last, "Browse pictures:"); 
	ui_state.side_top_last = gui_new_button(ui_state.side_top, ui_state.side_top_last, "Previous", ui_prev_shot);
	ui_state.side_top_last = gui_new_button(ui_state.side_top, ui_state.side_top_last, "Next", ui_next_shot);

	// other tols 
	tool_matching_create();
	tool_calibration_create();
	tool_resection_create();
	tool_triangulation_create();
	tool_coordinates_create();
	tool_image_create();

	// finalize tools creation 
	tool_finalize();

	return true; 
}

// finalize GUI creation 
bool ui_done()
{
	// activate first tool 
	if (tools_state.tools[0].begin) 
	{
		tools_state.tools[0].begin();
	}

	return true;
}

// create gui structures and initialize 
bool ui_create()
{
	bool state =  
		ui_create_main_window() && 
		ui_create_menu() && 
		ui_context_initialize() &&
		ui_register_tools() &&
		ui_done()
	;

	return state;
}

/*// get viewport width and height in shot pixels 
// note unused and not tested
void ui_get_viewport_width_in_shot_coordinates(double & width, double & height) 
{
	double x1, y1, x2, y2; 

	ui_convert_xy_from_screen_to_shot(0, 0, x1, y1); 
	ui_convert_xy_from_screen_to_shot(gui_get_width(ui_state.gl) - 1, gui_get_height(ui_state.gl) - 1, x2, y2); 

	width = x2 - x1; 
	height = y2 - y1;
}*/

// converts screen coordinates to shot coordinate; if no shot is displayed, 
// convert to percentages of screen (aka virtual shot) 
void ui_convert_xy_from_screen_to_shot(Uint16 screen_x, Uint16 screen_y, double & x, double & y)
{
	// check if there's shot being displayed 
	if (INDEX_IS_SET(ui_state.current_shot))
	{
		const double
			shot_ratio = shots.data[ui_state.current_shot].width / (double)shots.data[ui_state.current_shot].height,
			window_ratio = gui_get_width(ui_state.gl) / (double)gui_get_height(ui_state.gl);
		UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
		const double zoom_x = meta->view_zoom * window_ratio / shot_ratio; 
		x = meta->view_center_x + (screen_x / (double)gui_get_width(ui_state.gl) - 0.5) * 2 * zoom_x; 
		y = meta->view_center_y + (screen_y / (double)gui_get_height(ui_state.gl) - 0.5) * 2 * meta->view_zoom;
	}
	else
	{
		x = screen_x / (double)gui_get_width(ui_state.gl);
		y = screen_y / (double)gui_get_height(ui_state.gl);
	}
}

// converts shot coordinates to opengl coordinates
void ui_convert_xy_from_shot_to_opengl(const double shot_x, const double shot_y, double & x, double & y)
{
	x = -1 + 2 * shot_x;
	y = 1 - 2 * shot_y;
}

// check if viewport is set
bool ui_viewport_set(const size_t shot_id) 
{
	const UI_Shot_Meta * const meta = ui_check_shot_meta(shot_id);
	return meta->view_zoom >= 0;
}

// release dualview 
void ui_release_dualview() 
{
	if (INDEX_IS_SET(ui_state.dualview))
	{
		// release request for this image 
		// ASSERT(image_loader_nonempty_handle(shots.data[ui_state.dualview].image_loader_request), "dualview image doesn't have nonempty request even though dualview was shown");
		// note this probably shouldn't be an invariant - for example, the user can turn on and off the dualview so quickly, that no request is sent.
		if (image_loader_nonempty_handle(shots.data[ui_state.dualview].image_loader_request))
		{
			image_loader_cancel_request(&shots.data[ui_state.dualview].image_loader_request);
		}

		INDEX_CLEAR(ui_state.dualview);
	}
}

// clears key state 
void ui_clear_key(int key) 
{
	ui_state.key_state[key] = 0;
}

// prepare user interface for deletition of points, vertices or polygons
void ui_prepare_for_deletition(bool points, bool vertices, bool polygons, bool shots, bool calibrations)
{
	ui_empty_selection_list();
	if (points) { INDEX_CLEAR(ui_state.focused_point); }
	if (vertices) { ui_workflow_no_vertex(); }
	if (polygons) { INDEX_CLEAR(ui_state.processed_polygon); }
	if (shots) { INDEX_CLEAR(ui_state.current_shot); }
	if (calibrations) { INDEX_CLEAR(ui_state.current_calibration); }
}
