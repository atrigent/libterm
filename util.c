#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libterm.h"

/*
 * Until I write an integer overflow checker, this is good enough.
 * Will act strangely if you give it anything of the max length for
 * that base.
 */
s2 num_len(u8 num, u1 base) {
	u8 place = base;
	s2 num_chrs = 0;
	
	while(num) {
		num -= num % place;
		place *= base;
		num_chrs++;
	}

	return num_chrs;
}

int file_check_type(u1 * path, u1 * type, dev_t * dev) {
	struct stat stat;

	if(stat(path, &stat) == -1) {
		if(errno == ENOENT ||
		   errno == EACCES ||
		   errno == ENOTDIR ||
		   errno == ENAMETOOLONG ||
		   errno == ELOOP) FIXABLE_LTM_ERR("stat", path)
		 else FATAL_ERR("stat", path)
	}

	if(stat.st_mode & S_IFIFO) *type = 'p';
	else if(stat.st_mode & S_IFCHR) *type = 'c';
	else if(stat.st_mode & S_IFDIR) *type = 'd';
	else if(stat.st_mode & S_IFBLK) *type = 'b';
	else if(stat.st_mode & S_IFREG) *type = '-';
	else if(stat.st_mode & S_IFLNK) *type = 'l';
	else if(stat.st_mode & S_IFSOCK) *type = 's';
	else *type = '?';

	*dev = stat.st_rdev;

	return 0;
}

int is_chrdev(u1 * path, dev_t inpdev) {
	u1 type;
	dev_t dev;

	if(file_check_type(path, &type, &dev) == -1) return -1;

	if(type == 'c' && dev == inpdev) return 1;
	else return 0;
}
