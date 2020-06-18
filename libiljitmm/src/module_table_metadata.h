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
   *@file module_table_metadata.h
 */

#ifndef MODULE_TABLE_METADATA_H
#define MODULE_TABLE_METADATA_H

#include <jitsystem.h>

#define MODULE_TABLE_ROW_SIZE   12

/**
 * @brief Rows of Module table
 *
 * Structure to fill the informations about one row of the Module metadata table
 */
typedef struct {
    JITUINT16 generation;
    JITUINT32 name;                         /**< string_stream_index	*/
    JITUINT32 module_version_id;            /**< Guid stream index          */
    JITUINT32 enc_id;
    JITUINT32 enc_base_id;
    JITINT32 index;
    void *binary;
} t_row_module_table;

/**
 * @brief Module table
 *
 * Structure to fill up all the rows of the Module metadata table and to export some methods
 */
typedef struct {
    t_row_module_table      **table;
    JITUINT32 cardinality;
} t_module_table;

/**
 * @brief Load the Module table
 *
 * Load the Module metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_module_table (t_module_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the Module table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the Module table and it free the memory occupied by that rows.
 */
void shutdown_module_table (t_module_table *table);

#endif
