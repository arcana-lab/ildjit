/*
 * Copyright (C) 2008  Campanoni Simone
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
#ifndef INTERNAL_CALLS_ARRAY_H
#define INTERNAL_CALLS_ARRAY_H

#include <jitsystem.h>

/****************************************************************************************************************************
*                                               ARRAY									    *
****************************************************************************************************************************/
JITINT32        System_Array_GetRank (void *object);
JITINT32        System_Array_GetLengthi (void *object, JITINT32 dimension);
JITINT32        System_Array_GetLength (void *object);
JITINT32        System_Array_GetLowerBound (void *object, JITINT32 dimension);
void            System_Array_InternalCopy (void *sourceArray, JITINT32 sourceIndex, void *destArray, JITINT32 destIndex, JITINT32 length);
void *          System_Array_GetRelative (void *array, JITINT32 index);
void            System_Array_SetRelative (void *array, void *object, JITINT32 index);
void            System_Array_Clear (void *array, JITINT32 index, JITINT32 length);
void            System_Array_Initialize (void *array);
void *          System_Array_CreateArray (JITNINT elementType, JITINT32 rank, JITINT32 length1, JITINT32 length2, JITINT32 length3);
void *          System_Array_CreateArrayi (JITNINT elementType, void *lengths, void *lowerBounds);
JITINT32        System_Array_GetUpperBound (void *object, JITINT32 dimension);
void *          System_Array_Get (void *array, JITINT32 index1, JITINT32 index2, JITINT32 index3);
void *          System_Array_Geti (void *array, void *indices);
void            System_Array_Set (void *array, void *value, JITINT32 index1, JITINT32 index2, JITINT32 index3);
void            System_Array_Seti (void *array, void *value, void *indices);

/**
 * Get the element at position, without doing any bounds checking
 *
 * This is a Mono internal call.
 *
 * @param self like this in C++
 * @param position the index of the element to retrieve
 *
 * @return the element at the given position
 */
void* System_Array_GetValueImpl (void* self, JITINT32 position);

/**
 * Store value inside self at given position, without any bound checking
 *
 * This is a Mono internal call
 *
 * @param self like this in C++
 * @param value the value to store
 * @param position where store value inside self
 */
void System_Array_SetValueImpl (void* self, void* value, JITINT32 position);

/**
 * Returns a shallow copy of self
 *
 * This is a Mono internal call.
 *
 * @param self like this in C++
 *
 * @return a shallow copy of self
 */
void* System_Array_Clone (void* self);

/**
 * Reset count elements of the given array, starting from the start element
 *
 * This is a Mono internal call.
 *
 * @param start the first array element to erase
 * @param count how many elements erase
 */
void System_Array_ClearInternal (void* array, JITINT32 start, JITINT32 count);

#endif
