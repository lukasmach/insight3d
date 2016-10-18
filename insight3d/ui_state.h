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

#ifndef __UI_STATE
#define __UI_STATE

#include "gui.h"
#include "geometry_structures.h"

// application mode 
enum UI_Mode { UI_MODE_OVERVIEW, UI_MODE_INSPECTION, UI_MODE_SHOT, UI_MODE_CALCULATION, UI_MODE_UNSPECIFIED };

// object grouping 
struct Group
{
	bool set;
	bool hidden;    // true if the group is hidden
	char * title;   // user can assign arbitrary name to a group 
	GEOMETRY_ITEM_TYPE restriction;    // group can be restricted to contain only some type of items (vertices, polygons, ...) // unused
};

DYNAMIC_STRUCTURE_DECLARATIONS(Groups, Group);

// user iterface state
struct UI_State {

	// * global UI states *
	UI_Mode mode, previous_mode;

	// * GUI elements * 

	// main application window, it's OpenGL widget and other layout widgets 
	GUI_Panel 
		* window, * pane, * pane_tools, * div_tools, * div_glview, 
		* div_selections, * div_tabs, * vbox_selections, * vbox_tabs, 
		* treeview, * treeview_shots, * treeview_vertices, /* * tabs, */
		* div_toolbar, * toolbar, * list;

	// T gui transition elements
	GUI_Panel
		* root_panel, * top, * side, * gl, * side_top, * side_bottom, 
		* side_top_last, * tabs
	;

	// icons
	SDL_Surface * icons;

	// * SDL controls *

	// keyboard
	Uint8 * keys, * key_state; // pointer to array containing keys' state info (supplied by SDL)
	bool * inspection_clear_keys, * shot_clear_keys, * overview_clear_keys;    // determines which key states should be automatically cleared
	int keys_length; // length of the keys_state array

	// mouse
	Uint8 mouse_button;
	int mouse_x, mouse_y, mouse_down_x, mouse_down_y;
	bool mouse_down;
	Uint32 mouse_down_ticks, mouse_dragging_ticks;
	bool mouse_no_dragging;
	bool mouse_over;

	// mouse coordinates for tools (recalculated into their own frame, with scrolling etc.)
	double tool_x, tool_y, tool_down_x, tool_down_y;

	// dragging 
	double ground_phi_dragging_start, ground_alpha_dragging_start;    // changing camera angle in ground inspection mode
	double ground_phi_dragging_speed, ground_alpha_dragging_speed;    // todo 
	double ground_POI_dragging_start[3];                              // changing camera position in ground inspection mode 

	// * workflow indices - current items and selections * 

	// selection 
	Selected_Items selection_list;

	// currently active items 
	INDEX_DECLARATION(current_shot);

	// items currently being processed
	// for example, polygon is being processed when we're entering points corresponding 
	// with it's vertices; after the user is done with the polygon, processed_polygon
	// index will jump to next polygon (or unset itself)
	INDEX_DECLARATION(processed_vertex); 
	INDEX_DECLARATION(processed_polygon);
	int processed_vertex_stage;

	// focused items
	// these are the items under user's mouse cursor 
	INDEX_DECLARATION(focused_point);

	// displayed calibration 
	INDEX_DECLARATION(current_calibration);

	// * object grouping *
	Groups groups;

	// some special visualizations
	size_t dualview;
	bool dualview_set;

	// * options *
	const double inspection_camera_movement_speed;    // movement speed (in space units per second)
	const double inspection_camera_rotation_speed;    // rotation speed (in radians per second)
	const double ground_camera_movement_speed;

	// defaults 
	UI_State(): 
		inspection_camera_movement_speed(10),
		inspection_camera_rotation_speed(1), 
		ground_camera_movement_speed(10)
	{}

};

// global UI state variable
extern UI_State ui_state;

#endif
