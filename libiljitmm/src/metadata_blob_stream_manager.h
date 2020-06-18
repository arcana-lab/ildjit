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
   *@file metadata_blob_stream_manager.h
 */

#ifndef METADATA_BLOB_STREAM_MANAGER_H
#define METADATA_BLOB_STREAM_MANAGER_H

#include <iljit-utils.h>
#include <jitsystem.h>

/**
 * @brief Rows of Blob stream
 *
 * Structure to fill the informations about one row of the Blob metadata stream
 */
typedef struct {
    JITINT8         *blob;
    JITUINT32 size;
    JITUINT32 offset;
} t_row_blob_table;

/**
 * @brief Blob stream
 *
 * The Blob stream is fill inside a table; this structure has the table and some other informations about this stream.
 */
typedef struct {
    JITUINT32 offset;                       /**< Offset from the begin of the file	*/
    JITUINT32 size;
    JITINT8 *               raw_stream;     /**< The #Blob stream, dumped from the file */
} t_blob_stream;

/**
 * @brief Read the Blob stream
 *
 * Read the blob stream of the assembly referenced by the binary parameter and fill all the information about it into the structure referenced by the blob_stream pointer
 */
JITINT32 read_blob_stream (t_blob_stream *blob_stream, t_binary *binary);

/**
 * @brief Delete the blob stream
 *
 * Delete all the rows of the table used to fill up the Blob stream
 */
void shutdown_blob_stream (t_blob_stream *stream);

/**
 *
 * Fetch and return a blob with offset from the start Blob stream equal to the value of the parameter offet.
 */
void  get_blob (t_blob_stream *stream, JITUINT32 offset, t_row_blob_table *row);

#endif
