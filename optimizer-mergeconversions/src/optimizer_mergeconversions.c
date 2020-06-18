/*
 * Copyright (C) 2007 - 2009  Campanoni Simone
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
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_mergeconversions.h>
#include <delete_conversions.h>
#include <config.h>
// End

#define INFORMATIONS    "This is a mergeconversions plugin"
#define AUTHOR          "Timothy M. Jones"

static inline JITUINT64 mergeconversions_get_ID_job (void);
static inline char * mergeconversions_get_version (void);
static inline void mergeconversions_do_job (ir_method_t * method);
static inline char * mergeconversions_get_informations (void);
static inline char * mergeconversions_get_author (void);
static inline JITUINT64 mergeconversions_get_dependences (void);
static inline void mergeconversions_shutdown (JITFLOAT32 totalTime);
static inline void mergeconversions_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 mergeconversions_get_invalidations (void);
static inline void mergeconversions_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void mergeconversions_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;


/**
 * Get a list of conversion instructions in the order they appear in the
 * program.  All instructions reside in the same basic block and the result
 * of one instruction is used by the next.  However, there can be different
 * instructions in between the conversions (or other conversions that don't
 * use the previous result) that are not included in the list.
 */
static XanList *
getConvSeq (ir_method_t *method, ir_instruction_t *startInst,
            XanHashTable *instInSeqTable) {
    JITBOOLEAN finishedSeq = JITFALSE;
    ir_instruction_t *next = startInst;
    ir_instruction_t *prev = NULL;

    /* Create the list. */
    XanList *convList = xanList_new(allocFunction, freeFunction, NULL);
    assert(startInst->type == IRCONV);
    PDEBUG("MergeConvs: Conversion sequence starting inst %u\n", startInst->ID);

    /* Add successive conversions to the list. */
    while (!finishedSeq) {
        ir_instruction_t *inst = next;

        /* Check for the final instruction in a basic block. */
        if (IRMETHOD_getSuccessorsNumber(method, inst) == 1) {
            next = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
        } else {
            finishedSeq = JITTRUE;
        }

        /* Must belong to the same basic block as predecessor. */
        if (prev != NULL && IRMETHOD_getPredecessorsNumber(method, inst) != 1) {
            finishedSeq = JITTRUE;
        }

        /* Check for a conversion. */
        else if (inst->type == IRCONV) {
            ir_item_t *param = IRMETHOD_getInstructionParameter1(inst);

            /* Must be converting previous variable, else skip. */
            if (prev == NULL || (param->type == IROFFSET &&
                                 param->value.v == IRMETHOD_getInstructionResultValue(prev))) {
                xanList_append(convList, inst);
                xanHashTable_insert(instInSeqTable, inst, inst);
                prev = inst;
                PDEBUG("MergeConvs:   Added inst %u\n", inst->ID);
            }
        }

        /* Don't redefine the last variable. */
        else {
            ir_item_t *result = IRMETHOD_getInstructionResult(inst);
            assert(prev);
            if (result->type == IROFFSET && result->value.v == IRMETHOD_getInstructionResultValue(prev)) {
                finishedSeq = JITTRUE;
                PDEBUG("MergeConvs:   Redefinition of last variable, conversion sequence ends\n");
            }
        }
    }

    /* Return the created list. */
    return convList;
}


/**
 * Repeatedly remove the final conversion in a sequence if the target variable
 * is defined or used anywhere within the sequence.  This is required since we
 * add the new conversions / store right before the sequence and if there's
 * any other definition or use of the target variable then the semantics of
 * the program change.
 */
static void removeDefinedOrUsedTargets (ir_method_t *method, XanList *convList, XanHashTable *instInSeqTable) {
    JITBOOLEAN removed = JITTRUE;

    /* Repeatedly perform this loop until no more instructions are removed. */
    while (removed) {
        ir_instruction_t *inst = xanList_first(convList)->data;
        ir_instruction_t *endInst = xanList_last(convList)->data;
        IR_ITEM_VALUE targetVar = IRMETHOD_getInstructionResultValue(endInst);
        JITUINT32 blockNum = targetVar / (sizeof(JITNUINT) * 8);
        JITUINT32 blockBit = targetVar % (sizeof(JITNUINT) * 8);
        removed = JITFALSE;

        /* Iterate from start to end, checking for a def or use of the
                 * variable.  All instructions in the list are in the same
                 * basic block.
         */
        while (!removed && inst != endInst) {
            if ((livenessGetDef(method, inst) == targetVar) ||
                    ((livenessGetUse(method, inst, blockNum) >> blockBit) & 0x1)) {
                xanList_removeElement(convList, endInst, JITTRUE);
                xanHashTable_removeElement(instInSeqTable, endInst);
                removed = JITTRUE;
            }
            inst = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
        }
    }
}


/**
 * Remove all conversions from a list, starting with the first floating point
 * conversion found.  This is required because this pass does not necessarily
 * deal with floating point conversion merging correctly.  Once fixed, this
 * function will be defunct and can be removed.
 */
static void
removeAfterFloatConv (XanList *convList) {
    JITBOOLEAN foundFloat = JITFALSE;
    XanListItem *currConv = xanList_first(convList);

    /* Remove all if starting with a float type. */
    ir_instruction_t *startInst = currConv->data;

    if (IRMETHOD_hasAFloatType(IRMETHOD_getInstructionParameter1(startInst))) {
        xanList_emptyOutList(convList);
        return;
    }

    /* Check each conversion for a float. */
    while (currConv) {
        XanListItem *nextConv = currConv->next;

        /* Check for a conversion to a float. */
        ir_instruction_t *inst = currConv->data;
        if (IRMETHOD_hasAFloatType(IRMETHOD_getInstructionParameter2(inst))) {
            foundFloat = JITTRUE;
        }

        /* Delete all instructions after the first float is found. */
        if (foundFloat) {
            xanList_deleteItem(convList, currConv);
        }

        /* Next conversion. */
        currConv = nextConv;
    }
}

/**
 * Determine the number of conversions needed based on the key types within
 * the sequence of conversions.  This assumes that the startType and endType
 * parameters are variables whereas no assumption is required for the
 * shortestType or unsignedChangeType.
 */
static JITUINT8 calculateNumConvs (ir_item_t *startType, ir_item_t *endType, ir_item_t *shortestType, ir_item_t *unsignedChangeType) {
    JITUINT8 numConvs = 0;
    JITINT32 startSize = IRDATA_getSizeOfType(startType);
    JITINT32 endSize = IRDATA_getSizeOfType(endType);
    JITINT32 shortestSize = IRDATA_getSizeOfType(shortestType);

    /* Check whether bits are lost or we need to change sign. */
    if (shortestSize < startSize ||
            (IRMETHOD_hasASignedType(shortestType) != IRMETHOD_hasASignedType(startType))) {
        numConvs += 1;

        /* Check whether bits are added. */
        if (shortestSize < endSize) {
            numConvs += 1;

            /* Check if there's a change to unsigned on the way. */
            if (unsignedChangeType) {
                numConvs += 1;
            }
        }
        /* No bits are added. */
        else {
            assert(shortestSize == endSize);
            assert(!unsignedChangeType);
        }
    }
    /* No bits are lost or no initial change to signed. */
    else {
        assert(shortestSize == startSize);

        /* Check whether bits are added. */
        if (shortestSize < endSize) {
            numConvs += 1;

            /* Check if there's a change to unsigned on the way. */
            if (unsignedChangeType) {
                numConvs += 1;
            }
        }
        /* No bits are added. */
        else {
            assert(shortestSize == endSize);
            assert(!unsignedChangeType);
            if (startType->internal_type != endType->internal_type) {
                numConvs += 1;
            }
        }
    }

    /* Return the number of conversions required. */
    return numConvs;
}

/**
 * Create new conversion instructions from the given list.  The number of
 * conversions to create is given, along with the type containing the least
 * information, if needed.
 */
static void createNewConversions (ir_method_t *method, XanList *convList, JITUINT8 numConvs, ir_item_t *shortestType, ir_item_t *unsignedChangeType) {
    ir_instruction_t *firstInst = xanList_first(convList)->data;
    ir_instruction_t *lastInst = xanList_last(convList)->data;

    /* No conversions needed, create a new store. */
    if (numConvs == 0) {
        ir_instruction_t *newStore = IRMETHOD_newInstructionOfTypeBefore(method, firstInst, IRMOVE);
        IRMETHOD_cpInstructionParameter(method, firstInst, 1, newStore, 1);
        IRMETHOD_cpInstructionParameter(method, lastInst, 0, newStore, 0);
    }
    /* Convert from first type to last. */
    else if (numConvs == 1) {
        ir_instruction_t *newConv = IRMETHOD_newInstructionOfTypeBefore(method, firstInst, IRCONV);
        IRMETHOD_cpInstructionParameter(method, firstInst, 1, newConv, 1);
        IRMETHOD_cpInstructionParameter(method, lastInst, 0, newConv, 0);
        IRMETHOD_cpInstructionParameter(method, lastInst, 2, newConv, 2);
    }
    /* Convert to at least one intermediate type, then to last. */
    else {
        ir_instruction_t *newConv1;
        ir_instruction_t *newConv2;
        IR_ITEM_VALUE newVarId = IRMETHOD_newVariableID(method);

        /* First conversion to least information type or unsigned change type. */
        newConv1 = IRMETHOD_newInstructionOfTypeBefore(method, firstInst, IRCONV);
        IRMETHOD_cpInstructionParameter(method, firstInst, 1, newConv1, 1);
        if (numConvs == 2 && unsignedChangeType) {
            IRMETHOD_setInstructionResult(method, newConv1, newVarId, 0.0, IROFFSET, unsignedChangeType->value.v, NULL);
            IRMETHOD_cpInstructionParameter2(newConv1, unsignedChangeType);
        } else {
            IRMETHOD_setInstructionResult(method, newConv1, newVarId, 0.0, IROFFSET, shortestType->value.v, NULL);
            IRMETHOD_cpInstructionParameter2(newConv1, shortestType);
        }

        /* Second conversion to unsigned change type or end type. */
        newConv2 = IRMETHOD_newInstructionOfTypeBefore(method, firstInst, IRCONV);
        IRMETHOD_cpInstructionParameter(method, newConv1, 0, newConv2, 1);
        if (numConvs == 2) {
            IRMETHOD_cpInstructionParameter(method, lastInst, 0, newConv2, 0);
            IRMETHOD_cpInstructionParameter(method, lastInst, 2, newConv2, 2);
        } else {
            ir_instruction_t *newConv3;
            newVarId = IRMETHOD_newVariableID(method);
            IRMETHOD_setInstructionResult(method, newConv2, newVarId, 0.0, IROFFSET, unsignedChangeType->value.v,  NULL);
            IRMETHOD_cpInstructionParameter2(newConv2, unsignedChangeType);

            /* Third conversion to end type. */
            newConv3 = IRMETHOD_newInstructionOfTypeBefore(method, firstInst, IRCONV);
            IRMETHOD_cpInstructionParameter(method, newConv2, 0, newConv3, 1);
            IRMETHOD_cpInstructionParameter(method, lastInst, 0, newConv3, 0);
            IRMETHOD_cpInstructionParameter(method, lastInst, 2, newConv3, 2);
        }
    }
}


/**
 * Delete the final conversion in the given list, since the variable is
 * defined by the new sequence of conversions or store.  We leave the other
 * conversions where they are because their results may be used by later
 * instructions.  If not and their result variables are no longer live then
 * a later dead code elimination pass will delete them.
 */
static void
deleteFinalConversion (ir_method_t *method, XanList *convList) {
    ir_instruction_t *endInst = xanList_last(convList)->data;

    xanList_removeElement(convList, endInst, JITTRUE);
    IRMETHOD_deleteInstruction(method, endInst);
}


/**
 * Merge a list of conversions into zero, one, two or three conversions.  The
 * given list contains the conversions in program order.  To perform the merge
 * we first find the start type, end type, type with the least information
 * that is used and a type that converts from signed to unsigned as the size
 * of the conversion increases.  We then call a separate function to compute
 * the number of conversions required and a second to actually add in the new
 * instructions.
 */
static void
mergeConversionList (ir_method_t *method, XanList *convList) {
    JITUINT8 numConvs;
    ir_item_t *startType;
    ir_item_t *endType;
    ir_item_t *shortestType;
    ir_item_t *unsignedChangeType;
    ir_instruction_t *inst;
    XanListItem *currConv;

    /* Must be at least one conversion. */
    assert(xanList_length(convList) > 0);

    /* Set start type. */
    inst = xanList_first(convList)->data;
    assert(IRMETHOD_getInstructionType(inst) == IRCONV);
    startType = IRMETHOD_getInstructionParameter1(inst);
    /*   PDEBUG("MergeConvs: Merging list starting at inst %u\n", inst->ID); */

    /* Set end type. */
    inst = xanList_last(convList)->data;
    assert(IRMETHOD_getInstructionType(inst) == IRCONV);
    endType = IRMETHOD_getInstructionResult(inst);

    /* Find the type with the least information and whether the sign changes. */
    shortestType = startType;
    unsignedChangeType = NULL;
    currConv = xanList_first(convList);
    while (currConv) {
        ir_item_t *currType;
        inst = currConv->data;
        assert(IRMETHOD_getInstructionType(inst) == IRCONV);
        currType = IRMETHOD_getInstructionParameter2(inst);

        /* We want the last occurrence of the shortest type. */
        if (!shortestType || (IRDATA_getSizeOfType(currType) <=
                              IRDATA_getSizeOfType(shortestType))) {
            shortestType = currType;
            unsignedChangeType = NULL;
            /*       PDEBUG("MergeConvs: Least info type at inst %u\n", inst->ID); */
        }
        /* Work out whether there's a change from signed to unsigned. */
        else if (IRMETHOD_hasASignedType(shortestType) && !IRMETHOD_hasASignedType(currType) &&
                 currConv->next) {
            if (!unsignedChangeType ||
                    (IRDATA_getSizeOfType(currType) <
                     IRDATA_getSizeOfType(unsignedChangeType))) {
                unsignedChangeType = currType;
                /*         PDEBUG("MergeConvs: Change to unsigned at inst %u\n", inst->ID); */
            }
        }

        /* Next conversion. */
        currConv = currConv->next;
    }

    /* If the final type isn't larger than the unsigned change type, fix up. */
    if (unsignedChangeType &&
            (IRDATA_getSizeOfType(endType) <=
             IRDATA_getSizeOfType(unsignedChangeType))) {
        unsignedChangeType = NULL;
    }

    /* Determine the number of conversions needed. */
    numConvs = calculateNumConvs(startType, endType, shortestType, unsignedChangeType);
    /*   PDEBUG("MergeConvs: Requires %u conversions\n", numConvs); */

    /* Only continue if it's worth doing the work. */
    if (numConvs < xanList_length(convList)) {

        /* Create the new conversions. */
        createNewConversions(method, convList, numConvs, shortestType, unsignedChangeType);

        /* Then delete the final conversion which is now unnecessary. */
        deleteFinalConversion(method, convList);

        /* Information. */
        if (numConvs == 0) {
            PDEBUG("MergeConvs: Reduced %u conversions to 1 store in %s\n",
                   xanList_length(convList) + 1,
                   IRMETHOD_getCompleteMethodName(method));
        } else {
            PDEBUG("MergeConvs: Reduced %u conversions to %u conversions in %s\n",
                   xanList_length(convList) + 1, numConvs,
                   IRMETHOD_getCompleteMethodName(method));
        }
    }
}


#if 0
static void
checkValues (JITINT32 sval, JITUINT32 uval) {
    printf("Signed value is %d, unsigned value is %u\n", sval, uval);
}


static void
insertValueCheck (ir_method_t *method) {
    JITUINT32 cpNum;
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, 13);
    ir_instruction_t *newInst = IRMETHOD_newInstructionBefore(method, inst);

    IRMETHOD_setInstructionType(method, newInst, IRNATIVECALL);
    method->setInstrPar1(newInst, IRVOID, 0.0, IRTYPE, IRTYPE, JITFALSE, NULL);
    method->setInstrPar3(newInst, (JITUINT64) (JITNUINT) checkValues, 0.0,
                         IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, JITFALSE, NULL);

    cpNum = method->newCallParameter(method, newInst);
    method->setCallParameterValue(newInst, cpNum, 207);
    method->setCallParameterType(newInst, cpNum, IROFFSET);
    method->setCallParameterInternalType(newInst, cpNum, IRINT32);

    cpNum = method->newCallParameter(method, newInst);
    method->setCallParameterValue(newInst, cpNum, 205);
    method->setCallParameterType(newInst, cpNum, IROFFSET);
    method->setCallParameterInternalType(newInst, cpNum, IRUINT32);
}
#endif


/**
 * This pass attempts to merge conversion instructions within a basic block
 * into zero, one, two or three conversions.  This occurs in the following
 * situations:
 *
 * Zero conversions:  When none of the results from the intermediate
 * conversions are needed.  If the sequence of conversions starts and ends at
 * the same type, and there is no conversion to a type with less information,
 * then the whole sequence can be removed and replaced by a store.
 *
 * One conversion:  If the initial or final types contain the least
 * information and there is no change to unsigned as the size of the value
 * increases for the former, then one conversion will suffice.
 *
 * Two conversions.  If there is an intermediate type with less information
 * than the initial and final types.  Or, this type contains less information
 * than the end type and there is an intermediate type that changes from
 * signed to unsigned as the size of the value increases, then two conversions
 * are required.
 *
 * Three conversions.  If there is an intermediate type with the least
 * information and an intermediate type with a change from signed to unsigned
 * as the size of the value increases, then we must use three conversions.
 *
 * The intermediate type with the least information is required because this
 * will remove bits from the value which may not be recovered when increasing
 * the size again.  We also need to know whether the sign of the value changes
 * from signed to unsigned as the size of the value increases.  This is
 * because a signed value is sign-extended whereas an unsigned one obviously
 * isn't.  Therefore a value can be sign-extended for the first part of a
 * conversion but not for the last part.
 *
 * Improvements:
 * - Float type conversions aren't supported at the moment.
 */
static inline void
mergeconversions_do_job (ir_method_t * method) {
    JITUINT32 i;
    XanList *convListList;
    XanHashTable *instInSeqTable;

    /* Assertions. */
    assert(method != NULL);

    /* Debugging output. */
    /* PDEBUG("\nMergeConvs: Starting method %s\n", IRMETHOD_getCompleteMethodName(method)); */
    /* char *prevDotPrinterName = getenv("DOTPRINTER_FILENAME"); */
    /* setenv("DOTPRINTER_FILENAME", "Merge_conversions", 1); */
    /* IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER); */
    /* setenv("DOTPRINTER_FILENAME", prevDotPrinterName, 1); */

    /* Initialisation. */
    convListList = xanList_new(allocFunction, freeFunction, NULL);
    instInSeqTable = xanHashTable_new(11, 0, allocFunction, reallocFunction, freeFunction, NULL, NULL);

    /* Find sequences of conversions to merge together. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst;

        inst 	= IRMETHOD_getInstructionAtPosition(method, i);
        if (inst->type != IRCONV) {
            continue ;
        }
        assert(inst->type == IRCONV);

        if (xanHashTable_lookup(instInSeqTable, inst)) {
            continue ;
        }
        assert(!xanHashTable_lookup(instInSeqTable, inst));

        /* Find a sequence of conversions. */
        XanList *convList = getConvSeq(method, inst, instInSeqTable);

        /* Remove the last instruction(s) if the variables are redefined. */
        removeDefinedOrUsedTargets(method, convList, instInSeqTable);

        /* Remove conversions after the first float type. */
        removeAfterFloatConv(convList);

        /* Add to the running list, or free the memory used. */
        if (xanList_length(convList) > 0) {
            /*         PDEBUG("MergeConvs: Found list length %d starting at inst %u\n", */
            /*                xanList_length(convList), */
            /*                ((ir_instruction_t *)convList->first(convList)->data)->ID); */
            xanList_append(convListList, convList);
        } else {
            xanList_destroyList(convList);
        }
    }

    /* Merge each sequence that was found. */
    while (xanList_length(convListList) > 0) {
        XanList *convList = xanList_first(convListList)->data;
        mergeConversionList(method, convList);
        xanList_destroyList(convList);
        xanList_removeElement(convListList, convList, JITTRUE);
    }

    delete_useless_conversions(method);

    /* Clean up. */
    xanHashTable_destroyTable(instInSeqTable);
    xanList_destroyList(convListList);

    /* Debugging output. */
    /*   IROPTIMIZER_callMethodOptimization(irOptimizer, method, */
    /*                                      METHOD_PRINTER); */
}

ir_optimization_interface_t plugin_interface = {
    mergeconversions_get_ID_job,
    mergeconversions_get_dependences,
    mergeconversions_init,
    mergeconversions_shutdown,
    mergeconversions_do_job,
    mergeconversions_get_version,
    mergeconversions_get_informations,
    mergeconversions_get_author,
    mergeconversions_get_invalidations,
    NULL,
    mergeconversions_getCompilationFlags,
    mergeconversions_getCompilationTime
};

static inline void mergeconversions_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

static inline void mergeconversions_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

static inline JITUINT64 mergeconversions_get_ID_job (void) {
    return CONVERSION_MERGING;
}

static inline JITUINT64 mergeconversions_get_dependences (void) {
    return LIVENESS_ANALYZER;
}

static inline char * mergeconversions_get_version (void) {
    return VERSION;
}

static inline char * mergeconversions_get_informations (void) {
    return INFORMATIONS;
}

static inline char * mergeconversions_get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 mergeconversions_get_invalidations (void) {
    return INVALIDATE_ALL;
}

static inline void mergeconversions_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

static inline void mergeconversions_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
