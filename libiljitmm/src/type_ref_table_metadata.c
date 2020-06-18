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
   *@file type_ref_table_metadata.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata/tables/metadata_table_manager.h>
#include <metadata_streams_manager.h>
#include <decoder.h>
#include <mtm_utils.h>
#include <module_ref_table_metadata.h>
#include <iljitmm-system.h>
// End

JITINT32 load_type_ref_table (t_type_ref_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;
    JITUINT32 temp_resolution_scope;
    JITBOOLEAN more_than_16k_in_table;
    JITBOOLEAN resolution_scope_found;
    JITINT8 field_length;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != 0);
    if ( ((valid >> TYPE_REF_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    temp_resolution_scope = 0;
    more_than_16k_in_table = JITFALSE;
    resolution_scope_found = JITFALSE;

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Type ref table\n");
    bytes_read      = 0;
    table->table    = (t_row_type_ref_table **) sharedAllocFunction(sizeof(t_row_type_ref_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_type_ref_table *temp_row;
        temp_row                        = (t_row_type_ref_table *) sharedAllocFunction(sizeof(t_row_type_ref_table));
        temp_row->index                 = cardinality + 1;                // The index of the table starts at one value
        temp_row->binary = binary;

        field_length = 0;

        /* Early decoding of the value, to discover the table it points */
        if (((t_module_table *) private_get_table(NULL, MODULE_TABLE))->cardinality > METADATA_TABLE_LIMIT_14_BITS_ENCODING) {
            PDEBUG("TABLE MANAGER:	using 4 byte index for ResolutionScope, Module table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, MODULE_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 4, &(temp_resolution_scope));
            more_than_16k_in_table = JITTRUE;
            field_length = 4;
        } else {
            PDEBUG("TABLE MANAGER:	using 2 byte index for ResolutionScope, Module table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, MODULE_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 2, &(temp_resolution_scope));
            more_than_16k_in_table = JITFALSE;
            field_length = 2;
        }
        /* Masking last two bits to early decode the field */
        temp_resolution_scope = temp_resolution_scope & 0x00000003;
        /* Checking where the Resolution scope field points */
        if (temp_resolution_scope == 0) {
            PDEBUG("TABLE MANAGER:	ResolutionScope refers to the Module table\n");
            resolution_scope_found = JITTRUE;
            if (more_than_16k_in_table) {
                read_from_buffer(table_buf, bytes_read, 4, &(temp_row->resolution_scope));
            } else {
                read_from_buffer(table_buf, bytes_read, 2, &(temp_row->resolution_scope));
            }
        }
        if (!resolution_scope_found) {
            if (((t_module_ref_table *) private_get_table(NULL, MODULE_REF_TABLE))->cardinality > METADATA_TABLE_LIMIT_14_BITS_ENCODING) {
                PDEBUG("TABLE MANAGER:	using 4 byte index for ResolutionScope, ModuleRef table cardinality is %d\n", ((t_module_ref_table *) private_get_table(NULL, MODULE_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 4, &(temp_resolution_scope));
                more_than_16k_in_table = JITTRUE;
                field_length = MAX(field_length, 4);
            } else {
                PDEBUG("TABLE MANAGER:	using 2 byte index for ResolutionScope, ModuleRef table cardinality is %d\n", ((t_module_ref_table *) private_get_table(NULL, MODULE_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 2, &(temp_resolution_scope));
                more_than_16k_in_table = JITFALSE;
                field_length = MAX(field_length, 2);
            }
            /* Masking last two bits to early decode the field */
            temp_resolution_scope = temp_resolution_scope & 0x00000003;
            if (temp_resolution_scope == 1) {
                PDEBUG("TABLE MANAGER:	ResolutionScope refers to the ModuleRef table\n");
                resolution_scope_found = JITTRUE;
                if (more_than_16k_in_table) {
                    read_from_buffer(table_buf, bytes_read, 4, &(temp_row->resolution_scope));
                } else {
                    read_from_buffer(table_buf, bytes_read, 2, &(temp_row->resolution_scope));
                }
            }
        }
        if (!resolution_scope_found) {
            if (((t_assembly_ref_table *) private_get_table(NULL, ASSEMBLY_REF_TABLE))->cardinality > METADATA_TABLE_LIMIT_14_BITS_ENCODING) {
                PDEBUG("TABLE MANAGER:	using 4 byte index for ResolutionScope, AssemblyRef table cardinality is %d\n", ((t_assembly_ref_table *) private_get_table(NULL, ASSEMBLY_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 4, &(temp_resolution_scope));
                more_than_16k_in_table = JITTRUE;
                field_length = MAX(field_length, 4);
            } else {
                PDEBUG("TABLE MANAGER:	using 2 byte index for ResolutionScope, AssemblyRef table cardinality is %d\n", ((t_assembly_ref_table *) private_get_table(NULL, ASSEMBLY_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 2, &(temp_resolution_scope));
                more_than_16k_in_table = JITFALSE;
                field_length = MAX(field_length, 2);
            }
            /* Masking last two bits to early decode the field */
            temp_resolution_scope = temp_resolution_scope & 0x00000003;
            if (temp_resolution_scope == 2) {
                PDEBUG("TABLE MANAGER:	ResolutionScope refers to the AssemblyRef table\n");
                resolution_scope_found = JITTRUE;
                if (more_than_16k_in_table) {
                    read_from_buffer(table_buf, bytes_read, 4, &(temp_row->resolution_scope));
                } else {
                    read_from_buffer(table_buf, bytes_read, 2, &(temp_row->resolution_scope));
                }
            }
        }
        if (!resolution_scope_found) {
            if (((t_type_ref_table *) private_get_table(NULL, TYPE_REF_TABLE))->cardinality > METADATA_TABLE_LIMIT_14_BITS_ENCODING) {
                PDEBUG("TABLE MANAGER:	using 4 byte index for ResolutionScope, TypeRef table cardinality is %d\n", ((t_type_ref_table *) private_get_table(NULL, TYPE_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 4, &(temp_resolution_scope));
                more_than_16k_in_table = JITTRUE;
                field_length = MAX(field_length, 4);
            } else {
                PDEBUG("TABLE MANAGER:	using 2 byte index for ResolutionScope, TypeRef table cardinality is %d\n", ((t_type_ref_table *) private_get_table(NULL, TYPE_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 2, &(temp_resolution_scope));
                more_than_16k_in_table = JITFALSE;
                field_length = MAX(field_length, 2);
            }
            /* Masking last two bits to early decode the field */
            temp_resolution_scope = temp_resolution_scope & 0x00000003;
            if (temp_resolution_scope == 3) {
                PDEBUG("TABLE MANAGER:	ResolutionScope refers to the TypeRef table\n");
                resolution_scope_found = JITTRUE;
                if (more_than_16k_in_table) {
                    read_from_buffer(table_buf, bytes_read, 4, &(temp_row->resolution_scope));
                } else {
                    read_from_buffer(table_buf, bytes_read, 2, &(temp_row->resolution_scope));
                }
            }
        }
        if (!resolution_scope_found) {
            /* TODO: should we abort here? */
            PDEBUG("TABLE MANAGER:	error with the ResolutionScope field, aborting\n");
            abort();
        }
        bytes_read += field_length;
        resolution_scope_found = JITFALSE;
        /* Saving Name and NameSpace fields */
        if (string_index_is_32(heap_size)) {
            PDEBUG("TABLE MANAGER:	using 4 byte index for Name and NameSpace\n");
            // Index of string stream is 32 bit
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->type_name));
            read_from_buffer(table_buf, bytes_read + 4, 4, &(temp_row->type_name_space));
            bytes_read += 8;
        } else {
            PDEBUG("TABLE MANAGER:	using 2 byte index for Name and NameSpace\n");
            // Index of string stream is 16 bit
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->type_name));
            read_from_buffer(table_buf, bytes_read + 2, 2, &(temp_row->type_name_space));
            bytes_read += 4;
        }
        table->table[cardinality] = temp_row;
    }

#ifdef PRINTDEBUG
    JITINT32 count;
    PDEBUG("			TYPE REF TABLE		Cardinality = %d\n", table->cardinality);
    PDEBUG("Index		Resolution scope	Type name	Type name space\n");
    PDEBUG("					(String index)	(String index)\n");
    for (count = 0; count < (table->cardinality); count++) {
        t_row_type_ref_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		0x%X			%d		%d\n", (count + 1), temp->resolution_scope, temp->type_name, temp->type_name_space);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_type_ref_table (t_type_ref_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }
    PDEBUG("TABLE MANAGER:		Type ref shutting down\n");
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
