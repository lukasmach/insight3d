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

#include "tool_extrude.h"

// do extrusion on the ordered set of selected vertices
bool tool_extrude_to_ground(double * ground, Selected_Items * selected_vertices, size_t group /*= 0*/)
{
	/* 

	Description
	===========

	Go through all selected vertices and for each create shadow point lying 
	on ground plane. Then create polygons forming the extrusion from ground 
	to selected vertices.

	Specification 
	=============

	1) Create dynamic array of indices. 
	2) Go through all vertices in selection list and for each one create new vertex, 
	   store its index in the dynamic array and position it on the closest point on 
	   the ground plane to the original vertex.
	3) Go once again through all vertices and create polygons connecting 
	   neighbouring vertices and their shadow vertices. 
	4) Finally create polygon between the last and the first applicable vertex in the 
	   list of selected vertices.

	Notes
	=====

	We might want to do extrusion on other set's than current selection list. This 
	function should take a list of vertices as a parameter. 

	*/

	// Check that ground plane has been reconstructed, otherwise we don't have anything to do. 
	if (!ground) return false;

	// Create two dynamic arrays of indices, one for storing the set of vertices that are 
	// being extruded and the other to store newly created copies (shadow vertices) of every 
	// one of them. 
	Indices original_vertices, shadow_vertices; 
	DYN_INIT(original_vertices); 
	DYN_INIT(shadow_vertices); 

	// Go through all items in the selection list,
	for ALL(*selected_vertices, i)
	{
		const Selected_Item * selected_item = selected_vertices->data + i; 
		
		// while skipping everything but vertices
		if (selected_item->item_type != GEOMETRY_VERTEX) continue; 

		// that are reconstructed. 
		ASSERT_IS_SET(vertices, selected_item->item_id); 
		if (!vertices.data[selected_item->item_id].reconstructed) continue; 

		// Take each of these vertices 
		const Vertex * vertex = vertices.data + selected_item->item_id; 
		
		// and create a new one - shadow vertex.
		size_t shadow_vertex_id; 
		geometry_new_vertex(shadow_vertex_id); 
		Vertex * const shadow_vertex = vertices.data + shadow_vertex_id;

		// Position it on the closest point on the ground plane to the original vertex.
		double nearest_point[3]; 
		const double temp[3] = { vertex->x, vertex->y, vertex->z }; 
		nearest_point_on_plane(ground, temp, nearest_point); 
		shadow_vertex->x = nearest_point[0]; 
		shadow_vertex->y = nearest_point[1]; 
		shadow_vertex->z = nearest_point[2]; 

		// Also mark it as reconstructed and set it's group. 
		shadow_vertex->reconstructed = true;
		shadow_vertex->group = group;

		// Store index of each vertex in the first dynamic array,
		ADD(original_vertices); 
		LAST(original_vertices).value = selected_item->item_id; 

		// and index of it's 'shadow' counterpart in the other array. 
		ADD(shadow_vertices); 
		LAST(shadow_vertices).value = shadow_vertex_id; 
	}

	// If there were less than 3 vertices, we can't perform the extrusion. 
	if (original_vertices.count < 3) return false; 

	// Go once again through the same set of original vertices.
	for ALL(original_vertices, i)
	{
		// Take each original vertex,
		const Index * original_vertex_index = original_vertices.data + i;

		// the one next to it
		const size_t j = (i + 1) % original_vertices.count;
		const Index * next_original_vertex_index = original_vertices.data + j;

		// and their shadow counterparts on ground. 
		const Index * shadow_vertex_index = shadow_vertices.data + i; 
		const Index * next_shadow_vertex_index = shadow_vertices.data + j; 

		// (Check consistency.)
		ASSERT_IS_SET(shadow_vertices, j); 
		ASSERT_IS_SET(original_vertices, j); 
		ASSERT_IS_SET(shadow_vertices, j); 
		ASSERT_IS_SET(vertices, original_vertex_index->value); 
		ASSERT_IS_SET(vertices, next_original_vertex_index->value); 
		ASSERT_IS_SET(vertices, shadow_vertex_index->value); 
		ASSERT_IS_SET(vertices, next_shadow_vertex_index->value);
		ASSERT(vertices.data[original_vertex_index->value].reconstructed, "only reconstructed vertices should be here"); 
		ASSERT(vertices.data[next_original_vertex_index->value].reconstructed, "only reconstructed vertices should be here"); 
		ASSERT(vertices.data[shadow_vertex_index->value].reconstructed, "only reconstructed vertices should be here"); 
		ASSERT(vertices.data[next_shadow_vertex_index->value].reconstructed, "only reconstructed vertices should be here"); 

		// Finally make a polygon 
		size_t polygon_id; 
		geometry_new_polygon(polygon_id);

		// connecting neighbouring vertices and their shadow vertices.
		geometry_polygon_add_vertex(polygon_id, original_vertex_index->value); 
		geometry_polygon_add_vertex(polygon_id, next_original_vertex_index->value); 
		geometry_polygon_add_vertex(polygon_id, next_shadow_vertex_index->value); 
		geometry_polygon_add_vertex(polygon_id, shadow_vertex_index->value); 
	}

	return true; 
}
