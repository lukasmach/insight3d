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

#ifndef __CORE_IMAGE_LOADER
#define __CORE_IMAGE_LOADER

#include "interface_opengl.h"
#include "interface_opencv.h"
#include "interface_sdl.h"
#include "core_state.h"
#include "core_debug.h"
#include "core_structures.h"
#include <iostream>

// specifies the desired quality of requested image
enum Image_Loader_Quality { IMAGE_LOADER_NOT_LOADED, IMAGE_LOADER_LOW_RESOLUTION, IMAGE_LOADER_FULL_RESOLUTION, IMAGE_LOADER_CONTINUOUS_LOADING };
enum Image_Loader_Content { IMAGE_LOADER_ALL, IMAGE_LOADER_CENTER, IMAGE_LOADER_REGION };

// small structure uniquely identifying single request
struct Image_Loader_Request_Handle
{
	size_t id, time;
};

// request descriptor
struct Image_Loader_Request
{
	bool set;

	// what do we want to load
	size_t shot_id;
	Image_Loader_Quality quality;
	Image_Loader_Content content;
	double x, y, sx, sy;

	// result
	bool done;
	Image_Loader_Quality current_quality;
	GLuint gl_texture_id;
	Image_Loader_Quality gl_texture_quality;
	double gl_texture_min_x, gl_texture_min_y, gl_texture_max_x, gl_texture_max_y;
	IplImage * image;
};

DYNAMIC_STRUCTURE_DECLARATIONS(Image_Loader_Requests, Image_Loader_Request);

// we'll need to manage a set of images
struct Image_Loader_Shot
{
	bool set;

	// flag used to suggest that this shot might be potencially needed in the future
	bool suggested;

	// image meta
	const char * filename;
	int width, height;

	// full version
	IplImage * full;
	GLuint full_texture; 
	int full_counter, full_unprocessed_counter; 

	// low version 
	IplImage * low; 
	GLuint low_texture;
	int low_counter, low_unprocessed_counter;
};

DYNAMIC_STRUCTURE_DECLARATIONS(Image_Loader_Shots, Image_Loader_Shot);

// release unused image from memory (full resolution version)
// global_lock must be locked 
// note we could use some more sophisticated releasing strategy
// note that we could count requests on full and low versions separately
void image_loader_free_full();

// release unused image from memory (low resolution version)
// global_lock must be locked
// note see the note to function above
void image_loader_free_low();

// try to resolve request immediately
// must be locked
void image_loader_resolve_request(const size_t request_id);

// thread function
void * image_loader_thread_function(void * arg);

// initialize image loader subsystem 
bool image_loader_initialize(const int cache_full_count, const int cache_low_count);
bool image_loader_start_thread();

// release image loader subsystem 
// todo release also shots and requests 
void image_loader_release();

// creates new request to load shot image 
Image_Loader_Request_Handle image_loader_new_request(
	const size_t shot_id,
	const char * const filename,
	const Image_Loader_Quality quality, 
	const Image_Loader_Content content = IMAGE_LOADER_ALL, 
	const double x = -1, 
	const double y = -1, 
	const double sx = -1,
	const double sy = -1,
	const bool fake = false
);

// check if the handle is nonempty 
bool image_loader_nonempty_handle(Image_Loader_Request_Handle handle);

// cancel existing request 
void image_loader_cancel_request(Image_Loader_Request_Handle * handle);

// determines if requested image is loaded into memory (at least low res version if we're ok with continuous loading)
static bool image_loader_request_ready_nolock(Image_Loader_Request_Handle handle);

// version of the above function for use outside of this lbrary 
bool image_loader_request_ready(Image_Loader_Request_Handle handle);

// determines if the requested image is already waiting for us on gpu (checks for both low and full version of the texture)
bool image_loader_opengl_upload_ready_dual(
	Image_Loader_Request_Handle handle, GLuint * full_texture, GLuint * low_texture,
	double * texture_min_x = NULL, double * texture_min_y = NULL, double * texture_max_x = NULL, double * texture_max_y = NULL
);

// determined if the requested image is already on gpu
bool image_loader_opengl_upload_ready(
	Image_Loader_Request_Handle handle, GLuint * texture, 
	double * texture_min_x = NULL, double * texture_min_y = NULL, double * texture_max_x = NULL, double * texture_max_y = NULL
);

// uploads texture to opengl
// note if there are more requests for one image, it's cause multiple uploads to opengl
void image_loader_upload_to_opengl(Image_Loader_Request_Handle handle);

// get original dimensions of this request's image 
void image_loader_get_original_dimensions(Image_Loader_Request_Handle handle, int * width, int * height);

// flush texture ids
void image_loader_flush_texture_ids();

// clear all suggested flags 
void image_loaded_flush_suggested();

// cancel all requests
void image_loader_cancel_all_requests();

#endif
