#ifndef __TOOL_SELECTION
#define __TOOL_SELECTION

#include "core_math_routines.h"
#include "geometry_textures.h"
#include "ui_core.h"
#include "ui_context.h"
#include "ui_visualization.h"
#include "ui_selection.h"
#include "ui_workflow.h"
#include "tool_core.h"

// selection tool handles viewing options which are read by other tools and the rest of the application 
extern bool option_show_dualview, option_thumbs_only_for_selected, option_hide_automatic;

// selection tool routines 
void tool_selection_create();
bool tool_selection_mouse_down(double x, double y, int button);
void tool_selection_move(double x, double y);
void tool_selection_dragging(double x, double y, int button);
void tool_selection_dragging_done(double x1, double y1, double x2, double y2, int button);
void tool_selection_click(double x, double y, int button);
void tool_selection_key();
void tool_selection_begin();
void tool_selection_end();

// switching options
void selection_option_show_dualview();
void selection_option_thumbs_only_for_selected();
void selection_option_show_automatic_points();

// some debugging functions 
void debug_print_Ps();
void debug_save_initial_solution();
void debug_save_vertices();

#endif
