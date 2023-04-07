#ifndef __TOOL_PLANE_EXTRACTION
#define __TOOL_PLANE_EXTRACTION

#include "interface_opencv.h"
#include "core_math_routines.h"
#include "geometry_structures.h"

// find major plane in point cloud using RANSAC
// todo time bounds
// todo normalization 
// todo refactor code (look at RANSAC implementation from OpenCV)
// todo check that we have at least 3 _reconstructed_ vertices
// todo optimize this (sqrt...)
double * tool_plane_extraction(Vertices & vertices, double threshold = 0.5, bool flatten_inliers = false, size_t group = 0);

// version using only a subset of vertices
double * tool_plane_extraction_subset(Vertices & vertices, size_t * ids, size_t count);

#endif
