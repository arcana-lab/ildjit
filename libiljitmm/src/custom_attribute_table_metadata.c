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
   *@file custom_attribute_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <jitsystem.h>
#include <iljitmm-system.h>
#include <mtm_utils.h>
// End

JITINT32 load_custom_attribute_table (t_custom_attribute_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> CUSTOM_ATTRIBUTE_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Custom Attribute table\n");
    bytes_read = 0;
    table->table    = (t_row_custom_attribute_table **) sharedAllocFunction(sizeof(t_row_custom_attribute_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_custom_attribute_table *temp_row;
        temp_row                = (t_row_custom_attribute_table *) sharedAllocFunction(sizeof(t_row_custom_attribute_table));
        temp_row->index         = cardinality + 1;        //Index with begin value egual to one
        temp_row->binary = binary;
        if (    (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality > 2048)         ||
                (((t_field_table *) private_get_table(NULL, FIELD_TABLE))->cardinality > 2048)                   ||
                (((t_type_ref_table *) private_get_table(NULL, TYPE_REF_TABLE))->cardinality > 2048)             ||
                (((t_type_def_table *) private_get_table(NULL, TYPE_DEF_TABLE))->cardinality > 2048)             ||
                (((t_param_table *) private_get_table(NULL, PARAM_TABLE))->cardinality > 2048)                   ||
                (((t_interface_impl_table *) private_get_table(NULL, INTERFACE_IMPL_TABLE))->cardinality > 2048) ||
                (((t_member_ref_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality > 2048)         ||
                (((t_module_table *) private_get_table(NULL, MODULE_TABLE))->cardinality > 2048)                 ||
                //(((t_permission_table *)private_get_table(NULL, PERMISSION_TABLE))->cardinality > 2048)		||// FIXME eliminare i commenti quando le tabelle sono state create
                (((t_property_table *) private_get_table(NULL, PROPERTY_TABLE))->cardinality > 2048)             ||
                (((t_event_table *) private_get_table(NULL, EVENT_TABLE))->cardinality > 2048)                   ||
                (((t_standalone_sig_table *) private_get_table(NULL, STANDALONE_SIG_TABLE))->cardinality > 2048) ||
                (((t_module_ref_table *) private_get_table(NULL, MODULE_REF_TABLE))->cardinality > 2048)         ||
                (((t_type_spec_table *) private_get_table(NULL, TYPE_SPEC_TABLE))->cardinality > 2048)           ||
                (((t_assembly_table *) private_get_table(NULL, ASSEMBLY_TABLE))->cardinality > 2048)             ||
                (((t_assembly_ref_table *) private_get_table(NULL, ASSEMBLY_REF_TABLE))->cardinality > 2048)     ||
                (((t_file_table *) private_get_table(NULL, FILE_TABLE))->cardinality > 2048)                     ||
                (((t_exported_type_table *) private_get_table(NULL, EXPORTED_TYPE_TABLE))->cardinality > 2048)   ||
                (((t_manifest_resource_table *) private_get_table(NULL, MANIFEST_RESOURCE_TABLE))->cardinality > 2048)) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->parent));
            bytes_read += 4;
        } else {
            temp_row->parent = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->parent));
            bytes_read += 2;
        }
        if (    (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality > 8192)         ||
                (((t_member_ref_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality > 8192)) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->type));
            bytes_read += 4;
        } else {
            temp_row->type = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->type));
            bytes_read += 2;
        }
        if (blob_index_is_32(heap_size)) {
            // Index of blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->value));
            bytes_read += 4;
        } else {
            // Index of blob stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->value));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    JITINT32 count;
    PDEBUG("			CUSTOM ATTRIBUTE TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Parent		Type		Value\n");
    PDEBUG("						(Blob)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_custom_attribute_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X		0x%X		%d\n", (count + 1), temp->parent, temp->type, temp->value);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_custom_attribute_table (t_custom_attribute_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Parent shutting down\n");
    size = table->cardinality;
    PDEBUG("TABLE MANAGER:			Rows=%d\n", size);

    /* Delete the rows	*/
    for (count = 0; count < size; count++) {
        freeFunction(table->table[count]);
    }

    /* Delete the table	*/
    freeFunction(table->table);
    PDEBUG("TABLE MANAGER:			Table deleted\n");
}
