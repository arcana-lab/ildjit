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
   *@file manifest_resource_table_metadata.c
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

/* functions for the t_row_manifest_resource_table */
JITBOOLEAN      isPublic (t_row_manifest_resource_table *manifest);
JITBOOLEAN      isPrivate (t_row_manifest_resource_table *manifest);
JITINT32        decodeImplementationIndex (t_row_manifest_resource_table *manifest, JITINT32 *destination_table);


JITINT32 load_manifest_resource_table (t_manifest_resource_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;
    JITUINT32 max_cardinality = 0;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> MANIFEST_RESOURCE_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Manifest resource table\n");
    bytes_read      = 0;
    table->table    = (t_row_manifest_resource_table **) sharedAllocFunction(sizeof(t_row_manifest_resource_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_manifest_resource_table *temp_row;
        temp_row                        = (t_row_manifest_resource_table *) sharedAllocFunction(sizeof(t_row_manifest_resource_table));
        temp_row->index                 = cardinality + 1;        //Index with begin value egual to one
        temp_row->binary = binary;
        read_from_buffer(table_buf, bytes_read, 4, &(temp_row->offset));
        bytes_read += 4;
        read_from_buffer(table_buf, bytes_read, 4, &(temp_row->flags));
        bytes_read += 4;
        if (string_index_is_32(heap_size)) {
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->name));
            bytes_read += 4;
        } else {
            // Index of string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->name));
            bytes_read += 2;
        }

        max_cardinality = ((t_file_table *) private_get_table(NULL, FILE_TABLE))->cardinality;
        max_cardinality = MAX(max_cardinality, ((t_assembly_ref_table *) private_get_table(NULL, ASSEMBLY_REF_TABLE))->cardinality);
        max_cardinality = MAX(max_cardinality, ((t_exported_type_table *) private_get_table(NULL, EXPORTED_TYPE_TABLE))->cardinality);

        if (max_cardinality > METADATA_TABLE_LIMIT_14_BITS_ENCODING) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->implementation));
            bytes_read += 4;
        } else {
            temp_row->implementation = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->implementation));
            bytes_read += 2;
        }
        temp_row->isPublic = isPublic;
        temp_row->isPrivate = isPrivate;
        temp_row->decodeImplementationIndex = decodeImplementationIndex;

        table->table[cardinality] = temp_row;
    }

#ifdef DEBUG
    JITINT32 count;
    PDEBUG("			MANIFEST RESOURCE TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Offset	Flags	Name		Implementation\n");
    PDEBUG("				(String Index)\n");
    for (count = 0; count < cardinality; count++) {
        t_row_manifest_resource_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X	0x%X	%d		0x%X\n", (count + 1), temp->offset, temp->flags, temp->name, temp->implementation);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_manifest_resource_table (t_manifest_resource_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }

    PDEBUG("TABLE MANAGER:		Manifest resource shutting down\n");
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

JITBOOLEAN isPublic (t_row_manifest_resource_table *manifest) {

    /* Assertions	*/
    assert(manifest != NULL);

    return (manifest->flags & PUBLIC_MANIFEST) == PUBLIC_MANIFEST;

}

JITBOOLEAN isPrivate (t_row_manifest_resource_table *manifest) {

    /* Assertions	*/
    assert(manifest != NULL);

    return (manifest->flags & PRIVATE_MANIFEST) == PRIVATE_MANIFEST;

}

JITINT32 decodeImplementationIndex (t_row_manifest_resource_table *manifest, JITINT32 *destination_table) {

    /* Assertions	*/
    assert(manifest != NULL);

    JITINT32 coded_index;
    JITINT32 decoded_index;
    JITINT32 i;
    JITINT32 table[2];

    /* Initialize the variables */
    coded_index = 0;

    coded_index = manifest->implementation;
    assert(coded_index > 0);

    /* The reference to the right table is coded in to the two low significant bits	*/
    for (i = 0; i < 2; i++) {
        table[i] = coded_index % 2;
        assert((table[i] == 0) || (table[i] == 1));
        coded_index = coded_index / 2;
    }

    /* The index is now decoded	*/
    decoded_index = coded_index;


    /* Now discover in which table points
     * FIXME trasformare i valori in macro. dove metterli?
     * We return into destination_table parameter the table which points
     *
     * From the standard we know that if the value is:
     *      value | table           | destination table value
     *      --------------------------------------------------
     *      00      File		  1
     *      01      AssemblyRef	  2
     *      10      ExportedType	  3
     */
    if ((table[0] == 0) && (table[1] == 0)) {
        *destination_table = 1;
    } else if ((table[0] == 1) && (table[1] == 0)) {
        *destination_table = 2;
    } else if ((table[0] == 0) && (table[1] == 1)) {
        *destination_table = 3;
    } else {
        print_err("ERROR: The destination table is not valid. ", 0);
    }

    return decoded_index;
}
