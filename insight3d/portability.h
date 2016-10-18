
// some compillers don't define SIZE_MAX
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

// comparing strings
#ifndef _MSC_VER
#define strcmpi strcasecmp
#endif

// define LINUX if we're on real operating system
#ifndef _MSC_VER 
#define LINUX
#endif

