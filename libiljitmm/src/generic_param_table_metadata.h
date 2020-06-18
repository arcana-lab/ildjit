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
   *@file generic_param_table_metadata.h
 */
#ifndef GENERIC_PARAM_TABLE_METADATA_H
#define GENERIC_PARAM_TABLE_METADATA_H

#include <jitsystem.h>

/**
 * @brief Rows of TypeSpec table
 *
 * Structure to fill the informations about one row of the TypeSpec metadata table
 */
typedef struct t_row_generic_param_table {
    JITUINT16 number;
    JITUINT16 flags;
    JITINT32 owner;
    JITINT32 name;
    JITINT32 index;
    void *binary;
    JITBOOLEAN (*isCovariant)(struct t_row_generic_param_table *param);
    JITBOOLEAN (*isContravariant)(struct t_row_generic_param_table *param);
    JITBOOLEAN (*isReferenceType)(struct t_row_generic_param_table *param);
    JITBOOLEAN (*isNotNullableValueType )(struct t_row_generic_param_table *param);
    JITBOOLEAN (*haveDefaultConstructor)(struct t_row_generic_param_table *param);
} t_row_generic_param_table;

/**
 * @brief TypeSpec table
 *
 * Structure to fill up all the rows of the TypeSpec metadata table and to export some methods
 */
typedef struct {
    t_row_generic_param_table       **table;
    JITUINT32 cardinality;
} t_generic_param_table;

/**
 * @brief Load the GenericParam table
 *
 * Load the TypeSpec metadata table starting reading from the memory pointed by the table_buf parameter; all the information fetched are stored inside the struct pointed by the table parameter.
 */
JITINT32 load_generic_param_table (t_generic_param_table *table, void *binary, JITUINT64 valid, JITUINT16 heap_size, JITINT8 *row_buf, JITINT8 *table_buf, JITUINT32 *bytes);

/**
 * @brief Shutdown the GenericParam table
 *
 * This functions fetches all the rows from the table pointed by the table parameter, it eliminate them from the TypeSpec table and it free the memory occupied by that rows.
 */
void shutdown_generic_param_table (t_generic_param_table *table);

#endif
