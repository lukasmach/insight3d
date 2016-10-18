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

#include "core_debug.h"

struct Allocated_Memory 
{
	const char * description, * file, * line;
	void * p;
	size_t effective_size;
	bool active;
};

const size_t core_magic_number = 0xDEADF00D;
const size_t number_of_chunks = 500000;
Allocated_Memory chunks[number_of_chunks];
size_t chunks_count;

bool core_debug_initialize()
{
	memset(chunks, 0, sizeof(Allocated_Memory) * number_of_chunks);
	chunks_count = 0;

	return true;
}

int core_abort()
{
	abort();
	return 1;
}

void debug(const char * message) 
{
	printf("%s\n", message);
}

void * allocate_memory(size_t size, const char * description, const char * file, const char * line) 
{
	// printf("Allocating #%d [%s:%s] %s\n", chunks_count, strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file, line, description);
	const size_t chunk_size = sizeof(size_t) * 501 + size + sizeof(size_t) * 500;
	void * p = malloc(chunk_size);
	memset(p, 0, chunk_size);

	for (size_t i = 1; i < 501; i++) 
	{
		((size_t *)p)[i] = core_magic_number;
	}

	unsigned char * q = (unsigned char *)((size_t *)p + 501);

	for (size_t i = 0; i < size; i++) 
	{
		q[i] = rand() % 256;
	}

	q += size;

	for (size_t i = 0; i < 500; i++) 
	{
		((size_t *)q)[i] = core_magic_number;
		/*if (rand() == 0) // deliberate corruption
		{
			((size_t *)q)[i]++;
		}*/
	}

	chunks[chunks_count].active = true;
	chunks[chunks_count].description = description;
	chunks[chunks_count].file = file;
	chunks[chunks_count].line = line;
	chunks[chunks_count].p = p;
	chunks[chunks_count].effective_size = size;
	*(size_t *)p = chunks_count++;
	return (size_t *)p + 501;
}

void free_memory(void * p) 
{
	size_t * c = (size_t *)p - 501; 
	size_t id = *c;
	// printf("Free #%d ", id);
	// printf("[%s:%s] %s\n", strrchr(chunks[id].file, '\\') ? strrchr(chunks[id].file, '\\') + 1 : chunks[id].file, chunks[id].line, chunks[id].description);

	if (!check_chunk(id))
	{
		report_invalid_chunk(id, "free");
		((char *)NULL)[0] = 1;
		// core_abort();
	}

	chunks[id].active = false;
}

bool check_chunk(const size_t id) 
{
	// check the id is correct 
	if (id >= chunks_count)
	{
		printf("Invalid chunk id.\n");
		return false;
	}

	// the chunk must be active 
	if (!chunks[id].active) 
	{
		printf("Chunk free'd more than once.\n");
		return false; 
	}

	// check the front buffer
	for (size_t i = 1; i < 501; i++) 
	{
		if (((size_t *)chunks[id].p)[i] != core_magic_number)
		{
			printf("Front buffer corrupted.\n");
			return false;
		}
	}

	// check the back buffer 
	char * p = (char *)((size_t *)chunks[id].p + 501);
	p += chunks[id].effective_size;
	for (size_t i = 0; i < 500; i++) 
	{
		if (
			((size_t *)p)[i] != core_magic_number
		)
		{
			printf("Back buffer corrupted.\n");
			return false;
		}
	}

	return true;
}

void check_all_chunks()
{
	printf("--\n"); 
	printf("Checking allocated memory\n"); 

	for (size_t i = 0; i < chunks_count; i++)
	{
		check_chunk(i);
		printf(".");
	}

	printf("\nFinished\n");
}

void report_invalid_chunk(const size_t id, const char * situation) 
{
	printf(
		"WHAT: chunk #%d\nWHEN: on %s\nDESC: %s, %s:%s\n", 
		id, 
		situation, 
		chunks[id].description, 
		chunks[id].file, 
		chunks[id].line
	);
}
