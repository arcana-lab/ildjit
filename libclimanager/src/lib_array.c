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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <jitsystem.h>
#include <platform_API.h>

// My headers
#include <lib_array.h>
#include <lib_reflect.h>
#include <climanager_system.h>
#include <layout_manager.h>
// End

extern CLIManager_t *cliManager;

static inline void * internal_getArrayElement (void *array, JITUINT32 index);
static inline void internal_setValueArrayElement (void *array, JITUINT32 index, void *element);
static inline void * internal_getValueArrayElement (void *array, JITUINT32 index);
static inline JITINT32 internal_getArrayLowerBound (void *array, JITINT32 dimension);
static inline JITINT32 internal_getArrayUpperBound (void *array, JITINT32 dimension);
static inline JITUINT32 internal_getArrayLength (void *object, JITINT32 dimension);
static inline JITUINT16 internal_getRank (void *object);
static inline void internal_lib_array_initialize (void);
static inline void * internal_getArrayElementsType (void *object);
static inline void internal_array_check_metadata (void);
static inline TypeDescriptor *internal_fillILArrayType ();
static JITBOOLEAN internal_storesPrimitiveTypes (void* array);
static TypeDescriptor *internal_getArrayElementsILType (void* array);
static JITINT32 internal_getArrayLengthInBytes (void* array);
static inline JITINT32 internal_getArrayElementSize (void *array);
static void* internal_newInstanceFromList (XanList* elements, TypeDescriptor* elementType);
static void* internal_newInstanceFromType (TypeDescriptor* elementType, JITUINT32 count);

pthread_once_t arrayMetadataLock = PTHREAD_ONCE_INIT;

void init_arrayManager (t_arrayManager *arrayManager) {

    /* Initialize the functions			*/
    arrayManager->getArrayElement = internal_getArrayElement;
    arrayManager->setValueArrayElement = internal_setValueArrayElement;
    arrayManager->getValueArrayElement = internal_getValueArrayElement;
    arrayManager->getRank = internal_getRank;
    arrayManager->getArrayLowerBound = internal_getArrayLowerBound;
    arrayManager->getArrayUpperBound = internal_getArrayUpperBound;
    arrayManager->getArrayLength = internal_getArrayLength;
    arrayManager->getArrayElementsType = internal_getArrayElementsType;
    arrayManager->fillILArrayType = internal_fillILArrayType;
    arrayManager->initialize = internal_lib_array_initialize;
    arrayManager->storesPrimitiveTypes = internal_storesPrimitiveTypes;
    arrayManager->getArrayElementsILType = internal_getArrayElementsILType;
    arrayManager->getArrayLengthInBytes = internal_getArrayLengthInBytes;
    arrayManager->getArrayElementSize = internal_getArrayElementSize;
    arrayManager->newInstanceFromList = internal_newInstanceFromList;
    arrayManager->newInstanceFromType = internal_newInstanceFromType;


    /* Return					*/
    return;
}

static inline void * internal_getArrayElement (void *array, JITUINT32 index) {
    JITUINT32 arrayLength;
    JITUINT32 arraySlotSize;
    void            *element;

    /* Assertions					*/
    assert(array != NULL);

    PLATFORM_pthread_once(&arrayMetadataLock, internal_array_check_metadata);

    /* Fetch the length of the array		*/
    arrayLength = internal_getArrayLength(array, 0);
    if (index >= arrayLength) {
        return NULL;
    }

    /* Fetch the size of each singe	element of the  *
     * array					*/
    arraySlotSize = internal_getArrayElementSize(array);
    assert(arraySlotSize > 0);

    /* Compute the element				*/
    element = array + (arraySlotSize * index);
    assert(element != NULL);

    /* Return					*/
    return element;
}

static inline void internal_setValueArrayElement (void *array, JITUINT32 index, void *element) {
    JITUINT32 arrayLength;
    JITUINT32 arraySlotSize;
    void            *first;

    /* Assertions					*/
    assert(array != NULL);
    assert(element != NULL);

    PLATFORM_pthread_once(&arrayMetadataLock, internal_array_check_metadata);

    /* Fetch the length of the array		*/
    arrayLength = (cliManager->CLR).arrayManager.getArrayLength(array, 0);
    if ((index < 0) || (index >= arrayLength)) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
        abort();
    }

    /* Fetch the size of each singe	element of the  *
     * array					*/
    arraySlotSize = internal_getArrayElementSize(array);
    assert(arraySlotSize > 0);

    first = (cliManager->CLR).arrayManager.getArrayElement(array, index);
    assert(first != NULL);

    memmove(first, element, arraySlotSize);

    /* Return					*/
    return;
}

static inline void * internal_getValueArrayElement (void *array, JITUINT32 index) {
    JITUINT32 arrayLength;
    void            *value;
    JITUINT32 arraySlotSize;
    JITINT32 offset;
    TypeDescriptor          *arr;
    TypeDescriptor          *elType;
    FieldDescriptor         *a;

    /* Assertions                   */
    assert(array != NULL);

    /* Fetch the length of the array                */
    arrayLength = (cliManager->CLR).arrayManager.getArrayLength(array, 0);
    if ((index < 0) || (index >= arrayLength)) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "IndexOutOfRangeException");
    }

    arr = cliManager->gc->getType(array);
    elType = GET_ARRAY(arr)->type;
    assert(elType != NULL);

    value = cliManager->gc->allocObject(elType, 0);
    arraySlotSize = internal_getArrayElementSize(array);

    a = elType->getFieldFromName(elType, (JITINT8 *) "value_");

    /* Mono BCL loaded */
    if (a == NULL) {
        a = elType->getFieldFromName(elType, (JITINT8 *) "m_value");
    }

    assert(a != NULL);

    offset = cliManager->gc->fetchFieldOffset(a);

    memcpy(value + offset, (cliManager->CLR).arrayManager.getArrayElement(array, index), arraySlotSize);

    return value;
}

static inline JITINT32 internal_getArrayLowerBound (void *object, JITINT32 dimension) {

    /* Assertions           */
    assert(object != NULL);

    // FIXME TODO
    /* Return the dimension *
    * of the array         */
    return 0;
}

static inline JITUINT32 internal_getArrayLength (void *object, JITINT32 dimension) {

    /* Assertions           */
    assert(object != NULL);

    /* Return the dimension *
     *          * of the array         */
    return cliManager->gc->getArrayLength(object);
}

static inline void * internal_getArrayElementsType (void *object) {
    TypeDescriptor *array;
    TypeDescriptor *ilType;
    void                    *objectClass;

    /* Assertions                           */
    assert(object != NULL);
    assert(cliManager->gc->getType != NULL);

    /* Initialize the variables             */
    objectClass = NULL;

    /* Fetch the type of the object given   *
     *          * as incoming argument                 */
    array = cliManager->gc->getType(object);
    assert(array->element_type = ELEMENT_ARRAY);
    ilType = GET_ARRAY(array)->type;
    assert(ilType != NULL);


    /* Fetch the instance of System.Type    *
     *          * of the class of the object given as  *
     *                   * incoming argument                    */
    objectClass = (cliManager->CLR).reflectManager.getType(ilType);
    assert(objectClass != NULL);

    /* Return the System.Type of the object */
    return objectClass;
}

static inline JITUINT16 internal_getRank (void *object) {
    JITUINT16 rank;

    /* Assertions           */
    assert(object != NULL);

    rank = cliManager->gc->getArrayRank(object);
    assert(rank > 0);

    return rank;
}

static inline JITINT32 internal_getArrayUpperBound (void *object, JITINT32 dimension) {
    JITINT32 upperBound = -1;

    /* Assertions                   */
    assert(object != NULL);

    /*FIXME*/

    // upperBound = (* ((JITUINT32 *) cliManager->gc->fetchOverSize(object)) + (dimension * sizeof(JITUINT32)));
    upperBound = (cliManager->CLR).arrayManager.getArrayLength(object, dimension) - 1;
    assert(upperBound > 0);
    /* Return                       */

    return upperBound;
}

static inline TypeDescriptor *internal_fillILArrayType () {

    PLATFORM_pthread_once(&arrayMetadataLock, internal_array_check_metadata);

    return (cliManager->CLR).arrayManager.ArrayType;
}

static inline void internal_array_check_metadata (void) {

    /* Check if we have to load the metadata	*/
    if ((cliManager->CLR).arrayManager.ArrayType == NULL) {


        /* Fetch the System.String type			*/
        (cliManager->CLR).arrayManager.ArrayType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "Array");
    }

    /* Final assertions				*/
    assert((cliManager->CLR).arrayManager.ArrayType != NULL);

    /* Return					*/
    return;
}

static inline void internal_lib_array_initialize (void) {

    /* Initialize the module	*/
    PLATFORM_pthread_once(&arrayMetadataLock, internal_array_check_metadata);

    /* Return			*/
    return;
}

/* Checks whether array stores primitive types */
static JITBOOLEAN internal_storesPrimitiveTypes (void* array) {
    TypeDescriptor *elementsType;

    JITBOOLEAN isPrimitiveType;

    elementsType = internal_getArrayElementsILType(array);
    isPrimitiveType = elementsType->isPrimitive(elementsType);

    return isPrimitiveType;
}

/* Get the ILType of the elements of the array */
static TypeDescriptor *internal_getArrayElementsILType (void* array) {
    t_running_garbage_collector* garbageCollector;
    TypeDescriptor *arr;
    TypeDescriptor *type;

    garbageCollector = (cliManager->gc);

    arr = garbageCollector->getType(array);
    type = GET_ARRAY(arr)->type;

    return type;
}

static inline JITINT32 internal_getArrayElementSize (void *array) {
    ILLayout_manager                *layoutManager;
    TypeDescriptor                          *elementType;
    ILLayout                        *elementLayout;
    JITINT32 size;
    JITINT32 irType;

    layoutManager = &(cliManager->layoutManager);

    if (internal_getRank(array) != 1) {
        print_err("The input array must be monodimensional", 0);
        abort();
    }

    elementType = internal_getArrayElementsILType(array);

    irType = elementType->getIRType(elementType);
    if (irType != IRVALUETYPE) {
        size = getIRSize(irType);
    } else {
        elementLayout = layoutManager->layoutType(layoutManager, elementType);
        size = elementLayout->typeSize;
    }
    assert(size > 0);

    /* Return			*/
    return size;
}

/* Get the array length in bytes */
static JITINT32 internal_getArrayLengthInBytes (void* array) {
    ILLayout_manager* layoutManager;
    t_running_garbage_collector* garbageCollector;
    TypeDescriptor *elementType;
    ILLayout* elementLayout;
    JITINT32 length;

    layoutManager = &(cliManager->layoutManager);
    garbageCollector = cliManager->gc;

    if (internal_getRank(array) != 1) {
        print_err("The input array must be monodimensional", 0);
        abort();
    }

    elementType = internal_getArrayElementsILType(array);
    elementLayout = layoutManager->layoutType(layoutManager, elementType);
    length = garbageCollector->getArrayLength(array) * elementLayout->typeSize;

    return length;
}

/* Build a new array storing the given elements */
static void* internal_newInstanceFromList (XanList* elements, TypeDescriptor* elementType) {
    t_running_garbage_collector* garbageCollector;
    JITINT32 elementCount;
    JITINT32 i;
    XanListItem* current;
    void* array;

    garbageCollector = cliManager->gc;

    elementCount = xanList_length(elements);
    array = garbageCollector->allocArray(elementType, 1, elementCount);

    current = xanList_first(elements);
    for (i = 0; i < elementCount; i++) {
        internal_setValueArrayElement(array, i, &current->data);
        current = current->next;
    }

    return array;
}


/* Builds an empty array with count elements */
static void* internal_newInstanceFromType (TypeDescriptor* elementType, JITUINT32 count) {
    t_running_garbage_collector* garbageCollector;
    void* array;

    garbageCollector = cliManager->gc;

    array = garbageCollector->allocArray(elementType, 1, count);

    return array;
}
