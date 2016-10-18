#include "gui_style.h"
#include "SDL.h"
#include "SDL_opengl.h"

void gui_style_menu_item(GUI_Panel * panel)
{
	// render the menu item background 
	if (!panel->focus)
	{
		glBegin(GL_POLYGON); 
			glColor3d(0.3, 0.3, 0.3);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.25, 0.25, 0.25);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.2, 0.2, 0.2);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glEnd();
	}
	else
	{
		glBegin(GL_POLYGON); 
			glColor3d(0.6, 0.6, 0.6);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.35, 0.35, 0.35);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.2, 0.2, 0.2);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glEnd();
	}

	// display also the text, if it's ready 
	if (panel->caption_texture_id) 
	{
		gui_opengl_display_text(
			panel->caption_texture_id, 
			panel->caption_image->width, 
			panel->caption_image->height, 
			panel->caption_width, 
			panel->caption_height, 
			panel->effective_x1 + 3, 
			panel->effective_y1 + (panel->y2 - panel->y1 - panel->caption_height) / 2,
			1
		);
	}
}

void gui_style_background(GUI_Panel * panel)
{
	glBegin(GL_POLYGON); 
		glColor3d(0.1, 0.1, 0.1);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
	glEnd();
}

void gui_style_tab_button(GUI_Panel * panel)
{
	if (!gui_is_panel_visible(panel->tab_complement))
	{
		if (!panel->focus)
		{
			glBegin(GL_POLYGON); 
				glColor3d(0.2, 0.2, 0.2);
				glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
				glColor3d(0.1, 0.1, 0.1);
				glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glEnd();
		}
		else
		{
			glBegin(GL_POLYGON); 
				glColor3d(0.3, 0.3, 0.3);
				glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
				glColor3d(0.2, 0.2, 0.2);
				glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
				glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glEnd();
		}
	}
	else
	{
		glBegin(GL_POLYGON); 
			glColor3d(0.25, 0.25, 0.25);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glEnd();
	}

	// display also the text, if it's ready 
	if (panel->caption_texture_id)
	{
		gui_opengl_display_text(
			panel->caption_texture_id, 
			panel->caption_image->width, 
			panel->caption_image->height, 
			panel->caption_width, 
			panel->caption_height, 
			panel->effective_x1 + 2, 
			panel->effective_y1 + (panel->y2 - panel->y1 - panel->caption_height - panel->margin_bottom - panel->margin_top) / 2,
			1
		);
	}
}

void gui_style_tab_content(GUI_Panel * panel)
{
	float middle_y = (5 * panel->effective_y1 + panel->effective_y2) / (float)6.0;

	glBegin(GL_POLYGON); 
		glColor3d(0.25, 0.25, 0.25);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
		glColor3d(0.15, 0.15, 0.15);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * middle_y / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * middle_y / (float)gui_context.height, 0);
	glEnd();

	glBegin(GL_POLYGON); 
		glColor3d(0.15, 0.15, 0.15);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * middle_y / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * middle_y / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
	glEnd();
}

void gui_style_label(GUI_Panel * panel)
{
	gui_opengl_display_text(
		panel->caption_texture_id, 
		panel->caption_image->width, 
		panel->caption_image->height, 
		panel->caption_width, 
		panel->caption_height, 
		panel->effective_x1 + 2, 
		panel->effective_y1 + (panel->y2 - panel->y1 - panel->caption_height - panel->margin_bottom - panel->margin_top) / 2,
		!gui_is_panel_disabled(panel) ? 1.0 : 0.6
	);
}

void gui_style_checkbox(GUI_Panel * panel)
{
	gui_opengl_display_text(
		panel->caption_texture_id,
		panel->caption_image->width,
		panel->caption_image->height,
		panel->caption_width,
		panel->caption_height,
		panel->effective_x1 + 16,
		panel->effective_y1 + (panel->y2 - panel->y1 - panel->caption_height - panel->margin_bottom - panel->margin_top) / 2,
		1
	);

	const double checkbox_side = 8; // length of the side of the checkbox's square
	const double offset_y = (panel->y2 - panel->y1 - checkbox_side - panel->margin_bottom - panel->margin_top) / 2 - 1;

	glColor3d(1.0, 1.0, 1.0);
	glBegin(GL_POLYGON);
		gui_opengl_vertex(panel->effective_x1 + 6, panel->effective_y1 + offset_y);
		gui_opengl_vertex(panel->effective_x1 + 6 + checkbox_side, panel->effective_y1 + offset_y);
		gui_opengl_vertex(panel->effective_x1 + 6 + checkbox_side, panel->effective_y1 + offset_y + checkbox_side);
		gui_opengl_vertex(panel->effective_x1 + 6, panel->effective_y1 + offset_y + checkbox_side);
		gui_opengl_vertex(panel->effective_x1 + 6, panel->effective_y1 + offset_y);
	glEnd();

	const int inner_d = 1;
	if (!panel->focus) 
	{
		glColor3d(0, 0, 0);
	}
	else
	{
		glColor3d(0.3, 0.3, 0.3);
	}

	glBegin(GL_POLYGON);
		gui_opengl_vertex(panel->effective_x1 + 6 + inner_d, panel->effective_y1 + offset_y + inner_d);
		gui_opengl_vertex(panel->effective_x1 + 6 + checkbox_side - inner_d, panel->effective_y1 + offset_y + inner_d);
		gui_opengl_vertex(panel->effective_x1 + 6 + checkbox_side - inner_d, panel->effective_y1 + offset_y + checkbox_side - inner_d);
		gui_opengl_vertex(panel->effective_x1 + 6 + inner_d, panel->effective_y1 + offset_y + checkbox_side - inner_d);
		gui_opengl_vertex(panel->effective_x1 + 6 + inner_d, panel->effective_y1 + offset_y + inner_d);
	glEnd();

	if (panel->i)
	{
		const int inner_d = 2;

		glColor3d(1, 1, 1);
		glBegin(GL_POLYGON);
			gui_opengl_vertex(panel->effective_x1 + 6 + inner_d, panel->effective_y1 + offset_y + inner_d);
			gui_opengl_vertex(panel->effective_x1 + 6 + checkbox_side - inner_d, panel->effective_y1 + offset_y + inner_d);
			gui_opengl_vertex(panel->effective_x1 + 6 + checkbox_side - inner_d, panel->effective_y1 + offset_y + checkbox_side - inner_d);
			gui_opengl_vertex(panel->effective_x1 + 6 + inner_d, panel->effective_y1 + offset_y + checkbox_side - inner_d);
			gui_opengl_vertex(panel->effective_x1 + 6 + inner_d, panel->effective_y1 + offset_y + inner_d);
		glEnd();
	}
}

void gui_style_radio(GUI_Panel * panel)
{
	gui_opengl_display_text(
		panel->caption_texture_id,
		panel->caption_image->width,
		panel->caption_image->height,
		panel->caption_width,
		panel->caption_height,
		panel->effective_x1 + 16,
		panel->effective_y1 + (panel->y2 - panel->y1 - panel->caption_height - panel->margin_bottom - panel->margin_top) / 2,
		1
	);

	const double radio_side = 4; // half-length of the side of the checkbox's square
	const double offset_y = (panel->y2 - panel->y1 - radio_side - panel->margin_bottom - panel->margin_top) / 2 - 3;

	glColor3d(1.0, 1.0, 1.0);
	glBegin(GL_POLYGON);
		for (float i = 0; i < 1; i += 0.05)
		{
			gui_opengl_vertex(
				panel->effective_x1 + 6 + radio_side + radio_side * cos(i * 2 * 3.14159), 
				panel->effective_y1 + offset_y + radio_side + radio_side * sin(i * 2 * 3.14159)
			);
		}
	glEnd();

	const int inner_d = 1;
	if (!panel->focus) 
	{
		glColor3d(0, 0, 0);
	}
	else
	{
		glColor3d(0.3, 0.3, 0.3);
	}

	glBegin(GL_POLYGON);
		for (float i = 0; i < 1; i += 0.05)
		{
			gui_opengl_vertex(
				panel->effective_x1 + 6 + radio_side + (radio_side - inner_d) * cos(i * 2 * 3.14159), 
				panel->effective_y1 + offset_y + radio_side + (radio_side - inner_d) * sin(i * 2 * 3.14159)
			);
		}
	glEnd();

	if (panel->i)
	{
		const int inner_d = 2;

		glColor3d(1, 1, 1);
		glBegin(GL_POLYGON);
			for (float i = 0; i < 1; i += 0.05)
			{
				gui_opengl_vertex(
					panel->effective_x1 + 6 + radio_side + (radio_side - inner_d) * cos(i * 2 * 3.14159), 
					panel->effective_y1 + offset_y + radio_side + (radio_side - inner_d) * sin(i * 2 * 3.14159)
				);
			}
		glEnd();
	}
}

void gui_style_button(GUI_Panel * panel)
{
	// render the menu item background 
	if (!panel->focus)
	{
		glBegin(GL_POLYGON); 
			glColor3d(0.4, 0.4, 0.4);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.35, 0.35, 0.35);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.2, 0.2, 0.2);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glEnd();
	}
	else
	{
		glBegin(GL_POLYGON); 
			glColor3d(0.6, 0.6, 0.6);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.35, 0.35, 0.35);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
			glColor3d(0.2, 0.2, 0.2);
			glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
			glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glEnd();
	}

	// display also the text, if it's ready 
	if (panel->caption_texture_id) 
	{
		gui_opengl_display_text(
			panel->caption_texture_id, 
			panel->caption_image->width, 
			panel->caption_image->height, 
			panel->caption_width, 
			panel->caption_height, 
			panel->effective_x1 + 3, 
			panel->effective_y1 + (panel->effective_y2 - panel->effective_y1 - panel->caption_height) / 2,
			1
		);
	}
}

void gui_style_separator(GUI_Panel * panel)
{
	// render the menu item background 
	glBegin(GL_POLYGON); 
		glColor3d(0.4, 0.4, 0.4);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
		glColor3d(0.35, 0.35, 0.35);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y1 / (float)gui_context.height, 0);
		glColor3d(0.2, 0.2, 0.2);
		glVertex3d(-1 + 2 * panel->effective_x2 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
		glVertex3d(-1 + 2 * panel->effective_x1 / (float)gui_context.width, 1 - 2 * panel->effective_y2 / (float)gui_context.height, 0);
	glEnd();
}