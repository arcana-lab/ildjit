/*
 * Copyright (C) 2008 - 2012  Campanoni Simone
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
#include <stdio.h>
#include <lib_array.h>
#include <ildjit.h>

// My headers
#include <internal_calls_object.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

void System_Array_Clear (void *array, JITINT32 index, JITINT32 length) {
    JITUINT32 internalIndex;
    void            *element;

    /* Assertions                   */
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.Clear");

    /*exceptions part*/
    if (array == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
    }
    if ((index < ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0)))  ||
            (length < 0)  ||
            ((index + length) > (((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)) + ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0))))) {

        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "ArgumentOutOfRangeException");
    }
    /*end of exceptions part*/


    for (internalIndex = index; internalIndex < index + length; internalIndex++) {

        element = ((ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(array, internalIndex));

        assert(element != NULL);

        (*(JITNINT *) element) = 0;

    }

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.Clear");
    return;
}

void System_Array_Initialize (void *array) {
    JITUINT32 arrayIRType;

    /* Assertions                   */
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.Initialize");

    arrayIRType = (ildjitSystem->garbage_collectors).gc.getArraySlotIRType(array);

    if (arrayIRType == IRVALUETYPE) {

        // FIXME bisogna chiamare il suo costruttore di defult
        print_err("INTERNAL CALLS: ERROR= Initialization of ValueType arrays is not yet implemented. ", 0);
        abort();
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.Initialize");
    return;
}

void * System_Array_CreateArray (JITNINT elementType, JITINT32 rank, JITINT32 length1, JITINT32 length2, JITINT32 length3) {
    CLRType         *clrType;
    void            *object;
    JITINT32 maxLengthSize;

    METHOD_BEGIN(ildjitSystem, "System.Array.CreateArray");

    /*exceptions part*/
    if ((void *) elementType == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
        abort();
    }

    if ((length1 < 0) || (length2 < 0) || (length3 < 0)) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OverflowException");
        abort();
    } /*end of exceptions part*/

    maxLengthSize = -1;
    if (rank == 1) {
        maxLengthSize = length1;
    } else if (rank == 2) {
        maxLengthSize = length1 * length2;
    } else {
        maxLengthSize = length1 * length2 * length3;
    }
    assert(maxLengthSize >= 0);

    /*multidimension version
       if (rank == 1) {
            size[0] = length1;
       }
       else if (rank == 2) {
            size[0] = length1;
            size[1] = length2;
       }
       else {
            size[0] = length1;
            size[1] = length2;
            size[2] = length3;
       }
       end multidimensional array */

    clrType = (CLRType *) elementType;
    assert(clrType->ID != NULL);
    object = (ildjitSystem->garbage_collectors).gc.allocArray(clrType->ID, rank, maxLengthSize);

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.CreateArray");
    return object;
}

void * System_Array_CreateArrayi (JITNINT elementType, void *lengths, void *lowerBounds) { //FIXME TODO implementare i lowerbounds
    void            *object;
    CLRType         *clrType;
    JITINT32 i;
    JITINT32 rank;
    JITINT32 maxLengthSize = -1;
    JITINT32 maxLowerBound = -1;
    JITINT32 size;
    JITINT32        *length;

    METHOD_BEGIN(ildjitSystem, "System.Array.CreateArrayi");

    /*exceptions part*/
    if (((void *) elementType == NULL) ||  (lengths == NULL) || (lowerBounds == NULL)) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
    }
    if (((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(lengths, 0) < 0)
            || ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(lowerBounds, 0) != (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(lengths, 0))) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "Exception");
    }
    rank = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(lengths, 0);

    for (i = 0; i < rank; i++) {
        length = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(lengths, i);
        assert(length != NULL);
        if ((*length) < 0) {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "OverflowException");
        }

    } /*end of exceptions part*/

    for (i = 0; i < rank; i++) {
        length = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(lengths, i);
        assert(length != NULL);

        if ((*length) > maxLengthSize) {

            maxLengthSize = *length;
        }
    }
    assert(maxLengthSize > 0);


    for (i = 0; i < rank; i++) {
        length = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(lowerBounds, i);
        assert(length != NULL);

        if ((*length) > maxLowerBound) {

            maxLowerBound = *length;
        }
    }
    assert(maxLowerBound > 0);

    size = maxLowerBound + maxLengthSize;

    clrType = (CLRType *) elementType;
    assert(clrType->ID != NULL);
    object = (ildjitSystem->garbage_collectors).gc.allocArray(clrType->ID, rank, size);

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.CreateArrayi");

    return object;
}


JITINT32 System_Array_GetRank (void *object) {
    JITINT32 rank = -1;

    /* Assertions		*/
    assert(object != NULL);

    METHOD_BEGIN(ildjitSystem, "System.Array.GetRank");

    rank = (ildjitSystem->garbage_collectors).gc.getArrayRank(object);
    assert(rank > 0);

    METHOD_END(ildjitSystem, "System.Array.GetRank");
    return rank;
}

JITINT32 System_Array_GetLowerBound (void *object, JITINT32 dimension) {
    JITINT32 	lowerBound;
    t_arrayManager  *arrayManager;

    /* Assertions                   */
    assert(object != NULL);

    /* Fetch the ildjitSystem             */
    METHOD_BEGIN(ildjitSystem, "System.Array.GetLowerBound");

    /* Cache some pointers		*/
    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);

    /*exceptions part*/
    if (    (dimension < 0)                                 ||
            (dimension >= arrayManager->getRank(object))    ) {

        /* Throw the exception			*/
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");

        fprintf(stderr, "The excpetion ildjitSystem does not work\n");
        abort();
    }

    /* Fetch the lower bound	*/
    lowerBound = arrayManager->getArrayLowerBound(object, dimension);
    assert(lowerBound >= 0);

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.GetLowerBound");
    return lowerBound;
}

JITINT32 System_Array_GetUpperBound (void *object, JITINT32 dimension) {
    JITINT32 upperBound = -1;

    /* Assertions                   */
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.GetUpperBound");

    /*exceptions part*/
    if ((dimension < 0) || (dimension >= (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(object, 0))) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    }
    /*end of exceptions part*/


    upperBound = (ildjitSystem->cliManager).CLR.arrayManager.getArrayUpperBound(object, dimension);

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.GetUpperBound");

    return upperBound;


}

void * System_Array_Get (void *array, JITINT32 index1, JITINT32 index2, JITINT32 index3) {
    void            *value;
    JITUINT32 globalIndex;
    JITUINT16 arrayRank;

    /* Assertions                   */
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.Get(int index1, int index2, int index3)");

    /*exceptions part*/
    arrayRank = (ildjitSystem->cliManager).CLR.arrayManager.getRank(array);
    if (    (arrayRank != 3)        &&
            (index2 != 0)           &&
            (index3 != 0)           ) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "Exception");
    }

    if ((index1 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
            || (index1 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0))
            || (index2 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 1) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
            || (index2 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 1))
            || (index3 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 2) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
            || (index3 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 2))) {

        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    }
    /*end of exceptions part*/


    if (    (index2 != 0)   &&
            (index3 != 0)   ) {
        JITUINT32 upperSize1;
        JITUINT32 upperSize2;
        upperSize1 = (ildjitSystem->cliManager).CLR.arrayManager.getArrayUpperBound(array, 0) + 1;
        upperSize2 = (ildjitSystem->cliManager).CLR.arrayManager.getArrayUpperBound(array, 1) + 1;
        //globalIndex   = ((index3 - 1) * upperSize1 * upperSize2) + ((index2 - 1) * upperSize1) + (index1 - 1);
        globalIndex = (index3 * upperSize1 * upperSize2) + (index2 * upperSize1) + index1;
    } else {
        globalIndex = index1;
    }

    value = ((ildjitSystem->cliManager).CLR.arrayManager.getValueArrayElement(array, globalIndex));

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.Get(int index1, int index2, int index3)");
    return value;
}

void * System_Array_Geti (void *array, void *indices) {
    void            *value;
    JITINT32 i = 0;
    JITINT32 rank;
    JITUINT32       *globalIndex;
    JITUINT32       *length;

    /* Assertions                   */
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.Get");

    /*exceptions part*/
    if (indices == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
    }

    if ((ildjitSystem->cliManager).CLR.arrayManager.getRank(array) != (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(indices, 0)) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "Exception");
    }

    rank = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(indices, 0);

    for (i = 0; i < rank; i++) {
        length = (JITUINT32 *) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(indices, i);
        assert(length != NULL);
        if ((*length > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, i) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, i)))
                || *length < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, i)) {

            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        }
    } /*end of exceptions part*/

    if (rank == 1) {
        globalIndex = (JITUINT32 *) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(indices, 0);
        assert(globalIndex != NULL);
        value = ((ildjitSystem->cliManager).CLR.arrayManager.getValueArrayElement(array, *globalIndex));
        assert(value != NULL);
    } else {
        abort();
    }
    //FIXME multidimensional case

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.Get");
    return value;
}

void System_Array_Set (void *array, void *value, JITINT32 index1, JITINT32 index2, JITINT32 index3) {
    JITUINT32 globalIndex;
    JITINT32 rank;

    /* Assertions                   */
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.Set");

    rank = (ildjitSystem->cliManager).CLR.arrayManager.getRank(array);
    assert(rank > 0);

    /*exceptions part*/
    if (rank > 3) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "Exception");
    }
    if (rank == 3) {
        if ((index1 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
                || (index1 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0))
                || (index2 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 1) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
                || (index2 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 1))
                || (index3 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 2) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
                || (index3 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 2))) {

            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        }
    } else if (rank == 2) {
        if ((index1 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
                || (index1 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0))
                || (index2 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 1) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
                || (index2 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 1))
                || (index3 != 0)) {

            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        }
    } else if (rank == 1) {
        if ((index1 > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, 0)))
                || (index1 < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, 0))
                || (index2 != 0) || (index3 != 0)) {
            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        }
    }

    if (rank == 1) {

        globalIndex = index1;
    } else {
        //FIXME multidimensional case
        abort();
    }

    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, globalIndex, value);

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.Set");
    return;
}

void System_Array_Seti (void *array, void *value, void *indices) {
    JITINT32 i = 0;
    JITUINT32 globalIndex;
    JITUINT32       *length;
    JITINT32 rank;

    /* Assertions                   */
    assert(array != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.Set");

    /*exceptions part*/
    if (indices == NULL) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "NullReferenceException");
    }

    if ((ildjitSystem->cliManager).CLR.arrayManager.getRank(array) != (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(indices, 0)) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "Exception");
    }

    rank = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(indices, 0);

    for (i = 0; i < rank; i++) {
        length = (JITUINT32 *) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(indices, i);
        assert(length != NULL);
        if ((*length > ((ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, i) + (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(array, i)))
                || *length < (ildjitSystem->cliManager).CLR.arrayManager.getArrayLowerBound(array, i)) {

            ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        }
    } /*end of exceptions part*/
    if (rank == 1) {
        globalIndex = (*(JITUINT32 *) (ildjitSystem->cliManager).CLR.arrayManager.getArrayElement(indices, 0));
        assert(globalIndex >= 0);
    } else {
        abort();
    }
    //FIXME multidimensional case

    (ildjitSystem->cliManager).CLR.arrayManager.setValueArrayElement(array, globalIndex, value);

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.Set");
    return;
}

JITINT32 System_Array_GetLengthi (void *object, JITINT32 dimension) { //non uso la dimensione essendo monodimensionale per ora
    JITINT32 value;

    /* Assertions		*/
    assert(object != NULL);
    assert(ildjitSystem != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.GetLength(Array object, int dimension)");

    /*exception part*/
    if ((dimension < 0) || dimension >= (ildjitSystem->cliManager).CLR.arrayManager.getRank(object)) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    }
    /*end of exception part*/

    value = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(object, dimension);

    METHOD_END(ildjitSystem, "System.Array.GetLength(Array object, int dimension)");
    return value;
}

JITINT32 System_Array_GetLength (void *object) {
    JITINT32 	value;

    /* Assertions		*/
    assert(object != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.GetLength(Array object)");

    value = (ildjitSystem->cliManager).CLR.arrayManager.getArrayLength(object, 0);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Array.GetLength(Array object)");
    return value;
}

void System_Array_InternalCopy (void *sourceArray, JITINT32 sourceIndex, void *destArray, JITINT32 destIndex, JITINT32 length) {
    JITUINT32 arraySlotSize;
    JITINT32 sourceLowerBound;
    JITINT32 destLowerBound;
    JITUINT32 sourceLength;
    JITUINT32 destLength;
    t_arrayManager          *arrayManager;

    /* Assertions			*/
    assert(sourceArray != NULL);
    assert(destArray != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.InternalCopy");

    /* Cache some pointers		*/
    arrayManager = &((ildjitSystem->cliManager).CLR.arrayManager);

    /* Fetch array information	*/
    sourceLowerBound = arrayManager->getArrayLowerBound(sourceArray, 0);
    sourceLength = arrayManager->getArrayLength(sourceArray, 0);
    destLowerBound = arrayManager->getArrayLowerBound(destArray, 0);
    destLength = arrayManager->getArrayLength(destArray, 0);

    /*exceptions part*/
    if (    (sourceIndex > (sourceLowerBound + sourceLength))       ||
            (sourceIndex < sourceLowerBound)                        ||
            (destIndex > (destLowerBound + destLength))             ||
            (destIndex < destLowerBound)                            ) {

        /* Throw the exception				*/
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    } /*end of exceptions part*/

    /* Fetch the size of each singe	*
     * element of the array		*/
    arraySlotSize = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(sourceArray);
    assert(arraySlotSize > 0);

    /* Copy the array		*/
    if (sourceArray == destArray) {
        memmove((void *) (((JITNUINT) destArray) + (destIndex * arraySlotSize)), (void *) (((JITNUINT) sourceArray) + (sourceIndex * arraySlotSize)), length * arraySlotSize );
    } else {
        memcpy((void *) (((JITNUINT) destArray) + (destIndex * arraySlotSize)), (void *) (((JITNUINT) sourceArray) + (sourceIndex * arraySlotSize)), length * arraySlotSize );
    }

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.InternalCopy");
    return;
}

void * System_Array_GetRelative (void *array, JITINT32 index) {
    void                    * obj;
    JITUINT32 arraySlotSize;
    JITUINT32 arrayLength;

    /* Assertions			*/
    assert(array != NULL);
    assert(ildjitSystem != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.GetRelative");

    /* Initialize the variables	*/
    obj = NULL;

    /* Fetch the size of each singe	*
     * element of the array		*/
    arraySlotSize = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(array);
    assert(arraySlotSize > 0);

    /* Fetch the length of the array*/
    arrayLength = (ildjitSystem->garbage_collectors).gc.getArrayLength(array);
    assert(arrayLength > 0);

    /* Check the out of range	*/
    if (index >= arrayLength) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOufOfRangeException");
    }

    /* Check the type               */
    obj = *((void **) (array + (index * arraySlotSize)));

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.GetRelative");

    return obj;
}

void System_Array_SetRelative (void *array, void *object, JITINT32 index) {
    JITUINT32 arraySlotSize;
    JITUINT32 arraySlotType;
    JITUINT32 arrayLength;

    /* Assertions			*/
    assert(array != NULL);
    assert(ildjitSystem != NULL);
    METHOD_BEGIN(ildjitSystem, "System.Array.SetRelative");

    /* Fetch the type of each       *
     * single element of the array	*/
    arraySlotType = (ildjitSystem->garbage_collectors).gc.getArraySlotIRType(array);

    /* Fetch the size of each singe	*
     * element of the array		*/
    arraySlotSize = (ildjitSystem->cliManager).CLR.arrayManager.getArrayElementSize(array);
    assert(arraySlotSize > 0);

    /* Fetch the length of the array*/
    arrayLength = (ildjitSystem->garbage_collectors).gc.getArrayLength(array);
    assert(arrayLength > 0);

    /* Check the out of range	*/
    if (index >= arrayLength) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System", (JITINT8 *) "IndexOufOfRangeException");
    }

    /* Check the type               */
    if (    (arraySlotType != IROBJECT)
            && (arraySlotType != IRMPOINTER)) {
        print_err("System.Array.SetRelative: ERROR = The type of the element of the array "
                  "is not a reference. ", 0);
        abort();
    }

    /* Store the object		*/
    (*((void **) (array + (index * arraySlotSize)))) = object;

    /* Return                       */
    METHOD_END(ildjitSystem, "System.Array.SetRelative");
}

/* Get the self element at position index */
void* System_Array_GetValueImpl (void* self, JITINT32 position) {
    void* element;

    METHOD_BEGIN(ildjitSystem, "System.Array.GetValueImpl");

    element = System_Array_Get(self, position, 0, 0);

    METHOD_END(ildjitSystem, "System.Array.GetValueImpl");

    return element;
}

/* Store value inside the position element of self */
void System_Array_SetValueImpl (void* self, void* value, JITINT32 position) {

    METHOD_BEGIN(ildjitSystem, "System.Array.SetValueImpl");
    print_err("The internal call System.Array.SetValueImpl "
              "is not yet implemented", 0);
    abort();
    METHOD_END(ildjitSystem, "System.Array.SetValueImpl");
}

/* Performs a shallow copy of self */
void* System_Array_Clone (void* self) {
    void* clone;


    METHOD_BEGIN(ildjitSystem, "System.Array.Clone");

    clone = System_Object_MemberwiseClone(self);

    METHOD_END(ildjitSystem, "System.Array.Clone");

    return clone;
}

/* Erase some elements of the given array */
void System_Array_ClearInternal (void* array, JITINT32 start, JITINT32 count) {

    METHOD_BEGIN(ildjitSystem, "System.Array.ClearInternal");

    System_Array_Clear(array, start, count);

    METHOD_END(ildjitSystem, "System.Array.ClearInternal");

}

/* Copies between two arrays */
JITBOOLEAN System_Array_FastCopy (void* source, JITINT32 sourceStart, void* destination, JITINT32 destinationStart, JITINT32 count) {
    JITBOOLEAN copyDone;


    METHOD_BEGIN(ildjitSystem, "System.Array.FastCopy");

    print_err("The System.Array.FastCopy internal call is not yet implemented",
              0);
    abort();

    METHOD_END(ildjitSystem, "System.Array.FastCopy");

    return copyDone;
}
