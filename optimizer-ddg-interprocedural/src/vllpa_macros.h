/*
 * Copyright (C) 2008 - 2012 Timothy M. Jones, Simone Campanoni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef VLLPA_MACROS_H
#define VLLPA_MACROS_H

#include "optimizer_ddg.h"

/**
 * Defines to set constants for this pass.
 */
#define UNCONSTRAIN_ANALYSIS
/* #define DEBUG_TIMINGS */

/* The types of checking we want to perform (always disabled without DEBUG). */
/* #define CHECK_UIVS */
/* #define CHECK_ABSTRACT_ADDRESSES */
/* #define CHECK_ABS_ADDR_SETS */

#ifdef UNCONSTRAIN_ANALYSIS
#define CONSTRAIN_UIV_NESTING      JITFALSE
#define CONSTRAIN_ABS_ADDR_OFFSETS JITFALSE
#else
#define CONSTRAIN_UIV_NESTING      JITTRUE
#define CONSTRAIN_ABS_ADDR_OFFSETS JITTRUE
#endif

#define MAX_UIV_NESTING_LEVEL      0
#define MAX_ABS_ADDR_OFFSET        0
#define MIN_ABS_ADDR_OFFSET        0

#define MAX_FREE_SETS 1000

/**
 * Mark a variable as possibly unused.
 */
#define VLLPA_UNUSED __attribute__ ((unused))


/**
 * Convert to a pointer, or back to a different type.
 */
#define toPtr(val) (void *)(JITNUINT)(val)
#define fromPtr(type, val) (type)(JITNUINT)(val)


/**
 * My own debugging system to prevent unnecessary output.
 */
#ifdef PRINTDEBUG
#undef PDEBUG
#undef PDEBUGB
#undef PDEBUGNL
#define PDEBUG(fmt, args...) if (enablePDebug) fprintf(stderr, "DDG_VLLPA: " fmt, ## args)
#define PDEBUGB(fmt, args...) if (enablePDebug) fprintf(stderr, fmt, ## args)
#define PDEBUGNL(fmt, args...) if (enablePDebug) fprintf(stderr, "DDG_VLLPA: \nDDG_VLLPA: " fmt, ## args)
#define PDEBUGLITE(fmt, args...) if (enablePDebug) { PDEBUGNL(fmt, ## args); } else fprintf(stderr, "DDG_VLLPA: " fmt, ## args)
#define PDEBUGDEF(expr) expr
#else
#define PDEBUGLITE(fmt, args...)
#define PDEBUGDEF(expr)
#endif  /* ifdef PRINTDEBUG */


#endif /* VLLPA_MACROS_H */
