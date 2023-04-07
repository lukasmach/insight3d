#ifndef __CORE_DEBUG
#define __CORE_DEBUG

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CORE_DEBUG_QUOTE_HELPER(x) #x
#define CORE_DEBUG_QUOTE(x) CORE_DEBUG_QUOTE_HELPER(x)
#define ASSERT(statement, error_message) ((statement) ? 0 : (printf("%s, %d: %s\n", __FILE__, __LINE__, (error_message)), core_abort()))
#define ALLOC(type, count) (type *)malloc(sizeof(type) * (count))
// (type *)allocate_memory(sizeof(type) * (count), CORE_DEBUG_QUOTE(type), CORE_DEBUG_QUOTE(__FILE__), CORE_DEBUG_QUOTE(__LINE__))
// (type *)malloc(sizeof(type) * (count))
#define FREE(p) free(p)
// free_memory(p)

bool core_debug_initialize();
int core_abort();

void debug(const char * message);

void * allocate_memory(size_t size, const char * description, const char * file, const char * line);
void free_memory(void * p);
bool check_chunk(const size_t id);
void check_all_chunks();
void report_invalid_chunk(const size_t id, const char * situation);

#endif

