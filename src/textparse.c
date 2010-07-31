#include <string.h>
#include <stdlib.h>

unsigned int find_length_without_trailing(char *buf, char *not_trailing, char *search_end) {
	unsigned int len, cur;

	for(cur = 1, len = 0; *buf && !strchr(search_end, buf[0]); buf++, cur++)
		if(!strchr(not_trailing, buf[0])) len = cur;

	return len;
}

/* FAST FORWARD FUNCTIONS */

int parse_ff(char **buf, unsigned int len) {
	*buf += len;

	return len;
}

int parse_ff_past_consec(char **buf, char *accept) {
	return parse_ff(buf, strspn(*buf, accept));
}

int parse_ff_past(char **buf, char *accept) {
	return parse_ff(buf, strcspn(*buf, accept)+1);
}

/* READ FUNCTIONS */

int parse_read(char **buf, char **readbuf, unsigned int len) {
	*readbuf = NULL;

	if(len) {
		*readbuf = malloc(len+1);
		if(!*readbuf)
			return -1;

		memcpy(*readbuf, *buf, len);
		(*readbuf)[len] = 0;

		*buf += len;
	}

	return len;
}

int parse_read_consec(char **buf, char **readbuf, char *accept) {
	return parse_read(buf, readbuf, strspn(*buf, accept));
}

int parse_read_to(char **buf, char **readbuf, char *reject) {
	return parse_read(buf, readbuf, strcspn(*buf, reject));
}

int parse_read_to_without_trailing(char **buf, char **readbuf, char *reject, char *trailing_reject) {
	return parse_read(buf, readbuf, find_length_without_trailing(*buf, trailing_reject, reject));
}
