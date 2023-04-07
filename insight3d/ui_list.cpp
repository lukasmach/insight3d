#include "ui_list.h"

// update the list of shots
// note: currently doesn't do anything 
void ui_list_update()
{
}

// switch to next shot
void ui_next_shot(const GUI_Event_Descriptor event)
{
	if (!INDEX_IS_SET(ui_state.current_shot))
	{
		ui_workflow_default_shot();
	}
	else
	{
		for (size_t shot_id = ui_state.current_shot + 1; shot_id < shots.count; shot_id++) 
		{
			if (validate_shot(shot_id))
			{
				ui_workflow_select_shot(shot_id);
				return; 
			}
		}
	}
}

// switch to previous shot
void ui_prev_shot(const GUI_Event_Descriptor event)
{
	// note remove this in release 
	if (!INDEX_IS_SET(ui_state.current_shot))
	{
		ui_workflow_default_shot();
	}
	else
	{
		for (size_t shot_id = ui_state.current_shot; shot_id > 0;) 
		{
			shot_id--;

			if (validate_shot(shot_id))
			{
				ui_workflow_select_shot(shot_id);
				return; 
			}
		}
	}
}
