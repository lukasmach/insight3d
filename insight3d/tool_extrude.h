#ifndef __TOOL_EXTRUDE
#define __TOOL_EXTRUDE

#include "core_math_routines.h"
#include "geometry_structures.h"

// do extrusion on the ordered set of selected vertices
bool tool_extrude_to_ground(double * ground, Selected_Items * selected_vertices, size_t group = 0);

#endif
