#ifndef __APPLICATION
#define __APPLICATION

#include "geometry_structures.h"
#include "core_image_loader.h"
#include "gui.h"
#include "ui_core.h"
#include "ui_visualization.h"

extern bool mousealreadydown;
extern double delta_time; // time elapsed since last frame rendering\

// initialize debuging (at this point simply prints out some info about 
// application data structures)
bool debug_initialize();

// initialize application subsystems 
bool initialization();

// main loop 
bool main_loop(); 

// deallocate program structures
bool release();

// error reporting routine 
bool report_error();

#endif
