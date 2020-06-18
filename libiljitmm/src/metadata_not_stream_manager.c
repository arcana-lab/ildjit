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
   *@file metadata_not_stream_manager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <compiler_memory_manager.h>

// My headers
#include <decoder.h>
#include <metadata_streams_manager.h>
#include <metadata/tables/metadata_table_manager.h>
#include <iljitmm-system.h>
// End

#define DIM_BUF 2056

JITINT32 read_not_stream (t_not_stream *not_stream, void *void_binary_info) {
    JITINT32 number_of_tables;
    JITINT8         *buffer;
    t_binary_information *binary_info;
    t_binary *binary;

    /* Assertions	*/
    assert(not_stream != NULL);
    assert(void_binary_info != NULL);

    binary_info = (t_binary_information *) void_binary_info;
    binary = &(binary_info->binary);

    number_of_tables = 0;
    PDEBUG("METADATA STREAM MANAGER:		Not stream\n");
    if (not_stream->size == 0) {
        PDEBUG("METADATA STREAM MANAGER:			Not stream is empty\n");
        return 0;
    }
    buffer = (JITINT8 *) allocFunction(sizeof(char) * DIM_BUF);
    assert(not_stream->offset != 0);

    /* Print the offset			*/
#ifdef PRINTDEBUG
    PDEBUG("METADATA STREAM MANAGER:			Current file offset\n");
    print_binary_offset(*binary);
    PDEBUG("METADATA STREAM MANAGER:			Not stream offset = %d\n", not_stream->offset);
#endif

    /* Unroll the file			*/
    if (((*(binary->reader))->offset) > (not_stream->offset)) {
        unroll_file(binary);
        PDEBUG("METADATA STREAM MANAGER:				Unroll the file\n");
    }

    /* Seek to the begin of the Not stream	*/
    if (seek_within_file(binary, buffer, (*(binary->reader))->offset, not_stream->offset) != 0) {
        print_err("METADATA STREAM MANAGER:			ERROR= Cannot seek to the set stream metadata. ", 0);
        freeFunction(buffer);
        return -1;
    }
#ifdef PRINTDEBUG
    PDEBUG("METADATA STREAM MANAGER:			Seek to the %d relative offset\n", not_stream->offset);
    print_binary_offset(*binary);
#endif

    /* Read all the Not stream		*/
    if (DIM_BUF < (not_stream->size)) {
        buffer = dynamicReallocFunction(buffer, ((not_stream->size) * sizeof(char)) );
    }
    if (il_read(buffer, not_stream->size, binary) != 0) {
        print_err("METADATA STREAM MANAGER:                     ERROR= Cannot read from file. ", 0);
        freeFunction(buffer);
        return -1;
    }

    PDEBUG("METADATA STREAM MANAGER:			Read %d bytes for the Not stream\n", not_stream->size);

    read_from_buffer(buffer, 0, 4, &(not_stream->reserved));
    read_from_buffer(buffer, 4, 1, &(not_stream->mayor_version));
    read_from_buffer(buffer, 5, 1, &(not_stream->minor_version));
    read_from_buffer(buffer, 6, 1, &(not_stream->heap_size));
    read_from_buffer(buffer, 7, 1, &(not_stream->reserved_2));
    read_from_buffer(buffer, 8, 8, &(not_stream->valid));
    read_from_buffer(buffer, 16, 8, &(not_stream->sorted));

    PDEBUG("METADATA STREAM MANAGER:			Reserved			=	0x%X		%d\n", (JITUINT32) not_stream->reserved, not_stream->reserved);
    PDEBUG("METADATA STREAM MANAGER:			Version				=	0x(%X.%X)		%d.%d\n", (JITUINT32) not_stream->mayor_version, (JITUINT32) not_stream->minor_version, (JITUINT32) not_stream->mayor_version, (JITUINT32) not_stream->minor_version);
    PDEBUG("METADATA STREAM MANAGER:			Heap size			=	0x%X		%d\n", (JITUINT32) not_stream->heap_size, not_stream->heap_size);
    PDEBUG("METADATA STREAM MANAGER:			Reserved 2			=	0x%X		%d\n", (JITUINT32) not_stream->reserved_2, not_stream->reserved_2);
    PDEBUG("METADATA STREAM MANAGER:			Valid				=	0x%llX		%lld\n", not_stream->valid, not_stream->valid);
    PDEBUG("METADATA STREAM MANAGER:			Sorted				=	0x%llX		%lld\n", not_stream->sorted, not_stream->sorted);
    {
        JITINT32 count;
        JITUINT64 valid_temp;
        for (count = 0, valid_temp = not_stream->valid; count < 64; count++) {
            if ((valid_temp & 0x01) != 0) {
                number_of_tables++;
            }
            valid_temp = valid_temp >> 1;
        }
    }
    PDEBUG("METADATA STREAM MANAGER:			Number of tables = %d\n", number_of_tables);
    if (load_metadata_tables(&(not_stream->tables), binary_info, not_stream->valid, not_stream->heap_size, buffer + 24, buffer + 24 + (number_of_tables * 4)) != 0) {
        freeFunction(buffer);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(*binary);
#endif
    freeFunction(buffer);
    return 0;
}

void shutdown_not_stream (t_not_stream *stream) {
    assert(stream != NULL);
    PDEBUG("METADATA STREAM MANAGER:	Not stream shutting down\n");
    shutdown_tables_manager(&(stream->tables));
    PDEBUG("METADATA STREAM MANAGER:	Done\n");
}
