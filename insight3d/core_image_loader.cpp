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

/* 

  Note that the following functions are called from outside the 
  image loader thread. To prevent deadlocking, they can't have
  opengl/opencv locked if they already don't have image_loader 
  lock: 

  image_loader_upload_to_opengl
  image_loader_cancel_request

*/

#include "core_image_loader.h"

DYNAMIC_STRUCTURE(Image_Loader_Requests, Image_Loader_Request);
DYNAMIC_STRUCTURE(Image_Loader_Shots, Image_Loader_Shot);

// settings
static const int IMAGE_LOADER_FULL_SIZE = 2048, IMAGE_LOADER_LOW_SIZE = 256;
static unsigned int image_loader_cache_full_count;
static unsigned int image_loader_cache_low_count;
static const size_t IMAGE_LOADER_MAX_REQUESTS = 1000;

// threading variables 
static pthread_t image_loader_thread;
static pthread_mutex_t image_loader_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; // PTHREAD_MUTEX_INITIALIZER;
static bool image_loader_terminate;

// * global state variables *

// discrete time is used to ensure uniqueness of request handles
static size_t image_loader_time;

// number of images stored in memory
static size_t image_loader_full_counter, image_loader_low_counter;

// requests and shots
static Image_Loader_Shots image_loader_shots;
static Image_Loader_Requests image_loader_requests; 

// indices of free request slots
static size_t image_loader_free_ids[IMAGE_LOADER_MAX_REQUESTS];
static size_t image_loader_free_ids_counter;

// counter of unprocessed requests 
static size_t image_loader_unprocessed_counter;

// release unused image from memory (full resolution version)
// note we could use some more sophisticated releasing strategy
// expects LOCK_RW(image_loader), obtains LOCK_RW(opencv)
void image_loader_free_full()
{
	// if it's necessary to release something 
	if (image_loader_full_counter >= image_loader_cache_full_count)
	{
		// find shot without requests 
		size_t i; 
		bool found;
		LAMBDA_FIND(image_loader_shots, i, found, image_loader_shots.data[i].full && image_loader_shots.data[i].full_counter == 0)

		if (found)
		{
			// release this shot 
			ATOMIC_RW(opencv, cvReleaseImage(&image_loader_shots.data[i].full); );
			image_loader_shots.data[i].full = NULL;
			image_loader_full_counter--;
		}
		else
		{
			// no photo to release found... 
			// {} 
			// strong todo 
			ASSERT(false, "image cache too small... this really shouldn't happen, please report this bug; on the upside, increasing image_loader_full_count will probably fix this right away"); // todo change this message
		}
	}
}

// release unused image from memory (low resolution version)
// note see the note to function above
// expects LOCK_RW(image_loader), obtains LOCK_RW(opencv)
void image_loader_free_low()
{
	// if it's necessary to release something 
	if (image_loader_low_counter >= image_loader_cache_low_count)
	{
		// find shot without requests 
		size_t i;
		bool found;
		LAMBDA_FIND(image_loader_shots, i, found, image_loader_shots.data[i].low && image_loader_shots.data[i].low_counter == 0)

		if (found) 
		{
			// release this shot 
			ATOMIC_RW(opencv, cvReleaseImage(&image_loader_shots.data[i].low); );
			image_loader_shots.data[i].low = NULL;
			image_loader_low_counter--;
		}
		else
		{
			// no photo to release found... 
			// {} 
			// strong todo 
			ASSERT(false, "image cache too small... this really shouldn't happen, please report this bug; on the upside, increasing image_loader_low_count will probably fix this right away"); // todo change this message
		}
	}
}

// try to resolve request immediately
// expects LOCK_RW(image_loader), obtains LOCK_RW(opencv)
void image_loader_resolve_request(const size_t request_id)
{
	ASSERT_IS_SET(image_loader_requests, request_id);
	Image_Loader_Request * const request = image_loader_requests.data + request_id;
	ASSERT_IS_SET(image_loader_shots, request->shot_id);
	Image_Loader_Shot * const shot = image_loader_shots.data + request->shot_id;

	// there must be something to do
	if (request->quality == request->current_quality || request->quality == IMAGE_LOADER_CONTINUOUS_LOADING && request->current_quality == IMAGE_LOADER_FULL_RESOLUTION)
	{
		ASSERT(request->done, "request is apparently done, but not flagged as such");
		return;
	}

	// try to resolve the request
	switch (request->content) 
	{
		case IMAGE_LOADER_REGION:
		case IMAGE_LOADER_CENTER:
		{
			// we might resolve this only if something was loaded 
			if (shot->full || shot->low) 
			{
				// printf("resolving request %d for shot %d\n", request_id, request->shot_id);
				double min_x, min_y, max_x, max_y;

				// calculate the coordinates
				switch (request->content) 
				{
					case IMAGE_LOADER_CENTER:
					{
						min_x = (int)(shot->width * request->x - request->sx / 2); 
						min_y = (int)(shot->height * request->y - request->sy / 2); 
						max_x = (int)(shot->width * request->x + request->sx / 2); 
						max_y = (int)(shot->height * request->y + request->sy / 2); 
						break; 
					}

					case IMAGE_LOADER_REGION:
					{
						min_x = (int)(shot->width * request->x); 
						min_y = (int)(shot->height * request->y); 
						max_x = (int)(shot->width * request->sx); 
						max_y = (int)(shot->height * request->sy); 
						break;
					}
			
					default: 
						ASSERT(false, "unknown content type in request");
				}

				// determine which image to use
				IplImage * img = NULL;
				Image_Loader_Quality achieved_quality = IMAGE_LOADER_NOT_LOADED;
				switch (request->quality)
				{
					case IMAGE_LOADER_LOW_RESOLUTION: 
					{
						// we can fulfill the request only if the low version is loaded
						if (shot->low) 
						{
							img = shot->low;
							achieved_quality = IMAGE_LOADER_LOW_RESOLUTION;
						}
						break;
					}

					case IMAGE_LOADER_FULL_RESOLUTION: 
					{ 
						// full version has to be loaded
						if (shot->full) 
						{
							img = shot->full;
							achieved_quality = IMAGE_LOADER_FULL_RESOLUTION;
						}
						break;
					}

					case IMAGE_LOADER_CONTINUOUS_LOADING: 
					{
						if (shot->full) 
						{
							img = shot->full;
							achieved_quality = IMAGE_LOADER_FULL_RESOLUTION;
						}
						else if (shot->low) 
						{
							img = shot->low;
							achieved_quality = IMAGE_LOADER_LOW_RESOLUTION;
						}
						break;
					}
				}

				// decide, if we can actually improve upon something 
				if  (achieved_quality > request->current_quality)
				{
					// extract the region 
					IplImage * cut = NULL; 
					if (img) 
					{
						// recalculate coordinates
						int
							min_xi = (int)(min_x / shot->width * img->width),
							min_yi = (int)(min_y / shot->height * img->height),
							max_xi = (int)(max_x / shot->width * img->width),
							max_yi = (int)(max_y / shot->height * img->height)
						;

						// create image for the region 
						ATOMIC_RW(opencv, cut = opencv_create_exp_image(max_xi - min_xi + 1, max_yi - min_yi + 1, img->depth, img->nChannels); );

						if (cut) 
						{
							// set it as black 
							ATOMIC_RW(opencv, cvZero(cut); );

							// calculate what might eventually become texturing coordinates 
							request->gl_texture_min_x = 0;
							request->gl_texture_min_y = 0; 
							request->gl_texture_max_x = (max_xi - min_xi + 1) / (double)(cut->width); 
							request->gl_texture_max_y = (max_yi - min_yi + 1) / (double)(cut->height);

							// trim the region to fit the image
							if (min_xi < 0) min_xi = 0;
							if (min_yi < 0) min_yi = 0;
							if (max_xi >= img->width) max_xi = img->width - 1;
							if (max_yi >= img->height) max_yi = img->height - 1;

							// copy the region 
							for (int y = min_yi; y <= max_yi; y++)
							{
								for (int x = min_xi; x <= max_xi; x++)
								{
									((uchar*)(cut->imageData + cut->widthStep * (y - min_yi)))[(x - min_xi) * 3 + 0] = ((uchar*)(img->imageData + img->widthStep * y))[x * 3 + 0];
									((uchar*)(cut->imageData + cut->widthStep * (y - min_yi)))[(x - min_xi) * 3 + 1] = ((uchar*)(img->imageData + img->widthStep * y))[x * 3 + 1];
									((uchar*)(cut->imageData + cut->widthStep * (y - min_yi)))[(x - min_xi) * 3 + 2] = ((uchar*)(img->imageData + img->widthStep * y))[x * 3 + 2];
								}
							}
						}
					}

					// save the result
					if (cut)
					{
						switch (request->quality)
						{
							case IMAGE_LOADER_LOW_RESOLUTION:
							{
								if (achieved_quality == IMAGE_LOADER_LOW_RESOLUTION)
								{
									ASSERT(!request->image, "unprocessed request has allocated memory for image");
									request->image = cut;
									request->current_quality = IMAGE_LOADER_LOW_RESOLUTION;
									request->done = true;
									shot->low_counter--; // here, we're decrementing the total amount of requests, because we don't need the original image anymore
									shot->low_unprocessed_counter--;
									image_loader_unprocessed_counter--;
								}
								else 
								{
									ASSERT(false, "inconsistent state variable");
								}
								break; 
							}

							case IMAGE_LOADER_FULL_RESOLUTION: 
							{
								if (achieved_quality == IMAGE_LOADER_FULL_RESOLUTION)
								{
									ASSERT(!request->image, "unprocessed request has allocated memory for image");
									request->image = cut;
									request->current_quality = IMAGE_LOADER_FULL_RESOLUTION;
									request->done = true;
									shot->full_counter--;
									shot->full_unprocessed_counter--;
									image_loader_unprocessed_counter--;
								}
								else
								{
									ASSERT(false, "inconsistent state variable");
								}
								break;
							}

							case IMAGE_LOADER_CONTINUOUS_LOADING:
							{
								if (achieved_quality >= IMAGE_LOADER_LOW_RESOLUTION)
								{
									if (request->image)
									{
										ASSERT(achieved_quality == IMAGE_LOADER_FULL_RESOLUTION, "inconsistent state variable");
										ASSERT(request->current_quality == IMAGE_LOADER_LOW_RESOLUTION, "inconsistent state variable");
										LOCK_RW(opencv)
										{
											cvReleaseImage(&request->image);
										}
										UNLOCK_RW(opencv); 
									}

									if (request->current_quality < IMAGE_LOADER_LOW_RESOLUTION)
									{
										shot->low_counter--;
										shot->low_unprocessed_counter--;
									}

									if (achieved_quality == IMAGE_LOADER_FULL_RESOLUTION) 
									{
										shot->full_counter--; 
										shot->full_unprocessed_counter--; 
										image_loader_unprocessed_counter--;
										request->done = true;
									}

									request->image = cut;
									request->current_quality = achieved_quality;
								}
								else
								{
									ASSERT(false, "achieved_quality must be at least low resolution");
								}
								break; 
							}
						}
					}

					// we're done, if the request was resolved (at least partially), it was marked as such 
					// and appropriate counters were decremented
				}
			}

			break; 
		}
		case IMAGE_LOADER_ALL:
		{
			switch (request->quality) 
			{
				case IMAGE_LOADER_LOW_RESOLUTION: 
				{
					if (shot->low)
					{
						request->image = shot->low; 
						request->current_quality = IMAGE_LOADER_LOW_RESOLUTION;
						request->done = true;
						shot->low_unprocessed_counter--;
						image_loader_unprocessed_counter--;
					}
					break; 
				}

				case IMAGE_LOADER_FULL_RESOLUTION: 
				{
					if (shot->full) 
					{
						request->image = shot->full; 
						request->current_quality = IMAGE_LOADER_FULL_RESOLUTION;
						request->done = true;
						shot->full_unprocessed_counter--;
						image_loader_unprocessed_counter--;
					}
					break;
				}

				case IMAGE_LOADER_CONTINUOUS_LOADING:
				{
					if (shot->low && request->current_quality < IMAGE_LOADER_LOW_RESOLUTION)
					{
						request->image = shot->low; 
						request->current_quality = IMAGE_LOADER_LOW_RESOLUTION;
						shot->low_unprocessed_counter--;
					}
					if (shot->full) 
					{
						request->image = shot->full; 
						request->current_quality = IMAGE_LOADER_FULL_RESOLUTION;
						request->done = true;
						shot->full_unprocessed_counter--;
						image_loader_unprocessed_counter--;
					}
					break; 
				}
			}

			break; 
		}
	}
}

// thread function
// obtains LOCK_RW(image_loader), LOCK_RW(opencv)
void * image_loader_thread_function(void * arg)
{
	while (true) 
	{
		LOCK_RW(image_loader)
		{
			ASSERT(
				image_loader_unprocessed_counter <= image_loader_requests.count - image_loader_free_ids_counter, 
				"number of unprocessed requests seems to be higher than the number of non-empty request slots"
			);

			// terminate 
			if (image_loader_terminate) 
			{
				UNLOCK_RW(image_loader);
				break;
			}

			// check if there are requests to process
			if (image_loader_unprocessed_counter == 0) 
			{
				// probably nothing...
				// SDL_Delay(100);
			}
			else
			{
				// * we have work to do *

				// try to resolve requests which can be resolved immediately 
				for ALL(image_loader_requests, i) 
				{
					Image_Loader_Request * const request = image_loader_requests.data + i;

					if (!request->done) 
					{
						image_loader_resolve_request(i);
					}
				}
				
				// find optimal shot to process
				size_t best_shot = SIZE_MAX; 
				bool best_found = false; 
				size_t best_count = 0;

				for ALL(image_loader_shots, i) 
				{
					Image_Loader_Shot * const shot = image_loader_shots.data + i; 
					// printf("%d ", i); 

					if (shot->full_unprocessed_counter + shot->low_unprocessed_counter >= best_count || !best_found)
					{
						best_found = true;
						best_shot = i;
						best_count = shot->full_unprocessed_counter + shot->low_unprocessed_counter;
					}

					ASSERT(best_count <= 2 * image_loader_requests.count, "number of unprocessed requests for one shot is bigger than the number of all requests");
					// note that we're multiplying by 2 because continuous loading request adds one to both full and low counters
					// (thus, this is not necessarily tight bound)
				}

				ASSERT(best_found, "no shot with pending requests found even though there are unprocessed requests");
				Image_Loader_Shot * shot = image_loader_shots.data + best_shot;

				// full resolution version is requested and currently not in memory
				if (shot->full_unprocessed_counter > 0 && !shot->full) 
				{
					// if necessary, free memory by releasing one of unused images
					image_loader_free_full();

					// get image parameters and unlock
					const char * const filename = shot->filename;
					bool calculate_low = !shot->low && image_loader_low_counter < image_loader_cache_low_count;

					int loaded_width, loaded_height; 
					IplImage * full, * low; 

					UNLOCK_RW(image_loader)
					{
						// load image
						LOCK_RW(opencv)
						{
							full = cvLoadImage(filename); 
						
							if (!full) 
							{
								full = opencv_create_substitute_image();
							}
							loaded_width = full->width;
						       	loaded_height = full->height;
							IplImage * resize = cvCreateImage(cvSize(IMAGE_LOADER_FULL_SIZE, IMAGE_LOADER_FULL_SIZE), full->depth, full->nChannels);
							cvResize(full, resize); 
							cvReleaseImage(&full);
							full = resize;
						
							// we might want to compute low version 
							low = NULL;
							if (calculate_low) 
							{
								low = cvCreateImage(cvSize(IMAGE_LOADER_LOW_SIZE, IMAGE_LOADER_LOW_SIZE), full->depth, full->nChannels);
								cvResize(full, low);
							}
						}
						UNLOCK_RW(opencv);
					}
					LOCK_RW(image_loader);

					// lock again and save the data
					image_loader_full_counter++;
					shot->full = full;
					shot->width = loaded_width;
					shot->height = loaded_height;
					if (calculate_low)
					{
						image_loader_low_counter++;
						shot->low = low;
					}
				}

				// low resolution version is requested and currently not in memory
				if (shot->low_unprocessed_counter > 0 && !shot->low)
				{
					// if necessary, free memory by releasing one of unused images
					// locks opencv
					image_loader_free_low();

					// get image parameters 
					const char * const filename = shot->filename;

					IplImage * low; 
					int loaded_width, loaded_height; 

					// load image
					UNLOCK_RW(image_loader)
					{
						LOCK_RW(opencv)
						{
							// opencv_begin();
							low = cvLoadImage(filename);
							if (!low)
							{
								low = opencv_create_substitute_image();
							}
							loaded_width = low->width; 
							loaded_height = low->height;
							IplImage * resize = cvCreateImage(cvSize(IMAGE_LOADER_LOW_SIZE, IMAGE_LOADER_LOW_SIZE), low->depth, low->nChannels);
							cvResize(low, resize);
							cvReleaseImage(&low);
							low = resize;
						}
						UNLOCK_RW(opencv)
					}
					LOCK_RW(image_loader)

					// lock again and save info about the low-res version
					image_loader_low_counter++;
					shot->low = low;
					shot->width = loaded_width;
					shot->height = loaded_height;
				}

				// process requests 
				for ALL(image_loader_requests, i)
				{
					Image_Loader_Request * const request = image_loader_requests.data + i; 

					// if the request is for shot we've just loaded, resolve the request
					if (
						request->shot_id == best_shot 
						&& 
						!(request->quality == request->current_quality || request->quality == IMAGE_LOADER_CONTINUOUS_LOADING && request->current_quality == IMAGE_LOADER_FULL_RESOLUTION)
					)
					{
						image_loader_resolve_request(i);
					}
				}
			}
		}
		UNLOCK_RW(image_loader);
	}

	return NULL;
}

// initialize image loader subsystem 
bool image_loader_initialize(const int cache_full_count, const int cache_low_count) 
{
	image_loader_terminate = false;
	image_loader_time = 1; 
	image_loader_cache_full_count = cache_full_count; 
	image_loader_cache_low_count = cache_low_count;
	image_loader_full_counter = 0; 
	image_loader_low_counter = 0;
	image_loader_free_ids_counter = 0;
	image_loader_unprocessed_counter = 0;
	DYN_INIT(image_loader_shots); 
	DYN_INIT(image_loader_requests);

	/*sched_param param; 
	int priority, policy, ret; 
	ret = pthread_getschedparam(image_loader_thread, &policy, &param);
	param.sched_priority = 0;
	pthread_setschedparam(image_loader_thread, policy, &param);*/

	return true;
}

// initialize image loader thread 
bool image_loader_start_thread()
{
	if (pthread_create(&image_loader_thread, NULL, image_loader_thread_function, NULL)) 
	{
		core_state.error = CORE_ERROR_UNABLE_TO_CREATE_THREAD;
		return false;
	}

	return true; 
}

// release image loader subsystem 
// todo release also shots and requests 
// obtains LOCK_RW(image_loader)
void image_loader_release()
{
	ATOMIC_RW(image_loader, image_loader_terminate = true; );

	if (pthread_join(image_loader_thread, NULL))
	{
		printf("Error joining thread\n");
		abort(); 
	}
}

// creates new request to load shot image 
// obtains LOCK_RW(image_loader)
Image_Loader_Request_Handle image_loader_new_request(
	const size_t shot_id,
	const char * const filename,
	const Image_Loader_Quality quality, 
	const Image_Loader_Content content /*= IMAGE_LOADER_ALL*/, 
	const double x /*= -1*/, 
	const double y /*= -1*/, 
	const double sx /*= -1*/,
	const double sy /*= -1*/,
	const bool fake /*= false*/ // note unused!
)
{
	Image_Loader_Request_Handle handle;
		
	LOCK_RW(image_loader)
	{
		ASSERT(image_loader_requests.count < IMAGE_LOADER_MAX_REQUESTS, "maximum number of requests reached");

		// create request structure
		size_t id;

		if (image_loader_free_ids_counter > 0)
		{
			id = image_loader_free_ids[--image_loader_free_ids_counter]; 
			DYN(image_loader_requests, id);
		}
		else
		{
			ADD(image_loader_requests);
			id = LAST_INDEX(image_loader_requests);
		}

		// create handle
		Image_Loader_Request * const request = image_loader_requests.data + id;
		handle.id = id;
		handle.time = image_loader_time++;
		image_loader_unprocessed_counter++;

		// fill in the data
		request->shot_id = shot_id; 
		request->quality = quality; 
		request->current_quality = IMAGE_LOADER_NOT_LOADED;
		request->content = content; 
		request->x = x; 
		request->y = y; 
		request->sx = sx; 
		request->sy = sy; 
		request->done = false;

		// increase the number of active requests for this shot
		DYN(image_loader_shots, shot_id);
		Image_Loader_Shot * const shot = image_loader_shots.data + shot_id;

		shot->filename = filename;

		switch (quality) 
		{
			case IMAGE_LOADER_LOW_RESOLUTION: 
			{
				shot->low_unprocessed_counter++; 
				shot->low_counter++; 
				break; 
			}

			case IMAGE_LOADER_FULL_RESOLUTION: 
			{
				shot->full_unprocessed_counter++; 
				shot->full_counter++; 
				break; 
			}

			case IMAGE_LOADER_CONTINUOUS_LOADING:
			{
				shot->low_counter++; 
				shot->low_unprocessed_counter++; 
				shot->full_counter++; 
				shot->full_unprocessed_counter++; 
				break; 
			}
		}

		// try to resolve this request immediately (if it looks like it's important enough) 
		if (request->content == IMAGE_LOADER_ALL)
		{
			image_loader_resolve_request(handle.id);
		}
	}
	UNLOCK_RW(image_loader);

	return handle;
}

// check if the handle is nonempty 
bool image_loader_nonempty_handle(Image_Loader_Request_Handle handle)
{
	return (handle.time > 0);
}

// cancel existing request 
// obtains LOCK_RW(image_loader), LOCK_RW(opencv), LOCK_RW(opengl)
void image_loader_cancel_request(Image_Loader_Request_Handle * handle) 
{
	LOCK_RW(image_loader)
	{
		ASSERT(image_loader_nonempty_handle(*handle), "trying to release empty request handle");
		ASSERT_IS_SET(image_loader_requests, handle->id);
		Image_Loader_Request * const request = image_loader_requests.data + handle->id;
		const size_t shot_id = request->shot_id;
		ASSERT_IS_SET(image_loader_shots, shot_id); 
		Image_Loader_Shot * const shot = image_loader_shots.data + shot_id;

		// if this request was for the whole image 
		if (request->content == IMAGE_LOADER_ALL)
		{
			// decrease the counter of unprocessed requests 
			if (!image_loader_requests.data[handle->id].done)
			{
				ASSERT(image_loader_unprocessed_counter > 0, "number of unprocessed requests is not positive even though we've found at least one");
				image_loader_unprocessed_counter--;

				switch (request->quality) 
				{
					case IMAGE_LOADER_LOW_RESOLUTION:
						ASSERT(shot->low_unprocessed_counter > 0, "number of unprocessed requests for low version is zero although one unfinished is being cancelled");
						shot->low_unprocessed_counter--; break; 
					case IMAGE_LOADER_FULL_RESOLUTION: 
						ASSERT(shot->full_unprocessed_counter > 0, "number of unprocessed requests for full version is zero although one unfinished is being cancelled");
						shot->full_unprocessed_counter--; break;
					case IMAGE_LOADER_CONTINUOUS_LOADING: 
						ASSERT(shot->full_unprocessed_counter > 0, "number of unprocessed requests for full version is zero although one unfinished is being cancelled");
						shot->full_unprocessed_counter--;
						if (request->current_quality < IMAGE_LOADER_LOW_RESOLUTION)
						{
							ASSERT(shot->low_unprocessed_counter > 0, "number of unprocessed requests for low version is zero although one unfinished is being cancelled");
							shot->low_unprocessed_counter--; 
						}
						break; 
				}
			}

			// either way, decrease the number of requests 
			switch (request->quality) 
			{
				case IMAGE_LOADER_LOW_RESOLUTION: shot->low_counter--; break; 
				case IMAGE_LOADER_FULL_RESOLUTION: shot->full_counter--; break;
				case IMAGE_LOADER_CONTINUOUS_LOADING: shot->low_counter--; shot->full_counter--; break; 
			}
		}
		// if the request was only for smaller region 
		else
		{
			// decrease the counter of unprocessed requests 
			// strong todo (priority) add free
			if (!request->done) 
			{
				// if the request wasn't fullfilled, simply decrement counters
				ASSERT(image_loader_unprocessed_counter > 0, "number of unprocessed requests is not positive even though we've found at least one");
				image_loader_unprocessed_counter--; 

				switch (request->quality) 
				{
					case IMAGE_LOADER_LOW_RESOLUTION:
						ASSERT(shot->low_unprocessed_counter > 0, "number of unprocessed requests for low version is zero although one unfinished is being cancelled");
						ASSERT(shot->low_counter > 0, "number of requests for low version is zero although one unfinished is being cancelled");
						shot->low_unprocessed_counter--;
						shot->low_counter--;
						break; 
					case IMAGE_LOADER_FULL_RESOLUTION: 
						ASSERT(shot->full_unprocessed_counter > 0, "number of unprocessed requests for full version is zero although one unfinished is being cancelled");
						ASSERT(shot->full_counter > 0, "number of requests for full version is zero although one unfinished is being cancelled");
						shot->full_unprocessed_counter--;
						shot->full_counter--; 
						break;
					case IMAGE_LOADER_CONTINUOUS_LOADING: 
						ASSERT(shot->full_unprocessed_counter > 0, "number of unprocessed requests for full version is zero although one unfinished is being cancelled");
						ASSERT(shot->full_counter > 0, "number of requests for full version is zero although one unfinished is being cancelled");
						shot->full_unprocessed_counter--;
						shot->full_counter--;
						if (request->current_quality < IMAGE_LOADER_LOW_RESOLUTION)
						{
							ASSERT(shot->low_unprocessed_counter > 0, "number of unprocessed requests for low version is zero although one unfinished is being cancelled");
							ASSERT(shot->low_counter > 0, "number of requests for low version is zero although one unfinished is being cancelled");
							shot->low_unprocessed_counter--; 
							shot->low_counter--; 
						}
						break; 
				}
			}

			// we're not decreasing the number of requests here, because requests for parts
			// of image do not depend on the whole image - a copy is made instead

			// we'll also immediately delete textures from memory
			if (request->image) 
			{
				LOCK_RW(opencv)
				{
					cvReleaseImage(&request->image); 
				}
				UNLOCK_RW(opencv);

				request->image = NULL; 
			}

			if (request->gl_texture_id) 
			{
				LOCK_RW(opengl)
				{
					glDeleteTextures(1, &request->gl_texture_id);
				}
				UNLOCK_RW(opengl);
			}
		}

		// if there is no request for this image any more, release it's OpenGL texture 
		if (shot->full_counter == 0) 
		{
			if (shot->full_texture) 
			{
				LOCK_RW(opengl)
				{
					glDeleteTextures(1, &shot->full_texture); 
				}
				UNLOCK_RW(opengl);
			}
			shot->full_texture = 0; 
		}

		if (shot->low_counter == 0) 
		{
			if (shot->low_texture)
			{
				LOCK_RW(opengl)
				{
					glDeleteTextures(1, &shot->low_texture);
				}
				UNLOCK_RW(opengl);
			}
			shot->low_texture = 0;
		}

		// delete the request 
		request->set = false;
		image_loader_free_ids[image_loader_free_ids_counter++] = handle->id;
	
		// mark handle as empty 
		handle->time = 0; 
		handle->id = SIZE_MAX;
	}
	UNLOCK_RW(image_loader);
}

// determines if requested image is loaded into memory (at least low res version if we're ok with continuous loading)
// expects LOCK_RW(image_loader)
static bool image_loader_request_ready_nolock(Image_Loader_Request_Handle handle) 
{
	ASSERT_IS_SET(image_loader_requests, handle.id);
	return
		image_loader_requests.data[handle.id].done 
		|| 
		(
			image_loader_requests.data[handle.id].quality == IMAGE_LOADER_CONTINUOUS_LOADING 
			&& 
			image_loader_requests.data[handle.id].current_quality >= IMAGE_LOADER_LOW_RESOLUTION
		);
}

// version of the above function for use outside of this lbrary 
// obtains LOCK_RW(image_loader)
bool image_loader_request_ready(Image_Loader_Request_Handle handle) 
{
	ATOMIC_RW(image_loader, const bool ready = image_loader_request_ready_nolock(handle); );

	return ready;
}

// determines if the requested image is already waiting for us on gpu (checks for both low and full version of the texture)
// obtains LOCK_RW(image_loader)
bool image_loader_opengl_upload_ready_dual(
	Image_Loader_Request_Handle handle, GLuint * full_texture, GLuint * low_texture,
	double * texture_min_x /*= NULL*/, double * texture_min_y /*= NULL*/, double * texture_max_x /*= NULL*/, double * texture_max_y /*= NULL*/
)
{
	LOCK_RW(image_loader)
	{
		ASSERT_IS_SET(image_loader_requests, handle.id);
		Image_Loader_Request * request = image_loader_requests.data + handle.id; 
		
		*low_texture = 0;
		*full_texture = 0; 

		if (request->content == IMAGE_LOADER_ALL) 
		{
			// * if we want the whole region *

			ASSERT_IS_SET(image_loader_shots, request->shot_id); 
			Image_Loader_Shot * const shot = image_loader_shots.data + request->shot_id;
			
			if (texture_min_x) *texture_min_x = 0;
			if (texture_min_y) *texture_min_y = 0; 
			if (texture_max_x) *texture_max_x = 1; 
			if (texture_max_y) *texture_max_y = 1;

			bool result = false;		
			switch (request->quality)
			{
				case IMAGE_LOADER_LOW_RESOLUTION: 
					if (shot->low_texture != 0) { *low_texture = shot->low_texture; result = true; } 
					break; 
				case IMAGE_LOADER_CONTINUOUS_LOADING: 
					if (shot->low_texture != 0) { *low_texture = shot->low_texture; result = true; } // note absence of break
				case IMAGE_LOADER_FULL_RESOLUTION: 
					if (shot->full_texture != 0) { *full_texture = shot->full_texture; result = true; } 
					break;
			}

			UNLOCK_RW(image_loader);
			return result; 
		}
		else
		{
			// * if it's only a small region, return it's individual texture *

			if (texture_min_x) *texture_min_x = request->gl_texture_min_x;
			if (texture_min_y) *texture_min_y = request->gl_texture_min_y;
			if (texture_max_x) *texture_max_x = request->gl_texture_max_x;
			if (texture_max_y) *texture_max_y = request->gl_texture_max_y;

			if (request->gl_texture_id) 
			{
				*full_texture = request->gl_texture_id;
				*low_texture = 0;
			}
			
			UNLOCK_RW(image_loader);
			return request->gl_texture_id != 0;
		}
	}
	WAS_UNLOCKED_RW(image_loader);
}

// determined if the requested image is already on gpu
bool image_loader_opengl_upload_ready(
	Image_Loader_Request_Handle handle, GLuint * texture, 
	double * texture_min_x /*= NULL*/, double * texture_min_y /*= NULL*/, double * texture_max_x /*= NULL*/, double * texture_max_y /*= NULL*/
)
{
	GLuint low_texture, full_texture; 
	if (image_loader_opengl_upload_ready_dual(handle, &full_texture, &low_texture, texture_min_x, texture_min_y, texture_max_x, texture_max_y))
	{
		if (full_texture != 0)
		{
			*texture = full_texture; 
			return true; 
		}
		else if (low_texture != 0)
		{
			*texture = low_texture; 
			return true;
		}

		return false;
	}
	else
	{
		return false; 
	}
}

// uploads texture to opengl
// note if there are more requests for one image, it's cause multiple uploads to opengl
// obtains LOCK_RW(image_loader), LOCK_RW(opengl)
void image_loader_upload_to_opengl(Image_Loader_Request_Handle handle) 
{
	LOCK_RW(image_loader)
	{
		// error c:\_\projects\insight3d\insight3d\core_image_loader.cpp, 1031: accessing undefin
		// ed index -572662307 in dynamic array
		// [happened when terminating the application by clicking the [x] button]
		ASSERT_IS_SET(image_loader_requests, handle.id);
		Image_Loader_Request * const request = image_loader_requests.data + handle.id;
		ASSERT_IS_SET(image_loader_shots, request->shot_id);
		Image_Loader_Shot * const shot = image_loader_shots.data + request->shot_id;

		// check if this request is ready (at least partially)
		if (image_loader_request_ready_nolock(handle))
		{
			// which type of request this is? 
			if (request->content == IMAGE_LOADER_ALL) 
			{
				// * it's new entire image * 
				
				// check if there's actually something new to upload
				if (!shot->full_texture && shot->full) 
				{
					// upload full texture
					LOCK_RW(opengl)
					{
						glGenTextures(1, &shot->full_texture);
						glBindTexture(GL_TEXTURE_2D, shot->full_texture);
						glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shot->full->width, shot->full->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, shot->full->imageData);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
					UNLOCK_RW(opengl);
				}

				if (!shot->low_texture && shot->low) 
				{
					// upload low texture 
					LOCK_RW(opengl)
					{
						glGenTextures(1, &shot->low_texture);
						glBindTexture(GL_TEXTURE_2D, shot->low_texture);
						glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shot->low->width, shot->low->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, shot->low->imageData);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
					UNLOCK_RW(opengl);
				}
			}
			else
			{
				// * it's just a part of an image *

				// if we loaded better version of this texture 
				if (request->gl_texture_id && request->current_quality > request->gl_texture_quality)
				{
					// delete the texture 
					ATOMIC_RW(opengl, glDeleteTextures(1, &request->gl_texture_id); );
					request->gl_texture_id = 0;
				}

				ASSERT(request->image, "image not ready although request is done");

				if (!request->gl_texture_id)
				{
					// upload the texture
					LOCK_RW(opengl)
					{
						glGenTextures(1, &request->gl_texture_id);
						glBindTexture(GL_TEXTURE_2D, request->gl_texture_id);
						glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, request->image->width, request->image->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, request->image->imageData);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
					UNLOCK_RW(opengl);
				}
			}
		}
	}
	UNLOCK_RW(image_loader);
}

// get original dimensions of this request's image 
// obtains LOCK_RW(image_loader)
void image_loader_get_original_dimensions(Image_Loader_Request_Handle handle, int * width, int * height) 
{
	LOCK_R(image_loader)
	{
		ASSERT(image_loader_request_ready_nolock(handle), "trying to get dimensions of image of a request that's not ready");

		const size_t shot_id = image_loader_requests.data[handle.id].shot_id; 
		ASSERT_IS_SET(image_loader_shots, shot_id); 

		*width  = image_loader_shots.data[shot_id].width; 
		*height = image_loader_shots.data[shot_id].height;
	}
	UNLOCK_R(image_loader);
}

// flush texture ids
// obtains LOCK_RW(image_loader)
void image_loader_flush_texture_ids() 
{
	LOCK_RW(image_loader)
	{
		// go through all requests 
		for ALL(image_loader_requests, i)
		{
			Image_Loader_Request * const request = image_loader_requests.data + i; 

			request->gl_texture_quality = IMAGE_LOADER_NOT_LOADED; 
			request->gl_texture_id = 0;
		}

		// and through all shots 
		for ALL(image_loader_shots, i) 
		{
			Image_Loader_Shot * const shot = image_loader_shots.data + i; 

			shot->full_texture = 0; 
			shot->low_texture = 0;
		}
	}
	UNLOCK_RW(image_loader);
}

// todo set suggested flag

// clear all suggested flags 
// obtains LOCK_RW(image_loader)
void image_loaded_flush_suggested() 
{
	LOCK_RW(image_loader)
	{
		// go through all shots
		for ALL(image_loader_shots, i) 
		{
			Image_Loader_Shot * const shot = image_loader_shots.data + i; 
			shot->suggested = false; 
		}
	}
	UNLOCK_RW(image_loader);
}

// cancel all requests 
// obtains LOCK_RW(image_loader), LOCK_RW(opencv)
void image_loader_cancel_all_requests() 
{
	LOCK_RW(image_loader)
	{
		for ALL(image_loader_requests, i) 
		{
			Image_Loader_Request_Handle handle; 
			handle.id = i; 
			handle.time = 1;

			UNLOCK_RW(image_loader)
			{
				image_loader_cancel_request(&handle);
			}
			LOCK_RW(image_loader);
		}

		for ALL(image_loader_shots, i)
		{
			Image_Loader_Shot * const shot = image_loader_shots.data + i; 

			LOCK_RW(opencv)
			{
				if (shot->full) cvReleaseImage(&shot->full);
				if (shot->low) cvReleaseImage(&shot->low);
			}
			UNLOCK_RW(opencv);
		}

		image_loader_full_counter = 0;
		image_loader_low_counter = 0;

		DYN_FREE(image_loader_shots);
	}
	UNLOCK_RW(image_loader);
}
