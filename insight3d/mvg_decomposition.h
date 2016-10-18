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

#ifndef __MVG_DECOMPOSITION
#define __MVG_DECOMPOSITION

#include "core_debug.h"
#include "interface_opencv.h"

// decomposition of projection matrix into rotation, translation and internal calibration 
bool mvg_finite_projection_matrix_decomposition(CvMat * const P, CvMat * const K, CvMat * const R, CvMat * const T);

// assemble projection matrix
void mvg_assemble_projection_matrix(CvMat * internal_calibration, CvMat * rotation, CvMat * translation, CvMat * projection);

#endif
