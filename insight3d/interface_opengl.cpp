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

#include "interface_opengl.h"

pthread_mutex_t opengl_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

// go to 2d mode 
void opengl_2d_mode(double x1, double y1, double x2, double y2)
{
	x1 = x1 * 2 - 1;
	y1 = y1 * 2 - 1; 
	x2 = x2 * 2 - 1; 
	y2 = y2 * 2 - 1; 

    glMatrixMode(GL_MODELVIEW); 
	glPushMatrix(); 
	glLoadIdentity(); 

	glMatrixMode(GL_PROJECTION); 
	glPushMatrix(); 
	glLoadIdentity();
	glFrustum(0.1 * x1, 0.1 * x2, 0.1 * y1, 0.1 * y2, 0.1, 1000.0);

	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST); 
}

// restore saved matrices
void opengl_end_2d_mode()
{
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

// set drawing style
void opengl_drawing_style(const Drawing_Style & style) 
{
	glColor4f(style.color[0], style.color[1], style.color[2], style.opacity); 
	glPointSize(style.point_size); 
	glLineWidth(style.line_width);
}

// saves settings of some common OpenGL attributes
void opengl_push_attribs()
{
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_POINT_BIT | GL_LINE_BIT);
}
