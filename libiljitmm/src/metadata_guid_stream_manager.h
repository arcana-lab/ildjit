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
   *@file metadata_guid_stream_manager.h
 */

#ifndef METADATA_GUID_STREAM_MANAGER_H
#define METADATA_GUID_STREAM_MANAGER_H

#include <iljit-utils.h>

/**
 * @brief Rows of GUID stream
 *
 * Structure to fill the informations about one row of the GUID metadata stream
 */
typedef struct {
    JITUINT16 guid;
} t_row_guid_stream;

/**
 * @brief GUID stream
 *
 * The GUID stream is fill inside a table; this structure has the table and some other informations about this stream.
 */
typedef struct {
    JITUINT32 offset;                       /**< Offset from the begin of the file	*/
    JITUINT32 size;
    t_row_guid_stream       **table;
    JITUINT32 tableLength;
} t_guid_stream;

/**
 * @brief Read the GUID stream
 *
 * Read the GUID stream of the assembly referenced by the binary parameter and fill all the information about it into the structure referenced by the guid_stream pointer
 */
JITINT32 read_guid_string_stream (t_guid_stream *guid_stream, t_binary *binary);

/**
 * @brief Delete the GUID stream
 *
 * Delete all the rows of the table used to fill up the GUID stream
 */
void shutdown_guid_stream (t_guid_stream *stream);

#endif
