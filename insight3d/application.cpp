#include "application.h"

double delta_time; // extern

// initialize debuging (at this point simply prints out some info about 
// application data structures)
bool debug_initialize()
{
	/* printf("debug mode initialization\n");
	printf("=========================\n");
	printf("size of Point structure: %d\n", sizeof(Point)); 
	printf("size of Vertex structure: %d\n", sizeof(Vertex)); 
	printf("\n"); */

	return true;
}

// initialize application subsystems 
bool initialization()
{
	// GNU GPL license notification
	printf("insight3d 0.5, 2007-2010\n");
	printf("licensed under GNU AGPL 3\n\n");

	// note this crashes in debug on MSVC (2008 EE), for now avoid using strdup
	/*char * s = strdup("ahoy"); 
	free(s);*/

	// test memory allocation
	printf("testing memory allocation ... "); 
	size_t * p = ALLOC(size_t, 100); 
	FREE(p);
	p = ALLOC(size_t, 1); 
	FREE(p);
	printf("ok\n"); // if we're still alive, everything's fine

	// initialize application structures 
	printf("initializing application structures ... "); 
	fflush(stdout);
	bool state = 
		core_debug_initialize() && 
		debug_initialize() && // todo merge this with core_debug
		core_initialize() &&
		geometry_initialize() && 
		image_loader_initialize(4, 32) &&
		ui_initialize() &&
		visualization_initialize() && 
		ui_library_initialization() &&
		ui_create() && 
		image_loader_start_thread();

	printf("ok\n");

	// initialize the whole package
	return state; 
}

static pthread_t gui_rendering_thread;

// main loop 
bool main_loop()
{
	bool is_active = true;
	Uint32 timestamp1 = SDL_GetTicks(), timestamp2 = 0;

	// create rendering thread 
	printf("creating UI thread ... \n"); 

	// start gui rendering thread 
	gui_start_rendering_thread();

	// respond to application events 
	while (core_state.running)
	{
		GUI_Event_Descriptor event;

		// process event 
		LOCK(geometry)
		{
			if (gui_poll_event(&event))
			{
				// is this a GUI event?
				if (event.handler) 
				{
					event.handler(event);
				}
				else // or is it an SDL event? 
				{
					// handle an SDL event
					switch (event.sdl_event.type)
					{
						case SDL_KEYDOWN: 
						{
							ui_state.key_state[event.sdl_event.key.keysym.sym] = 1;
							break;
						}

						case SDL_KEYUP: 
						{
							ui_state.key_state[event.sdl_event.key.keysym.sym] = 0;
							break; 
						}

						case SDL_MOUSEBUTTONUP: 
						{
							ui_event_agar_button_up(&event.sdl_event);
							break; 
						}

						case SDL_QUIT:
						{
							core_state.running = false;
							break;
						}
					}
				}
			}
		}
		UNLOCK(geometry);
	};

	return true; 
}

// deallocate program structures
bool release()
{
	geometry_release();
	image_loader_release();

	return true;
}

// error reporting routine 
bool report_error()
{
	return false;
}
