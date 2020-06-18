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
   *@file constant_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <jitsystem.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End

JITINT32 load_constant_table (t_constant_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> CONSTANT_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Constant table\n");
    bytes_read = 0;
    table->table    = (t_row_constant_table **) sharedAllocFunction(sizeof(t_row_constant_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_constant_table *temp_row;
        temp_row                = (t_row_constant_table *) sharedAllocFunction(sizeof(t_row_constant_table));
        temp_row->index         = cardinality + 1;        //Index with begin value egual to one
        temp_row->binary = binary;
        read_from_buffer(table_buf, bytes_read, 2, &(temp_row->type));
        temp_row->type = ((temp_row->type) >> 8);
        bytes_read += 2;
        temp_row->parent = 0;
        if (    (((t_field_table *) private_get_table(NULL, FIELD_TABLE))->cardinality > 16384)          ||
                (((t_param_table *) private_get_table(NULL, PARAM_TABLE))->cardinality > 16384)          ||
                (((t_property_table *) private_get_table(NULL, PROPERTY_TABLE))->cardinality > 16384)               ) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->parent));
            bytes_read += 4;
        } else {
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->parent));
            bytes_read += 2;
        }
        if (blob_index_is_32(heap_size)) {
            // Index of blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->signature));
            bytes_read += 4;
        } else {
            // Index of blob stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->signature));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    JITINT32 count;
    PDEBUG("			CONSTANT TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Type		Parent		Signature\n");
    PDEBUG("						(Blob)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_constant_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X		0x%X		%d\n", (count + 1), temp->type, temp->parent, temp->signature);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_constant_table (t_constant_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions           */
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
