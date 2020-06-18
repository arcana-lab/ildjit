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
#include <stdlib.h>
#include <assert.h>
#include <platform_API.h>

// My headers
#include <mtm_utils.h>
#include <decoding_tools.h>
#include <type_spec_decoding_tool.h>
#include <iljitmm-system.h>
#include <metadata_types_manager.h>
#include <type_spec_descriptor.h>
#include <basic_concept_manager.h>
// End

#define LOCAL_SIG 0x7

#define BSEARCH(tables, table, field, current_type, index ) \
	{ \
		JITUINT32 table_cardinality = get_table_cardinality(tables, table); \
		JITUINT32 type_table_cardinality = get_table_cardinality(tables, TYPE_DEF_TABLE); \
		JITUINT32 left = 0; \
		JITUINT32 right = type_table_cardinality + 1; \
		JITUINT32 from_method = 0; \
		JITUINT32 to_method = table_cardinality; \
		do { \
			JITUINT32 probe = left + ((right - left) / 2); \
			current_type = (BasicClass *) get_row(tables, TYPE_DEF_TABLE, probe); \
			from_method = current_type->field; \
			if (from_method > index) { \
				right = probe; \
				continue; \
			} \
			if (probe < type_table_cardinality) { \
				BasicClass *next_type = (BasicClass *) get_row(tables, TYPE_DEF_TABLE, probe + 1); \
				to_method = next_type->field - 1; \
			}else{ \
				to_method = table_cardinality; \
			} \
			if (to_method < index) { \
				left = probe; \
				continue; \
			} \
		} while (!(from_method <= index && index <= to_method)); \
	}

static inline JITUINT32 internalGetTypeDescriptorFromParamBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, TypeDescriptor **param);
static inline JITUINT32 internalGetPointerDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, PointerDescriptor **infos);
static inline JITUINT32 internalGetSZArrayDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, ArrayDescriptor **infos);
static inline JITUINT32 internalGetArrayDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, ArrayDescriptor **infos);
static inline JITUINT32 internalGetClassDescriptorFromGenericInstBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, ClassDescriptor **infos);
static inline MethodDescriptor *internalFindMethodInTypeDescriptor (metadataManager_t *manager, TypeDescriptor *owner, JITINT8 *ref_method_name, FunctionPointerSpecDescriptor *ref_signature);
static inline TypeDescriptor *internalGetTypeDescriptorFromMemberRefParentToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 type_token);
static inline JITUINT32 internalDecodeMethodLocal (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, LocalDescriptor **infos);

static inline JITUINT32 internalGetTypeDescriptorFromParamBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, TypeDescriptor **param) {
    PDEBUG("METADATA MANAGER: internalGetTypeDescriptorFromParamBlob: Start\n");
    JITUINT32 bytesRead = 0;
    while (isCmod(signature + bytesRead)) {
        bytesRead += skipCustomMod(signature + bytesRead);
    }
    if (isVoid(signature + bytesRead)) {
        (*param) = getTypeDescriptorFromElementType(manager, ELEMENT_TYPE_VOID);
        bytesRead++;
    } else if (isTypedRef(signature + bytesRead)) {
        (*param) = getTypeDescriptorFromElementType(manager, ELEMENT_TYPE_TYPEDBYREF);
        bytesRead++;
    } else if (isByref(signature + bytesRead)) {
        TypeDescriptor *type;
        bytesRead++;
        bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &type);
        (*param) = makeByRef(type);
    } else {
        bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, param);
    }
    PDEBUG("METADATA MANAGER: internalGetTypeDescriptorFromParamBlob: End\n");
    return bytesRead;
}

inline JITUINT32 getTypeDescriptorFromTypeBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, TypeDescriptor **infos) {
    JITUINT32 bytesRead = 0;

    switch (signature[bytesRead]) {
        case ELEMENT_TYPE_PTR: {
            bytesRead++;
            PointerDescriptor *pointer;
            bytesRead += internalGetPointerDescriptorFromBlob(manager, container, binary, signature + bytesRead, &pointer);
            (*infos) = getTypeDescriptorFromPointerDescriptor(pointer);
            break;
        }
        case ELEMENT_TYPE_CMOD_OPT:
        case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VALUETYPE: {
            bytesRead++;
            JITUINT32 token;
            bytesRead += uncompressValue(signature + bytesRead, &token);
            (*infos) = getTypeDescriptorFromTypeDeforRefToken(manager, container, binary, token);
            break;
        }
        case ELEMENT_TYPE_VAR: {
            bytesRead++;
            JITUINT32 number;
            bytesRead += uncompressValue(signature + bytesRead, &number);
            switch (container->container_type) {
                case CLASS_LEVEL:
                    (*infos) = container->paramType[number];
                    break;
                case METHOD_LEVEL:
                    (*infos) = container->parent->paramType[number];
                    break;
                default:
                    PDEBUG("METADATA MANAGER: getTypeDescriptorFromTypeBlob: ERROR! INVALID CONTAINER LEVEL! %d\n", container->container_type);
                    abort();
            }
            ;
            break;
        }
        case ELEMENT_TYPE_MVAR: {
            bytesRead++;
            JITUINT32 number;
            bytesRead += uncompressValue(signature + bytesRead, &number);
            if (container->container_type == METHOD_LEVEL) {
                (*infos) = container->paramType[number];
            } else {
                PDEBUG("METADATA MANAGER: getTypeDescriptorFromTypeBlob: ERROR! " "INVALID CONTAINER LEVEL! %d\n", container->container_type);
                abort();
            }
            break;
        }
        case ELEMENT_TYPE_GENERICINST: {
            bytesRead++;
            ClassDescriptor *class;
            bytesRead += internalGetClassDescriptorFromGenericInstBlob(manager, container, binary, signature + bytesRead, &class);
            (*infos) = getTypeDescriptorFromClassDescriptor(class);
            break;
        }
        case ELEMENT_TYPE_SZARRAY:
            bytesRead++;
            ArrayDescriptor *array;
            bytesRead += internalGetSZArrayDescriptorFromBlob(manager, container, binary, signature + bytesRead, &array);
            (*infos) = getTypeDescriptorFromArrayDescriptor(array);
            break;
        case ELEMENT_TYPE_ARRAY: {
            bytesRead++;
            ArrayDescriptor *array;
            bytesRead += internalGetArrayDescriptorFromBlob(manager, container, binary, signature + bytesRead, &array);
            (*infos) = getTypeDescriptorFromArrayDescriptor(array);
            break;
        }
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_VOID: {
            bytesRead++;
            (*infos) = getTypeDescriptorFromElementType(manager, signature[0]);
            break;
        }
        case ELEMENT_TYPE_FNPTR: {
            bytesRead++;
            FunctionPointerDescriptor *pointer;
            bytesRead += getFunctionPointerDescriptorFromBlob(manager, container, binary, signature + bytesRead, &pointer);
            (*infos) = getTypeDescriptorFromFunctionPointerDescriptor(pointer);
            break;
        }
        case ELEMENT_TYPE_TYPEDBYREF:
        case ELEMENT_TYPE_BYREF:
        case ELEMENT_TYPE_END:
        default:
            abort();
    }
    return bytesRead;
}

inline JITUINT32 getFunctionPointerDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, FunctionPointerDescriptor **infos) {
    PDEBUG("METADATA MANAGER: getFunctionPointerDescriptorFromBlob: Start\n");
    JITUINT32 bytesRead = 0;
    JITBOOLEAN checkSentinel = JITFALSE;
    JITBOOLEAN hasThis = JITFALSE;
    JITBOOLEAN explicitThis = JITFALSE;
    JITBOOLEAN hasSentinel = JITFALSE;
    JITUINT32 calling_convention = 0;
    JITUINT32 generic_param_count = 0;
    JITUINT32 sentinel_index = 0;
    if ((signature[0] & HASTHIS) == HASTHIS) {
        hasThis = JITTRUE;
    }
    if ((signature[0] & EXPLICITTHIS) == EXPLICITTHIS) {
        explicitThis = JITTRUE;
    }
    if ((signature[0] & VARARG) == VARARG) {
        checkSentinel = JITTRUE;
        calling_convention = VARARG;
    } else if ((signature[0] & CLI_FASTCALL) == CLI_FASTCALL) {
        calling_convention = CLI_FASTCALL;
    } else if ((signature[0] & C_CALL) == C_CALL) {
        calling_convention = C_CALL;
    } else if ((signature[0] & CLI_STDCALL) == CLI_STDCALL) {
        calling_convention = CLI_STDCALL;
    } else if ((signature[0] & THISCALL) == THISCALL) {
        calling_convention = THISCALL;
    } else if ((signature[0] & GENERIC) == GENERIC) {
        calling_convention = GENERIC;
    } else {
        calling_convention = DEFAULT;
    }
    bytesRead++;
    if (calling_convention == GENERIC) {
        bytesRead += uncompressValue(signature + bytesRead, &generic_param_count);
    }
    JITUINT32 param_count;
    bytesRead += uncompressValue(signature + bytesRead, &param_count);
    XanList *params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    TypeDescriptor *result;
    bytesRead += internalGetTypeDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &result);
    JITUINT32 count;
    if (!checkSentinel) {
        for (count = 0; count < param_count; count++) {
            TypeDescriptor *param;
            bytesRead += internalGetTypeDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &param);
            xanList_append(params, param);
        }
    } else {
        for (count = 0; count < param_count; count++) {
            if (isSentinel(signature + bytesRead)) {
                hasSentinel = JITTRUE;
                sentinel_index = count;
                bytesRead++;
            }
            TypeDescriptor *param;
            bytesRead += internalGetTypeDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &param);
            xanList_append(params, param);
        }
    }
    (*infos) = newFunctionPointerDescriptor(manager, hasThis, explicitThis, hasSentinel, sentinel_index, calling_convention, generic_param_count, result, params);
    xanList_destroyList(params);
    PDEBUG("METADATA MANAGER: getFunctionPointerDescriptorFromBlob: End\n");
    return bytesRead;
}

inline FunctionPointerDescriptor *getFormalSignatureFromBlob (metadataManager_t *manager, GenericContainer *container, BasicMethod *row) {
    FunctionPointerDescriptor *pointer = NULL;

    t_binary_information *binary = (t_binary_information *) row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_blob_stream *blob_stream = &(streams->blob_stream);
    t_row_blob_table blob;
    get_blob(blob_stream, row->signature, &blob);

    JITINT8 *signature = blob.blob;
    JITUINT32 bytesRead = 0;
    JITBOOLEAN hasThis = JITFALSE;
    JITBOOLEAN explicitThis = JITFALSE;
    JITUINT32 calling_convention = 0;
    JITUINT32 generic_param_count = 0;

    if ((signature[0] & HASTHIS) == HASTHIS) {
        hasThis = JITTRUE;
    }
    if ((signature[0] & EXPLICITTHIS) == EXPLICITTHIS) {
        explicitThis = JITTRUE;
    }
    if ((signature[0] & VARARG) == VARARG) {
        calling_convention = VARARG;
    } else if ((signature[0] & CLI_FASTCALL) == CLI_FASTCALL) {
        calling_convention = CLI_FASTCALL;
    } else if ((signature[0] & C_CALL) == C_CALL) {
        calling_convention = C_CALL;
    } else if ((signature[0] & CLI_STDCALL) == CLI_STDCALL) {
        calling_convention = CLI_STDCALL;
    } else if ((signature[0] & THISCALL) == THISCALL) {
        calling_convention = THISCALL;
    } else if ((signature[0] & GENERIC) == GENERIC) {
        calling_convention = GENERIC;
    } else {
        calling_convention = DEFAULT;
    }
    bytesRead++;
    if (calling_convention == GENERIC) {
        bytesRead += uncompressValue(signature + bytesRead, &generic_param_count);
    }
    JITUINT32 param_count;
    bytesRead += uncompressValue(signature + bytesRead, &param_count);
    XanList *params = xanList_new(sharedAllocFunction, freeFunction, NULL);
    TypeDescriptor *result;
    bytesRead += internalGetTypeDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &result);
    JITUINT32 count;
    for (count = 0; count < param_count; count++) {
        TypeDescriptor *param;
        bytesRead += internalGetTypeDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &param);
        xanList_append(params, param);
    }
    pointer = newUnCachedFunctionPointerDescriptor(manager, hasThis, explicitThis, JITFALSE, 0, calling_convention, generic_param_count, result, params);
    return pointer;
}


static inline JITUINT32 internalGetPointerDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, PointerDescriptor **infos) {
    JITUINT32 bytesRead = 0;

    while (!isType(signature + bytesRead)) {
        bytesRead += skipCustomMod(signature + bytesRead);
    }
    TypeDescriptor *type;
    bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &type);
    (*infos) = newPointerDescriptor(manager, type);
    return bytesRead;
}

static inline JITUINT32 internalGetSZArrayDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, ArrayDescriptor **infos) {
    JITUINT32 bytesRead = 0;

    while (!isType(signature + bytesRead)) {
        bytesRead += skipCustomMod(signature + bytesRead);
    }
    TypeDescriptor *type;
    bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &type);
    (*infos) = newArrayDescriptor(manager, type, 1, 0, NULL, 0, NULL);
    return bytesRead;
}

static inline JITUINT32 internalGetArrayDescriptorFromBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, ArrayDescriptor **infos) {
    JITUINT32 bytesRead = 0;
    JITUINT32 rank;
    JITUINT32 numSizes;
    JITUINT32 *sizes = NULL;
    JITUINT32 numBounds;
    JITUINT32 *bounds = NULL;
    JITUINT32 count;
    TypeDescriptor *type;

    bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &type);
    bytesRead += uncompressValue(signature + bytesRead, &rank);
    bytesRead += uncompressValue(signature + bytesRead, &numSizes);
    if (numSizes > 0) {
        sizes = alloca(sizeof(JITUINT32) * numSizes);
        for (count = 0; count < numSizes; count++) {
            bytesRead += uncompressValue(signature + bytesRead, sizes + count);
        }
    }
    bytesRead += uncompressValue(signature + bytesRead, &numBounds);
    if (numBounds > 0) {
        bounds = alloca(sizeof(JITUINT32) * numBounds);
        for (count = 0; count < numBounds; count++) {
            bytesRead += uncompressValue(signature + bytesRead, bounds + count);
        }
    }
    (*infos) = newArrayDescriptor(manager, type, rank, numSizes, sizes, numBounds, bounds);
    return bytesRead;
}

static inline JITUINT32 internalGetClassDescriptorFromGenericInstBlob (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, ClassDescriptor **infos) {
    JITUINT32 bytesRead = 0;
    JITUINT32 token;
    JITUINT32 arg_count;

    if ((signature[0] & ELEMENT_TYPE_VALUETYPE) == ELEMENT_TYPE_VALUETYPE) {
        bytesRead++;
    } else if ((signature[0] & ELEMENT_TYPE_CLASS) == ELEMENT_TYPE_CLASS) {
        bytesRead++;
    } else {
        PDEBUG("METADATA MANAGER: internalGetClassDescriptorFromGenericInstBlob: ERROR! INVALID TYPE! %d\n", signature[0]);
        abort();
    }

    bytesRead += uncompressValue(signature + bytesRead, &token);
    BasicClass *row = getBasicClassFromTypeDefOrRef(manager, binary, token);
    bytesRead += uncompressValue(signature + bytesRead, &arg_count);
    GenericContainer class_container;
    initTempGenericContainer(class_container, CLASS_LEVEL, NULL, arg_count);
    JITUINT32 count;
    for (count = 0; count < arg_count; count++) {
        TypeDescriptor *typeArg;
        bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &typeArg);
        insertTypeDescriptorIntoGenericContainer(&class_container, typeArg, count);
    }
    (*infos) = newClassDescriptor(manager, row, &class_container);
    assert((*infos)->container != &class_container);
    return bytesRead;
}

inline TypeDescriptor *getTypeDescriptorFromTypeDeforRefToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 type_token) {
    JITUINT8 table_id = (type_token & 0x3);
    JITUINT32 row_id = ((type_token >> 2) & 0x3FFFFFFF );

    return getTypeDescriptorFromTableAndRow(manager, container, binary, table_id, row_id);
}

inline TypeDescriptor *getTypeDescriptorFromTableAndRow (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_blob_stream                   *blob_stream = &(streams->blob_stream);
    RowObjectID *objID = (RowObjectID *) createObjectRowID(manager, binary, container, table_id, row_id, ILTYPE);

    if (objID->object == NULL) {
        TypeDescriptor *return_type;
        switch (table_id) {
            case 0x0:
                /* TypeDef table	*/
            case 0x1:
                /* TypeRef table	*/
            {
                BasicClass *row = getBasicClassFromTableAndRow(manager, binary, table_id, row_id);
                assert(row != NULL);
                ClassDescriptor *class = newClassDescriptor(manager, row, NULL);
                return_type = getTypeDescriptorFromClassDescriptor(class);
                break;
            }
            case 0x2:
                /* TypeSpec table	*/
            {
                t_row_type_spec_table *typeSpec = (t_row_type_spec_table *) get_row(tables, TYPE_SPEC_TABLE, row_id);
                t_row_blob_table blob;
                get_blob(blob_stream, typeSpec->signature, &blob);
                getTypeDescriptorFromTypeBlob(manager, container, binary, blob.blob, &return_type);
                break;
            }
            default:
                PDEBUG("METADATA MANAGER: getTypeDescriptorFromTableAndRow: ERROR! " "INVALID TABLE! %d\n", table_id);
                abort();
        }
        broadcastObjectRowID(manager, objID, (void *) return_type);
    }
    return (TypeDescriptor *) objID->object;
}

static inline TypeDescriptor *internalGetTypeDescriptorFromMemberRefParentToken (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 type_token) {
    JITUINT8 table_id = (type_token  & 0x7);
    JITUINT32 row_id = ((type_token >> 3) & 0x1FFFFFFF);
    TypeDescriptor *return_type;

    switch (table_id) {
        case 0x0:
            /* TypeDef Table */
            return_type = getTypeDescriptorFromTableAndRow(manager, container, binary, 0x0, row_id);
            break;
        case 0x1:
            /* TypeRef Table */
            return_type = getTypeDescriptorFromTableAndRow(manager, container, binary, 0x1, row_id);
            break;
        case 0x4:
            /* TypeSpec Table */
            return_type = getTypeDescriptorFromTableAndRow(manager, container, binary, 0x2, row_id);
            break;
        default:
            PDEBUG("METADATA MANAGER: internalGetTypeDescriptorFromMemberRefParentToken: ERROR! INVALID TABLE! %d\n", table_id);
            abort();
    }
    return return_type;
}

inline FieldDescriptor *getFieldDescriptorFromTableAndRow (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_string_stream *string_stream = &(streams->string_stream);

    RowObjectID *objID = (RowObjectID *) createObjectRowID(manager, binary, container, table_id, row_id, ILFIELD);

    if (objID->object == NULL) {
        FieldDescriptor *return_field;
        BasicField *row = NULL;
        TypeDescriptor *owner;
        switch (table_id) {
            case 0x0:
                /* Field Table */
            {
                row = (BasicField *) get_row(tables, FIELD_TABLE, row_id);
                BasicClass *previous_type;
                BSEARCH(tables, FIELD_TABLE, field_list, previous_type, row->index);

                /*t_type_def_table *type_table = get_table(tables, TYPE_DEF_TABLE);
                   BasicClass *previous_type = (BasicClass *) get_row (tables, TYPE_DEF_TABLE, 1);
                   BasicClass *current_type;
                   JITUINT32 count;
                   for (count=2; count <= type_table->cardinality; count++){
                        current_type = (BasicClass *) get_row (tables, TYPE_DEF_TABLE, count);
                        if (current_type->field_list>previous_type->field_list){;
                                if (current_type->field_list > row->index) break;
                        }
                        previous_type = current_type;
                   }*/

                ClassDescriptor *class = newClassDescriptor(manager, previous_type, NULL);
                owner = getTypeDescriptorFromClassDescriptor(class);
                return_field = newFieldDescriptor(manager, row, owner);
                break;
            }
            case 0x1:
                /* FieldRef table */
            {
                t_row_member_ref_table *member_ref = (t_row_member_ref_table*) get_row(tables, MEMBER_REF_TABLE, row_id);
                JITINT8 *field_name = get_string(string_stream, member_ref->name);

                owner = internalGetTypeDescriptorFromMemberRefParentToken(manager, container, binary, member_ref->classIndex);
                return_field = getFieldDescriptorFromNameTypeDescriptor(owner, field_name);
                break;
            }
            default:
                PDEBUG("METADATA MANAGER: getFieldDescriptorFromTableAndRow: ERROR: Reference to the unknow table\n");
                abort();
        }
        broadcastObjectRowID(manager, objID, (void *) return_field);
    }
    return (FieldDescriptor *) objID->object;
}

inline MethodDescriptor *getMethodDescriptorFromMethodDefOrRef (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT32 type_token, FunctionPointerDescriptor **actualSignature) {
    JITUINT8 table_id = (type_token  & 0x1);
    JITUINT32 row_id = ((type_token >> 1) & 0x7FFFFFFF);

    return getMethodDescriptorFromTableAndRow(manager, container, binary, table_id, row_id, actualSignature);
}

inline FunctionPointerSpecDescriptor *getRawSignatureFromBlob (metadataManager_t *manager, GenericSpecContainer *container, BasicMethod *row) {
    FunctionPointerSpecDescriptor *rawSignature;
    t_binary_information *binary = (t_binary_information *) row->binary;
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_blob_stream *blob_stream = &(streams->blob_stream);
    t_row_blob_table blob;

    get_blob(blob_stream, row->signature, &blob);
    JITINT8 *signature = blob.blob;
    getFunctionPointerSpecDescriptorFromBlob(manager, container, binary, signature, &(rawSignature));
    return rawSignature;
}

static inline MethodDescriptor *internalFindMethodInTypeDescriptor (metadataManager_t *manager, TypeDescriptor *owner, JITINT8 *ref_method_name, FunctionPointerSpecDescriptor *ref_signature) {
    MethodDescriptor *result;

    /* Initialize the variables		*/
    result = NULL;

    switch (owner->element_type) {
        case ELEMENT_ARRAY: {
            ClassDescriptor *object_class = manager->System_Object;
            XanList *list = object_class->getConstructors(object_class);
            result = (MethodDescriptor *) xanList_first(list)->data;
            break;
        }
        case ELEMENT_CLASS: {
            XanList 	*methodList;
            XanListItem 	*item;
            JITBOOLEAN 	found;
            methodList	= getMethodDescriptorFromNameTypeDescriptor(owner, ref_method_name);
            found 		= JITFALSE;
            item 		= xanList_first(methodList);
            while (item != NULL) {
                result = (MethodDescriptor *) item->data;
                FunctionPointerSpecDescriptor *def_signature = getRawSignatureFromBlob(manager, NULL, result->row);
                found = equalsNoVarArgFunctionPointerSpecDescriptor(def_signature, ref_signature);
                destroyFunctionPointerSpecDescriptor(def_signature);
                if (found) {
                    break;
                }
                item = item->next;
            }
            break;
        }
        default:
            PDEBUG("METADATA MANAGER: internalFindMethodInTypeDescriptor: ERROR! INVALID TYPE! %d\n", owner->element_type);
            abort();
    }
    return result;
}

inline MethodDescriptor *getMethodDescriptorFromTableAndRow (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id, FunctionPointerDescriptor **actualSignature) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);
    t_string_stream *string_stream = &(streams->string_stream);
    t_blob_stream   *blob_stream = &(streams->blob_stream);

    RowObjectID *objID = (RowObjectID *) createObjectRowID(manager, binary, container, table_id, row_id, ILMETHOD);

    if (objID->object == NULL) {
        ActualMethodDescriptor *object = (ActualMethodDescriptor *) sharedAllocFunction(sizeof(ActualMethodDescriptor));
        switch (table_id) {
            case 0x0:
                /* MethodDef table	*/
            {

                BasicMethod *row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, row_id);
                BasicClass *previous_type;
                BSEARCH(tables, METHOD_DEF_TABLE, method_list, previous_type, row->index);

                /*t_type_def_table *type_table = get_table(tables, TYPE_DEF_TABLE);
                   BasicClass *previous_type = (BasicClass *) get_row (tables, TYPE_DEF_TABLE, 1);
                   BasicClass *current_type;
                   JITUINT32 count;
                   for (count=2; count <= type_table->cardinality; count++){
                        current_type = (BasicClass *) get_row (tables, TYPE_DEF_TABLE, count);
                        if (current_type->method_list>previous_type->method_list){;
                                if (current_type->method_list > row->index) break;
                        }
                        previous_type = current_type;
                   }*/

                ClassDescriptor *class = newClassDescriptor(manager, previous_type, NULL);
                TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(class);
                object->method = newMethodDescriptor(manager, row, owner, NULL);
                object->actualSignature = getFormalSignature(object->method);
                break;
            }
            case 0x1:
                /* MethodRef table	*/
            {

                t_row_member_ref_table *member_ref = (t_row_member_ref_table*) get_row(tables, MEMBER_REF_TABLE, row_id);

                JITUINT8 ref_table_id = (member_ref->classIndex  & 0x7);
                JITUINT32 ref_row_id = ((member_ref->classIndex >> 3) & 0x1FFFFFFF);

                t_row_blob_table blob;
                get_blob(blob_stream, member_ref->signature, &blob);
                JITINT8 *signature = blob.blob;

                switch (ref_table_id) {
                    case 0x3:
                        /* MethodDef table reference */
                    {
                        BasicMethod *row = (BasicMethod *) get_row(tables, METHOD_DEF_TABLE, ref_row_id);
                        BasicClass *previous_type;
                        BSEARCH(tables, METHOD_DEF_TABLE, method_list, previous_type, row->index);

                        /*t_type_def_table *type_table = get_table(tables, TYPE_DEF_TABLE);
                           BasicClass *previous_type = (BasicClass *) get_row (tables, TYPE_DEF_TABLE, 1);
                           BasicClass *current_type;
                           JITUINT32 count;
                           for (count=2; count <= type_table->cardinality; count++){
                                current_type = (BasicClass *) get_row (tables, TYPE_DEF_TABLE, count);
                                if (current_type->method_list>previous_type->method_list){;
                                        if (current_type->method_list > row->index) break;
                                }
                                previous_type = current_type;
                           }*/

                        ClassDescriptor *class = newClassDescriptor(manager, previous_type, NULL);
                        TypeDescriptor *owner = getTypeDescriptorFromClassDescriptor(class);
                        object->method = newMethodDescriptor(manager, row, owner, NULL);
                        break;
                    }
                    case 0x00:
                    case 0x01:
                    case 0x04:
                        /* TypeDef,TypeRef,TypeSpec table reference */
                    {
                        FunctionPointerSpecDescriptor *ref_signature;
                        getFunctionPointerSpecDescriptorFromBlob(manager, NULL, binary, signature, &ref_signature);
                        JITINT8 *ref_method_name = get_string(string_stream, member_ref->name);
                        TypeDescriptor *owner_type = internalGetTypeDescriptorFromMemberRefParentToken(manager, container, binary, member_ref->classIndex);
                        object->method = internalFindMethodInTypeDescriptor(manager, owner_type, ref_method_name, ref_signature);
                        destroyFunctionPointerSpecDescriptor(ref_signature);
                        break;
                    }
                    default:
                        abort();
                }
                if (!isGenericMethodDefinition(object->method)) {
                    if (!object->method->attributes->is_vararg) {
                        object->actualSignature = getFormalSignature(object->method);
                    } else {
                        GenericContainer *resolving_container = getResolvingContainerFromMethodDescriptor(object->method);
                        getFunctionPointerDescriptorFromBlob(manager, resolving_container, binary, signature, &(object->actualSignature));
                    }
                } else {
                    object->actualSignature = NULL;
                }
                break;
            }
            case 0x2:
                /* Method Spec */
            {
                t_row_method_spec_table *methodSpec = (t_row_method_spec_table*) get_row(tables, METHOD_SPEC_TABLE, row_id);
                MethodDescriptor *genericDefinition = getMethodDescriptorFromMethodDefOrRef(manager, container, binary, methodSpec->method, NULL);
                t_row_blob_table blob;
                get_blob(blob_stream, methodSpec->instantiation, &blob);
                JITINT8 *signature = blob.blob;
                JITUINT32 bytesRead = 0;
                if (signature[0] != GENERICINST) {
                    PDEBUG("METADATA MANAGER: getMethodDescriptorFromTableAndRow: ERROR!: Blob is not a Method Spec\n");
                    abort();
                }
                bytesRead++;
                JITUINT32 arg_count;
                bytesRead += uncompressValue(signature + bytesRead, &arg_count);
                GenericContainer method_container;
                initTempGenericContainer(method_container, METHOD_LEVEL, NULL, arg_count);
                JITUINT32 count;
                for (count = 0; count < arg_count; count++) {
                    TypeDescriptor *typeArg;
                    bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &typeArg);
                    insertTypeDescriptorIntoGenericContainer(&method_container, typeArg, count);
                }
                object->method = getInstanceFromMethodDescriptor(genericDefinition, &method_container);
                object->actualSignature = getFormalSignature(object->method);
            }
            break;
            default:
                PDEBUG("METADATA MANAGER: getMethodDescriptorFromTableAndRow: ERROR! INVALID TABLE! %d\n", table_id);
                abort();
        }
        broadcastObjectRowID(manager, objID, (void *) object);
    }
    if (actualSignature != NULL) {
        (*actualSignature) = ((ActualMethodDescriptor *) objID->object)->actualSignature;
    }
    return ((ActualMethodDescriptor *) objID->object)->method;
}


static inline JITUINT32 internalDecodeMethodLocal (metadataManager_t *manager, GenericContainer *container, t_binary_information *binary, JITINT8 *signature, LocalDescriptor **infos) {
    JITBOOLEAN is_pinned = JITFALSE;
    TypeDescriptor *type = NULL;
    JITUINT32 bytesRead = 0;

    if (isTypedRef(signature + bytesRead)) {
        bytesRead++;
        type = getTypeDescriptorFromElementType(manager, ELEMENT_TYPE_TYPEDBYREF);
    } else {
        JITBOOLEAN is_byRef = JITFALSE;
        bytesRead += (isPinned(signature + bytesRead) == JITTRUE);
        while ((!isByref(signature + bytesRead)) && (!isType(signature + bytesRead))) {
            bytesRead += skipCustomMod(signature + bytesRead);
            bytesRead += (isPinned(signature + bytesRead) == JITTRUE);
        }
        if (isByref(signature + bytesRead)) {
            bytesRead++;
            is_byRef = JITTRUE;
        }
        bytesRead += getTypeDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, &(type));
        if (is_byRef) {
            type = makeByRef(type);
        }
    }
    (*infos) = newLocalDescriptor(is_pinned, type);

    return bytesRead;
}

inline XanList *decodeMethodLocals (metadataManager_t *manager, t_binary_information *binary, JITUINT32 token, GenericContainer *container) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_blob_stream                   *blob_stream = &(streams->blob_stream);

    JITUINT32 row_id = token & 0x00FFFFFF;
    t_row_standalone_sig_table *row = (t_row_standalone_sig_table *) get_row(tables, STANDALONE_SIG_TABLE, row_id);

    t_row_blob_table blob;

    get_blob(blob_stream, row->signature, &blob);
    JITINT8 *signature = blob.blob;
    JITUINT32 bytesRead = 0;
    if ((signature[0] & LOCAL_SIG) != LOCAL_SIG) {
        PDEBUG("METADATA MANAGER: decodeMethodLocal: ERROR: INVALID LOCALVARSIG STREAM!\n");
        abort();
    }
    bytesRead++;
    JITUINT32 local_count;
    bytesRead += uncompressValue(signature + bytesRead, &local_count);
    XanList *result = xanList_new(sharedAllocFunction, freeFunction, NULL);
    JITUINT32 count;
    for (count = 0; count < local_count; count++) {
        LocalDescriptor *local;
        bytesRead += internalDecodeMethodLocal(manager, container, binary, signature + bytesRead, &local);
        PDEBUG("METADATA MANAGER: decodeMethodLocal: Local %d is a %s\n", count, local->type->getSignatureInString(local->type));
        xanList_append(result, local);
    }
    return result;
}
