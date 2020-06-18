#ifndef CLIMANAGER_SYSTEM_H
#define CLIMANAGER_SYSTEM_H

// My headers
#include <cli_manager.h>
// End

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#endif
