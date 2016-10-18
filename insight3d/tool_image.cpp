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

#include "tool_image.h"

// generate textures
void tool_image_generate_textures()
{
	geometry_extract_all_textures();
}

// colorize vertices 
void tool_image_colorize()
{
	// allocate memory for accumulators
	int
		* accum_red = ALLOC(int, vertices.count),
		* accum_green = ALLOC(int, vertices.count), 
		* accum_blue = ALLOC(int, vertices.count);
	int 
		* count = ALLOC(int, vertices.count); 

	memset(accum_red, 0, sizeof(int) * vertices.count);
	memset(accum_green, 0, sizeof(int) * vertices.count);
	memset(accum_blue, 0, sizeof(int) * vertices.count);
	memset(count, 0, sizeof(int) * vertices.count);

	// go through all photos
	int loaded_img_counter = 0;
	for ALL(shots, shot_id)
	{
		const Shot * const shot = shots.data + shot_id;

		// load the image
		IplImage * img = NULL;

		// take every point on this image
		for ALL(shot->points, point_id)
		{
			Point * point = shot->points.data + point_id;

			if (!img)
			{
				ATOMIC_RW(opencv, img = cvLoadImage(shot->image_filename); );
				if (!img) continue;
			}

			const int
				x = (int)(point->x * img->width),
				y = (int)(point->y * img->height);

			accum_red[point->vertex] += ((uchar*)(img->imageData + img->widthStep*y))[x*3+2];
			accum_green[point->vertex] += ((uchar*)(img->imageData + img->widthStep*y))[x*3+1];
			accum_blue[point->vertex] += ((uchar*)(img->imageData + img->widthStep*y))[x*3+0];
			count[point->vertex]++;
		}

		// release resources
		if (img) 
		{
			printf("[%d]", loaded_img_counter);
			ATOMIC_RW(opencv, cvReleaseImage(&img); );
			loaded_img_counter++;
			
			// debug
			// if (loaded_img_counter >= 200) break;
		}
	}

	// colorize vertices 
	for ALL(vertices, vertex_id) 
	{
		Vertex * const vertex = vertices.data + vertex_id; 

		vertex->color[0] = accum_red[vertex_id] / ((float)count[vertex_id] * 255); 
		vertex->color[1] = accum_green[vertex_id] / ((float)count[vertex_id] * 255); 
		vertex->color[2] = accum_blue[vertex_id] / ((float)count[vertex_id] * 255); 
	}

	printf("\n");

	// release resources
	FREE(accum_red);
	FREE(accum_green);
	FREE(accum_blue);
	FREE(count);

	/*// take each vertex
	for ALL(vertices, i) 
	{
		ASSERT_IS_SET(vertices_incidence, i); 

		unsigned int accum_red = 0, accum_green = 0, accum_blue = 0;
		unsigned int count = 0;

		// go through all shots where this vertex is marked
		for ALL(vertices_incidence.data[i].shot_point_ids, j) 
		{
			const Double_Index * index = vertices_incidence.data[i].shot_point_ids.data + j;

			// what is the color of this point
			ASSERT(validate_shot
			IplImage * img = shots.data[
			accum_blue += ((uchar*)(img->imageData + img->widthStep*pt.y))[pt.x*3+0];
			accum_green += ((uchar*)(img->imageData + img->widthStep*pt.y))[pt.x*3+1];
			accum_red += ((uchar*)(img->imageData + img->widthStep*pt.y))[pt.x*3+2];

			count++;
		}

		// calculate the color 
		vertices.data[i].color[0] = accum_red / (float)count; 
		vertices.data[i].color[1] = accum_green / (float)count; 
		vertices.data[i].color[2] = accum_blue / (float)count;
	}*/
}

// deform image so that the calibrated cameras all have the same internal calibration
void tool_image_pinhole_deform()
{
	if (!INDEX_IS_SET(ui_state.current_shot)) return;

	// take the camera calibration of the first calibrated camera 
	size_t first_calibrated = 0; 
	bool found = false;
	LAMBDA_FIND(
		shots, 
		first_calibrated, 
		found, 
		shots.data[first_calibrated].calibrated && first_calibrated == 0 /*!shots.data[first_calibrated].resected*/
	);
	if (!found) return;
	const Shot * const first_shot = shots.data + first_calibrated;

	LOCK_RW(opencv)
	{
		for ALL(shots, i)
		{
			Shot * const shot = shots.data + i; 

			// we only care about calibrated cameras
			if (shot->calibrated && /*shot->resected // note temporary */ i > 0) 
			{
				// compute homography 
				CvMat * H = cvCreateMat(3, 3, CV_64F), * K_inv = cvCreateMat(3, 3, CV_64F);
				cvInvert(shot->internal_calibration, K_inv);
				cvMatMul(first_shot->internal_calibration, K_inv, H);

				CvMat * A = cvCreateMat(3, 3, CV_64F);
				cvMatMul(H, shot->internal_calibration, A); 

				// replace this K with the one from first shot 
				cvCopy(first_shot->internal_calibration, shot->internal_calibration);
				shot->fovx = first_shot->fovx; 
				shot->film_back = first_shot->film_back; 
				shot->pp_x = first_shot->pp_x;
				shot->pp_y = first_shot->pp_y;
				shot->f = first_shot->f;
				UNLOCK_RW(opencv)
				{
					geometry_calibration_from_decomposed_matrices(i);
				}
				LOCK_RW(opencv);

				// perform the deformation 
				IplImage * full_image = NULL;
				int cnter = 0; 
				while (!full_image) 
				{
					full_image = cvLoadImage(shot->image_filename); 
					cnter++;
					if (cnter > 10) break;
				}

				if (cnter < 10) 
				{
					IplImage * deformed = cvCreateImage(cvSize(full_image->width, full_image->height), full_image->depth, full_image->nChannels);
					for (int y = 0; y < deformed->height; y++) 
					{
						for (int x = 0; x < deformed->width; x++) 
						{
							((uchar*)(deformed->imageData + deformed->widthStep*y))[x*3]   = 0;
							((uchar*)(deformed->imageData + deformed->widthStep*y))[x*3+1] = 255;
							((uchar*)(deformed->imageData + deformed->widthStep*y))[x*3+2] = 0;
						}
					}
					cvWarpPerspective(full_image, deformed, H);
					printf("deforming %s ... ", shot->image_filename);

					cvSaveImage(shot->image_filename, deformed);
					printf("done\n");

					std::ofstream H_out((std::string(shot->image_filename) + ".H.txt").c_str());
					for (int i = 0; i < 3; i++) 
					{
						for (int j = 0; j < 3; j++) 
						{
							H_out << OPENCV_ELEM(H, i, j) << " "; 
						}
						H_out << std::endl;
					}
					H_out.close(); 
					
					// update internal calibration matrix 
					cvCopy(first_shot->internal_calibration, shot->internal_calibration);

					cvReleaseImage(&deformed);
				}
				
				// release data 
				cvReleaseMat(&H);
				cvReleaseMat(&K_inv);
				cvReleaseImage(&full_image); 
			}
		}
	}
	UNLOCK_RW(opencv);
}

// register tool 
void tool_image_create() 
{
	tool_create(UI_MODE_UNSPECIFIED, "Resection", "Estimate cameras using correspondence between reconstructred 3d vertices and their projection");
	tool_register_menu_function("Main menu|Image|Colorize vertices|", tool_image_colorize);
	tool_register_menu_function("Main menu|Image|Generate textures|", tool_image_generate_textures);
	// tool_register_menu_function("Main menu|Image|Pinhole correction|", tool_image_pinhole_deform);
}
