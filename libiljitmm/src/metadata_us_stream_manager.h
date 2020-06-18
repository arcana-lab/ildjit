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
   *@file metadata_us_stream_manager.h
 */

#ifndef METADATA_US_STREAM_MANAGER_H
#define METADATA_US_STREAM_MANAGER_H

// My headers
#include <metadata/streams/metadata_streams_manager.h>
// End

#define MAX_STRING_LENGTH 1024

/**
 * @brief Rows of US stream
 *
 * Structure to fill the informations about one row of the US metadata stream
 */
typedef struct {
    JITINT8         *string;        /**< Pointer to the string into the #US stream. DO NOT MODIFY the content of the stream. */
    JITUINT32 offset;               /**< Offset from the begin of the String stream to the begin of this single string      */
    JITUINT32 bytesLength;          /**< Bytes of the string								*/
} t_row_us_stream;

/**
 * @brief US stream
 *
 * The US stream is fill inside a table; this structure has the table and some other informations about this stream.
 */
typedef struct {
    JITUINT32 offset;               /**< Offset from the begin of the file	*/
    JITUINT32 size;
    JITINT8 *       raw_stream;     /**< The #US stream, dumped from the file */
} t_us_stream;

/**
 * @brief Read the US stream
 *
 * Read the US stream of the assembly referenced by the binary parameter and fill all the information about it into the structure referenced by the us_stream pointer
 */
JITINT32 read_us_stream (t_us_stream *us_stream, t_binary *binary);

/**
 * @brief Delete the US stream
 *
 * Delete all the rows of the table used to fill up the US stream
 */
void shutdown_us_stream (t_us_stream *stream);

void get_us_row (t_us_stream *stream, JITUINT32 offset, t_row_us_stream *temp);

#endif
