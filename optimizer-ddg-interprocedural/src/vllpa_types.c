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
 * This file implements the types for the VLLPA algorithm from the following
 * paper:
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
#include <xanlib.h>

// My headers
#include "optimizer_ddg.h"
#include "vllpa_macros.h"
#include "vllpa_memory.h"
#include "vllpa_types.h"
#include "vset.h"
// End


/**
 * These constants control the nesting level that uivs are allowed.  If
 * unconstrained then the analysis is its most accurate, but also takes
 * the longest and uses the most memory.  In particular, at the moment
 * this must be constrained slightly to get certain cpu2000 benchmarks to
 * pass through this optimiser.
 */
static const JITBOOLEAN constrainUivNesting = CONSTRAIN_UIV_NESTING;
static const JITUINT8 maxUivNestingLevel = MAX_UIV_NESTING_LEVEL;


/**
 * These constants control the offsets generated for abstract addresses.
 * If unconstrained then the analysis its most accurate, but also takes
 * the longest and uses the most memory.  In particular, at the moment
 * this must be constrained slightly to get certain cpu2000 benchmarks to
 * pass through this optimiser.
 */
static const JITBOOLEAN constrainAbsAddrOffsets = CONSTRAIN_ABS_ADDR_OFFSETS;
static const JITNINT maxAbsAddrOffset = MAX_ABS_ADDR_OFFSET;
static const JITNINT minAbsAddrOffset = MIN_ABS_ADDR_OFFSET;


/**
 * This constant determines whether merges between abstract addresses with
 * the same uiv (i.e. only their offsets differ) should be merged to zero
 * or not.  Merging to zero each time reduces accuracy but could prevent
 * issues with too many offsets being created.  This hasn't been tested
 * well, but is here for future use, if required.
 */
static const JITBOOLEAN mergeAbsAddrOffsetsToZero = JITFALSE;


/**
 * A global hash table to hold all uivs created during this pass.  The hash
 * table key is the type of uiv.  This prevents the same uivs being created
 * for each method in the application.  Ideally, this global would be
 * removed and put in a local structure that can be passed around, but for
 * now it's just here.
 */
static SmallHashTable *allUivs;


/**
 * A global hash table to hold all abstract addresses created during this
 * pass.  The hash table key is the uiv within the abstract address.  This
 * prevents the same abstract addresses being created for each method in
 * the application.  Ideally, this global would be removed and put in a
 * a local structure that can be passed around, but for now it's just here.
 */
static SmallHashTable *allAbsAddrsTable;


/**
 * A global hash table to hold all merges that are defined between abstract
 * addresses.  The hash table key is the uiv within the abstract address.
 * This is basically a cache for all merges, avoiding them being created
 * multiple times within each method.
 */
static SmallHashTable *allMerges;


/**
 * A global hash table of abstract address sets that are shared.  The hash
 * table maps the set to the number of sharers.  The abstract address set can
 * only be properly freed if it is not in this table.
 */
static SmallHashTable *sharedAbsAddrSets;


/**
 * Internal-only functions.
 */
static abstract_addr_t * absAddrSetNextNoChecks(abs_addr_set_t *set, abstract_addr_t *prev);
static void freeAbstractAddressSetNoChecks(abs_addr_set_t *aaSet);
#ifdef PRINTDEBUG
static void printAbstractAddressActualSet(VSet *set);
#else
#define printAbstractAddressActualSet(set)
#endif  /* ifdef PRINTDEBUG */


/**
 * For debugging memory leaks.
 */
#ifdef MEMUSE_DEBUG
static JITUINT32 uivsAllocated = 0, maxUivsAllocated = 0;
static JITUINT32 absAddrsAllocated = 0, maxAbsAddrsAllocated = 0;
static JITUINT32 absAddrSetsAllocated = 0, maxAbsAddrSetsAllocated = 0;
static SmallHashTable *allAbsAddrSetsTable;

void
vllpaTypesPrintMemUse(void) {
    /* Directly print, since PRINT_DEBUG might not be enabled. */
    fprintf(stderr, "Uivs:            %8u : %10llu\tMax: %8u : %10llu\n", uivsAllocated, (JITUINT64)uivsAllocated * sizeof(uiv_t), maxUivsAllocated, (JITUINT64)maxUivsAllocated * sizeof(uiv_t));
    fprintf(stderr, "AbsAddrs:        %8u : %10llu\tMax: %8u : %10llu\n", absAddrsAllocated, (JITUINT64)absAddrsAllocated * sizeof(abstract_addr_t), maxAbsAddrsAllocated, (JITUINT64)maxAbsAddrsAllocated * sizeof(abstract_addr_t));
    fprintf(stderr, "AbsAddr Sets:    %8u : %10llu\tMax: %8u : %10llu\n", absAddrSetsAllocated, (JITUINT64)absAddrSetsAllocated * sizeof(abs_addr_set_t), maxAbsAddrSetsAllocated, (JITUINT64)maxAbsAddrSetsAllocated * sizeof(abs_addr_set_t));
}

void
vllpaTypesPrintAbsAddrSets(void) {
    JITUINT32 numVSets = 0, numSingle = 0, numSets = 0, numEmpty = 0;
    JITUINT32 vsetSizes[6] = {0, 0, 0, 0, 0, 0};
    SmallHashTableItem *setItem = smallHashTableFirst(allAbsAddrSetsTable);
    while (setItem) {
        abs_addr_set_t *aaSet = (abs_addr_set_t *)setItem->key;
        fprintf(stderr, "Still allocated %p type %u\n", aaSet, fromPtr(JITUINT32, setItem->element));
        switch (absAddrSetGetType(aaSet)) {
            case ABS_ADDR_SET_EMPTY:
                numEmpty += 1;
                break;
            case ABS_ADDR_SET_SINGLE:
                numSingle += 1;
                break;
            case ABS_ADDR_SET_SET:
                numSets += 1;
                break;
            case ABS_ADDR_SET_VSET: {
                VSet *vset = absAddrSetGetVSet(aaSet);
                JITUINT32 size = vSetSize(vset);
                if (size < 5) {
                    vsetSizes[size] += 1;
                } else {
                    vsetSizes[5] += 1;
                }
                numVSets += 1;
            }
            break;
            default:
                abort();
        }
        setItem = smallHashTableNext(allAbsAddrSetsTable, setItem);
    }
    fprintf(stderr, "AbsAddr sets as vsets: %u\n", numVSets);
    fprintf(stderr, "Of those, size 0: %u, size 1: %u, size 2: %u, size 3: %u, size 4: %u, more: %u\n", vsetSizes[0], vsetSizes[1], vsetSizes[2], vsetSizes[3], vsetSizes[4], vsetSizes[5]);
}
#endif  /* ifdef MEMUSE_DEBUG */


/**
 * Initialise types required for this analysis.
 */
void
vllpaInitTypes(void) {
    JITUINT32 u;

    /* Create global abstract address hash table and array. */
    allAbsAddrsTable = allocateNewHashTable();
    allMerges = allocateNewHashTable();
    sharedAbsAddrSets = allocateNewHashTable();

    /* Create global uiv hash table. */
    allUivs = allocateNewHashTable();
    for (u = UIV_FIELD; u <= UIV_SPECIAL; ++u) {
        VSet *uivSet = allocateNewSet(15);
        smallHashTableInsert(allUivs, toPtr(u), uivSet);
    }

#ifdef MEMUSE_DEBUG
    allAbsAddrSetsTable = allocateNewHashTable();
#endif  /* ifdef MEMUSE_DEBUG */
}


/**
 * Free memory used by types for this analysis.
 */
void
vllpaFinishTypes(void) {
    SmallHashTableItem *tableItem;

    /* Free the shared abstract address set table. */
    assert(smallHashTableSize(sharedAbsAddrSets) == 0);
    freeHashTable(sharedAbsAddrSets);

    /* Free all abstract address merges used by the pass. */
    tableItem = smallHashTableFirst(allMerges);
    while (tableItem) {
        VSet *mergeSet = tableItem->element;
        VSetItem *mergeItem = vSetFirst(mergeSet);
        while (mergeItem) {
            aa_merge_t *merge = getSetItemData(mergeItem);
            vllpaFreeFunction(merge, sizeof(aa_merge_t));
            mergeItem = vSetNext(mergeSet, mergeItem);
        }
        freeSet(mergeSet);
        tableItem = smallHashTableNext(allMerges, tableItem);
    }
    freeHashTable(allMerges);

    /* Free all abstract addresses used by the pass. */
    tableItem = smallHashTableFirst(allAbsAddrsTable);
    while (tableItem) {
        SmallHashTable *aaMap = tableItem->element;
        while (smallHashTableSize(aaMap) > 0) {
            SmallHashTableItem *aaItem = smallHashTableFirst(aaMap);
            freeAbstractAddress(aaItem->element);
            smallHashTableRemove(aaMap, aaItem->key);
        }
        freeHashTable(aaMap);
        tableItem = smallHashTableNext(allAbsAddrsTable, tableItem);
    }
    freeHashTable(allAbsAddrsTable);

    /* Free all uivs used by the pass. */
    tableItem = smallHashTableFirst(allUivs);
    while (tableItem) {
        VSet *uivSet = tableItem->element;
        VSetItem *uivItem = vSetFirst(uivSet);
        while (uivItem) {

            /* Field uivs have lists of uivs in each set entry. */
            if (tableItem->key == toPtr(UIV_FIELD)) {
                field_uiv_holder_t *holder = getSetItemData(uivItem);
                if (holder->isList) {
                    XanListItem *uivListItem = xanList_first(holder->value.list);
                    while (uivListItem) {
                        uiv_t *uiv = uivListItem->data;
                        freeUivLevel(uiv);
                        uivListItem = xanList_next(uivListItem);
                    }
                    freeList(holder->value.list);
                } else {
                    freeUivLevel(holder->value.uiv);
                }
                vllpaFreeFunction(holder, sizeof(field_uiv_holder_t));
            }

            /* All others have one uiv in each set entry. */
            else {
                uiv_t *uiv = getSetItemData(uivItem);
                freeUivLevel(uiv);
            }
            uivItem = vSetNext(uivSet, uivItem);
        }
        freeSet(uivSet);
        tableItem = smallHashTableNext(allUivs, tableItem);
    }
    freeHashTable(allUivs);

#ifdef MEMUSE_DEBUG
    /* Make sure everything got deallocated. */
    assert(uivsAllocated == 0);
    assert(absAddrsAllocated == 0);
    assert(absAddrSetsAllocated == 0);
    assert(smallHashTableSize(allAbsAddrSetsTable) == 0);
    freeHashTable(allAbsAddrSetsTable);
#endif  /* ifdef MEMUSE_DEBUG */
}


/**
 * Functions for dealing with uivs.
 */


/**
 * Allocate a new unknown initial value.
 */
uiv_t *
allocUiv(void) {
    uiv_t *uiv = vllpaAllocFunction(sizeof(uiv_t), 19);
#ifdef MEMUSE_DEBUG
    ++uivsAllocated;
    maxUivsAllocated = MAX(uivsAllocated, maxUivsAllocated);
#endif
    return uiv;
}


/**
 * Free an unknown initial value.
 */
void
freeUivLevel(uiv_t *uiv) {
    vllpaFreeFunction(uiv, sizeof(uiv_t));
#ifdef MEMUSE_DEBUG
    assert(uivsAllocated > 0);
    --uivsAllocated;
#endif
}


/**
 * Get the base unknown initial value within the given unknown initial
 * value.
 */
inline uiv_t *
getBaseUiv(uiv_t *uiv) {
    assert(uiv);
    /*   if (uiv->type == UIV_FIELD) { */
    /*     return getBaseUiv(uiv->value.field.inner); */
    /*   } */
    return uiv->base;
}


/**
 * Get the nesting level of the base unknown initial value within the
 * given unknown initial value.
 */
JITUINT32
getLevelOfBaseUiv(uiv_t *uiv) {
    assert(uiv);
    if (uiv->type == UIV_FIELD) {
        return getLevelOfBaseUiv(uiv->value.field.inner) + 1;
    }
    return 0;
}


/**
 * Get the offset associated with the field uiv at the given level from
 * within the given unknown initial value.  Returns -1 if the given uiv
 * is not a field type.
 */
JITINT32
getFieldOffsetAtLevelFromUiv(uiv_t *uiv, JITUINT32 level) {
    assert(uiv);
    if (constrainUivNesting && maxUivNestingLevel <= level) {
        if (maxUivNestingLevel == 0) {
            return 0;
        }
        level = maxUivNestingLevel;
    }
    assert(uiv->type == UIV_FIELD);
    if (level != 0) {
        return getFieldOffsetAtLevelFromUiv(uiv->value.field.inner, level - 1);
    }
    return uiv->value.field.offset;
}


/**
 * Get the unknown initial value within the given unknown initial value
 * at the given nesting level.
 */
uiv_t *
getUivAtLevel(uiv_t *uiv, JITUINT32 level) {
    assert(uiv);
    if (constrainUivNesting && maxUivNestingLevel < level) {
        level = maxUivNestingLevel;
    }
    if (level == 0) {
        return uiv;
    }
    assert(uiv->type == UIV_FIELD);
    return getUivAtLevel(uiv->value.field.inner, level - 1);
}


/**
 * Get a set of uivs that make up the given uiv.
 */
VSet *
getConstituentUivSet(uiv_t *uiv) {
    VSet *set = allocateNewSet(16);
    vSetInsert(set, uiv);
    while (uiv != uiv->base) {
        assert(uiv->type == UIV_FIELD);
        uiv = uiv->value.field.inner;
        vSetInsert(set, uiv);
    }
    return set;
}


/**
 * Find a uiv in a field uiv holder.
 */
static inline uiv_t *
findFieldUivInHolder(uiv_t *inner, JITNINT offset, field_uiv_holder_t *holder) {
    if (holder->isList) {
        XanListItem *uivListItem = xanList_first(holder->value.list);
        while (uivListItem) {
            uiv_t *uiv = uivListItem->data;
            assert(uiv->value.field.inner == inner);
            if (uiv->value.field.offset == offset) {
                return uiv;
            }
            uivListItem = xanList_next(uivListItem);
        }
    } else {
        assert(holder->value.uiv->value.field.inner == inner);
        if (holder->value.uiv->value.field.offset == offset) {
            return holder->value.uiv;
        }
    }
    return NULL;
}


/**
 * Insert a uiv into a field uiv holder.
 */
static inline void
insertFieldUivIntoHolder(uiv_t *uiv, field_uiv_holder_t *holder, JITBOOLEAN hasUiv) {
    if (!hasUiv) {
        holder->value.uiv = uiv;
    } else {
        if (!holder->isList) {
            uiv_t *oldUiv = holder->value.uiv;
            holder->isList = JITTRUE;
            holder->value.list = allocateNewList();
            xanList_insert(holder->value.list, oldUiv);
        }
        xanList_insert(holder->value.list, uiv);
    }
}


/**
 * Create a new unknown initial value from another.
 */
uiv_t *
newUivFromUiv(uiv_t *inner, JITNINT offset, SmallHashTable *uivTable) {
    uiv_t *uiv;
    VSet *uivSetGlobal;
    field_uiv_holder_t *holderLocal;
    field_uiv_holder_t *holderGlobal;
    JITBOOLEAN localHasUiv;
    JITBOOLEAN globalHasUiv;

    /* If uiv levels are constrained, ensure this meets those constraints. */
    if (constrainUivNesting && getLevelOfBaseUiv(inner) >= maxUivNestingLevel) {
        return inner;
    }

    /* Search for a matching uiv in the local table. */
    if (uivTable) {
        VSet *uivSetLocal = smallHashTableLookup(uivTable, toPtr(UIV_FIELD));
        holderLocal = vSetFindWithKey(uivSetLocal, inner);
        if (holderLocal) {
            localHasUiv = JITTRUE;
            uiv = findFieldUivInHolder(inner, offset, holderLocal);
            if (uiv) {
                return uiv;
            }
        } else {
            holderLocal = vllpaAllocFunction(sizeof(field_uiv_holder_t), 20);
            holderLocal->isList = JITFALSE;
            localHasUiv = JITFALSE;
            vSetInsertWithKey(uivSetLocal, inner, holderLocal);
        }
    } else {
        holderLocal = NULL;
    }

    /* Search for a matching uiv in the global table. */
    uivSetGlobal = smallHashTableLookup(allUivs, toPtr(UIV_FIELD));
    holderGlobal = vSetFindWithKey(uivSetGlobal, inner);
    if (holderGlobal) {
        globalHasUiv = JITTRUE;
        uiv = findFieldUivInHolder(inner, offset, holderGlobal);
        if (uiv) {
            if (holderLocal) {
                insertFieldUivIntoHolder(uiv, holderLocal, localHasUiv);
            }
            return uiv;
        }
    } else {
        holderGlobal = vllpaAllocFunction(sizeof(field_uiv_holder_t), 21);
        holderGlobal->isList = JITFALSE;
        globalHasUiv = JITFALSE;
        vSetInsertWithKey(uivSetGlobal, inner, holderGlobal);
    }

    /* Not found a matching uiv in the list. */
    uiv = allocUiv();
    uiv->type = UIV_FIELD;
    uiv->value.field.inner = inner;
    uiv->value.field.offset = offset;
    uiv->base = inner->base;
    if (holderLocal) {
        insertFieldUivIntoHolder(uiv, holderLocal, localHasUiv);
    }
    insertFieldUivIntoHolder(uiv, holderGlobal, globalHasUiv);
    return uiv;
}


/**
 * Create a new unknown initial value from a register.
 */
uiv_t *
newUivFromVar(ir_method_t *method, IR_ITEM_VALUE var, SmallHashTable *uivTable) {
    uiv_t *uiv;
    VSet *uivSetLocal;
    VSet *uivSetGlobal;
    VSetItem *uivItem;

    /* Search for a matching uiv in the local table. */
    uivSetLocal = smallHashTableLookup(uivTable, toPtr(UIV_VAR));
    uivItem = vSetFirst(uivSetLocal);
    while (uivItem) {
        uiv = getSetItemData(uivItem);
        if (uiv->value.var.method == method && uiv->value.var.var == var) {
            return uiv;
        }
        uivItem = vSetNext(uivSetLocal, uivItem);
    }

    /* Search for a matching uiv in the global table. */
    uivSetGlobal = smallHashTableLookup(allUivs, toPtr(UIV_VAR));
    uivItem = vSetFirst(uivSetGlobal);
    while (uivItem) {
        uiv = getSetItemData(uivItem);
        if (uiv->value.var.method == method && uiv->value.var.var == var) {
            vSetInsert(uivSetLocal, uiv);
            return uiv;
        }
        uivItem = vSetNext(uivSetGlobal, uivItem);
    }

    /* Not found a matching uiv in the list. */
    uiv = allocUiv();
    uiv->type = UIV_VAR;
    uiv->value.var.method = method;
    uiv->value.var.var = var;
    uiv->base = uiv;
    vSetInsert(uivSetLocal, uiv);
    vSetInsert(uivSetGlobal, uiv);
    return uiv;
}


/**
 * Create a new unknown initial value from an alloc call.
 */
uiv_t *
newUivFromAlloc(ir_method_t *method, ir_instruction_t *inst, SmallHashTable *uivTable) {
    uiv_t *uiv;
    VSet *uivSetLocal;
    VSet *uivSetGlobal;

    /* Search for a matching uiv in the local table. */
    uivSetLocal = smallHashTableLookup(uivTable, toPtr(UIV_ALLOC));
    uiv = vSetFindWithKey(uivSetLocal, inst);
    if (uiv) {
        assert(uiv->value.alloc.method == method && uiv->value.alloc.inst == inst);
        return uiv;
    }

    /* Search for a matching uiv in the global table. */
    uivSetGlobal = smallHashTableLookup(allUivs, toPtr(UIV_ALLOC));
    uiv = vSetFindWithKey(uivSetGlobal, inst);
    if (uiv) {
        assert(uiv->value.alloc.method == method && uiv->value.alloc.inst == inst);
        vSetInsertWithKey(uivSetLocal, inst, uiv);
        return uiv;
    }

    /* Not found a matching uiv in the list. */
    uiv = allocUiv();
    uiv->type = UIV_ALLOC;
    uiv->value.alloc.method = method;
    uiv->value.alloc.inst = inst;
    uiv->base = uiv;
    vSetInsertWithKey(uivSetLocal, inst, uiv);
    vSetInsertWithKey(uivSetGlobal, inst, uiv);
    return uiv;
}


/**
 * Create a new unknown initial value from a resolved function pointer.
 */
static uiv_t *
newUivFromResolvedFunc(ir_method_t *method, SmallHashTable *uivTable) {
    uiv_t *uiv;
    VSet *uivSetLocal;
    VSet *uivSetGlobal;

    /* Search for a matching uiv in the local table. */
    uivSetLocal = smallHashTableLookup(uivTable, toPtr(UIV_FUNC));
    uiv = vSetFindWithKey(uivSetLocal, method);
    if (uiv) {
        assert(uiv->value.method == method);
        return uiv;
    }

    /* Search for a matching uiv in the global table. */
    uivSetGlobal = smallHashTableLookup(allUivs, toPtr(UIV_FUNC));
    uiv = vSetFindWithKey(uivSetGlobal, method);
    if (uiv) {
        assert(uiv->value.method == method);
        vSetInsertWithKey(uivSetLocal, method, uiv);
        return uiv;
    }

    /* Not found a matching uiv in the list. */
    uiv = allocUiv();
    uiv->type = UIV_FUNC;
    uiv->value.method = method;
    uiv->base = uiv;
    vSetInsertWithKey(uivSetLocal, method, uiv);
    vSetInsertWithKey(uivSetGlobal, method, uiv);
    return uiv;
}


/**
 * Create a new unknown initial value from a function pointer.
 */
uiv_t *
newUivFromFunc(IR_ITEM_VALUE addr, SmallHashTable *uivTable) {
    ir_method_t *method;
    ir_value_t entryPoint;

    /* Resolve the function pointer. */
    entryPoint = IRSYMBOL_resolveSymbol(toPtr(addr));
    method = IRMETHOD_getIRMethodFromEntryPoint(toPtr(entryPoint.v));

    /* Return a new uiv. */
    return newUivFromResolvedFunc(method, uivTable);
}


/**
 * Create a new unknown initial value from a global.
 */
uiv_t *
newUivFromGlobal(ir_item_t *param, JITNINT offset, SmallHashTable *uivTable) {
    uiv_t *uiv;
    VSet *uivSetLocal;
    VSet *uivSetGlobal;
    JITNUINT addr;

    /* Search for a matching uiv in the local table. */
    addr = param->value.v + offset;
    uivSetLocal = smallHashTableLookup(uivTable, toPtr(UIV_GLOBAL));
    uiv = vSetFindWithKey(uivSetLocal, toPtr(addr));
    if (uiv) {
        assert(uiv->value.global.param->value.v == param->value.v && uiv->value.global.offset == offset);
        return uiv;
    }

    /* Search for a matching uiv in the global table. */
    uivSetGlobal = smallHashTableLookup(allUivs, toPtr(UIV_GLOBAL));
    uiv = vSetFindWithKey(uivSetGlobal, toPtr(addr));
    if (uiv) {
        assert(uiv->value.global.param->value.v == param->value.v && uiv->value.global.offset == offset);
        vSetInsertWithKey(uivSetLocal, toPtr(addr), uiv);
        return uiv;
    }

    /* Not found a matching uiv in the list. */
    uiv = allocUiv();
    uiv->type = UIV_GLOBAL;
    uiv->value.global.param = param;
    uiv->value.global.offset = offset;
    uiv->base = uiv;
    vSetInsertWithKey(uivSetLocal, toPtr(addr), uiv);
    vSetInsertWithKey(uivSetGlobal, toPtr(addr), uiv);
    return uiv;
}


/**
 * Create a new unknown initial value from a global.  This is also called
 * with function pointer parameters, so a check distinguishes between
 * them and selects the correct one to use.
 */
uiv_t *
newUivFromGlobalParam(ir_item_t *param, SmallHashTable *uivTable) {
    if (IRDATA_isAFunctionPointer(param)) {
        return newUivFromFunc(param->value.v, uivTable);
    } else {
        return newUivFromGlobal(param, 0, uivTable);
    }
}


/**
 * Create a new unknown initial value based on the given unknown initial
 * value parameter.  This is useful for creating a new uiv within 'uivTable'
 * when it doesn't already exist.  For example, within a different method.
 */
uiv_t *
newUivBasedOnUiv(uiv_t *uiv, SmallHashTable *uivTable) {
    uiv_t *uivInner;
    switch (uiv->type) {
        case UIV_FIELD:
            uivInner = newUivBasedOnUiv(uiv->value.field.inner, uivTable);
            return newUivFromUiv(uivInner, uiv->value.field.offset, uivTable);
        case UIV_VAR:
            return newUivFromVar(uiv->value.var.method, uiv->value.var.var, uivTable);
        case UIV_ALLOC:
            return newUivFromAlloc(uiv->value.alloc.method, uiv->value.alloc.inst, uivTable);
        case UIV_GLOBAL:
            return newUivFromGlobal(uiv->value.global.param, uiv->value.global.offset, uivTable);
        case UIV_FUNC:
            return newUivFromResolvedFunc(uiv->value.method, uivTable);
        case UIV_SPECIAL:
            return newUivSpecial(uiv->value.id, uivTable);
        default:
            abort();
    }
}


/**
 * Create a new special unknown initial value using the given ID.
 */
uiv_t *
newUivSpecial(uiv_special_t id, SmallHashTable *uivTable) {
    uiv_t *uiv;
    VSet *uivSetLocal;
    VSet *uivSetGlobal;

    /* Search for a matching uiv in the local table. */
    uivSetLocal = smallHashTableLookup(uivTable, toPtr(UIV_SPECIAL));
    uiv = vSetFindWithKey(uivSetLocal, toPtr(id));
    if (uiv) {
        assert(uiv->value.id == id);
        return uiv;
    }

    /* Search for a matching uiv in the global table. */
    uivSetGlobal = smallHashTableLookup(allUivs, toPtr(UIV_SPECIAL));
    uiv = vSetFindWithKey(uivSetGlobal, toPtr(id));
    if (uiv) {
        assert(uiv->value.id == id);
        vSetInsertWithKey(uivSetLocal, toPtr(id), uiv);
        return uiv;
    }

    /* Not found a matching uiv in the list. */
    uiv = allocUiv();
    uiv->type = UIV_SPECIAL;
    uiv->value.id = id;
    uiv->base = uiv;
    vSetInsertWithKey(uivSetLocal, toPtr(id), uiv);
    vSetInsertWithKey(uivSetGlobal, toPtr(id), uiv);
    return uiv;
}


/**
 * Return JITTRUE if the given uiv is used within the method that contains
 * the given uiv table.
 */
JITBOOLEAN
uivExistsInTable(uiv_t *uiv, SmallHashTable *uivTable) {
    VSet *uivSetLocal;
    switch (uiv->type) {
        case UIV_ALLOC:
            uivSetLocal = smallHashTableLookup(uivTable, (void *)(JITNUINT)UIV_ALLOC);
            return vSetFindWithKey(uivSetLocal, uiv->value.alloc.inst) != NULL;
        default:
            abort();
    }
}


/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a variable type.
 */
JITBOOLEAN
isBaseUivFromVar(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->type == UIV_VAR;
}


/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a function pointer type.
 */
JITBOOLEAN
isBaseUivFromFunc(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->type == UIV_FUNC;
}


/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a global type.
 */
JITBOOLEAN
isBaseUivFromGlobal(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->type == UIV_GLOBAL;
}


/**
 * Return JITTRUE if the base unknown initial value within the given
 * unknown initial value is a global type from a symbol.
 */
JITBOOLEAN
isBaseUivFromGlobalSymbol(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->type == UIV_GLOBAL && baseUiv->value.global.param->type == IRSYMBOL;
}


/**
 * Get the variable from the base unknown initial value within the given
 * unknown initial value.  Does no checking but assumes that the base is
 * definitely a variable type.
 */
JITUINT32
getVarFromBaseUiv(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->value.var.var;
}


/**
 * Get the instruction that allocates memory from the base unknown initial
 * value within the given unknown initial value.  Does no checking but
 * assumes that the base is definitely an alloc type.
 */
ir_instruction_t *
getInstFromBaseUiv(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->value.alloc.inst;
}


/**
 * Get the parameter that represents the global from the base unknown initial
 * value within the given unknown initial value.  Does no checking but assumes
 * that the base is definitely a global type.
 */
ir_item_t *
getParamFromGlobalBaseUiv(uiv_t *uiv) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    return baseUiv->value.global.param;
}


/**
 * Check whether prefix is a prefix of uiv, with fewer options.
 */
static JITBOOLEAN
isUivAPrefixHelper(uiv_t *prefix, uiv_t *uiv) {
    if (prefix == uiv) {
        return JITTRUE;
    } else if (uiv->type == UIV_FIELD) {
        return isUivAPrefix(prefix, uiv->value.field.inner);
    }
    return JITFALSE;
}


/**
 * Check whether prefix is a prefix of uiv.
 */
JITBOOLEAN
isUivAPrefix(uiv_t *prefix, uiv_t *uiv) {
    if (prefix == uiv) {
        return JITTRUE;
    } else if (uiv->type == UIV_FIELD) {
        if (getBaseUiv(prefix) != getBaseUiv(uiv)) {
            return JITFALSE;
        }
        return isUivAPrefixHelper(prefix, uiv->value.field.inner);
    }
    return JITFALSE;
}


/**
 * Return a number less than zero if the first uiv should come before the
 * second in a sorted set, or greater than zero if it should come afterwards.
 */
static int
compareUivs(uiv_t *first, uiv_t *second) {
    uiv_t *firstBase = getBaseUiv(first);
    uiv_t *secondBase = getBaseUiv(second);

    /* Easy option, the same so sorted. */
    if (first == second) {
        return 0;
    }

    /* Ordering is variables, then globals, then allocs, then pointers, special. */
    if (firstBase->type != secondBase->type) {
        switch (firstBase->type) {
            case UIV_VAR:
                return -1;
            case UIV_GLOBAL:
              return secondBase->type != UIV_VAR ? -1 : 1;
            case UIV_ALLOC:
              return (secondBase->type == UIV_FUNC || secondBase->type == UIV_SPECIAL) ? -1 : 1;
            case UIV_FUNC:
              return secondBase->type == UIV_SPECIAL ? -1 : 1;
            case UIV_SPECIAL:
                return 1;
            default:
                abort();
        }
    }

    /* Order within the same base type. */
    switch (firstBase->type) {
        case UIV_VAR: {
            JITINT32 cmp = STRCMP(IRMETHOD_getMethodName(firstBase->value.var.method), IRMETHOD_getMethodName(secondBase->value.var.method));
            if (cmp < 0) {
                return -1;
            } else if (cmp > 0) {
                return 1;
            } else {
                if (firstBase->value.var.var < secondBase->value.var.var) {
                    return -1;
                } else if (firstBase->value.var.var > secondBase->value.var.var) {
                    return 1;
                }
            }
            break;
        }
        case UIV_GLOBAL:
            if (firstBase->value.global.param < secondBase->value.global.param) {
                return -1;
            } else if (firstBase->value.global.param > secondBase->value.global.param) {
                return 1;
            } else {
                if (firstBase->value.global.offset < secondBase->value.global.offset) {
                    return -1;
                } else if (firstBase->value.global.offset > secondBase->value.global.offset) {
                    return 1;
                }
            }
            break;
        case UIV_ALLOC: {
            JITINT32 cmp = STRCMP(IRMETHOD_getMethodName(firstBase->value.alloc.method), IRMETHOD_getMethodName(secondBase->value.alloc.method));
            if (cmp < 0) {
                return -1;
            } else if (cmp > 0) {
                return 1;
            } else {
                if (firstBase->value.alloc.inst->ID < secondBase->value.alloc.inst->ID) {
                    return -1;
                } else if (firstBase->value.alloc.inst->ID > secondBase->value.alloc.inst->ID) {
                    return 1;
                }
            }
            break;
        }
        case UIV_FUNC: {
            JITINT32 cmp = STRCMP(IRMETHOD_getMethodName(firstBase->value.method), IRMETHOD_getMethodName(secondBase->value.method));
            if (cmp < 0) {
                return -1;
            } else if (cmp > 0) {
                return 1;
            }
            break;
        }
        case UIV_SPECIAL:
            if (firstBase->value.id < secondBase->value.id) {
                return -1;
            } else if (firstBase->value.id > secondBase->value.id) {
                return 1;
            }
            break;
        default:
            abort();
    }

    /* Same base, lowest levels first. */
    if (getLevelOfBaseUiv(first) < getLevelOfBaseUiv(second)) {
        return -1;
    } else if (getLevelOfBaseUiv(first) > getLevelOfBaseUiv(second)) {
        return 1;
    }

    /* Levels are the same, check offsets if UIV_FIELDS. */
    if (first->type == UIV_FIELD) {
        if (second->type == UIV_FIELD) {
            return first->value.field.offset <= second->value.field.offset ? -1 : 1;
        } else {
            return 1;
        }
    }

    /* First is not a field access, default to JITTRUE. */
    return -1;
}


/**
 * Check two unknown initial values to see if they match in terms of
 * their structure fields.  Returns JITTRUE if they match.
 */
#if defined(DEBUG) && (defined(CHECK_ABSTRACT_ADDRESSES) || defined(CHECK_UIVS))
JITBOOLEAN
checkWhetherUivsMatch(uiv_t *first, uiv_t *second) {
    assert(first && second);
    if (first->type == second->type) {
        switch (first->type) {
            case UIV_FIELD:
                if (checkWhetherUivsMatch(first->value.field.inner, second->value.field.inner)) {
                    return first->value.field.offset == second->value.field.offset;
                }
                break;
            case UIV_VAR:
                return first->value.var.method == second->value.var.method && first->value.var.var == second->value.var.var;
            case UIV_GLOBAL:
                return first->value.global.param == second->value.global.param && first->value.global.offset == second->value.global.offset;
            case UIV_ALLOC:
                return first->value.alloc.method == second->value.alloc.method && first->value.alloc.inst == second->value.alloc.inst;
            case UIV_FUNC:
                return first->value.method == second->value.method;
            case UIV_SPECIAL:
                return first->value.id == second->value.id;
        }
    }
    return JITFALSE;
}
#endif  /* if definded(DEBUG) && defined(CHECK_ABSTRACT_ADDRESSES) */


/**
 * Check an unknown initial value to ensure it is sane.
 */
#if defined(DEBUG) && (defined(CHECK_ABSTRACT_ADDRESSES) || defined(CHECK_UIVS))
void
checkUiv(uiv_t *uiv) {
    assert(uiv);
    switch (uiv->type) {
        case UIV_FIELD:
        case UIV_VAR:
        case UIV_ALLOC:
        case UIV_GLOBAL:
        case UIV_FUNC:
        case UIV_SPECIAL:
            break;
        default:
            abort();
    }
}
#endif  /* if defined(DEBUG) && (defined(CHECK_ABSTRACT_ADDRESSES) || defined(CHECK_UIVS)) */


/**
 * Print a single unknown initial value.
 */
#ifdef PRINTDEBUG
void
printUiv(uiv_t *uiv, char *prefix, JITBOOLEAN newline) {
    if (enablePDebug) {
        switch (uiv->type) {
            case UIV_FIELD:
                printUiv(uiv->value.field.inner, prefix, JITFALSE);
                if (uiv->value.field.offset == WHOLE_ARRAY_OFFSET) {
                    PDEBUGB("@*%s", newline ? "\n" : "");
                } else if (sizeof(JITNINT) == 8) {
                    PDEBUGB("@%lld%s", (JITINT64)uiv->value.field.offset, newline ? "\n" : "");
                } else {
                    PDEBUGB("@%d%s", (JITINT32)uiv->value.field.offset, newline ? "\n" : "");
                }
                break;
            case UIV_VAR:
                PDEBUGB("%s[%s+v%u]%s", prefix, IRMETHOD_getMethodName(uiv->value.var.method), uiv->value.var.var, newline ? "\n" : "");
                break;
            case UIV_ALLOC:
                PDEBUGB("%s[%s+%d(A)]%s", prefix, IRMETHOD_getMethodName(uiv->value.alloc.method), uiv->value.alloc.inst->ID, newline ? "\n" : "");
                break;
            case UIV_GLOBAL:
                PDEBUGB("%s[", prefix);
                switch (uiv->value.global.param->type) {
                    case IRSYMBOL:
                        IRMETHOD_dumpSymbol((ir_symbol_t *)(JITNUINT)uiv->value.global.param->value.v, stderr);
                        PDEBUGB("+%d(G)]%s", uiv->value.global.offset, newline ? "\n" : "");
                        break;
                    default:
                        abort();
                }
                break;
            case UIV_FUNC:
                PDEBUGB("%s[%s(F)]%s", prefix, IRMETHOD_getMethodName(uiv->value.method), newline ? "\n" : "");
                break;
            case UIV_SPECIAL:
                PDEBUGB("%s[%u(S)]%s", prefix, uiv->value.id, newline ? "\n" : "");
                break;
            default:
                abort();
        }
    }
}
#else
#define printUiv(uiv, prefix, newline)
#endif  /* ifdef PRINTDEBUG */


/**
 * Print a single uiv making sure that debug printing is on first.
 */
#ifdef PRINTDEBUG
static void
printUivEnablePrint(uiv_t *uiv, char *prefix, JITBOOLEAN newline) {
    JITBOOLEAN oldEnablePDebug = enablePDebug;
    enablePDebug = JITTRUE;
    printUiv(uiv, prefix, newline);
    enablePDebug = oldEnablePDebug;
}
#else
#define printUivEnablePrint(uiv, prefix, newline)
#endif  /* ifdef PRINTDEBUG */


/**
 * Functions for dealing with abstract addresses.
 */


/**
 * Allocate a new abstract address.
 */
static inline abstract_addr_t *
allocAbstractAddress(void) {
    abstract_addr_t *aa = vllpaAllocFunction(sizeof(abstract_addr_t), 22);
#ifdef MEMUSE_DEBUG
    ++absAddrsAllocated;
    maxAbsAddrsAllocated = MAX(absAddrsAllocated, maxAbsAddrsAllocated);
#endif
    return aa;
}


/**
 * Create a new abstract address with the given parameters.
 */
abstract_addr_t *
newAbstractAddress(uiv_t *uiv, JITNINT off) {
    abstract_addr_t *aa;
    SmallHashTable *aaMap;
    JITNINT offset;

    /* Debugging. */
    assert(uiv);

    /* Make sure any offset constraints are met, if there are any. */
    offset = off;
    if (constrainAbsAddrOffsets) {
        if (offset > maxAbsAddrOffset) {
            offset = maxAbsAddrOffset;
        } else if (offset < minAbsAddrOffset) {
            offset = minAbsAddrOffset;
        }
    }

    /* Search for the abstract address first. */
    aaMap = smallHashTableLookup(allAbsAddrsTable, uiv);
    if (aaMap) {
        aa = smallHashTableLookup(aaMap, toPtr(offset));
        if (aa) {
            return aa;
        }
    } else {
        aaMap = allocateNewHashTable();
        smallHashTableInsert(allAbsAddrsTable, uiv, aaMap);
    }

    /* Not found, allocate a new abstract address. */
    aa = allocAbstractAddress();
    aa->uiv = uiv;
    aa->offset = offset;

    /* Keep a track of it. */
    smallHashTableInsert(aaMap, toPtr(offset), aa);
    assert(smallHashTableLookup(allAbsAddrsTable, uiv) == aaMap);
    return aa;
}


/**
 * Return JITTRUE if the abstract address could represent a function
 * pointer.
 */
JITBOOLEAN
isAbstractAddressAFunctionPointer(abstract_addr_t *aa) {
    /* Must have no offset and a global uiv. */
    if (aa->offset == 0 && getLevelOfBaseUiv(aa->uiv) == 0 && aa->uiv->type == UIV_FUNC) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Check whether the first abstract address could be used as a prefix for the
 * second.
 */
static JITBOOLEAN
isAbstractAddressAPrefix(abstract_addr_t *first, abstract_addr_t *second) {
    /**
     * This doesn't account for abstract addresses with the same uiv but
     * different offsets, since the prefixUiv will now have more elements in than
     * the second one.
     *
     * For example,  <[uiv], 0> and <[uiv], 8> will check [uiv]@0 against [uiv].
     * Therefore we revert to just checking uivs and accept the small loss in
     * accuracy that this incurs.
     */
    uiv_t *prefixUiv = newUivFromUiv(first->uiv, first->offset, NULL);
    return isUivAPrefix(prefixUiv, second->uiv);
    /* return isUivAPrefix(first->uiv, second->uiv); */
}


/**
 * Check whether two abstract addresses are prefixes of each other, depending
 * on which one is allowed to be a prefix of the other.
 */
static inline JITBOOLEAN
areAbstractAddressesPrefixesOfEachOther(abstract_addr_t *first, abstract_addr_t *second, aaset_prefix_t checkPrefixType) {
    switch (checkPrefixType) {
        case AASET_PREFIX_NONE:
            break;
        case AASET_PREFIX_FIRST:
            if (isAbstractAddressAPrefix(first, second)) {
                return JITTRUE;
            }
            break;
        case AASET_PREFIX_SECOND:
            if (isAbstractAddressAPrefix(second, first)) {
                return JITTRUE;
            }
            break;
        case AASET_PREFIX_BOTH:
            if (isAbstractAddressAPrefix(first, second)) {
                return JITTRUE;
            } else if (isAbstractAddressAPrefix(second, first)) {
                return JITTRUE;
            }
            break;
        default:
            abort();
    }
    return JITFALSE;
}


/**
 * Allocate memory for an abstract address set.
 */
static inline abs_addr_set_t *
allocAbsAddrSet(void) {
    abs_addr_set_t *aaSet = vllpaAllocFunction(sizeof(abs_addr_set_t), 23);
#ifdef MEMUSE_DEBUG
    absAddrSetsAllocated += 1;
    maxAbsAddrSetsAllocated = MAX(absAddrSetsAllocated, maxAbsAddrSetsAllocated);
    smallHashTableInsert(allAbsAddrSetsTable, aaSet, aaSet);
#endif
    /* PDEBUG("Created new abstract address set %p\n", aaSet); */
    return aaSet;
}


/**
 * Free and abstract address set's memory.
 */
static inline void
freeAbsAddrSet(abs_addr_set_t *aaSet) {
#ifdef MEMUSE_DEBUG
    assert(absAddrSetsAllocated > 0);
    assert(smallHashTableContains(allAbsAddrSetsTable, aaSet));
    absAddrSetsAllocated -= 1;
    smallHashTableRemove(allAbsAddrSetsTable, aaSet);
#endif
    vllpaFreeFunction(aaSet, sizeof(abs_addr_set_t));
}


/**
 * Free an internal abstract address set's memory.
 */
static inline void
freeAbsAddrInternalSet(VSet *set) {
    VSetItem *setItem = vSetFirst(set);
    while (setItem) {
        abs_addr_set_t *innerSet = getSetItemData(setItem);
        switch (absAddrSetGetType(innerSet)) {
            case ABS_ADDR_SET_SINGLE:
                break;
            case ABS_ADDR_SET_VSET:
                freeSet(absAddrSetGetVSet(innerSet));
                break;
            default:
                abort();
        }
        freeAbsAddrSet(innerSet);
        setItem = vSetNext(set, setItem);
    }
    freeSet(set);
}


/**
 * Create a new abstract address set that is in single form.
 */
static inline abs_addr_set_t *
newAbstractAddressSetSingle(abstract_addr_t *aa) {
    abs_addr_set_t *aaSet = allocAbsAddrSet();
    absAddrSetSetSingle(aaSet, aa);
    return aaSet;
}


/**
 * Create a new abstract address set that is in set form.
 */
static inline abs_addr_set_t *
newAbstractAddressSetSet(JITUINT32 type) {
    abs_addr_set_t *aaSet = allocAbsAddrSet();
    VSet *set = allocateNewSet(type);
    absAddrSetSetSet(aaSet, set);
    return aaSet;
}


/**
 * Check a uiv is sane.
 */
#if defined(DEBUG) && defined(CHECK_UIVS)
static JITBOOLEAN
checkUivSanity(uiv_t *uiv) {
    assert(uiv);
    assert((((JITNUINT)uiv) & (sizeof(void *) -1)) == 0);
    assert(uiv->type <= UIV_SPECIAL);
    assert(uiv->base);
    assert((((JITNUINT)uiv->base) & (sizeof(void *) -1)) == 0);
    if (uiv->type == UIV_FIELD) {
        assert(checkUivSanity(uiv->value.field.inner));
    }
    return JITTRUE;
}
#else
#define checkUivSanity(uiv) JITTRUE
#endif  /* if defined(DEBUG) && defined(CHECK_UIVS) */

/**
 * Check an abstract address is sane.
 */
#if defined(DEBUG) && defined(CHECK_ABSTRACT_ADDRESSES)
static JITBOOLEAN
checkAbsAddrSanity(abstract_addr_t *aa) {
    assert(aa);
    assert((((JITNUINT)aa) & (sizeof(void *) - 1)) == 0);
    /* assert((((JITNUINT)aa->uiv) & (sizeof(void *) - 1)) == 0); */
    assert(checkUivSanity(aa->uiv));
    return JITTRUE;
}
#else
#define checkAbsAddrSanity(aa) JITTRUE
#endif  /* if defined(DEBUG) && defined(CHECK_ABSTRACT_ADDRESSES) */


/**
 * Check a set of abstract addresses in VSet format to ensure there's no
 * corruption.
 */
#if defined(DEBUG) && defined(CHECK_ABS_ADDR_SETS)
static JITBOOLEAN
checkAbsAddrSetSanityActualSet(VSet *set) {
    VSetItem *item;
    assert(set);
    item = vSetFirst(set);
    while (item) {
        abstract_addr_t *aa VLLPA_UNUSED = getSetItemData(item);
        assert(checkAbsAddrSanity(aa));
        item = vSetNext(set, item);
    }
    return JITTRUE;
}
#else
#define checkAbsAddrSetSanityActualSet(set) JITTRUE
#endif  /* if defined(DEBUG) && defined(CHECK_ABS_ADDR_SETS) */


/**
 * Check an internal abstract address set to ensure there's no corruption.
 */
#if defined(DEBUG) && defined(CHECK_ABS_ADDR_SETS)
static JITBOOLEAN
checkAbsAddrInternalSetSanity(VSet *set) {
    VSetItem *setItem = vSetFirst(set);
    while (setItem) {
        abs_addr_set_t *innerSet = getSetItemData(setItem);
        switch (absAddrSetGetType(innerSet)) {
            case ABS_ADDR_SET_SINGLE: {
                abstract_addr_t *single VLLPA_UNUSED = absAddrSetGetSingle(innerSet);
                assert(checkAbsAddrSanity(single));
                break;
            }

            case ABS_ADDR_SET_VSET: {
                VSet *vset = absAddrSetGetVSet(innerSet);
                assert(vSetSize(vset) > 1);
                assert(checkAbsAddrSetSanityActualSet(vset));
                break;
            }

            default:
                abort();
        }
        setItem = vSetNext(set, setItem);
    }
    return JITTRUE;
}
#else
#define checkAbsAddrInternalSetSanity(set) JITTRUE
#endif  /* if defined(DEBUG) && defined(CHECK_ABS_ADDR_SETS) */


/**
 * Check a set of abstract addresses to ensure there's no corruption.  Returns
 * JITTRUE if all is well, JITFALSE if there's a problem.
 */
#if defined(DEBUG) && defined(CHECK_ABS_ADDR_SETS)
static JITBOOLEAN
checkAbsAddrSetSanity(abs_addr_set_t *aaSet) {
    assert(aaSet);
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            return JITTRUE;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single VLLPA_UNUSED = absAddrSetGetSingle(aaSet);
            assert(checkAbsAddrSanity(single));
            break;
        }

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            assert(checkAbsAddrInternalSetSanity(set));
            break;
        }

        default:
            abort();
    }
    return JITTRUE;
}
#else
#define checkAbsAddrSetSanity(aaSet) JITTRUE
#endif  /* if defined(DEBUG) && defined(CHECK_ABS_ADDR_SETS) */


/**
 * Create a new abstract address set that is in empty form.
 */
inline abs_addr_set_t *
newAbstractAddressSetEmpty(void) {
    abs_addr_set_t *aaSet = allocAbsAddrSet();
    absAddrSetSetEmpty(aaSet);
    return aaSet;
}


/**
 * Create a new abstract address set containing one abstract address.
 */
inline abs_addr_set_t *
newAbstractAddressSet(abstract_addr_t *aa) {
    abs_addr_set_t *aaSet = newAbstractAddressSetSingle(aa);
    return aaSet;
}


/**
 * Create a new abstract address set containing a single new abstract
 * address made up from the given parameters.
 */
inline abs_addr_set_t *
newAbstractAddressInNewSet(uiv_t *uiv, JITNINT offset) {
    abstract_addr_t *aa = newAbstractAddress(uiv, offset);
    abs_addr_set_t *aaSet = newAbstractAddressSet(aa);
    return aaSet;
}


/**
 * Free an abstract address.
 */
void
freeAbstractAddress(abstract_addr_t *aa) {
    assert(checkAbsAddrSanity(aa));
    vllpaFreeFunction(aa, sizeof(abstract_addr_t));
#ifdef MEMUSE_DEBUG
    assert(absAddrsAllocated > 0);
    --absAddrsAllocated;
#endif
}


/**
 * Free a set of abstract addresses.  Only frees the memory used by the set
 * and leaves the abstract addresses alone.  Internal-only version with no
 * checks.
 */
static void
freeAbstractAddressSetNoChecks(abs_addr_set_t *aaSet) {
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            break;

        case ABS_ADDR_SET_SINGLE:
            break;

        case ABS_ADDR_SET_SET:
            freeAbsAddrInternalSet(absAddrSetGetSet(aaSet));
            break;

        default:
            abort();
    }
    freeAbsAddrSet(aaSet);
}


/**
 * Free a set of abstract addresses.  Only frees the memory used by the set
 * and leaves the abstract addresses alone.
 */
void
freeAbstractAddressSet(abs_addr_set_t *aaSet) {
    SmallHashTableItem *shared = smallHashTableLookupItem(sharedAbsAddrSets, aaSet);
    assert(checkAbsAddrSetSanity(aaSet));
    if (shared) {
        JITUINT32 sharers = fromPtr(JITUINT32, shared->element) - 1;
        if (sharers > 0) {
            shared->element = toPtr(sharers);
            /* PDEBUG("Sharing set %p with %d sharers\n", aaSet, fromPtr(JITUINT32, shared->element)); */
        } else {
            smallHashTableRemove(sharedAbsAddrSets, aaSet);
            /* PDEBUG("Sharing set %p with 0 sharers\n", aaSet); */
        }
    } else {
        /* PDEBUG("Freeing abstract address set %p\n", aaSet); */
        freeAbstractAddressSetNoChecks(aaSet);
    }
}


/**
 * Get the first abstract address in a set without sanity checks.  For internal
 * use only.
 */
static abstract_addr_t *
absAddrSetFirstNoChecks(abs_addr_set_t *aaSet) {
    switch(absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            return NULL;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            return single;
        }

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFirst(set);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE: {
                        abstract_addr_t *single = absAddrSetGetSingle(innerSet);
                        return single;
                    }
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        VSetItem *aaItem = vSetFirst(vset);
                        assert(vSetSize(vset) > 0);
                        return getSetItemData(aaItem);
                    }
                    default:
                        abort();
                }
            }
            break;
        }

        default:
            abort();
    }
    return NULL;
}


/**
 * Get the first abstract address in a set.
 */
abstract_addr_t *
absAddrSetFirst(abs_addr_set_t *aaSet) {
    abstract_addr_t *aa;
    assert(checkAbsAddrSetSanity(aaSet));
    aa = absAddrSetFirstNoChecks(aaSet);
    if (aa) {
        assert(checkAbsAddrSanity(aa));
    }
    return aa;
}


/**
 * Get the next abstract address in a set but don't do sanity checks.  This is
 * for internal use only.
 */
static abstract_addr_t *
absAddrSetNextNoChecks(abs_addr_set_t *aaSet, abstract_addr_t *prev) {
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_SINGLE:
            return NULL;
        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFindItem(set, getBaseUiv(prev->uiv));
            abs_addr_set_t *innerSet = getSetItemData(item);

            /* Iterate over inner set elements. */
            if (absAddrSetIsVSet(innerSet)) {
                VSet *vset = absAddrSetGetVSet(innerSet);
                VSetItem *aaItem = vSetFindItem(vset, prev);
                aaItem = vSetNext(vset, aaItem);
                if (aaItem) {
                    return getSetItemData(aaItem);
                }
            }

            /* Single inner set or finished all elements. */
            item = vSetNext(set, item);
            while (item) {
                innerSet = getSetItemData(item);
                if (absAddrSetIsSingle(innerSet)) {
                    return absAddrSetGetSingle(innerSet);
                } else {
                    VSet *vset = absAddrSetGetVSet(innerSet);
                    VSetItem *aaItem = vSetFirst(vset);
                    assert(vSetSize(vset) > 0);
                    return getSetItemData(aaItem);
                }
            }
            break;
        }
        default:
            abort();
    }
    return NULL;
}


/**
 * Get the next abstract address in a set.
 */
abstract_addr_t *
absAddrSetNext(abs_addr_set_t *aaSet, abstract_addr_t *prev) {
    abstract_addr_t *aa;
    assert(checkAbsAddrSetSanity(aaSet));
    assert(checkAbsAddrSanity(prev));
    aa = absAddrSetNextNoChecks(aaSet, prev);
    if (aa) {
        assert(checkAbsAddrSanity(aa));
    }
    return aa;
}


/**
 * Get the size of an inner abstract address set.
 */
static JITUINT32
absAddrInnerSetSize(abs_addr_set_t *innerSet) {
    switch (absAddrSetGetType(innerSet)) {
        case ABS_ADDR_SET_SINGLE:
            return 1;
        case ABS_ADDR_SET_VSET:
            return vSetSize(absAddrSetGetVSet(innerSet));
        default:
            abort();
    }
    return 0;
}


/**
 * Get the number of abstract addresses in an abstract address set without
 * checks.  For internal use only.
 */
static JITUINT32
absAddrSetSizeNoChecks(abs_addr_set_t *aaSet) {
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            return 0;
        case ABS_ADDR_SET_SINGLE:
            return 1;
        case ABS_ADDR_SET_SET: {
            JITUINT32 size = 0;
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFirst(set);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                size += absAddrInnerSetSize(innerSet);
                item = vSetNext(set, item);
            }
            return size;
        }
        default:
            abort();
    }
    return 0;
}


/**
 * Get the number of abstract addresses in an abstract address set.
 */
JITUINT32
absAddrSetSize(abs_addr_set_t *aaSet) {
    assert(checkAbsAddrSetSanity(aaSet));
    return absAddrSetSizeNoChecks(aaSet);
}


/**
 * Convert an abstract address set to a set that can contain multiple items.
 */
static inline void
convertAbsAddrSetToSet(abs_addr_set_t *aaSet) {
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            absAddrSetSetSet(aaSet, allocateNewSet(18));
            break;

        case ABS_ADDR_SET_SINGLE: {
            VSet *set = allocateNewSet(19);
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            abs_addr_set_t *innerSet = newAbstractAddressSetSingle(single);
            vSetInsertWithKey(set, getBaseUiv(single->uiv), innerSet);
            absAddrSetSetSet(aaSet, set);
            break;
        }

        case ABS_ADDR_SET_SET:
            break;

        default:
            abort();
    }
}


/**
 * Convert an abstract address set to a set that can contain only one item.
 */
static inline void
convertAbsAddrSetToSingle(abs_addr_set_t *aaSet) {
    assert(absAddrSetSizeNoChecks(aaSet) == 1);
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_SINGLE:
            break;

        case ABS_ADDR_SET_SET: {
            abstract_addr_t *single = absAddrSetFirstNoChecks(aaSet);
            freeAbsAddrInternalSet(absAddrSetGetSet(aaSet));
            absAddrSetSetSingle(aaSet, single);
            break;
        }

        default:
            abort();
    }
    assert(checkAbsAddrSetSanity(aaSet));
}


/**
 * Convert an abstract address set to a set that can contain no items.
 */
static inline void
convertAbsAddrSetToEmpty(abs_addr_set_t *aaSet) {
    //fprintf(stderr, "Converting set %p to empty\n", *aaSet);
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            break;

        case ABS_ADDR_SET_SINGLE:
            absAddrSetSetEmpty(aaSet);
            break;

        case ABS_ADDR_SET_SET: {
            freeAbsAddrInternalSet(absAddrSetGetSet(aaSet));
            absAddrSetSetEmpty(aaSet);
            break;
        }

        default:
            abort();
    }
    assert(checkAbsAddrSetSanity(aaSet));
}


/**
 * Share an abstract address between two or more owners.
 */
abs_addr_set_t *
shareAbstractAddressSet(abs_addr_set_t *aaSet) {
    SmallHashTableItem *shared = smallHashTableLookupItem(sharedAbsAddrSets, aaSet);
    if (shared) {
        shared->element = toPtr(fromPtr(JITUINT32, shared->element) + 1);
        /* PDEBUG("Sharing set %p with %d sharers\n", aaSet, fromPtr(JITUINT32, shared->element)); */
    } else {
        smallHashTableInsert(sharedAbsAddrSets, aaSet, toPtr(1));
        /* PDEBUG("Sharing set %p with 1 sharer\n", aaSet); */
    }
    return aaSet;
}


/**
 * Clone an inner abstract address set.
 */
static abs_addr_set_t *
cloneInnerAbsAddrSet(abs_addr_set_t *innerSet) {
    abs_addr_set_t *cloneInner = allocAbsAddrSet();
    switch (absAddrSetGetType(innerSet)) {
        case ABS_ADDR_SET_SINGLE:
            absAddrSetSetSingle(cloneInner, absAddrSetGetSingle(innerSet));
            break;
        case ABS_ADDR_SET_VSET: {
            VSet *vset = absAddrSetGetVSet(innerSet);
            assert(vSetSize(vset) > 0);
            absAddrSetSetVSet(cloneInner, vllpaCloneSet(vset));
            break;
        }
        default:
            abort();
    }
    return cloneInner;
}


/**
 * Clone a set of abstract addresses.  Basically create a new set with
 * the same abstract addresses in the new as the old.
 */
abs_addr_set_t *
absAddrSetClone(abs_addr_set_t *aaSet) {
    abs_addr_set_t *cloneSet = NULL;
    assert(checkAbsAddrSetSanity(aaSet));
    switch(absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            cloneSet = newAbstractAddressSetEmpty();
            break;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            assert(checkAbsAddrSanity(single));
            cloneSet = newAbstractAddressSetSingle(single);
            break;
        }

        case ABS_ADDR_SET_SET: {
            VSet *cset;
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFirst(set);
            cloneSet = newAbstractAddressSetSet(30);
            cset = absAddrSetGetSet(cloneSet);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                abs_addr_set_t *cloneInner = cloneInnerAbsAddrSet(innerSet);
                assert(cloneInner);
                vSetInsertWithKey(cset, vSetItemKey(item), cloneInner);
                item = vSetNext(set, item);
            }
            break;
        }

        default:
            abort();
    }
    assert(checkAbsAddrSetSanity(cloneSet));
    assert(abstractAddressSetsAreIdentical(cloneSet, aaSet));
    return cloneSet;
}


/**
 * Clone an inner abstract address set without a given abstract address.
 */
static abs_addr_set_t *
cloneInnerAbsAddrSetWithoutAbsAddr(abs_addr_set_t *innerSet, abstract_addr_t *aaRemove) {
    abs_addr_set_t *cloneInner = NULL;
    switch (absAddrSetGetType(innerSet)) {
        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(innerSet);
            if (single != aaRemove) {
                cloneInner = allocAbsAddrSet();
                absAddrSetSetSingle(cloneInner, single);
            }
            break;
        }
        case ABS_ADDR_SET_VSET: {
            VSet *vset = absAddrSetGetVSet(innerSet);
            if (vSetSize(vset) > 0) {
                JITUINT32 cloneSize;
                VSet *cloneVSet = vllpaCloneSet(vset);
                vSetRemove(cloneVSet, aaRemove);
                cloneSize = vSetSize(cloneVSet);
                if (cloneSize > 0) {
                    cloneInner = allocAbsAddrSet();
                    if (cloneSize == 1) {
                        absAddrSetSetSingle(cloneInner, getSetItemData(vSetFirst(cloneVSet)));
                        freeSet(cloneVSet);
                    } else {
                        absAddrSetSetVSet(cloneInner, cloneVSet);
                    }
                } else {
                    freeSet(cloneVSet);
                }
            }
            break;
        }
        default:
            abort();
    }
    return cloneInner;
}


/**
 * Clone a set of abstract addresses without a given abstract address.
 */
abs_addr_set_t *
absAddrSetCloneWithoutAbsAddr(abs_addr_set_t *aaSet, abstract_addr_t *aaRemove) {
    abs_addr_set_t *cloneSet = NULL;
    assert(checkAbsAddrSetSanity(aaSet));
    switch(absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            cloneSet = newAbstractAddressSetEmpty();
            break;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            assert(checkAbsAddrSanity(single));
            if (single == aaRemove) {
                cloneSet = newAbstractAddressSetEmpty();
            } else {
                cloneSet = newAbstractAddressSetSingle(single);
            }
            break;
        }

        case ABS_ADDR_SET_SET: {
            VSet *cset;
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFirst(set);
            JITUINT32 size = 0;
            cloneSet = newAbstractAddressSetSet(31);
            cset = absAddrSetGetSet(cloneSet);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                abs_addr_set_t *cloneInner = cloneInnerAbsAddrSetWithoutAbsAddr(innerSet, aaRemove);
                if (cloneInner) {
                    vSetInsertWithKey(cset, vSetItemKey(item), cloneInner);
                    size += absAddrInnerSetSize(cloneInner);
                }
                item = vSetNext(set, item);
            }
            if (size == 1) {
                convertAbsAddrSetToSingle(cloneSet);
            }
            break;
        }

        default:
            abort();
    }
    assert(checkAbsAddrSetSanity(cloneSet));
    return cloneSet;
}


/**
 * Determine whether an abstract address exists within a set but don't do any
 * sanity checks.  This is for internal use only.
 */
static JITBOOLEAN
absAddrSetContainsNoChecks(abs_addr_set_t *aaSet, abstract_addr_t *aa) {
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            return JITFALSE;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            return single == aa;
        }

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            abs_addr_set_t *innerSet = vSetFindWithKey(set, getBaseUiv(aa->uiv));
            if (innerSet) {
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE:
                        return aa == absAddrSetGetSingle(innerSet);
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        return vSetContains(vset, aa);
                    }
                    default:
                        abort();
                }
            }
            break;
        }

        default:
            abort();
    }
    return JITFALSE;
}


/**
 * Determine whether an abstract address exists within a set.
 */
JITBOOLEAN
absAddrSetContains(abs_addr_set_t *aaSet, abstract_addr_t *aa) {
    assert(checkAbsAddrSetSanity(aaSet));
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            return JITFALSE;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            assert(checkAbsAddrSanity(single));
            return single == aa;
        }

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            abs_addr_set_t *innerSet = vSetFindWithKey(set, getBaseUiv(aa->uiv));
            if (innerSet) {
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE: {
                        abstract_addr_t *single = absAddrSetGetSingle(innerSet);
                        assert(checkAbsAddrSanity(single));
                        return single == aa;
                    }
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        return vSetContains(vset, aa);
                    }
                    default:
                        abort();
                }
            } else {
                return JITFALSE;
            }
        }

        default:
            abort();
    }
    return JITFALSE;
}


/**
 * Determine whether two sets of abstract addresses are identical.  This means
 * that all elements in one set are in the other too, and vice-versa.
 */
JITBOOLEAN
abstractAddressSetsAreIdentical(abs_addr_set_t *first, abs_addr_set_t *second) {
    assert(checkAbsAddrSetSanity(first));
    assert(checkAbsAddrSetSanity(second));
    if (first == second) {
        return JITTRUE;
    } else if (absAddrSetIsEmpty(first)) {
        if (absAddrSetIsEmpty(second)) {
            return JITTRUE;
        }
        return JITFALSE;
    } else if (absAddrSetGetType(first) != absAddrSetGetType(second)) {
        return JITFALSE;
    } else if (absAddrSetSize(first) != absAddrSetSize(second)) {
        return JITFALSE;
    } else {
        switch (absAddrSetGetType(first)) {
            case ABS_ADDR_SET_SINGLE:
                return absAddrSetGetSingle(first) == absAddrSetGetSingle(second);

            case ABS_ADDR_SET_SET: {
                VSet *fset = absAddrSetGetSet(first);
                VSetItem *item = vSetFirst(fset);
                while (item) {
                    abs_addr_set_t *innerSet = getSetItemData(item);
                    switch (absAddrSetGetType(innerSet)) {
                        case ABS_ADDR_SET_SINGLE:
                            if (!absAddrSetContainsNoChecks(second, absAddrSetGetSingle(innerSet))) {
                                return JITFALSE;
                            }
                            break;
                        case ABS_ADDR_SET_VSET: {
                            VSet *vset = absAddrSetGetVSet(innerSet);
                            VSetItem *aaItem = vSetFirst(vset);
                            while (aaItem) {
                                if (!absAddrSetContainsNoChecks(second, getSetItemData(aaItem))) {
                                    return JITFALSE;
                                }
                                aaItem = vSetNext(vset, aaItem);
                            }
                            break;
                        }
                        default:
                            abort();
                    }
                    item = vSetNext(fset, item);
                }
                return JITTRUE;
            }

            default:
                abort();
        }
    }
    return JITFALSE;
}


/**
 * Empty an abstract address set.
 */
void
absAddrSetEmpty(abs_addr_set_t *aaSet) {
    assert(checkAbsAddrSetSanity(aaSet));
    convertAbsAddrSetToEmpty(aaSet);
    assert(checkAbsAddrSetSanity(aaSet));
}


/**
 * Get a set of merges, creating if not already there.
 */
static VSet *
getAndCreateMergeSet(uiv_t *uiv, SmallHashTable *mergeMap) {
    VSet *mergeSet = smallHashTableLookup(mergeMap, uiv);
    if (!mergeSet) {
        mergeSet = allocateNewSet(20);
        smallHashTableInsert(mergeMap, uiv, mergeSet);
    }
    return mergeSet;
}


/**
 * Add a generic merge to target a specific abstract address with a given
 * stride.
 */
void
addGenericAbsAddrOffsetMerge(abstract_addr_t *aaFinal, JITUINT32 stride, SmallHashTable *mergeMap) {
    VSet *mergeSetLocal;
    VSet *mergeSetGlobal;
    VSetItem *mergeItem;
    aa_merge_t *merge;

    /* Find the local merge and global sets. */
    mergeSetLocal = getAndCreateMergeSet(aaFinal->uiv, mergeMap);
    mergeSetGlobal = getAndCreateMergeSet(aaFinal->uiv, allMerges);

    /* Check for an existing merge locally, but only if no uiv merge. */
    mergeItem = vSetFirst(mergeSetLocal);
    while (mergeItem) {
        merge = getSetItemData(mergeItem);
        if (merge->type == AA_MERGE_UIVS) {
            return;
        }
        if (merge->to == aaFinal && merge->stride == stride) {
            return;
        }
        mergeItem = vSetNext(mergeSetLocal, mergeItem);
    }

    /* Check for an existing merge globally. */
    mergeItem = vSetFirst(mergeSetGlobal);
    while (mergeItem) {
        merge = getSetItemData(mergeItem);
        if (merge->type == AA_MERGE_OFFSETS && merge->to == aaFinal && merge->stride == stride) {
            vSetInsert(mergeSetLocal, merge);
            return;
        }
        mergeItem = vSetNext(mergeSetGlobal, mergeItem);
    }

    /* Add the new merge. */
    merge = vllpaAllocFunction(sizeof(aa_merge_t), 25);
    merge->type = AA_MERGE_OFFSETS;
    merge->to = aaFinal;
    merge->stride = stride;
    vSetInsert(mergeSetLocal, merge);
    vSetInsert(mergeSetGlobal, merge);
    PDEBUG("Created merge to ");
    printAbstractAddress(aaFinal, JITFALSE);
    PDEBUGB(" stride %u\n", stride);
}


/**
 * Add a generic merge to target a specific abstract address from one uiv to
 * another.
 */
void
addGenericAbsAddrUivMerge(uiv_t *from, uiv_t *to, SmallHashTable *mergeMap) {
    VSet *mergeSetLocal;
    VSet *mergeSetGlobal;
    VSetItem *mergeItem;
    aa_merge_t *merge;

    /* Find the local and global sets. */
    mergeSetLocal = getAndCreateMergeSet(from, mergeMap);
    mergeSetGlobal = getAndCreateMergeSet(from, allMerges);

    /* Check for an existing merge locally. */
    mergeItem = vSetFirst(mergeSetLocal);
    if (mergeItem) {
        merge = getSetItemData(mergeItem);
        if (merge->type == AA_MERGE_OFFSETS) {
            vSetEmpty(mergeSetLocal);
        } else if (compareUivs(merge->uiv, to) == 1) {
            merge->uiv = to;
        }
        return;
    }

    /* Check for an existing merge globally. */
    mergeItem = vSetFirst(mergeSetGlobal);
    while (mergeItem) {
        merge = getSetItemData(mergeItem);
        if (merge->type == AA_MERGE_UIVS && merge->uiv == to) {
            vSetInsert(mergeSetLocal, merge);
            return;
        }
        mergeItem = vSetNext(mergeSetGlobal, mergeItem);
    }

    /* Add the new merge. */
    merge = vllpaAllocFunction(sizeof(aa_merge_t), 25);
    merge->type = AA_MERGE_UIVS;
    merge->uiv = to;
    vSetInsert(mergeSetLocal, merge);
    vSetInsert(mergeSetGlobal, merge);
    PDEBUG("Created merge from ");
    printUiv(from, "", JITFALSE);
    printUiv(to, " to ", JITTRUE);
}


/**
 * Add a generic mapping between an abstract address that should be merged
 * away and the abstract address it was merged to within the given merge
 * map.  For abstract addresses with the same uivs, the stride between
 * their offsets is calculated and a new abstract address is created that
 * has the lowest positive stride as its offset.  This is the final
 * abstract and is returned to the caller.  Otherwise, no merge is created
 * and NULL is returned.
 */
abstract_addr_t *
addGenericAbsAddrMergeMapping(abstract_addr_t *aaFrom, abstract_addr_t *aaTo, SmallHashTable *mergeMap) {
    abstract_addr_t *aaFinal = NULL;

    /* Different types of merge depending on what matches. */
    if (aaFrom->uiv == aaTo->uiv) {
        JITNUINT stride;

        /* Need to check the stride calculation. */
        assert(aaFrom->offset != aaTo->offset);
        if (aaTo->offset == WHOLE_ARRAY_OFFSET) {
            stride = 1;
            aaFinal = aaTo;
        } else {
            stride = labs(aaTo->offset - aaFrom->offset);
            assert(stride > 0);
            if (aaTo->offset >= 0) {
                aaFinal = newAbstractAddress(aaTo->uiv, aaTo->offset % stride);
            } else {
                JITNINT mult = labs((JITNINT)floor(1.0 * aaTo->offset / stride));
                /*       fprintf(stderr, "From offset %ld to offset %ld, stride %u, mult %ld\n", aaFrom->offset, aaTo->offset, stride, mult); */
                /*       fprintf(stderr, "div val %lf, floor val %lf\n", 1.0 * aaTo->offset / stride, floor(1.0 * aaTo->offset / stride)); */
                assert(mult > 0);
                aaFinal = newAbstractAddress(aaTo->uiv, aaTo->offset + mult * stride);
                assert(aaFinal->offset >= 0);
            }
        }
        assert(aaFinal->offset >= 0);
        if (mergeMap) {
            addGenericAbsAddrOffsetMerge(aaFinal, stride, mergeMap);
        }
    }

    /* Merge based on the uivs. */
    else {
        aaFinal = aaTo;
        if (mergeMap) {
            addGenericAbsAddrUivMerge(aaFrom->uiv, aaTo->uiv, mergeMap);
        }
    }

    /* Return the target abstract address. */
    return aaFinal;
}


/**
 * Get the abstract address to merge to from the given abstract address.
 * More specifically, check each merge for the uiv from the given abstract
 * address.  If the offset for the current merge candidate can be reduced
 * to the offset of the merge point, then update the current merge
 * candidate to the new merge.
 */
static abstract_addr_t *
getGenericMergeAbstractAddress(abstract_addr_t *aaFrom, SmallHashTable *mergeMap) {
    abstract_addr_t *aaTo = aaFrom;
    VSet *mergeSet = smallHashTableLookup(mergeMap, aaFrom->uiv);
    if (mergeSet) {
        VSetItem *mergeItem = vSetFirst(mergeSet);
        while (mergeItem) {
            aa_merge_t *merge = getSetItemData(mergeItem);
            if (merge->type == AA_MERGE_UIVS) {
                assert(vSetSize(mergeSet) == 1);
                if (aaFrom->uiv == merge->uiv) {
                    return aaFrom;
                } else {
                    return newAbstractAddress(merge->uiv, aaFrom->offset);
                }
            } else {
                JITNINT offset;

                /* Get the offset to consider. */
                if (aaFrom->offset >= 0) {
                    offset = aaFrom->offset % merge->stride;
                } else {
                    JITNINT mult = labs((JITNINT)floor(1.0 * aaFrom->offset / merge->stride));
                    assert(mult > 0);
                    offset = aaFrom->offset + mult * merge->stride;
                }

                /* Check against this merge. */
                if (offset == merge->to->offset) {

                    /* If we've got a choice, prefer offsets closer to zero. */
                    if (aaTo == aaFrom || aaTo->offset > merge->to->offset) {
                        aaTo = merge->to;
                    }
                }
            }
            mergeItem = vSetNext(mergeSet, mergeItem);
        }
    }
    return aaTo;
}


/**
 * Apply the given generic merge map to an abstract address.
 */
static abstract_addr_t *
applyGenericMergeMapToAbsAddr(abstract_addr_t *aa, SmallHashTable *mergeMap) {
    abstract_addr_t *aaPrev = aa;
    abstract_addr_t *aaMerge = aa;
    while ((aaMerge = getGenericMergeAbstractAddress(aaMerge, mergeMap)) != aaPrev) {
        aaPrev = aaMerge;
    }
    return aaMerge;
}


/**
 * A type of merge.
 */
typedef enum {MERGE_NEITHER, MERGE_FIRST, MERGE_SECOND} merge_type_t;


/**
 * Choose between two offsets to merge.  We always choose to keep positive
 * offsets first, then those closer to zero.  In the event of a tie, the
 * first offset will be chosen.  Returns JITTRUE if the first offset is
 * preferred for merging (i.e. the second is positive or closer to zero).
 */
static inline merge_type_t
getMergeTypeFromOffset(JITNINT firstOffset, JITNINT secondOffset) {

    /* Merge the other if one is a whole array offset. */
    if (firstOffset == WHOLE_ARRAY_OFFSET) {
        return MERGE_SECOND;
    } else if (secondOffset == WHOLE_ARRAY_OFFSET) {
        return MERGE_FIRST;
    }
  
    /* Neither is a whole array access. */
    else if (firstOffset >= 0) {
        if (secondOffset < 0) {
            return MERGE_SECOND;
        }

        /* First and second positive. */
        else if (firstOffset < secondOffset) {
            return MERGE_SECOND;
        } else {
            return MERGE_FIRST;
        }
    }

    /* First is negative. */
    else if (secondOffset >= 0) {
        return MERGE_FIRST;
    }

    /* Both negative. */
    else if (firstOffset > secondOffset) {
        return MERGE_SECOND;
    } else {
        return MERGE_FIRST;
    }
}


/**
 * Check whether two abstract addresses are merge candidates.  If they
 * are then return the abstract address that should be merged (removed).
 * Merging prevents too many uivs and abstract addresses from being
 * created.  We therefore merge abstract addresses where the offsets are
 * the same and the base uivs are also the same.  We also merge if the
 * uivs from the abstract addresses are the same but the offsets differ.
 * In this case, we always choose to keep the abstract address with an
 * positive offset close to zero.  Positive offsets are prefered, then
 * distance from zero.
 */
static abstract_addr_t *
getAbstractAddressToMerge(abstract_addr_t *aaFirst, abstract_addr_t *aaSecond) {
    /* Easy check for identical abstract addresses. */
    if (aaFirst == aaSecond) {
        return aaSecond;
    }

    /* Merge if uivs are the same but offsets differ. */
    else if (aaFirst->uiv == aaSecond->uiv) {
        merge_type_t mergeType = getMergeTypeFromOffset(aaFirst->offset, aaSecond->offset);
        if (mergeType == MERGE_FIRST) {
            return aaFirst;
        } else if (mergeType == MERGE_SECOND) {
            return aaSecond;
        }
    }

    /* Merge if base uivs and offsets are the same. */
    else if (aaFirst->offset == aaSecond->offset  && getBaseUiv(aaFirst->uiv) == getBaseUiv(aaSecond->uiv)) {
    /* /\* Merge if one is a prefix of the other and offsets are the same. *\/ */
    /* else if (aaFirst->offset == aaSecond->offset && (isUivAPrefix(aaFirst->uiv, aaSecond->uiv) || isUivAPrefix(aaSecond->uiv, aaFirst->uiv))) { */
        JITUINT32 firstLevel = getLevelOfBaseUiv(aaFirst->uiv);
        JITUINT32 secondLevel = getLevelOfBaseUiv(aaSecond->uiv);
        /* PDEBUG("Merging "); */
        /* printAbstractAddress(aaFirst, JITFALSE); */
        /* PDEBUGB(" with "); */
        /* printAbstractAddress(aaSecond, JITTRUE); */

        /**
         * If we keep this as one uiv having to be a prefix of the other to
         * perform the merge, then we can avoid most of this below and just
         * take the uiv that is a prefix.
         */

        /* Take the shallowest uiv. */
        if (firstLevel < secondLevel) {
            return aaSecond;
        } else if (firstLevel > secondLevel) {
            return aaFirst;
        }

        /* Levels match, choose deepest, smallest offset in the stack. */
        else {
            JITINT32 l;
            for (l = firstLevel - 1; l >= 0; --l) {
                JITNINT firstOffset = getFieldOffsetAtLevelFromUiv(aaFirst->uiv, l);
                JITNINT secondOffset = getFieldOffsetAtLevelFromUiv(aaSecond->uiv, l);
                if (firstOffset != secondOffset) {
                    merge_type_t mergeType = getMergeTypeFromOffset(firstOffset, secondOffset);
                    if (mergeType == MERGE_FIRST) {
                        return aaFirst;
                    } else if (mergeType == MERGE_SECOND) {
                        return aaSecond;
                    }
                }
            }

            /* Should not get here. */
            SET_DEBUG_ENABLE(JITTRUE);
            PDEBUG("Could not merge ");
            printAbstractAddress(aaFirst, JITFALSE);
            PDEBUGB(" and ");
            printAbstractAddress(aaSecond, JITTRUE);
            abort();
        }
    }

    /* Abstract addresses don't merge. */
    return NULL;
}


/**
 * Record a uiv as being a merge target.
 */
void
addUivMergeTarget(SmallHashTable *targets, uiv_t *uiv, aa_merge_type_t type) {
    SmallHashTableItem *mapItem = smallHashTableLookupItem(targets, uiv);
    if (mapItem) {
        mapItem->element = toPtr(fromPtr(JITNUINT, mapItem->element) | type);
    } else {
        smallHashTableInsert(targets, uiv, toPtr(type));
    }
}


/**
 * Get the final abstract address to keep from a merge.  For abstract addresses
 * with the same uivs, the stride between their offsets is calculated and a new
 * abstract address is created that has the lowest positive stride as its
 * offset.  This is the final abstract and is returned to the caller.  If no
 * merge can take place then NULL is returned.
 */
static abstract_addr_t *
getFinalAbsAddrFromMerge(abstract_addr_t *aaFirst, abstract_addr_t *aaSecond, SmallHashTable *uivMergeTargets) {
    abstract_addr_t *aaFinal = NULL;
    abstract_addr_t *aaFrom;

    /* Try merging. */
    aaFrom = getAbstractAddressToMerge(aaFirst, aaSecond);
    if (aaFrom) {
        abstract_addr_t *aaTo = aaFrom == aaFirst ? aaSecond : aaFirst;

        /* Different types of merge depending on what matches. */
        if (aaFrom->uiv == aaTo->uiv) {
            JITNUINT stride;
            assert(aaFrom->offset != aaTo->offset);
            if (aaTo->offset == WHOLE_ARRAY_OFFSET) {
                stride = 1;
                aaFinal = aaTo;
            } else {
                stride = labs(aaTo->offset - aaFrom->offset);
                assert(stride > 0);
                if (aaTo->offset >= 0) {
                    aaFinal = newAbstractAddress(aaTo->uiv, aaTo->offset % stride);
                } else {
                    JITNINT mult = labs((JITNINT)floor(1.0 * aaTo->offset / stride));
                    /*       fprintf(stderr, "From offset %ld to offset %ld, stride %u, mult %ld\n", aaFrom->offset, aaTo->offset, stride, mult); */
                    /*       fprintf(stderr, "div val %lf, floor val %lf\n", 1.0 * aaTo->offset / stride, floor(1.0 * aaTo->offset / stride)); */
                    assert(mult > 0);
                    aaFinal = newAbstractAddress(aaTo->uiv, aaTo->offset + mult * stride);
                    assert(aaFinal->offset >= 0);
                }
            }
            assert(aaFinal->offset >= 0);

            /* Record a uiv merge target. */
            if (uivMergeTargets) {
                addUivMergeTarget(uivMergeTargets, aaFinal->uiv, AA_MERGE_OFFSETS);
            }
        }

        /* Merge based on the uivs. */
        else {
            aaFinal = aaTo;
            if (uivMergeTargets) {
                addUivMergeTarget(uivMergeTargets, aaFinal->uiv, AA_MERGE_UIVS);
            }
        }
    }

    /* Return the target abstract address. */
    return aaFinal;
}


/**
 * Return a number less than zero if the first abstract address should come
 * before the second in a sorted set, or greater than zero if it should come
 * afterwards.
 */
static int
compareAbsAddrs(const void *first, const void *second) {
    abstract_addr_t *f = *(abstract_addr_t * const *)first;
    abstract_addr_t *s = *(abstract_addr_t * const *)second;

    /* If uivs are the same, go with the offset. */
    if (f->uiv == s->uiv) {
      return f->offset < s->offset ? -1 : 1;
    }

    /* Go with the uiv ordering. */
    return compareUivs(f->uiv, s->uiv);
}


/**
 * Merge abstract addresses within a set.  We compare all abstract addresses
 * with all others in the set, even once they have been merged away, so that we
 * ensure that all possible merges can take place.  This never adds abstract
 * addresses to the set, only removes them.
 */
static void
absAddrSetMergeInActualSet(VSet *set, SmallHashTable *uivMergeTargets) {
    VSet *toRemove;
    VSet *toInsert;
    VSetItem *aaFirstItem;
    JITBOOLEAN insertions;

    /* Debugging. */
    /* PDEBUG("Merging in actual set: "); */
    /* printAbstractAddressActualSet(set); */

    /* Sets of abstract addresses to remove and insert. */
    toRemove = allocateNewSet(21);
    toInsert = allocateNewSet(22);

    /* Unfortunately it seems we must keep going whilst the set is added to. */
    insertions = JITTRUE;
    while (insertions) {
        insertions = JITFALSE;

        /* Find addresses to remove. */
        aaFirstItem = vSetFirst(set);
        while (aaFirstItem) {
            abstract_addr_t *aaFirst = getSetItemData(aaFirstItem);
            VSetItem *aaNextItem = vSetNext(set, aaFirstItem);
            VSetItem *aaSecondItem = aaNextItem;
            while (aaSecondItem) {
                abstract_addr_t *aaSecond = getSetItemData(aaSecondItem);
                abstract_addr_t *aaFinal = getFinalAbsAddrFromMerge(aaFirst, aaSecond, uivMergeTargets);
                if (aaFinal) {
                    if (aaFinal != aaFirst) {
                        vSetInsert(toRemove, aaFirst);
                        if (aaFinal != aaSecond) {
                            vSetInsert(toRemove, aaSecond);
                            vSetInsert(toInsert, aaFinal);
                        }
                    } else if (aaFinal != aaSecond) {
                        vSetInsert(toRemove, aaSecond);
                    }
                    /* PDEBUG("Merge between "); */
                    /* printAbstractAddress(aaFirst, JITFALSE); */
                    /* PDEBUGB(" and "); */
                    /* printAbstractAddress(aaSecond, JITFALSE); */
                    /* PDEBUGB(" merge away "); */
                    /* printAbstractAddress(aaMerge, JITTRUE); */
                }
                aaSecondItem = vSetNext(set, aaSecondItem);
            }
            aaFirstItem = aaNextItem;
        }

        /* Insert new abstract addresses and repeat. */
        while (vSetSize(toInsert) > 0) {
            abstract_addr_t *aaInsert = getSetItemData(vSetFirst(toInsert));
            if (!vSetContains(set, aaInsert)) {
                vSetInsert(set, aaInsert);
                insertions = JITTRUE;
            }
            vSetRemove(toInsert, aaInsert);
        }
    }

    /* Remove the merged-away addresses. */
    aaFirstItem = vSetFirst(toRemove);
    while (aaFirstItem) {
        vSetRemove(set, getSetItemData(aaFirstItem));
        aaFirstItem = vSetNext(toRemove, aaFirstItem);
    }

    /* A check, make an assertion later. */
    if (vSetSize(set) == 0) {
        fprintf(stderr, "Merged whole set away\n");
        abort();
    }

    /* Debugging. */
    /* PDEBUG("Merged actual set:     "); */
    /* printAbstractAddressActualSet(set); */

    /* Clean up. */
    freeSet(toRemove);
    freeSet(toInsert);
    assert(checkAbsAddrSetSanityActualSet(set));
}


/**
 * Merge abstract addresses with the same base unknown initial value and
 * offsets, and those that have the same unknown initial value but
 * different offsets.
 */
void
absAddrSetMerge(abs_addr_set_t *aaSet, SmallHashTable *uivMergeTargets) {
    assert(checkAbsAddrSetSanity(aaSet));
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            break;

        case ABS_ADDR_SET_SINGLE:
            break;

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFirst(set);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE:
                        break;
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        JITUINT32 afterSize, beforeSize = vSetSize(vset);
                        absAddrSetMergeInActualSet(vset, uivMergeTargets);
                        afterSize = vSetSize(vset);
                        if (afterSize != beforeSize) {
                            assert(afterSize > 0);
                            assert(afterSize < beforeSize);
                            if (afterSize == 1) {
                                VSetItem *aaItem = vSetFirst(vset);
                                abstract_addr_t *single = getSetItemData(aaItem);
                                freeSet(vset);
                                absAddrSetSetSingle(innerSet, single);
                            }
                        }
                        break;
                    }
                    default:
                        abort();
                }
                item = vSetNext(set, item);
            }
            /* PDEBUG("  Merged to:  "); */
            /* printAbstractAddressSet(aaSet); */
            break;
        }

        default:
            abort();
    }
    assert(checkAbsAddrSetSanity(aaSet));
}


/**
 * Create an abstract address set from a merge between two abstract addresses.
 */
static abs_addr_set_t *
absAddrSetMergeBetweenSingles(abstract_addr_t *aaFirst, abstract_addr_t *aaSecond, SmallHashTable *uivMergeTargets) {
    abstract_addr_t *aaFinal;
    if (aaFirst == aaSecond || !(aaFinal = getFinalAbsAddrFromMerge(aaFirst, aaSecond, uivMergeTargets))) {
        return newAbstractAddressSetSingle(aaFirst);
    }
    return newAbstractAddressSetSingle(aaFinal);
}


/**
 * Check for merge points between two sets of abstract addresses.  Return a set
 * of abstract addresses that consists of the first set without any merges
 * that were found.
 */
static VSet *
absAddrSetMergeBetweenActualSets(VSet *firstSet, VSet *secondSet, SmallHashTable *uivMergeTargets) {
    VSet *mergedSet;
    VSetItem *aaFirstItem;

    /* The set to return. */
    mergedSet = allocateNewSet(23);

    /* Check for merge points between the two sets. */
    aaFirstItem = vSetFirst(firstSet);
    while (aaFirstItem) {
        abstract_addr_t *aaFirst = getSetItemData(aaFirstItem);
        VSetItem *aaSecondItem = vSetFirst(secondSet);
        JITBOOLEAN merged = JITFALSE;

        /* Try to merge away the first abstract address. */
        while (aaSecondItem && !merged) {
            abstract_addr_t *aaSecond = getSetItemData(aaSecondItem);
            if (aaFirst != aaSecond) {
                abstract_addr_t *aaFinal = getFinalAbsAddrFromMerge(aaFirst, aaSecond, uivMergeTargets);
                if (aaFinal && aaFinal != aaFirst) {
                    vSetInsert(mergedSet, aaFinal);
                    merged = JITTRUE;
                }
            }
            aaSecondItem = vSetNext(secondSet, aaSecondItem);
        }

        /* Add the first abstract address if it wasn't merged away. */
        if (!merged) {
            vSetInsert(mergedSet, aaFirst);
        }
        aaFirstItem = vSetNext(firstSet, aaFirstItem);
    }

    /* Return the merged set. */
    assert(checkAbsAddrSetSanityActualSet(mergedSet));
    return mergedSet;
}


/**
 * Check for merge points between two sets of abstract addresses.  Return
 * a set of abstract addresses that consists of the first set with any
 * merges that were found.
 */
abs_addr_set_t *
absAddrSetMergeBetweenSets(abs_addr_set_t *first, abs_addr_set_t *second, SmallHashTable *uivMergeTargets) {
    abs_addr_set_t *merged;

    /* Shouldn't be called with the same sets. */
    assert(checkAbsAddrSetSanity(first));
    assert(checkAbsAddrSetSanity(second));
    assert(absAddrSetIsEmpty(first) || first != second);
    /* PDEBUG("  Merging between: "); */
    /* printAbstractAddressSet(first); */
    /* PDEBUG("  and:             "); */
    /* printAbstractAddressSet(second); */

    /* One empty set is easy. */
    if (absAddrSetIsEmpty(first) || absAddrSetIsEmpty(second)) {
        merged = absAddrSetClone(first);
    }

    /* Sets have the same time, handle here. */
    else if (absAddrSetGetType(first) == absAddrSetGetType(second)) {
        switch (absAddrSetGetType(first)) {
            case ABS_ADDR_SET_SINGLE: {
                merged = absAddrSetMergeBetweenSingles(absAddrSetGetSingle(first), absAddrSetGetSingle(second), uivMergeTargets);
                break;
            }

            case ABS_ADDR_SET_SET: {
                VSet *mset;
                VSet *fset = absAddrSetGetSet(first);
                VSet *sset = absAddrSetGetSet(second);
                VSetItem *item = vSetFirst(fset);
                merged = newAbstractAddressSetSet(32);
                mset = absAddrSetGetSet(merged);

                /* Check each internal set. */
                while (item) {
                    abs_addr_set_t *minnerSet;
                    abs_addr_set_t *finnerSet = getSetItemData(item);
                    abs_addr_set_t *sinnerSet = vSetFindWithKey(sset, vSetItemKey(item));
                    VSetItem *next = vSetNext(fset, item);

                    /* No corresponding inner set in the second set for this base uiv. */
                    if (!sinnerSet) {
                        minnerSet = cloneInnerAbsAddrSet(finnerSet);
                        assert(minnerSet);
                    }

                    /* Inner sets both have the same type. */
                    else if (absAddrSetGetType(finnerSet) == absAddrSetGetType(sinnerSet)) {
                        switch (absAddrSetGetType(finnerSet)) {
                            case ABS_ADDR_SET_SINGLE:
                                minnerSet = absAddrSetMergeBetweenSingles(absAddrSetGetSingle(finnerSet), absAddrSetGetSingle(sinnerSet), uivMergeTargets);
                                break;
                            case ABS_ADDR_SET_VSET: {
                                VSet *fvset = absAddrSetGetVSet(finnerSet);
                                VSet *svset = absAddrSetGetVSet(sinnerSet);
                                VSet *mvset = absAddrSetMergeBetweenActualSets(fvset, svset, uivMergeTargets);
                                assert(vSetSize(mvset) > 0);
                                minnerSet = allocAbsAddrSet();
                                if (vSetSize(mvset) == 1) {
                                    absAddrSetSetSingle(minnerSet, getSetItemData(vSetFirst(mvset)));
                                    freeSet(mvset);
                                } else {
                                    absAddrSetSetVSet(minnerSet, mvset);
                                }
                                break;
                            }
                            default:
                                abort();
                        }
                    }

                    /* Differing types for inner sets, convert the single to a vset. */
                    else {
                        VSet *fvset, *svset, *mvset;
                        minnerSet = allocAbsAddrSet();
                        if (absAddrSetIsSingle(finnerSet)) {
                            assert(absAddrSetIsVSet(sinnerSet));
                            fvset = allocateNewSet(24);
                            vSetInsert(fvset, absAddrSetGetSingle(finnerSet));
                            svset = absAddrSetGetVSet(sinnerSet);
                        } else {
                            assert(absAddrSetIsVSet(finnerSet));
                            fvset = absAddrSetGetVSet(finnerSet);
                            svset = allocateNewSet(25);
                            vSetInsert(svset, absAddrSetGetSingle(sinnerSet));
                        }
                        mvset = absAddrSetMergeBetweenActualSets(fvset, svset, uivMergeTargets);
                        assert(vSetSize(mvset) > 0);
                        if (vSetSize(mvset) == 1) {
                            absAddrSetSetSingle(minnerSet, getSetItemData(vSetFirst(mvset)));
                            freeSet(mvset);
                        } else {
                            absAddrSetSetVSet(minnerSet, mvset);
                        }
                        if (absAddrSetIsSingle(finnerSet)) {
                            freeSet(fvset);
                        } else {
                            freeSet(svset);
                        }
                    }
                    if (minnerSet) {
                        vSetInsertWithKey(mset, vSetItemKey(item), minnerSet);
                    }
                    item = next;
                }
                break;
            }

            default:
                abort();
        }
    }

    /* Differing types for sets, convert single to set and recurse. */
    else {
        if (absAddrSetIsSingle(first)) {
            abs_addr_set_t *firstSet = absAddrSetClone(first);
            convertAbsAddrSetToSet(firstSet);
            assert(absAddrSetIsSet(second));
            merged = absAddrSetMergeBetweenSets(firstSet, second, uivMergeTargets);
            freeAbstractAddressSet(firstSet);
        } else {
            abs_addr_set_t *secondSet = absAddrSetClone(second);
            convertAbsAddrSetToSet(secondSet);
            assert(absAddrSetIsSet(first));
            merged = absAddrSetMergeBetweenSets(first, secondSet, uivMergeTargets);
            freeAbstractAddressSet(secondSet);
        }
    }

    /* Finished. */
    assert(checkAbsAddrSetSanity(merged));
    /* PDEBUG("  Merged to:       "); */
    /* printAbstractAddressSet(merged); */
    return merged;
}


/**
 * Check to see whether an abstract address would be merged into the given set
 * of abstract addresses.
 */
JITBOOLEAN
absAddrSetContainsAbsAddr(abs_addr_set_t *aaSet, abstract_addr_t *aaToMerge) {
    assert(checkAbsAddrSetSanity(aaSet));
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            return JITFALSE;
        case ABS_ADDR_SET_SINGLE:
            return getAbstractAddressToMerge(aaToMerge, absAddrSetGetSingle(aaSet)) != NULL;
        case ABS_ADDR_SET_SET: {
            uiv_t *base = getBaseUiv(aaToMerge->uiv);
            VSet *set = absAddrSetGetSet(aaSet);
            abs_addr_set_t *innerSet = vSetFindWithKey(set, base);
            if (innerSet) {
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE:
                        return getAbstractAddressToMerge(aaToMerge, absAddrSetGetSingle(innerSet)) != NULL;
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        VSetItem *aaItem = vSetFirst(vset);
                        while (aaItem) {
                            if (getAbstractAddressToMerge(aaToMerge, getSetItemData(aaItem))) {
                                return JITTRUE;
                            }
                            aaItem = vSetNext(vset, aaItem);
                        }
                        return JITFALSE;
                    }
                    default:
                        abort();
                }
            } else {
                return JITFALSE;
            }
        }
        default:
            abort();
    }
    return JITFALSE;
}


/**
 * Check to see whether the first set of abstract addresses contains all
 * of the abstract addresses in the second set, after merging.
 */
JITBOOLEAN
abstractAddressSetContainsSet(abs_addr_set_t *aaFirstSet, abs_addr_set_t *aaSecondSet) {
    assert(checkAbsAddrSetSanity(aaFirstSet));
    assert(checkAbsAddrSetSanity(aaSecondSet));
    switch (absAddrSetGetType(aaSecondSet)) {
        case ABS_ADDR_SET_EMPTY:
            return JITTRUE;

        case ABS_ADDR_SET_SINGLE:
            return absAddrSetContainsAbsAddr(aaFirstSet, absAddrSetGetSingle(aaSecondSet));

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSecondSet);
            VSetItem *item = vSetFirst(set);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE:
                        if (!absAddrSetContainsAbsAddr(aaFirstSet, absAddrSetGetSingle(innerSet))) {
                            return JITFALSE;
                        }
                        break;
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        VSetItem *aaItem = vSetFirst(vset);
                        while (aaItem) {
                            if (!absAddrSetContainsAbsAddr(aaFirstSet, getSetItemData(aaItem))) {
                                return JITFALSE;
                            }
                            aaItem = vSetNext(vset, aaItem);
                        }
                        break;
                    }
                    default:
                        abort();
                }
                item = vSetNext(set, item);
            }
            return JITTRUE;
        }

        default:
            abort();
    }

    /* All abstract addresses in the second set merged. */
    return JITTRUE;
}


/**
 * Append an abstract address to a single abstract address and possible merge.
 * Returns the number of elements in the set after appending and merging.
 */
static JITUINT32
appendSingleAbsAddrSet(abs_addr_set_t *innerSet, abstract_addr_t *aa) {
    abstract_addr_t *single;
    assert(absAddrSetIsSingle(innerSet));
    single = absAddrSetGetSingle(innerSet);
    if (single == aa) {
        return 1;
    }

    /* No match with existing address. */
    absAddrSetSetVSet(innerSet, allocateNewSet(26));
    vSetInsert(absAddrSetGetVSet(innerSet), single);
    vSetInsert(absAddrSetGetVSet(innerSet), aa);
    assert(vSetSize(absAddrSetGetVSet(innerSet)) == 2);
    return 2;
}


/**
 * Append a single abstract address to a set.  This also performs a merge
 * on the final set if the given parameter is JITTRUE.  Duplicates are
 * automatically removed from the final set too.  Returns JITTRUE if the
 * abstract address is appended and not merged.
 */
void
absAddrSetAppendAbsAddr(abs_addr_set_t *aaSet, abstract_addr_t *aa) {
    assert(checkAbsAddrSetSanity(aaSet));
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            absAddrSetSetSingle(aaSet, aa);
            break;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *single = absAddrSetGetSingle(aaSet);
            if (single != aa) {
                VSet *set;
                uiv_t *base = getBaseUiv(aa->uiv);
                convertAbsAddrSetToSet(aaSet);
                set = absAddrSetGetSet(aaSet);
                if (base != getBaseUiv(single->uiv)) {
                    vSetInsertWithKey(set, base, newAbstractAddressSet(aa));
                } else {
                    JITUINT32 size;
                    abs_addr_set_t *innerSet = vSetFindWithKey(set, base);
                    assert(absAddrSetIsSingle(innerSet));
                    assert(absAddrSetGetSingle(innerSet) == single);
                    size = appendSingleAbsAddrSet(innerSet, aa);
                    vSetReplaceWithKey(set, base, innerSet);
                    if (size == 1) {
                        convertAbsAddrSetToSingle(aaSet);
                    } else {
                        assert(size == 2);
                        assert(absAddrSetIsVSet(innerSet));
                        assert(vSetSize(absAddrSetGetVSet(innerSet)) == 2);
                        assert(vSetFindWithKey(set, base) == innerSet);
                    }
                }
            }
            break;
        }

        case ABS_ADDR_SET_SET: {
            uiv_t *base = getBaseUiv(aa->uiv);
            VSet *set = absAddrSetGetSet(aaSet);
            abs_addr_set_t *innerSet = vSetFindWithKey(set, base);
            if (innerSet) {
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE: {
                        appendSingleAbsAddrSet(innerSet, aa);
                        break;
                    }
                    case ABS_ADDR_SET_VSET: {
                        VSet *vset = absAddrSetGetVSet(innerSet);
                        vSetInsert(vset, aa);
                        break;
                    }
                    default:
                        abort();
                }
            } else {
                innerSet = newAbstractAddressSetSingle(aa);
                vSetInsertWithKey(set, base, innerSet);
            }
            break;
        }

        default:
            abort();
    }
    assert(checkAbsAddrSetSanity(aaSet));
}


/**
 * Append a vset of abstract addresses to another.  Returns the number of
 * abstract addresses appended to the set.
 */
static JITUINT32
absAddrVSetAppendVSet(VSet *vset, VSet *toAppend)
{
    JITUINT32 appended = 0;
    VSetItem *aaItem = vSetFirst(toAppend);
    while (aaItem) {
        if (vSetInsert(vset, getSetItemData(aaItem))) {
            appended += 1;
        }
        aaItem = vSetNext(toAppend, aaItem);
    }
    return appended;
}


/**
 * Append a vset of abstract addresses with the same base uiv to the given
 * set of abstract addresses.
 */
static void
absAddrSetAppendVSet(abs_addr_set_t *aaSet, uiv_t *base, VSet *vset)
{
    VSet *set;
    abs_addr_set_t *innerSet;
    assert(absAddrSetGetType(aaSet) == ABS_ADDR_SET_SET);
    set = absAddrSetGetSet(aaSet);
    innerSet = vSetFindWithKey(set, base);
    if (innerSet) {
        switch (absAddrSetGetType(innerSet)) {
            case ABS_ADDR_SET_SINGLE: {
                VSet *superSet = allocateNewSet(27);
                abstract_addr_t *single = absAddrSetGetSingle(innerSet);
                absAddrSetSetVSet(innerSet, superSet);
                vSetInsert(superSet, single);
                absAddrVSetAppendVSet(superSet, vset);
                break;
            }
            case ABS_ADDR_SET_VSET: {
                absAddrVSetAppendVSet(absAddrSetGetVSet(innerSet), vset);
                break;
            }
            default:
                abort();
        }
    } else {
        VSet *superSet = vllpaCloneSet(vset);
        innerSet = newAbstractAddressSetEmpty();
        vSetInsertWithKey(set, base, innerSet);
        absAddrSetSetVSet(innerSet, superSet);
    }
}


/**
 * Append a set of abstract addresses to another set.  This also performs
 * a merge on the final set if the given parameter is JITTRUE.  Duplicates
 * are automatically removed from the final set too.  Returns JITTRUE if
 * any additions or subtractions are made to the first set.  However,
 * items can be reordered.
 */
JITBOOLEAN
absAddrSetTrackAppendSet(abs_addr_set_t *aaSet, abs_addr_set_t *toAppend) {
    abstract_addr_t *aa;
    abs_addr_set_t *originalSet;
    JITBOOLEAN changes = JITFALSE;

    /* Appending empty set. */
    assert(checkAbsAddrSetSanity(aaSet));
    assert(checkAbsAddrSetSanity(toAppend));
    if (absAddrSetIsEmpty(toAppend)) {
        return JITFALSE;
    }

    /* Same set. */
    if (aaSet == toAppend) {
        return JITFALSE;
    }

    /* Take a copy to record changes. */
    originalSet = absAddrSetClone(aaSet);

    /* Append each abstract address from the second set. */
    aa = absAddrSetFirst(toAppend);
    while (aa) {
        absAddrSetAppendAbsAddr(aaSet, aa);
        aa = absAddrSetNext(toAppend, aa);
    }

    /* Decide on whether changes have occurred. */
    if (!abstractAddressSetsAreIdentical(originalSet, aaSet)) {
        PDEBUG("Sets differ:\n");
        PDEBUG("Was: ");
        printAbstractAddressSet(originalSet);
        PDEBUG("Now: ");
        printAbstractAddressSet(aaSet);
        PDEBUG("Appended: ");
        printAbstractAddressSet(toAppend);
        changes = JITTRUE;
    }

    /* Clean up. */
    freeAbstractAddressSet(originalSet);

    /* Return whether there were changes. */
    assert(checkAbsAddrSetSanity(aaSet));
    return changes;
}


/**
 * Append a set of abstract addresses to another set but don't track
 * whether changes have been made.  This also performs a merge on the final
 * set if the given parameter is JITTRUE.  Duplicates are automatically
 * removed from the final set too if required.
 */
void
absAddrSetAppendSet(abs_addr_set_t *aaSet, abs_addr_set_t *toAppend) {
   /* Appending empty set. */
    assert(checkAbsAddrSetSanity(aaSet));
    assert(checkAbsAddrSetSanity(toAppend));
    if (absAddrSetIsEmpty(toAppend)) {
        return;
    }

    /* Same set. */
    if (aaSet == toAppend) {
        return;
    }

    /* Append each abstract address from the second set. */
    switch (absAddrSetGetType(toAppend)) {
        case ABS_ADDR_SET_SINGLE:
            absAddrSetAppendAbsAddr(aaSet, absAddrSetGetSingle(toAppend));
            break;

        case ABS_ADDR_SET_SET: {
            VSet *set;
            VSetItem *item;
            convertAbsAddrSetToSet(aaSet);
            set = absAddrSetGetSet(toAppend);
            item = vSetFirst(set);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                switch (absAddrSetGetType(innerSet)) {
                    case ABS_ADDR_SET_SINGLE:
                        absAddrSetAppendAbsAddr(aaSet, absAddrSetGetSingle(innerSet));
                        break;
                    case ABS_ADDR_SET_VSET:
                        absAddrSetAppendVSet(aaSet, vSetItemKey(item), absAddrSetGetVSet(innerSet));
                        break;
                    default:
                        abort();
                }
                item = vSetNext(set, item);
            }
            break;
        }

        default:
            abort();
    }

    /* Might need converting. */
    assert(checkAbsAddrSetSanity(aaSet));
}


/**
 * Apply the given generic merge map to each abstract address in the given
 * internal set.
 */
static void
applyGenericMergeMapToAbsAddrInternalSet(VSet *set, SmallHashTable *mergeMap) {
    VSetItem *setItem = vSetFirst(set);
    while (setItem) {
        abs_addr_set_t *innerSet = getSetItemData(setItem);
        switch (absAddrSetGetType(innerSet)) {
            case ABS_ADDR_SET_SINGLE: {
                abstract_addr_t *aaMerge = applyGenericMergeMapToAbsAddr(absAddrSetGetSingle(innerSet), mergeMap);
                absAddrSetSetSingle(innerSet, aaMerge);
                break;
            }

            case ABS_ADDR_SET_VSET: {
                VSet *vset = absAddrSetGetVSet(innerSet);
                abstract_addr_t **array = (abstract_addr_t **)vSetToDataArray(vset);
                JITINT32 a, size = vSetSize(vset);
                vSetEmpty(vset);
                /* set->size -= size; */
                for (a = 0; a < size; ++a) {
                    abstract_addr_t *aaMerge = applyGenericMergeMapToAbsAddr(array[a], mergeMap);
                    if (vSetInsert(vset, aaMerge)) {
                        /* set->size += 1; */
                    }
                }
                vllpaFreeFunctionUnallocated(array);
                break;
            }

            default:
                abort();
        }
        setItem = vSetNext(set, setItem);
    }
}


/**
 * Apply the given generic merge map to each abstract address in the given
 * set.
 */
void
applyGenericMergeMapToAbstractAddressSet(abs_addr_set_t *aaSet, SmallHashTable *mergeMap) {
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            break;

        case ABS_ADDR_SET_SINGLE: {
            abstract_addr_t *aaMerge = applyGenericMergeMapToAbsAddr(absAddrSetGetSingle(aaSet), mergeMap);
            absAddrSetSetSingle(aaSet, aaMerge);
            break;
        }

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            applyGenericMergeMapToAbsAddrInternalSet(set, mergeMap);
            break;
        }

        default:
            abort();
    }
}


/**
 * Determine whether two sets of abstract addresses share at least one
 * abstract address.  Furthermore, if an abstract address in one set is
 * a merged address, then consider other abstract addresses that would
 * also have been merged as the same.
 */
static JITBOOLEAN
abstractAddressSetsOverlapActualSets(VSet *first, VSet *second, JITBOOLEAN checkMerge, aaset_prefix_t checkPrefixType) {
    /* Check basic case where the set intersection is non-empty. */
    if (vSetShareSomeData(first, second)) {
        return JITTRUE;
    }

    /* Check for overlap from merges. */
    if (checkMerge || checkPrefixType != AASET_PREFIX_NONE) {
        VSetItem *aaFirstItem = vSetFirst(first);
        while (aaFirstItem) {
            abstract_addr_t *aaFirst = getSetItemData(aaFirstItem);
            VSetItem *aaSecondItem = vSetFirst(second);
            while (aaSecondItem) {
                abstract_addr_t *aaSecond = getSetItemData(aaSecondItem);
                if (checkMerge && getAbstractAddressToMerge(aaFirst, aaSecond)) {
                    PDEBUG("      Overlap from merge\n");
                    return JITTRUE;
                } else if (areAbstractAddressesPrefixesOfEachOther(aaFirst, aaSecond, checkPrefixType)) {
                    PDEBUG("      Overlap from prefixes\n");
                    return JITTRUE;
                }
                aaSecondItem = vSetNext(second, aaSecondItem);
            }
            aaFirstItem = vSetNext(first, aaFirstItem);
        }
    }

    /* No overlaps found. */
    return JITFALSE;
}


/**
 * Check whether two abstract addresses overlap.
 */
static JITBOOLEAN
absAddrsOverlap(abstract_addr_t *first, abstract_addr_t *second, JITBOOLEAN checkMerge, aaset_prefix_t checkPrefixType) {
    if (first == second) {
        return JITTRUE;
    } else if (checkMerge && getAbstractAddressToMerge(first, second)) {
        PDEBUG("      Overlap from merge\n");
        return JITTRUE;
    } else if (areAbstractAddressesPrefixesOfEachOther(first, second, checkPrefixType)) {
        PDEBUG("      Overlap from prefixes\n");
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Determine whether two sets of abstract addresses share at least one abstract
 * address.  With the 'checkMerge' option, if an abstract address in one set
 * would merge into an abstract address in the other set, then consider that as
 * an overlap.  With the 'checkPrefixType' option, check whether an abstract
 * address in one set could be used as a prefix to an abstract address in the
 * other set.
 */
JITBOOLEAN
abstractAddressSetsOverlap(abs_addr_set_t *first, abs_addr_set_t *second, JITBOOLEAN checkMerge, aaset_prefix_t checkPrefixType) {
    JITBOOLEAN overlap = JITFALSE;
    assert(checkAbsAddrSetSanity(first));
    assert(checkAbsAddrSetSanity(second));
    assert(checkPrefixType >= AASET_PREFIX_NONE && checkPrefixType <= AASET_PREFIX_BOTH);

    /* Empty is easy. */
    if (absAddrSetIsEmpty(first)) {
        return JITFALSE;
    } else if (absAddrSetIsEmpty(second)) {
        return JITFALSE;
    }

    /* The same type for the two sets. */
    else if (absAddrSetGetType(first) == absAddrSetGetType(second)) {
        switch (absAddrSetGetType(first)) {
            case ABS_ADDR_SET_SINGLE: {
                abstract_addr_t *fSingle = absAddrSetGetSingle(first);
                abstract_addr_t *sSingle = absAddrSetGetSingle(second);
                return absAddrsOverlap(fSingle, sSingle, checkMerge, checkPrefixType);
            }

            case ABS_ADDR_SET_SET: {
                VSet *fset = absAddrSetGetSet(first);
                VSet *sset = absAddrSetGetSet(second);
                VSetItem *fitem = vSetFirst(fset);
                while (fitem) {
                    abs_addr_set_t *sinnerSet = vSetFindWithKey(sset, vSetItemKey(fitem));
                    if (sinnerSet) {
                        abs_addr_set_t *finnerSet = getSetItemData(fitem);
                        if (absAddrSetGetType(finnerSet) == absAddrSetGetType(sinnerSet)) {
                            switch (absAddrSetGetType(finnerSet)) {
                                case ABS_ADDR_SET_SINGLE: {
                                    abstract_addr_t *fSingle = absAddrSetGetSingle(finnerSet);
                                    abstract_addr_t *sSingle = absAddrSetGetSingle(sinnerSet);
                                    if (absAddrsOverlap(fSingle, sSingle, checkMerge, checkPrefixType)) {
                                        return JITTRUE;
                                    }
                                    break;
                                }
                                case ABS_ADDR_SET_VSET: {
                                    VSet *fvset = absAddrSetGetVSet(finnerSet);
                                    VSet *svset = absAddrSetGetVSet(sinnerSet);
                                    if (abstractAddressSetsOverlapActualSets(fvset, svset, checkMerge, checkPrefixType)) {
                                        return JITTRUE;
                                    }
                                    break;
                                }
                                default:
                                    abort();
                            }
                        } else {
                            VSet *fvset, *svset;
                            if (absAddrSetIsSingle(finnerSet)) {
                                assert(absAddrSetIsVSet(sinnerSet));
                                fvset = allocateNewSet(28);
                                vSetInsert(fvset, absAddrSetGetSingle(finnerSet));
                                svset = absAddrSetGetVSet(sinnerSet);
                            } else {
                                assert(absAddrSetIsVSet(finnerSet));
                                fvset = absAddrSetGetVSet(finnerSet);
                                svset = allocateNewSet(29);
                                vSetInsert(svset, absAddrSetGetSingle(sinnerSet));
                            }
                            overlap = abstractAddressSetsOverlapActualSets(fvset, svset, checkMerge, checkPrefixType);
                            if (absAddrSetIsSingle(finnerSet)) {
                                freeSet(fvset);
                            } else {
                                freeSet(svset);
                            }
                            if (overlap) {
                                return JITTRUE;
                            }
                        }
                    }
                    fitem = vSetNext(fset, fitem);
                }
                break;
            }

            default:
                abort();
        }
    }

    /* Different types for the two sets. */
    else {
        if (absAddrSetIsSingle(first)) {
            abs_addr_set_t *firstSet = absAddrSetClone(first);
            convertAbsAddrSetToSet(firstSet);
            assert(absAddrSetIsSet(second));
            overlap = abstractAddressSetsOverlap(firstSet, second, checkMerge, checkPrefixType);
            freeAbstractAddressSet(firstSet);
        } else {
            abs_addr_set_t *secondSet = absAddrSetClone(second);
            convertAbsAddrSetToSet(secondSet);
            assert(absAddrSetIsSet(first));
            overlap = abstractAddressSetsOverlap(first, secondSet, checkMerge, checkPrefixType);
            freeAbstractAddressSet(secondSet);
        }
    }
    return overlap;
}


/**
 * Return a set of elements that are in both given sets of abstract
 * addresses.  Actually returns the set using the conditions for the
 * function abstractAddressSetsOverlap above.
 */
abs_addr_set_t *
getOverlapSetFromAbstractAddressSets(abs_addr_set_t *first, abs_addr_set_t *second) {
    abs_addr_set_t *overlaps;
    abstract_addr_t *aaFirst;
    assert(checkAbsAddrSetSanity(first));
    assert(checkAbsAddrSetSanity(second));
    overlaps = newAbstractAddressSetEmpty();
    aaFirst = absAddrSetFirst(first);
    while (aaFirst) {
        JITBOOLEAN found = JITFALSE;
        abstract_addr_t *aaSecond = absAddrSetFirst(second);
        while (aaSecond && !found) {
            if (aaFirst == aaSecond) {
                absAddrSetAppendAbsAddr(overlaps, aaFirst);
                found = JITTRUE;
            } else {
                abstract_addr_t *aaMerge = getAbstractAddressToMerge(aaFirst, aaSecond);
                if (aaMerge == aaFirst) {
                    absAddrSetAppendAbsAddr(overlaps, aaSecond);
                } else if (aaMerge == aaSecond) {
                    absAddrSetAppendAbsAddr(overlaps, aaFirst);
                    found = JITTRUE;
                }
            }
            aaSecond = absAddrSetNext(second, aaSecond);
        }
        aaFirst = absAddrSetNext(first, aaFirst);
    }
    assert(checkAbsAddrSetSanity(overlaps));
    return overlaps;
}


/**
 * Add an abstract address to a list keeping it sorted.
 */
void
addAbsAddrToOrderedList(XanList *list, abstract_addr_t *aaAppend) {
    XanListItem *aaItem = xanList_first(list);

    /* Find the correct position in the set. */
    while (aaItem) {
        abstract_addr_t *aa = aaItem->data;
        if (compareAbsAddrs(&aa, &aaAppend) < 0) {
            xanList_insertBefore(list, aaItem, aaAppend);
            return;
        }
        aaItem = aaItem->next;
    }

    /* Not yet appended, add to the end. */
    xanList_append(list, aaAppend);
}


/**
 * Create an ordered list of abstract addresses from a set.
 */
XanList *
absAddrSetToOrderedList(abs_addr_set_t *aaSet) {
    XanList *list;
    abstract_addr_t *aa;

    /* The ordered list. */
    list = allocateNewList();

    /* Add each item in turn. */
    aa = absAddrSetFirst(aaSet);
    while (aa) {
        addAbsAddrToOrderedList(list, aa);
        aa = absAddrSetNext(aaSet, aa);
    }

    /* Return this list. */
    return list;
}


/**
 * Create an unordered list of abstract addresses from a set.
 */
XanList *
absAddrSetToUnorderedList(abs_addr_set_t *aaSet) {
    XanList *list;
    abstract_addr_t *aa;

    /* The ordered list. */
    list = allocateNewList();

    /* Add each item in turn. */
    aa = absAddrSetFirst(aaSet);
    while (aa) {
        xanList_append(list, aa);
        aa = absAddrSetNext(aaSet, aa);
    }

    /* Return this list. */
    return list;
}


/**
 * Add abstract addresses from an inner set to an array.
 */
static JITUINT32
addInnerAbsAddrSetToArray(abs_addr_set_t *innerSet, abstract_addr_t **array, JITUINT32 pos) {
    switch (absAddrSetGetType(innerSet)) {
        case ABS_ADDR_SET_SINGLE:
            array[pos] = absAddrSetGetSingle(innerSet);
            pos += 1;
            break;
        case ABS_ADDR_SET_VSET: {
            VSet *vset = absAddrSetGetVSet(innerSet);
            VSetItem *item = vSetFirst(vset);
            while (item) {
                array[pos] = vSetItemData(item);
                pos += 1;
                item = vSetNext(vset, item);
            }
            break;
        }
        default:
            abort();
    }
    return pos;
}


/**
 * Create an array of abstract addresses from a set.
 */
abstract_addr_t **
absAddrSetToArray(abs_addr_set_t *aaSet, JITUINT32 *num) {
    JITUINT32 size = absAddrSetSizeNoChecks(aaSet);
    abstract_addr_t **array = NULL;
    switch (absAddrSetGetType(aaSet)) {
        case ABS_ADDR_SET_EMPTY:
            break;

        case ABS_ADDR_SET_SINGLE:
            array = (abstract_addr_t **)vllpaAllocFunction(sizeof(abstract_addr_t *), 28);
            array[0] = absAddrSetGetSingle(aaSet);
            break;

        case ABS_ADDR_SET_SET: {
            VSet *set = absAddrSetGetSet(aaSet);
            VSetItem *item = vSetFirst(set);
            JITUINT32 pos = 0;
            array = (abstract_addr_t **)vllpaAllocFunction(size * sizeof(abstract_addr_t *), 29);
            while (item) {
                abs_addr_set_t *innerSet = getSetItemData(item);
                pos = addInnerAbsAddrSetToArray(innerSet, array, pos);
                item = vSetNext(set, item);
            }
            assert(pos == size);
            break;
        }

        default:
            abort();
    }

    if (num) {
        *num = size;
    }
    return array;
}


/**
 * Debugging printing functions.
 */


/**
 * Print a single abstract address.
 */
#ifdef PRINTDEBUG
void
printAbstractAddress(abstract_addr_t *aa, JITBOOLEAN newline) {
    if (enablePDebug) {
        PDEBUGB("<");
        printUiv(aa->uiv, "", JITFALSE);
        if (aa->offset == WHOLE_ARRAY_OFFSET) {
            PDEBUGB(",*>%s", newline ? "\n" : "");
        } else if (sizeof(JITNINT) == 8) {
            PDEBUGB(",%lld>%s", (JITINT64)aa->offset, newline ? "\n" : "");
        } else {
            PDEBUGB(",%d>%s", (JITINT32)aa->offset, newline ? "\n" : "");
        }
    }
}
#else
#define printAbstractAddress(aa, newline)
#endif  /* ifdef PRINTDEBUG */


/**
 * Get an ordered array of abstract addresses from an actual set.
 */
#ifdef PRINTDEBUG
static abstract_addr_t **
getOrderedAbsAddrArrayFromActualSet(VSet *set, JITUINT32 *num) {
    abstract_addr_t **array = (abstract_addr_t **)vSetToDataArray(set);
    *num = vSetSize(set);
    qsort(array, *num, sizeof(void *), compareAbsAddrs);
    return array;
}
#endif  /* ifdef PRINTDEBUG */


/**
 * Print the abstract addresses from the given actual set.
 */
#ifdef PRINTDEBUG
static void
printAbstractAddressActualSet(VSet *set) {
    if (enablePDebug) {
        JITUINT32 i, num;
        abstract_addr_t **array = getOrderedAbsAddrArrayFromActualSet(set, &num);
        for (i = 0; i < num; ++i) {
            printAbstractAddress(array[i], JITFALSE);
            if (i + 1 < num) {
                PDEBUGB(", ");
            }
        }
        PDEBUGB("\n");
        vllpaFreeFunctionUnallocated(array);
    }
}
#else
#define printAbstractAddressActualSet(set)
#endif  /* ifdef PRINTDEBUG */


/**
 * Print the abstract addresses from the given list.
 */
#ifdef PRINTDEBUG
void
printAbstractAddressList(XanList *aaList) {
    if (enablePDebug) {
        XanListItem *aaItem = xanList_first(aaList);
        while (aaItem) {
            abstract_addr_t *aa = aaItem->data;
            printAbstractAddress(aa, JITFALSE);
            aaItem = aaItem->next;
            if (aaItem) {
                PDEBUGB(", ");
            }
        }
        PDEBUGB("\n");
    }
}
#endif  /* ifdef PRINTDEBUG */


/**
 * Print the abstract addresses from the given set.
 */
#ifdef PRINTDEBUG
void
printAbstractAddressSet(abs_addr_set_t *aaSet) {
    if (enablePDebug) {
        XanList *sorted;

        /* Create ordered list. */
        sorted = absAddrSetToOrderedList(aaSet);

        /* Print in order. */
        printAbstractAddressList(sorted);

        /* No longer need the sorted set. */
        freeList(sorted);
    }
}
#endif  /* ifdef PRINTDEBUG */
