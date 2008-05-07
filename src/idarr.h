#ifndef IDARR_H
#define IDARR_H

struct idarr {
	char allocated;
};

extern int idarr_id_alloc(struct idarr **, int, int *);
extern int idarr_id_dealloc(struct idarr **, int, int *, int);

#define IDARR_ID_ALLOC(arr, next) \
	idarr_id_alloc((struct idarr **)&arr, sizeof(*arr), &next)

#define IDARR_ID_DEALLOC(arr, next, id) \
	idarr_id_dealloc((struct idarr **)&arr, sizeof(*arr), &next, id)

#endif
