#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "linkedlist.h"

int hashtable_set_pair(struct list_node *hashtable[], char *, void *, uint);
int hashtable_del_pair(struct list_node *hashtable[], char *);
void *hashtable_get_value(struct list_node *hashtable[], char *);
void hashtable_free(struct list_node *hashtable[]);

#endif
