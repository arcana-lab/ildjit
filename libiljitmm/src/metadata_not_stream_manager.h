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
   *@file metadata_not_stream_manager.h
 */

#ifndef METADATA_NOT_STREAM_H
#define METADATA_NOT_STREAM_H

// My headers
#include <metadata/streams/metadata_streams_manager.h>
// End

/**
 * @brief Not stream
 *
 * Structure to fill the informations about the Not metadata stream
 */
typedef struct {
    JITUINT32 offset;                       /**< Offset from the begin of the file	*/
    JITUINT32 size;
    JITUINT32 reserved;
    JITUINT8 mayor_version;
    JITUINT8 minor_version;
    JITUINT8 heap_size;
    JITUINT8 reserved_2;
    JITUINT64 valid;
    JITUINT64 sorted;
    t_metadata_tables tables;
} t_not_stream;

/**
 * @brief Read the Not stream
 *
 * Read the not stream of the assembly referenced by the binary parameter and fill all the information about it into the structure referenced by the not_stream pointer
 */
int read_not_stream (t_not_stream *not_stream, void *binary_info);

/**
 * @brief Delete the Not stream
 *
 * Delete all the rows of the table used to fill up the Not stream
 */
void shutdown_not_stream (t_not_stream *stream);

#endif
