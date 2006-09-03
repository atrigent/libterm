#include <sys/ioctl.h>
#include <stdio.h>

int alloc_unix98_pty() {
	FILE *master, *slave;
	char slave_path[32]; /* big enough */
	int unlock = 0, err;

	master = fopen("/dev/ptmx", "r+");
	/* This should always be successful - if not, there's nothing anyone
	 * can do about it. */
	if(!master) FATAL_ERR("fopen", "/dev/ptmx")

	err = ioctl(fileno(master), TIOCSPTLCK, &unlock);
	/* all errors from ioctl are weird and unfixable */
	if(err == -1) FATAL_ERR("ioctl", "TIOCSPTLCK")


