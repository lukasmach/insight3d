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

#ifndef __UI_SELECTION
#define __UI_SELECTION

#include "core_structures.h"
#include "geometry_structures.h"
#include "ui_core.h"

// selection box can perform following set operations
enum Selection_Type { SELECTION_TYPE_REPLACEMENT, SELECTION_TYPE_UNION, SELECTION_TYPE_INTERSECTION, SELECTION_TYPE_REMOVE };

// check which shots user selected in GUI 
void ui_get_selection();

// get ids of the first n selected shots
size_t ui_selected_shots_n(size_t * const selected, const size_t count);

// throw all items out of selection list
void ui_empty_selection_list();

// add vertex to selection list
bool ui_add_vertex_to_selection(const size_t vertex_id);

// remove vertex from selection box
void ui_remove_vertex_from_selection(const size_t selection_id);

// add point to selection box 
bool ui_add_point_to_selection(const size_t shot_id, const size_t point_id);

// remove point from selection 
void ui_remove_point_from_selection(const size_t selection_id);

// select all points on shot 
void ui_select_points_on_shot(const size_t shot_id);

// select all points (everywhere) 
void ui_select_all_points();

// select points with reconstructed vertices 
void ui_select_points_with_reconstructed_vertices();

// finds point on current shot under position [x,y] (if there is any)
// uses UI_FOCUS_PIXEL_DISTANCE to determine if the point is close enough
bool ui_selection_get_point_by_position(const double x, const double y, size_t & point_id);

// does this vertex belong to an invisible group? 
bool ui_vertex_invisible(size_t vertex_id);

// perform 3d selection of vertices
void ui_3d_selection_box(double x1, double y1, double x2, double y2, Selection_Type operation);

// perform 2d selection of points 
void ui_2d_selection_box(double x1, double y1, double x2, double y2, Selection_Type operation);

// delete selected points (only on current shot)
void ui_delete_selected_points(bool dont_restrict_to_current_shot = false);

// show selection box 
void ui_selection_box();

#endif
