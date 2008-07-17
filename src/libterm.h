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
#include "rangeset.h"
#include "descriptor.h"
#include "callbacks.h"
#include "threading.h"

#ifdef WIN32
#  define DLLEXPORT __declspec(dllexport)
#elif defined(HAVE_GCC_VISIBILITY)
#  define DLLEXPORT __attribute__ ((visibility("default")))
#else
#  define DLLEXPORT
#endif

#define INIT_NOT_DONE 0
#define INIT_IN_PROGRESS 1
#define INIT_DONE 2

#define CHECK_INITED \
	if(init_state != INIT_DONE) LTM_ERR(EPERM, "libterm has not yet been inited", before_anything);

#define CHECK_INITED_PTR \
	if(init_state != INIT_DONE) LTM_ERR_PTR(EPERM, "libterm has not yet been inited", before_anything);

#define CHECK_NOT_INITED \
	if(init_state != INIT_NOT_DONE) LTM_ERR(EPERM, "libterm has been inited", before_anything);

extern char init_state;

#endif
