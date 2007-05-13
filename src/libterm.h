#ifndef LIBTERM_H
#define LIBTERM_H

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

/* length of a character array, allows for
 * 32 characters and one null byte
 */
#define CHR_ARR_LEN 33

#include "error.h"
#include "ptydev.h"
#include "descriptor.h"

#endif
