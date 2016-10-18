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

#ifndef __UI_CONSTANTS
#define __UI_CONSTANTS

#include "interface_opengl.h"

// * drawing styles *

// background 
extern const float UI_STYLE_BACKGROUND[3];

// 3d rendering
extern const Drawing_Style UI_STYLE_VERTEX;
extern const Drawing_Style UI_STYLE_POLYGON;

// 2d drawing
extern const Drawing_Style UI_STYLE_POINT;
extern const Drawing_Style UI_STYLE_POINT_NOT_CREDIBLE;
extern const Drawing_Style UI_STYLE_POLYGON_JOINING_POINTS;
extern const Drawing_Style UI_STYLE_CONTOUR;

// special objects 
extern const Drawing_Style UI_STYLE_CAMERA;

// selected items 
extern const Drawing_Style UI_STYLE_SELECTED_POINT;
extern const Drawing_Style UI_STYLE_SELECTED_VERTEX;
extern const Drawing_Style UI_STYLE_SELECTED_POLYGON;

// helpers 
extern const Drawing_Style UI_STYLE_HELPERS;

// user interaction
extern const Drawing_Style UI_STYLE_SELECTION_BOX;
extern const Drawing_Style UI_STYLE_SELECTION_BORDER;

// * user interface * 

// UI behavior
extern const double UI_CLICK_DELAY;
extern const double UI_CLICK_DISTANCE_SQ;
extern const double UI_GROUND_ANGLE_DRAGGING_STEP;
extern const double UI_FOCUS_PIXEL_DISTANCE_SQ;

// shadows 
extern const double UI_SHADOW_DISTANCE;
extern const double UI_SHADOW_ALPHA;
extern const double UI_SHADOW_SIZE;
extern const int UI_SHADOW_PRECISION;

// note take a loot at http://www.sapdesignguild.org/editions/philosophy_articles/colors.asp
// note http://kuler.adobe.com/ (nice palettes are: "warm lounge", "old man winter" by nine70, 
// "warm" by dario88, "the balloons" by whoneycutt, "fabric spot test | 02" by whoneycutt, 
// "orange on gray" by mats.holmberg, "oddend" by martin, "1944mustang" by nathaniel,
// "Howard Johnsons" by johnl)

#endif
