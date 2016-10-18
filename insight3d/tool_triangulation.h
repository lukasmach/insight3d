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

#ifndef __TOOL_TRIANGULATION
#define __TOOL_TRIANGULATION

#include "tool_typical_includes.h"
#include "tool_plane_extraction.h"
#include "core_math_routines.h"
#include "actions.h"
#include "mvg_triangulation.h"
// #include "ANN/ANN.h"
#include <utility>
#include <set>

void tool_triangulation_create();
void tool_triangulate_vertices_user();
void tool_triangulate_vertices();
void tool_triangulate_vertices_trusted();
void tool_triangulate_vertices_using_selected_shots();
void tool_triangulate_clear();
// void tool_triangulate_compute_normals();
// void compute_vertex_normal_from_pointcloud(const size_t vertex_id, ANNkd_tree * ann_kdtree, size_t * vertices_reindex);

#endif
