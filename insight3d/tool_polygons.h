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

#ifndef __TOOL_POLYGONS
#define __TOOL_POLYGONS

#include "tool_typical_includes.h"
#include "tool_points.h"

void tool_polygons_create();
void tool_polygons_key();
bool tool_polygons_mouse_down(double x, double y, int button);
void tool_polygons_move(double x, double y);
void tool_polygons_dragging(double x, double y, int button);
void tool_polygons_dragging_done(double x1, double y1, double x2, double y2, int button);
void tool_polygons_click(double x, double y, int button);
void tool_polygons_begin();
void tool_polygons_end();

#endif
