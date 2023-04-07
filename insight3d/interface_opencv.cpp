#include "interface_opencv.h"

pthread_mutex_t opencv_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

// create double precision matrix filled with zeros
CvMat * opencv_create_matrix(const size_t rows, const size_t cols) 
{
	CvMat * M = cvCreateMat(rows, cols, CV_64F); 
	cvZero(M); 
	return M;
}

// create double precision matrix filled with data from an array
CvMat * opencv_create_matrix(const size_t rows, const size_t cols, double * data)
{
	CvMat * M = opencv_create_matrix(rows, cols); 
	size_t iter = 0;
	for (size_t i = 0; i < rows; i++) 
	{
		for (size_t j = 0; j < cols; j++) 
		{
			OPENCV_ELEM(M, i, j) = data[iter++];
		}
	}
	return M;
}

// new identity matrix
CvMat * opencv_create_I_matrix(const size_t n) 
{
	CvMat * M = cvCreateMat(n, n, CV_64F); 
	cvZero(M); 

	for (size_t i = 0; i < n; i++) 
	{
		OPENCV_ELEM(M, i, i) = 1;
	}

	return M;
}

// create double precision column vector from array of doubles 
CvMat * opencv_create_vector(const double a[], const size_t length)
{
	// create vector
	CvMat * v = cvCreateMat(length, 1, CV_64F);

	// fill it with values
	for (size_t i = 0; i < length; i++)
	{
		CV_MAT_ELEM(*v, double, i, 0) = a[i]; 
	}

	return v;
}

// x axis rotation matrix 
// todo optimize 
// todo document the fact, that these matrices are transposes of conventional mathematical rotation matrices
CvMat * opencv_create_rotation_matrix_x(const double angle)
{
	CvMat * R = opencv_create_I_matrix(3);
	OPENCV_ELEM(R, 1, 1) = cos(angle); 
	OPENCV_ELEM(R, 1, 2) = -sin(angle); 
	OPENCV_ELEM(R, 2, 1) = sin(angle); 
	OPENCV_ELEM(R, 2, 2) = cos(angle);
	return R;
}

// y axis rotation matrix 
// todo optimize
CvMat * opencv_create_rotation_matrix_y(const double angle)
{
	CvMat * R = opencv_create_I_matrix(3);
	OPENCV_ELEM(R, 0, 0) = cos(angle); 
	OPENCV_ELEM(R, 0, 2) = sin(angle); 
	OPENCV_ELEM(R, 2, 0) = -sin(angle); 
	OPENCV_ELEM(R, 2, 2) = cos(angle);
	return R;
}

// z axis rotation matrix 
// todo optimize
CvMat * opencv_create_rotation_matrix_z(const double angle)
{
	CvMat * R = opencv_create_I_matrix(3);
	OPENCV_ELEM(R, 0, 0) = cos(angle); 
	OPENCV_ELEM(R, 0, 1) = -sin(angle); 
	OPENCV_ELEM(R, 1, 0) = sin(angle); 
	OPENCV_ELEM(R, 1, 1) = cos(angle);
	return R;
}

// euler angles to rotation matrix 
// todo optimize
CvMat * opencv_create_rotation_matrix_from_euler(const double euler[], bool reverse_order)
{
	CvMat * Rx = opencv_create_rotation_matrix_x(-euler[0] - OPENCV_PI); // todo move the substraction
	CvMat * Ry = opencv_create_rotation_matrix_y(-euler[1]);
	CvMat * Rz = opencv_create_rotation_matrix_z(-euler[2]);

	CvMat * temp = opencv_create_matrix(3, 3);
	CvMat * result = opencv_create_matrix(3, 3); 

	if (!reverse_order)
	{
		cvMatMul(Rz, Ry, temp);
		cvMatMul(temp, Rx, result);
	}
	else
	{
		cvMatMul(Rx, Ry, temp);
		cvMatMul(temp, Rz, result);
	}

	cvReleaseMat(&Rx);
	cvReleaseMat(&Ry);
	cvReleaseMat(&Rz);
	cvReleaseMat(&temp);

	return result; 
}

// project vertex - used only for visualization purposes
void opencv_vertex_projection_visualization(const CvMat * const P, const double x, const double y, const double z, double reprojection[])
{
	double w = OPENCV_ELEM(P, 2, 0) * x + OPENCV_ELEM(P, 2, 1) * y + OPENCV_ELEM(P, 2, 2) * z + OPENCV_ELEM(P, 2, 3);
	if (w == 0) w = 0.00001;
	reprojection[0] =
		(OPENCV_ELEM(P, 0, 0) * x + OPENCV_ELEM(P, 0, 1) * y + OPENCV_ELEM(P, 0, 2) * z + OPENCV_ELEM(P, 0, 3)) / w;
	reprojection[1] =
		(OPENCV_ELEM(P, 1, 0) * x + OPENCV_ELEM(P, 1, 1) * y + OPENCV_ELEM(P, 1, 2) * z + OPENCV_ELEM(P, 1, 3)) / w;
}

// project vertex - used only for visualization purposes
void opencv_vertex_projection_visualization(const CvMat * const P, const double x, const double y, const double z, const double w, double reprojection[])
{
	double v = OPENCV_ELEM(P, 2, 0) * x + OPENCV_ELEM(P, 2, 1) * y + OPENCV_ELEM(P, 2, 2) * z + OPENCV_ELEM(P, 2, 3) * w;
	if (v == 0) v = 0.00001;
	reprojection[0] =
		(OPENCV_ELEM(P, 0, 0) * x + OPENCV_ELEM(P, 0, 1) * y + OPENCV_ELEM(P, 0, 2) * z + OPENCV_ELEM(P, 0, 3) * w) / v;
	reprojection[1] =
		(OPENCV_ELEM(P, 1, 0) * x + OPENCV_ELEM(P, 1, 1) * y + OPENCV_ELEM(P, 1, 2) * z + OPENCV_ELEM(P, 1, 3) * w) / v;
}

// project vertex represented by homogeneous vector - used only for visualization purposes
void opencv_vertex_projection_visualization(const CvMat * const P, const CvMat * X, double reprojection[])
{
	double w = 
		OPENCV_ELEM(P, 2, 0) * OPENCV_ELEM(X, 0, 0) + 
		OPENCV_ELEM(P, 2, 1) * OPENCV_ELEM(X, 1, 0) + 
		OPENCV_ELEM(P, 2, 2) * OPENCV_ELEM(X, 2, 0) + 
		OPENCV_ELEM(P, 2, 3) * OPENCV_ELEM(X, 3, 0);

	if (w == 0) w = 0.00001;

	reprojection[0] = (
		OPENCV_ELEM(P, 0, 0) * OPENCV_ELEM(X, 0, 0) + 
		OPENCV_ELEM(P, 0, 1) * OPENCV_ELEM(X, 1, 0) + 
		OPENCV_ELEM(P, 0, 2) * OPENCV_ELEM(X, 2, 0) + 
		OPENCV_ELEM(P, 0, 3) * OPENCV_ELEM(X, 3, 0)
	) / w;

	reprojection[1] = (
		OPENCV_ELEM(P, 1, 0) * OPENCV_ELEM(X, 0, 0) + 
		OPENCV_ELEM(P, 1, 1) * OPENCV_ELEM(X, 1, 0) + 
		OPENCV_ELEM(P, 1, 2) * OPENCV_ELEM(X, 2, 0) + 
		OPENCV_ELEM(P, 1, 3) * OPENCV_ELEM(X, 3, 0)
	) / w;
}

// decompose rotation matrix into euler angles 
void opencv_rotation_matrix_to_angles(const CvMat * const R, double & ax, double & ay, double & az) 
{
	const double 
		r02 = OPENCV_ELEM(R, 0, 2),
		r12 = OPENCV_ELEM(R, 1, 2),
		r10 = OPENCV_ELEM(R, 1, 0), 
		r11 = OPENCV_ELEM(R, 1, 1), 
		r01 = OPENCV_ELEM(R, 0, 1), 
		r00 = OPENCV_ELEM(R, 0, 0),
		r22 = OPENCV_ELEM(R, 2, 2);

	if (r02 < 1) 
	{
		if (r02 > -1) 
		{
			ay = asin(r02); 
			ax = atan2(-1 * r12, r22); 
			az = atan2(-1 * r01, r00);
		}
		else
		{
			ay = -OPENCV_PI / 2.0; 
			ax = -atan2(r10, r11);
			az = 0;
		}
	}
	else
	{
		ay = OPENCV_PI / 2;
		ax = atan2(r10, r11);
		az = 0;
	}
}

// debug function prints matrix to standard output 
void opencv_debug(const char * title, CvMat * A) 
{
	if (title) printf("%s:\n", title);
	for (int i = 0; i < A->rows; i++) 
	{
		for (int j = 0; j < A->cols; j++) 
		{
			printf("%f\t", OPENCV_ELEM(A, i, j));
		}
		printf("\n");
	}
	printf("\n");
}

// downsize image
void opencv_downsize(IplImage ** img, const int max_size)
{
	if (max_size <= 0 || (*img)->width <= max_size && (*img)->height <= max_size)
	{
		return; 
	}

	int width = (*img)->width, height = (*img)->height; 
	while (width > max_size || height > max_size)
	{
		width = width / 1.2; 
		height = height / 1.2; 
	}

	IplImage * scaled_img = cvCreateImage(cvSize(width, height), (*img)->depth, (*img)->nChannels);
	cvResize(*img, scaled_img, CV_INTER_AREA);
	cvReleaseImage(img);
	*img = scaled_img; 
}

// create copy scaled down below some width threshold
IplImage * opencv_downsize_copy(IplImage * img, const int max_size)
{
	int width = img->width, height = img->height; 
	while (max_size > 0 && (width > max_size || height > max_size)) 
	{
		width /= 2; 
		height /= 2; 
	}

	IplImage * scaled_img = cvCreateImage(cvSize(width, height), img->depth, img->nChannels); 
	cvResize(img, scaled_img, CV_INTER_AREA);
	return scaled_img;
}

// load image and scale it down below some width threshold 
IplImage * opencv_load_image(const char * filename, const int max_size) 
{
	IplImage * img = cvLoadImage(filename, 1); 
	if (!img) return NULL;
	opencv_downsize(&img, max_size);
	return img; 
}

// creates trivial image
IplImage * opencv_create_substitute_image() 
{
	IplImage * img = cvCreateImage(cvSize(200, 200), IPL_DEPTH_8U, 3); 
 	cvZero(img); 
	return img;
}

// creates image with sides of the form 2^n
IplImage * opencv_create_exp_image(const int width, const int height, const int depth, const int channels) 
{
	int w = 1, h = 1; 
	while (w < width) w *= 2; 
	while (h < height) h *= 2; 
	return cvCreateImage(cvSize(w, h), depth, channels);
}

// calculate right null-vector of matrix A 
CvMat * opencv_right_null_vector(CvMat * A) 
{
	CvMat * W = opencv_create_matrix(A->cols, 1); // used to store singular values of A 
	CvMat * V_transposed = opencv_create_matrix(A->cols, A->cols); 
	cvSVD(A, W,	NULL, V_transposed, CV_SVD_V_T); // todo we could check W for numerical stability 
	CvMat * x = opencv_create_matrix(A->cols, 1); // result 

	// fill in the result 
	for (size_t i = 0; i < A->cols; i++) 
	{
		CV_MAT_ELEM(*x, double, i, 0) = CV_MAT_ELEM(*V_transposed, double, A->cols - 1, i);
	}

	cvReleaseMat(&W); 
	cvReleaseMat(&V_transposed);
	return x;
}

// calculate left null-vector of matrix A
CvMat * opencv_left_null_vector(CvMat * A) 
{
	CvMat * A_transposed = opencv_create_matrix(A->rows, A->cols); 
	cvTranspose(A, A_transposed);
	CvMat * x = opencv_right_null_vector(A_transposed);
	cvReleaseMat(&A_transposed);
	return x;
}

// create cross product matrix for 3-vector x 
CvMat * opencv_create_cross_product_matrix(CvMat * x) 
{
	CvMat * A = opencv_create_matrix(3, 3); 
	const double a1 = OPENCV_ELEM(x, 0, 0), a2 = OPENCV_ELEM(x, 1, 0), a3 = OPENCV_ELEM(x, 2, 0);

	OPENCV_ELEM(A, 0, 0) = 0; 
	OPENCV_ELEM(A, 0, 1) = -a3; 
	OPENCV_ELEM(A, 0, 2) = a2; 

	OPENCV_ELEM(A, 1, 0) = a3; 
	OPENCV_ELEM(A, 1, 1) = 0; 
	OPENCV_ELEM(A, 1, 2) = -a1; 

	OPENCV_ELEM(A, 2, 0) = -a2; 
	OPENCV_ELEM(A, 2, 1) = a1; 
	OPENCV_ELEM(A, 2, 2) = 0; 

	return A;
}

// scale vector homogeneous vector so that it's last coordinate is 1
void opencv_rescale_homogeneous_vector(CvMat * X) 
{
	double scale = 1.0 / OPENCV_ELEM(X, X->rows - 1, 0);
	for (int i = 0; i < X->rows; i++) 
	{
		OPENCV_ELEM(X, i, 0) *= scale;
	}
}

// normalize vector 
void opencv_normalize(CvMat * x) 
{
	double norm = 0; 
	for (int i = 0; i < x->rows; i++) 
	{
		norm += OPENCV_ELEM(x, i, 0) * OPENCV_ELEM(x, i, 0);
	}
	double scale = 1.0 / norm; 
	for (int i = 0; i < x->rows; i++) 
	{
		OPENCV_ELEM(x, i, 0) *= scale;
	}
}

// normalize vector 
void opencv_normalize_inhomogeneous(CvMat * x) 
{
	if (x->rows < 2) return; 
	double norm = 0; 
	for (int i = 0; i < x->rows - 1; i++) 
	{
		norm += OPENCV_ELEM(x, i, 0) * OPENCV_ELEM(x, i, 0);
	}
	double scale = 1.0 / sqrt(norm); 
	for (int i = 0; i < x->rows; i++) 
	{
		OPENCV_ELEM(x, i, 0) *= scale;
	}
}

// normalize vector 
void opencv_normalize_homogeneous(CvMat * x) 
{
	if (x->rows < 2) return; 
	double scale = 1.0 / OPENCV_ELEM(x, x->rows - 1, 0); 
	for (int i = 0; i < x->rows; i++) 
	{
		OPENCV_ELEM(x, i, 0) *= scale;
	}
}

// calculate epipolar line
void opencv_epipolar(const CvMat * const F, const double x, const double y, double & a, double & b, double & c)
{
	a = OPENCV_ELEM(F, 0, 0) * x + OPENCV_ELEM(F, 0, 1) * y + OPENCV_ELEM(F, 0, 2);
	b = OPENCV_ELEM(F, 1, 0) * x + OPENCV_ELEM(F, 1, 1) * y + OPENCV_ELEM(F, 1, 2);
	c = OPENCV_ELEM(F, 2, 0) * x + OPENCV_ELEM(F, 2, 1) * y + OPENCV_ELEM(F, 2, 2);

	const double scale = (a != 0 || b != 0) ? 1 / sqrt(a * a + b * b) : 1;

	a *= scale;
	b *= scale;
	c *= scale;
}

// point-in-polygon test
bool opencv_pip(const double x, const double y, const CvMat * polygon)
{
	int i, j = polygon->cols - 1;
	bool oddNodes = false;

	for (i = 0; i < polygon->cols; i++)
	{
		if (
			OPENCV_ELEM(polygon, 1, i) < y && OPENCV_ELEM(polygon, 1, j) >= y ||
			OPENCV_ELEM(polygon, 1, j) < y && OPENCV_ELEM(polygon, 1, i) >= y
		)
		{
			if (
				OPENCV_ELEM(polygon, 0, i) + (y - OPENCV_ELEM(polygon, 1, i)) /
				(OPENCV_ELEM(polygon, 1, j) - OPENCV_ELEM(polygon, 1, i)) * (OPENCV_ELEM(polygon, 0, j) - OPENCV_ELEM(polygon, 0, i))
				< x
			)
			{
				oddNodes =! oddNodes;
			}
		}

		j = i;
	}

	return oddNodes;
}
