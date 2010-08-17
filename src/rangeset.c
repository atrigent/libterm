#include <stdlib.h>

#include "libterm.h"

int range_add(struct rangeset *set) {
	int ret = 0;

	set->ranges = realloc(set->ranges, ++set->nranges * sizeof(struct range *));
	if(!set->ranges) SYS_ERR("realloc", NULL, error);

	TOPRANGE(set) = malloc(sizeof(struct range));
	if(!TOPRANGE(set)) SYS_ERR("malloc", NULL, error);

error:
	return ret;
}

void range_shift(struct rangeset *set) {
	uint i = 0;

	while(i < set->nranges)
		if(!set->ranges[i]->end.y) {
			/* this update has been scrolled off,
			 * free it and rotate the ranges down
			 */
			free(set->ranges[i]);

			--set->nranges;

			memmove(&set->ranges[i], &set->ranges[i+1],
					(set->nranges - i) * sizeof(struct range *));
		} else {
			/* move it down... */
			if(set->ranges[i]->start.y)
				/* if it's not at the top yet, move it one up */
				set->ranges[i]->start.y--;
			else
				/* if it's already at the top, don't move it;
				 * set the X coord to the left bound. Think
				 * about it! :)
				 */
				set->ranges[i]->start.x = set->ranges[i]->leftbound;

			set->ranges[i]->end.y--;

			i++;
		}
}

int range_finish(struct rangeset *set) {
	int ret = 0;

	set->ranges = realloc(set->ranges, (set->nranges+1) * sizeof(struct range *));
	if(!set->ranges) SYS_ERR("realloc", NULL, error);

	set->ranges[set->nranges] = NULL;

error:
	return ret;
}

void range_free(struct rangeset *set) {
	uint i;

	for(i = 0; i < set->nranges; i++)
		free(set->ranges[i]);

	free(set->ranges);
	set->ranges = NULL;
	set->nranges = 0;
}
