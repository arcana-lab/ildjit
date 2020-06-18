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
 * This file implements the alias detection for the VLLPA algorithm from the
 * following paper:
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
#include "smalllib.h"
#include "vllpa_aliases.h"
#include "vllpa_info.h"
#include "vllpa_macros.h"
#include "vllpa_memory.h"
#include "vllpa_types.h"
#include "vllpa_util.h"
#include "vset.h"
// End


/**
 * Set outside instructions in dependences or not.
 */
extern const JITBOOLEAN computeOutsideInsts;


/**
 * Use type infos information to exclude dependences.
 */
extern JITBOOLEAN useTypeInfos;


/**
 * Statistics about memory data dependences identified.  All pairs and pairs
 * between unique instructions.
 */
extern JITUINT32 memoryDataDependencesAll;
extern JITUINT32 memoryDataDependencesInst;


/**
 * Parameters read and written by each instruction that can access memory, and
 * their abstract address sets of read and write locations.
 */
typedef struct {
    ir_item_t *readParams[3];
    ir_item_t *writeParam;
    abs_addr_set_t *readSets[3];
    abs_addr_set_t *writeSet;
} read_write_loc_t;


/**
 * Determine whether there is an abstract address based on a variable
 * within the given abstract address set.
 */
static JITBOOLEAN
absAddrSetContainsVarAbsAddr(abs_addr_set_t *aaSet) {
    abstract_addr_t *aa = absAddrSetFirst(aaSet);
    while (aa) {
        if (aa->uiv->type == UIV_VAR && aa->offset == 0) {
            return JITTRUE;
        }
        aa = absAddrSetNext(aaSet, aa);
    }
    return JITFALSE;
}


/**
 * Get a set of abstract addresses for a variable, if it is escaped.
 */
static abs_addr_set_t *
getMergedVarAbsAddrSetIfEscaped(IR_ITEM_VALUE var, method_info_t *info) {
    if (var != NOPARAM && IRMETHOD_isAnEscapedVariable(info->ssaMethod, var)) {
        abs_addr_set_t *mergedVarSet = absAddrSetClone(getVarAbstractAddressSetVarKey(var, info, JITFALSE));
        applyGenericMergeMapToAbstractAddressSet(mergedVarSet, info->mergeAbsAddrMap);
        if (absAddrSetIsEmpty(mergedVarSet)) {
            uiv_t *uiv = newUivFromVar(info->ssaMethod, var, info->uivs);
            abstract_addr_t *aa = newAbstractAddress(uiv, 0);
            absAddrSetAppendAbsAddr(mergedVarSet, aa);
        }
        return mergedVarSet;
    }
    return NULL;
}


/**
 * Get the set of abstract addresses that are written by a generic instruction.
 */
static abs_addr_set_t *
getGenericInstWriteSet(ir_instruction_t *inst, method_info_t *info) {
    IR_ITEM_VALUE defVar = IRMETHOD_getVariableDefinedByInstruction (info->ssaMethod, inst);
    /* PDEBUG("    Generic inst %u write set is: ", inst->ID); */
    if (defVar != NOPARAM && IRMETHOD_isAnEscapedVariable(info->ssaMethod, defVar)) {
        uiv_t *uiv = newUivFromVar(info->ssaMethod, defVar, info->uivs);
        abs_addr_set_t *aaSet = newAbstractAddressInNewSet(uiv, 0);
        /* printAbstractAddressSet(aaSet); */
        return aaSet;
    }
    /* PDEBUGB("NULL\n"); */
    return NULL;
}


/**
 * Get a set of abstract addresses accessed through a variable parameter.
 */
static abs_addr_set_t *
getVarParamAbsAddrSet(ir_item_t *param, method_info_t *info) {
    abs_addr_set_t *mergedSet;
    assert(param->type == IROFFSET);
    mergedSet = absAddrSetClone(getVarAbstractAddressSet(param, info, JITFALSE));
    applyGenericMergeMapToAbstractAddressSet(mergedSet, info->mergeAbsAddrMap);
    return mergedSet;
}


/**
 * Get the set of abstract addresses accessed by a load or store.
 */
static abs_addr_set_t *
getLoadStoreAbsAddrSet(ir_instruction_t *inst, method_info_t *info) {
    abs_addr_set_t *aaSet = getLoadStoreAccessedAbsAddrSet(inst, info);
    applyGenericMergeMapToAbstractAddressSet(aaSet, info->mergeAbsAddrMap);
    return aaSet;
}


/**
 * Get the set of abstract addresses accessed through a parameter, numbered
 * starting at 1.  Includes all abstract addresses based on these abstract
 * addresses, given by the parameter as position 'lenNum'.
 */
static abs_addr_set_t *
getParameterAccessedAbsAddrSet(ir_instruction_t *inst, JITUINT32 baseNum, method_info_t *info) {
    ir_item_t *param = IRMETHOD_getInstructionParameter(inst, baseNum);
    abs_addr_set_t *aaSet = getVarParamAbsAddrSet(param, info);
    return aaSet;
}


/**
 * Determine whether the type_infos fields of the parameters containing memory
 * addresses for two instructions are NULL or assignable, according to the type
 * of dependence being checked.  If so, return JITTRUE (meaning they are
 * assignable or it's not known, since one or both is NULL).  Otherwise return
 * JITFALSE (meaning they are definitely not assignable).
 */
static JITBOOLEAN
typeInfosFieldsMayBeAssignable(ir_item_t *fromParam, ir_item_t *toParam) {
    if (!useTypeInfos) {
        return JITTRUE;
    }
    if (fromParam->type_infos == NULL || toParam->type_infos == NULL) {
        return JITTRUE;
    }
    return IRDATA_isAssignable(fromParam->type_infos, toParam->type_infos);
}


/**
 * Record data dependences between two instructions if their read and / or
 * write sets overlap.
 */
static void
recordAbsAddrSetDataDependences(ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst, read_write_loc_t *fromLocs, read_write_loc_t *toLocs, JITBOOLEAN checkMerges, aaset_prefix_t checkPrefixType) {
    JITBOOLEAN added = JITFALSE;
    JITUINT32 p;

    /* Memory RAW dependence. */
    if (toLocs->writeSet) {
        for (p = 0; p < 3; ++p) {
            if (fromLocs->readSets[p] &&
                    typeInfosFieldsMayBeAssignable(fromLocs->readParams[p], toLocs->writeParam) &&
                    abstractAddressSetsOverlap(fromLocs->readSets[p], toLocs->writeSet, checkMerges, checkPrefixType)) {
                IRMETHOD_addInstructionDataDependence(method, fromInst, toInst, DEP_MRAW, NULL);
                IRMETHOD_addInstructionDataDependence(method, toInst, fromInst, DEP_MWAR, NULL);
                PDEBUG("Dependence MRAW %u -> %u\n", fromInst->ID, toInst->ID);
                PDEBUG("Dependence MWAR %u -> %u\n", toInst->ID, fromInst->ID);
                memoryDataDependencesAll +=1;
                if (!added) {
                    memoryDataDependencesInst += 1;
                }
                added = JITTRUE;
                break;
            }
        }
    }

    /* Memory WA* dependences. */
    if (fromLocs->writeSet) {
        for (p = 0; p < 3; ++p) {
            if (toLocs->readSets[p] &&
                    typeInfosFieldsMayBeAssignable(fromLocs->writeParam, toLocs->readParams[p]) &&
                    abstractAddressSetsOverlap(fromLocs->writeSet, toLocs->readSets[p], checkMerges, checkPrefixType)) {
                IRMETHOD_addInstructionDataDependence(method, fromInst, toInst, DEP_MWAR, NULL);
                IRMETHOD_addInstructionDataDependence(method, toInst, fromInst, DEP_MRAW, NULL);
                PDEBUG("Dependence MWAR %u -> %u\n", fromInst->ID, toInst->ID);
                PDEBUG("Dependence MRAW %u -> %u\n", toInst->ID, fromInst->ID);
                memoryDataDependencesAll +=1;
                if (!added) {
                    memoryDataDependencesInst += 1;
                }
                added = JITTRUE;
                break;
            }
        }
        if (toLocs->writeSet &&
                typeInfosFieldsMayBeAssignable(fromLocs->writeParam, toLocs->writeParam) &&
                abstractAddressSetsOverlap(fromLocs->writeSet, toLocs->writeSet, checkMerges, checkPrefixType)) {
            IRMETHOD_addInstructionDataDependence(method, fromInst, toInst, DEP_MWAW, NULL);
            IRMETHOD_addInstructionDataDependence(method, toInst, fromInst, DEP_MWAW, NULL);
            PDEBUG("Dependence MWAW %u -> %u\n", fromInst->ID, toInst->ID);
            PDEBUG("Dependence MWAW %u -> %u\n", toInst->ID, fromInst->ID);
            memoryDataDependencesAll +=1;
            if (!added) {
                memoryDataDependencesInst += 1;
            }
        }
    }
}


/**
 * Check whether there is a library call within the call tree rooted at a
 * single call site.
 */
static JITBOOLEAN
callTreeAtCallSiteContainsLibraryCall(call_site_t *callSite, SmallHashTable *methodsInfo) {
    JITBOOLEAN containsLibraryCall = (callSite->callType == LIBRARY_CALL);
    if (callSite->callType == NORMAL_CALL) {
        method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
        assert(calleeInfo);
        containsLibraryCall |= calleeInfo->containsLibraryCall;
    }
    return containsLibraryCall;
}


/**
 * Check whether the call tree rooted at the given instruction could contain
 * a call to a library somewhere within it.
 */
static JITBOOLEAN
callTreeContainsLibraryCall(ir_instruction_t *inst, method_info_t *info, SmallHashTable *methodsInfo) {
    void *callTarget;
    JITBOOLEAN containsLibraryCall;

    /* Initialize variables.			*/
    containsLibraryCall = JITFALSE;

    /* Check for a library call somewhere in the tree. */
    callTarget = smallHashTableLookup(info->callMethodMap, inst);
    if (inst->type == IRCALL) {
        containsLibraryCall |= callTreeAtCallSiteContainsLibraryCall(callTarget, methodsInfo);
    } else {
        XanList *callSiteList = (XanList *)callTarget;
        XanListItem *callSiteItem = xanList_first(callSiteList);
        while ((callSiteItem != NULL) && (!containsLibraryCall)) {
            call_site_t *callSite = callSiteItem->data;
            containsLibraryCall |= callTreeAtCallSiteContainsLibraryCall(callSite, methodsInfo);
            callSiteItem = callSiteItem->next;
        }
    }

    /* Return the result. */
    return containsLibraryCall;
}


/**
 * Check whether the given call instruction is a call to a special, known
 * library method.  These are methods whose semantics (and therefore the
 * memory locations read and written) are understood.
 */
static JITBOOLEAN
instructionCallsKnownLibrary(ir_instruction_t *inst, method_info_t *info) {
    void *callTarget;
    call_site_t *callSite;

    /* Must be a single target method. */
    callTarget = smallHashTableLookup(info->callMethodMap, inst);
    if (inst->type == IRCALL) {
        callSite = callTarget;
    } else {
        XanList *callSiteList = (XanList *)callTarget;
        XanListItem *callSiteItem;
        if (xanList_length(callSiteList) != 1) {
            return JITFALSE;
        }
        callSiteItem = xanList_first(callSiteList);
        callSite = callSiteItem->data;
    }

    /* Check this method. */
    switch (callSite->callType) {
        case NORMAL_CALL:
        case LIBRARY_CALL:
            return JITFALSE;
        default:
            return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Get the original instruction counterpart to the given instruction and check
 * the type is ok if the original instruction doesn't exist.  Branches, nops
 * and labels don't get assigned counterpart instructions.  Extra adds and
 * stores can be created to aid analysis of the SSA method.  Phi instructions
 * don't have counterparts in the first place.
 */
static ir_instruction_t *
getOriginalInst(SmallHashTable *instMap, ir_instruction_t *inst) {
    ir_instruction_t *orig = smallHashTableLookup(instMap, inst);
    if (!orig) {
        assert(inst->type == IRBRANCH || inst->type == IRNOP || inst->type == IRLABEL || inst->type == IRADD || inst->type == IRMOVE || inst->type == IRPHI);
    }
    return orig;
}


/**
 * Compute variable data dependences between two instructions for the
 * variables used as abstract addresses in the given set that are defined
 * by the first instruction.
 **/
static void
computeVarDefDepsFromAbsAddrSet(ir_method_t *method, ir_method_t *origMethod, ir_instruction_t *defInst, ir_instruction_t *origDefInst, ir_instruction_t *otherInst, ir_instruction_t *origOtherInst, abs_addr_set_t *aaSet) {
    abstract_addr_t *aa = absAddrSetFirst(aaSet);
    /* Until iljit can be informed about the variable that the dependence
       relates to, this is disabled and we'd better hope that these types of
       dependence are rare. */
    while (aa) {
        if (aa->uiv->type == UIV_VAR && aa->offset == 0) {
            JITUINT32 var = getVarFromBaseUiv(aa->uiv);
            JITUINT32 block = var / (sizeof(JITNUINT) * 8);
            JITUINT32 offset = var % (sizeof(JITNUINT) * 8);
            if (otherInst->type != IRGETADDRESS && ((livenessGetUse(method, otherInst, block) >> offset) & 0x1)) {
                DDG_addPossibleDependence(origMethod, origDefInst, origOtherInst, DEP_WAR);
                DDG_addPossibleDependence(origMethod, origOtherInst, origDefInst, DEP_RAW);
            }
            if (livenessGetDef(method, otherInst) == var) {
                DDG_addPossibleDependence(origMethod, origDefInst, origOtherInst, DEP_WAW);
                DDG_addPossibleDependence(origMethod, origOtherInst, origDefInst, DEP_WAW);
            }
        }
        aa = absAddrSetNext(aaSet, aa);
    }
}


/**
 * Compute variable data dependences between two instructions for the
 * variables used as abstract addresses in the given set that are used by
 * the first instruction.
 */
static void
computeVarUseDepsFromAbsAddrSet(ir_method_t *method, ir_method_t *origMethod, ir_instruction_t *useInst, ir_instruction_t *origUseInst, ir_instruction_t *otherInst, ir_instruction_t *origOtherInst, abs_addr_set_t *aaSet) {
    abstract_addr_t *aa = absAddrSetFirst(aaSet);
    /* Until iljit can be informed about the variable that the dependence
       relates to, this is disabled and we'd better hope that these types of
       dependence are rare. */
    return;
    while (aa) {
        if (aa->uiv->type == UIV_VAR && aa->offset == 0) {
            if (livenessGetDef(method, otherInst) == getVarFromBaseUiv(aa->uiv)) {
                /*         PDEBUG("Dependence between %u and %u\n", origUseInst->ID, origOtherInst->ID); */
                DDG_addPossibleDependence(origMethod, origUseInst, origOtherInst, DEP_RAW);
                DDG_addPossibleDependence(origMethod, origOtherInst, origUseInst, DEP_WAR);
                /* IRMETHOD_addInstructionDataDependence(origMethod, origUseInst, origOtherInst, DEP_RAW, NULL); */
                /* IRMETHOD_addInstructionDataDependence(origMethod, origOtherInst, origUseInst, DEP_WAR, NULL); */
            }
        }
        aa = absAddrSetNext(aaSet, aa);
    }
}


/**
 * Print out read and write sets.
 */
#ifdef PRINTDEBUG
static void
printReadWriteSets(read_write_loc_t *readWriteLoc, char *prefix) {
    JITUINT32 p;
    for (p = 0; p < 3; ++p) {
        if (readWriteLoc->readSets[p]) {
            PDEBUG("%sRead set: ", prefix);
            printAbstractAddressSet(readWriteLoc->readSets[p]);
        }
    }
    if (readWriteLoc->writeSet) {
        PDEBUG("%sWrite set: ", prefix);
        printAbstractAddressSet(readWriteLoc->writeSet);
    }
}
#else
#define printReadWriteSets(readWriteLoc, prefix)
#endif  /* ifdef PRINTDEBUG */


/**
 * Compute memory dependences between the given non-call instruction and other
 * (later) non-call instructions within the same method.
 */
static void
computeNonCallMemoryDependences(ir_instruction_t *inst, ir_method_t *origMethod, read_write_loc_t **readWriteLocs, method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i, p, numInsts;
    ir_instruction_t *origInst = getOriginalInst(info->instMap, inst);
    JITBOOLEAN varInReadSet = JITFALSE;
    JITBOOLEAN varInWriteSet = JITFALSE;
    JITBOOLEAN instLoadStore = JITFALSE;
    JITBOOLEAN instFieldAccess = JITFALSE;
    aaset_prefix_t checkPrefixType = AASET_PREFIX_NONE;

    /* Sometimes moves added by the ssa conversion can arrive here, so ignore those. */
    if (!origInst) {
        assert(inst->type == IRMOVE);
        assert(IRMETHOD_getBasicBlockContainingInstruction(((method_info_t *)smallHashTableLookup(methodsInfo, origMethod))->ssaMethod, inst)->pos == 0);
        assert(inst->ID >= IRMETHOD_getMethodParametersNumber(origMethod));
        return;
    }

    /* Check whether there's a variable in any input set. */
    assert(readWriteLocs[inst->ID] != NULL);
    for (p = 0; p < 3; ++p) {
        if (readWriteLocs[inst->ID]->readSets[p]) {
            varInReadSet |= absAddrSetContainsVarAbsAddr(readWriteLocs[inst->ID]->readSets[p]);
        }
    }
    if (readWriteLocs[inst->ID]->writeSet) {
        varInWriteSet = absAddrSetContainsVarAbsAddr(readWriteLocs[inst->ID]->writeSet);
    }
    printReadWriteSets(readWriteLocs[inst->ID], "  ");

    /* Field accesses and prefix checking, only relevant to certain instructions. */
    switch (inst->type) {
        case IRLOADREL:
        case IRSTOREREL:
            instLoadStore = JITTRUE;
            instFieldAccess = vSetContains(info->fieldInsts, origInst);
            PDEBUG("  Is %sa field access instruction\n", instFieldAccess ? "" : "not ");
            break;
        case IRINITMEMORY:
        case IRFREEOBJ:
        case IRFREE:
            checkPrefixType = AASET_PREFIX_FIRST;
            PDEBUG("  Need to check abstract address prefixes\n");
            break;
    }

    /* Consider each later instruction. */
    numInsts = IRMETHOD_getInstructionsNumber(info->ssaMethod);
    for (i = inst->ID; i < numInsts; ++i) {
        ir_instruction_t *other = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_instruction_t *origOther = getOriginalInst(info->instMap, other);
        if (!origOther) {
            continue;
        }

        /* Compute dependences. */
        switch (other->type) {
            case IRLOADREL:
                PDEBUG("  Compare to load inst %u (orig %u)\n", other->ID, origOther->ID);
                printReadWriteSets(readWriteLocs[other->ID], "    ");

                /* /\** */
                /*  * Need to add support for loading multiple memory addresses based on the */
                /*  * length of the destination type.  Otherwise this could miss addresses. */
                /*  *\/ */
                /* abort(); */
                if (instLoadStore) {
                    JITBOOLEAN otherFieldAccess = vSetContains(info->fieldInsts, origOther);
                    PDEBUG("    Is %sa field access instruction\n", otherFieldAccess ? "" : "not ");
                    recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], !instFieldAccess && !otherFieldAccess, checkPrefixType);
                } else {
                    recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], JITTRUE, checkPrefixType);
                }
                break;

            case IRSTOREREL:
                PDEBUG("  Compare to store inst %u (orig %u)\n", other->ID, origOther->ID);
                printReadWriteSets(readWriteLocs[other->ID], "    ");
                if (instLoadStore) {
                    JITBOOLEAN otherFieldAccess = vSetContains(info->fieldInsts, origOther);
                    PDEBUG("    Is %sa field access instruction\n", otherFieldAccess ? "" : "not ");
                    recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], !instFieldAccess && !otherFieldAccess, checkPrefixType);
                } else {
                    recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], JITTRUE, checkPrefixType);
                }
                break;

            case IRMEMCPY:
            case IRMEMCMP:
            case IRSTRINGCMP:
            case IRSTRINGLENGTH:
            case IRSTRINGCHR:
                PDEBUG("  Compare to inst %u (orig %u)\n", other->ID, origOther->ID);
                printReadWriteSets(readWriteLocs[other->ID], "    ");
                recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], JITTRUE, checkPrefixType);
                break;

            case IRINITMEMORY:
            case IRFREEOBJ:
            case IRFREE: {
                aaset_prefix_t combinedCheckPrefixType;
                if (checkPrefixType == AASET_PREFIX_NONE) {
                    combinedCheckPrefixType = AASET_PREFIX_SECOND;
                } else {
                    assert(checkPrefixType == AASET_PREFIX_FIRST);
                    combinedCheckPrefixType = AASET_PREFIX_BOTH;
                }
                PDEBUG("  Compare to inst %u (orig %u)\n", other->ID, origOther->ID);
                PDEBUG("  Checking abstract address prefixes for %s\n", combinedCheckPrefixType == AASET_PREFIX_SECOND ? "the second set" : "both sets");
                printReadWriteSets(readWriteLocs[other->ID], "    ");
                recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], JITTRUE, combinedCheckPrefixType);
                break;
            }

            case IRMOVE:
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IRNEG:
            case IROR:
            case IRNOT:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRCONV:
            case IRCONVOVF:
            case IRBITCAST:
            case IRLT:
            case IRGT:
            case IREQ:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRCOSH:
            case IRCEIL:
            case IRSIN:
            case IRCOS:
            case IRACOS:
            case IRSQRT:
            case IRFLOOR:
            case IRPOW:
            case IREXP:
            case IRLOG10:
                if (readWriteLocs[other->ID] != NULL) {
                    PDEBUG("  Compare to inst %u (orig %u)\n", other->ID, origOther->ID);
                    printReadWriteSets(readWriteLocs[other->ID], "    ");
                    recordAbsAddrSetDataDependences(origMethod, origInst, origOther, readWriteLocs[inst->ID], readWriteLocs[other->ID], JITTRUE, checkPrefixType);
                }
                break;
        }

        /* Add variable dependences. */
        if (origOther) {
            if (varInReadSet) {
                for (p = 0; p < 3; ++p) {
                    if (readWriteLocs[inst->ID]->readSets[p]) {
                        computeVarUseDepsFromAbsAddrSet(info->ssaMethod, origMethod, inst, origInst, other, origOther, readWriteLocs[inst->ID]->readSets[p]);
                    }
                }
            }
            if (varInWriteSet) {
                computeVarDefDepsFromAbsAddrSet(info->ssaMethod, origMethod, inst, origInst, other, origOther, readWriteLocs[inst->ID]->writeSet);
            }
        }
    }
}


/**
 * Compute memory dependences between the given library or native call
 * instruction and all other instructions within the same method.
 */
static void
computeLibraryMemoryDependences(ir_instruction_t *inst, ir_method_t *origMethod, method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i, numInsts;
    ir_instruction_t *origInst = getOriginalInst(info->instMap, inst);

    /* Look at all instructions, since this library call is only dealt with here. */
    numInsts = IRMETHOD_getInstructionsNumber(info->ssaMethod);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *other = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_instruction_t *origOther = getOriginalInst(info->instMap, other);
        if (!origOther) {
            continue;
        }

        /* Select by instruction type. */
        switch (other->type) {
            case IRLOADREL:
            case IRMEMCMP:
            case IRSTRINGCMP:
            case IRSTRINGLENGTH:
            case IRSTRINGCHR:
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAR, NULL);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW, NULL);
                PDEBUG("Dependence MWAR %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MRAW %u -> %u\n", origOther->ID, origInst->ID);
                memoryDataDependencesAll += 1;
                memoryDataDependencesInst += 1;
                break;

            case IRSTOREREL:
            case IRINITMEMORY:
            case IRFREEOBJ:
            case IRFREE:
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW | DEP_MWAW, NULL);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAR | DEP_MWAW, NULL);
                PDEBUG("Dependence MRAW %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MWAW %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MWAR %u -> %u\n", origOther->ID, origInst->ID);
                PDEBUG("Dependence MWAW %u -> %u\n", origOther->ID, origInst->ID);
                memoryDataDependencesAll += 2;
                memoryDataDependencesInst += 1;
                break;

            case IRMEMCPY:
            case IRLIBRARYCALL:
            case IRNATIVECALL:
            case IRCALL:
            case IRICALL:
            case IRVCALL:
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW | DEP_MWAR | DEP_MWAW, NULL);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW | DEP_MWAR | DEP_MWAW, NULL);
                PDEBUG("Dependence MRAW %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MWAR %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MWAW %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MRAW %u -> %u\n", origOther->ID, origInst->ID);
                PDEBUG("Dependence MWAR %u -> %u\n", origOther->ID, origInst->ID);
                PDEBUG("Dependence MWAW %u -> %u\n", origOther->ID, origInst->ID);
                memoryDataDependencesAll += 3;
                memoryDataDependencesInst += 1;
                break;

            case IRMOVE:
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IRNEG:
            case IROR:
            case IRNOT:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRCONV:
            case IRCONVOVF:
            case IRBITCAST:
            case IRLT:
            case IRGT:
            case IREQ:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRCOSH:
            case IRCEIL:
            case IRSIN:
            case IRCOS:
            case IRACOS:
            case IRSQRT:
            case IRFLOOR:
            case IRPOW:
            case IREXP:
            case IRLOG10:
                if (IRMETHOD_mayInstructionAccessHeapMemory(info->ssaMethod, other)) {
                    IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW | DEP_MWAR | DEP_MWAW, NULL);
                    IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW | DEP_MWAR | DEP_MWAW, NULL);
                    PDEBUG("Dependence MRAW %u -> %u\n", origInst->ID, origOther->ID);
                    PDEBUG("Dependence MWAR %u -> %u\n", origInst->ID, origOther->ID);
                    PDEBUG("Dependence MWAW %u -> %u\n", origInst->ID, origOther->ID);
                    PDEBUG("Dependence MRAW %u -> %u\n", origOther->ID, origInst->ID);
                    PDEBUG("Dependence MWAR %u -> %u\n", origOther->ID, origInst->ID);
                    PDEBUG("Dependence MWAW %u -> %u\n", origOther->ID, origInst->ID);
                    memoryDataDependencesAll += 3;
                    memoryDataDependencesInst += 1;
                }
                break;
        }
    }
}


/**
 * Compute the list of instructions from all callee methods that cause an
 * overlap between the given sets of read or write abstract addresses from
 * the caller and callee.
 */
static XanList *
computeCalleeInstsCausingDependences(method_info_t *callerInfo, abs_addr_set_t *callerSet, abs_addr_set_t *calleeSet, XanList *callSiteList, SmallHashTable *methodsInfo, JITBOOLEAN readInsts) {
    XanList *calleeInsts = allocateNewList();

    /* Find overlapping abstract addresses. */
    if (abstractAddressSetsOverlap(callerSet, calleeSet, JITTRUE, AASET_PREFIX_NONE)) {
        abs_addr_set_t *overlaps = getOverlapSetFromAbstractAddressSets(callerSet, calleeSet);
        XanListItem *callSiteItem;

        /* Get the callee instructions that read or write these locations. */
        assert(!absAddrSetIsEmpty(overlaps));
        callSiteItem = xanList_first(callSiteList);
        while (callSiteItem) {
            call_site_t *callSite = callSiteItem->data;

            /* Ignore library calls. */
            if (callSite->callType == NORMAL_CALL) {
                method_info_t *calleeInfo = smallHashTableLookup(methodsInfo, callSite->method);
                SmallHashTable *calleeUivMap = smallHashTableLookup(callerInfo->calleeUivMaps, callSite->method);
                abs_addr_set_t *calleeUseSet;
                SmallHashTable *calleeInstMap;
                abstract_addr_t *aaCallee;

                /* Select the right set. */
                if (readInsts) {
                    calleeUseSet = calleeInfo->readSet;
                    calleeInstMap = calleeInfo->readInsts;
                } else {
                    calleeUseSet = calleeInfo->writeSet;
                    calleeInstMap = calleeInfo->writeInsts;
                }

                /* Check each callee abstract address in the use set. */
                aaCallee = absAddrSetFirst(calleeUseSet);
                while (aaCallee) {
                    abs_addr_set_t *callerUseSet;

                    /* TODO: Cache this and deal with the map cache. */
                    abort();

                    /* Get the caller's use set. */
                    callerUseSet = mapCalleeAbsAddrToCallerAbsAddrSet(aaCallee, callerInfo, calleeInfo, calleeUivMap, JITFALSE);
                    applyGenericMergeMapToAbstractAddressSet(callerUseSet, callerInfo->mergeAbsAddrMap);

                    /* Overlaps from instructions using this callee abstract address. */
                    if (abstractAddressSetsOverlap(overlaps, callerUseSet, JITTRUE, AASET_PREFIX_NONE)) {
                        VSet *aaInsts = smallHashTableLookup(calleeInstMap, aaCallee);
                        VSetItem *instItem;
                        assert(aaInsts);
                        instItem = vSetFirst(aaInsts);
                        while (instItem) {
                            ir_instruction_t *inst = getSetItemData(instItem);
                            if (!xanList_find(calleeInsts, inst)) {
                                xanList_append(calleeInsts, inst);
                            }
                            instItem = vSetNext(aaInsts, instItem);
                        }
                    }
                    freeAbstractAddressSet(callerUseSet);
                    aaCallee = absAddrSetNext(calleeUseSet, aaCallee);
                }
            }
            callSiteItem = callSiteItem->next;
        }

        /* Clean up. */
        freeAbstractAddressSet(overlaps);

        /* There was an overlap caused by at least one instruction. */
        assert(xanList_length(calleeInsts) > 0);
    }

    /* Return the list of instructions. */
    return calleeInsts;
}


/**
 * Compute memory dependences between the given instructions.  The first
 * instruction is a call instruction, possibly with known semantics (indicated
 * by the 'knownCalls' argument.  However, although it's obvious what is the
 * set of abstract addresses that could be directly read and written by the
 * call, other abstract addresses that derive from these could also be read
 * or written.  For example, in the case of fseek(), the first argument is a
 * file pointer, which is a structure containing information about the file
 * stream.  The fields in this structure are manipulated by the library call,
 * but we don't exactly know which ones.  Therefore, to be conservative, we
 * need to not only look for overlaps between the abstract address sets
 * encountered, but also the overlaps that occur when the library call abstract
 * addresses are the prefix of other abstract addresses.  Otherwise an
 * instruction that also manipulates these fields could be falsely identified
 * as not having a dependence.  For normal calls we don't need to worry about
 * doing this analysis.
 */
static void
computeCallAndInstMemoryDependences(ir_instruction_t *origInst, ir_instruction_t *origOther, ir_method_t *origMethod, abs_addr_set_t *instReadSet, abs_addr_set_t *instWriteSet, read_write_loc_t *otherLocs, XanList *callSiteList, aaset_prefix_t checkPrefixType, method_info_t *info, SmallHashTable *methodsInfo) {
    JITBOOLEAN added = JITFALSE;
    JITUINT32 p;

    /* Compute memory read dependences. */
    assert(instReadSet);
    assert(instWriteSet);
    if (otherLocs->writeSet) {
        if (computeOutsideInsts) {
            XanList *calleeReadInsts = computeCalleeInstsCausingDependences(info, otherLocs->writeSet, instReadSet, callSiteList, methodsInfo, JITTRUE);
            if (xanList_length(calleeReadInsts) > 0) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW, calleeReadInsts);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAR, NULL);
                memoryDataDependencesAll += 1;
                memoryDataDependencesInst += 1;
                added = JITTRUE;
            }
            freeList(calleeReadInsts);
        } else {
            if (abstractAddressSetsOverlap(instReadSet, otherLocs->writeSet, JITTRUE, checkPrefixType)) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW, NULL);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAR, NULL);
                PDEBUG("Dependence MRAW %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MWAR %u -> %u\n", origOther->ID, origInst->ID);
                memoryDataDependencesAll += 1;
                memoryDataDependencesInst += 1;
                added = JITTRUE;
            }
        }
    }

    /* Compute memory write dependences. */
    if (computeOutsideInsts) {
        for (p = 0; p < 3; ++p) {
            if (otherLocs->readSets[p]) {
                XanList *calleeWriteInsts = computeCalleeInstsCausingDependences(info, otherLocs->readSets[p], instWriteSet, callSiteList, methodsInfo, JITFALSE);
                if (xanList_length(calleeWriteInsts) > 0) {
                    IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAR, calleeWriteInsts);
                    IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW, NULL);
                    memoryDataDependencesAll += 1;
                    if (!added) {
                        memoryDataDependencesInst += 1;
                    }
                    added = JITTRUE;
                    freeList(calleeWriteInsts);
                    break;
                }
                freeList(calleeWriteInsts);
            }
        }
        if (otherLocs->writeSet) {
            XanList *calleeWriteInsts = computeCalleeInstsCausingDependences(info, otherLocs->writeSet, instWriteSet, callSiteList, methodsInfo, JITFALSE);
            if (xanList_length(calleeWriteInsts) > 0) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAW, calleeWriteInsts);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAW, NULL);
                memoryDataDependencesAll += 1;
                if (!added) {
                    memoryDataDependencesInst += 1;
                }
            }
            freeList(calleeWriteInsts);
        }
    } else {
        for (p = 0; p < 3; ++p) {
            if (otherLocs->readSets[p] && abstractAddressSetsOverlap(instWriteSet, otherLocs->readSets[p], JITTRUE, checkPrefixType)) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAR, NULL);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW, NULL);
                PDEBUG("Dependence MWAR %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MRAW %u -> %u\n", origOther->ID, origInst->ID);
                memoryDataDependencesAll += 1;
                if (!added) {
                    memoryDataDependencesInst += 1;
                }
                added = JITTRUE;
                break;
            }
        }
        if (otherLocs->writeSet && abstractAddressSetsOverlap(instWriteSet, otherLocs->writeSet, JITTRUE, checkPrefixType)) {
            IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAW, NULL);
            IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAW, NULL);
            PDEBUG("Dependence MWAW %u -> %u\n", origInst->ID, origOther->ID);
            PDEBUG("Dependence MWAW %u -> %u\n", origOther->ID, origInst->ID);
            memoryDataDependencesAll += 1;
            if (!added) {
                memoryDataDependencesInst += 1;
            }
        }
    }
}


/**
 * Compute memory dependences between the given instructions which are both
 * calls, possibly with known semantics.
 */
static void
computeCallAndCallMemoryDependences(ir_instruction_t *origInst, ir_instruction_t *origOther, ir_method_t *origMethod, abs_addr_set_t *instReadSet, abs_addr_set_t *instWriteSet, abs_addr_set_t *otherReadSet, abs_addr_set_t *otherWriteSet, XanList *callSiteList, aaset_prefix_t checkPrefixType, method_info_t *info, SmallHashTable *methodsInfo) {
    JITBOOLEAN added = JITFALSE;

    /* Compute memory read dependences. */
    assert(instReadSet);
    assert(instWriteSet);
    if (otherWriteSet) {
        if (computeOutsideInsts) {
            XanList *calleeReadInsts = computeCalleeInstsCausingDependences(info, otherWriteSet, instReadSet, callSiteList, methodsInfo, JITTRUE);
            if (xanList_length(calleeReadInsts) > 0) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW, calleeReadInsts);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAR, NULL);
                memoryDataDependencesAll += 1;
                memoryDataDependencesInst += 1;
                added = JITTRUE;
            }
            freeList(calleeReadInsts);
        } else {
            if (abstractAddressSetsOverlap(instReadSet, otherWriteSet, JITTRUE, checkPrefixType)) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MRAW, NULL);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAR, NULL);
                PDEBUG("Dependence MRAW %u -> %u\n", origInst->ID, origOther->ID);
                PDEBUG("Dependence MWAR %u -> %u\n", origOther->ID, origInst->ID);
                memoryDataDependencesAll += 1;
                memoryDataDependencesInst += 1;
                added = JITTRUE;
            }
        }
    }

    /* Compute memory write dependences. */
    if (computeOutsideInsts) {
        if (otherReadSet) {
            XanList *calleeWriteInsts = computeCalleeInstsCausingDependences(info, otherReadSet, instWriteSet, callSiteList, methodsInfo, JITFALSE);
            if (xanList_length(calleeWriteInsts) > 0) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAR, calleeWriteInsts);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW, NULL);
                memoryDataDependencesAll += 1;
                if (!added) {
                    memoryDataDependencesInst += 1;
                }
                added = JITTRUE;
            }
            freeList(calleeWriteInsts);
        }
        if (otherWriteSet) {
            XanList *calleeWriteInsts = computeCalleeInstsCausingDependences(info, otherWriteSet, instWriteSet, callSiteList, methodsInfo, JITFALSE);
            if (xanList_length(calleeWriteInsts) > 0) {
                IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAW, calleeWriteInsts);
                IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAW, NULL);
                memoryDataDependencesAll += 1;
                if (!added) {
                    memoryDataDependencesInst += 1;
                }
            }
            freeList(calleeWriteInsts);
        }
    } else {
        if (otherReadSet && abstractAddressSetsOverlap(instWriteSet, otherReadSet, JITTRUE, checkPrefixType)) {
            IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAR, NULL);
            IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MRAW, NULL);
            PDEBUG("Dependence MWAR %u -> %u\n", origInst->ID, origOther->ID);
            PDEBUG("Dependence MRAW %u -> %u\n", origOther->ID, origInst->ID);
            memoryDataDependencesAll += 1;
            if (!added) {
                memoryDataDependencesInst += 1;
            }
            added = JITTRUE;
        }
        if (otherWriteSet && abstractAddressSetsOverlap(instWriteSet, otherWriteSet, JITTRUE, checkPrefixType)) {
            IRMETHOD_addInstructionDataDependence(origMethod, origInst, origOther, DEP_MWAW, NULL);
            IRMETHOD_addInstructionDataDependence(origMethod, origOther, origInst, DEP_MWAW, NULL);
            PDEBUG("Dependence MWAW %u -> %u\n", origInst->ID, origOther->ID);
            PDEBUG("Dependence MWAW %u -> %u\n", origOther->ID, origInst->ID);
            memoryDataDependencesAll += 1;
            if (!added) {
                memoryDataDependencesInst += 1;
            }
        }
    }
}


/**
 * Compute memory dependences between the given call instruction and all other
 * instruction types within the same method.
 */
static void
computeCallMemoryDependences(ir_instruction_t *inst, ir_method_t *origMethod, read_write_loc_t **readWriteLocs, method_info_t *info, SmallHashTable *methodsInfo) {
    JITUINT32 i;
    ir_instruction_t *origInst = getOriginalInst(info->instMap, inst);
    XanList *callSiteList = smallHashTableLookup(info->callMethodMap, inst);
    abs_addr_set_t *instReadSet = smallHashTableLookup(info->callReadMap, inst);
    abs_addr_set_t *instWriteSet = smallHashTableLookup(info->callWriteMap, inst);
    JITBOOLEAN knownCall = instructionCallsKnownLibrary(inst, info);
    aaset_prefix_t checkPrefixType;
    PDEBUG("  Is %sa known call\n", knownCall ? "" : "not ");
    if (knownCall) {
        checkPrefixType = AASET_PREFIX_FIRST;
    } else {
        checkPrefixType = AASET_PREFIX_NONE;
    }

    /* Debug printing. */
    PDEBUG("  Read set: ");
    printAbstractAddressSet(instReadSet);
    PDEBUG("  Write set: ");
    printAbstractAddressSet(instWriteSet);

    /* Debugging. */
    assert(instReadSet);
    assert(instWriteSet);

    /* Consider each other instruction. */
    for (i = 0; i< IRMETHOD_getInstructionsNumber(info->ssaMethod); ++i) {
        ir_instruction_t *other = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);
        ir_instruction_t *origOther = getOriginalInst(info->instMap, other);
        if (!origOther) {
            continue;
        }

        /* Dependences depend on type of call. */
        switch (other->type) {
            case IRLOADREL:
            case IRSTOREREL:
            case IRMEMCPY:
            case IRMEMCMP:
            case IRSTRINGCMP:
            case IRSTRINGLENGTH:
            case IRSTRINGCHR:
                PDEBUG("  Compare to inst %u (orig %u)\n", other->ID, origOther->ID);
                printReadWriteSets(readWriteLocs[other->ID], "    ");
                computeCallAndInstMemoryDependences(origInst, origOther, origMethod, instReadSet, instWriteSet, readWriteLocs[other->ID], callSiteList, checkPrefixType, info, methodsInfo);
                break;

            case IRINITMEMORY:
            case IRFREEOBJ:
            case IRFREE: {
                aaset_prefix_t combinedCheckPrefixType;
                if (checkPrefixType == AASET_PREFIX_NONE) {
                    combinedCheckPrefixType = AASET_PREFIX_SECOND;
                } else {
                    assert(checkPrefixType == AASET_PREFIX_FIRST);
                    combinedCheckPrefixType = AASET_PREFIX_BOTH;
                }
                PDEBUG("  Compare to inst %u (orig %u)\n", other->ID, origOther->ID);
                printReadWriteSets(readWriteLocs[other->ID], "    ");
                computeCallAndInstMemoryDependences(origInst, origOther, origMethod, instReadSet, instWriteSet, readWriteLocs[other->ID], callSiteList, combinedCheckPrefixType, info, methodsInfo);
                break;
            }

            case IRMOVE:
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IRNEG:
            case IROR:
            case IRNOT:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRCONV:
            case IRCONVOVF:
            case IRBITCAST:
            case IRLT:
            case IRGT:
            case IREQ:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRCOSH:
            case IRCEIL:
            case IRSIN:
            case IRCOS:
            case IRACOS:
            case IRSQRT:
            case IRFLOOR:
            case IRPOW:
            case IREXP:
            case IRLOG10:
                if (readWriteLocs[other->ID] != NULL) {
                    PDEBUG("  Compare to inst %u (orig %u)\n", other->ID, origOther->ID);
                    printReadWriteSets(readWriteLocs[other->ID], "    ");
                    computeCallAndInstMemoryDependences(origInst, origOther, origMethod, instReadSet, instWriteSet, readWriteLocs[other->ID], callSiteList, checkPrefixType, info, methodsInfo);
                }
                break;

            case IRCALL:
            case IRICALL:
            case IRVCALL: {
                abs_addr_set_t *otherReadSet;
                abs_addr_set_t *otherWriteSet;
                JITBOOLEAN otherKnownCall;
                aaset_prefix_t combinedCheckPrefixType;

                /* If there is a library call, it will be dealt with later. */
                if (callTreeContainsLibraryCall(other, info, methodsInfo)) {
                    break;
                }

                /* If the other is a known call and this isn't then we want to do it the other way round. */
                otherKnownCall = instructionCallsKnownLibrary(other, info);
                if (!knownCall && otherKnownCall) {
                    break;
                }

                /* To avoid duplicating work, only do this one way round. */
                if (knownCall == otherKnownCall && inst->ID > other->ID) {
                    break;
                }

                /* Initialisation. */
                PDEBUG("  Compare to call inst %u (orig %u)\n", other->ID, origOther->ID);
                otherReadSet = smallHashTableLookup(info->callReadMap, other);
                otherWriteSet = smallHashTableLookup(info->callWriteMap, other);
                if (otherKnownCall) {
                    assert(checkPrefixType == AASET_PREFIX_FIRST);
                    combinedCheckPrefixType = AASET_PREFIX_BOTH;
                } else {
                    combinedCheckPrefixType = checkPrefixType;
                }

                /* Debugging. */
                assert(otherReadSet);
                assert(otherWriteSet);
                PDEBUG("    Read set: ");
                printAbstractAddressSet(otherReadSet);
                PDEBUG("    Write set: ");
                printAbstractAddressSet(otherWriteSet);

                /* Compute memory read / write dependencs. */
                computeCallAndCallMemoryDependences(origInst, origOther, origMethod, instReadSet, instWriteSet, otherReadSet, otherWriteSet, callSiteList, combinedCheckPrefixType, info, methodsInfo);
                break;
            }
        }
    }
}


/**
 * Compute memory aliases through variables before the given instruction.  This
 * means checking whether any variable could hold the same abstract address as
 * any other variable before the given instruction.  This is made more
 * complicated because the actual analysis is performed on an SSA version of
 * the method, where each variable is declared only once.  Fortunately, at
 * initialisation, we have set up a mapping between variables from the SSA
 * method to their counterpart in the original method.  There is a mapping for
 * each instruction in the SSA method, as long as the SSA variable has been
 * defined at that point.  So we only need to check the variables that have
 * mappings.
 */
static void
computeVariableAliasesForInst(ir_instruction_t *inst, ir_method_t *origMethod, method_info_t *info, SmallHashTable *methodVarMap) {
    JITINT32 sVar1, sVar2, oVar1, oVar2;
    JITINT32 numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod);
    ir_instruction_t *origInst = smallHashTableLookup(info->instMap, inst);
    abs_addr_set_t **mergedSets = vllpaAllocFunction(numVars * sizeof(abs_addr_set_t *), 7);

    /* If there's no original instruction, we can ignore this one. */
    if (origInst) {
        JITINT32 *varArray = vllpaAllocFunction(numVars * sizeof(JITINT32), 27);

        /* Build up a mapping between SSA variables and live original variables. */
        for (sVar1 = 0; sVar1 < numVars; ++sVar1) {
            if (IRMETHOD_isVariableLiveIN(info->ssaMethod, inst, sVar1)) {
                oVar1 = lookupMappingVarToVar(methodVarMap, sVar1);
                assert(oVar1 != -1);
                if (oVar1 != -2) {
                    varArray[sVar1] = oVar1;
                } else {
                    varArray[sVar1] = -1;
                }
            } else {
                varArray[sVar1] = -1;
            }
        }

        /* Check all SSA variables that have mappings. */
        for (sVar1 = 0; sVar1 < numVars; ++sVar1) {
            oVar1 = varArray[sVar1];
            if (oVar1 != -1) {
                for (sVar2 = 0; sVar2 < numVars; ++sVar2) {
                    oVar2 = varArray[sVar2];
                    if (oVar2 != -1 && oVar1 != oVar2) {
                        abs_addr_set_t *var1Set = mergedSets[sVar1];
                        abs_addr_set_t *var2Set = mergedSets[sVar2];

                        /* Get the abstract address sets if not cached. */
                        if (!var1Set) {
                            var1Set = absAddrSetClone(getVarAbstractAddressSetVarKey(sVar1, info, JITFALSE));
                            applyGenericMergeMapToAbstractAddressSet(var1Set, info->mergeAbsAddrMap);
                            mergedSets[sVar1] = var1Set;
                        }
                        if (!var2Set) {
                            var2Set = absAddrSetClone(getVarAbstractAddressSetVarKey(sVar2, info, JITFALSE));
                            applyGenericMergeMapToAbstractAddressSet(var2Set, info->mergeAbsAddrMap);
                            mergedSets[sVar2] = var2Set;
                        }

                        /* Check for aliases. */
                        if (abstractAddressSetsOverlap(var1Set, var2Set, JITTRUE, 0)) {
                            IRMETHOD_setAlias(origMethod, origInst, oVar1, oVar2);
                            PDEBUG("Alias between %u (%u) and %u (%u)\n", sVar1, oVar1, sVar2, oVar2);
                        }
                    }
                }
            }
        }

        /* Clean up. */
        vllpaFreeFunction(varArray, numVars * sizeof(JITINT32));
    }

    /* Clean up. */
    for (sVar1 = 0; sVar1 < numVars; ++sVar1) {
        if (mergedSets[sVar1]) {
            freeAbstractAddressSet(mergedSets[sVar1]);
        }
    }
    vllpaFreeFunction(mergedSets, numVars * sizeof(abs_addr_set_t *));
}


/**
 * Insert a mapping into a variable to variable map, if the mapping is new
 * or an update to the existing mapping.  Returns JITTRUE if the mapping was
 * inserted, or JITFALSE otherwise.
 */
static JITBOOLEAN
insertVarToVarMappingIfNew(SmallHashTable *ssaToOrigMap, JITINT32 sVar, JITINT32 oVar) {
    JITINT32 mVar = lookupMappingVarToVar(ssaToOrigMap, sVar);
    assert(mVar == -1 || mVar == -2 || oVar == -2 || mVar == oVar);
    if (mVar == -1) {
        insertMappingVarToVar(ssaToOrigMap, sVar, oVar);
        return JITTRUE;
    } else if (mVar == -2 && oVar >= 0) {
        deleteMappingVarToVar(ssaToOrigMap, sVar);
        insertMappingVarToVar(ssaToOrigMap, sVar, oVar);
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Create a mapping between all variables in the given SSA method to their
 * counterparts in the original method.  This enables correct dependence
 * calculation at the end of the pass.  Create a mapping for each instruction
 * in the SSA method, that represents the variables just before that
 * instruction is executed.
 */
static SmallHashTable *
createVarMappingBetweenMethods(ir_method_t *origMethod, ir_method_t *ssaMethod, SmallHashTable *instMap) {
    /* SmallHashTable *varMapping; */
    SmallHashTable *ssaToOrigMap;
    /* JITUINT32 numVars; */
    JITUINT32 ssaNumBlocks;
    JITUINT32 b, c, i, v;
    JITUINT32 ssaCount;
    JITBOOLEAN additions;

    /* Debugging output. */
    PDEBUG("Creating variable mappings for %s\n", IRMETHOD_getMethodName(ssaMethod));
    /*   IROPTIMIZER_callMethodOptimization(irOptimizer, origMethod, METHOD_PRINTER); */
    /*   IROPTIMIZER_callMethodOptimization(irOptimizer, ssaMethod, METHOD_PRINTER); */

    /* /\* Number of variables required. *\/ */
    /* numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(ssaMethod); */

    /* /\* Allocate the map between instructions and variable map. *\/ */
    /* varMapping = allocateNewHashTable(); */

    /* Early exit. */
    assert(IRMETHOD_getNumberOfMethodBasicBlocks(origMethod) == IRMETHOD_getNumberOfMethodBasicBlocks(ssaMethod));
    if (IRMETHOD_getNumberOfMethodBasicBlocks(origMethod) == 0) {
        /* return varMapping; */
        return NULL;
    }

    /* Build an overall mapping between SSA variables and their originals. */
    ssaToOrigMap = allocateNewHashTable();
    for (v = 0;  v < IRMETHOD_getMethodParametersNumber(ssaMethod); ++v) {
        insertMappingVarToVar(ssaToOrigMap, v, v);
    }

    /* Add variables defined through instructions. */
    additions = JITTRUE;
    ssaNumBlocks = IRMETHOD_getNumberOfMethodBasicBlocks(ssaMethod);
    while (additions) {
        additions = JITFALSE;
        ssaCount = 0;
        for (b = 0; b < ssaNumBlocks; ++b) {
            IRBasicBlock *ssaBlock = IRMETHOD_getBasicBlockAtPosition(ssaMethod, b);
            ir_instruction_t *ssaInst = IRMETHOD_getFirstInstructionWithinBasicBlock(ssaMethod, ssaBlock);
            JITUINT32 ssaLen = IRMETHOD_getBasicBlockLength(ssaBlock);

            /* Mappings for each instruction in this block. */
            for (i = 0; i < ssaLen; ++i) {
                ir_instruction_t *origInst = smallHashTableLookup(instMap, ssaInst);
                if (IRMETHOD_doesInstructionDefineAVariable(ssaMethod, ssaInst)) {
                    JITINT32 sVar, oVar = -1;

                    /* Deal with phi nodes separately - there's no original var. */
                    if (ssaInst->type == IRPHI) {
                        sVar = IRMETHOD_getVariableDefinedByInstruction(ssaMethod, ssaInst);

                        /* All merged variables should target the same original variable. */
                        for (c = 0; c < IRMETHOD_getInstructionCallParametersNumber(ssaInst); ++c) {
                            ir_item_t *param = IRMETHOD_getInstructionCallParameter(ssaInst, c);
                            if (param->type == IROFFSET) {
                                JITINT32 pVar = lookupMappingVarToVar(ssaToOrigMap, param->value.v);
                                assert(oVar == -1 || oVar == -2 || pVar == -1 || pVar == -2 || oVar == pVar);

                                /* Prefer original variables over no mapping. */
                                if (oVar == -1 || (oVar == -2 && pVar >= 0)) {
                                    oVar = pVar;
                                }
                            }
                        }
                    } else {

                        /* Store a mapping for counterpart instructions. */
                        sVar = IRMETHOD_getVariableDefinedByInstruction(ssaMethod, ssaInst);
                        if (origInst) {
                            oVar = IRMETHOD_getVariableDefinedByInstruction(origMethod, origInst);
                            assert(oVar != -1);
                        }

                        /* For new instructions, store a marker to indicate no mapping. */
                        else {
                            oVar = -2;
                        }
                    }

                    /* Insert a mapping if it doesn't already exist. */
                    if (oVar != -1) {
                        /* PDEBUG("Inserting mapping %d -> %d from inst %u (%d)\n", sVar, oVar, ssaInst->ID, origInst ? origInst->ID : -1); */
                        additions |= insertVarToVarMappingIfNew(ssaToOrigMap, sVar, oVar);
                    }

                    /* For get_address instructions, also check the source variable. */
                    if (ssaInst->type == IRGETADDRESS) {
                        assert(origInst);
                        sVar = IRMETHOD_getInstructionParameter1Value(ssaInst);
                        oVar = IRMETHOD_getInstructionParameter1Value(origInst);
                        /* PDEBUG("Inserting mapping %d -> %d from inst %u (%d)\n", sVar, oVar, ssaInst->ID, origInst->ID); */
                        additions |= insertVarToVarMappingIfNew(ssaToOrigMap, sVar, oVar);
                    }
                }

                /* Next instruction. */
                ssaCount += 1;
                ssaInst = IRMETHOD_getNextInstruction(ssaMethod, ssaInst);
            }
        }
        assert((ssaCount - 1) == IRMETHOD_getInstructionsNumber(ssaMethod));
    }

    /* /\* Create the mappings between SSA variables and their original equivalents. *\/ */
    /* for (b = 0; b < ssaNumBlocks; ++b) { */
    /*   IRBasicBlock *ssaBlock = IRMETHOD_getBasicBlockAtPosition(ssaMethod, b); */
    /*   ir_instruction_t *ssaInst = IRMETHOD_getFirstInstructionWithinBasicBlock(ssaMethod, ssaBlock); */

    /*   /\* Iterate over instructions. *\/ */
    /*   for (i = 0; i < IRMETHOD_getBasicBlockLength(ssaBlock); ++i) { */

    /*     /\* Create new mappings. *\/ */
    /*     JITINT32 *varArray = vllpaAllocFunction(numVars * sizeof(JITINT32), 8); */
    /*     for (v = 0; v < numVars; ++v) { */
    /*       if (IRMETHOD_isVariableLiveIN(ssaMethod, ssaInst, v)) { */
    /*         JITINT32 oVar = lookupMappingVarToVar(ssaToOrigMap, v); */
    /*         assert(oVar != -1); */
    /*         if (oVar != -2) { */
    /*           varArray[v] = oVar; */
    /*         } else { */
    /*           varArray[v] = -1; */
    /*         } */
    /*       } else { */
    /*         varArray[v] = -1; */
    /*       } */
    /*     } */

    /*     /\* Store this map. *\/ */
    /*     smallHashTableInsert(varMapping, ssaInst, varArray); */

    /*     /\* Next SSA instruction. *\/ */
    /*     ssaInst = IRMETHOD_getNextInstruction(ssaMethod, ssaInst); */
    /*   } */
    /* } */

    /* /\* Clean up. *\/ */
    /* freeHashTable(ssaToOrigMap); */

    /* /\* Return the variable map. *\/ */
    /* return varMapping; */
    return ssaToOrigMap;
}


/**
 * Create structures holding information about the parameters that read and
 * write memory, as well as their corresponding sets of abstract addresses,
 * for each non-call instruction that can access memory.
 */
static read_write_loc_t **
createNonCallReadWriteLocations(ir_method_t *method, method_info_t *info, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    JITUINT32 i, p, numInsts;
    read_write_loc_t **readWriteLocs;

    /* Create the array. */
    numInsts = IRMETHOD_getInstructionsNumber(method);
    readWriteLocs = vllpaAllocFunction(numInsts * sizeof(read_write_loc_t *), 9);

    /* Fill it up. */
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);

        /* Action depends on instruction type. */
        switch (inst->type) {
            case IRLOADREL:
                readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 10);
                readWriteLocs[inst->ID]->readParams[0] = IRMETHOD_getInstructionParameter(inst, 1);
                readWriteLocs[inst->ID]->readSets[0] = getLoadStoreAbsAddrSet(inst, info);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->readSets[0], info, concreteValues, concreteUivToKeyMap);
                break;

            case IRSTRINGLENGTH:
            case IRSTRINGCHR:
                readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 11);
                readWriteLocs[inst->ID]->readParams[0] = IRMETHOD_getInstructionParameter(inst, 1);
                readWriteLocs[inst->ID]->readSets[0] = getParameterAccessedAbsAddrSet(inst, 1, info);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->readSets[0], info, concreteValues, concreteUivToKeyMap);
                break;

            case IRSTOREREL:
                readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 12);
                readWriteLocs[inst->ID]->writeParam = IRMETHOD_getInstructionParameter(inst, 1);
                readWriteLocs[inst->ID]->writeSet = getLoadStoreAbsAddrSet(inst, info);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->writeSet, info, concreteValues, concreteUivToKeyMap);
                break;

            case IRINITMEMORY:
            case IRFREEOBJ:
            case IRFREE:
                readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 13);
                readWriteLocs[inst->ID]->writeParam = IRMETHOD_getInstructionParameter(inst, 1);
                readWriteLocs[inst->ID]->writeSet = getParameterAccessedAbsAddrSet(inst, 1, info);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->writeSet, info, concreteValues, concreteUivToKeyMap);
                break;

            case IRMEMCPY:
                readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 14);
                readWriteLocs[inst->ID]->readParams[0] = IRMETHOD_getInstructionParameter(inst, 2);
                readWriteLocs[inst->ID]->writeParam = IRMETHOD_getInstructionParameter(inst, 1);
                readWriteLocs[inst->ID]->readSets[0] = getParameterAccessedAbsAddrSet(inst, 2, info);
                readWriteLocs[inst->ID]->writeSet = getParameterAccessedAbsAddrSet(inst, 1, info);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->readSets[0], info, concreteValues, concreteUivToKeyMap);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->writeSet, info, concreteValues, concreteUivToKeyMap);
                break;

            case IRMEMCMP:
            case IRSTRINGCMP:
                readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 15);
                readWriteLocs[inst->ID]->readParams[0] = IRMETHOD_getInstructionParameter(inst, 1);
                readWriteLocs[inst->ID]->readParams[1] = IRMETHOD_getInstructionParameter(inst, 2);
                readWriteLocs[inst->ID]->readSets[0] = getParameterAccessedAbsAddrSet(inst, 1, info);
                readWriteLocs[inst->ID]->readSets[1] = getParameterAccessedAbsAddrSet(inst, 2, info);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->readSets[0], info, concreteValues, concreteUivToKeyMap);
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->readSets[1], info, concreteValues, concreteUivToKeyMap);
                break;

            case IRMOVE:
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IRNEG:
            case IROR:
            case IRNOT:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRCONV:
            case IRCONVOVF:
            case IRBITCAST:
            case IRLT:
            case IRGT:
            case IREQ:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRCOSH:
            case IRCEIL:
            case IRSIN:
            case IRCOS:
            case IRACOS:
            case IRSQRT:
            case IRFLOOR:
            case IRPOW:
            case IREXP:
            case IRLOG10:
                if (IRMETHOD_mayInstructionAccessHeapMemory(info->ssaMethod, inst)) {
                    readWriteLocs[inst->ID] = vllpaAllocFunction(sizeof(read_write_loc_t), 16);
                    for (p = 1; p < 4; ++p) {
                        ir_item_t *param = IRMETHOD_getInstructionParameter(inst, p);
                        if (param && param->type == IROFFSET) {
                            abs_addr_set_t *aaSet = getMergedVarAbsAddrSetIfEscaped(param->value.v, info);
                            if (aaSet) {
                                readWriteLocs[inst->ID]->readParams[p - 1] = param;
                                readWriteLocs[inst->ID]->readSets[p - 1] = aaSet;
                                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->readSets[p - 1], info, concreteValues, concreteUivToKeyMap);
                            }
                        }
                    }
                }
                break;
        }

        /* Add in the write set for defined escaped variables. */
        if (readWriteLocs[inst->ID] != NULL) {
            abs_addr_set_t *aaSet = getGenericInstWriteSet(inst, info);
            if (aaSet) {
                assert(readWriteLocs[inst->ID]->writeParam == NULL);
                readWriteLocs[inst->ID]->writeParam = IRMETHOD_getInstructionResult(inst);
                readWriteLocs[inst->ID]->writeSet = aaSet;
                addConcreteValuesToAbsAddrSet(readWriteLocs[inst->ID]->writeSet, info, concreteValues, concreteUivToKeyMap);
            }
        }
    }

    /* Return this array. */
    return readWriteLocs;
}


/**
 * Compute memory dependences between instructions within the given
 * method.
 */
static void
computeMemoryDependencesInMethod(ir_method_t *origMethod, method_info_t *info, SmallHashTable *methodsInfo, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    JITUINT32 numVars VLLPA_UNUSED;
    JITUINT32 i, p, numInsts;
    SmallHashTable *methodVarMap;
    /* SmallHashTableItem *tableItem; */
    read_write_loc_t **readWriteLocs;

    /* Dummy alias to set up structures. */
    IRMETHOD_setAlias(origMethod, IRMETHOD_getInstructionAtPosition(origMethod, 0), 0, 0);

    /* Create a mapping between this method's SSA variables and their originals. */
    methodVarMap = createVarMappingBetweenMethods(origMethod, info->ssaMethod, info->instMap);

    /* Extract read and write locations for each non-call that can access memory. */
    readWriteLocs = createNonCallReadWriteLocations(info->ssaMethod, info, concreteValues, concreteUivToKeyMap);

    /* Consider each instruction. */
    numInsts = IRMETHOD_getInstructionsNumber(info->ssaMethod);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->ssaMethod, i);

        /* Action depends on instruction type. */
        switch (inst->type) {
            case IRLOADREL:
            case IRSTOREREL:
            case IRMEMCPY:
            case IRMEMCMP:
            case IRSTRINGCMP:
            case IRSTRINGLENGTH:
            case IRSTRINGCHR:
            case IRINITMEMORY:
            case IRFREEOBJ:
            case IRFREE:
                PDEBUG("Dependences for inst %u (non-call memory)\n", inst->ID);
                computeNonCallMemoryDependences(inst, origMethod, readWriteLocs, info, methodsInfo);
                break;

            case IRMOVE:
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IRNEG:
            case IROR:
            case IRNOT:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRCONV:
            case IRCONVOVF:
            case IRBITCAST:
            case IRLT:
            case IRGT:
            case IREQ:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRCOSH:
            case IRCEIL:
            case IRSIN:
            case IRCOS:
            case IRACOS:
            case IRSQRT:
            case IRFLOOR:
            case IRPOW:
            case IREXP:
            case IRLOG10:
                /* Accesses memory if there are read or write locations. */
                if (readWriteLocs[inst->ID] != NULL) {
                    PDEBUG("Dependences for inst %u (non-call generic)\n", inst->ID);
                    computeNonCallMemoryDependences(inst, origMethod, readWriteLocs, info, methodsInfo);
                }
                break;

            case IRLIBRARYCALL:
            case IRNATIVECALL:
                if (IRMETHOD_mayInstructionAccessHeapMemory(info->ssaMethod, inst)) {
                    PDEBUG("Dependences for inst %u (library call)\n", inst->ID);
                    computeLibraryMemoryDependences(inst, origMethod, info, methodsInfo);
                }
                break;

            case IRCALL:
            case IRICALL:
            case IRVCALL:
                if (callTreeContainsLibraryCall(inst, info, methodsInfo)) {
                    PDEBUG("Dependences for inst %u (general call with library in tree)\n", inst->ID);
                    computeLibraryMemoryDependences(inst, origMethod, info, methodsInfo);
                } else {
                    PDEBUG("Dependences for inst %u (general or known call)\n", inst->ID);
                    computeCallMemoryDependences(inst, origMethod, readWriteLocs, info, methodsInfo);
                }
                break;
        }

        /* Compute variable aliases before this instruction. */
        computeVariableAliasesForInst(inst, origMethod, info, methodVarMap);
    }

    /* Clean up the variable mappings. */
    /* numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(info->ssaMethod); */
    /* tableItem = smallHashTableFirst(methodVarMap); */
    /* while (tableItem) { */
    /*   JITINT32 *varArray = tableItem->element; */
    /*   vllpaFreeFunction(varArray, numVars * sizeof(JITINT32)); */
    /*   tableItem = smallHashTableNext(methodVarMap, tableItem); */
    /* } */
    if (methodVarMap) {
        freeHashTable(methodVarMap);
    }

    /* Clean up read and write locations. */
    for (i = 0; i < numInsts; ++i) {
        if (readWriteLocs[i] != NULL) {
            for (p = 0; p < 3; ++p) {
                if (readWriteLocs[i]->readSets[p] != NULL) {
                    freeAbstractAddressSet(readWriteLocs[i]->readSets[p]);
                }
            }
            if (readWriteLocs[i]->writeSet != NULL) {
                freeAbstractAddressSet(readWriteLocs[i]->writeSet);
            }
            vllpaFreeFunction(readWriteLocs[i], sizeof(read_write_loc_t));
        }
    }
    vllpaFreeFunction(readWriteLocs, numInsts * sizeof(read_write_loc_t *));
}


/**
 * Compute the memory dependences between instructions within each method
 * within an SCC.
 */
void
computeMemoryDependencesInSCC(XanList *scc, SmallHashTable *methodsInfo, SmallHashTable *concreteValues, SmallHashTable *concreteUivToKeyMap) {
    XanListItem *currMethod = xanList_first(scc);
    while (currMethod) {
        ir_method_t *method = currMethod->data;
        method_info_t *info = smallHashTableLookup(methodsInfo, method);
        GET_DEBUG_ENABLE(JITBOOLEAN prevEnablePrintDebug);
        if (enableExtendedDebugPrinting(method)) {
            SET_DEBUG_ENABLE(JITTRUE);
        }
        PDEBUGLITE("Compute aliases for %s\n", IRMETHOD_getMethodName(info->ssaMethod));

        /* Compute memory dependences in this method. */
        computeMemoryDependencesInMethod(method, info, methodsInfo, concreteValues, concreteUivToKeyMap);
        if (enablePDebug) {
            printDependences(method, stderr);
        }

        /* Next method. */
        SET_DEBUG_ENABLE(prevEnablePrintDebug);
        currMethod = xanList_next(currMethod);
    }
}


/**
 * Print out the number of memory data dependences identified.
 */
void
printMemoryDataDependenceStats(void) {
    char *env = getenv("DDG_IP_PRINT_MEMORY_DEPENDENCE_STATS");
    if (env && atoi(env) != 0) {
        fprintf(stderr, "DDGIP: All memory data dependences %u\n", memoryDataDependencesAll);
        fprintf(stderr, "DDGIP: Instruction memory data dependences %u\n", memoryDataDependencesInst);
    }
}
