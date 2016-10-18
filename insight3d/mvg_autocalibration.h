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
