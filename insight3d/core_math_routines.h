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

#ifndef __CORE_MATH_ROUTINES
#define __CORE_MATH_ROUTINES

#include <cmath>
#include "core_debug.h"
#include "core_constants.h"

// * basic math functionality *

// returns true if the value's magnitude borders machine precision
bool nearly_zero(const double x);

// negate a variable 
void negate(bool & b);

// signum 
signed char sgn(const double x);

// square of a real number 
double sqr_value(const double x);

// maximum of two values
double max_value(const double x, const double y);

// minimum of two values 
double min_value(const double x, const double y);

// average of two values  
double average_value(const double a, const double b);

// swap values of variables 
void swap_double(double & a, double & b);

// swap size_t values 
void swap_size_t(size_t & a, size_t & b);

// swap bool values 
void swap_bool(bool & a, bool & b);

// value is inside closed interval
bool inside_interval(const double value, double boundary1, double boundary2);

// value is inside open interval
bool inside_open_interval(const double value, double boundary1, double boundary2);

// vector is inside rectangle 
bool inside_2d_interval(const double x, const double y, const double x1, const double y1, const double x2, const double y2);

// convert radians to degrees 
double rad2deg(const double a);

// convert degrees to radians 
double deg2rad(const double d);

// substraction of two 3-vectors 
void sub_3(const double * a, const double * b, double * result);

// addition of two 3-vectors 
void add_3(const double * a, const double * b, double * result);

// multiply 3-vector by scalar 
void mul_3(const double x, const double * a, double * result);

// add multiplied 3-vector to another 3-vector
void add_mul_3(const double * a, const double x, const double * b, double * result);

// dot product of two 3-vectors
double dot_3(const double * a, const double * b);

// dot product of two 3-vectors, the second one given by it's coordinates
double dot_3xyz(const double * a, const double x, const double y, const double z);

// cross product of two 3-vectors 
void cross_3(const double * a, const double * b, double * result);

// linear combination of four 3-vectors
void linear_combination_43(const double * a, const double * b, const double * c, const double * d, 
                           const double xa, const double xb, const double xc, const double xd, 
						   double * result);

// linear combination of three 3-vectors
void linear_combination_33(const double * a, const double * b, const double * c, 
                           const double xa, const double xb, const double xc, 
						   double * result);

// linear combination of three 3-vectors
void linear_combination_23(const double * a, const double * b,
                           const double xa, const double xb, 
						   double * result);

// vector L2 norm
double vector_norm(double * x, size_t length);

// L2 norm of a 3-vector 
double vector_norm_3(const double * x);

// distance of two 3-vectors 
double distance_sq_3(const double * a, const double * b);

// distance of two-3-vectors (different arguments)
double distance_sq_3(const double x1, const double y1, const double z1, const double x2, const double y2, const double z2);

// distance of two 2-vectors 
double distance_sq_2(const double * a, const double * b);

// distance of two 2-vectors (different arguments)
double distance_sq_2(const double x1, const double y1, const double x2, const double y2);

// normalize vector to unit length
void normalize_vector(double * x, size_t length);

// normalize inhomogeneous part of homogeneous vector 
void normalize_inhomogeneous_part(double * v, size_t length);

// normalize angle 
double normalize_angle(double a);

// nearest point on plane
void nearest_point_on_plane(const double * plane, const double * point, double * result);

// estimate plane parameters from 3 points (first 3 of the inhomogeneous coordinates are normalized) 
bool plane_from_three_points(const double * a, const double * b, const double * c, double * normal);

#endif
