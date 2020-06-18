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
   *@file metadata_string_stream_manager.c
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata_streams_manager.h>
#include <jitsystem.h>
#include <iljitmm-system.h>
// End

#define DIM_BUF                         2056
#define STRING_INITIAL_TABLE_LENGTH     10
#define STRING_STEP_TABLE_LENGTH        10

JITINT32 read_string_stream (t_string_stream * string_stream, t_binary *binary) {
    JITINT8                 *buffer;

    /* Assertions	*/
    assert(string_stream->offset != 0);
    buffer = (JITINT8 *) allocFunction(sizeof(JITINT8) * DIM_BUF);

    /* Unroll the file			*/
    if (((*(binary->reader))->offset) > (string_stream->offset)) {
        unroll_file(binary);
        PDEBUG("METADATA STREAM MANAGER:				Unroll the file\n");
    }

    /* Seek to the begin of the String stream	*/
    PDEBUG("METADATA STREAM MANAGER:			Seek to the %d relative offset\n", string_stream->offset);
    if (seek_within_file(binary, buffer, (*(binary->reader))->offset, string_stream->offset) != 0) {
        print_err("METADATA STREAM MANAGER:	ERROR= Cannot seek to the string stream metadata. ", 0);
        freeFunction(buffer);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(*binary);
#endif

    /* Allocate the space for the stream in memory */
    string_stream->raw_stream = (JITINT8 *) sharedAllocFunction(string_stream->size);

    /* Read the entire stream			*/
    PDEBUG("METADATA STREAM MANAGER:			Read %d bytes for the string stream\n", string_stream->size);
    if (il_read(string_stream->raw_stream, string_stream->size, binary) != 0) {
        print_err("METADATA STREAM MANAGER:     ERROR= Cannot read from file. ", 0);
        freeFunction(buffer);
        return -1;
    }

    /* Check the first BLOB, which has to   *
     * be constant				*/
    if (string_stream->raw_stream[0] != 0) {
        print_err("METADATA STREAM MANAGER: ERROR= The first string is not equal to 0x00 as the ECMA 335 standard prescribes. ", 0);
        freeFunction(buffer);
        return -1;
    }

    freeFunction(buffer);

    return 0;
}

void shutdown_string_stream (t_string_stream *stream) {

    /* Assertions		*/
    assert(stream != NULL);

    /* Delete the stream */
    if (stream->raw_stream != NULL) {
        freeFunction(stream->raw_stream);
    }
}

JITUINT32 getStringLength (JITINT8 *stringStream) {
    JITUINT32 length;

    length = 0;
    while (stringStream[length] != STRING_END_CHARACTER) {
        length++;
    }

    return length;
}

JITINT8 * get_string (t_string_stream *stream, JITUINT32 offset) {
    JITINT8                 *string_start;

    /* Assertions			*/
    assert(stream != NULL);
    assert(offset >= 0);

    /* Check the trivial conditions	*/
    if (offset == 0) {
        return (JITINT8 *) "\0";
    }


    /* Initialization */
    string_start = stream->raw_stream + offset;

    return string_start;
}
