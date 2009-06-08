#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "libterm.h"
#include "hashtable.h"
#include "textparse.h"

#define USER_CONFIG ".libterm"

#define PARSE_ERR(file, line, msg) \
	fprintf(DUMP_DEST, "Parse error in %s:%i: %s\n", file, line, msg)

struct list_node *conf[256];

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

static int set_config_entry(char *key, char *val) {
	fprintf(DUMP_DEST, "Setting config entry \"%s\" to value \"%s\"\n", key, val);

	if(hashtable_set_pair(conf, key, val, strlen(val)+1) == -1)
		return -1;

	return 0;
}

/* ok, it's official: text parsing in C is REALLY FUCKING ANNOYING */
static int parse_config_text(char *buf, char *filename) {
	int linenum, ret = 0;
	char *key, *val;

	for(linenum = 1; *buf; linenum++) {
		if(*buf == '#' || *buf == '\n') /* comment or empty line */
			goto next;

		/* "fast forward" past horizontal tabs and spaces */
		parse_ff_past_consec(&buf, "\t ");

		/* config key name should come next. they can contain:
		 * alphanumerics (upper and lower case), underscores, and periods
		 */
		if(parse_read_consec(&buf, &key, range_a_z range_A_Z range_0_9 "_.") == -1)
			PASS_ERR(next);
		if(!key) {
			PARSE_ERR(filename, linenum, "line appears to lack a setting name");
			goto next;
		}

		parse_ff_past_consec(&buf, "\t ");

		/* there needs to be an equals sign here! */
		if(*buf != '=') {
			/* in the interest of correct error reporting... */
			if(*buf == '\n')
				PARSE_ERR(filename, linenum, "line lacks an = sign and setting value");
			else
				PARSE_ERR(filename, linenum, "line contains garbage after setting name");

			goto after_key_alloc;
		} else
			/* go past the equals sign */
			buf++;

		parse_ff_past_consec(&buf, "\t ");

		if(parse_read_to_without_trailing(&buf, &val, "\n", "\t ") == -1)
			PASS_ERR(after_key_alloc);
		if(!val) {
			PARSE_ERR(filename, linenum, "line appears to lack a setting value");
			goto after_key_alloc;
		}

		parse_ff_past_consec(&buf, "\t ");

		/* can this ever be true? */
		if(*buf != '\n') {
			PARSE_ERR(filename, linenum, "line contains garbage after value");
			goto after_val_alloc;
		}

		if(set_config_entry(key, val) == -1)
			PASS_ERR(after_val_alloc);

after_val_alloc:
		free(val);
after_key_alloc:
		free(key);
next:
		parse_ff_past(&buf, "\n");

		if(ret == -1) break;
	}

	return ret;
}

int DLLEXPORT ltm_set_config_entry(char *key, char *val) {
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	if(set_config_entry(key, val) == -1) PASS_ERR(after_lock);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
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

int config_init() {
	memset(conf, 0, sizeof(conf));

	if(config_parse_files() == -1) return -1;

	return 0;
}

char *config_get_entry(char *class, char *module, char *name, char *def) {
	char *hashname, *ret;

	hashname = malloc(strlen(class) + 1 + (module ? strlen(module) + 1 : 0) + strlen(name) + 1);
	if(!hashname) SYS_ERR_PTR("malloc", NULL, error);

	if(module)
		sprintf(hashname, "%s.%s.%s", class, module, name);
	else
		sprintf(hashname, "%s.%s", class, name);

	ret = hashtable_get_value(conf, hashname);
	if(!ret) ret = def;

	free(hashname);

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

void config_free() {
	hashtable_free(conf);
}
