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

#include "tool_selection.h"

// tool's constants 
const double TOOL_SELECTION_ZOOM_RATE = 1.2;
const double TOOL_SELECTION_ZOOM_SNAPPING_MIN = 0.5 / TOOL_SELECTION_ZOOM_RATE + 0.001; 
const double TOOL_SELECTION_ZOOM_SNAPPING_MAX = 0.5 * TOOL_SELECTION_ZOOM_RATE - 0.001;

// selection tool handles viewing options which are read by other tools and the rest of the application 
bool option_show_dualview, option_thumbs_only_for_selected, option_hide_automatic;

// tool's state structure 
struct Tool_Selection
{ 
	double shot_down_x, shot_down_y;
};

static Tool_Selection tool_selection;

// create tool 
void tool_selection_create()
{
	tool_create(UI_MODE_SHOT, "Select and zoom", "Let's you scroll, zoom and select points displayed on images");

	tool_set_process_events_function(tool_requires_current_shot);
	tool_set_mouse_down_handler(tool_selection_mouse_down);
	tool_set_click_handler(tool_selection_click);
	tool_set_move_handler(tool_selection_move);
	tool_set_dragging_handler(tool_selection_dragging);
	tool_set_dragging_done_handler(tool_selection_dragging_done);
	tool_set_begin_handler(tool_selection_begin);
	tool_set_end_handler(tool_selection_end);
	tool_set_key_pressed_event_handler(tool_selection_key);

	tool_register_toolbar_button("Select and zoom");

	tool_register_menu_function("Main menu|View|Show/hide automatic points|", selection_option_show_automatic_points);
	tool_register_menu_function("Main menu|View|Enable/disable dualview|", selection_option_show_dualview);
	tool_register_menu_function("Main menu|View|Thumbnails only for selected|", selection_option_thumbs_only_for_selected);
	tool_register_menu_function("Main menu|View|Print projection matrices|", debug_print_Ps);
	tool_register_menu_function("Main menu|View|Save initial solution|", debug_save_initial_solution);
	tool_register_menu_function("Main menu|View|Save vertices|", debug_save_vertices);

	option_show_dualview = false;
	option_thumbs_only_for_selected = false;
	option_hide_automatic = false;
}

// tool activated 
void tool_selection_begin() 
{
	ui_context_set_delay(-2);
}

// tool deactivated 
void tool_selection_end()
{
	ui_context_hide();
}

// handle keyboard events 
void tool_selection_key() 
{
	if (ui_state.key_state[SDLK_SPACE] && INDEX_IS_SET(ui_state.current_shot))
	{
		// vertices visible on current shot 
		bool * visible_vertices = ALLOC(bool, vertices.count);
		memset(visible_vertices, 0, sizeof(bool) * vertices.count);
		size_t * visible_vertices_point_id = ALLOC(size_t, vertices.count);
		memset(visible_vertices_point_id, 0, sizeof(size_t) * vertices.count);
		for ALL(shots.data[ui_state.current_shot].points, i) 
		{
			visible_vertices[shots.data[ui_state.current_shot].points.data[i].vertex] = true;
			visible_vertices_point_id[shots.data[ui_state.current_shot].points.data[i].vertex] = i;
		}

		// we'll go through polygons and pick those which are not entirely visible on current shot 
		// and best suited for registration 
		size_t best_shot_id = 0, best_polygon_id = 0, best_points_count = 0;
		bool best_found = false;
		size_t * count = ALLOC(size_t, shots.count);
		for ALL(polygons, i)
		{
			const Polygon_3d * const polygon = polygons.data + i; 
			memset(count, 0, sizeof(size_t) * shots.count);
			size_t max = 0, max_shot_id = 0;

			for ALL(polygon->vertices, j) 
			{
				const size_t vertex_id = polygon->vertices.data[j].value; 

				for ALL(vertices_incidence.data[vertex_id].shot_point_ids, k)
				{
					const size_t shot_id = vertices_incidence.data[vertex_id].shot_point_ids.data[k].primary;
					count[shot_id]++;
					if (count[shot_id] > max) 
					{
						max = count[shot_id];
						max_shot_id = shot_id;
					}
				}
			}
		
			// if this polygon is not entirely visible on current shot 
			if (count[ui_state.current_shot] < max) 
			{
				// then it might be good candidate, count the features in it
				for (size_t k = 0; k < shots.count; k++) 
				{
					if (count[k] == max)
					{
						// export it 
						CvMat * polygon_mat = publish_polygon(k, i);

						if (polygon_mat)
						{
							ASSERT_IS_SET(shots, k);

							size_t points_count = 0;
							for ALL(shots.data[k].points, j)
							{
								Point * point = shots.data[k].points.data + j;
								if (!visible_vertices[point->vertex]) continue;
								if (opencv_pip(point->x, point->y, polygon_mat))
								{
									points_count++;
								}
							}

							if (points_count > 0 && (points_count > best_points_count || !best_found))
							{
								best_found = true;
								best_points_count = points_count; 
								best_polygon_id = i;
								best_shot_id = k;
							}

							ATOMIC_RW(opencv, cvReleaseMat(&polygon_mat); );
						}
					}
				}
			}
		}

		FREE(count);

		if (best_found) 
		{
			// * estimate the polygon *
			size_t 
				* points1_id = ALLOC(size_t, shots.data[ui_state.current_shot].points.count + 128),
				* points2_id = ALLOC(size_t, shots.data[ui_state.current_shot].points.count + 128);

			ASSERT_IS_SET(shots, best_shot_id);

			// vertices of this polygon on both shots 
			bool * polygon_vertices = ALLOC(bool, vertices.count);
			memset(polygon_vertices, 0, sizeof(bool) * vertices.count);
			size_t count_manually_entered = 0;
			for ALL(polygons.data[best_polygon_id].vertices, i) 
			{
				const size_t vertex_id = polygons.data[best_polygon_id].vertices.data[i].value;
				polygon_vertices[vertex_id] = true;

				bool point_found = false; 
				size_t point_iter = 0; 
				LAMBDA_FIND(vertices_incidence.data[vertex_id].shot_point_ids, point_iter, point_found, 
					vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].primary == best_shot_id
				);

				// this vertex is visible on both shots
				if (point_found && visible_vertices[vertex_id])
				{
					points1_id[count_manually_entered] = vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].secondary;
					points2_id[count_manually_entered] = visible_vertices_point_id[vertex_id];
					count_manually_entered++;
				}
			}

			// other correspondences
			size_t count_tracked = 0;
			CvMat * polygon_mat = publish_polygon(best_shot_id, best_polygon_id);

			if (polygon_mat)
			{
				for ALL(shots.data[best_shot_id].points, j)
				{
					Point * point = shots.data[best_shot_id].points.data + j;
					if (!visible_vertices[point->vertex] || polygon_vertices[point->vertex]) continue;

					// this vertex is visible on both shots and it's inside the polygon
					if (opencv_pip(point->x, point->y, polygon_mat))
					{
						points1_id[count_manually_entered + count_tracked] = j;
						points2_id[count_manually_entered + count_tracked] = visible_vertices_point_id[point->vertex];
						count_tracked++;
					}
				}

				ATOMIC_RW(opencv, cvReleaseMat(&polygon_mat); );
			}

			// estimate the homography 
			if (count_manually_entered + count_tracked >= 4)
			{
				// use RANSAC
				CvMat * points1;
				CvMat * points2;
				CvMat * H;
				CvMat * best_H;

				LOCK_RW(opencv) 
				{
					points1 = opencv_create_matrix(2, 4); 
					points2 = opencv_create_matrix(2, 4);
					H = opencv_create_matrix(3, 3);
					best_H = opencv_create_matrix(3, 3);
				}
				UNLOCK_RW(opencv);

				int inliers, best_inliers = -1;

				for (size_t j = 0; j < count_manually_entered; j++) 
				{
					OPENCV_ELEM(points1, 0, j) = shots.data[best_shot_id].points.data[points1_id[j]].x; 
					OPENCV_ELEM(points1, 1, j) = shots.data[best_shot_id].points.data[points1_id[j]].y; 
					OPENCV_ELEM(points2, 0, j) = shots.data[ui_state.current_shot].points.data[points2_id[j]].x; 
					OPENCV_ELEM(points2, 1, j) = shots.data[ui_state.current_shot].points.data[points2_id[j]].y; 
				}

				int trials = 0; 
				while (trials++ < 60) 
				{
					if (count_manually_entered < 4) 
					{
						for (size_t j = count_manually_entered; j < (count_tracked + count_manually_entered) && j < 4; j++) 
						{
							int r = count_manually_entered + rand() % count_tracked;
							OPENCV_ELEM(points1, 0, j) = shots.data[best_shot_id].points.data[points1_id[r]].x; 
							OPENCV_ELEM(points1, 1, j) = shots.data[best_shot_id].points.data[points1_id[r]].y; 
							OPENCV_ELEM(points2, 0, j) = shots.data[ui_state.current_shot].points.data[points2_id[r]].x; 
							OPENCV_ELEM(points2, 1, j) = shots.data[ui_state.current_shot].points.data[points2_id[r]].y; 
						}
					}

					ATOMIC_RW(opencv, cvFindHomography(points1, points2, H); );

					// go through all points and count the number of inliers 
					inliers = 0;
					for (size_t n = 0; n < count_tracked + count_manually_entered; n++) 
					{
						// project the point 
						const double w = 
							OPENCV_ELEM(H, 2, 0) * shots.data[best_shot_id].points.data[points1_id[n]].x + 
							OPENCV_ELEM(H, 2, 1) * shots.data[best_shot_id].points.data[points1_id[n]].y + 
							OPENCV_ELEM(H, 2, 2);

						if (!nearly_zero(w))
						{
							const double 
								x = (
								OPENCV_ELEM(H, 0, 0) * shots.data[best_shot_id].points.data[points1_id[n]].x + 
								OPENCV_ELEM(H, 0, 1) * shots.data[best_shot_id].points.data[points1_id[n]].y + 
								OPENCV_ELEM(H, 0, 2)
								) / w,
								y = (
								OPENCV_ELEM(H, 1, 0) * shots.data[best_shot_id].points.data[points1_id[n]].x +
								OPENCV_ELEM(H, 1, 1) * shots.data[best_shot_id].points.data[points1_id[n]].y + 
								OPENCV_ELEM(H, 1, 2)
								) / w;

							const double 
								dx = shots.data[ui_state.current_shot].width * (x - shots.data[ui_state.current_shot].points.data[points2_id[n]].x),
								dy = shots.data[ui_state.current_shot].height * (y - shots.data[ui_state.current_shot].points.data[points2_id[n]].y);
							const double er = dx * dx + dy * dy;

							if (sqrt(er) < 4)
							{
								// printf("[%f] ", sqrt(er));
								inliers++;
							}
							else
							{
								// printf("{%f} ", sqrt(er));
							}
						}
					}

					if (count_manually_entered >= 4)
					{
						ATOMIC_RW(opencv, cvCopy(H, best_H); );
						best_inliers = inliers;
						break;
					}

					if (best_inliers == -1 || inliers > best_inliers)
					{
						best_inliers = inliers; 
						ATOMIC_RW(opencv, cvCopy(H, best_H); );
					}
				}

				// if there is big enough support set
				// printf("inliers: %d\n", best_inliers);
				if (best_inliers >= 8)
				{
					ATOMIC_RW(opencv, cvCopy(best_H, H); );

					// now save estimated points 
					for ALL(polygons.data[best_polygon_id].vertices, i)
					{
						const size_t vertex_id = polygons.data[best_polygon_id].vertices.data[i].value;

						// skip vertices already visible on destination shot 
						if (visible_vertices[vertex_id]) continue;

						bool point_found = false; 
						size_t point_iter = 0; 
						LAMBDA_FIND(
							vertices_incidence.data[vertex_id].shot_point_ids, point_iter, point_found,
							vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].primary == best_shot_id
						);

						const size_t point_id = vertices_incidence.data[vertex_id].shot_point_ids.data[point_iter].secondary;
						const Shot * shot = shots.data + best_shot_id;

						// transform this point using computed homography 
						const double w = 
							OPENCV_ELEM(H, 2, 0) * shot->points.data[point_id].x + 
							OPENCV_ELEM(H, 2, 1) * shot->points.data[point_id].y + 
							OPENCV_ELEM(H, 2, 2);

						if (!nearly_zero(w)) 
						{
							const double 
								x = (
								OPENCV_ELEM(H, 0, 0) * shot->points.data[point_id].x + 
								OPENCV_ELEM(H, 0, 1) * shot->points.data[point_id].y + 
								OPENCV_ELEM(H, 0, 2)
								) / w,
								y = (
								OPENCV_ELEM(H, 1, 0) * shot->points.data[point_id].x + 
								OPENCV_ELEM(H, 1, 1) * shot->points.data[point_id].y + 
								OPENCV_ELEM(H, 1, 2)
								) / w;

							size_t new_point_id;
							geometry_new_point(new_point_id, x, y, ui_state.current_shot, vertex_id);
						}
					}
				}

				// release resources
				LOCK_RW(opencv)
				{
					cvReleaseMat(&points1);
					cvReleaseMat(&points2);
					cvReleaseMat(&H);
					cvReleaseMat(&best_H);
				}
				UNLOCK_RW(opencv);
			}

			FREE(points1_id);
			FREE(points2_id);

			/*printf("Best polygon is %d on shot %d with %d features.\n", best_polygon_id, best_shot_id, best_points_count);
			printf("There are %d correspondences from user and %d tracked.\n", count_manually_entered, count_tracked);*/
		}
		else
		{
			printf("No suitable polygon found.\n");
		}

		FREE(visible_vertices);
		FREE(visible_vertices_point_id);

		ui_workflow_first_vertex();
		INDEX_CLEAR(ui_state.processed_polygon);
		ui_state.key_state[SDLK_SPACE] = false;
	}
}

// middle button performs scrolling 
bool tool_selection_mouse_down(double x, double y, int button)
{
	UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);
	bool snap_zoom = false;

	// save info about the mouse down event for dragging 
	tool_selection.shot_down_x = x; 
	tool_selection.shot_down_y = y;

	switch (button)
	{
		case SDL_BUTTON_WHEELUP:
		{
			meta->view_zoom /= TOOL_SELECTION_ZOOM_RATE; 
	
			const double 
				point_under_cursor_x = meta->view_center_x + (x - meta->view_center_x) / TOOL_SELECTION_ZOOM_RATE,
				point_under_cursor_y = meta->view_center_y + (y - meta->view_center_y) / TOOL_SELECTION_ZOOM_RATE
			; 

			meta->view_center_x -= point_under_cursor_x - x;
			meta->view_center_y -= point_under_cursor_y - y;
			snap_zoom = true; 
			break;
		}

		case SDL_BUTTON_WHEELDOWN:
		{
			meta->view_zoom *= TOOL_SELECTION_ZOOM_RATE;
	
			const double 
				point_under_cursor_x = meta->view_center_x + (x - meta->view_center_x) * TOOL_SELECTION_ZOOM_RATE,
				point_under_cursor_y = meta->view_center_y + (y - meta->view_center_y) * TOOL_SELECTION_ZOOM_RATE
			; 

			meta->view_center_x -= point_under_cursor_x - x;
			meta->view_center_y -= point_under_cursor_y - y;
			snap_zoom = true;
			break;
		}
	}

	if (snap_zoom) 
	{
		// if we're close enough to initial zoom factor, snap to zoom = 0.5
		if (inside_open_interval(meta->view_zoom, TOOL_SELECTION_ZOOM_SNAPPING_MIN, TOOL_SELECTION_ZOOM_SNAPPING_MAX))
		{
			meta->view_zoom = 0.5;
		}
		
		// if the viewport exceeds the image, normalize it's position 
		visualization_move_into_viewport(true, true); // !INDEX_IS_SET(ui_state.dualview));
	}

	// we'll let the user start dragging
	return true;
}

// points are being focused when the user moves the mouse cursor over them
void tool_selection_move(double x, double y)
{
	size_t point_id;
	bool found = ui_selection_get_point_by_position(x, y, point_id);

	if (found)
	{
		if (!INDEX_IS_SET(ui_state.focused_point) || point_id != ui_state.focused_point)
		{
			ui_workflow_set_focused_point(point_id);

			// fill the context popup 
			const size_t vertex_id = shots.data[ui_state.current_shot].points.data[point_id].vertex;
			if (query_count_points_by_vertex(vertex_id) > 1)
			{
				// show context popup 
				ui_context_show();
				ui_context_clear();

				// sort the thumbnails by shot_id
				Vertex_Incidence * const incidence = vertices_incidence.data + vertex_id;
				geometry_sort_double_indices_primary_desc(&incidence->shot_point_ids);

				// fill it with thumbnails
				for ALL(vertices_incidence.data[vertex_id].shot_point_ids, i) 
				{
					Double_Index * const id = vertices_incidence.data[vertex_id].shot_point_ids.data + i; 

					if (!option_thumbs_only_for_selected || ui_check_shot_meta(id->primary)->selected) 
					{
						ui_context_add_thumbnail(
							id->primary, 
							shots.data[id->primary].points.data[id->secondary].x, 
							shots.data[id->primary].points.data[id->secondary].y, 
							option_thumbs_only_for_selected ? 525 : 300, 
							option_thumbs_only_for_selected ? 410 : 234,
							UI_CONTEXT_CROSSHAIR
						);
					}
				}
			}
		}
	}
	else if (INDEX_IS_SET(ui_state.focused_point))
	{
		ui_workflow_unset_focused_point();
		ui_context_clear();
		ui_context_hide();
	}
}

// move shot when dragging 
void tool_selection_dragging(double x, double y, int button)
{
	// get information about current frame 
	UI_Shot_Meta * meta = ui_check_shot_meta(ui_state.current_shot);

	if (button == SDL_BUTTON_MIDDLE)
	{
		meta->view_center_x += tool_selection.shot_down_x - x;
		meta->view_center_y += tool_selection.shot_down_y - y;
	}
}

// perform selection when the dragging is over 
void tool_selection_dragging_done(double x1, double y1, double x2, double y2, int button)
{
	if (button == SDL_BUTTON_RIGHT)
	{
		// selection box modifiers 
		Selection_Type operation = SELECTION_TYPE_REPLACEMENT; 

		if (sdl_shift_pressed())
		{
			operation = SELECTION_TYPE_UNION;
		}
		else if (sdl_ctrl_pressed())
		{
			operation = SELECTION_TYPE_INTERSECTION;
		}

		ui_2d_selection_box(x1, y1, x2, y2, operation);
	}
}

// select point under cursor 
void tool_selection_click(double x, double y, int button) 
{
	if (sdl_wheel_button(button)) return;

	ui_empty_selection_list();

	if (INDEX_IS_SET(ui_state.focused_point)) 
	{
		ui_add_point_to_selection(ui_state.current_shot, ui_state.focused_point);
	}
}

// * switching options * 

// dualview let's the user verify correspondences 
void selection_option_show_dualview()
{
	option_show_dualview = !option_show_dualview;

	if (!option_show_dualview) 
	{
		ui_release_dualview();
	}
}

// thumbnails can be shown for all shots or only for those, which are selected
void selection_option_thumbs_only_for_selected()
{
	option_thumbs_only_for_selected = !option_thumbs_only_for_selected;
}

// show and hide automatically tracked points 
void selection_option_show_automatic_points()
{
	option_hide_automatic = !option_hide_automatic;
	ui_workflow_no_vertex();
}

// * debugging functions * 

void debug_print_Ps()
{
	std::ofstream Ps_out("./Ps.txt");

	for ALL(shots, i) 
	{
		const Shot * const shot = shots.data + i;
		if (!shot->projection) continue;

		Ps_out << shot->name << std::endl; 

		/* decide which projection matrix we want to print (corrected or not?) */

		CvMat * P; 
		ATOMIC_RW(opencv, P = opencv_create_matrix(3, 4); );

		if (!P) 
		{
			printf("Failed to allocate memory for P\n");
			return;
		}
		
		if (false) 
		{
			// just copy the uncorrected matrix
			ATOMIC_RW(opencv, cvCopy(shot->projection, P); );
		}
		else
		{
			// obtain corrected projection matrix by loading the correcting homography from a file 
			char fn[5000];
			memset(fn, 0, sizeof(char) * 5000);
			strcat(fn, shot->image_filename);
			strcat(fn, ".H.txt");
			FILE * fp = fopen(fn, "r");

			if (fp)
			{
				// load homography
				CvMat * H;
				ATOMIC_RW(opencv, H = opencv_create_matrix(3, 3); );

				for (int i = 0; i < 9; i++)
				{
					double d;
					const int entry_read = fscanf(fp, "%lf", &d);
					// TODO use entry read
					OPENCV_ELEM(H, i / 3, i % 3) = d;
				}

				fclose(fp);

				LOCK_RW(opencv) 
				{
					cvInvert(H, H);
					cvMatMul(H, shot->projection, P);
					cvReleaseMat(&H);
				}
				UNLOCK_RW(opencv);
			}
			else
			{
				printf("ERROR opening file %s, using uncorrected matrix.\n", fn);
				ATOMIC_RW(opencv, cvCopy(shot->projection, P); );
			}
		}

		// print projection matrix
		for (int i = 0; i < P->rows; i++)
		{
			const double q = i < 2 ? 0.5 : 1; // todo what's this for?
			for (int j = 0; j < P->cols; j++)
			{
				Ps_out << q * OPENCV_ELEM(P, i, j) << " ";
			}
		}
		Ps_out << std::endl;

		// print it's pseudoinverse
		CvMat * P_inv;

		LOCK_RW(opencv)
		{
			P_inv = opencv_create_matrix(4, 3);
			cvInvert(P, P_inv, CV_SVD);
		}
		UNLOCK_RW(opencv);

		for (int i = 0; i < P_inv->rows; i++)
		{
			for (int j = 0; j < P_inv->cols; j++)
			{
				Ps_out << OPENCV_ELEM(P_inv, i, j) << " ";
			}
		}
		Ps_out << std::endl;

		// print the camera center 
		for (int i = 0; i < shot->translation->rows; i++) 
		{
			Ps_out << OPENCV_ELEM(shot->translation, i, 0) << " ";
		}
		Ps_out << std::endl;
	}

	printf("Calibration matrices saved\n"); 
}

// save initial solution for use in external bundle adjustment tool
void debug_save_initial_solution() 
{
	// export works only if current calibration is selected
	if (!INDEX_IS_SET(ui_state.current_calibration)) return;
	const Calibration * const calibration = calibrations.data + ui_state.current_calibration;

	// index arrays 
	size_t 
		* index_Ps = dyn_build_reindex(calibration->Ps),
		* index_Xs = dyn_build_reindex(calibration->Xs), 
		* index_shot_to_P = ALLOC(size_t, shots.count);
	bool * index_shot_to_P_set = ALLOC(bool, shots.count);
	memset(index_shot_to_P, 0, sizeof(size_t) * shots.count);
	memset(index_shot_to_P_set, 0, sizeof(bool) * shots.count);

	// counters 
	size_t count_Ps = 0, count_Xs = 0;

	// create indexing array for calibrated cameras
	std::ofstream cameras_out("./cameras.txt");
	for ALL(calibration->Ps, i)
	{
		const Calibration_Camera * const P = calibration->Ps.data + i; 
		index_shot_to_P[P->shot_id] = i; 
		index_shot_to_P_set[P->shot_id] = true;
		count_Ps++;
	}

	// export cameras 
	cameras_out << count_Ps << std::endl;
	for ALL(calibration->Ps, i)
	{
		const Calibration_Camera * const P = calibration->Ps.data + i;
		for (int j = 0; j < 12; j++)
		{
			cameras_out << OPENCV_ELEM(P->P, j / 4, j % 4) << " ";
		}
		cameras_out << std::endl;
	}

	// export 3d and 2d points
	std::ofstream points_out("./points.txt");
	for ALL(calibration->Xs, i)
	{
		const Calibration_Vertex * const X = calibration->Xs.data + i;
		
		// count the number of points
		size_t points_count = 0; 
		for ALL(vertices_incidence.data[X->vertex_id].shot_point_ids, j) 
		{
			const Double_Index * const index = vertices_incidence.data[X->vertex_id].shot_point_ids.data + j;
			if (index_shot_to_P_set[index->primary])
			{
				if (++points_count > 2) break;
			}
		}

		if (points_count)
		{
			count_Xs++;

			// export estimated vertex 
			points_out 
				<< OPENCV_ELEM(X->X, 0, 0) << " " 
				<< OPENCV_ELEM(X->X, 1, 0) << " " 
				<< OPENCV_ELEM(X->X, 2, 0) << " "
				<< "1.0 ";

			// number of points for this vertex
			points_out << points_count << " ";

			// and it's occurences on all shots
			for ALL(vertices_incidence.data[X->vertex_id].shot_point_ids, j)
			{
				const Double_Index * const index = vertices_incidence.data[X->vertex_id].shot_point_ids.data + j;
				points_out 
					<< index_Ps[index_shot_to_P[index->primary]] << " "
					<< shots.data[index->primary].points.data[index->secondary].x << " " 
					<< shots.data[index->primary].points.data[index->secondary].y << " ";
			}

			points_out << std::endl;
		}
	}

	// release data 
	FREE(index_Ps);
	FREE(index_Xs);
	FREE(index_shot_to_P);
	FREE(index_shot_to_P_set);
}

// save vertices coordinates 
void debug_save_vertices()
{
	std::ofstream vertices_out("./vertices.txt"); 
	
	for ALL(vertices, i) 
	{
		vertices_out << i << " " << vertices.data[i].x << " " << vertices.data[i].y << " " << vertices.data[i].z << std::endl;
	}
}

