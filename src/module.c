#include <stdlib.h>
#include <string.h>

#include "libterm.h"
#include "module.h"
#include "idarr.h"

const char DLLEXPORT *const ltm_classes[] = {
	/* The 'libterm' class is actually a pseudoclass -
	 * no module can actually have that class. It is
	 * exclusively for configuration purposes (i.e.
	 * so libterm itself can have configuration
	 * entries).
	 */
	/* 0 */ "libterm",
	/* 1 */ "conf",
	NULL
};

struct module *modules = NULL;
int next_mid = 0;

void *module_get_sym(int mid, char *sym) {
	void *ret;

	DIE_ON_INVAL_MID_PTR(mid);

	LTDL_CALL_SAVERET_PTR(ret, sym, (modules[mid].handle, sym), error);

error:
	return ret;
}

int module_load(enum moduleclass class, char *basename) {
	modulefunc registerfunc;
	lt_dlhandle handle;
	int ret = -1;
	char *path;

	if(class == LIBTERM)
		LTM_ERR(EINVAL, "Modules cannot have class libterm", before_anything);

	for(ret = 0; ret < next_mid; ret++)
		if(modules[ret].allocated && modules[ret].class == class && !strcmp(modules[ret].name, basename)) {
			modules[ret].refcount++;
			goto before_anything;
		}
	
	path = malloc(strlen(ltm_classes[class]) + 1 + strlen(basename) + 1);
	if(!path) SYS_ERR("malloc", NULL, before_anything);

	sprintf(path, "%s_%s", ltm_classes[class], basename);

	LTDL_CALL_SAVERET(handle, openext, (path), after_alloc);

	ret = IDARR_ID_ALLOC(modules, next_mid);
	if(ret == -1) goto after_alloc;

	modules[ret].handle = handle;
	modules[ret].class = class;
	modules[ret].name = strdup(basename);
	if(!modules[ret].name) SYS_ERR("strdup", NULL, after_alloc);
	modules[ret].refcount++;

	registerfunc = module_get_sym(ret, "libterm_module_init");
	if(!registerfunc) PASS_ERR(after_alloc);

	MODULE_CALL(modules[ret].entrypoint, registerfunc, ret);
	if(!modules[ret].entrypoint) LTM_ERR(ENODEV, path, after_alloc);

after_alloc:
	free(path);
before_anything:
	return ret;
}

int module_unload(int mid) {
	int ret = 0;

	DIE_ON_INVAL_MID(mid);

	if(--modules[mid].refcount) return 0;

	LTDL_CALL(close, (modules[mid].handle), error);

	free(modules[mid].name);

	if(IDARR_ID_DEALLOC(modules, next_mid, mid) == -1)
		return -1;

error:
	return ret;
}
