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
#include <decoding_tools.h>
#include <stdlib.h>
#include <platform_API.h>

#define NAME_TICKET 0
#define CONSTRAINT_TICKET 1

#define GENERIC_VAR_TICKET_COUNT 2

/* Generic Var Descriptor Accessor Methods */
JITUINT32 hashGenericVarDescriptor (void *element) {
    if (element == NULL) {
        return 0;
    }
    JITUINT32 seed;
    GenericVarDescriptor *type = (GenericVarDescriptor *) element;
    seed = (JITUINT32) type->row;
    return combineHash((JITUINT32) type->owner, seed);
}

JITINT32 equalsGenericVarDescriptor (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    GenericVarDescriptor *type1 = (GenericVarDescriptor *) key1;
    GenericVarDescriptor *type2 = (GenericVarDescriptor *) key2;
    return type1->row == type2->row && type1->owner == type2->owner;
}

inline GenericVarDescriptor *newGenericVarDescriptor (metadataManager_t *manager, BasicGenericParam *row, TypeDescriptor *owner) {
    if (row == NULL) {
        return NULL;
    }
    GenericVarDescriptor param;
    param.manager = manager;
    param.owner = owner;
    param.row = row;
    GenericVarDescriptor *found = (GenericVarDescriptor *) xanHashTable_lookup(manager->genericVarDescriptors, (void *) &param);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(GenericVarDescriptor));
        found->manager = manager;
        found->row = row;
        found->owner = owner;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(GENERIC_VAR_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = genericVarDescriptorLock;
        found->unlock = genericVarDescriptorUnLock;
        found->createDescriptorMetadataTicket = genericVarDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = genericVarDescriptorBroadcastDescriptorMetadataTicket;
        /* Init Default value */
        found->name = NULL;
        found->constraints = NULL;
        found->getName = getNameFromGenericVarDescriptor;
        found->getConstraints = getConstraintsFromGenericVarDescriptor;
        found->getIRType = getIRTypeFromGenericVarDescriptor;
        found->getInstance = getInstanceFromGenericVarDescriptor;
        /* end init Default value */
        found->attributes = getBasicGenericParamAttributes(manager, row);
        xanHashTable_insert(manager->genericVarDescriptors, (void *) found, (void *) found);

    }
    return found;
}

inline JITINT8 *getNameFromGenericVarDescriptor (GenericVarDescriptor *type) {
    if (!WAIT_TICKET(type, NAME_TICKET)) {
        /* Not already Cached */
        t_binary_information *binary = (t_binary_information *) type->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_string_stream *string_stream = &(streams->string_stream);
        type->name = get_string(string_stream, type->row->name);
        BROADCAST_TICKET(type, NAME_TICKET);
    }
    return type->name;
}

inline ClassDescriptor *getUnderliningClassDescriptorFromGenericVarDescriptor (GenericVarDescriptor *type) {
    TypeDescriptor *superType;

    superType = getSuperTypeFromGenericVarDescriptor(type);
    return getUnderliningClassDescriptorFromTypeDescriptor(superType);
}

inline TypeDescriptor *getInstanceFromGenericVarDescriptor (GenericVarDescriptor *type, GenericContainer *container) {
    TypeDescriptor *result;
    JITUINT32 number = type->row->number;

    switch (container->container_type) {
        case CLASS_LEVEL:
            result = container->paramType[number];
            break;
        case METHOD_LEVEL:
            result = container->parent->paramType[number];
            break;
        default:
            PDEBUG("METADATA MANAGER: getInstanceFromGenericVarDescriptor: ERROR! INVALID CONTAINER LEVEL! %d\n", container->container_type);
            abort();
    }
    ;
    return result;
}

inline TypeDescriptor *getSuperTypeFromGenericVarDescriptor (GenericVarDescriptor *type) {
    TypeDescriptor *superType;

    initializeMetadata();
    XanList *constraints = getConstraintsFromGenericVarDescriptor(type);
    if (xanList_length(constraints) == 1) {
        superType = (TypeDescriptor *) xanList_first(constraints)->data;
    } else if (type->attributes->isValueType) {
        superType = getTypeDescriptorFromClassDescriptor(type->manager->System_ValueType);
    } else {
        superType = getTypeDescriptorFromClassDescriptor(type->manager->System_Object);
    }
    return superType;
}


inline JITUINT32 getIRTypeFromGenericVarDescriptor (GenericVarDescriptor *type) {
    JITUINT32 irType;

    if (type->attributes->isReference) {
        irType = IROBJECT;
    } else if (type->attributes->isValueType) {
        irType = IRVALUETYPE;
    } else {
        XanList *constraints = getConstraintsFromGenericVarDescriptor(type);
        XanListItem *item = xanList_first(constraints);
        while (item != NULL) {
            TypeDescriptor *constraint = (TypeDescriptor *) item->data;
            if (getIRTypeFromTypeDescriptor(constraint) == IRVALUETYPE) {
                return IRVALUETYPE;
            }
            item = item->next;
        }
        PDEBUG("METADATA MANAGER: getIRTypeFromGenericVarDescriptor: ERROR! UNDEFINED FOR OPEN PARAMETER!\n");
        abort();
    }
    return irType;
}

inline XanList *getConstraintsFromGenericVarDescriptor (GenericVarDescriptor *type) {
    if (!WAIT_TICKET(type, CONSTRAINT_TICKET)) {
        /*Not already cached */
        type->constraints = xanList_new(sharedAllocFunction, freeFunction, NULL);

        t_binary_information *binary = (t_binary_information *) type->row->binary;
        XanList *list = getRowByKey(type->manager, binary, GENERIC_PARAM_CONSTRAINT, (void *)(JITNUINT) type->row->index);
        if (list != NULL) {
            GenericContainer *container = getResolvingContainerFromTypeDescriptor(type->owner);
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                t_row_generic_param_constraint_table *row = (t_row_generic_param_constraint_table *) item->data;
                TypeDescriptor *resolved_type = getTypeDescriptorFromTypeDeforRefToken(type->manager, container, binary, row->constraint);
                xanList_append(type->constraints, resolved_type);
                item = item->next;
            }
        }
        BROADCAST_TICKET(type, CONSTRAINT_TICKET);
    }
    return type->constraints;
}

inline void genericVarDescriptorLock (GenericVarDescriptor *type) {
    PLATFORM_lockMutex(&(type->mutex));
}

inline void genericVarDescriptorUnLock (GenericVarDescriptor *type) {
    PLATFORM_unlockMutex(&(type->mutex));
}

inline DescriptorMetadataTicket *genericVarDescriptorCreateDescriptorMetadataTicket (GenericVarDescriptor *type_desc, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(type_desc->metadata), type);
}

inline void genericVarDescriptorBroadcastDescriptorMetadataTicket (GenericVarDescriptor *type,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}

/* Generic Method Var Descriptor Accessor Methods */
JITUINT32 hashGenericMethodVarDescriptor (void *element) {
    if (element == NULL) {
        return 0;
    }
    JITUINT32 seed;
    GenericMethodVarDescriptor *type = (GenericMethodVarDescriptor *) element;
    seed = (JITUINT32) type->row;
    return combineHash((JITUINT32) type->owner, seed);
}

JITINT32 equalsGenericMethodVarDescriptor (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    GenericMethodVarDescriptor *type1 = (GenericMethodVarDescriptor *) key1;
    GenericMethodVarDescriptor *type2 = (GenericMethodVarDescriptor *) key2;
    return type1->row == type2->row && type1->owner == type2->owner;
}

inline GenericMethodVarDescriptor *newGenericMethodVarDescriptor (metadataManager_t *manager, BasicGenericParam *row, MethodDescriptor *owner) {
    if (row == NULL) {
        return NULL;
    }
    GenericMethodVarDescriptor param;
    param.manager = manager;
    param.owner = owner;
    param.row = row;
    GenericMethodVarDescriptor *found = (GenericMethodVarDescriptor *) xanHashTable_lookup(manager->genericMethodVarDescriptors, (void *) &param);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(GenericMethodVarDescriptor));
        found->manager = manager;
        found->row = row;
        found->owner = owner;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(GENERIC_VAR_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = genericMethodVarDescriptorLock;
        found->unlock = genericMethodVarDescriptorUnLock;
        found->createDescriptorMetadataTicket = genericMethodVarDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = genericMethodVarDescriptorBroadcastDescriptorMetadataTicket;
        /* Init Default value */
        found->name = NULL;
        found->constraints = NULL;
        found->getName = getNameFromGenericMethodVarDescriptor;
        found->getConstraints = getConstraintsFromGenericMethodVarDescriptor;
        found->getIRType = getIRTypeFromGenericMethodVarDescriptor;
        found->getInstance = getInstanceFromGenericMethodVarDescriptor;
        /* end init Default value */
        found->attributes = getBasicGenericParamAttributes(manager, row);
        xanHashTable_insert(manager->genericMethodVarDescriptors, (void *) found, (void *) found);

    }
    return found;
}

inline JITINT8 *getNameFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type) {
    if (!WAIT_TICKET(type, NAME_TICKET)) {
        /* Not already Cached */
        t_binary_information *binary = (t_binary_information *) type->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_string_stream *string_stream = &(streams->string_stream);
        type->name = get_string(string_stream, type->row->name);
        BROADCAST_TICKET(type, NAME_TICKET);
    }
    return type->name;
}

inline ClassDescriptor *getUnderliningClassDescriptorFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type) {
    TypeDescriptor *superType;

    superType = getSuperTypeFromGenericMethodVarDescriptor(type);
    return getUnderliningClassDescriptorFromTypeDescriptor(superType);
}

inline TypeDescriptor *getInstanceFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type, GenericContainer *container) {
    TypeDescriptor *result;
    JITUINT32 number = type->row->number;

    if (container->container_type == METHOD_LEVEL) {
        result = container->paramType[number];
    } else {
        PDEBUG("METADATA MANAGER: getInstanceFromGenericMethodVarDescriptor: ERROR! " "INVALID CONTAINER LEVEL! %d\n", container->container_type);
        abort();
    }
    return result;
}

inline TypeDescriptor *getSuperTypeFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type) {
    TypeDescriptor *superType;

    initializeMetadata();
    XanList *constraints = getConstraintsFromGenericMethodVarDescriptor(type);
    if (xanList_length(constraints) == 1) {
        superType = (TypeDescriptor *) xanList_first(constraints)->data;
    } else if (type->attributes->isValueType) {
        superType = getTypeDescriptorFromClassDescriptor(type->manager->System_ValueType);
    } else {
        superType = getTypeDescriptorFromClassDescriptor(type->manager->System_Object);
    }
    return superType;
}



inline JITUINT32 getIRTypeFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type) {
    JITUINT32 irType;

    if (type->attributes->isReference) {
        irType = IROBJECT;
    } else if (type->attributes->isValueType) {
        irType = IRVALUETYPE;
    } else {
        XanList *constraints = getConstraintsFromGenericMethodVarDescriptor(type);
        XanListItem *item = xanList_first(constraints);
        while (item != NULL) {
            TypeDescriptor *constraint = (TypeDescriptor *) item->data;
            if (getIRTypeFromTypeDescriptor(constraint) == IRVALUETYPE) {
                return IRVALUETYPE;
            }
            item = item->next;
        }
        PDEBUG("METADATA MANAGER: getIRTypeFromGenericMethodVarDescriptor: ERROR! UNDEFINED FOR OPEN PARAMETER!\n");
        abort();
    }
    return irType;
}

inline XanList *getConstraintsFromGenericMethodVarDescriptor (GenericMethodVarDescriptor *type) {
    if (!WAIT_TICKET(type, CONSTRAINT_TICKET)) {
        /*Not already cached */
        type->constraints = xanList_new(sharedAllocFunction, freeFunction, NULL);

        t_binary_information *binary = (t_binary_information *) type->row->binary;
        XanList *list = getRowByKey(type->manager, binary, GENERIC_PARAM_CONSTRAINT, (void *) type->row->index);
        if (list != NULL) {
            GenericContainer *container = getResolvingContainerFromMethodDescriptor(type->owner);
            XanListItem *item = xanList_first(list);
            while (item != NULL) {
                t_row_generic_param_constraint_table *row = (t_row_generic_param_constraint_table *) item->data;
                TypeDescriptor *resolved_type = getTypeDescriptorFromTypeDeforRefToken(type->manager, container, binary, row->constraint);
                ClassDescriptor *class = getUnderliningClassDescriptorFromTypeDescriptor(resolved_type);
                xanList_append(type->constraints, class);
                item = item->next;
            }
        }
        BROADCAST_TICKET(type, CONSTRAINT_TICKET);
    }
    return type->constraints;
}

inline void genericMethodVarDescriptorLock (GenericMethodVarDescriptor *type) {
    PLATFORM_lockMutex(&(type->mutex));
}

inline void genericMethodVarDescriptorUnLock (GenericMethodVarDescriptor *type) {
    PLATFORM_unlockMutex(&(type->mutex));
}

inline DescriptorMetadataTicket *genericMethodVarDescriptorCreateDescriptorMetadataTicket (GenericMethodVarDescriptor *type_desc, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(type_desc->metadata), type);
}

inline void genericMethodVarDescriptorBroadcastDescriptorMetadataTicket (GenericMethodVarDescriptor *type,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}


