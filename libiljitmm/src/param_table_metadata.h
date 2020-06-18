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
   *@file param_table_metadata.h
 */

#ifndef PARAM_TABLE_METADATA_H
#define PARAM_TABLE_METADATA_H

#include <jitsystem.h>

/**
 * @brief Rows of Param table
 *
 * Structure to fill the informations about one row of the Param metadata table
 */
typedef struct {
    JITUINT16 flags;                        /**< Bitmask of type ParamAttributes		*/
    JITUINT32 sequence;                     /**< Constant					*/
    JITUINT32 name;                         /**< String index				*/
    JITINT32 index;
    void *binary;
} t_row_param_table;

/**
 * @brief Param table
 *
 * Structure to fill up all the rows of the Param metadata table and to export some methods
 */
typedef struct {
    t_row_param_table       *table;
    JITUINT32 cardinality;
} t_param_table;

/**
 * @brief Load the Param table
 *
 * Load the Param metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_param_table (t_param_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the Param table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the Param table and it free the memory occupied by that rows.
 */
void shutdown_param_table (t_param_table *table);

#endif
