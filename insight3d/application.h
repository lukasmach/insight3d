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

#ifndef __APPLICATION
#define __APPLICATION

#include "geometry_structures.h"
#include "core_image_loader.h"
#include "gui.h"
#include "ui_core.h"
#include "ui_visualization.h"

extern bool mousealreadydown;
extern double delta_time; // time elapsed since last frame rendering\

// initialize debuging (at this point simply prints out some info about 
// application data structures)
bool debug_initialize();

// initialize application subsystems 
bool initialization();

// main loop 
bool main_loop(); 

// deallocate program structures
bool release();

// error reporting routine 
bool report_error();

#endif
