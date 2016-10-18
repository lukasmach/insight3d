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

#include "core_math_routines.h"

// * basic math functionality *

// returns true if the value's magnitude borders machine precision
bool nearly_zero(const double x) 
{
	return abs(x) < CORE_PRECISION;
}

// negate a variable 
void negate(bool & b) 
{
	b = !b;
}

// signum 
signed char sgn(const double x) 
{
	return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

// square of a real number 
double sqr_value(const double x)
{
	return x * x; 
}

// maximum of two values
double max_value(const double x, const double y)
{
	return x > y ? x : y; 
}

// minimum of two values 
double min_value(const double x, const double y) 
{
	return x < y ? x : y; 
}

// average of two values  
double average_value(const double a, const double b) 
{
	return (a + b) / 2.0;
}

// swap values of variables 
void swap_double(double & a, double & b) 
{
	double t = a;
	a = b; 
	b = t; 
}

// swap size_t values 
void swap_size_t(size_t & a, size_t & b) 
{
	size_t t = a;
	a = b; 
	b = t; 
}

// swap bool values 
void swap_bool(bool & a, bool & b) 
{
	bool t = a;
	a = b; 
	b = t; 
}

// value is inside closed interval
bool inside_interval(const double value, double boundary1, double boundary2) 
{
	if (boundary1 > boundary2) 
	{
		swap_double(boundary1, boundary2); 
	}

	return value >= boundary1 && value <= boundary2;
}

// value is inside open interval
bool inside_open_interval(const double value, double boundary1, double boundary2) 
{
	if (boundary1 > boundary2) 
	{
		swap_double(boundary1, boundary2); 
	}

	return value > boundary1 && value < boundary2;
}

// vector is inside rectangle 
bool inside_2d_interval(const double x, const double y, const double x1, const double y1, const double x2, const double y2) 
{
	return inside_interval(x, x1, x2) && inside_interval(y, y1, y2); 
}

// convert radians to degrees 
double rad2deg(const double a) 
{
	return a / CORE_PI * 180.0;
}

// convert degrees to radians 
double deg2rad(const double d) 
{
	return d / 180.0 * CORE_PI;
}

// substraction of two 3-vectors 
void sub_3(const double * a, const double * b, double * result) 
{
	result[0] = a[0] - b[0]; 
	result[1] = a[1] - b[1]; 
	result[2] = a[2] - b[2]; 
}

// addition of two 3-vectors 
void add_3(const double * a, const double * b, double * result) 
{
	result[0] = a[0] + b[0]; 
	result[1] = a[1] + b[1]; 
	result[2] = a[2] + b[2]; 
}

// multiply 3-vector by scalar 
void mul_3(const double x, const double * a, double * result) 
{
	result[0] = x * a[0];
	result[1] = x * a[1];
	result[2] = x * a[2];
}

// add multiplied 3-vector to another 3-vector
void add_mul_3(const double * a, const double x, const double * b, double * result)
{
	result[0] = a[0] + x * b[0];
	result[1] = a[1] + x * b[1];
	result[2] = a[2] + x * b[2];
}

// dot product of two 3-vectors
double dot_3(const double * a, const double * b) 
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; 
}

// dot product of two 3-vectors, the second one given by it's coordinates
double dot_3xyz(const double * a, const double x, const double y, const double z) 
{
	return a[0] * x + a[1] * y + a[2] * z; 
}

// cross product of two 3-vectors 
void cross_3(const double * a, const double * b, double * result) 
{
	// cross product
	result[0] = a[1] * b[2] - a[2] * b[1]; 
	result[1] = a[2] * b[0] - a[0] * b[2]; 
	result[2] = a[0] * b[1] - a[1] * b[0]; 
}

// linear combination of four 3-vectors
void linear_combination_43(const double * a, const double * b, const double * c, const double * d, 
                           const double xa, const double xb, const double xc, const double xd, 
						   double * result)
{
	result[0] = xa * a[0] + xb * b[0] + xc * c[0] + xd * d[0];
	result[1] = xa * a[1] + xb * b[1] + xc * c[1] + xd * d[1];
	result[2] = xa * a[2] + xb * b[2] + xc * c[2] + xd * d[2];
}

// linear combination of three 3-vectors
void linear_combination_33(const double * a, const double * b, const double * c, 
                           const double xa, const double xb, const double xc, 
						   double * result)
{
	result[0] = xa * a[0] + xb * b[0] + xc * c[0];
	result[1] = xa * a[1] + xb * b[1] + xc * c[1];
	result[2] = xa * a[2] + xb * b[2] + xc * c[2];
}

// linear combination of three 3-vectors
void linear_combination_23(const double * a, const double * b,
                           const double xa, const double xb, 
						   double * result)
{
	result[0] = xa * a[0] + xb * b[0];
	result[1] = xa * a[1] + xb * b[1];
	result[2] = xa * a[2] + xb * b[2];
}

// vector L2 norm
double vector_norm(double * x, size_t length)
{
	double norm = 0; 

	for (size_t i = 0; i < length; i++) 
	{
		norm += sqr_value(x[i]);
	}

	return sqrt(norm);
}

// L2 norm of a 3-vector 
double vector_norm_3(const double * x) 
{
	return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

// distance of two 3-vectors 
double distance_sq_3(const double * a, const double * b)
{
	return sqr_value(a[0] - b[0]) + sqr_value(a[1] - b[1]) + sqr_value(a[2] - b[2]);
}

// distance of two 3-vectors (different arguments)
double distance_sq_3(const double x1, const double y1, const double z1, const double x2, const double y2, const double z2)
{
	double d = sqr_value(x1 - x2) + sqr_value(y1 - y2) + sqr_value(z1 - z2);
	return d;
}

// distance of two 2-vectors 
double distance_sq_2(const double * a, const double * b)
{
	return sqr_value(a[0] - b[0]) + sqr_value(a[1] - b[1]);
}

// distance of two 2-vectors (different arguments)
double distance_sq_2(const double x1, const double y1, const double x2, const double y2)
{
	double d = sqr_value(x1 - x2) + sqr_value(y1 - y2);
	return d;
}

// normalize vector to unit length
void normalize_vector(double * x, size_t length) 
{
	const double norm = vector_norm(x, length); 
	
	for (size_t i = 0; i < length; i++) 
	{
		x[i] /= norm; 
	}
}

// normalize inhomogeneous part of homogeneous vector 
void normalize_inhomogeneous_part(double * v, size_t length) 
{
	double norm = 0;

	for (size_t i = 0; i < length - 1; i++)
	{
		norm += sqr_value(v[i]); 
	}

	for (size_t i = 0; i < length; i++)
	{
		v[i] /= sqrt(norm);
	}
}

// normalize angle 
double normalize_angle(double a) 
{
	const double pi2 = 2 * CORE_PI;
	if (a > 0) 
	{
		return a - (int)(a / pi2) * pi2;
	}
	else
	{
		return a + (int)(-a / pi2) * pi2;
	}
}

// nearest point on plane
void nearest_point_on_plane(const double * plane, const double * point, double * result)
{
	ASSERT(plane[0] != 0 || plane[1] != 0 || plane[2] != 0, "not a plane in P3");

	// point distance 
	double plane_normal_length = vector_norm_3(plane);
	double distance = (dot_3(plane, point) + plane[3]) / plane_normal_length;

	for (int i = 0; i < 3; i++) 
	{
		result[i] = point[i] - distance * plane[i] / plane_normal_length;
	}
}

// estimate plane parameters from 3 points (first 3 of the inhomogeneous coordinates are normalized) 
bool plane_from_three_points(const double * a, const double * b, const double * c, double * normal)
{
	double direction_1[3], direction_2[3]; 

	// direction vectors 
	direction_1[0] = b[0] - a[0]; 
	direction_1[1] = b[1] - a[1]; 
	direction_1[2] = b[2] - a[2]; 

	direction_2[0] = c[0] - a[0]; 
	direction_2[1] = c[1] - a[1]; 
	direction_2[2] = c[2] - a[2]; 

	// cross product
	cross_3(direction_1, direction_2, normal);
	double len = sqrt(sqr_value(normal[0]) + sqr_value(normal[1]) + sqr_value(normal[2])); 

	// check for singularity 
	if (nearly_zero(len)) 
	{
		// the plane is not determined uniquely
		normal[0] = 0.0; 
		normal[1] = 0.0; 
		normal[2] = 0.0; 
		normal[3] = 1.0;
		return false; 
	}
	else
	{
		// normalization 
		normal[0] /= len; 
		normal[1] /= len;
		normal[2] /= len; 
	}

	// find d 
	normal[3] = -dot_3(a, normal); 

	// check the plane 
	double t1 = dot_3(a, normal) + normal[3]; 
	double t2 = dot_3(b, normal) + normal[3]; 
	double t3 = dot_3(c, normal) + normal[3]; 
	return true;
}
