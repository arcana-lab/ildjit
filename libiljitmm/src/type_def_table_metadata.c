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
   *@file type_def_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End


JITINT32 load_type_def_table (t_type_def_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != 0);
    if ( ((valid >> TYPE_DEF_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Type def table\n");
    bytes_read      = 0;
    table->table    = (t_row_type_def_table **) sharedAllocFunction(sizeof(t_row_type_def_table *) * table->cardinality);
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_type_def_table *temp_row;
        temp_row                        = (t_row_type_def_table *) sharedAllocFunction(sizeof(t_row_type_def_table));
        temp_row->index                 = cardinality + 1;        // The index of the table start at one value
        temp_row->binary = binary;
        read_from_buffer(table_buf, bytes_read, 4, &(temp_row->flags));
        bytes_read += 4;
        if (string_index_is_32(heap_size)) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->type_name));
            read_from_buffer(table_buf, bytes_read + 4, 4, &(temp_row->type_name_space));
            bytes_read += 8;
        } else {
            // Index of string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->type_name));
            read_from_buffer(table_buf, bytes_read + 2, 2, &(temp_row->type_name_space));
            bytes_read += 4;
        }
        if (private_is_type_def_or_ref_token_32()) {
            /* Index TypeDefOrRef is 32 bit		*/
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->extendsIndex));
            bytes_read += 4;
        } else {
            /* Index TypeDefOrRef is 16 bit		*/
            temp_row->extendsIndex = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->extendsIndex));
            bytes_read += 2;
        }
        if (((t_field_table *) private_get_table(NULL, FIELD_TABLE))->cardinality <= METADATA_TABLE_LIMIT_16_BITS_ENCODING ) {
            /* Index of Field table	is 16 bit		*/
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->field_list));
            bytes_read += 2;
        } else {
            /* Index of Field table	is 32 bit		*/
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->field_list));
            bytes_read += 4;
        }
        if (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality <= METADATA_TABLE_LIMIT_16_BITS_ENCODING ) {
            /* Index of Method def table is 16 bit		*/
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_list));
            bytes_read += 2;
        } else {
            /* Index of Method def table is 32 bit		*/
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_list));
            bytes_read += 4;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef PRINTDEBUG
    JITINT32 count;
    PDEBUG("			TYPE DEF TABLE		Cardinality = %d\n", cardinality);
    PDEBUG("Index	Flags	Type name	Type name space		Extends			Field list	Method list\n");
    PDEBUG("		(String index)	(String index)		(TypeDefOrRef index)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_type_def_table *temp;
        temp = table->table[count];
        PDEBUG("%d	0x%X	%d		%d			0x%X			%d		%d\n", (count + 1), temp->flags, temp->type_name, temp->type_name_space, temp->extends, temp->field_list, temp->method_list);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_type_def_table (t_type_def_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }
    PDEBUG("TABLE MANAGER:		Type def shutting down\n");
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
