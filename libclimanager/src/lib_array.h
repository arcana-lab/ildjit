/*
 * Copyright (C) 2008  Simone Campanoni
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
#ifndef LIB_ARRAY_H
#define LIB_ARRAY_H

#include <jitsystem.h>
#include <metadata/metadata_types.h>

/**
 *
 *
 */
typedef struct {
    TypeDescriptor          *ArrayType;

    TypeDescriptor *                (*fillILArrayType)();                           /**< Store the information about the System.Array class		*/
    void *          (*getArrayElement)(void *array, JITUINT32 index);               /**< Return the pointer of the element index of the array; return NULL if index is grater or equal to the length of the array.  */
    void (*setValueArrayElement)(void *array, JITUINT32 index, void *element);      /**< Set a single element into array at specified index.	*/
    void *          (*getValueArrayElement)(void *array, JITUINT32 index);          /**< Return a single element from array at specified index.	*/
    JITINT32 (*getArrayLowerBound)(void *object, JITINT32 dimension);               /**< Return the lower bound of the array at specified dimension	*/
    JITINT32 (*getArrayUpperBound)(void *object, JITINT32 dimension);               /**< Return the upper bound of the array at specified dimension     */
    JITUINT32 (*getArrayLength)(void *object, JITINT32 dimension);                  /**< Return the dimension of the array (only for monodimensional arrays)	*/
    JITUINT16 (*getRank)(void *object);                                             /**< Return the rank of the specified array	*/
    void *          (*getArrayElementsType)(void *object);                          /**< Return the System.Class type of the array's elements	*/
    void (*initialize)(void);                                                       /**< Force the initialization of the module			*/

    /**
     * Check whether given array stores primitive values
     *
     * @param array the array to check
     *
     * @return JITTRUE if array stores primitive types, JITFALSE otherwise
     */
    JITBOOLEAN (*storesPrimitiveTypes)(void* array);

    /**
     * Gets the ILType of the elements stored inside array
     *
     * @param array the array from which get the type of the elements
     *
     * @return the ILType of the elements inside array
     */
    TypeDescriptor * (*getArrayElementsILType)(void* array);

    /**
     * Gets the array length in bytes
     *
     * @param array the array with unknown size
     *
     * @return the length of array in bytes
     */
    JITINT32 (*getArrayLengthInBytes)(void* array);

    /* Return the number of bytes used for each element of the array	*/
    JITINT32 (*getArrayElementSize)(void *array);

    /**
     * Builds a new array storing the given elements
     *
     * @param elements the elements of the new array
     * @param elementType the type of the elements
     *
     * @return an array storing the given elements
     */
    void* (*newInstanceFromList)(XanList * elements, TypeDescriptor * elementType);

    /**
     * Builds a new array with the given size
     *
     * @param elementType the type of all the elements
     * @param count the number of elements to create
     *
     * @return a new empty array
     */
    void* (*newInstanceFromType)(TypeDescriptor * elementType, JITUINT32 count);

} t_arrayManager;

void init_arrayManager (t_arrayManager *arrayManager);

#endif
