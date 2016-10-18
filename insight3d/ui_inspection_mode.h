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
