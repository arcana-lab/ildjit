/*
 * Copyright (C) 2008 - 2010  Campanoni Simone
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
#ifndef INTERNAL_CALLS_RUNTIME_H
#define INTERNAL_CALLS_RUNTIME_H

#include <jitsystem.h>

/**
 * Initialize the array 'array'
 *
 * This is a Mono internal call.
 *
 * @param array the array to be initialized
 * @param fieldDescription description of an element of the array
 */
void            System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray_ap (void* array, JITNINT fieldDescription);

/**
 * Get the offset from the start of a String object to the first character in
 * the string
 *
 * This is a Mono internal call.
 *
 * @return the byte offset from the start of a String to the first character in
 * the string
 */
JITINT32        System_Runtime_CompilerServices_RuntimeHelpers_get_OffsetToStringData (void);
void            System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray (void *array, void *runtimeHandler);
void 		System_Runtime_CompilerServices_RuntimeHelpers_RunClassConstructor (void *classHandle);

JITINT32        System_Runtime_InteropServices_Marshal_SizeOf (void *t);
void            System_Runtime_InteropServices_Marshal_FreeHGlobal (JITNINT hglobal);
void *          System_Runtime_InteropServices_Marshal_StringToHGlobalAnsi (void *string);
void *          System_Runtime_InteropServices_Marshal_PtrToStringAnsiInternal (JITNINT ptr, JITINT32 len);
void *          System_Runtime_InteropServices_Marshal_AllocHGlobal (JITNINT cb);
void *          System_Runtime_InteropServices_Marshal_ReAllocHGlobal (void *pv, JITNINT cb);
void            System_Runtime_InteropServices_Marshal_WriteIntPtr (JITNINT ptr, JITINT32 ofs, JITNINT val);

#endif
