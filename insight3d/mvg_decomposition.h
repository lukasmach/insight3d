#ifndef __MVG_DECOMPOSITION
#define __MVG_DECOMPOSITION

#include "core_debug.h"
#include "interface_opencv.h"

// decomposition of projection matrix into rotation, translation and internal calibration 
bool mvg_finite_projection_matrix_decomposition(CvMat * const P, CvMat * const K, CvMat * const R, CvMat * const T);

// assemble projection matrix
void mvg_assemble_projection_matrix(CvMat * internal_calibration, CvMat * rotation, CvMat * translation, CvMat * projection);

#endif
