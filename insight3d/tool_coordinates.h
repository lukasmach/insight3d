#ifndef __TOOL_COORDINATES
#define __TOOL_COORDINATES

#include "tool_typical_includes.h"
#include "geometry_routines.h"

void tool_coordinates_create();
void coordinates_reorient_using_current_camera();
bool coordinates_rotate_all_cameras(size_t shot_id);
bool coordinates_apply_homography_to_cameras(CvMat * H);

#endif
