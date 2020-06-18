/*
 * Copyright (C) 2006 - 2012  Campanoni Simone
 *
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
#ifndef GARBAGE_COLLECTOR_INTERACTIONS_H
#define GARBAGE_COLLECTOR_INTERACTIONS_H

#include <system_manager.h>

void * GC_allocUntracedObject (JITUINT32 overallSize, TypeDescriptor *ilType, void *vTable, IMTElem *IMT);

void * GC_allocUntracedArray (JITUINT32 overallSize, TypeDescriptor *ilType, void *vTable, IMTElem *IMT, JITUINT32 length, JITINT32 isValueType);

void * GC_allocArray (ArrayDescriptor *arrayType, JITINT32 size);

/**
 * @file garbage_collector_interactions.h
 *
 * ILJIT module to manage the garbage collector plugin
 *
 * This module control the access to the garbage collector plugin. It known
 * differences beetween memory areas (e.g. it known if a chunk of bytes are an
 * object or an array). Currently it see an allocated memory zone into 3
 * contiguous blocks:
 *
 *	-# an header of size HEADER_FIXED_SIZE
 *	-# the real object (object fields)
 *	-# an overhead
 *
 * It don't show to the garbage collector plugin this memory organization. For
 * futher details see garbage_collector.h.
 */

/**
 * Initialize garbage collector interactions subsystem
 *
 * Initialize internal data structures and setup links with other subsystems by
 * t_system function pointers.
 */
void setup_garbage_collector_interaction (void);

/**
 * Check if given object is collectable
 *
 * @param object	an IR object (without header) to check
 *
 * @return		JITTRUE if object is collectable, JITFALSE otherwise
 */
JITBOOLEAN isCollectable (void* object);

/**
 * Get object size
 *
 * @param object	the IR object (without header) whose size is unknown
 *
 * @return		given object size
 */
JITNUINT sizeObject (void* object);

/**
 * Get current root set
 *
 * Return a list of void*. Each returned list element is a root reference
 * address.
 *
 * @return	the current root set
 *
 * @note May change
 */
XanList* getRootSet (void);

/**
 * Get the list of reference fields of given object
 *
 * Return a list of void*. Each list element identify a reference field in
 * given object.
 *
 * @param object	an object or an array
 *
 * @return		a list of addresses
 *
 * @note May change
 * @note What happesn when internal GC is enabled an garbage collector plugin free the returned list?
 */
XanList* getReferencedObject (void* object);

/**
 * Get the size of an element of given array
 *
 * @param array		an array
 *
 * @return		the given array slot size
 */
JITUINT32 getArraySlotSize (void* array);

/**
 * Call Finalize method on given object
 *
 * @param object	the object to finalize
 */
void callFinalizer (void* object);

size_t GC_getHeaderSize ();

#endif /* GARBAGE_COLLECTOR_INTERACTIONS_H */
