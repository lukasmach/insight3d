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

#include "tool_file.h"

// tool's state structure 
struct Tool_File
{ 
};

static Tool_File tool_file;

void tool_file_new()
{
	ui_prepare_for_deletition(true, true, true, true, true);
	image_loader_cancel_all_requests();
	
	for ALL(shots, i) 
	{
		memset(&shots.data[i].image_loader_request, 0, sizeof(Image_Loader_Request));
	}

	geometry_release();
	ui_list_update();
	ui_workflow_default_shot();
}

void tool_file_open_project()
{
	char * const filename = tool_choose_file();

	// if no file has been selected, there's nothing to do
	if (!filename) return;

	// detete everything 
	ui_prepare_for_deletition(true, true, true, true, true);
	image_loader_cancel_all_requests();
	
	for ALL(shots, i) 
	{
		memset(&shots.data[i].image_loader_request, 0, sizeof(Image_Loader_Request));
	}

	geometry_release();
	ui_list_update();
	ui_workflow_default_shot();

	// load something new 
	geometry_load_project(filename);
	ui_list_update();
	ui_workflow_default_shot();
	visualization_process_data(vertices, shots);

	FREE(filename);
}

void tool_file_save_project()
{
	char * filename = tool_choose_new_file();
	if (!filename) return; 

	geometry_save(filename);

	FREE(filename);
}

void tool_file_add_list_of_images()
{
	char * filename = tool_choose_file(); 
	if (!filename) return; 

	geometry_loader_ifl(filename);
	ui_list_update();
	ui_workflow_default_shot();

	FREE(filename);
}

void tool_file_add_image()
{
	char * filename = tool_choose_file();
	if (!filename) return; 
	
	geometry_loader_add_shot(filename);
	ui_list_update();
	ui_workflow_default_shot();

	FREE(filename);
}

void tool_file_import_realviz_project()
{
	/* char * filename = tool_choose_file();
	if (!filename) return; 
	
	geometry_loader(filename, shots);
	visualization_process_data(vertices, shots);
	ui_list_update();
	ui_workflow_default_shot();
	visualization_process_data(vertices, shots);

	FREE(filename); */
}

void tool_file_import_points()
{
	char * filename = tool_choose_file();
	if (!filename) return; 
	
	bool success = geometry_loader_points_guess_filepair(filename); // {}
	ui_list_update();

	FREE(filename);
}

void tool_file_import_pointcloud()
{
	char * filename = tool_choose_file();
	if (!filename) return; 
	
	bool success = geometry_loader_vertices(filename, vertices);
	visualization_process_data(vertices, shots);

	FREE(filename);
}

void tool_file_export_vrml()
{
	char * filename = tool_choose_new_file();
	if (!filename) return; 
	
	bool success = geometry_export_vrml(filename, vertices, polygons, !option_hide_automatic, true); // {}

	FREE(filename);
}

void tool_file_export_sandy3d()
{
	char * filename = tool_choose_new_file();
	if (!filename) return; 
	
	bool success = geometry_export_sandy3d(filename, vertices, polygons, !option_hide_automatic); // {}

	FREE(filename);
}

void tool_file_export_realviz_project()
{
	char * filename = tool_choose_new_file();
	if (!filename) return; 
	
	bool success = geometry_export_rzml(filename, shots); // {}

	FREE(filename);
}

void tool_file_export_pointcloud()
{
	char * filename = tool_choose_new_file();
	if (!filename) return; 
		
	FILE * fp = fopen(filename, "w");
	// fprintf(fp, "# insight3d vertices export; line format = x y z nx ny nz r g b");

	for ALL(vertices, i) 
	{
		const Vertex * vertex = vertices.data + i; 
		if (!vertex->reconstructed) continue; 

		fprintf(
			fp, 
			"%lf %lf %lf %lf %lf %lf %lf %lf %lf\n", 
			vertex->x, vertex->y, vertex->z, 
			vertex->nx, vertex->ny, vertex->nz,
			vertex->color[0], vertex->color[1], vertex->color[2]
		);
	}
	
	fclose(fp); 

	FREE(filename);
}

void tool_file_export_cameras()
{
	char * filename = tool_choose_new_file();
	if (!filename) return; 
	
	
	FILE * fp = fopen(filename, "w");
	// fprintf(fp, "# insight3d camera export; line format = x y z nx ny nz r g b");

	for ALL(shots, i) 
	{
		const Shot * shot = shots.data + i; 
		if (!shot->projection) continue; 
		const CvMat * P = shot->projection;

		// print projection matrix
		for (int i = 0; i < P->rows; i++)
		{
			for (int j = 0; j < P->cols; j++)
			{
				fprintf(fp, "%lf ", OPENCV_ELEM(P, i, j));
			}
		}
		fprintf(fp, "\n");
	}
	
	fclose(fp);

	FREE(filename);
}

void tool_file_quit()
{
	core_state.running = false;
}

void tool_file_create() 
{
	tool_register_menu_function("Main menu|File|New|", tool_file_new);
	tool_register_menu_function("Main menu|File|Open project (.i3d)|", tool_file_open_project);
	tool_register_menu_function("Main menu|File|Save project (.i3d)|", tool_file_save_project);
	tool_register_menu_function("Main menu|File|Add list of images (.ifl)|", tool_file_add_list_of_images);
	tool_register_menu_function("Main menu|File|Add image (.jpg, .png)|", tool_file_add_image);
	tool_register_menu_function("Main menu|File|Import RealVIZ project (.rzml, .rzi)|", tool_file_import_realviz_project);
	tool_register_menu_function("Main menu|File|Import points (file pair)|", tool_file_import_points);
	tool_register_menu_function("Main menu|File|Import pointcloud (.txt)|", tool_file_import_pointcloud);
	tool_register_menu_function("Main menu|File|Export VRML (.vrml)|", tool_file_export_vrml);
	tool_register_menu_function("Main menu|File|Export Sandy3D ActionScript (.as)|", tool_file_export_sandy3d);
	tool_register_menu_function("Main menu|File|Export RealVIZ project (.rzml, .rzi)|", tool_file_export_realviz_project);
	tool_register_menu_function("Main menu|File|Export cameras (.txt)|", tool_file_export_cameras);
	tool_register_menu_function("Main menu|File|Export pointcloud (.txt)|", tool_file_export_pointcloud);
	tool_register_menu_function("Main menu|File|Quit|", tool_file_quit);
}
