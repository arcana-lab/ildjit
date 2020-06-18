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

// My headers
#include <mtm_utils.h>
#include <iljitmm-system.h>
#include <type_spec_decoding_tool.h>
#include <basic_concept_manager.h>
#include <type_spec_descriptor.h>
#include <metadata_types_manager.h>
// End

static inline ClassSpecDescriptor *internalGetClassSpecDescriptorElementType (metadataManager_t *manager, JITUINT32 element_type);

static inline JITUINT32 internalGetTypeSpecDescriptorFromParamBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, TypeSpecDescriptor **param);
static inline JITUINT32 internalGetTypeSpecDescriptorFromTypeBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, JITBOOLEAN isByRef, TypeSpecDescriptor **infos);
static inline JITUINT32 internalGetPointerSpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, PointerSpecDescriptor **infos);
static inline JITUINT32 internalGetSZArraySpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, ArraySpecDescriptor **infos);
static inline JITUINT32 internalGetArraySpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, ArraySpecDescriptor **infos);
static inline JITUINT32 internalGetClassSpecDescriptorFromGenericInstBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, ClassSpecDescriptor **infos);
static inline TypeSpecDescriptor *internalGetTypeSpecDescriptorFromTypeDeforRefToken (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITUINT32 type_token, JITBOOLEAN isByRef);
static inline TypeSpecDescriptor *internalGetTypeSpecDescriptorFromTableAndRow (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id, JITBOOLEAN isByRef);




static inline ClassSpecDescriptor *internalGetClassSpecDescriptorElementType (metadataManager_t *manager, JITUINT32 element_type) {
    BasicClass *row = getClassDescriptorFromElementType(manager, element_type)->row;

    return newClassSpecDescriptor(row, NULL);
}

static inline JITUINT32 internalGetTypeSpecDescriptorFromParamBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, TypeSpecDescriptor **param) {
    JITUINT32 bytesRead = 0;

    if (!isType(signature + bytesRead)) {
        bytesRead += skipCustomMod(signature + bytesRead);
    }
    if (isVoid(signature + bytesRead)) {
        ClassSpecDescriptor *class = internalGetClassSpecDescriptorElementType(manager, ELEMENT_TYPE_VOID);
        (*param) = newTypeSpecDescriptor(ELEMENT_CLASS, class, JITFALSE);
        bytesRead++;
    } else if (isTypedRef(signature + bytesRead)) {
        ClassSpecDescriptor *class = internalGetClassSpecDescriptorElementType(manager, ELEMENT_TYPE_TYPEDBYREF);
        (*param) = newTypeSpecDescriptor(ELEMENT_CLASS, class, JITFALSE);
        bytesRead++;
    } else if (isByref(signature + bytesRead)) {
        bytesRead++;
        bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITTRUE, param);
    } else {
        bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITFALSE, param);
    }
    return bytesRead;
}

static inline JITUINT32 internalGetTypeSpecDescriptorFromTypeBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, JITBOOLEAN isByRef, TypeSpecDescriptor **infos) {
    JITUINT32 bytesRead = 0;

    switch (signature[0]) {
        case ELEMENT_TYPE_PTR: {
            bytesRead++;
            PointerSpecDescriptor *pointer;
            bytesRead += internalGetPointerSpecDescriptorFromBlob(manager, container, binary, signature + bytesRead, &pointer);
            (*infos) = newTypeSpecDescriptor(ELEMENT_PTR, (void *) pointer, isByRef);
            break;
        }
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VALUETYPE: {
            bytesRead++;
            JITUINT32 token;
            bytesRead += uncompressValue(signature + bytesRead, &token);
            (*infos) = internalGetTypeSpecDescriptorFromTypeDeforRefToken(manager, container, binary, token, isByRef);
            break;
        }
        case ELEMENT_TYPE_VAR: {
            bytesRead++;
            JITUINT32 number;
            bytesRead += uncompressValue(signature + bytesRead, &number);
            if (container != NULL) {
                container->paramType[number]->refCount++;
                (*infos) = container->paramType[number];
            } else {
                GenericVarSpecDescriptor *var;
                var = newGenericVarSpecDescriptor(number);
                (*infos) = newTypeSpecDescriptor(ELEMENT_VAR, (void *) var, isByRef);
            }
            break;
        }
        case ELEMENT_TYPE_MVAR: {
            bytesRead++;
            JITUINT32 number;
            bytesRead += uncompressValue(signature + bytesRead, &number);
            GenericMethodVarSpecDescriptor *mvar;
            mvar = newGenericMethodVarSpecDescriptor(number);
            (*infos) = newTypeSpecDescriptor(ELEMENT_MVAR, (void *) mvar, isByRef);
            break;
        }
        case ELEMENT_TYPE_GENERICINST: {
            bytesRead++;
            ClassSpecDescriptor *class;
            bytesRead += internalGetClassSpecDescriptorFromGenericInstBlob(manager, container, binary, signature + bytesRead, &class);
            (*infos) = newTypeSpecDescriptor(ELEMENT_CLASS, (void *) class, isByRef);
            break;
        }
        case ELEMENT_TYPE_SZARRAY: {
            bytesRead++;
            ArraySpecDescriptor *array;
            bytesRead += internalGetSZArraySpecDescriptorFromBlob(manager, container, binary, signature + bytesRead, &array);
            (*infos) = newTypeSpecDescriptor(ELEMENT_ARRAY, array, isByRef);
            break;
        }
        case ELEMENT_TYPE_ARRAY: {
            bytesRead++;
            ArraySpecDescriptor *array;
            bytesRead += internalGetArraySpecDescriptorFromBlob(manager, container, binary, signature + bytesRead, &array);
            (*infos) = newTypeSpecDescriptor(ELEMENT_ARRAY, (void *) array, isByRef);
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
            ClassSpecDescriptor *class = internalGetClassSpecDescriptorElementType(manager, signature[0]);
            (*infos) = newTypeSpecDescriptor(ELEMENT_CLASS, (void *) class, isByRef);
            break;
        }
        case ELEMENT_TYPE_FNPTR: {
            bytesRead++;
            FunctionPointerSpecDescriptor *pointer;
            bytesRead += getFunctionPointerSpecDescriptorFromBlob(manager, container, binary, signature + bytesRead, &pointer);
            (*infos) = newTypeSpecDescriptor(ELEMENT_FNPTR, (void *) pointer, isByRef);
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

inline JITUINT32 getFunctionPointerSpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, FunctionPointerSpecDescriptor **infos) {
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
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: VARARG calling convention detected.\n");
        checkSentinel = JITTRUE;
        calling_convention = VARARG;
    } else if ((signature[0] & CLI_FASTCALL) == CLI_FASTCALL) {
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: CLI_FASTCALL calling convention detected.\n");
        calling_convention = CLI_FASTCALL;
    } else if ((signature[0] & C_CALL) == C_CALL) {
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: C calling convention detected.\n");
        calling_convention = C_CALL;
    } else if ((signature[0] & CLI_STDCALL) == CLI_STDCALL) {
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: CLI_STDCALL calling convention detected.\n");
        calling_convention = CLI_STDCALL;
    } else if ((signature[0] & THISCALL) == THISCALL) {
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: THISCALL calling convention detected.\n");
        calling_convention = THISCALL;
    } else if ((signature[0] & GENERIC) == GENERIC) {
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: THISCALL calling convention detected.\n");
        calling_convention = GENERIC;
    } else {
        PDEBUG("METADATA MANAGER: getFunctionPointerSpecDescriptorFromBlob: DEFAULT calling convention detected.\n");
        calling_convention = DEFAULT;
    }
    bytesRead++;
    if (calling_convention == GENERIC) {
        bytesRead += uncompressValue(signature + bytesRead, &generic_param_count);
    }
    JITUINT32 param_count;
    bytesRead += uncompressValue(signature + bytesRead, &param_count);
    TypeSpecDescriptor **params = NULL;
    TypeSpecDescriptor *result;
    bytesRead += internalGetTypeSpecDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &result);
    JITUINT32 count;
    if (param_count > 0) {
        params = sharedAllocFunction(sizeof(TypeDescriptor *) * param_count);
        if (!checkSentinel) {
            for (count = 0; count < param_count; count++) {
                TypeSpecDescriptor *param;
                bytesRead += internalGetTypeSpecDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &param);
                params[count] = param;
            }
        } else {
            for (count = 0; count < param_count; count++) {
                if (isSentinel(signature + bytesRead)) {
                    hasSentinel = JITTRUE;
                    sentinel_index = count;
                    bytesRead++;
                }
                TypeSpecDescriptor *param;
                bytesRead += internalGetTypeSpecDescriptorFromParamBlob(manager, container, binary, signature + bytesRead, &param);
                params[count] = param;
            }
        }
    }
    (*infos) = newFunctionPointerSpecDescriptor(hasThis, explicitThis, hasSentinel, sentinel_index, calling_convention, generic_param_count, result, param_count, params);
    return bytesRead;
}

static inline JITUINT32 internalGetPointerSpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, PointerSpecDescriptor **infos) {
    JITUINT32 bytesRead = 0;

    while (!isType(signature + bytesRead)) {
        bytesRead += skipCustomMod(signature + bytesRead);
    }
    TypeSpecDescriptor *type;
    bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITFALSE, &type);
    (*infos) = newPointerSpecDescriptor(type);
    return bytesRead;
}

static inline JITUINT32 internalGetSZArraySpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, ArraySpecDescriptor **infos) {
    JITUINT32 bytesRead = 0;

    while (!isType(signature + bytesRead)) {
        bytesRead += skipCustomMod(signature + bytesRead);
    }
    TypeSpecDescriptor *type;
    bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITFALSE, &type);
    (*infos) = newArraySpecDescriptor(type, 1, 0, NULL, 0, NULL);
    return bytesRead;
}

static inline JITUINT32 internalGetArraySpecDescriptorFromBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, ArraySpecDescriptor **infos) {
    JITUINT32 bytesRead = 0;
    JITUINT32 rank;
    JITUINT32 numSizes;
    JITUINT32 *sizes = NULL;
    JITUINT32 numBounds;
    JITUINT32 *bounds = NULL;
    JITUINT32 count;
    TypeSpecDescriptor *type;

    bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITFALSE, &type);
    bytesRead += uncompressValue(signature + bytesRead, &rank);
    bytesRead += uncompressValue(signature + bytesRead, &numSizes);
    if (numSizes > 0) {
        sizes = sharedAllocFunction(sizeof(JITUINT32) * numSizes);
        for (count = 0; count < numSizes; count++) {
            bytesRead += uncompressValue(signature + bytesRead, sizes + count);
        }
    }
    bytesRead += uncompressValue(signature + bytesRead, &numBounds);
    if (numBounds > 0) {
        bounds = sharedAllocFunction(sizeof(JITUINT32) * numBounds);
        for (count = 0; count < numBounds; count++) {
            bytesRead += uncompressValue(signature + bytesRead, bounds + count);
        }
    }
    (*infos) = newArraySpecDescriptor(type, rank, numSizes, sizes, numBounds, bounds);
    return bytesRead;
}

static inline JITUINT32 internalGetClassSpecDescriptorFromGenericInstBlob (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITINT8 *signature, ClassSpecDescriptor **infos) {
    JITUINT32 bytesRead = 0;
    JITUINT32 token;
    JITUINT32 arg_count;

    if ((signature[0] & ELEMENT_TYPE_VALUETYPE) == ELEMENT_TYPE_VALUETYPE) {
        bytesRead++;
    } else if ((signature[0] & ELEMENT_TYPE_CLASS) == ELEMENT_TYPE_CLASS) {
        bytesRead++;
    } else {
        PDEBUG("METADATA MANAGER: internalGetClassSpecDescriptorFromGenericInstBlob: ERROR! INVALID TYPE! %d\n", signature[0]);
        abort();
    }

    bytesRead += uncompressValue(signature + bytesRead, &token);
    BasicClass *row = getBasicClassFromTypeDefOrRef(manager, binary, token);
    bytesRead += uncompressValue(signature + bytesRead, &arg_count);
    GenericSpecContainer *class_container = newGenericSpecContainer(arg_count);
    JITUINT32 count;
    for (count = 0; count < arg_count; count++) {
        TypeSpecDescriptor *typeArg;
        bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITFALSE, &typeArg);
        insertTypeSpecDescriptorIntoGenericSpecContainer(class_container, typeArg, count);
    }
    (*infos) = newClassSpecDescriptor(row, class_container);
    return bytesRead;
}

static inline TypeSpecDescriptor *internalGetTypeSpecDescriptorFromTypeDeforRefToken (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITUINT32 type_token, JITBOOLEAN isByRef) {
    JITUINT8 table_id = (type_token & 0x3);
    JITUINT32 row_id = ((type_token >> 2) & 0x3FFFFFFF );

    return internalGetTypeSpecDescriptorFromTableAndRow(manager, container, binary, table_id, row_id, isByRef);
}

static inline TypeSpecDescriptor *internalGetTypeSpecDescriptorFromTableAndRow (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id, JITBOOLEAN isByRef) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_blob_stream                   *blob_stream = &(streams->blob_stream);
    TypeSpecDescriptor *return_type;

    switch (table_id) {
        case 0x0:
            /* TypeDef table	*/
        case 0x1:
            /* TypeRef table	*/
        {
            BasicClass *row = getBasicClassFromTableAndRow(manager, binary, table_id, row_id);
            ClassSpecDescriptor *class = newClassSpecDescriptor(row, NULL);
            return_type = newTypeSpecDescriptor(ELEMENT_CLASS, (void *) class, isByRef);
            break;
        }
        case 0x2:
            /* TypeSpec table	*/
        {
            t_row_type_spec_table *typeSpec = (t_row_type_spec_table *) get_row(tables, TYPE_SPEC_TABLE, row_id);
            t_row_blob_table blob;
            get_blob(blob_stream, typeSpec->signature, &blob);
            internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, blob.blob, isByRef, &return_type);
            break;
        }
        default:
            PDEBUG("METADATA MANAGER: internalGetTypeSpecDescriptorFromTableAndRow: ERROR! " "INVALID TABLE! %d\n", table_id);
            abort();
    }
    return return_type;
}

inline GenericSpecContainer *getGenericSpecContainerFromTypeDeforRefToken (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITUINT32 type_token) {
    JITUINT8 table_id = (type_token & 0x3);
    JITUINT32 row_id = ((type_token >> 2) & 0x3FFFFFFF );

    return getGenericSpecContainerFromTableAndRow(manager, container, binary, table_id, row_id);
}

inline GenericSpecContainer *getGenericSpecContainerFromTableAndRow (metadataManager_t *manager, GenericSpecContainer *container, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id) {
    t_streams_metadata              *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables               *tables = &((streams->not_stream).tables);
    t_blob_stream                   *blob_stream = &(streams->blob_stream);
    GenericSpecContainer *return_container;

    switch (table_id) {
        case 0x0:
            /* TypeDef table	*/
        case 0x1:
            /* TypeRef table	*/
            return_container = NULL;
            break;
        case 0x2:
            /* TypeSpec table	*/
        {
            t_row_type_spec_table *typeSpec = (t_row_type_spec_table *) get_row(tables, TYPE_SPEC_TABLE, row_id);
            t_row_blob_table blob;
            get_blob(blob_stream, typeSpec->signature, &blob);
            JITINT8 *signature = blob.blob;
            switch (signature[0]) {
                case ELEMENT_TYPE_CLASS:
                case ELEMENT_TYPE_VALUETYPE:
                    return_container = NULL;
                    break;
                case ELEMENT_TYPE_GENERICINST: {
                    JITUINT32 bytesRead = 2;
                    JITUINT32 token;
                    JITUINT32 arg_count;
                    bytesRead += uncompressValue(signature + bytesRead, &token);
                    bytesRead += uncompressValue(signature + bytesRead, &arg_count);
                    return_container = newGenericSpecContainer(arg_count);
                    JITUINT32 count;
                    for (count = 0; count < arg_count; count++) {
                        TypeSpecDescriptor *typeArg;
                        bytesRead += internalGetTypeSpecDescriptorFromTypeBlob(manager, container, binary, signature + bytesRead, JITFALSE, &typeArg);
                        insertTypeSpecDescriptorIntoGenericSpecContainer(return_container, typeArg, count);
                    }
                }
                break;
                default:
                    PDEBUG("METADATA MANAGER: getGenericSpecContainerFromTableAndRow: ERROR! " "INVALID TOKEN! %d\n");
                    abort();
            }
            break;
        }
        default:
            PDEBUG("METADATA MANAGER: getGenericSpecContainerFromTableAndRow: ERROR! " "INVALID TABLE! %d\n", table_id);
            abort();
    }
    return return_container;
}
