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
   *@file metadata_streams_manager.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define DIM_BUF 2056

// My header
#include <decoder.h>
#include <metadata_streams_manager.h>
#include <metadata_string_stream_manager.h>
#include <metadata_us_stream_manager.h>
#include <metadata_not_stream_manager.h>
#include <metadata_guid_stream_manager.h>
#include <iljitmm-system.h>
// End

JITINT32 read_streams (t_streams_metadata *streams, void *void_binary_info) {
    PDEBUG("METADATA STREAM MANAGER:        Streams reading\n");
    t_binary_information *binary_info;
    binary_info = (t_binary_information *) void_binary_info;
    if (read_not_stream(&(streams->not_stream), binary_info) != 0) {
        return -1;
    }
    if (read_string_stream(&(streams->string_stream), &(binary_info->binary)) != 0) {
        return -1;
    }
    if (read_blob_stream(&(streams->blob_stream), &(binary_info->binary)) != 0) {
        return -1;
    }
    if (read_us_stream(&(streams->us_stream), &(binary_info->binary)) != 0) {
        return -1;
    }
    if (read_guid_string_stream(&(streams->guid_stream), &(binary_info->binary)) != 0) {
        return -1;
    }
    return 0;
}

void set_guid_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size) {
    streams->guid_stream.offset = offset;
    streams->guid_stream.size = size;
}

void set_not_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size) {
    streams->not_stream.offset = offset;
    streams->not_stream.size = size;
}

void set_us_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size) {
    streams->us_stream.offset = offset;
    streams->us_stream.size = size;
}

void set_string_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size) {
    streams->string_stream.offset = offset;
    streams->string_stream.size = size;
}

void set_blob_stream (t_streams_metadata *streams, JITUINT32 offset, JITUINT32 size) {
    streams->blob_stream.offset = offset;
    streams->blob_stream.size = size;
}

void shutdown_streams (t_streams_metadata *streams) {
    PDEBUG("METADATA STREAM MANAGER: Shutting down\n");
    shutdown_string_stream(&(streams->string_stream));
    shutdown_guid_stream(&(streams->guid_stream));
    shutdown_us_stream(&(streams->us_stream));
    shutdown_blob_stream(&(streams->blob_stream));
    shutdown_not_stream(&(streams->not_stream));
    PDEBUG("METADATA STREAM MANAGER: Done\n");
}
