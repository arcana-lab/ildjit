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
   *@file field_marshal_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <compiler_memory_manager.h>
#include <jitsystem.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <mtm_utils.h>
#include <iljitmm-system.h>
// End

#define HAS_FIELD_MARSHAL_FLAG_MASK 0x00000001

JITINT32 load_field_marshal_table (t_field_marshal_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    JITUINT32 temp_parent;
    JITBOOLEAN parent_decoded;
    JITBOOLEAN table_threshold_exceeded;
    JITUINT8 field_length;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> FIELD_MARSHAL_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    temp_parent = 0;
    parent_decoded = JITFALSE;
    table_threshold_exceeded = JITFALSE;

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Field marshal table\n");
    bytes_read      = 0;
    table->table    = (t_row_field_marshal_table **) sharedAllocFunction(sizeof(t_row_field_marshal_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_field_marshal_table       *temp_row;
        temp_row                        = (t_row_field_marshal_table *) sharedAllocFunction(sizeof(t_row_field_marshal_table));
        temp_row->index                 = cardinality + 1;        //Index with begin value egual to one
        temp_row->binary = binary;
        /* Resetting local vars for checkings */
        temp_parent = 0;
        parent_decoded = JITFALSE;
        table_threshold_exceeded = JITFALSE;
        field_length = 0;

        /* Early decoding the signature to check if it refers to a field or a param */
        if (((t_field_table *) private_get_table(NULL, FIELD_TABLE))->cardinality > METADATA_TABLE_LIMIT_15_BITS_ENCODING) {
            PDEBUG("TABLE MANAGER:	using 4 byte index for HasFieldMarshal, Field table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, FIELD_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 4, &(temp_parent));
            table_threshold_exceeded = JITTRUE;
            field_length = 4;
        } else {
            PDEBUG("TABLE MANAGER:	using 2 byte index for HasFieldMarshal, Field table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, FIELD_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 2, &(temp_parent));
            table_threshold_exceeded = JITFALSE;
            field_length = 2;
        }
        /* Masking the first bit which encodes the reference */
        temp_parent = temp_parent & HAS_FIELD_MARSHAL_FLAG_MASK;
        if (temp_parent == 0) {
            PDEBUG("FIELD_MARSHAL_TABLE: The Parent refers to a Field\n");
            parent_decoded = JITTRUE;
            if (table_threshold_exceeded) {
                PDEBUG("FIELD_MARSHAL_TABLE: Using 4 byte index for Parent\n");
                read_from_buffer(table_buf, bytes_read, 4, &(temp_row->parent));
            } else {
                PDEBUG("FIELD_MARSHAL_TABLE: Using 2 byte index for Parent\n");
                read_from_buffer(table_buf, bytes_read, 2, &(temp_row->parent));
            }
        }
        /* The Parent index does not reference the Field table, trying with the Param table */
        if (!parent_decoded) {
            if (((t_field_table *) private_get_table(NULL, PARAM_TABLE))->cardinality > METADATA_TABLE_LIMIT_15_BITS_ENCODING) {
                PDEBUG("TABLE MANAGER:	using 4 byte index for HasFieldMarshal, Param table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, PARAM_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 4, &(temp_parent));
                table_threshold_exceeded = JITTRUE;
                field_length = MAX(field_length, 4);
            } else {
                PDEBUG("TABLE MANAGER:	using 2 byte index for HasFieldMarshal, Param table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, PARAM_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 2, &(temp_parent));
                table_threshold_exceeded = JITFALSE;
                field_length = MAX(field_length, 2);
            }
            /* Masking the first bit which encodes the reference */
            temp_parent = temp_parent & HAS_FIELD_MARSHAL_FLAG_MASK;
            if (temp_parent == 1) {
                PDEBUG("FIELD_MARSHAL_TABLE: The Parent refers to a Param\n");
                parent_decoded = JITTRUE;
                if (table_threshold_exceeded) {
                    PDEBUG("FIELD_MARSHAL_TABLE: Using 4 byte index for Parent\n");
                    read_from_buffer(table_buf, bytes_read, 4, &(temp_row->parent));
                } else {
                    PDEBUG("FIELD_MARSHAL_TABLE: Using 2 byte index for Parent\n");
                    read_from_buffer(table_buf, bytes_read, 2, &(temp_row->parent));
                }
            }
        }
        /* If the Parent index cannot be decoded, generate an error here */
        if (!parent_decoded) {
            print_err("TABLE MANAGER: ERROR= Failed decoding the Parent reference. ", errno);
            return -1;
        }

        bytes_read += field_length;

        if (blob_index_is_32(heap_size)) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->native_type));
            bytes_read += 4;
        } else {
            // Index of string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->native_type));
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef PRINTDEBUG
    JITINT32 count;
    PDEBUG("			FIELD MARSHAL TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Parent		Native type\n");
    PDEBUG("				(Blob)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_field_marshal_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X		%d\n", (count + 1), temp->parent, temp->native_type);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_field_marshal_table (t_field_marshal_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Method def shutting down\n");
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
