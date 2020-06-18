/*
 * Copyright (C) 2009  Simone Campanoni, Luca Rocchini
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

#include <compiler_memory_manager.h>
#include <metadata_types_manager.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>
#include <stdlib.h>
#include <assert.h>
#include <platform_API.h>

#define TYPE_TICKET 0
#define BYREF_TYPE_TICKET 1
#define NAME_TICKET 2
#define COMPLETE_NAME_TICKET 3

#define AUTOMATIC_TYPE_TICKET_COUNT 4

static inline JITINT8 *internalGetSignatureInStringFromPointerDescriptor (PointerDescriptor *pointer, JITBOOLEAN complete);
static inline JITINT8 *internalGetSignatureInStringFromArrayDescriptor (ArrayDescriptor *array, JITBOOLEAN complete);

/* Pointer Description Accessor Method */
JITUINT32 hashPointerDescriptor (PointerDescriptor *pointer) {
    if (pointer == NULL) {
        return 0;
    }
    return (JITUINT32) (pointer->type);
}

JITINT32 equalsPointerDescriptor (PointerDescriptor *key1, PointerDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return key1->type == key2->type;
}

inline PointerDescriptor *newPointerDescriptor (metadataManager_t *manager, TypeDescriptor *type) {
    PointerDescriptor pointer;

    pointer.manager = manager;
    pointer.type = type;
    xanHashTable_lock(manager->pointerDescriptors);
    PointerDescriptor *found = (PointerDescriptor *) xanHashTable_lookup(manager->pointerDescriptors, (void *) &pointer);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(PointerDescriptor));
        found->manager = manager;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(AUTOMATIC_TYPE_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = pointerDescriptorLock;
        found->unlock = pointerDescriptorUnLock;
        found->createDescriptorMetadataTicket = pointerDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = pointerDescriptorBroadcastDescriptorMetadataTicket;
        found->descriptor = NULL;
        found->byRefDescriptor = NULL;
        found->name = NULL;
        found->completeName = NULL;
        found->type = type;
        found->getName = getNameFromPointerDescriptor;
        found->getCompleteName = getCompleteNameFromPointerDescriptor;
        found->containOpenGenericParameter = containOpenGenericParameterFromPointerDescriptor;
        found->getInstance = getInstanceFromPointerDescriptor;
        found->getGenericDefinition = getGenericDefinitionFromPointerDescriptor;
        xanHashTable_insert(manager->pointerDescriptors, (void *) found, (void *) found);
    }
    xanHashTable_unlock(manager->pointerDescriptors);
    return found;
}

inline TypeDescriptor *getTypeDescriptorFromPointerDescriptor (PointerDescriptor *class) {
    if (!WAIT_TICKET(class, TYPE_TICKET)) {
        /* not already cached */
        class->descriptor = newTypeDescriptor(class->manager, ELEMENT_PTR, (void *) class, JITFALSE);
        BROADCAST_TICKET(class, TYPE_TICKET);
    }
    return class->descriptor;
}

inline TypeDescriptor *getByRefTypeDescriptorFromPointerDescriptor (PointerDescriptor *class) {
    if (!WAIT_TICKET(class, BYREF_TYPE_TICKET)) {
        /* not already cached */
        class->byRefDescriptor = newTypeDescriptor(class->manager, ELEMENT_PTR, (void *) class, JITTRUE);
        BROADCAST_TICKET(class, BYREF_TYPE_TICKET);
    }
    return class->byRefDescriptor;
}

static inline JITINT8 *internalGetSignatureInStringFromPointerDescriptor (PointerDescriptor *pointer, JITBOOLEAN complete) {
    JITINT8 *result;
    JITINT8 *type_signatureInString;

    if (complete) {
        type_signatureInString = getCompleteNameFromTypeDescriptor(pointer->type);
    } else {
        type_signatureInString = getNameFromTypeDescriptor(pointer->type);
    }
    size_t length = sizeof(JITINT8) * (STRLEN(type_signatureInString) + 1 + 1);
    result = sharedAllocFunction(length);
    SNPRINTF(result, length, "%s*", type_signatureInString);
    return result;
}

inline JITINT8 *getNameFromPointerDescriptor (PointerDescriptor *pointer) {
    if (!WAIT_TICKET(pointer, NAME_TICKET)) {
        /*not already cached */
        pointer->name = internalGetSignatureInStringFromPointerDescriptor(pointer, JITFALSE);
        BROADCAST_TICKET(pointer, NAME_TICKET);
    }
    return pointer->name;
}

inline JITINT8 *getCompleteNameFromPointerDescriptor (PointerDescriptor *pointer) {
    if (!WAIT_TICKET(pointer, COMPLETE_NAME_TICKET)) {
        /*not already cached */
        pointer->completeName = internalGetSignatureInStringFromPointerDescriptor(pointer, JITTRUE);
        BROADCAST_TICKET(pointer, COMPLETE_NAME_TICKET);
    }
    return pointer->completeName;
}

inline JITBOOLEAN containOpenGenericParameterFromPointerDescriptor (PointerDescriptor *pointer) {
    return containOpenGenericParameterFromTypeDescriptor(pointer->type);
}

inline TypeDescriptor *getInstanceFromPointerDescriptor (PointerDescriptor *type, GenericContainer *container) {
    TypeDescriptor *instType = getInstanceFromTypeDescriptor(type->type, container);
    PointerDescriptor *pointer = newPointerDescriptor(type->manager, instType);

    return getTypeDescriptorFromPointerDescriptor(pointer);
}

inline TypeDescriptor *getGenericDefinitionFromPointerDescriptor (PointerDescriptor *type) {
    TypeDescriptor *genType = getGenericDefinitionFromTypeDescriptor(type->type);
    PointerDescriptor *pointer = newPointerDescriptor(type->manager, genType);

    return getTypeDescriptorFromPointerDescriptor(pointer);
}

inline void pointerDescriptorLock (PointerDescriptor *type) {
    PLATFORM_lockMutex(&(type->mutex));
}

inline void pointerDescriptorUnLock (PointerDescriptor *type) {
    PLATFORM_unlockMutex(&(type->mutex));
}

inline DescriptorMetadataTicket *pointerDescriptorCreateDescriptorMetadataTicket (PointerDescriptor *pointer, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(pointer->metadata), type);
}

inline void pointerDescriptorBroadcastDescriptorMetadataTicket (PointerDescriptor *type,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}


/* Array Descriptor Accessor Method */
JITUINT32 hashArrayDescriptor (ArrayDescriptor *array) {
    if (array == NULL) {
        return 0;
    }
    JITUINT32 seed = (JITUINT32) array->type;
    seed = combineHash(seed, array->numSizes);
    seed = combineHash(seed, array->numBounds);
    seed = combineHash(seed, array->rank);
    JITUINT32 count;
    for (count = 0; count < array->numSizes; count++) {
        seed = combineHash(seed, array->sizes[count]);
    }
    for (count = 0; count < array->numBounds; count++) {
        seed = combineHash(seed, array->bounds[count]);
    }
    return seed;
}

JITINT32 equalsArrayDescriptor (ArrayDescriptor *key1, ArrayDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->rank != key2->rank) {
        return 0;
    }
    if (key1->numSizes != key2->numSizes) {
        return 0;
    }
    if (key1->numBounds != key2->numBounds) {
        return 0;
    }
    if (key1->type != key2->type) {
        return 0;
    }
    if (key1->numSizes > 0) {
        if (memcmp(key1->sizes, key2->sizes, sizeof(JITUINT32) * key1->numSizes) != 0) {
            return 0;
        }
    }
    if (key1->numBounds > 0) {
        if (memcmp(key1->bounds, key2->bounds, sizeof(JITUINT32) * key1->numBounds) != 0) {
            return 0;
        }
    }
    return 1;
}

inline ArrayDescriptor *newArrayDescriptor (metadataManager_t *manager, TypeDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds) {
    ArrayDescriptor array;

    array.manager = manager;
    array.type = type;
    array.rank = rank;
    array.numSizes = numSizes;
    array.sizes = sizes;
    array.numBounds = numBounds;
    array.bounds = bounds;
    xanHashTable_lock(manager->arrayDescriptors);
    ArrayDescriptor *found = (ArrayDescriptor *) xanHashTable_lookup(manager->arrayDescriptors, (void *) &array);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(ArrayDescriptor));
        found->manager = manager;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(AUTOMATIC_TYPE_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = arrayDescriptorLock;
        found->unlock = arrayDescriptorUnLock;
        found->createDescriptorMetadataTicket = arrayDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = arrayDescriptorBroadcastDescriptorMetadataTicket;
        found->descriptor = NULL;
        found->byRefDescriptor = NULL;
        found->name = NULL;
        found->completeName = NULL;
        found->type = type;
        found->rank = rank;

        found->numSizes = numSizes;
        if (numSizes > 0) {
            found->sizes = sharedAllocFunction(sizeof(JITUINT32) * numSizes);
            memcpy(found->sizes, sizes, sizeof(JITUINT32) * numSizes);
        } else {
            found->sizes = NULL;
        }

        found->numBounds = numBounds;
        if (numBounds > 0) {
            found->bounds = sharedAllocFunction(sizeof(JITUINT32) * numBounds);
            memcpy(found->bounds, bounds, sizeof(JITUINT32) * numBounds);
        } else {
            found->bounds = NULL;
        }

        found->getTypeDescriptor = getTypeDescriptorFromArrayDescriptor;
        found->getByRefTypeDescriptor = getByRefTypeDescriptorFromArrayDescriptor;
        found->getName = getNameFromArrayDescriptor;
        found->getCompleteName = getCompleteNameFromArrayDescriptor;
        found->getTypeNameSpace = getTypeNameSpaceFromArrayDescriptor;
        found->containOpenGenericParameter = containOpenGenericParameterFromArrayDescriptor;
        found->getInstance = getInstanceFromArrayDescriptor;
        found->getGenericDefinition = getGenericDefinitionFromArrayDescriptor;
        found->getTypeNameSpace = getTypeNameSpaceFromArrayDescriptor;
        xanHashTable_insert(manager->arrayDescriptors, (void *) found, (void *) found);
    }
    xanHashTable_unlock(manager->arrayDescriptors);
    return found;
}

inline TypeDescriptor *getTypeDescriptorFromArrayDescriptor (ArrayDescriptor *class) {
    if (!WAIT_TICKET(class, TYPE_TICKET)) {
        /* not already cached */
        assert(class->descriptor == NULL);
        class->descriptor = newTypeDescriptor(class->manager, ELEMENT_ARRAY, (void *) class, JITFALSE);
        BROADCAST_TICKET(class, TYPE_TICKET);
    }
    return class->descriptor;
}

TypeDescriptor *getByRefTypeDescriptorFromArrayDescriptor (ArrayDescriptor *class) {
    if (!WAIT_TICKET(class, BYREF_TYPE_TICKET)) {
        /* not already cached */
        class->byRefDescriptor = newTypeDescriptor(class->manager, ELEMENT_ARRAY, (void *) class, JITTRUE);
        BROADCAST_TICKET(class, BYREF_TYPE_TICKET);
    }
    return class->byRefDescriptor;
}

inline JITBOOLEAN ArrayDescriptorAssignableTo (ArrayDescriptor *type_to_cast, ArrayDescriptor *desired_type) {
    if (type_to_cast->rank != desired_type->rank) {
        return JITFALSE;
    }
    /* TODO:
     * check: bounds, sizes
     */
    if (isValueTypeTypeDescriptor(type_to_cast->type) != isValueTypeTypeDescriptor(desired_type->type)) {
        return JITFALSE;
    }
    return typeDescriptorAssignableTo(type_to_cast->type, desired_type->type);
}


inline JITINT8 *getTypeNameSpaceFromArrayDescriptor (ArrayDescriptor *array) {
    return getTypeNameSpaceFromTypeDescriptor(array->type);
}

static inline JITINT8 *internalGetSignatureInStringFromArrayDescriptor (ArrayDescriptor *array, JITBOOLEAN complete) {
    JITINT8 	*result;
    JITINT8 	*type_signatureInString;
    JITUINT32	type_signatureInStringLength;
    JITUINT32 	length;
    JITUINT32	currentPosition;

    if (complete) {
        type_signatureInString = getCompleteNameFromTypeDescriptor(array->type);
    } else {
        type_signatureInString = getNameFromTypeDescriptor(array->type);
    }
    type_signatureInStringLength	= STRLEN(type_signatureInString);
    length				= type_signatureInStringLength + 1;
    length 				+= 2;
    if (array->rank != 0) {
        length				+= (2 * (array->rank - 1));
    }

    result 		= (JITINT8 *) sharedAllocFunction(length);
    currentPosition	= type_signatureInStringLength + 1;
    SNPRINTF(result, length, "%s[", type_signatureInString);
    if (array->rank > 1) {
        JITUINT32 	count;
        for (count = 0; count < (array->rank - 1); count++) {
            STRCAT(&(result[currentPosition]), ", ");
            currentPosition	+= 2;
        }
    }
    STRCAT(&(result[currentPosition]), "]");

    return result;
}

inline JITINT8 *getNameFromArrayDescriptor (ArrayDescriptor *array) {
    if (!WAIT_TICKET(array, NAME_TICKET)) {
        /*not already cached */
        array->name = internalGetSignatureInStringFromArrayDescriptor(array, JITFALSE);
        BROADCAST_TICKET(array, NAME_TICKET);
    }
    return array->name;
}

inline JITINT8 *getCompleteNameFromArrayDescriptor (ArrayDescriptor *array) {
    if (!WAIT_TICKET(array, COMPLETE_NAME_TICKET)) {
        /*not already cached */
        array->completeName = internalGetSignatureInStringFromArrayDescriptor(array, JITTRUE);
        BROADCAST_TICKET(array, COMPLETE_NAME_TICKET);
    }
    return array->completeName;
}

inline BasicAssembly *getAssemblyFromArrayDescriptor (ArrayDescriptor *array) {
    return getAssemblyFromTypeDescriptor(array->type);
}


inline JITBOOLEAN containOpenGenericParameterFromArrayDescriptor (ArrayDescriptor *array) {
    return containOpenGenericParameterFromTypeDescriptor(array->type);
}

inline TypeDescriptor *getInstanceFromArrayDescriptor (ArrayDescriptor *type, GenericContainer *container) {
    TypeDescriptor *instType = getInstanceFromTypeDescriptor(type->type, container);
    ArrayDescriptor *array = newArrayDescriptor(type->manager, instType, type->rank, type->numSizes, type->sizes, type->numBounds, type->bounds);

    return getTypeDescriptorFromArrayDescriptor(array);
}

inline TypeDescriptor *getGenericDefinitionFromArrayDescriptor (ArrayDescriptor *type) {
    TypeDescriptor *genType = getGenericDefinitionFromTypeDescriptor(type->type);
    ArrayDescriptor *array = newArrayDescriptor(type->manager, genType, type->rank, type->numSizes, type->sizes, type->numBounds, type->bounds);

    return getTypeDescriptorFromArrayDescriptor(array);
}

inline void arrayDescriptorLock (ArrayDescriptor *type) {
    PLATFORM_lockMutex(&(type->mutex));
}

inline void arrayDescriptorUnLock (ArrayDescriptor *type) {
    PLATFORM_unlockMutex(&(type->mutex));
}

inline DescriptorMetadataTicket *arrayDescriptorCreateDescriptorMetadataTicket (ArrayDescriptor *array, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(array->metadata), type);
}

inline void arrayDescriptorBroadcastDescriptorMetadataTicket (ArrayDescriptor *array,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}

/* Function Pointer Descriptor Accessor Method */

JITUINT32 hashFunctionPointerDescriptor (FunctionPointerDescriptor *pointer) {
    if (pointer == NULL) {
        return 0;
    }
    JITUINT32 seed = pointer->hasThis;
    seed = combineHash(seed, pointer->explicitThis);
    seed = combineHash(seed, pointer->hasSentinel);
    seed = combineHash(seed, pointer->sentinel_index);
    seed = combineHash(seed, pointer->calling_convention);
    seed = combineHash(seed, pointer->generic_param_count);
    seed = combineHash(seed, (JITUINT32) pointer->result);
    XanListItem *item = xanList_first(pointer->params);
    while (item != NULL) {
        TypeDescriptor *param = (TypeDescriptor *) item->data;
        seed = combineHash(seed, (JITUINT32) param);
        item = item->next;
    }
    return seed;
}

JITINT32 equalsFunctionPointerDescriptor (FunctionPointerDescriptor *key1, FunctionPointerDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->hasThis != key2->hasThis) {
        return 0;
    }
    if (key1->explicitThis != key2->explicitThis) {
        return 0;
    }
    if (key1->hasSentinel != key2->hasSentinel) {
        return 0;
    }
    if (key1->sentinel_index != key2->sentinel_index) {
        return 0;
    }
    if (key1->calling_convention != key2->calling_convention) {
        return 0;
    }
    if (key1->generic_param_count != key2->generic_param_count) {
        return 0;
    }
    if (key1->result != key2->result) {
        return 0;
    }
    if (xanList_length(key1->params) != xanList_length(key2->params)) {
        return 0;
    }
    XanListItem *item1 = xanList_first(key1->params);
    XanListItem *item2 = xanList_first(key2->params);
    while (item1 != NULL && item2 != NULL) {
        TypeDescriptor *param1 = (TypeDescriptor *) item1->data;
        TypeDescriptor *param2 = (TypeDescriptor *) item2->data;
        if (param1 != param2) {
            return 0;
        }
        item1 = item1->next;
        item2 = item2->next;
    }
    return 1;
}

inline FunctionPointerDescriptor *newFunctionPointerDescriptor (metadataManager_t *manager, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeDescriptor *result, XanList *params) {
    FunctionPointerDescriptor pointer;

    pointer.manager = manager;
    pointer.result = result;
    pointer.params = params;
    pointer.hasThis = hasThis;
    pointer.explicitThis = explicitThis;
    pointer.hasSentinel = hasSentinel;
    pointer.sentinel_index = sentinel_index;
    pointer.calling_convention = calling_convention;
    pointer.generic_param_count = generic_param_count;
    xanHashTable_lock(manager->functionPointerDescriptors);
    FunctionPointerDescriptor *found = (FunctionPointerDescriptor *) xanHashTable_lookup(manager->functionPointerDescriptors, (void *) &pointer);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(FunctionPointerDescriptor));
        found->manager = manager;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(AUTOMATIC_TYPE_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = functionPointerDescriptorLock;
        found->unlock = functionPointerDescriptorUnLock;
        found->createDescriptorMetadataTicket = functionPointerDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = functionPointerDescriptorBroadcastDescriptorMetadataTicket;
        found->descriptor = NULL;
        found->byRefDescriptor = NULL;
        found->signatureInString = NULL;
        found->result = result;
        found->hasThis = hasThis;
        found->explicitThis = explicitThis;
        found->hasSentinel = hasSentinel;
        found->sentinel_index = sentinel_index;
        found->calling_convention = calling_convention;
        found->generic_param_count = generic_param_count;
        found->params = xanList_cloneList(params);
        found->getTypeDescriptor = getTypeDescriptorFromFunctionPointerDescriptor;
        found->getByRefTypeDescriptor = getByRefTypeDescriptorFromFunctionPointerDescriptor;
        found->getSignatureInString = getSignatureInStringFromFunctionPointerDescriptor;
        found->containOpenGenericParameter = containOpenGenericParameterFromFunctionPointerDescriptor;
        found->getInstance = getInstanceFromFunctionPointerDescriptor;
        found->getGenericDefinition = getGenericDefinitionFromFunctionPointerDescriptor;
        xanHashTable_insert(manager->functionPointerDescriptors, (void *) found, (void *) found);
    }
    xanHashTable_unlock(manager->functionPointerDescriptors);
    return found;
}

inline FunctionPointerDescriptor *newUnCachedFunctionPointerDescriptor (metadataManager_t *manager, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeDescriptor *result, XanList *params) {
    FunctionPointerDescriptor *pointer = sharedAllocFunction(sizeof(FunctionPointerDescriptor));

    pointer->manager = manager;
    pthread_mutexattr_t mutex_attr;
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(pointer->mutex), &mutex_attr);
    pointer->tickets = allocTicketArray(AUTOMATIC_TYPE_TICKET_COUNT);
    init_DescriptorMetadata(&(pointer->metadata));
    pointer->lock = functionPointerDescriptorLock;
    pointer->unlock = functionPointerDescriptorUnLock;
    pointer->createDescriptorMetadataTicket = functionPointerDescriptorCreateDescriptorMetadataTicket;
    pointer->broadcastDescriptorMetadataTicket = functionPointerDescriptorBroadcastDescriptorMetadataTicket;
    pointer->descriptor = NULL;
    pointer->byRefDescriptor = NULL;
    pointer->signatureInString = NULL;
    pointer->result = result;
    pointer->hasThis = hasThis;
    pointer->explicitThis = explicitThis;
    pointer->hasSentinel = hasSentinel;
    pointer->sentinel_index = sentinel_index;
    pointer->calling_convention = calling_convention;
    pointer->generic_param_count = generic_param_count;
    pointer->params = params;
    pointer->getTypeDescriptor = getTypeDescriptorFromFunctionPointerDescriptor;
    pointer->getByRefTypeDescriptor = getByRefTypeDescriptorFromFunctionPointerDescriptor;
    pointer->getSignatureInString = getSignatureInStringFromFunctionPointerDescriptor;
    pointer->containOpenGenericParameter = containOpenGenericParameterFromFunctionPointerDescriptor;
    return pointer;
}


inline TypeDescriptor *getTypeDescriptorFromFunctionPointerDescriptor (FunctionPointerDescriptor *class) {
    if (!WAIT_TICKET(class, TYPE_TICKET)) {
        /* not already cached */
        class->descriptor = newTypeDescriptor(class->manager, ELEMENT_FNPTR, (void *) class, JITFALSE);
        BROADCAST_TICKET(class, TYPE_TICKET);
    }
    return class->descriptor;
}

inline TypeDescriptor *getByRefTypeDescriptorFromFunctionPointerDescriptor (FunctionPointerDescriptor *class) {
    if (!WAIT_TICKET(class, BYREF_TYPE_TICKET)) {
        /* not already cached */
        class->byRefDescriptor = newTypeDescriptor(class->manager, ELEMENT_FNPTR, (void *) class, JITTRUE);
        BROADCAST_TICKET(class, BYREF_TYPE_TICKET);
    }
    return class->byRefDescriptor;
}

inline JITINT8 *getSignatureInStringFromFunctionPointerDescriptor (FunctionPointerDescriptor *pointer) {
    if (!WAIT_TICKET(pointer, NAME_TICKET)) {
        /* if not already loaded */
        JITINT8 *completeName = (JITINT8*) "FUNCTION POINTER";
        JITINT8 *resultInString = getCompleteNameFromTypeDescriptor(pointer->result);
        JITUINT32 param_count = xanList_length(pointer->params);
        JITINT8 **paramInString = NULL;
        if (param_count > 0) {
            paramInString = alloca(sizeof(JITINT8 *) * param_count);
        }
        JITUINT32 count = 0;
        XanListItem *item = xanList_first(pointer->params);
        JITUINT32 char_count = STRLEN(resultInString) + 1 + STRLEN(completeName) + 1 + 1 + 1;
        while (item != NULL) {
            TypeDescriptor *param = (TypeDescriptor *) item->data;
            paramInString[count] = getCompleteNameFromTypeDescriptor(param);
            char_count += STRLEN(paramInString[count]) + 1;
            item = item->next;
            count++;
        }

        size_t length = sizeof(JITINT8) * (char_count);
        pointer->signatureInString = sharedAllocFunction(length);
        SNPRINTF(pointer->signatureInString, length, "%s %s(", resultInString, completeName);
        if (param_count > 0) {
            for (count = 0; count < param_count - 1; count++) {
                pointer->signatureInString = STRCAT(pointer->signatureInString, paramInString[count]);
                pointer->signatureInString = STRCAT(pointer->signatureInString, (JITINT8 *) ",");
            }
            pointer->signatureInString = STRCAT(pointer->signatureInString, paramInString[count]);
        }
        pointer->signatureInString = STRCAT(pointer->signatureInString, (JITINT8 *) ")");
        BROADCAST_TICKET(pointer, NAME_TICKET);
    }
    return pointer->signatureInString;
}

inline JITBOOLEAN containOpenGenericParameterFromFunctionPointerDescriptor (FunctionPointerDescriptor *pointer) {
    if (containOpenGenericParameterFromTypeDescriptor(pointer->result)) {
        return JITTRUE;
    }
    XanListItem *item = xanList_first(pointer->params);
    while (item != NULL) {
        TypeDescriptor *param = (TypeDescriptor *) item->data;
        if (containOpenGenericParameterFromTypeDescriptor(param)) {
            return JITTRUE;
        }
        item = item->next;
    }
    return JITFALSE;
}

inline TypeDescriptor *getInstanceFromFunctionPointerDescriptor (FunctionPointerDescriptor *type, GenericContainer *container) {
    TypeDescriptor *result = getInstanceFromTypeDescriptor(type->result, container);
    XanList *params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    XanListItem *item = xanList_first(type->params);

    while (item != NULL) {
        TypeDescriptor *param = (TypeDescriptor *) item->data;
        TypeDescriptor *newParam = getInstanceFromTypeDescriptor(param, container);
        xanList_append(params, newParam);
        item = item->next;
    }
    FunctionPointerDescriptor *pointer = newFunctionPointerDescriptor(type->manager, type->hasThis, type->explicitThis, type->hasSentinel, type->sentinel_index, type->calling_convention, type->generic_param_count, result, params);
    return getTypeDescriptorFromFunctionPointerDescriptor(pointer);
}

inline TypeDescriptor *getGenericDefinitionFromFunctionPointerDescriptor (FunctionPointerDescriptor *type) {
    TypeDescriptor *result = getGenericDefinitionFromTypeDescriptor(type->result);
    XanList *params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    XanListItem *item = xanList_first(type->params);

    while (item != NULL) {
        TypeDescriptor *param = (TypeDescriptor *) item->data;
        TypeDescriptor *newParam = getGenericDefinitionFromTypeDescriptor(param);
        xanList_append(params, newParam);
        item = item->next;
    }
    FunctionPointerDescriptor *pointer = newFunctionPointerDescriptor(type->manager, type->hasThis, type->explicitThis, type->hasSentinel, type->sentinel_index, type->calling_convention, type->generic_param_count, result, params);
    return getTypeDescriptorFromFunctionPointerDescriptor(pointer);
}

inline void functionPointerDescriptorLock (FunctionPointerDescriptor *type) {
    PLATFORM_lockMutex(&(type->mutex));
}

inline void functionPointerDescriptorUnLock (FunctionPointerDescriptor *type) {
    PLATFORM_unlockMutex(&(type->mutex));
}

inline DescriptorMetadataTicket *functionPointerDescriptorCreateDescriptorMetadataTicket (FunctionPointerDescriptor *pointer, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(pointer->metadata), type);
}

inline void functionPointerDescriptorBroadcastDescriptorMetadataTicket (FunctionPointerDescriptor *pointer,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}
