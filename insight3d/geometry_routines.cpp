#include "geometry_routines.h"

// decompose projective matrix into rotation, translation and internal calibration matrix 
// note check this
// obtains LOCK_RW(opencv)
bool geometry_calibration_from_P(const size_t shot_id)
{
	ASSERT(validate_shot(shot_id), "invalid shot suppiled when decomposing P"); 
	Shot * const shot = shots.data + shot_id;
	ASSERT(shot->projection, "projection matrix not allocated");

	if (shot->calibrated) 
	{
		ASSERT(shot->rotation, "rotation matrix not allocated"); 
		ASSERT(shot->translation, "translation vector allocated"); 
		ASSERT(shot->internal_calibration, "internal calibration matrix not allocated"); 
	}
	else
	{
		ASSERT(!shot->rotation, "rotation matrix allocated on uncalibrated shot" ); 
		ASSERT(!shot->translation, "translation vector allocated on uncalibrated shot" ); 
		ASSERT(!shot->internal_calibration, "internal calibration matrix allocated on uncalibrated shot" ); 

		shot->rotation = opencv_create_matrix(3, 3);
		shot->translation = opencv_create_matrix(4, 1);
		shot->internal_calibration = opencv_create_matrix(3, 3);
	}

	LOCK_RW(opencv)
	{
		// decompose projection matrix
		const bool finite = mvg_finite_projection_matrix_decomposition(
			shot->projection,
			shot->internal_calibration,
			shot->rotation,
			shot->translation
		);

		// we'll yet have to create a routine for decomposition of cameras at infinity
		if (!finite) 
		{
			// if it wasn't calibrated before, deallocated newly calibrated matrices
			if (!shot->calibrated) 
			{
				cvReleaseMat(&shot->rotation);
				cvReleaseMat(&shot->translation);
				cvReleaseMat(&shot->internal_calibration);
			}

			UNLOCK_RW(opencv);
			return false; 
		}

		// * we have everything decomposed, now just recalculate helper vectors *

		// decompose rotation matrix into euler angles for OpenGL
		opencv_rotation_matrix_to_angles(shot->rotation, shot->R_euler[0], shot->R_euler[1], shot->R_euler[2]);
		shot->R_euler[0] = -(shot->R_euler[0] + OPENCV_PI);
		if (shot->R_euler[0] > 2 * OPENCV_PI) shot->R_euler[0] -= 2 * OPENCV_PI;
		shot->R_euler[1] = -(shot->R_euler[1]);
		shot->R_euler[2] = -(shot->R_euler[2]);
	}
	UNLOCK_RW(opencv);

	// translation vector as C array
	shot->T[0] = OPENCV_ELEM(shot->translation, 0, 0);
	shot->T[1] = OPENCV_ELEM(shot->translation, 1, 0); 
	shot->T[2] = OPENCV_ELEM(shot->translation, 2, 0); 
	shot->T[3] = 1.0;

	// internal calibration holds principal points coordinates 
	shot->pp_x = OPENCV_ELEM(shot->internal_calibration, 0, 2); 
	shot->pp_y = OPENCV_ELEM(shot->internal_calibration, 1, 2); 

	return true;
}

// assemble projective matrix from rotation, translation and internal calibration matrix 
// obtains LOCK_RW(opencv)
void geometry_calibration_from_decomposed_matrices(const size_t shot_id) 
{
	LOCK_RW(opencv)
	{
		// todo add checking for allocated P... we probably want to be consistent with the previous function
		ASSERT(validate_shot(shot_id), "assembling projection matrix for invalid shot");
		Shot * const shot = shots.data + shot_id; 

		shot->projection = opencv_create_matrix(3, 4);
		mvg_assemble_projection_matrix(shot->internal_calibration, shot->rotation, shot->translation, shot->projection);

		// * we have everything assembled, now just recalculate helper vectors *

		// decompose rotation matrix into euler angles for OpenGL
		opencv_rotation_matrix_to_angles(shot->rotation, shot->R_euler[0], shot->R_euler[1], shot->R_euler[2]);
		shot->R_euler[0] = -(shot->R_euler[0] + OPENCV_PI);
		if (shot->R_euler[0] > 2 * OPENCV_PI) shot->R_euler[0] -= 2 * OPENCV_PI;
		shot->R_euler[1] = -(shot->R_euler[1]);
		shot->R_euler[2] = -(shot->R_euler[2]);

		// translation vector as C array
		shot->T[0] = OPENCV_ELEM(shot->translation, 0, 0);
		shot->T[1] = OPENCV_ELEM(shot->translation, 1, 0); 
		shot->T[2] = OPENCV_ELEM(shot->translation, 2, 0); 
		shot->T[3] = 1.0;

		// internal calibration holds principal points coordinates 
		shot->pp_x = OPENCV_ELEM(shot->internal_calibration, 0, 2); 
		shot->pp_y = OPENCV_ELEM(shot->internal_calibration, 1, 2); 
	}
	UNLOCK_RW(opencv); 
}

// lattice test 
// todo check for points out of picture
bool geometry_lattice_test(const size_t shot_id)
{
	ASSERT(validate_shot(shot_id), "invalid shot supplied for lattice test");

	// clear lattice array 
	const int lattice_no = 4; 
	const int lattice_no_sq = 16; 
	int lattice_count[lattice_no_sq]; 
	memset(lattice_count, 0, sizeof(int) * lattice_no_sq);

	// go through all points on this image 
	for ALL(shots.data[shot_id].points, i) 
	{
		const Shot * const shot = shots.data + shot_id; 
		const Point * const point = shot->points.data + i;

		ASSERT(validate_vertex(point->vertex), "invalid vertex when checking for lattice test");
		const Vertex * const vertex = vertices.data + point->vertex; 

		// count reconstructed vertices
		if (vertex->reconstructed)
		{
			const int 
				lattice_x = (int)(point->x * lattice_no),
				lattice_y = (int)(point->y * lattice_no);

			lattice_count[lattice_y * lattice_no + lattice_x]++;
		}
	}

	// count parts of lattice with nonzero number of vertices 
	int ok = 0;
	for (int i = 0; i < lattice_no_sq; i++)
	{
		if (lattice_count[i] >= 1) 
		{
			ok++;
		}
	}

	// todo remove
	printf("%d ", ok);

	return ok >= 6;
}
