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
   *@file metadata_table_manager.h
 */

#ifndef METADATA_TABLE_MANAGER_H
#define METADATA_TABLE_MANAGER_H

// My headers
#include <jitsystem.h>
#include <ecma_constant.h>
#include <iljit-utils.h>
#include <section_manager.h>
#include <xanlib.h>
#include <metadata/streams/metadata_blob_stream_manager.h>
#include <metadata/tables/module_table_metadata.h>
#include <metadata/tables/type_ref_table_metadata.h>
#include <metadata/tables/type_def_table_metadata.h>
#include <metadata/tables/field_table_metadata.h>
#include <metadata/tables/method_def_table_metadata.h>
#include <metadata/tables/param_table_metadata.h>
#include <metadata/tables/member_ref_table_metadata.h>
#include <metadata/tables/module_ref_table_metadata.h>
#include <metadata/tables/assembly_table_metadata.h>
#include <metadata/tables/assembly_ref_table_metadata.h>
#include <metadata/tables/standalone_sig_table_metadata.h>
#include <metadata/tables/interface_impl_table_metadata.h>
#include <metadata/tables/constant_table_metadata.h>
#include <metadata/tables/property_table_metadata.h>
#include <metadata/tables/custom_attribute_table_metadata.h>
#include <metadata/tables/field_marshal_table_metadata.h>
#include <metadata/tables/decl_security_table_metadata.h>
#include <metadata/tables/class_layout_table_metadata.h>
#include <metadata/tables/field_layout_table_metadata.h>
#include <metadata/tables/event_map_table_metadata.h>
#include <metadata/tables/event_table_metadata.h>
#include <metadata/tables/property_map_table_metadata.h>
#include <metadata/tables/method_impl_table_metadata.h>
#include <metadata/tables/method_semantics_table_metadata.h>
#include <metadata/tables/type_spec_table_metadata.h>
#include <metadata/tables/impl_map_table_metadata.h>
#include <metadata/tables/field_rva_table_metadata.h>
#include <metadata/tables/manifest_resource_table_metadata.h>
#include <metadata/tables/file_table_metadata.h>
#include <metadata/tables/exported_type_table_metadata.h>
#include <metadata/tables/nested_class_table_metadata.h>
#include <metadata/tables/method_spec_table_metadata.h>
#include <metadata/tables/generic_param_table_metadata.h>
#include <metadata/tables/generic_param_constraint_table_metadata.h>
// End

/** Define the amount of rows in the table causing the index to use 4 bytes instead of 2 */
#define METADATA_TABLE_LIMIT_16_BITS_ENCODING 65536
#define METADATA_TABLE_LIMIT_15_BITS_ENCODING 32768
#define METADATA_TABLE_LIMIT_14_BITS_ENCODING 16384
#define METADATA_TABLE_LIMIT_13_BITS_ENCODING 8192

/**
 * @brief Metadata tables
 *
 * This struct contains all the metadata tables stored inside the Not stream.
 */
typedef struct {
    t_module_table module_table;
    t_type_ref_table type_ref_table;
    t_type_def_table type_def_table;
    t_field_table field_table;
    t_method_def_table method_def_table;
    t_param_table param_table;
    t_member_ref_table member_ref_table;
    t_module_ref_table module_ref_table;
    t_assembly_table assembly_table;
    t_assembly_ref_table assembly_ref_table;
    t_standalone_sig_table standalone_sig_table;
    t_interface_impl_table interface_impl_table;
    t_constant_table constant_table;
    t_custom_attribute_table custom_attribute_table;
    t_property_table property_table;
    t_field_marshal_table field_marshal_table;
    t_decl_security_table decl_security_table;
    t_class_layout_table class_layout_table;
    t_field_layout_table field_layout_table;
    t_event_map_table event_map_table;
    t_event_table event_table;
    t_property_map_table property_map_table;
    t_method_impl_table method_impl_table;
    t_method_semantics_table method_semantics_table;
    t_type_spec_table type_spec_table;
    t_impl_map_table impl_map_table;
    t_field_rva_table field_rva_table;
    t_manifest_resource_table manifest_resource_table;
    t_file_table file_table;
    t_exported_type_table exported_type_table;
    t_nested_class_table nested_class_table;
    t_generic_param_table generic_param_table;
    t_generic_param_constraint_table generic_param_constraint_table;
    t_method_spec_table method_spec_table;
} t_metadata_tables;

/**
 * @brief Load tables
 * @row_buf It has to point to the first byte of the buffer describes the rows of the metadata tables
 * @tables_buf It has to point to the first byte of the buffer describes the metadata tables
 *
 * Load all the metadata tables stored inside the Not stream. All information decoded is stored inside the structure pointed by the tables parameter.
 */
JITINT32 load_metadata_tables (t_metadata_tables *tables, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *tables_buf);

/**
 * @brief Delete all the tables
 *
 * Delete all the rows of the all tables stored inside the Not stream calling the shutdown function of every table.
 */
void shutdown_tables_manager (t_metadata_tables *tables);

/**
 *
 * Return a pointer to the row identified by the row_ID constant, of the table identified by the table_ID constant.
 */
void * get_row (t_metadata_tables *tables, JITUINT16 table_ID, JITUINT32 row_ID);

/**
 *
 * Return a pointer to the first row  of the table identified by the table_ID constant.
 */
void * get_first_row (t_metadata_tables *tables, JITUINT16 table_ID);

/**
 *
 * Return cardinality of the table identified by the table_ID constant.
 */
JITUINT32 get_table_cardinality (t_metadata_tables *tables, JITUINT16 table_ID);

/**
 *
 * Return a pointer to the table identified by the table_ID constant.
 */
void * get_table (t_metadata_tables *tables, JITUINT16 table_ID);


#endif
