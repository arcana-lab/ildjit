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
   *@file assembly_ref_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <metadata_streams_manager.h>
#include <jitsystem.h>
#include <decoder.h>
#include <iljitmm-system.h>
// End

/**
 * @brief Fetch RVA from an assembly ref
 *
 * Search an assembly ref inside the AssemblyRef table of metadata with TypeName equal to type_name and TypeNameSpace equal to type_name_space
 */
JITBOOLEAN assembly_ref_get_rva_from_row (void *streams, JITINT8 *type_name, JITINT8 *type_name_space, void *method, t_row_assembly_ref_table *row);
void * assembly_ref_get_type (void *streams, JITINT8 *type_name, JITINT8 *type_name_space, t_row_assembly_ref_table *assembly_ref);

JITINT32 load_assembly_ref_table (t_assembly_ref_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != 0);
    if ( ((valid >> ASSEMBLY_REF_TABLE) & 0x1 ) == 0) {
        (*bytes) = 0;
        return 0;
    }
    assert(table->cardinality > 0);

    /* Create the table	*/
    PDEBUG("ASSEMBLY_REF_TABLE: load_assembly_ref_table: Start\n");
    bytes_read      = 0;
    table->table    = (t_row_assembly_ref_table **) sharedAllocFunction(sizeof(t_row_assembly_ref_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_assembly_ref_table *temp_row;
        temp_row                        = (t_row_assembly_ref_table *) sharedAllocFunction(sizeof(t_row_assembly_ref_table));
        assert(temp_row != NULL);
        temp_row->binary_info = NULL;
        temp_row->binary = binary;
        temp_row->index = cardinality + 1;                                  // The index of the table start at one value
        read_from_buffer(table_buf, bytes_read, 2, &(temp_row->major_version));
        read_from_buffer(table_buf, bytes_read + 2, 2, &(temp_row->minor_version));
        read_from_buffer(table_buf, bytes_read + 4, 2, &(temp_row->build_number));
        read_from_buffer(table_buf, bytes_read + 6, 2, &(temp_row->revision_number));
        read_from_buffer(table_buf, bytes_read + 8, 2, &(temp_row->flags));
        bytes_read += 12;
        if (blob_index_is_32(heap_size)) {
            // Index of blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->public_key_or_token));
            bytes_read += 4;
        } else {
            // Index of blob stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->public_key_or_token));
            bytes_read += 2;
        }
        if (string_index_is_32(heap_size)) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->name));
            read_from_buffer(table_buf, bytes_read + 4, 4, &(temp_row->culture));
            bytes_read += 8;
        } else {
            // Index of string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->name));
            read_from_buffer(table_buf, bytes_read + 2, 2, &(temp_row->culture));
            bytes_read += 4;
        }
        if (blob_index_is_32(heap_size)) {
            // Index of blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->hash_value));
            bytes_read += 4;
        } else {
            // Index of blob stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->hash_value));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    PDEBUG("			ASSEMBLY REF TABLE	Cardinality = %d\n", cardinality);
    JITINT32 count;
    PDEBUG("Index	Version		Build	Revision	Flags		Public key	Name		Culture		Hash	Decoded\n");
    PDEBUG("			                                        (Blob index)	(String index)	(String index)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_assembly_ref_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d	%d.%d		%d	%d		0x%X		%d		%d		%d		0x%X\n", (count + 1), temp->major_version, temp->minor_version, temp->build_number, temp->revision_number, temp->flags, temp->public_key_or_token, temp->name, temp->culture, temp->hash_value);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_assembly_ref_table (t_assembly_ref_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Assembly ref shutting down\n");
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
