#include "ui_epipolars.h"

// displays epipolars
void ui_epipolars_display(const size_t shot_id, const size_t point_id)
{
	ASSERT(validate_shot(shot_id), "invalid shot supplied");
	ASSERT(validate_point(shot_id, point_id), "invalid point supplied");
	Shot * const shot = shots.data + shot_id;
	const size_t vertex_id = shot->points.data[point_id].vertex;
	ASSERT_IS_SET(vertices_incidence, vertex_id);
	ASSERT(shots.data[shot_id].info_status >= GEOMETRY_INFO_DEDUCED, "image is displayed, but it's meta information wasn't stored");

	// which calibration to use 
	if (INDEX_IS_SET(ui_state.current_calibration))
	{
		// find this shot in current calibration 
		Calibration_Cameras * cameras = &calibrations.data[ui_state.current_calibration].Ps;
		bool found;
		size_t camera_id;
		LAMBDA_FIND(*cameras, camera_id, found, cameras->data[camera_id].shot_id == shot_id);

		if (found)
		{
			// * the shot is calibrated in this calibration, show epipolars *
			Calibration_Camera * camera = cameras->data + camera_id;

			// consider every fundamental matrix
			for ALL(camera->Fs, i)
			{
				Calibration_Fundamental_Matrix * fund = camera->Fs.data + i; 
				const size_t first_shot_id = fund->first_shot_id;

				// check if we know the size of this image 
				if (shots.data[first_shot_id].info_status < GEOMETRY_INFO_DEDUCED) continue;

				// check if this vertex is visible on this shot 
				Double_Indices * ids = &vertices_incidence.data[vertex_id].shot_point_ids;
				bool first_shot_found; 
				size_t first_shot_iter; 
				LAMBDA_FIND(*ids, first_shot_iter, first_shot_found, ids->data[first_shot_iter].primary == first_shot_id);

				if (first_shot_found)
				{
					const size_t point_id = ids->data[first_shot_iter].secondary;

					// fundamental matrix is estimated and correspondence was marked on this shot
					double a, b, c;
					double 
						x = shots.data[first_shot_id].points.data[point_id].x * shots.data[first_shot_id].width,
						y = shots.data[first_shot_id].points.data[point_id].y * shots.data[first_shot_id].height;

					// calculate epipolar 
					opencv_epipolar(fund->F, x, y, a, b, c);

					// dipslay epipolar 
					if (b != 0) 
					{
						double x1 = 0, x2 = shots.data[first_shot_id].width;
						double y1 = (-a * x1 - c) / b, y2 = (-a * x2 - c) / b;
						x1 /= shots.data[first_shot_id].width; 
						x2 /= shots.data[first_shot_id].width; 
						y1 /= shots.data[first_shot_id].height;
						y2 /= shots.data[first_shot_id].height;

						ui_convert_xy_from_shot_to_opengl(x1, y1, x1, y1); 
						ui_convert_xy_from_shot_to_opengl(x2, y2, x2, y2); 

						LOCK_RW(opengl)
						{
							glBegin(GL_LINES);
								glVertex3d(x1, y1, -1); 
								glVertex3d(x2, y2, -1);
							glEnd();
						}
						UNLOCK_RW(opengl);
					}
				}
			}
		}
	}
}
