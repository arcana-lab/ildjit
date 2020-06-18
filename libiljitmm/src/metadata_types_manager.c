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


#include <stdlib.h>
#include <assert.h>
#include <platform_API.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata_types_manager.h>
#include <decoding_tools.h>
#include <generic_type_rules.h>
#include <basic_concept_manager.h>
#include <iljitmm-system.h>


#define DIM_BUF 2056

void extractInternalCall ();

pthread_once_t metadata_init_lock = PTHREAD_ONCE_INIT;

JITUINT32 internalHashRowObjectID (void *element);
JITINT32 internalEqualsRowObjectID (void *key1, void *key2);

metadataManager_t *metadataManager;

XanHashTable *internal_hash_interface_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_explicit_override_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_field_rva_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_field_layout_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_method_sematics_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_class_layout_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_class_generic_param_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_nested_class_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_enclosing_class_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_method_generic_param_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_properties_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_events_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_generic_param_constraint_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_property_method_class_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_event_method_class_table (metadataManager_t *manager, t_binary_information *binary);
XanHashTable *internal_hash_implementation_map_table (metadataManager_t *manager, t_binary_information *binary);

void internalLoadMetadata (void);
static void internal_metadataManager_initialize ();
static inline void internal_freeParameterDescriptor (ParamDescriptor *p);

/* GenericParameterRule */

JITUINT32 hashGenericParameterRule (void *element) {
    if (element == NULL) {
        return 0;
    }
    GenericParameterRule *token = (GenericParameterRule *) element;
    JITUINT32 seed = combineHash((JITUINT32) token->owner, token->var);
    return seed;
}

JITINT32 equalsGenericParameterRule (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    GenericParameterRule *token1 = (GenericParameterRule *) key1;
    GenericParameterRule *token2 = (GenericParameterRule *) key2;
    return token1->var == token2->var && token1->owner == token2->owner;
}

/* TableID */

typedef struct _TableID {
    JITUINT32 table;
    t_binary_information *binary;
} TableID;

JITUINT32 hashTableID (void *element) {
    if (element == NULL) {
        return 0;
    }
    TableID *token = (TableID *) element;
    JITUINT32 seed = combineHash((JITUINT32) token->binary, token->table);
    return seed;
}

JITINT32 equalsTableID (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    TableID *token1 = (TableID *) key1;
    TableID *token2 = (TableID *) key2;
    return token1->table == token2->table && token1->binary == token2->binary;
}

JITUINT32 internalHashRowObjectID (void *element) {
    if (element == NULL) {
        return 0;
    }
    RowObjectID *token = (RowObjectID *) element;
    JITUINT32 seed = hashGenericContainer(token->container);
    seed = combineHash(seed, token->rowID);
    seed = combineHash(seed, token->tableID);
    seed = combineHash(seed, (JITUINT32) token->binary);
    seed = combineHash(seed, (JITUINT32) token->concept);
    return seed;
}

JITINT32 internalEqualsRowObjectID (void *key1, void *key2) {
    if (key1 == key2) {
        return 1;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    RowObjectID *token1 = (RowObjectID *) key1;
    RowObjectID *token2 = (RowObjectID *) key2;
    if (token1->rowID != token2->rowID || token1->tableID != token2->tableID || token1->binary != token2->binary || token1->concept != token2->concept) {
        return 0;
    }
    return equalsGenericContainer(token1->container, token2->container);
}

inline RowObjectID *createObjectRowID (metadataManager_t *manager, t_binary_information *binary, GenericContainer *container, JITUINT32 tableID, JITUINT32 rowID, ILConcept concept) {
    RowObjectID objectID;

    objectID.binary = binary;
    objectID.container = container;
    objectID.tableID = tableID;
    objectID.rowID = rowID;
    objectID.concept = concept;

    xanHashTable_lock(manager->resolvedTokens);
    RowObjectID *rowObject = xanHashTable_lookup(manager->resolvedTokens, &objectID);
    if (rowObject == NULL) {
        rowObject = sharedAllocFunction(sizeof(RowObjectID));
        rowObject->binary = binary;
        rowObject->container = cloneGenericContainer(container);
        rowObject->tableID = tableID;
        rowObject->rowID = rowID;
        rowObject->concept = concept;
        rowObject->object = NULL;

        pthread_mutexattr_t mutex_attr;
        pthread_condattr_t cond_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_initCondVarAttr(&cond_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        MEMORY_shareMutex(&mutex_attr);
        MEMORY_shareConditionVariable(&cond_attr);
        PLATFORM_initMutex(&(rowObject->mutex), &mutex_attr);
        PLATFORM_initCondVar(&(rowObject->cond), &cond_attr);
        xanHashTable_insert(manager->resolvedTokens, (void *) rowObject, (void *) rowObject);

        xanHashTable_unlock(manager->resolvedTokens);
    } else {
        xanHashTable_unlock(manager->resolvedTokens);

        PLATFORM_lockMutex(&(rowObject->mutex));
        if (rowObject->object == NULL) {
            PLATFORM_waitCondVar(&(rowObject->cond), &(rowObject->mutex));
        }
        PLATFORM_unlockMutex(&(rowObject->mutex));
    }
    return rowObject;
}

inline void broadcastObjectRowID (metadataManager_t *manager, RowObjectID *rowObject, void *object) {
    PLATFORM_lockMutex(&(rowObject->mutex));
    rowObject->object = object;
    PLATFORM_unlockMutex(&(rowObject->mutex));
    PLATFORM_broadcastCondVar(&(rowObject->cond));
}

Ticket *allocTicketArray (JITUINT32 count) {
    JITUINT32 i;

    pthread_mutexattr_t mutex_attr;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);

    Ticket *result = sharedAllocFunction(sizeof(Ticket) * count);
    for (i = 0; i < count; i++) {
        PLATFORM_initMutex(&(result[i].mutex), &mutex_attr);
        result[i].completed = JITFALSE;
    }

    return result;
}

JITBOOLEAN waitTicket (Ticket *metadata) {
    PLATFORM_lockMutex(&(metadata->mutex));
    if (metadata->completed) {
        PLATFORM_unlockMutex(&(metadata->mutex));
    }

    return metadata->completed;
}

void broadcastTicket (Ticket *metadata) {
    metadata->completed = JITTRUE;
    PLATFORM_unlockMutex(&(metadata->mutex));
}
/* Class Descriptor Accessor Method */

inline ClassDescriptor *getClassDescriptorFromNameAndAssembly (metadataManager_t *manager, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name) {
    ClassDescriptor *type = NULL;
    BasicClass *row = getBasicClassFromNameAndTypeNameSpace(manager, binary, type_name_space, type_name);

    if (row != NULL) {
        type = newClassDescriptor(manager, row, NULL);
    }
    return type;
}

inline ClassDescriptor * getClassDescriptorFromName (metadataManager_t *manager, JITINT8 *type_name_space, JITINT8 *type_name) {
    ClassDescriptor *type = NULL;
    BasicClass *row = getBasicClassFromName(manager, type_name_space, type_name);

    if (row != NULL) {
        type = newClassDescriptor(manager, row, NULL);
    }
    return type;
}

inline ClassDescriptor *getClassDescriptorFromElementType (metadataManager_t *manager, JITUINT32 type) {
    ClassDescriptor *return_type = NULL;

    initializeMetadata();

    switch (type) {
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_SZARRAY:
            return_type = manager->System_Array;
            break;
        case ELEMENT_TYPE_VOID:
            return_type = manager->System_Void;
            break;
        case ELEMENT_TYPE_BOOLEAN:
            return_type = manager->System_Boolean;
            break;
        case ELEMENT_TYPE_I:
            return_type = manager->System_IntPtr;
            break;
        case ELEMENT_TYPE_I1:
            return_type = manager->System_SByte;
            break;
        case ELEMENT_TYPE_I2:
            return_type = manager->System_Int16;
            break;
        case ELEMENT_TYPE_I4:
            return_type = manager->System_Int32;
            break;
        case ELEMENT_TYPE_I8:
            return_type = manager->System_Int64;
            break;
        case ELEMENT_TYPE_R4:
            return_type = manager->System_Single;
            break;
        case ELEMENT_TYPE_R8:
            return_type = manager->System_Double;
            break;
        case ELEMENT_TYPE_U:
            return_type = manager->System_UIntPtr;
            break;
        case ELEMENT_TYPE_U1:
            return_type = manager->System_Byte;
            break;
        case ELEMENT_TYPE_U2:
            return_type = manager->System_UInt16;
            break;
        case ELEMENT_TYPE_U4:
            return_type = manager->System_UInt32;
            break;
        case ELEMENT_TYPE_U8:
            return_type = manager->System_UInt64;
            break;
        case ELEMENT_TYPE_CHAR:
            return_type = manager->System_Char;
            break;
        case ELEMENT_TYPE_OBJECT:
            return_type = manager->System_Object;
            break;
        case ELEMENT_TYPE_STRING:
            return_type = manager->System_String;
            break;
        case ELEMENT_TYPE_TYPEDBYREF:
            return_type = manager->System_TypedReference;
            break;
        default:
            PDEBUG("METADATA MANAGER: getClassDescriptorFromElementType: ElementType unknown!\n");
            abort();
    }
    return return_type;
}

inline ClassDescriptor *getClassDescriptorFromIRType (metadataManager_t *manager, JITUINT32 IRType) {
    ClassDescriptor *return_type = NULL;

    initializeMetadata();

    switch (IRType) {
        case IROBJECT:
            return manager->System_Object;
        case IRINT8:
            return manager->System_SByte;
        case IRINT16:
            return manager->System_Int16;
        case IRINT32:
            return manager->System_Int32;
        case IRINT64:
            return manager->System_Int64;
        case IRUINT8:
            return manager->System_Byte;
        case IRUINT16:
            return manager->System_UInt16;
        case IRUINT32:
            return manager->System_UInt32;
        case IRUINT64:
            return manager->System_UInt64;
        case IRMPOINTER:
        case IRFPOINTER:
        case IRNINT:
            return manager->System_IntPtr;
        case IRNUINT:
            return manager->System_UIntPtr;
        case IRFLOAT32:
            return manager->System_Single;
        case IRFLOAT64:
            return manager->System_Double;
        default:
            PDEBUG("METADATA MANAGER: getClassDescriptorIRType:  IR Type %d unknown! \n", IRType);
            abort();
    }
    return return_type;
}

TypeDescriptor *metadataManagerMakeArrayDescriptor (metadataManager_t *manager, TypeDescriptor *type, JITUINT32 rank, JITUINT32 numSizes, JITUINT32 *sizes, JITUINT32 numBounds, JITUINT32 *bounds) {
    ArrayDescriptor *array = newArrayDescriptor(manager, type, rank, numSizes, sizes, numBounds, bounds);

    return array->getTypeDescriptor(array);
}

TypeDescriptor *metadataManagerMakeFunctionPointerDescriptor (metadataManager_t *manager, JITBOOLEAN hasThis, JITBOOLEAN explicitThis, JITBOOLEAN hasSentinel, JITUINT32 sentinel_index, JITUINT32 calling_convention, JITUINT32 generic_param_count, TypeDescriptor *result, XanList *params) {
    FunctionPointerDescriptor *fptr = newFunctionPointerDescriptor(manager, hasThis, explicitThis, hasSentinel, sentinel_index, calling_convention, generic_param_count, result, params);

    return fptr->getTypeDescriptor(fptr);
}

TypeDescriptor *getTypeDescriptorFromNameAndAssembly (metadataManager_t *manager, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name) {
    ClassDescriptor *return_type = getClassDescriptorFromNameAndAssembly(manager, binary, type_name_space, type_name);

    if (return_type == NULL) {
        return NULL;
    }
    return getTypeDescriptorFromClassDescriptor(return_type);
}

TypeDescriptor * getTypeDescriptorFromName (metadataManager_t *manager, JITINT8 *type_name_space, JITINT8 *type_name) {
    ClassDescriptor *return_type = getClassDescriptorFromName(manager, type_name_space, type_name);

    if (return_type == NULL) {
        return NULL;
    }
    return getTypeDescriptorFromClassDescriptor(return_type);
}

TypeDescriptor *getTypeDescriptorFromElementType (metadataManager_t *manager, JITUINT32 type) {
    ClassDescriptor *return_type = getClassDescriptorFromElementType(manager, type);

    if (return_type == NULL) {
        return NULL;
    }
    return getTypeDescriptorFromClassDescriptor(return_type);
}

TypeDescriptor *getTypeDescriptorFromIRType (metadataManager_t *manager, JITUINT32 IRType) {
    ClassDescriptor *return_type = getClassDescriptorFromIRType(manager, IRType);

    if (return_type == NULL) {
        return NULL;
    }
    return getTypeDescriptorFromClassDescriptor(return_type);
}


inline ILConcept fetchDescriptor (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token, void **descriptor) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_blob_stream                   *blob_stream = &(streams->blob_stream);
    JITUINT32 table_id = (((token & 0xFF000000) >> 24) & 0x000000FF);
    JITUINT32 row_id = (token & 0x00FFFFFF);
    ILConcept return_concept;

    switch (table_id) {
        case TYPE_DEF_TABLE:
        case TYPE_REF_TABLE:
        case TYPE_SPEC_TABLE: {
            (*descriptor) = getTypeDescriptorFromToken(manager, container, binary, token);
            return_concept = ILTYPE;
            break;
        }
        case METHOD_DEF_TABLE:
        case METHOD_SPEC_TABLE: {
            ActualMethodDescriptor *actualMethod = newActualMethodDescriptor();
            (*actualMethod) = getMethodDescriptorFromToken(manager, container, binary, token);
            (*descriptor) = actualMethod->method;
            return_concept = ILMETHOD;
            break;
        }
        case FIELD_TABLE: {
            (*descriptor) = getFieldDescriptorFromToken(manager, container, binary, token);
            return_concept = ILFIELD;
            break;
        }
        case MEMBER_REF_TABLE: {
            t_row_member_ref_table *member_ref = (t_row_member_ref_table *) get_row(tables, MEMBER_REF_TABLE, row_id);
            t_row_blob_table blob;
            get_blob(blob_stream, member_ref->signature, &blob);
            JITINT8 *signature = blob.blob;
            if (signature[0] == 0x06) {
                (*descriptor) = getFieldDescriptorFromToken(manager, container, binary, token);
                return_concept = ILFIELD;
            } else {
                ActualMethodDescriptor *actualMethod = newActualMethodDescriptor();
                (*actualMethod) = getMethodDescriptorFromToken(manager, container, binary, token);
                (*descriptor) = actualMethod->method;
                return_concept = ILMETHOD;
            }
            break;
        }
        case STANDALONE_SIG_TABLE: {
            t_row_standalone_sig_table *stand_alone_sig = (t_row_standalone_sig_table *) get_row(tables, STANDALONE_SIG_TABLE, row_id);
            t_row_blob_table blob;
            get_blob(blob_stream, stand_alone_sig->signature, &blob);
            JITINT8 *signature = blob.blob;
            FunctionPointerDescriptor *pointer;
            getFunctionPointerDescriptorFromBlob(manager, container, binary, signature, &pointer);
            (*descriptor) = getTypeDescriptorFromFunctionPointerDescriptor(pointer);
            return_concept = ILTYPE;
            break;
        }
        default:
            PDEBUG("METADATA MANAGER: fetchDescriptor: ERROR!:	Reference to the unknow table\n");
            abort();
    }
    return return_concept;
}

inline TypeDescriptor *getTypeDescriptorFromToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_blob_stream                   *blob_stream = &(streams->blob_stream);
    JITUINT32 table_id = (((token & 0xFF000000) >> 24) & 0x000000FF);
    JITUINT32 row_id = (token & 0x00FFFFFF);
    TypeDescriptor *type;

    switch (table_id) {
        case TYPE_DEF_TABLE:
            /* TypeDef table		*/
            PDEBUG("METADATA MANAGER: getTypeDescriptorFromToken:	Reference to the TypeDef table\n");
            type = getTypeDescriptorFromTableAndRow(manager, container, binary, 0x0, row_id);
            break;
        case TYPE_REF_TABLE:
            /* TypeRef table		*/
            PDEBUG("METADATA MANAGER: getTypeDescriptorFromToken:	Reference to the TypeRef table\n");
            type = getTypeDescriptorFromTableAndRow(manager, container, binary, 0x1, row_id);
            break;
        case TYPE_SPEC_TABLE:
            /* TypeSpec table		*/
            PDEBUG("METADATA MANAGER: getTypeDescriptorFromToken:	Reference to the TypeSpec table\n");
            type = getTypeDescriptorFromTableAndRow(manager, container, binary, 0x2, row_id);
            break;
        case STANDALONE_SIG_TABLE: {
            t_row_standalone_sig_table *stand_alone_sig = (t_row_standalone_sig_table *) get_row(tables, STANDALONE_SIG_TABLE, row_id);
            t_row_blob_table blob;
            get_blob(blob_stream, stand_alone_sig->signature, &blob);
            JITINT8 *signature = blob.blob;
            FunctionPointerDescriptor *pointer;
            getFunctionPointerDescriptorFromBlob(manager, container, binary, signature, &pointer);
            type = getTypeDescriptorFromFunctionPointerDescriptor(pointer);
            break;
        }
        default:
            PDEBUG("METADATA MANAGER: getTypeDescriptorFromToken: ERROR!:	Reference to the unknow table\n");
            abort();
    }
    return type;
}

inline MethodDescriptor *getMethodDescriptorFromEntryPoint (metadataManager_t * manager, t_binary_information *binary) {
    JITUINT32 token = (binary->cli_information).entry_point_token;
    MethodDescriptor *result = getMethodDescriptorFromToken(manager, NULL, binary, token).method;

    return result;
}

inline ActualMethodDescriptor getMethodDescriptorFromToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    ActualMethodDescriptor result;
    JITUINT32 table_id = (((token & 0xFF000000) >> 24) & 0x000000FF);
    JITUINT32 row_id = (token & 0x00FFFFFF);

    switch (table_id) {
        case METHOD_DEF_TABLE:
            /* MethodDef table		*/
            PDEBUG("METADATA MANAGER: getMethodDescriptorFromToken:	Reference to the MethodDef table\n");
            result.method = getMethodDescriptorFromTableAndRow(manager, container, binary, 0x0, row_id, &(result.actualSignature));
            break;
        case MEMBER_REF_TABLE:
            /* MethodRef table		*/
            PDEBUG("METADATA MANAGER: getMethodDescriptorFromToken:	Reference to the MethodRef table\n");
            result.method = getMethodDescriptorFromTableAndRow(manager, container, binary, 0x1, row_id, &(result.actualSignature));
            break;
        case METHOD_SPEC_TABLE:
            /* MethodSpec table		*/
            PDEBUG("METADATA MANAGER: getMethodDescriptorFromToken:	Reference to the MethodSpec table\n");
            result.method = getMethodDescriptorFromTableAndRow(manager, container, binary, 0x2, row_id, &(result.actualSignature));
            break;
        default:
            PDEBUG("METADATA MANAGER: getMethodDescriptorFromToken: ERROR!: Reference to the unknow table\n");
            abort();
    }
    return result;
}

inline FieldDescriptor  *getFieldDescriptorFromToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 token) {
    JITUINT32 table_id = (((token & 0xFF000000) >> 24) & 0x000000FF);
    JITUINT32 row_id = (token & 0x00FFFFFF);
    FieldDescriptor *field;

    switch (table_id) {
        case FIELD_TABLE:
            /* Field table		*/
            PDEBUG("METADATA MANAGER: getFieldDescriptorFromToken:	Reference to the MethodSpec table\n");
            field = getFieldDescriptorFromTableAndRow(manager, container, binary, 0x0, row_id);
            break;
        case MEMBER_REF_TABLE:
            /* FieldRef table		*/
            PDEBUG("METADATA MANAGER: getFieldDescriptorFromToken:	Reference to the MethodRef table\n");
            field = getFieldDescriptorFromTableAndRow(manager, container, binary, 0x1, row_id);
            break;
        default:
            PDEBUG("METADATA MANAGER: getFieldDescriptorFromToken: ERROR!: Reference to the unknow table\n");
            abort();
    }
    return field;
}


/* Assembly Accessor Method */

XanList *getMethodsFromAssembly (metadataManager_t *manager, BasicAssembly *assembly) {
    t_binary_information *current_binary = (t_binary_information *) assembly->binary;
    t_streams_metadata              *streams = &((current_binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_assembly_table *method_table = (t_assembly_table *) get_table(tables, METHOD_DEF_TABLE);
    XanList *result = xanList_new(sharedAllocFunction, freeFunction, NULL);

    if (method_table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= method_table->cardinality; count++) {
            MethodDescriptor *methodID = getMethodDescriptorFromTableAndRow(manager, NULL, current_binary, 0, count, NULL);
            xanList_append(result, (void *) methodID);
        }
    }
    return result;
}

inline BasicAssembly *metadataManagerGetAssemblyFromName (metadataManager_t *manager, JITINT8 *name) {
    BasicAssembly *ass = NULL;

    PDEBUG("METADATA MANAGER: getAssemblyFromName: Start\n");
    XanListItem *item = xanList_first(manager->binaries);
    PDEBUG("METADATA MANAGER: getAssemblyFromName:  Search the assembly %s\n", name);
    while (item != NULL) {
        t_binary_information *current_binary = (t_binary_information *) item->data;
        PDEBUG("METADATA MANAGER: getAssemblyFromName:          Binary	= %s\n", current_binary->name);
        t_streams_metadata              *streams = &((current_binary->metadata).streams_metadata);
        t_metadata_tables               *tables = &((streams->not_stream).tables);
        t_string_stream  *string_stream = &(streams->string_stream);
        t_assembly_table *assembly_table = (t_assembly_table *) get_table(tables, ASSEMBLY_TABLE);
        if (assembly_table != NULL) {
            JITUINT32 count;
            for (count = 1; count <= assembly_table->cardinality; count++) {
                BasicAssembly *current_ass = (BasicAssembly *) get_row(tables, ASSEMBLY_TABLE, count);
                JITINT8 *current_name = get_string(string_stream, current_ass->name);
                PDEBUG("METADATA MANAGER: getAssemblyFromName:                  Current assembly	= %s\n", current_name);
                if (STRCMP(name, current_name) == 0) {
                    ass = current_ass;
                    break;
                }
            }
        }
        item = item->next;
    }
    return ass;
}

/* Library method */

JITINT32 getIndexOfTypeDescriptor (TypeDescriptor *type) {
    return simpleHash((void *) type) % TYPE_COMP_SIZE;
}

inline void initializeMetadata (void) {
    PLATFORM_pthread_once(&metadata_init_lock, internalLoadMetadata);
}

void internalLoadMetadata (void) {

    assert(metadataManager != NULL);
    metadataManager->System_Delegate = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Delegate");
    metadataManager->System_Nullable = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Nullable");
    metadataManager->System_Enum = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Enum");
    metadataManager->System_ValueType = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "ValueType");
    metadataManager->System_Void = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Void");
    metadataManager->System_Boolean = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Boolean");
    metadataManager->System_IntPtr = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "IntPtr");
    metadataManager->System_UIntPtr = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "UIntPtr");
    metadataManager->System_SByte = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "SByte");
    metadataManager->System_Byte = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Byte");
    metadataManager->System_Int16 = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Int16");
    metadataManager->System_Int32 = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Int32");
    metadataManager->System_Int64 = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Int64");
    metadataManager->System_UInt16 = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "UInt16");
    metadataManager->System_UInt32 = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "UInt32");
    metadataManager->System_UInt64 = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "UInt64");
    metadataManager->System_Single = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Single");
    metadataManager->System_Double = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Double");
    metadataManager->System_Char = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Char");
    metadataManager->System_String = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "String");
    metadataManager->System_Array = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Array");
    metadataManager->System_Object = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "Object");
    metadataManager->System_TypedReference = getClassDescriptorFromName(metadataManager, (JITINT8 *) "System", (JITINT8 *) "TypedReference");

    metadataManager->arrayAttributes = *(metadataManager->System_Array->attributes);
    (metadataManager->arrayAttributes).isAbstract = JITFALSE;
    (metadataManager->arrayAttributes).isSealed = JITTRUE;
    (metadataManager->arrayAttributes).flags &= ~80;
    (metadataManager->arrayAttributes).flags &= ~20;

}

static void internal_metadataManager_initialize () {
    PLATFORM_pthread_once(&metadata_init_lock, internalLoadMetadata);
}

static inline void internal_metadata_manager_shutdown (metadataManager_t * self) {
    XanHashTableItem	*hashItem;

    /* Assertions.
     */
    assert(self != NULL);
    assert(self->methodDescriptors != NULL);

    /* Destroy the method descriptors.
     */
    hashItem	= xanHashTable_first(self->methodDescriptors);
    while (hashItem != NULL) {
        MethodDescriptor 		*m;
        DescriptorMetadata 		*metadata;
        DescriptorMetadataTicket 	*ticket;

        m 	= (MethodDescriptor *) hashItem->element;
        assert(m != NULL);
        if (m->container != NULL) {
            if (m->container->paramType != NULL) {
                freeFunction(m->container->paramType);
                m->container->paramType	= NULL;
            }
            freeFunction(m->container);
            m->container	= NULL;
        }
        if (m->tickets != NULL) {
            freeFunction(m->tickets);
            m->tickets 	= NULL;
        }
        if (m->attributes != NULL) {
            freeFunction(m->attributes);
            m->attributes	= NULL;
        }
        if (m->params != NULL) {
            XanListItem	*item;
            item	= xanList_first(m->params);
            while (item != NULL) {
                ParamDescriptor	*p;
                p		= item->data;
                internal_freeParameterDescriptor(p);
                item		= item->next;
            }
            xanList_destroyList(m->params);
            m->params	= NULL;
        }
        internal_freeParameterDescriptor(m->result);
        m->result	= NULL;
        if (m->signature != NULL) {
            xanList_destroyList(m->signature->params);
            freeFunction(m->signature->tickets);
            freeFunction(m->signature);
            m->signature	= NULL;
        }
        if (m->signatureInString != NULL) {
            freeFunction(m->signatureInString);
            m->signatureInString	= NULL;
        }
        if (m->internalName != m->completeName) {
            freeFunction(m->internalName);
            m->internalName	= NULL;
        }
        if (m->completeName != NULL) {
            freeFunction(m->completeName);
            m->completeName	= NULL;
        }

        /* Destroy the metadata.
         */
        metadata	= &(m->metadata);
        ticket 		= metadata->next;
        while (ticket != NULL) {
            DescriptorMetadataTicket 	*prev_ticket;
            prev_ticket	= ticket;
            ticket 		= ticket->next;
            freeFunction(prev_ticket);
        }

        /* Destroy the method.
         */
        freeFunction(m);

        hashItem	= xanHashTable_next(self->methodDescriptors, hashItem);
    }
    xanHashTable_destroyTable(self->methodDescriptors);
    self->methodDescriptors	= NULL;

    return ;
}

void init_metadataManager (metadataManager_t *manager, XanList *binaries) {
    /* Temporary empty List */
    manager->emptyList = xanList_new(sharedAllocFunction, freeFunction, NULL);

    manager->binaries = binaries;
    /* utilites cache area */
    manager->resolvedTokens = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, internalHashRowObjectID, internalEqualsRowObjectID);
    manager->binaryDB = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashTableID, equalsTableID);

    /* safe cache area */
    manager->classDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashClassDescriptor, (JITINT32 (*)(void *, void *))equalsClassDescriptor);
    manager->genericVarDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashGenericVarDescriptor, (JITINT32 (*)(void *, void *))equalsGenericVarDescriptor);
    manager->genericMethodVarDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashGenericMethodVarDescriptor, (JITINT32 (*)(void *, void *))equalsGenericMethodVarDescriptor);
    manager->pointerDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashPointerDescriptor, (JITINT32 (*)(void *, void *))equalsPointerDescriptor);
    manager->arrayDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashArrayDescriptor, (JITINT32 (*)(void *, void *))equalsArrayDescriptor);
    manager->functionPointerDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashFunctionPointerDescriptor, (JITINT32 (*)(void *, void *))equalsFunctionPointerDescriptor);

    manager->propertyDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashPropertyDescriptor, (JITINT32 (*)(void *, void *))equalsPropertyDescriptor);
    manager->eventDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashEventDescriptor, (JITINT32 (*)(void *, void *))equalsEventDescriptor);
    manager->fieldDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashFieldDescriptor, (JITINT32 (*)(void *, void *))equalsFieldDescriptor);
    manager->methodDescriptors = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))hashMethodDescriptor, (JITINT32 (*)(void *, void *))equalsMethodDescriptor);

    manager->generic_type_rules = new_generic_type_checking_rules();
    manager->getMethodDescriptorFromTableAndRow = getMethodDescriptorFromTableAndRow;
    manager->getTypeDescriptorFromTableAndRow = getTypeDescriptorFromTableAndRow;
    manager->getTypeDescriptorFromToken = getTypeDescriptorFromToken;
    manager->getMethodDescriptorFromToken = getMethodDescriptorFromToken;
    manager->getFieldDescriptorFromToken = getFieldDescriptorFromToken;
    manager->fetchDescriptor = fetchDescriptor;
    manager->decodeMethodLocals = decodeMethodLocals;
    manager->getEntryPointMethod = getMethodDescriptorFromEntryPoint;
    manager->getTypeDescriptorFromNameAndAssembly = getTypeDescriptorFromNameAndAssembly;
    manager->getTypeDescriptorFromName = getTypeDescriptorFromName;
    manager->getTypeDescriptorFromIRType = getTypeDescriptorFromIRType;
    manager->getTypeDescriptorFromElementType = getTypeDescriptorFromElementType;
    manager->getAssemblyFromName = metadataManagerGetAssemblyFromName;
    manager->getMethodsFromAssembly = getMethodsFromAssembly;
    manager->getIndexOfTypeDescriptor = getIndexOfTypeDescriptor;
    manager->makeArrayDescriptor = metadataManagerMakeArrayDescriptor;
    manager->makeFunctionPointerDescriptor = metadataManagerMakeFunctionPointerDescriptor;
    manager->initialize = internal_metadataManager_initialize;
    manager->shutdown = internal_metadata_manager_shutdown;

    metadataManager = manager;
}

ArrayDescriptor * METADATA_fromTypeToArray (TypeDescriptor *t) {
    TypeDescriptor	*tArray;
    ArrayDescriptor	*arrayDescriptor;

    tArray			= t->makeArrayDescriptor(t, 1);
    assert(tArray != NULL);
    arrayDescriptor         = GET_ARRAY(tArray);

    return arrayDescriptor;
}

TypeDescriptor * METADATA_fromArrayToTypeDescriptor (ArrayDescriptor *array) {
    TypeDescriptor	*ilType;

    ilType	= array->getTypeDescriptor(array);
    assert(ilType != NULL);

    return ilType;
}

JITBOOLEAN isValidCallToken (t_binary_information *binary, JITUINT32 token) {
    JITUINT32 table_ID;
    JITUINT32 row_ID;
    t_streams_metadata *streams;
    t_metadata_tables               *tables;
    t_row_method_def_table          *method_def_row;
    t_row_member_ref_table          *member_ref_row;
    JITINT8 buf[DIM_BUF];
    t_row_method_spec_table         *method_spec_row;

    /* assertions */
    assert(binary != NULL);

    /* a token cannot be zero */
    if (token == 0) {
        return 0;
    }

    /* Fetch the tables			*/
    streams = &(binary->metadata.streams_metadata);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    /* Decode the <table, row> pair		*/
    table_ID = ((token & 0xFF000000) >> 24);
    row_ID = (token & 0x00FFFFFF);

    /* Fetch the RVA of the entry point	*/
    switch (table_ID) {
        case METHOD_DEF_TABLE:
            /* Method def table		*/
            method_def_row = (t_row_method_def_table *) get_row(tables, METHOD_DEF_TABLE, row_ID);
            if (method_def_row == NULL) {
                return 0;
            }
            break;
        case MEMBER_REF_TABLE:
            /* Member ref table		*/
            member_ref_row = (t_row_member_ref_table *) get_row(tables, MEMBER_REF_TABLE, row_ID);
            if (member_ref_row == NULL) {
                return 0;
            }
            break;
        case METHOD_SPEC_TABLE:
            /* Method spec table		*/
            method_spec_row = (t_row_method_spec_table *) get_row(tables, METHOD_SPEC_TABLE, row_ID);
            if (method_spec_row == NULL) {
                return 0;
            }
            break;
        default:
            SNPRINTF(buf, DIM_BUF, "METADATA MANAGER: isValidCallToken: ERROR = Table %d is not known. ", table_ID);
            PRINT_ERR(buf, 0);
            abort();
    }
    return 1;
}

JITBOOLEAN isValidTypeToken (t_binary_information *binary, JITUINT32 token) {
    JITUINT32 table_ID;
    JITUINT32 row_ID;
    t_streams_metadata *streams;
    t_metadata_tables               *tables;
    t_row_type_def_table            *type_def_row;
    t_row_type_ref_table            *type_ref_row;
    t_row_type_spec_table           *type_spec_row;
    JITINT8 buf[DIM_BUF];

    /* assertions */
    assert(binary != NULL);

    /* a token cannot be zero */
    if (token == 0) {
        return 0;
    }

    /* Fetch the tables			*/
    streams = &(binary->metadata.streams_metadata);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    /* Decode the <table, row> pair		*/
    table_ID = ((token & 0xFF000000) >> 24);
    row_ID = (token & 0x00FFFFFF);

    /* Fetch the RVA of the entry point	*/
    switch (table_ID) {
        case TYPE_DEF_TABLE:
            /* Type def table		*/
            type_def_row = (t_row_type_def_table *) get_row(tables, TYPE_DEF_TABLE, row_ID);
            if (type_def_row == NULL) {
                return 0;
            }
            break;
        case TYPE_REF_TABLE:
            /* Type ref table		*/
            type_ref_row = (t_row_type_ref_table *) get_row(tables, TYPE_REF_TABLE, row_ID);
            if (type_ref_row == NULL) {
                return 0;
            }
            break;
        case TYPE_SPEC_TABLE:
            /* Type spec table		*/
            type_spec_row = (t_row_type_spec_table *) get_row(tables, TYPE_SPEC_TABLE, row_ID);
            if (type_spec_row == NULL) {
                return 0;
            }
            break;
        default:
            SNPRINTF(buf, DIM_BUF, "METADATA MANAGER: isValidTypeToken: ERROR = Table %d is not known. ", table_ID);
            PRINT_ERR(buf, 0);
            abort();
    }
    return 1;
}

JITBOOLEAN isValidFieldToken (t_binary_information *binary, JITUINT32 token) {
    JITUINT32 table_ID;
    JITUINT32 row_ID;
    t_streams_metadata *streams;
    t_metadata_tables               *tables;
    t_row_member_ref_table          *member_ref_row;
    t_row_field_table               *field_def_row;
    JITINT8 buf[DIM_BUF];

    /* assertions */
    assert(binary != NULL);

    /* a token cannot be zero */
    if (token == 0) {
        return 0;
    }

    /* Fetch the tables			*/
    streams = &(binary->metadata.streams_metadata);
    tables = &(streams->not_stream.tables);
    assert(tables != NULL);

    /* Decode the <table, row> pair		*/
    table_ID = ((token & 0xFF000000) >> 24);
    row_ID = (token & 0x00FFFFFF);

    /* Fetch the RVA of the entry point	*/
    switch (table_ID) {
        case FIELD_TABLE:
            /* Method def table		*/
            field_def_row = (t_row_field_table *) get_row(tables, FIELD_TABLE, row_ID);
            if (field_def_row == NULL) {
                return 0;
            }
            break;
        case MEMBER_REF_TABLE:
            /* Member ref table		*/
            member_ref_row = (t_row_member_ref_table *) get_row(tables, MEMBER_REF_TABLE, row_ID);
            if (member_ref_row == NULL) {
                return 0;
            }
            break;
        default:
            SNPRINTF(buf, DIM_BUF, "METADATA MANAGER: isValidFieldToken: ERROR = Table %d is not known. ", table_ID);
            PRINT_ERR(buf, 0);
            abort();
    }
    return 1;
}

void *getRowByKey (metadataManager_t *manager, t_binary_information *binary, TableIndexes table_id, void *key) {
    TableID elem;

    elem.table = table_id;
    elem.binary = binary;
    xanHashTable_lock(manager->binaryDB);
    XanHashTable *hashTable = (XanHashTable *) xanHashTable_lookup(manager->binaryDB, (void *) &elem);
    if (hashTable == NULL) {
        TableID *elemID = sharedAllocFunction(sizeof(TableID));
        elemID->table = table_id;
        elemID->binary = binary;
        switch (table_id) {
            case INTERFACE:
                hashTable = internal_hash_interface_table(manager, binary);
                break;
            case EXPLICIT_OVERRIDE:
                hashTable = internal_hash_explicit_override_table(manager, binary);
                break;
            case FIELD_RVA:
                hashTable = internal_hash_field_rva_table(manager, binary);
                break;
            case FIELD_OFFSET:
                hashTable = internal_hash_field_layout_table(manager, binary);
                break;
            case METHOD_SEMANTICS:
                hashTable = internal_hash_method_sematics_table(manager, binary);
                break;
            case CLASS_LAYOUT:
                hashTable = internal_hash_class_layout_table(manager, binary);
                break;
            case CLASS_GENERIC_PARAM:
                hashTable = internal_hash_class_generic_param_table(manager, binary);
                break;
            case METHOD_GENERIC_PARAM:
                hashTable = internal_hash_method_generic_param_table(manager, binary);
                break;
            case NESTED_CLASS:
                hashTable = internal_hash_nested_class_table(manager, binary);
                break;
            case ENCLOSING_CLASS:
                hashTable = internal_hash_enclosing_class_table(manager, binary);
                break;
            case PROPERTIES:
                hashTable = internal_hash_properties_table(manager, binary);
                break;
            case EVENTS:
                hashTable = internal_hash_events_table(manager, binary);
                break;
            case PROPERTY_METHOD:
                hashTable = internal_hash_property_method_class_table(manager, binary);
                break;
            case EVENT_METHOD:
                hashTable = internal_hash_event_method_class_table(manager, binary);
                break;
            case GENERIC_PARAM_CONSTRAINT:
                hashTable = internal_hash_generic_param_constraint_table(manager, binary);
                break;
            case IMPLEMENTATION_MAP:
                hashTable = internal_hash_implementation_map_table(manager, binary);
                break;
            default:
                PDEBUG("METADATA MANAGER: getRowByKey: ERROR! " "INVALID TABLE! %d\n", table_id);
                abort();
        }
        xanHashTable_insert(manager->binaryDB, (void *) elemID, (void *) hashTable);
    }
    xanHashTable_unlock(manager->binaryDB);
    return xanHashTable_lookup(hashTable, key);
}

XanHashTable *internal_hash_interface_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_interface_impl_table  *table = get_table(tables, INTERFACE_IMPL_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= table->cardinality; count++) {
            t_row_interface_impl_table *row = (t_row_interface_impl_table *) get_row(tables, INTERFACE_IMPL_TABLE, count);
            XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row->classIndex);
            if (list == NULL) {
                list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                xanHashTable_insert(result, (void *) row->classIndex, list);
            }
            xanList_append(list, row);
        }
    }
    return result;
}

XanHashTable *internal_hash_explicit_override_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_method_impl_table     *table = (t_method_impl_table *) get_table(tables, METHOD_IMPL_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= table->cardinality; count++) {
            t_row_method_impl_table *row = (t_row_method_impl_table *) get_row(tables, METHOD_IMPL_TABLE, count);
            XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row->classIndex);
            if (list == NULL) {
                list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                xanHashTable_insert(result, (void *) row->classIndex, list);
            }
            xanList_append(list, row);
        }
    }
    return result;
}


XanHashTable *internal_hash_field_rva_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_field_rva_table *table = (t_field_rva_table *) get_table(tables, FIELD_RVA_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_field_rva_table  *row = (t_row_field_rva_table  *) get_row(tables, FIELD_RVA_TABLE, count);
            xanHashTable_insert(result, (void *) row->field, (void *) row);
        }
    }
    return result;
}

XanHashTable *internal_hash_field_layout_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_field_layout_table *table = (t_field_layout_table *) get_table(tables, FIELD_LAYOUT_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_field_layout_table  *row = (t_row_field_layout_table  *) get_row(tables, FIELD_LAYOUT_TABLE, count);
            xanHashTable_insert(result, (void *) row->field, (void *) row);
        }
    }

    return result;
}

XanHashTable *internal_hash_method_sematics_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_method_semantics_table        *table = (t_method_semantics_table *) get_table(tables, METHOD_SEMANTICS_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_method_semantics_table  *row = (t_row_method_semantics_table  *) get_row(tables, METHOD_SEMANTICS_TABLE, count);
            xanHashTable_insert(result, (void *) row->method, row);
        }
    }

    return result;
}

XanHashTable *internal_hash_class_layout_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_class_layout_table    *table = (t_class_layout_table *) get_table(tables, CLASS_LAYOUT_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_class_layout_table  *row = (t_row_class_layout_table  *) get_row(tables, CLASS_LAYOUT_TABLE, count);
            xanHashTable_insert(result, (void *) row->parent, row);
        }
    }

    return result;
}

XanHashTable *internal_hash_class_generic_param_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_generic_param_table *table = (t_generic_param_table *) get_table(tables, GENERIC_PARAM_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_generic_param_table *row = (t_row_generic_param_table *) get_row(tables, GENERIC_PARAM_TABLE, count);
            JITUINT32 table_id = (row->owner  & 0x1);
            JITUINT32 row_id = ((row->owner >> 1) & 0x7FFFFFFF);
            if (table_id == 0x0) {
                XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row_id);
                if (list == NULL) {
                    list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                    xanHashTable_insert(result, (void *) row_id, list);
                }
                xanList_append(list, (void *) row);
            }
        }
    }
    return result;
}

XanHashTable *internal_hash_nested_class_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_nested_class_table *table = (t_nested_class_table *) get_table(tables, NESTED_CLASS_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_nested_class_table *row = (t_row_nested_class_table *) get_row(tables, NESTED_CLASS_TABLE, count);
            XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row->enclosing_class);
            if (list == NULL) {
                list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                xanHashTable_insert(result, (void *) row->enclosing_class, list);
            }
            xanList_append(list, (void *) row);
        }
    }
    return result;
}

XanHashTable *internal_hash_enclosing_class_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_nested_class_table *table = (t_nested_class_table *) get_table(tables, NESTED_CLASS_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_nested_class_table *row = (t_row_nested_class_table *) get_row(tables, NESTED_CLASS_TABLE, count);
            xanHashTable_insert(result, (void *) row->nested_class, row);
        }
    }
    return result;
}

XanHashTable *internal_hash_method_generic_param_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_generic_param_table *table = (t_generic_param_table *) get_table(tables, GENERIC_PARAM_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_generic_param_table *row = (t_row_generic_param_table *) get_row(tables, GENERIC_PARAM_TABLE, count);
            JITUINT32 table_id = (row->owner  & 0x1);
            JITUINT32 row_id = ((row->owner >> 1) & 0x7FFFFFFF);
            if (table_id == 0x1) {
                XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row_id);
                if (list == NULL) {
                    list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                    xanHashTable_insert(result, (void *) row_id, list);
                }
                xanList_append(list, (void *) row);
            }
        }
    }
    return result;
}

XanHashTable *internal_hash_properties_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_property_map_table *table = (t_property_map_table *) get_table(tables, PROPERTY_MAP_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_property_map_table *row = (t_row_property_map_table *) get_row(tables, PROPERTY_MAP_TABLE, count);
            xanHashTable_insert(result, (void *) row->parent, (void *) row);
        }
    }
    return result;
}

XanHashTable *internal_hash_events_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_event_map_table *table = (t_event_map_table *) get_table(tables, EVENT_MAP_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_event_map_table *row = (t_row_event_map_table *) get_row(tables, EVENT_MAP_TABLE, count);
            xanHashTable_insert(result, (void *) row->parent, (void *) row);
        }
    }
    return result;
}

XanHashTable *internal_hash_generic_param_constraint_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_generic_param_constraint_table *table = (t_generic_param_constraint_table *) get_table(tables, GENERIC_PARAM_CONSTRAINT_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_generic_param_constraint_table *row = (t_row_generic_param_constraint_table *) get_row(tables, GENERIC_PARAM_CONSTRAINT_TABLE, count);
            XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row->owner);
            if (list == NULL) {
                list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                xanHashTable_insert(result, (void *) row->owner, list);
            }
            xanList_append(list, (void *) row);
        }
    }
    return result;
}

XanHashTable *internal_hash_property_method_class_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_method_semantics_table *table = (t_method_semantics_table *) get_table(tables, METHOD_SEMANTICS_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_method_semantics_table *row = (t_row_method_semantics_table *) get_row(tables, METHOD_SEMANTICS_TABLE, count);
            JITUINT32 table_id = (row->association  & 0x1);
            JITUINT32 row_id = ((row->association >> 1) & 0x7FFFFFFF);
            if (table_id == 0x1) {
                XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row_id);
                if (list == NULL) {
                    list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                    xanHashTable_insert(result, (void *) row_id, list);
                }
                xanList_append(list, (void *) row);
            }
        }
    }
    return result;
}


XanHashTable *internal_hash_event_method_class_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_method_semantics_table *table = (t_method_semantics_table *) get_table(tables, METHOD_SEMANTICS_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_method_semantics_table *row = (t_row_method_semantics_table *) get_row(tables, METHOD_SEMANTICS_TABLE, count);
            JITUINT32 table_id = (row->association  & 0x1);
            JITUINT32 row_id = ((row->association >> 1) & 0x7FFFFFFF);
            if (table_id == 0x0) {
                XanList *list = (XanList *) xanHashTable_lookup(result, (void *) row_id);
                if (list == NULL) {
                    list = xanList_new(sharedAllocFunction, freeFunction, NULL);
                    xanHashTable_insert(result, (void *) row_id, list);
                }
                xanList_append(list, (void *) row);
            }
        }
    }
    return result;
}

XanHashTable *internal_hash_implementation_map_table (metadataManager_t *manager, t_binary_information *binary) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);

    XanHashTable *result = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    t_impl_map_table *table = (t_impl_map_table *) get_table(tables, IMPL_MAP_TABLE);

    if (table != NULL) {
        JITUINT32 count;
        for (count = 1; count <= (table->cardinality); count++) {
            t_row_impl_map_table *row = (t_row_impl_map_table *) get_row(tables, IMPL_MAP_TABLE, count);
            JITUINT32 table_id = (row->member_forwarded & 0x1);
            JITUINT32 row_id = ((row->member_forwarded >> 1) & 0x7FFFFFFF);
            if (table_id == 0x1) {
                xanHashTable_insert(result, (void *) row_id, (void *) row);
            }
        }
    }
    return result;
}

void extractInternalCall () {

    JITUINT32 count;

    XanListItem *item = xanList_first(metadataManager->binaries);

    while (item != NULL) {
        t_binary_information *binary = (t_binary_information *) item->data;
        t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
        t_metadata_tables *tables = &((streams->not_stream).tables);
        t_method_def_table *method_table = get_table(tables, METHOD_DEF_TABLE);
        for (count = 1; count <= (method_table->cardinality); count++) {
            BasicMethod *row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, count);
            if ((row->impl_flags & 0x1000) != 0x0) {
                MethodDescriptor *method = getMethodDescriptorFromTableAndRow(metadataManager, NULL, binary, 0x0, count, NULL);
                JITINT8 *internalName = getMethodDescriptorInternalName(method);
                printf("%s\n", internalName);
            }
        }
        item = item->next;
    }
    abort();
}

static inline void internal_freeParameterDescriptor (ParamDescriptor *p) {

    /* Check whether there is something to free.
     */
    if (p == NULL) {
        return ;
    }

    /* Free the memory.
     */
    if (p->signatureInString != NULL) {
        freeFunction(p->signatureInString);
    }
    if (p->attributes != NULL) {
        freeFunction(p->attributes);
    }
    freeFunction(p->tickets);
    freeFunction(p);

    return ;
}
