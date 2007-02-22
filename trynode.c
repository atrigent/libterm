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
	"~/.pts/",
	NULL
};

char * bsd_ptmx_paths[] = {
	"/dev/",
	"/tmp/pt/",
	"~/.pt/",
	NULL
};

/* Arguments:
 * orig - the basename of the node file to try and create
 * type - 'c' (for making a character device) or 'b' (for making a block device)
 * device - device major and minor info - created using the makedev() macro
 * paths - see full description
 * path - the return argument
 *
 * So here's how this works:
 * The function loops through each string in paths and tries appending orig to
 * it to create the full node file path. It then tries creating the node. If it
 * can't, it goes to the next element in the paths array.
 *
 * Note that the strings in the paths array can contain part of what will
 * become the basename of the final path - orig is appended to each element of
 * paths no questions asked.
 *
 * Also, if a particular element of paths ends with a /, trynode will attempt
 * to create a directory with that name.
 */
int try_create(char * orig, char type, dev_t device, char * paths[], char * path) {
	int i, created;
	mode_t mode;
	struct stat stat;

	for(i = 0; paths[i]; i++) {
		mode = S_IRUSR | S_IWUSR;
		created = LTM_TRUE;

		if(paths[i][strlen(paths[i])-1] == '/') {
			if(mkdir(paths[i], mode) == -1) {
				if(errno == EACCES ||
				   errno == ENOENT) continue;
				else if(errno == EEXIST) created = LTM_FALSE;
				else FATAL_ERR("mkdir", paths[i])
			}
		}
		else created = LTM_FALSE;
		
		if(type == 'b')      mode |= S_IFBLK;
		else if(type == 'c') mode |= S_IFCHR;
		else FIXABLE_LTM_ERR(EINVAL)

		sprintf(path, "%s%s", paths[i], orig);

		if(mknod(path, mode, device) == -1) {
			if(errno == EACCES ||
			   errno == ENOENT) continue;
			else if(errno == EEXIST) {
				if(is_chrdev(path, device)) break;
				else continue;
			}
			else {
				/* I think it's safe to assume that there's something very
				 * wrong if we can't delete the directory we created a split
				 * second ago, you?
				 */
				if(created && rmdir(paths[i]) == -1) FATAL_ERR("rmdir", paths[i]);
				FATAL_ERR("mknod", path)
			}
		}

		break;
	}

	return LTM_TRUE;
}

int ptmx_try_create(char * path) {
	return try_create("ptmx", 'c', makedev(PTMX_MAJOR, PTMX_MINOR), bsd_ptmx_paths, path);
}

int unix98_slave_try_create(u4 num, char * path) {
	char orig[PTY_PATH_LEN];

	sprintf(orig, "%u", num);

	return try_create(orig, 'c', makedev(136 + num/256, num % 256), unix98_paths, path);
}

int bsd_slave_try_create(u1 chr, u1 num, char * path) {
	char orig[PTY_PATH_LEN];

	sprintf(orig, "tty%c%x", chr, num);

	return try_create(orig, 'c', makedev(3, (chr >= 112 ? chr-112 : chr-86)*16 + num), bsd_ptmx_paths, path);
}

int bsd_master_try_create(u1 chr, u1 num, char * path) {
	char orig[PTY_PATH_LEN];

	sprintf(orig, "pty%c%x", chr, num);

	return try_create(orig, 'c', makedev(2, (chr >= 112 ? chr-112 : chr-86)*16 + num), bsd_ptmx_paths, path);
}
