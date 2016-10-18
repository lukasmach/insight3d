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

#ifndef __GEOMETRY_ROUTINES
#define __GEOMETRY_ROUTINES

#include "core_debug.h"
#include "geometry_structures.h"
#include "mvg_decomposition.h"

// decompose projective matrix into rotation, translation and internal calibration matrix 
// note check this
bool geometry_calibration_from_P(const size_t shot_id);

// assemble projective matrix from rotation, translation and internal calibration matrix 
void geometry_calibration_from_decomposed_matrices(const size_t shot_id);

// lattice test 
// todo check for points out of picture
bool geometry_lattice_test(const size_t shot_id);

#endif
