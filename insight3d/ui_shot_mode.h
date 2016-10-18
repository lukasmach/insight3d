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

#ifndef __UI_SHOT_MODE
#define __UI_SHOT_MODE

#include "interface_sdl.h"
#include "core_structures.h"
#include "core_math_routines.h"
#include "ui_state.h"
#include "ui_selection.h"

// mode switching 
void ui_switch_to_shot_mode();

// process mouse click events in shot mode
void ui_shot_mouse_click();

// respond to selection box in shot mode
void ui_shot_mouse_selection();

// switch current shot (when the user works in shot mode and decides to switch to another image/camera)
bool ui_switch_shot(size_t shot_id);

// process user input (in shot mode) 
void ui_update_shot(const Uint32 delta_time);

#endif
