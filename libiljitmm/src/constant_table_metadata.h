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
   *@file constant_table_metadata.h
 */

#ifndef CONSTANT_TABLE_METADATA_H
#define CONSTANT_TABLE_METADATA_H

#include <jitsystem.h>

/**
 * @brief Rows of Constant table
 *
 * Structure to fill the informations about one row of the Constant metadata table
 */
typedef struct {
    JITUINT16 type;                         /**< ELEMENT TYPE constant			*/
    JITUINT32 parent;                       /**< HasConstant index				*/
    JITUINT32 signature;                    /**< Blob index					*/
    JITINT32 index;
    void *binary;
} t_row_constant_table;

/**
 * @brief Constant table
 *
 * Structure to fill up all the rows of the Constant metadata table and to export some methods
 */
typedef struct {
    t_row_constant_table    **table;
    JITUINT32 cardinality;
} t_constant_table;

/**
 * @brief Load the Constant table
 *
 * Load the Constant metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_constant_table (t_constant_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the Constant table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the Constant table and it free the memory occupied by that rows.
 */
void shutdown_constant_table (t_constant_table *table);

#endif
