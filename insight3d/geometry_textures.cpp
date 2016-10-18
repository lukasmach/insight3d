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

#include "geometry_textures.h"

// perform texture extraction of all vertices 
void geometry_extract_all_textures()
{
	for ALL(polygons, i) 
	{
		geometry_extract_texture(i);
	}
}

// extract texture for given polygon
void geometry_extract_texture(const size_t polygon_id)
{
	Polygon_3d * const polygon = polygons.data + polygon_id;

	// are there shots with all of it's vertices marked?
	size_t * count = ALLOC(size_t, shots.count);
	memset(count, 0, sizeof(size_t) * shots.count);
	size_t total_vertices = 0;

	for ALL(polygon->vertices, j)
	{
		const size_t vertex_id = polygon->vertices.data[j].value;
		total_vertices++;

		for ALL(vertices_incidence.data[vertex_id].shot_point_ids, k)
		{
			const size_t shot_id = vertices_incidence.data[vertex_id].shot_point_ids.data[k].primary;
			count[shot_id]++;
		}
	}

	// don't extract textures from degenerate polygons
	if (total_vertices < 3) return;

	// go through those shots 
	size_t best_shot; 
	double best_area = 0;
	bool best_shot_set = false;
	for ALL(shots, i) 
	{
		if (count[i] == total_vertices) 
		{
			// todo * check if it's not self-intersecting *
			// ... 

			// * count the area *
			double area = 0; 
			bool not_first = false;
			double prev_x, prev_y;
			for ALL(polygon->vertices, j) 
			{
				const size_t vertex_id = polygon->vertices.data[j].value; 

				// find this vertex on this shot 
				bool found = false; 
				size_t point_iter = 0; 
				LAMBDA_FIND(
					vertices_incidence.data[vertex_id].shot_point_ids, 
					point_iter, 
					found, 
					vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].primary == i
				);

				ASSERT(found, "polygon's vertex not found on shot even though entire polygon should be visible");

				// obtain the values
				const size_t point_id = vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].secondary;
				const double 
					x = shots.data[i].points.data[point_id].x,
					y = shots.data[i].points.data[point_id].y;

				// area of non-intersecting polygon is computed easily
				if (not_first)
				{
					area += prev_x * y - x * prev_y;
				}
				else 
				{
					not_first = true;
				}
				
				prev_x = x;
				prev_y = y;
			}

			// keep track of maximum 
			if (best_area < area || !best_shot_set) 
			{
				best_area = area;
				best_shot = i;
				best_shot_set = true; 
			}
		}
	}

	// extract the texture from best view
	if (best_shot_set) 
	{
		const size_t i = best_shot; 

		ASSERT(count[i] == total_vertices, "the shot chosen for texture extraction doesn't have all polygon's vertices marked");

		// calculate bounding box and texture coordinates 
		double * texture_coords = ALLOC(double, 2 * total_vertices);
		size_t v = 0;
		double min_x = 1000, min_y = 1000, max_x = -1000, max_y = -1000; // todo dirty
		for ALL(polygon->vertices, j)
		{
			const size_t vertex_id = polygon->vertices.data[j].value;

			// find this vertex on this shot 
			bool found = false; 
			size_t point_iter = 0; 
			LAMBDA_FIND(
				vertices_incidence.data[vertex_id].shot_point_ids, 
				point_iter, 
				found, 
				vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].primary == i
			);

			ASSERT(found, "polygon's vertex not found on shot even though entire polygon should be visible");

			// obtain the values
			const size_t point_id = vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].secondary;
			const double 
				x = shots.data[i].points.data[point_id].x,
				y = shots.data[i].points.data[point_id].y;

			// save the texturing coordinates
			texture_coords[2 * v + 0] = x;
			texture_coords[2 * v + 1] = y;

			// update the bounding box
			if (x < min_x) min_x = x; 
			if (x > max_x) max_x = x; 
			if (y < min_y) min_y = y; 
			if (y > max_y) max_y = y;

			v++;
		}

		const double width = max_x - min_x, height = max_y - min_y;

		// eventually free previously allocated data
		if (image_loader_nonempty_handle(polygon->image_loader_request))
		{
			image_loader_cancel_request(&polygon->image_loader_request);
		}

		if (polygon->texture_coords) 
		{
			FREE(polygon->texture_coords);
		}

		// recalculate texture coordinates 
		for (v = 0; v < total_vertices; v++) 
		{
			texture_coords[2 * v + 0] = (texture_coords[2 * v + 0] - min_x) / width;
			texture_coords[2 * v + 1] = (texture_coords[2 * v + 1] - min_y) / height;
		}

		// send request for this image region and save texturing coordinates
		polygon->image_loader_request = image_loader_new_request(i, shots.data[i].image_filename, IMAGE_LOADER_CONTINUOUS_LOADING, IMAGE_LOADER_REGION, min_x, min_y, max_x, max_y);
		polygon->texture_coords = texture_coords;
	}

	FREE(count);
}

