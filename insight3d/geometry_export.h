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

#ifndef __GEOMETRY_EXPORT
#define __GEOMETRY_EXPORT

#include "interface_filesystem.h"
#include "core_math_routines.h"
#include "geometry_structures.h"
#include "ui_visualization.h"
#include <fstream> 
#include <string>

// save insight3d project
bool geometry_save(const char * filename);

// export scene into VRML
bool geometry_export_vrml(const char * filename, Vertices & vertices, Polygons_3d & polygons, bool export_vertices = false, bool export_polygons = true, size_t restrict_vertices_by_group = 0);

// export scene into Sandy3D ActionScript file 
bool geometry_export_sandy3d(const char * filename, Vertices & vertices, Polygons_3d & polygons, bool export_vertices = false);

// debugging export for calibration into VRLM
bool geometry_export_vrml_calibration(const char * filename, Calibration &calibration);

// exports calibration into RealVIZ exchange format supported by both ImageModeler and MatchMover
bool geometry_export_rzml(const char * filename, Shots & shots);

#endif
