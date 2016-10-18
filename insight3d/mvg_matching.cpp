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

#include "mvg_matching.h"

size_t * mvg_index_buckets(MVG_FEATURE * features, double scale, size_t count, int bucket_size, int bucket_cols, int bucket_rows)
{
	size_t * index = ALLOC(size_t, bucket_cols * bucket_rows + 1);
	int current_bucket = -1;

	size_t i;
	for (i = 0; i < count; i++)
	{
		int 
			col = scale * features[i].x / bucket_size,
			row = scale * features[i].y / bucket_size
		;

		// skip features out of picture (perhaps manually created by the user or malformed import)
		if (col < 0 || row < 0 || col >= bucket_cols || row >= bucket_rows) continue;

		int point_bucket = row * bucket_cols + col;

		// if the vertices are not ordered in buckets, terminate 
		if (point_bucket < current_bucket)
		{
			FREE(index);
			return NULL;
		}
		// if the vertex is in a bucket in a gallaxy far far away
		else if (point_bucket > current_bucket)
		{
			current_bucket++;
			while (current_bucket < point_bucket)
			{
				index[current_bucket++] = i;
			}
			index[current_bucket] = i;
		}
	}

	// mark the rest of the buckets as empty
	while (current_bucket <= bucket_cols * bucket_rows) 
	{
		index[current_bucket++] = count;
	}

	return index;
}

int mvg_bucket_size = 0;
double mvg_scale = 0;

int mvg_buckets_compare(const void * p_f1, const void * p_f2) 
{
	MVG_FEATURE * f1 = (MVG_FEATURE *)p_f1, * f2 = (MVG_FEATURE *)p_f2; 
	size_t 
		bucket1x = mvg_scale * f1->x / mvg_bucket_size, 
		bucket1y = mvg_scale * f1->y / mvg_bucket_size, 
		bucket2x = mvg_scale * f2->x / mvg_bucket_size, 
		bucket2y = mvg_scale * f2->y / mvg_bucket_size
	;

	if (bucket1y < bucket2y) // todo can this be optimized
	{
		return -1;
	}
	else if (bucket1y > bucket2y) 
	{
		return 1;
	}
	else if (bucket1x < bucket2x) 
	{
		return -1;
	}
	else if (bucket1x > bucket2x) 
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

size_t * mvg_build_buckets(MVG_FEATURE * features, const double scale, size_t count, int bucket_size, int buckets_x, int buckets_y) 
{
	mvg_bucket_size = bucket_size; 
	mvg_scale = scale;
	qsort(features, count, sizeof(MVG_FEATURE), mvg_buckets_compare);

	// verify that it's sorted 
	/*for (size_t i = 0; i < count; i++) 
	{
		MVG_FEATURE * f = features + i;

		int row = f->y / bucket_size;
		int col = f->x / bucket_size; 

		printf("%d ", row * buckets_x + col);
	}

	int i;
	// scanf("%d", &i);*/

	return mvg_index_buckets(features, scale, count, bucket_size, buckets_x, buckets_y);
}

size_t mvg_guided_matching(
	MVG_FEATURE * features1, const size_t count1, const int width1, const int height1, const double scale1, 
	MVG_FEATURE * features2, const size_t count2, const int width2, const int height2, const double scale2, 
	CvMat * F,
	const double threshold,
	const double fsor_threshold,
	int * matches
)
{
	// calculate number of buckets and other constants
	const int bucket_size = 50; 
	const double fsor_threshold_sq = fsor_threshold * fsor_threshold;
	const int
		buckets1_x = width1 / bucket_size + 1,
		buckets1_y = height1 / bucket_size + 1,
		buckets2_x = width2 / bucket_size + 1,
		buckets2_y = height2 / bucket_size + 1 // note that this implies more buckets than really needed in rare cases
	;

	// * index the buckets *
	size_t * index2 = NULL; // mvg_index_buckets(features2, scale2, count2, bucket_size, buckets2_x, buckets2_y);
	if (!index2)
	{
		index2 = mvg_build_buckets(features2, scale2, count2, bucket_size, buckets2_x, buckets2_y); 
		printf(".");
	}

	// * match features *
	int matches_count = 0; // number of found matches

	// go through all vertices in the first image
	for (size_t i = 0; i < count1; i++) 
	{
		MVG_FEATURE * f1 = features1 + i;

		// calculate the epipolar line
		CvMat * e = opencv_create_matrix(3, 1);
		double x = features1[i].x * scale1, y = features1[i].y * scale1;
		OPENCV_ELEM(e, 0, 0) = x;
		OPENCV_ELEM(e, 1, 0) = y;
		OPENCV_ELEM(e, 2, 0) = 1.0;
		cvMatMul(F, e, e);
		const double norm = sqrt(sqr_value(OPENCV_ELEM(e, 0, 0)) + sqr_value(OPENCV_ELEM(e, 1, 0)));
		OPENCV_ELEM(e, 0, 0) *= 1 / norm; 
		OPENCV_ELEM(e, 1, 0) *= 1 / norm; 
		OPENCV_ELEM(e, 2, 0) *= 1 / norm; 

		int checked_buckets = 0; // we'll be counting the buckets we checked

		// go through all buckets on the second image, we'll be keeping running min 
		double min1 = scale2 * scale2 * width2 * height2 + 2, min2 = scale2 * scale2 * width2 * height2 + 3; // note the + 2 is there only for the (unrealistic) case that width and height are both below 1
		ASSERT(min1 != min2, "image resolution is too large to store the pixel count in double; this should be easy to fix");
		size_t min1_id = SIZE_MAX, min2_id = SIZE_MAX;
		size_t checked_features = 0;
		for (size_t j = 0; j < buckets2_x * buckets2_y; j++)
		{
			// decide if the bucket is near the epipolar line
			const int 
				bucket_x = (j % buckets2_x) * bucket_size + bucket_size / 2, 
				bucket_y = (j / buckets2_x) * bucket_size + bucket_size / 2
			;
			
			const double distance = abs(bucket_x * OPENCV_ELEM(e, 0, 0) + bucket_y * OPENCV_ELEM(e, 1, 0) + OPENCV_ELEM(e, 2, 0));
			if (distance <= (0.5 * sqrt(2.0) * bucket_size + threshold))
			{
				// checked_buckets++; // todo 

				// go through all vertices in this bucket
				for (size_t k = index2[j]; k < index2[j + 1]; k++)
				{
					checked_features++;
					// is it near the epipolar line
					const double distance = abs(scale2 * features2[k].x * OPENCV_ELEM(e, 0, 0) + scale2 * features2[k].y * OPENCV_ELEM(e, 1, 0) + OPENCV_ELEM(e, 2, 0));
					if (distance <= threshold)
					{
						// calculate descriptor distance
						double descriptor_d = 0; 
						for (int l = 0; l < 128; l += 4) 
						{
							descriptor_d += sqr_value(f1->descr[l] - features2[k].descr[l]);
							descriptor_d += sqr_value(f1->descr[l + 1] - features2[k].descr[l + 1]);
							descriptor_d += sqr_value(f1->descr[l + 2] - features2[k].descr[l + 2]);
							descriptor_d += sqr_value(f1->descr[l + 3] - features2[k].descr[l + 3]);

							if (descriptor_d > min2) break;
						}

						// if it's closer than the running minimum, change it
						if (descriptor_d < min1) 
						{
							min2 = min1; 
							min2_id = min1_id;
							min1 = descriptor_d; 
							min1_id = k;
						}
						else if (descriptor_d < min2) 
						{
							min2 = descriptor_d;
							min2_id = k;
						}
					}
				}
			}
		}

		// feature space outlier check
		if (min1 / min2 <= fsor_threshold_sq) 
		{
			matches[2 * matches_count + 0] = i;
			matches[2 * matches_count + 1] = min1_id;
			matches_count++;
		}
	}

	FREE(index2);
	return matches_count;
}

size_t mvg_guided_matching_translation(
	MVG_FEATURE * features1, const size_t count1, const int width1, const int height1, const double scale1, 
	MVG_FEATURE * features2, const size_t count2, const int width2, const int height2, const double scale2, 
	const double T_x, const double T_y,
	const double threshold,
	const double fsor_threshold,
	int * matches
)
{
	// calculate number of buckets and other constants
	const int bucket_size = 50; 
	const double fsor_threshold_sq = fsor_threshold * fsor_threshold;
	const int
		buckets1_x = width1 / bucket_size + 1,
		buckets1_y = height1 / bucket_size + 1,
		buckets2_x = width2 / bucket_size + 1,
		buckets2_y = height2 / bucket_size + 1 // note that this implies more buckets than really needed in rare cases
	;

	// * index the buckets *
	size_t * index2 = NULL; // mvg_index_buckets(features2, scale2, count2, bucket_size, buckets2_x, buckets2_y);
	if (!index2)
	{
		index2 = mvg_build_buckets(features2, scale2, count2, bucket_size, buckets2_x, buckets2_y); 
		printf(".");
	}

	// * match features *
	int matches_count = 0; // number of found matches

	// go through all vertices in the first image
	for (size_t i = 0; i < count1; i++) 
	{
		MVG_FEATURE * f1 = features1 + i;

		// calculate approximate position on the second image 
		const double 
			x = scale1 * f1->x + T_x, 
			y = scale1 * f1->y + T_y
		;

		int checked_buckets = 0; // we'll be counting the buckets we checked

		// go through all buckets on the second image that are close enough, we'll be keeping running min 
		double min1 = scale2 * scale2 * width2 * height2 + 2, min2 = scale2 * scale2 * width2 * height2 + 3; // note the + 2 is there only for the (unrealistic) case that width and height are both below 1
		ASSERT(min1 != min2, "image resolution is too large to store the pixel count in double; this should be easy to fix");
		size_t min1_id = SIZE_MAX, min2_id = SIZE_MAX;
		for (size_t j = 0; j < buckets2_x * buckets2_y; j++)
		{
			// decide if the bucket is near the approximate position
			const int 
				bucket_x = (j % buckets2_x) * bucket_size + bucket_size / 2, 
				bucket_y = (j / buckets2_x) * bucket_size + bucket_size / 2
			;
			
			const double distance = sqrt(sqr_value(x - bucket_x) + sqr_value(y - bucket_y));
			if (distance <= (0.5 * sqrt(2.0) * bucket_size + threshold))
			{
				checked_buckets++;

				// go through all vertices in this bucket
				for (size_t k = index2[j]; k < index2[j + 1]; k++)
				{
					// is it near the epipolar line
					const double distance = sqrt(sqr_value(x - scale2 * features2[k].x) + sqr_value(y - scale2 * features2[k].y));
					if (distance <= threshold)
					{
						// calculate descriptor distance
						double descriptor_d = 0;
						for (int l = 0; l < 128; l += 4)
						{
							descriptor_d += sqr_value(f1->descr[l] - features2[k].descr[l]);
							descriptor_d += sqr_value(f1->descr[l + 1] - features2[k].descr[l + 1]);
							descriptor_d += sqr_value(f1->descr[l + 2] - features2[k].descr[l + 2]);
							descriptor_d += sqr_value(f1->descr[l + 3] - features2[k].descr[l + 3]);

							if (descriptor_d > min2) break;
						}

						// if it's closer than the running minimum, change it
						if (descriptor_d < min1)
						{
							min2 = min1;
							min2_id = min1_id;
							min1 = descriptor_d;
							min1_id = k;
						}
						else if (descriptor_d < min2)
						{
							min2 = descriptor_d;
							min2_id = k;
						}
					}
				}
			}
		}

		// feature space outlier check
		if (min1 / min2 <= fsor_threshold_sq)
		{
			matches[2 * matches_count + 0] = i;
			matches[2 * matches_count + 1] = min1_id;
			matches_count++;
		}
	}

	FREE(index2);
	return matches_count;
}
