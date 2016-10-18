#include "cv_extensions.h"
#define CV_VERYSMALLDOUBLE 1.0e-10

void cvComputeRQDecomposition(CvMat *matrixM, CvMat *matrixR, CvMat *matrixQ, CvMat *matrixQx, CvMat *matrixQy, CvMat *matrixQz, CvPoint3D64f *eulerAngles)
{
	
	CvMat *tmpMatrix1 = 0;
	CvMat *tmpMatrix2 = 0;
	CvMat *tmpMatrixM = 0;
	CvMat *tmpMatrixR = 0;
	CvMat *tmpMatrixQ = 0;
	CvMat *tmpMatrixQx = 0;
	CvMat *tmpMatrixQy = 0;
	CvMat *tmpMatrixQz = 0;
	double tmpEulerAngleX, tmpEulerAngleY, tmpEulerAngleZ;
	
	CV_FUNCNAME("cvRQDecomp3x3");
    __BEGIN__;
	
	/* Validate parameters. */
	if(matrixM == 0 || matrixR == 0 || matrixQ == 0)
		CV_ERROR(CV_StsNullPtr, "Some of parameters is a NULL pointer!");
	
	if(!CV_IS_MAT(matrixM) || !CV_IS_MAT(matrixR) || !CV_IS_MAT(matrixQ))
		CV_ERROR(CV_StsUnsupportedFormat, "Input parameters must be a matrices!");
	
	if(matrixM->cols != 3 || matrixM->rows != 3 || matrixR->cols != 3 || matrixR->rows != 3 || matrixQ->cols != 3 || matrixQ->rows != 3)
		CV_ERROR(CV_StsUnmatchedSizes, "Size of matrices must be 3x3!");
	
	CV_CALL(tmpMatrix1 = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrix2 = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrixM = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrixR = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrixQ = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrixQx = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrixQy = cvCreateMat(3, 3, CV_64F));
	CV_CALL(tmpMatrixQz = cvCreateMat(3, 3, CV_64F));
	
	cvCopy(matrixM, tmpMatrixM);
	
	/* Find Givens rotation Q_x for x axis. */
	/*
	      ( 1  0  0 )
	 Qx = ( 0  c -s ), cos = -m33/sqrt(m32^2 + m33^2), cos = m32/sqrt(m32^2 + m33^2)
		  ( 0  s  c )
	 */
	
	double x, y, z, c, s;
	x = cvmGet(tmpMatrixM, 2, 1);
	y = cvmGet(tmpMatrixM, 2, 2);
	z = x * x + y * y;
	assert(z != 0); // Prevent division by zero.
	c = -y / sqrt(z);
	s = x / sqrt(z);
	
	cvSetIdentity(tmpMatrixQx);
	cvmSet(tmpMatrixQx, 1, 1, c);
	cvmSet(tmpMatrixQx, 1, 2, -s);
	cvmSet(tmpMatrixQx, 2, 1, s);
	cvmSet(tmpMatrixQx, 2, 2, c);
	
	tmpEulerAngleX = acos(c) * 180.0 / CV_PI;
	
	/* Multiply M on the right by Q_x. */
	
	cvMatMul(tmpMatrixM, tmpMatrixQx, tmpMatrixR);
	cvCopy(tmpMatrixR, tmpMatrixM);
	
	assert(cvmGet(tmpMatrixM, 2, 1) < CV_VERYSMALLDOUBLE && cvmGet(tmpMatrixM, 2, 1) > -CV_VERYSMALLDOUBLE); // Should actually be zero.
	
	if(cvmGet(tmpMatrixM, 2, 1) != 0.0)
		cvmSet(tmpMatrixM, 2, 1, 0.0); // Rectify arithmetic precision error.
	
	/* Find Givens rotation for y axis. */
	/*
	      ( c  0  s )
	 Qy = ( 0  1  0 ), cos = m33/sqrt(m31^2 + m33^2), cos = m31/sqrt(m31^2 + m33^2)
	      (-s  0  c )
	 */
	
	x = cvmGet(tmpMatrixM, 2, 0);
	y = cvmGet(tmpMatrixM, 2, 2);
	z = x * x + y * y;
	assert(z != 0); // Prevent division by zero.
	c = y / sqrt(z);
	s = x / sqrt(z);
	
	cvSetIdentity(tmpMatrixQy);
	cvmSet(tmpMatrixQy, 0, 0, c);
	cvmSet(tmpMatrixQy, 0, 2, s);
	cvmSet(tmpMatrixQy, 2, 0, -s);
	cvmSet(tmpMatrixQy, 2, 2, c);
	
	tmpEulerAngleY = acos(c) * 180.0 / CV_PI;
	
	/* Multiply M*Q_x on the right by Q_y. */
	
	cvMatMul(tmpMatrixM, tmpMatrixQy, tmpMatrixR);
	cvCopy(tmpMatrixR, tmpMatrixM);
	
	assert(cvmGet(tmpMatrixM, 2, 0) < CV_VERYSMALLDOUBLE && cvmGet(tmpMatrixM, 2, 0) > -CV_VERYSMALLDOUBLE); // Should actually be zero.
	
	if(cvmGet(tmpMatrixM, 2, 0) != 0.0)
		cvmSet(tmpMatrixM, 2, 0, 0.0); // Rectify arithmetic precision error.
	
	/* Find Givens rotation for z axis. */
	/*
	      ( c -s  0 )
	 Qz = ( s  c  0 ), cos = -m22/sqrt(m21^2 + m22^2), cos = m21/sqrt(m21^2 + m22^2)
	      ( 0  0  1 )
	 */
	
	x = cvmGet(tmpMatrixM, 1, 0);
	y = cvmGet(tmpMatrixM, 1, 1);
	z = x * x + y * y;
	assert(z != 0); // Prevent division by zero.
	c = -y / sqrt(z);
	s = x / sqrt(z);
	
	cvSetIdentity(tmpMatrixQz);
	cvmSet(tmpMatrixQz, 0, 0, c);
	cvmSet(tmpMatrixQz, 0, 1, -s);
	cvmSet(tmpMatrixQz, 1, 0, s);
	cvmSet(tmpMatrixQz, 1, 1, c);
	
	tmpEulerAngleZ = acos(c) * 180.0 / CV_PI;
	
	/* Multiply M*Q_x*Q_y on the right by Q_z. */
	
	cvMatMul(tmpMatrixM, tmpMatrixQz, tmpMatrixR);
	
	assert(cvmGet(tmpMatrixR, 1, 0) < CV_VERYSMALLDOUBLE && cvmGet(tmpMatrixR, 1, 0) > -CV_VERYSMALLDOUBLE); // Should actually be zero.
	
	if(cvmGet(tmpMatrixR, 1, 0) != 0.0)
		cvmSet(tmpMatrixR, 1, 0, 0.0); // Rectify arithmetic precision error.
	
	/* Calulate orthogonal matrix. */
	/*
	 Q = QzT * QyT * QxT
	 */
	
	cvTranspose(tmpMatrixQz, tmpMatrix1);
	cvTranspose(tmpMatrixQy, tmpMatrix2);
	cvMatMul(tmpMatrix1, tmpMatrix2, tmpMatrixQ);
	cvCopy(tmpMatrixQ, tmpMatrix1);
	cvTranspose(tmpMatrixQx, tmpMatrix2);
	cvMatMul(tmpMatrix1, tmpMatrix2, tmpMatrixQ);
	
	/* Solve decomposition ambiguity. */
	/*
	 Diagonal entries of R should be positive, so swap signs if necessary.
	 */
	
	if(cvmGet(tmpMatrixR, 0, 0) < 0.0) {
		cvmSet(tmpMatrixR, 0, 0, -1.0 * cvmGet(tmpMatrixR, 0, 0));
		cvmSet(tmpMatrixQ, 0, 0, -1.0 * cvmGet(tmpMatrixQ, 0, 0));
		cvmSet(tmpMatrixQ, 0, 1, -1.0 * cvmGet(tmpMatrixQ, 0, 1));
		cvmSet(tmpMatrixQ, 0, 2, -1.0 * cvmGet(tmpMatrixQ, 0, 2));
	}
	if(cvmGet(tmpMatrixR, 1, 1) < 0.0) {
		cvmSet(tmpMatrixR, 0, 1, -1.0 * cvmGet(tmpMatrixR, 0, 1));
		cvmSet(tmpMatrixR, 1, 1, -1.0 * cvmGet(tmpMatrixR, 1, 1));
		cvmSet(tmpMatrixQ, 1, 0, -1.0 * cvmGet(tmpMatrixQ, 1, 0));
		cvmSet(tmpMatrixQ, 1, 1, -1.0 * cvmGet(tmpMatrixQ, 1, 1));
		cvmSet(tmpMatrixQ, 1, 2, -1.0 * cvmGet(tmpMatrixQ, 1, 2));
	}

	/* Enforce det(Q) = 1 */ 
	if (cvDet(tmpMatrixQ) < 0) 
	{
		for (int i = 0; i < 3; i++) 
		{
			for (int j = 0; j < 3; j++) 
			{
				cvmSet(tmpMatrixQ, j, i, -cvmGet(tmpMatrixQ, j, i)); 
			}
		}
	}
	
	/* Save R and Q matrices. */
	
	cvCopy(tmpMatrixR, matrixR);
	cvCopy(tmpMatrixQ, matrixQ);
	
	if(matrixQx && CV_IS_MAT(matrixQx) && matrixQx->cols == 3 || matrixQx->rows == 3)
		cvCopy(tmpMatrixQx, matrixQx);
	if(matrixQy && CV_IS_MAT(matrixQy) && matrixQy->cols == 3 || matrixQy->rows == 3)
		cvCopy(tmpMatrixQy, matrixQy);
	if(matrixQz && CV_IS_MAT(matrixQz) && matrixQz->cols == 3 || matrixQz->rows == 3)
		cvCopy(tmpMatrixQz, matrixQz);
	
	/* Save Euler angles. */
	
	if(eulerAngles)
		*eulerAngles = cvPoint3D64f(tmpEulerAngleX, tmpEulerAngleY, tmpEulerAngleZ);
	
	__END__;
	
	cvReleaseMat(&tmpMatrix1);
	cvReleaseMat(&tmpMatrix2);
	cvReleaseMat(&tmpMatrixM);
	cvReleaseMat(&tmpMatrixR);
	cvReleaseMat(&tmpMatrixQ);
	cvReleaseMat(&tmpMatrixQx);
	cvReleaseMat(&tmpMatrixQy);
	cvReleaseMat(&tmpMatrixQz);

}
