#include "interface_filesystem.h"

// returns file's directory, NULL is returned if filename ends with path separator
char * interface_filesystem_dirpath(const char * const filename)
{
	const size_t len = strlen(filename);

	// check if the last character is path separator 
	if (strchr(FILESYSTEM_PATH_SEPARATORS, filename[len - 1]))
	{
		return NULL; 
	}

	// find last path separator
	const char * last_separator = NULL, * separator = filename - 1;
	while (separator = strpbrk(separator + 1, FILESYSTEM_PATH_SEPARATORS)) 
	{
		last_separator = separator; 
	}

	// finally return the path to directory 
	const size_t separator_offset = last_separator - filename;
	char * dirpath = ALLOC(char, separator_offset + 1);
	memcpy(dirpath, filename, separator_offset);
	dirpath[separator_offset] = '\0';

	return dirpath;
}

// returns file name without basepath 
char * interface_filesystem_extract_filename(const char * filename)
{
	const size_t len = strlen(filename);

	// check if the last character is path separator 
	if (strchr(FILESYSTEM_PATH_SEPARATORS, filename[len - 1]))
	{
		return NULL; 
	}

	// find last path separator
	const char * last_separator = NULL, * separator = filename;
	while (separator = strpbrk(++separator, FILESYSTEM_PATH_SEPARATORS)) 
	{
		last_separator = separator; 
	}

	if (!last_separator)
	{
		const size_t len = strlen(filename); 
		char * p = ALLOC(char, len + 1); 
		memcpy(p, filename, len + 1);
		return p; 
	}

	// copy part of the filename 
	const size_t separator_pos = last_separator - filename;
	const size_t filename_len = len - separator_pos;
	char * extracted = /*(char *)malloc(sizeof(char) * (filename_len + 1));*/ ALLOC(char, filename_len + 1); 
	memcpy(extracted, last_separator + 1, sizeof(char) * (filename_len + 1)); 
	extracted[filename_len] = '\0';
	return extracted;
}

// cleans up "/C:/something/..." to "C:/something/..." if necessary 
char * interface_filesystem_cleanup_windows_filename(const char * filename) 
{
	const size_t len = strlen(filename);
	char * clean_filename;

	if (len >= 3 && filename[0] == '/' && filename[2] == ':') 
	{
		clean_filename = ALLOC(char, len);
		memcpy(clean_filename, filename + 1, len);
	}
	else
	{
		char * str = ALLOC(char, len + 1); 
		memcpy(str, filename, sizeof(char) * (len + 1));
		str[len] = '\0';
		clean_filename = str;
	}

	return clean_filename;
}

// cleans up path file path and converts relative path to absolute
char * interface_filesystem_cleanup_filename(const char * fn, const char * directory)
{
	// cleanup windows filename 
	char * filename = interface_filesystem_cleanup_windows_filename(fn);
	if (!directory) return filename;

	// convert relative path to absolute 
	if (filename[0] == '.') 
	{
		// first take the directory part of the path 
		const size_t directory_len = strlen(directory);
		char * relative_path = filename; 
		const size_t relative_path_len = strlen(relative_path);

		// merge it
		filename = ALLOC(char, (directory_len + 1 + relative_path_len + 1));
		filename[0] = '\0';
		strcat(filename, directory); 
		strcat(filename, "/"); 
		strcat(filename, relative_path);

		// free helper string
		FREE(relative_path);
	}

	return filename; 
}

// compares two filenames based on the name of the file without basepath 
bool interface_filesystem_compare_filenames(const char * f1, const char * f2) 
{
	char * e1 = interface_filesystem_extract_filename(f1), * e2 = interface_filesystem_extract_filename(f2); 
	bool result = strcmp(e1, e2) == 0; 
	FREE(e1); 
	FREE(e2); 
	return result;
}

// adjusts filename for export into RealVIZ file format
char * interface_filesystem_realviz_filename(const char * filename) 
{ 
	// if it's not a unix-style filename, add root at the beginning 
	if (filename[0] != '/') 
	{
		const size_t len = strlen(filename);
		char * modified = ALLOC(char, len + 2);
		modified[0] = '/'; 
		memcpy(modified + 1, filename, len + 1);
		return modified;
	}
	else
	{
		const size_t len = strlen(filename);
		char * copy = ALLOC(char, len + 1); 
		memcpy(copy, filename, (len + 1) * sizeof(char));
		copy[len] = '\0';
		return copy;
	}
} 

// determines if path is absolute or relative 
// note this is dirty...
bool interface_filesystem_is_relative(const char * filename) 
{
	if (!filename) return true; // note really necessary
	return !(filename[0] == '/' || strlen(filename) > 1 && filename[1] == ':');
}
