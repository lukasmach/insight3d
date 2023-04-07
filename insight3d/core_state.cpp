#include "core_state.h"

Core_State core_state;

// initialize core state
bool core_initialize()
{
	memset(&core_state, 0, sizeof(core_state));

	// initialize random number generator and timing
	srand(time(NULL));
	core_state.last_ticks = SDL_GetTicks(); 
	core_state.ticks = SDL_GetTicks(); 
	core_state.running = true; 

	return true;
}
