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

#ifndef __MVG_THRESHOLDS
#define __MVG_THRESHOLDS

// RANSAC
const int MVG_RANSAC_TRIALS = 500;
const int MVG_RANSAC_TRIANGULATION_TRIALS = 25;
const double MVG_RANSAC_PROBABILITY = 0.999;

/*// image measurement 
const double MVG_MEASUREMENT_THRESHOLD = 4.0;

// vertices
const int 
	MVG_MIN_INLIERS_TO_TRIANGULATE = 6,
	MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER = 4   // when there are no outliers in the dataset
;*/

// image measurement 
const double MVG_MEASUREMENT_THRESHOLD = 10.0;

// vertices
const int 
	MVG_MIN_INLIERS_TO_TRIANGULATE = 2,
	MVG_MIN_INLIERS_TO_TRIANGULATE_WEAKER = 2   // when there are no outliers in the dataset
;

#endif
