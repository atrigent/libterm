#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "linkedlist.h"

/* stupid hash function */
static uchar hash_function(char *key) {
	uchar hash = 0;
	size_t i;

	for(i = 0; i < strlen(key); i++)
		switch(i%3) {
			case 0:
				hash ^= key[i] << 1;
				break;
			case 1:
				hash ^= key[i];
				break;
			case 2:
				/* ensure logical bit shift... */
				hash ^= (uchar)key[i] >> 1;
				break;
		}

	return hash;
}

int hashtable_set_pair(struct list_node *hashtable[], char *key, void *data, uint len) {
	uchar bucket = hash_function(key);
	struct list_node **head, *node;
	char *databuf;
	int ret = 0;

	head = &hashtable[bucket];
	node = linkedlist_find_str_node(*head, key);

	if(node) {
		free(node->data);

		databuf = malloc(len);
		if(!databuf) SYS_ERR("malloc", NULL, error);

		memcpy(databuf, data, len);

		node->data = databuf;
		node->len = len;
	} else
		if(linkedlist_add_str_node(head, key, data, len) == -1) PASS_ERR(error);

error:
	return ret;
}

int hashtable_del_pair(struct list_node *hashtable[], char *key) {
	struct list_node **head, *node;
	uchar bucket = hash_function(key);

	head = &hashtable[bucket];
	node = linkedlist_find_str_node(*head, key);

	if(!node) return -1;

	linkedlist_del_node(head, node);

	return 0;
}

void *hashtable_get_value(struct list_node *hashtable[], char *key) {
	struct list_node *node;
	uchar bucket = hash_function(key);

	node = linkedlist_find_str_node(hashtable[bucket], key);

	if(node)
		return node->data;
	else
		return NULL;
}

void hashtable_free(struct list_node *hashtable[]) {
	short i;

	for(i = 0; i < 256; i++)
		if(hashtable[i]) linkedlist_free(&hashtable[i]);
}
