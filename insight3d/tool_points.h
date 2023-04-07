#ifndef __TOOL_POINTS
#define __TOOL_POINTS

#include "tool_typical_includes.h"

void tool_points_create();
void tool_points_key();
bool tool_points_mouse_down(double x, double y, int button);
void tool_points_move(double x, double y);
void tool_points_dragging(double x, double y, int button);
void tool_points_dragging_done(double x1, double y1, double x2, double y2, int button);
void tool_points_click(double x, double y, int button);
void tool_points_begin();
void tool_points_end();

#endif
