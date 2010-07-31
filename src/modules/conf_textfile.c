#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <libterm_conf_module.h>
#include "error.h"
#include "textparse.h"
#include "config.h"

#define USER_CONFIG ".libterm"

#define PARSE_ERR(file, line, msg) \
	fprintf(DUMP_DEST, "Parse error in %s:%i: %s\n", file, line, msg)

#ifdef WIN32
#  define DLLEXPORT __declspec(dllexport)
#elif defined(HAVE_GCC_VISIBILITY)
#  define DLLEXPORT __attribute__ ((visibility("default")))
#else
#  define DLLEXPORT
#endif

static char *get_user_config_path() {
	char *homedir, *ret;

	homedir = getenv("HOME");
	if(!homedir || !homedir[0]) SYS_ERR_PTR("getenv", "HOME", error);

	/* plus 2 for slash and null terminator */
	ret = malloc(strlen(homedir) + 1 + sizeof(USER_CONFIG)-1 + 1);
	if(!ret) SYS_ERR_PTR("malloc", NULL, error);

	sprintf(ret, "%s/" USER_CONFIG, homedir);

error:
	return ret;
}

/* ok, it's official: text parsing in C is REALLY FUCKING ANNOYING */
static int parse_config_text(char *buf, char *filename) {
	char *class, *module, *name, *val;
	enum ltm_moduleclass classnum;
	int linenum, ret = 0;

	for(linenum = 1; *buf; linenum++) {
		if(*buf == '#' || *buf == '\n') /* comment or empty line */
			goto next;

		/* "fast forward" past horizontal tabs and spaces */
		parse_ff_past_consec(&buf, "\t ");

		/* config key name should come next. each part of it can contain
		 * alphanumerics (upper and lower case) and underscores
		 */
		if(parse_read_consec(&buf, &class, range_a_z range_A_Z range_0_9 "_") == -1)
			SYS_ERR("parse_read_consec", NULL, next);
		if(!class) {
			PARSE_ERR(filename, linenum, "line appears to lack a class name");
			goto next;
		}

		if(*buf != '.') {
			PARSE_ERR(filename, linenum, "no period after class name");
			goto after_class_alloc;
		} else
			buf++;

		if(parse_read_consec(&buf, &module, range_a_z range_A_Z range_0_9 "_") == -1)
			SYS_ERR("parse_read_consec", NULL, after_class_alloc);
		if(!module) {
			PARSE_ERR(filename, linenum, "line appears to lack a module/setting name");
			goto after_class_alloc;
		}

		if(*buf == '.') {
			buf++;
			if(parse_read_consec(&buf, &name, range_a_z range_A_Z range_0_9 "_") == -1)
				SYS_ERR("parse_read_consec", NULL, after_module_alloc);
			if(!name) {
				PARSE_ERR(filename, linenum, "line appears to lack a setting name");
				goto after_module_alloc;
			}
		} else {
			name = module;
			module = NULL;
		}

		parse_ff_past_consec(&buf, "\t ");

		/* there needs to be an equals sign here! */
		if(*buf != '=') {
			/* in the interest of correct error reporting... */
			if(*buf == '\n')
				PARSE_ERR(filename, linenum, "line lacks an = sign and setting value");
			else
				PARSE_ERR(filename, linenum, "line contains garbage after setting name");

			goto after_name_alloc;
		} else
			/* go past the equals sign */
			buf++;

		parse_ff_past_consec(&buf, "\t ");

		if(parse_read_to_without_trailing(&buf, &val, "\n", "\t ") == -1)
			SYS_ERR("parse_read_to_without_trailing", NULL, after_name_alloc);
		if(!val) {
			PARSE_ERR(filename, linenum, "line appears to lack a setting value");
			goto after_name_alloc;
		}

		parse_ff_past_consec(&buf, "\t ");

		/* can this ever be true? */
		if(*buf != '\n') {
			PARSE_ERR(filename, linenum, "line contains garbage after value");
			goto after_val_alloc;
		}

		for(classnum = 0; ltm_classes[classnum]; classnum++)
			if(!strcmp(class, ltm_classes[classnum])) break;

		if(!ltm_classes[classnum]) {
			PARSE_ERR(filename, linenum, "line contains an invalid class name");
			goto after_val_alloc;
		}

		if(ltm_set_config_entry(classnum, module, name, val) == -1)
			PASS_ERR(after_val_alloc);

after_val_alloc:
		free(val);
after_name_alloc:
		free(name);
after_module_alloc:
		if(module) free(module);
after_class_alloc:
		free(class);
next:
		parse_ff_past(&buf, "\n");

		if(ret == -1) break;
	}

	return ret;
}

static int parse_config_file(char *filename) {
	struct stat info;
	FILE *config;
	int ret = 0;
	char *buf;

	config = fopen(filename, "r");
	if(!config) SYS_ERR("fopen", filename, before_anything);

	if(fstat(fileno(config), &info) == -1)
		SYS_ERR("fstat", filename, after_fopen);

	if(!S_ISREG(info.st_mode))
		LTM_ERR(EISDIR, filename, after_fopen); /* FIXME!!! WRONG ERROR! */

	buf = malloc(info.st_size+1);
	if(!buf) SYS_ERR("malloc", NULL, after_fopen);
	buf[info.st_size] = 0;

	if((off_t)fread(buf, 1, info.st_size, config) < info.st_size)
		SYS_ERR("fread", NULL, after_alloc);

	if(memchr(buf, 0, info.st_size))
		LTM_ERR(EILSEQ, "Config file contains null bytes!", after_alloc);

	if(parse_config_text(buf, filename) == -1) PASS_ERR(after_alloc);

after_alloc:
	free(buf);
after_fopen:
	if(fclose(config) == -1) SYS_ERR("fclose", NULL, before_anything);
before_anything:
	return ret;
}

static int config_parse_files() {
	char *user_config_file;
	int ret = 0;

	user_config_file = get_user_config_path();
	if(!user_config_file) PASS_ERR(before_anything);

	if(parse_config_file(GLOBAL_CONFIG) == -1)
		if(ltm_curerr.err_no != ENOENT) PASS_ERR(after_alloc);
	if(parse_config_file(user_config_file) == -1)
		if(ltm_curerr.err_no != ENOENT) PASS_ERR(after_alloc);

after_alloc:
	free(user_config_file);
before_anything:
	return ret;
}

void DLLEXPORT *libterm_module_init() {
	return config_parse_files;
}
