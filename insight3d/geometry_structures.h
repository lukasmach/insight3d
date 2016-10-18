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

#ifndef __GEOMETRY_STRUCTURES
#define __GEOMETRY_STRUCTURES

#include "core_structures.h"
#include "core_image_loader.h"
#include <map>

// Rob Hess's SIFT library 
extern "C" { 
#ifndef _MSC_VER
#include "./sift/include/sift.h"
#include "./sift/include/imgfeatures.h"
#include "./sift/include/kdtree.h"
#include "./sift/include/utils.h"
#include "./sift/include/xform.h"
#else
#include "./sift_win/sift.h"
#include "./sift_win/imgfeatures.h"
#include "./sift_win/kdtree.h"
#include "./sift_win/utils.h"
#include "./sift_win/xform.h"
#endif
}

// * threading support *

extern pthread_mutex_t geometry_mutex;

// * meta information about geometric objects *

enum GEOMETRY_ITEM_TYPE { GEOMETRY_UNSPECIFIED, GEOMETRY_POINT, GEOMETRY_VERTEX, GEOMETRY_SHOT };
enum GEOMETRY_RELATION { GEOMETRY_CORRESPONDENCE, GEOMETRY_INCIDENCE };
enum GEOMETRY_DATA_ORIGIN { GEOMETRY_MANUAL_INPUT, GEOMETRY_GENERATED = -1, GEOMETRY_NOT_CREDIBLE = -2, GEOMETRY_NOT_CHANGED = -3, GEOMETRY_MANUALLY_MARKED_AS_INVISIBLE = -4 }; // todo probably change this to something better
enum GEOMETRY_VERTEX_TYPE { GEOMETRY_VERTEX_USER, GEOMETRY_VERTEX_AUTO, GEOMETRY_VERTEX_EQUIVALENCE };

// how certain are we about metainformation about image files (width and height in pixels)
enum GEOMETRY_IMAGE_INFO_STATUS 
{ 
	GEOMETRY_INFO_NOT_LOADED,    // info is unknown 
	GEOMETRY_INFO_DEDUCED,       // this file specifically wasn't loaded, but image dimensions have been taken from a different source (another image from the same sequence, .xml file, ...)
	GEOMETRY_INFO_LOADED         // file was loaded and reliable dimensions save were stored
}; 

// * data structures * 

// dynamic set of values // todo probably rename to Value
struct Index { 
	bool set; 
	size_t value; 
}; 

DYNAMIC_STRUCTURE_DECLARATIONS(Indices, Index);

// dynamic array of double indices 
struct Double_Index { 
	bool set; 
	size_t primary, secondary; 
};

DYNAMIC_STRUCTURE_DECLARATIONS(Double_Indices, Double_Index);

// selection of different items 
struct Selected_Item { 
	bool set;
	GEOMETRY_ITEM_TYPE item_type;    // type of selected item (vertex, point, polygon, ...)
	size_t shot_id;                  // id of shot on which selected item is (if applicable) 
	size_t item_id;                  // item's id
}; 

DYNAMIC_STRUCTURE_DECLARATIONS(Selected_Items, Selected_Item)

// point
struct Point {
	bool set; 
	GEOMETRY_RELATION relation;          // type of correspondence
	GEOMETRY_DATA_ORIGIN data_origin;    // where does this point come from (automatically generated, user submitted, ...)
	double x, y;                         // point coordinates, x and y are numbers from interval [0, 1]
	size_t vertex;                       // 3d vertex id
	bool selected;                       // denotes if this point is selected
};

// vertex 
struct Vertex {
	bool set, reconstructed;
	double x, y, z;              // space coordinates of vertex
	double tex_x, tex_y;         // texture coordinates
	double nx, ny, nz;           // vertex normal
	float color[3];              // vertex can be colored by user or by any algorithm to help distinguish between different groups of vertices
	size_t group;                // vertex can be a part of group
	GEOMETRY_VERTEX_TYPE vertex_type; // vertex type
	bool selected;               // denotes if this vertex is selected // todo move int UI item meta structure
};

// stores from which shot each vertex is visible (and it's point id on this shot)
struct Vertex_Incidence { 
	bool set; 
	Double_Indices shot_point_ids;
}; 

// 3d polygon
struct Polygon_3d {
	bool set;
	float color[3];
	Indices vertices;            // vertices
	Image_Loader_Request_Handle image_loader_request;
	double * texture_coords;
	bool selected;               // denotes if this polygon is selected
};

// 2d polygon
struct Contour {
	bool set; 
	float color[3];   
	Indices vertices;            // vertices
	bool selected;               // denotes if this contour is selected
};

DYNAMIC_STRUCTURE_DECLARATIONS(Points, Point); 
DYNAMIC_STRUCTURE_DECLARATIONS(Vertices, Vertex);
DYNAMIC_STRUCTURE_DECLARATIONS(Vertices_Incidence, Vertex_Incidence);
DYNAMIC_STRUCTURE_DECLARATIONS(Polygons_3d, Polygon_3d);
DYNAMIC_STRUCTURE_DECLARATIONS(Contours, Contour);

// photograph metainformation
struct Shot {

	bool set;

	// camera data
	double T[4];           // camera translation in homogeneous coordinates
	double R_euler[3];     // camera rotation (euler angles in radians)
	double f;              // focal length 
	double fovx, fovy;     // field of view (x axis and y axis in degrees)
	double film_back;      // CCD size in mm
	double pp_x;           // principal point x coordinate (percentage of image size) 
	double pp_y;           // principal point y coordinate ...
	bool calibrated;       // denotes whether valid calibration data have been determined or supplied
	bool partial_calibration; // current partial calibration has estimate of this camera's calibration matrix 
	bool resected;         // specific
	
	// meta-data
	char * name;           // user-assigned name
	char * image_filename; // image's filename, all changes should be also sent to image_loader

	// image info
	int width, height;     // size of image plane in pixels 
	GEOMETRY_IMAGE_INFO_STATUS info_status; // how reliably do we know the information about width and height of the image 

	// image loader 
	Image_Loader_Request_Handle image_loader_request; 

	// extracted keypoints
	feature * keypoints;   // SIFT keypoints
	int keypoints_count;
	kd_node * kd_tree;     // root node of kd-tree containing nodes
	void * matching;       // additional info for matching tool

	// visualization values
	double visualization_T[3]; // used for storing recomputed inhomogeneous coordinates in visualization routines 
	double 
		visualization_pyr_00[3], 
		visualization_pyr_10[3], 
		visualization_pyr_11[3], 
		visualization_pyr_01[3]		
	;

	// ui specific data 
	void * ui;

	// data on this shot (this is what it's all about)
	Points points;         // 2d points 
	Contours contours;     // 2d polygons on this shot // unused but nice to have for some vision algorithms

	// matrices used for geometric computation
	CvMat * projection, * rotation, * translation, * internal_calibration;
};

// dynamic array of photographs
DYNAMIC_STRUCTURE_DECLARATIONS(Shots, Shot);

// shot pair relation describes relations between pair of shots 
// (like number of correspondences between the pair)
struct Shot_Pair_Relation 
{
	bool set; 
	size_t correspondences_count;    // number of correspondences
};

DYNAMIC_STRUCTURE_DECLARATIONS(Shot_Pair_Relations, Shot_Pair_Relation); 

struct Shot_Relations
{
	bool set; 
	Shot_Pair_Relations pair_relations;
}; 

DYNAMIC_STRUCTURE_DECLARATIONS(Shots_Relations, Shot_Relations);

// * calibrations *

struct Calibration_Point_Meta 
{
	bool set;

	signed char inlier; // determines if this point is inlier (= 1) or outlier (= 0),
	                    // other values than 0 and 1 are reserved for future use
};

DYNAMIC_STRUCTURE_DECLARATIONS(Calibration_Points_Meta, Calibration_Point_Meta);

struct Calibration_Fundamental_Matrix 
{
	bool set; 

	size_t first_shot_id;
	CvMat * F; 
};

DYNAMIC_STRUCTURE_DECLARATIONS(Calibration_Fundamental_Matrices, Calibration_Fundamental_Matrix);

struct Calibration_Camera 
{
	bool set; 

	size_t shot_id; // id of the shot with this calibration 
	CvMat * P;
	Calibration_Points_Meta points_meta;
	Calibration_Fundamental_Matrices Fs; // note currently not used // fundamental matrices transforming points in different shots to lines on this shot
};

DYNAMIC_STRUCTURE_DECLARATIONS(Calibration_Cameras, Calibration_Camera);

struct Calibration_Vertex 
{
	bool set; 

	size_t vertex_id; // id of the vertex from the original dataset
	CvMat * X;
};

DYNAMIC_STRUCTURE_DECLARATIONS(Calibration_Vertices, Calibration_Vertex);

// partial calibration 
struct Calibration
{
	bool set; 

	Calibration_Cameras Ps; 
	Calibration_Vertices Xs;
	CvMat * pi_infinity;
	bool refined;
};

DYNAMIC_STRUCTURE_DECLARATIONS(Calibrations, Calibration);

// * allocated instances *

extern Shots shots; // shots
extern Shots_Relations shots_relations; // relations between shots (usualy pairs of them)
extern Polygons_3d polygons; // 3d polygons 
extern Vertices vertices; // 3d vertices
extern Vertices_Incidence vertices_incidence; // incidence
extern Calibrations calibrations; // calibrations
extern std::map<int, std::map<int, unsigned int> > detected_edges;

// * initializers *

// initialize scene info 
bool geometry_initialize();

// * destructors *

// release shot structure 
void geometry_release_shot(Shot * & shot);
void geometry_release();

// * data validators * 

// check if shot is valid 
bool validate_shot(size_t id);

// check if point is valid (allocated properly, etc.)
bool validate_point(size_t shot_id, size_t point_id);

// check if vertex is valid
bool validate_vertex(size_t vertex_id);

// check if polygon is valid 
bool validate_polygon(size_t polygon_id);

// * sorting * 

int geometry_sort_double_indices_primary_comparator(const void * pi, const void * pj);
void geometry_sort_double_indices_primary(Double_Indices * indices);
int geometry_sort_double_indices_secondary_comparator(const void * pi, const void * pj);
void geometry_sort_double_indices_secondary(Double_Indices * indices);
int geometry_sort_double_indices_primary_comparator_desc(const void * pi, const void * pj);
void geometry_sort_double_indices_primary_desc(Double_Indices * indices);
int geometry_sort_double_indices_secondary_comparator_desc(const void * pi, const void * pj);
void geometry_sort_double_indices_secondary_desc(Double_Indices * indices);

// * deleting * 

// delete point
void geometry_delete_point(size_t shot_id, size_t point_id);

// delete vertex (and all it's points, incidence structure, ...) 
void geometry_delete_vertex(size_t vertex_id);

// delete polygon
void geometry_delete_polygon(const size_t polygon_id);

// * accessors and modifiers *

// add 2d point vertex incidence
void geometry_point_vertex_incidence(size_t shot_id, size_t point_id, size_t vertex_id);

// get 2d point x coordinate 
double geometry_get_point_x(size_t shot_id, size_t point_id);

// get 2d point y coordinate 
double geometry_get_point_y(size_t shot_id, size_t point_id);

// modify 2d point 
void geometry_point_xy(size_t shot_id, size_t point_id, double x, double y);

// * initialization of new structures *

// initialize new shot
bool geometry_new_shot(size_t & shot);

// initialize containers for camera calibration 
bool geometry_shot_new_calibration_containers(const size_t shot_id);

// create new 3d vertex 
bool geometry_new_vertex(size_t & id);

// create new 2d point 
bool geometry_new_point(size_t & point_id, double x, double y, size_t shot_id, size_t vertex_id);

// create new polygon 
bool geometry_new_polygon(size_t & id);

// * modifying polygons *

// add vertex to polygon 
bool geometry_polygon_add_vertex(size_t polygon_id, size_t vertex_index);

// * releasing * 

// release calibration matrices
void geometry_release_shot_calibration(size_t shot_id);

// release calibration matrices for all shots
void geometry_release_shots_calibrations();

// * builders * 

// for each vertex create list of shots on which said vertex is visible
void geometry_build_vertices_incidence();

// for each shot compute how many correspondences this shot has 
// with the other ones 
void geometry_build_shots_relations();

#endif

