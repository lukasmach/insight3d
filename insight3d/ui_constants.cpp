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

#include "ui_constants.h"

// * drawing styles *

// todo UI_STYLE_ constants are now somewhat unused since the UI was being redone, integrate this 

// background 
const float UI_STYLE_BACKGROUND[3] = { 0.0F, 0.0F, 0.0F }; 

// 3d rendering
const Drawing_Style UI_STYLE_VERTEX = { { 1.0F, 0.44F, 1.0F }, 1.0F, 2.0F, 1.0F }; // { { 1, 0.44, 1 }, 1, 2, 1 }; 
const Drawing_Style UI_STYLE_POLYGON = { { 1.0F, 1.0F, 1.0F }, 1.0F, 1.0F, 1.0F }; // { { 0.6, 0.6, 1 }, 1.5, 1, 0.4 }; 

// 2d drawing
const Drawing_Style UI_STYLE_POINT = { { 1.0F, 1.0F, 1.0F }, 1.0F, 2.0F, 1.0F };
const Drawing_Style UI_STYLE_POINT_NOT_CREDIBLE = { { 0.7F, 0.7F, 0.7F }, 1.0F, 2.0F, 1.0F };
const Drawing_Style UI_STYLE_POLYGON_JOINING_POINTS = { { 1.0F, 1.0F, 1.0F }, 1.0F, 1.0F, 1.0F }; 
const Drawing_Style UI_STYLE_CONTOUR = { { 0.7F, 0.7F, 0.7F }, 1.0F, 1.0F, 1.0F }; // todo 

// special objects 
const Drawing_Style UI_STYLE_CAMERA = { { 1.0F, 1.0F, 1.0F }, 1.0F, 2.0F, 1.0F }; 

// selected items 
const Drawing_Style UI_STYLE_SELECTED_POINT = { { 1.0F, 1.0F, 0.44F }, 1.0F, 2.0F, 1.0F };
const Drawing_Style UI_STYLE_SELECTED_VERTEX = { { 0.0F, 1.0F, 0.44F }, 1.0F, 2.0F, 1.0F }; 
const Drawing_Style UI_STYLE_SELECTED_POLYGON = { { 1.0F, 1.0F, 0.44F }, 1.0F, 2.0F, 1.0F }; 

// helpers 
const Drawing_Style UI_STYLE_HELPERS = { { 0.8F, 0.8F, 0.6F }, 1.0F, 1.0F, 1.0F }; 

// user interaction
const Drawing_Style UI_STYLE_SELECTION_BORDER = { { 1.0F, 1.0F, 0.44F }, 1.0F, 1.0F, 1.0F }; 

// * user interface * 

// UI behavior
const double UI_CLICK_DELAY = 350;
const double UI_CLICK_DISTANCE_SQ = 64;
const double UI_GROUND_ANGLE_DRAGGING_STEP = 0.01;
const double UI_FOCUS_PIXEL_DISTANCE_SQ = 64;

// shadows 
const double UI_SHADOW_DISTANCE = 0;
const double UI_SHADOW_ALPHA = 0.05;
const double UI_SHADOW_SIZE = 0.0075;
const int UI_SHADOW_PRECISION = 4;

// note take a loot at http://www.sapdesignguild.org/editions/philosophy_articles/colors.asp
// note http://kuler.adobe.com/ (nice palettes are: "warm lounge", "old man winter" by nine70, 
// "warm" by dario88, "the balloons" by whoneycutt, "fabric spot test | 02" by whoneycutt, 
// "orange on gray" by mats.holmberg, "oddend" by martin, "1944mustang" by nathaniel,
// "Howard Johnsons" by johnl)
