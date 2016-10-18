#ifndef __MVG_MATCHING
#define __MVG_MATCHING

#include "core_debug.h"
#include "interface_opencv.h"
#include "core_math_routines.h"
#include "geometry_structures.h"

// we have to reference the type containing the structure
#define MVG_FEATURE struct feature

size_t * mvg_index_buckets(
	MVG_FEATURE * features, 
	double scale, 
	size_t count, 
	int bucket_size, 
	int bucket_cols, 
	int bucket_rows
);

size_t * mvg_build_buckets(
	MVG_FEATURE * features, 
	const double scale, 
	size_t count, 
	int bucket_size, 
	int buckets_x, 
	int buckets_y
);

size_t mvg_guided_matching(
	MVG_FEATURE * features1, const size_t count1, const int width1, const int height1, const double scale1, 
	MVG_FEATURE * features2, const size_t count2, const int width2, const int height2, const double scale2, 
	CvMat * F,
	const double threshold,
	const double fsor_threshold,
	int * matches
);

size_t mvg_guided_matching_translation(
	MVG_FEATURE * features1, const size_t count1, const int width1, const int height1, const double scale1, 
	MVG_FEATURE * features2, const size_t count2, const int width2, const int height2, const double scale2, 
	const double T_x, const double T_y,
	const double threshold,
	const double fsor_threshold,
	int * matches
);

#endif
