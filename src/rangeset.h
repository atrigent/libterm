#ifndef RANGESET_H
#define RANGESET_H

#define TOPRANGE(set) (set)->ranges[(set)->nranges-1]

struct rangeset {
	struct range **ranges;
	uint nranges;
};

extern int range_add(struct rangeset *);
extern void range_shift(struct rangeset *);
extern int range_finish(struct rangeset *);
extern void range_free(struct rangeset *);

#endif
