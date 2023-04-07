#include "core_structures.h"
#include <climits>

 pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// simple routines 
bool dyn_found(const size_t index)
{
	return index != SIZE_MAX;
}
