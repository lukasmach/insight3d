#ifndef __GEOMETRY_PUBLISH
#define __GEOMETRY_PUBLISH

#include "interface_opencv.h"
#include "geometry_structures.h"
#include "geometry_queries.h"

// exports data from application structures into structures suitable for OpenCV and MVG
// note opencv should be locked
bool publish_triangulation_data(
	const Vertex_Incidence & incidence, size_t vertex_id, const CvMat * * & projection_matrices, CvMat * & points, bool * shots_to_use
);

// exports data from application structures of given calibration into structures suitable for OpenCV and MVG
// note opencv should be locked
bool publish_triangulation_data_from_calibration(
	const size_t calibration_id, const Vertex_Incidence & incidence, size_t vertex_id, 
	const CvMat * * & projection_matrices, CvMat * & points, size_t * & indices
);

// export data for the computation of fundamental matrix
// note opencv should be locked
bool publish_2_view_reconstruction_data(
	const size_t shot_id1, const size_t shot_id2, CvMat ** points1, CvMat ** points2, 
	size_t ** points1_indices, size_t ** points2_indices
);

// export data for resection of given camera, vertices' coordinates are inhomogeneous
// but the coordinates of points are homogeneous vectors
// note opencv should be locked
bool publish_resection_data_from_calibration(
	const size_t calibration_id, const size_t shot_id, 
	CvMat ** points, CvMat ** vertices, size_t ** points_indices
);

// export polygon
CvMat * publish_polygon(const size_t shot_id, const size_t polygon_id);

#endif
