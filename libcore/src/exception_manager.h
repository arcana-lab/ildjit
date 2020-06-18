/*
 * Copyright (C) 2006 Simone Campanoni,  Di Biagio Andrea
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
#ifndef EXCEPTION_MAMAGER_H
#define EXCEPTION_MAMAGER_H

#include <jitsystem.h>

void EXCEPTION_initExceptionManager (void);
void EXCEPTION_allocateOutOfMemoryException (void);
JITBOOLEAN isConstructed ();
void setConstructed (JITBOOLEAN constructed);
void * getCurrentThreadException ();
void setCurrentThreadException (void * new_exception_object);
void updateTheStackTrace ();
void print_stack_trace ();
void throw_thread_exception_ByName (JITINT8 *typeNameSpace, JITINT8 *typeName);
void throw_thread_exception (void *exception_object);
void * get_OutOfMemoryException ();

#endif
