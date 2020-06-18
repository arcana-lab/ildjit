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
#define SEMANTIC_TICKET 3

#define PROPERTY_TICKET_COUNT 4

/* PropertyDescriptor Accessor Method */
static inline void internalParsePropertyMethodSemantics (PropertyDescriptor *property);

JITUINT32 hashPropertyDescriptor (void *element) {
    if (element == NULL) {
        return 0;
    }
    PropertyDescriptor *property = (PropertyDescriptor *) element;
    JITUINT32 seed = combineHash((JITUINT32) property->row, (JITUINT32) property->owner);
    return seed;
}

JITINT32 equalsPropertyDescriptor (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1 == key2) {
        return 1;
    }
    PropertyDescriptor *property1 = (PropertyDescriptor *) key1;
    PropertyDescriptor *property2 = (PropertyDescriptor *) key2;
    return property1->row == property2->row && property1->owner == property2->owner;
}

inline PropertyDescriptor *newPropertyDescriptor (metadataManager_t *manager, BasicProperty *row, TypeDescriptor *owner) {
    if (row == NULL) {
        return NULL;
    }
    PropertyDescriptor property;
    property.manager = manager;
    property.owner = owner;
    property.row = row;
    PropertyDescriptor *found = (PropertyDescriptor *) xanHashTable_lookup(manager->propertyDescriptors, (void *) &property);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(PropertyDescriptor));
        found->manager = manager;
        found->row = row;
        found->owner = owner;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(PROPERTY_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = propertyDescriptorLock;
        found->unlock = propertyDescriptorUnLock;
        found->createDescriptorMetadataTicket = propertyDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = propertyDescriptorBroadcastDescriptorMetadataTicket;
        /* Init Default value */
        found->name = NULL;
        found->completeName = NULL,
               found->type = NULL;
        found->other = NULL;
        found->getter = NULL;
        found->setter = NULL;
        found->getName = getNameFromPropertyDescriptor;
        found->getCompleteName = getCompleteNameFromPropertyDescriptor;
        found->getType = getTypeDescriptorFromProperty;
        found->getGetter = getGetter;
        found->getSetter = getSetter;
        found->getOther = getOtherFromProperty;
        /* end init Default value */
        found->attributes = getBasicPropertyAttributes(manager, row);
        xanHashTable_insert(manager->propertyDescriptors, (void *) found, (void *) found);

    }
    return found;
}

inline TypeDescriptor *getTypeDescriptorFromProperty (PropertyDescriptor *property) {
    if (!WAIT_TICKET(property, TYPE_TICKET)) {
        /* if not already loaded */
        MethodDescriptor *getter = getGetter(property);
        property->type = getter->getResult(getter)->type;
        BROADCAST_TICKET(property, TYPE_TICKET);
    }
    return property->type;
}

inline JITINT8 *getNameFromPropertyDescriptor (PropertyDescriptor *property) {
    if (!WAIT_TICKET(property, NAME_TICKET)) {
        /* if not already loaded */
        t_binary_information *binary = (t_binary_information *) property->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_string_stream *string_stream = &(streams->string_stream);
        property->name = get_string(string_stream, property->row->name);
        BROADCAST_TICKET(property, NAME_TICKET);
    }
    return property->name;
}

inline JITINT8 *getCompleteNameFromPropertyDescriptor (PropertyDescriptor *property) {
    if (!WAIT_TICKET(property, COMPLETE_NAME_TICKET)) {
        /* if not already loaded */
        JITINT8 *completeTypeName = getCompleteNameFromTypeDescriptor(property->owner);
        JITINT8 *name = getNameFromPropertyDescriptor(property);
        size_t length = sizeof(JITINT8) * (STRLEN(name) + STRLEN(completeTypeName) + 1 + 1);
        property->completeName = sharedAllocFunction(length);
        SNPRINTF(property->completeName, length, "%s.%s", completeTypeName, name);
        BROADCAST_TICKET(property, COMPLETE_NAME_TICKET);
    }
    return property->completeName;
}

static inline void internalParsePropertyMethodSemantics (PropertyDescriptor *property) {
    t_binary_information *binary = (t_binary_information *) property->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanList *list = getRowByKey(property->manager, binary, PROPERTY_METHOD, (void *) property->row->index);

    if (list != NULL) {
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            t_row_method_semantics_table *line = (t_row_method_semantics_table *) item->data;
            BasicMethod *row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, line->method);
            MethodDescriptor *found_method = newMethodDescriptor(property->manager, row, property->owner, NULL);
            if ((line->semantics & 0x04) == 0x04) {
                property->other = found_method;
            } else if ((line->semantics & 0x02) == 0x02) {
                property->getter = found_method;
            } else if ((line->semantics & 0x01) == 0x01) {
                property->setter = found_method;
            }
            item = item->next;
        }
    }
}

inline MethodDescriptor * getOtherFromProperty (PropertyDescriptor *property) {
    if (!WAIT_TICKET(property, SEMANTIC_TICKET)) {
        internalParsePropertyMethodSemantics(property);
        BROADCAST_TICKET(property, SEMANTIC_TICKET);
    }
    return property->other;
}

inline MethodDescriptor *getGetter (PropertyDescriptor *property) {
    if (!WAIT_TICKET(property, SEMANTIC_TICKET)) {
        internalParsePropertyMethodSemantics(property);
        BROADCAST_TICKET(property, SEMANTIC_TICKET);
    }
    return property->getter;
}

inline MethodDescriptor *getSetter (PropertyDescriptor *property) {
    if (!WAIT_TICKET(property, SEMANTIC_TICKET)) {
        internalParsePropertyMethodSemantics(property);
        BROADCAST_TICKET(property, SEMANTIC_TICKET);
    }
    return property->setter;
}

inline void propertyDescriptorLock (PropertyDescriptor *property) {
    PLATFORM_lockMutex(&(property->mutex));
}

inline void propertyDescriptorUnLock (PropertyDescriptor *property) {
    PLATFORM_unlockMutex(&(property->mutex));
}

inline DescriptorMetadataTicket * propertyDescriptorCreateDescriptorMetadataTicket (PropertyDescriptor *property, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(property->metadata), type);
}

inline void  propertyDescriptorBroadcastDescriptorMetadataTicket (PropertyDescriptor *property,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}
