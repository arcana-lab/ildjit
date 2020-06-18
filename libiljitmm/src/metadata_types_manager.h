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


// My headers
#ifndef METADATA_TYPES_MANAGER_H
#define METADATA_TYPES_MANAGER_H


#include <metadata_types.h>
#include <generic_container.h>
#include <class_descriptor.h>
#include <method_descriptor.h>
#include <field_descriptor.h>
#include <type_descriptor.h>
#include <automatic_type_descriptor.h>
#include <property_descriptor.h>
#include <event_descriptor.h>
#include <var_descriptor.h>

typedef struct _GenericParameterRule {
    TypeDescriptor *owner;
    JITUINT32 var;
    TypeSpecDescriptor *replace;
} GenericParameterRule;
JITUINT32 hashGenericParameterRule (void *element);
JITINT32 equalsGenericParameterRule (void *key1, void *key2);

/* RowObject ID Method */
typedef struct {
    t_binary_information  *binary;
    JITUINT32 tableID;
    JITUINT32 rowID;
    ILConcept concept;
    GenericContainer *container;
    void *object;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} RowObjectID;

typedef enum _TableIndexes {
    INTERFACE,
    EXPLICIT_OVERRIDE,
    FIELD_RVA,
    FIELD_OFFSET,
    METHOD_SEMANTICS,
    CLASS_LAYOUT,
    CLASS_GENERIC_PARAM,
    METHOD_GENERIC_PARAM,
    NESTED_CLASS,
    ENCLOSING_CLASS,
    PROPERTIES,
    EVENTS,
    PROPERTY_METHOD,
    EVENT_METHOD,
    GENERIC_PARAM_CONSTRAINT,
    IMPLEMENTATION_MAP
} TableIndexes;

/* Ticket */
typedef struct _Ticket {
    JITBOOLEAN completed;

    pthread_mutex_t mutex;
} Ticket;

Ticket *allocTicketArray (JITUINT32 count);
JITBOOLEAN waitTicket (Ticket *metadata);
void broadcastTicket (Ticket *metadata);

#define TICKET(descriptor, type) (& ((Ticket *) descriptor->tickets)[type])
#define WAIT_TICKET(descriptor, type) waitTicket(TICKET(descriptor, type))
#define BROADCAST_TICKET(descriptor, type) broadcastTicket(TICKET(descriptor, type))

inline RowObjectID *createObjectRowID (metadataManager_t *manager, t_binary_information *binary, GenericContainer *container, JITUINT32 tableID, JITUINT32 rowID, ILConcept concept);
inline void broadcastObjectRowID (metadataManager_t *manager, RowObjectID *rowObject, void *object);

inline void initializeMetadata (void);

inline MethodDescriptor *getMethodDescriptorFromEntryPoint (metadataManager_t * manager, t_binary_information *binary);

inline ClassDescriptor *getClassDescriptorFromNameAndAssembly (metadataManager_t *self, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name);
inline ClassDescriptor * getClassDescriptorFromName (metadataManager_t *manager, JITINT8 *type_name_space, JITINT8 *type_name);
inline ClassDescriptor *getClassDescriptorFromElementType (metadataManager_t *manager, JITUINT32 element_type);
inline ClassDescriptor *getClassDescriptorFromIRType (metadataManager_t *manager, JITUINT32 IRType);

inline TypeDescriptor *getTypeDescriptorFromNameAndAssembly (metadataManager_t *self, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name);
inline TypeDescriptor * getTypeDescriptorFromName (metadataManager_t *manager, JITINT8 *type_name_space, JITINT8 *type_name);
inline TypeDescriptor *getTypeDescriptorFromElementType (metadataManager_t *manager, JITUINT32 ELEMENT_TYPE_type);
inline TypeDescriptor *getTypeDescriptorFromIRType (metadataManager_t *manager, JITUINT32 IRType);

TypeDescriptor *metadataManagerMakeArrayDescriptor (metadataManager_t *manager, TypeDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds);
TypeDescriptor *metadataManagerMakeFunctionPointerDescriptor (metadataManager_t *manager, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeDescriptor *result, XanList *params);

inline ILConcept fetchDescriptor (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token, void **descriptor);
inline TypeDescriptor *getTypeDescriptorFromToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token);
inline ActualMethodDescriptor getMethodDescriptorFromToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token);
inline FieldDescriptor  *getFieldDescriptorFromToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token);

XanList *getMethodsFromAssembly (metadataManager_t *manager, BasicAssembly *assembly);
BasicAssembly *metadataManagerGetAssemblyFromName (metadataManager_t *manager, JITINT8 *name);

JITINT32 getIndexOfTypeDescriptor (TypeDescriptor *type);

void *getRowByKey (metadataManager_t *manager, t_binary_information *binary, TableIndexes table, void *key);

#endif
