#ifndef __INTERFACE_FILESYSTEM
#define __INTERFACE_FILESYSTEM

const char * const FILESYSTEM_PATH_SEPARATORS = "\\/"; // note somewhat platform dependent
#ifndef _MSC_VER
const char * const FILESYSTEM_PATH_SEPARATOR = "/";
#else
const char * const FILESYSTEM_PATH_SEPARATOR = "\\";
#endif

#include "core_debug.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "portability.h"

// returns file's directory, NULL is returned if filename ends with path separator
char * interface_filesystem_dirpath(const char * const filename);

// returns file name without basepath 
char * interface_filesystem_extract_filename(const char * filename);

// cleans up "/C:/something/..." to "C:/something/..." if necessary 
char * interface_filesystem_cleanup_windows_filename(const char * filename);

// cleans up path file path and converts relative path to absolute
char * interface_filesystem_cleanup_filename(const char * fn, const char * directory);

// compares two filenames based on the name of the file without basepath 
bool interface_filesystem_compare_filenames(const char * f1, const char * f2);

// adjusts filename for export into RealVIZ file format
char * interface_filesystem_realviz_filename(const char * filename);

// determines if path is absolute or relative 
bool interface_filesystem_is_relative(const char * filename);

#endif
