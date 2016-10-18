INSIGHT 3D, VERSION 0.3.2
-------------------------

COMPILING UNDEX LINUX
---------------------

To compile insight3d under Linux, you must have the following libraries: 

	- opencv (which is pretty standard computer vision toolkit)
	- opengl
	- SDL
	- freetype2
	- libxml2 (to parse xml files)
	- lapack and blas (to do some math)
	- libgtk+-2.0

And this should be pretty much everything. Some of them have additional 
requirements (opencv needs libpng, libtiff, ...) but these should be trivial. 

Also, pkg-config should know about those libraries. 

Makefile and source code is in the "insight3d" subdirectory.

COMPILING ON WINDOWS
--------------------

Please, refer to the file HOW_TO_BUILD_WINDOWS_VISUALSTUDIO.txt.