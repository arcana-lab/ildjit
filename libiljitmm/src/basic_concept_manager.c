/*
 * Copyright (C) 2009 - 2010 Simone Campanoni, Luca Rocchini
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <compiler_memory_manager.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <decoder.h>
#include <stdlib.h>
#include <assert.h>

// My headers
#include <iljitmm-system.h>
#include <basic_concept_manager.h>
#include <metadata_types_manager.h>
#include <mtm_utils.h>
// End

#define LAYOUT_MASK 0x00000018

inline JITUINT32 skipCustomMod (JITINT8 *signature) {
    JITUINT32 bytesRead = 0;
    JITUINT32 identifier;
    JITUINT32 token;

    bytesRead += uncompressValue(signature + bytesRead, &identifier);
    assert((identifier == ELEMENT_TYPE_CMOD_OPT) || (identifier == ELEMENT_TYPE_CMOD_REQD));
    bytesRead += uncompressValue(signature + bytesRead, &token);
    return bytesRead;
}

inline BasicClass *getBasicClassFromName (metadataManager_t *manager, JITINT8 *type_name_space, JITINT8 *type_name) {
    BasicClass *class = NULL;
    t_binary_information    *current_binary;
    XanListItem             *current_assembly = xanList_first(manager->binaries);

    while (current_assembly != NULL) {
        current_binary = (t_binary_information *) current_assembly->data;
        class = getBasicClassFromNameAndTypeNameSpace(manager, current_binary, type_name_space, type_name);
        if (class != NULL) {
            break;
        }
        current_assembly = current_assembly->next;
    }
    return class;
}

inline BasicClass *getBasicClassFromNameAndTypeNameSpace (metadataManager_t *manager, t_binary_information *binary, JITINT8 *type_name_space, JITINT8 *type_name) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);
    t_string_stream  *string_stream = &(streams->string_stream);

    JITINT8 *correct_type_name_space;
    BasicClass *result = NULL;

    if (type_name_space == NULL) {
        correct_type_name_space = (JITINT8*) "\0";
    } else {
        correct_type_name_space = type_name_space;
    }

    t_type_def_table *table = (t_type_def_table *) get_table(tables, TYPE_DEF_TABLE);
    JITUINT32 count;
    for (count = 1; count <= (table->cardinality); count++) {
        BasicClass *current_row = (BasicClass *) get_row(tables, TYPE_DEF_TABLE, count);
        JITINT8 *current_type_name = get_string(string_stream, current_row->type_name);
        JITINT8 *current_type_name_space = get_string(string_stream, current_row->type_name_space);
        if (STRCMP(current_type_name, type_name) == 0 && STRCMP(current_type_name_space, correct_type_name_space) == 0) {
            result = current_row;
            break;
        }
    }

    return result;
}

inline BasicClass *getBasicClassFromTypeDefOrRef (metadataManager_t *manager, t_binary_information *binary, JITUINT32 type_token) {
    JITUINT8 table_id = (type_token & 0x3);
    JITUINT32 row_id = ((type_token >> 2) & 0x3FFFFFFF );

    return getBasicClassFromTableAndRow(manager, binary, table_id, row_id);
}

inline BasicClass *getBasicClassFromTableAndRow (metadataManager_t *manager, t_binary_information *binary, JITUINT8 table_id, JITUINT32 row_id) {
    t_streams_metadata      *streams = &((binary->metadata).streams_metadata);
    t_metadata_tables *tables = &((streams->not_stream).tables);
    t_string_stream  *string_stream = &(streams->string_stream);
    BasicClass *row = NULL;

    switch (table_id) {
        case 0x0:
            /* TypeDef table	*/
            row = (BasicClass*) get_row(tables, TYPE_DEF_TABLE, row_id);
            assert(row != NULL);
            break;
        case 0x1:
            /* TypeRef table	*/
        {
            t_row_type_ref_table *typeRef = (t_row_type_ref_table *) get_row(tables, TYPE_REF_TABLE, row_id);
            JITUINT32 resolution_scope = (typeRef->resolution_scope & 0x3 );
            JITUINT32 resolved_row_id = ((typeRef->resolution_scope >> 2) & 0x3FFFFFFF);
            JITINT8 *type_name = get_string(string_stream, typeRef->type_name);
            JITINT8 *type_name_space = get_string(string_stream, typeRef->type_name_space);
            switch (resolution_scope) {
                case 0x2:
                    /* A reference to the AssemblyRef table */
                    PDEBUG("METATADATA MANAGER: getBasicClassFromTableAndRow: referencing the AssemblyRef table\n");
                    t_row_assembly_ref_table *assembly_ref = (t_row_assembly_ref_table *) get_row(tables, ASSEMBLY_REF_TABLE, resolved_row_id);
                    t_binary_information *ref_binary = assembly_ref->binary_info;
                    row = getBasicClassFromNameAndTypeNameSpace(manager, ref_binary, type_name_space, type_name);
                    if (row == NULL) {
                        fprintf(stderr, "ILDJIT: Class \"%s.%s\" is not found in %s\n", type_name_space, type_name, ref_binary->name);
                        abort();
                    }
                    assert(row != NULL);
                    break;
                case 0x0:
                    /* A reference to the Module table */
                    PDEBUG("METATADATA MANAGER: getBasicClassFromTableAndRow: referencing the Module table\n");
                    abort();
                case 0x1:
                    /* A reference to the ModuleRef table */
                    PDEBUG("METATADATA MANAGER: getBasicClassFromTableAndRow: referencing the ModuleRef table\n");
                    abort();
                case 0x3:
                    /* A reference to the TypeRef table */
                    PDEBUG("TYPE_CHECKER: getBasicClassFromTableAndRow: referencing the TypeRef table\n");
                    abort();
                default:
                    PDEBUG("METADATA MANAGER: getBasicClassFromTableAndRow: ERROR! " "INVALID RESOLUTION SCOPE! %d\n", resolution_scope);
                    abort();
            }
            break;
        }
        default:
            PDEBUG("METADATA MANAGER: getBasicClassFromTableAndRow: ERROR! " "INVALID TABLE! %d\n", table_id);
            abort();
    }

    return row;
}

inline BasicTypeAttributes *getBasicTypeAttributes (metadataManager_t *manager, BasicClass *class) {
    PDEBUG("getBasicTypeAttributes: Start\n");
    t_binary_information *binary = (t_binary_information *) class->binary;

    BasicTypeAttributes *attributes = sharedAllocFunction(sizeof(BasicTypeAttributes));
    attributes->flags = class->flags;
    attributes->visibility = class->flags & 0x7;
    attributes->layout = (class->flags & LAYOUT_MASK);
    attributes->auto_layout = ((class->flags & LAYOUT_MASK) == AUTOLAYOUT);
    attributes->sequential_layout = ((class->flags & LAYOUT_MASK) == SEQUENTIAL_LAYOUT);
    attributes->explicit_layout = ((class->flags & LAYOUT_MASK) == EXPLICIT_LAYOUT);
    attributes->isAbstract = ((class->flags & 0x80) == 0x80);
    attributes->isInterface = ((class->flags & 0x20) == 0x20);
    attributes->isSealed = ((class->flags & 0x100) == 0x100);
    attributes->isSerializable = ((class->flags & 0x2000) == 0x2000);
    attributes->allowsRelaxedInitialization = ((class->flags & 0x100000) == 0x100000);
    attributes->isNested = (attributes->visibility == NESTED_PUBLIC_TYPE
                            || attributes->visibility == NESTED_PRIVATE_TYPE
                            || attributes->visibility == NESTED_FAMILY_TYPE
                            || attributes->visibility == NESTED_ASSEMBLY_TYPE
                            || attributes->visibility == NESTED_FAM_AND_ASSEM_TYPE
                            || attributes->visibility == NESTED_FAM_OR_ASSEM_TYPE);


    t_row_class_layout_table *class_layout = (t_row_class_layout_table *) getRowByKey(manager, binary, CLASS_LAYOUT, (void *) class->index);
    if (class_layout != NULL) {
        attributes->class_size = class_layout->class_size;
        attributes->packing_size = class_layout->packing_size;
    } else {
        attributes->class_size = 0;
        attributes->packing_size = 0;
    }
    XanList *list = (XanList *) getRowByKey(manager, binary, CLASS_GENERIC_PARAM, (void *) class->index);
    if (list != NULL) {
        attributes->param_count = xanList_length(list);
    } else {
        attributes->param_count = 0;
    }
    PDEBUG("getBasicTypeAttributes: END\n");
    return attributes;
}

inline BasicMethodAttributes *getBasicMethodAttributes (metadataManager_t *manager, BasicMethod *method) {
    t_binary_information *binary = (t_binary_information *) method->binary;

    BasicMethodAttributes *attributes = sharedAllocFunction(sizeof(BasicMethodAttributes));

    attributes->rva = method->RVA;
    attributes->accessMask = (method->flags & 0x7);
    attributes->is_static = ((method->flags & 0x10) == 0x10);
    attributes->is_virtual = ((method->flags & 0x40) == 0x40);
    attributes->is_pinvoke = ((method->flags & 0x2000) == 0x2000);
    attributes->is_special_name = ((method->flags & 0x800) == 0x800);
    attributes->need_new_slot = ((method->flags & 0x100) == 0x100);
    attributes->override = ((method->flags & 0x100) == 0x0);
    attributes->is_abstract = ((method->flags & 0x400) == 0x400);

    attributes->is_internal_call = ((method->impl_flags & 0x1000) != 0x0);
    attributes->is_native = ((method->impl_flags & 0x3) == 0x1);
    attributes->is_cil = ((method->impl_flags & 0x3) == 0x0);
    attributes->is_provided_by_the_runtime = ((method->impl_flags & 0x3) == 0x3);

    t_row_blob_table blob;
    get_blob(&((binary->metadata).streams_metadata.blob_stream), method->signature, &blob);
    JITINT8 *signature = blob.blob;
    JITUINT32 bytesRead = 0;
    attributes->is_vararg = ((signature[bytesRead] & VARARG ) == VARARG);
    attributes->has_this = ((signature[bytesRead] & HASTHIS) == HASTHIS);
    attributes->explicit_this = ((signature[bytesRead] & EXPLICITTHIS) == EXPLICITTHIS);
    if ((signature[0] & VARARG) == VARARG) {
        attributes->calling_convention = VARARG;
    } else if ((signature[0] & CLI_FASTCALL) == CLI_FASTCALL) {
        attributes->calling_convention = CLI_FASTCALL;
    } else if ((signature[0] & C_CALL) == C_CALL) {
        attributes->calling_convention = C_CALL;
    } else if ((signature[0] & CLI_STDCALL) == CLI_STDCALL) {
        attributes->calling_convention = CLI_STDCALL;
    } else if ((signature[0] & THISCALL) == THISCALL) {
        attributes->calling_convention = THISCALL;
    } else if ((signature[0] & GENERIC) == GENERIC) {
        attributes->calling_convention = GENERIC;
    } else {
        attributes->calling_convention = DEFAULT;
    }
    bytesRead++;
    if (attributes->calling_convention == GENERIC) {
        uncompressValue(signature + bytesRead, &(attributes->param_count));
    }

    t_row_method_semantics_table *row = (t_row_method_semantics_table *) getRowByKey(manager, binary, METHOD_SEMANTICS, (void *) method->index);
    if (row != NULL) {
        switch (row->semantics) {
            case 0x1:
                attributes->semantics = SETTER;
                break;
            case 0x2:
                attributes->semantics = GETTER;
                break;
            case 0x4:
                attributes->semantics = OTHER;
                break;
            case 0x8:
                attributes->semantics = ADD_ON;
                break;
            case 0x10:
                attributes->semantics = REMOVE_ON;
                break;
            case 0x20:
                attributes->semantics = FIRE;
                break;
        }
        if (row->association & 0x1) {
            attributes->association = PROPERTY_ASSOCIATION;
        } else {
            attributes->association = EVENT_ASSOCIATION;
        }
    } else {
        attributes->semantics = STANDARD;
        attributes->association = DEFAULT_ASSOCIATION;
    }

    return attributes;
}

inline BasicFieldAttributes *getBasicFieldAttributes (metadataManager_t *manager, BasicField *field) {
    t_binary_information *binary = (t_binary_information *) field->binary;

    BasicFieldAttributes *attributes = sharedAllocFunction(sizeof(BasicFieldAttributes));

    attributes->accessMask = (field->flags & 0x07);
    attributes->is_static = ((field->flags & 0x10) == 0x10);
    attributes->is_init_only = ((field->flags & 0x20) == 0x20);
    attributes->is_literal = ((field->flags & 0x40) == 0x40);
    attributes->is_serializable = ((field->flags & 0x80) == 0x80);
    attributes->is_special = ((field->flags & 0x200) == 0x200);
    attributes->is_pinvoke = ((field->flags & 0x2000) == 0x2000);
    attributes->is_rt_special_name = ((field->flags & 0x400) == 0x400);
    attributes->has_marshal_information = ((field->flags & 0x1000) == 0x1000);
    attributes->has_default = ((field->flags & 0x8000) == 0x8000);
    attributes->has_rva = ((field->flags & 0x100) == 0x100);

    t_row_field_rva_table *rva_row = (t_row_field_rva_table *) getRowByKey(manager, binary, FIELD_RVA, (void *) field->index);
    if (rva_row != NULL) {
        attributes->rva = rva_row->rva;
    } else {
        attributes->rva = 0;
    }

    t_row_field_layout_table *layout_row = (t_row_field_layout_table *) getRowByKey(manager, binary, FIELD_OFFSET, (void *) field->index);
    if (     layout_row != NULL) {
        attributes->has_offset = JITTRUE;
        attributes->offset = layout_row->offset;
    } else {
        attributes->has_offset = JITFALSE;
        attributes->offset = 0;
    }

    return attributes;
}

inline BasicEventAttributes *getBasicEventAttributes (metadataManager_t *manager, BasicEvent *event) {
    BasicEventAttributes *attributes = sharedAllocFunction(sizeof(BasicEventAttributes));

    attributes->is_special_name = ((event->event_flags & 0x200) == 0x200);
    return attributes;
}

inline BasicPropertyAttributes *getBasicPropertyAttributes (metadataManager_t *manager, BasicProperty *property) {
    BasicPropertyAttributes *attributes = sharedAllocFunction(sizeof(BasicPropertyAttributes));

    attributes->is_special_name = ((property->flags & 0x200) == 0x200);
    attributes->hasDefault = ((property->flags & 0x1000) == 0x1000);
    return attributes;
}

inline BasicParamAttributes *getBasicParamAttributes (metadataManager_t *manager, BasicParam *param) {
    BasicParamAttributes *attributes = sharedAllocFunction(sizeof(BasicParamAttributes));

    attributes->is_in = ((param->flags & 0x1) == 0x1);
    attributes->is_out = ((param->flags & 0x2) == 0x2);
    attributes->is_optional = ((param->flags & 0x10) == 0x10);
    attributes->has_default = ((param->flags & 0x1000) == 0x1000);
    attributes->has_field_marshal = ((param->flags & 0x2000) == 0x2000);
    return attributes;
}

inline BasicGenericParamAttributes *getBasicGenericParamAttributes (metadataManager_t *manager, BasicGenericParam *param) {
    BasicGenericParamAttributes *attributes = sharedAllocFunction(sizeof(BasicGenericParamAttributes));

    attributes->isCovariant = ((param->flags & 0x1) == 0x1);
    attributes->isContravariant = ((param->flags & 0x2) == 0x2);
    attributes->isReference = ((param->flags & 0x4) == 0x4);
    attributes->isValueType = ((param->flags & 0x8) == 0x8);
    attributes->hasDefaultConstructor = ((param->flags & 0x10) == 0x10);
    attributes->number = param->number;
    return attributes;
}
