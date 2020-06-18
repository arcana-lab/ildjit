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


#ifndef REACHING_DEFINITIONS_H
#define REACHING_DEFINITIONS_H

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

struct iList {
    JITUINT32 instId;
    struct iList* next;
};
typedef struct iList instructionList;

struct defs {
    JITUINT32 numberOfVariables;
    //a block of memory containing pointers to each list of instruction IDs per variable, which define the variable accessed
    instructionList** variables; //pointer to array of pointers: array contains variable, for each one there is a pointer to its list
};
typedef struct defs definitions;

#endif
