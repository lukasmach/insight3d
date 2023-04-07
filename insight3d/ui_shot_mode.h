#ifndef __UI_SHOT_MODE
#define __UI_SHOT_MODE

#include "interface_sdl.h"
#include "core_structures.h"
#include "core_math_routines.h"
#include "ui_state.h"
#include "ui_selection.h"

// mode switching 
void ui_switch_to_shot_mode();

// process mouse click events in shot mode
void ui_shot_mouse_click();

// respond to selection box in shot mode
void ui_shot_mouse_selection();

// switch current shot (when the user works in shot mode and decides to switch to another image/camera)
bool ui_switch_shot(size_t shot_id);

// process user input (in shot mode) 
void ui_update_shot(const Uint32 delta_time);

#endif
