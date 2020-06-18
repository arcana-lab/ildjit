#ifndef ILJITMM_SYSTEM_H
#define ILJITMM_SYSTEM_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#endif
