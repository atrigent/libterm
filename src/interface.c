#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "libterm.h"
#include "process.h"

#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

int ltm_feed_input_to_program(int tid, char * input, uint n) {
	DIE_ON_INVAL_TID(tid)

	if(fwrite(input, 1, n, descriptors[tid].pty.master) < n)
		FATAL_ERR("fwrite", input)

	return 0;
}

int resize_width(int tid, ushort width, ushort height, char ** screen) {
	ushort i, n;

	if(descriptors[tid].width != width)
		for(i = 0; i < height; i++) {
			screen[i] = realloc(screen[i], width * sizeof(uint));
			if(!screen[i]) FATAL_ERR("realloc", NULL)

			if(width > descriptors[tid].width)
				for(n = descriptors[tid].width; n < width; n++)
					screen[i][n] = ' ';
		}

	return 0;
}

int set_screen_dimensions(int tid, ushort width, ushort height, char *** screen) {
	char ** dscreen = *screen;
	ushort i, diff, n;

	if(!descriptors[tid].width || !descriptors[tid].height) {
		*screen = dscreen = malloc(height * sizeof(uint *));
		if(!dscreen) FATAL_ERR("malloc", NULL)

		for(i = 0; i < height; i++) {
			dscreen[i] = malloc(width * sizeof(uint));
			if(!dscreen[i]) FATAL_ERR("malloc", NULL)

			for(n = 0; n < width; n++)
				dscreen[i][n] = ' ';
		}
	} else if(height < descriptors[tid].height) {
		/* in order to minimize the work that's done,
		 * here are some orders in which to do things:
		 *
		 * if the height is being decreased:
		 * 	the lines at the top should be freed
		 * 	the lines should be moved up
		 * 	the lines array should be realloced
		 * 	the lines should be realloced (if the width is changing)
		 *
		 * if the height is being increased:
		 * 	the lines should be realloced (if the width is changing)
		 * 	the lines array should be realloced
		 * 	the lines should be moved down
		 * 	the lines at the top should be malloced
		 *
		 * if the height is not changing:
		 * 	the lines should be realloced (if the width is changing)
		 */

		diff = descriptors[tid].height - height;

		for(i = 0; i < diff; i++)
			/* probably push each line into the buffer later */
			free(dscreen[i]);

/*		for(i = 0; i < height; i++)
			dscreen[i] = dscreen[i+diff];*/

		memmove(dscreen, &dscreen[diff], height * sizeof(uint *));

		*screen = dscreen = realloc(dscreen, height * sizeof(uint *));
		if(!dscreen) FATAL_ERR("realloc", NULL)

		if(resize_width(tid, width, height, dscreen) == -1) return -1;
	} else if(height > descriptors[tid].height) {
		diff = height - descriptors[tid].height;

		if(resize_width(tid, width, descriptors[tid].height, dscreen) == -1) return -1;

		*screen = dscreen = realloc(dscreen, height * sizeof(uint *));
		if(!dscreen) FATAL_ERR("realloc", NULL)

/*		for(i = diff; i < height; i++)
			dscreen[i] = dscreen[i-diff];*/

		memmove(&dscreen[diff], dscreen, descriptors[tid].height * sizeof(uint *));

		for(i = 0; i < diff; i++) {
			*screen = dscreen = malloc(width * sizeof(uint));
			if(!dscreen) FATAL_ERR("malloc", NULL)

			/* probably pop lines off the buffer and put them in here later */
			for(n = 0; n < width; n++)
				dscreen[i][n] = ' ';
		}
	} else
		if(resize_width(tid, width, height, dscreen) == -1) return -1;

	return 0;
}

/* a function like this should've been put in POSIX...
 * (minus the libterm-specific stuff, of course)
 */
int tcsetwinsz(int tid) {
	int mfd = fileno(descriptors[tid].pty.master);
	struct winsize size;

	size.ws_row = descriptors[tid].width;
	size.ws_col = descriptors[tid].height;
	size.ws_xpixel = size.ws_ypixel = 0; /* necessary? */

	if(ioctl(mfd, TIOCSWINSZ, &size) == -1) FATAL_ERR("ioctl", NULL)
	
	return 0;
}

int ltm_set_window_dimensions(int tid, ushort width, ushort height) {
	pid_t pgrp;

	DIE_ON_INVAL_TID(tid)

	if(!width || !height) FIXABLE_LTM_ERR(EINVAL)

	/* if not changing anything, just pretend we did something and exit successfully immediately */
	if(width == descriptors[tid].width && height == descriptors[tid].height) return 0;

	if(set_screen_dimensions(tid, width, height, &descriptors[tid].main_screen) == -1) return -1;
	/* probably:
	 * set_screen_dimensions(tid, width, height, &descriptors[tid].alt_screen); 
	 * in the future
	 * ... or more! (who knows?)
	 *
	 * even though everything else is enabled to support an alternate screen,
	 * I don't want to enable this as it's a fairly expensive operation, and
	 * what's the point of enabling it if it's not actually used yet?
	 */

	descriptors[tid].height = height;
	descriptors[tid].width = width;

	/* since the memory for a descriptor gets zero'd out and MAINSCREEN
	 * evaluates to zero, this should work even when cur_screen hasn't
	 * been explicitly set yet
	 *
	 * is it wrong to make an assumption like this? is it ok to perform
	 * unnecessary operations (explicitly setting cur_screen to MAINSCREEN)
	 * in the name of correctness? I don't know :(
	 */
	if(descriptors[tid].cur_screen == MAINSCREEN)
		descriptors[tid].screen = descriptors[tid].main_screen;
	else if(descriptors[tid].cur_screen == ALTSCREEN)
		descriptors[tid].screen = descriptors[tid].alt_screen;
	/* what to do if cur_screen contains an invalid value??? */

	/* only do this if the pty has been created and the shell has been started! */
	if(descriptors[tid].pid) {
		if(tcsetwinsz(tid) == -1) return -1;

		pgrp = tcgetpgrp(fileno(descriptors[tid].pty.master));
		if(pgrp == -1) FATAL_ERR("tcgetpgrp", NULL)

		/* not caring whether this fails or not
		 * it can do so if there's currently no
		 * foreground process group or if we don't
		 * have permission to signal some processes
		 * in the process group
		 */
		killpg(pgrp, SIGWINCH);
	}

	return 0;
}

int ltm_set_shell(int tid, char * shell) {
	DIE_ON_INVAL_TID(tid)

	descriptors[tid].shell = strdup(shell);

	if(!descriptors[tid].shell) FATAL_ERR("strdup", shell)

	return 0;
}

int ltm_get_callbacks_ptr(int tid, struct ltm_callbacks ** cb) {
	DIE_ON_INVAL_TID(tid)

	*cb = &descriptors[tid].cb;

	return 0;
}

int ltm_toggle_threading(int tid) {
	DIE_ON_INVAL_TID(tid)

	descriptors[tid].threading = !descriptors[tid].threading;

	return 0;
}

/* This function is intended to be used in the case that an application needs
 * to call something (another library, for example) that it has no control over
 * and that changes the handler for SIGCHLD after the call to ltm_init. The use
 * of this function in this situation can produce a race condition if not
 * handled carefully. Here's how to manage it:
 *
 * CODE START
 *
 * sigset_t mask;
 *
 * sigemptyset(&mask);
 * sigaddset(&mask, SIGCHLD);
 * 
 * sigprocmask(SIG_BLOCK, &mask, NULL);
 *
 * j_random_function_that_changes_the_sigchld_handler(blaz, blitz, blargh);
 *
 * ltm_reload_sigchld_handler();
 *
 * sigprocmask(SIG_UNBLOCK, &mask, NULL);
 *
 * CODE END
 *
 * This is required because if a shell exits in the gap between the function
 * that changes the handler and the ltm_reload_sigchld_handler call, the SIGCHLD
 * signal will be handled by the handler that just got set rather than libterm's
 * handler. If the child that exited was a shell, this will cause libterm to be
 * completely unaware of this and it will think that this child is still running
 * and that the terminal it was associated with is still valid. Blocking SIGCHLD
 * before the function that sets a new handler and unblocking it after
 * ltm_reload_sigchld_handler solves this problem.
 */
int ltm_reload_sigchld_handler() {
	if(!init_done) FIXABLE_LTM_ERR(EPERM)

	return reload_handler(SIGCHLD, dontfearthereaper);
}

/* If the application has control over the code that sets the new handler, this is the
 * better function to use as it has no race condition problems.
 */
int ltm_set_sigchld_handler(struct sigaction * action) {
	if(!init_done) FIXABLE_LTM_ERR(EPERM)

	return set_handler_struct(SIGCHLD, dontfearthereaper, action);
}

int ltm_get_notifier(FILE ** notifier) {
	if(!init_done) FIXABLE_LTM_ERR(EPERM)

	*notifier = pipefiles[0];

	return 0;
}
