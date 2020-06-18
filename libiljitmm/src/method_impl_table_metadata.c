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
   *@file method_impl_table_metadata.c
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

#define METHOD_DEF_OR_REF_FLAG_MASK 0x00000001

JITINT32 load_method_impl_table (t_method_impl_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes) {
    JITUINT32 cardinality;
    JITUINT32 bytes_read;

    JITUINT32 temp_method_body;
    JITUINT32 temp_method_declaration;
    JITBOOLEAN index_decoded;
    JITBOOLEAN table_threshold_exceeded;
    JITUINT8 max_increment;

    /* Assertions	*/
    assert(bytes != 0);
    assert(table != NULL);
    assert(row_buf != NULL);
    if ( ((valid >> METHOD_IMPL_TABLE) & 0x1 ) == 0) {
        *bytes = 0;
        return 0;
    }
    assert(table->cardinality != 0);

    temp_method_body = 0;
    temp_method_declaration = 0;
    index_decoded = JITFALSE;
    table_threshold_exceeded = JITFALSE;

    /* Create the table	*/
    PDEBUG("TABLE MANAGER:	Method impl table\n");
    bytes_read      = 0;
    table->table    = (t_row_method_impl_table **) sharedAllocFunction(sizeof(t_row_method_impl_table *) * (table->cardinality));
    assert(table->table != NULL);

    /* Fill up the table	*/
    for (cardinality = 0; cardinality < (table->cardinality); cardinality++) {
        t_row_method_impl_table *temp_row;
        temp_row                        = (t_row_method_impl_table *) sharedAllocFunction(sizeof(t_row_method_impl_table));
        temp_row->index                 = cardinality + 1;        //Index with initial value equal to one
        temp_row->binary = binary;
        if (    (((t_type_def_table *) private_get_table(NULL, TYPE_DEF_TABLE))->cardinality > METADATA_TABLE_LIMIT_16_BITS_ENCODING)) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->classIndex));
            bytes_read += 4;
        } else {
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->classIndex));
            bytes_read += 2;
        }

        /* Resetting local vars */
        temp_method_body = 0;
        index_decoded = JITFALSE;
        table_threshold_exceeded = JITFALSE;
        max_increment = 0;

        /* Early decoding */
        if (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality > METADATA_TABLE_LIMIT_15_BITS_ENCODING) {
            PDEBUG("TABLE MANAGER:	using 4 byte index for MethodDeforRef, MethodDef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 4, &(temp_method_body));
            table_threshold_exceeded = JITTRUE;
            max_increment = 4;
        } else {
            PDEBUG("TABLE MANAGER:	using 2 byte index for MethodDeforRef, MethodDef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 2, &(temp_method_body));
            table_threshold_exceeded = JITFALSE;
            max_increment = 2;
        }

        temp_method_body = temp_method_body & METHOD_DEF_OR_REF_FLAG_MASK;
        if (temp_method_body == 0) {
            PDEBUG("METHOD_IMPL_TABLE: The MethodBody refers to a MethodDef\n");
            index_decoded = JITTRUE;
            if (table_threshold_exceeded) {
                PDEBUG("METHOD_IMPL_TABLE: Using 4 byte index for MethodBody\n");
                read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_body));
            } else {
                PDEBUG("METHOD_IMPL_TABLE: Using 2 byte index for MethodBody\n");
                temp_row->method_body = 0;
                read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_body));
            }
        }
        if (!index_decoded) {
            if (((t_method_def_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality > METADATA_TABLE_LIMIT_15_BITS_ENCODING) {
                PDEBUG("TABLE MANAGER:	using 4 byte index for MethodDeforRef, MemberRef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 4, &(temp_method_body));
                table_threshold_exceeded = JITTRUE;
                max_increment = MAX(max_increment, 4);
            } else {
                PDEBUG("TABLE MANAGER:	using 2 byte index for MethodDeforRef, MemberRef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 2, &(temp_method_body));
                table_threshold_exceeded = JITFALSE;
                max_increment = MAX(max_increment, 2);
            }
            temp_method_body = temp_method_body & METHOD_DEF_OR_REF_FLAG_MASK;
            if (temp_method_body == 1) {
                PDEBUG("METHOD_IMPL_TABLE: The MethodBody refers to a MemberRef\n");
                index_decoded = JITTRUE;
                if (table_threshold_exceeded) {
                    PDEBUG("METHOD_IMPL_TABLE: Using 4 byte index for MethodBody\n");
                    read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_body));
                } else {
                    PDEBUG("METHOD_IMPL_TABLE: Using 2 byte index for MethodBody\n");
                    temp_row->method_body = 0;
                    read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_body));
                }
            }
        }
        /* If the MethodBody index cannot be decoded, generate an error here */
        if (!index_decoded) {
            print_err("TABLE MANAGER: ERROR= Failed decoding the MethodBody reference. ", errno);
            return -1;
        }
        bytes_read += max_increment;

#if 0
        if (    (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality > 32768)        ||
                (((t_member_ref_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality > 32768)) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_body));
            bytes_read += 4;
        } else {
            temp_row->method_body = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_body));
            bytes_read += 2;
        }
#endif

        /* Resetting local vars */
        temp_method_declaration = 0;
        index_decoded = JITFALSE;
        table_threshold_exceeded = JITFALSE;
        max_increment = 0;

        /* Early decoding */
        if (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality > METADATA_TABLE_LIMIT_15_BITS_ENCODING) {
            PDEBUG("TABLE MANAGER:	using 4 byte index for MethodDeforRef, MethodDef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 4, &(temp_method_declaration));
            table_threshold_exceeded = JITTRUE;
            max_increment = 4;
        } else {
            PDEBUG("TABLE MANAGER:	using 2 byte index for MethodDeforRef, MethodDef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality );
            read_from_buffer(table_buf, bytes_read, 2, &(temp_method_declaration));
            table_threshold_exceeded = JITFALSE;
            max_increment = 2;
        }

        temp_method_declaration = temp_method_declaration & METHOD_DEF_OR_REF_FLAG_MASK;
        if (temp_method_declaration == 0) {
            PDEBUG("METHOD_IMPL_TABLE: The MethodDeclaration refers to a MethodDef\n");
            index_decoded = JITTRUE;
            if (table_threshold_exceeded) {
                PDEBUG("METHOD_IMPL_TABLE: Using 4 byte index for MethodDeclaration\n");
                read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_declaration));
            } else {
                PDEBUG("METHOD_IMPL_TABLE: Using 2 byte index for MethodDeclaration\n");
                temp_row->method_declaration = 0;
                read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_declaration));
            }
        }
        if (!index_decoded) {
            if (((t_method_def_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality > METADATA_TABLE_LIMIT_15_BITS_ENCODING) {
                PDEBUG("TABLE MANAGER:	using 4 byte index for MethodDeforRef, MemberRef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 4, &(temp_method_declaration));
                table_threshold_exceeded = JITTRUE;
                max_increment = MAX(max_increment, 4);
            } else {
                PDEBUG("TABLE MANAGER:	using 2 byte index for MethodDeforRef, MemberRef table cardinality is %d\n", ((t_module_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality );
                read_from_buffer(table_buf, bytes_read, 2, &(temp_method_declaration));
                table_threshold_exceeded = JITFALSE;
                max_increment = MAX(max_increment, 2);
            }
            temp_method_declaration = temp_method_declaration & METHOD_DEF_OR_REF_FLAG_MASK;
            if (temp_method_declaration == 1) {
                PDEBUG("METHOD_IMPL_TABLE: The MethodDeclaration refers to a MemberRef\n");
                index_decoded = JITTRUE;
                if (table_threshold_exceeded) {
                    PDEBUG("METHOD_IMPL_TABLE: Using 4 byte index for MethodDeclaration\n");
                    read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_declaration));
                } else {
                    PDEBUG("METHOD_IMPL_TABLE: Using 2 byte index for MethodDeclaration\n");
                    temp_row->method_declaration = 0;
                    read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_declaration));
                }
            }
        }
        /* If the MethodBody index cannot be decoded, generate an error here */
        if (!index_decoded) {
            print_err("TABLE MANAGER: ERROR= Failed decoding the MethodDeclaration reference. ", errno);
            return -1;
        }
        bytes_read += max_increment;

#if 0
        if (    (((t_method_def_table *) private_get_table(NULL, METHOD_DEF_TABLE))->cardinality > 32768)        ||
                (((t_member_ref_table *) private_get_table(NULL, MEMBER_REF_TABLE))->cardinality > 32768)) {
            read_from_buffer(table_buf, bytes_read, 4, &(temp_row->method_declaration));
            bytes_read += 4;
        } else {
            temp_row->method_declaration = 0;
            read_from_buffer(table_buf, bytes_read, 2, &(temp_row->method_declaration));
            bytes_read += 2;
        }
#endif


        table->table[cardinality] = temp_row;
    }

#ifdef PRINTDEBUG
    JITINT32 count;
    PDEBUG("			METHOD IMPL TABLE	Cardinality = %d\n", cardinality);
    PDEBUG("Index		Class	Method body	Method declaration\n");
    for (count = 0; count < cardinality; count++) {
        t_row_method_impl_table *temp;
        temp = table->table[count];
        assert(temp != NULL);
        PDEBUG("%d		%d	0x%X		0x%X\n", (count + 1), temp->classIndex, temp->method_body, temp->method_declaration);
    }
    PDEBUG("\n");
    PDEBUG("TABLE MANAGER:		Bytes read = %d\n", bytes_read);
#endif

    (*bytes) = bytes_read;
    return 0;
}

void shutdown_method_impl_table (t_method_impl_table *table) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(table != NULL);

    if (table->table == NULL) {
        return;
    }
    PDEBUG("TABLE MANAGER:		Method Impl shutting down\n");
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
