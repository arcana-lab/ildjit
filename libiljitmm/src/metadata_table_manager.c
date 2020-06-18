/*
 * Copyright (C) 2006  Campanoni Simone
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

/**
   *@file metadata_table_manager.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// My headers
#include <jitsystem.h>
#include <metadata/tables/metadata_table_manager.h>
#include <section_manager.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End

JITBOOLEAN read_tables_cardinality (t_metadata_tables *tables, JITUINT64 valid, JITINT8 *row_buf);

JITINT32 load_metadata_tables (t_metadata_tables *tables, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *tables_buf) {
    JITUINT32 bytes_read;
    JITUINT32 row_offset;                   // offset in bytes
    JITUINT32 tables_offset;                // offset in bytes

    bytes_read = tables_offset = row_offset = 0;
    private_get_table(tables, 0);
    PDEBUG("METADATA TABLE MANAGER: begin\n");

    /* Read the cardinality of all tables	*/
    if (read_tables_cardinality(tables, valid, row_buf) != 0) {
        return -1;
    }

    // Module table
    if (load_module_table(&(tables->module_table), binary, valid, heap_size, row_buf, tables_buf, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Type ref table
    if (load_type_ref_table(&(tables->type_ref_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Type def table
    if (load_type_def_table(&(tables->type_def_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Field table
    if (load_field_table(&(tables->field_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Method def table
    if (load_method_def_table(&(tables->method_def_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Param table
    if (load_param_table(&(tables->param_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Interface Impl table
    if (load_interface_impl_table(&(tables->interface_impl_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Member ref table
    if (load_member_ref_table(&(tables->member_ref_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Constant table
    if (load_constant_table(&(tables->constant_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Custom Attribute table
    if (load_custom_attribute_table(&(tables->custom_attribute_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Field marshal table
    if (load_field_marshal_table(&(tables->field_marshal_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Decl security table
    if (load_decl_security_table(&(tables->decl_security_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Class layout table
    if (load_class_layout_table(&(tables->class_layout_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Field layout table
    if (load_field_layout_table(&(tables->field_layout_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Standalone sig table
    if (load_standalone_sig_table(&(tables->standalone_sig_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Event map table
    if (load_event_map_table(&(tables->event_map_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Event table
    if (load_event_table(&(tables->event_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Property map table
    if (load_property_map_table(&(tables->property_map_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Property table
    if (load_property_table(&(tables->property_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Method semantics table
    if (load_method_semantics_table(&(tables->method_semantics_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Method impl table
    if (load_method_impl_table(&(tables->method_impl_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Module ref table
    if (load_module_ref_table(&(tables->module_ref_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Type spec table
    if (load_type_spec_table(&(tables->type_spec_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Impl map table
    if (load_impl_map_table(&(tables->impl_map_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Field RVA table
    if (load_field_rva_table(&(tables->field_rva_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Assembly table
    if (load_assembly_table(&(tables->assembly_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Assembly ref table
    if (load_assembly_ref_table(&(tables->assembly_ref_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // File table
    if (load_file_table(&(tables->file_table), binary, valid,  heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Exported type table
    if (load_exported_type_table(&(tables->exported_type_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Manifest resource table
    if (load_manifest_resource_table(&(tables->manifest_resource_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Nested class table
    if (load_nested_class_table(&(tables->nested_class_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Generic param table
    if (load_generic_param_table(&(tables->generic_param_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Method spec table
    if (load_method_spec_table(&(tables->method_spec_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    // Generic param constraint table
    if (load_generic_param_constraint_table(&(tables->generic_param_constraint_table), binary, valid, heap_size, row_buf + row_offset, tables_buf + tables_offset, &bytes_read) != 0) {
        return -1;
    }
    if (bytes_read != 0) {
        tables_offset += bytes_read;
        row_offset += 4;
        bytes_read = 0;
    }

    PDEBUG("METADATA TABLE MANAGER: end\n");
    return 0;
}

void shutdown_tables_manager (t_metadata_tables *tables) {

    /* Assertions			*/
    assert(tables != NULL);

    PDEBUG("METADATA TABLE MANAGER: Shutting down\n");

    /* Assembly table		*/
    shutdown_assembly_table(&(tables->assembly_table));

    /* Assembly ref table		*/
    shutdown_assembly_ref_table(&(tables->assembly_ref_table));

    /* Class layout table		*/
    shutdown_class_layout_table(&(tables->class_layout_table));

    /* Constant table		*/
    shutdown_constant_table(&(tables->constant_table));

    /* Custom attribute table	*/
    shutdown_custom_attribute_table(&(tables->custom_attribute_table));

    /* Decl security table		*/
    shutdown_decl_security_table(&(tables->decl_security_table));

    /* Event table			*/
    shutdown_event_table(&(tables->event_table));

    /* Event map table		*/
    shutdown_event_map_table(&(tables->event_map_table));

    /* Exported type table		*/
    shutdown_exported_type_table(&(tables->exported_type_table));

    /* Field table			*/
    shutdown_field_table(&(tables->field_table));

    /* Field layout table		*/
    shutdown_field_layout_table(&(tables->field_layout_table));

    /* Field marshal table		*/
    shutdown_field_marshal_table(&(tables->field_marshal_table));

    /* Field RVA table		*/
    shutdown_field_rva_table(&(tables->field_rva_table));

    /* File table			*/
    shutdown_file_table(&(tables->file_table));

    /* Impl map table		*/
    shutdown_impl_map_table(&(tables->impl_map_table));

    /* Interface impl table		*/
    shutdown_interface_impl_table(&(tables->interface_impl_table));

    /* Manifest resource table	*/
    shutdown_manifest_resource_table(&(tables->manifest_resource_table));

    /* Member ref table		*/
    shutdown_member_ref_table(&(tables->member_ref_table));

    /* Method def table		*/
    shutdown_method_def_table(&(tables->method_def_table));

    /* Method impl table		*/
    shutdown_method_impl_table(&(tables->method_impl_table));

    /* Method semantics table	*/
    shutdown_method_semantics_table(&(tables->method_semantics_table));

    /* Module table			*/
    shutdown_module_table(&(tables->module_table));

    /* Module table			*/
    shutdown_module_ref_table(&(tables->module_ref_table));

    /* Nested class table		*/
    shutdown_nested_class_table(&(tables->nested_class_table));

    /* Param table			*/
    shutdown_param_table(&(tables->param_table));

    /* Property table		*/
    shutdown_property_table(&(tables->property_table));

    /* Property map table		*/
    shutdown_property_map_table(&(tables->property_map_table));

    /* Standalone sig table		*/
    shutdown_standalone_sig_table(&(tables->standalone_sig_table));

    /* Type def table		*/
    shutdown_type_def_table(&(tables->type_def_table));

    /* Type ref table		*/
    shutdown_type_ref_table(&(tables->type_ref_table));

    /* Type spec table		*/
    shutdown_type_spec_table(&(tables->type_spec_table));

    /* Generic param table		*/
    shutdown_generic_param_table(&(tables->generic_param_table));

    /* Generic param constraint table		*/
    shutdown_generic_param_constraint_table(&(tables->generic_param_constraint_table));

    /* Method spec table		*/
    shutdown_method_spec_table(&(tables->method_spec_table));

    PDEBUG("METADATA TABLE MANAGER: Done\n");
}

void * get_table (t_metadata_tables *tables, JITUINT16 table_ID) {
    char buffer[1024];

    /* Assertions	*/
    assert(tables != NULL);

    switch (table_ID) {
        case ASSEMBLY_TABLE:
            if ((tables->assembly_table).table != NULL) {
                return &(tables->assembly_table);
            }
            return NULL;
        case ASSEMBLY_REF_TABLE:
            if ((tables->assembly_ref_table).table != NULL) {
                return &(tables->assembly_ref_table);
            }
            return NULL;
        case CLASS_LAYOUT_TABLE:
            if ((tables->class_layout_table).table != NULL) {
                return &(tables->class_layout_table);
            }
            return NULL;
        case CONSTANT_TABLE:
            if ((tables->constant_table).table != NULL) {
                return &(tables->constant_table);
            }
            return NULL;
        case CUSTOM_ATTRIBUTE_TABLE:
            if ((tables->custom_attribute_table).table != NULL) {
                return &(tables->custom_attribute_table);
            }
            return NULL;
        case DECL_SECURITY_TABLE:
            if ((tables->decl_security_table).table != NULL) {
                return &(tables->decl_security_table);
            }
            return NULL;
        case EVENT_MAP_TABLE:
            if ((tables->event_map_table).table != NULL) {
                return &(tables->event_map_table);
            }
            return NULL;
        case EVENT_TABLE:
            if ((tables->event_table).table != NULL) {
                return &(tables->event_table);
            }
            return NULL;
        case EXPORTED_TYPE_TABLE:
            if ((tables->exported_type_table).table != NULL) {
                return &(tables->exported_type_table);
            }
            return NULL;
        case FIELD_TABLE:
            if ((tables->field_table).table != NULL) {
                return &(tables->field_table);
            }
            return NULL;
        case FIELD_LAYOUT_TABLE:
            if ((tables->field_layout_table).table != NULL) {
                return &(tables->field_layout_table);
            }
            return NULL;
        case FIELD_MARSHAL_TABLE:
            if ((tables->field_marshal_table).table != NULL) {
                return &(tables->field_marshal_table);
            }
            return NULL;
        case FIELD_RVA_TABLE:
            if ((tables->field_rva_table).table != NULL) {
                return &(tables->field_rva_table);
            }
            return NULL;
        case FILE_TABLE:
            if ((tables->file_table).table != NULL) {
                return &(tables->file_table);
            }
            return NULL;
        case IMPL_MAP_TABLE:
            if ((tables->impl_map_table).table != NULL) {
                return &(tables->impl_map_table);
            }
            return NULL;
        case INTERFACE_IMPL_TABLE:
            if ((tables->interface_impl_table).table != NULL) {
                return &(tables->interface_impl_table);
            }
            return NULL;
        case MANIFEST_RESOURCE_TABLE:
            if ((tables->manifest_resource_table).table != NULL) {
                return &(tables->manifest_resource_table);
            }
            return NULL;
        case METHOD_DEF_TABLE:
            if ((tables->method_def_table).table != NULL) {
                return &(tables->method_def_table);
            }
            return NULL;
        case METHOD_IMPL_TABLE:
            if ((tables->method_impl_table).table != NULL) {
                return &(tables->method_impl_table);
            }
            return NULL;
        case METHOD_SEMANTICS_TABLE:
            if ((tables->method_semantics_table).table != NULL) {
                return &(tables->method_semantics_table);
            }
            return NULL;
        case MEMBER_REF_TABLE:
            if ((tables->member_ref_table).table != NULL) {
                return &(tables->member_ref_table);
            }
            return NULL;
        case MODULE_TABLE:
            if ((tables->module_table).table != NULL) {
                return &(tables->module_table);
            }
            return NULL;
        case MODULE_REF_TABLE:
            if ((tables->module_ref_table).table != NULL) {
                return &(tables->module_ref_table);
            }
            return NULL;
        case NESTED_CLASS_TABLE:
            if ((tables->nested_class_table).table != NULL) {
                return &(tables->nested_class_table);
            }
            return NULL;
        case PARAM_TABLE:
            if ((tables->param_table).table != NULL) {
                return &(tables->param_table);
            }
            return NULL;
        case PROPERTY_TABLE:
            if ((tables->property_table).table != NULL) {
                return &(tables->property_table);
            }
            return NULL;
        case PROPERTY_MAP_TABLE:
            if ((tables->property_map_table).table != NULL) {
                return &(tables->property_map_table);
            }
            return NULL;
        case STANDALONE_SIG_TABLE:
            if ((tables->standalone_sig_table).table != NULL) {
                return &(tables->standalone_sig_table);
            }
            return NULL;
        case TYPE_DEF_TABLE:
            if ((tables->type_def_table).table != NULL) {
                return &(tables->type_def_table);
            }
            return NULL;
        case TYPE_REF_TABLE:
            if ((tables->type_ref_table).table != NULL) {
                return &(tables->type_ref_table);
            }
            return NULL;
        case TYPE_SPEC_TABLE:
            if ((tables->type_spec_table).table != NULL) {
                return &(tables->type_spec_table);
            }
            return NULL;
        case GENERIC_PARAM_TABLE:
            if ((tables->generic_param_table).table != NULL) {
                return &(tables->generic_param_table);
            }
            return NULL;
        case GENERIC_PARAM_CONSTRAINT_TABLE:
            if ((tables->generic_param_constraint_table).table != NULL) {
                return &(tables->generic_param_constraint_table);
            }
            return NULL;
        case METHOD_SPEC_TABLE:
            if ((tables->method_spec_table).table != NULL) {
                return &(tables->method_spec_table);
            }
            return NULL;
        default:
            snprintf(buffer, sizeof(buffer), "METADATA_TABLE_MANAGER: get_table: ERROR= Table 0x%X not know. ", table_ID);
            print_err(buffer, 0);
            abort();
    }
    return NULL;
}
void * get_first_row (t_metadata_tables *tables, JITUINT16 table_ID) {

    assert(tables != NULL);
    return get_row(tables, table_ID, 1);
}

void * get_row (t_metadata_tables *tables, JITUINT16 table_ID, JITUINT32 row_ID) {
    t_method_def_table              *method_def_table;
    t_method_impl_table             *method_impl_table;
    t_method_semantics_table        *method_semantics_table;
    t_module_table                  *module_table;
    t_module_ref_table              *module_ref_table;
    t_property_table                *property_table;
    t_property_map_table            *property_map_table;
    t_nested_class_table            *nested_class_table;
    t_member_ref_table              *member_ref_table;
    t_class_layout_table            *class_layout_table;
    t_constant_table                *constant_table;
    t_decl_security_table           *decl_security_table;
    t_custom_attribute_table        *custom_attribute_table;
    t_event_map_table               *event_map_table;
    t_event_table                   *event_table;
    t_exported_type_table           *exported_type_table;
    t_type_def_table                *type_def_table;
    t_type_ref_table                *type_ref_table;
    t_type_spec_table               *type_spec_table;
    t_assembly_ref_table            *assembly_ref_table;
    t_assembly_table                *assembly_table;
    t_param_table                   *param_table;
    t_standalone_sig_table          *standalone_sig_table;
    t_interface_impl_table          *interface_impl_table;
    t_manifest_resource_table       *manifest_resource_table;
    t_field_table                   *field_table;
    t_field_layout_table            *field_layout_table;
    t_field_marshal_table           *field_marshal_table;
    t_field_rva_table               *field_rva_table;
    t_file_table                    *file_table;
    t_impl_map_table                *impl_map_table;
    t_generic_param_table   *generic_param_table;
    t_generic_param_constraint_table        *generic_param_constraint_table;
    t_method_spec_table             *method_spec_table;
    char buffer[1024];

    /* Assertions			*/
    assert(tables != NULL);

    /* Initialize the variables	*/
    method_def_table = NULL;
    method_impl_table = NULL;
    method_semantics_table = NULL;
    module_table = NULL;
    module_ref_table = NULL;
    property_table = NULL;
    property_map_table = NULL;
    nested_class_table = NULL;
    member_ref_table = NULL;
    class_layout_table = NULL;
    constant_table = NULL;
    decl_security_table = NULL;
    custom_attribute_table = NULL;
    event_map_table = NULL;
    event_table = NULL;
    exported_type_table = NULL;
    type_def_table = NULL;
    type_ref_table = NULL;
    type_spec_table = NULL;
    assembly_ref_table = NULL;
    assembly_table = NULL;
    param_table = NULL;
    standalone_sig_table = NULL;
    interface_impl_table = NULL;
    manifest_resource_table = NULL;
    field_table = NULL;
    field_layout_table = NULL;
    field_marshal_table = NULL;
    field_rva_table = NULL;
    file_table = NULL;
    impl_map_table = NULL;
    generic_param_table = NULL;
    generic_param_constraint_table = NULL;
    method_spec_table = NULL;

    /* Check if the index is valid	*/
    if (row_ID == 0) {
        return NULL;
    }

    switch (table_ID) {
        case ASSEMBLY_TABLE:
            assembly_table = (t_assembly_table *) get_table(tables, ASSEMBLY_TABLE);
            if (assembly_table == NULL) {
                return NULL;
            }
            assert(assembly_table != NULL);
            if (assembly_table->table == NULL) {
                return NULL;
            }
            if (row_ID > assembly_table->cardinality) {
                return NULL;
            }
            assert(assembly_table->table[row_ID - 1] != NULL);
            return assembly_table->table[row_ID - 1];
        case ASSEMBLY_REF_TABLE:
            assembly_ref_table = (t_assembly_ref_table *) get_table(tables, ASSEMBLY_REF_TABLE);
            if (assembly_ref_table == NULL) {
                return NULL;
            }
            assert(assembly_ref_table != NULL);
            if (assembly_ref_table->table == NULL) {
                return NULL;
            }
            if (row_ID > assembly_ref_table->cardinality) {
                return NULL;
            }
            return assembly_ref_table->table[row_ID - 1];
        case CLASS_LAYOUT_TABLE:
            class_layout_table = (t_class_layout_table *) get_table(tables, CLASS_LAYOUT_TABLE);
            if (class_layout_table == NULL) {
                return NULL;
            }
            assert(class_layout_table != NULL);
            if (class_layout_table->table == NULL) {
                return NULL;
            }
            if (row_ID > class_layout_table->cardinality) {
                return NULL;
            }
            return class_layout_table->table[row_ID - 1];
        case CONSTANT_TABLE:
            constant_table = (t_constant_table *) get_table(tables, CONSTANT_TABLE);
            if (constant_table == NULL) {
                return NULL;
            }
            assert(constant_table != NULL);
            if (constant_table->table == NULL) {
                return NULL;
            }
            if (row_ID > constant_table->cardinality) {
                return NULL;
            }
            return constant_table->table[row_ID - 1];
        case CUSTOM_ATTRIBUTE_TABLE:
            custom_attribute_table = (t_custom_attribute_table *) get_table(tables, CUSTOM_ATTRIBUTE_TABLE);
            if (custom_attribute_table == NULL) {
                return NULL;
            }
            assert(custom_attribute_table != NULL);
            if (custom_attribute_table->table == NULL) {
                return NULL;
            }
            if (row_ID > custom_attribute_table->cardinality) {
                return NULL;
            }
            return custom_attribute_table->table[row_ID - 1];
        case DECL_SECURITY_TABLE:
            decl_security_table = (t_decl_security_table *) get_table(tables, DECL_SECURITY_TABLE);
            if (decl_security_table == NULL) {
                return NULL;
            }
            assert(decl_security_table != NULL);
            if (decl_security_table->table == NULL) {
                return NULL;
            }
            if (row_ID > decl_security_table->cardinality) {
                return NULL;
            }
            return decl_security_table->table[row_ID - 1];
        case EVENT_MAP_TABLE:
            event_map_table = (t_event_map_table *) get_table(tables, EVENT_MAP_TABLE);
            if (event_map_table == NULL) {
                return NULL;
            }
            assert(event_map_table != NULL);
            if (event_map_table->table == NULL) {
                return NULL;
            }
            if (row_ID > event_map_table->cardinality) {
                return NULL;
            }
            return event_map_table->table[row_ID - 1];
        case EVENT_TABLE:
            event_table = (t_event_table *) get_table(tables, EVENT_TABLE);
            if (event_table == NULL) {
                return NULL;
            }
            assert(event_table != NULL);
            if (event_table->table == NULL) {
                return NULL;
            }
            if (row_ID > event_table->cardinality) {
                return NULL;
            }
            return event_table->table[row_ID - 1];
        case EXPORTED_TYPE_TABLE:
            exported_type_table = (t_exported_type_table *) get_table(tables, EXPORTED_TYPE_TABLE);
            if (exported_type_table == NULL) {
                return NULL;
            }
            assert(exported_type_table != NULL);
            if (exported_type_table->table == NULL) {
                return NULL;
            }
            if (row_ID > exported_type_table->cardinality) {
                return NULL;
            }
            return exported_type_table->table[row_ID - 1];
        case FIELD_TABLE:
            field_table = (t_field_table *) get_table(tables, FIELD_TABLE);
            if (field_table == NULL) {
                return NULL;
            }
            assert(field_table != NULL);
            if (field_table->table == NULL) {
                return NULL;
            }
            if (row_ID > field_table->cardinality) {
                return NULL;
            }
            assert(field_table->table[row_ID - 1] != NULL);
            return field_table->table[row_ID - 1];
        case FIELD_LAYOUT_TABLE:
            field_layout_table = (t_field_layout_table *) get_table(tables, FIELD_LAYOUT_TABLE);
            if (field_layout_table == NULL) {
                return NULL;
            }
            assert(field_layout_table != NULL);
            if (field_layout_table->table == NULL) {
                return NULL;
            }
            if (row_ID > field_layout_table->cardinality) {
                return NULL;
            }
            assert(field_layout_table->table[row_ID - 1] != NULL);
            return field_layout_table->table[row_ID - 1];
        case FIELD_MARSHAL_TABLE:
            field_marshal_table = (t_field_marshal_table *) get_table(tables, FIELD_MARSHAL_TABLE);
            if (field_marshal_table == NULL) {
                return NULL;
            }
            assert(field_marshal_table != NULL);
            if (field_marshal_table->table == NULL) {
                return NULL;
            }
            if (row_ID > field_marshal_table->cardinality) {
                return NULL;
            }
            assert(field_marshal_table->table[row_ID - 1] != NULL);
            return field_marshal_table->table[row_ID - 1];
        case FIELD_RVA_TABLE:
            field_rva_table = (t_field_rva_table *) get_table(tables, FIELD_RVA_TABLE);
            if (field_rva_table == NULL) {
                return NULL;
            }
            assert(field_rva_table != NULL);
            if (field_rva_table->table == NULL) {
                return NULL;
            }
            if (row_ID > field_rva_table->cardinality) {
                return NULL;
            }
            assert(field_rva_table->table[row_ID - 1] != NULL);
            return field_rva_table->table[row_ID - 1];
        case FILE_TABLE:
            file_table = (t_file_table *) get_table(tables, FILE_TABLE);
            if (file_table == NULL) {
                return NULL;
            }
            assert(file_table != NULL);
            if (file_table->table == NULL) {
                return NULL;
            }
            if (row_ID > file_table->cardinality) {
                return NULL;
            }
            assert(file_table->table[row_ID - 1] != NULL);
            return file_table->table[row_ID - 1];
        case IMPL_MAP_TABLE:
            impl_map_table = (t_impl_map_table *) get_table(tables, IMPL_MAP_TABLE);
            if (impl_map_table == NULL) {
                return NULL;
            }
            assert(impl_map_table != NULL);
            if (impl_map_table->table == NULL) {
                return NULL;
            }
            if (row_ID > impl_map_table->cardinality) {
                return NULL;
            }
            assert(impl_map_table->table[row_ID - 1] != NULL);
            return impl_map_table->table[row_ID - 1];
        case INTERFACE_IMPL_TABLE:
            interface_impl_table = (t_interface_impl_table *) get_table(tables, INTERFACE_IMPL_TABLE);
            if (interface_impl_table == NULL) {
                return NULL;
            }
            assert(interface_impl_table != NULL);
            if (interface_impl_table->table == NULL) {
                return NULL;
            }
            if (row_ID > interface_impl_table->cardinality) {
                return NULL;
            }
            assert(interface_impl_table->table[row_ID - 1] != NULL);
            return interface_impl_table->table[row_ID - 1];
        case MANIFEST_RESOURCE_TABLE:
            manifest_resource_table = (t_manifest_resource_table *) get_table(tables, MANIFEST_RESOURCE_TABLE);
            if (manifest_resource_table == NULL) {
                return NULL;
            }
            assert(manifest_resource_table != NULL);
            if (manifest_resource_table->table == NULL) {
                return NULL;
            }
            if (row_ID > manifest_resource_table->cardinality) {
                return NULL;
            }
            assert(manifest_resource_table->table[row_ID - 1] != NULL);
            return manifest_resource_table->table[row_ID - 1];
        case MEMBER_REF_TABLE:
            member_ref_table = (t_member_ref_table *) get_table(tables, MEMBER_REF_TABLE);
            if (member_ref_table == NULL) {
                return NULL;
            }
            assert(member_ref_table != NULL);
            if (member_ref_table->table == NULL) {
                return NULL;
            }
            if (row_ID > member_ref_table->cardinality) {
                return NULL;
            }
            assert(member_ref_table->table[row_ID - 1] != NULL);
            return member_ref_table->table[row_ID - 1];
        case METHOD_DEF_TABLE:
            method_def_table = (t_method_def_table *) get_table(tables, METHOD_DEF_TABLE);
            if (method_def_table == NULL) {
                return NULL;
            }
            assert(method_def_table != NULL);
            if (method_def_table->table == NULL) {
                return NULL;
            }
            if (row_ID > method_def_table->cardinality) {
                return NULL;
            }
            return &(method_def_table->table[row_ID - 1]);
        case METHOD_IMPL_TABLE:
            method_impl_table = (t_method_impl_table *) get_table(tables, METHOD_IMPL_TABLE);
            if (method_impl_table == NULL) {
                return NULL;
            }
            assert(method_impl_table != NULL);
            if (method_impl_table->table == NULL) {
                return NULL;
            }
            if (row_ID > method_impl_table->cardinality) {
                return NULL;
            }
            assert(method_impl_table->table[row_ID - 1] != NULL);
            return method_impl_table->table[row_ID - 1];
        case METHOD_SEMANTICS_TABLE:
            method_semantics_table = (t_method_semantics_table *) get_table(tables, METHOD_SEMANTICS_TABLE);
            if (method_semantics_table == NULL) {
                return NULL;
            }
            assert(method_semantics_table != NULL);
            if (method_semantics_table->table == NULL) {
                return NULL;
            }
            if (row_ID > method_semantics_table->cardinality) {
                return NULL;
            }
            assert(method_semantics_table->table[row_ID - 1] != NULL);
            return method_semantics_table->table[row_ID - 1];
        case MODULE_TABLE:
            module_table = (t_module_table *) get_table(tables, MODULE_TABLE);
            if (module_table == NULL) {
                return NULL;
            }
            assert(module_table != NULL);
            if (module_table->table == NULL) {
                return NULL;
            }
            if (row_ID > module_table->cardinality) {
                return NULL;
            }
            assert(module_table->table[row_ID - 1] != NULL);
            return module_table->table[row_ID - 1];
        case MODULE_REF_TABLE:
            module_ref_table = (t_module_ref_table *) get_table(tables, MODULE_REF_TABLE);
            if (module_ref_table == NULL) {
                return NULL;
            }
            assert(module_ref_table != NULL);
            if (module_ref_table->table == NULL) {
                return NULL;
            }
            if (row_ID > module_ref_table->cardinality) {
                return NULL;
            }
            assert(module_ref_table->table[row_ID - 1] != NULL);
            return module_ref_table->table[row_ID - 1];
        case NESTED_CLASS_TABLE:
            nested_class_table = (t_nested_class_table *) get_table(tables, NESTED_CLASS_TABLE);
            if (nested_class_table == NULL) {
                return NULL;
            }
            assert(nested_class_table != NULL);
            if (nested_class_table->table == NULL) {
                return NULL;
            }
            if (row_ID > nested_class_table->cardinality) {
                return NULL;
            }
            assert(nested_class_table->table[row_ID - 1] != NULL);
            return nested_class_table->table[row_ID - 1];
        case PARAM_TABLE:
            param_table = (t_param_table *) get_table(tables, PARAM_TABLE);
            if (param_table == NULL) {
                return NULL;
            }
            assert(param_table != NULL);
            if (param_table->table == NULL) {
                return NULL;
            }
            if (row_ID > param_table->cardinality) {
                return NULL;
            }
            return &(param_table->table[row_ID - 1]);
        case PROPERTY_TABLE:
            property_table = (t_property_table *) get_table(tables, PROPERTY_TABLE);
            if (property_table == NULL) {
                return NULL;
            }
            assert(property_table != NULL);
            if (property_table->table == NULL) {
                return NULL;
            }
            if (row_ID > property_table->cardinality) {
                return NULL;
            }
            assert(property_table->table[row_ID - 1] != NULL);
            return property_table->table[row_ID - 1];
        case PROPERTY_MAP_TABLE:
            property_map_table = (t_property_map_table *) get_table(tables, PROPERTY_MAP_TABLE);
            if (property_map_table == NULL) {
                return NULL;
            }
            assert(property_map_table != NULL);
            if (property_map_table->table == NULL) {
                return NULL;
            }
            if (row_ID > property_map_table->cardinality) {
                return NULL;
            }
            assert(property_map_table->table[row_ID - 1] != NULL);
            return property_map_table->table[row_ID - 1];
        case STANDALONE_SIG_TABLE:
            standalone_sig_table = (t_standalone_sig_table *) get_table(tables, STANDALONE_SIG_TABLE);
            if (standalone_sig_table == NULL) {
                return NULL;
            }
            assert(standalone_sig_table != NULL);
            if (standalone_sig_table->table == NULL) {
                return NULL;
            }
            if (row_ID > standalone_sig_table->cardinality) {
                return NULL;
            }
            assert(standalone_sig_table->table[row_ID - 1] != NULL);
            return standalone_sig_table->table[row_ID - 1];
        case TYPE_DEF_TABLE:
            type_def_table = (t_type_def_table *) get_table(tables, TYPE_DEF_TABLE);
            if (type_def_table == NULL) {
                return NULL;
            }
            assert(type_def_table != NULL);
            if (type_def_table->table == NULL) {
                return NULL;
            }
            if (row_ID > type_def_table->cardinality) {
                return NULL;
            }
            assert(type_def_table->table[row_ID - 1] != NULL);
            return type_def_table->table[row_ID - 1];
        case TYPE_REF_TABLE:
            type_ref_table = (t_type_ref_table *) get_table(tables, TYPE_REF_TABLE);
            if (type_ref_table == NULL) {
                return NULL;
            }
            assert(type_ref_table != NULL);
            if (type_ref_table->table == NULL) {
                return NULL;
            }
            if (row_ID > type_ref_table->cardinality) {
                return NULL;
            }
            assert(type_ref_table->table[row_ID - 1] != NULL);
            return type_ref_table->table[row_ID - 1];
        case TYPE_SPEC_TABLE:
            type_spec_table = (t_type_spec_table *) get_table(tables, TYPE_SPEC_TABLE);
            if (type_spec_table == NULL) {
                return NULL;
            }
            assert(type_spec_table != NULL);
            if (type_spec_table->table == NULL) {
                return NULL;
            }
            if (row_ID > type_spec_table->cardinality) {
                return NULL;
            }
            assert(type_spec_table->table[row_ID - 1] != NULL);
            return type_spec_table->table[row_ID - 1];
        case GENERIC_PARAM_TABLE:
            generic_param_table = (t_generic_param_table *) get_table(tables, GENERIC_PARAM_TABLE);
            if (generic_param_table == NULL) {
                return NULL;
            }
            assert(generic_param_table != NULL);
            if (generic_param_table->table == NULL) {
                return NULL;
            }
            if (row_ID > generic_param_table->cardinality) {
                return NULL;
            }
            assert(generic_param_table->table[row_ID - 1] != NULL);
            return generic_param_table->table[row_ID - 1];
        case GENERIC_PARAM_CONSTRAINT_TABLE:
            generic_param_constraint_table = (t_generic_param_constraint_table *) get_table(tables, GENERIC_PARAM_CONSTRAINT_TABLE);
            if (generic_param_constraint_table == NULL) {
                return NULL;
            }
            assert(generic_param_constraint_table != NULL);
            if (generic_param_constraint_table->table == NULL) {
                return NULL;
            }
            if (row_ID > generic_param_constraint_table->cardinality) {
                return NULL;
            }
            assert(generic_param_constraint_table->table[row_ID - 1] != NULL);
            return generic_param_constraint_table->table[row_ID - 1];
        case METHOD_SPEC_TABLE:
            method_spec_table = (t_method_spec_table *) get_table(tables, METHOD_SPEC_TABLE);
            if (method_spec_table == NULL) {
                return NULL;
            }
            assert(method_spec_table != NULL);
            if (method_spec_table->table == NULL) {
                return NULL;
            }
            if (row_ID > method_spec_table->cardinality) {
                return NULL;
            }
            assert(method_spec_table->table[row_ID - 1] != NULL);
            return method_spec_table->table[row_ID - 1];
        default:
            snprintf(buffer, sizeof(buffer), "METADATA_TABLE_MANAGER: get_row: ERROR= Table 0x%X not know. ", table_ID);
            print_err(buffer, 0);
            abort();
    }
    return NULL;
}

JITUINT32 get_table_cardinality (t_metadata_tables *tables, JITUINT16 table_ID) {
    char buffer[1024];

    /* Assertions	*/
    assert(tables != NULL);

    switch (table_ID) {
        case ASSEMBLY_TABLE:
            if ((tables->assembly_table).table != NULL) {
                return tables->assembly_table.cardinality;
            }
            return 0;
        case ASSEMBLY_REF_TABLE:
            if ((tables->assembly_ref_table).table != NULL) {
                return tables->assembly_ref_table.cardinality;
            }
            return 0;
        case CLASS_LAYOUT_TABLE:
            if ((tables->class_layout_table).table != NULL) {
                return tables->class_layout_table.cardinality;
            }
            return 0;
        case CONSTANT_TABLE:
            if ((tables->constant_table).table != NULL) {
                return tables->constant_table.cardinality;
            }
            return 0;
        case CUSTOM_ATTRIBUTE_TABLE:
            if ((tables->custom_attribute_table).table != NULL) {
                return tables->custom_attribute_table.cardinality;
            }
            return 0;
        case DECL_SECURITY_TABLE:
            if ((tables->decl_security_table).table != NULL) {
                return tables->decl_security_table.cardinality;
            }
            return 0;
        case EVENT_MAP_TABLE:
            if ((tables->event_map_table).table != NULL) {
                return tables->event_map_table.cardinality;
            }
            return 0;
        case EVENT_TABLE:
            if ((tables->event_table).table != NULL) {
                return tables->event_table.cardinality;;
            }
            return 0;
        case EXPORTED_TYPE_TABLE:
            if ((tables->exported_type_table).table != NULL) {
                return tables->exported_type_table.cardinality;
            }
            return 0;
        case FIELD_TABLE:
            if ((tables->field_table).table != NULL) {
                return tables->field_table.cardinality;
            }
            return 0;
        case FIELD_LAYOUT_TABLE:
            if ((tables->field_layout_table).table != NULL) {
                return tables->field_layout_table.cardinality;
            }
            return 0;
        case FIELD_MARSHAL_TABLE:
            if ((tables->field_marshal_table).table != NULL) {
                return tables->field_marshal_table.cardinality;
            }
            return 0;
        case FIELD_RVA_TABLE:
            if ((tables->field_rva_table).table != NULL) {
                return tables->field_rva_table.cardinality;
            }
            return 0;
        case FILE_TABLE:
            if ((tables->file_table).table != NULL) {
                return tables->file_table.cardinality;
            }
            return 0;
        case IMPL_MAP_TABLE:
            if ((tables->impl_map_table).table != NULL) {
                return tables->impl_map_table.cardinality;
            }
            return 0;
        case INTERFACE_IMPL_TABLE:
            if ((tables->interface_impl_table).table != NULL) {
                return tables->interface_impl_table.cardinality;
            }
            return 0;
        case MANIFEST_RESOURCE_TABLE:
            if ((tables->manifest_resource_table).table != NULL) {
                return tables->manifest_resource_table.cardinality;
            }
            return 0;
        case METHOD_DEF_TABLE:
            if ((tables->method_def_table).table != NULL) {
                return tables->method_def_table.cardinality;
            }
            return 0;
        case METHOD_IMPL_TABLE:
            if ((tables->method_impl_table).table != NULL) {
                return tables->method_impl_table.cardinality;
            }
            return 0;
        case METHOD_SEMANTICS_TABLE:
            if ((tables->method_semantics_table).table != NULL) {
                return tables->method_semantics_table.cardinality;
            }
            return 0;
        case MEMBER_REF_TABLE:
            if ((tables->member_ref_table).table != NULL) {
                return tables->member_ref_table.cardinality;
            }
            return 0;
        case MODULE_TABLE:
            if ((tables->module_table).table != NULL) {
                return tables->module_table.cardinality;
            }
            return 0;
        case MODULE_REF_TABLE:
            if ((tables->module_ref_table).table != NULL) {
                return tables->module_ref_table.cardinality;
            }
            return 0;
        case NESTED_CLASS_TABLE:
            if ((tables->nested_class_table).table != NULL) {
                return tables->nested_class_table.cardinality;
            }
            return 0;
        case PARAM_TABLE:
            if ((tables->param_table).table != NULL) {
                return tables->param_table.cardinality;
            }
            return 0;
        case PROPERTY_TABLE:
            if ((tables->property_table).table != NULL) {
                return tables->property_table.cardinality;
            }
            return 0;
        case PROPERTY_MAP_TABLE:
            if ((tables->property_map_table).table != NULL) {
                return tables->property_map_table.cardinality;
            }
            return 0;
        case STANDALONE_SIG_TABLE:
            if ((tables->standalone_sig_table).table != NULL) {
                return tables->standalone_sig_table.cardinality;
            }
            return 0;
        case TYPE_DEF_TABLE:
            if ((tables->type_def_table).table != NULL) {
                return tables->type_def_table.cardinality;
            }
            return 0;
        case TYPE_REF_TABLE:
            if ((tables->type_ref_table).table != NULL) {
                return tables->type_ref_table.cardinality;
            }
            return 0;
        case TYPE_SPEC_TABLE:
            if ((tables->type_spec_table).table != NULL) {
                return tables->type_spec_table.cardinality;
            }
            return 0;
        case GENERIC_PARAM_TABLE:
            if ((tables->generic_param_table).table != NULL) {
                return tables->generic_param_table.cardinality;
            }
            return 0;
        case GENERIC_PARAM_CONSTRAINT_TABLE:
            if ((tables->generic_param_constraint_table).table != NULL) {
                return tables->generic_param_constraint_table.cardinality;
            }
            return 0;
        case METHOD_SPEC_TABLE:
            if ((tables->method_spec_table).table != NULL) {
                return tables->method_spec_table.cardinality;
            }
            return 0;
        default:
            snprintf(buffer, sizeof(buffer), "METADATA_TABLE_MANAGER: get_table_cardinality: ERROR= Table 0x%X not know. ", table_ID);
            print_err(buffer, 0);
            abort();
    }
    return 0;

}

JITBOOLEAN read_tables_cardinality (t_metadata_tables *tables, JITUINT64 valid, JITINT8 *row_buf) {
    JITUINT16 offset;

    /* Assertions	*/
    assert(tables != NULL);
    assert(row_buf != NULL);

    PDEBUG("METADATA TABLE MANAGER: Tables cardinality\n");
    offset = 0;

    /* Module table			*/
    if ( ((valid >> MODULE_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->module_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Module          =	%d\n", tables->module_table.cardinality);
    } else {
        tables->module_table.cardinality = 0;
    }

    /* Type ref table		*/
    if ( ((valid >> TYPE_REF_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->type_ref_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Type ref	=	%d\n", tables->type_ref_table.cardinality);
    } else {
        tables->type_ref_table.cardinality = 0;
    }

    /* Type def table		*/
    if ( ((valid >> TYPE_DEF_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->type_def_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Type def	=	%d\n", tables->type_def_table.cardinality);
    } else {
        tables->type_def_table.cardinality = 0;
    }

    /* Field table			*/
    if ( ((valid >> FIELD_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->field_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Field		=	%d\n", tables->field_table.cardinality);
    } else {
        tables->field_table.cardinality = 0;
    }

    /* Method def table		*/
    if ( ((valid >> METHOD_DEF_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->method_def_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Method def	=	%d\n", tables->method_def_table.cardinality);
    } else {
        tables->method_def_table.cardinality = 0;
    }

    /* Param table			*/
    if ( ((valid >> PARAM_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->param_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Param		=	%d\n", tables->param_table.cardinality);
    } else {
        tables->param_table.cardinality = 0;
    }

    /* Interface Impl table		*/
    if ( ((valid >> INTERFACE_IMPL_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->interface_impl_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Interface Impl	=	%d\n", tables->interface_impl_table.cardinality);
    } else {
        tables->interface_impl_table.cardinality = 0;
    }

    /* Member ref table		*/
    if ( ((valid >> MEMBER_REF_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->member_ref_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Member ref	=	%d\n", tables->member_ref_table.cardinality);
    } else {
        tables->member_ref_table.cardinality = 0;
    }

    /* Constant table		*/
    if ( ((valid >> CONSTANT_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->constant_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Constant	=	%d\n", tables->constant_table.cardinality);
    } else {
        tables->constant_table.cardinality = 0;
    }

    /* Custom Attribute table		*/
    if ( ((valid >> CUSTOM_ATTRIBUTE_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->custom_attribute_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Custom Attribute=	%d\n", tables->custom_attribute_table.cardinality);
    } else {
        tables->custom_attribute_table.cardinality = 0;
    }

    /* Field marshal table			*/
    if ( ((valid >> FIELD_MARSHAL_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->field_marshal_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Field marshal	=	%d\n", tables->field_marshal_table.cardinality);
    } else {
        tables->field_marshal_table.cardinality = 0;
    }

    /* Decl security table			*/
    if ( ((valid >> DECL_SECURITY_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->decl_security_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Decl security	=	%d\n", tables->decl_security_table.cardinality);
    } else {
        tables->decl_security_table.cardinality = 0;
    }

    /* Class layout table			*/
    if ( ((valid >> CLASS_LAYOUT_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->class_layout_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Class layout	=	%d\n", tables->class_layout_table.cardinality);
    } else {
        tables->class_layout_table.cardinality = 0;
    }

    /* Field layout table			*/
    if ( ((valid >> FIELD_LAYOUT_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->field_layout_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Field layout	=	%d\n", tables->field_layout_table.cardinality);
    } else {
        tables->field_layout_table.cardinality = 0;
    }

    /* Standalone sig table		*/
    if ( ((valid >> STANDALONE_SIG_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->standalone_sig_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Standalone Sig	=	%d\n", tables->standalone_sig_table.cardinality);
    } else {
        tables->standalone_sig_table.cardinality = 0;
    }

    /* Event map table		*/
    if ( ((valid >> EVENT_MAP_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->event_map_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Event map	=	%d\n", tables->event_map_table.cardinality);
    } else {
        tables->event_map_table.cardinality = 0;
    }

    /* Event table			*/
    if ( ((valid >> EVENT_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->event_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Event           =	%d\n", tables->event_table.cardinality);
    } else {
        tables->event_table.cardinality = 0;
    }

    /* Property map table			*/
    if ( ((valid >> PROPERTY_MAP_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->property_map_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Property map	=	%d\n", tables->property_map_table.cardinality);
    } else {
        tables->property_map_table.cardinality = 0;
    }

    /* Property table		*/
    if ( ((valid >> PROPERTY_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->property_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Property	=	%d\n", tables->property_table.cardinality);
    } else {
        tables->property_table.cardinality = 0;
    }

    /* Method semantics table	*/
    if ( ((valid >> METHOD_SEMANTICS_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->method_semantics_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Method Semantics=	%d\n", tables->method_semantics_table.cardinality);
    } else {
        tables->method_semantics_table.cardinality = 0;
    }

    /* Method impl table		*/
    if ( ((valid >> METHOD_IMPL_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->method_impl_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Method impl	=	%d\n", tables->method_impl_table.cardinality);
    } else {
        tables->method_impl_table.cardinality = 0;
    }

    /* Module ref table		*/
    if ( ((valid >> MODULE_REF_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->module_ref_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Module ref	=	%d\n", tables->module_ref_table.cardinality);
    } else {
        tables->module_ref_table.cardinality = 0;
    }

    /* Type spec table		*/
    if ( ((valid >> TYPE_SPEC_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->type_spec_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Type spec	=	%d\n", tables->type_spec_table.cardinality);
    } else {
        tables->type_spec_table.cardinality = 0;
    }

    /* Impl map table		*/
    if ( ((valid >> IMPL_MAP_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->impl_map_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Impl map	=	%d\n", tables->impl_map_table.cardinality);
    } else {
        tables->impl_map_table.cardinality = 0;
    }

    /* Field RVA table		*/
    if ( ((valid >> FIELD_RVA_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->field_rva_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Field RVA	=	%d\n", tables->field_rva_table.cardinality);
    } else {
        tables->field_rva_table.cardinality = 0;
    }

    /* Assembly table		*/
    if ( ((valid >> ASSEMBLY_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->assembly_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Assembly	=	%d\n", tables->assembly_table.cardinality);
    } else {
        tables->assembly_table.cardinality = 0;
    }

    /* Assembly ref table		*/
    if ( ((valid >> ASSEMBLY_REF_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->assembly_ref_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Assembly ref	=	%d\n", tables->assembly_ref_table.cardinality);
    } else {
        tables->assembly_ref_table.cardinality = 0;
    }

    /* File table		*/
    if ( ((valid >> FILE_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->file_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         File		=	%d\n", tables->file_table.cardinality);
    } else {
        tables->file_table.cardinality = 0;
    }

    /* Exported type table		*/
    if ( ((valid >> EXPORTED_TYPE_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->exported_type_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Exported type	=	%d\n", tables->exported_type_table.cardinality);
    } else {
        tables->exported_type_table.cardinality = 0;
    }

    /* Manifest resource table		*/
    if ( ((valid >> MANIFEST_RESOURCE_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->manifest_resource_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Manifest resource=	%d\n", tables->manifest_resource_table.cardinality);
    } else {
        tables->manifest_resource_table.cardinality = 0;
    }

    /* Nested class table			*/
    if ( ((valid >> NESTED_CLASS_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->nested_class_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Nested class	=	%d\n", tables->nested_class_table.cardinality);
    } else {
        tables->nested_class_table.cardinality = 0;
    }

    /* Generic param table			*/
    if ( ((valid >> GENERIC_PARAM_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->generic_param_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Generic param	=	%d\n", tables->generic_param_table.cardinality);
    } else {
        tables->generic_param_table.cardinality = 0;
    }

    /* Method spec table			*/
    if ( ((valid >> METHOD_SPEC_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->method_spec_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Method spec	=	%d\n", tables->method_spec_table.cardinality);
    } else {
        tables->method_spec_table.cardinality = 0;
    }

    /* Generic param constraint table			*/
    if ( ((valid >> GENERIC_PARAM_CONSTRAINT_TABLE) & 0x1 ) != 0) {
        read_from_buffer(row_buf, offset, 4, &(tables->generic_param_constraint_table.cardinality));
        offset += 4;
        PDEBUG("METADATA TABLE MANAGER:         Generic param constraint	=	%d\n", tables->generic_param_constraint_table.cardinality);
    } else {
        tables->generic_param_constraint_table.cardinality = 0;
    }

    return 0;
}
