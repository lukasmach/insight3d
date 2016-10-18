#ifndef __NORMALIZATION
#define __NORMALIZATION

#include "core_debug.h"
#include "interface_opencv.h"

// normalize image points so that their centroid and typical magnitude of 
// the vector is (1, 1)
bool mvg_normalize_points(CvMat * points, CvMat * H, double * output_scale = NULL);

#endif
