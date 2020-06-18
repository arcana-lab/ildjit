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
   *@file assembly_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <iljitmm-system.h>
// End

JITINT32 load_assembly_table (t_assembly_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != 0);
    if ( ((valid >> ASSEMBLY_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Assembly table\n");
    bytes_read      = 0;
    table->table    = (t_row_assembly_table **) sharedAllocFunction(sizeof(t_row_assembly_table *) * (table->cardinality));
    if (table->table == NULL) {
        print_err("TABLE MANAGER: ERROR= Cannot create a table. ", 0);
        abort();
    }
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_assembly_table *temp_row;
        temp_row                        = (t_row_assembly_table *) sharedAllocFunction(sizeof(t_row_assembly_table));
        if (temp_row == NULL) {
            print_err("TABLE MANAGER: ERROR= Malloc return NULL. ", errno);
            return -1;
        }
        temp_row->index = cardinality + 1;                                  // The index of the table start at one value
        temp_row->binary = binary;
        read_from_buffer(table_buf, 0, 4, &(temp_row->hash_algorithm_ID));
        read_from_buffer(table_buf, 4, 2, &(temp_row->major_version));
        read_from_buffer(table_buf, 6, 2, &(temp_row->minor_version));
        read_from_buffer(table_buf, 8, 2, &(temp_row->build_number));
        read_from_buffer(table_buf, 10, 2, &(temp_row->revision_number));
        read_from_buffer(table_buf, 12, 4, &(temp_row->flags));
        bytes_read += 16;
        if (blob_index_is_32(heap_size)) {
            // Index of the blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->public_key));
            bytes_read += 4;
        } else {
            // Index of the blob stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->public_key));
            bytes_read += 2;
        }
        if (string_index_is_32(heap_size)) {
            // Index of the string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->name));
            bytes_read += 4;
        } else {
            // Index of the string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->name));
            bytes_read += 2;
        }
        if (string_index_is_32(heap_size)) {
            // Index of the string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->culture));
            bytes_read += 4;
        } else {
            // Index of the string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->culture));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    PDEBUG("			ASSEMBLY TABLE	Cardinality = %d\n", cardinality);
    JITINT32 count;
    PDEBUG("Index	HashAlgorithmID		Version		Build N	Revision N	Flags	Public key	Name		Culture\n");
    PDEBUG("										(Blob index)	(String index)	(String index)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_assembly_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d	0x%X			%d.%d		%d	%d		0x%X	%d		%d		%d\n", (count + 1), temp->hash_algorithm_ID, temp->major_version, temp->minor_version, temp->build_number, temp->revision_number, temp->flags, temp->public_key, temp->name, temp->culture);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_assembly_table (t_assembly_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Assembly shutting down\n");
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
