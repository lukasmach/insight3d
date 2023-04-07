#ifndef __TOOL_CALIBRATION
#define __TOOL_CALIBRATION

#include "tool_typical_includes.h"
#include "geometry_publish.h"
#include "geometry_export.h"
#include "mvg_triangulation.h"
#include "mvg_resection.h"
#include "mvg_normalization.h"
#include "mvg_camera.h"
#include "mvg_autocalibration.h"

// methods
void tool_calibration_create();
void tool_calibration_pair();
void tool_calibration_next();
void tool_calibration_prev();
void tool_calibration_add_views();
void tool_calibration_refine();
void tool_calibration_refine_strict();
void tool_calibration_metric();
void tool_calibration_print();
void tool_calibration_use();
void tool_calibration_affine();
void tool_calibration_triangulate();
void tool_calibration_triangulate_trusted();
void tool_calibration_debug_export();
void tool_calibration_clear();
void tool_calibration_bundle();
void tool_calibration_auto(); 
void tool_calibration_auto_begin();
void tool_calibration_auto_step();
void tool_calibration_auto_end(); 
void tool_calibration_test_rectification();

#endif
