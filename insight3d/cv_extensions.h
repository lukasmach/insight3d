#ifndef __CV_EXTENSIONS
#define __CV_EXTENSIONS

#include "interface_opencv.h"

void cvComputeRQDecomposition(CvMat * matrixM, CvMat * matrixR, CvMat * matrixQ, CvMat * matrixQx, CvMat * matrixQy, CvMat * matrixQz, CvPoint3D64f * eulerAngles);

#endif
