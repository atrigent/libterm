#ifndef MODULE_H
#define MODULE_H

#include <ltdl.h>

#define LTDL_CALL(func, args, label) \
	do { \
		errno = 0; \
		if(lt_dl##func args) SYS_ERR("lt_dl" #func, lt_dlerror(), label); \
	} while(0)

enum moduleclass {
	LIBTERM
};

extern const char *const ltm_classes[];

#endif
