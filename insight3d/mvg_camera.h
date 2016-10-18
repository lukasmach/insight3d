#ifndef __MVG_CAMERA
#define __MVG_CAMERA

#include "core_debug.h"
#include "interface_opencv.h"

// computes projective depth of a point X with respect to finite camera P
double mvg_projective_depth(CvMat * P, CvMat * X);

// returns true if the point is in front of finite camera 
bool mvg_point_in_front_of_camera(CvMat * P, CvMat * X);

// extracts canonical pair of projective matrices from fundamental matrix
bool mvg_extract_Ps_from_F(CvMat * F, CvMat * P1, CvMat * P2);

// calculates fundamental matrix for a pair of views given their projective matrices 
bool mvg_calculate_F_from_Ps(CvMat * P1, CvMat * P2, CvMat * F);

#endif
