#include "geometry_export.h"

// save current application state
bool geometry_save(const char * filename)
{
	std::ofstream out(filename);

	if (!out) 
	{
		printf("Unable to open file for writing.\n");
		return false;
	}

	// header
	out << "insight3d data file" << std::endl;

	// refactor vertices 
	size_t * vertices_reindex = ALLOC(size_t, vertices.count);
	size_t vertices_count = 0;
	memset(vertices_reindex, 0, sizeof(size_t) * vertices.count);

	for ALL(vertices, i) 
	{
		vertices_reindex[i] = vertices_count++;
	}

	// dump vertices 
	out << "vertices " << vertices_count << std::endl;
	for ALL(vertices, i) 
	{
		const Vertex * const vertex = vertices.data + i; 

		out << vertex->x << " " << vertex->y << " " << vertex->z << " " << vertex->reconstructed << " " << vertex->vertex_type << std::endl;
	}

	// prepare polygons 
	size_t polygons_count = 0; 
	for ALL(polygons, i) 
	{
		polygons_count++;
	}

	// dump polygons 
	out << "polygons " << polygons_count << std::endl; 
	for ALL(polygons, i) 
	{
		const Polygon_3d * const polygon = polygons.data + i; 
		for ALL(polygons.data[i].vertices, j) 
		{
			out << vertices_reindex[polygons.data[i].vertices.data[j].value] << " ";
		}

		out << "-1" << std::endl;
	}

	// prepare shots
	size_t shots_count = 0;
	for ALL(shots, i) 
	{
		shots_count++;
	}

	// shots 
	out << "shots " << shots_count << std::endl;
	for ALL(shots, i) 
	{
		const Shot * const shot = shots.data + i; 
		out 
			<< shot->calibrated << " "
			<< shot->f << " "
			<< shot->film_back << " "
			<< shot->fovx << " "
			<< shot->fovy << " "
			<< shot->height << " "
			<< "\"" << shot->image_filename << "\" "
			<< shot->info_status << " "
			<< "\"" << shot->name << "\" "
			<< shot->pp_x << " "
			<< shot->pp_y << " "
			<< shot->resected << " "
			<< shot->width << " ";

		for (int ki = 0; ki < 3; ki++) 
		{
			for (int kj = 0; kj < 4; kj++) 
			{
				if (shot->projection) 
				{
					out << OPENCV_ELEM(shot->projection, ki, kj) << " ";
				}
				else
				{
					out << "0 "; // note that projection matrices of finite cameras must have rank 3 and thus this is clean
				}
			}
		}

		out << std::endl;

		// prepare points 
		size_t points_count = 0;
		for ALL(shot->points, j) 
		{
			points_count++;
		}

		// dump points
		out << "points " << points_count << std::endl;
		for ALL(shot->points, j)
		{
			const Point * const point = shot->points.data + j; 
			out 
				<< point->x << " "
				<< point->y << " "
				<< vertices_reindex[point->vertex] << std::endl;
		}
	}

	// write checksum 
	out << "checksum " << (vertices_count + polygons_count) % 1000 << std::endl;

	out.close();
	FREE(vertices_reindex);

	return true;
}

// export the scene and polygons into VRML
bool geometry_export_vrml(const char * filename, Vertices & vertices, Polygons_3d & polygons, bool export_vertices /*= true*/, bool export_polygons /*= true*/, size_t restrict_vertices_by_group /*= 0*/)
{
	// open file for output 
	std::ofstream vrml_output(filename); 
	if (!vrml_output) 
	{
		core_state.error = CORE_ERROR_UNABLE_TO_OPEN_FILE;
		return false; 
	}
	vrml_output.precision(9);

	// write vrml header 
	vrml_output << "#VRML V2.0 utf8" << std::endl; 
	vrml_output << "NavigationInfo {type [\"EXAMINE\", \"ANY\"]}" << std::endl;

	// * export data *
	
	// vertices 
	if (export_vertices) 
	{
		// vertices header
		vrml_output << "Group { children [ Shape { geometry PointSet { coord Coordinate {point [" << std::endl;

		// dump all vertices as point cloud 
		for ALL(vertices, i) 
		{
			const Vertex * vertex = vertices.data + i; 
			if (!vertex->reconstructed) continue;
			if (restrict_vertices_by_group && vertex->group != restrict_vertices_by_group) continue; 
			vrml_output << -visualization_normalize(vertex->x, X) << " " << -visualization_normalize(vertex->y, Y) << " " << visualization_normalize(vertex->z, Z) << std::endl;
		}

		// separate coordinates from colors
		vrml_output << "] } color Color { color [ " << std::endl;
		
		for ALL(vertices, i) 
		{
			const Vertex * vertex = vertices.data + i; 
			if (!vertex->reconstructed) continue;
			if (restrict_vertices_by_group && vertex->group != restrict_vertices_by_group) continue; 
			if (
				inside_interval(vertex->color[0], 0, 1) && 
				inside_interval(vertex->color[1], 0, 1) && 
				inside_interval(vertex->color[2], 0, 1)
			)
			{
				vrml_output 
					<< vertex->color[0] << " " 
					<< vertex->color[1] << " "
					<< vertex->color[2] << std::endl; 
			}
			else
			{
				vrml_output << "1 1 1" << std::endl;
			}
		}

		vrml_output << "] } } } ] }" << std::endl; 
	}

	// polygons 
	if (export_polygons) 
	{
		vrml_output << "Group { children [" << std::endl;
		vrml_output << "DEF exported_model Shape " << std::endl;
		vrml_output << "{ " << std::endl;
		vrml_output << "    geometry IndexedFaceSet " << std::endl;
		vrml_output << "    { " << std::endl;
		vrml_output << "        coord Coordinate " << std::endl;
		vrml_output << "        { " << std::endl;
		vrml_output << "            point [ " << std::endl;
		
		// reindex vertices
		bool * used = ALLOC(bool, vertices.count);
		memset(used, 0, sizeof(bool) * vertices.count);
		size_t * ids = ALLOC(size_t, vertices.count);
		memset(ids, SIZE_MAX, sizeof(size_t) * vertices.count);
		size_t count = 0;

		for ALL(polygons, i)
		{
			for ALL(polygons.data[i].vertices, j)
			{
				if (!vertices.data[polygons.data[i].vertices.data[j].value].reconstructed) continue;
				used[polygons.data[i].vertices.data[j].value] = true;
			}
		}

		for ALL(vertices, i)
		{
			const Vertex * const vertex = vertices.data + i;

			if (!used[i]) continue;
			
			ids[i] = count++;
			vrml_output << -visualization_normalize(vertex->x, X) << " " << -visualization_normalize(vertex->y, Y) << " " << visualization_normalize(vertex->z, Z) << std::endl;
		}

		vrml_output << "            ]" << std::endl;
		vrml_output << "        } " << std::endl;
		vrml_output << "        coordIndex [ "; 
	
		for ALL(polygons, i)
		{
			for ALL(polygons.data[i].vertices, j)
			{
				vrml_output << ids[polygons.data[i].vertices.data[j].value] << " ";
			}

			vrml_output << "-1 " << std::endl;
		}

		FREE(ids);
		FREE(used);
		
		vrml_output << "        ] " << std::endl; 
		vrml_output << "        color Color { " << std::endl; 
		vrml_output << "            color [  " << std::endl; 
		
		for ALL(vertices, i)
		{
			const Vertex * const vertex = vertices.data + i;
			if (!vertex->reconstructed) continue;
			vrml_output << vertex->color[0] << " " << vertex->color[1] << " " << vertex->color[2] << " ";
		}

		vrml_output << std::endl;
		vrml_output << "            ] " << std::endl;
		vrml_output << "        } " << std::endl;
		vrml_output << "        colorPerVertex TRUE" << std::endl; 
		vrml_output << "        solid FALSE" << std::endl; 
		vrml_output << "    } " << std::endl;
		vrml_output << "    appearance DEF my_polygons Appearance { material Material { diffuseColor .5 .5 .5 specularColor .3 .3 .3 emissiveColor .5 .5 .5 ambientIntensity .05 shininess .05 } } " << std::endl;
		vrml_output << "} ] }" << std::endl;
	}

	vrml_output.close();
	return true;
}

// export scene into Sandy3D ActionScript file
bool geometry_export_sandy3d(const char * filename, Vertices & vertices, Polygons_3d & polygons, bool export_vertices)
{
	// open file for output 
	std::ofstream as_output(filename), stars_output((std::string(filename) + ".stars.txt").c_str()); // todo remove point cloud extraction for release

	if (!as_output) 
	{
		core_state.error = CORE_ERROR_UNABLE_TO_OPEN_FILE;
		return false; 
	}
	as_output.precision(9);

	// class name 
	char 
		* class_name = interface_filesystem_extract_filename(filename),
		* dot = strchr(class_name, '.');

	if (dot) *dot = '\0';

	// * export data *

	// write sandy header 
	as_output << "package" << std::endl;
	as_output << "{" << std::endl;
	as_output << "import sandy.primitive.Primitive3D;" << std::endl << std::endl;

	as_output << "import sandy.core.scenegraph.Geometry3D;" << std::endl;
	as_output << "import sandy.core.scenegraph.Shape3D;" << std::endl;
	as_output << "public class " << class_name << " extends Shape3D implements Primitive3D" << std::endl;
	as_output << "{" << std::endl;
	as_output << "private var l:Geometry3D ;" << std::endl;
	as_output << "private function v(x:Number,y:Number,z:Number):void" << std::endl;
	as_output << "{" << std::endl;
	as_output << "l.setVertex(l.getNextVertexID(),x,y,z );" << std::endl;
	as_output << "}" << std::endl;
	as_output << "private function vn(nx:Number,ny:Number,nz:Number):void" << std::endl;
	as_output << "{" << std::endl;
	as_output << "l.setVertexNormal(l.getNextVertexNormalID(),nx,ny,nz );" << std::endl;
	as_output << "}" << std::endl;
	as_output << "private function uv(u:Number,v:Number):void" << std::endl;
	as_output << "{" << std::endl;
	as_output << "l.setUVCoords(l.getNextUVCoordID(),u,v);" << std::endl;
	as_output << "}" << std::endl;
	as_output << "private function f(vn0:int, vn1:int, vn2:int, uvn0:int, uvn1:int,uvn2:int):void" << std::endl;
	as_output << "{" << std::endl;
	as_output << "l.setFaceVertexIds(l.getNextFaceID(), vn0, vn1,vn2);" << std::endl;
	as_output << "l.setFaceUVCoordsIds( l.getNextFaceUVCoordID(), uvn0,uvn1,uvn2);" << std::endl;
	as_output << "}" << std::endl;
	as_output << "public function " << class_name << "( p_Name:String=null )" << std::endl;
	as_output << "{" << std::endl;
	as_output << "super( p_Name ) ;" << std::endl << std::endl;

	as_output << "geometry = generate() ;" << std::endl;
	as_output << "}" << std::endl << std::endl;

	as_output << "public function generate(... arguments):Geometry3D" << std::endl;
	as_output << "{" << std::endl;
	as_output << "l = new Geometry3D();	" << std::endl;

	// * export polygons *

	// reindex vertices
	bool * used = ALLOC(bool, vertices.count);
	memset(used, 0, sizeof(bool) * vertices.count);
	size_t * ids = ALLOC(size_t, vertices.count);
	memset(ids, SIZE_MAX, sizeof(size_t) * vertices.count);
	size_t count = 0;

	for ALL(polygons, i)
	{
		for ALL(polygons.data[i].vertices, j)
		{
			if (!vertices.data[polygons.data[i].vertices.data[j].value].reconstructed) continue;
			used[polygons.data[i].vertices.data[j].value] = true;
		}
	}

	// output vertices
	for ALL(vertices, i)
	{
		const Vertex * const vertex = vertices.data + i;

		if (!used[i]) continue;
		
		ids[i] = count++;
		as_output << "v(" << -100 * visualization_normalize(vertex->x, X) << "," << 100 * visualization_normalize(vertex->y, Y) << "," << 100 * visualization_normalize(vertex->z, Z) << ");" << std::endl;
	}

	// output polygons
	for ALL(polygons, i)
	{
		size_t count = 0;
		as_output << "l.setFaceVertexIds(l.getNextFaceID(), ";
		for ALL(polygons.data[i].vertices, j)
		{
			if (count > 0) as_output << ",";
			as_output << ids[polygons.data[i].vertices.data[j].value];
			count++;
		}
		as_output << ");" << std::endl;
	}

	// export pointcloud 
	if (export_vertices) 
	{
		/*const double point_size = 1.3;

		for ALL(vertices, i) 
		{
			const Vertex * const vertex = vertices.data + i;
			if (i % 2 > 0) continue;

			as_output << "v(" << -100 * visualization_normalize(vertex->x, X) + point_size << "," << 100 * visualization_normalize(vertex->y, Y) + point_size << "," << 100 * visualization_normalize(vertex->z, Z) << ");" << std::endl;
			as_output << "v(" << -100 * visualization_normalize(vertex->x, X) + point_size << "," << 100 * visualization_normalize(vertex->y, Y) - point_size << "," << 100 * visualization_normalize(vertex->z, Z) << ");" << std::endl;
			as_output << "v(" << -100 * visualization_normalize(vertex->x, X) - point_size << "," << 100 * visualization_normalize(vertex->y, Y) - point_size << "," << 100 * visualization_normalize(vertex->z, Z) << ");" << std::endl;
			as_output << "l.setFaceVertexIds(l.getNextFaceID(), " << count << "," << count + 1 << "," << count + 2 << ");" << std::endl;
			count += 3;
		}*/

		for ALL(vertices, i) 
		{
			const Vertex * const vertex = vertices.data + i;
			stars_output << "sf.stars.push(new Vertex (" << -100 * visualization_normalize(vertex->x, X) << "," << 100 * visualization_normalize(vertex->y, Y) << "," << 100 * visualization_normalize(vertex->z, Z) << "));" << std::endl;
			stars_output << "sf.starColors.push(0xff000000 + " << (int)(255 * vertex->color[0]) * 0x010000 + (int)(255 * vertex->color[1]) * 0x0100 + (int)(255 * vertex->color[2]) << ");" << std::endl;
			// stars_output << "sf.starColors.push(0xffff0000);" << std::endl;
		}
	}

	// release resources 
	FREE(ids);
	FREE(used);
	FREE(class_name);

	// write footer
	as_output << "return (l);" << std::endl;
	as_output << "}" << std::endl;
	as_output << "}" << std::endl;
	as_output << "}" << std::endl;
	as_output.close();

	return true;
}

// debugging export for calibration into VRLM
bool geometry_export_vrml_calibration(const char * filename, Calibration &calibration) 
{
	Calibration_Vertices * const vertices = &calibration.Xs;
	
	// open file for output 
	std::ofstream vrml_output(filename); 
	if (!vrml_output) 
	{
		core_state.error = CORE_ERROR_UNABLE_TO_OPEN_FILE;
		return false; 
	}
	vrml_output.precision(16);

	// write vrml header 
	vrml_output << "#VRML V2.0 utf8" << std::endl; 

	// * export data *
	
	// vertices header
	vrml_output << "Group { children [ Shape { geometry PointSet { coord Coordinate {point [" << std::endl;

	// dump all vertices as point cloud 
	for ALL(*vertices, i) 
	{
		const Calibration_Vertex * vertex = vertices->data + i; 

		const double w = OPENCV_ELEM(vertex->X, 3, 0); 
		if (w != 0) 
		{
			double 
				x = -OPENCV_ELEM(vertex->X, 0, 0) / w,
				y = -OPENCV_ELEM(vertex->X, 1, 0) / w, 
				z = OPENCV_ELEM(vertex->X, 2, 0) / w;
			vrml_output << x << " " << y << " " << z << " " << std::endl; 

			/* 
			// debug - used to indicate plane at infinity in the dataset
			// if the plane at infinity is defined, cover it with some points 
			if (calibration.pi_infinity) 
			{
				double n[3], d = OPENCV_ELEM(calibration.pi_infinity, 3, 0);
				n[0] = OPENCV_ELEM(calibration.pi_infinity, 0, 0);
				n[1] = OPENCV_ELEM(calibration.pi_infinity, 1, 0);
				n[2] = OPENCV_ELEM(calibration.pi_infinity, 2, 0);

				double norm = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
				n[0] /= norm;
				n[1] /= norm;
				n[2] /= norm;
				d /= norm;

				double distance = n[0] * x + n[1] * y + n[2] * z;
				distance = d - distance; 

				x += distance * n[0]; 
				y += distance * n[1]; 
				z += distance * n[2];

				vrml_output << x << " " << y << " " << z << " " << std::endl; 
			}*/
		}
	}

	// vertices footer
	vrml_output << "] } } } ] }" << std::endl; 

	vrml_output.close();
	return true;
}

// exports calibration into RealVIZ exchange format supported by both ImageModeler and MatchMover
bool geometry_export_rzml(const char * filename, Shots & shots)
{
	// open file for output 
	std::ofstream rzml_output(filename); 
	if (!rzml_output) 
	{
		core_state.error = CORE_ERROR_UNABLE_TO_OPEN_FILE;
		return false;
	}

	rzml_output.precision(9);

	// write header 
	rzml_output << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << std::endl; 
	char * s = interface_filesystem_realviz_filename(filename); 
	rzml_output << "<RZML v=\"1.3.0\" app=\"ib3dms\" path=\"" << s << "\">" << std::endl;
	FREE(s);
	rzml_output << "\t<EXPORT ulin=\"cm\"/>" << std::endl;

	// go through all shots and export their data
	bool first = true; 
	size_t realviz_id = 1;
	for ALL(shots, i) 
	{
		const Shot * const shot = shots.data + i; 

		if (first) 
		{
			rzml_output 
				<< "\t<CINF i=\"1\" n=\"Camera 01\" sw=\"" 
				<< shot->width 
				<< "\" sh=\"" 
				<< shot->height 
				<< "\" fbw=\"" 
				<< shot->film_back 
				<< "\" fbh=\"" 
				<< shot->film_back 
				<< "\" fovs=\"k\" fovx=\"" 
				<< shot->fovx
				<< "\"/>" << std::endl;
			first = false; 
		}

		s = interface_filesystem_realviz_filename(shot->image_filename);

		// shot tag
		rzml_output
			<< "\t<SHOT i=\"" << realviz_id << "\" n=\"" << shot->name << "\" ci=\"1\" w=\"" << shot->width << "\" h=\"" << shot->height << "\">" << std::endl;

		// calibration data 
		if (shot->calibrated)
		{
			rzml_output 
				<< "\t\t<CFRM cf=\"1\" fovx=\"" << shot->fovx << "\" pr=\"1.00048852\">" << std::endl
				<< "\t\t\t<T x=\"" << OPENCV_ELEM(shot->translation, 0, 0) << "\" y=\"" << OPENCV_ELEM(shot->translation, 1, 0) << "\" z=\"" << OPENCV_ELEM(shot->translation, 2, 0) << "\"/>" << std::endl
				<< "\t\t\t<R x=\"" << rad2deg(shot->R_euler[0]) << "\" y=\"" << rad2deg(shot->R_euler[1]) << "\" z=\"" << rad2deg(shot->R_euler[2]) << "\"/>" << std::endl
				<< "\t\t</CFRM>" << std::endl;
		}

		// image plane 
		rzml_output
			<< "\t\t<IPLN img=\"" << s << "\">" << std::endl
			<< "\t\t<IFRM/>" << std::endl
			<< "\t\t</IPLN>" << std::endl;

		// finalize shot
		rzml_output << "\t</SHOT>" << std::endl; 
		FREE(s);
		realviz_id++;
	}

	rzml_output << "</RZML>";
	rzml_output.close();

	return true;
}
