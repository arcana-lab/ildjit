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
   *@file metadata_manager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <compiler_memory_manager.h>

// My headers
#include <config.h>
#include <metadata_manager.h>
#include <decoder.h>
#include <metadata/tables/metadata_table_manager.h>
#include <iljitmm-system.h>
// End

#define DIM_BUF 2056

JITINT32 read_metadata (t_metadata *metadata, void *void_binary_info) {
    JITUINT32 metadata_address;
    JITUINT32 padding;
    JITUINT32 offset;
    JITUINT32 size;
    JITINT32 count;
    JITINT32 streams_number;
    JITINT8 buffer[DIM_BUF];
    t_binary_information *binary_info;
    t_binary *binary;
    JITUINT32 metadata_RVA;
    t_section *sections;

    binary_info = (t_binary_information *) void_binary_info;
    binary = &(binary_info->binary);
    metadata_RVA = binary_info->cli_information.metadata_RVA;
    sections = binary_info->sections;

    PDEBUG("METADATA MANAGER: Metadata root	RVA     = 0x%X	%d\n", metadata_RVA, metadata_RVA);
    metadata_address = convert_virtualaddress_to_realaddress(sections, metadata_RVA);
    PDEBUG("METADATA MANAGER: Metadata root	address = 0x%X	%d\n", metadata_address, metadata_address);
    PDEBUG("METADATA MANAGER: Begin to read metadata\n");
    if (il_read(buffer, 16, binary) != 0) {
        return -1;
    }

    read_from_buffer(buffer, 0, 4, &(metadata->signature));
    read_from_buffer(buffer, 4, 2, &(metadata->mayor_version));
    read_from_buffer(buffer, 6, 2, &(metadata->minor_version));
    read_from_buffer(buffer, 8, 4, &(metadata->reserved));
    read_from_buffer(buffer, 12, 4, &(metadata->length));

    PDEBUG("METADATA MANAGER:	Signature		=	0x%X	%d\n",      metadata->signature, metadata->signature);
    PDEBUG("METADATA MANAGER:	Version			=	0x%X.0x%X		%d.%d\n", metadata->mayor_version, metadata->minor_version, metadata->mayor_version, metadata->minor_version);
    PDEBUG("METADATA MANAGER:	Reserved		=	0x%X		%d\n",      metadata->reserved, metadata->reserved);
    PDEBUG("METADATA MANAGER:	Length			=	0x%X		%d\n",       metadata->length, metadata->length);
    PDEBUG("METADATA MANAGER:	Check values\n");
    if (metadata->signature != ECMA_SIGNATURE) {
        print_err("METADATA MANAGER: ERROR= The signature item is not as described in the ECMA 335 spectification. ", 0);
        return -1;
    }
    PDEBUG("METADATA MANAGER:               Signature OK\n");
    if (metadata->reserved != 0) {
        print_err("METADATA MANAGER: ERROR= The reserved item is not as described in the ECMA 335 spectification. ", 0);
        return -1;
    }
    PDEBUG("METADATA MANAGER:               Reserved OK\n");
    if (metadata->length > 255) {
        print_err("METADATA MANAGER: ERROR= The length is not less than 255 as described in the ECMA 335 spectification. ", 0);
        return -1;
    }
    if (metadata->length > MAX_VERSION_LENGTH) {
        print_err("METADATA MANAGER:            ERROR= The length is too big. ", 0);
        return -1;
    }
    PDEBUG("METADATA MANAGER:               Length OK\n");
    if (il_read(buffer, metadata->length, binary) != 0) {
        return -1;
    }
    STRNCPY(metadata->version, buffer, metadata->length);
#ifdef PRINTDEBUG
    PDEBUG("METADATA MANAGER:	Version			=	\"%s\"\n",        metadata->version);
    print_binary_offset(*binary);
#endif
    padding = (4 - (((*(binary->reader))->relative_offset) % 4));
    if (padding != 4) {
        if (il_read(buffer, padding, binary) != 0) {
            return -1;
        }
#ifdef PRINTDEBUG
        PDEBUG("METADATA MANAGER:	Read %d byte of padding\n", padding);
        print_binary_offset(*binary);
#endif
    }
    if (il_read(buffer, 4, binary) != 0) {
        return -1;
    }
    read_from_buffer(buffer, 0, 2, &(metadata->flags));
    read_from_buffer(buffer, 2, 2, &(metadata->streams_number));
    streams_number = metadata->streams_number;

    PDEBUG("METADATA MANAGER:	Flags			=	0x%X		%d\n", (JITUINT32) metadata->flags, metadata->flags);
    PDEBUG("METADATA MANAGER:	Streams number		=	0x%X		%d\n", (JITUINT32) metadata->streams_number, metadata->streams_number);
    PDEBUG("METADATA MANAGER:	Check values\n");
    if (metadata->flags != 0) {
        print_err("METADATA MANAGER:            ERROR= The Flags item is not as described in the ECMA 335 spectification. ", 0);
        return -1;
    }
    PDEBUG("METADATA MANAGER:               Flags OK\n");
    PDEBUG("METADATA MANAGER:	Streams\n");
    for (streams_number = 0; streams_number < metadata->streams_number; streams_number++) {
        JITINT8 name[33];
        if (il_read(buffer, 8, binary) != 0) {
            return -1;
        }
        read_from_buffer(buffer, 0, 4, &offset);
        read_from_buffer(buffer, 4, 4, &size);
        PDEBUG("METADATA MANAGER:		Offset from the metadata root	=	0x%X		%d\n", (JITUINT32) offset, offset);
        PDEBUG("METADATA MANAGER:		Offset                          =	0x%X		%d\n", (JITUINT32) offset, offset + metadata_address);
        PDEBUG("METADATA MANAGER:		Size				=	0x%X		%d\n", (JITUINT32) size, size);
        for (count = 0; count < 32; count++) {
            if (il_read(&name[count], 1, binary) != 0) {
                return -1;
            }
            if (name[count] == '\0') {
                break;
            }
        }
        if (count == 32) {
            PDEBUG("METADATA MANAGER: ERROR= Found a name stream not standard\n");
            return -1;
        }
        PDEBUG("METADATA MANAGER:		Stream			=	%s\n", name);
        padding = (4 - (((*(binary->reader))->relative_offset) % 4));
        if (padding != 4) {
            if (il_read(buffer, padding, binary) != 0) {
                return -1;
            }
            PDEBUG("METADATA MANAGER:		Read %d byte of padding\n", padding);
        }
        if (STRNCMP(name, "#~", 32) == 0) {
            set_not_stream(&(metadata->streams_metadata), offset + metadata_address, size);
            continue;
        }
        if (STRNCMP(name, "#US", 32) == 0) {
            set_us_stream(&(metadata->streams_metadata), offset + metadata_address, size);
            continue;
        }
        if (STRNCMP(name, "#Blob", 32) == 0) {
            set_blob_stream(&(metadata->streams_metadata), offset + metadata_address, size);
            continue;
        }
        if (STRNCMP(name, "#GUID", 32) == 0) {
            set_guid_stream(&(metadata->streams_metadata), offset + metadata_address, size);
            continue;
        }
        if (STRNCMP(name, "#Strings", 32) == 0) {
            set_string_stream(&(metadata->streams_metadata), offset + metadata_address, size);
            continue;
        }
        PDEBUG("METADATA MANAGER: ERROR= Found one kind of stream not standard with name=%s\n", buffer);
        return -1;
    }
    if (read_streams( &(metadata->streams_metadata), void_binary_info ) != 0) {
        print_err("METADATA MANAGER: Metadata stream manager return an error. ", 0);
        return -1;
    }
#ifdef PRINTDEBUG
    print_binary_offset(*binary);
#endif
    PDEBUG("METADATA MANAGER: Metadata read\n");
    return 0;
}

void metadata_shutdown (t_metadata *metadata) {
    PDEBUG("METADATA MANAGER: Shutting down\n");
    shutdown_streams(&(metadata->streams_metadata));
}

void libiljitmmCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(JITINT8) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(JITINT8) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(JITINT8) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(JITINT8) * bufferLength);
#endif
}

void libiljitmmCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libiljitmmVersion () {
    return (char *) VERSION;
}

void metadata_init (t_metadata *metadata) {

    /* Assertions                   */
    assert(metadata != NULL);

    memset(metadata, 0, sizeof(t_metadata));
}
