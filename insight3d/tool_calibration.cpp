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

#include "tool_calibration.h"

// tool's state structure
struct Tool_Calibration
{
	// we don't any state information at this point... but luck favors the prepared :-)
};

Tool_Calibration tool_calibration;

// parameter ids
const size_t 
	CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD = 0,
	CALIBRATION_NORMALIZE_DATA = 1,
	CALIBRATION_NORMALIZE_A = 2,
	CALIBRATION_RANDOMNESS = 3
;

static size_t tool_calibration_id;

// forward declarations
void calibration_bundle(const double measurement_error);
void calibration_triangulate_vertices(
	const size_t calibration_id, const double measurement_threshold, const int min_inliers,
	const bool normalize_data, const bool normalize_A, const int shot_id = -1
);
void calibration_rectify(bool affine = false);

// update calibration flag in shots structure - it must be true iff the shot is calibrated in given partial calibration
void calibration_refresh_flag(size_t calibration_id)
{
	size_t shot_iter; 
	LAMBDA(shots, shot_iter, shots.data[shot_iter].partial_calibration = false; );
	LAMBDA(calibrations.data[calibration_id].Ps, shot_iter, shots.data[calibrations.data[calibration_id].Ps.data[shot_iter].shot_id].partial_calibration = true; );
}

// refresh lists in UI containing information modified by this tool
void calibration_refresh_UI()
{
	calibration_refresh_flag(ui_state.current_calibration);
	ui_list_update();
}

// create UI elements for calibration tool  
void tool_calibration_create()
{
	tool_calibration_id = tool_create(UI_MODE_UNSPECIFIED, "Calibration", "Camera calibration algorithm");

	tool_create_tab("Calibration");

	/*tool_create_label("Browser:");

	tool_create_button("Previous partial calibration", tool_calibration_prev);
	tool_create_button("Next partial calibration", tool_calibration_next);*/

	tool_register_menu_function("Main menu|Calibration|Automatic calibration|", tool_calibration_auto);

	tool_create_label("Camera calibration:");

	tool_register_real(CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD, "Image measurement threshold (px): ", 12, 0.01, 250, 1);
	tool_register_bool(CALIBRATION_NORMALIZE_DATA, "Normalization of input data", 1);
	tool_register_bool(CALIBRATION_NORMALIZE_A, "Normalization of linear systems", 0);
	tool_register_int(CALIBRATION_RANDOMNESS, "Randomness of automatic calibration: ", 3, 0, 10000, 1);

	tool_create_separator(); 
	tool_create_button("Automatic calibration", tool_calibration_auto);
	tool_create_label("If you want to see the details:"); 
	tool_create_button("Begin new calibration", tool_calibration_auto_begin);
	tool_create_button("1 calibration step", tool_calibration_auto_step);
	tool_create_button("Finish calibration", tool_calibration_auto_end);

	tool_create_separator();
	tool_create_button("New calibration from image pair", tool_calibration_pair);
	tool_create_button("Add view(s) by resection", tool_calibration_add_views);
	tool_create_button("Triangulate vertices", tool_calibration_triangulate);
	tool_create_button("Optimize calibration", tool_calibration_bundle);
	tool_create_button("Test rectification", tool_calibration_test_rectification);
	tool_create_button("Print calibration", tool_calibration_print);
	tool_create_button("Use calibration", tool_calibration_use);

	tool_create_separator();
	tool_create_button("Add views and refine", tool_calibration_refine);
	tool_create_button("Refine (strict)", tool_calibration_refine_strict);
	tool_create_button("Metric reconstruction", tool_calibration_metric);
	tool_create_button("Affine reconstruction", tool_calibration_affine);
	
	tool_create_separator();
	// tool_create_button("Triangulate vertices (only trusted)", tool_calibration_triangulate_trusted);
	tool_create_button("Clear shots' calibration", tool_calibration_clear);
	// tool_create_button("Affine reconstruction", tool_calibration_affine);

	/*tool_create_separator();
	tool_create_button("VRML export (for debugging)", tool_calibration_debug_export);*/
}

// next calibration 
void tool_calibration_next()
{
	// proceed to next calibration
	size_t id = 0;
	if (INDEX_IS_SET(ui_state.current_calibration))
	{
		id = ui_state.current_calibration + 1;
	}

	// find next set calibration
	while (id < calibrations.count && !calibrations.data[id].set)
	{
		id++;
	}

	if (id < calibrations.count) 
	{
		INDEX_SET(ui_state.current_calibration, id);
		calibration_refresh_UI();
	}
}

// previous calibration 
void tool_calibration_prev()
{
	if (!INDEX_IS_SET(ui_state.current_calibration)) return;

	// proceed to previous calibration
	long int id = ui_state.current_calibration;
	id--;

	// find next set calibration
	while (id >= 0 && !calibrations.data[id].set)
	{
		id--;
	}

	if (id >= 0) 
	{
		INDEX_SET(ui_state.current_calibration, (size_t)id);
		calibration_refresh_UI();
	}
	else
	{
		INDEX_CLEAR(ui_state.current_calibration);
	}
}

// update current estimation of the set of inlying points on one shot
void calibration_update_inliers(const size_t calibration_id, const size_t P_id, const size_t count, const size_t * points_indices, const CvMat * status)
{
	ASSERT_IS_SET(calibrations, calibration_id);
	const Calibration * calibration = calibrations.data + calibration_id;
	ASSERT_IS_SET(calibration->Ps, P_id);
	Calibration_Camera * camera = calibration->Ps.data + P_id;

	for (size_t i = 0; i < count; i++)
	{
		DYN(camera->points_meta, points_indices[i]);
		int j = CV_MAT_ELEM(*status, signed char, 0, i);

		camera->points_meta.data[points_indices[i]].inlier = j;

		if (j != 0 && j != 1)
		{
			ASSERT(false, "unknown value in status matrix");
		}
	}
}

// update current estimation of the set of inlying points on one shot
void calibration_update_inliers(const size_t calibration_id, const size_t P_id, const size_t count, const size_t * points_indices, const bool * status)
{
	ASSERT_IS_SET(calibrations, calibration_id);
	const Calibration * calibration = calibrations.data + calibration_id;
	ASSERT_IS_SET(calibration->Ps, P_id);
	Calibration_Camera * camera = calibration->Ps.data + P_id;

	for (size_t i = 0; i < count; i++)
	{
		DYN(camera->points_meta, points_indices[i]);
		camera->points_meta.data[points_indices[i]].inlier = status[i]; 
	}
}

// update current estimation of the set of inlying points for one vertex
void calibration_update_inliers(const size_t calibration_id, const size_t count, const size_t * indices, const bool * status)
{
	ASSERT_IS_SET(calibrations, calibration_id);
	const Calibration * calibration = calibrations.data + calibration_id;

	for (size_t i = 0; i < count; i++) 
	{
		const size_t P_id = indices[2 * i + 0], point_id = indices[2 * i + 1];
		ASSERT_IS_SET(calibration->Ps, P_id);
		Calibration_Camera * P = calibration->Ps.data + P_id;
		ASSERT(validate_point(P->shot_id, point_id), "invalid shot/point referenced in triangulation data");

		DYN(P->points_meta, point_id);
		P->points_meta.data[point_id].inlier = status[i];
	}
}

// update current estimation of the set of inlying points for the whole calibration 
// todo 
void calibration_update_inliers(const size_t calibration_id)
{
	ASSERT_IS_SET(calibrations, calibration_id);
	const Calibration * calibration = calibrations.data + calibration_id;

	// size_t * vertex_to_X = dyn_empty_index(calibration->Ps), Xs_i;
	// LAMBDA(calibration->Ps, Xs_i, vertex_to_X[calibration->Xs.data[Xs_i].vertex_id] = Xs_i; );

	// go through all cameras 
	for ALL(calibration->Ps, i) 
	{
		// todo
	}
}

// calibrate pair of shots and create new calibration to enclose it
// note that opencv should be locked
bool calibration_pair(
	const size_t shot_id1, 
	const size_t shot_id2, 
	const bool normalize_data, 
	const bool normalize_A, 
	const double epipolar_distance_threshold,
	size_t * const calibration_id
)
{
	// export data into OpenCV structures and generate additional indexing arrays
	CvMat * points1, * points2;
	size_t * points1_indices, * points2_indices;
	if (!publish_2_view_reconstruction_data(shot_id1, shot_id2, &points1, &points2, &points1_indices, &points2_indices))
	{
		printf("Not enough correspondences.\n");
		return false;
	}
	ASSERT(points1->cols == points2->cols, "sizes of matrices containing correspondences do not match");

	// compute F
	ATOMIC_RW(opencv, 
		CvMat * F = opencv_create_matrix(3, 3); CvMat * status = cvCreateMat(1, points1->cols, CV_8S);
	);

	if (!cvFindFundamentalMat(points1, points2, F, CV_FM_RANSAC, epipolar_distance_threshold, 0.999, status))
	{
		printf("Fundamental matrix couldn't be estimated.\n");
		ATOMIC_RW(opencv, 
			cvReleaseMat(&F); cvReleaseMat(&status); cvReleaseMat(&points1); cvReleaseMat(&points2); 
		);
		FREE(points1_indices); FREE(points2_indices);
		return false;
	}

	// my friday afternoon experiment 
	/*CvMat * E = opencv_create_matrix(3, 3); 
	cvCopy(F, E); 
	double min = 1000000, min_val = -1;;
	for (double f = 1000; f < 8000; f += 1)
	{
		CvMat * K = opencv_create_matrix(3, 3), * K_T = opencv_create_matrix(3, 3); 

		// compose calibration matrix
		cvZero(K); 
		OPENCV_ELEM(K, 0, 0) = f; 
		OPENCV_ELEM(K, 1, 1) = f; 
		OPENCV_ELEM(K, 2, 2) = 1;
		OPENCV_ELEM(K, 0, 2) = shots.data[shot_id1].width / 2;
		OPENCV_ELEM(K, 1, 2) = shots.data[shot_id1].height / 2;

		// transpose to obtain K_prime
		cvTranspose(K, K_T);

		// multiply to obtain guessed E
		cvMatMul(K_T, F, E);
		cvMatMul(E, K, E);

		// normalize 
		//double norm = 0;
		//for (int i = 0; i < 3; i++) 
		//{
		//	for (int j = 0; j < 3; j++) 
		//	{
		//		norm += OPENCV_ELEM(E, i, j) * OPENCV_ELEM(E, i, j);
		//	}
		//}
		//norm = 1000.0 / norm;
		//for (int i = 0; i < 3; i++) 
		//{
		//	for (int j = 0; j < 3; j++) 
		//	{
		//		OPENCV_ELEM(E, i, j) *= norm;
		//	}
		//}

		// SVD decomposition
		CvMat * W = opencv_create_matrix(3, 1);
		cvSVD(E, W);

		// error function 
		double w1 = OPENCV_ELEM(W, 0, 0), w2 = OPENCV_ELEM(W, 1, 0), w3 = OPENCV_ELEM(W, 2, 0);
		w2 /= w1;
		double error = abs(w2 - 1);
		printf("W_1 = %f\t W_2 = %f\t error = %f\n", w1, w2, error);
		if (error < min)
		{
			min = error;
			min_val = f;
		}
	}

	// print it 
	printf("\n\nminimum value %f at %f\n", min, min_val);*/

	// ***
	// my sunday afternoon experiment 
	
	// move F into semi-calibrated frame by undoing the effect of non-zero principal point
	/*CvMat * G = opencv_create_matrix(3, 3); 
	CvMat * K_pp = opencv_create_I_matrix(3);
	OPENCV_ELEM(K_pp, 0, 2) = (shots.data[shot_id1].width) / 2; 
	OPENCV_ELEM(K_pp, 1, 2) = (shots.data[shot_id1].height) / 2;
	opencv_debug("K_pp", K_pp);
	cvMatMul(F, K_pp, G);
	cvTranspose(K_pp, K_pp);
	cvMatMul(K_pp, G, G);

	// now perform SVD decomposition 
	CvMat * W = opencv_create_matrix(3, 1), * V_T = opencv_create_matrix(3, 3), * U = opencv_create_matrix(3, 3);
	cvSVD(G, W, U, V_T);
	opencv_debug("F", F);
	opencv_debug("G", G);
	opencv_debug("U", U);
	opencv_debug("W", W);
	opencv_debug("V_T", V_T);
	const double U_31 = OPENCV_ELEM(U, 2, 0), U_32 = OPENCV_ELEM(U, 2, 1), V_31 = OPENCV_ELEM(V_T, 0, 2), V_32 = OPENCV_ELEM(V_T, 1, 2);
	const double U_31_sq = U_31*U_31, U_32_sq = U_32*U_32, V_31_sq = V_31*V_31, V_32_sq = V_32*V_32;
	const double a = OPENCV_ELEM(W, 0, 0), b = OPENCV_ELEM(W, 1, 0);
	const double a_sq = a*a, b_sq = b*b;

	// construct quadratic equation in f^2
	const double 
		qa = a_sq * (1 - U_31_sq) * (1 - V_31_sq) - b_sq * (1 - U_32_sq) * (1 - V_32_sq),
		qb = a_sq * (U_31_sq + V_31_sq - 2 * U_31_sq * V_31_sq) - b_sq * (U_32_sq + V_32_sq - 2 * U_32_sq * V_32_sq),
		qc = a_sq * U_31_sq * V_31_sq - b_sq * U_32_sq * V_32_sq
	;

	printf("%f * f^4 + %f * f^2 + %f = 0\n", qa, qb, qc);

	// solve the quadratic equation
	double D = qb*qb - 4 * qa * qc;
	if (D > 0) 
	{
		D = sqrt(D);
		double 
			x1 = (-qb + D) / 2 * qa, 
			x2 = (-qb - D) / 2 * qa;

		printf("Roots are %f and %f...\n", x1, x2);
	}
	else
	{
		printf("Quadratic equation has no root\n");
	}*/

	// normalize
	LOCK_RW(opencv)
	{
		CvMat * H_normalization1, * H_normalization2, * H_normalization1_inv, * H_normalization2_inv, * F_prime;
		if (normalize_data)
		{
			H_normalization1 = opencv_create_matrix(3, 3); 
			H_normalization2 = opencv_create_matrix(3, 3); 
			H_normalization1_inv = opencv_create_matrix(3, 3); 
			H_normalization2_inv = opencv_create_matrix(3, 3); 
			if (!H_normalization1 || !H_normalization2 || !H_normalization1_inv || !H_normalization2_inv) 
			{
				cvReleaseMat(&F); cvReleaseMat(&status); cvReleaseMat(&points1); cvReleaseMat(&points2); 
				FREE(points1_indices); FREE(points2_indices);
				if (H_normalization1) cvReleaseMat(&H_normalization1);
				if (H_normalization2) cvReleaseMat(&H_normalization2);
				if (H_normalization1_inv) cvReleaseMat(&H_normalization1_inv);
				if (H_normalization2_inv) cvReleaseMat(&H_normalization2_inv);

				UNLOCK_RW(opencv);
				return false;
			}

			mvg_normalize_points(points1, H_normalization1);
			mvg_normalize_points(points2, H_normalization2);
			cvInvert(H_normalization1, H_normalization1_inv, CV_SVD);
			cvInvert(H_normalization2, H_normalization2_inv, CV_SVD);

			// apply normalization to F
			F_prime = opencv_create_matrix(3, 3);
			CvMat * T = opencv_create_matrix(3, 3), * H_normalization2_inv_T = opencv_create_matrix(3, 3);
			cvTranspose(H_normalization2_inv, H_normalization2_inv_T);
			cvMatMul(F, H_normalization1_inv, T);
			cvMatMul(H_normalization2_inv_T, T, F_prime);
			cvReleaseMat(&T);
			cvReleaseMat(&H_normalization2_inv_T);
		}
		else
		{
			F_prime = F;
		}

		// extract P_1, P_2 from F
		CvMat * P1 = opencv_create_matrix(3, 4), * P2 = opencv_create_matrix(3, 4);
		if (!mvg_extract_Ps_from_F(F_prime, P1, P2))
		{
			printf("Unable to extract camera calibration from fundamental matrix");
			cvReleaseMat(&F_prime);
			if (normalize_data)
			{
				cvReleaseMat(&H_normalization1);
				cvReleaseMat(&H_normalization1_inv);
				cvReleaseMat(&H_normalization2);
				cvReleaseMat(&H_normalization2_inv);
				cvReleaseMat(&F);
			}
			cvReleaseMat(&status);
			cvReleaseMat(&points1);
			cvReleaseMat(&points2);
			cvReleaseMat(&P1);
			cvReleaseMat(&P2);
			FREE(points1_indices);
			FREE(points2_indices);

			UNLOCK_RW(opencv);
			return false;
		}

		/*opencv_debug("First camera matrix", P1);
		opencv_debug("Second camera matrix", P2);*/

		// create new calibration
		ADD(calibrations); 
		*calibration_id = LAST_INDEX(calibrations);
		Calibration * const calibration = calibrations.data + *calibration_id;
		DYN_INIT(calibration->Xs);
		DYN_INIT(calibration->Ps);

		// set it as selected 
		INDEX_SET(ui_state.current_calibration, *calibration_id);

		// reconstruct individual points
		CvMat * projected_points = opencv_create_matrix(2, 2); 
		const CvMat * Ps[2];
		Ps[0] = P1;
		Ps[1] = P2;
		for (size_t i = 0; i < points1->cols; i++)
		{
			// triangulate only inliers 
			if (CV_MAT_ELEM(*status, signed char, 0, i) == 0) continue;

			ADD(calibration->Xs); 
			Calibration_Vertex * const vertex = calibration->Xs.data + LAST_INDEX(calibration->Xs);
			ASSERT(validate_point(shot_id1, points1_indices[i]), "invalid point encountered in triangulation code");
			ASSERT(validate_point(shot_id2, points2_indices[i]), "invalid point encountered in triangulation code");
			ASSERT(shots.data[shot_id1].points.data[points1_indices[i]].vertex == shots.data[shot_id2].points.data[points2_indices[i]].vertex, "inconsistent indexing of vertex");
			vertex->vertex_id = shots.data[shot_id1].points.data[points1_indices[i]].vertex;

			// fill the data in a matrix 
			OPENCV_ELEM(projected_points, 0, 0) = OPENCV_ELEM(points1, 0, i); 
			OPENCV_ELEM(projected_points, 1, 0) = OPENCV_ELEM(points1, 1, i); 
			OPENCV_ELEM(projected_points, 0, 1) = OPENCV_ELEM(points2, 0, i); 
			OPENCV_ELEM(projected_points, 1, 1) = OPENCV_ELEM(points2, 1, i); 

			// call triangulation routine and save the result
			vertex->X = mvg_triangulation_SVD(Ps, projected_points, normalize_A, 2);
		}

		// denormalize projection matrices
		if (normalize_data)
		{
			cvMatMul(H_normalization1_inv, P1, P1); 
			cvMatMul(H_normalization2_inv, P2, P2);
			cvReleaseMat(&H_normalization1);
			cvReleaseMat(&H_normalization2);
		}

		// save first camera
		ADD(calibration->Ps);
		size_t P1_id = LAST_INDEX(calibration->Ps);
		Calibration_Camera * const camera1 = calibration->Ps.data + P1_id;
		camera1->P = P1;
		camera1->shot_id = shot_id1;
		DYN_INIT(camera1->Fs);
		DYN_INIT(camera1->points_meta);

		// save second camera
		ADD(calibration->Ps);
		size_t P2_id = LAST_INDEX(calibration->Ps);
		Calibration_Camera * const camera2 = calibration->Ps.data + P2_id;
		camera2->P = P2;
		camera2->shot_id = shot_id2;
		DYN_INIT(camera2->Fs);
		DYN_INIT(camera2->points_meta);
		/*ADD(camera2->Fs);
		LAST(camera2->Fs).first_shot_id = shot_id1;
		LAST(camera2->Fs).F_prime = F_prime;*/

		// save current estimation of the set of inliers 
		calibration_update_inliers(*calibration_id, P1_id, points1->cols, points1_indices, status);
		calibration_update_inliers(*calibration_id, P2_id, points1->cols, points2_indices, status);

		// release resources
		if (normalize_data) 
		{
			cvReleaseMat(&H_normalization1);
			cvReleaseMat(&H_normalization1_inv);
			cvReleaseMat(&H_normalization2);
			cvReleaseMat(&H_normalization2_inv);
			cvReleaseMat(&F);
		}
		cvReleaseMat(&projected_points);
		cvReleaseMat(&status);
		cvReleaseMat(&points1); 
		cvReleaseMat(&points2);
		FREE(points1_indices); 
		FREE(points2_indices);
	}
	UNLOCK_RW(opencv);

	return true;
}

// perform projective reconstruction of a view pair 
void tool_calibration_pair()
{
	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double epipolar_distance_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);

	// get first two selected cameras
	size_t selected[2];
	if (ui_selected_shots_n(selected, 2) < 2)
	{
		printf("Two images must be selected.\n");
		return;
	}

	const size_t shot_id1 = selected[0], shot_id2 = selected[1];
	ASSERT(validate_shot(shot_id1) && validate_shot(shot_id2), "shot not valid even though it is selected");
	ASSERT(shot_id1 != shot_id2, "selected shots are identical");

	// check that meta of those two shots is loaded 
	if (shots.data[shot_id1].info_status < GEOMETRY_INFO_DEDUCED || shots.data[shot_id2].info_status < GEOMETRY_INFO_DEDUCED)
	{
		printf("Meta info not loaded. Shouldn't ever happen, but clicking on that photo will resolve this.\n");
		return;
	}

	// call routine to actually do all the work
	size_t calibration_id;
	
	LOCK_RW(opencv)
	{
		if (!calibration_pair(shot_id1, shot_id2, normalize_data, normalize_A, epipolar_distance_threshold, &calibration_id))
		{
			printf("Failed to calibrate image pair.\n");
	
			UNLOCK_RW(opencv);
			return;
		}
	}
	UNLOCK_RW(opencv);

	// refresh UI
	calibration_refresh_UI();

	return;
}

// internal function used to calibrate given shot by resection
bool calibration_add_view(const size_t calibration_id, const size_t shot_id, const double threshold, const bool normalize_data, const bool normalize_A)
{
	Shot * const shot = shots.data + shot_id;
	Calibration * const calibration = calibrations.data + calibration_id;

	// export data
	CvMat * points = NULL, * vertices = NULL;
	size_t * points_indices = NULL;
	bool ready = publish_resection_data_from_calibration(calibration_id, shot_id, &points, &vertices, &points_indices);

	// check the data
	if (!ready || points->cols < 10) 
	{ 
		if (points) 
		{
			ATOMIC_RW(opencv, cvReleaseMat(&points); );
		}

		if (vertices) 
		{
			ATOMIC_RW(opencv, cvReleaseMat(&vertices); );
		}

		if (points_indices) 
		{
			FREE(points_indices);
		}

		printf("Unable to resect this image.\n"); 

		return false;
	}

	ASSERT(points->cols == vertices->cols, "resection data inconsistent");
	if (ready && points->cols < 6) 
	{
		ready = false; 
		ATOMIC_RW(opencv, 
			cvReleaseMat(&points); 
			cvReleaseMat(&vertices); 
		);
		free(points_indices);
	}

	// normalize data 
	CvMat * H_normalization_inv = NULL;
	double scale = 1;
	if (normalize_data)
	{
		LOCK_RW(opencv)
		{
			CvMat * H_normalization = opencv_create_matrix(3, 3);
			mvg_normalize_points(points, H_normalization, &scale);
			H_normalization_inv = opencv_create_matrix(3, 3);
			cvInvert(H_normalization, H_normalization_inv);
			cvReleaseMat(&H_normalization);
		}
		UNLOCK_RW(opencv);
	}

	// calculate resection
	bool ok; 
	LOCK_RW(opencv)
	{
		CvMat * P = opencv_create_matrix(3, 4);
		bool * inliers = ALLOC(bool, points->cols);
		ok = mvg_resection_RANSAC(vertices, points, P, NULL, NULL, NULL, normalize_A, 500, threshold * scale, inliers);
		if (ok)
		{
			// * if the resection was successful, save it *

			// try to find the camera among those already calibrated
			size_t P_id;
			bool P_found;
			LAMBDA_FIND(calibration->Ps, P_id, P_found, calibration->Ps.data[P_id].shot_id == shot_id);

			// if it hasn't been found, create a new one
			if (!P_found) 
			{
				ADD(calibration->Ps);
				P_id = LAST_INDEX(calibration->Ps);
			}
			else
			{
				ASSERT(calibration->Ps.data[P_id].P, "camera calibration structure without allocated P matrix found");
				cvReleaseMat(&calibration->Ps.data[P_id].P);
			}

			// denormalize P
			if (normalize_data) cvMatMul(H_normalization_inv, P, P);
			
			// save it
			calibration->Ps.data[P_id].P = P;
			calibration->Ps.data[P_id].shot_id = shot_id;

			// also update the estimate of inliers and outliers
			calibration_update_inliers(calibration_id, P_id, points->cols, points_indices, inliers);
		}

		// release resources
		if (normalize_data) cvReleaseMat(&H_normalization_inv);
		FREE(inliers);
		FREE(points_indices);
		cvReleaseMat(&points);
		cvReleaseMat(&vertices);
	}
	UNLOCK_RW(opencv);

	return ok;
}

// decide which 2 shots should be used to start-up calibration and then do so 
bool calibration_auto_begin(
	const unsigned int randomness, 
	const bool normalize_data,
	const bool normalize_A, 
	const double distance_threshold
)
{
	const int len = randomness + 1;

	printf("Starting new calibration.\n");

	// first make sure that meta data are up to date 
	geometry_build_shots_relations();

	// allocate array to keep track of randomness+1 best pairs 
	size_t 
		* const best_pairs = ALLOC(size_t, 2 * (len + 1)),
		* const best_corr = ALLOC(size_t, (len + 1));
	
	size_t best_count = 0;

	// go through all pairs 
	for ALL(shots_relations, i) 
	{
		const Shot_Relations * const shot_relations = shots_relations.data + i; 
		
		for ALL(shot_relations->pair_relations, j) 
		{
			if (i >= j) continue;
			const Shot_Pair_Relation * const pair_relation = shot_relations->pair_relations.data + j; 
			const size_t correspondences = pair_relation->correspondences_count;
			
			// insert the value into sorted array 
			if (best_count > 0)
			{
				size_t no = best_count; // not overflow, since has length len + 1 and best_count <= len
				best_pairs[2 * no + 0] = i; 
				best_pairs[2 * no + 1] = j;
				best_corr[no] = correspondences;
				while (no > 0 && best_corr[no - 1] < correspondences) 
				{
					swap_size_t(best_pairs[2 * no + 0], best_pairs[2 * (no - 1) + 0]); 
					swap_size_t(best_pairs[2 * no + 1], best_pairs[2 * (no - 1) + 1]); 
					swap_size_t(best_corr[no], best_corr[no - 1]);
					no--;
				}
				if (best_count < len) best_count++; 
			}
			else
			{
				best_count = 1;
				best_pairs[0] = i;
				best_pairs[1] = j; 
				best_corr[0] = correspondences;
			}
		}
	}

	// print the list of image pairs with the most correspondences
	printf("  List of image pairs considered to start the calibration:\n");
	printf("  ");
	for (size_t i = 0; i < best_count; i++) 
	{
		// todo this doesn't work with MSVC
		printf("%zd-%zd[%zd] ", best_pairs[2 * i + 0], best_pairs[2 * i + 1], best_corr[i]);
	}
	if (best_count == 0) printf("(empty)");
	printf("\n");

	// calculate how many of these are good enough to estimate fundamental matrix
	size_t sufficient_count; // how many have at least 8 correspondences between them
	for (sufficient_count = 0; sufficient_count < best_count; sufficient_count++)
	{
		if (best_corr[sufficient_count] < 8) break;
	}

	// terminate if there is no suitable pair
	if (sufficient_count == 0)
	{
		printf("  No image pair with enough correspondences.\n");
		FREE(best_pairs);
		FREE(best_corr);
		return false; 
	}

	// otherwise pick randomly any of the suitable image pairs and start the calibration
	const size_t pick = rand() % sufficient_count;

	const size_t shot_id1 = best_pairs[2 * pick + 0], shot_id2 = best_pairs[2 * pick + 1];
	FREE(best_pairs);
	FREE(best_corr);
	ASSERT(validate_shot(shot_id1) && validate_shot(shot_id2), "shot not valid even though it is selected");
	ASSERT(shot_id1 != shot_id2, "selected shots are identical");

	// check that meta of those two shots is loaded 
	if (shots.data[shot_id1].info_status < GEOMETRY_INFO_DEDUCED || shots.data[shot_id2].info_status < GEOMETRY_INFO_DEDUCED)
	{
		printf("  Meta info not loaded. Shouldn't ever happen, but clicking on that photo will resolve this.\n");
		return false;
	}

	// Thanks for reading this :-) -- Lukas 

	// call routine to actually do all the work
	size_t calibration_id;
	LOCK_RW(opencv)
	{
		if (!calibration_pair(shot_id1, shot_id2, normalize_data, normalize_A, distance_threshold, &calibration_id))
		{
			printf("  Failed to calibrate image pair.\n");
			UNLOCK_RW(opencv);
			return false;
		}
	}
	UNLOCK_RW(opencv);

	// refresh UI
	calibration_refresh_UI();

	return true;
}

// extend calibration to another view (if possible)
bool calibration_auto_step(
	const unsigned int randomness, 
	const bool normalize_data,
	const bool normalize_A, 
	const double distance_threshold
)
{
	// check if there is selected calibration 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No partial calibration selected - nothing to extend.\n");
		return false;
	}

	const size_t calibration_id = ui_state.current_calibration;
	Calibration * const calibration = calibrations.data + calibration_id;

	// if this hasn't been refined yet, we could do bundle adjustment 
	if (!calibration->refined && rand() % 3 == 0) 
	{
		// let's refine using nonlinear optimization
		printf("Refining calibration using bundle adjustment.\n");
		calibration_bundle(distance_threshold);
		calibration->refined = true;
		// opencv_begin();    // todo unify calibration_*'s requirement for opencv lock
		// calibration_triangulate_vertices(calibration_id, distance_threshold, 2, normalize_data, normalize_A);
		// opencv_end();
		printf("  Bundle adjustment done.\n");
		calibration_refresh_UI();
		
		return true;
	}
	else
	{
		// * extend the calibration to another camera *

		printf("Extending calibration by resection.\n");
		const size_t len = randomness + 1;

		// mark calibrated shots (we don't care about those)
		calibration_refresh_flag(calibration_id);

		// count how many estimated vertices are on each uncalibrated shot
		size_t * const vertex_count = ALLOC(size_t, shots.count);
		memset(vertex_count, 0, sizeof(size_t) * shots.count);
		for ALL(calibration->Xs, i)
		{
			const size_t vertex_id = calibration->Xs.data[i].vertex_id; 
			
			// go through all points of this vertex
			ASSERT_IS_SET(vertices_incidence, vertex_id);
			for ALL(vertices_incidence.data[vertex_id].shot_point_ids, j)
			{
				const Double_Index * const index = vertices_incidence.data[vertex_id].shot_point_ids.data + j; 
				
				ASSERT(validate_shot(index->primary), "invalid shot encountered");
				if (shots.data[index->primary].partial_calibration) continue;
				ASSERT(index->primary < shots.count, "shot index out of bounds");
				vertex_count[index->primary]++;
			}
		}

		// now pick 'len' best shots 
		// allocate array to keep track of randomness+1 best pairs 
		size_t 
			* const best_shot = ALLOC(size_t, len + 1),
			* const best_corr = ALLOC(size_t, len + 1);
		size_t best_count = 0;
	
		// go through all available shots
		bool uncalibrated = false;
		for ALL(shots, i) 
		{
			const Shot * const shot = shots.data + i; 
			if (shot->partial_calibration) continue;
			uncalibrated = true;

			const size_t correspondences = vertex_count[i];
			
			// insert the value into sorted array 
			if (best_count > 0)
			{
				size_t no = best_count; // not overflow, since has length len + 1 and best_count <= len
				best_shot[no] = i; 
				best_corr[no] = correspondences;
				while (no > 0 && best_corr[no - 1] < correspondences) 
				{
					swap_size_t(best_shot[no], best_shot[no - 1]); 
					swap_size_t(best_corr[no], best_corr[no - 1]);
					no--;
				}
				if (best_count < len) best_count++; 
			}
			else
			{
				best_count = 1;
				best_shot[0] = i;
				best_corr[0] = correspondences;
			}
		}

		// is there an uncalibrated shot? 
		if (!uncalibrated) 
		{
			printf("  All shots are calibrated.\n");
			FREE(best_shot); 
			FREE(best_corr); 
			return false;
		}

		// print the list of images with the largest amount of estimated vrtices 
		printf("  List images considered:\n");
		printf("  ");
		for (size_t i = 0; i < best_count; i++) 
		{
			printf("%zd[%zd] ", best_shot[i], best_corr[i]);
		}
		if (best_count == 0) printf("(empty)");
		printf("\n");
	
		// calculate how many of these are good enough to perform resection
		size_t sufficient_count; // how many have at least 8 correspondences between them
		for (sufficient_count = 0; sufficient_count < best_count; sufficient_count++)
		{
			if (best_corr[sufficient_count] < 6) break;
		}
	
		// terminate if there is no suitable pair
		if (sufficient_count == 0)
		{
			printf("  No image with enough reconstructed vertices.\n");
			return false; 
		}
		
		// try to perform resection 
		for (int i = 0; i < 5; i++) 
		{
			const size_t pick = rand() % sufficient_count;
			const size_t shot_to_resect = best_shot[pick];

			printf("  Resecting image %zd.\n", shot_to_resect); 
			const bool status = calibration_add_view(calibration_id, shot_to_resect, distance_threshold, normalize_data, normalize_A);
			
			if (status) 
			{
				calibration->refined = false;
				printf("  Resection performed.\n");
				FREE(best_shot);
				FREE(best_corr);
				calibration_refresh_UI();
				calibration_triangulate_vertices(calibration_id, distance_threshold, 2, normalize_data, normalize_A);
				return true;
			}

			printf("  Failed to resect.\n");
		}

		FREE(best_shot); 
		FREE(best_corr);

		return false;
	}
	
	return false;
}

// perform final nonlinear refinement and metric stratification 
bool calibration_auto_end(
	const unsigned int randomness, // note that these parameters are not currently used
	const bool normalize_data,
	const bool normalize_A, 
	const double distance_threshold
)
{
	// check if there is selected calibration 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No partial calibration selected - nothing to finalize.\n");
		return false;
	}

	const size_t calibration_id = ui_state.current_calibration;
	Calibration * const calibration = calibrations.data + calibration_id;

	// final refinement
	printf("Refining calibration using bundle adjustment.\n");
	calibration_bundle(distance_threshold);
	//opencv_begin();    // todo unify calibration_*'s requirement for opencv lock
	//calibration_triangulate_vertices(calibration_id, distance_threshold, 2, normalize_data, normalize_A);
	//opencv_end();

	// metric stratification 
	calibration_rectify(true);

	// * use calibration * // note similarity with code in tool_calibration_use

	// clear all calibration matrices
	geometry_release_shots_calibrations();

	// copy all calibrated projection matrices into workspace
	for ALL(calibrations.data[calibration_id].Ps, i)
	{
		const Calibration_Camera * P = calibrations.data[calibration_id].Ps.data + i; 
		ASSERT(validate_shot(P->shot_id), "invalid shot referenced in partial calibration");

		shots.data[P->shot_id].projection = opencv_create_matrix(3, 4);
		ATOMIC_RW(opencv, cvCopy(P->P, shots.data[P->shot_id].projection); );

		// decompose all matrices into KR[I|-t]
		geometry_calibration_from_P(P->shot_id);

		// finally mark as calibrated 
		shots.data[P->shot_id].calibrated = true;
	}

	// update visualization
	visualization_process_data(vertices, shots);

	// refresh UI
	calibration_refresh_UI();

	// move from calibration to main workspace 
	INDEX_CLEAR(ui_state.current_calibration);

	// pick the first calibrated camera and align the coordinate frame 
	bool aligned = false; 
	for ALL(shots, i) 
	{
		const Shot * shot = shots.data + i; 
		if (shot->calibrated) 
		{
			coordinates_rotate_all_cameras(i);
			aligned = true; 
			break;
		}
	}

	if (!aligned) 
	{
		printf("Warning: the coordinate frame couldn't be aligned with any calibrated camera (is there one?). Expect mirroring effects!\n");
	}

	return true;
}

// completely automatic calibration 
bool calibration_auto(
	const unsigned int randomness, 
	const bool normalize_data,
	const bool normalize_A, 
	const double distance_threshold
)
{
	tool_start_progressbar();

	if (calibration_auto_begin(randomness, normalize_data, normalize_A, distance_threshold))
	{
		int step = 0; 

		while (calibration_auto_step(randomness, normalize_data, normalize_A, distance_threshold))
		{
			step++; 
			tool_show_progress((step % 10) * 0.1);
		}

		const bool result = calibration_auto_end(randomness, normalize_data, normalize_A, distance_threshold);
		tool_show_progress((++step % 10) * 0.1);
		tool_end_progressbar();

		return true;
	}
	else
	{
		tool_end_progressbar();
		return false;
	}
}

// automatic calibration 
void tool_calibration_auto()
{
	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double distance_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);
	const int randomness = tool_get_int(tool_calibration_id, CALIBRATION_RANDOMNESS);

	printf("Automatic calibration started\n"); 
	calibration_auto(randomness, normalize_data, normalize_A, distance_threshold);
}

void tool_calibration_auto_begin()
{
	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double distance_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);
	const int randomness = tool_get_int(tool_calibration_id, CALIBRATION_RANDOMNESS);

	calibration_auto_begin(randomness, normalize_data, normalize_A, distance_threshold);
}

void tool_calibration_auto_step()
{
	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double distance_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);
	const int randomness = tool_get_int(tool_calibration_id, CALIBRATION_RANDOMNESS);

	calibration_auto_step(randomness, normalize_data, normalize_A, distance_threshold);
}

void tool_calibration_auto_end()
{
	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double distance_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);
	const int randomness = tool_get_int(tool_calibration_id, CALIBRATION_RANDOMNESS);

	calibration_auto_end(randomness, normalize_data, normalize_A, distance_threshold);
}

// add current view by resection 
void tool_calibration_add_views()
{
	// get settings
	const size_t calibration_id = ui_state.current_calibration;

	tool_fetch_parameters(tool_calibration_id);
	const double measurement_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);

	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	// go through all shots and process the selected ones
	size_t count = 0; 
	for ALL(shots, i)
	{
		// calibrate the selected ones
		if (ui_check_shot_meta(i)->selected)
		{
			if (shots.data[i].info_status < GEOMETRY_INFO_DEDUCED)
			{
				printf("Meta not loaded.\n");
				continue;
			}

			calibration_add_view(ui_state.current_calibration, i, measurement_threshold, normalize_data, normalize_A);
			count++;
		}

		// note that we're not performing triangulation when resecting more than one image
		// (with current UI, it is not even possible)
	}

	// if no shot was selected, resect the current one 
	if (count == 0 && INDEX_IS_SET(ui_state.current_shot))
	{
		if (shots.data[ui_state.current_shot].info_status < GEOMETRY_INFO_DEDUCED)
		{
			printf("Meta not loaded.\n");
			return;
		}

		calibration_add_view(ui_state.current_calibration, ui_state.current_shot, measurement_threshold, normalize_data, normalize_A);
		calibration_triangulate_vertices(calibration_id, measurement_threshold, 2, normalize_data, normalize_A, ui_state.current_shot);
	}

	// update calibrated flag 
	calibration_refresh_UI();
}

// print the projection matrices in estimated in current calibration 
void tool_calibration_print() 
{
	// if no partial calibration is selected, print the main calibration 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		// go through all shots 
		for ALL(shots, i) 
		{
			const Shot * const shot = shots.data + i; 

			opencv_debug("projection matrix", shot->projection); 
			opencv_debug("internal calibration", shot->internal_calibration); 
		}
	}
	else
	{
		Calibration * const calibration = calibrations.data + ui_state.current_calibration;

		// print the projection matrices
		for ALL(calibration->Ps, i)
		{
			LOCK(opencv)
			{
				CvMat 
					* I = opencv_create_matrix(3, 3), 
					* R = opencv_create_matrix(3, 3), 
					* T = opencv_create_matrix(3, 1)
				;

				// decompose projection matrix
				const bool finite = mvg_finite_projection_matrix_decomposition(
					calibration->Ps.data[i].P, I, R, T
				);

				// if the camera is finite, let's see what we can do to take it into normal form
				if (finite) 
				{
					// now everything should be as before, but we can be sure the matrix is decomposed 
					opencv_debug("projection matrix", calibration->Ps.data[i].P);
					opencv_debug("internal calibration", I);
				}

				cvReleaseMat(&I); 
				cvReleaseMat(&R); 
				cvReleaseMat(&T); 
			}
			UNLOCK(opencv);
		}
	}
}

// test how close are we to metric reconstruction by decomposing the projection matrices 
// and equating specific elements of the internal calibration matrix to ones or zeros 
void tool_calibration_test_rectification() 
{
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	Calibration * const calibration = calibrations.data + ui_state.current_calibration;

	// count the number of calibrated cameras 
	for ALL(calibration->Ps, i)
	{
		// decompose the projection matrix 
		LOCK_RW(opencv)
		{
			CvMat 
				* I = opencv_create_matrix(3, 3), 
				* R = opencv_create_matrix(3, 3), 
				* T = opencv_create_matrix(3, 1)
			;

			// decompose projection matrix
			const bool finite = mvg_finite_projection_matrix_decomposition(
				calibration->Ps.data[i].P, I, R, T
			);

			// if the camera is finite, let's see what we can do to take it into normal form
			if (finite) 
			{
				// now everything should be as before, but we can be sure the matrix is decomposed 
				opencv_debug("Internal calibration", I);

				// normalize the internal calibration a little 
				const double 
					fm1 = OPENCV_ELEM(I, 0, 0), 
					fm2 = OPENCV_ELEM(I, 1, 1)
				;

				OPENCV_ELEM(I, 0, 0) = (10 * fm1 + fm2) / 11.0; 
				OPENCV_ELEM(I, 1, 1) = (fm1 + 10 * fm2) / 11.0; 
				OPENCV_ELEM(I, 0, 1) = OPENCV_ELEM(I, 0, 1) / 2.0;

				// show internal calibration matrix after adjustments 
				opencv_debug("After rectification", I);

				// rebuild the projection matrix again 
				mvg_assemble_projection_matrix(I, R, T, calibration->Ps.data[i].P);
			}

			cvReleaseMat(&I); 
			cvReleaseMat(&R); 
			cvReleaseMat(&T); 
		}
		UNLOCK_RW(opencv);
	}
}

// internal routine used to rectify to metric/affine reconstruction using the knowledge of principal point, 
// aspect ratio and zero skew
// todo investigate the need for opencv lock
void calibration_rectify(bool affine)
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	Calibration * const calibration = calibrations.data + ui_state.current_calibration;

	// count the number of calibrated cameras 
	size_t count = 0; 
	for ALL(calibration->Ps, i)
	{
		count++;
		
		if (shots.data[calibration->Ps.data[i].shot_id].info_status < GEOMETRY_INFO_DEDUCED)
		{
			printf("Looks like some images couldn't be loaded and thus their sizes aren't known.\n");
			return;
		}
	}

	if (count < 3) 
	{
		printf("Not enough views to perform autocalibration into metric space.");
		return;
	}

	// allocate array to store principal points 
	double * principal_points = ALLOC(double, 2 * count);
	CvMat ** Ps = ALLOC(CvMat *, count);
	CvMat ** Xs = ALLOC(CvMat *, calibration->Xs.count);

	// fill pointers to all points 
	size_t point_count = 0;
	for ALL(calibration->Xs, i) 
	{
		Xs[point_count++] = calibration->Xs.data[i].X;
	}

	// fill the principal points and the array of projection matrices
	size_t j = 0;
	for ALL(calibration->Ps, i)
	{
		principal_points[2 * j + 0] = shots.data[calibration->Ps.data[i].shot_id].width / 2.0;
		principal_points[2 * j + 1] = shots.data[calibration->Ps.data[i].shot_id].height / 2.0;
		Ps[j++] = calibration->Ps.data[i].P;
	}

	ASSERT(j == count, "inconsistency in values of counters");

	// perform autocalibration 
	// CvMat * H = opencv_create_matrix(4, 4);
	CvMat * pi_inf;
	ATOMIC_RW(opencv, mvg_autocalibration_2(Ps, principal_points, count, Xs, point_count, &pi_inf); );
	FREE(Xs);
	FREE(Ps);

	// save the plane at infinity 
	if (calibration->pi_infinity)
	{
		ATOMIC_RW(opencv, cvReleaseMat(&calibration->pi_infinity); );
	}
	calibration->pi_infinity = pi_inf;

	// release resources 
	FREE(principal_points);
}

// stratify to metric reconstruction 
void tool_calibration_metric()
{
	calibration_rectify(false);
}

// stratify to affine reconstruction 
void tool_calibration_affine()
{
	calibration_rectify(true);
}

void tool_calibration_use()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	const size_t calibration_id = ui_state.current_calibration;

	// perform metric stratification
	// calibration_rectify(false);

	// clear all calibration matrices
	geometry_release_shots_calibrations();

	// copy all calibrated projection matrices into workspace
	for ALL(calibrations.data[calibration_id].Ps, i)
	{
		const Calibration_Camera * P = calibrations.data[calibration_id].Ps.data + i; 
		ASSERT(validate_shot(P->shot_id), "invalid shot referenced in partial calibration");

		LOCK_RW(opencv)
		{
			shots.data[P->shot_id].projection = opencv_create_matrix(3, 4);
			cvCopy(P->P, shots.data[P->shot_id].projection);
		}
		UNLOCK_RW(opencv);

		// decompose all matrices into KR[I|-t]
		geometry_calibration_from_P(P->shot_id);

		// finally mark as calibrated 
		shots.data[P->shot_id].calibrated = true;
	}

	// update visualization
	visualization_process_data(vertices, shots);

	// copy all triangulated vertices // note we might do this, but there's no real reason
	/*for ALL(calibrations.data[calibration_id].Xs, i) 
	{
		Calibration_Vertex * vertex = calibrations.data[calibration_id].Xs.data + i; 
		ASSERT_IS_SET(vertices, vertex->vertex_id);
		ASSERT(vertex->X, "calibration vertex contains undefined vector");

		const double w = OPENCV_ELEM(vertex->X, 3, 0);
		if (!nearly_zero(w))
		{
			vertices.data[vertex->vertex_id].x = OPENCV_ELEM(vertex->X, 0, 0) / w;
			vertices.data[vertex->vertex_id].y = OPENCV_ELEM(vertex->X, 1, 0) / w;
			vertices.data[vertex->vertex_id].z = OPENCV_ELEM(vertex->X, 2, 0) / w;
			vertices.data[vertex->vertex_id].reconstructed = true;
		}
		else
		{
			vertices.data[vertex->vertex_id].reconstructed = false;
		}
	}*/

	// move to main workspace 
	INDEX_CLEAR(ui_state.current_calibration);
}

// internal routine used to triangulate vertex and revise its credibility
void calibration_triangulate_vertex(
	const size_t calibration_id, const size_t vertex_id, const double measurement_threshold, const int min_inliers,
	const bool normalize_data, const bool normalize_A
)
{
	ASSERT_IS_SET(calibrations, calibration_id);
	ASSERT(validate_vertex(vertex_id), "tried to triangulate invalid vertex");
	Calibration * calibration = calibrations.data + calibration_id;

	// publish triangulation data 
	const CvMat * * projection_matrices_original; 
	CvMat * * projection_matrices_normalized = NULL;
	CvMat * points;
	size_t * indices;
	if (publish_triangulation_data_from_calibration(calibration_id, vertices_incidence.data[vertex_id], vertex_id, projection_matrices_original, points, indices))
	{
		// triangulation data successfully obtained
		bool * inliers = ALLOC(bool, points->cols);
		CvMat * X;

		// optionally normalize
		double scale = 1;
		if (normalize_data)
		{
			LOCK_RW(opencv)
			{
				// normalize the points
				CvMat * H_normalization = opencv_create_matrix(3, 3);
				mvg_normalize_points(points, H_normalization, &scale);
				projection_matrices_normalized = ALLOC(CvMat *, points->cols);

				// transform projection matrices accordingly
				for (int i = 0; i < points->cols; i++)
				{
					CvMat * M = opencv_create_matrix(3, 4);
					cvMatMul(H_normalization, projection_matrices_original[i], M);
					projection_matrices_normalized[i] = M;
				}
				cvReleaseMat(&H_normalization);
			}
			UNLOCK_RW(opencv);
		}

		// try to find the vertex in existing dataset
		size_t X_id; 
		bool X_found;
		LAMBDA_FIND(calibration->Xs, X_id, X_found, calibration->Xs.data[X_id].vertex_id == vertex_id);
		Calibration_Vertex * vertex = NULL;
		if (X_found)
		{
			vertex = calibration->Xs.data + X_id;
		}

		LOCK_RW(opencv)
		{
			if (
				points->cols >= 2 &&
				(
					X = mvg_triangulation_RANSAC(
							normalize_data ? (const CvMat **)projection_matrices_normalized : projection_matrices_original, 
							points, false, normalize_A, min_inliers, min_inliers, 5, // note magic constant
							measurement_threshold * scale, inliers)
					/*(X = mvg_triangulation_SVD(
						normalize_data ? (const CvMat **)projection_matrices_normalized : projection_matrices_original, 
						points, normalize_A, min_inliers, 0, -1
					)*/
				)
			)
			{
				// vertex has been triangulated - save its coordinates
				if (vertex) 
				{
					if (vertex->X) cvReleaseMat(&vertex->X);
					vertex->X = X;
				}
				else
				{
					ADD(calibration->Xs);
					vertex = calibration->Xs.data + LAST_INDEX(calibration->Xs);
					vertex->vertex_id = vertex_id;
					vertex->X = X;
				}

				// also update the set of inliers and outliers 
				calibration_update_inliers(calibration_id, points->cols, indices, inliers);
			}
			else if (vertex)
			{
				if (vertex->X) cvReleaseMat(&vertex->X);
				vertex->set = false;

				// mark the inliers and outliers anyway
				calibration_update_inliers(calibration_id, points->cols, indices, inliers);
			}

			// release resources 
			if (normalize_data)
			{
				for (int i = 0; i < points->cols; i++)
				{
					cvReleaseMat(projection_matrices_normalized + i);
				}
				FREE(projection_matrices_normalized);
			}
			FREE(indices);
			FREE(projection_matrices_original);
			cvReleaseMat(&points);
		}
		UNLOCK_RW(opencv);
	}
}

// triangulate all vertices (internal routine)
void calibration_triangulate_vertices(
	const size_t calibration_id, const double measurement_threshold, const int min_inliers,
	const bool normalize_data, const bool normalize_A, const int shot_id
)
{
	// go through all vertices either on current photo or all of them 
	if (shot_id < 0) 
	{
		for ALL(vertices, i) 
		{
			// try to triangulate it 
			calibration_triangulate_vertex(calibration_id, i, measurement_threshold, 2, normalize_data, normalize_A);
		}
	}
	else
	{
		for ALL(shots.data[shot_id].points, i)
		{
			const Point * const point = shots.data[shot_id].points.data + i;

			// try to triangulate it 
			calibration_triangulate_vertex(calibration_id, point->vertex, measurement_threshold, 2, normalize_data, normalize_A);
		}
	}
}

// triangulate vertices 
void tool_calibration_triangulate()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	const size_t calibration_id = ui_state.current_calibration;

	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double measurement_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);

	calibration_triangulate_vertices(calibration_id, measurement_threshold, 2, normalize_data, normalize_A);
}

// triangulate vertices with at least 3 inlying points corresponding points
void tool_calibration_triangulate_trusted()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	const size_t calibration_id = ui_state.current_calibration;

	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double measurement_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);

	// go through all vertices and triangulate
	for ALL(vertices, i) 
	{
		// try to triangulate it 
		calibration_triangulate_vertex(calibration_id, i, measurement_threshold, 3, normalize_data, normalize_A);
	}
}

// refine existing calibration using different thresholding levels 
void calibration_refine(bool strict)
{
	ASSERT(INDEX_IS_SET(ui_state.current_shot), "current shot must be set");

	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double measurement_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);

	const size_t calibration_id = ui_state.current_calibration;

	// update calibrated flag 
	calibration_refresh_flag(calibration_id);

	// resection of selected shots
	for ALL(shots, i) 
	{
		if (ui_check_shot_meta(i)->selected && !shots.data[i].partial_calibration)
		{
			calibration_add_view(calibration_id, i, measurement_threshold, normalize_data, normalize_A);
		}
	}

	// update UI 
	calibration_refresh_UI();

	if (!strict) 
	{
		// try 3 more and more restrictive thresholds
		for (int i = 0; i < 3; i++) 
		{
			const double threshold = measurement_threshold / (double)i;
			printf("refine");

			// do a couple of refinements for this threshold 
			for (int j = 0; j < 3; j++)
			{
				// first triangulate the vertices
				calibration_triangulate_vertices(calibration_id, threshold, 2, normalize_data, normalize_A);

				// then call resectioning routine on all cameras
				for ALL(shots, i)
				{
					if (shots.data[i].calibrated)
					{
						calibration_add_view(calibration_id, i, threshold, normalize_data, normalize_A);
					}
				}

				printf(".");
			}

			printf("\n");
		}
	}
	else
	{
		// do some refinements using strict threshold
		printf("refine");
		for (int j = 0; j < 4; j++)
		{
			// first triangulate the vertices
			calibration_triangulate_vertices(calibration_id, 1, 2, normalize_data, normalize_A);

			// then call resectioning routine on all cameras
			for ALL(shots, i)
			{
				if (shots.data[i].calibrated)
				{
					calibration_add_view(calibration_id, i, 1.0, normalize_data, normalize_A);
				}
			}

			printf(".");
		}

		printf("\n");
	}
}

void tool_calibration_refine()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	calibration_refine(false);
}

void tool_calibration_refine_strict()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	calibration_refine(true);
}

// export reconstruction from current calibration into VRML
void tool_calibration_debug_export()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	// export it
	geometry_export_vrml_calibration("./debug.vrml", calibrations.data[ui_state.current_calibration]);
}

// clear 
void tool_calibration_clear()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration) || !INDEX_IS_SET(ui_state.current_shot))
	{
		printf("No calibration or shot selected.\n");
		return;
	}

	// find current shot's calibration and erase it 
	bool found;
	size_t id;
	LAMBDA_FIND(calibrations.data[ui_state.current_calibration].Ps, id, found, calibrations.data[ui_state.current_calibration].Ps.data[id].shot_id == ui_state.current_shot);

	if (found)
	{
		calibrations.data[ui_state.current_calibration].Ps.data[id].set = false;
	}

	// update calibrated flag 
	calibration_refresh_UI();
}

// calculating projection of vertex using the current camera 
void calibration_projection(int j, int i, double * aj, double * bi, double * xij, void * adata)
{
	double w = aj[2 * 4 + 0] * bi[0] + aj[2 * 4 + 1] * bi[1] + aj[2 * 4 + 2] * bi[2] + aj[2 * 4 + 3] * bi[3];
	if (w == 0) w = 0.00000001; // note dirty...
	xij[0] = (aj[0 * 4 + 0] * bi[0] + aj[0 * 4 + 1] * bi[1] + aj[0 * 4 + 2] * bi[2] + aj[0 * 4 + 3] * bi[3]) / w;
	xij[1] = (aj[1 * 4 + 0] * bi[0] + aj[1 * 4 + 1] * bi[1] + aj[1 * 4 + 2] * bi[2] + aj[1 * 4 + 3] * bi[3]) / w;

	// print the error
	/*const size_t INDEX = I[j * C + i] * 2;
	const double dx = M[INDEX + 0] - bi[0], dy = M[INDEX + 1] - bi[1]; 
	const double d = sqrt(dx * dx + dy * dy);
	printf("[%f]", d);*/
}

// calculating jacobian of the cost function 
void calibration_jacobian(int j, int i, double * aj, double * bi, double * Aij, double * Bij, void * adata)
{
	// clear with zeros
	memset(Aij, 0, sizeof(double) * 24);
	memset(Bij, 0, sizeof(double) * 8);

	// compute partial derivatives along a_j
	Aij[0]  = bi[0] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[1]  = bi[1] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[2]  = bi[2] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[3]  = bi[3] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);

	const double C = aj[0] * bi[0] + aj[1] * bi[1] + aj[2] * bi[2] + aj[3] * bi[3];
	Aij[8]  = -C * bi[0] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[9]  = -C * bi[1] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[10] = -C * bi[2] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[11] = -C * bi[3] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);

	Aij[16]  = bi[0] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[17]  = bi[1] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[18]  = bi[2] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[19]  = bi[3] / (aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);

	const double D = aj[4] * bi[0] + aj[5] * bi[1] + aj[6] * bi[2] + aj[7] * bi[3];
	Aij[20] = -D * bi[0] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[21] = -D * bi[1] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[22] = -D * bi[2] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);
	Aij[23] = -D * bi[3] / sqr_value(aj[8] * bi[0] + aj[9] * bi[1] + aj[10] * bi[2] + aj[11] * bi[3]);

	// compute partial derivatives along b_i
	double w = aj[2 * 4 + 0] * bi[0] + aj[2 * 4 + 1] * bi[1] + aj[2 * 4 + 2] * bi[2] + aj[2 * 4 + 3] * bi[3];
	if (w == 0) w = 0.00000001; // note dirty...
	Bij[0] = ( aj[0] * w - aj[8]  * (aj[0 * 4 + 0] * bi[0] + aj[0 * 4 + 1] * bi[1] + aj[0 * 4 + 2] * bi[2] + aj[0 * 4 + 3] * bi[3]) ) / sqr_value(w);
	Bij[1] = ( aj[1] * w - aj[9]  * (aj[0 * 4 + 0] * bi[0] + aj[0 * 4 + 1] * bi[1] + aj[0 * 4 + 2] * bi[2] + aj[0 * 4 + 3] * bi[3]) ) / sqr_value(w);
	Bij[2] = ( aj[2] * w - aj[10] * (aj[0 * 4 + 0] * bi[0] + aj[0 * 4 + 1] * bi[1] + aj[0 * 4 + 2] * bi[2] + aj[0 * 4 + 3] * bi[3]) ) / sqr_value(w);
	Bij[3] = ( aj[3] * w - aj[11] * (aj[0 * 4 + 0] * bi[0] + aj[0 * 4 + 1] * bi[1] + aj[0 * 4 + 2] * bi[2] + aj[0 * 4 + 3] * bi[3]) ) / sqr_value(w);

	Bij[4] = ( aj[4] * w - aj[8]  * (aj[1 * 4 + 0] * bi[0] + aj[1 * 4 + 1] * bi[1] + aj[1 * 4 + 2] * bi[2] + aj[1 * 4 + 3] * bi[3]) ) / sqr_value(w);
	Bij[5] = ( aj[5] * w - aj[9]  * (aj[1 * 4 + 0] * bi[0] + aj[1 * 4 + 1] * bi[1] + aj[1 * 4 + 2] * bi[2] + aj[1 * 4 + 3] * bi[3]) ) / sqr_value(w);
	Bij[6] = ( aj[6] * w - aj[10] * (aj[1 * 4 + 0] * bi[0] + aj[1 * 4 + 1] * bi[1] + aj[1 * 4 + 2] * bi[2] + aj[1 * 4 + 3] * bi[3]) ) / sqr_value(w);
	Bij[7] = ( aj[7] * w - aj[11] * (aj[1 * 4 + 0] * bi[0] + aj[1 * 4 + 1] * bi[1] + aj[1 * 4 + 2] * bi[2] + aj[1 * 4 + 3] * bi[3]) ) / sqr_value(w);
}

// run bundle adjustment 
void calibration_bundle(const double measurement_threhsold) 
{
	const size_t BA_CAMERA_PARAMETERS = 12;

	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	const size_t calibration_id = ui_state.current_calibration;
	Calibration * const calibration = calibrations.data + calibration_id;

	// update calibrated flag 
	calibration_refresh_flag(calibration_id);

	// * build input for bundle adjustment routine *

	// count the number of vertices and cameras
	int Xs_count = 0, Ps_count = 0; 
	{
		size_t i; // strong todo why can't labda define the iterator? maybe to break from inside of it and read last iterated index?
		LAMBDA(calibration->Xs, i, Xs_count++; );
		LAMBDA(calibration->Ps, i, Ps_count++; );
	}

	// allocate memory for the visibility mask 
	const int maximum_count = Xs_count * Ps_count;
	char * visibility_mask = ALLOC(char, maximum_count);
	memset(visibility_mask, 0, sizeof(char) * maximum_count);
	double * measurement = ALLOC(double, maximum_count * 2); // note isn't this potentionally large?

	// precompute index from shot_ids to order in Calibration_Cameras array 
	size_t * shots_order = ALLOC(size_t, shots.count); 
	for (size_t i = 0; i < shots.count; i++) shots_order[i] = SIZE_MAX;

	{
		size_t k, j = 0;
		LAMBDA(
			calibration->Ps, k, 
			ASSERT(calibration->Ps.data[k].shot_id < shots.count, "invalid shot index");
			shots_order[calibration->Ps.data[k].shot_id] = j++;
		);
		// note that the domain of the shots_order mapping is defined by the 
		//      partial_calibration flag - therefore, it must be kept consistent
		// note could we have a dedicated function for this stuff (generally)? 
		// something like: INDEX(shots_order, shots, calibration->Ps)
	}

	// go through all vertices and generate visibility mask and measurement vector 
	size_t measurement_count = 0, X_count = 0;
	char * vertex_visibility = ALLOC(char, Ps_count);
	size_t * incidence_ids = ALLOC(size_t, Ps_count);
	
	for ALL(calibration->Xs, i)
	{
		Calibration_Vertex * const X = calibration->Xs.data + i;
		const size_t vertex_id = X->vertex_id;

		ASSERT_IS_SET(vertices_incidence, vertex_id);

		// go through all photos with this vertex and decide which ones will be inserted 
		memset(vertex_visibility, 0, sizeof(char) * Ps_count);
		for ALL(vertices_incidence.data[vertex_id].shot_point_ids, j)
		{
			const Double_Index * const index = vertices_incidence.data[vertex_id].shot_point_ids.data + j;
			const Shot * const shot = shots.data + index->primary;

			// we care only about photos in this calibration (we updated the calibrated flag, remember?)
			if (shot->partial_calibration) // note is this done right? will the compiler join this and the following ifs in release? // obsolete comment
			{
				if (!(IS_SET(calibration->Ps.data[shots_order[index->primary]].points_meta, index->secondary))) continue;
				
				// and we also discard outliers
				if (calibration->Ps.data[shots_order[index->primary]].points_meta.data[index->secondary].inlier == 1)
				{
					ASSERT(index->primary < shots.count, "invalid shot index");
					ASSERT(shots_order[index->primary] < Ps_count, "invalid shot order index");
					vertex_visibility[shots_order[index->primary]] = 1; 
					incidence_ids[shots_order[index->primary]] = j;
				}
			}
		}

		// set visibility 
		ASSERT(X_count * Ps_count + Ps_count <= maximum_count, "accessing elements outside of visibility mask");
		memcpy(visibility_mask + (X_count * Ps_count), vertex_visibility, sizeof(char) * Ps_count);

		// insert their values into measurement vector
		for (size_t j = 0; j < Ps_count; j++) 
		{
			// skip invisible
			if (vertex_visibility[j] != 1) continue;

			const size_t incidence_id = incidence_ids[j];
			ASSERT_IS_SET(vertices_incidence.data[vertex_id].shot_point_ids, incidence_id);
			const Double_Index * const index = vertices_incidence.data[vertex_id].shot_point_ids.data + incidence_id;
			const Shot * const shot = shots.data + index->primary; 

			// the meassurement will be used
			ASSERT_IS_SET(shot->points, index->secondary);
			// printf("[%d/%d] ", measurement_count * 2 + 1, maximum_count * 2);
			ASSERT(measurement_count * 2 + 1 < 2 * maximum_count, "measurement index out of bounds");
			measurement[measurement_count * 2 + 0] = shot->points.data[index->secondary].x * shot->width;
			measurement[measurement_count * 2 + 1] = shot->points.data[index->secondary].y * shot->height;
			measurement_count++;
		}

		// increase the counter of vertices in calibration 
		X_count++;
		ASSERT(X_count <= Xs_count, "inconsistent counters");
	}

	// * build vector of parameters *

	const size_t parameters_count = Ps_count * BA_CAMERA_PARAMETERS + Xs_count * 4;
	double * parameters = ALLOC(double, parameters_count); 

	size_t Ps_i = 0;
	for ALL(calibration->Ps, i)
	{
		const Calibration_Camera * camera = calibration->Ps.data + i;
		for (int j = 0; j < 12; j++) 
		{
			const double d = OPENCV_ELEM(camera->P, j / 4, j % 4);
			ASSERT(camera->shot_id < shots.count, "invalid shot index"); 
			ASSERT(shots_order[camera->shot_id] * BA_CAMERA_PARAMETERS + j < parameters_count, "parameter index out of bounds");
			parameters[shots_order[camera->shot_id] * BA_CAMERA_PARAMETERS + j] = d;
		}
		Ps_i++; 
	}
	ASSERT(Ps_i == Ps_count, "inconsistent counters");

	const size_t parameter_offset = Ps_count * BA_CAMERA_PARAMETERS;
	size_t Xs_i = 0; 
	for ALL(calibration->Xs, i) 
	{
		const Calibration_Vertex * vertex = calibration->Xs.data + i;
		for (int j = 0; j < 4; j++) 
		{
			ASSERT(parameter_offset + Xs_i * 4 + j < parameters_count, "parameter index out of bounds");
			parameters[parameter_offset + Xs_i * 4 + j] = OPENCV_ELEM(vertex->X, j, 0);
		}
		Xs_i++;
	}
	ASSERT(Xs_i == Xs_count, "inconsistent counters");

	// * input verification * 

	// go through all points and print out their reprojection error (or something)
	/*
	{
		size_t m = 0; 
		double sum_err = 0;

		size_t * Xs_reindex = (size_t *)malloc(sizeof(size_t) * calibration->Xs.count); 
		{
			size_t i, j = 0;
			LAMBDA(calibration->Xs, i, Xs_reindex[j++] = i; ); 
		}

		size_t * Ps_reindex = (size_t *)malloc(sizeof(size_t) * calibration->Ps.count);
		{
			size_t i, j = 0;
			LAMBDA(calibration->Ps, i, Ps_reindex[j++] = i; ); 
		}

		for (size_t i = 0; i < Xs_count; i++)
		{
			for (size_t j = 0; j < Ps_count; j++)
			{
				// if this value has been measured
				if (visibility_mask[i * Ps_count + j] == 1)
				{
					// project vertex
					double pw =
						parameters[j * BA_CAMERA_PARAMETERS + 2 * 4 + 0] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 0] +
						parameters[j * BA_CAMERA_PARAMETERS + 2 * 4 + 1] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 1] +
						parameters[j * BA_CAMERA_PARAMETERS + 2 * 4 + 2] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 2] +
						parameters[j * BA_CAMERA_PARAMETERS + 2 * 4 + 3] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 3]
					;

					double px = (
						parameters[j * BA_CAMERA_PARAMETERS + 0 * 4 + 0] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 0] + 
						parameters[j * BA_CAMERA_PARAMETERS + 0 * 4 + 1] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 1] + 
						parameters[j * BA_CAMERA_PARAMETERS + 0 * 4 + 2] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 2] + 
						parameters[j * BA_CAMERA_PARAMETERS + 0 * 4 + 3] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 3]
					) / pw;

					double py = (
						parameters[j * BA_CAMERA_PARAMETERS + 1 * 4 + 0] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 0] + 
						parameters[j * BA_CAMERA_PARAMETERS + 1 * 4 + 1] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 1] + 
						parameters[j * BA_CAMERA_PARAMETERS + 1 * 4 + 2] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 2] + 
						parameters[j * BA_CAMERA_PARAMETERS + 1 * 4 + 3] * parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 3]
					) / pw;

					// calculate error 
					const double dx = measurement[2 * m + 0] - px, dy = measurement[2 * m + 1] - py; 
					const double d = sqrt(dx * dx + dy * dy);
					if (d > 144) 
					{
						printf("=== Sumtyn fishy about a vertex with squared residual %f: ===\n", d);
					}
					sum_err += d * d;

					{
						// find which shot this is 
						size_t shot_id; 
						bool found; 
						LAMBDA_FIND(shots, shot_id, found, shots_order[shot_id] == j); 
						ASSERT(found, "shot not found");

						// find which point is it (i.e., what's the measurement)
						size_t point_id; 
						LAMBDA_FIND(shots.data[shot_id].points, point_id, found, shots.data[shot_id].points.data[point_id].vertex == calibration->Xs.data[Xs_reindex[i]].vertex_id);
						ASSERT(found, "point not found");

						// does the measurement align with what's saved in the measurement vector? 
						/*printf("Measured data in shots variable:\n%f\n%f\n", 
							shots.data[shot_id].points.data[point_id].x * shots.data[shot_id].width, 
							shots.data[shot_id].points.data[point_id].y * shots.data[shot_id].height
						);

						printf("Measured data in measurement vector:\n%f\n%f\n", 
							measurement[2 * m + 0], 
							measurement[2 * m + 1]
						);* /

						// is it marked as inlier? 
						// printf((calibration->Ps.data[Ps_reindex[j]].points_meta.data[shot_id].inlier == 1) ? "inlier " : "outlier");
						if (calibration->Ps.data[Ps_reindex[j]].points_meta.data[point_id].inlier == 0) 
						{
							printf("whooops, outlier in a data which should be outlier free; what gives?\n");
						}

						// then what the hell is it? 

						/*opencv_debug("Initial coordinates", calibration->Xs.data[Xs_reindex[i]].X);
						printf("Coordinates in measurement vector: \n\n%f\n%f\n%f\n%f\n", 
							parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 0],
							parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 1],
							parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 2],
							parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 3]
						);
						printf("End of record\n");
						printf("\n");* /
					}

					// verify against original data
					const double dX = OPENCV_ELEM(calibration->Xs.data[Xs_reindex[i]].X, 0, 0) - parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 0];
					const double dY = OPENCV_ELEM(calibration->Xs.data[Xs_reindex[i]].X, 1, 0) - parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 1];
					const double dZ = OPENCV_ELEM(calibration->Xs.data[Xs_reindex[i]].X, 2, 0) - parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 2];
					const double dW = OPENCV_ELEM(calibration->Xs.data[Xs_reindex[i]].X, 3, 0) - parameters[Ps_count * BA_CAMERA_PARAMETERS + i * 4 + 3];
					const double dd = dX * dX + dY * dY + dZ * dZ + dW * dW;
					if (dd > 0.000000001)
					{
						printf("hmmpf %f ", dd);
					}

					// print error 
					// printf("%f ", d);

					m++;
				}
			}
		}

		int i;
		printf("computed initial error %f [%f]\n", sum_err, sum_err / m);
		scanf("%d", &i);
	}*/

	// * additional settings and info *

	// optimization options
	double options[SBA_OPTSSZ];
	ASSERT(SBA_OPTSSZ > 4, "sba has fewer options than expected, this should be easy to fix");
	memset(options, 0, sizeof(double) * SBA_OPTSSZ);
	options[0] = SBA_INIT_MU;
	options[1] = SBA_STOP_THRESH;
	options[2] = SBA_STOP_THRESH;
	options[3] = SBA_STOP_THRESH;
	options[4] = 0;

	// info
	double info[SBA_INFOSZ];

	// * call bundle adjustment routine * 
	sba_motstr_levmar(
		Xs_count,
		0, 
		Ps_count,
		0, 
		visibility_mask,
		parameters,
		BA_CAMERA_PARAMETERS,
		4,
		measurement,
		NULL,
		2,
		calibration_projection,
		calibration_jacobian,
		NULL,
		10,
		0, // verbose option
		options,
		info
	);

	printf("  Initial average squared error %f, optimized to %f.\n", info[0] / measurement_count, info[1] / measurement_count);

	// * save obtained estimate back into the Calibration structure *

	Ps_i = 0;
	for ALL(calibration->Ps, i)
	{
		const Calibration_Camera * camera = calibration->Ps.data + i;
		for (int j = 0; j < 12; j++) 
		{
			ASSERT(camera->shot_id < shots.count, "invalid shot index");
			ASSERT(shots_order[camera->shot_id] * BA_CAMERA_PARAMETERS + j < parameters_count, "invalid parameter index");
			OPENCV_ELEM(camera->P, j / 4, j % 4) = parameters[shots_order[camera->shot_id] * BA_CAMERA_PARAMETERS + j];
		}
		Ps_i++; 
	}
	ASSERT(Ps_i == Ps_count, "inconsistent counters");

	Xs_i = 0; 
	for ALL(calibration->Xs, i) 
	{
		const Calibration_Vertex * vertex = calibration->Xs.data + i;
		for (int j = 0; j < 4; j++) 
		{
			ASSERT(parameter_offset + Xs_i * 4 + j < parameters_count, "parameters out of bounds");
			OPENCV_ELEM(vertex->X, j, 0) = parameters[parameter_offset + Xs_i * 4 + j];
		}
		Xs_i++;
	}
	ASSERT(Xs_i == Xs_count, "inconsistent counters");

	// * re-evaluate inliers and outliers * 

	// we'll need reindexing arrays
	/*size_t * vertices_to_Xs_reindex = (size_t *)malloc(sizeof(size_t) * vertices.count); 
	{
		size_t i, j = 0;
		LAMBDA(calibration->Xs, i, vertices_to_Xs_reindex[calibration->Xs.data[i].vertex_id] = i; ); 
	}

	for ALL(calibration->Ps, i) 
	{
		const Calibration_Camera * camera = calibration->Ps.data + i; 

		// go through this camera's points 
		for ALL(camera->points_meta, j) 
		{
			const size_t vertex_id = shots.data[camera->shot_id].points.data[j].vertex;
			const size_t X_id = vertices_to_Xs_reindex[vertex_id];

			// project the point 
			double reprojection[2];
			opencv_vertex_projection_visualization(camera->P, calibration->Xs.data[X_id].X, reprojection);

			// calculate the reprojection error 
			
		}
	}

	FREE(vertices_to_Xs_reindex);*/

	// * release structures *
	FREE(parameters);
	FREE(measurement);
	FREE(visibility_mask);
	FREE(vertex_visibility);
	FREE(incidence_ids);
}

// call nonlinear optimization routine 
void tool_calibration_bundle()
{
	// if no calibration is selected, we don't have anything to do 
	if (!INDEX_IS_SET(ui_state.current_calibration))
	{
		printf("No calibration selected.\n");
		return;
	}

	const size_t calibration_id = ui_state.current_calibration;
	
	// get settings
	tool_fetch_parameters(tool_calibration_id);
	const double measurement_threshold = tool_get_real(tool_calibration_id, CALIBRATION_IMAGE_MEASUREMENT_THRESHOLD);
	const bool
		normalize_data = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_DATA),
		normalize_A = tool_get_bool(tool_calibration_id, CALIBRATION_NORMALIZE_A);

	calibration_bundle(measurement_threshold);
	// calibration_triangulate_vertices(calibration_id, measurement_threshold, 2, normalize_data, normalize_A);
}

