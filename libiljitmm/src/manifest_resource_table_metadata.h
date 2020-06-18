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
   *@file manifest_resource_table_metadata.h
 */

#ifndef MANIFEST_RESOURCE_TABLE_METADATA_H
#define MANIFEST_RESOURCE_TABLE_METADATA_H

#include <jitsystem.h>

/**
 * @brief Rows of ManifestResource table
 *
 * Structure to fill the informations about one row of the ManifestResource metadata table
 */
typedef struct t_row_manifest_resource_table {
    JITUINT32 offset;                       /**< Constant						*/
    JITUINT32 flags;                        /**< Bitmask of ManifestResourceAttributes		*/
    JITUINT32 name;                         /**< String index					*/
    JITUINT32 implementation;               /**< Implementation index				*/
    JITINT32 index;
    void *binary;
    JITBOOLEAN (* isPublic)(struct t_row_manifest_resource_table *manifest);
    JITBOOLEAN (* isPrivate)(struct t_row_manifest_resource_table *manifest);
    JITINT32 (* decodeImplementationIndex)(struct t_row_manifest_resource_table *manifest, JITINT32 *destination_table);           /**< Decode the implementation index. Returns the decoded index, and fills into destination_table the reference table: 1 if is a File table, 2 if is a AssemblyRef table, 3 if is an ExportedType	*/

} t_row_manifest_resource_table;

/**
 * @brief ManifestResource table
 *
 * Structure to fill up all the rows of the ManifestResource metadata table and to export some methods
 */
typedef struct {
    t_row_manifest_resource_table   **table;
    JITUINT32 cardinality;
} t_manifest_resource_table;

/**
 * @brief Load the ManifestResource table
 *
 * Load the ManifestResource metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_manifest_resource_table (t_manifest_resource_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the ManifestResource table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the ManifestResource table and it free the memory occupied by that rows.
 */
void shutdown_manifest_resource_table (t_manifest_resource_table *table);

#endif
