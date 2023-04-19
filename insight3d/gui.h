#ifndef GUI_MAIN
#define GUI_MAIN

// #include <stdbool.h> // todo
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "cv.h"
#include "highgui.h"
#include <math.h>
#include "pthread.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
#define OPENCV_PIXEL(img, x, y, ch) ((uchar*)(img->imageData + img->widthStep*(y)))[(x)*3 + (ch)] 

// settings 
#define GUI_MAX_PANELS 4192
#define GUI_EVENT_QUEUE_LENGTH 4192

// fixes 
extern bool mousealreadydown;

// forward declarations 
struct GUI_Panel;
struct GUI_Event_Descriptor;

// enums 
enum GUI_Menu_Type { GUI_MAIN_MENU_CONTAINER, GUI_MAIN_MENU_ITEM, GUI_MENU_ITEM };

// callbacks
typedef void (*GUI_Positioner)(GUI_Panel * panel);
// typedef void (*GUI_Event)(GUI_Panel * panel, SDL_Event sdl_event);
typedef void (*GUI_Event)(GUI_Event_Descriptor event);
typedef void (*GUI_Render)(GUI_Panel * panel);
typedef void (*GUI_GLView_Render)();

// the entire GUI is constructed from simple rectangular elements called panels
struct GUI_Panel
{
	// debug identifier
	const char * debugging_title;

	// pointer to original in case we make copies of the structure 
	GUI_Panel * original;

	// parent and sibling can be used to calculate panel's position and size
	GUI_Panel * parent, * sibling;

	// function used to calculate the position and size
	GUI_Positioner positioner;

	// parameters used by positioner
	int width, height;

	// properties
	bool 
		hidden, // panel is hidden 
		effectively_hidden // panel is hasn't been displayed in last rendering either because 
		                   // it is hidden or because it's parent is effectively hidden
	;

	// states
	bool focus, disabled;

	// events
	GUI_Event 
		on_focus, on_unfocus, on_mousemove, on_mousedown, on_mousedownout, 
		on_mouseup, on_menuitemaction, on_buttonaction;

	// event meta info 
	bool 
		on_focus_is_internal, on_unfocus_is_internal, on_mousemove_is_internal, on_mousedown_is_internal, on_mousedownout_is_internal, 
		on_mouseup_is_internal, on_menuitemaction_is_internal, on_buttonaction_is_internal;

	// computed coordinates - the region taken by the panel
	int x1, y1, x2, y2;

	// computed coordinates - after taking account for the margins
	int effective_x1, effective_y1, effective_x2, effective_y2;

	// margins 
	int margin_top, margin_bottom, margin_left, margin_right;

	// rendering function
	GUI_Render on_render;
	bool on_render_is_internal;

	// data controlled by the UI element 
	int i; 
	double d; 
	void * p;

	/* Specific purposes */

	// rendered text 
	const char * caption; 
	IplImage * caption_image;
	int caption_width, caption_height;
	GLuint caption_texture_id; 

	// todo use cases on the following

	// menu
	GUI_Menu_Type menu_type;
	GUI_Panel * menu_first_child, * menu_sibling, * menu_parent;

	// tab
	GUI_Panel * tab_complement, * tab_sibling, * tab_container;

	// radio button 
	GUI_Panel * radio_next, * radio_parent;

	// glview 
	GUI_GLView_Render glview_render;
};

// event stored in queue waiting to be processed 
struct GUI_Event_Descriptor
{
	SDL_Event sdl_event;
	GUI_Panel * panel;
	GUI_Event handler;
};

// context holds information needed to draw on screen, respond to events, etc. 
struct GUI_Context
{
	const SDL_VideoInfo * video_info;
	SDL_Surface * surface;
	int video_flags;

	int width, height;
	double px, py; // pixel width and height

	char title[256];

	GUI_Panel root_panel;
	GUI_Panel * panels[GUI_MAX_PANELS];
	size_t panels_count;

	/*// todo document this
	SDL_Event * event;
	bool event_cancelled;*/
	
	// event queue 
	GUI_Event_Descriptor event_queue[GUI_EVENT_QUEUE_LENGTH]; 
	size_t event_queue_top, event_queue_bottom;

	// font support 
	CvFont font;
};

extern GUI_Context gui_context;

// initialization 
void gui_initialize();

// helper functions 
bool gui_helper_initialize();
bool gui_helper_initialize_sdl();
bool gui_helper_initialize_opengl(); 
void gui_helper_opengl_adjust_size();

// settings
void gui_set_size(const int width, const int height); 
void gui_set_title(const char * const title);

// define gui elements 
GUI_Panel * gui_get_root_panel();
GUI_Panel * gui_new_panel(GUI_Panel * parent, GUI_Panel * sibling, GUI_Positioner positioner, const char * debugging_title = NULL);
void gui_set_width(GUI_Panel * panel, const int width);
void gui_set_height(GUI_Panel * panel, const int height);

// core functions
void gui_calculate_coordinates();

// positioners
void gui_fill(GUI_Panel * panel);
void gui_top_hfill(GUI_Panel * panel);
void gui_below_left_vfill(GUI_Panel * panel);
void gui_below_left_vfill_rest(GUI_Panel * panel);
void gui_below(GUI_Panel * panel);
void gui_below_hfill(GUI_Panel * panel);
void gui_left_vfill(GUI_Panel * panel);
void gui_on_the_right_vfill(GUI_Panel * panel);
void gui_on_the_right(GUI_Panel * panel);
void gui_below_fill(GUI_Panel * panel);
void gui_below_on_the_right_fill(GUI_Panel * panel);
void gui_no_position(GUI_Panel * panel);

// event handling
#define GUI_EVENT_HANDLER(panel, event) panel->on_##event
#define GUI_SET_RENDERING_STYLE(panel, style) panel->on_render = gui_style_##style
void gui_set_style(GUI_Panel * panel, GUI_Render rendering_function);
// void gui_cancel_event(); // currently unused // todo this probably stopped working since the introduction of multiple threads... might work on internally handled events 
bool gui_poll_event(GUI_Event_Descriptor * event);

// setters
void gui_set_panel_visible(GUI_Panel * panel, bool visibility);
void gui_set_debugging_title(GUI_Panel * panel, const char * debugging_title);
void gui_set_top_margin(GUI_Panel * panel, const int margin); 
void gui_set_bottom_margin(GUI_Panel * panel, const int margin); 
void gui_set_left_margin(GUI_Panel * panel, const int margin); 
void gui_set_right_margin(GUI_Panel * panel, const int margin); 
void gui_set_margins(GUI_Panel * panel, const int top = 0, const int right = 0, const int bottom = 0, const int left = 0);
void gui_set_disabled(GUI_Panel * panel, const bool value);

// queries
bool gui_is_panel_visible(const GUI_Panel * panel);
bool gui_is_panel_disabled(const GUI_Panel * panel);

// draw
void gui_render();
void gui_caption_render(GUI_Panel * panel);
void gui_caption_discard(GUI_Panel * panel);
void gui_caption_discard_opengl_texture(GUI_Panel * panel);

// rendering thread 
bool gui_start_rendering_thread();

// respond to SDL events
bool gui_resolve_mousemotion(SDL_Event * event);
bool gui_resolve_mousebuttondown(SDL_Event * event);
bool gui_resolve_mousebuttonup(SDL_Event * event);
bool gui_resolve_event(SDL_Event * event);

// locking GUI system 
void gui_lock();
void gui_unlock(); 

/* Auxiliary functions - opengl */

GLuint gui_upload_opengl_texture(IplImage * image);
void gui_opengl_display_text(
	GLuint texture_id, 
	int texture_width, int texture_height, 
	int width, int height, 
	double x, double y,
	double alpha
);
void gui_opengl_vertex(double x, double y);

/* Cleanup */
void gui_release();

/* Components */

/* Menu */

// creating menu
void gui_make_menu(GUI_Panel * panel);
GUI_Panel * gui_new_menu_item(GUI_Panel * parent, char * caption);
void gui_set_menu_action(GUI_Panel * panel, GUI_Event event);

// menu event handlers
void gui_menu_event_mousedown(const GUI_Event_Descriptor event);
void gui_menu_event_mousedownout(const GUI_Event_Descriptor event);

/* Tabs */

// creating tabs
GUI_Panel * gui_new_tabs(GUI_Panel * parent, GUI_Panel * sibling, GUI_Positioner positioner);
GUI_Panel * gui_new_tab(GUI_Panel * tabs_container, const char * title);

// mouse event handlers 
void gui_tab_event_mousedown(const GUI_Event_Descriptor event);

/* Labels */ 

// creating labels 
GUI_Panel * gui_new_label(GUI_Panel * parent, GUI_Panel * sibling, const char * caption);

/* Checkbox */ 

// creating checkboxes 
GUI_Panel * gui_new_checkbox(GUI_Panel * parent, GUI_Panel * sibling, const char * caption);

// getters and setters of the value represented by the checkbox 
bool gui_get_checkbox(GUI_Panel * checkbox);
void gui_set_checkbox(GUI_Panel * checkbox, bool value);

// mouse event handlers
void gui_checkbox_event_mousedown(const GUI_Event_Descriptor event);

/* Radio buttons */ 

// creating radio buttons 
GUI_Panel * gui_new_radio_group(GUI_Panel * parent);
GUI_Panel * gui_new_radio_button(GUI_Panel * parent, GUI_Panel * sibling, GUI_Panel * group, const char * caption);

// getters and setters 
int gui_get_radio_value(GUI_Panel * radio);
int gui_get_width(GUI_Panel * panel); 
int gui_get_height(GUI_Panel * panel);

// mouse event handlers 
void gui_radio_event_mousedown(const GUI_Event_Descriptor event);

/* Buttons */ 

// creating buttons 
GUI_Panel * gui_new_button(GUI_Panel * parent, GUI_Panel * sibling, const char * caption, GUI_Event button_clicked);

// mouse event handlers
void gui_button_event_mousedown(const GUI_Event_Descriptor event);

/* GLView */ 

// transform a panel into glview
void gui_make_glview(GUI_Panel * panel, GUI_GLView_Render render);

// glview render event 
void gui_glview_event_render(GUI_Panel * panel);

/* Separators */

// creating horizontal separator
GUI_Panel * gui_new_hseparator(GUI_Panel * parent, GUI_Panel * sibling);

#endif
