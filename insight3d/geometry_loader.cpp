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

#include "geometry_loader.h"

/* loader constants */ 

// xml elements
/*const xmlChar * XML_ELEM_SHOT = xmlCharStrdup("SHOT"); 
const xmlChar * XML_ELEM_TRANSLATION = xmlCharStrdup("T");
const xmlChar * XML_ELEM_ROTATION = xmlCharStrdup("R");
const xmlChar * XML_ELEM_CAMERA = xmlCharStrdup("CINF"); 
const xmlChar * XML_ELEM_FRAME = xmlCharStrdup("CFRM");
const xmlChar * XML_ELEM_IMAGE_PLANE = xmlCharStrdup("IPLN");
const xmlChar * XML_ELEM_VERTEX = xmlCharStrdup("P");

// xml attributes
const xmlChar * XML_ATTR_NAME = xmlCharStrdup("n"); 
const xmlChar * XML_ATTR_WIDTH = xmlCharStrdup("w");
const xmlChar * XML_ATTR_HEIGHT = xmlCharStrdup("h");
const xmlChar * XML_ATTR_X = xmlCharStrdup("x");
const xmlChar * XML_ATTR_Y = xmlCharStrdup("y");
const xmlChar * XML_ATTR_Z = xmlCharStrdup("z");
const xmlChar * XML_ATTR_FOVX = xmlCharStrdup("fovx");
const xmlChar * XML_ATTR_FILM_BACK = xmlCharStrdup("fbh");
const xmlChar * XML_ATTR_IMAGE = xmlCharStrdup("img");*/

// load project 
bool geometry_load_project(const char * filename)
{
	// open the file 
	std::ifstream in(filename);

	if (!in) 
	{
		printf("Unable to open file for reading.\n");
		return false;
	}

	// load the header 
	std::string s1, s2, s3; // temporary strings
	in >> s1 >> s2 >> s3;

	if (s1 != "insight3d" || s2 != "data" || s3 != "file")
	{
		printf("Invalid header.\n");
		return false;
	}

	// load vertices
	in >> s1;
	if (s1 != "vertices")
	{
		printf("Keyword 'vertices' expected but not found.\n");
		return false;
	}

	size_t vertices_count, vertices_iter = 0;
	in >> vertices_count;
	size_t * vertices_reindex = ALLOC(size_t, vertices_count);
	for (size_t i = 0; i < vertices_count; i++)
	{
		size_t vertex_id;
		geometry_new_vertex(vertex_id);
		vertices_reindex[vertices_iter++] = vertex_id;
		int vertex_type = 0;
		in >> vertices.data[vertex_id].x >> vertices.data[vertex_id].y >> vertices.data[vertex_id].z >> vertices.data[vertex_id].reconstructed >> vertex_type;
		switch (vertex_type)
		{
			case GEOMETRY_VERTEX_AUTO: vertices.data[vertex_id].vertex_type = GEOMETRY_VERTEX_AUTO; break;
			case GEOMETRY_VERTEX_EQUIVALENCE: vertices.data[vertex_id].vertex_type = GEOMETRY_VERTEX_EQUIVALENCE; break;
			case GEOMETRY_VERTEX_USER: vertices.data[vertex_id].vertex_type = GEOMETRY_VERTEX_USER; break;
		}
	}

	// load polygons
	in >> s1;
	size_t polygons_count; 
	in >> polygons_count;
	for (size_t i = 0; i < polygons_count; i++) 
	{
		size_t polygon_id; 
		geometry_new_polygon(polygon_id);

		signed long int vertex_id;
		in >> vertex_id;
		while (vertex_id != -1) 
		{
			geometry_polygon_add_vertex(polygon_id, vertices_reindex[vertex_id]);
			in >> vertex_id;
		}
	}

	// load shots and their points 
	in >> s1; 
	size_t shots_count; 
	in >> shots_count; 

	for (size_t i = 0; i < shots_count; i++) 
	{
		size_t shot_id;
		geometry_new_shot(shot_id);

		Shot * const shot = shots.data + shot_id;
		int status = 0;

		in 
			>> shot->calibrated
			>> shot->f
			>> shot->film_back
			>> shot->fovx
			>> shot->fovy
			>> shot->height;

		char x; 
		in >> x; 
		if (x != '"')
		{
			printf("Expected '\"' character.\n"); 
			return false;
		}

		s1 = "";
		while (in.get(x))
		{
			if (x == '"') break;

			s1 += x;
		}

		in >> status;

		in >> x;  
		if (x != '"')
		{
			printf("Expected '\"' character.\n"); 
			return false;
		}

		s2 = "";
		while (in.get(x))
		{
			if (x == '"') break;

			s2 += x;
		}

		in
			>> shot->pp_x
			>> shot->pp_y
			>> shot->resected
			>> shot->width;

		shot->image_filename = strdup(s1.c_str());
		shot->name = strdup(s2.c_str());

		switch (status) 
		{
			case GEOMETRY_INFO_DEDUCED: shot->info_status = GEOMETRY_INFO_DEDUCED; break;
			case GEOMETRY_INFO_LOADED: shot->info_status = GEOMETRY_INFO_LOADED; break;
			case GEOMETRY_INFO_NOT_LOADED: shot->info_status = GEOMETRY_INFO_NOT_LOADED; break;
		}

		if (shot->calibrated) 
		{
			shot->projection = opencv_create_matrix(3, 4);
		}
		
		for (int ki = 0; ki < 3; ki++) 
		{
			for (int kj = 0; kj < 4; kj++) 
			{
				if (shot->calibrated) 
				{
					in >> OPENCV_ELEM(shot->projection, ki, kj);
				}
				else
				{
					double zero;
					in >> zero;
				}
			}
		}

		if (shot->calibrated)
		{
			shot->rotation = opencv_create_matrix(3, 3);
			shot->internal_calibration = opencv_create_matrix(3, 3);
			shot->translation = opencv_create_matrix(3, 1);
			geometry_calibration_from_P(shot_id);
		}

		// load it's points
		in >> s1;
		size_t points_count;
		in >> points_count; 

		for (size_t j = 0; j < points_count; j++) 
		{
			size_t point_id; 
			double x, y; 
			size_t vertex_id;
			in >> x >> y >> vertex_id;
			geometry_new_point(point_id, x, y, shot_id, vertices_reindex[vertex_id]);
		}
	}

	return true;
}

// process loaded data (compute focal length from fov, assemble projection matrices, ...)
// note isn't this redundant? look at geometry_routines
void geometry_process_data(Shots shots)
{
	// process all shot information 
	for ALL(shots, i)
	{
		Shot * const shot = shots.data + i; 
		if (!shot->calibrated) continue;

		// either focal length or filed of view must be specified 
		if ((shot->f == 0) && (shot->fovx)) {}

		// calculate focal length from field of view
		if (shot->f == 0)
		{
			shot->f = 0.5 * shot->width / tan(0.5 * deg2rad(shot->fovx));
		}

		// convert translation vector to opencv
		LOCK_RW(opencv)
		{
			shot->translation = opencv_create_vector(shot->T, 3); 

			// compute rotation matrix
			shot->rotation = opencv_create_rotation_matrix_from_euler(shots.data[i].R_euler);
			// printf("%d\n", i); 
			// opencv_debug("Rotation matrix", shot->rotation);

			// build calibration matrix
			shot->internal_calibration = opencv_create_matrix(3, 3);
			OPENCV_ELEM(shot->internal_calibration, 0, 0) = shot->f; 
			OPENCV_ELEM(shot->internal_calibration, 1, 1) = shot->f; 
			OPENCV_ELEM(shot->internal_calibration, 0, 2) = shot->pp_x; // shot->width / 2.0; 
			OPENCV_ELEM(shot->internal_calibration, 1, 2) = shot->pp_y; // shot->height / 2.0; 
			OPENCV_ELEM(shot->internal_calibration, 2, 2) = 1; 
		}
		UNLOCK_RW(opencv);

		// finally assemble projection matrix 
		geometry_calibration_from_decomposed_matrices(i);

		// debug
		// specific 
		// check whether we have rectifying homography and apply it's inverse 
		// to the camera
		std::ifstream H((std::string(shot->image_filename) + ".H.txt").c_str());
		if (H)
		{
			LOCK_RW(opencv) 
			{
				printf("~");
				CvMat * M = opencv_create_matrix(3, 3), * M_inv = opencv_create_matrix(3, 3); 

				H >> OPENCV_ELEM(M, 0, 0);
				H >> OPENCV_ELEM(M, 0, 1);
				H >> OPENCV_ELEM(M, 0, 2);

				H >> OPENCV_ELEM(M, 1, 0);
				H >> OPENCV_ELEM(M, 1, 1);
				H >> OPENCV_ELEM(M, 1, 2);

				H >> OPENCV_ELEM(M, 2, 0);
				H >> OPENCV_ELEM(M, 2, 1);
				H >> OPENCV_ELEM(M, 2, 2);
				
				cvInvert(M, M_inv, CV_SVD);
				cvMatMul(M_inv, shot->projection, shot->projection);

				cvReleaseMat(&M);
				cvReleaseMat(&M_inv);
			}
			UNLOCK_RW(opencv);
		}
	}
}

// SAX callbacks
/*void geometry_loader_SAX_start_element(geometry_loader_SAX_state * state, const xmlChar * name, const xmlChar ** attrs)
{
	if (xmlStrcmp(name, XML_ELEM_SHOT) == 0)
	{
		// image shot

		// initialize new shot structure
		size_t new_shot_id; 
		geometry_new_shot(new_shot_id); // {}
		state->current_shot = state->shots->data + new_shot_id;

		// save information from the shot tag 
		const xmlChar ** attr = attrs; 

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// todo save shots name and use it instead of it's filename
			if (xmlStrcmp(*attr, XML_ATTR_NAME) == 0) state->current_shot->name = (char *)xmlStrdup(value); // note conversion
			// width
			else if (xmlStrcmp(*attr, XML_ATTR_WIDTH) == 0) 
			{
				state->current_shot->width = atoi((char *)value); // note conversion

				if (state->current_shot->pp_x == 0) 
				{
					state->current_shot->pp_x = state->current_shot->width / 2;
				}
			}
			// height
			else if (xmlStrcmp(*attr, XML_ATTR_HEIGHT) == 0)
			{
				state->current_shot->height = atoi((char *)value); // note conversion

				if (state->current_shot->pp_y == 0) 
				{
					state->current_shot->pp_y = state->current_shot->height / 2;
				}
			}
			// else: unknown attribute, not necessarily a problem 

			// move on to next attribute
			attr += 2; 
		}
	}
	else if (xmlStrcmp(name, XML_ELEM_TRANSLATION) == 0)
	{
		// camera translation 
		const xmlChar ** attr = attrs; 

		// mark as calibrated
		if (attr) state->current_shot->calibrated = true;

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// x coordinate
			if (xmlStrcmp(*attr, XML_ATTR_X) == 0) state->current_shot->T[0] = atof((char *)value); // note conversion
			// y coordinate
			else if (xmlStrcmp(*attr, XML_ATTR_Y) == 0) state->current_shot->T[1] = atof((char *)value); // note conversion
			// z coordinate
			else if (xmlStrcmp(*attr, XML_ATTR_Z) == 0) state->current_shot->T[2] = atof((char *)value); // note conversion

			// we're using homogeneous coordinates 
			state->current_shot->T[3] = 1.0; 

			// move on to next attribute
			attr += 2; 
		}
	}
	else if (xmlStrcmp(name, XML_ELEM_ROTATION) == 0)
	{
		// camera rotation (euler angles)
		const xmlChar ** attr = attrs; 

		// mark as calibrated
		if (attr) state->current_shot->calibrated = true;

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// x angle 
			if (xmlStrcmp(*attr, XML_ATTR_X) == 0) state->current_shot->R_euler[0] = deg2rad(atof((char *)value)); // note conversion
			// y angle
			else if (xmlStrcmp(*attr, XML_ATTR_Y) == 0) state->current_shot->R_euler[1] = deg2rad(atof((char *)value)); // note conversion
			// z angle
			else if (xmlStrcmp(*attr, XML_ATTR_Z) == 0) state->current_shot->R_euler[2] = deg2rad(atof((char *)value)); // note conversion

			// move on to next attribute
			attr += 2; 
		}
	}
	else if (xmlStrcmp(name, XML_ELEM_FRAME) == 0)
	{
		// frame info (contains field of view)
		const xmlChar ** attr = attrs; 

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// fovx angle 
			if (xmlStrcmp(*attr, XML_ATTR_FOVX) == 0) state->current_shot->fovx = atof((char *)value); // note conversion

			// move on to next attribute
			attr += 2; 
		}
	}
	else if (xmlStrcmp(name, XML_ELEM_IMAGE_PLANE) == 0)
	{
		// frame info (contains field of view)
		const xmlChar ** attr = attrs; 

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// image filename (with path)
			// note this shots attribute is set also in shot tag, but it will be overwritten in case it is also in ipln tag
			if (xmlStrcmp(*attr, XML_ATTR_IMAGE) == 0) 
				state->current_shot->image_filename = interface_filesystem_cleanup_filename((char *)value, state->directory); // note conversion

			// move on to next attribute
			attr += 2; 
		}
	}
	else if (xmlStrcmp(name, XML_ELEM_CAMERA) == 0)
	{
		// frame info (contains field of view)
		const xmlChar ** attr = attrs; 

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// film back size
			if (xmlStrcmp(*attr, XML_ATTR_FILM_BACK) == 0) state->first_film_back = atof((char *)value); // note conversion

			// move on to next attribute
			attr += 2; 
		}
	}
	else if (xmlStrcmp(name, XML_ELEM_VERTEX) == 0) 
	{
		// vertex 
		const xmlChar ** attr = attrs; 
		double x = 0, y = 0, z = 0; 

		while (*attr != NULL) 
		{
			// read the attribute value 
			const xmlChar * value = *(attr + 1);
			if (value == NULL) { return; }  // {} 

			// process attributes 

			// x coordinate 
			if (xmlStrcmp(*attr, XML_ATTR_X) == 0) x = atof((char *)value); // note conversion
			// y coordinate 
			else if (xmlStrcmp(*attr, XML_ATTR_Y) == 0) y = atof((char *)value); // note conversion
			// z coordinate 
			else if (xmlStrcmp(*attr, XML_ATTR_Z) == 0) z = atof((char *)value); // note conversion

			// move on to next attribute
			attr += 2; 
		}

		// add vertex
		ADD(*(state->vertices)); 
		LAST(*(state->vertices)).x = x; 
		LAST(*(state->vertices)).y = y; 
		LAST(*(state->vertices)).z = z; 
		LAST(*(state->vertices)).reconstructed = true;
	}
}

void geometry_loader_SAX_end_element(geometry_loader_SAX_state * state, const xmlChar * name)
{
	if (xmlStrcmp(name, XML_ELEM_SHOT) == 0)
	{
		// if no name for this shot was submitted, use it's filename without directory path
		if (!state->current_shot->name && state->current_shot->image_filename)
		{
			state->current_shot->name = interface_filesystem_extract_filename(state->current_shot->image_filename);
		}
	}
}

void geometry_loader_SAX_characters(geometry_loader_SAX_state * state, const xmlChar * cdata, int len)
{
}*/

// load data from realviz xml file // todo probably rename this function to reflect that it loads .rzml, .rzi files
/*void geometry_loader(const char * xml_filename, Shots & shots)
{
	// initialization
	xmlSAXHandler handler; 
	geometry_loader_SAX_state * state = ALLOC(geometry_loader_SAX_state, 1);
	memset(&handler, 0, sizeof(handler)); 
	memset(state, 0, sizeof(geometry_loader_SAX_state));
	state->filename = strdup(xml_filename);
	state->directory = interface_filesystem_dirpath(state->filename);

	// state setup 
	state->shots = &shots; 
	state->vertices = &vertices;

	// register callbacks to SAX parser
	handler.startElement = (startElementSAXFunc)geometry_loader_SAX_start_element;
	handler.endElement = (endElementSAXFunc)geometry_loader_SAX_end_element;
	handler.characters = (charactersSAXFunc)geometry_loader_SAX_characters;

	// open xml file using SAX parser 
	xmlSAXUserParseFile(&handler, state, xml_filename);

	// process loaded data (compute remaining values)
	shots.count = state->shots->count; 
	geometry_process_data(shots);

	// free resources 
	free(state->filename); 
	FREE(state->directory);
	FREE(state); 
}*/

// load data from realviz rz3 file
bool geometry_loader_rz3(const char * xml_filename, Shots & shots)
{
	// open the file 
	FILE * fp = fopen(xml_filename, "r"); 
	if (!fp) return false; 

	// read all entries
	while (!feof(fp)) 
	{
		int shot_id; 
		double ratio, focal_length, principal_point[2], t[3], R[9];

		// read the entry
		const int entry_read = fscanf(
			fp,
			"%d  F ( %lf ) Pr ( %lf ) Pp ( %lf %lf ) K ( 0 ) Oc ( %lf %lf %lf ) Rot ( %lf %lf %lf %lf %lf %lf %lf %lf %lf )",
			&shot_id,
			&focal_length,
			&ratio,
			principal_point + 0,
			principal_point + 1,
			t + 0, t + 1, t + 2,
			R + 0, R + 1, R + 2,
			R + 3, R + 4, R + 5,
			R + 6, R + 7, R + 8
		); // TODO use entry_read

		// save it
		printf("accessing shot %d\n", shot_id);
		ASSERT_IS_SET(shots, shot_id);
		Shot * const shot = shots.data + shot_id;
		shot->T[0] = t[0];
		shot->T[1] = t[1];
		shot->T[2] = t[2];
		shot->T[3] = 1.0;
		shot->pp_x = principal_point[0]; 
		shot->pp_y = principal_point[1];

		// decompose matrix
		LOCK_RW(opencv)
		{
			CvMat * R_in_cv = opencv_create_matrix(3, 3, R);
			opencv_rotation_matrix_to_angles(R_in_cv, shot->R_euler[0], shot->R_euler[1], shot->R_euler[2]); 
			cvReleaseMat(&R_in_cv);
		}
		UNLOCK_RW(opencv);

		shot->R_euler[0] += OPENCV_PI;
		shot->R_euler[0] *= -1;
		shot->R_euler[1] *= -1;
		shot->R_euler[2] *= -1;

		shot->f = focal_length;
		shot->calibrated = true;
	}

	// release resources  
	fclose(fp);

	// specific clean principal point of first shot 
	shots.data[0].pp_x = shots.data[0].width / 2; 
	shots.data[0].pp_y = shots.data[0].height / 2;

	// data post processing 
	geometry_process_data(shots);
}

// load 3d vertices from text file (one vertex per line: <x> <y> <z>); returns true on success // note previously we had vertex_id on the begining of the line
bool geometry_loader_vertices(const char * txt_filename, Vertices & vertices, size_t group /*= 0*/) 
{
	// Open the file.
			std::ifstream input_vertices(txt_filename);
	// If opening succeeded,
			if (input_vertices) {
	// read all it's lines
			size_t vertex_input_id = 0; double x, y, z; while (input_vertices >> x >> y >> z) {
	// and interpret each as a vertex which is then added to array vertices.
			ADD(vertices); LAST(vertices).x = x; LAST(vertices).y = y; LAST(vertices).z = z;
	// Also set vertex's group to the value passed in function parameter group.
			LAST(vertices).group = group; LAST(vertices).reconstructed = true; } return true; // todo check bounds
	// But if the file couldn't be opened, return false.
			} else { return false; }
}

// load 2d points from text file (one point per line: <picture_no> <vertex_no> <credibility> <x> <y> <...>); returns true on success
// todo how to distinguish between this and currently used geometry_loader_points
// todo if we decide to uncomment this, check thread safety 
/*bool geometry_loader_points(const char * txt_filename, Shots & shots, Vertices & vertices, size_t group = 0)
{
	// open file 
	std::ifstream input_points(txt_filename);

	if (input_points)
	{
		// read info about all 2d points 
		size_t shot_input_id, vertex_input_id, point_input_id; 
		int credibility;
		double x, y; 

		while (input_points >> shot_input_id >> vertex_input_id >> point_input_id >> credibility >> x >> y) 
		{
			// make sure that the shot is defined 
			ASSERT_IS_SET(shots, shot_input_id); 

			// add 2d point 
			DYN(shots.data[shot_input_id].points, point_input_id);
			shots.data[shot_input_id].points.data[point_input_id].vertex = vertex_input_id; 
			shots.data[shot_input_id].points.data[point_input_id].x = x; 
			shots.data[shot_input_id].points.data[point_input_id].y = y; 
			if (credibility != 0) 
			{
				shots.data[shot_input_id].points.data[point_input_id].data_origin = GEOMETRY_MANUAL_INPUT; 
			}
			else
			{
				shots.data[shot_input_id].points.data[point_input_id].data_origin = GEOMETRY_NOT_CREDIBLE;
			}

			// create vertex 
			DYN(vertices, vertex_input_id); 
			vertices.data[vertex_input_id].group = group;
		}

		// rebuild precomputed structures
		geometry_build_vertices_incidence();
	}
	else
	{
		// {} 
		return false;
	}

	return true;
}*/

// load contours
bool geometry_loader_contours(const char * txt_filename, Shots & shots) 
{
	// open file 
	std::ifstream input_points(txt_filename);

	if (input_points)
	{
		// read info about all shot polygons 
		bool contour_data = false; 
		size_t shot_input_id; 
		long int i; // note using long int to address memory

		while (input_points >> i)
		{
			if (!contour_data) 
			{
				contour_data = true; 
				ASSERT_IS_SET(shots, (size_t)i);
				shot_input_id = i; 
				ADD(shots.data[i].contours);
				LAST(shots.data[i].contours).set = true;
			}
			else if (i >= 0) 
			{
				ADD(LAST(shots.data[shot_input_id].contours).vertices);
				LAST(LAST(shots.data[shot_input_id].contours).vertices).set = true;
				LAST(LAST(shots.data[shot_input_id].contours).vertices).value = i;
			}
			else if (i == -1)
			{
				contour_data = false;
			}
			else
			{
				ASSERT(false, "invalid index");
			}
		}

		ASSERT(!contour_data, "polygon not closed");
	}
	else
	{
		// {} 
		return false;
	}

	return false;
}

// load points from text files
bool geometry_loader_points(const char * pictures_filename, const char * tracks_filename, Shots & shots, Vertices & vertices, size_t group /*= 0*/)
{
	// open file 
	std::ifstream input_pictures(pictures_filename), input_tracks(tracks_filename); 

	// tracks were maybe generated for different set of images, we want to import
	// only the pictures in our database of shots
	Indices index_in_shots;

	// load list of pictures from input file
	if (input_pictures) 
	{
		size_t track_picture_id = 0; 
		DYN_INIT(index_in_shots);
		std::string picture_filename; 

		while (input_pictures >> picture_filename) 
		{
			// find this picture in shots
			for ALL(shots, i) 
			{
				// if it is there, save it's index
				if (interface_filesystem_compare_filenames(picture_filename.c_str(), shots.data[i].name)) 
				{
					DYN(index_in_shots, track_picture_id);
					index_in_shots.data[track_picture_id].value = i; 
					break; 
				}
			}

			track_picture_id++; 
		}
	}
	else
	{
		DYN_FREE(index_in_shots);
		return false;
	}

	// load tracks 
	const size_t vertices_offset = vertices.count; 

	if (input_tracks)
	{
		size_t picture_id, track_id; 
		double x, y; 
		size_t t = 0;

		while (input_tracks >> picture_id >> track_id >> x >> y)
		{
			// skip track if it's picture isn't in our sequence
			if (!IS_SET(index_in_shots, picture_id)) continue;

			// create vertex for this point (does nothing if it already exists)
			const size_t vertex_id = vertices_offset + track_id;
			if (t++ % 10000 == 0)
			{
				// debug
				printf("vertices: accessing %d, count %d, allocated %d\n", vertex_id, vertices.count, vertices.allocated);
			}
			DYN(vertices, vertex_id); 
			vertices.data[vertex_id].group = group;
			vertices.data[vertex_id].vertex_type = GEOMETRY_VERTEX_AUTO;

			// save point information
			ASSERT_IS_SET(shots, index_in_shots.data[picture_id].value);
			const size_t shot_id = index_in_shots.data[picture_id].value; 
			ADD(shots.data[shot_id].points);
			LAST(shots.data[shot_id].points).x = x / shots.data[shot_id].width; 
			LAST(shots.data[shot_id].points).y = y / shots.data[shot_id].height; 
			LAST(shots.data[shot_id].points).vertex = vertex_id;
			geometry_point_vertex_incidence(shot_id, LAST_INDEX(shots.data[shot_id].points), vertex_id);
		}
	}
	else
	{
		DYN_FREE(index_in_shots);
		return false; 
	}

	DYN_FREE(index_in_shots);
	return true;
}

// add another image to the sequence 
bool geometry_loader_add_shot(const char * filename) // note that we're being inconsistent here by not passing reference to shots structure, but we can't do that since we manipulate it by geometry_new_shot - isn't this a bigger problem? 
{
	// obtain id for new shot 
	size_t shot_id; 
	if (!geometry_new_shot(shot_id)) return false; 

	// fill in what we know about the image (which is just it's filename)
	Shot * const shot = shots.data + shot_id;
	if (!(shot->image_filename = ALLOC(char, strlen(filename) + 1)))
	{
		core_state.error = CORE_ERROR_OUT_OF_MEMORY; 
		return false; 
	}
	strcpy(shot->image_filename, filename);
	shot->name = interface_filesystem_extract_filename(shot->image_filename);

	return true;
}

// load IFL file (i.e., image file list) 
bool geometry_loader_ifl(const char * filename) // note the same inconsistency as in geometry_loader_add_shot
{
	// get path to ifl file's directory 
	const char * dirpath = interface_filesystem_dirpath(filename); 
	const size_t dirpath_length = strlen(dirpath);

	// open the file 
	std::ifstream input_list(filename); 
	if (!input_list) 
	{
		core_state.error = CORE_ERROR_UNABLE_TO_OPEN_FILE;
		return false; 
	}

	// add the shots 
	std::string picture_filename;
	while (input_list >> picture_filename) 
	{
		// check if this looks like a filename 
		for (size_t filename_iter = 0; filename_iter < picture_filename.length(); filename_iter++) 
		{
			if (picture_filename[filename_iter] >= 0 && picture_filename[filename_iter] <= 31 && picture_filename[filename_iter] != 13 && picture_filename[filename_iter] != 10)
			{
				printf("Non-printable characters found, doesn't look like .ifl file.\n");
				return false;
			}
		}

		// decide if this is absolute or relative path 
		const bool is_relative = interface_filesystem_is_relative(picture_filename.c_str());

		// construct filename
		const size_t filename_length = strlen(picture_filename.c_str());
		char * picture_path;
		if (is_relative)
		{
			picture_path = ALLOC(char, dirpath_length + 1 + filename_length + 1);
			picture_path[0] = '\0';

			strcat(picture_path, dirpath); 
			strcat(picture_path, FILESYSTEM_PATH_SEPARATOR);
			strcat(picture_path, picture_filename.c_str());
		}
		else
		{
			picture_path = (char *)picture_filename.c_str();
		}

		// try to find it - we don't add duplicates
		bool skip = false;
		for ALL(shots, i)
		{
			// if it is there, save it's index
			if (interface_filesystem_compare_filenames(picture_filename.c_str(), shots.data[i].name)) 
			{
				skip = true;
				break;
			}
		}

		// load it 
		if (!skip)
		{
			geometry_loader_add_shot(picture_path);
		}

		// clean-up if necessary 
		if (is_relative) 
		{
			FREE(picture_path);
		}
	}

	input_list.close();
	return true;
}

// load points from *_points.txt and *_pictures.ifl imagepair
bool geometry_loader_points_guess_filepair(char * const filename)
{
	// we expect that there is pair of files, one containing list of images (which specifies frame 
	// numbering) and the second one containing points coordinates in simple text format

	// at first, we have to guess which one was selected by the user
	char * points_filename, * pictures_filename;
	const char * extension = strrchr(filename, 'p'); // we either look for files ending with "points.txt" or "pictures.ifl"
	const size_t len = strlen(filename);
	if (strcmpi(extension, "points.txt") == 0)
	{
		// its the points.txt file, construct the other one
		points_filename = filename;
		pictures_filename = ALLOC(char, len + 2 + 1);
		memcpy(pictures_filename, filename, sizeof(char) * (len - 10));
		memcpy(pictures_filename + len - 10, "pictures.ifl", sizeof(char) * 13);
	}
	else if (strcmpi(extension, "pictures.ifl") == 0)
	{
		// its the pictures.ifl file
		pictures_filename = filename; 
		points_filename = ALLOC(char, len - 2 + 1);
		memcpy(points_filename, filename, sizeof(char) * (len - 12));
		memcpy(points_filename + len - 12, "points.txt", sizeof(char) * 11);
	}
	else 
	{
		return false; 
	}

	// we can now load it 
	return geometry_loader_points(pictures_filename, points_filename, shots, vertices, 0);
}
