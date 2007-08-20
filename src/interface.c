#include <string.h>
#include <stdlib.h>

#include "libterm.h"

int ltm_feed_input_to_program(int tid, char * input, uint n) {
	DIE_ON_INVAL_TID(tid)

	if(fwrite(input, 1, n, descriptors[tid].pty.master) < n)
		FATAL_ERR("fwrite", input)

	return 0;
}

int resize_width(int tid, uint width, uint height) {
	uint i, n;

	if(descriptors[tid].width != width)
		for(i = 0; i < height; i++) {
			descriptors[tid].main_screen[i] = realloc(
					descriptors[tid].main_screen[i],
					width * sizeof(uint)
				);
			if(!descriptors[tid].main_screen[i]) FATAL_ERR("realloc", NULL)

			if(width > descriptors[tid].width)
				for(n = descriptors[tid].width; n < width; n++)
					descriptors[tid].main_screen[i][n] = ' ';
		}

	return 0;
}

int ltm_set_window_dimensions(int tid, uint width, uint height) {
	uint i, diff, n;

	DIE_ON_INVAL_TID(tid)

	if(!width || !height) FIXABLE_LTM_ERR(EINVAL)

	if(!descriptors[tid].width || !descriptors[tid].height) {
		descriptors[tid].main_screen = malloc(height * sizeof(uint *));
		if(!descriptors[tid].main_screen) FATAL_ERR("malloc", NULL)

		for(i = 0; i < height; i++) {
			descriptors[tid].main_screen[i] = malloc(width * sizeof(uint));
			if(!descriptors[tid].main_screen[i]) FATAL_ERR("malloc", NULL)

			for(n = 0; n < width; n++)
				descriptors[tid].main_screen[i][n] = ' ';
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
			free(descriptors[tid].main_screen[i]);

/*		for(i = 0; i < height; i++)
			descriptors[tid].main_screen[i] = descriptors[tid].main_screen[i+diff];*/

		memmove(
				&descriptors[tid].main_screen,
				&descriptors[tid].main_screen[diff],
				height * sizeof(uint *)
		       );

		descriptors[tid].main_screen = realloc(
				descriptors[tid].main_screen,
				height * sizeof(uint *)
			);
		if(!descriptors[tid].main_screen) FATAL_ERR("realloc", NULL)

		if(resize_width(tid, width, height) == -1) return -1;
	} else if(height > descriptors[tid].height) {
		diff = height - descriptors[tid].height;

		if(resize_width(tid, width, descriptors[tid].height) == -1) return -1;

		descriptors[tid].main_screen = realloc(
				descriptors[tid].main_screen,
				height * sizeof(uint *)
			);
		if(!descriptors[tid].main_screen) FATAL_ERR("realloc", NULL)

/*		for(i = diff; i < height; i++)
			descriptors[tid].main_screen[i] = descriptors[tid].main_screen[i-diff];*/

		memmove(
				&descriptors[tid].main_screen[diff],
				&descriptors[tid].main_screen,
				descriptors[tid].height * sizeof(uint *)
			);

		for(i = 0; i < diff; i++) {
			descriptors[tid].main_screen[i] = malloc(width * sizeof(uint));
			if(!descriptors[tid].main_screen[i]) FATAL_ERR("malloc", NULL)

			/* probably pop lines off the buffer and put them in here later */
			for(n = 0; n < width; n++)
				descriptors[tid].main_screen[i][n] = ' ';
		}
	} else
		if(resize_width(tid, width, height) == -1) return -1;

	descriptors[tid].height = height;
	descriptors[tid].width = width;

	return 0;
}

int ltm_set_shell(int tid, char * shell) {
	descriptors[tid].shell = strdup(shell);

	if(!descriptors[tid].shell) FATAL_ERR("strdup", shell)

	return 0;
}
