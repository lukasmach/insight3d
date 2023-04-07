#ifndef __CORE_STATE
#define __CORE_STATE

#include "interface_sdl.h"
#include <ctime>

// possible errors 
enum CORE_ERROR { 
	CORE_NO_ERROR,
	CORE_ERROR_GUI_INITIALIZATION,
	CORE_ERROR_OUT_OF_MEMORY,
	CORE_ERROR_UNABLE_TO_OPEN_FILE, 
	CORE_ERROR_UNABLE_TO_CREATE_THREAD
};

// core application state info
struct Core_State 
{
	// information about OS interaction, timing, etc.
	bool running;
	bool visible;
	bool mouse_focus;
	bool keyboard_focus;
	Uint32 ticks, last_ticks;

	// error management
	CORE_ERROR error;
};

extern Core_State core_state;

// error handling macro 
// todo think about using assertions for this
#define CHECK_ERROR(condition, error_code) if ((condition)) { core_state.error = (error_code); return false; }

// initialize core state
bool core_initialize();

#endif
