#include "tool_matching.h"

// parameter ids
const size_t 
	MATCHING_METHOD = 0, 
	MATCHING_TOPOLOGY = 1, 
	MATCHING_NEIGHBOURS = 2,
	MATCHING_IMAGE_MEASUREMENT_THRESHOLD = 3,
	MATCHING_SIMILARITY_THRESHOLD = 4, 
	MATCHING_GUIDED = 5, 
	MATCHING_RESOLUTION = 6,
	MATCHING_SKIP_FEATURE_EXTRACTION = 7,
	MATCHING_F_RANSAC = 8,
	MATCHING_INCLUDE_UNVERIFIED = 9
	;

const size_t
	MATCHING_TOPOLOGY_UNORDERED = 0, 
	MATCHING_TOPOLOGY_SEQUENCE = 1
	;

const size_t 
	MATCHING_SIFT = 0, 
	MATCHING_MSER = 1
	;

static const char * matching_method_labels[] = { "SIFT", "SIFT+MSER", NULL };
static const char * matching_topology_labels[] = { "All pairs (unordered set of images)", "Just neighbours (linear sequence)", NULL };
static const char * matching_resolution_labels[] = { "medium (up to 1600px)", "high (up to 2600px)", "low (up to 1024px)", NULL }; 
static const int matching_resolution_values[] = { 1600, 2600, 1024, NULL };

// tool's state structure 
struct Tool_Matching
{ 
	// at this point, we don't need any... 
};

// structures for union-find
struct Matching_UF_Node 
{ 
	bool set;
	bool root; // is this root element 
	bool root_with_vertex; // does this root element have created vertex? (in that case the value i holds it's vertex_id)
	size_t i; // index of parent for children, size for root nodes
};

DYNAMIC_STRUCTURE_DECLARATIONS(Matching_UF_Nodes, Matching_UF_Node);
DYNAMIC_STRUCTURE(Matching_UF_Nodes, Matching_UF_Node);

static Tool_Matching tool_matching;
static size_t tool_matching_id;

// forward declarations of private routines
void matching_extract_features(const double max_width);
void matching_extract_tracks(const double fsor_limit, const bool use_ransac, const bool include_unverified, const double epipolar_distance_threshold, Matching_UF_Nodes & uf_nodes, const int topology, const int neighbours);
void matching_remove_conflicting_tracks();
size_t matching_find_parent_node(Matching_UF_Nodes & nodes, feature * first, bool & found);
void matching_features_union(Matching_UF_Nodes & nodes, feature * first, feature * second);

// refresh lists in UI containing information modified by this tool
void tool_matching_refresh_UI()
{
}

// create UI elements for matcher 
void tool_matching_create() 
{
	tool_matching_id = tool_create(UI_MODE_UNSPECIFIED, "Matching", "Automatic matching of images");

	tool_register_menu_function("Main menu|Matching|Start matching|", tool_matching_standard);
	tool_create_tab("Matching");

	// tool_register_int(MATCHING_RESOLUTION, "Image size to use (px): ", 1024, 64, 4096, 256);
	tool_register_enum(MATCHING_RESOLUTION, "Image size to use (px): ", matching_resolution_labels);
	tool_register_real(MATCHING_IMAGE_MEASUREMENT_THRESHOLD, "Measurement threshold (px): ", 12, 0.01, 250, 1);
	tool_register_real(MATCHING_SIMILARITY_THRESHOLD, "Similarity threshold: ", 0.7, 0, 1, 0.02);

	//tool_register_enum(MATCHING_METHOD, "Method to use:", matching_method_labels);
	tool_register_enum(MATCHING_TOPOLOGY, "Which image-pairs to match:", matching_topology_labels);

	// tool_register_bool(MATCHING_GUIDED, "Use guided matching", 1);
	tool_register_bool(MATCHING_F_RANSAC, "Use RANSAC filtering", 1);
	tool_register_bool(MATCHING_INCLUDE_UNVERIFIED, "Include matches unverified by RANSAC", 0);
	tool_register_bool(MATCHING_SKIP_FEATURE_EXTRACTION, "Skip feature extraction", 0);

	tool_create_separator();
	tool_create_label("For linear sequences:");
	tool_register_int(MATCHING_NEIGHBOURS, "Number of neighbours: ", 2, 0, 250, 1);

	tool_create_button("Start matching", tool_matching_standard);
	// tool_create_separator();
	// tool_create_button("Remove conflicts", tool_matching_remove_conflicts);
}

// start automatic matching 
void tool_matching_standard() 
{
	// save the current values of parameters 
	tool_fetch_parameters(tool_matching_id);
	
	// fetch settings 
	const double fsor_limit = tool_get_real(tool_matching_id, MATCHING_SIMILARITY_THRESHOLD);
	const int max_size = matching_resolution_values[tool_get_enum(tool_matching_id, MATCHING_RESOLUTION)];
	const double epipolar_distance_threshold = tool_get_real(tool_matching_id, MATCHING_IMAGE_MEASUREMENT_THRESHOLD);
	const bool skip_feature_extraction = tool_get_bool(tool_matching_id, MATCHING_SKIP_FEATURE_EXTRACTION);
	const bool use_ransac = tool_get_bool(tool_matching_id, MATCHING_F_RANSAC);
	const bool include_unverified = tool_get_bool(tool_matching_id, MATCHING_INCLUDE_UNVERIFIED);
	const int topology = tool_get_enum(tool_matching_id, MATCHING_TOPOLOGY);
	const int neighbours = tool_get_int(tool_matching_id, MATCHING_NEIGHBOURS);

	// extract features
	if (!skip_feature_extraction)
	{
		matching_extract_features(max_size);
	}

	// perform matching and extend correspondences into full-tracks 
	Matching_UF_Nodes uf_nodes;
	matching_extract_tracks(fsor_limit, use_ransac, include_unverified, epipolar_distance_threshold, uf_nodes, topology, neighbours);

	// take all classes of equivalence and create corresponding vertices
	for ALL(uf_nodes, i)
	{
		Matching_UF_Node * const node = uf_nodes.data + i; 
		if (!node->root || node->i < 2) continue; 

		// create vertex
		size_t vertex_id; 
		geometry_new_vertex(vertex_id);
		vertices.data[vertex_id].vertex_type = GEOMETRY_VERTEX_AUTO;
		node->i = vertex_id;
		node->root_with_vertex = true;
	}

	// create points for vertices
	for ALL(shots, i) 
	{
		const Shot * const shot = shots.data + i; 
		if (!shot->keypoints) continue;
		ASSERT(shot->matching, "matching meta not defined"); 
		Matching_Shot * meta = (Matching_Shot *)shot->matching;
		ASSERT(meta->width > 0 && meta->height > 0, "invalid picture sizes");

		// go through all features on this image
		for (size_t f = 0; f < shot->keypoints_count; f++) 
		{
			// take a look at it's root element
			bool found = false; 
			const size_t root_id = matching_find_parent_node(uf_nodes, shot->keypoints + f, found);

			if (found && uf_nodes.data[root_id].root_with_vertex) 
			{
				size_t point_id;
				geometry_new_point(point_id, shot->keypoints[f].x / meta->width, shot->keypoints[f].y / meta->height, i, uf_nodes.data[root_id].i);
			}
		}
	} 

	// release meta information of all shots  
	for ALL(shots, i)
	{
		Shot * const shot = shots.data + i; 
		if (shot->matching)
		{
			FREE(shot->matching);
		}
	}

	// finally delete inconsistent vertices 
	ui_empty_selection_list();
	matching_remove_conflicting_tracks();
}

/*// remove tracks occuring on a single shot more than once
void tool_matching_remove_conflicts()
{
	ui_empty_selection_list();
	matching_remove_conflicting_tracks();
}*/

// extract features 
// todo delete existing keypoints on relevant images
void matching_extract_features(const double max_size)
{
	// extract keypoints from all images 
	debug("extracting keypoints");

	double progress; 
	size_t shots_count = 0; 
	for ALL(shots, i)
	{
		shots_count++;
	}

	tool_start_progressbar(); 

	int shot_no = 0;
	for ALL(shots, i)
	{
		shot_no++;

		Shot * const shot = shots.data + i; 
		debug(shot->image_filename);

		// delete all previous information, if any 
		if (shot->matching) { FREE(shot->matching); shot->matching = NULL; }
		if (shot->keypoints) { free(shot->keypoints); shot->keypoints = NULL; shot->keypoints_count = 0; }
		if (shot->kd_tree) { kdtree_release(shot->kd_tree); shot->kd_tree = NULL; }

		// update progressbar
		tool_show_progress((shot_no - 1) * (1.0 / shots_count));

		// load the picture
		IplImage * img; 
		ATOMIC_RW(opencv, img = opencv_load_image(shot->image_filename, (int)max_size); );
		if (!img)
		{
			TOOL_PARTIAL_FAIL("Cannot load image from disk", continue);
		}

		// create new matching meta structure, since we'll regenerate the features
		shot->matching = ALLOC(Matching_Shot, 1);
		memset(shot->matching, 0, sizeof(Matching_Shot));

		// fill in image's meta-values
		Matching_Shot * const meta = (Matching_Shot *)shot->matching;
		meta->width = img->width;
		meta->height = img->height;

		// extract SIFT keypoints
		UNLOCK(geometry)
		{
			ATOMIC_RW(opencv, shot->keypoints_count = sift_features(img, &shot->keypoints); );
			printf("[count = %d]\n", shot->keypoints_count);
			fflush(stdout);
		}
		LOCK(geometry);

		// build kd tree
		ATOMIC_RW(opencv, shot->kd_tree = kdtree_build(shot->keypoints, shot->keypoints_count); ); // todo check if this is really necessary; maybe kdtree_build doesn't use opencv that much

		// if kd-tree failed, we release everything and mark as unmatched
		if (!shot->kd_tree)
		{
			FREE(shot->matching);
			shot->matching = NULL;
			free(shot->keypoints); 
			shot->keypoints = NULL; 
			shot->keypoints_count = 0;
			ATOMIC_RW(opencv, cvReleaseImage(&img); );
			TOOL_PARTIAL_FAIL("Failed to build kd-tree", continue);
		}

		// clear user defined pointer of all features
		for (size_t j = 0; j < shot->keypoints_count; j++)
		{
			shot->keypoints[j].feature_data = NULL;
		}

		// release resources 
		ATOMIC_RW(opencv, cvReleaseImage(&img); );
	}

	tool_end_progressbar();
}

// extract tracks
void matching_extract_tracks(const double fsor_limit, const bool use_ransac, const bool include_unverified, const double epipolar_distance_threshold, Matching_UF_Nodes & uf_nodes, const int topology, const int neighbours)
{
	const double fsor_limit_sq = fsor_limit * fsor_limit;

	DYN_INIT(uf_nodes);
	printf("x");

	// find maximum number of detected features
	int max_features_count = 0; 
	for ALL(shots, i) 
	{
		if (max_features_count < shots.data[i].keypoints_count)
		{
			max_features_count = shots.data[i].keypoints_count;
		}
	}

	if (max_features_count == 0) return;

	// allocate memory to store matches 
	int * matches = ALLOC(int, 2 * max_features_count);

	// count image pairs 
	size_t image_pairs_count = 0; 
	size_t images_count = 0;
	for ALL(shots, i) 
	{
		images_count++;

		for ALL(shots, j) 
		{
			image_pairs_count++; // note how currently this is very stupid 
		}
	}

	FILE * fplan = fopen("matching_plan.txt", "r"); 
	bool * plan = NULL;
	if (fplan) 
	{
		printf("Using matching plan.\n");
		plan = ALLOC(bool, images_count * images_count);
		memset(plan, 0, sizeof(bool) * images_count * images_count);
		while (!feof(fplan)) 
		{
			int img1, img2; 
			if (fscanf(fplan, "%d %d\n", &img1, &img2) == 2)
			{
				printf(".");
				plan[img1 * images_count + img2] = true; 
				plan[img2 * images_count + img1] = true; 
			}
		}
		fclose(fplan); 
	}
	else
	{
		printf("Not using matching plan.\n");
	}

	// match all image-pairs 
	tool_start_progressbar();
	int ith = 0;
	size_t image_pair_no = 0;
	for ALL(shots, i) 
	{
		printf("matching");
		fflush(stdout);
		ith++;
		Shot * const first_shot = shots.data + i; 
		if (!first_shot->kd_tree) continue;
		ASSERT(first_shot->matching, "metadata not loaded");
		Matching_Shot * first_meta = (Matching_Shot *)first_shot->matching;

		int jth = 0;
		for ALL(shots, j) 
		{
			jth++;
			image_pair_no++;
			tool_show_progress((image_pair_no - 1) * (1.0 / image_pairs_count));
			// printf("%f(%d, %d) ", (image_pair_no - 1) / (1.0 / image_pairs_count), image_pair_no, image_pairs_count);

			if (topology == MATCHING_TOPOLOGY_SEQUENCE && abs(ith - jth) > neighbours) continue;
			if (i == j) continue; 
			if (plan && !plan[ith * images_count + jth]) continue;
			Shot * const second_shot = shots.data + j;
			if (!second_shot->kd_tree) continue; 
			Matching_Shot * second_meta = (Matching_Shot *)second_shot->matching;

			// consider all keypoints in the first image and match them against keypoints from the second image 
			double * debug_dist = ALLOC(double, 2 * first_shot->keypoints_count);
			size_t correspondences = 0;
			printf("{");
			fflush(stdout);
			for (size_t keypoint = 0; keypoint < first_shot->keypoints_count; keypoint++)
			// for (size_t tries = 0; tries < 2000; tries++) // todo this causes overflow because the number of matches can now be greater that the number of features 
			{
				// size_t keypoint = rand() % first_shot->keypoints_count;

				feature * first_feature = first_shot->keypoints + keypoint;
				feature ** neighbours; 
				ATOMIC_RW(opencv, const int found = kdtree_bbf_knn(second_shot->kd_tree, first_feature, 2, &neighbours, 200); );

				if (found == 2) 
				{
					const double 
						d1 = descr_dist_sq(first_feature, neighbours[0]), 
						d2 = descr_dist_sq(first_feature, neighbours[1])
					; 

					if (d1 < fsor_limit_sq * d2)
					{
						matches[2 * correspondences + 0] = keypoint;
						matches[2 * correspondences + 1] = neighbours[0] - second_shot->keypoints;
						debug_dist[2 * keypoint + 0] = d1;
						debug_dist[2 * keypoint + 1] = d2;
						correspondences++;
					}
				}
			}
			printf("}");
			fflush(stdout);

			// optional RANSAC filtering 
			if (use_ransac && correspondences >= 18) 
			{
				// allocate structures
				ATOMIC_RW(opencv, 
					CvMat * first_points = cvCreateMat(2, correspondences, CV_64F); 
					CvMat * second_points = cvCreateMat(2, correspondences, CV_64F);
					CvMat * status = cvCreateMat(1, correspondences, CV_8S);
					cvZero(status);
					CvMat * F = cvCreateMat(3, 3, CV_64F);
				);

				// fill in the data
				for (size_t k = 0; k < correspondences; k++) 
				{
					feature 
						* const first_feature = first_shot->keypoints + matches[2 * k + 0],
						* const second_feature = second_shot->keypoints + matches[2 * k + 1];

					OPENCV_ELEM(first_points, 0, k) = first_feature->x / first_meta->width * first_shot->width;
					OPENCV_ELEM(first_points, 1, k) = first_feature->y / first_meta->height * first_shot->height;
					OPENCV_ELEM(second_points, 0, k) = second_feature->x / second_meta->width * second_shot->width;
					OPENCV_ELEM(second_points, 1, k) = second_feature->y / second_meta->height * second_shot->height;
				}

				// calculate the fundamental matrix
				ATOMIC_RW(opencv, cvFindFundamentalMat(first_points, second_points, F, CV_FM_RANSAC, epipolar_distance_threshold, 0.99, status); );

				// export into om, calculate average translation
				int * om = ALLOC(int, first_shot->keypoints_count);
				double tx = 0, ty = 0;
				size_t inliers_count = 0;
				memset(om, -1, first_shot->keypoints_count * sizeof(int));
				for (size_t k = 0; k < correspondences; k++) 
				{
					if (CV_MAT_ELEM(*status, signed char, 0, k) == 1)
					{
						om[matches[2 * k + 0]] = matches[2 * k + 1]; 
						tx += first_shot->keypoints[matches[2 * k + 0]].x - second_shot->keypoints[matches[2 * k + 1]].x;
						ty += first_shot->keypoints[matches[2 * k + 0]].y - second_shot->keypoints[matches[2 * k + 1]].y;
						inliers_count++;
					}
				}

				tx *= (second_shot->width / (double)second_meta->width) / inliers_count;
				ty *= (second_shot->width / (double)second_meta->width) / inliers_count;

				// duplicate the features 
				feature
					* features1_copy = ALLOC(feature, first_shot->keypoints_count),
					* features2_copy = ALLOC(feature, second_shot->keypoints_count)
				;

				memcpy(features1_copy, first_shot->keypoints, sizeof(feature) * first_shot->keypoints_count); 
				memcpy(features2_copy, second_shot->keypoints, sizeof(feature) * second_shot->keypoints_count); 

				for (size_t k = 0; k < first_shot->keypoints_count; k++)
				{
					features1_copy[k].feature_data = (void *)(first_shot->keypoints + k);
				}

				for (size_t k = 0; k < second_shot->keypoints_count; k++)
				{
					features2_copy[k].feature_data = (void *)(second_shot->keypoints + k);
				}

				// improve the number of correspondences using guided matching 
				printf("[");
				fflush(stdout);
				LOCK_RW(opencv)
				{
					correspondences = mvg_guided_matching(
						features1_copy, first_shot->keypoints_count, first_shot->width, first_shot->height, first_shot->width / (double)first_meta->width, 
						features2_copy, second_shot->keypoints_count, second_shot->width, second_shot->height, second_shot->width / (double)second_meta->width, 
						F,
						epipolar_distance_threshold,
						fsor_limit,
						matches
					);
					/*correspondences = mvg_guided_matching_translation(
						features1_copy, first_shot->keypoints_count, first_shot->width, first_shot->height, first_shot->width / (double)first_meta->width, 
						features2_copy, second_shot->keypoints_count, second_shot->width, second_shot->height, second_shot->width / (double)second_meta->width, 
						tx, ty,
						100,
						fsor_limit,
						matches
					);*/
				}
				UNLOCK_RW(opencv);

				printf("]");
				fflush(stdout);

				// save inliers
				for (size_t k = 0; k < correspondences; k++) 
				{
					matching_features_union(
						uf_nodes, 
						(feature *)(features1_copy[matches[2 * k + 0]].feature_data), 
						(feature *)(features2_copy[matches[2 * k + 1]].feature_data)
					);
				}

				FREE(features1_copy);
				FREE(features2_copy);
			}
			else if (!use_ransac || include_unverified)
			{
				// save all correspondences
				// note we could also remove these altogether
				for (size_t k = 0; k < 2 * correspondences; k += 2) 
				{
					matching_features_union(uf_nodes, first_shot->keypoints + matches[k + 0], second_shot->keypoints + matches[k + 1]);
				}
			}

		}

		printf("\n");
		fflush(stdout);
	}

	tool_end_progressbar();

	// release memory 
	FREE(matches);
}

// remove tracks occuring on a single shot more than once
void matching_remove_conflicting_tracks()
{
	// create array used to count occurences 
	bool * is_there = ALLOC(bool, shots.count);

	// go through all vertices 
	for ALL(vertices_incidence, i) 
	{
		const Vertex_Incidence * const incidence = vertices_incidence.data + i; 

		// for every vertex, check it's validity 
		memset(is_there, 0, sizeof(bool) * shots.count);
		bool valid = true; 
		for ALL(incidence->shot_point_ids, j)
		{
			const Double_Index * const index = incidence->shot_point_ids.data + j; 

			if (is_there[index->primary]) 
			{
				valid = false; 
				break; 
			}

			is_there[index->primary] = true;
		}

		// if it's not valid, remove it
		if (!valid) geometry_delete_vertex(i);
	}
}

// find parent node
size_t matching_find_parent_node(Matching_UF_Nodes & nodes, feature * node, bool & found) 
{
	if (!node->feature_data) 
	{
		found = false; 
		return SIZE_MAX;
	}

	size_t id = (Matching_UF_Node *)(node->feature_data) - (Matching_UF_Node *)(NULL) - 1;
	ASSERT_IS_SET(nodes, id);
	while (!nodes.data[id].root) 
	{
		id = nodes.data[id].i;
		ASSERT_IS_SET(nodes, id);
	}

	found = true; 
	return id;
}

// merge two tracks
void matching_features_union(Matching_UF_Nodes & nodes, feature * first, feature * second)
{
	// get corresponding root nodes 
	bool first_root_found, second_root_found; 
	size_t first_root_id, second_root_id; 
	first_root_id = matching_find_parent_node(nodes, first, first_root_found);
	second_root_id = matching_find_parent_node(nodes, second, second_root_found);
	size_t first_size = first_root_found ? nodes.data[first_root_id].i : 0; 
	size_t second_size = second_root_found ? nodes.data[second_root_id].i : 0; 
	ASSERT(first_size == 0 || IS_SET(nodes, first_root_id), "non-empty class of equivalence must have valid id");
	ASSERT(second_size == 0 || IS_SET(nodes, second_root_id), "non-empty class of equivalence must have valid id");

	// if these two already are in the same class of equivalence, we're done immediately 
	if (first_root_found && second_root_found && first_root_id == second_root_id) return;

	// swap the pointers so that the first feature is at least as big/valuable as the second 
	if (first_size < second_size)
	{
		feature * t = first; first = second; second = t;
		swap_bool(first_root_found, second_root_found); 
		swap_size_t(first_root_id, second_root_id);
		swap_size_t(first_size, second_size);
	}

	// if both of them are singletons, create brand new class of equivalence for this lucky pair 
	if (first_size == 0) 
	{
		ASSERT(second_size == 0, "logical inconsistency");
		ADD(nodes); 
		LAST(nodes).i = 2; 
		LAST(nodes).root = true; 
		first->feature_data = (void *)((Matching_UF_Node *)NULL + LAST_INDEX(nodes) + 1); // little bit hacky... 
		second->feature_data = first->feature_data;
		return;
	}

	// if only the second one is a singleton, merge it with the first one 
	if (second_size == 0) 
	{
		nodes.data[first_root_id].i++;
		second->feature_data = first->feature_data;
		return; 
	}

	// if both of them are not singletons, join the smaller one under the larger 
	nodes.data[first_root_id].i += nodes.data[second_root_id].i;
	nodes.data[second_root_id].root = false; 
	nodes.data[second_root_id].i = first_root_id;
}
