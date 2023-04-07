#ifndef __MVG_THRESHOLDS
#define __MVG_THRESHOLDS

// RANSAC
const int MVG_RANSAC_TRIALS = 500;
const int MVG_RANSAC_TRIANGULATION_TRIALS = 25;
const double MVG_RANSAC_PROBABILITY = 0.999;

/*// image measurement 
const double MVG_MEASUREMENT_THRESHOLD = 4.0;

// vertices
const int 
	MVG_MIN_INLIERS_TO_TRIANGULATE = 6,
	MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER = 4   // when there are no outliers in the dataset
;*/

// image measurement 
const double MVG_MEASUREMENT_THRESHOLD = 10.0;

// vertices
const int 
	MVG_MIN_INLIERS_TO_TRIANGULATE = 2,
	MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER = 2   // when there are no outliers in the dataset
;

#endif
