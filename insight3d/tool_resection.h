#ifndef __TOOL_RESECTION
#define __TOOL_RESECTION

#include "tool_typical_includes.h"
#include "geometry_routines.h"
#include "ui_list.h"
#include "actions.h"
#include "sba.h"

void tool_resection_create();
void tool_resection_current_camera();
void tool_resection_all_uncalibrated();
void tool_resection_all();
void tool_resection_all_enough();
void tool_resection_all_lattice();
void tool_resection_sydney();

#endif

