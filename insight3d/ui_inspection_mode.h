#ifndef __UI_INSPECTION_MODE
#define __UI_INSPECTION_MODE

#include "geometry_structures.h"
#include "ui_visualization.h"
#include "ui_selection.h"
#include "tool_plane_extraction.h"

// mode switching 
void ui_switch_to_inspection_mode();

// initialize "ground" inspection mode
bool ui_inspection_ground_initialization();

// process user input (in inspection mode) 
void ui_update_inspection(const Uint32 delta_time);

// respond to mouse click in inspection mode 
void ui_inspection_mouse_click();

// respond to selection box in inspection mode 
void ui_inspection_mouse_selection();

// handle mouse button down event in inspection mode 
void ui_inspection_mouse_button_down();

// handle mouse movement in inspection mode 
void ui_inspection_mouse_move();

#endif
