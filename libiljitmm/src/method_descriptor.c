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
#include <type_spec_descriptor.h>
#include <decoding_tools.h>
#include <type_spec_decoding_tool.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>
#include <stdlib.h>
#include <assert.h>
#include <platform_API.h>

#define NAME_TICKET 0
#define STRING_SIGNATURE_TICKET 1
#define COMPLETE_NAME_TICKET 2
#define INTERNAL_NAME_TICKET 3
#define PINVOKE_TICKET 4
#define PROPERTY_TICKET 5
#define EVENT_TICKET 6
#define SIGNATURE_TICKET 7
#define GENERIC_PARAM_TICKET 8
#define RESOLVING_CONTAINER_TICKET 9
#define METHOD_TICKET_COUNT 10
#define PARAM_TICKET_COUNT 2

static inline JITBOOLEAN checkMethodDescriptorContainerConsistency (MethodDescriptor *method, GenericContainer *container);
static inline void internalGetNameFromMethodDescriptor (MethodDescriptor *method);
static inline void internalParseMethodSignature (MethodDescriptor *method);
static inline void internalGetImport (MethodDescriptor *method);

/* Actual Method Descriptor utility methods */

ActualMethodDescriptor *newActualMethodDescriptor () {
    ActualMethodDescriptor *result = sharedAllocFunction(sizeof(ActualMethodDescriptor));

    return result;
}

/* MethodDescriptor Accessor Method */

JITUINT32 hashMethodDescriptor (MethodDescriptor *method) {
    if (method == NULL) {
        return 0;
    }
    JITUINT32 seed = hashGenericContainer(method->container);
    seed = combineHash(seed, (JITUINT32) method->owner);
    seed = combineHash(seed, (JITUINT32) method->row);
    return seed;
}

JITINT32 equalsMethodDescriptor (MethodDescriptor *key1, MethodDescriptor *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->row != key2->row) {
        return 0;
    }
    if (key1->owner != key2->owner) {
        return 0;
    }
    return equalsGenericContainer(key1->container, key2->container);
}

static inline JITBOOLEAN checkMethodDescriptorContainerConsistency (MethodDescriptor *method, GenericContainer *container) {
    return JITTRUE;
}

inline MethodDescriptor *newMethodDescriptor (metadataManager_t *manager, BasicMethod *row, TypeDescriptor *owner, GenericContainer *container) {
    if (row == NULL) {
        return NULL;
    }
    MethodDescriptor method;
    method.manager = manager;
    method.row = row;
    method.owner = owner;
    method.container = container;
    xanHashTable_lock(manager->methodDescriptors);
    MethodDescriptor *found = (MethodDescriptor *) xanHashTable_lookup(manager->methodDescriptors, (void *) &method);
    if (found == NULL) {
        found = sharedAllocFunction(sizeof(MethodDescriptor));
        found->manager = manager;
        found->row = row;
        found->owner = owner;
        found->container = cloneGenericContainer(container);
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        PLATFORM_initMutex(&(found->mutex), &mutex_attr);
        found->tickets = allocTicketArray(METHOD_TICKET_COUNT);
        init_DescriptorMetadata(&(found->metadata));
        found->lock = methodDescriptorLock;
        found->unlock = methodDescriptorUnLock;
        found->createDescriptorMetadataTicket = methodDescriptorCreateDescriptorMetadataTicket;
        found->broadcastDescriptorMetadataTicket = methodDescriptorBroadcastDescriptorMetadataTicket;
        found->resolving_container = NULL;
        found->getResolvingContainer = getResolvingContainerFromMethodDescriptor;
        /* Init Default value */
        found->name = NULL;
        found->completeName = NULL;
        found->event = NULL;
        found->property = NULL;
        found->signatureInString = NULL;
        found->signature = NULL;
        found->result = NULL;
        found->params = NULL;
        found->importName = NULL;
        found->importModule = NULL;
        found->getResolvingContainer = getResolvingContainerFromMethodDescriptor;
        found->requireConservativeOwnerInitialization = requireConservativeOwnerInitialization;
        found->getParams = getParams;
        found->getResult = getResult;
        found->getName = getNameFromMethodDescriptor;
        found->getCompleteName = getCompleteNameFromMethodDescriptor;;
        found->getSignatureInString = getMethodDescriptorSignatureInString;
        found->getInternalName = getMethodDescriptorInternalName;
        found->getFormalSignature = getFormalSignature;
        found->getOverloadingSequenceID = getOverloadingSequenceID;
        found->getBaseDefinition = getBaseDefinition;
        found->isCallable = isCallebleMethod;
        found->isCtor = isCtor;
        found->isCctor = isCctor;
        found->isFinalizer = isFinalizer;
        found->getImportName = getImportName;
        found->getImportModule = getImportModule;
        found->getProperty = getProperty;
        found->getEvent = getEvent;
        found->isGenericMethodDefinition = isGenericMethodDefinition;
        found->containOpenGenericParameter = containOpenGenericParameterFromMethodDescriptor;
        found->isGeneric = isGenericMethodDescriptor;
        found->getInstance = getInstanceFromMethodDescriptor;
        found->getGenericDefinition = getGenericDefinitionFromMethodDescriptor;
        found->getGenericParameters = getGenericParametersFromMethodDescriptor;
        /* end init Default value */
        found->attributes = getBasicMethodAttributes(manager, row);
        xanHashTable_insert(manager->methodDescriptors, (void *) found, (void *) found);
    }
    xanHashTable_unlock(manager->methodDescriptors);
    return found;
}

inline JITBOOLEAN requireConservativeOwnerInitialization (MethodDescriptor *method) {
    JITBOOLEAN triggerMethod;
    JITBOOLEAN allowsRelaxedInitialization;
    TypeDescriptor *definingType = method->owner;

    triggerMethod = isCtor(method) || (method->attributes->is_static && !isCctor(method));
    allowsRelaxedInitialization = definingType->attributes->allowsRelaxedInitialization;
    return triggerMethod && !allowsRelaxedInitialization;
}

static inline void internalGetNameFromMethodDescriptor (MethodDescriptor *method) {
    t_binary_information *binary = (t_binary_information *) method->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_string_stream *string_stream = &(streams->string_stream);

    JITINT8 *basicMethodName = get_string(string_stream, method->row->name);

    if (method->container != NULL) {
        JITUINT32 count;
        GenericContainer *container = getResolvingContainerFromMethodDescriptor(method);
        JITUINT32 param_count = container->arg_count;
        JITINT8 **paramName = alloca(sizeof(JITINT8 *) * param_count);
        for (count = 0; count < param_count; count++) {
            paramName[count] = getCompleteNameFromTypeDescriptor(container->paramType[count]);
        }
        JITUINT32 alloc_length = STRLEN(basicMethodName) + 2;           // \0 plus generic open
        for (count = 0; count < param_count; count++) {
            alloc_length += STRLEN(paramName[count]) + 1;           // , or generic close
        }
        method->name = sharedAllocFunction(sizeof(JITINT8) * alloc_length);
        SNPRINTF(method->name, alloc_length, "%s<", basicMethodName);
        for (count = 0; count < param_count - 1; count++) {
            method->name = STRCAT(method->name, paramName[count]);
            method->name = STRCAT(method->name, ",");
        }
        method->name = STRCAT(method->name, paramName[count]);
        method->name = STRCAT(method->name, ">");
    } else {
        method->name = basicMethodName;
    }
}

inline JITINT8 *getNameFromMethodDescriptor (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, NAME_TICKET)) {
        internalGetNameFromMethodDescriptor(method);
        BROADCAST_TICKET(method, NAME_TICKET);
    }
    /*already cached */
    return method->name;
}

inline JITINT8 *getCompleteNameFromMethodDescriptor (MethodDescriptor *method) {
    if (!WAIT_TICKET(method,  COMPLETE_NAME_TICKET)) {
        /* if not already loaded */
        JITINT8 *completeTypeName = getCompleteNameFromTypeDescriptor(method->owner);
        JITINT8 *name = getNameFromMethodDescriptor(method);
        size_t length = sizeof(JITINT8) * (STRLEN(name) + STRLEN(completeTypeName) + 1 + 1);
        method->completeName = sharedAllocFunction(length);
        SNPRINTF(method->completeName, length, "%s.%s", completeTypeName, name);
        BROADCAST_TICKET(method, COMPLETE_NAME_TICKET);
    }
    return method->completeName;
}

inline JITINT8 *getMethodDescriptorSignatureInString (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, STRING_SIGNATURE_TICKET)) {
        /* if not already loaded */
        internalParseMethodSignature(method);
        JITINT8 *completeName = getCompleteNameFromMethodDescriptor(method);
        ParamDescriptor *result = getResult(method);
        JITINT8 *resultInString = getSignatureInStringFromParamDescriptor(result);
        XanList *params = getParams(method);
        JITUINT32 param_count = xanList_length(params);
        JITINT8 **paramInString = NULL;
        if (param_count > 0) {
            paramInString = alloca(sizeof(JITINT8 *) * param_count);
        }
        JITUINT32 count = 0;
        JITUINT32 char_count = STRLEN(resultInString) + 1 + STRLEN(completeName) + 1 + 1 + 1;
        XanListItem *item = xanList_first(params);
        while (item != NULL) {
            ParamDescriptor *param = (ParamDescriptor *) item->data;
            paramInString[count] = getSignatureInStringFromParamDescriptor(param);
            char_count += STRLEN(paramInString[count]) + 1;
            item = item->next;
            count++;
        }
        size_t length = sizeof(JITINT8) * (char_count);
        method->signatureInString = sharedAllocFunction(length);
        SNPRINTF(method->signatureInString, length, "%s %s(", resultInString, completeName);
        if (param_count > 0) {
            for (count = 0; count < param_count - 1; count++) {
                method->signatureInString = STRCAT(method->signatureInString, paramInString[count]);
                method->signatureInString = STRCAT(method->signatureInString, (JITINT8 *) ",");
            }
            method->signatureInString = STRCAT(method->signatureInString, paramInString[count]);
        }
        method->signatureInString = STRCAT(method->signatureInString, (JITINT8 *) ")");
        BROADCAST_TICKET(method, STRING_SIGNATURE_TICKET);
    }
    return method->signatureInString;
}

inline JITINT8 *getMethodDescriptorInternalName (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, INTERNAL_NAME_TICKET)) {
        /* if not already loaded */
        internalParseMethodSignature(method);
        JITINT8 *completeName = getCompleteNameFromMethodDescriptor(method);
        XanList *params = getParams(method);
        JITUINT32 param_count = xanList_length(params);
        JITINT8 **paramInString = NULL;
        if (param_count > 0) {
            paramInString = alloca(sizeof(JITINT8 *) * param_count);
            JITUINT32 count = 0;
            JITUINT32 char_count = STRLEN(completeName) + 1;
            XanListItem *item = xanList_first(params);
            while (item != NULL) {
                ParamDescriptor *param = (ParamDescriptor *) item->data;
                paramInString[count] = getNameFromTypeDescriptor(param->type);
                char_count += STRLEN(paramInString[count]) + 1 + 1;
                item = item->next;
                count++;
            }
            size_t length = sizeof(JITINT8) * (char_count);
            method->internalName = sharedAllocFunction(length);
            SNPRINTF(method->internalName, length, "%s_", completeName);
            for (count = 0; count < param_count - 1; count++) {
                method->internalName = STRCAT(method->internalName, paramInString[count]);
                method->internalName = STRCAT(method->internalName, (JITINT8 *) "_");
            }
            method->internalName = STRCAT(method->internalName, paramInString[count]);
        } else {
            method->internalName = getCompleteNameFromMethodDescriptor(method);

        }
        BROADCAST_TICKET(method, INTERNAL_NAME_TICKET);
    }
    return method->internalName;
}

inline JITBOOLEAN isCtor (MethodDescriptor *method) {
    if (!method->attributes->is_special_name) {
        return JITFALSE;
    }
    JITINT8 *name = getNameFromMethodDescriptor(method);
    return STRCMP(name, ".ctor") == 0;
}

inline JITBOOLEAN isCctor (MethodDescriptor *method) {
    if (!method->attributes->is_special_name) {
        return JITFALSE;
    }
    JITINT8 *name = getNameFromMethodDescriptor(method);
    return STRCMP(name, ".cctor") == 0;
}

inline JITBOOLEAN isFinalizer (MethodDescriptor *method) {
    JITINT8 *name = getNameFromMethodDescriptor(method);

    return STRCMP(name, "Finalize") == 0;
}

static inline void internalGetImport (MethodDescriptor *method) {
    t_binary_information *binary = (t_binary_information *) method->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_string_stream *string_stream = &(streams->string_stream);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    t_row_impl_map_table *row = (t_row_impl_map_table *) getRowByKey(method->manager, binary, IMPLEMENTATION_MAP, (void *) (JITNUINT)method->row->index);
    t_row_module_ref_table *module = (t_row_module_ref_table *) get_row(tables, MODULE_REF_TABLE, row->import_scope);

    method->importName = get_string(string_stream, row->import_name);
    method->importModule = get_string(string_stream, module->name);
}


inline JITINT8 *getImportName (MethodDescriptor *method) {
    if (!(method->attributes->is_pinvoke)) {
        return NULL;
    }
    if (!WAIT_TICKET(method, PINVOKE_TICKET)) {
        internalGetImport(method);
        BROADCAST_TICKET(method, PINVOKE_TICKET);
    }
    return method->importName;
}


inline JITINT8 *getImportModule (MethodDescriptor *method) {
    if (!(method->attributes->is_pinvoke)) {
        return NULL;
    }
    if (!WAIT_TICKET(method, PINVOKE_TICKET)) {
        internalGetImport(method);
        BROADCAST_TICKET(method, PINVOKE_TICKET);
    }
    return method->importModule;
}

inline MethodDescriptor *getInstanceFromMethodDescriptor (MethodDescriptor *method, GenericContainer *container) {
    assert(container != NULL);
    MethodDescriptor *result = NULL;
    if (isGenericMethodDefinition(method)) {
        if (checkMethodDescriptorContainerConsistency(method, container)) {
            result = newMethodDescriptor(method->manager, method->row, method->owner, container);
        }
    }
    return result;
}

inline FunctionPointerSpecDescriptor *getRawSignature (MethodDescriptor *method, GenericSpecContainer *container) {
    return getRawSignatureFromBlob(method->manager, container, method->row);
}


inline JITUINT32 getOverloadingSequenceID (MethodDescriptor *method) {
    JITUINT32 result = 0;

    if (isGenericMethodDescriptor(method)) {
        method = getGenericDefinitionFromMethodDescriptor(method);
    }

    JITINT8 *name = getNameFromMethodDescriptor(method);
    XanList *methodsList = getMethodDescriptorFromNameTypeDescriptor(method->owner, name);
    XanListItem *item = xanList_first(methodsList);
    while (item != NULL) {
        MethodDescriptor *currentMethod = (MethodDescriptor *) item->data;
        if (currentMethod == method) {
            return result;
        }
        result++;
        item = item->next;
    }

    return result;
}

inline MethodDescriptor *getBaseDefinition (MethodDescriptor *method) {
    MethodDescriptor *result = NULL;

    if (!(method->attributes->is_static || isCtor(method) || method->owner->attributes->isInterface)) {
        if (method->attributes->need_new_slot) {
            result = method;
        } else {
            XanHashTable *vtable = getVTableFromTypeDescriptor(method->owner, NULL);
            MethodDescriptor *method_to_search;
            if (method->attributes->param_count > 0) {
                method_to_search = getGenericDefinitionFromMethodDescriptor(method);
            } else {
                method_to_search = method;
            }
            VTableSlot *slot = (VTableSlot *) xanHashTable_lookup(vtable, method_to_search);
            if (slot->overriden != NULL) {
                slot = slot->overriden;
                if (method->attributes->param_count > 0) {
                    result = getInstanceFromMethodDescriptor(slot->declaration, method->container);
                } else {
                    result = slot->declaration;
                }
            }
        }
    }
    return result;
}

inline PropertyDescriptor *getProperty (MethodDescriptor *method) {
    if (method->attributes->association != PROPERTY_ASSOCIATION) {
        return NULL;
    }
    if (!WAIT_TICKET(method, PROPERTY_TICKET)) {
        t_binary_information *binary = (t_binary_information *) method->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_metadata_tables *tables = &((streams->not_stream).tables);

        t_row_method_semantics_table *line = getRowByKey(method->manager, binary, METHOD_SEMANTICS, (void *) (JITNUINT)method->row->index);
        JITUINT32 row_id = ((line->association >> 1) & 0x7FFFFFFF);
        BasicProperty *row = (BasicProperty *) get_row(tables, PROPERTY_TABLE, row_id);
        method->property = newPropertyDescriptor(method->manager, row, method->owner);
        BROADCAST_TICKET(method, PROPERTY_TICKET);
    }
    return method->property;
}

inline EventDescriptor *getEvent (MethodDescriptor *method) {
    if (method->attributes->association != EVENT_ASSOCIATION) {
        return NULL;
    }
    if (!WAIT_TICKET(method, EVENT_TICKET)) {
        t_binary_information *binary = (t_binary_information *) method->row->binary;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_metadata_tables *tables = &((streams->not_stream).tables);

        t_row_method_semantics_table *line = getRowByKey(method->manager, binary, METHOD_SEMANTICS, (void *) (JITNUINT)method->row->index);
        JITUINT32 row_id = ((line->association >> 1) & 0x7FFFFFFF);
        BasicEvent *row = (BasicEvent *) get_row(tables, EVENT_TABLE, row_id);
        method->event = newEventDescriptor(method->manager, row, method->owner);
        BROADCAST_TICKET(method, EVENT_TICKET);
    }
    return method->event;

}

static inline void internalParseMethodSignature (MethodDescriptor *method) {
    t_binary_information *binary = (t_binary_information *) method->row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    /* Assertions.
     */
    assert(method != NULL);

    GenericContainer *container = getResolvingContainerFromMethodDescriptor(method);

    /* Parse the signature.
     */
    if (method->signature == NULL) {
        method->signature = getFormalSignatureFromBlob(method->manager, container, method->row);
    }

    /* try to associate Param Attributes */
    t_param_table *param_table = get_table(tables, PARAM_TABLE);
    if (method->params == NULL) {
        method->params = xanList_new(sharedAllocFunction, freeFunction, NULL);
        if (param_table == NULL) {
            if (method->result == NULL) {
                method->result = newParamDescriptor(method->manager, NULL, method->signature->result, method);
            }
            return;
        }
    }
    BasicMethod *next_method_row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, method->row->index + 1);
    JITUINT32 last_param;
    if (next_method_row != NULL) {
        last_param = next_method_row->param_list - 1;
    } else {
        last_param = 0;
        if (param_table != NULL) {
            last_param = param_table->cardinality;
        }
    }

    JITUINT32 first_param = method->row->param_list;
    if (method->result == NULL) {
        if (first_param <= last_param) {
            /* try associate result */
            BasicParam *result_param_row = (BasicParam *) get_row(tables, PARAM_TABLE, first_param);
            if (result_param_row->sequence == 0) {
                method->result = newParamDescriptor(method->manager, result_param_row, method->signature->result, method);
                first_param++;
            } else {
                method->result = newParamDescriptor(method->manager, NULL, method->signature->result, method);
            }
        } else {
            method->result = newParamDescriptor(method->manager, NULL, method->signature->result, method);
        }
    }

    /* Try associate parameter */
    JITUINT32 count = count = first_param;
    if (xanList_length(method->params) == 0) {
        XanListItem *item = xanList_first(method->signature->params);
        while (item != NULL) {
            BasicParam *param_row = NULL;
            if (count <= last_param) {
                param_row = (BasicParam *) get_row(tables, PARAM_TABLE, count);
            }
            TypeDescriptor *param_type = (TypeDescriptor *) item->data;
            ParamDescriptor *param = newParamDescriptor(method->manager, param_row, param_type, method);
            xanList_append(method->params, param);
            item = item->next;
            count++;
        }
    }

    return ;
}

inline FunctionPointerDescriptor *getFormalSignature (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, SIGNATURE_TICKET)) {
        internalParseMethodSignature(method);
        BROADCAST_TICKET(method, SIGNATURE_TICKET);
    }
    /* already cached */
    return method->signature;
}

inline XanList *getParams (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, SIGNATURE_TICKET)) {
        internalParseMethodSignature(method);
        BROADCAST_TICKET(method, SIGNATURE_TICKET);
    }
    /* already cached */
    return method->params;
}

inline ParamDescriptor *getResult (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, SIGNATURE_TICKET)) {
        internalParseMethodSignature(method);
        BROADCAST_TICKET(method, SIGNATURE_TICKET);
    }
    /* already cached */
    return method->result;
}

inline JITBOOLEAN isCallebleMethod (MethodDescriptor *callee, MethodDescriptor *caller) {
    BasicMethodAttributes *callee_attributes = callee->attributes;

    if (callee_attributes->is_abstract) {
        return JITFALSE;
    }
    switch (callee_attributes->accessMask) {
        case COMPILER_CONTROLLED_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : Member not referenceable\n");
            return JITFALSE;
        case PRIVATE_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : callee method is private \n");
            if (caller->row->binary != callee->row->binary) {
                PDEBUG("METADATA MANAGER : isCallableMethod : callee method is defined in an assembly different from the caller's assembly\n");
                return JITFALSE;
            }
            if (caller->owner == callee->owner) {
                PDEBUG("METADATA MANAGER : isCallableMethod : callee method is defined in the same type of caller\n");
                return JITTRUE;
            }
            TypeDescriptor *parent_type = getEnclosingType(caller->owner);
            while (parent_type != NULL) {
                if (parent_type == callee->owner) {
                    return JITTRUE;
                }
                parent_type = getEnclosingType(parent_type);
            }
            return JITFALSE;
        case FAM_AND_ASSEM_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : callee method is FAM_AND_ASSEM_METHOD \n");
            if (caller->row->binary != callee->row->binary) {
                PDEBUG("METADATA MANAGER : isCallableMethod : callee method is defined in an assembly different from the caller's assembly\n");
                return JITFALSE;
            }
            return typeDescriptorIsSubtypeOf(caller->owner, callee->owner);
        case ASSEM_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : callee method is ASSEM_METHOD \n");
            return caller->row->binary == callee->row->binary;
        case FAMILY_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : callee method is FAMILY_METHOD \n");
            return typeDescriptorIsSubtypeOf(caller->owner, callee->owner);
        case FAM_OR_ASSEM_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : callee method is FAM_OR_ASSEM_METHOD \n");
            if (caller->row->binary != callee->row->binary) {
                return typeDescriptorIsSubtypeOf(caller->owner, callee->owner);
            }
            return JITTRUE;
        case PUBLIC_METHOD:
            PDEBUG("METADATA MANAGER : isCallableMethod : callee method is public \n");
            return JITTRUE;
        default:
            abort();
    }
    return JITFALSE;
}

inline XanList *getGenericParametersFromMethodDescriptor (MethodDescriptor *method) {
    if (!WAIT_TICKET(method, GENERIC_PARAM_TICKET)) {
        if (method->attributes->param_count == 0) {
            method->genericParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
        } else {
            if (method->container != NULL) {
                MethodDescriptor *genericMethod = newMethodDescriptor(method->manager, method->row, method->owner, NULL);
                method->genericParameters = getGenericParametersFromMethodDescriptor(genericMethod);
            } else {
                method->genericParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
                t_binary_information *binary = (t_binary_information *) method->row->binary;
                XanList *list = getRowByKey(method->manager, binary, METHOD_GENERIC_PARAM, (void *) method->row->index);
                if (list != NULL) {
                    XanListItem *item = xanList_first(list);
                    while (item != NULL) {
                        BasicGenericParam *row = (BasicGenericParam *) item->data;
                        GenericMethodVarDescriptor *var = newGenericMethodVarDescriptor(method->manager, row, method);
                        xanList_append(method->genericParameters, (void *) var);
                        item = item->next;
                    }
                }
            }
        }
        BROADCAST_TICKET(method, GENERIC_PARAM_TICKET);
    }
    return method->genericParameters;
}


inline JITBOOLEAN isGenericMethodDefinition (MethodDescriptor *method) {
    return method->attributes->param_count > 0 && method->container == NULL;
}

inline JITBOOLEAN containOpenGenericParameterFromMethodDescriptor (MethodDescriptor *method) {
    if (isGenericMethodDefinition(method)) {
        return JITTRUE;
    }
    JITBOOLEAN owner_status = containOpenGenericParameterFromTypeDescriptor(method->owner);
    if (owner_status) {
        return JITTRUE;
    }
    if (method->container == NULL) {
        return JITFALSE;
    }
    JITUINT32 count;
    for (count = 0; count < method->container->arg_count; count++) {
        if (containOpenGenericParameterFromTypeDescriptor(method->container->paramType[count])) {
            return JITTRUE;
        }
    }
    return JITFALSE;
}

inline JITBOOLEAN isGenericMethodDescriptor (MethodDescriptor *method) {
    return method->attributes->param_count > 0;
}


inline GenericContainer *getResolvingContainerFromMethodDescriptor (MethodDescriptor *method) {
    if (method->attributes->param_count == 0 && method->owner->attributes->param_count == 0) {
        return NULL;
    }
    if (method->resolving_container != NULL) {
        return method->resolving_container;
    }
    if (!WAIT_TICKET(method, RESOLVING_CONTAINER_TICKET)) {
        /*not already cached*/
        GenericContainer *owner_container = getResolvingContainerFromTypeDescriptor(method->owner);
        method->resolving_container = newGenericContainer(METHOD_LEVEL, owner_container, method->attributes->param_count);
        if (method->container != NULL) {
            JITUINT32 count;
            for (count = 0; count < method->container->arg_count; count++) {
                insertTypeDescriptorIntoGenericContainer(method->resolving_container, (method->container->paramType)[count], count);
            }
        } else {
            XanList *genericParameters = getGenericParametersFromMethodDescriptor(method);
            JITUINT32 count = 0;
            XanListItem *item = xanList_first(genericParameters);
            while (item != NULL) {
                GenericMethodVarDescriptor *mvar = (GenericMethodVarDescriptor *) item->data;
                TypeDescriptor *resolved_type = newTypeDescriptor(method->manager, ELEMENT_MVAR, (void *) mvar, JITFALSE);
                insertTypeDescriptorIntoGenericContainer(method->resolving_container, resolved_type, count);
                item = item->next;
                count++;
            }
        }
        BROADCAST_TICKET(method, RESOLVING_CONTAINER_TICKET);
    }
    return method->resolving_container;
}

inline MethodDescriptor *getGenericDefinitionFromMethodDescriptor (MethodDescriptor *method) {
    MethodDescriptor *result = NULL;

    if (!isGenericMethodDefinition(method)) {
        result = newMethodDescriptor(method->manager, method->row, method->owner, NULL);
    }
    return result;
}

inline void methodDescriptorLock (MethodDescriptor *method) {
    PLATFORM_lockMutex(&(method->mutex));
}

inline void methodDescriptorUnLock (MethodDescriptor *method) {
    PLATFORM_unlockMutex(&(method->mutex));
}

inline DescriptorMetadataTicket *methodDescriptorCreateDescriptorMetadataTicket (MethodDescriptor *method, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(method->metadata), type);
}

inline void methodDescriptorBroadcastDescriptorMetadataTicket (MethodDescriptor *method,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}

/* Param Descriptor Accessor Method */
inline ParamDescriptor *newParamDescriptor (metadataManager_t *manager, BasicParam *row, TypeDescriptor *type, MethodDescriptor *owner) {
    ParamDescriptor *param = (ParamDescriptor *) sharedAllocFunction(sizeof(ParamDescriptor));

    pthread_mutexattr_t mutex_attr;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(param->mutex), &mutex_attr);
    param->tickets = allocTicketArray(PARAM_TICKET_COUNT);
    init_DescriptorMetadata(&(param->metadata));
    param->lock = paramDescriptorLock;
    param->unlock = paramDescriptorUnLock;
    param->createDescriptorMetadataTicket = paramDescriptorCreateDescriptorMetadataTicket;
    param->broadcastDescriptorMetadataTicket = paramDescriptorBroadcastDescriptorMetadataTicket;
    param->row = row;
    param->attributes = NULL;
    param->name = NULL;
    param->signatureInString = NULL;
    param->type = type;
    param->getName = getNameFromParamDescriptor;
    param->getSignatureInString = getSignatureInStringFromParamDescriptor;
    param->getIRType = getIRTypeFromParamDescriptor;
    if (row != NULL) {
        param->attributes = getBasicParamAttributes(manager, row);
    }
    return param;
}

inline JITINT8* getNameFromParamDescriptor (ParamDescriptor *param) {
    if (!WAIT_TICKET(param, NAME_TICKET)) {
        /* not already cached */
        if (param->row != NULL) {
            t_binary_information *binary = (t_binary_information *) param->row->binary;
            t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
            t_string_stream *string_stream = &(streams->string_stream);
            param->name = get_string(string_stream, param->row->name);
        } else {
            param->name = (JITINT8*) "\0";
        }
        BROADCAST_TICKET(param, NAME_TICKET);
    }
    return param->name;
}

inline JITINT8* getSignatureInStringFromParamDescriptor (ParamDescriptor *param) {
    if (!WAIT_TICKET(param, STRING_SIGNATURE_TICKET)) {
        /* not already cached */
        JITINT8* name = getNameFromParamDescriptor(param);
        JITINT8* typeInString = getCompleteNameFromTypeDescriptor(param->type);
        if (STRLEN(name) == 0) {
            param->signatureInString = (JITINT8 *)strdup((char *)typeInString);
        } else {
            JITUINT32 char_count = STRLEN(typeInString) + 1 + STRLEN(name) + 1;
            size_t length = sizeof(JITINT8) * char_count;
            param->signatureInString = sharedAllocFunction(length);
            SNPRINTF(param->signatureInString, length, "%s %s", typeInString, name);
        }
        BROADCAST_TICKET(param, STRING_SIGNATURE_TICKET);
    }
    return param->signatureInString;
}

inline JITUINT32 getIRTypeFromParamDescriptor (ParamDescriptor *param) {
    return getIRTypeFromTypeDescriptor(param->type);
}

inline void paramDescriptorLock (ParamDescriptor *param) {
    PLATFORM_lockMutex(&(param->mutex));
}

inline void paramDescriptorUnLock (ParamDescriptor *param) {
    PLATFORM_unlockMutex(&(param->mutex));
}

inline DescriptorMetadataTicket *paramDescriptorCreateDescriptorMetadataTicket (ParamDescriptor *param, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(param->metadata), type);
}

inline void paramDescriptorBroadcastDescriptorMetadataTicket (ParamDescriptor *param,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}

/* Local Descriptor Accessor Method */
inline LocalDescriptor *newLocalDescriptor (JITBOOLEAN is_pinned, TypeDescriptor *type) {
    LocalDescriptor *local = (LocalDescriptor *) sharedAllocFunction(sizeof(LocalDescriptor));

    init_DescriptorMetadata(&(local->metadata));
    local->createDescriptorMetadataTicket = localDescriptorCreateDescriptorMetadataTicket;
    local->broadcastDescriptorMetadataTicket = localDescriptorBroadcastDescriptorMetadataTicket;
    local->is_pinned = is_pinned;
    local->type = type;
    local->getIRType = getIRTypeFromLocalDescriptor;
    return local;
}

inline JITUINT32 getIRTypeFromLocalDescriptor (LocalDescriptor *local) {
    return getIRTypeFromTypeDescriptor(local->type);
}

inline DescriptorMetadataTicket *localDescriptorCreateDescriptorMetadataTicket (LocalDescriptor *local, JITUINT32 type) {
    return createDescriptorMetadataTicket(&(local->metadata), type);
}

inline void localDescriptorBroadcastDescriptorMetadataTicket (LocalDescriptor *local,  DescriptorMetadataTicket *ticket, void * data) {
    broadcastDescriptorMetadataTicket(ticket, data);
}
