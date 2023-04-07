#include "application.h"

FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }

int main(int argc, char* argv[])
{
	// start, do stuff and finish happily
	return initialization() && main_loop() && release() ? EXIT_SUCCESS : EXIT_FAILURE;
}
