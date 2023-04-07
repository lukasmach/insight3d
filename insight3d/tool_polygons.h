#ifndef __TOOL_POLYGONS
#define __TOOL_POLYGONS

#include "tool_typical_includes.h"
#include "tool_points.h"

void tool_polygons_create();
void tool_polygons_key();
bool tool_polygons_mouse_down(double x, double y, int button);
void tool_polygons_move(double x, double y);
void tool_polygons_dragging(double x, double y, int button);
void tool_polygons_dragging_done(double x1, double y1, double x2, double y2, int button);
void tool_polygons_click(double x, double y, int button);
void tool_polygons_begin();
void tool_polygons_end();

#endif
