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
   *@file param_table_metadata.c
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

JITINT32 load_param_table (t_param_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 	cardinality;
    JITUINT32 	bytes_read;
    JITBOOLEAN	stringIndexIs32;

    /* Assertions.
     */
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> PARAM_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table.
     */
    PDEBUG("TABLE MANAGER:	Param table\n");
    bytes_read      = 0;
    table->table    = (t_row_param_table *) sharedAllocFunction(sizeof(t_row_param_table) * table->cardinality);
    assert(table->table != NULL);

    /* Check string and blob index sizes.
     */
    stringIndexIs32	= string_index_is_32(heap_size);

    /* Fill up the table.
     */
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_param_table *temp_row;
        temp_row                        = &(table->table[cardinality]);
        temp_row->index                 = cardinality + 1;        //Index with initial value equal to one
        temp_row->binary 		= binary;

        read_from_buffer(table_buf, bytes_read, 2, &(temp_row->flags));
        bytes_read += 2;
        read_from_buffer(table_buf, bytes_read, 2, &(temp_row->sequence));
        bytes_read += 2;

        if (stringIndexIs32) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->name));
            bytes_read += 4;
        } else {
            // Index of string stream is 16 bit
            temp_row->name = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->name));
            bytes_read += 2;
        }
    }

#ifdef PRINTDEBUG
    int count;
    PDEBUG("			PARAM TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Flags	Sequence	Name\n");
    PDEBUG("					(String Index)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_param_table *temp;
        temp = table->table[count];
        PDEBUG("%d		0x%X		0x%X		%d\n", (count + 1), temp->flags, temp->sequence, temp->name);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_param_table (t_param_table *table) {

    /* Assertions.
     */
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    /* Delete the table.
     */
    freeFunction(table->table);

    return ;
}
