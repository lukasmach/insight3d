#ifndef __TOOL_CORE
#define __TOOL_CORE

#include "portability.h"
#include "core_structures.h"
#include "core_debug.h"
#include "ui_state.h"
#include "gui.h"

#ifdef LINUX
#include <gtk/gtk.h>
#else
#include "windows.h"
#include "Commdlg.h"
#endif

// threading support 
extern pthread_mutex_t tools_mutex;

// tool event handlers
typedef bool (* Tool_Process_Events)();
typedef void (* Tool_Begin_Event_Handler)(); 
typedef void (* Tool_Key_Pressed_Event_Handler)();
typedef bool (* Tool_Mouse_Down_Event_Handler)(double x, double y, int button);
typedef void (* Tool_Click_Event_Handler)(double x, double y, int button);
typedef void (* Tool_Move_Event_Handler)(double x, double y);
typedef void (* Tool_Dragging_Event_Handler)(double x, double y, int button); 
typedef void (* Tool_Dragging_Done_Event_Handler)(double x1, double y1, double x2, double y2, int button); // todo what to do when the user moves mouse out of window - this event is not fired... 
typedef void (* Tool_End_Event_Handler)(); 

// tool UI interface
typedef void (* Tool_Function_Call)(); // callback function to be triggered whenever user presses button in menu

// tool's parameter, which can be changed in UI
enum Tool_Parameter_Type { TOOL_PARAMETER_REAL, TOOL_PARAMETER_INT, TOOL_PARAMETER_ENUM, TOOL_PARAMETER_BOOL }; 

struct Tool_Parameter
{
	bool set;

	Tool_Parameter_Type type; 

	const char * title;

	// pointer to UI widgets 
	GUI_Panel * real_widget, * int_widget, * enum_widget, * bool_widget;

	// temporary value of parameter (set externally from UI toolkit) 
	void * value;

	// saved value of parameter (filled by calling tool_fetch_parameters())
	int fetched_int;
	double fetched_real; 

	const char * * enum_labels;
};

DYNAMIC_STRUCTURE_DECLARATIONS(Tool_Parameters, Tool_Parameter);

// structure representing tools
struct Tool
{
	char title[100];    // name of the tool
	char hint[2000];    // short help text

	UI_Mode mode_affinity;    // in which mode is this tool available?

	// UI handles 
	GUI_Panel * tab, * tab_last;
	GUI_Panel * box; // T note unused

	// parameters
	Tool_Parameters parameters;

	// event handlers
	Tool_Process_Events process_events;    // decides whether or not to actually process user input in some situation
	Tool_Mouse_Down_Event_Handler mouse_down;
	Tool_Key_Pressed_Event_Handler key_pressed;
	Tool_Click_Event_Handler click;
	Tool_Move_Event_Handler move;
	Tool_Dragging_Event_Handler dragging; 
	Tool_Dragging_Done_Event_Handler dragging_done;
	Tool_Begin_Event_Handler begin; 
	Tool_End_Event_Handler end;
};

// button type 
enum Tool_Action_Type { TOOL_ACTION_NONE, TOOL_ACTION_SHOW, TOOL_ACTION_FILE_DIALOG, TOOL_ACTION_FUNCTION_CALL };

// structure for menu items
struct Tool_Menu_Item
{
	bool set;
	char * path;                        // path in menu
	unsigned short int order[20];       // order of items in menu
	Tool * tool;                        // pointer to represented tool
	Tool_Action_Type action_type;       // type of action to be triggered (show tool's panel, show file dialog, ...)
	GUI_Panel * menu_item;              // pointer to GUI element
	Tool_Function_Call function_call;   // menu can trigger function call
};

DYNAMIC_STRUCTURE_DECLARATIONS(Tool_Menu_Items, Tool_Menu_Item);

// global structure encapsulating tools
struct Tools_State
{
	// the following variables are not changed atomically,
	// only their writes and reads are protected by mutex // <-- // TODO
	GUI_Panel * application_menu;
	Tool tools[100];
	size_t current, count;
	Tool_Menu_Items menu_items;
	bool finalized;

	// progressbar (accessed by the main thread and gui redering thread)
	bool progressbar_show;
	double progressbar_percentage;
};

extern Tools_State tools_state;

// passing events from one tool to another
// for example, if the tool doesn't process dragging with mouse wheel, it might 
// pass this event to selection tool, which will perform scrolling 
#define TOOL_PASS_CLICK(tool_name) tool_##tool_name##_click(x, y, button); return // note it's not possible to use this macro without enclosing it into curly braces
#define TOOL_PASS_MOUSE_DOWN(tool_name) return tool_##tool_name##_mouse_down(x, y, button)
#define TOOL_PASS_KEY(tool_name) tool_##tool_name##_key(); return
#define TOOL_PASS_MOVE(tool_name) tool_##tool_name##_move(x, y); return
#define TOOL_PASS_DRAGGING(tool_name) tool_##tool_name##_dragging(x, y, button); return
#define TOOL_PASS_DRAGGING_DONE(tool_name) tool_##tool_name##_dragging_done(x1, y1, x2, y2, button); return

// exceptions handling 
#define TOOL_PARTIAL_FAIL(message, action) action;

// helper constant 
extern const int TOOL_ENUM_FIRST;

// callback function activating a tool
void tool_activate_handler(const GUI_Event_Descriptor event);

// menu item pressed 
void tool_menu_item_pressed(const GUI_Event_Descriptor event);

// tab button pressed 
void tool_tab_button_pressed(const GUI_Event_Descriptor event);

// creates new UI tool 
size_t tool_create(const UI_Mode mode_affinity, const char * const title, const char * const hint);

// * adding items into main menu and toolbar *

// adds another menu item into menu structure
void tool_add_menu_item(const char * const menu_path, Tool_Action_Type action_type, Tool_Function_Call function_call);

// void menu items 
void tool_register_menu_void(const char * const menu_path);

// if the user clicks this menu item, fire a function call (usually from tools source code)
void tool_register_menu_function(const char * const menu_path, Tool_Function_Call function_call);

// if the user clicks this toolbar item, set this tool as current tool 
void tool_register_toolbar_button(const char * const toolbar_title);

// create tab for this tool 
void tool_create_tab(const char * const tab_title);

// comparator of menu items 
int tool_menu_items_comparator(const void * a, const void * b);

// helper routine - returns number of common path parts the two parts have
int tool_menu_common_path_parts(const char * path1, const char * path2);

// helper routine - returns true iff the second part (from prefix to prefix_end) 
// is prefix of the first one 
bool tool_menu_path_prefix(const char * path, const char * prefix, const char * prefix_end);

// finalizes the registration of application tools 
void tool_finalize();

// * parameters * 

void tool_register_real(size_t id, const char * const title, double default_real, double min, double max, double increment);
void tool_register_int(size_t id, const char * const title, int default_int, int min, int max, int increment);
void tool_register_enum(size_t id, const char * const title, const char * labels[]);
void tool_register_bool(size_t id, const char * const title, int default_bool);
void tool_create_label(const char * const title);
void tool_create_button(const char * const title, Tool_Function_Call function_call);
void tool_create_separator();

// * getters for parameter values * 

// save all values 
void tool_fetch_parameters(size_t tool_id);

// real parameter getter 
double tool_get_real(size_t tool_id, size_t parameter_id);

// int parameter getter
int tool_get_int(size_t tool_id, size_t parameter_id);

// enum parameter getter 
int tool_get_enum(size_t tool_id, size_t parameter_id);

// bool parameter getter
bool tool_get_bool(size_t tool_id, size_t parameter_id);

// * typical requirements * 

// most tools work only when current shot is selected and loaded 
bool tool_requires_current_shot();

// * registering event handlers * 

void tool_set_process_events_function(const Tool_Process_Events process_events);
void tool_set_key_pressed_event_handler(const Tool_Key_Pressed_Event_Handler handler);
void tool_set_mouse_down_handler(const Tool_Mouse_Down_Event_Handler handler);
void tool_set_click_handler(const Tool_Click_Event_Handler handler);
void tool_set_move_handler(const Tool_Move_Event_Handler handler);
void tool_set_dragging_handler(const Tool_Dragging_Event_Handler handler);
void tool_set_dragging_done_handler(const Tool_Dragging_Done_Event_Handler handler);
void tool_set_begin_handler(const Tool_Begin_Event_Handler handler);
void tool_set_end_handler(const Tool_End_Event_Handler handler);

/* Actions */ 

char * tool_choose_file();
char * tool_choose_new_file();

/* Progressbar */ 

void tool_start_progressbar(); 
void tool_end_progressbar();
void tool_show_progress(double percentage); 

#endif
