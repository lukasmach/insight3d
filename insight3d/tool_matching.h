#ifndef __TOOL_MATCHING
#define __TOOL_MATCHING

#include "tool_typical_includes.h"
#include "ui_list.h"
#include "mvg_matching.h"

// tool registration and public routines
void tool_matching_create();
void tool_matching_standard();
void tool_matching_remove_conflicts();

// additional shot info 
struct Matching_Shot
{
	int width, height; // size of loaded shot 
};

#endif
