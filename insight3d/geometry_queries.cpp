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

#include "geometry_queries.h"

// determine if all of contour's vertices have been reconstructed
bool query_is_contour_reconstructed(const Contour & contour, const Points & points, const Vertices & vertices) 
{
	for ALL(contour.vertices, i) 
	{
		ASSERT_IS_SET(points, contour.vertices.data[i].value);
		const size_t vertex_id = points.data[contour.vertices.data[i].value].vertex;
		ASSERT_IS_SET(vertices, vertex_id); 

		if (!vertices.data[vertex_id].reconstructed) return false;
	}

	return true;
}

// determine if all polygon's vertices have been reconstructed
bool query_is_polygon_reconstructed(const Polygon_3d & polygon, const Vertices & vertices)
{
	for ALL(polygon.vertices, i)
	{
		const size_t vertex_id = polygon.vertices.data[i].value; 
		ASSERT_IS_SET(vertices, vertex_id); 
		if (!vertices.data[vertex_id].reconstructed) return false;
	}

	return true;
}

// find point corresponding to a vertex on a given shot 
bool query_find_point_on_shot_by_vertex_id(size_t shot_id, size_t vertex_id, size_t & point_id)
{
	// consistency check 
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(vertices, vertex_id); 
	ASSERT_IS_SET(vertices_incidence, vertex_id);

	// go through all points of this vertex
	for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i) 
	{
		const Double_Index * const points_double_index = vertices_incidence.data[vertex_id].shot_point_ids.data + i; 

		// if this point is on our shot  
		if (points_double_index->primary == shot_id) 
		{
			point_id = points_double_index->secondary; 
			return true; 
		}
	}

	// point was never found
	point_id = 0;
	return false; 
}

// find nearest point on shot, returns squared distance in image pixels
double query_nearest_point(const size_t shot_id, const double x, const double y, size_t & point_id, bool skipping_auto /*= false*/)
{
	ASSERT(shots.data[shot_id].width > 0, "image has 0 width");
	ASSERT(shots.data[shot_id].height > 0, "image has 0 height");

	// go through all points on this shot and find the nearest one
	size_t i, best_point_id;
	double best_distance, d;
	bool best_found = false;

	LAMBDA(
		shots.data[shot_id].points, i,

		if (skipping_auto && vertices.data[shots.data[shot_id].points.data[i].vertex].vertex_type == GEOMETRY_VERTEX_AUTO) continue;

		d = distance_sq_2(
			shots.data[shot_id].points.data[i].x * shots.data[shot_id].width, 
			shots.data[shot_id].points.data[i].y * shots.data[shot_id].height, 
			x * shots.data[shot_id].width, 
			y * shots.data[shot_id].height
		);

		if (!best_found || d < best_distance)
		{
			best_distance = d;
			best_found = true;
			best_point_id = i;
		}
	);

	if (best_found)
	{
		point_id = best_point_id;
		return best_distance; 
	}
	else
	{
		point_id = SIZE_MAX;
		return -1.0;
	}
}

// count the number of reconstructed points on this shot 
size_t query_count_reconstructed_points_on_shot(const size_t shot_id) 
{
	ASSERT_IS_SET(shots, shot_id); 
	const Shot * const shot = shots.data + shot_id; 
	size_t count = 0; 

	for ALL(shot->points, i) 
	{
		const size_t vertex_id = shot->points.data[i].vertex;
		ASSERT_IS_SET(vertices, vertex_id);
		const Vertex * const vertex = vertices.data + vertex_id; 
		if (vertex->reconstructed) count++;
	}

	return count;
}

// count the number of vertex's points 
size_t query_count_points_by_vertex(const size_t vertex_id) 
{
	ASSERT_IS_SET(vertices_incidence, vertex_id);

	size_t count = 0; 
	for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i) 
	{
		count++;
	}

	return count;
}
