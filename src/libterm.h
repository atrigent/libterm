#ifndef LIBTERM_H
#define LIBTERM_H

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

/* length of a character array, allows for
 * 32 characters and one null byte
 */
#define CHR_ARR_LEN 33

#include "error.h"
#include "ptydev.h"
#include "descriptor.h"
#include "util.h"

extern int spawn(char *, FILE *);

#endif
