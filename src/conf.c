#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libterm.h"
#include "hashtable.h"
#include "textparse.h"
#include "module.h"

struct list_node *conf[256];
char conf_init_done = 0;

static char *make_config_entry_name(enum moduleclass classnum, char *module, char *name) {
	const char *class;
	char *ret;

	if(classnum == LIBTERM && module)
		LTM_ERR_PTR(EINVAL, "A config entry of class libterm "
				"cannot have a module name associated with it", error);

	if(classnum != LIBTERM && !module)
		LTM_ERR_PTR(EINVAL, "A config entry of anything other than class "
				"libterm must have a module name associated with it", error);

	class = ltm_classes[classnum];
	ret = malloc(strlen(class) + 1 + (module ? strlen(module) + 1 : 0) + strlen(name) + 1);
	if(!ret) SYS_ERR_PTR("malloc", NULL, error);

	if(module)
		sprintf(ret, "%s.%s.%s", class, module, name);
	else
		sprintf(ret, "%s.%s", class, name);

error:
	return ret;
}

char *config_get_entry(enum moduleclass class, char *module, char *name, char *def) {
	char *hashname, *ret;

	if(!conf_init_done)
		LTM_ERR_PTR(EPERM, "Configuration subsystem has not yet been inited", error);

	hashname = make_config_entry_name(class, module, name);
	if(!hashname) return NULL;

	ret = hashtable_get_value(conf, hashname);
	if(!ret) ret = def;

	free(hashname);

error:
	return ret;
}

char *config_get_mid_entry(int mid, char *name, char *def) {
	char *ret;

	DIE_ON_INVAL_MID_PTR(mid);

	ret = config_get_entry(modules[mid].class, modules[mid].name, name, def);

error:
	return ret;
}

static int set_config_entry(enum moduleclass class, char *module, char *name, char *val) {
	char *hashname;
	int ret = 0;

	if(!conf_init_done)
		LTM_ERR(EPERM, "Configuration subsystem has not yet been inited", before_anything);

	hashname = make_config_entry_name(class, module, name);
	if(!hashname) PASS_ERR(before_anything);

	fprintf(DUMP_DEST, "Setting config entry \"%s\" to value \"%s\"\n", hashname, val);

	if(hashtable_set_pair(conf, hashname, val, strlen(val)+1) == -1)
		PASS_ERR(after_alloc);

after_alloc:
	free(hashname);
before_anything:
	return ret;
}

int DLLEXPORT ltm_set_config_entry(enum moduleclass class, char *module, char *name, char *val) {
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	if(set_config_entry(class, module, name, val) == -1) PASS_ERR(after_lock);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

int config_set_mid_entry(int mid, char *name, char *val) {
	int ret;

	DIE_ON_INVAL_MID(mid);

	ret = set_config_entry(modules[mid].class, modules[mid].name, name, val);

error:
	return ret;
}

int config_interpret_boolean(char *val) {
	int ret;

	/* lol
	 *   -Herb
	 */
	if(!val) LTM_ERR(EINVAL, NULL, error);

	if(
		!strcmp(val, "false") ||
		!strcmp(val, "no") ||
		!strcmp(val, "off") ||
		(strspn(val, "0") == strlen(val))
	  ) ret = 0;
	else if(
		!strcmp(val, "true") ||
		!strcmp(val, "yes") ||
		!strcmp(val, "on") ||
		(strspn(val, range_0_9) == strlen(val))
	       ) ret = 1;
	else LTM_ERR(EINVAL, val, error);

error:
	return ret;
}

int config_free() {
	int i;

	/* Not synchronizing here as was done in the main
	 * libterm uninit function because this function
	 * is not exposed as part of the API.
	 */
	if(!conf_init_done) return 0;

	hashtable_free(conf);

	for(i = 0; i < next_mid; i++)
		if(modules[i].allocated && modules[i].class == CONF)
			if(module_unload(i) == -1)
				return -1;

	conf_init_done = 0;

	return 0;
}

int config_process() {
	char one_worked = 0;
	void *modret;
	int i, ret = 0;

	if(!conf_init_done) return 0;

	hashtable_free(conf);

	for(i = 0; i < next_mid; i++) {
		if(!modules[i].allocated || modules[i].class != CONF)
			continue;

		MODULE_CALL(modret, modules[i].entrypoint, i);
		/* Don't fail here if the module
		 * fails, just go on to the other
		 * modules...
		 */

		if(modret != -1) one_worked = 1;
	}

	/* ...but fail here if no working modules
	 * were found.
	 */
	if(!one_worked)
		LTM_ERR(ENODEV, "No functioning conf modules were found!", error);

error:
	return ret;
}

int config_init() {
	/* Not synchronizing here as was done in the main
	 * libterm init function because this function
	 * is not exposed as part of the API.
	 */
	if(conf_init_done) return 0;

	if(module_load(CONF, DEFAULT_CONF_MODULE) == -1)
		return -1;

	conf_init_done = 1;

	memset(conf, 0, sizeof(conf));
	if(config_process() == -1) return -1;

	return 0;
}
