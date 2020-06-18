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
   *@file assembly_ref_table_metadata.h
 */

#ifndef ASSEMBLY_REF_TABLE_METADATA_H
#define ASSEMBLY_REF_TABLE_METADATA_H

#include <jitsystem.h>

#define MAX_NAME_ASSEMBLY 1024

/**
 * @brief Rows of AssemblyRef table
 *
 * Structure to fill the informations about one row of the AssemblyRef metadata table
 */
typedef struct {
    JITUINT16 major_version;
    JITUINT16 minor_version;
    JITUINT16 build_number;
    JITUINT16 revision_number;
    JITUINT32 flags;
    JITUINT32 public_key_or_token;                  /**< Index to the blob heap                                     */
    JITUINT32 name;                                 /**< Index to the string heap                                   */
    JITUINT32 culture;                              /**< Index to the string heap                                   */
    JITUINT32 hash_value;                           /**< Index to the blob heap                                     */
    JITBOOLEAN is_named;                            /**< Flag to show if the assembly referenced has its
	                                                 *  name stored in the name_assembly field			*/
    void                    *binary_info;           /**< Struct to fill information about this assembly		*/
    void *binary;
    JITINT32 index;
} t_row_assembly_ref_table;

/**
 * @brief AssemblyRef table
 *
 * Structure to fill up all the rows of the AssemblyRef metadata table and to export some methods
 */
typedef struct {
    t_row_assembly_ref_table        **table;        /**< AssemblyRef metadata table			*/
    JITUINT32 cardinality;                          /**< Cardinality of the AssemblyRef table	*/
} t_assembly_ref_table;

/**
 * @brief Load the AssemblyRef table
 *
 * Load the AssemblyRef metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_assembly_ref_table (t_assembly_ref_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the AssemblyRef table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the AssemblyRef table and it free the memory occupied by that rows.
 */
void shutdown_assembly_ref_table (t_assembly_ref_table *table);

#endif
