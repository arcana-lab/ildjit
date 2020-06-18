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
   *@file exported_type_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End

JITINT32 load_exported_type_table (t_exported_type_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> EXPORTED_TYPE_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Exported type table\n");
    bytes_read      = 0;
    table->table    = (t_row_exported_type_table **) sharedAllocFunction(sizeof(t_row_exported_type_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_exported_type_table *temp_row;
        temp_row                        = (t_row_exported_type_table *) sharedAllocFunction(sizeof(t_row_exported_type_table));
        temp_row->index                 = cardinality + 1;        //Index with begin value egual to one
        temp_row->binary = binary;
        read_from_buffer(table_buf, bytes_read, 4, &(temp_row->flags));
        bytes_read += 4;
        read_from_buffer(table_buf, bytes_read, 4, &(temp_row->type_def_id));
        bytes_read += 4;
        if (string_index_is_32(heap_size)) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->type_name));
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->type_name_space));
            bytes_read += 8;
        } else {
            // Index of string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->type_name));
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->type_name_space));
            bytes_read += 4;
        }
        if (    (((t_file_table *) private_get_table(NULL, FILE_TABLE))->cardinality > 16384)                    ||
                (((t_assembly_ref_table *) private_get_table(NULL, ASSEMBLY_REF_TABLE))->cardinality > 16384)    ||
                (((t_exported_type_table *) private_get_table(NULL, EXPORTED_TYPE_TABLE))->cardinality > 16384)) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->implementation));
            bytes_read += 4;
        } else {
            temp_row->implementation = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->implementation));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    JITINT32 count;
    PDEBUG("			EXPORTED TYPE TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Flags	TypeDefId	Type name	Type name space		Implementation\n");
    PDEBUG("					(String)	(String)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_exported_type_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X	0x%X		%d		%d			0x%X\n", (count + 1), temp->flags, temp->type_def_id, temp->type_name, temp->type_name_space, temp->implementation);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_exported_type_table (t_exported_type_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Exported type shutting down\n");
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
