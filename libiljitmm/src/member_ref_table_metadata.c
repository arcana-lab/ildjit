/*
 * Copyright (C) 2009  Campanoni Simone, Farina Roberto, Michele Tartara
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
   *@file member_ref_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <jitsystem.h>
#include <ecma_constant.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata_types.h>
#include <decoding_tools.h>
#include <metadata_manager.h>
#include <metadata/tables/metadata_table_manager.h>
#include <metadata_streams_manager.h>
#include <metadata/tables/type_def_table_metadata.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End

JITINT32 load_member_ref_table (t_member_ref_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != 0);
    if ( ((valid >> MEMBER_REF_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Member ref table\n");
    bytes_read      = 0;
    table->table    = (t_row_member_ref_table **) sharedAllocFunction(sizeof(t_row_member_ref_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < table->cardinality; cardinality++) {
        t_row_member_ref_table *temp_row;
        JITUINT32 max_cardinality;
        JITUINT32 current_cardinality;
        temp_row                        = (t_row_member_ref_table *) sharedAllocFunction(sizeof(t_row_member_ref_table));
        temp_row->index                 = cardinality + 1;        // The index of the table start at one value
        temp_row->binary = binary;
        /* Compute the max cardinality between the TypeDef, TypeRef, ModuleRef, MethodDef, TypeSpec tables	*/
        max_cardinality = ((t_type_def_table *) private_get_table(NULL, TYPE_DEF_TABLE))->cardinality;
        current_cardinality = ((t_type_ref_table *) private_get_table(NULL, TYPE_REF_TABLE))->cardinality;
        if (current_cardinality > max_cardinality ) {
            max_cardinality = current_cardinality;
        }
        current_cardinality = ((t_module_ref_table *) private_get_table(NULL, MODULE_REF_TABLE))->cardinality;
        if (current_cardinality > max_cardinality ) {
            max_cardinality = current_cardinality;
        }
        current_cardinality = ((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality;
        if (current_cardinality > max_cardinality ) {
            max_cardinality = current_cardinality;
        }
        current_cardinality = ((t_type_spec_table *) private_get_table(NULL, TYPE_SPEC_TABLE))->cardinality;
        if (current_cardinality > max_cardinality ) {
            max_cardinality = current_cardinality;
        }

        /* Fill the value	*/
        if (max_cardinality > METADATA_TABLE_LIMIT_13_BITS_ENCODING) { //13 bits encode 8192 rows + 3 bit encode the table => 16 bits are enough
            // Index is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->classIndex));
            bytes_read += 4;
        } else {
            // Index is 16 bit
            temp_row->classIndex = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->classIndex));
            bytes_read += 2;
        }
        if (string_index_is_32(heap_size)) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->name));
            bytes_read += 4;
        } else {
            // Index of string stream is 16 bit
            temp_row->name = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->name));
            bytes_read += 2;
        }
        if (blob_index_is_32(heap_size)) {
            // Index of Blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->signature));
            bytes_read += 4;
        } else {
            // Index of Blob stream is 16 bit
            temp_row->signature = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->signature));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    JITINT32 count;
    PDEBUG("			MEMBER REF TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Class		Name		Signature\n");
    PDEBUG("				(String index)	(Blob index)\n");
    for (count = 0; count < (table->cardinality); count++) {
        t_row_member_ref_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X		%d		%d\n", (count + 1), temp->class, temp->name, temp->signature);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_member_ref_table (t_member_ref_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Method ref shutting down\n");
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
