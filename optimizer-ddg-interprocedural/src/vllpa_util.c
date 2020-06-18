/*
 * Copyright (C) 2009 - 2012 Timothy M Jones, Simone Campanoni
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
 * This file implements utility functions for the VLLPA algorithm from
 * the following paper:
 *
 * Practical and Accurate Low-Level Pointer Analysis.  Bolei Guo, Matthew
 * J. Bridges, Spyridon Triantafyllis, Guilherme Ottoni, Easwaran Raman
 * and David I. August.  Proceedings of the third International Symposium
 * on Code Generation and Optimization (CGO), March 2005.
 */

#include <assert.h>
#include <compiler_memory_manager.h>
#include <stdio.h>
#include <stdlib.h>

// My headers
#include "smalllib.h"
#include "vllpa_util.h"
#include "vllpa_macros.h"
// End


/**
 * Name of specific function for extended debug printing.
 */
extern char *extendedPrintDebugFunctionName;


/**
 * Store a mapping to a variable in a hash table.  Since the lookup will return
 * NULL on failure, we can't store a 0 as a variable ID, so increase by 1.
 */
inline void
insertMappingToVar(SmallHashTable *table, void *key, JITINT32 var) {
    /* smallHashTableInsert(table, key, (void *)(JITNINT)(var + 1)); */
    smallHashTableInsert(table, key, (void *)(JITNINT)var);
}


/**
 * Find a mapping to a variable in a hash table.  We haven't actually stored
 * the variable ID, but one more than it so that we can distinguish between
 * failed lookups and the value 0 being stored.
 */
inline JITINT32
lookupMappingToVar(SmallHashTable *table, void *key) {
    /* void *result = smallHashTableLookup(table, key); */
    /* if (result == NULL) { */
    /*   return -1; */
    /* } */
    /* return (JITINT32)(JITNINT)result - 1; */
    SmallHashTableItem *result = smallHashTableLookupItem(table, key);
    if (result == NULL) {
        return -1;
    }
    return (JITINT32)(JITNINT)result->element;
}


/**
 * Check whether a parameter type is an immediate integer type.
 */
inline JITBOOLEAN
isImmIntType(JITUINT32 type) {
    switch (type) {
        case IRINT8:
        case IRUINT8:
        case IRINT16:
        case IRUINT16:
        case IRINT32:
        case IRUINT32:
        case IRINT64:
        case IRUINT64:
        case IRNINT:
        case IRNUINT:
            return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Check whether a parameter is an immediate integer type.
 */
inline JITBOOLEAN
isImmIntParam(ir_item_t *param) {
    return isImmIntType(param->type);
}


/**
 * Check whether a parameter is an immediate with the value zero.  If it is
 * a register or other non-constant then it is not zero.  Floating point
 * values are also counted as non-zero here.
 */
inline JITBOOLEAN
isZeroImmParam(ir_item_t *param) {
    if (isImmIntType(param->type) && param->value.v == 0) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Check whether a parameter is an immediate with a non-zero value.  If it
 * is a register or other non-constant then it is not non-zero.  Floating
 * point values are also counted as not non-zero here.
 */
inline JITBOOLEAN
isNonZeroImmParam(ir_item_t *param) {
    if (isImmIntType(param->type) && param->value.v != 0) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Check whether a parameter has a global pointer type.
 */
inline JITBOOLEAN
paramIsGlobalPointer(ir_item_t *param) {
    /*   switch (param->type) { */
    /*   case IROBJECT: */
    /*   case IRMPOINTER: */
    /*   case IRTPOINTER: */
    /*   case IRSYMBOL: */
    /*   case IRFPOINTER: */
    /*     return JITTRUE; */
    /*   } */
    /*   return JITFALSE; */
    if (param->type == IROFFSET) {
        return JITFALSE;
    }
    return IRDATA_isAGlobalVariable(param) || IRDATA_isAFunctionPointer(param);
}


/**
 * Check for a variable parameter type.
 */
inline JITBOOLEAN
isVariableParam(ir_item_t *param) {
    return param->type == IROFFSET;
}


/**
 * Check for a symbol parameter and an immediate integer parameter in
 * either order.
 */
inline JITBOOLEAN
areSymbolAndImmIntParams(ir_item_t *param1, ir_item_t *param2) {
    if ((param1->type == IRSYMBOL || param2->type == IRSYMBOL) && (isImmIntType(param1->type) || isImmIntType(param2->type))) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Enable extended debug printing for certain functions.
 */
#ifdef PRINTDEBUG
JITBOOLEAN
enableExtendedDebugPrinting(ir_method_t *method) {
    if (extendedPrintDebugFunctionName == NULL) {
        return JITFALSE;
    } else if (STRCMP(extendedPrintDebugFunctionName, IRMETHOD_getMethodName(method)) == 0) {
        return JITTRUE;
    }
    return JITFALSE;
}
#endif  /* ifdef PRINTDEBUG */
