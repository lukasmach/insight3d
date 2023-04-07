#ifndef __INTERFACE_OPENCV
#define __INTERFACE_OPENCV

#include "stdio.h"
#include "pthread.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"

extern pthread_mutex_t opencv_mutex;

const double OPENCV_PI = 3.14159265358979323846;

// macro for accessing double precision matrix elements // todo avoid using OpenCV macro
#define OPENCV_ELEM(matrix, i, j) (CV_MAT_ELEM((*(matrix)), double, (i), (j)))

// create double precision matrix filled with zeros
CvMat * opencv_create_matrix(const size_t rows, const size_t cols);

// create double precision matrix filled with data from an array
CvMat * opencv_create_matrix(const size_t rows, const size_t cols, double data[]);

// new identity matrix
CvMat * opencv_create_I_matrix(const size_t n);

// create double precision column vector from array of doubles 
CvMat * opencv_create_vector(const double a[], const size_t length);

// x axis rotation matrix 
// todo optimize 
// note these matrices are transposes of conventional mathematical rotation matrices 
// (which is ugly, but it just so happens, that we need it this way)
CvMat * opencv_create_rotation_matrix_x(const double angle);

// y axis rotation matrix 
// todo optimize
CvMat * opencv_create_rotation_matrix_y(const double angle);

// z axis rotation matrix 
// todo optimize
CvMat * opencv_create_rotation_matrix_z(const double angle);

// euler angles to rotation matrix 
// todo optimize
CvMat * opencv_create_rotation_matrix_from_euler(const double euler[], bool reverse_order = true);

// project vertex - used only for visualization purposes
void opencv_vertex_projection_visualization(const CvMat * const P, const double x, const double y, const double z, double reprojection[]);

// project vertex - used only for visualization purposes
void opencv_vertex_projection_visualization(const CvMat * const P, const double x, const double y, const double z, const double w, double reprojection[]);

// project vertex represented by homogeneous vector - used only for visualization purposes
void opencv_vertex_projection_visualization(const CvMat * const P, const CvMat * X, double reprojection[]);

// decompose rotation matrix into euler angles 
void opencv_rotation_matrix_to_angles(const CvMat * const R, double & ax, double & ay, double & az);

// debug function prints matrix to standard output 
void opencv_debug(const char * title, CvMat * A);

// downsize image
void opencv_downsize(IplImage ** img, const int max_size);

// create copy scaled down below some width threshold
IplImage * opencv_downsize_copy(IplImage * img, const int max_size);

// load image and scale it down below some width threshold 
IplImage * opencv_load_image(const char * filename, const int max_size);

// creates trivial image
IplImage * opencv_create_substitute_image();

// creates image with sides of the form 2^n
IplImage * opencv_create_exp_image(const int width, const int height, const int depth, const int channels);

// calculate right null-vector of matrix A 
CvMat * opencv_right_null_vector(CvMat * A);

// calculate left null-vector of matrix A
CvMat * opencv_left_null_vector(CvMat * A);

// create cross product matrix for 3-vector x 
CvMat * opencv_create_cross_product_matrix(CvMat * x);

// scale vector homogeneous vector so that it's last coordinate is 1
void opencv_rescale_homogeneous_vector(CvMat * X);

// normalize vector 
void opencv_normalize(CvMat * x);

// normalize vector 
void opencv_normalize_inhomogeneous(CvMat * x);

// normalize vector
void opencv_normalize_homogeneous(CvMat * x);

// calculate epipolar line
void opencv_epipolar(const CvMat * const F, const double x, const double y, double & a, double & b, double & c);

// point-in-polygon test
bool opencv_pip(const double x, const double y, const CvMat * polygon);

#endif
