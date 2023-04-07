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
