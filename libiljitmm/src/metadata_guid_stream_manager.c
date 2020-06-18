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
   *@file metadata_guid_stream_manager.c
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata_guid_stream_manager.h>
#include <iljitmm-system.h>
// End

#define DIM_BUF                         2056
#define GUID_INITIAL_TABLE_LENGTH       10
#define GUID_STEP_TABLE_LENGTH          10

JITINT32 read_guid_string_stream (t_guid_stream *guid_stream, t_binary *binary) {
    JITINT8         *buffer;
    JITINT32 bytes_read;
    JITINT32 counter;
    JITINT32 buffer_dimension;

    /* Assertions				*/
    assert(guid_stream != NULL);
    assert(binary != NULL);

    PDEBUG("METADATA STREAM MANAGER:		GUID string stream\n");
    if (guid_stream->size == 0) {
        PDEBUG("METADATA STREAM MANAGER:			GUID string stream is empty\n");
        guid_stream->tableLength = 0;
        return 0;
    }
    assert(guid_stream->offset != 0);

    /* Allocate the buffer			*/
    buffer = (JITINT8 *) allocFunction(sizeof(JITINT8) * DIM_BUF);
    buffer_dimension = DIM_BUF;

    /* Set the table length			*/
    guid_stream->tableLength = GUID_INITIAL_TABLE_LENGTH;

    /* Unrool the file			*/
    if (((*(binary->reader))->offset) > (guid_stream->offset)) {
        unroll_file(binary);
        PDEBUG("METADATA STREAM MANAGER:				Unroll the file\n");
    }

    /* Seek to the file			*/
    if ((guid_stream->offset - (*(binary->reader))->offset) > buffer_dimension) {
        buffer                  = (JITINT8 *) dynamicReallocFunction(buffer, guid_stream->offset - (*(binary->reader))->offset + 1);
        buffer_dimension        = guid_stream->offset - (*(binary->reader))->offset + 1;
    }
    if (seek_within_file(binary, buffer, (*(binary->reader))->offset, guid_stream->offset) != 0) {
        print_err("METADATA STREAM MANAGER:			ERROR= Cannot seek to the GUID stream metadata. ", 0);
        freeFunction(buffer);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(*binary);
    PDEBUG("METADATA STREAM MANAGER:			Seek to the %d relative offset\n", guid_stream->offset);
#endif
    if (guid_stream->size > buffer_dimension) {

        /* The last read from the buffer can overflow of 2 bytes from the dimension of the buffer	*/
        buffer_dimension = guid_stream->size + 2 + 1;
        buffer = (JITINT8 *) dynamicReallocFunction(buffer, buffer_dimension);
    }
    if (il_read(buffer, guid_stream->size, binary) != 0) {
        print_err("METADATA STREAM MANAGER:                     ERROR= Cannot read from file. ", 0);
        freeFunction(buffer);
        return -1;
    }
    PDEBUG("METADATA STREAM MANAGER:			Read %d bytes for the GUID stream\n", guid_stream->size);

    /* Create the GUID table	*/
    guid_stream->table      =       (t_row_guid_stream **) sharedAllocFunction(guid_stream->tableLength * sizeof(t_row_guid_stream *));
    assert(guid_stream != NULL);
    memset(guid_stream->table, 0, sizeof(t_row_guid_stream *) * guid_stream->tableLength);

    /* Fill up the table		*/
    counter = bytes_read = 0;
    while (bytes_read < guid_stream->size) {
        t_row_guid_stream *current;
        current = (t_row_guid_stream *) sharedAllocFunction(sizeof(t_row_guid_stream));
        assert((bytes_read + 2) < buffer_dimension);
        read_from_buffer(buffer, bytes_read, 2, &(current->guid));
        if (guid_stream->tableLength == counter) {
            (guid_stream->tableLength) += GUID_STEP_TABLE_LENGTH;
            guid_stream->table = (t_row_guid_stream **) dynamicReallocFunction(guid_stream->table, guid_stream->tableLength * sizeof(t_row_guid_stream *));
            assert(guid_stream->table != NULL);
            memset(guid_stream->table + counter, 0, sizeof(t_row_guid_stream *));
        }
        guid_stream->table[counter] = current;
        bytes_read += 2;
        counter++;
    }
    guid_stream->tableLength = counter;

#ifdef PRINTDEBUG
    PDEBUG("		GUID STREAM\n");
    PDEBUG(" Value\n");
    JITINT32 count;
    for (count = 0; count < counter; count++) {
        t_row_guid_stream *temp = guid_stream->table[count];
        assert(temp != NULL);
        PDEBUG("  0x%X\n", temp->guid);
    }
    print_binary_offset(*binary);
#endif

    /* Free the memory		*/
    freeFunction(buffer);

    /* Return			*/
    return 0;
}

void shutdown_guid_stream (t_guid_stream *stream) {
    JITINT32 count;
    JITINT32 size;

    /* Assertions		*/
    assert(stream != NULL);

    if (stream->table == NULL) {
        return;
    }

    PDEBUG("METADATA STREAM MANAGER:	GUID stream shutting down\n");
    size = stream->tableLength;
    PDEBUG("METADATA STREAM MANAGER:		Rows=%d\n", size);

    /* Delete rows		*/
    for (count = 0; count < size; count++) {
        freeFunction(stream->table[count]);
    }

    /* Delete the table	*/
    freeFunction(stream->table);
    PDEBUG("METADATA STREAM MANAGER:		Table deleted\n");
}
