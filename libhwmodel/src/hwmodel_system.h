#ifndef CACHE_SYSTEM_H
#define CACHE_SYSTEM_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#define DIM_BUF 	1024

#endif
