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
   *@file metadata_blob_stream_manager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <compiler_memory_manager.h>

// My headers
#include <metadata_streams_manager.h>
#include <iljitmm-system.h>
#include <mtm_utils.h>
// End

#define DIM_BUF                         2056
#define BLOB_INITIAL_TABLE_LENGTH       10
#define BLOB_STEP_TABLE_LENGTH          10

JITINT32 read_blob_stream (t_blob_stream *blob_stream, t_binary *binary) {
    JITINT8         *buffer;

    /* Assertions	*/
    assert(blob_stream != NULL);
    assert(binary != NULL);

    PDEBUG("METADATA STREAM MANAGER:		Blob stream\n");
    if (blob_stream->size == 0) {
        blob_stream->raw_stream = NULL;
        PDEBUG("METADATA STREAM MANAGER:			Blob string stream is empty\n");
        return 0;
    }
    buffer = (JITINT8 *) allocFunction(DIM_BUF);

    /* Print the offset			*/
    assert(blob_stream->offset != 0);
#ifdef PRINTDEBUG
    PDEBUG("METADATA STREAM MANAGER:			Current file offset\n");
    print_binary_offset(*binary);
    PDEBUG("METADATA STREAM MANAGER:			Blob stream offset = %d\n", blob_stream->offset);
#endif

    /* Unroll the file			*/
    unroll_file(binary);

    /* Seek to the begin of the Blob stream	*/
    if (seek_within_file(binary, buffer, (*(binary->reader))->offset, blob_stream->offset) != 0) {
        print_err("METADATA STREAM MANAGER:			ERROR= Cannot seek to the blob stream metadata. ", 0);
        freeFunction(buffer);
        return -1;
    }
#ifdef PRINTDEBUG
    PDEBUG("METADATA STREAM MANAGER:			Seek to the %d relative offset\n", blob_stream->offset);
    print_binary_offset(*binary);
#endif

    /* Allocate the space for the stream in memory */
    blob_stream->raw_stream = (JITINT8 *) sharedAllocFunction(blob_stream->size);

    /* Read all the Blob stream		*/
    if (il_read(blob_stream->raw_stream, blob_stream->size, binary) != 0) {
        print_err("METADATA STREAM MANAGER: ERROR= Cannot read from file. ", 0);
        freeFunction(buffer);
        return -1;
    }
    PDEBUG("METADATA STREAM MANAGER:			Read %d bytes for the Blob stream\n", blob_stream->size);

    /* Check the first BLOB, which has to   *
     * be constant				*/
    if (blob_stream->raw_stream[0] != 0) {
        print_err("METADATA STREAM MANAGER: ERROR= The first blob is not equal to 0x00 as the ECMA 335 standard prescribes. ", 0);
        freeFunction(buffer);
        return -1;
    }

    freeFunction(buffer);

    return 0;
}

void shutdown_blob_stream (t_blob_stream *stream) {
    PDEBUG("METADATA STREAM MANAGER:	BLOB stream shutting down\n");

    /* Delete the stream */
    if (stream->raw_stream != NULL) {
        freeFunction(stream->raw_stream);
    }

    /* Return		*/
    return;
}

void get_blob (t_blob_stream *stream, JITUINT32 offset, t_row_blob_table *row) {
    JITINT8 *blob_start;
    JITUINT32 bytesToRead;
    JITUINT32 bytesRead;

    assert(stream != NULL);
    assert(offset >= 0);
    assert(row != NULL);

    /* Initialization */
    blob_start = stream->raw_stream + offset;
    bytesRead = 0;
    bytesToRead = 0;
    row->blob = NULL;

    /* Compute the bytes to read	*/
    bytesToRead = getBlobLength(blob_start, &bytesRead);
    assert(bytesToRead >= 0);

    /* Read the blob		*/
    if (bytesToRead > 0) {
        row->blob = blob_start + bytesRead;
        row->size = bytesToRead;
        row->offset = offset;
        PDEBUG("METADATA STREAM MANAGER:	Current blob size 0x%X, offset 0x%X\n", bytesToRead, offset);
    } else {
        PDEBUG("METADATA STREAM MANAGER:		Empty blob found\n");
    }

    return;
}
