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
#include <metadata_types_spec.h>
#include <type_spec_descriptor.h>
#include <metadata_types_manager.h>
#include <type_descriptor.h>
#include <generic_container.h>
#include <class_descriptor.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>
#include <assert.h>
#include <stdlib.h>

/* TypeSpecDescriptor Accessor Methods */
JITUINT32 hashTypeSpecDescriptor (TypeSpecDescriptor *type) {
    if (type == NULL) {
        return 0;
    }
    JITUINT32 seed;
    switch (type->element_type) {
        case ELEMENT_ARRAY:
            seed = hashArraySpecDescriptor((ArraySpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
            seed = hashFunctionPointerSpecDescriptor((FunctionPointerSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            seed = hashPointerSpecDescriptor((PointerSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            seed = hashClassSpecDescriptor((ClassSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_VAR:
            seed = hashGenericVarSpecDescriptor((GenericVarSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_MVAR:
            seed = hashGenericMethodVarSpecDescriptor((GenericMethodVarSpecDescriptor *) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: hashTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    seed = combineHash(type->element_type, seed);
    seed = combineHash(type->isByRef, seed);
    return seed;
}

JITINT32 equalsTypeSpecDescriptor (TypeSpecDescriptor *type1, TypeSpecDescriptor *type2) {
    if (type1 == NULL || type2 == NULL) {
        return 0;
    }
    if (type1->element_type != type2->element_type) {
        return 0;
    }
    if (type1->isByRef != type2->isByRef) {
        return 0;
    }
    JITUINT32 equal;
    switch (type1->element_type) {
        case ELEMENT_ARRAY:
            equal = equalsArraySpecDescriptor((ArraySpecDescriptor *) type1->descriptor, (ArraySpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_FNPTR:
            equal = equalsFunctionPointerSpecDescriptor((FunctionPointerSpecDescriptor *) type1->descriptor, (FunctionPointerSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_PTR:
            equal = equalsPointerSpecDescriptor((PointerSpecDescriptor *) type1->descriptor, (PointerSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_CLASS:
            equal = equalsClassSpecDescriptor((ClassSpecDescriptor *) type1->descriptor, (ClassSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_VAR:
            equal = equalsGenericVarSpecDescriptor((GenericVarSpecDescriptor *) type1->descriptor, (GenericVarSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_MVAR:
            equal = equalsGenericMethodVarSpecDescriptor((GenericMethodVarSpecDescriptor *) type1->descriptor, (GenericMethodVarSpecDescriptor *) type2->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: equalsTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type1->element_type);
            abort();
    }
    return equal;
}

inline JITBOOLEAN equalsWithMapperTypeSpecDescriptor (TypeSpecDescriptor *type1, TypeDescriptor *owner, XanHashTable *mapper, TypeSpecDescriptor *type2) {
    if (type1 == NULL || type2 == NULL) {
        return 0;
    }
    if (type1->isByRef != type2->isByRef) {
        return 0;
    }
    if (type1->element_type != type2->element_type && type1->element_type != ELEMENT_VAR) {
        return 0;
    }
    JITBOOLEAN equal = JITFALSE;
    switch (type1->element_type) {
        case ELEMENT_ARRAY:
            equal = equalsWithMapperArraySpecDescriptor((ArraySpecDescriptor *) type1->descriptor, owner, mapper, (ArraySpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_FNPTR:
            equal = equalsWithMapperFunctionPointerSpecDescriptor((FunctionPointerSpecDescriptor *) type1->descriptor, owner, mapper, (FunctionPointerSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_PTR:
            equal = equalsWithMapperPointerSpecDescriptor((PointerSpecDescriptor *) type1->descriptor, owner, mapper, (PointerSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_CLASS:
            equal = equalsWithMapperClassSpecDescriptor((ClassSpecDescriptor *) type1->descriptor, owner, mapper, (ClassSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_MVAR:
            equal = equalsGenericMethodVarSpecDescriptor((GenericMethodVarSpecDescriptor *) type1->descriptor, (GenericMethodVarSpecDescriptor *) type2->descriptor);
            break;
        case ELEMENT_VAR: {
            GenericVarSpecDescriptor *var = (GenericVarSpecDescriptor *) type1->descriptor;
            GenericParameterRule rule;
            rule.owner = owner;
            rule.var = var->var;
            GenericParameterRule *found = xanHashTable_lookup(mapper, &rule);
            equal = equalsTypeSpecDescriptor(found->replace, type2);
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: equalsWithMapperTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type1->element_type);
            abort();
    }
    return equal;
}

inline TypeSpecDescriptor *newTypeSpecDescriptor (ILElemType element_type, void * descriptor, JITBOOLEAN isByRef) {
    TypeSpecDescriptor *result = sharedAllocFunction(sizeof(TypeSpecDescriptor));

    result->element_type = element_type;
    result->isByRef = isByRef;
    result->refCount = 0;
    result->descriptor = descriptor;
    result->isClosed = isClosedTypeSpecDescriptor;
    result->replaceGenericVar = replaceGenericVarTypeSpecDescriptor;
    return result;
}

inline TypeSpecDescriptor *replaceTypeSpecDescriptor (TypeSpecDescriptor *type, GenericSpecContainer *container) {
    TypeSpecDescriptor *result;
    void *descriptor = NULL;

    switch (type->element_type) {
        case ELEMENT_ARRAY:
            descriptor = replaceArraySpecDescriptor((ArraySpecDescriptor *) type->descriptor, container);
            result = newTypeSpecDescriptor(type->element_type, descriptor, type->isByRef);
            break;
        case ELEMENT_FNPTR:
            descriptor = replaceFunctionPointerSpecDescriptor((FunctionPointerSpecDescriptor *) type->descriptor, container);
            result = newTypeSpecDescriptor(type->element_type, descriptor, type->isByRef);
            break;
        case ELEMENT_PTR:
            descriptor = replacePointerSpecDescriptor((PointerSpecDescriptor *) type->descriptor, container);
            result = newTypeSpecDescriptor(type->element_type, descriptor, type->isByRef);
            break;
        case ELEMENT_CLASS:
            descriptor = replaceClassSpecDescriptor((ClassSpecDescriptor *) type->descriptor, container);
            result = newTypeSpecDescriptor(type->element_type, descriptor, type->isByRef);
            break;
        case ELEMENT_VAR: {
            GenericVarSpecDescriptor *var = (GenericVarSpecDescriptor *) type->descriptor;
            container->paramType[var->var]->refCount++;
            result = container->paramType[var->var];
        }
        break;
        case ELEMENT_MVAR:
            (type->refCount)++;
            result = type;
            break;
        default:
            PDEBUG("METADATA MANAGER: replaceTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline void destroyTypeSpecDescriptor (TypeSpecDescriptor *type) {
    assert(type->refCount >= 0);
    if (type->refCount == 0) {
        switch (type->element_type) {
            case ELEMENT_ARRAY:
                destroyArraySpecDescriptor((ArraySpecDescriptor *) type->descriptor);
                break;
            case ELEMENT_FNPTR:
                destroyFunctionPointerSpecDescriptor((FunctionPointerSpecDescriptor *) type->descriptor);
                break;
            case ELEMENT_PTR:
                destroyPointerSpecDescriptor((PointerSpecDescriptor *) type->descriptor);
                break;
            case ELEMENT_CLASS:
                destroyClassSpecDescriptor((ClassSpecDescriptor *) type->descriptor);
                break;
            case ELEMENT_VAR:
                destroyGenericVarSpecDescriptor((GenericVarSpecDescriptor *) type->descriptor);
                break;
            case ELEMENT_MVAR:
                destroyGenericMethodVarSpecDescriptor((GenericMethodVarSpecDescriptor *) type->descriptor);
                break;
            default:
                PDEBUG("METADATA MANAGER: destroyTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
                abort();
        }
        freeFunction(type);
    }
}

inline JITBOOLEAN isClosedTypeSpecDescriptor (TypeSpecDescriptor *type) {
    JITBOOLEAN result;

    switch (type->element_type) {
        case ELEMENT_ARRAY:
            result = isClosedArraySpecDescriptor((ArraySpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_FNPTR:
            result = isClosedFunctionPointerSpecDescriptor((FunctionPointerSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_PTR:
            result = isClosedPointerSpecDescriptor((PointerSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_CLASS:
            result = isClosedClassSpecDescriptor((ClassSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_VAR:
            result = isClosedGenericVarSpecDescriptor((GenericVarSpecDescriptor *) type->descriptor);
            break;
        case ELEMENT_MVAR:
            result = isClosedGenericMethodVarSpecDescriptor((GenericMethodVarSpecDescriptor *) type->descriptor);
            break;
        default:
            PDEBUG("METADATA MANAGER: hashTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

inline TypeDescriptor *replaceGenericVarTypeSpecDescriptor (metadataManager_t *manager, TypeSpecDescriptor *type, GenericContainer *container) {
    TypeDescriptor *result;

    switch (type->element_type) {
        case ELEMENT_ARRAY: {
            ArrayDescriptor *new_descriptor = replaceGenericVarArraySpecDescriptor(manager, (ArraySpecDescriptor *) type->descriptor, container);
            if (type->isByRef) {
                result = new_descriptor->getByRefTypeDescriptor(new_descriptor);
            } else {
                result = new_descriptor->getTypeDescriptor(new_descriptor);
            }
        }
        break;
        case ELEMENT_FNPTR: {
            FunctionPointerDescriptor *new_descriptor = replaceGenericVarFunctionPointerSpecDescriptor(manager, (FunctionPointerSpecDescriptor *) type->descriptor, container);
            if (type->isByRef) {
                result = new_descriptor->getByRefTypeDescriptor(new_descriptor);
            } else {
                result = new_descriptor->getTypeDescriptor(new_descriptor);
            }
        }
        break;
        case ELEMENT_PTR: {
            PointerDescriptor *new_descriptor = replaceGenericVarPointerSpecDescriptor(manager, (PointerSpecDescriptor *) type->descriptor, container);
            if (type->isByRef) {
                result = new_descriptor->getByRefTypeDescriptor(new_descriptor);
            } else {
                result = new_descriptor->getTypeDescriptor(new_descriptor);
            }
        }
        break;
        case ELEMENT_CLASS: {
            ClassDescriptor *new_descriptor = replaceGenericVarClassSpecDescriptor(manager, (ClassSpecDescriptor *) type->descriptor, container);
            if (type->isByRef) {
                result = new_descriptor->getByRefTypeDescriptor(new_descriptor);
            } else {
                result = new_descriptor->getTypeDescriptor(new_descriptor);
            }
        }
        break;
        case ELEMENT_MVAR: {
            GenericMethodVarSpecDescriptor *descriptor = (GenericMethodVarSpecDescriptor *) type->descriptor;
            JITUINT32 number = descriptor->mvar;
            result = container->paramType[number];
        }
        break;
        case ELEMENT_VAR: {
            GenericVarSpecDescriptor *descriptor = (GenericVarSpecDescriptor *) type->descriptor;
            JITUINT32 number = descriptor->var;
            switch (container->container_type) {
                case CLASS_LEVEL:
                    result = container->paramType[number];
                    break;
                case METHOD_LEVEL:
                    result = container->parent->paramType[number];
                    break;
                default:
                    PDEBUG("METADATA MANAGER: getTypeDescriptorFromTypeBlob: ERROR! INVALID CONTAINER LEVEL! %d\n", container->container_type);
                    abort();
            }
        }
        break;
        default:
            PDEBUG("METADATA MANAGER: hashTypeSpecDescriptor: ERROR! INVALID TYPE! %d\n", type->element_type);
            abort();
    }
    return result;
}

/* Pointer Description Accessor Method */
JITUINT32 hashPointerSpecDescriptor (PointerSpecDescriptor *Pointer) {
    if (Pointer == NULL) {
        return 0;
    }
    JITUINT32 seed = hashTypeSpecDescriptor(Pointer->type);
    return seed;
}

JITINT32 equalsPointerSpecDescriptor (PointerSpecDescriptor *key1, PointerSpecDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return equalsTypeSpecDescriptor(key1->type, key2->type);
}

inline JITBOOLEAN equalsWithMapperPointerSpecDescriptor (PointerSpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, PointerSpecDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return equalsWithMapperTypeSpecDescriptor(key1->type, owner, mapper, key2->type);
}

inline PointerSpecDescriptor *newPointerSpecDescriptor (TypeSpecDescriptor *type) {
    PointerSpecDescriptor *pointer = sharedAllocFunction(sizeof(PointerSpecDescriptor));

    pointer->type = type;
    pointer->destroy = destroyPointerSpecDescriptor;
    pointer->isClosed = isClosedPointerSpecDescriptor;
    pointer->replaceGenericVar = replaceGenericVarPointerSpecDescriptor;
    return pointer;
}

inline PointerSpecDescriptor *replacePointerSpecDescriptor (PointerSpecDescriptor *pointer, GenericSpecContainer *container) {
    TypeSpecDescriptor *type = replaceTypeSpecDescriptor(pointer->type, container);

    return newPointerSpecDescriptor(type);
}


inline void destroyPointerSpecDescriptor (PointerSpecDescriptor *pointer) {
    destroyTypeSpecDescriptor(pointer->type);
    freeFunction(pointer);
}

inline JITBOOLEAN isClosedPointerSpecDescriptor (PointerSpecDescriptor *pointer) {
    return isClosedTypeSpecDescriptor(pointer->type);
}

inline PointerDescriptor *replaceGenericVarPointerSpecDescriptor (metadataManager_t *manager, PointerSpecDescriptor *pointer, GenericContainer *container) {
    TypeDescriptor *type = replaceGenericVarTypeSpecDescriptor(manager, pointer->type, container);

    return newPointerDescriptor(manager, type);
}

/* Array descriptor Accessor Method */
JITUINT32 hashArraySpecDescriptor (ArraySpecDescriptor *array) {
    if (array == NULL) {
        return 0;
    }
    JITUINT32 seed = hashTypeSpecDescriptor(array->type);
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

JITINT32 equalsArraySpecDescriptor (ArraySpecDescriptor *key1, ArraySpecDescriptor *key2) {
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
    if (key1->numSizes) {
        if (memcmp(key1->sizes, key2->sizes, sizeof(JITUINT32) * key1->numSizes) != 0) {
            return 0;
        }
    }
    if (key1->numBounds) {
        if (memcmp(key1->bounds, key2->bounds, sizeof(JITUINT32) * key1->numBounds) != 0) {
            return 0;
        }
    }
    if (!equalsTypeSpecDescriptor(key1->type, key2->type)) {
        return 0;
    }
    return 1;
}

inline JITBOOLEAN equalsWithMapperArraySpecDescriptor (ArraySpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, ArraySpecDescriptor *key2) {
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
    if (key1->numSizes) {
        if (memcmp(key1->sizes, key2->sizes, sizeof(JITUINT32) * key1->numSizes) != 0) {
            return 0;
        }
    }
    if (key1->numBounds) {
        if (memcmp(key1->bounds, key2->bounds, sizeof(JITUINT32) * key1->numBounds) != 0) {
            return 0;
        }
    }
    if (!equalsWithMapperTypeSpecDescriptor(key1->type, owner, mapper, key2->type)) {
        return 0;
    }
    return 1;
}

inline ArraySpecDescriptor *newArraySpecDescriptor (TypeSpecDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds) {
    ArraySpecDescriptor *array = sharedAllocFunction(sizeof(ArraySpecDescriptor));

    array->type = type;
    array->rank = rank;
    array->numSizes = numSizes;
    array->sizes = sizes;
    array->numBounds = numBounds;
    array->bounds = bounds;
    array->destroy = destroyArraySpecDescriptor;
    array->isClosed = isClosedArraySpecDescriptor;
    array->replaceGenericVar = replaceGenericVarArraySpecDescriptor;
    return array;
}

inline ArraySpecDescriptor *replaceArraySpecDescriptor (ArraySpecDescriptor *array, GenericSpecContainer *container) {
    JITUINT32 *bounds = NULL;
    JITUINT32 *sizes = NULL;

    if (array->numSizes > 0) {
        sizes = sharedAllocFunction(sizeof(JITUINT32) * array->numSizes);
        memcpy(sizes, array->sizes, sizeof(JITUINT32) * array->numSizes);
    }
    if (array->numBounds > 0) {
        bounds = sharedAllocFunction(sizeof(JITUINT32) * array->numBounds);
        memcpy(bounds, array->sizes, sizeof(JITUINT32) * array->numBounds);
    }
    TypeSpecDescriptor *type = replaceTypeSpecDescriptor(array->type, container);
    return newArraySpecDescriptor(type, array->rank, array->numSizes, sizes, array->numBounds, bounds);
}

inline void destroyArraySpecDescriptor (ArraySpecDescriptor *array) {
    destroyTypeSpecDescriptor(array->type);
    if (array->sizes != NULL) {
        freeFunction(array->sizes);
    }
    if (array->bounds != NULL) {
        freeFunction(array->bounds);
    }
    freeFunction(array);
}

inline JITBOOLEAN isClosedArraySpecDescriptor (ArraySpecDescriptor *array) {
    return isClosedTypeSpecDescriptor(array->type);
}

inline ArrayDescriptor *replaceGenericVarArraySpecDescriptor (metadataManager_t *manager, ArraySpecDescriptor *array, GenericContainer *container) {
    TypeDescriptor *type = replaceGenericVarTypeSpecDescriptor(manager, array->type, container);

    return newArrayDescriptor(manager, type, array->rank, array->numSizes, array->sizes, array->numBounds, array->bounds);
}

/* Generic Var descriptor Accessor Method */
JITUINT32 hashGenericVarSpecDescriptor (GenericVarSpecDescriptor *var) {
    if (var == NULL) {
        return 0;
    }
    return var->var;
}

JITINT32 equalsGenericVarSpecDescriptor (GenericVarSpecDescriptor *key1, GenericVarSpecDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return key1->var == key2->var;
}

inline GenericVarSpecDescriptor *newGenericVarSpecDescriptor (JITUINT32 var) {
    GenericVarSpecDescriptor *GenericVar = sharedAllocFunction(sizeof(GenericVarSpecDescriptor));

    GenericVar->var = var;
    GenericVar->destroy = destroyGenericVarSpecDescriptor;
    GenericVar->isClosed = isClosedGenericVarSpecDescriptor;
    return GenericVar;
}

inline void destroyGenericVarSpecDescriptor (GenericVarSpecDescriptor *GenericVar) {
    freeFunction(GenericVar);
}

inline JITBOOLEAN isClosedGenericVarSpecDescriptor (GenericVarSpecDescriptor *var) {
    return JITFALSE;
}


/* Generic Method Var descriptor Accessor Method */

JITUINT32 hashGenericMethodVarSpecDescriptor (GenericMethodVarSpecDescriptor *mvar) {
    if (mvar == NULL) {
        return 0;
    }
    return mvar->mvar;
}

JITINT32 equalsGenericMethodVarSpecDescriptor (GenericMethodVarSpecDescriptor *key1, GenericMethodVarSpecDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    return key1->mvar == key2->mvar;
}

inline GenericMethodVarSpecDescriptor *newGenericMethodVarSpecDescriptor (JITUINT32 mvar) {
    GenericMethodVarSpecDescriptor *GenericMethodVar = sharedAllocFunction(sizeof(GenericMethodVarSpecDescriptor));

    GenericMethodVar->mvar = mvar;
    GenericMethodVar->destroy = destroyGenericMethodVarSpecDescriptor;
    GenericMethodVar->isClosed = isClosedGenericMethodVarSpecDescriptor;
    return GenericMethodVar;
}

inline void destroyGenericMethodVarSpecDescriptor (GenericMethodVarSpecDescriptor *GenericMethodVar) {
    freeFunction(GenericMethodVar);
}

inline JITBOOLEAN isClosedGenericMethodVarSpecDescriptor (GenericMethodVarSpecDescriptor *mvar) {
    return JITFALSE;
}

/* Generic SpecContainer Accessor Method */

inline void destroyGenericSpecContainer (GenericSpecContainer *container) {
    if (container != NULL) {
        if (container->arg_count > 0) {
            JITUINT32 count;
            for (count = 0; count < container->arg_count; count++) {
                (container->paramType)[count]->refCount--;
                destroyTypeSpecDescriptor((container->paramType)[count]);
            }
            freeFunction(container->paramType);
        }
        freeFunction(container);
    }
}

inline GenericSpecContainer *newGenericSpecContainer (JITUINT32 arg_count) {
    GenericSpecContainer *container = sharedAllocFunction(sizeof(GenericSpecContainer));

    container->arg_count = arg_count;
    if (arg_count > 0) {
        container->paramType = sharedAllocFunction(sizeof(TypeSpecDescriptor *) * arg_count);
    } else {
        container->paramType = NULL;
    }
    container->destroy = destroyGenericSpecContainer;
    return container;
}

JITUINT32 hashGenericSpecContainer (GenericSpecContainer *container) {
    if (container == NULL) {
        return 0;
    }
    JITUINT32 seed = hashTypeSpecDescriptor((container->paramType)[0]);
    JITUINT32 count;
    for (count = 1; count < container->arg_count; count++) {
        TypeSpecDescriptor *current_element = (container->paramType)[count];
        seed = combineHash(seed, hashTypeSpecDescriptor(current_element));
    }
    return seed;
}

JITINT32 equalsGenericSpecContainer (GenericSpecContainer *key1, GenericSpecContainer *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->arg_count != key2->arg_count) {
        return 0;
    }
    JITUINT32 count;
    for (count = 0; count < key1->arg_count; count++) {
        if (!equalsTypeSpecDescriptor((key1->paramType)[count], (key2->paramType)[count])) {
            return 0;
        }
    }
    return 1;
}

inline JITBOOLEAN equalsWithMapperGenericSpecContainer (GenericSpecContainer *key1, TypeDescriptor *owner, XanHashTable *mapper, GenericSpecContainer *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->arg_count != key2->arg_count) {
        return 0;
    }
    JITUINT32 count;
    for (count = 0; count < key1->arg_count; count++) {
        if (!equalsWithMapperTypeSpecDescriptor((key1->paramType)[count], owner, mapper, (key2->paramType)[count])) {
            return 0;
        }
    }
    return 1;
}

inline void insertTypeSpecDescriptorIntoGenericSpecContainer (GenericSpecContainer *self, TypeSpecDescriptor *type, JITUINT32 position) {
    assert(self->arg_count > position);
    type->refCount++;
    self->paramType[position] = type;
}


/* Class descriptor Accessor Method*/

inline void destroyClassSpecDescriptor (ClassSpecDescriptor *class) {
    destroyGenericSpecContainer(class->container);
    freeFunction(class);
}

inline ClassSpecDescriptor *newClassSpecDescriptor (BasicClass *row, GenericSpecContainer *container) {
    ClassSpecDescriptor *class = sharedAllocFunction(sizeof(ClassSpecDescriptor));

    class->row = row;
    class->container = container;
    class->destroy = destroyClassSpecDescriptor;
    class->isClosed = isClosedClassSpecDescriptor;
    class->replaceGenericVar = replaceGenericVarClassSpecDescriptor;
    return class;
}

inline ClassSpecDescriptor *replaceClassSpecDescriptor (ClassSpecDescriptor *class, GenericSpecContainer *container) {
    GenericSpecContainer *new_container;

    if (class->container != NULL) {
        new_container = newGenericSpecContainer(class->container->arg_count);
        JITUINT32 count;
        for (count = 0; count < class->container->arg_count; count++) {
            TypeSpecDescriptor *typeArg = replaceTypeSpecDescriptor(class->container->paramType[count], container);
            insertTypeSpecDescriptorIntoGenericSpecContainer(new_container, typeArg, count);
        }
    } else {
        new_container = NULL;
    }
    return newClassSpecDescriptor(class->row, new_container);
}

JITUINT32 hashClassSpecDescriptor (ClassSpecDescriptor *class) {
    if (class == NULL) {
        return 0;
    }
    JITUINT32 seed = (JITINT32) class->row;
    return combineHash(seed, hashGenericSpecContainer(class->container));
}

JITINT32 equalsClassSpecDescriptor (ClassSpecDescriptor *key1, ClassSpecDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->row != key2->row) {
        return 0;
    }
    return equalsGenericSpecContainer(key1->container, key2->container);
}

inline JITBOOLEAN equalsWithMapperClassSpecDescriptor (ClassSpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, ClassSpecDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->row != key2->row) {
        return 0;
    }
    return equalsWithMapperGenericSpecContainer(key1->container, owner, mapper, key2->container);
}


inline JITBOOLEAN isClosedClassSpecDescriptor (ClassSpecDescriptor *class) {
    JITUINT32 count;

    for (count = 0; count < class->container->arg_count; count++) {
        if (!isClosedTypeSpecDescriptor(class->container->paramType[count])) {
            return JITFALSE;
        }
    }
    return JITTRUE;
}

inline ClassDescriptor *replaceGenericVarClassSpecDescriptor (metadataManager_t *manager, ClassSpecDescriptor *class, GenericContainer *container) {
    GenericContainer *new_container = NULL;

    if (class->container != NULL) {
        new_container = newGenericContainer(CLASS_LEVEL, NULL, class->container->arg_count);
        JITUINT32 count;
        for (count = 0; count < class->container->arg_count; count++) {
            TypeDescriptor *typeArg = replaceGenericVarTypeSpecDescriptor(manager, class->container->paramType[count], container);
            insertTypeDescriptorIntoGenericContainer(new_container, typeArg, count);
        }
    }
    return newClassDescriptor(manager, class->row, new_container);
}

/* Function Pointer descriptor Accessor Method */

JITUINT32 hashFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer) {
    if (pointer == NULL) {
        return 0;
    }
    JITUINT32 seed = pointer->hasThis;
    seed = combineHash(seed, pointer->explicitThis);
    seed = combineHash(seed, pointer->hasSentinel);
    seed = combineHash(seed, pointer->sentinel_index);
    seed = combineHash(seed, pointer->calling_convention);
    seed = combineHash(seed, pointer->generic_param_count);
    seed = combineHash(seed, hashTypeSpecDescriptor(pointer->result));
    JITUINT32 count;
    for (count = 0; count < pointer->param_count; count++) {
        seed = combineHash(seed, hashTypeSpecDescriptor((pointer->params)[count]));
    }
    return seed;
}

JITINT32 equalsFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *key1, FunctionPointerSpecDescriptor *key2) {
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
    if (key1->param_count != key2->param_count) {
        return 0;
    }
    if (!equalsTypeSpecDescriptor(key1->result, key2->result)) {
        return 0;
    }
    JITUINT32 count;
    for (count = 0; count < key1->param_count; count++) {
        if (!equalsTypeSpecDescriptor((key1->params)[count], (key2->params)[count])) {
            return 0;
        }
    }
    return 1;
}

inline JITBOOLEAN equalsWithMapperFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *key1, TypeDescriptor *owner, XanHashTable *mapper, FunctionPointerSpecDescriptor *key2) {
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
    if (key1->param_count != key2->param_count) {
        return 0;
    }
    if (!equalsWithMapperTypeSpecDescriptor(key1->result, owner, mapper, key2->result)) {
        return 0;
    }
    JITUINT32 count;
    for (count = 0; count < key1->param_count; count++) {
        if (!equalsWithMapperTypeSpecDescriptor((key1->params)[count], owner, mapper, (key2->params)[count])) {
            return 0;
        }
    }
    return 1;

}

inline FunctionPointerSpecDescriptor *newFunctionPointerSpecDescriptor (JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeSpecDescriptor *result, JITUINT32 param_count, TypeSpecDescriptor **params) {
    FunctionPointerSpecDescriptor *pointer = sharedAllocFunction(sizeof(FunctionPointerSpecDescriptor));

    pointer->result = result;
    pointer->param_count = param_count;
    pointer->params = params;
    pointer->hasThis = hasThis;
    pointer->explicitThis = explicitThis;
    pointer->hasSentinel = hasSentinel;
    pointer->sentinel_index = sentinel_index;
    pointer->calling_convention = calling_convention;
    pointer->generic_param_count = generic_param_count;
    pointer->equalsNoVarArg = equalsNoVarArgFunctionPointerSpecDescriptor;
    pointer->destroy = destroyFunctionPointerSpecDescriptor;
    pointer->replaceGenericVar = replaceGenericVarFunctionPointerSpecDescriptor;
    return pointer;
}

inline FunctionPointerSpecDescriptor *replaceFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer, GenericSpecContainer *container) {
    TypeSpecDescriptor *result = replaceTypeSpecDescriptor(pointer->result, container);
    TypeSpecDescriptor **params = sharedAllocFunction(sizeof(TypeDescriptor *) * pointer->param_count);
    JITUINT32 count;

    for (count = 0; count < pointer->param_count; count++) {
        params[count] = replaceTypeSpecDescriptor((pointer->params)[count], container);
    }
    FunctionPointerSpecDescriptor *result_ptr = newFunctionPointerSpecDescriptor(pointer->hasThis, pointer->explicitThis, pointer->hasSentinel, pointer->sentinel_index, pointer->calling_convention, pointer->generic_param_count, result, pointer->param_count, params);
    return result_ptr;
}

inline void destroyFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer) {
    JITUINT32 count;

    for (count = 0; count < pointer->param_count; count++) {
        destroyTypeSpecDescriptor((pointer->params)[count]);
    }
    destroyTypeSpecDescriptor(pointer->result);
    freeFunction(pointer);
}

inline JITBOOLEAN isClosedFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *pointer) {
    if (!isClosedTypeSpecDescriptor(pointer->result)) {
        return JITFALSE;
    }
    JITUINT32 count;
    for (count = 0; count < pointer->param_count; count++) {
        if (!isClosedTypeSpecDescriptor((pointer->params)[count])) {
            return JITFALSE;
        }
    }
    return JITTRUE;
}

inline FunctionPointerDescriptor *replaceGenericVarFunctionPointerSpecDescriptor (metadataManager_t *manager, FunctionPointerSpecDescriptor *pointer, GenericContainer *container) {
    TypeDescriptor*result = replaceGenericVarTypeSpecDescriptor(manager, pointer->result, container);
    XanList *params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    JITUINT32 count;

    for (count = 0; count < pointer->param_count; count++) {
        TypeDescriptor *param = replaceGenericVarTypeSpecDescriptor(manager, (pointer->params)[count], container);
        xanList_append(params, param);
    }
    FunctionPointerDescriptor *result_ptr = newFunctionPointerDescriptor(manager, pointer->hasThis, pointer->explicitThis, pointer->hasSentinel, pointer->sentinel_index, pointer->calling_convention, pointer->generic_param_count, result, params);
    xanList_destroyList(params);
    return result_ptr;
}

inline JITBOOLEAN equalsNoVarArgFunctionPointerSpecDescriptor (FunctionPointerSpecDescriptor *method1, FunctionPointerSpecDescriptor *method2) {
    if (method1->calling_convention != method2->calling_convention) {
        return JITFALSE;
    }
    if (method1->generic_param_count != method2->generic_param_count) {
        return JITFALSE;
    }

    JITUINT32 max = 0;
    JITUINT32 max2 = 0;
    JITUINT32 method1_param = method1->param_count;
    JITUINT32 method2_param = method2->param_count;

    if (!method1->hasSentinel && !method2->hasSentinel) {
        max = method1_param;
        max2 = method2_param;
        if ( max != max2 ) {
            return JITFALSE;
        }
    } else if (!method1->hasSentinel && method2->hasSentinel) {
        max = method1_param;
        max2 = method2->sentinel_index;
        if ( max2 > max ) {
            return JITFALSE;
        }
    } else if (method1->hasSentinel && !method2->hasSentinel) {
        max = method1->sentinel_index;
        max2 = method2_param;
        if ( max > max2 ) {
            return JITFALSE;
        }
    } else if (method1->hasSentinel && method2->hasSentinel) {
        max = method1->sentinel_index;
        max2 = method2->sentinel_index;
        if ( max != max2 ) {
            return JITFALSE;
        }
    }
    JITUINT32 param_count = max < max2 ? max : max2;
    JITUINT32 count;

    if (!equalsTypeSpecDescriptor(method1->result, method2->result)) {
        return JITFALSE;
    }

    for (count = 0; count < param_count; count++) {
        if (!equalsTypeSpecDescriptor((method1->params)[count], (method2->params)[count])) {
            return JITFALSE;
        }
    }
    return JITTRUE;
}
