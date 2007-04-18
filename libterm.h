#ifndef LIBTERM_H
#define LIBTERM_H

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define LTM_TRUE  (1)
#define LTM_FALSE (0)

/* length of a character array, allows for
 * 32 character and one null byte
 */
#define CHR_ARR_LEN 33

#include "ltmint.h"
#include "error.h"
#include "ptydev.h"

#endif
