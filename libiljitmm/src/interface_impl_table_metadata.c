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
   *@file interface_impl_table_metadata.c
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


JITINT32 load_interface_impl_table (t_interface_impl_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> INTERFACE_IMPL_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Interface Impl table\n");
    bytes_read      = 0;
    table->table    = (t_row_interface_impl_table **) sharedAllocFunction(sizeof(t_row_interface_impl_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_interface_impl_table *temp_row;
        temp_row                        = (t_row_interface_impl_table *) sharedAllocFunction(sizeof(t_row_interface_impl_table));
        temp_row->index                 = cardinality + 1;        //Index with initial value equal to one
        temp_row->binary = binary;
        if ((((t_type_def_table *) private_get_table(NULL, TYPE_DEF_TABLE))->cardinality <= METADATA_TABLE_LIMIT_16_BITS_ENCODING)) {
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->classIndex));
            bytes_read += 2;
        } else {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->classIndex));
            bytes_read += 4;
        }
        if (private_is_type_def_or_ref_token_32()) {
            /* Index TypeDefOrRef is 32 bit		*/
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->interfaceDescription));
            bytes_read += 4;
        } else {
            /* Index TypeDefOrRef is 16 bit		*/
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->interfaceDescription));
            temp_row->interfaceDescription = temp_row->interfaceDescription & 0x0000FFFF;
            bytes_read += 2;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef PRINTDEBUG
    JITINT32 count;
    PDEBUG("			INTERFACE IMPL TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Class	Interface\n");
    for (count = 0; count < cardinality; count++) {
        t_row_interface_impl_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X		0x%X\n", (count + 1), temp->classIndex, temp->interface);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_interface_impl_table (t_interface_impl_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Interface impl shutting down\n");
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
