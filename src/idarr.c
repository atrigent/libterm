#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "idarr.h"

int idarr_id_alloc(struct idarr **idarr, int size, int *next) {
	int ret;

	for(ret = 0; ret < *next; ret++)
		if(!(*idarr)[ret].allocated)
			break;

	if(ret == *next) {
		*idarr = realloc(*idarr, ++*next * size);
		if(!*idarr) SYS_ERR("realloc", NULL, error);

		memset(&(*idarr)[ret], 0, size);
	}

	(*idarr)[ret].allocated = 1;

error:
	return ret;
}

int idarr_id_dealloc(struct idarr **idarr, int size, int *next, int id) {
	int ret = 0;

	if(id == *next-1) {
		*idarr = realloc(*idarr, --*next * size);
		if(!*idarr && *next) SYS_ERR("realloc", NULL, error);
	} else
		memset(&(*idarr)[id], 0, size);

error:
	return ret;
}
