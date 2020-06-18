/*
 * Copyright (C) 2006  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
 * @file decoder.h
 * @brief Decoder interface
 */

#ifndef DECODER_H
#define DECODER_H

#include <jitsystem.h>
#include <iljit-utils.h>
#include <metadata_manager.h>

#define MAX_NAME 1024

typedef struct _t_library_imported {
    JITUINT32 import_lookup_table;
    JITUINT32 data_timestamp;
    JITUINT32 forwarder_chain;
    JITUINT32 name_RVA;
    JITUINT32 import_address_table;
    struct _t_library_imported *next;
} t_library_imported;

/**
 * @brief CLI Header
 *
 * Struct to fill information about the CLI Header
 */
typedef struct {
    JITUINT32 cb;
    JITUINT16 mayor_runtime_version;
    JITUINT16 minor_runtime_version;
    JITUINT32 metadata_RVA;
    JITUINT32 metadata_size;
    JITUINT32 flags;
    JITUINT32 entry_point_token;
    JITUINT32 resources_RVA;
    JITUINT32 resources_size;
    JITUINT64 strong_name_signature;
    JITUINT64 vtable_fixups;
} t_cli_information;

/**
 * @brief PE Header
 *
 * Struct to fill information about the PE Header
 */
typedef struct {
    JITUINT16 section_cardinality;
    JITUINT32 time_stamp;
    JITUINT32 optional_header_size;
    JITUINT32 characteristic;
    JITBOOLEAN isDLL;
    JITBOOLEAN onlyFor32BitsMachines;
} t_PE_information;

/**
 * @brief Standard fields
 *
 * This is the standard fields as specified in the ECMA-335
 */
typedef struct {
    JITUINT16 magic;
    JITUINT32 lmayor : 8;
    JITUINT32 lminor : 8;
    JITUINT32 code_size;
    JITUINT32 size_initialized_data;
    JITUINT32 size_unitialized_data;
    JITUINT32 entry_point_address;
    JITUINT32 base_of_code;
    JITUINT32 base_of_data;
} t_standard_fields;

/**
 * @brief NT Header
 *
 * Struct to fill information about the NT Header
 */
typedef struct {
    JITUINT32 file_alignment;
    JITUINT32 image_size;
    JITUINT32 header_size;
    JITUINT16 subsystem;
} t_NT_information;

/**
 * @brief Data directories
 *
 * Struct to fill information about the Data Directories
 */
typedef struct {
    JITUINT32 import_table_RVA;
    JITUINT32 import_table_size;
    JITUINT32 relocation_table_RVA;
    JITUINT32 relocation_table_size;
    JITUINT32 import_address_table_RVA;
    JITUINT32 import_address_table_size;
    JITUINT32 cli_header_RVA;
    JITUINT32 cli_header_size;
} t_Data_directories;

/**
 * @brief PE Optional Header
 *
 * Struct to fill information about the PE Optional Header
 */
typedef struct {
    t_standard_fields standard_fields;
    t_NT_information NT_info;
    t_Data_directories data_directories;
} t_PE_Optional_header;

/**
 * @brief MS-DOS Header
 *
 * Struct to fill information about the MS-DOS Header
 */
typedef struct {
    JITUINT32 base;
} t_MSDOS_header;

/**
 * @brief Assembly
 *
 * Struct to fill information about an assembly
 */
typedef struct {
    JITINT8			name[MAX_NAME];	/**< Name of the binary				*/
    JITINT8			*prefix;        /**< Path name where the binary file is found.	*/
    JITBOOLEAN		is_named;
    t_binary		binary;
    t_MSDOS_header		MSDOS_header;
    t_PE_information	PE_info;
    t_PE_Optional_header	PE_optional_header;
    t_cli_information	cli_information;
    t_metadata		metadata;
    t_section		*sections;
    t_library_imported	*libraries;
} t_binary_information;

/**
 * @brief Plugin
 *
 * Interface that a loader plugin has to provide
 */
typedef struct {
    JITINT8 *       (*get_ID_binary)();
    JITBOOLEAN (* decode_image)(t_binary_information *binary_info);
    JITBOOLEAN (* shutdown)(t_binary_information *binary_info);
    char *          (*getName)();
    char *          (*getVersion)();
    char *          (*getAuthor)();
    void (* getCompilationFlags)(char *buffer, JITINT32 bufferLength);
    void (* getCompilationTime)(char *buffer, JITINT32 bufferLength);
} t_plugin_interface;

#define DECODER_SYMBOL "decoder_interface"

#endif
