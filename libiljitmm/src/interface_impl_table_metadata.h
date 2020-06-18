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
   *@file interface_impl_table_metadata.h
 */

#ifndef INTERFACE_IMPL_TABLE_METADATA_H
#define INTERFACE_IMPL_TABLE_METADATA_H

#include <xanlib.h>
#include <jitsystem.h>

/**
 * @brief Rows of InterfaceImpl table
 *
 * Structure to fill the informations about one row of the InterfaceImpl metadata table
 */
typedef struct {
    JITUINT32       classIndex;                  /**< Index into the TypeDef table		*/
    JITUINT32 interfaceDescription;         /**< TypeDefOrRef index				*/
    JITINT32 index;
    void *binary;
} t_row_interface_impl_table;

/**
 * @brief InterfaceImpl table
 *
 * Structure to fill up all the rows of the InterfaceImpl metadata table and to export some methods
 */
typedef struct t_interface_impl_table {
    t_row_interface_impl_table      **table;
    JITUINT32 cardinality;
} t_interface_impl_table;

/**
 * @brief Load the InterfaceImpl table
 *
 * Load the InterfaceImpl metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_interface_impl_table (t_interface_impl_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the InterfaceImpl table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the InterfaceImpl table and it free the memory occupied by that rows.
 */
void shutdown_interface_impl_table (t_interface_impl_table *table);

#endif
