/*

  insight3d - image based 3d modelling software
  Copyright (C) 2007-2008  Lukas Mach
                           email: lukas.mach@gmail.com 
                           web: http://mach.matfyz.cz/

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
  
*/

#include "tool_core.h"

Tools_State tools_state;
pthread_mutex_t tools_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

DYNAMIC_STRUCTURE(Tool_Menu_Items, Tool_Menu_Item);
DYNAMIC_STRUCTURE(Tool_Parameters, Tool_Parameter);

const int TOOL_ENUM_FIRST = 1;

// callback function activating a tool
void tool_activate_handler(const GUI_Event_Descriptor event)
{
	LOCK(tools)
	{
		// call end routine for previous tool
		if (tools_state.tools[tools_state.current].end)
		{
			tools_state.tools[tools_state.current].end();
		}

		// select new tool 
		const int tool_id = event.panel->i;
		tools_state.current = tool_id; 

		// call begin routine for new tool
		if (tools_state.tools[tool_id].begin) 
		{
			Tool_Begin_Event_Handler call = tools_state.tools[tool_id].begin;
			UNLOCK(tools);
			call();
		}
		else
		{
			UNLOCK(tools);
		}
	}
	WAS_UNLOCKED_RW(tools);
}

// menu item pressed 
void tool_menu_item_pressed(const GUI_Event_Descriptor event)
{
	const int i = event.panel->i;
	const Tool_Menu_Item * const item = tools_state.menu_items.data + i;

	// take a look what this menu item should do 
	switch (item->action_type) 
	{
		case TOOL_ACTION_FUNCTION_CALL:
		{
			item->function_call();
			break; 
		}
	}
}

// tab button pressed 
void tool_tab_button_pressed(const GUI_Event_Descriptor event)
{
	const Tool_Function_Call call = (Tool_Function_Call)(event.panel->p); 
	call();
}

// creates new UI tool 
size_t tool_create(const UI_Mode mode_affinity, const char * const title, const char * const hint)
{
	ASSERT(!tools_state.finalized, "tried to create new tool after finalization of tools subsystem");

	// initialize new tool structure
	size_t & n = tools_state.count; 
	memset(tools_state.tools + n, 0, sizeof(Tool)); 

	// specify affinity 
	tools_state.tools[n].mode_affinity = mode_affinity;

	// set it's title
	strncpy(tools_state.tools[n].title, title, 100);
	tools_state.tools[n].title[99] = '\0';

	// set hint
	strncpy(tools_state.tools[n].hint, hint, 2000); 
	tools_state.tools[n].hint[1999] = '\0';

	// initialize array of parameters 
	DYN_INIT(tools_state.tools[n].parameters);

	return n++; 
}

// * adding items into main menu and toolbar *

// adds another menu item into menu structure 
void tool_add_menu_item(const char * const menu_path, Tool_Action_Type action_type, Tool_Function_Call function_call)
{
	ASSERT(!tools_state.finalized, "tried to create new menu item after finalization of tools subsystem");

	const size_t len = strlen(menu_path); 
	ASSERT(len < 250, "maximum menu path length exceeded");
	ASSERT(menu_path[len - 1] == '|', "error when registering a menu item for tool - menu path must end with a pipe character");
	ASSERT(action_type != TOOL_ACTION_FUNCTION_CALL || function_call, "NULL supplied as function_call when registering menu item which triggers function call");

	ADD(tools_state.menu_items);
	LAST(tools_state.menu_items).action_type = action_type;
	LAST(tools_state.menu_items).path = strdup(menu_path);
	LAST(tools_state.menu_items).menu_item = NULL;
	LAST(tools_state.menu_items).tool = tools_state.tools + (tools_state.count - 1);
	LAST(tools_state.menu_items).function_call = function_call;
}

// void menu items 
void tool_register_menu_void(const char * const menu_path) 
{
	tool_add_menu_item(menu_path, TOOL_ACTION_NONE, NULL);
}

// if the user clicks this menu item, fire a function call (usually from tools source code)
void tool_register_menu_function(const char * const menu_path, Tool_Function_Call function_call)
{
	tool_add_menu_item(menu_path, TOOL_ACTION_FUNCTION_CALL, function_call);
}

// if the user clicks this toolbar item, set this tool as current tool 
void tool_register_toolbar_button(const char * const toolbar_title) 
{
	ASSERT(tools_state.count > 0, "tried to create toolbar button, but no tool exists");

	// add button to gui 
	const size_t n = tools_state.count - 1;
	const bool first = ui_state.side_top_last == NULL;
	ui_state.side_top_last = gui_new_button(ui_state.side_top, ui_state.side_top_last, toolbar_title, tool_activate_handler);
	/*if (!first) 
	{
		// gui_set_height(ui_state.side_top_last, gui_get_height(ui_state.side_top_last) - ui_state.side_top_last->margin_top);
		gui_set_height(ui_state.side_top_last, 30);
		gui_set_top_margin(ui_state.side_top_last, 0);
	}*/

	ui_state.side_top_last->i = n;
}

// create tab for this tool 
void tool_create_tab(const char * const tab_title)
{
	ASSERT(tools_state.count > 0, "tried to create tab for tool, but no tool exists");
	ASSERT(!tools_state.finalized, "tab creation requested for tool which was finalized - this is not nice (although probably harmless)");
	const size_t n = tools_state.count - 1; 
	ASSERT(!tools_state.tools[n].tab, "tab already created - only one tab per tool is supported");

	tools_state.tools[n].tab = gui_new_tab(ui_state.tabs, tab_title); // AG_NotebookAddTab(ui_state.tabs, tab_title, AG_BOX_VERT);
	// T
	// tools_state.tools[n].box = AG_VBoxNew(tools_state.tools[n].tab, AG_VBOX_HFILL | AG_BOX_FRAME);*/
}

// comparator of menu items 
int tool_menu_items_comparator(const void * a, const void * b) 
{
	Tool_Menu_Item * first = (Tool_Menu_Item *)a, * second = (Tool_Menu_Item *)b; 

	// unset items should be at the end 
	if (first->set == false && second->set == false) return 0; 
	if (first->set == false) return 1; 
	if (second->set == false) return -1; 

	// compare two paths 
	size_t i; 
	for (i = 0; first->order[i] != 0 && second->order[i] != 0 && first->order[i] == second->order[i]; i++); 
	
	if (first->order[i] < second->order[i]) return -1;
	else if (first->order[i] > second->order[i]) return 1; 
	else return 0;
}

// helper routine - returns number of common path parts the two parts have
int tool_menu_common_path_parts(const char * path1, const char * path2) 
{
	int count = 0; 
	while (*path1 != '\0' && *path2 != '\0' && *path1 == *path2)
	{
		if (*path1 == '|') count++;
		path1++;
		path2++;
	}

	return count;
}

// helper routine - returns true iff the second part (from prefix to prefix_end) 
// is prefix of the first one 
bool tool_menu_path_prefix(const char * path, const char * prefix, const char * prefix_end) 
{
	while (*prefix != '\0' && *path != '\0') 
	{
		if (*prefix != *path) return false;
		prefix++; 
		path++;
	}

	return *prefix == '\0';
}

// finalizes the registration of application tools 
void tool_finalize()
{
	// preprocess order of menu items
	unsigned short int order = 1;
	for ALL(tools_state.menu_items, i) 
	{
		Tool_Menu_Item * const item = tools_state.menu_items.data + i; 

		// skip ordered prefixes
		int j = -1; 
		char * path = item->path; 
		while (*path != '\0')
		{
			if (*path == '|')
			{
				j++;

				// consider only the unordered ones
				if (item->order[j] == 0) 
				{
					// save information about order of this submenu into menu paths of all relevant items 
					for ALL(tools_state.menu_items, k) 
					{
						Tool_Menu_Item * const ordered_item = tools_state.menu_items.data + k; 

						if (tool_menu_path_prefix(ordered_item->path, item->path, path))
						{
							ordered_item->order[j] = order;
						}
					}
				}

				order++;
			}

			path++;
		}
	}

	// sort the menu_items array
	qsort((void *)tools_state.menu_items.data, tools_state.menu_items.count, sizeof(Tool_Menu_Item), tool_menu_items_comparator);

	// build menus
	GUI_Panel * items[100];
	memset(&items, 0, sizeof(GUI_Panel *) * 100);
	int prev_depth = 0;

	const Tool_Menu_Item * prev_item = NULL; 
	for ALL(tools_state.menu_items, menu_items_iterator) 
	{
		Tool_Menu_Item * const item = tools_state.menu_items.data + menu_items_iterator; 

		// how many path parts do these two items have in common?
		int common_parts = 0;
		if (prev_item != NULL) 
		{
			common_parts = tool_menu_common_path_parts(prev_item->path, item->path);
		}

		// if there are no common parts, then we're dealing with special root element - let's create it 
		if (common_parts == 0) 
		{
			if (strcmp(item->path, "Main menu|") == 0) 
			{
				items[0] = item->menu_item = tools_state.application_menu;
			}
			//else if (strcmp(item->path, "Toolbar/") == 0)
			//{
			//	items[0] = item->menu_item = NULL;
			//}
			else
			{
				ASSERT(false, "unknown root menu item in tool registration");
			}

			items[0] = item->menu_item;
			prev_depth = 1;
		}
		else
		{
			// the previous and current item have some common path parts 
			const char * delimiter = item->path;
			int i = 0;
			while (i < common_parts)
			{
				ASSERT(*delimiter != '\0', "inconsistency with calculated number of common path parts in menu path");
				delimiter = strchr(delimiter, '|'); 
				ASSERT(delimiter, "menu path must end with a pipe character"); 
				delimiter++;
				i++;
			}

			ASSERT(*delimiter != '\0', "duplicit menu items"); 

			// we have the name of the first new menu item at the beginning of the delimiter string 
			while (*delimiter != '\0')
			{
				// find next delimiter 
				const char * next_delimiter = strchr(delimiter, '|');
				ASSERT(next_delimiter, "menu path must end with a pipe character");

				// extract menu item 
				char * name = ALLOC(char, next_delimiter - delimiter + 1);
				memcpy(name, delimiter, sizeof(char) * (next_delimiter - delimiter));
				name[next_delimiter - delimiter] = '\0'; 

				// create new menu item
				if (*(next_delimiter + 1) == '\0')
				{
					// if it's the last one, then it might not be void menu item 
					items[i] = gui_new_menu_item(items[i - 1], name);
					gui_set_menu_action(items[i], tool_menu_item_pressed); 
					items[i]->i = menu_items_iterator;
					i++;
				}
				else
				{
					// create new menu item
					items[i] = gui_new_menu_item(items[i - 1], name);
					items[i]->i = 0;
					i++;
				}

				// free constructed name 
				FREE(name);
				delimiter = next_delimiter + 1;
			}

			// save the last inserted menu item 
			ASSERT(i > 0, "menu depth has to be positive");
			item->menu_item = items[i - 1];

			prev_depth = i;
		}

		prev_item = item;
	}
}

// * parameters * 

void tool_register_real(size_t id, const char * const title, double default_real, double min, double max, double increment)
{
	ASSERT(!tools_state.finalized, "new parameter created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before parameter is registered");

	DYN(tools_state.tools[n].parameters, id);

	char text[1000];
	sprintf(text, "%s%.1lf", title, default_real);

	tools_state.tools[n].parameters.data[id].type = TOOL_PARAMETER_REAL;
	tools_state.tools[n].parameters.data[id].title = strdup(title); 
	tools_state.tools[n].parameters.data[id].value = ALLOC(double, 1);
	*((double *)tools_state.tools[n].parameters.data[id].value) = default_real;
	tools_state.tools[n].parameters.data[id].real_widget
		= tools_state.tools[n].tab_last
		= gui_new_label(tools_state.tools[n].tab, tools_state.tools[n].tab_last, text); 
	gui_set_disabled(tools_state.tools[n].tab_last, true);
	tools_state.tools[n].tab_last->p = tools_state.tools[n].parameters.data[id].value;
	/* AG_NumericalNewDbl(tools_state.tools[n].box, AG_NUMERICAL_HFILL, NULL, title, (double *)tools_state.tools[n].parameters.data[id].value);
	AG_NumericalSetMinDbl(tools_state.tools[n].parameters.data[id].real_widget, min);
	AG_NumericalSetMaxDbl(tools_state.tools[n].parameters.data[id].real_widget, max);
	AG_NumericalSetIncrement(tools_state.tools[n].parameters.data[id].real_widget, increment);*/
}

void tool_register_int(size_t id, const char * const title, int default_int, int min, int max, int increment)
{
	ASSERT(!tools_state.finalized, "new parameter created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before parameter is registered");

	DYN(tools_state.tools[n].parameters, id);

	char text[1000];
	sprintf(text, "%s%d", title, default_int);

	tools_state.tools[n].parameters.data[id].type = TOOL_PARAMETER_INT;
	tools_state.tools[n].parameters.data[id].title = strdup(title); 
	tools_state.tools[n].parameters.data[id].value = ALLOC(int, 1);
	*((int *)tools_state.tools[n].parameters.data[id].value) = default_int;
	tools_state.tools[n].parameters.data[id].int_widget
		= tools_state.tools[n].tab_last
		= gui_new_label(tools_state.tools[n].tab, tools_state.tools[n].tab_last, text);
	gui_set_disabled(tools_state.tools[n].tab_last, true);
	tools_state.tools[n].tab_last->p = tools_state.tools[n].parameters.data[id].value;
	/*AG_NumericalSetMinDbl(tools_state.tools[n].parameters.data[id].int_widget, min);
	AG_NumericalSetMaxDbl(tools_state.tools[n].parameters.data[id].int_widget, max);
	AG_NumericalSetIncrement(tools_state.tools[n].parameters.data[id].int_widget, increment);*/
}

void tool_register_enum(size_t id, const char * const title, const char * labels[])
{
	ASSERT(!tools_state.finalized, "new parameter created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before parameter is registered");

	tools_state.tools[n].tab_last
		= gui_new_label(tools_state.tools[n].tab, tools_state.tools[n].tab_last, title); 

	DYN(tools_state.tools[n].parameters, id); 

	tools_state.tools[n].parameters.data[id].type = TOOL_PARAMETER_ENUM;
	tools_state.tools[n].parameters.data[id].title = strdup(title); 
	tools_state.tools[n].parameters.data[id].enum_labels = labels;

	GUI_Panel * group = gui_new_radio_group(tools_state.tools[n].tab);
	tools_state.tools[n].parameters.data[id].enum_widget = group;

	size_t i = 0; 
	const char * label;

	while (label = tools_state.tools[n].parameters.data[id].enum_labels[i++])
	{
		tools_state.tools[n].tab_last
			= gui_new_radio_button(tools_state.tools[n].tab, tools_state.tools[n].tab_last, group, label);
	}

	// tools_state.tools[n].parameters.data[id].value = malloc(sizeof(int));
	// *((int *)tools_state.tools[n].parameters.data[id].value) = 0;
	// AG_WidgetBindInt(tools_state.tools[n].parameters.data[id].enum_widget, "value", (int *)tools_state.tools[n].parameters.data[id].value);
}

void tool_register_bool(size_t id, const char * const title, int default_bool)
{
	ASSERT(!tools_state.finalized, "new parameter created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before parameter is registered");

	DYN(tools_state.tools[n].parameters, id);
	tools_state.tools[n].parameters.data[id].value = ALLOC(int, 1);
	*((int *)tools_state.tools[n].parameters.data[id].value) = default_bool;
	tools_state.tools[n].parameters.data[id].type = TOOL_PARAMETER_BOOL;
	tools_state.tools[n].parameters.data[id].title = strdup(title); 
	tools_state.tools[n].tab_last 
		= tools_state.tools[n].parameters.data[id].bool_widget 
		= gui_new_checkbox(tools_state.tools[n].tab, tools_state.tools[n].tab_last, title);
	gui_set_checkbox(tools_state.tools[n].tab_last, (bool)tools_state.tools[n].parameters.data[id].value);
	// AG_CheckboxNewInt((void *)tools_state.tools[n].box, (int *)tools_state.tools[n].parameters.data[id].value, title);
}

void tool_create_label(const char * const title)
{
	ASSERT(!tools_state.finalized, "new tab label created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before label created");

	tools_state.tools[n].tab_last = 
		gui_new_label(tools_state.tools[n].tab, tools_state.tools[n].tab_last, title);
	// AG_LabelNew(tools_state.tools[n].box, 0, title);
}

void tool_create_button(const char * const title, Tool_Function_Call function_call)
{
	ASSERT(!tools_state.finalized, "new tab button created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before button is created");

	tools_state.tools[n].tab_last = gui_new_button(
		tools_state.tools[n].tab, 
		tools_state.tools[n].tab_last, 
		title,
		tool_tab_button_pressed
	);

	tools_state.tools[n].tab_last->p = (void *)function_call;
}

void tool_create_separator()
{
	ASSERT(!tools_state.finalized, "new tab separator created after finalization of tools subsystem");
	size_t n = tools_state.count - 1; 
	ASSERT(tools_state.tools[n].tab, "tab must be created before separator is created");

	tools_state.tools[n].tab_last = gui_new_hseparator(
		tools_state.tools[n].tab, 
		tools_state.tools[n].tab_last
	);
}

// * getters for parameter values * 

// save all values 
void tool_fetch_parameters(size_t tool_id)
{
	ASSERT(tool_id < tools_state.count, "fetching parameters from non-existent tool"); 

	// go through all parameters of this tool and save it 
	for ALL(tools_state.tools[tool_id].parameters, i) 
	{
		Tool_Parameter * const parameter = tools_state.tools[tool_id].parameters.data + i; 

		switch (parameter->type) 
		{
			case TOOL_PARAMETER_ENUM: 
			{
				parameter->fetched_int = gui_get_radio_value(parameter->enum_widget) - 1;
				break; 
			}

			case TOOL_PARAMETER_INT:
			case TOOL_PARAMETER_BOOL: 
			{
				parameter->fetched_int = *(int *)parameter->value;
				break; 
			}

			case TOOL_PARAMETER_REAL: 
			{
				parameter->fetched_real = *(double *)parameter->value;
				break; 
			}

			default: 
			{
				ASSERT(false, "unknown parameter type");
				break;
			}
		}
	}
}

// real parameter getter 
double tool_get_real(size_t tool_id, size_t parameter_id)
{
	ASSERT(tool_id < tools_state.count, "fetching parameters from non-existent tool");
	ASSERT_IS_SET(tools_state.tools[tool_id].parameters, parameter_id);
	ASSERT(tools_state.tools[tool_id].parameters.data[parameter_id].type == TOOL_PARAMETER_REAL, "real parameter expected");

	return tools_state.tools[tool_id].parameters.data[parameter_id].fetched_real;
}

// int parameter getter
int tool_get_int(size_t tool_id, size_t parameter_id)
{
	ASSERT(tool_id < tools_state.count, "fetching parameters from non-existent tool");
	ASSERT_IS_SET(tools_state.tools[tool_id].parameters, parameter_id);
	ASSERT(tools_state.tools[tool_id].parameters.data[parameter_id].type == TOOL_PARAMETER_INT, "int parameter expected");

	return tools_state.tools[tool_id].parameters.data[parameter_id].fetched_int;
}

// enum parameter getter 
int tool_get_enum(size_t tool_id, size_t parameter_id)
{
	ASSERT(tool_id < tools_state.count, "fetching parameters from non-existent tool");
	ASSERT_IS_SET(tools_state.tools[tool_id].parameters, parameter_id);
	ASSERT(tools_state.tools[tool_id].parameters.data[parameter_id].type == TOOL_PARAMETER_ENUM, "enum parameter expected");

	return tools_state.tools[tool_id].parameters.data[parameter_id].fetched_int;
}

// bool parameter getter
bool tool_get_bool(size_t tool_id, size_t parameter_id)
{
	ASSERT(tool_id < tools_state.count, "fetching parameters from non-existent tool");
	ASSERT_IS_SET(tools_state.tools[tool_id].parameters, parameter_id);
	ASSERT(tools_state.tools[tool_id].parameters.data[parameter_id].type == TOOL_PARAMETER_BOOL, "bool parameter expected");

	return tools_state.tools[tool_id].parameters.data[parameter_id].fetched_int == 1;
}

// * typical requirements * 

// most tools work only when current shot is selected and loaded 
bool tool_requires_current_shot()
{
	// if no shot is currently displayed, we don't have anything to do
	return INDEX_IS_SET(ui_state.current_shot) && shots.data[ui_state.current_shot].info_status >= GEOMETRY_INFO_DEDUCED;
}

// * registering event handlers * 

void tool_set_process_events_function(const Tool_Process_Events process_events)
{
	tools_state.tools[tools_state.count - 1].process_events = process_events;
}

void tool_set_key_pressed_event_handler(const Tool_Key_Pressed_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].key_pressed = handler;
}

void tool_set_mouse_down_handler(const Tool_Mouse_Down_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].mouse_down = handler;
}

void tool_set_click_handler(const Tool_Click_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].click = handler;
}

void tool_set_move_handler(const Tool_Move_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].move = handler;
}

void tool_set_dragging_handler(const Tool_Dragging_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].dragging = handler; 
}

void tool_set_dragging_done_handler(const Tool_Dragging_Done_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].dragging_done = handler;
}

void tool_set_begin_handler(const Tool_Begin_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].begin = handler;
}

void tool_set_end_handler(const Tool_End_Event_Handler handler)
{
	tools_state.tools[tools_state.count - 1].end = handler;
}

/* Actions */ 

char * tool_choose_file()
{
	char * filename = NULL; 

	UNLOCK(geometry);
	{
#ifdef LINUX
		GtkWidget * dialog = gtk_file_chooser_dialog_new(
					"Open File",
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL
		);

		if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char * fn = NULL; 
			
			fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			const size_t len = strlen(fn);
			filename = ALLOC(char, len + 1); 
			memcpy(filename, fn, len + 1);
		}

		gtk_widget_destroy(dialog);
#else
		// todo eventually replace with Vista's common item dialog 
		OPENFILENAMEA ofn;
		char szFile[1000];
		const HWND hwnd = GetForegroundWindow();

		// initialize the structure
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = szFile;
		ofn.lpstrFile[0] = '\0'; // maybe we could initialize this 
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// display the dialog
		if (GetOpenFileNameA(&ofn) == TRUE) 
		{
			filename = ALLOC(char, sizeof(szFile));
			memcpy(filename, szFile, sizeof(szFile));
		}
#endif
	}
	LOCK(geometry);
	return filename;
}

char * tool_choose_new_file()
{
	char * filename = NULL; 
#ifdef LINUX
	GtkWidget *dialog;
	
	dialog = gtk_file_chooser_dialog_new(
						"Save File",
						NULL,
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						NULL
	);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	
	// TODO 
	// if (user_edited_a_new_document)
	{
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), ".");
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "Untitled document");
	}
	// else
	//	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), filename_for_existing_document);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char * fn;
	
		fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		const size_t len = strlen(fn);
		filename = ALLOC(char, len + 1); 
		memcpy(filename, fn, len + 1);

		g_free(fn);
	}
	
	gtk_widget_destroy(dialog);
#else
	// todo eventually replace with Vista's common item dialog 
	OPENFILENAMEA ofn;
	char szFile[1000];
	const HWND hwnd = GetForegroundWindow();
	HANDLE hf;

	// initialize the structure
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0'; // maybe we could initialize this 
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;

	// display the dialog
	if (GetSaveFileNameA(&ofn) == TRUE) 
	{
		filename = ALLOC(char, sizeof(szFile));
		memcpy(filename, szFile, sizeof(szFile));
	}
#endif
	return filename;
}

/* Progressbar */

void tool_start_progressbar()
{
	/*LOCK_RW(opencv)
	{
		cvNamedWindow("progressbar"); 
		IplImage * img = cvCreateImage(cvSize(200, 30), IPL_DEPTH_8U, 3); 
		cvZero(img); 
		cvShowImage("progressbar", img); 
		cvWaitKey(10);
		cvReleaseImage(&img);
	}
	UNLOCK_RW(opencv);*/

	ATOMIC(tools, 
		tools_state.progressbar_show = true; 
		tools_state.progressbar_percentage = 0;
	);
}

void tool_end_progressbar()
{
	/*LOCK_RW(opencv)
	{
		// cvDestroyWindow("progressbar");
		cvShowImage("progressbar", NULL);
		cvDestroyAllWindows();
		cvWaitKey(1);
	}
	UNLOCK_RW(opencv);*/

	ATOMIC(tools, 
		tools_state.progressbar_show = false;
		tools_state.progressbar_percentage = 0;
	);
}

void tool_show_progress(double percentage)
{
	if (percentage < 0) percentage = 0; 
	if (percentage > 1) percentage = 1;

	ATOMIC(tools, tools_state.progressbar_percentage = percentage; );

	/*LOCK_RW(opencv)
	{
		IplImage * img = cvCreateImage(cvSize(200, 30), IPL_DEPTH_8U, 3); 
		cvZero(img); 
		for (int i = 0; i < 30; i++) 
		{
			cvLine(img, cvPoint(0, i), cvPoint(199 * percentage, i), cvScalar(255, 160, 160));
		}
		cvShowImage("progressbar", img);
		cvWaitKey(10);
		cvReleaseImage(&img); 
	}
	UNLOCK_RW(opencv);*/
}

