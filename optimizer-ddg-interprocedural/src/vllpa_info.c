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
 * This file implements functions for manipulating the methods information
 * structure or the VLLPA algorithm from the following paper:
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
#include "vllpa_aliases.h"
#include "vllpa_info.h"
#include "vllpa_macros.h"
#include "vllpa_memory.h"
#include "vllpa_types.h"
#include "vllpa_util.h"
#include "vset.h"
// End


/**
 * Create a mapping of an abstract address set with the given number of
 * entries in the updated array.
 */
aa_mapped_set_t *
createAbsAddrMappedSet(JITUINT32 numUpdates) {
    aa_mapped_set_t *aaMappedSet = vllpaAllocFunction(sizeof(aa_mapped_set_t), 17);
    aaMappedSet->aaSet = newAbstractAddressSetEmpty();
    aaMappedSet->aaSuperSet = aaMappedSet->aaSet;
    aaMappedSet->originalAbsAddrs = NULL;
    aaMappedSet->numOriginalAddrs = 0;
    return aaMappedSet;
}


/**
 * Free a mapped abstract address set.
 */
void
freeAbsAddrMappedSet(aa_mapped_set_t *aaMappedSet) {
    freeAbstractAddressSet(aaMappedSet->aaSet);
    if (aaMappedSet->aaSuperSet != aaMappedSet->aaSet) {
        freeAbstractAddressSet(aaMappedSet->aaSuperSet);
    }
    assert(aaMappedSet->aaOriginalSet == NULL);
    assert(aaMappedSet->originalAbsAddrs == NULL);
    vllpaFreeFunction(aaMappedSet, sizeof(aa_mapped_set_t));
}


/**
 * Copy a single mapped abstract address set.
 */
static void
copySingleMappedAbsAddrSet(aa_mapped_set_t *aaMap, JITBOOLEAN pointToSuper) {

    /* Record the orginal abstract addresses. */
    aaMap->originalAbsAddrs = absAddrSetToArray(aaMap->aaSet, &(aaMap->numOriginalAddrs));

    /* Point to the super set. */
    if (pointToSuper && aaMap->aaSet != aaMap->aaSuperSet) {
        freeAbstractAddressSet(aaMap->aaSet);
        aaMap->aaSet = aaMap->aaSuperSet;
    }
}


/**
 * Copy each abstract address set in the given mapping table so that later,
 * after appending abstract addresses and merging, each new set can be
 * compared to the old one to determine whether any changes have been made.
 */
void
copyMappedAbsAddrSetsTable(SmallHashTable *mapToAbsAddrSet) {
    SmallHashTableItem *mapItem = smallHashTableFirst(mapToAbsAddrSet);
    while (mapItem) {
        copySingleMappedAbsAddrSet(mapItem->element, JITTRUE);
        mapItem = smallHashTableNext(mapToAbsAddrSet, mapItem);
    }
}


/**
 * Copy each mapped abstract address set in the given array.
 */
void
copyMappedAbsAddrSetsArray(aa_mapped_set_t **arrayOfAbsAddrSets, JITUINT32 num) {
    JITUINT32 i;
    for (i = 0; i < num; ++i) {
        if (arrayOfAbsAddrSets[i]) {
            copySingleMappedAbsAddrSet(arrayOfAbsAddrSets[i], JITFALSE);
        }
    }
}


/**
 * Replace a mapped abstract address set.
 */
static void
replaceSingleMappedAbsAddrSet(aa_mapped_set_t *aaMap, abs_addr_set_t *aaSet) {
    absAddrSetEmpty(aaMap->aaSet);
    absAddrSetAppendSet(aaMap->aaSet, aaSet);
    if (aaMap->aaSet != aaMap->aaSuperSet) {
        absAddrSetEmpty(aaMap->aaSuperSet);
        absAddrSetAppendSet(aaMap->aaSuperSet, aaSet);
    }
}


/**
 * Append to a single mapped abstract address set.
 */
static void
appendSingleMappedAbsAddrSet(aa_mapped_set_t *aaMap, abs_addr_set_t *aaSet, SmallHashTable *uivMergeTargets) {

    /* Need to append to the super set but keep the main set merged. */
    if (aaMap->aaSet != aaMap->aaSuperSet) {
        absAddrSetAppendSet(aaMap->aaSuperSet, aaSet);
        freeAbstractAddressSet(aaMap->aaSet);
        aaMap->aaSet = absAddrSetClone(aaMap->aaSuperSet);
        absAddrSetMerge(aaMap->aaSet, uivMergeTargets);
    } else {
        absAddrSetAppendSet(aaMap->aaSet, aaSet);
    }
}


/**
 * Print a mapping between an abstract address and a set of abstract addresses.
 */
void
printAbstractAddressMapping(abstract_addr_t *key, abs_addr_set_t *set, char *prefix) {
    PDEBUG("%s", prefix);
    printAbstractAddress(key, JITFALSE);
    PDEBUGB(": ");
    printAbstractAddressSet(set);
}


/**
 * Merge a single mapped abstract address set.
 */
static void
mergeSingleMappedAbsAddrSet(aa_mapped_set_t *aaMap, SmallHashTable *uivMergeTargets) {

    /* Clone the super set and merge within that. */
    aaMap->aaSet = absAddrSetClone(aaMap->aaSuperSet);
    absAddrSetMerge(aaMap->aaSet, uivMergeTargets);
}


/**
 * Merge the abstract addresses in all abstract address sets in the given
 * mapping table.
 */
void
mergeMappedAbsAddrSetsTable(SmallHashTable *mapToAbsAddrSet, SmallHashTable *uivMergeTargets) {
    SmallHashTableItem *mapItem = smallHashTableFirst(mapToAbsAddrSet);
    while (mapItem) {
        mergeSingleMappedAbsAddrSet(mapItem->element, uivMergeTargets);
        mapItem = smallHashTableNext(mapToAbsAddrSet, mapItem);
    }
}


/**
 * Merge each mapped abstract address set in the given array.
 */
void
mergeMappedAbsAddrSetsArray(aa_mapped_set_t **arrayOfAbsAddrSets, JITUINT32 num, SmallHashTable *uivMergeTargets) {
    JITUINT32 i;
    for (i = 0; i < num; ++i) {
        if (arrayOfAbsAddrSets[i]) {
            mergeSingleMappedAbsAddrSet(arrayOfAbsAddrSets[i], uivMergeTargets);
        }
    }
}


/**
 * Check whether a single mapped abstract address set has changed.
 */
static JITBOOLEAN
hasSingleMappedAbsAddrSetChanged(aa_mapped_set_t *aaMap, JITBOOLEAN *new, abstract_addr_t **aaMissing) {
    JITBOOLEAN changed = JITFALSE;
    if (absAddrSetSize(aaMap->aaSet) != aaMap->numOriginalAddrs) {
        changed = JITTRUE;
        if (aaMap->originalAbsAddrs) {
            vllpaFreeFunction(aaMap->originalAbsAddrs, aaMap->numOriginalAddrs * sizeof(abstract_addr_t *));
            aaMap->originalAbsAddrs = NULL;
        } else {
            *new = JITTRUE;
        }
    } else if (aaMap->originalAbsAddrs) {
        JITUINT32 a;
        for (a = 0; a < aaMap->numOriginalAddrs && !changed; ++a) {
            if (!absAddrSetContains(aaMap->aaSet, aaMap->originalAbsAddrs[a])) {
                *aaMissing = aaMap->originalAbsAddrs[a];
                changed = JITTRUE;
            }
        }
        vllpaFreeFunction(aaMap->originalAbsAddrs, aaMap->numOriginalAddrs * sizeof(abstract_addr_t *));
        aaMap->originalAbsAddrs = NULL;
    }

    /* Return whether there were changes. */
    return changed;
}


/**
 * Check each abstract address set in the given mapping table against the
 * orginal copy that was previously made to determine whether any changes
 * have occurred.  At the same time, free all of the original copies.
 */
JITBOOLEAN
haveMappedAbsAddrSetsChangedTable(SmallHashTable *mapToAbsAddrSet, JITBOOLEAN fromAbsAddr) {
    JITBOOLEAN changes = JITFALSE;
    SmallHashTableItem *mapItem = smallHashTableFirst(mapToAbsAddrSet);
    while (mapItem) {
        aa_mapped_set_t *aaMap = mapItem->element;
        abstract_addr_t *aaMissing = NULL;
        JITBOOLEAN new = JITFALSE;
        JITBOOLEAN changed = hasSingleMappedAbsAddrSetChanged(aaMap, &new, &aaMissing);

        /* Debugging. */
        if (changed) {
            PDEBUG("%s abstract address mapping ", new ? "New" : "Changed");
            if (fromAbsAddr) {
                printAbstractAddress(mapItem->key, JITFALSE);
            } else {
                printUiv(mapItem->key, "", JITFALSE);
            }
            PDEBUGB(": ");
            printAbstractAddressSet(aaMap->aaSet);
            if (aaMissing) {
                PDEBUG("  Missing abstract address ");
                printAbstractAddress(aaMissing, JITTRUE);
            } else if (!new) {
                PDEBUG("  Different sized set - was %u now %u\n", aaMap->numOriginalAddrs, absAddrSetSize(aaMap->aaSet));
            }
        }

        /* Record overall changes. */
        changes |= changed;

        /* Next set. */
        mapItem = smallHashTableNext(mapToAbsAddrSet, mapItem);
    }

    /* Return whether there were changes. */
    return changes;
}


/**
 * Check each mapped abstract address set in the given array for changes.
 */
JITBOOLEAN
haveMappedAbsAddrSetsChangedArray(aa_mapped_set_t **arrayOfAbsAddrSets, JITUINT32 num) {
    JITUINT32 i;
    JITBOOLEAN changes = JITFALSE;
    for (i = 0; i < num; ++i) {
        if (arrayOfAbsAddrSets[i]) {
            aa_mapped_set_t *aaMap = arrayOfAbsAddrSets[i];
            abstract_addr_t *aaMissing = NULL;
            JITBOOLEAN new = JITFALSE;
            JITBOOLEAN changed = hasSingleMappedAbsAddrSetChanged(aaMap, &new, &aaMissing);

            /* Debugging. */
            if (changed) {
                PDEBUG("%s abstract address mapping %u: ", new ? "New" : "Changed", i);
                printAbstractAddressSet(aaMap->aaSet);
                if (aaMissing) {
                    PDEBUG("  Missing abstract address ");
                    printAbstractAddress(aaMissing, JITTRUE);
                } else if (!new) {
                    PDEBUG("  Different sized set - was %u now %u\n", aaMap->numOriginalAddrs, absAddrSetSize(aaMap->aaSet));
                }
            }

            /* Record overall changes. */
            changes |= changed;
        }
    }
    return changes;
}


/**
 * Add a key abstract address to the mapping between uivs and the lists of
 * memory key abstract addresses that they are part of.
 */
static void
addUivToMemAbsAddrKeyMapping(abstract_addr_t *aaKey, method_info_t *info) {
    uiv_t *baseUiv = getBaseUiv(aaKey->uiv);
    XanList *aaList = smallHashTableLookup(info->uivToMemAbsAddrKeysMap, baseUiv);
    if (!aaList) {
        aaList = allocateNewList();
        smallHashTableInsert(info->uivToMemAbsAddrKeysMap, baseUiv, aaList);
    }
    xanList_append(aaList, aaKey);
}


/**
 * Get a set of abstract addresses that the given memory location (abstract
 * address) can point to within the given method.  If there isn't a set for
 * this location already, and the 'create' flag is set, then create one.
 * Returns a pointer to the set for this location, so that it can be altered
 * if necessary.
 */
abs_addr_set_t *
getMemAbstractAddressSet(abstract_addr_t *aa, method_info_t *info, JITBOOLEAN create) {
    aa_mapped_set_t *existingMapping;

    /* Find the existing set, if there. */
    if (!(existingMapping = smallHashTableLookup(info->memAbsAddrMap, aa))) {
        if (create) {
            existingMapping = createAbsAddrMappedSet(info->numCallers);
            smallHashTableInsert(info->memAbsAddrMap, aa, existingMapping);
            addUivToMemAbsAddrKeyMapping(aa, info);
            /* PDEBUG("  Created new mem mapping: "); */
            /* printAbstractAddress(aa, JITFALSE); */
            /* PDEBUGB(" -> "); */
            /* printAbstractAddressSet(existingMapping->aaSet); */
        } else {
            return NULL;
        }
    }

    /* Return the set. */
    return existingMapping->aaSet;
}


/**
 * Get a set of abstract addresses that the given variable can point to
 * within the given method.  Creates a new set if the there isn't one
 * already saved.  If the variable is an escaped variable then the actual
 * set of abstract addresses will be in memory, so look there instead.
 * This version takes the variable directly as an argument.
 */
abs_addr_set_t *
getVarAbstractAddressSetVarKey(JITUINT32 var, method_info_t *info, JITBOOLEAN now) {
    abs_addr_set_t *set;

    /* If this is an escaped variable then the set will be in memory. */
    if (IRMETHOD_isAnEscapedVariable(info->ssaMethod, var)) {
        uiv_t *uiv = newUivFromVar(info->ssaMethod, var, info->uivs);
        abstract_addr_t *aa = newAbstractAddress(uiv, 0);
        set = getMemAbstractAddressSet(aa, info, JITTRUE);
    }

    /* A normal variable. */
    else {
        if (!info->varAbsAddrSets[var]) {
            info->varAbsAddrSets[var] = createAbsAddrMappedSet(1);
        }
        set = info->varAbsAddrSets[var]->aaSet;
    }

    /* Return this set. */
    return set;
}


/**
 * Get a set of abstract addresses that the given variable can point to
 * within the given method.  Creates a new set if the there isn't one
 * already saved.  If the variable is an escaped variable then the actual
 * set of abstract addresses will be in memory, so look there instead.
 */
abs_addr_set_t *
getVarAbstractAddressSet(ir_item_t *param, method_info_t *info, JITBOOLEAN now) {
    /* Must be a variable parameter. */
    assert(param->type == IROFFSET);

    /* Pass this off to the version that uses a variable. */
    return getVarAbstractAddressSetVarKey(param->value.v, info, now);
}


/**
 * Append the given set of abstract addresses to the set that the given
 * memory location (abstract address) can point to within the given
 * method.  Will not free the given set of abstract addresses.
 */
void
appendSetToMemAbsAddrSet(abstract_addr_t *aaKey, method_info_t *info, abs_addr_set_t *set) {
    abs_addr_set_t *setToAppend;

    /* Function pointer uivs can't point to anything. */
    if (isBaseUivFromFunc(aaKey->uiv)) {
        return;
    }

    /* Remove the key abstract address from a clone of the set, if there. */
    if (absAddrSetContains(set, aaKey)) {
        setToAppend = absAddrSetCloneWithoutAbsAddr(set, aaKey);
    } else {
        setToAppend = set;
    }

    /* If there's nothing new to store, don't do anything. */
    if (!absAddrSetIsEmpty(setToAppend)) {
        aa_mapped_set_t *existingMapping;

        /* Find the existing set, if there. */
        if((existingMapping = smallHashTableLookup(info->memAbsAddrMap, aaKey))) {
            absAddrSetAppendSet(existingMapping->aaSet, setToAppend);
        }

        /* No existing set, use a clone of the given one. */
        else {
            existingMapping = createAbsAddrMappedSet(info->numCallers);
            absAddrSetAppendSet(existingMapping->aaSet, setToAppend);
            smallHashTableInsert(info->memAbsAddrMap, aaKey, existingMapping);
            addUivToMemAbsAddrKeyMapping(aaKey, info);
            /* PDEBUG("  Created new mem mapping: "); */
            /* printAbstractAddress(aaKey, JITFALSE); */
            /* PDEBUGB(" -> "); */
            /* printAbstractAddressSet(existingMapping->aaSet); */
        }
        /* PDEBUG("  Setting mem mapping: "); */
        /* printAbstractAddress(aaKey, JITFALSE); */
        /* PDEBUGB(" -> "); */
        /* printAbstractAddressSet(existingMapping->aaSet); */
    }

    /* If cloned, remove the set to append. */
    if (setToAppend != set) {
        freeAbstractAddressSet(setToAppend);
    }
}


/**
 * Set the set of abstract addresses that the given variable can point
 * to within the given method.  This actually stores the given set so any
 * changes made after a call to this function will be reflected in the
 * set stored for this variable.  If the given variable is an escaped
 * variable then the set will be stored in memory instead.  Returns JITTRUE
 * if there is a change in the stored set (i.e. any of the set items are
 * different, or a new set is stored).
 */
JITBOOLEAN
setVarAbstractAddressSet(JITUINT32 var, method_info_t *info, abs_addr_set_t *set, JITBOOLEAN replace) {
    JITBOOLEAN change = JITFALSE;

    /* If there's nothing new to store, don't do anything. */
    if (!absAddrSetIsEmpty(set)) {
        /* PDEBUG("  Setting var %4u set to: ", var); */
        /* printAbstractAddressSet(set); */

        /* If this is an escaped variable then the set will be in memory. */
        if (IRMETHOD_isAnEscapedVariable(info->ssaMethod, var)) {
            uiv_t *uiv = newUivFromVar(info->ssaMethod, var, info->uivs);
            abstract_addr_t *aa = newAbstractAddress(uiv, 0);

            /* Changes will be picked up later. */
            appendSetToMemAbsAddrSet(aa, info, set);
        }

        /* A normal variable. */
        else {
            aa_mapped_set_t *existingSet;

            /* Get the existing set, if there. */
            if ((existingSet = info->varAbsAddrSets[var])) {

                /* Replacem or append the set. */
                if (replace) {
                    replaceSingleMappedAbsAddrSet(existingSet, set);
                } else {
                    appendSingleMappedAbsAddrSet(existingSet, set, info->uivMergeTargets);
                }
            }

            /* No existing set, store this one. */
            else {
                existingSet = createAbsAddrMappedSet(1);
                info->varAbsAddrSets[var] = existingSet;
                appendSingleMappedAbsAddrSet(existingSet, set, info->uivMergeTargets);
            }

            /* Debugging. */
            /* PDEBUG("  Setting var mapping: %u -> ", var); */
            /* printAbstractAddressSet(existingSet->aaSet); */
        }
    }

    /* Always need to free the set. */
    freeAbstractAddressSet(set);

    /* Return whether there was any change in the set. */
    return change;
}


/**
 * Map a callee abstract address to a set of caller abstract addresses.
 */
static abs_addr_set_t *
mapCalleeAbsAddrToCallerAbsAddrSetInternal(abstract_addr_t *aa, method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap, JITBOOLEAN expandVarMappings) {
    aa_mapped_set_t *callerMap;
    abs_addr_set_t *aaSet = NULL;
    /*   PDEBUGLITE("Mapping callee to caller\n"); */

    /* Assertions. */
    assert(aa);
    assert(callerInfo);
    assert(calleeInfo);
    assert(calleeUivMap);

    /* Check whether this uiv maps to a set. */
    callerMap = smallHashTableLookup(calleeUivMap, aa->uiv);
    if (callerMap) {

        /* Only do the mapping if required. */
        if (JITTRUE) {
        /* if (!onlyIfUpdated) { */
            abs_addr_set_t *callerSet = callerMap->aaSet;
            abstract_addr_t *aaCaller;
            assert(callerSet);

            /* The set to be returned. */
            aaSet = newAbstractAddressSetEmpty();

            /* Add new abstract addresses based on the mapping to the caller. */
            aaCaller = absAddrSetFirst(callerSet);
            while (aaCaller) {
                /*         PDEBUG("  Considering: "); */
                /*         printAbstractAddress(aaCaller, JITTRUE); */
                if (aa->offset == 0) {
                    absAddrSetAppendAbsAddr(aaSet, aaCaller);
                } else {
                    abstract_addr_t *aaUpdated = newAbstractAddress(aaCaller->uiv, aaCaller->offset + aa->offset);
                    absAddrSetAppendAbsAddr(aaSet, aaUpdated);
                }
                aaCaller = absAddrSetNext(callerSet, aaCaller);
            }
            /*       PDEBUGLITE("Finished mapping\n"); */
            /*     } else { */
            /*       PDEBUG("  No need to do mapping\n"); */
        }
    }

    /* The uiv does not map to a set. */
    else if (!isBaseUivFromVar(aa->uiv)) {
        uiv_t *uiv;
        /*     PDEBUGLITE("Creating new aa\n"); */
        assert(callerInfo->uivs);
        uiv = newUivBasedOnUiv(aa->uiv, callerInfo->uivs);
        assert(uiv);
        aaSet = newAbstractAddressInNewSet(uiv, aa->offset);
    }

    /**
     * No mapping and the base uiv is from a variable.  We allow this to be
     * mapped to a new set when dealing with function pointers because these
     * are typically derived from an offset from the variable (which should
     * be a method parameter), so don't exactly match an existing uiv.  To
     * do this we recurse down the uiv levels and build back up again on a
     * match.
     */
    else if (expandVarMappings && getLevelOfBaseUiv(aa->uiv) > 0) {
        uiv_t *uivSubLevel = getUivAtLevel(aa->uiv, 1);
        JITNINT offsetSubLevel = getFieldOffsetAtLevelFromUiv(aa->uiv, 0);
        abstract_addr_t *aaSubLevel = newAbstractAddress(uivSubLevel, offsetSubLevel);
        PDEBUG("Couldn't map abstract address: ");
        printAbstractAddress(aa, JITTRUE);
        PDEBUG("Attempting to map sub abstract address: ");
        printAbstractAddress(aaSubLevel, JITTRUE);
        abs_addr_set_t *mappedSet = mapCalleeAbsAddrToCallerAbsAddrSetInternal(aaSubLevel, callerInfo, calleeInfo, calleeUivMap, expandVarMappings);

        /* Reconstruct if there's a mapping now found. */
        if (mappedSet) {
            abstract_addr_t *aaMapped = absAddrSetFirst(mappedSet);
            while (aaMapped) {
                if (!isBaseUivFromVar(aaMapped->uiv)) {
                    uiv_t *uivBuilt = newUivFromUiv(aaMapped->uiv, aaMapped->offset, callerInfo->uivs);
                    abstract_addr_t *aaBuilt = newAbstractAddress(uivBuilt, aa->offset);
                    if (!aaSet) {
                        aaSet = newAbstractAddressSetEmpty();
                    }
                    absAddrSetAppendAbsAddr(aaSet, aaBuilt);
                }
                aaMapped = absAddrSetNext(mappedSet, aaMapped);
            }
            freeAbstractAddressSet(mappedSet);
            if (aaSet) {
                PDEBUG("Reconstructed set is: ");
                printAbstractAddressSet(aaSet);
            }
        }
    }

    /* Return the set of caller abstract addresses. */
    /*   PDEBUGLITE("Finished mapping callee to caller\n"); */
    return aaSet;
}


/**
 * Map a callee abstract address to a set of caller abstract addresses and
 * cache the result for use later.
 */
abs_addr_set_t *
mapCalleeAbsAddrToCallerAbsAddrSet(abstract_addr_t *aa, method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap, JITBOOLEAN expandVarMappings) {
    SmallHashTable *calleeMappings;
    abs_addr_set_t *aaSet = NULL;

    /* See if the set is cached. */
    calleeMappings = smallHashTableLookup(callerInfo->calleeToCallerAbsAddrMap, calleeInfo);
    if (!calleeMappings) {
        calleeMappings = allocateNewHashTable();
        smallHashTableInsert(callerInfo->calleeToCallerAbsAddrMap, calleeInfo, calleeMappings);
    } else {
        aaSet = smallHashTableLookup(calleeMappings, aa);
    }

    /* If not cached, do the mapping. */
    if (!aaSet) {
        aaSet = mapCalleeAbsAddrToCallerAbsAddrSetInternal(aa, callerInfo, calleeInfo, calleeUivMap, expandVarMappings);
        if (aaSet) {
            smallHashTableInsert(calleeMappings, aa, aaSet);
            aaSet = shareAbstractAddressSet(aaSet);
        }
    } else {
        aaSet = shareAbstractAddressSet(aaSet);
    }

    /* Return this set. */
    return aaSet;
}


/**
 * Map a set of callee abstract addresses to a set of caller abstract
 * addresses, returning the set of mapped abstract addresses.
 */
abs_addr_set_t *
mapCalleeAbsAddrSetToCallerAbsAddrSet(abs_addr_set_t *calleeSet, method_info_t *callerInfo, method_info_t *calleeInfo, SmallHashTable *calleeUivMap) {
    abstract_addr_t *aaCallee;
    abs_addr_set_t *mappedSet = newAbstractAddressSetEmpty();

    /* Map each abstract address in the register's set to the caller. */
    aaCallee = absAddrSetFirst(calleeSet);
    while (aaCallee) {
        abs_addr_set_t *callerSet = mapCalleeAbsAddrToCallerAbsAddrSet(aaCallee, callerInfo, calleeInfo, calleeUivMap, JITFALSE);

        /* Add to the set that will be returned. */
        if (callerSet) {
            absAddrSetAppendSet(mappedSet, callerSet);
            freeAbstractAddressSet(callerSet);
        }

        /* Next abstract address in the callee set. */
        aaCallee = absAddrSetNext(calleeSet, aaCallee);
    }
    absAddrSetMerge(mappedSet, callerInfo->uivMergeTargets);

    /* Return the new set. */
    return mappedSet;
}


/**
 * Get the set of abstract addresses accessed through a symbol and possible
 * offset.
 */
static abs_addr_set_t *
getSymbolAbsAddrSet(ir_item_t *baseParam, ir_item_t *offsetParam, method_info_t *info) {
    uiv_t *uiv;
    if (offsetParam != NULL) {
        assert(offsetParam->type != IROFFSET);
        uiv = newUivFromGlobal(baseParam, offsetParam->value.v, info->uivs);
    } else {
        uiv = newUivFromGlobal(baseParam, 0, info->uivs);
    }
    return newAbstractAddressInNewSet(uiv, 0);
}


/**
 * Get the set of abstract addresses accessed through a function pointer.
 */
static abs_addr_set_t *
getFunctionPointerAbsAddrSet(ir_item_t *param, method_info_t *info) {
    uiv_t *uiv = newUivFromFunc(param->value.v, info->uivs);
    return newAbstractAddressInNewSet(uiv, 0);
}


/**
 * Get the set of abstract addresses that a load or store instruction accesses
 * through a variable and offset.
 */
static abs_addr_set_t *
getLoadStoreVarAbsAddrSet(ir_instruction_t *inst, ir_item_t *baseParam, ir_item_t *offsetParam, method_info_t *info) {
    abs_addr_set_t *aaSet;

    /* A couple of checks. */
    assert(baseParam->type == IROFFSET);
    assert(offsetParam->type != IROFFSET);

    /* Get the base set first. */
    aaSet = getVarAbstractAddressSet(baseParam, info, JITTRUE);
    /* PDEBUG("  Base abstract addresses are: "); */
    /* printAbstractAddressSet(aaSet); */

    /* Non-zero offset requires some alterations. */
    if (offsetParam->value.v != 0) {
        abs_addr_set_t *baseSet = aaSet;
        abstract_addr_t *aaBase;
        IR_ITEM_VALUE offset;

        /* Calculate the correct offset. */
        offset = offsetParam->value.v;

        /* Construct the actual access set. */
        aaSet = newAbstractAddressSetEmpty();
        aaBase = absAddrSetFirst(baseSet);
        while (aaBase) {
            if (aaBase->offset != WHOLE_ARRAY_OFFSET) {
                abstract_addr_t *aaOffset = newAbstractAddress(aaBase->uiv, aaBase->offset + offset);
                absAddrSetAppendAbsAddr(aaSet, aaOffset);
            } else {
                absAddrSetAppendAbsAddr(aaSet, aaBase);
            }
            aaBase = absAddrSetNext(baseSet, aaBase);
        }
    }

    /* We've got to return a set that can be freed. */
    else {
        aaSet = absAddrSetClone(aaSet);
    }

    /* Return this set. */
    return aaSet;
}


/**
 * Get the set of abstract addresses that a load or store instruction accesses.
 * I.e. those that are keys into the memory abstract address map.  The returned
 * set needs freeing after use.
 */
abs_addr_set_t *
getLoadStoreAccessedAbsAddrSet(ir_instruction_t *inst, method_info_t *info) {
    ir_item_t *baseParam = IRMETHOD_getInstructionParameter1(inst);
    ir_item_t *offsetParam = IRMETHOD_getInstructionParameter2(inst);
    assert(offsetParam->type != IROFFSET);

    /* Deal with different types of base pointer and offset. */
    if (isVariableParam(baseParam)) {
        return getLoadStoreVarAbsAddrSet(inst, baseParam, offsetParam, info);
    } else if (IRDATA_isASymbol(baseParam) || isImmIntParam(baseParam)) {
        return getSymbolAbsAddrSet(baseParam, offsetParam, info);
    } else {
        abort();
    }
}


/**
 * Get the set of abstract addresses accessed through a call parameter.
 */
static abs_addr_set_t *
getParamAccessedAbsAddrSet(ir_item_t *param, method_info_t *info) {
    if (isVariableParam(param)) {
        abs_addr_set_t *aaSet = getVarAbstractAddressSet(param, info, JITFALSE);
        return absAddrSetClone(aaSet);
    } else if (IRDATA_isASymbol(param) || isImmIntParam(param)) {
        return getSymbolAbsAddrSet(param, NULL, info);
    } else if (IRDATA_isAFunctionPointer(param)) {
        return getFunctionPointerAbsAddrSet(param, info);
    } else {
        abort();
    }
}


/**
 * Get the set of abstract addresses accessed through a call parameter.
 */
abs_addr_set_t *
getCallParamAccessedAbsAddrSet(ir_instruction_t *inst, JITUINT32 nth, method_info_t *info) {
    ir_item_t *param = IRMETHOD_getInstructionCallParameter(inst, nth);
    return getParamAccessedAbsAddrSet(param, info);
}


/**
 * Add abstract addresses that are reachable in memory from the given set until
 * no more abstract addresses can be accessed.
 */
static void
addReachableMemoryAbstractAddresses(abs_addr_set_t **aaSet, method_info_t *info) {
    abs_addr_set_t *workingSet = absAddrSetClone(*aaSet);
    while (!absAddrSetIsEmpty(workingSet)) {
        abs_addr_set_t *justFound = newAbstractAddressSetEmpty();
        abstract_addr_t *aa = absAddrSetFirst(workingSet);
        while (aa) {
            abs_addr_set_t *keySet = getKeyAbsAddrSetFromUivPrefix(aa->uiv, info);
            abstract_addr_t *aaKey = absAddrSetFirst(keySet);
            while (aaKey) {
                abs_addr_set_t *memSet = getMemAbstractAddressSet(aaKey, info, JITTRUE);
                if (!abstractAddressSetContainsSet(*aaSet, memSet)) {
                    absAddrSetAppendSet(justFound, memSet);
                    absAddrSetAppendSet(*aaSet, memSet);
                    absAddrSetMerge(justFound, info->uivMergeTargets);
                    absAddrSetMerge(*aaSet, info->uivMergeTargets);
                }
                aaKey = absAddrSetNext(keySet, aaKey);
            }
            freeAbstractAddressSet(keySet);
            aa = absAddrSetNext(workingSet, aa);
        }
        freeAbstractAddressSet(workingSet);
        workingSet = justFound;
    }
    freeAbstractAddressSet(workingSet);
}


/**
 * Get the set of abstract addresses accessed through a call parameter, along
 * with all others that are reachable from these addresses too.
 **/
abs_addr_set_t *
getCallParamReachableAbsAddrSet(ir_instruction_t *inst, JITUINT32 nth, method_info_t *info) {
    abs_addr_set_t *set = getCallParamAccessedAbsAddrSet(inst, nth, info);
    addReachableMemoryAbstractAddresses(&set, info);
    return set;
}


/**
 * Get the set of abstract addresses accessed through an instruction parameter.
 */
abs_addr_set_t *
getInstParamAccessedAbsAddrSet(ir_instruction_t *inst, JITUINT32 parNum, method_info_t *info) {
    ir_item_t *param = IRMETHOD_getInstructionParameter(inst, parNum);
    return getParamAccessedAbsAddrSet(param, info);
}


/**
 * Determine whether an abstract address is a key into the given abstract
 * address map.
 */
JITBOOLEAN
isKeyAbsAddr(abstract_addr_t *aa, SmallHashTable *absAddrMap) {
    return smallHashTableLookup(absAddrMap, aa) != NULL;
}


/**
 * Get a set of abstract addresses that are used as keys into the memory
 * abstract addresses for the given abstract address.
 */
abs_addr_set_t *
getKeyAbsAddrSetFromAbsAddr(abstract_addr_t *aa, method_info_t *info) {
    abs_addr_set_t *keySet = newAbstractAddressSetEmpty();
    SmallHashTableItem *mapItem = smallHashTableFirst(info->memAbsAddrMap);
    while (mapItem) {
        aa_mapped_set_t *mapping = mapItem->element;
        abs_addr_set_t *memSet = mapping->aaSet;
        if (absAddrSetContains(memSet, aa)) {
            abstract_addr_t *aaKey = mapItem->key;
            absAddrSetAppendAbsAddr(keySet, aaKey);
        }
        mapItem = smallHashTableNext(info->memAbsAddrMap, mapItem);
    }
    return keySet;
}


/**
 * Get a set of abstract addresses that are used as keys into the memory
 * abstract addresses, where the keys contain the given uiv.
 */
abs_addr_set_t *
getKeyAbsAddrSetFromUivKey(uiv_t *uiv, method_info_t *info) {
    uiv_t *baseUiv = getBaseUiv(uiv);
    abs_addr_set_t *keySet = newAbstractAddressSetEmpty();
    XanList *aaList = smallHashTableLookup(info->uivToMemAbsAddrKeysMap, baseUiv);
    if (aaList) {
        XanListItem *aaItem = xanList_first(aaList);
        while (aaItem) {
            abstract_addr_t *aaKey = aaItem->data;
            if (aaKey->uiv == uiv) {
                absAddrSetAppendAbsAddr(keySet, aaKey);
            }
            aaItem = aaItem->next;
        }
    }
    return keySet;
}


/**
 * Get a set of abstract addresses that are used as keys into the given
 * abstract address map when they have a prefix of the given uiv.
 */
abs_addr_set_t *
getKeyAbsAddrSetFromUivPrefix(uiv_t *uivPrefix, method_info_t *info) {
    uiv_t *baseUiv = getBaseUiv(uivPrefix);
    abs_addr_set_t *keySet = newAbstractAddressSetEmpty();
    XanList *aaList = smallHashTableLookup(info->uivToMemAbsAddrKeysMap, baseUiv);
    if (aaList) {
        XanListItem *aaItem = xanList_first(aaList);
        while (aaItem) {
            abstract_addr_t *aaKey = aaItem->data;
            if (isUivAPrefix(uivPrefix, aaKey->uiv)) {
                absAddrSetAppendAbsAddr(keySet, aaKey);
            }
            aaItem = aaItem->next;
        }
    }
    return keySet;
}


/**
 * Check whether a uiv is a merge target for this method.
 */
static JITBOOLEAN
isUivAMergeTarget(uiv_t *uiv, method_info_t *info) {
    return smallHashTableContains(info->uivMergeTargets, uiv);
}


/**
 * Get a set of uivs that are merge targets and constituents of another uiv.
 */
static VSet *
getConstituentUivMergeTargets(uiv_t *uiv, method_info_t *info) {
    VSet *constituentSet = getConstituentUivSet(uiv);
    VSetItem *uivItem = vSetFirst(constituentSet);
    while (uivItem) {
        VSetItem *nextItem = vSetNext(constituentSet, uivItem);
        uiv_t *consUiv = getSetItemData(uivItem);
        if (!isUivAMergeTarget(consUiv, info)) {
            vSetRemoveItem(constituentSet, uivItem);
        }
        uivItem = nextItem;
    }
    return constituentSet;
}

/**
 * Get a set of abstract addresses that are keys into memory based on merges
 * that could have led to the given abstract address.  In other words, the uiv
 * that makes up the abstract address, and its components if it is a field uiv,
 * could have been the result of a merge.  If that is the case, we need to
 * build a set of abstract addresses that are keys into memory and could have
 * been constructed had that merge not taken place.
 */
abs_addr_set_t *
getMemKeyAbsAddrSetFromMergedUivKey(uiv_t *uiv, method_info_t *info) {
    abs_addr_set_t *keySet;
    VSet *constituentSet;
    VSetItem *uivItem;

    /* The final set to return. */
    keySet = newAbstractAddressSetEmpty();

    /* The uivs that have been merged to. */
    constituentSet = getConstituentUivMergeTargets(uiv, info);

    /* Get keys for each uiv. */
    uivItem = vSetFirst(constituentSet);
    while (uivItem) {
        uiv_t *consUiv = getSetItemData(uivItem);
        abs_addr_set_t *memKeys = getKeyAbsAddrSetFromUivPrefix(consUiv, info);
        absAddrSetAppendSet(keySet, memKeys);
        freeAbstractAddressSet(memKeys);
        uivItem = vSetNext(constituentSet, uivItem);
    }

    /* Clean up. */
    freeSet(constituentSet);
    return keySet;
}


/**
 * Add concrete values to a single set.
 */
void
addConcreteValuesToAbsAddrSet(abs_addr_set_t *aaSet, method_info_t *info, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    JITUINT32 i, num;
    abstract_addr_t **aaArray = absAddrSetToArray(aaSet, &num);
    for (i = 0; i < num; ++i) {
        abstract_addr_t *aaPtr = aaArray[i];
        XanList *keyList = smallHashTableLookup(concreteUivToKeyMap, getBaseUiv(aaPtr->uiv));

        /* No point continuing if there's no concrete values. */
        if (keyList) {
            VSet *constituentUivMergeTargets = getConstituentUivMergeTargets(aaPtr->uiv, info);
            VSetItem *uivMergeItem = vSetFirst(constituentUivMergeTargets);

            /* Unfortunately we have to do this for each uiv. */
            while (uivMergeItem) {
                uiv_t *targetUiv = getSetItemData(uivMergeItem);
                XanListItem *aaItem = xanList_first(keyList);
                while (aaItem) {
                    abstract_addr_t *aaKey = aaItem->data;
                    if (aaKey->uiv == targetUiv || isUivAPrefix(targetUiv, aaKey->uiv)) {
                        abs_addr_set_t *concreteSet = smallHashTableLookup(concreteValues, aaKey);
                        absAddrSetAppendSet(aaSet, concreteSet);
                    }
                    aaItem = aaItem->next;
                }
                uivMergeItem = vSetNext(constituentUivMergeTargets, uivMergeItem);
            }
            freeSet(constituentUivMergeTargets);

            /* Check this abstract address even if not a merge target. */
            abs_addr_set_t *concreteSet = smallHashTableLookup(concreteValues, aaPtr);
            if (concreteSet) {
                absAddrSetAppendSet(aaSet, concreteSet);
            }
        }
    }

    /* Clean up. */
    vllpaFreeFunction(aaArray, num * sizeof(abstract_addr_t *));
}


/**
 * Check whether the given set of abstract addresses would merge into any
 * of the target sets of memory abstract addresses that have a key
 * containing the given uiv.  If the set would be merged into any of the
 * current sets in memory then it may not be added in a later stage, which
 * can avoid problems of continually adding mappings and never terminating.
 * Returns JITTRUE if the given set would merge into any of the existing
 * sets.
 */
JITBOOLEAN
doesAbsAddrSetMergeIntoMemSet(uiv_t *uivKey, abs_addr_set_t *aaSet, method_info_t *info) {
    uiv_t *baseUiv = getBaseUiv(uivKey);
    XanList *aaList = smallHashTableLookup(info->uivToMemAbsAddrKeysMap, baseUiv);
    if (aaList) {
        XanListItem *aaItem = xanList_first(aaList);
        while (aaItem) {
            abstract_addr_t *aaKey = aaItem->data;
            if (aaKey->uiv == uivKey) {
                abs_addr_set_t *memSet = getMemAbstractAddressSet(aaKey, info, JITFALSE);
                assert(memSet);

                /* Check whether the new set would merge into the current. */
                if (abstractAddressSetContainsSet(memSet, aaSet)) {
                    return JITTRUE;
                }
            }
            aaItem = aaItem->next;
        }
    }

    /* No merge found. */
    return JITFALSE;
}
