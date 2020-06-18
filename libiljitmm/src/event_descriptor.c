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

#define EVENT_TICKET_COUNT 4

/* EventDescriptor Accessor Method */
static inline void internalParseEventMethodSemantics (EventDescriptor *event);

JITUINT32 hashEventDescriptor (void *element) {
    if (element == NULL) {
        return 0;
    }
    EventDescriptor *event = (EventDescriptor *) element;
    JITUINT32 seed = combineHash((JITUINT32) event->row, (JITUINT32) event->owner);
    return seed;
}

JITINT32 equalsEventDescriptor (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    EventDescriptor *event1 = (EventDescriptor *) key1;
    EventDescriptor *event2 = (EventDescriptor *) key2;
    return event1->row == event2->row && event1->owner == event2->owner;
}

inline EventDescriptor *newEventDescriptor (metadataManager_t *manager, BasicEvent *row, TypeDescriptor *owner) {
    if (row == NULL) {
        return NULL;
    }
    EventDescriptor event;
    event.manager = manager;
    event.owner = owner;
    event.row = row;
    xanHashTable_lock(manager->eventDescriptors);
    EventDescriptor *found = (EventDescriptor *) xanHashTable_lookup(manager->eventDescriptors, (void *) &event);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(EventDescriptor));
        found->manager = manager;
        found->row = row;
        found->owner = owner;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(EVENT_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = eventDescriptorLock;
        found->unlock = eventDescriptorUnLock;
        found->createDescriptorMetadataTicket = eventDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = eventDescriptorBroadcastDescriptorMetadataTicket;
        /* Init Default value */
        found->name = NULL;
        found->completeName = NULL,
               found->type = NULL;
        found->other = NULL;
        found->addon = NULL;
        found->removeon = NULL;
        found->fire = NULL;
        found->getName = getNameFromEventDescriptor;
        found->getCompleteName = getCompleteNameFromEventDescriptor;
        found->getType = getTypeDescriptorFromEvent;
        found->getAddOn = getAddOn;
        found->getRemoveOn = getRemoveOn;
        found->getFire = getFire;
        found->getOther = getOtherFromEvent;
        /* end init Default value */
        found->attributes = getBasicEventAttributes(manager, row);
        xanHashTable_insert(manager->eventDescriptors, (void *) found, (void *) found);

    }
    xanHashTable_unlock(manager->eventDescriptors);
    return found;
}

inline TypeDescriptor *getTypeDescriptorFromEvent (EventDescriptor *event) {
    if (event->row->event_type == 0) {
        return NULL;
    }
    if (!WAIT_TICKET(event, TYPE_TICKET)) {
        /* if not already loaded */
        GenericContainer *container = getResolvingContainerFromTypeDescriptor(event->owner);
        event->type = getTypeDescriptorFromTypeDeforRefToken(event->manager, container, (t_binary_information *) event->row->binary, event->row->event_type);
        BROADCAST_TICKET(event, TYPE_TICKET);
    }
    return event->type;
}

inline JITINT8 *getNameFromEventDescriptor (EventDescriptor *event) {
    if (!WAIT_TICKET(event, NAME_TICKET)) {
        /* if not already loaded */
        t_binary_information *binary = (t_binary_information *) event->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_string_stream *string_stream = &(streams->string_stream);
        event->name = get_string(string_stream, event->row->name);
        BROADCAST_TICKET(event, NAME_TICKET);
    }
    return event->name;
}

inline JITINT8 *getCompleteNameFromEventDescriptor (EventDescriptor *event) {
    if (!WAIT_TICKET(event, COMPLETE_NAME_TICKET)) {
        /* if not already loaded */
        JITINT8 *completeTypeName = getCompleteNameFromTypeDescriptor(event->owner);
        JITINT8 *name = getNameFromEventDescriptor(event);
        size_t length = sizeof(JITINT8) * (STRLEN(name) + STRLEN(completeTypeName) + 1 + 1);
        event->completeName = sharedAllocFunction(length);
        SNPRINTF(event->completeName, length, "%s.%s", completeTypeName, name);
        BROADCAST_TICKET(event, COMPLETE_NAME_TICKET);
    }
    return event->completeName;
}

static inline void internalParseEventMethodSemantics (EventDescriptor *event) {
    t_binary_information *binary = (t_binary_information *) event->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanList *list = getRowByKey(event->manager, binary, EVENT_METHOD, (void *) event->row->index);

    if (list != NULL) {
        XanListItem *item = xanList_first(list);
        while (item != NULL) {
            t_row_method_semantics_table *line = (t_row_method_semantics_table *) item->data;
            BasicMethod *row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, line->method);
            MethodDescriptor *found_method = newMethodDescriptor(event->manager, row, event->owner, NULL);
            if ((line->semantics & 0x04) == 0x04) {
                event->other = found_method;
            } else if ((line->semantics & 0x08) == 0x08) {
                event->addon = found_method;
            } else if ((line->semantics & 0x010) == 0x010) {
                event->removeon = found_method;
            } else if ((line->semantics & 0x020) == 0x020) {
                event->fire = found_method;
            }
            item = item->next;
        }
    }
}

inline MethodDescriptor * getOtherFromEvent (EventDescriptor *event) {
    if (!WAIT_TICKET(event, SEMANTIC_TICKET)) {
        internalParseEventMethodSemantics(event);
        BROADCAST_TICKET(event, SEMANTIC_TICKET);
    }
    return event->other;
}

inline MethodDescriptor *getAddOn (EventDescriptor *event) {
    if (!WAIT_TICKET(event, SEMANTIC_TICKET)) {
        internalParseEventMethodSemantics(event);
        BROADCAST_TICKET(event, SEMANTIC_TICKET);
    }
    return event->addon;
}

inline MethodDescriptor *getRemoveOn (EventDescriptor *event) {
    if (!WAIT_TICKET(event, SEMANTIC_TICKET)) {
        internalParseEventMethodSemantics(event);
        BROADCAST_TICKET(event, SEMANTIC_TICKET);
    }
    return event->removeon;
}

inline MethodDescriptor *getFire (EventDescriptor *event) {
    if (!WAIT_TICKET(event, SEMANTIC_TICKET)) {
        internalParseEventMethodSemantics(event);
        BROADCAST_TICKET(event, SEMANTIC_TICKET);
    }
    return event->fire;
}

inline void eventDescriptorLock (EventDescriptor *event) {
    PLATFORM_lockMutex(&(event->mutex));
}

inline void eventDescriptorUnLock (EventDescriptor *event) {
    PLATFORM_unlockMutex(&(event->mutex));
}

inline DescriptorMetadataTicket *eventDescriptorCreateDescriptorMetadataTicket (EventDescriptor *event, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(event->metadata), type);
}

inline void eventDescriptorBroadcastDescriptorMetadataTicket (EventDescriptor *event,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}
