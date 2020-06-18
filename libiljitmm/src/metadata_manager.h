/*
 * Copyright (C) 2006 - 2010 Campanoni Simone
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
   *@file metadata_manager.h
 */
#ifndef METADATA_MANAGER_H
#define METADATA_MANAGER_H

#define MAX_VERSION_LENGTH 512

#include <iljit-utils.h>
#include <ecma_constant.h>
#include <section_manager.h>

// My header
#include <metadata/streams/metadata_streams_manager.h>
// End
/**
 * @brief Metadata
 *
 * This struct contain the metadata header information and all pointer to fetch all metadata items
 */
typedef struct {
    JITUINT32 signature;
    JITUINT16 mayor_version;
    JITUINT16 minor_version;
    JITUINT32 reserved;
    JITUINT32 length;
    JITINT8 version[MAX_VERSION_LENGTH];
    JITUINT16 flags;
    JITUINT16 streams_number;
    t_streams_metadata streams_metadata;
} t_metadata;

/**
 * @brief Read all metadata
 * @param metadata_RVA This is the RVA of the begin of the metadata; this value is not relative to the begin of the CLI section
 *
 * Read the metadata of the assembly pointed by the binary parameter. It fill all informations inside the struct pointed by the metadata parameter
 */
JITINT32 read_metadata (t_metadata *metadata, void *binary_info);

/**
 * @brief Destroy the metadata
 *
 * Destroy the struct needed to store informations about metadata inside one assembly
 */
void metadata_shutdown (t_metadata *metadata);

char * libiljitmmVersion ();
void libiljitmmCompilationFlags (char *buffer, JITINT32 bufferLength);
void libiljitmmCompilationTime (char *buffer, JITINT32 bufferLength);

void metadata_init (t_metadata *metadata);

#endif
