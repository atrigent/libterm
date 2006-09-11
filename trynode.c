#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "libterm.h"

char * unix98_paths[] = {
	"/dev/pty",
	"/tmp/pts/",
	"/tmp/pty",
	"~/.pts/",
	NULL
};

char * bsdpty_ptmx_paths[] = {
	"/tmp/pt/",
	"~/.pt/",
	NULL
};

char * try_create(char * orig, char type, dev_t device, char * paths[]) {
	int i, len, created;
	mode_t mode;
	struct stat stat;
	char path[PTY_PATH_LEN];

	for(i = 0; paths[i]; i++) {
		len = strlen(paths[i]);

		if(paths[i][len-1] == '/') {
			if(mkdir(paths[i], 0) == -1) {
				if(errno == EACCES ||
				   errno == ENOENT ||
				   errno == ENOTDIR) continue;
				else if(errno == EEXIST);
				else FATAL_ERR("mkdir", paths[i])
			}

			created = LTM_TRUE;
		}
		else created = LTM_FALSE;
		
		mode = S_IRUSR|S_IWUSR;
		if(type == 'b')      mode |= S_IFBLK;
		else if(type == 'c') mode |= S_IFCHR;
		else FIXABLE_LTM_ERR(EINVAL)

		sprintf(path, "%s%s", paths[i], orig);

		if(mknod(path, mode, device) == -1) {
			if(errno == EEXIST) {

	}
}
char * ptmx_try_create() {
	char path[PTY_PATH_LEN];

	
