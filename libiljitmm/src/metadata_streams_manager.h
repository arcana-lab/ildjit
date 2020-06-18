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
   *@file metadata_streams_manager.h
 */

#ifndef METADATA_STREAM_MANAGER_H
#define METADATA_STREAM_MANAGER_H

// My header
#include <iljit-utils.h>
#include <metadata/tables/metadata_table_manager.h>
#include <metadata/streams/metadata_string_stream_manager.h>
#include <metadata/streams/metadata_blob_stream_manager.h>
#include <metadata/streams/metadata_guid_stream_manager.h>
#include <metadata/streams/metadata_not_stream_manager.h>
#include <metadata/streams/metadata_us_stream_manager.h>
// End

#define STRING_END_CHARACTER 0
#define US_END_CHARACTER 0
#define LINUX_STRING_END_CHARACTER 13

/**
 * @brief Streams
 *
 * The five metadata streams is fill inside this structure.
 */
typedef struct {
    t_string_stream string_stream;
    t_us_stream us_stream;
    t_blob_stream blob_stream;
    t_guid_stream guid_stream;
    t_not_stream not_stream;
} t_streams_metadata;

/**
 * @brief Read all the streams
 * @param binary Binary.file has to point to the first byte where streams description start
 *
 * Fetches the streams associated with the current module; it's invoked after the Stream headers has been read
 */
int read_streams (t_streams_metadata *streams, void *void_binary_info);

/**
 * @brief Free resources allocated for the streams
 * @param streams struct containing references to the streams
 *
 * Shuts down the streams freeing the resources
 */
void shutdown_streams (t_streams_metadata *streams);

/**
 * @brief Stores #GUID stream information
 *
 * Stores the information related to the GUID stream theat has been fetched from the module
 */
void set_guid_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size);
/**
 * @brief Stores #~ stream information
 *
 * Stores the information related to the ~ stream theat has been fetched from the module
 */
void set_not_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size);
/**
 * @brief Stores #US stream information
 *
 * Stores the information related to the US stream theat has been fetched from the module
 */
void set_us_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size);
/**
 * @brief Stores #String stream information
 *
 * Stores the information related to the String stream theat has been fetched from the module
 */
void set_string_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size);

/**
 * @brief Stores #Blob stream information
 *
 * Stores the information related to the Blob stream theat has been fetched from the module
 */
void set_blob_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size);

#endif
