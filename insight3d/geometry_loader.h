#ifndef __GEOMETRY_LOADER
#define __GEOMETRY_LOADER

#include "interface_filesystem.h"
#include "core_math_routines.h"
#include "geometry_structures.h"
#include "geometry_routines.h"
#include <fstream>
#include <string>
// #include "libxml/parser.h"

// SAX state
struct geometry_loader_SAX_state
{
	// loaded file's filename and directory
	char * filename, * directory;

	// photos and camera data 
	Shots * shots;

	// pointer to currently loaded shot
	Shot * current_shot;

	// first cammera's film back
	double first_film_back;

	// number of points currently loaded from the file 
	size_t points_count; 

	// points data  
	Vertices * vertices; 
};

// process loaded data (compute focal length from fov, assemble projection matrices, ...)
void geometry_process_data(Shots shots);

// SAX callbacks
/*void geometry_loader_SAX_start_element(geometry_loader_SAX_state * state, const xmlChar * name, const xmlChar ** attrs);
void geometry_loader_SAX_end_element(geometry_loader_SAX_state * state, const xmlChar * name);
void geometry_loader_SAX_characters(geometry_loader_SAX_state * state, const xmlChar * cdata, int len);*/

// load saved project
bool geometry_load_project(const char * filename);

// load data from realviz xml file
/* void geometry_loader(const char * xml_filename, Shots & shots); */

// load data from realviz rz3 file
bool geometry_loader_rz3(const char * xml_filename, Shots & shots);

// load 3d vertices from text file (one vertex per line: <id> <x> <y> <z>); returns true on success
bool geometry_loader_vertices(const char * txt_filename, Vertices & vertices, size_t group = 0);

// load contours 
bool geometry_loader_contours(const char * txt_filename, Shots & shots);

// load points from text files
bool geometry_loader_points(const char * pictures_filename, const char * tracks_filename, Shots & shots, Vertices & vertices, size_t group = 0);

// add another image to the sequence 
bool geometry_loader_add_shot(const char * filename);

// load IFL file (i.e., image file list) 
bool geometry_loader_ifl(const char * filename);

// load points from *_points.txt and *_pictures.ifl imagepair
bool geometry_loader_points_guess_filepair(char * const filename);

#endif
