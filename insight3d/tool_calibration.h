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

#ifndef __TOOL_CALIBRATION
#define __TOOL_CALIBRATION

#include "tool_typical_includes.h"
#include "geometry_publish.h"
#include "geometry_export.h"
#include "mvg_triangulation.h"
#include "mvg_resection.h"
#include "mvg_normalization.h"
#include "mvg_camera.h"
#include "mvg_autocalibration.h"

// methods
void tool_calibration_create();
void tool_calibration_pair();
void tool_calibration_next();
void tool_calibration_prev();
void tool_calibration_add_views();
void tool_calibration_refine();
void tool_calibration_refine_strict();
void tool_calibration_metric();
void tool_calibration_print();
void tool_calibration_use();
void tool_calibration_affine();
void tool_calibration_triangulate();
void tool_calibration_triangulate_trusted();
void tool_calibration_debug_export();
void tool_calibration_clear();
void tool_calibration_bundle();
void tool_calibration_auto(); 
void tool_calibration_auto_begin();
void tool_calibration_auto_step();
void tool_calibration_auto_end(); 
void tool_calibration_test_rectification();

#endif
