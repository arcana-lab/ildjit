#ifndef MANFRED_SYSTEM_H
#define MANFRED_SYSTEM_H

// My headers
#include <manfred.h>
// End

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#define DIM_BUF 2048

manfred_t *manfred;

#endif
