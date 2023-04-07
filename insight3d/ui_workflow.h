#ifndef __UI_WORKFLOW
#define __UI_WORKFLOW

#include "geometry_queries.h"
#include "ui_state.h"
#include "ui_visualization.h"
#include "tool_core.h"

// cancels processing of a vertex, this usually results in a new vertex being created
void ui_workflow_no_vertex();

// next vertex to be processed
void ui_workflow_next_vertex();

// next vertex to be processed
void ui_workflow_prev_vertex();

// first vertex to be processed
void ui_workflow_first_vertex();

// next polygon to be processed
void ui_workflow_next_polygon();

// prev polygon to be processed
void ui_workflow_prev_polygon();

// sets focused point
void ui_workflow_set_focused_point(const size_t point_id);

// unsets focused point
void ui_workflow_unset_focused_point();

// switches to another image
void ui_workflow_select_shot(size_t shot_id);

// set current shot to default value (if necessary)
// this function is called after any modification to the image sequence (i.e., addition or
// removal of images)
void ui_workflow_default_shot();

#endif
