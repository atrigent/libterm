#ifndef LIBTERM_H
#define LIBTERM_H

#include <stdio.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#include "config.h"
#include "error.h"
#include "rangeset.h"
#include "descriptor.h"
#include "threading.h"

#ifdef WIN32
#  define DLLEXPORT __declspec(dllexport)
#elif defined(HAVE_GCC_VISIBILITY)
#  define DLLEXPORT __attribute__ ((visibility("default")))
#else
#  define DLLEXPORT
#endif

enum initstate {
	INIT_NOT_DONE,
	INIT_IN_PROGRESS,
	INIT_DONE
};

#define CHECK_INITED \
	if(init_state != INIT_DONE) LTM_ERR(EPERM, "libterm has not yet been inited", before_anything);

#define CHECK_INITED_PTR \
	if(init_state != INIT_DONE) LTM_ERR_PTR(EPERM, "libterm has not yet been inited", before_anything);

#define CHECK_NOT_INITED \
	if(init_state != INIT_NOT_DONE) LTM_ERR(EPERM, "libterm has been inited", before_anything);

extern enum initstate init_state;

#endif
