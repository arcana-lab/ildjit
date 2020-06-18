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
#include <decoding_tools.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>
#include <stdlib.h>
#include <platform_API.h>

#define TYPE_TICKET 0
#define NAME_TICKET 1
#define COMPLETE_NAME_TICKET 2

#define FIELD_TICKET_COUNT 3

/* FieldDescriptor Accessor Method */

JITUINT32 hashFieldDescriptor (void *element) {
    FieldDescriptor *field = (FieldDescriptor *) element;
    JITUINT32 seed = combineHash((JITUINT32) field->row, (JITUINT32) field->owner);

    return seed;
}

JITINT32 equalsFieldDescriptor (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    FieldDescriptor *field1 = (FieldDescriptor *) key1;
    FieldDescriptor *field2 = (FieldDescriptor *) key2;
    return field1->row == field2->row && field1->owner == field2->owner;
}

inline FieldDescriptor *newFieldDescriptor (metadataManager_t *manager, BasicField *row, TypeDescriptor *owner) {
    if (row == NULL) {
        return NULL;
    }
    FieldDescriptor field;
    field.manager = manager;
    field.owner = owner;
    field.row = row;
    xanHashTable_lock(manager->fieldDescriptors);
    FieldDescriptor *found = (FieldDescriptor *) xanHashTable_lookup(manager->fieldDescriptors, (void *) &field);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(FieldDescriptor));
        found->manager = manager;
        found->row = row;
        found->owner = owner;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(FIELD_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = fieldDescriptorLock;
        found->unlock = fieldDescriptorUnLock;
        found->createDescriptorMetadataTicket = fieldDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = fieldDescriptorBroadcastDescriptorMetadataTicket;
        /* Init Default value */
        found->name = NULL;
        found->completeName = NULL,
               found->type = NULL;
        found->getName = getNameFromFieldDescriptor;
        found->getCompleteName = getCompleteNameFromFieldDescriptor;
        found->getType = getTypeDescriptorFromField;
        found->isAccessible = isFieldAccessible;
        /* end init Default value */
        found->attributes = getBasicFieldAttributes(manager, row);
        xanHashTable_insert(manager->fieldDescriptors, (void *) found, (void *) found);

    }
    xanHashTable_unlock(manager->fieldDescriptors);
    return found;
}

inline TypeDescriptor *getTypeDescriptorFromField (FieldDescriptor *field) {
    if (!WAIT_TICKET(field, TYPE_TICKET)) {
        /* if not already loaded */
        t_binary_information *binary = (t_binary_information *) field->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_blob_stream *blob_stream = &(streams->blob_stream);

        t_row_blob_table blob;
        get_blob(blob_stream, field->row->signature, &blob);
        JITINT8 *signature = blob.blob;
        JITUINT32 bytesRead = 0;
        bytesRead++;
        while (!isType(signature + bytesRead)) {
            bytesRead += skipCustomMod(signature + bytesRead);
        }
        GenericContainer *container = getResolvingContainerFromTypeDescriptor(field->owner);
        getTypeDescriptorFromTypeBlob(field->manager, container, binary, signature + bytesRead, &(field->type));
        BROADCAST_TICKET(field, TYPE_TICKET);
    }
    return field->type;
}

inline JITINT8 *getNameFromFieldDescriptor (FieldDescriptor *field) {
    if (!WAIT_TICKET(field, NAME_TICKET)) {
        /* if not already loaded */
        t_binary_information *binary = (t_binary_information *) field->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_string_stream *string_stream = &(streams->string_stream);
        field->name = get_string(string_stream, field->row->name);
        BROADCAST_TICKET(field, NAME_TICKET);
    }
    return field->name;
}

inline JITINT8 *getCompleteNameFromFieldDescriptor (FieldDescriptor *field) {
    if (!WAIT_TICKET(field, COMPLETE_NAME_TICKET)) {
        /* if not already loaded */
        JITINT8 *completeTypeName = getCompleteNameFromTypeDescriptor(field->owner);
        JITINT8 *name = getNameFromFieldDescriptor(field);
        size_t length = sizeof(JITINT8) * (STRLEN(name) + STRLEN(completeTypeName) + 1 + 1);
        field->completeName = sharedAllocFunction(length);
        SNPRINTF(field->completeName, length, "%s.%s", completeTypeName, name);
        BROADCAST_TICKET(field, COMPLETE_NAME_TICKET);
    }
    return field->completeName;
}

inline JITBOOLEAN isFieldAccessible (FieldDescriptor *field, MethodDescriptor *caller) {
    BasicFieldAttributes *field_attributes = field->attributes;

    switch (field_attributes->accessMask) {
        case COMPILER_CONTROLLED_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : Member not referenceable\n");
            return JITFALSE;
        case PRIVATE_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is private \n");
            if (caller->row->binary != field->row->binary) {
                PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is defined in an assembly different from the field's assembly\n");
                return JITFALSE;
            }
            if (caller->owner == field->owner) {
                PDEBUG("METADATA MANAGER : isFieldAccessible : field is defined in the same type of caller\n");
                return JITTRUE;
            }
            TypeDescriptor *parent_type = getEnclosingType(caller->owner);
            while (parent_type != NULL) {
                if (parent_type == field->owner) {
                    return JITTRUE;
                }
                parent_type = getEnclosingType(parent_type);
            }
            return JITFALSE;
        case FAM_AND_ASSEM_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is FAM_AND_ASSEM_FIELD \n");
            if (caller->row->binary != field->row->binary) {
                PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is defined in an assembly different from the field's assembly\n");
                return JITFALSE;
            }
            return typeDescriptorIsSubtypeOf(caller->owner, field->owner);
        case ASSEM_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is ASSEM_FIELD \n");
            return caller->row->binary == field->row->binary;
        case FAMILY_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is FAMILY_FIELD \n");
            return typeDescriptorIsSubtypeOf(caller->owner, field->owner);
        case FAM_OR_ASSEM_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : callee method is FAM_OR_ASSEM_FIELD \n");
            if (caller->row->binary != field->row->binary) {
                return typeDescriptorIsSubtypeOf(caller->owner, field->owner);
            }
            return JITTRUE;
        case PUBLIC_FIELD:
            PDEBUG("METADATA MANAGER : isFieldAccessible : field is public \n");
            return JITTRUE;
        default:
            abort();
    }
    return JITFALSE;
}

inline void fieldDescriptorLock (FieldDescriptor *field) {
    PLATFORM_lockMutex(&(field->mutex));
}

inline void fieldDescriptorUnLock (FieldDescriptor *field) {
    PLATFORM_unlockMutex(&(field->mutex));
}

inline DescriptorMetadataTicket *fieldDescriptorCreateDescriptorMetadataTicket (FieldDescriptor *field, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(field->metadata), type);
}

inline void fieldDescriptorBroadcastDescriptorMetadataTicket (FieldDescriptor *field,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}
