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
   *@file nested_class_table_metadata.h
 */

#ifndef NESTED_CLASS_TABLE_METADATA_H
#define NESTED_CLASS_TABLE_METADATA_H

#include <jitsystem.h>

/**
 * @brief Rows of NestedClass table
 *
 * Structure to fill the informations about one row of the NestedClass metadata table
 */
typedef struct {
    JITUINT32 nested_class;                 /**< Index into TypeDef table			*/
    JITUINT32 enclosing_class;              /**< Index into TypeDef table			*/
    JITINT32 index;
    void *binary;
} t_row_nested_class_table;

/**
 * @brief NestedClass table
 *
 * Structure to fill up all the rows of the NestedClass metadata table and to export some methods
 */
typedef struct {
    t_row_nested_class_table        **table;
    JITUINT32 cardinality;
} t_nested_class_table;

/**
 * @brief Load the NestedClass table
 *
 * Load the NestedClass metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_nested_class_table (t_nested_class_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the NestedClass table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the NestedClass table and it free the memory occupied by that rows.
 */
void shutdown_nested_class_table (t_nested_class_table *table);

#endif
