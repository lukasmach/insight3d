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
