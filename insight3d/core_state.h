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

#ifndef __CORE_STATE
#define __CORE_STATE

#include "interface_sdl.h"
#include <ctime>

// possible errors 
enum CORE_ERROR { 
	CORE_NO_ERROR,
	CORE_ERROR_GUI_INITIALIZATION,
	CORE_ERROR_OUT_OF_MEMORY,
	CORE_ERROR_UNABLE_TO_OPEN_FILE, 
	CORE_ERROR_UNABLE_TO_CREATE_THREAD
};

// core application state info
struct Core_State 
{
	// information about OS interaction, timing, etc.
	bool running;
	bool visible;
	bool mouse_focus;
	bool keyboard_focus;
	Uint32 ticks, last_ticks;

	// error management
	CORE_ERROR error;
};

extern Core_State core_state;

// error handling macro 
// todo think about using assertions for this
#define CHECK_ERROR(condition, error_code) if ((condition)) { core_state.error = (error_code); return false; }

// initialize core state
bool core_initialize();

#endif
