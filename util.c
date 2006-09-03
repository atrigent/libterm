#include "libterm.h"

s2 num_len(u8 num, u1 base) {
	u8 place = base;
	u1 num_chrs = 1;
	
	while(num >= place) {
		if(num_chrs > 19) return -1;
		
		num_chrs++;
		place *= base;
	}

	return num_chrs;
}

int main() {
	return 0;
}
