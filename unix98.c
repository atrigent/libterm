#include <sys/ioctl.h>
#include <stdio.h>

int alloc_unix98_pty(FILE ** omaster, FILE ** oslave) {
	FILE *master, *slave;
	char slave_path[32]; /* big enough */
	int unlock = 0, err, pty_num;

	master = fopen("/dev/ptmx", "r+");
	/* This should always be successful - if not, there's nothing anyone
	 * can do about it. */
	if(!master) FATAL_ERR("fopen", "/dev/ptmx")

	err = ioctl(fileno(master), TIOCSPTLCK, &unlock);
	/* all errors from ioctl are weird and unfixable */
	if(err == -1) FATAL_ERR("ioctl", "TIOCSPTLCK")

	err = ioctl(fileno(master), TIOCGPTN, &pty_num);
	if(err == -1) FATAL_ERR("ioctl", "TIOCGPTN")

	sprintf(slave_path, "/dev/pts/%u", pty_num);

	slave = fopen(slave_path, "r+");
	if(!master) FATAL_ERR("fopen", slave_path)

	*omaster = master;
	*oslave = slave;

	return 1;
}
