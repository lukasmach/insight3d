#ifndef __CORE_STRUCTURES
#define __CORE_STRUCTURES

#include <cstdlib>
#include "pthread.h"
#include "core_debug.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

// macros for smart(er) indexing variables
#define INDEX_DECLARATION(indexing_variable) size_t indexing_variable; bool indexing_variable##_set
#define INDEX_CLEAR(indexing_variable) (indexing_variable) = 0; (indexing_variable##_set) = false
#define INDEX_SET(indexing_variable, value) (indexing_variable) = (value); (indexing_variable##_set) = true
#define INDEX_IS_SET(indexing_variable) (indexing_variable##_set)
#define INDEX_IS_SET_AND_VALID(indexing_variable, indexed_dyn_array) (INDEX_IS_SET(indexing_variable) && IS_SET(indexed_dyn_array, indexing_variable))

// accessing dynamically allocated array 
#define DYN_INIT(variable_name) (dyn_initialize(&(variable_name)))
#define DYN_FREE(variable_name) (dyn_free(&(variable_name)))
#define DYN(variable_name, index) (dyn(&(variable_name), (index)))
#define ADD(variable_name) (DYN((variable_name), ((variable_name).count)))
#define LAST(variable_name) ((variable_name).data[((variable_name).count) - 1])
#define LAST_INDEX(variable_name) (((variable_name).count) - 1)
#define IS_SET(variable_name, index) (((index) < (variable_name).count) && (index >= 0) && ((variable_name).data[(index)].set))
#define ASSERT_IS_SET(variable_name, index) (IS_SET((variable_name), (index)) ? 0 : (printf("%s, %d: accessing undefined index %d in dynamic array", __FILE__, __LINE__, (index)), core_abort()))
#define ALL(variable_name, iterator) (size_t (iterator) = 0; (iterator) < (variable_name).count; ++(iterator)) if ((variable_name).data[(iterator)].set) 
#define LAMBDA_FROM(variable_name, from, iterator, function) for ((iterator) = (from); (iterator) < (variable_name).count; ++(iterator)) if ((variable_name).data[(iterator)].set) { function }
#define LAMBDA(variable, iterator, function) LAMBDA_FROM((variable), 0, (iterator), function)
#define LAMBDA_FIND_FROM(variable, from, iterator, found, condition) \
	(found) = false; \
	LAMBDA_FROM((variable), (from), (iterator), if (condition) { found = true; break; }) \
	if (!(found)) { (iterator) = SIZE_MAX; }
#define LAMBDA_FIND(variable, iterator, found, condition) LAMBDA_FIND_FROM((variable), 0, (iterator), (found), (condition))

// simple routines 
bool dyn_found(const size_t index);

// threading 
extern pthread_mutex_t print_mutex;

#define ATOMIC_DEBUG_IN(resource) // pthread_mutex_lock(&print_mutex); printf("%s\t(%s:%d) ", #resource, __FILE__ + 30, __LINE__); pthread_mutex_unlock(&print_mutex);
#define ATOMIC_DEBUG_OUT() // pthread_mutex_lock(&print_mutex); printf("[*]\n"); pthread_mutex_unlock(&print_mutex);
#define LOCK_RW(resource) ATOMIC_DEBUG_IN(resource); pthread_mutex_lock(&(resource##_mutex)); ATOMIC_DEBUG_OUT();
#define LOCK_R(resource) ATOMIC_DEBUG_IN(resource); pthread_mutex_lock(&(resource##_mutex)); ATOMIC_DEBUG_OUT();
#define LOCK(resource) LOCK_RW(resource)
#define UNLOCK_RW(resource) pthread_mutex_unlock(&(resource##_mutex));
#define UNLOCK_R(resource) pthread_mutex_unlock(&(resource##_mutex));
#define UNLOCK(resource) UNLOCK_RW(resource)
#define UPGRADE_TO_RW(resource)
#define WAS_UNLOCKED_RW(resource) 
#define WAS_UNLOCKED_R(resource)
#define ATOMIC_RW(resource, code) LOCK_RW(resource); code; UNLOCK_RW(resource);
#define ATOMIC_R(resource, code) LOCK_R(resource); code; UNLOCK_R(resource);
#define ATOMIC(resource, code) ATOMIC_RW(resource, code)

// template for safe dynamically allocated array (I for one welcome our new macro overlords...)
#define DYNAMIC_STRUCTURE_DECLARATIONS(structure_name, structure_type) \
struct structure_name { \
	size_t allocated, count; \
	structure_type * data; \
}; \
\
void dyn_initialize(structure_name * dynamic_structure); \
void dyn_free(structure_name * dynamic_structure); \
structure_name * dyn(structure_name * dynamic_structure, size_t index); \
size_t dyn_next(const structure_name & dynamic_structure, const size_t from); \
size_t dyn_first(const structure_name & dynamic_structure); \
size_t * dyn_build_reindex(const structure_name & dynamic_structure);

#define DYNAMIC_STRUCTURE(structure_name, structure_type) \
\
void dyn_initialize(structure_name * dynamic_structure) \
{ \
	dynamic_structure->allocated = 0; \
	dynamic_structure->count = 0; \
	dynamic_structure->data = NULL; \
} \
\
void dyn_free(structure_name * dynamic_structure) \
{ \
	if (dynamic_structure->data) free(dynamic_structure->data); \
	dyn_initialize(dynamic_structure); \
} \
\
structure_name * dyn(structure_name * dynamic_structure, size_t index) \
{ \
\
	if (dynamic_structure->count <= index) dynamic_structure->count = index + 1; \
\
	if (index < dynamic_structure->allocated) \
	{ \
		if (!(dynamic_structure->data[index].set)) \
		{ \
			memset(dynamic_structure->data + index, 0, sizeof(structure_type)); \
		} \
		dynamic_structure->data[index].set = true; \
		return dynamic_structure; \
	} \
	else \
	{ \
		size_t previously_allocated = dynamic_structure->allocated; \
\
		if (dynamic_structure->allocated > 0) \
		{ \
			dynamic_structure->allocated += dynamic_structure->allocated / 2; \
		} \
		else \
		{ \
			dynamic_structure->allocated = 4; \
		} \
\
		structure_type * q; \
		if (NULL == (q = (structure_type *)realloc(dynamic_structure->data, dynamic_structure->allocated * sizeof(structure_type)))) \
		{ \
			dynamic_structure->allocated = 0; \
			dynamic_structure->count = 0; \
			dynamic_structure->data = NULL; \
			return NULL; \
		} \
		else \
		{ \
			dynamic_structure->data = q; \
			memset(dynamic_structure->data + previously_allocated, 0, (dynamic_structure->allocated - previously_allocated) * sizeof(structure_type)); \
			return dyn(dynamic_structure, index); \
		} \
	} \
}\
\
size_t dyn_next(const structure_name & dynamic_structure, const size_t from)\
{\
	for (size_t i = from; i < dynamic_structure.count; ++i) if (dynamic_structure.data[i].set)\
	{\
		return i;\
	}\
\
	return SIZE_MAX;\
}\
\
size_t dyn_first(const structure_name & dynamic_structure)\
{\
	return dyn_next(dynamic_structure, 0);\
}\
\
size_t * dyn_build_reindex(const structure_name & dynamic_structure)\
{\
	size_t * reindex = (size_t *)malloc(sizeof(size_t) * dynamic_structure.count);\
	memset(reindex, 0, sizeof(size_t) * dynamic_structure.count);\
	size_t new_id = 0;\
	for ALL(dynamic_structure, i)\
	{\
		reindex[i] = new_id++;\
	}\
	return reindex;\
}\
\
size_t * dyn_empty_index(const structure_name & dynamic_structure)\
{\
	return (size_t *)malloc(sizeof(size_t) * dynamic_structure.count);\
};

#endif

