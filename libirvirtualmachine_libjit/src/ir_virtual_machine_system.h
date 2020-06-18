#ifndef IR_VIRTUAL_MACHINE_SYSTEM_H
#define IR_VIRTUAL_MACHINE_SYSTEM_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#endif
