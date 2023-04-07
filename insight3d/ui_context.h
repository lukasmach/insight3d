#ifndef __UI_CONTEXT
#define __UI_CONTEXT

#include "core_image_loader.h"
#include "ui_core.h"
#include "ui_visualization_helpers.h"

// decoration style 
enum Context_Content { UI_CONTEXT_THUMBNAIL, UI_CONTEXT_ZOOM };
enum Context_Decoration { UI_CONTEXT_NONE, UI_CONTEXT_CROSSHAIR };

// setting 
extern const double UI_CONTEXT_SCALE;

// context popup items
struct Context_Item
{
	bool set;

	// content type
	Context_Content content;

	// image and position shown
	size_t shot_id;
	double x, y, width, height;

	// decoration 
	Context_Decoration decoration;

	// request for this image sent to loading subsystem
	Image_Loader_Request_Handle request;
};

DYNAMIC_STRUCTURE_DECLARATIONS(Context_Items, Context_Item);

// provides info about context popup
struct Context_State 
{
	// popup state 
	bool visible; 
	double x, y; // where is the context menu shown 
	bool positive_x, positive_y; // direction in which the window is shown 

	// settings 
	double delay, timer;

	// popup content
	size_t count;
	Context_Items items;
};

extern const size_t UI_CONTEXT_MAX_SHOWN_ITEMS;

// routines
bool ui_context_initialize();
void ui_context_show();
void ui_context_hide();
void ui_context_display(double shot_x, double shot_y);
void ui_context_add_thumbnail(const size_t shot_id, const double x, const double y, const double width, const double height, Context_Decoration decoration);
void ui_context_add_zoom(const double width, const double height, Context_Decoration decoration);
void ui_context_clear();
void ui_context_set_delay(const double delay);

#endif
