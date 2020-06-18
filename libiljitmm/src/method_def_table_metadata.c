/*
 * Copyright (C) 2009  Campanoni Simone, Michele Tartara
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
   *@file method_def_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <metadata/streams/metadata_streams_manager.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End

JITINT32 load_method_def_table (t_method_def_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 	cardinality;
    JITUINT32 	bytes_read;
    JITBOOLEAN	lessOrEqualThan2Bytes;
    JITBOOLEAN	stringIndexIs32;
    JITBOOLEAN	blobIndexIs32;

    /* Assertions.
     */
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> METHOD_DEF_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table.
     */
    PDEBUG("TABLE MANAGER:	Method def table\n");
    bytes_read              = 0;
    table->table            = (t_row_method_def_table *) sharedAllocFunction(sizeof(t_row_method_def_table) * (table->cardinality));
    assert(table->table != NULL);

    /* Check whether the size of param table require 4 bytes.
     */
    if (((t_param_table *) private_get_table(NULL, PARAM_TABLE))->cardinality <= METADATA_TABLE_LIMIT_16_BITS_ENCODING) {
        lessOrEqualThan2Bytes	= JITTRUE;
    } else {
        lessOrEqualThan2Bytes	= JITFALSE;
    }

    /* Check string and blob index sizes.
     */
    stringIndexIs32	= string_index_is_32(heap_size);
    blobIndexIs32	= blob_index_is_32(heap_size);

    /* Fill up the table.
     */
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_method_def_table *temp_row;

        temp_row                        = &(table->table[cardinality]);
        temp_row->index                 = cardinality + 1;
        temp_row->binary 		= binary;

        read_from_buffer(table_buf, bytes_read, 4, &(temp_row->RVA));
        bytes_read += 4;
        read_from_buffer(table_buf, bytes_read, 2, &(temp_row->impl_flags));
        bytes_read += 2;
        read_from_buffer(table_buf, bytes_read, 2, &(temp_row->flags));
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
        if (blobIndexIs32) {

            // Index of blob stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->signature));
            bytes_read += 4;
        } else {

            // Index of blob stream is 16 bit
            temp_row->signature = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->signature));
            bytes_read += 2;
        }
        if (lessOrEqualThan2Bytes) {
            temp_row->param_list = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->param_list));
            bytes_read += 2;
        } else {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->param_list));
            bytes_read += 4;
        }
    }

#ifdef PRINTDEBUG
    int count;
    PDEBUG("			METHOD DEF TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		RVA		Impl flags	Flags		Name	Signature	Param list\n");
    PDEBUG("								(String)(Blob)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_method_def_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X		0x%X		0x%X		%d	%d		%d\n", (count + 1), temp->RVA, temp->impl_flags, temp->flags, temp->name, temp->signature, temp->param_list);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_method_def_table (t_method_def_table *table) {

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
