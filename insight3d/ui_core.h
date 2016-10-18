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

#ifndef __UI_CORE
#define __UI_CORE

#include "portability.h"
#include "core_structures.h"
#include "geometry_structures.h"
#include "ui_state.h"
#include "ui_events.h"
#include "gui_style.h"

#ifdef LINUX 
#include <gtk/gtk.h>
#endif

// ui core needs to see all tools to trigger their initialization
#include "tool_core.h"
#include "tool_file.h"
#include "tool_edit.h"
#include "tool_points.h"
#include "tool_polygons.h" 
#include "tool_triangulation.h"
#include "tool_resection.h"
#include "tool_matching.h"
#include "tool_calibration.h"
#include "tool_coordinates.h"
#include "tool_image.h"

// GUI items 
enum UI_Item_Type { UI_ITEM_SHOT, UI_ITEM_VERTEX, UI_ITEM_SECTION };

struct UI_Meta
{
	UI_Item_Type type;
	size_t index;
};

struct UI_Shot_Meta 
{
	UI_Item_Type type; 
	size_t index; 
	bool selected;

	// special shot properties 
	double view_center_x, view_center_y, view_zoom;    // zooming and scrolling 

	// GUI properties 
	int list_id;    // id in table displaying the list of all pictures
}; 

struct UI_Section_Meta
{
	UI_Item_Type type; 
	size_t index; 

	// special section properties
	bool unfolded;
};

// initialize user interface
bool ui_initialize();

// release allocated memory (called when program terminates)
void ui_release();

// create new GUI item structure 
UI_Meta * ui_create_meta(UI_Item_Type type, size_t index);

// create new GUI item structure for section
UI_Section_Meta * ui_check_section_meta(UI_Section_Meta * & meta);

// create new GUI item structure for shot 
UI_Shot_Meta * ui_check_shot_meta(size_t shot_id);

// initialize GUI library
bool ui_library_initialization();

// OpenGL settings 
bool ui_opengl_initialization();

// create dialogs for opening and saving files 
bool ui_create_file_dialogs();

// create additional dialogs
bool ui_create_dialogs();

// create main window with glview and friends
bool ui_create_main_window();

// create main application menu
bool ui_create_menu();

// create tools 
bool ui_register_tools();

// finalize GUI creation 
bool ui_done();

// create gui structures and initialize 
bool ui_create();

// get viewport width and height in shot pixels 
// void ui_get_viewport_width_in_shot_coordinates(double & width, double & height);

// converts screen coordinates to shot coordinate; if no shot is displayed, 
// convert to percentages of screen (aka virtual shot) 
void ui_convert_xy_from_screen_to_shot(Uint16 screen_x, Uint16 screen_y, double & x, double & y);

// converts shot coordinates to opengl coordinates
void ui_convert_xy_from_shot_to_opengl(const double shot_x, const double shot_y, double & x, double & y);

// check if viewport is set
bool ui_viewport_set(const size_t shot_id);

// release dualview 
void ui_release_dualview();

// clears key state 
void ui_clear_key(int key);

// prepare user interface for deletition of points, vertices or polygons
void ui_prepare_for_deletition(bool points, bool vertices, bool polygons, bool shots, bool calibrations);

#endif
