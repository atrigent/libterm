#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "libterm.h>

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

char * try_create(char * orig, char type, int major, int minor, char * paths[]) {
	int i, len, err;
	mode_t mknod_mode;
	char * path

	for(i = 0; paths[i]; i++) {
		path = paths[i];
		len = strlen(path);

		if(path[len-1] == '/')
			if(mkdir(path, 0) == -1) {
				if(errno == EACCES ||
				   errno == ENOENT ||
				   errno == ENOTDIR) continue;
				else if(errno == EEXIST);
				else FATAL_ERR("mkdir", path)
			}
		
		if(type == 'b')      mknod_mode = S_IFBLK;
		else if(type == 'c') mknod_mode = S_IFCHR;
		else FIXABLE_LTM_ERR(EINVAL)

		
	}
}
char * ptmx_try_create() {
	char path[PTY_PATH_LEN];

	
