/*

Issues: 

* it's not clear when the user can use panel properties 
  like i for her own purposes and when they are reserved
* check if caption is consistently strdup'ed

*/

#include "gui.h"
#include "gui_style.h"

#define GUI_NEW(type) ((type *)malloc(sizeof(type)))
#define GUI_NEW_ARRAY(type, n) ((type *)malloc(sizeof(type) * (n)))
#define GUI_CLEAR(var, type) memset((var), 0, sizeof(type))
#define GUI_CLEAR_ARRAY(var, n, type) (memset((var), 0, sizeof((type)) * (n)))

/* Fixes */ 
bool mousealreadydown;

/* Threading */ 
static pthread_t gui_rendering_thread;
static pthread_mutex_t gui_structures_lock = PTHREAD_MUTEX_INITIALIZER;

/* Global variables */
GUI_Context gui_context;

/* Auxiliary macros */ 

GUI_Event_Descriptor gui_construct_event_descriptor(GUI_Panel * const panel, const SDL_Event sdl);

#define GUI_FIRE_EVENT(panel, event, sdl) \
	if ((panel)->on_##event) \
	{ \
		if ((panel)->on_##event##_is_internal) \
		{ \
			(panel)->on_##event(gui_construct_event_descriptor((panel), (sdl))); \
		} \
		else \
		{ \
			gui_aux_add_to_event_queue(panel, (panel)->on_##event, (sdl)); \
		} \
	}

/* Auxiliary functions */ 

// value is inside interval
static bool aux_inside_interval(int x, int a, int b)
{
	return x >= a && x <= b || x >= b && x <= a;
}

// value is inside 2d interval
static bool aux_inside_2d_interval(int px, int py, int x1, int y1, int x2, int y2)
{
	return aux_inside_interval(px, x1, x2) && aux_inside_interval(py, y1, y2);
}

/* Initialization */

// initializes global structures
void gui_initialize()
{
	GUI_CLEAR(&gui_context.root_panel, GUI_Panel);
	gui_context.panels[0] = &gui_context.root_panel;
	gui_context.panels_count = 1;

	// initialize fonts subsystem
	cvInitFont(&gui_context.font, CV_FONT_HERSHEY_SIMPLEX, 0.35, 0.35, 0, 1, CV_AA);
}

/* helper functions do the routine stuff for you */

// initialize SDL window with OpenGL support
bool gui_helper_initialize()
{
	return
		// initialize SDL
		gui_helper_initialize_sdl() &&
		// create OpenGL window
		gui_helper_initialize_opengl()
	;
}

// initialize SDL
bool gui_helper_initialize_sdl()
{
	const int width = gui_context.width, height = gui_context.height;

    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "[GUI] Video initialization failed: %s\n", SDL_GetError());
	    return false;
	}

    // fetch the video info
	if (!(gui_context.video_info = SDL_GetVideoInfo()))
	{
		fprintf(stderr, "[GUI] Video query failed: %s\n", SDL_GetError());
		return false;
	}

    // set the flags
    gui_context.video_flags  = SDL_OPENGL;          // enable opengl
    gui_context.video_flags |= SDL_GL_DOUBLEBUFFER; // double buffering prevents flickering
    gui_context.video_flags |= SDL_HWPALETTE;       // store palette in hardware 
    gui_context.video_flags |= SDL_RESIZABLE;       // enable resizing

    // check if surfaces can be stored in hardware
    if (gui_context.video_info->hw_available)
	{
		gui_context.video_flags |= SDL_HWSURFACE;
	}
    else
	{
		gui_context.video_flags |= SDL_SWSURFACE;
	}

    // check if hardware blits can be done
	if (gui_context.video_info->blit_hw)
	{
		gui_context.video_flags |= SDL_HWACCEL;
	}

    // turn on opengl double buffering 
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // get sdl surface to draw on
	if (!(gui_context.surface = SDL_SetVideoMode(width, height, 32, gui_context.video_flags)))
	{
	    fprintf(stderr, "[GUI] Video mode set failed: %s\n", SDL_GetError());
		return false;
	}

	// set window title 
	SDL_WM_SetCaption(gui_context.title, NULL);

	return true;
}

// initialize OpenGL
bool gui_helper_initialize_opengl()
{
	glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 0.5);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glDisable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glDisable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return true;
}

// adjust opengl rendering settings for new window size
void gui_helper_opengl_adjust_size()
{
	int width = gui_context.width, height = gui_context.height;
	float ratio;

	// set size
	if (height == 0)
	{
		height = 1;
	}
	ratio = width / (float)height;
	glViewport(0, 0, width, height);

	// set camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, ratio, 0.001, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/* Settings */

// set window size
void gui_set_size(const int width, const int height)
{
	// save new size in the global structure
	gui_context.width = width;
	gui_context.height = height;

	// adjust pixel size constants
	gui_context.px = 1 / (double)width;
	gui_context.py = 1 / (double)height;

	// propagate it into root panel 
	gui_context.root_panel.x1 = 0; 
	gui_context.root_panel.y1 = 0; 
	gui_context.root_panel.x2 = width; 
	gui_context.root_panel.y2 = height;
}

// set window title 
void gui_set_title(const char * const title) 
{
	memcpy(gui_context.title, title, strlen(title) + 1);
}

/* Define gui elements */

// the root element contains everything 
GUI_Panel * gui_get_root_panel()
{
	return &gui_context.root_panel;
}

// create new panel
GUI_Panel * gui_new_panel(GUI_Panel * parent, GUI_Panel * sibling, GUI_Positioner positioner, const char * debugging_title)
{
	GUI_Panel * const panel = GUI_NEW(GUI_Panel);

	if (!panel || gui_context.panels_count >= GUI_MAX_PANELS)
	{
		fprintf(stderr, "maximum number of panels reached");
		return NULL;
	}

	GUI_CLEAR(panel, GUI_Panel);

	panel->parent = parent;
	panel->sibling = sibling;
	panel->positioner = positioner;
	panel->debugging_title = debugging_title;

	gui_context.panels[gui_context.panels_count++] = panel;

	return panel;
}

// set panels width 
void gui_set_width(GUI_Panel * panel, const int width)
{
	panel->width = width; 
}

// set panels height 
void gui_set_height(GUI_Panel * panel, const int height)
{
	panel->height = height;
}

/* Core functions */

void gui_calculate_coordinates()
{
	// go through all defined panels and calculate the coordinates 
	// of one after another 
	for (size_t i = 1; i < gui_context.panels_count; i++) 
	{
		GUI_Panel * panel = gui_context.panels[i]; 
		panel->positioner(panel);
	}
}

/* Positioners - we have various favors of them */

// simply fill all the space parent offers to you 
void gui_fill(GUI_Panel * panel)
{
	const GUI_Panel * parent = panel->parent; 
	panel->x1 = parent->x1; 
	panel->y1 = parent->y1; 
	panel->x2 = parent->x2; 
	panel->y2 = parent->y2; 
}

// horizontal panel placed at the top of parents area
void gui_top_hfill(GUI_Panel * panel)
{
	const GUI_Panel * parent = panel->parent;
	panel->x1 = parent->x1;
	panel->y1 = parent->y1;
	panel->x2 = parent->x2;
	panel->y2 = parent->y1 + panel->height;
}

// vertical panel placed on the left side of parents area
void gui_below_left_vfill(GUI_Panel * panel)
{
	const GUI_Panel * parent = panel->parent, * sibling = panel->sibling;
	panel->x1 = parent->x1;
	panel->y1 = sibling->y2;
	panel->x2 = parent->x1 + panel->width;
	panel->y2 = parent->y2;
}

// panel filling whats left by the above panel
void gui_below_left_vfill_rest(GUI_Panel * panel) 
{
	const GUI_Panel * left = panel->sibling, * top = panel->sibling->sibling;
	panel->x1 = left->x2; 
	panel->y1 = left->y1;
	panel->x2 = top->x2;
	panel->y2 = left->y2;
}

// place panel below another panel
void gui_below(GUI_Panel * panel)
{
	const GUI_Panel * top = panel->sibling;
	panel->x1 = top->x1;
	panel->y1 = top->y2;
	panel->x2 = panel->x1 + panel->width;
	panel->y2 = panel->y1 + panel->height;
}

// place panel below another panel, fill horizontally 
void gui_below_hfill(GUI_Panel * panel)
{
	const GUI_Panel * top = panel->sibling;
	panel->x1 = top->x1;
	panel->y1 = top->y2;
	panel->x2 = panel->parent->x2;
	panel->y2 = panel->y1 + panel->height;
}

// place it to the left side of the container
void gui_left_vfill(GUI_Panel * panel)
{
	const GUI_Panel * parent = panel->parent;
	panel->x1 = parent->x1; 
	panel->y1 = parent->y1; 
	panel->x2 = parent->x1 + panel->width; 
	panel->y2 = parent->y2;
}

// place it to the right of it's sibling, vfill the parent 
void gui_on_the_right_vfill(GUI_Panel * panel)
{
	const GUI_Panel * parent = panel->parent, * left = panel->sibling;
	panel->x1 = left->x2; 
	panel->y1 = parent->y1; 
	panel->x2 = panel->x1 + panel->width; 
	panel->y2 = parent->y2;
}

// place it to the right of another panel 
void gui_on_the_right(GUI_Panel * panel)
{
	const GUI_Panel * left = panel->parent;
	panel->x1 = left->x2; 
	panel->y1 = left->y1; 
	panel->x2 = panel->x1 + panel->width;
	panel->y2 = panel->y1 + panel->height;
}

// place it below another panel and fill the rest of the parent's space 
void gui_below_fill(GUI_Panel * panel)
{
	const GUI_Panel * parent = panel->parent, * top = panel->sibling; 
	panel->x1 = parent->x1; 
	panel->y1 = top->y2; 
	panel->x2 = parent->x2; 
	panel->y2 = parent->y2;
}

// place on the right side of one panel, which is placed below another one - fill what's left
void gui_below_on_the_right_fill(GUI_Panel * panel)
{
	const GUI_Panel * left = panel->sibling, * top = panel->sibling->sibling; 
	panel->x1 = left->x2; 
	panel->y1 = left->y1; 
	panel->x2 = top->x2; 
	panel->y2 = left->y2;
}

// unpositioned panel
void gui_no_position(GUI_Panel * panel)
{
	panel->x1 = 0; 
	panel->y1 = 0; 
	panel->x2 = 0; 
	panel->y2 = 0;
}

/* Events */ 

void gui_set_style(GUI_Panel * panel, GUI_Render rendering_function)
{
	panel->on_render = rendering_function;
}

/*void gui_cancel_event() // todo this probably stopped working since the introduction of multiple threads... might work on internally handled events 
{
	gui_context.event_cancelled = true;
}*/

bool gui_poll_event(GUI_Event_Descriptor * event)
{
	// lock the queue structure 
	gui_lock(); 

	// if there is no event, return false 
	if (gui_context.event_queue_bottom == gui_context.event_queue_top)
	{
		gui_unlock(); 
		return false; 
	}

	// check out an event 
	*event = gui_context.event_queue[gui_context.event_queue_bottom]; 
	gui_context.event_queue_bottom = (gui_context.event_queue_bottom + 1) % GUI_EVENT_QUEUE_LENGTH;

	// unlock the queue structure 
	gui_unlock();

	return true;
}

GUI_Event_Descriptor gui_construct_event_descriptor(GUI_Panel * const panel, const SDL_Event sdl)
{
	GUI_Event_Descriptor event; 
	event.panel = panel; 
	event.sdl_event = sdl; 
	return event;
}

void gui_aux_add_to_event_queue(GUI_Panel * panel, GUI_Event event, SDL_Event sdl_event)
{
	gui_lock();

	gui_context.event_queue[gui_context.event_queue_top].panel = panel;
	gui_context.event_queue[gui_context.event_queue_top].handler = event;
	gui_context.event_queue[gui_context.event_queue_top].sdl_event = sdl_event;

	gui_context.event_queue_top = (gui_context.event_queue_top + 1) % GUI_EVENT_QUEUE_LENGTH;

	gui_unlock();
}

/* Setters */ 

void gui_set_panel_visible(GUI_Panel * panel, bool visibility)
{
	panel->hidden = !visibility;
}

void gui_set_debugging_title(GUI_Panel * panel, const char * debugging_title)
{
	panel->debugging_title = debugging_title;
}

void gui_set_top_margin(GUI_Panel * panel, const int margin)
{
	panel->margin_top = margin;
}

void gui_set_bottom_margin(GUI_Panel * panel, const int margin)
{
	panel->margin_bottom = margin;
}

void gui_set_left_margin(GUI_Panel * panel, const int margin)
{
	panel->margin_left = margin;
}

void gui_set_right_margin(GUI_Panel * panel, const int margin)
{
	panel->margin_right = margin;
}

void gui_set_margins(GUI_Panel * panel, const int top, const int right, const int bottom, const int left)
{
	gui_set_top_margin(panel, top);
	gui_set_bottom_margin(panel, bottom);
	gui_set_left_margin(panel, left);
	gui_set_right_margin(panel, right);
}

void gui_set_disabled(GUI_Panel * panel, const bool value)
{
	panel->disabled = value;
}

/* Queries */ 

bool gui_is_panel_visible(const GUI_Panel * panel)
{
	return !panel->hidden;
}

bool gui_is_panel_disabled(const GUI_Panel * panel)
{
	return panel->disabled;
}

/* Render */

// render gui elements 
void gui_render()
{
	// set up rendering 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.375 * gui_context.px, 0.375 * gui_context.py, 0.0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// go through all gui elements and render them
	for (size_t i = 0; i < gui_context.panels_count; i++)
	{
		GUI_Panel * const panel = gui_context.panels[i];

		// skip invisible panels
		panel->effectively_hidden = true;
		if (!gui_is_panel_visible(panel) || panel->parent && panel->parent->effectively_hidden) continue;
		panel->effectively_hidden = false;

		/*float hash1 = 0.823F * (float)i;
		if (
			panel->on_buttonaction_is_internal || 
			panel->on_focus_is_internal || 
			panel->on_menuitemaction_is_internal || 
			panel->on_mousedown_is_internal || 
			panel->on_mousedownout_is_internal || 
			panel->on_mousemove_is_internal || 
			panel->on_mouseup_is_internal || 
			panel->on_render_is_internal || 
			panel->on_unfocus_is_internal
		)
		{
			hash1 = 0.823F * (float)(rand() % 100);
		}
		float hash2 = 1 - 0.2F * panel->focus;
		glColor3d(0.5 + 0.5 * hash2 * sin(hash1), 0.5 + 0.5 * hash2 * cos(hash1), i * 0.01);*/

		// calculate coordinates of the panel
		panel->effective_x1 = panel->x1 + panel->margin_left;
		panel->effective_y1 = panel->y1 + panel->margin_top;
		panel->effective_x2 = panel->x2 - panel->margin_right;
		panel->effective_y2 = panel->y2 - panel->margin_bottom;

		// check if we should render text 
		if (panel->caption)
		{
			if (!panel->caption_image)
			{
				gui_caption_render(panel);
			}
			else if (!panel->caption_texture_id)
			{
				panel->caption_texture_id = gui_upload_opengl_texture(panel->caption_image);
			}
		}

		// if there is dedicated rendering function, use it 
		if (panel->on_render) 
		{
			panel->on_render(panel);
		}
		else
		{
			/*glBegin(GL_POLYGON); 
				glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glEnd();*/
		}
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

// render caption onto image and upload it into opengl 
void gui_caption_render(GUI_Panel * panel)
{
	// release potentially allocated resources 
	gui_caption_discard(panel); 

	if (!panel->caption) return;

	// get the image size
	CvSize text_size;
	int baseline;
	cvGetTextSize(panel->caption, &gui_context.font, &text_size, &baseline);
	panel->caption_width = text_size.width + 4;
	panel->caption_height = 2 * text_size.height;

	// create appropriately large image
	// panel->caption_image = cvCreateImage(cvSize(text_size.width + 4, 2 * text_size.height), IPL_DEPTH_8U, 3);
	panel->caption_image = cvCreateImage(cvSize(512, 64), IPL_DEPTH_8U, 1);

	// render caption
	cvZero(panel->caption_image);
	cvPutText(panel->caption_image, panel->caption, cvPoint(2, text_size.height + 3), &gui_context.font, cvScalar(255, 255, 255));

	// upload to gpu 
	panel->caption_texture_id = gui_upload_opengl_texture(panel->caption_image);
}

void gui_caption_discard(GUI_Panel * panel)
{
	// release the image 
	if (panel->caption_image)
	{
		cvReleaseImage(&panel->caption_image); 
		panel->caption_image = NULL; 
	}

	gui_caption_discard_opengl_texture(panel);
}

void gui_caption_discard_opengl_texture(GUI_Panel * panel)
{
	// release opengl texture 
	if (panel->caption_texture_id) 
	{
		glDeleteTextures(1, &(panel->caption_texture_id));
		panel->caption_texture_id = 0;
	}
}

/* Rendering thread */

void * gui_rendering_thread_function(void * args) 
{
	// initialize GUI
	gui_helper_initialize_sdl();
	gui_helper_initialize_opengl();
	gui_helper_opengl_adjust_size();

	// GUI loop
	while (true) 
	{
		// note that we should render the window only if it's active

		// redraw scene
		gui_calculate_coordinates();
		gui_render();
		SDL_GL_SwapBuffers();

		// check out new events
		SDL_Event event;

		// handle events in queue
		mousealreadydown = false;
		while (SDL_PollEvent(&event))
		{
			/* gui_lock();
			printf("%d ", (gui_context.event_queue_top - gui_context.event_queue_bottom) % GUI_EVENT_QUEUE_LENGTH);
			gui_unlock();*/

			// first we let GUI handle this event or translate it into GUI event 
			if (!gui_resolve_event(&event)) 
			{
				// if that didn't work, check if we can handle it ourselves 
				if (event.type == SDL_VIDEORESIZE) 
				{
					// resize the screen 
					if (!(gui_context.surface = SDL_SetVideoMode(event.resize.w, event.resize.h, 32, gui_context.video_flags)))
					{
						fprintf(stderr, "[SDL] Could not get a surface after resize: %s\n", SDL_GetError());
						// todo terminate
						break;
					}

					gui_set_size(event.resize.w, event.resize.h);
					gui_helper_initialize_opengl();
					gui_helper_opengl_adjust_size();

					// release all opengl textures // note we're waiting for SDL 1.3 to do this right
					for (size_t i = 0; i < gui_context.panels_count; i++) 
					{
						gui_caption_discard_opengl_texture(gui_context.panels[i]);
					}
				}

				// if that didn't work, save it as an SDL event 
				gui_aux_add_to_event_queue(NULL, NULL, event);
			}
		}
	}

	return NULL;
}

bool gui_start_rendering_thread()
{
	return !pthread_create(&gui_rendering_thread, NULL, gui_rendering_thread_function, NULL);
}

/* Respond to events */

// resolve mouse move event - setting mouse over flags, sending events to individual panels 
bool gui_resolve_mousemotion(SDL_Event * event) 
{
	const int x = event->motion.x, y = event->motion.y;

	// go through all items 
	size_t focus = SIZE_MAX;
	for (size_t i = 0; i < gui_context.panels_count; i++) 
	{
		// skip invisible
		// if (!gui_is_panel_visible(gui_context.panels[i])) continue; 
		// note this needs some looking into
		if (gui_context.panels[i]->effectively_hidden) continue;
		GUI_Panel * const panel = gui_context.panels[i]; 

		// check if the cursor is inside this panel
		if (aux_inside_2d_interval(
				x, y, 
				panel->x1, panel->y1, 
				panel->x2, panel->y2
		))
		{
			focus = i;
		}
	}

	// if there is a panel with focus 
	if (focus < SIZE_MAX) 
	{
		// set focus and fire onfocus event
		if (!gui_context.panels[focus]->focus) 
		{
			gui_context.panels[focus]->focus = true;
			GUI_FIRE_EVENT(gui_context.panels[focus], focus, *event);
		}

		// send onmousemove event 
		GUI_FIRE_EVENT(gui_context.panels[focus], mousemove, *event);
	}

	// send onunfocus events to panels that were previously focused 
	// note I think that only one panel at a time can be focused, 
	// but this might change
	// note why we don't do unfocus before focus?
	for (size_t i = 0; i < gui_context.panels_count; i++) 
	{
		if (i != focus && gui_context.panels[i]->focus)
		{
			gui_context.panels[i]->focus = false;
			GUI_FIRE_EVENT(gui_context.panels[i], unfocus, *event);
		}
	}

	return true;
}

// resolve mouse button down event
bool gui_resolve_mousebuttondown(SDL_Event * event)
{
	// send mouse button down out event to all panels that haven't been clicked 
	// gui_context.event_cancelled = false; // mouse button out events can cancel processing of mouse button down events
	GUI_Panel * focused = NULL;
	for (size_t i = 0; i < gui_context.panels_count; i++) 
	{
		GUI_Panel * const panel = gui_context.panels[i]; 

		if (panel->focus) 
		{
			focused = panel;
			continue; 
		}

		GUI_FIRE_EVENT(panel, mousedownout, *event); 
	}

	// check if the event was cancelled 
	// if (gui_context.event_cancelled) 
	// {
	// 	gui_context.event_cancelled = false; 
	// 	return true;
	// }

	// get the focused panel and send appropriate mouse button down event
	if (focused)
	{
		GUI_FIRE_EVENT(focused, mousedown, *event);
	}

	return true;
}

// resolve mouse button down event
bool gui_resolve_mousebuttonup(SDL_Event * event)
{
	// get the focused panel and send appropriate mouse button up event
	// note shouldn't we check that matching up and down go to the same panel?
	GUI_Panel * focused = NULL;
	for (size_t i = 0; i < gui_context.panels_count; i++) 
	{
		GUI_Panel * const panel = gui_context.panels[i]; 

		if (panel->focus) 
		{
			focused = panel;
			continue; 
		}
	}

	if (focused)
	{
		if (!focused->on_mouseup) return false;

		GUI_FIRE_EVENT(focused, mouseup, *event);
	}

	return true;
}

// resolve one event
bool gui_resolve_event(SDL_Event * event)
{
	switch (event->type)
	{
		case SDL_MOUSEMOTION: return gui_resolve_mousemotion(event); break; 
		case SDL_MOUSEBUTTONDOWN: 
			if (mousealreadydown) return false;
			mousealreadydown = true;
			return gui_resolve_mousebuttondown(event); 
		break;
		case SDL_MOUSEBUTTONUP: return gui_resolve_mousebuttonup(event); break;
	}

	return false;
}

/* Auxiliary functions - OpenGL */ 

// upload texture to opengl
GLuint gui_upload_opengl_texture(IplImage * image)
{
	GLuint texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, image->width, image->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, image->imageData);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture_id;
}

// dipslay texture containing text
void gui_opengl_display_text(
	GLuint texture_id, 
	int texture_width, int texture_height, 
	int width, int height, 
	double x, double y,
	double alpha
)
{
	const float 
		wx = width / (float)texture_width, 
		wy = height / (float)texture_height
	; 

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glColor4d(1.0, 1.0, 1.0, alpha);
	glBegin(GL_POLYGON); 
		glTexCoord2d(0, 0);
		glVertex3d(-1 + 2 * x / (float)gui_context.width, 1 - 2 * y / (float)gui_context.height, 0);
		glTexCoord2d(wx, 0); 
		glVertex3d(-1 + 2 * (x + width) / (float)gui_context.width, 1 - 2 * y / (float)gui_context.height, 0);
		glTexCoord2d(wx, wy); 
		glVertex3d(-1 + 2 * (x + width) / (float)gui_context.width, 1 - 2 * (y + height) / (float)gui_context.height, 0);
		glTexCoord2d(0, wy); 
		glVertex3d(-1 + 2 * x / (float)gui_context.width, 1 - 2 * (y + height) / (float)gui_context.height, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
}

void gui_opengl_vertex(double x, double y) 
{
	glVertex3f((float)(-1 + 2 * x * gui_context.px), (float)(1 - 2 * y * gui_context.py), 0);
}

/* Cleanup */

// cleanup the stuff allocated by gui_helper_initialize
void gui_release()
{
	// todo
	// note we should release all caption_* data, textures, ...
	// meh
}

/* Components */

/* Menu */

void gui_make_menu(GUI_Panel * panel)
{
	panel->menu_sibling = NULL; 
	panel->menu_first_child = NULL;
	panel->menu_type = GUI_MAIN_MENU_CONTAINER;
	gui_set_debugging_title(panel, "main menu container");
}

GUI_Panel * gui_new_menu_item(GUI_Panel * parent, char * caption)
{
	// copy the menu caption 
	caption = strdup(caption);

	// find the last child of parent 
	GUI_Panel * sibling = parent->menu_first_child;

	if (sibling)
	{
		while (sibling->menu_sibling) 
		{
			sibling = sibling->menu_sibling;
		}
	}

	// create new panel 
	GUI_Panel * panel = NULL;
	if (sibling) 
	{
		// there already are some menu items inside parent 
		switch (parent->menu_type) 
		{
			case GUI_MAIN_MENU_CONTAINER:
				panel = gui_new_panel(parent, sibling, gui_on_the_right_vfill);
				gui_set_width(panel, 100);
				panel->menu_type = GUI_MAIN_MENU_ITEM;
				break;

			case GUI_MAIN_MENU_ITEM:
			case GUI_MENU_ITEM: 
				panel = gui_new_panel(parent, sibling, gui_below);
				gui_set_width(panel, 210);
				gui_set_height(panel, 25);
				panel->menu_type = GUI_MENU_ITEM;
				break;
		}

		sibling->menu_sibling = panel;
	}
	else
	{
		// this will be the first menu item in parent 
		switch (parent->menu_type) 
		{
			case GUI_MAIN_MENU_CONTAINER:
				panel = gui_new_panel(parent, NULL, gui_left_vfill);
				gui_set_width(panel, 100);
				panel->menu_type = GUI_MAIN_MENU_ITEM;
				break;
			case GUI_MAIN_MENU_ITEM: 
				panel = gui_new_panel(parent, parent, gui_below);
				gui_set_width(panel, 210);
				gui_set_height(panel, 25);
				panel->menu_type = GUI_MENU_ITEM;
				break;
			case GUI_MENU_ITEM: 
				panel = gui_new_panel(parent, parent, gui_on_the_right);
				gui_set_width(panel, 210); 
				gui_set_height(panel, 25);
				panel->menu_type = GUI_MENU_ITEM;
				break;
		}

		parent->menu_first_child = panel;
	}

	// set debugging title 
	gui_set_debugging_title(panel, "menu item");

	// set caption 
	panel->caption = caption;

	// add pointer to parent
	panel->menu_parent = parent;

	// register standard event handlers 
	GUI_EVENT_HANDLER(panel, mousedown) = gui_menu_event_mousedown; 
	panel->on_mousedown_is_internal = true;
	GUI_EVENT_HANDLER(panel, mousedownout) = gui_menu_event_mousedownout;
	panel->on_mousedownout_is_internal = true;
	GUI_SET_RENDERING_STYLE(panel, menu_item);

	// all menu items are hidden by default 
	if (panel->menu_type != GUI_MAIN_MENU_ITEM)
	{
		gui_set_panel_visible(panel, false);
	}

	return panel;
}

// set callback for action triggered by clicking the menu item 
void gui_set_menu_action(GUI_Panel * panel, GUI_Event event)
{
	panel->on_menuitemaction = event;
}

// hide menu 
void gui_aux_menu_hide(GUI_Panel * panel)
{
	GUI_Panel * child = panel->menu_first_child; 

	while (child) 
	{
		if (child->menu_type == GUI_MENU_ITEM)
		{
			gui_set_panel_visible(child, false);
		}

		gui_aux_menu_hide(child);
		child = child->menu_sibling;
	}
}

// mouse down event handler shows the menu item and it's children
void gui_menu_event_mousedown(const GUI_Event_Descriptor event)
{
	// respond only to left click
	if (event.sdl_event.button.button != SDL_BUTTON_LEFT) return;

	// show all it's ancestors
	GUI_Panel * parent = event.panel, * root = NULL;
	while (parent = parent->menu_parent)
	{
		gui_set_panel_visible(parent, true);

		// show immediate children
		GUI_Panel * child = parent->menu_first_child;
		while (child)
		{
			gui_set_panel_visible(child, true);
			child = child->menu_sibling;
		}

		root = parent; 
	}

	// show all it's children
	GUI_Panel * child = event.panel->menu_first_child;
	while (child)
	{
		gui_set_panel_visible(child, true);
		child = child->menu_sibling;
	}

	// check if this menu has assigned an action 
	if (event.panel->on_menuitemaction)
	{
		GUI_FIRE_EVENT(event.panel, menuitemaction, event.sdl_event);
		
		// if it has action assigned, we should hide it 
		gui_aux_menu_hide(root);
	}
}

// mouse down out event hides all menu items
void gui_menu_event_mousedownout(const GUI_Event_Descriptor event)
{
	// respond only to left click
	if (event.sdl_event.button.button != SDL_BUTTON_LEFT) return;

	if (event.panel->menu_type == GUI_MENU_ITEM)
	{
		gui_set_panel_visible(event.panel, false);
	}
}

/* Tabs */

// create new tabs group
GUI_Panel * gui_new_tabs(GUI_Panel * parent, GUI_Panel * sibling, GUI_Positioner positioner)
{
	GUI_Panel * tabs = gui_new_panel(parent, sibling, positioner);
	GUI_Panel * tabs_buttons = gui_new_panel(tabs, NULL, gui_top_hfill);
	gui_set_height(tabs_buttons, 30);
	GUI_Panel * tabs_content = gui_new_panel(tabs, tabs_buttons, gui_below_fill);

	tabs->tab_sibling = tabs_buttons;
	tabs->tab_complement = tabs_content; 

	// add debugging titles 
	gui_set_debugging_title(tabs, "tabs container"); 
	gui_set_debugging_title(tabs_buttons, "tabs buttons"); 
	gui_set_debugging_title(tabs_content, "tabs content space");

	return tabs;
}

// create new tab inside a tabs group 
GUI_Panel * gui_new_tab(GUI_Panel * tabs_container, const char * title)
{
	GUI_Panel * last_button = tabs_container->tab_sibling; 
	GUI_Panel * tab_content = tabs_container->tab_complement; 

	// find the last tab 
	while (last_button->tab_sibling) last_button = last_button->tab_sibling; 

	// add button
	GUI_Panel * new_button;
	if (last_button == tabs_container->tab_sibling)
	{
		new_button = gui_new_panel(tabs_container->tab_sibling, NULL, gui_left_vfill);
	}
	else
	{
		new_button = gui_new_panel(tabs_container->tab_sibling, last_button, gui_on_the_right_vfill);
	}

	gui_set_width(new_button, 80);
	last_button->tab_sibling = new_button;
	new_button->tab_container = tabs_container;
	gui_set_debugging_title(new_button, "tab button");
	new_button->caption = title;
	gui_set_margins(new_button, 4, 2, 0, 2);
	GUI_SET_RENDERING_STYLE(new_button, tab_button);

	// add the tab itself
	GUI_Panel * new_tab = gui_new_panel(tabs_container->tab_complement, NULL, gui_fill);
	new_button->tab_complement = new_tab;
	gui_set_debugging_title(new_tab, "tab content");
	gui_set_margins(new_tab, 0, 2, 4, 2);
	GUI_SET_RENDERING_STYLE(new_tab, tab_content);
	
	// if it is not the first one, hide it 
	if (last_button != tabs_container->tab_sibling) 
	{
		gui_set_panel_visible(new_button->tab_complement, false);
	}

	// register the event that makes it clickable 
	GUI_EVENT_HANDLER(new_button, mousedown) = gui_tab_event_mousedown;
	new_button->on_mousedown_is_internal = true;

	return new_tab;
}

// respond to the user clicking on a tab button 
void gui_tab_event_mousedown(const GUI_Event_Descriptor event)
{
	const GUI_Panel * const tabs = event.panel->tab_container;
	GUI_Panel * tab_button = tabs->tab_sibling->tab_sibling; 

	// hide all tabs
	while (tab_button != NULL) 
	{
		gui_set_panel_visible(tab_button->tab_complement, false); 
		tab_button = tab_button->tab_sibling;
	}

	// show the clicked tab 
	gui_set_panel_visible(event.panel->tab_complement, true);
}

/* Labels */ 

// create label 
GUI_Panel * gui_new_label(GUI_Panel * parent, GUI_Panel * sibling, const char * caption)
{
	GUI_Panel * label = gui_new_panel(parent, sibling, sibling ? gui_below_hfill : gui_top_hfill, "label");
	gui_set_height(label, 20);
	// gui_set_top_margin(label, 4);
	gui_set_margins(label, 2, 0, 2, 0);
	label->caption = strdup(caption);
	GUI_SET_RENDERING_STYLE(label, label); // note unfortunate combination of names makes this line unreadable
	return label;
}

/* Checkbox */ 

// creating checkboxes 
GUI_Panel * gui_new_checkbox(GUI_Panel * parent, GUI_Panel * sibling, const char * caption)
{
	GUI_Panel * checkbox = gui_new_panel(parent, sibling, sibling ? gui_below_hfill : gui_top_hfill, "checkbox");
	gui_set_height(checkbox, 20);
	// gui_set_top_margin(checkbox, 4);
	gui_set_margins(checkbox, 2, 0, 2, 0);
	checkbox->caption = caption;
	GUI_SET_RENDERING_STYLE(checkbox, checkbox); // note unfortunate combination of names makes this line unreadable
	GUI_EVENT_HANDLER(checkbox, mousedown) = gui_checkbox_event_mousedown;
	checkbox->on_mousedown_is_internal = true;
	return checkbox;
}

// set the value represented by the checkbox 
void gui_set_checkbox(GUI_Panel * checkbox, bool value)
{
	checkbox->i = (int)value;
}

// get the value represented by the checkbox 
bool gui_get_checkbox(GUI_Panel * checkbox) // todo we might want to unify the gui_is_* and gui_get_* naming conventions
{
	return checkbox->i != 0;
}

// mouse event handlers
void gui_checkbox_event_mousedown(const GUI_Event_Descriptor event)
{
	gui_set_checkbox(event.panel, !gui_get_checkbox(event.panel));
}

/* Radio buttons */ 

// creating radio buttons 
GUI_Panel * gui_new_radio_group(GUI_Panel * parent)
{
	GUI_Panel * group = gui_new_panel(parent, NULL, gui_no_position, "radio group");
	return group;
}

GUI_Panel * gui_new_radio_button(GUI_Panel * parent, GUI_Panel * sibling, GUI_Panel * group, const char * caption)
{
	caption = strdup(caption);
	GUI_Panel * radio = gui_new_panel(parent, sibling, sibling ? gui_below_hfill : gui_top_hfill, "radio");

	// connect with siblings and parent
	radio->radio_parent = group;
	if (!group->radio_next)
	{
		group->radio_next = radio;
		radio->i = 1;
	}
	else
	{
		GUI_Panel * last_radio = group->radio_next;
		while (last_radio->radio_next != NULL)
		{
			last_radio = last_radio->radio_next;
		}
		last_radio->radio_next = radio;
	}

	// style and everything
	gui_set_height(radio, 20);
	// gui_set_top_margin(radio, 4);
	gui_set_margins(radio, 2, 0, 2, 0);
	radio->caption = caption;
	GUI_SET_RENDERING_STYLE(radio, radio); // note unfortunate combination of names makes this line unreadable
	GUI_EVENT_HANDLER(radio, mousedown) = gui_radio_event_mousedown;
	radio->on_mousedown_is_internal = true;

	return radio;
}

// getters and setters 
int gui_get_radio_value(GUI_Panel * radio)
{
	if (radio->radio_parent) 
	{
		// it's a radio button 
		return radio->i; 
	}
	else
	{
		// it's a group 
		GUI_Panel * sibling = radio->radio_next;

		while (sibling) 
		{
			if (sibling->i) return sibling->i;
			sibling = sibling->radio_next;
		}

		return 0; // the group can be empty
	}
}

int gui_get_width(GUI_Panel * panel)
{
	return panel->x2 - panel->x1;
}

int gui_get_height(GUI_Panel * panel)
{
	return panel->y2 - panel->y1;
}

// mouse event handlers 
void gui_radio_event_mousedown(const GUI_Event_Descriptor event)
{
	// go through all it's siblings 
	GUI_Panel * group = event.panel->radio_parent;
	GUI_Panel * sibling = group->radio_next;
	// assert (sibling) 

	// assign zero to all other siblings and 
	int i = 1;
	while (sibling)
	{
		sibling->i = (sibling != event.panel) ? 0 : i;
		sibling = sibling->radio_next;
		i++;
	}
}

/* Buttons */ 

// creating buttons
GUI_Panel * gui_new_button(GUI_Panel * parent, GUI_Panel * sibling, const char * caption, GUI_Event button_clicked)
{
	GUI_Panel * button = gui_new_panel(parent, sibling, sibling ? gui_below_hfill : gui_top_hfill, "button");
	gui_set_height(button, 26);
	gui_set_margins(button, 2, 4, 2, 4);
	button->caption = caption;
	GUI_SET_RENDERING_STYLE(button, button); // note unfortunate combination of names makes this line unreadable
	GUI_EVENT_HANDLER(button, mousedown) = gui_button_event_mousedown;
	button->on_mousedown_is_internal = true; // note that it is strange to have macro for accessing events but we have to access internality flag directly
	GUI_EVENT_HANDLER(button, buttonaction) = button_clicked;
	return button;
}

// mouse event handlers
void gui_button_event_mousedown(const GUI_Event_Descriptor event)
{
	if (event.sdl_event.button.button == SDL_BUTTON_LEFT) 
	{
		GUI_FIRE_EVENT(event.panel, buttonaction, event.sdl_event);
	}
}

/* GLView */

// transform a panel into glview
void gui_make_glview(GUI_Panel * panel, GUI_GLView_Render render)
{
	panel->glview_render = render;
	GUI_EVENT_HANDLER(panel, render) = gui_glview_event_render;
	panel->on_render_is_internal = true;
}

// glview render event 
void gui_glview_event_render(GUI_Panel * panel)
{
	// note that we might also want to remember attributes

	glMatrixMode(GL_PROJECTION); 
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW); 
	glPushMatrix();

	glViewport(
		panel->effective_x1, 
		gui_context.height - panel->effective_y2, 
		panel->effective_x2 - panel->effective_x1, 
		panel->effective_y2 - panel->effective_y1
	);

	panel->glview_render();

	glViewport(0, 0, gui_context.width, gui_context.height);

	glMatrixMode(GL_MODELVIEW); 
	glPopMatrix();
	glMatrixMode(GL_PROJECTION); 
	glPopMatrix();
}

/* Separators */

// creating horizontal separator
GUI_Panel * gui_new_hseparator(GUI_Panel * parent, GUI_Panel * sibling)
{
	GUI_Panel * separator = gui_new_panel(parent, sibling, sibling ? gui_below_hfill : gui_top_hfill, "separator");
	gui_set_height(separator, 8);
	gui_set_margins(separator, 2, 4, 2, 4);
	GUI_SET_RENDERING_STYLE(separator, separator); // note unfortunate combination of names makes this line unreadable
	return separator;
}

/* Multi-threading */ 

void gui_lock()
{
	pthread_mutex_lock(&gui_structures_lock);
}

void gui_unlock()
{
	pthread_mutex_unlock(&gui_structures_lock);
}
