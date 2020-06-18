/*
 * Copyright (C) 2008 - 2012 Timothy M. Jones, Simone Campanoni
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

#ifndef VLLPA_UTIL_H
#define VLLPA_UTIL_H

/**
 * This file contains types and prototypes for utility functions for
 * the interprocedural pointer analysis.
 */

#include "smalllib.h"
#include "vllpa_macros.h"

/**
 * Store a mapping to a variable in a hash table.  Since the lookup will return
 * NULL on failure, we can't store a 0 as a variable ID, so increase by 1.
 */
void insertMappingToVar(SmallHashTable *table, void *key, JITINT32 var);
#define insertMappingVarToVar(table, key, var) insertMappingToVar(table, (void *)(JITNINT)key, var)

/**
 * Find a mapping to a variable in a hash table.  We haven't actually stored
 * the variable ID, but one more than it so that we can distinguish between
 * failed lookups and the value 0 being stored.
 */
JITINT32 lookupMappingToVar(SmallHashTable *table, void *key);
#define lookupMappingVarToVar(table, key) lookupMappingToVar(table, (void *)(JITNINT)key)
#define deleteMappingVarToVar(table, key) smallHashTableRemove(table, (void *)(JITNINT)key)

/**
 * Check whether a parameter type is an immediate integer type.
 */
JITBOOLEAN isImmIntType(JITUINT32 type);

/**
 * Check whether a parameter is an immediate integer type.
 */
JITBOOLEAN isImmIntParam(ir_item_t *param);

/**
 * Check whether a parameter is an immediate with the value zero.  If it is
 * a register or other non-constant then it is not zero.  Floating point
 * values are also counted as non-zero here.
 */
JITBOOLEAN isZeroImmParam(ir_item_t *param);

/**
 * Check whether a parameter is an immediate with a non-zero value.  If it
 * is a register or other non-constant then it is not non-zero.  Floating
 * point values are also counted as not non-zero here.
 */
JITBOOLEAN isNonZeroImmParam(ir_item_t *param);

/**
 * Check whether a parameter has a global pointer type.
 */
JITBOOLEAN paramIsGlobalPointer(ir_item_t *param);

/**
 * Check for a variable parameter type.
 */
JITBOOLEAN isVariableParam(ir_item_t *param);

/**
 * Check for a symbol parameter and an immediate integer parameter in
 * either order.
 */
JITBOOLEAN areSymbolAndImmIntParams(ir_item_t *param1, ir_item_t *param2);

/**
 * Enable extended debug printing for certain functions.
 */
#ifdef PRINTDEBUG
JITBOOLEAN enableExtendedDebugPrinting(ir_method_t *method);
#else
#define enableExtendedDebugPrinting(scc) JITFALSE
#endif  /* ifdef PRINTDEBUG */

#endif  /* VLLPA_UTIL_H */
