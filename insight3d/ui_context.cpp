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

#include "ui_context.h"

const double UI_CONTEXT_SCALE = 0.33;
DYNAMIC_STRUCTURE(Context_Items, Context_Item);
const size_t UI_CONTEXT_MAX_SHOWN_ITEMS = 10;
static Context_State context_state; 

// initialize context popup 
bool ui_context_initialize()
{
	memset(&context_state, 0, sizeof(context_state)); 
	return true;
}

// show context popup 
void ui_context_show() 
{
	context_state.visible = true;
}

// hide context popup
void ui_context_hide() 
{
	context_state.visible = false;
	context_state.timer = context_state.delay;
}

// render context popup using opengl 
void ui_context_display(double shot_x, double shot_y)
{
	if (!context_state.visible) return;

	// optionally delay showing the popup
	context_state.timer += 0.1;
	if (context_state.timer < 0) 
	{
		return; 
	}
	else if (context_state.timer > 1)
	{
		context_state.timer = 1;
	}

	const double animation_alpha = context_state.timer;

	// convert shot coordinates to opengl 
	double x, y;
	ui_convert_xy_from_shot_to_opengl(shot_x, shot_y, x, y);

	// count the number of items to show 
	size_t count = context_state.count;

	// decide where to show the popup 
	context_state.positive_x = true;
	context_state.positive_y = false;

	// calculate sizes and distances 
	const double 
		distance_to_cursor = visualization_calc_dx(50),
		symbol_height = visualization_calc_dy(20),
		symbol_width = visualization_calc_dx(20),
		border_x = visualization_calc_dx(5),
		border_y = visualization_calc_dy(5)
	;

	const double 
		x_px = visualization_calc_dx(1), 
		y_px = visualization_calc_dy(1)
	;

	// go through all items and calculate the size of the popup
	double total_height = border_y; 
	double max_width = 0;
		
	for ALL(context_state.items, i) 
	{
		Context_Item * const item = context_state.items.data + i;

		if (item->width > max_width) max_width = item->width; 
		total_height += visualization_calc_dy(UI_CONTEXT_SCALE * item->height) + border_y;
	}

	max_width *= UI_CONTEXT_SCALE;
	if (max_width == 0) return;
	double total_width = visualization_calc_dx(max_width) + 2 * border_x;

	// draw shadow 
	LOCK_RW(opengl)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		const int shadow_precision = 12, shadow_depth = 3;
		for (int j = 0; j <= shadow_depth; j++) 
		{
			for (int i = 0; i <= shadow_precision; i++) 
			{
				const double a = i * 2 * CORE_PI / shadow_precision;
				double dx = j * 2.0 * x_px * cos(a), dy = j * 2.0 * y_px * sin(a);
				if (j != shadow_depth || i != shadow_precision) 
				{
					glColor4d(0, 0, 0, 0.02 * animation_alpha);
				}
				else
				{
					glColor4d(105 / 255.0, 105 / 255.0, 105 / 255.0, animation_alpha);
					dx = 0; 
					dy = 0;
				}

				glBegin(GL_POLYGON);
					glVertex3d(x + dx + distance_to_cursor, y + dy - symbol_height / 2, -1);
					glVertex3d(x + dx + distance_to_cursor + total_width, y + dy - symbol_height / 2, -1);
					glVertex3d(x + dx + distance_to_cursor + total_width, y + dy - symbol_height / 2 + total_height, -1);
					glVertex3d(x + dx + distance_to_cursor, y + dy - symbol_height / 2 + total_height, -1);
				glEnd();
				glBegin(GL_POLYGON);
					glVertex3d(x + dx + distance_to_cursor, y + dy - symbol_height / 2, -1);
					glVertex3d(x + dx + distance_to_cursor, y + dy + symbol_height / 2, -1);
					glVertex3d(x + dx + distance_to_cursor - symbol_width / 2, y + dy, -1);
				glEnd();
			}
		}
		glDisable(GL_BLEND);
	}
	UNLOCK_RW(opengl);

	// draw items 
	double y_level = border_y;
	for ALL(context_state.items, i) 
	{
		Context_Item * const item = context_state.items.data + i;

		const double 
			item_width = visualization_calc_dx(UI_CONTEXT_SCALE * item->width), 
			item_height = visualization_calc_dy(UI_CONTEXT_SCALE * item->height)
		;

		const int 
			crosshair_distance = 4,
			crosshair_length = 12
		; 

		bool drawn = false;
		if (item->content == UI_CONTEXT_THUMBNAIL)
		{
			// upload texture to opengl
			GLuint texture;
			image_loader_upload_to_opengl(item->request);

			// draw it 
			double tx, ty, sx, sy; 
			if (image_loader_opengl_upload_ready(item->request, &texture, &tx, &ty, &sx, &sy))
			{
				LOCK_RW(opengl)
				{
					glBindTexture(GL_TEXTURE_2D, texture);
					glColor4d(1, 1, 1, animation_alpha);
					glBegin(GL_POLYGON);
						glTexCoord2d(tx, sy); glVertex3d(x + distance_to_cursor + border_x, y - symbol_height / 2 + y_level, -1);
						glTexCoord2d(sx, sy); glVertex3d(x + distance_to_cursor + border_x + item_width, y - symbol_height / 2 + y_level, -1);
						glTexCoord2d(sx, ty); glVertex3d(x + distance_to_cursor + border_x + item_width, y - symbol_height / 2 + y_level + item_height, -1);
						glTexCoord2d(tx, ty); glVertex3d(x + distance_to_cursor + border_x, y - symbol_height / 2 + y_level + item_height, -1);
					glEnd();
					glBindTexture(GL_TEXTURE_2D, 0);
				}
				UNLOCK_RW(opengl);

				drawn = true;
			}
		}
		else if (item->content == UI_CONTEXT_ZOOM)
		{
			// zoom into current shot should be displayed
			GLuint texture;
			if (
				image_loader_nonempty_handle(shots.data[ui_state.current_shot].image_loader_request) &&
				image_loader_opengl_upload_ready(shots.data[ui_state.current_shot].image_loader_request, &texture)
			)
			{
				double tx, ty, sx, sy;

				tx = shot_x - 0.33 * UI_CONTEXT_SCALE * (item->width / (double)shots.data[ui_state.current_shot].width);
				ty = shot_y - 0.33 * UI_CONTEXT_SCALE * (item->height / (double)shots.data[ui_state.current_shot].height);
				sx = shot_x + 0.33 * UI_CONTEXT_SCALE * (item->width / (double)shots.data[ui_state.current_shot].width);
				sy = shot_y + 0.33 * UI_CONTEXT_SCALE * (item->height / (double)shots.data[ui_state.current_shot].height);

				LOCK_RW(opengl)
				{
					glBindTexture(GL_TEXTURE_2D, texture);
					glColor4d(1, 1, 1, animation_alpha);
					glBegin(GL_POLYGON);
						glTexCoord2d(tx, sy); glVertex3d(x + distance_to_cursor + border_x, y - symbol_height / 2 + y_level, -1);
						glTexCoord2d(sx, sy); glVertex3d(x + distance_to_cursor + border_x + item_width, y - symbol_height / 2 + y_level, -1);
						glTexCoord2d(sx, ty); glVertex3d(x + distance_to_cursor + border_x + item_width, y - symbol_height / 2 + y_level + item_height, -1);
						glTexCoord2d(tx, ty); glVertex3d(x + distance_to_cursor + border_x, y - symbol_height / 2 + y_level + item_height, -1);
					glEnd();
					glBindTexture(GL_TEXTURE_2D, 0);
				}
				UNLOCK_RW(opengl);

				drawn = true;
			}
		}

		// if it has been drawn, render also decoration (crosshair, ...)
		if (drawn) 
		{
			if (item->decoration == UI_CONTEXT_CROSSHAIR)
			{
				LOCK_RW(opengl)
				{
					glLineWidth(1);
					const double
						cx = x + distance_to_cursor + border_x + item_width / 2,
						cy = y - symbol_height / 2 + y_level + item_height / 2;
					
					glBegin(GL_LINES);
						glColor4d(0, 0, 0, 0.7);
						glVertex3d(cx - (crosshair_distance + crosshair_length) * x_px, cy, -1);
						glVertex3d(cx - (crosshair_distance) * x_px, cy, -1);
						glVertex3d(cx + (crosshair_distance + crosshair_length) * x_px, cy, -1);
						glVertex3d(cx + (crosshair_distance) * x_px, cy, -1);
						glVertex3d(cx, cy - (crosshair_distance + crosshair_length) * y_px, -1);
						glVertex3d(cx, cy - (crosshair_distance) * y_px, -1);
						glVertex3d(cx, cy + (crosshair_distance + crosshair_length) * y_px, -1);
						glVertex3d(cx, cy + (crosshair_distance) * y_px, -1);
						glColor4d(1, 1, 1, 0.2);
						glVertex3d(cx, cy - (crosshair_distance) * y_px, -1);
						glVertex3d(cx, cy + (crosshair_distance) * y_px, -1);
						glVertex3d(cx - (crosshair_distance) * x_px, cy, -1);
						glVertex3d(cx + (crosshair_distance) * x_px, cy, -1);
					glEnd();
				}
				UNLOCK_RW(opengl);
			}
		}

		y_level += border_y + item_height;
	}
}

// add thumbnail
void ui_context_add_thumbnail(const size_t shot_id, const double x, const double y, const double width, const double height, Context_Decoration decoration)
{
	if (context_state.count >= UI_CONTEXT_MAX_SHOWN_ITEMS) return;

	ADD(context_state.items);
	LAST(context_state.items).content = UI_CONTEXT_THUMBNAIL;
	LAST(context_state.items).shot_id = shot_id;
	LAST(context_state.items).x = x;
	LAST(context_state.items).y = y;
	LAST(context_state.items).width = width;
	LAST(context_state.items).height = height;
	LAST(context_state.items).decoration = decoration;
	LAST(context_state.items).request = image_loader_new_request(
		shot_id, shots.data[shot_id].image_filename, IMAGE_LOADER_CONTINUOUS_LOADING, 
		IMAGE_LOADER_CENTER, x, y, width, height, true
	);

	context_state.count++;
}

// add zoom 
void ui_context_add_zoom(const double width, const double height, Context_Decoration decoration)
{
	if (context_state.count >= UI_CONTEXT_MAX_SHOWN_ITEMS) return;

	ADD(context_state.items);
	LAST(context_state.items).content = UI_CONTEXT_ZOOM;
	LAST(context_state.items).shot_id = SIZE_MAX;
	LAST(context_state.items).x = -1;
	LAST(context_state.items).y = -1;
	LAST(context_state.items).width = width;
	LAST(context_state.items).height = height;
	LAST(context_state.items).decoration = decoration;

	context_state.count++;
}

// clear item-list 
void ui_context_clear() 
{
	context_state.count = 0; 

	// release all requests for all items 
	for ALL(context_state.items, i) 
	{
		Context_Item * const item = context_state.items.data + i; 

		if (item->content == UI_CONTEXT_THUMBNAIL)
		{
			image_loader_cancel_request(&item->request);
		}
	}

	context_state.items.count = 0;
}

// set delay before actually showing the popup 
void ui_context_set_delay(const double delay)
{
	context_state.delay = delay;
	context_state.timer = context_state.delay;
}
