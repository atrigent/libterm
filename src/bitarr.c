#include <stdlib.h>
#include <string.h>

#include "libterm.h"

void bitarr_unset_index(uchar * arr, uint index) {
	arr[index/8] &= ~(1 << (7 - (index%8)));
}

void bitarr_set_index(uchar * arr, uint index) {
	arr[index/8] |= (1 << (7 - (index%8)));
}

int bitarr_test_index(uchar * arr, uint index) {
	return arr[index/8] & (1 << (7 - (index%8)));
}

int bitarr_resize(uchar ** arr, uint oldlen, uint newlen) {
	uint i, oldn, newn;
	uchar mask;
	int ret = 0;

	oldn = oldlen/8 + (oldlen % 8 ? 1 : 0);
	newn = newlen/8 + (newlen % 8 ? 1 : 0);

	*arr = realloc(*arr, newn);
	if(!*arr) SYS_ERR("realloc", NULL, error);

	if(newn > oldn)
		for(i = oldn; i < newn; i++) (*arr)[i] = 0;
	else {
		for(i = mask = 0; i < (newlen % 8); i++) mask |= (1<<(8-i));
		(*arr)[newn-1] &= mask;
	}

error:
	return ret;
}

void bitarr_shift_left(uchar * arr, uint len, uint num) {
	uint i, nelems;
	uchar mask;

	if(num > len) num = len;

	nelems = len/8 + (len % 8 ? 1 : 0);

	if(num >= 8) {
		nelems -= num/8;
		memmove(arr, &arr[num/8], nelems);
		memset(&arr[nelems], 0, num/8);
		num -= num/8 * 8; /* x/y*y doesn't always == x unless floats are involved */
	}

	if(!num) return;

	for(i = mask = 0; i < num; i++) mask |= (1<<(8-i));

	for(i = 0; i < nelems-1; i++) {
		arr[i] <<= num;

		arr[i] |= (arr[i+1] & mask) >> (8-num);
	}

	arr[i] <<= num;
}

void bitarr_shift_right(uchar * arr, uint len, uint num) {
	uint i, nelems, initnum;
	uchar mask;

	if(num > len) num = len;

	nelems = len/8 + (len % 8 ? 1 : 0);

	initnum = num;

	if(num >= 8) {
		memmove(&arr[num/8], arr, nelems - (num/8));
		memset(arr, 0, num/8);
		num -= num/8 * 8;
	}

	if(!num) return;

	for(i = mask = 0; i < num; i++) mask |= (1<<i);

	for(i = nelems-1; i > initnum/8; i--) {
		arr[i] >>= num;

		arr[i] |= (arr[i-1] & mask) << (8-num);
	}

	arr[i] >>= num;

	for(i = mask = 0; i < (len % 8); i++) mask |= (1<<(8-i));
	arr[nelems-1] &= mask;
}
