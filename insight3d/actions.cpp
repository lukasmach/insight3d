#include "tool_core.h"
#include "actions.h"

// triangulates vertices given current projection matrices and vertex projections
bool action_triangulate_vertices(
	bool * shots_to_use /*= NULL*/, 
	const int min_inliers /*= MVG_MIN_INLIERS_TO_TRIANGULATE*/, 
	const int min_inliers_weaker /*= MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER*/,
	const bool only_manual /*= false*/,
	const double measurement_threshold /*= MVG_MEASUREMENT_THRESHOLD*/
)
{
	// count vertices to reconstruct 
	size_t vertex_count = 0; 
	for ALL(vertices, i) 
	{
		Vertex * vertex = vertices.data + i;

		if (only_manual && vertex->vertex_type != GEOMETRY_VERTEX_USER) continue;
		if (IS_SET(vertices_incidence, i)) 
		{
			vertex_count++;
		}
	}

	// * reconstruct all vertices *

	// progressbar support 
	tool_start_progressbar();
	size_t vertex_counter = 0; 

	// go through all vertices
	for ALL(vertices, i)
	{
		Vertex * vertex = vertices.data + i;
		CvMat * reconstructed_vertex = NULL; 
		vertex_counter++;

		if (only_manual && vertex->vertex_type != GEOMETRY_VERTEX_USER) continue;

		// if incidence is defined (i.e., if the vertex has some 2d points)
		if (IS_SET(vertices_incidence, i)) 
		{
			tool_show_progress(vertex_counter / (double)vertex_count);

			// feed the data inside appropriate matrices 
			const CvMat * * projection_matrices;
			CvMat * points; 

			// locking call
			bool publish_status = publish_triangulation_data(vertices_incidence.data[i], i, projection_matrices, points, shots_to_use);

			// call reconstruction algorithm
			if (publish_status) 
			{
				LOCK_RW(opencv);
				{
					reconstructed_vertex = mvg_triangulation_RANSAC(
						projection_matrices, points, true, false, 
						min_inliers, 
						min_inliers_weaker, 
						MVG_RANSAC_TRIANGULATION_TRIALS, 
						MVG_MEASUREMENT_THRESHOLD
					);
					FREE(projection_matrices); 
					cvReleaseMat(&points);
				}
				UNLOCK_RW(opencv);
			}
		}

		// if the reconstruction succeeded
		if (reconstructed_vertex) 
		{
			if (reconstructed_vertex->rows == 4) 
			{
				LOCK_RW(opencv)
				{
					opencv_normalize_homogeneous(reconstructed_vertex);
				}
				UNLOCK_RW(opencv);
			}

			// save calculated coordinates
			vertex->reconstructed = true;
			vertex->x = OPENCV_ELEM(reconstructed_vertex, 0, 0); 
			vertex->y = OPENCV_ELEM(reconstructed_vertex, 1, 0); 
			vertex->z = OPENCV_ELEM(reconstructed_vertex, 2, 0);
		} 
		else
		{
			// mark as unreconstructed
			vertex->reconstructed = false;
			vertex->x = 0;
			vertex->y = 0;
			vertex->z = 0;
		}

		// release resources
		ATOMIC_RW(opencv, cvReleaseMat(&reconstructed_vertex); );
	}

	tool_end_progressbar();

	return true;
}

// compute projection matrix of current camera from 3d to 2d correspondences
bool action_camera_resection(size_t shot_id, const bool enforce_square_pixels, const bool enforce_zero_skew)
{
	ASSERT_IS_SET(shots, shot_id);
	Shot * const shot = shots.data + shot_id;

	// first count how many points on this shot have their vertices reconstructed 
	size_t n = 0; 
	for ALL(shots.data[shot_id].points, i) 
	{
		const Point * const point = shots.data[shot_id].points.data + i; 
		if (vertices.data[point->vertex].reconstructed) 
		{
			n++;
		}
	}

	if (n < 6)
	{
		return false;
	}

	// go through all points on this shot and fill them and their vertices 
	// into respective matrices
	ATOMIC_RW(opencv, 
		CvMat * mat_vertices = cvCreateMat(3, n, CV_64F); 
		CvMat * mat_projected = cvCreateMat(2, n, CV_64F);
	);

	n = 0;
	for ALL(shots.data[shot_id].points, i) 
	{
		const Point * const point = shots.data[shot_id].points.data + i; 
		const Vertex * const vertex = vertices.data + point->vertex;
		if (!vertex->reconstructed) continue;

		// copy the values into matrices
		OPENCV_ELEM(mat_vertices, 0, n) = vertex->x; 
		OPENCV_ELEM(mat_vertices, 1, n) = vertex->y; 
		OPENCV_ELEM(mat_vertices, 2, n) = vertex->z; 
		OPENCV_ELEM(mat_projected, 0, n) = point->x * shots.data[shot_id].width; 
		OPENCV_ELEM(mat_projected, 1, n) = point->y * shots.data[shot_id].height; 
		n++;
	}

	// perform the calculation
	bool ok;
	ATOMIC_RW(opencv, 
		CvMat * R = opencv_create_matrix(3, 3);
		CvMat * K = opencv_create_matrix(3, 3);
		CvMat * T = opencv_create_matrix(3, 1);
		CvMat * P = opencv_create_matrix(3, 4);

		ok = mvg_resection_RANSAC(mat_vertices, mat_projected, P, K, R, T, false);
	);

	// check result and optionally enforce some constraints
	if (ok)
	{
		LOCK_RW(opencv)
		{
			// enforce restrictions on calibration matrix 
			if (!mvg_restrict_calibration_matrix(K, enforce_zero_skew, enforce_square_pixels))
			{
				cvReleaseMat(&R);
				cvReleaseMat(&K);
				cvReleaseMat(&T);
				cvReleaseMat(&P);
				cvReleaseMat(&mat_vertices);
				cvReleaseMat(&mat_projected);
				UNLOCK_RW(opencv);
				return false;
			}

			// if it has been calibrated before, release previous calibration
			if (shot->calibrated) 
			{
				cvReleaseMat(&shot->projection); 
				cvReleaseMat(&shot->rotation);
				cvReleaseMat(&shot->translation); 
				cvReleaseMat(&shot->internal_calibration); 
			}

			// save estimated matrices into shot structure
			shot->projection = P;
			shot->rotation = R; 
			shot->translation = T; 
			shot->internal_calibration = K;

			// fill in the rest of the calibration 
			geometry_calibration_from_decomposed_matrices(shot_id);

			// mark as calibrated
			shot->calibrated = true;
		}
		UNLOCK_RW(opencv);
	}

	// release resources 
	LOCK_RW(opencv)
	{
		cvReleaseMat(&mat_vertices); 
		cvReleaseMat(&mat_projected);
	}
	UNLOCK_RW(opencv);

	return ok;
}
