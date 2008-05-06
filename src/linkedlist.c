#include <stdlib.h>
#include <string.h>

#include "libterm.h"
#include "linkedlist.h"

#define INTEGER 1
#define STRING 0

static int add_keyless_node(struct list_node **head, void *data, uint len) {
	struct list_node *new;
	int ret = 0;

	new = malloc(sizeof(struct list_node));
	if(!new) SYS_ERR("malloc", NULL, error);

	new->data = malloc(len);
	if(!new->data) SYS_ERR("malloc", NULL, error);

	new->next = *head;
	new->prev = NULL;

	memcpy(new->data, data, len);
	new->len = len;

	if(*head) (*head)->prev = new;

	*head = new;

error:
	return ret;
}

int linkedlist_add_str_node(struct list_node **head, char *key, void *data, uint len) {
	int ret = 0;

	if(add_keyless_node(head, data, len) == -1) PASS_ERR(error);

	(*head)->which = STRING;
	(*head)->u.strkey = strdup(key);
	if(!(*head)->u.strkey) SYS_ERR("strdup", NULL, error);

error:
	return ret;
}

int linkedlist_add_int_node(struct list_node **head, int key, void *data, uint len) {
	int ret = 0;

	if(add_keyless_node(head, data, len) == -1) PASS_ERR(error);

	(*head)->which = INTEGER;
	(*head)->u.intkey = key;

error:
	return ret;
}

struct list_node *linkedlist_find_str_node(struct list_node *head, char *key) {
	struct list_node *ret;

	for(ret = head; ret; ret = ret->next)
		if(ret->which == STRING && !strcmp(key, ret->u.strkey)) break;

	if(!ret) LTM_ERR_PTR(ESRCH, key, error);

error:
	return ret;
}

struct list_node *linkedlist_find_int_node(struct list_node *head, int key) {
	struct list_node *ret;

	for(ret = head; ret; ret = ret->next)
		if(ret->which == INTEGER && ret->u.intkey == key) break;

	if(!ret) LTM_ERR_PTR(ESRCH, NULL, error);

error:
	return ret;
}

static void free_node(struct list_node *doomed) {
	if(doomed->which == STRING)
		free(doomed->u.strkey);
	free(doomed->data);
	free(doomed);
}

void linkedlist_del_node(struct list_node **head, struct list_node *doomed) {
	if(doomed->next)
		doomed->next->prev = doomed->prev;

	if(doomed->prev)
		doomed->prev->next = doomed->next;
	else /* deleting head node... */
		*head = doomed->next;

	free_node(doomed);
}

void linkedlist_free(struct list_node **head) {
	struct list_node *curnode, *nextnode;

	for(curnode = *head; curnode; curnode = nextnode) {
		nextnode = curnode->next;
		free_node(curnode);
	}

	*head = NULL;
}
