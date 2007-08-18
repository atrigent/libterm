#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "libterm.h"

short numlen(unsigned long long num, char base) {
	unsigned long long place, placemax = -1ULL/base;
	short chrs;
	
	for(place = 1, chrs = 0; num; chrs++) {
		if(place > placemax) num = 0;
		else {
			place *= base;
			num -= num % place;
		}
	}

	return chrs;
}

int file_check_type(char * path, char * type, dev_t * dev) {
	struct stat statbuf;

	if(stat(path, &statbuf) == -1) {
		if(errno == ENOENT ||
		   errno == EACCES ||
		   errno == ENOTDIR ||
		   errno == ENAMETOOLONG ||
		   errno == ELOOP) FIXABLE_SYS_ERR("stat", path)
		else FATAL_ERR("stat", path)
	}

	if(statbuf.st_mode & S_IFIFO) *type = 'p';
	else if(statbuf.st_mode & S_IFCHR) *type = 'c';
	else if(statbuf.st_mode & S_IFDIR) *type = 'd';
	else if(statbuf.st_mode & S_IFBLK) *type = 'b';
	else if(statbuf.st_mode & S_IFREG) *type = '-';
	else if(statbuf.st_mode & S_IFLNK) *type = 'l';
	else if(statbuf.st_mode & S_IFSOCK) *type = 's';
	else *type = '?';

	*dev = statbuf.st_rdev;

	return 0;
}

int is_chrdev(char * path, dev_t inpdev) {
	char type;
	dev_t dev;

	if(file_check_type(path, &type, &dev) == -1) return -1;

	if(type == 'c' && dev == inpdev) return 1;
	else return 0;
}

char * get_tmp_dir() {
	char * tmpdir;

	tmpdir = getenv("TMPDIR");
	if(!tmpdir) tmpdir = "/tmp";

	return tmpdir;
}
