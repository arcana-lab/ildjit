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
   *@file metadata_us_stream_manager.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata_streams_manager.h>
#include <iljitmm-system.h>
#include <mtm_utils.h>
// End

#define DIM_BUF                         2056
#define US_INITIAL_TABLE_LENGTH         10
#define US_STEP_TABLE_LENGTH            10

JITINT32 read_us_stream (t_us_stream *us_stream, t_binary *binary) {
    JITINT8         *buffer;

    buffer = (JITINT8 *) allocFunction(sizeof(char) * DIM_BUF);

    /* Allocate space for the #US stream */
    us_stream->raw_stream = NULL;
    if (us_stream->size > 0) {
        us_stream->raw_stream = sharedAllocFunction(us_stream->size);

        /* Position the file pointer to the beginning of the stream */
        if (unroll_file(binary) != 0) {
            print_err("METADATA STREAM MANAGER: read_us_stream: ERROR = Unable to unroll the binary. ", 0);
            freeFunction(buffer);
            return -1;
        }

        if (seek_within_file(binary, buffer, (*(binary->reader))->offset, us_stream->offset) != 0) {
            print_err("METADATA STREAM MANAGER:			ERROR= Cannot seek to the #US stream metadata. ", 0);
            freeFunction(buffer);
            return -1;
        }

        /* Dump the content of the stream to memory */
        if (il_read(us_stream->raw_stream, us_stream->size, binary) != 0) {
            print_err("METADATA STREAM MANAGER: read_us_stream: ERROR = Unable to dump the #US stream. ", 0);
            freeFunction(buffer);
            return -1;
        }

    } else {
        PDEBUG("METADATA STREAM MANAGER:			User string stream is empty\n");
    }

    freeFunction(buffer);

    return 0;
}

void shutdown_us_stream (t_us_stream *stream) {
    PDEBUG("METADATA STREAM MANAGER:	US stream shutting down\n");
    freeFunction(stream->raw_stream);
}

void get_us_row (t_us_stream *stream, JITUINT32 offset, t_row_us_stream *temp) {
    JITINT8 *blob_start;
    JITUINT32 bytesToRead;
    JITUINT32 bytesRead;

    /* Assertions	*/
    assert(stream != NULL);
    assert(offset <= stream->size);

    /* Initialization of some variables	*/
    blob_start = stream->raw_stream + offset;

    /* Extract the element */
    bytesToRead = getBlobLength(blob_start, &bytesRead);

    if (bytesToRead > 0) {
        /* Make a new UTF16 string				*/

        /* Store the string					*/
        temp->string = blob_start + bytesRead;
        temp->offset = offset;
        temp->bytesLength = bytesToRead - 1;
    } else {
        temp->string = NULL;
    }

}
