#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

#include "libterm.h"

int next_desc = 0;
struct ltm_term_desc * descriptors = 0;

int alloc_tid() {
	int i;

	for(i = 0; i < next_desc; i++)
		if(descriptors[i].pty == NULL) return i;

	/* no unused slots found, make a new one... */
	descriptors = realloc(descriptors, ++next_desc * sizeof(struct ltm_term_desc));

	return next_desc-1;
}

/* errno values:
 * ENODEV: No available PTY devices were found.
 */
int ltm_init_with_shell(char * shell, uint width, uint height) {
	struct ptydev * pty;
	pid_t pid;
	int tid, i;

	/* If this hasn't been set yet, set it
	 * It's a bit of a hack to put this here,
	 * since it doesn't have anything to do
	 * with any specific terminal initialization
	 * I need to put it in the config code when I've
	 * written it.
	 */
	if(!dump_dest) dump_dest = stderr;

	pty = choose_pty_method();
	if(!pty) FIXABLE_LTM_ERR(ENODEV)
	
	pid = spawn(shell, fileno(pty->slave));

	tid = alloc_tid();

	descriptors[tid].pty = pty;

	descriptors[tid].main_screen = malloc(height * sizeof(uint *));
	if(!descriptors[tid].main_screen) FATAL_ERR("malloc", NULL)

	for(i=0; i < height; i++) {
		descriptors[tid].main_screen[i] = malloc_fill(width * sizeof(uint), ' ');
		if(!descriptors[tid].main_screen[i]) FATAL_ERR("malloc", NULL)
	}

	descriptors[tid].width = width;
	descriptors[tid].height = height;

	return tid;
}

int ltm_init(uint width, uint height) {
	struct passwd * pwd_entry;

	errno = 0; /* suggestion from getpwuid(3) */
	pwd_entry = getpwuid(getuid());
	/* I hear that many different things might be returned on a uid not
	 * found, depending on the UNIX system. This would cause problems.
	 * I've put in some of the possible errno values noted in the getpwuid
	 * manpage, but it's probably not all of them. If anyone finds that
	 * their system returns anything other than the values handled here,
	 * let me know.
	 */
	if(!pwd_entry &&
	   (errno == ENOENT || errno == ESRCH || errno == EBADF ||
	    errno == EPERM || errno == 0)) FATAL_ERR("getpwuid", "your current UID")

	return ltm_init_with_shell(pwd_entry->pw_shell, width, height);
}

int ltm_close(uint tid) {
	int i;

	DIE_ON_INVAL_TID(tid)

	free(descriptors[tid].pty);
	descriptors[tid].pty = 0;

	for(i=0; i < descriptors[tid].height; i++)
		free(descriptors[tid].main_screen[i]);

	free(descriptors[tid].main_screen);

	if(tid == next_desc-1)
		descriptors = realloc(descriptors, --next_desc * sizeof(struct ltm_term_desc));

	return 0;
}
