#ifndef __MVG_AUTOCALIBRATION
#define __MVG_AUTOCALIBRATION

#include "core_debug.h"
#include "interface_opencv.h"
#include "mvg_decomposition.h"

// perform autocalibration using absolute quadric
bool mvg_autocalibration(CvMat ** Ps, double * principal_points, const size_t n, CvMat ** Xs, const size_t m);

// get the coefficient in autocalibrating equations
double q(CvMat * P, int i, int j, int c);

// perform autocalibration using absolute quadric
bool mvg_autocalibration_2(CvMat ** Ps, double * principal_points, const size_t n, CvMat ** Xs, const size_t m, CvMat ** pi_infinity = NULL, bool affine = false);

#endif
