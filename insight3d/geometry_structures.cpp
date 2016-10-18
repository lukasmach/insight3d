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

#include "geometry_structures.h"

DYNAMIC_STRUCTURE(Indices, Index);
DYNAMIC_STRUCTURE(Double_Indices, Double_Index);
DYNAMIC_STRUCTURE(Selected_Items, Selected_Item)
DYNAMIC_STRUCTURE(Points, Point); 
DYNAMIC_STRUCTURE(Vertices, Vertex);
DYNAMIC_STRUCTURE(Vertices_Incidence, Vertex_Incidence);
DYNAMIC_STRUCTURE(Polygons_3d, Polygon_3d);
DYNAMIC_STRUCTURE(Contours, Contour);
DYNAMIC_STRUCTURE(Shots, Shot);
DYNAMIC_STRUCTURE(Shot_Pair_Relations, Shot_Pair_Relation);
DYNAMIC_STRUCTURE(Shots_Relations, Shot_Relations);
DYNAMIC_STRUCTURE(Calibration_Points_Meta, Calibration_Point_Meta);
DYNAMIC_STRUCTURE(Calibration_Fundamental_Matrices, Calibration_Fundamental_Matrix);
DYNAMIC_STRUCTURE(Calibration_Cameras, Calibration_Camera);
DYNAMIC_STRUCTURE(Calibration_Vertices, Calibration_Vertex);
DYNAMIC_STRUCTURE(Calibrations, Calibration);

// * allocated instances *

pthread_mutex_t geometry_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; // PTHREAD_MUTEX_INITIALIZER;
Shots shots; // shots
Shots_Relations shots_relations; // relations between groups of shots
Polygons_3d polygons; // 3d polygons 
Vertices vertices; // 3d vertices
Vertices_Incidence vertices_incidence; // incidence
Calibrations calibrations; // calibrations
std::map<int, std::map<int, unsigned int> > detected_edges; // debug

// * initializers *

// initialize scene info 
bool geometry_initialize()
{
	DYN_INIT(shots); 
	DYN_INIT(shots_relations);
	DYN_INIT(polygons); 
	DYN_INIT(vertices);
	DYN_INIT(calibrations);

	return true;
}

// * destructors *

// release shot structure 
// todo recheck this
void geometry_release_shot(Shot * & shot) 
{
	if (image_loader_nonempty_handle(shot->image_loader_request))
	{
		image_loader_cancel_request(&shot->image_loader_request);
	} 

	LOCK_RW(opencv)
	{
		cvReleaseMat(&(shot->projection));
		cvReleaseMat(&(shot->translation));
		cvReleaseMat(&(shot->internal_calibration)); 
		shot = NULL; 
	}
	UNLOCK_RW(opencv);
}

// release all allocated structures holding geometric data
void geometry_release()
{
	for ALL(shots, i) 
	{
		DYN_FREE(shots.data[i].points);
		if (shots.data[i].keypoints) 
		{
			free(shots.data[i].keypoints);
		}

		if (shots.data[i].kd_tree) 
		{ 
			kdtree_release(shots.data[i].kd_tree); 
			shots.data[i].kd_tree = NULL; 
		}
		shots.data[i].keypoints_count = 0;
	}

	for ALL(polygons, i) 
	{
		DYN_FREE(polygons.data[i].vertices);
	}

	for ALL(calibrations, i) 
	{
		if (calibrations.data[i].pi_infinity) cvReleaseMat(&calibrations.data[i].pi_infinity);
		DYN_FREE(calibrations.data[i].Ps);
		DYN_FREE(calibrations.data[i].Xs);
	}

	for ALL(vertices_incidence, i) 
	{
		DYN_FREE(vertices_incidence.data[i].shot_point_ids);
	}

	for ALL(calibrations, i) 
	{
		if (calibrations.data[i].pi_infinity) 
		{ 
			cvReleaseMat(&calibrations.data[i].pi_infinity); 
		}

		DYN_FREE(calibrations.data[i].Xs);
		DYN_FREE(calibrations.data[i].Ps);
	}

	DYN_FREE(shots);
	DYN_FREE(vertices_incidence);
	DYN_FREE(vertices);
	DYN_FREE(polygons);
	DYN_FREE(calibrations);
}

// * data validators * // note validators are probably unused and replaced by ASSERT_IS_SET (are they really?) // note currently I'm rewriting validators in terms of macros

// check if shot is valid 
bool validate_shot(size_t id)
{
	return IS_SET(shots, id);
}

// check if point is valid (allocated properly, etc.)
bool validate_point(size_t shot_id, size_t point_id)
{
	if (!validate_shot(shot_id)) return false;
	return IS_SET(shots.data[shot_id].points, point_id);
}

// check if vertex is valid
bool validate_vertex(size_t vertex_id)
{
	return IS_SET(vertices, vertex_id) && IS_SET(vertices_incidence, vertex_id);
}

// check if polygon is valid 
bool validate_polygon(size_t polygon_id)
{
	return IS_SET(polygons, polygon_id);
}

// * sorting * 

int geometry_sort_double_indices_primary_comparator(const void * pi, const void * pj) 
{
	const Double_Index * i = (Double_Index *)pi, * j = (Double_Index *)pj;

	if (!i->set && !j->set) return 0; 
	if (!i->set) return 1; 
	if (!j->set) return -1;

	if (i->primary < j->primary) return -1; 
	else if (i->primary > j->primary) return 1;
	else return 0;
}

void geometry_sort_double_indices_primary(Double_Indices * indices) 
{
	qsort(indices->data, indices->count, sizeof(Double_Index), geometry_sort_double_indices_primary_comparator);
}

int geometry_sort_double_indices_secondary_comparator(const void * pi, const void * pj) 
{
	const Double_Index * i = (Double_Index *)pi, * j = (Double_Index *)pj;

	if (!i->set && !j->set) return 0; 
	if (!i->set) return 1; 
	if (!j->set) return -1;

	if (i->secondary < j->secondary) return -1; 
	else if (i->secondary > j->secondary) return 1;
	else return 0;
}

void geometry_sort_double_indices_secondary(Double_Indices * indices) 
{
	qsort(indices->data, indices->count, sizeof(Double_Index), geometry_sort_double_indices_secondary_comparator);
}

int geometry_sort_double_indices_primary_comparator_desc(const void * pi, const void * pj) 
{
	const Double_Index * i = (Double_Index *)pi, * j = (Double_Index *)pj;

	if (!i->set && !j->set) return 0; 
	if (!i->set) return -1; 
	if (!j->set) return 1;

	if (i->primary < j->primary) return 1; 
	else if (i->primary > j->primary) return -1;
	else return 0;
}

void geometry_sort_double_indices_primary_desc(Double_Indices * indices) 
{
	qsort(indices->data, indices->count, sizeof(Double_Index), geometry_sort_double_indices_primary_comparator_desc);
}

int geometry_sort_double_indices_secondary_comparator_desc(const void * pi, const void * pj) 
{
	const Double_Index * i = (Double_Index *)pi, * j = (Double_Index *)pj;

	if (!i->set && !j->set) return 0; 
	if (!i->set) return -1; 
	if (!j->set) return 1;

	if (i->secondary < j->secondary) return 1; 
	else if (i->secondary > j->secondary) return -1;
	else return 0;
}

void geometry_sort_double_indices_secondary_desc(Double_Indices * indices) 
{
	qsort(indices->data, indices->count, sizeof(Double_Index), geometry_sort_double_indices_secondary_comparator);
}

// * deleting * 

// delete point
void geometry_delete_point(size_t shot_id, size_t point_id) // todo remove it from contours
{
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(shots.data[shot_id].points, point_id);

	// remove from incidence structure
	size_t vertex_id = shots.data[shot_id].points.data[point_id].vertex;
	ASSERT_IS_SET(vertices_incidence, vertex_id);
	for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i) 
	{
		Double_Index * index = vertices_incidence.data[vertex_id].shot_point_ids.data + i; 

		if (index->primary == shot_id && index->secondary == point_id)
		{
			index->set = false;
		}
	}

	shots.data[shot_id].points.data[point_id].set = false;
}

// delete polygon
void geometry_delete_polygon(const size_t polygon_id) 
{
	ASSERT_IS_SET(polygons, polygon_id);
	polygons.data[polygon_id].set = false; 
}

// delete vertex (and all it's points, incidence structure, ...) 
void geometry_delete_vertex(const size_t vertex_id)
{
	ASSERT_IS_SET(vertices, vertex_id);
	ASSERT_IS_SET(vertices_incidence, vertex_id);

	for ALL(polygons, polygon_id) 
	{
		// try to find the vertex on this polygon
		size_t id;
		bool found;
		LAMBDA_FIND(polygons.data[polygon_id].vertices, id, found, polygons.data[polygon_id].vertices.data[id].value == vertex_id);

		// if it is there, delete it 
		if (found) 
		{
			polygons.data[polygon_id].vertices.data[id].set = false;

			// is the polygon now degenerated?
			size_t count = 0;
			for ALL(polygons.data[polygon_id].vertices, i) 
			{
				count++;
				if (count >= 2) break;
			}

			if (count < 2)
			{
				// delete the polygon
				geometry_delete_polygon(polygon_id);
			}
		}
	}

	for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i)
	{
		const Double_Index * index = vertices_incidence.data[vertex_id].shot_point_ids.data + i;
		ASSERT_IS_SET(shots, index->primary);
		ASSERT_IS_SET(shots.data[index->primary].points, index->secondary);
		shots.data[index->primary].points.data[index->secondary].set = false;
	}

	DYN_FREE(vertices_incidence.data[vertex_id].shot_point_ids);
	vertices.data[vertex_id].set = false;
	vertices_incidence.data[vertex_id].set = false;
}

// * accessors and modifiers *

// add 2d point vertex incidence
void geometry_point_vertex_incidence(size_t shot_id, size_t point_id, size_t vertex_id)
{
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(shots.data[shot_id].points, point_id);

	// forward incidence
	shots.data[shot_id].points.data[point_id].vertex = vertex_id; 

	// backward incidence
	DYN(vertices_incidence, vertex_id);
	ADD(vertices_incidence.data[vertex_id].shot_point_ids); 
	LAST(vertices_incidence.data[vertex_id].shot_point_ids).primary = shot_id; 
	LAST(vertices_incidence.data[vertex_id].shot_point_ids).secondary = point_id;
}

// get 2d point x coordinate 
double geometry_get_point_x(size_t shot_id, size_t point_id)
{
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(shots.data[shot_id].points, point_id); 

	return shots.data[shot_id].points.data[point_id].x;
}

// get 2d point y coordinate 
double geometry_get_point_y(size_t shot_id, size_t point_id)
{
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(shots.data[shot_id].points, point_id);

	return shots.data[shot_id].points.data[point_id].y;
}

// modify 2d point 
void geometry_point_xy(size_t shot_id, size_t point_id, double x, double y)
{
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(shots.data[shot_id].points, point_id);

	shots.data[shot_id].points.data[point_id].x = x; 
	shots.data[shot_id].points.data[point_id].y = y; 
}

// * initialization of new structures *

// initialize new shot
bool geometry_new_shot(size_t & shot)
{
	shot = shots.count;
	DYN(shots, shot); // {}

	return true;
}

// initialize containers for camera calibration 
bool geometry_shot_new_calibration_containers(const size_t shot_id)
{
	ASSERT(validate_shot(shot_id), "trying to allocate containers for calibration data for invalid shot"); 
	ASSERT(!shots.data[shot_id].projection, "projection matrix not deallocated when trying to allocate new one"); 
	ASSERT(!shots.data[shot_id].rotation, "rotation matrix not deallocated when trying to allocate new one"); 
	ASSERT(!shots.data[shot_id].translation, "rotation matrix not deallocated when trying to allocate new one"); 
	ASSERT(!shots.data[shot_id].internal_calibration, "rotation matrix not deallocated when trying to allocate new one"); 

	shots.data[shot_id].projection = opencv_create_matrix(3, 4);
	shots.data[shot_id].rotation = opencv_create_matrix(3, 3); 
	shots.data[shot_id].internal_calibration = opencv_create_matrix(3, 3); 
	shots.data[shot_id].translation = opencv_create_matrix(3, 1); 

	return true;
}

// create new 3d vertex 
bool geometry_new_vertex(size_t & id) 
{
	// create new vertex
	id = vertices.count;
	DYN(vertices, id); // {} 
	DYN(vertices_incidence, id); // {}

	return true;
}

// create new 2d point 
bool geometry_new_point(size_t & point_id, double x, double y, size_t shot_id, size_t vertex_id)
{
	ASSERT_IS_SET(shots, shot_id); 
	ASSERT_IS_SET(vertices, vertex_id);

	// create new point
	point_id = shots.data[shot_id].points.count;
	DYN(shots.data[shot_id].points, point_id); // {}

	// set data
	geometry_point_xy(shot_id, point_id, x, y);
	geometry_point_vertex_incidence(shot_id, point_id, vertex_id);

	return true; 
}

// create new polygon 
bool geometry_new_polygon(size_t & id)
{
	// create new polygon 
	id = polygons.count; 
	DYN(polygons, id); // {}

	return true;
}

// * modifying polygons *

// add vertex to polygon 
bool geometry_polygon_add_vertex(size_t polygon_id, size_t vertex_index) 
{
	ASSERT_IS_SET(polygons, polygon_id);

	// allocate memory // note that we can't do it using ADD macro, since that doesn't guarantee us to preserve the order of elements (might as well use a slot occupied by deleted item) 
	size_t id = polygons.data[polygon_id].vertices.count;
	DYN(polygons.data[polygon_id].vertices, id); // {} 

	// add vertex
	polygons.data[polygon_id].vertices.data[id].value = vertex_index;

	return true; 
}

// * releasing * 

// release shot calibration 
void geometry_release_shot_calibration(size_t shot_id)
{
	ASSERT(validate_shot(shot_id), "invalid shot supplied when releasing calibration");
	Shot * shot = shots.data + shot_id;

	shot->calibrated = false;
	if (shot->projection) cvReleaseMat(&shot->projection);
	if (shot->rotation) cvReleaseMat(&shot->rotation);
	if (shot->translation) cvReleaseMat(&shot->translation);
	if (shot->internal_calibration) cvReleaseMat(&shot->internal_calibration);
}

// release calibration of all shots 
void geometry_release_shots_calibrations() 
{
	for ALL(shots, i) 
	{
		geometry_release_shot_calibration(i);
	}
}

// * builders * 

// for each vertex create list of shots on which said vertex is visible
void geometry_build_vertices_incidence()
{
	// initialize dynamic list (we have one entry for each vertex with 
	// each entry containing another dynamic list of shots)
	DYN_INIT(vertices_incidence); // todo fix leaking

	// go through all shots 
	for ALL(shots, i)
	{
		const Shot * shot = shots.data + i; 

		// and through all points on that shot
		for ALL(shot->points, j)
		{
			const Point * point = shot->points.data + j; 

			// save information about the fact that we can see this vertex on i-th shot
			DYN(vertices_incidence, point->vertex);
			ADD(vertices_incidence.data[point->vertex].shot_point_ids);

			// save the shot and point indices
			LAST(vertices_incidence.data[point->vertex].shot_point_ids).primary = i; 
			LAST(vertices_incidence.data[point->vertex].shot_point_ids).secondary = j; 
		}
	}
}

// for each shot compute how many correspondences this shot has
// with the other ones 
void geometry_build_shots_relations()
{
	DYN_FREE(shots_relations);
	DYN_INIT(shots_relations);

	// initialize to zeros 
	// note that this will eat up unnecessary amount of memory 
	// on large sparsely connected scenes 
	for ALL(shots, i) 
	{
		DYN(shots_relations, i);
		Shot_Relations * const relations = shots_relations.data + i;

		for ALL(shots, j) 
		{
			DYN(relations->pair_relations, j); 
			relations->pair_relations.data[j].correspondences_count = 0; 			
		}
	}

	// now count the correspondences 
	for ALL(vertices_incidence, i) 
	{
		const Vertex_Incidence * const vertex_incidence = vertices_incidence.data + i; 

		for ALL(vertex_incidence->shot_point_ids, j) 
		{
			const Double_Index * const index1 = vertex_incidence->shot_point_ids.data + j;

			for ALL(vertex_incidence->shot_point_ids, k) 
			{
				if (j == k) continue; 
				const Double_Index * const index2 = vertex_incidence->shot_point_ids.data + k; 
				
				shots_relations.data[index1->primary].pair_relations.data[index2->primary].correspondences_count++;
			}
		}
	}
}
















