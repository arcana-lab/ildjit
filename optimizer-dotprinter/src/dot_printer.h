/*
 * Copyright (C) 2006  Campanoni Simone
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
#ifndef DOT_PRINTER_H
#define DOT_PRINTER_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

#define PRINT_BASICBLOCK                JITTRUE
#define PRINT_METHOD_NAMES              JITTRUE
#define PRINT_BYTECODE_CLASS_NAMES      JITTRUE
#define PRINT_REACHING_DEFINITIONS      JITFALSE
#define PRINT_AVAILABLE_EXPRESSIONS     JITFALSE
#define PRINT_LIVENESS                  JITFALSE
#define PRINT_EXECUTION_PROBABILITY     JITFALSE
#define PRINT_DDG                       JITFALSE
#define PRINT_CDG                       JITFALSE
#define PRINT_PREDOMINATOR_TREE         JITFALSE
#define PRINT_POSTDOMINATOR_TREE        JITFALSE
#define PRINT_MEMORY_ALIASES            JITFALSE
#define PRINT_LOOPS                     JITFALSE

#endif
