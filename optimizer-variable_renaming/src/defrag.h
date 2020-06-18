/*
 * Copyright (C) 2010  Mastrandrea Daniele
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

/**
 * @file defrag.h
 * @brief ILDJIT Optimizer: variable renaming
 * @author Mastrandrea Daniele
 * @version 0.1.0
 * @date 2010
 **/

#ifndef OPTIMIZER_VARIABLE_RENAMING_H
#define OPTIMIZER_VARIABLE_RENAMING_H

#include "implementation.h"

#define INDENTATION_LEVEL ind_level

#ifdef SUPERDEBUG
#define SDEBUG(fmt, args ...) pretty_fprintf(&INDENTATION_LEVEL, stderr, fmt, ## args)
#else
#define SDEBUG(fmt, args ...)
#endif /* SUPERDEBUG */

#ifdef SUPERDEBUG
#ifndef PRINTDEBUG
#define PRINTDEBUG
#endif  /* PRINTDEBUG */
#endif  /* SUPERDEBUG */

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) pretty_fprintf(&INDENTATION_LEVEL, stderr, fmt, ## args)
#define PP_INDENT() pretty_indent(&INDENTATION_LEVEL)
#define PP_UNINDENT() pretty_unindent(&INDENTATION_LEVEL)
#define PP_MARK() pretty_mark(&INDENTATION_LEVEL)
#define PP_UNMARK() pretty_unmark(&INDENTATION_LEVEL)
#else
#define PDEBUG(fmt, args ...)
#define PP_INDENT()
#define PP_UNINDENT()
#define PP_MARK()
#define PP_UNMARK()
#endif  /* PRINTDEBUG */

#endif  /* OPTIMIZER_VARIABLE_RENAMING_H */