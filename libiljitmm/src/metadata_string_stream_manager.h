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
   *@file metadata_string_stream_manager.h
 */

#ifndef METADATA_STRING_STREAM_MANAGER_H
#define METADATA_STRING_STREAM_MANAGER_H

// My headers
#include <metadata/streams/metadata_streams_manager.h>
#include <jitsystem.h>
// End

#define MAX_STRING_LENGTH 1024

/**
 * @brief Rows of String stream
 *
 * Structure to fill the informations about one row of the String metadata stream
 */
typedef struct {
    char string[MAX_STRING_LENGTH];
    JITUINT32 offset;               /**< Offset from the begin of the String stream to the begin of this single string */
} t_row_string_stream;

/**
 * @brief String stream
 *
 * The String stream is fill inside a table; this structure has the table and some other informations about this stream.
 */
typedef struct {
    JITUINT32 offset;                               /**< Offset from the begin of the file	*/
    JITUINT32 size;
    JITINT8                         *raw_stream;
} t_string_stream;

/**
 * @brief Read the String stream
 *
 * Read the String stream of the assembly referenced by the binary parameter and fill all the information about it into the structure referenced by the string_stream pointer
 */
JITINT32 read_string_stream (t_string_stream * string_stream, t_binary *binary);

/**
 * @brief Delete the String stream
 *
 * Delete all the rows of the table used to fill up the String stream
 */
void shutdown_string_stream (t_string_stream *stream);

/**
 *
 * Fetch and return a string with offset from the start String stream equal to the value of the parameter offet.
 */
JITINT8 * get_string (t_string_stream *stream, JITUINT32 offset);

#endif
