#ifndef __GEOMETRY_QUERIES
#define __GEOMETRY_QUERIES

#include "geometry_structures.h"
#include "core_math_routines.h"

// determine if all of contour's vertices have been reconstructed
bool query_is_contour_reconstructed(const Contour & contour, const Points & points, const Vertices & vertices);

// determine if all polygon's vertices have been reconstructed
bool query_is_polygon_reconstructed(const Polygon_3d & polygon, const Vertices & vertices);

// find point corresponding to a vertex on a given shot 
bool query_find_point_on_shot_by_vertex_id(size_t shot_id, size_t vertex_id, size_t & point_id);

// find nearest point on shot, returns squared distance in image pixels
double query_nearest_point(const size_t shot_id, const double x, const double y, size_t & point_id, bool skipping_auto = false);

// count the number of reconstructed points on this shot 
size_t query_count_reconstructed_points_on_shot(const size_t shot_id);

// count the number of vertex's points 
size_t query_count_points_by_vertex(const size_t vertex_id);

#endif
