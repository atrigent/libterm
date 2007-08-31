#ifndef LIBTERM_H
#define LIBTERM_H

#include <stdio.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

/* length of a character array, allows for
 * 32 characters and one null byte
 */
#define CHR_ARR_LEN 33

#include "config.h"
#include "error.h"
#include "ptydev.h"
#include "descriptor.h"

#endif
