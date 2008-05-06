#ifndef LINKEDLIST_H
#define LINKEDLIST_H

struct list_node {
	struct list_node *next;
	struct list_node *prev;

	char which;
	union {
		char *strkey;
		int intkey;
	} u;

	void *data;
	uint len;
};

extern struct list_node *linkedlist_find_int_node(struct list_node *, int);
extern int linkedlist_add_int_node(struct list_node **, int, void *, uint);

extern struct list_node *linkedlist_find_str_node(struct list_node *, char *);
extern int linkedlist_add_str_node(struct list_node **, char *, void *, uint);

extern void linkedlist_del_node(struct list_node **, struct list_node *);
extern void linkedlist_free(struct list_node **);

#endif
