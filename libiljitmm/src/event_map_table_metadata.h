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
   *@file event_map_table_metadata.h
 */

#ifndef EVENT_MAP_TABLE_METADATA_H
#define EVENT_MAP_TABLE_METADATA_H

#include <jitsystem.h>

/**
 * @brief Rows of EventMap table
 *
 * Structure to fill the informations about one row of the EventMap metadata table
 */
typedef struct {
    JITUINT32 parent;               /**< Index into TypeDef table			*/
    JITUINT32 event_list;           /**< Index into Event table			*/
    JITINT32 index;
    void *binary;
} t_row_event_map_table;

/**
 * @brief EventMap table
 *
 * Structure to fill up all the rows of the EventMap metadata table and to export some methods
 */
typedef struct {
    t_row_event_map_table   **table;
    JITUINT32 cardinality;
} t_event_map_table;

/**
 * @brief Load the EventMap table
 *
 * Load the EventMap metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_event_map_table (t_event_map_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the EventMap table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the EventMap table and it free the memory occupied by that rows.
 */
void shutdown_event_map_table (t_event_map_table *table);

#endif
