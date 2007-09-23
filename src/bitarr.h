#ifndef BITARR_H
#define BITARR_H

extern void bitarr_unset_index(uchar *, uint);
extern void bitarr_set_index(uchar *, uint);
extern int bitarr_test_index(uchar *, uint);

extern void bitarr_shift_left(uchar *, uint, uint);
extern void bitarr_shift_right(uchar *, uint, uint);

extern int bitarr_resize(uchar **, uint, uint);

#endif
