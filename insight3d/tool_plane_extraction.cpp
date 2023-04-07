#include "tool_plane_extraction.h"

// find major plane in pointcloud using RANSAC
double * tool_plane_extraction(Vertices & vertices, double threshold /*= 0.5*/, bool flatten_inliers /*= false*/, size_t group /*= 0*/)
{
	// we can't estimate anything if we don't have enough vertices
	size_t reconstructed_vertices_count = 0; 

	for ALL(vertices, i) 
	{
		if (vertices.data[i].reconstructed && ++reconstructed_vertices_count == 3) break; 
	}

	if (reconstructed_vertices_count < 3) return NULL;

	// initialize variables and constants 
	threshold *= threshold;
	size_t tries = 0; 
	double best_sample[4]; 
	size_t best_sample_inliers = 0;

	// try many times
	while (tries++ < 1000) 
	{
		// pick random sample
		double picked[3][3]; 
		for (size_t i = 0; i < 3; i++) 
		{
			// pick random vertex
			size_t pick; 
			do
			{
				pick = rand() % vertices.count; 
			}
			while (!IS_SET(vertices, pick) && vertices.data[pick].reconstructed);

			// copy it into vector
			picked[i][0] = vertices.data[pick].x; 
			picked[i][1] = vertices.data[pick].y; 
			picked[i][2] = vertices.data[pick].z; 
		}

		// check sample validity 
		if (
			nearly_zero(distance_sq_3(picked[0], picked[1])) || 
			nearly_zero(distance_sq_3(picked[0], picked[2])) || 
			nearly_zero(distance_sq_3(picked[1], picked[2]))
		)
		{
			continue;
		}

		// put a plane through these points
		double plane[4]; 
		plane_from_three_points(picked[0], picked[1], picked[2], plane);

		// consistency check
		ASSERT(nearly_zero(dot_3(picked[2], plane) + plane[3]), "plane estimated incorrectly");
		ASSERT(nearly_zero(vector_norm_3(plane) - 1), "plane normal not normalized");

		// label vertices as inliers and outliers
		size_t inliers = 0;
		for ALL(vertices, i)
		{
			Vertex * const vertex = vertices.data + i; 
			if (!vertex->reconstructed) continue;

			// calculate point-plane distance
			const double distance = dot_3xyz(plane, vertex->x, vertex->y, vertex->z) + plane[3];

			// is this inlier or outlier? 
			if (abs(distance) <= threshold) 
			{
				inliers++;
			}
		}

		// keep track of the best sample 
		if (inliers > best_sample_inliers)
		{
			memcpy(best_sample, plane, 4 * sizeof(double));
			best_sample_inliers = inliers;
		}
	}

	// check for rare cases where no valid sample was tried 
	// if (best_sample_inliers == 0) return NULL; 
	ASSERT(best_sample_inliers > 0, "sample with inliers found after RANSAC"); // debug, replace by line of code above

	// * re-estimate the plane using least squares and inliers of the best sample *
	CvMat * p;
	LOCK_RW(opencv) 
	{
		CvMat * A = opencv_create_matrix(best_sample_inliers, 3), * b = opencv_create_matrix(best_sample_inliers, 1);
		p = opencv_create_matrix(3, 1);

		// put all inliers' values into A and b
		size_t row = 0;
		for ALL(vertices, i) 
		{
			Vertex * const vertex = vertices.data + i;
			if (!vertex->reconstructed) continue;

			// calculate point-plane distance
			const double distance = dot_3xyz(best_sample, vertex->x, vertex->y, vertex->z) + best_sample[3];

			// if it's inlier, we'll use it
			if (abs(distance) <= threshold)
			{
				OPENCV_ELEM(A, row, 0) = vertex->x; 
				OPENCV_ELEM(A, row, 1) = vertex->y; 
				OPENCV_ELEM(A, row, 2) = vertex->z; 
				OPENCV_ELEM(b, row, 0) = -1;
				row++; 
			}
		}

		// check 
		ASSERT(row == best_sample_inliers, "inlier count and matrix sizes don't match");
		
		// least-squares estimation
		cvSolve(A, b, p, CV_SVD);
	}
	UNLOCK_RW(opencv); 

	// normalize 
	best_sample[0] = OPENCV_ELEM(p, 0, 0);
	best_sample[1] = OPENCV_ELEM(p, 1, 0);
	best_sample[2] = OPENCV_ELEM(p, 2, 0);
	best_sample[3] = 1;
	normalize_inhomogeneous_part(best_sample, 4);
	
	// optionally color inliers and set their group value // todo color is mostly debug thing, remove
	for ALL(vertices, i) 
	{
		Vertex * const vertex = vertices.data + i; 

		// calculate point-plane distance
		const double distance = dot_3xyz(best_sample, vertex->x, vertex->y, vertex->z) + best_sample[3]; // note best_sample[3] should always be 1, optimize this

		// is this inlier?
		if (abs(distance) <= threshold)
		{
			// color it and optionally set it's group
			// vertex->color[0] = 0.52F; 
			// vertex->color[1] = 0.83F; 
			// vertex->color[2] = 0.52F; 

			// set hint meta value
			if (group)
			{
				vertex->group = group;
			}

			// move to the closest point on plane 
			if (flatten_inliers) 
			{
				vertex->x -= distance * best_sample[0]; 
				vertex->y -= distance * best_sample[1]; 
				vertex->z -= distance * best_sample[2]; 
			}
		}
		else
		{
			// vertex->color[0] = 0;
			vertex->group = 0;
		}
	}

	// return plane coefficients
	double * result = ALLOC(double, 4);
	memcpy(result, best_sample, 4 * sizeof(double));

	return result;
}

// extract a plane from a subset of all vertices
double * tool_plane_extraction_subset(Vertices & vertices, size_t * ids, size_t count)
{
	// we can't estimate anything if we don't have enough vertices
	size_t reconstructed_vertices_count = 0; 

	for (size_t i = 0; i < count; i++)
	{
		if (vertices.data[ids[i]].reconstructed && ++reconstructed_vertices_count == 3) break; 
	}

	if (reconstructed_vertices_count < 3) return NULL;

	// initialize variables and constants 
	const double threshold = 0.01; // MEDIAN
	size_t tries = 0; 
	double best_sample[4]; 
	size_t best_sample_inliers = 0;

	// try many times
	while (tries++ < 20) 
	{
		// pick random sample
		double picked[3][3]; 
		for (size_t i = 0; i < 3; i++) 
		{
			// pick random vertex
			size_t pick = ids[rand() % count]; 

			// copy it into vector
			picked[i][0] = vertices.data[pick].x; 
			picked[i][1] = vertices.data[pick].y; 
			picked[i][2] = vertices.data[pick].z; 
		}

		// check sample validity 
		if (
			nearly_zero(distance_sq_3(picked[0], picked[1])) || 
			nearly_zero(distance_sq_3(picked[0], picked[2])) || 
			nearly_zero(distance_sq_3(picked[1], picked[2]))
		)
		{
			continue;
		}

		// put a plane through these points
		double plane[4]; 
		if (!plane_from_three_points(picked[0], picked[1], picked[2], plane)) continue;

		// consistency check
		// const double tmp = dot_3(picked[2], plane);
		ASSERT(nearly_zero(dot_3(picked[2], plane) + plane[3]), "plane estimated incorrectly");
		/*if (!nearly_zero(tmp + plane[3]))
		{
			printf("plane estimated incorrectly"); 
		}*/
		ASSERT(nearly_zero(vector_norm_3(plane) - 1), "plane normal not normalized");

		// label vertices as inliers and outliers
		size_t inliers = 0;
		for (size_t i = 0; i < count; i++)
		{
			Vertex * const vertex = vertices.data + ids[i]; 
			if (!vertex->reconstructed) continue;

			// calculate point-plane distance
			const double distance = dot_3xyz(plane, vertex->x, vertex->y, vertex->z) + plane[3];

			// is this inlier or outlier? 
			if (abs(distance) <= threshold) 
			{
				inliers++;
			}
		}

		// keep track of the best sample 
		if (inliers > best_sample_inliers)
		{
			memcpy(best_sample, plane, 4 * sizeof(double));
			best_sample_inliers = inliers;
		}
	}

	// check for rare cases where no valid sample was tried 
	// if (best_sample_inliers == 0) return NULL; 
	ASSERT(best_sample_inliers > 0, "sample with inliers found after RANSAC"); // debug, replace by line of code above

	// * re-estimate the plane using least squares and inliers of the best sample *
	CvMat * p;
	LOCK_RW(opencv)
	{
		CvMat * A = opencv_create_matrix(best_sample_inliers, 3), * b = opencv_create_matrix(best_sample_inliers, 1); 
		p = opencv_create_matrix(3, 1);

		// put all inliers' values into A and b
		size_t row = 0;
		for (size_t i = 0; i < count; i++)
		{
			Vertex * const vertex = vertices.data + ids[i];
			if (!vertex->reconstructed) continue;

			// calculate point-plane distance
			const double distance = dot_3xyz(best_sample, vertex->x, vertex->y, vertex->z) + best_sample[3];

			// if it's inlier, we'll use it
			if (abs(distance) <= threshold)
			{
				OPENCV_ELEM(A, row, 0) = vertex->x; 
				OPENCV_ELEM(A, row, 1) = vertex->y; 
				OPENCV_ELEM(A, row, 2) = vertex->z; 
				OPENCV_ELEM(b, row, 0) = -1;
				row++; 
			}
		}

		// check 
		ASSERT(row == best_sample_inliers, "inlier count and matrix sizes don't match");
		
		// least-squares estimation
		cvSolve(A, b, p, CV_SVD);
	}
	UNLOCK_RW(opencv);

	// normalize 
	best_sample[0] = OPENCV_ELEM(p, 0, 0);
	best_sample[1] = OPENCV_ELEM(p, 1, 0);
	best_sample[2] = OPENCV_ELEM(p, 2, 0);
	best_sample[3] = 1;
	normalize_inhomogeneous_part(best_sample, 4);
	
	// optionally color inliers and set their group value // todo color is mostly debug thing, remove
	bool * inlier = ALLOC(bool, count);
	memset(inlier, 0, sizeof(bool) * count);
	size_t inliers_count = 0;
	for (size_t i = 0; i < count; i++)
	{
		Vertex * const vertex = vertices.data + ids[i]; 

		// calculate point-plane distance
		const double distance = dot_3xyz(best_sample, vertex->x, vertex->y, vertex->z) + best_sample[3]; // note best_sample[3] should always be 1, optimize this

		// is this inlier?
		if (abs(distance) <= threshold)
		{
			inliers_count++;
			inlier[i] = true;
		}
	}

	// * save data for surface reconstruction *

	for (size_t i = 0; i < count; i++) if (inlier[i])
	{
		inlier[i] = false; // clever way to avoid additional condition in the inner cycle // todo investigate how good/bad this is

		for (size_t j = 0; j < count; j++) if (inlier[j])
		{
			detected_edges[ids[i]][ids[j]]++;
			detected_edges[ids[j]][ids[i]]++;
		}

		inlier[i] = true;
	}

	// * extract triangles from planar surface * 

	// create structures to store the 2d projections 
	/*const int fix = 1000;
	double * projections_xy = ALLOC(double, 2 * inliers_count);
	size_t * projections_id = ALLOC(size_t, inliers_count);
	double projection_bounding_box[4];
	size_t projections_count = 0;*/

	// fix 2d coordinate system on the plane // we don't really need that...
	/*double axis_x[3], axis_y[3], origin[3], axis_helper[3] = { 0, 1, 0 }, zero_vector[3] = { 0, 0, 0};
	nearest_point_on_plane(best_sample, zero_vector, origin);
	nearest_point_on_plane(best_sample, axis_helper, axis_x); 
	sub_3(axis_x, origin, axis_x);
	normalize_vector(axis_x, 3);
	cross_3(axis_x, best_sample, axis_y);
	ASSERT(nearly_zero(vector_norm(axis_y) - 1), "axis_y doesn't have unit length");*/

	/*// calculate orthogonal projections and bounding box
	for (size_t i = 0; i < count; i++)
	{
		Vertex * const vertex = vertices.data + ids[i];

		// calculate point-plane distance
		const double distance = dot_3xyz(best_sample, vertex->x, vertex->y, vertex->z) + best_sample[3]; // note best_sample[3] should always be 1, optimize this

		// is this inlier?
		if (abs(distance) <= threshold)
		{
			// calculate projection of the point on plane
			double point_on_plane[3] = { vertex->x, vertex->y, vertex->z };
			add_mul_3(point_on_plane, -distance, best_sample, point_on_plane);

			// save projection
			const size_t id = 2 * projections_count; // projection's id in the projections_xy array
			projections_id[projections_count] = ids[i];
			projections_xy[id + X] = point_on_plane[X] * fix; 
			projections_xy[id + Y] = point_on_plane[Y] * fix; 
			
			// calculate bounding box
			if (projections_count != 0) 
			{
				if (projections_xy[id + X] < projection_bounding_box[0])
				{
					projection_bounding_box[0] = projections_xy[id + X];
				}

				if (projections_xy[id + X] > projection_bounding_box[2])
				{
					projection_bounding_box[2] = projections_xy[id + X];
				}

				if (projections_xy[id + Y] < projection_bounding_box[1])
				{
					projection_bounding_box[1] = projections_xy[id + Y];
				}

				if (projections_xy[id + Y] > projection_bounding_box[3])
				{
					projection_bounding_box[3] = projections_xy[id + Y];
				}
			}
			else
			{
				projection_bounding_box[0] = projection_bounding_box[2] = projections_xy[id + X];
				projection_bounding_box[1] = projection_bounding_box[3] = projections_xy[id + Y];
			}

			// increate counter
			projections_count++;
		}
	}

	// calculate delaunay triangulation 
	CvMemStorage * mem_storage = cvCreateMemStorage();
	CvSubdiv2D * subdivision = cvCreateSubdiv2D(CV_SEQ_KIND_SUBDIV2D, sizeof(*subdivision), sizeof(CvSubdiv2DPoint), sizeof(CvQuadEdge2D), mem_storage);
	cvInitSubdivDelaunay2D(
		subdivision, 
		cvRect(
			0, 
			0, 
			projection_bounding_box[2] - projection_bounding_box[0] + 10, 
			projection_bounding_box[3] - projection_bounding_box[1] + 10
		)
	);

	for (size_t i = 0; i < inliers_count; i++)
	{
		double 
			x = projections_xy[2 * i + 0] - projection_bounding_box[0] + 1,
			y = projections_xy[2 * i + 1] - projection_bounding_box[1] + 1
		;

		cvSubdivDelaunay2DInsert(subdivision, 
			cvPoint2D32f(
				x, 
				y
			)
		); 
	}

	{
		CvSeqReader reader;
		int total = subdivision->edges->total;
		int elem_size = subdivision->edges->elem_size;
		cvStartReadSeq((CvSeq *)(subdivision->edges), &reader, 0);

		for (int i = 0; i < total; i++)
		{
			CvQuadEdge2D * edge = (CvQuadEdge2D *)(reader.ptr);

			if (CV_IS_SET_ELEM(edge))
			{
				CvSubdiv2DPoint 
					* org = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge)edge), 
					* dst = cvSubdiv2DEdgeDst((CvSubdiv2DEdge)edge)
				;

				size_t org_id, dst_id;
				bool org_found = false, dst_found = false;

				// find these two points among the ones projected 
				for (size_t j = 0; j < inliers_count; j++) 
				{
					if (nearly_zero(
							sqr_value(org->pt.x - projections_xy[2 * j + X] + projection_bounding_box[0] - 1) + 
							sqr_value(org->pt.y - projections_xy[2 * j + Y] + projection_bounding_box[1] - 1)
						)
					)
					{
						org_found = true; 
						org_id = j; 
					}

					if (nearly_zero(
							sqr_value(dst->pt.x - projections_xy[2 * j + X] + projection_bounding_box[0] - 1) + 
							sqr_value(dst->pt.y - projections_xy[2 * j + Y] + projection_bounding_box[1] - 1)
						)
					)
					{
						dst_found = true; 
						dst_id = j; 
					}
				}

				// add the edge 
				if (org_found && dst_found) 
				{
					detected_edges[projections_id[org_id]][projections_id[dst_id]]++;
					detected_edges[projections_id[dst_id]][projections_id[org_id]]++;
				}
			}

			CV_NEXT_SEQ_ELEM(elem_size, reader);
		}
	}

	cvReleaseMemStorage(&mem_storage);*/

	// return plane coefficients
	double * result = ALLOC(double, 4);
	memcpy(result, best_sample, 4 * sizeof(double));
	FREE(inlier);

	return result;
}
