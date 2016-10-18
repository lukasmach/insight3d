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

#include "geometry_publish.h"

// exports data from application structures into structures suitable for OpenCV and MVG
bool publish_triangulation_data(
	const Vertex_Incidence & incidence, size_t vertex_id, const CvMat * * & projection_matrices, CvMat * & points, bool * shots_to_use
)
{
	// vertex and it's incidence must exist
	ASSERT_IS_SET(vertices, vertex_id); 
	ASSERT(incidence.set, "incidence information not processed for this vertex");
	const Vertex * const vertex = vertices.data + vertex_id; 
	
	// count on how many calibrated shots this vertex was marked by the user (credible points only)
	size_t count_points = 0; 
	for ALL(incidence.shot_point_ids, i) 
	{
		// check consistency
		const size_t shot_id = incidence.shot_point_ids.data[i].primary, point_id = incidence.shot_point_ids.data[i].secondary; 
		ASSERT_IS_SET(shots, shot_id); 
		ASSERT_IS_SET(shots.data[shot_id].points, point_id);
		ASSERT(shots.data[shot_id].points.data[point_id].vertex == vertex_id, "inconsistent data in vertex_incidence structure");
		
		// use only calibrated shots and skip unselected
		if (!shots.data[shot_id].calibrated || shots_to_use && !shots_to_use[shot_id]) continue;

		// check if this point is credible
		const Point * const point = shots.data[shot_id].points.data + point_id;
		if (point->data_origin >= 0 && point->relation == GEOMETRY_CORRESPONDENCE)
		{
			count_points++;
		}
	}

	// handle cases when there are no points on the shots we're interested in
	if (count_points == 0)
	{
		projection_matrices = NULL; 
		points = NULL; 
		return false;
	}

	// allocate and initialize data structures 
	projection_matrices = ALLOC(const CvMat *, count_points);
	memset(projection_matrices, 0, count_points * sizeof(CvMat * *)); 
	ATOMIC_RW(opencv, points = opencv_create_matrix(2, count_points); );
	
	// go through all shots with this vertex
	count_points = 0; 
	for ALL(incidence.shot_point_ids, i)
	{
		// check consistency
		const size_t shot_id = incidence.shot_point_ids.data[i].primary, point_id = incidence.shot_point_ids.data[i].secondary; 
		ASSERT_IS_SET(shots, shot_id); 
		ASSERT_IS_SET(shots.data[shot_id].points, point_id);
		ASSERT(shots.data[shot_id].points.data[point_id].vertex == vertex_id, "inconsistent data in vertex_incidence structure");
		if (!shots.data[shot_id].calibrated || shots_to_use && !shots_to_use[shot_id]) continue;
		const Point * const point = shots.data[shot_id].points.data + point_id;

		// use only credible points
		if (point->data_origin < 0 || point->relation != GEOMETRY_CORRESPONDENCE) continue;

		// copy pointer to projection matrix
		projection_matrices[count_points] = (const CvMat *)(shots.data[shot_id].projection);

		// copy vertex coordinates from this shot
		OPENCV_ELEM(points, 0, count_points) = shots.data[shot_id].points.data[point_id].x * shots.data[shot_id].width; 
		OPENCV_ELEM(points, 1, count_points) = shots.data[shot_id].points.data[point_id].y * shots.data[shot_id].height; 

		count_points++;
	}

	return true;
}

// exports data from application structures of given calibration into structures suitable for OpenCV and MVG
bool publish_triangulation_data_from_calibration(
	const size_t calibration_id, const Vertex_Incidence & incidence, size_t vertex_id, 
	const CvMat * * & projection_matrices, CvMat * & points, size_t * & indices
)
{
	// calibration, vertex and it's incidence must exist
	ASSERT_IS_SET(calibrations, calibration_id);
	Calibration * const calibration = calibrations.data + calibration_id;
	ASSERT_IS_SET(vertices, vertex_id); 
	ASSERT(incidence.set, "incidence information not processed for this vertex");
	const Vertex * const vertex = vertices.data + vertex_id; 
	
	// allocate array of indices 
	indices = ALLOC(size_t, 2 * incidence.shot_point_ids.count);
	if (!indices) 
	{
		core_state.error = CORE_ERROR_OUT_OF_MEMORY;
		return false;
	}
	memset(indices, 0, sizeof(size_t) * incidence.shot_point_ids.count);

	// count on how many calibrated shots this vertex was marked by the user
	size_t count_points = 0; 
	for ALL(incidence.shot_point_ids, i) 
	{
		const size_t shot_id = incidence.shot_point_ids.data[i].primary, point_id = incidence.shot_point_ids.data[i].secondary; 

		// consistency check
		ASSERT_IS_SET(shots, shot_id); 
		ASSERT_IS_SET(shots.data[shot_id].points, point_id);
		ASSERT(shots.data[shot_id].points.data[point_id].vertex == vertex_id, "inconsistent data in vertex_incidence structure");

		// try to find this shot among those calibrated 
		size_t P_id;
		bool found;
		LAMBDA_FIND(calibration->Ps, P_id, found, calibration->Ps.data[P_id].shot_id == shot_id);

		if (found) 
		{
			// this point is on calibrated shot - fill in the indices and increase counter 
			indices[2 * count_points + 0] = P_id;
			indices[2 * count_points + 1] = point_id;
			count_points++;
		}
	}

	// handle cases when there are no points on the shots we're interested in
	if (count_points == 0)
	{
		projection_matrices = NULL; 
		points = NULL; 
		FREE(indices);
		indices = NULL;
		return false;
	}

	// allocate and initialize data structures 
	projection_matrices = ALLOC(const CvMat *, count_points); 
	memset(projection_matrices, 0, count_points * sizeof(CvMat * *)); 
	ATOMIC_RW(opencv, points = opencv_create_matrix(2, count_points); );

	// go through all cameras on which this vertex is visible
	for (size_t i = 0; i < count_points; i++) 
	{
		const size_t 
			P_id = indices[2 * i], 
			point_id = indices[2 * i + 1], 
			shot_id = calibration->Ps.data[P_id].shot_id
		;

		// copy pointer to projection matrix
		projection_matrices[i] = (const CvMat *)(calibration->Ps.data[P_id].P);

		// copy vertex coordinates from this shot
		OPENCV_ELEM(points, 0, i) = shots.data[shot_id].points.data[point_id].x * shots.data[shot_id].width; 
		OPENCV_ELEM(points, 1, i) = shots.data[shot_id].points.data[point_id].y * shots.data[shot_id].height; 
	}
	
	return true;
}

// export data for the computation of fundamental matrix
bool publish_2_view_reconstruction_data(
	const size_t shot_id1, const size_t shot_id2, CvMat ** points1, CvMat ** points2, 
	size_t ** points1_indices, size_t ** points2_indices
)
{
	ASSERT(validate_shot(shot_id1) && validate_shot(shot_id2), "invalid shot");
	ASSERT(shots.data[shot_id1].info_status >= GEOMETRY_INFO_DEDUCED, "shot info not loaded");
	ASSERT(shots.data[shot_id2].info_status >= GEOMETRY_INFO_DEDUCED, "shot info not loaded");

	// count how many corresponding points do these two shots have in common 
	size_t correspondences = 0; 

	// go through all points on the first image
	for ALL(shots.data[shot_id1].points, i)
	{
		const size_t vertex_id = shots.data[shot_id1].points.data[i].vertex;
		ASSERT(validate_vertex(vertex_id), "vertex corresponding to point in the first shot is not valid");

		// try to find it on the second image
		for ALL(vertices_incidence.data[vertex_id].shot_point_ids, j)
		{
			const Double_Index * const incidence = vertices_incidence.data[vertex_id].shot_point_ids.data + j; 

			if (incidence->primary == shot_id2) 
			{
				correspondences++;
				break; 
			}
		}
	}

	if (correspondences == 0) 
	{
		return false; 
	}

	*points1_indices = ALLOC(size_t, correspondences);
	*points2_indices = ALLOC(size_t, correspondences);

	LOCK_RW(opencv)
	{
		*points1 = opencv_create_matrix(2, correspondences);
		*points2 = opencv_create_matrix(2, correspondences);
	}
	UNLOCK_RW(opencv);

	// go through all points on the first image
	size_t n = 0;
	for ALL(shots.data[shot_id1].points, i)
	{
		const size_t vertex_id = shots.data[shot_id1].points.data[i].vertex;
		ASSERT(validate_vertex(vertex_id), "vertex corresponding to point in the first shot is not valid");

		// try to find it on the second image
		for ALL(vertices_incidence.data[vertex_id].shot_point_ids, j)
		{
			const Double_Index * const incidence = vertices_incidence.data[vertex_id].shot_point_ids.data + j; 

			if (incidence->primary == shot_id2) 
			{
				// ok, here it is - fill in the coordinates
				ASSERT_IS_SET(shots.data[shot_id2].points, incidence->secondary);

				OPENCV_ELEM(*points1, 0, n) = shots.data[shot_id1].points.data[i].x * shots.data[shot_id1].width;
				OPENCV_ELEM(*points1, 1, n) = shots.data[shot_id1].points.data[i].y * shots.data[shot_id1].height;
				OPENCV_ELEM(*points2, 0, n) = shots.data[shot_id2].points.data[incidence->secondary].x * shots.data[shot_id2].width;
				OPENCV_ELEM(*points2, 1, n) = shots.data[shot_id2].points.data[incidence->secondary].y * shots.data[shot_id2].height;

				(*points1_indices)[n] = i;
				(*points2_indices)[n] = incidence->secondary;

				n++;
				
				break; 
			}
		}
	}

	// consistency check 
	ASSERT(n == correspondences, "inconsistency when filling data structures");

	return true;
}

// export data for resection of given camera, vertices' coordinates are inhomogeneous
// but the coordinates of points are homogeneous vectors
bool publish_resection_data_from_calibration(
	const size_t calibration_id, const size_t shot_id, 
	CvMat ** points, CvMat ** vertices, size_t ** points_indices
) 
{
	ASSERT(validate_shot(shot_id), "trynig to publish reconstruction data of invalid shot"); 
	ASSERT_IS_SET(calibrations, calibration_id);
	Shot * const shot = shots.data + shot_id;
	ASSERT(shot->info_status >= GEOMETRY_INFO_DEDUCED, "shot info not loaded");
	Calibration * const calibration = calibrations.data + calibration_id;
	
	// first count the number of reconstructed vertices marked on this shot 
	size_t count = 0;
	for ALL(shots.data[shot_id].points, i) 
	{
		Point * const point = shots.data[shot_id].points.data + i; 
		const size_t vertex_id = point->vertex;

		// try to find this point among the ones which are reconstructed 
		size_t X_id; 
		bool X_found; 
		LAMBDA_FIND(calibration->Xs, X_id, X_found, calibration->Xs.data[X_id].vertex_id == vertex_id); 

		if (X_found)
		{
			count++;
		}
	}

	// fail if there are no correspondences
	if (count == 0)
	{
		return false;
	}

	// allocate the structures 
	LOCK_RW(opencv)
	{
		*points = opencv_create_matrix(2, count); 
		*vertices = opencv_create_matrix(4, count); 
		*points_indices = ALLOC(size_t, count);

		if (!*points || !*vertices || !*points_indices) 
		{
			if (*points) cvReleaseMat(points); 
			if (*vertices) cvReleaseMat(vertices);
			if (*points_indices) FREE(points_indices);
			UNLOCK_RW(opencv);
			return false;
		}
	}
	UNLOCK_RW(opencv);

	// fill in the data 
	size_t j = 0;
	for ALL(shots.data[shot_id].points, i) 
	{
		Point * const point = shots.data[shot_id].points.data + i; 
		const size_t vertex_id = point->vertex;

		// find this point again
		size_t X_id; 
		bool X_found; 
		LAMBDA_FIND(calibration->Xs, X_id, X_found, calibration->Xs.data[X_id].vertex_id == vertex_id);

		// save the coordinates 
		if (X_found) 
		{
			OPENCV_ELEM(*points, 0, j) = point->x * shot->width; 
			OPENCV_ELEM(*points, 1, j) = point->y * shot->height;

			OPENCV_ELEM(*vertices, 0, j) = OPENCV_ELEM(calibration->Xs.data[X_id].X, 0, 0);
			OPENCV_ELEM(*vertices, 1, j) = OPENCV_ELEM(calibration->Xs.data[X_id].X, 1, 0);
			OPENCV_ELEM(*vertices, 2, j) = OPENCV_ELEM(calibration->Xs.data[X_id].X, 2, 0);
			OPENCV_ELEM(*vertices, 3, j) = OPENCV_ELEM(calibration->Xs.data[X_id].X, 3, 0);

			(*points_indices)[j] = i;

			j++;
		}
	}

	ASSERT(j == count, "counters j and count should be equal");

	return true;
}

// publish polygon 
CvMat * publish_polygon(const size_t shot_id, const size_t polygon_id)
{
	const Polygon_3d * const polygon = polygons.data + polygon_id; 

	// count visible vertices of this polygon
	size_t count = 0;
	for ALL(polygon->vertices, j)
	{
		const size_t vertex_id = polygon->vertices.data[j].value;
		size_t point_id; 

		// check if the vertex is marked on this shot
		if (query_find_point_on_shot_by_vertex_id(shot_id, vertex_id, point_id))
		{
			// get point coordinates
			count++;
		}
	}

	if (count < 3) 
	{
		return NULL;
	}

	// copy them into matrix
	ATOMIC_RW(opencv, CvMat * vertices = opencv_create_matrix(2, count); );
	count = 0;
	for ALL(polygon->vertices, j)
	{
		const size_t vertex_id = polygon->vertices.data[j].value;
		size_t point_id; 

		// check if the vertex is marked on this shot
		if (query_find_point_on_shot_by_vertex_id(shot_id, vertex_id, point_id))
		{
			// get point coordinates
			const double x = geometry_get_point_x(shot_id, point_id);
			const double y = geometry_get_point_y(shot_id, point_id);

			OPENCV_ELEM(vertices, 0, count) = x; 
			OPENCV_ELEM(vertices, 1, count) = y;

			count++;
		}
	}

	return vertices;
}

