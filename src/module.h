#ifndef MODULE_H
#define MODULE_H

#include <ltdl.h>

#define DIE_ON_INVAL_MID(mid) \
	if((mid) >= next_mid || modules[mid].allocated == 0) \
		LTM_ERR(EINVAL, "Invalid MID", error);

#define DIE_ON_INVAL_MID_PTR(mid) \
	if((mid) >= next_mid || modules[mid].allocated == 0) \
		LTM_ERR_PTR(EINVAL, "Invalid MID", error);

#define LTDL_CALL_SAVERET(var, func, args, label) \
	do { \
		errno = 0; \
		var = lt_dl##func args; \
		if(!var) SYS_ERR("lt_dl" #func, lt_dlerror(), label); \
	} while(0)

#define LTDL_CALL_SAVERET_PTR(var, func, args, label) \
	do { \
		errno = 0; \
		var = lt_dl##func args; \
		if(!var) SYS_ERR_PTR("lt_dl" #func, lt_dlerror(), label); \
	} while(0)

#define LTDL_CALL(func, args, label) \
	do { \
		errno = 0; \
		if(lt_dl##func args) SYS_ERR("lt_dl" #func, lt_dlerror(), label); \
	} while(0)

#define MODULE_CALL(ret, func, mid) \
	do { \
		cur_mid = mid; \
		ret = ((modulefunc)(func))(modules[mid].class, modules[mid].name, &modules[mid].private); \
		cur_mid = -1; \
	} while(0)

enum moduleclass {
	LIBTERM,
	CONF
};

typedef void *(*modulefunc)(enum moduleclass, char *, void **);

struct module {
	char allocated;
	int refcount;

	lt_dlhandle handle;

	enum moduleclass class;
	char *name;

	void *entrypoint;
	void *private;
};

extern int module_load(enum moduleclass, char *);
extern int module_unload(int);
extern void *module_get_sym(int, char *);

extern const char *const ltm_classes[];
extern struct module *modules;
extern int next_mid, cur_mid;

#endif
