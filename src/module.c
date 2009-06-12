#include <string.h>

const char *const ltm_classes[] = {
	/* The 'libterm' class is actually a pseudoclass -
	 * no module can actually have that class. It is
	 * exclusively for configuration purposes (i.e.
	 * so libterm itself can have configuration
	 * entries).
	 */
	/* 0 */ "libterm",
	NULL
};
