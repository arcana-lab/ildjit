/*
 * Copyright (C) 2010  Campanoni Simone
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
// My headers
#include <ir_method.h>
#include <ir_optimizer.h>
#include <stddef.h>

#include <stdio.h>
#include <assert.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_cse.h>
#include <config.h>
// End

#define INFORMATIONS     "This step does the common sub-expression elimination"
#define    AUTHOR        "Simone Campanoni and Timothy M. Jones"

static inline JITUINT64 get_ID_job (void);
static inline void cse_do_job (ir_method_t *method);
char * get_version (void);
char * get_informations (void);
char * get_author (void);
JITUINT64 get_dependences (void);
void cse_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
void cse_shutdown (JITFLOAT32 totalTime);
static inline JITUINT64 cse_get_invalidations (void);
static inline void cse_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void cse_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t    *irLib;
ir_optimizer_t  *irOptimizer;
char            *irOutputPrefix;

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    cse_init,
    cse_shutdown,
    cse_do_job,
    get_version,
    get_informations,
    get_author,
    cse_get_invalidations,
    NULL,
    cse_getCompilationFlags,
    cse_getCompilationTime
};

/**
 * Macros for debug printing.
 **/
#if defined(PRINTDEBUG)
static JITINT32 debugLevel = -1;
#undef PDEBUG
#define PDEBUG(lvl, fmt, args...) if (lvl <= debugLevel) fprintf(stderr, "CSE: " fmt, ## args);
#define PDEBUGC(lvl, fmt, args...) if (lvl <= debugLevel) fprintf(stderr, fmt, ## args);
#define PDEBUGNL(lvl, fmt, args...) if (lvl <= debugLevel) fprintf(stderr, "CSE:\nCSE: " fmt, ##args);
#define PDECL(decl) decl
#define PDEBUGLEVEL(lvl) debugLevel = lvl;
#else
#undef PDEBUG
#define PDEBUG(lvl, fmt, args...)
#define PDEBUGC(lvl, fmt, args...)
#define PDEBUGNL(lvl, fmt, args...)
#define PDECL(decl)
#define PDEBUGLEVEL(lvl)
#endif


/**
 * A structure that holds all unchanging information for this pass.
 **/
typedef struct {
    JITUINT32 dumpCfg;
    JITUINT32 maxMethodInsts;
    JITFLOAT32 maxGrowthFactor;
    ir_method_t *method;
} cse_info_t;


/**
 * A structure holding an instruction to remove and a list of instructions
 * that it duplicates.
 */
typedef struct {
    ir_instruction_t *inst;
    XanList *duplicated;
} removal_entry_t;

/**
 * A structure holding a load to remove and the prior store which stores to the
 * same memory address as is loaded from.
 **/
typedef struct {
    ir_instruction_t *load;
    ir_instruction_t *store;
} remove_load_t;


/**
 * Check whether two parameters are the same.
 **/
static inline JITBOOLEAN
parametersMatch(ir_item_t *param1, ir_item_t *param2) {
    assert(param1);
    assert(param2);
    if (param1->type != param2->type) {
        PDEBUG(5, "parametersMatch: Type of the parameter does not match\n");
        return JITFALSE;
    }
    if (IRMETHOD_hasAnIntType(param1)) {
        if (param1->value.v != param2->value.v) {
            PDEBUG(5, "parametersMatch: Value of the parameter does not match (%llu %llu)\n", param1->value.v, param2->value.v);
            return JITFALSE;
        }
    } else {
        if (param1->value.f != param2->value.f) {
            PDEBUG(5, "parametersMatch: F-value of the parameter does not match (%f %f)\n", param1->value.f, param2->value.f);
            return JITFALSE;
        }
    }
    return JITTRUE;
}


/**
 * Check whether the types of two parameters are the same.
 **/
static inline JITBOOLEAN
typesMatch(ir_item_t *param1, ir_item_t *param2) {
    assert(param1);
    assert(param2);
    if (param1->internal_type != param2->internal_type) {
        return JITFALSE;
    } else if (param1->internal_type == IRVALUETYPE &&
               param1->type_infos != param2->type_infos) {
        return JITFALSE;
    }
    return JITTRUE;
}


/**
 * Check whether an instruction has a source variable the same as its
 * destination.
 **/
static inline JITBOOLEAN
updatesSourceVar(ir_method_t *method, ir_instruction_t *inst) {
    assert(inst);
    if (inst->type != IRMOVE && IRMETHOD_doesInstructionDefineAVariable(method, inst)) {
        JITUINT32 p, numParams;
        ir_item_t *result = IRMETHOD_getIRItemOfVariableDefinedByInstruction(method, inst);
        numParams = IRMETHOD_getInstructionParametersNumber(inst);
        for (p = 1; p <= numParams; ++p) {
            ir_item_t *param = IRMETHOD_getInstructionParameter(inst, p);
            if (param->type == IROFFSET && param->value.v == result->value.v) {
                return JITTRUE;
            }
        }
    }
    return JITFALSE;
}


/**
 * Check whether two instructions are duplicates.  This means that their
 * types and parameters are exactly the same, although their return parameters
 * can be different.
 */
static JITBOOLEAN instsAreDuplicates (ir_instruction_t *i1, ir_instruction_t *i2) {

    /* Instruction types must match. */
    if (i1->type != i2->type) {
        return JITFALSE;
    }
    PDEBUG(5, "instsAreDuplicates: Start\n");
    PDEBUG(5, "instsAreDuplicates: 	Insts %u and %u\n", i1->ID, i2->ID);

    /* In case there is a load operation, we must check that the amount of data loaded is the same	*/
    if (i1->type == IRLOADREL) {
        ir_item_t	*returnItem1;
        ir_item_t	*returnItem2;
        returnItem1	= IRMETHOD_getInstructionResult(i1);
        returnItem2	= IRMETHOD_getInstructionResult(i2);
        if (IRDATA_getSizeOfType(returnItem1) != IRDATA_getSizeOfType(returnItem2)) {
            PDEBUG(5, "instsAreDuplicates: 	The loads operations do not read the same amount of data\n");
            PDEBUG(5, "instsAreDuplicates: Exit\n");
            return JITFALSE;
        }
    }

    /* Each parameter must match. */
    switch (IRMETHOD_getInstructionParametersNumber(i1)) {
        case 3:
            if (!parametersMatch(IRMETHOD_getInstructionParameter3(i1), IRMETHOD_getInstructionParameter3(i2))) {
                PDEBUG(5, "instsAreDuplicates: 	Parameter 3 does not match\n");
                PDEBUG(5, "instsAreDuplicates: Exit\n");
                return JITFALSE;
            }

            /* Intentional fall-through. */
        case 2:
            if (!parametersMatch(IRMETHOD_getInstructionParameter2(i1), IRMETHOD_getInstructionParameter2(i2))) {
                PDEBUG(5, "instsAreDuplicates: 	Parameter 2 does not match\n");
                PDEBUG(5, "instsAreDuplicates: Exit\n");
                return JITFALSE;
            }

            /* Intentional fall-through. */
        case 1:
            if (!parametersMatch(IRMETHOD_getInstructionParameter1(i1), IRMETHOD_getInstructionParameter1(i2))) {
                PDEBUG(5, "instsAreDuplicates: 	Parameter 1 does not match\n");
                PDEBUG(5, "instsAreDuplicates: Exit\n");
                return JITFALSE;
            }

            /* Type and all parameters match. */
            return JITTRUE;
    }
    PDEBUG(5, "instsAreDuplicates: 	No parameters\n");
    PDEBUG(5, "instsAreDuplicates: Exit\n");

    /* Shouldn't get here. */
    abort();
    return JITFALSE;
}


/**
 * Get the variable defined by a parameter, or -1 if there is no variable.
 **/
static IR_ITEM_VALUE
getVariableFromParam(ir_item_t *param) {
    if (param->type == IROFFSET) {
        return param->value.v;
    }
    return -1;
}


/**
 * Check that all definers of a source variable in the first instruction
 * reach the second instruction and that no extra definers reach the second
 * that don't reach the first.  The argument paramNum is 1-based, to coincide
 * with IRMETHOD_getInstructionParameter().
 **/
static JITBOOLEAN
variableDefinersMatch(ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2, int paramNum) {
    JITUINT32 numBlockBits, blockBit, numBlocks, block;
    IR_ITEM_VALUE srcVar;

    /* Get the source variable. */
    srcVar = getVariableFromParam(IRMETHOD_getInstructionParameter(inst1, paramNum));
    if (srcVar == -1) {
        return JITTRUE;
    }

    /* Find the definitions that reach each instruction. */
    numBlockBits = 8 * sizeof(JITNINT);
    numBlocks = method->reachingDefinitionsBlockNumbers;
    for (block = 0; block < numBlocks; ++block) {
        JITNUINT defBlock = ir_instr_reaching_definitions_in_get(method, inst1, block);
        JITNUINT reachedSecond = 0;

        /* Definitions reaching the first instruction. */
        if (defBlock) {
            for (blockBit = 0; blockBit < numBlockBits; ++blockBit) {
                if (defBlock & (1 << blockBit)) {
                    JITUINT32 defInstPos = block * numBlockBits + blockBit;
                    IR_ITEM_VALUE defVar;
                    assert(defInstPos < IRMETHOD_getInstructionsNumber(method));

                    /* Get the defined variable ID. */
                    if (defInstPos < IRMETHOD_getMethodParametersNumber(method)) {
                        defVar = defInstPos;
                    } else {
                        ir_instruction_t *defInst = IRMETHOD_getInstructionAtPosition(method, defInstPos);
                        defVar = IRMETHOD_getIRItemOfVariableDefinedByInstruction(method, defInst)->value.v;
                    }

                    /* Only consider those defining a source variable. */
                    if (defVar == srcVar) {

                        /* Must reach the second instruction too. */
                        if (!ir_instr_reaching_definitions_reached_in_is(method, inst2, defInstPos)) {
                            /*               PDEBUG(0, "    Defining instruction %u doesn't reach the second instruction\n", defInstPos); */
                            return JITFALSE;
                        } else {
                            reachedSecond |= 1 << blockBit;
                        }
                    }
                }
            }
        }

        /* Ensure that no other definers reach the second instruction. */
        defBlock = ir_instr_reaching_definitions_in_get(method, inst2, block);
        if (defBlock) {
            for (blockBit = 0; blockBit < numBlockBits; ++blockBit) {
                if (defBlock & (1 << blockBit)) {
                    JITUINT32 defInstPos = block * numBlockBits + blockBit;
                    IR_ITEM_VALUE defVar;
                    assert(defInstPos < IRMETHOD_getInstructionsNumber(method));

                    /* Get the defined variable ID. */
                    if (defInstPos < IRMETHOD_getMethodParametersNumber(method)) {
                        defVar = defInstPos;
                    } else {
                        ir_instruction_t *defInst = IRMETHOD_getInstructionAtPosition(method, defInstPos);
                        defVar = IRMETHOD_getIRItemOfVariableDefinedByInstruction(method, defInst)->value.v;
                    }

                    /* Defines a source variable and not seen already. */
                    if (defVar == srcVar && (reachedSecond & (1 << blockBit)) == 0) {
                        /*             PDEBUG(0, "    Defining instruction %u reaches the second instruction, but not the first\n", defInstPos); */
                        return JITFALSE;
                    }
                }
            }
        }
    }

    /* Checks passed. */
    return JITTRUE;
}


/**
 * Check whether the first instruction could be a definer of one of the second
 * instruction's source variables.  The argument paramNum is 1-based, to
 * coincide with IRMETHOD_getInstructionParameter().
 **/
static JITBOOLEAN
variableDefinerMightBe(ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2, int paramNum) {
    IR_ITEM_VALUE srcVar;
    IR_ITEM_VALUE defVar;
    JITUINT32 inst1Pos;

    /* Checks. */
    inst1Pos = IRMETHOD_getInstructionPosition(inst1);
    assert(inst1Pos >= IRMETHOD_getMethodParametersNumber(method));
    assert(inst1Pos < IRMETHOD_getInstructionsNumber(method));

    /* Get the source variable. */
    srcVar = getVariableFromParam(IRMETHOD_getInstructionParameter(inst2, paramNum));
    if (srcVar == -1) {
        return JITFALSE;
    }

    /* Check for a matching definition. */
    defVar = getVariableFromParam(IRMETHOD_getInstructionResult(inst1));
    if (defVar == srcVar) {
        if (ir_instr_reaching_definitions_reached_in_is(method, inst2, inst1Pos)) {
            return JITTRUE;
        }
    }

    /* Checks passed. */
    return JITFALSE;
}


/**
 * Determine whether a load and store access the same address in memory so that
 * the load can be replaced by a copy from the stored variable to the loaded
 * one.  For this to happen the store must predominate the load, the base
 * parameters must match (same type and variable or constant), the offsets must
 * be the same and all definitions of the base address variable that reach the
 * store must also reach the load.
 **/
static JITBOOLEAN
accessSameMemoryAddress(ir_method_t *method, ir_instruction_t *load, ir_instruction_t *store) {
    ir_item_t *loadBase;
    ir_item_t *loadResult;
    ir_item_t *storeBase;
    ir_item_t *storeValue;
    /*   PDEBUG(0, "  Checking prior store %u\n", store->ID); */

    /* Both base addresses must be the same variable or constant. */
    loadBase = IRMETHOD_getInstructionParameter1(load);
    storeBase = IRMETHOD_getInstructionParameter1(store);
    if (!parametersMatch(loadBase, storeBase)) {
        /*     PDEBUG(0, "    Base parameters don't match\n"); */
        return JITFALSE;
    }

    /* Types loaded an stored must be the same. */
    loadResult = IRMETHOD_getInstructionResult(load);
    storeValue = IRMETHOD_getInstructionParameter3(store);
    if (!typesMatch(loadResult, storeValue)) {
        /*     PDEBUG(0, "    Types don't match\n"); */
        return JITFALSE;
    }

    /* All definitions of the base addresses must reach both instructions. */
    if (loadBase->type == IROFFSET) {
        if (!variableDefinersMatch(method, store, load, 1)) {
            /*       PDEBUG(0, "    Variable definers don't match\n"); */
            return JITFALSE;
        }
    }

    /* Offsets must match. */
    if (load->type == IRLOADREL && store->type == IRSTOREREL) {
        ir_item_t *loadOffset = IRMETHOD_getInstructionParameter2(load);
        ir_item_t *storeOffset = IRMETHOD_getInstructionParameter2(store);
        if (!parametersMatch(loadOffset, storeOffset)) {
            /*       PDEBUG(0, "    Offset parameters don't match\n"); */
            return JITFALSE;
        }
    }

    /* All checks passed. */
    /*   PDEBUG(0, "    All checks passed\n"); */
    return JITTRUE;
}


/**
 * Get a memory store instruction that stores to the same address as the given
 * load instruction loads from, along all control flow paths, so that the load
 * can be replaced by a copy from the store's stored variable to the loads
 * destination variable.
 *
 * We use available expressions analysis to tell us the stores that are
 * available at the load and constrain the store to be a predominator of the
 * load, to keep the analysis as simple as possible.
 **/
static ir_instruction_t *
findPriorStoreToSameAddress(ir_method_t *method, ir_instruction_t *load) {
    JITUINT32 numBlockBits, blockBit, numBlocks, block;
    /*   PDEBUG(0, "Checking for prior store for load %u\n", load->ID); */

    /* Compute the size of the bitmaps for uses and defines. */
    numBlockBits = 8 * sizeof(JITNINT);

    /* Find the expressions that are available. */
    numBlocks = method->availableExpressionsBlockNumbers;
    for (block = 0; block < numBlocks; ++block) {
        JITNUINT exprBlock = ir_instr_available_expressions_in_get(method, load, block);

        /* Check expressions within this block. */
        if (exprBlock) {
            for (blockBit = 0; blockBit < numBlockBits; ++blockBit) {

                /* Check whether this expression is available. */
                if (exprBlock & (1 << blockBit)) {
                    JITUINT32 storePos = block * numBlockBits + blockBit;

                    /* This position must be valid. */
                    if (storePos < IRMETHOD_getInstructionsNumber(method)) {
                        ir_instruction_t *store = IRMETHOD_getInstructionAtPosition(method, storePos);
                        assert(store);

                        /* Must be a store. */
                        if (store->type == IRSTOREREL){

                            /* Store must predominate the load. */
                            if (IRMETHOD_isInstructionAPredominator(method, store, load)) {

                                /* Must access the same memory address. */
                                if (accessSameMemoryAddress(method, load, store)) {
                                    PDEBUG(0, "  Load %u redundant due to store %u\n", load->ID, store->ID);
                                    return store;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* No store found. */
    return NULL;
}


/**
 * Remove a redundant load because it loads from the same memory address as a
 * prior store.  Simply place a new instruction after the store that copies from
 * the variable stored into memory to a new variable.  The load is not removed
 * but replaced by another store from the new variable to the load's destination
 * variable.
 **/
static void
removeRedundantLoad(ir_method_t *method, remove_load_t *entry) {
    ir_item_t *defParam;
    ir_item_t intermediateParam;
    ir_instruction_t *copyInst;

    /* Debugging. */
    PDEBUG(0, "Removing redundant load %u\n", IRMETHOD_getInstructionPosition(entry->load));

    /* The parameter that is finally defined. */
    defParam = IRMETHOD_getIRItemOfVariableDefinedByInstruction(method, entry->load);
    assert(defParam->type != NOPARAM);
    assert(defParam->type == IROFFSET);

    /* The new variable that is written to. */
    memset(&intermediateParam, 0, sizeof(ir_item_t));
    intermediateParam.value.v = IRMETHOD_newVariableID(method);
    intermediateParam.type = IROFFSET;
    intermediateParam.internal_type = defParam->internal_type;
    intermediateParam.type_infos = defParam->type_infos;

    /* A new instruction after the store. */
    copyInst = IRMETHOD_newInstructionAfter(method, entry->store);
    IRMETHOD_setInstructionType(method, copyInst, IRMOVE);
    IRMETHOD_cpInstructionResult(copyInst, &intermediateParam);
    IRMETHOD_cpInstructionParameter1(copyInst, IRMETHOD_getInstructionParameter3(entry->store));

    /* A new instruction instead of the load. */
    copyInst = IRMETHOD_newInstructionBefore(method, entry->load);
    IRMETHOD_setInstructionType(method, copyInst, IRMOVE);
    IRMETHOD_cpInstructionResult(copyInst, defParam);
    IRMETHOD_cpInstructionParameter1(copyInst, &intermediateParam);

    /* Remove the load. */
    //fprintf(stderr, "CSE2 %s %s\n", IRMETHOD_getInstructionTypeName(entry->load->type), IRMETHOD_getSignatureInString(method));
    IRMETHOD_deleteInstruction(method, entry->load);
}


/**
 * Add two generators to a hash table recording whether they match or not in a
 * single direction.
 **/
static void
cacheDirectGeneratorMatch(XanHashTable *generatorsMatchCache, ir_instruction_t *inst1, ir_instruction_t *inst2, JITBOOLEAN match) {
    JITINT32 isMatch = ((JITINT32)match) + 1;
    XanHashTable *innerTable = xanHashTable_lookup(generatorsMatchCache, inst1);
    if (!innerTable) {
        innerTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        xanHashTable_insert(generatorsMatchCache, inst1, innerTable);
    }
    assert(isMatch > 0);
    assert(xanHashTable_lookup(innerTable, inst2) == NULL);
    xanHashTable_insert(innerTable, inst2, (void *)(JITNINT)isMatch);
}


/**
 * Add two generators to a hash table recording whether they match or not.
 **/
static void
cacheGeneratorsMatch(XanHashTable *generatorsMatchCache, ir_instruction_t *inst1, ir_instruction_t *inst2, JITBOOLEAN match) {
    cacheDirectGeneratorMatch(generatorsMatchCache, inst1, inst2, match);
    if (inst1 != inst2) {
        cacheDirectGeneratorMatch(generatorsMatchCache, inst2, inst1, match);
    }
}


/**
 * Return the cached value of whether two expression generators match.  A 0
 * means there was no cached value, 1 is no match and 2 is a match.
 **/
static inline JITINT32
getCachedGeneratorMatch(XanHashTable *generatorsMatchCache, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    XanHashTable *innerTable = xanHashTable_lookup(generatorsMatchCache, inst1);
    if (innerTable == NULL) {
        return 0;
    }
    return (JITINT32)(JITNINT)xanHashTable_lookup(innerTable, inst2);
}


/**
 * Free memory used by a table caching whether expression generators match.
 **/
static void
freeGeneratorMatchCache(XanHashTable *generatorsMatchCache) {
    XanHashTableItem *innerItem = xanHashTable_first(generatorsMatchCache);
    while (innerItem) {
        xanHashTable_destroyTable(innerItem->element);
        innerItem = xanHashTable_next(generatorsMatchCache, innerItem);
    }
    xanHashTable_destroyTable(generatorsMatchCache);
}


/**
 * Print out a removal entry.
 **/
#ifdef PRINTDEBUG
static void
printRemovalEntry(removal_entry_t *entry) {
    XanListItem *instItem = xanList_first(entry->duplicated);
    PDEBUG(2, "Instruction %u can be eliminated\n", entry->inst->ID);
    PDEBUG(2, "  Duplicates:");
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        PDEBUGC(2, " %u", inst->ID);
        instItem = instItem->next;
    }
    PDEBUGC(2, "\n");
}
#endif


/**
 * Check whether a generator can be removed.
 **/
static JITBOOLEAN
canRemoveGenerator(ir_method_t *method, ir_instruction_t *inst) {
    if (IRMETHOD_hasInstructionEscapes(method, inst)) {
        return JITFALSE;
    } else if (IRMETHOD_isAMathInstruction(inst) ||
               IRMETHOD_isAConversionInstruction(inst) ||
               IRMETHOD_isACompareInstruction(inst)) {
        return JITTRUE;
    } else {
        switch (inst->type) {
            case IRLOADREL:
            case IRARRAYLENGTH:
            case IRSIZEOF:
            case IRSTOREREL:
            case IRGETADDRESS:
                return JITTRUE;
        }
    }
    return JITFALSE;
}


/**
 * Check whether a generator requires copy operations when removing.
 **/
static JITBOOLEAN
generatorRequiresCopyOperation(ir_instruction_t *inst) {
    switch (inst->type) {
        case IRSTOREREL:
        case IRMOVE:
        case IRCHECKNULL:
            return JITFALSE;
    }
    return JITTRUE;
}


/**
 * Pass over all instructions and mark matches with other generators the
 * available expressions analysis says are available.
 **/
static void
identifyGeneratorsToRemove(cse_info_t *info, XanList *removeGeneratorsList, XanList *removeLoadsList) {
    JITUINT32 i, blockNum, bit;
    JITUINT32 numInsts = IRMETHOD_getInstructionsNumber(info->method);
    JITUINT32 numBlocks = info->method->availableExpressionsBlockNumbers;
    XanHashTable *generatorsMatchCache = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    PDEBUG(1, "Identifying generators to remove\n");
    for (i = 0; i < numInsts; ++i) {

        /* Fetch a generator that can be potentially removed.
         */
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(info->method, i);
        if (!canRemoveGenerator(info->method, inst)) {
            continue ;
        }
        XanList *matchingGenerators = xanList_new(allocFunction, freeFunction, NULL);

        /* Build up matching available expression generators. */
        for (blockNum = 0; blockNum < numBlocks; ++blockNum) {

            /* Check where the value generated by the current generator is available.
             */
            JITNUINT inBlock = ir_instr_available_expressions_in_get(info->method, inst, blockNum);
            if (inBlock == 0) {
                continue ;
            }
            for (bit = 0; bit < sizeof(JITNUINT) * 8; ++bit) {
                if ((inBlock & (1 << bit)) == 0) {
                    continue ;
                }

                /* The value generated by the current instruction, which is called otherGenerator, is available at the current generator, which is called inst.
                 */
                ir_instruction_t *otherGenerator = IRMETHOD_getInstructionAtPosition(info->method, blockNum * sizeof(JITNUINT) * 8 + bit);

                /* Check if otherGenerator does the same job as the current generator.
                 */
                if (otherGenerator->type != inst->type) {
                    continue ;
                }
                JITINT32 cachedMatch = getCachedGeneratorMatch(generatorsMatchCache, inst, otherGenerator);
                PDEBUG(3, "Generator %u is available at instruction %u\n", otherGenerator->ID, inst->ID);

                /* There is no cached match. */
                if (cachedMatch == 0) {
                    JITBOOLEAN match = instsAreDuplicates(inst, otherGenerator);
                    cacheGeneratorsMatch(generatorsMatchCache, inst, otherGenerator, match);
                    cachedMatch = ((JITINT32)match) + 1;
                }

                /* The expressions do match. */
                if (cachedMatch == 2) {
                    PDEBUG(4, "Generator matches\n");
                    if (IRMETHOD_canInstructionBeReachedFromPosition(info->method, inst, otherGenerator)) {
                        xanList_append(matchingGenerators, otherGenerator);
                        PDEBUG(2, "Generator %u reaches %u\n", otherGenerator->ID, inst->ID);
                    }
                }
            }
        }

        /* Determine whether this instruction can be removed. */
        if (xanList_length(matchingGenerators) > 0) {
            JITUINT32 p, numSrcs = IRMETHOD_getInstructionParametersNumber(inst);
            JITBOOLEAN eliminateInstruction = JITTRUE;
            XanListItem *generatorItem = xanList_first(matchingGenerators);

            /* Check all variables match for all generators. */
            while (generatorItem && eliminateInstruction) {
                ir_instruction_t *priorGenerator = generatorItem->data;
                for (p = 1; p <= numSrcs && eliminateInstruction; ++p) {
                    if (!variableDefinersMatch(info->method, priorGenerator, inst, p) || variableDefinerMightBe(info->method, priorGenerator, inst, p)) {
                        eliminateInstruction = JITFALSE;
                    }
                }
                generatorItem = generatorItem->next;
            }

            /* Mark this instruction for removal. */
            if (eliminateInstruction) {
                removal_entry_t *entry = allocFunction(sizeof(removal_entry_t));
                entry->inst = inst;
                entry->duplicated = matchingGenerators;
                assert(xanList_length(entry->duplicated) > 0);
                xanList_append(removeGeneratorsList, entry);
                PDECL(printRemovalEntry(entry));
            } else {
                xanList_destroyList(matchingGenerators);

                /* Check for a load with a prior store to the same address. */
                if (inst->type == IRLOADREL) {
                    ir_instruction_t *priorStore = findPriorStoreToSameAddress(info->method, inst);
                    if (priorStore) {
                        remove_load_t *entry = allocFunction(sizeof(remove_load_t));
                        entry->load = inst;
                        entry->store = priorStore;
                        xanList_append(removeLoadsList, entry);
                    }
                }
            }
        } else {
            xanList_destroyList(matchingGenerators);
        }
    }

    /* Clean up. */
    freeGeneratorMatchCache(generatorsMatchCache);
}


/**
 * Insert a copy operation after an instruction to copy its result value into
 * another variable.
 **/
static void
insertCopyFromResultToVarAfterInstruction(ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE toVar) {
    ir_instruction_t *copyInst;
    assert(IRMETHOD_getInstructionResultType(inst) == IROFFSET);
    copyInst = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRMOVE);
    IRMETHOD_cpInstructionParameter(method, inst, 0, copyInst, 1);
    IRMETHOD_cpInstructionParameter(method, inst, 0, copyInst, 0);
    IRMETHOD_setInstructionResultValue(method, copyInst, toVar);
    assert(IRMETHOD_getInstructionResultType(copyInst) == IROFFSET);
    PDEBUG(3, "Created new move instruction %u\n", copyInst->ID);
}


/**
 * Insert a copy operation before an operation to copy from a variable into its
 * result variable.
 **/
static void
insertCopyFromVarToResultBeforeInstruction(ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE toVar) {
    ir_instruction_t *copyInst;
    assert(IRMETHOD_getInstructionResultType(inst) == IROFFSET);
    copyInst = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRMOVE);
    IRMETHOD_cpInstructionParameter(method, inst, 0, copyInst, 0);
    IRMETHOD_cpInstructionParameter(method, inst, 0, copyInst, 1);
    IRMETHOD_setInstructionParameter1Value(method, copyInst, toVar);
    assert(IRMETHOD_getInstructionParameter1Type(copyInst) == IROFFSET);
    PDEBUG(3, "Created new move instruction %u\n", copyInst->ID);
}


/**
 * Insert copy operations and remove instructions that duplicate other
 * expression generators.
 **/
static XanHashTable *
eliminateGenerators(cse_info_t *info, XanList *removalList) {
    JITUINT32 numMethodInsts = IRMETHOD_getInstructionsNumber(info->method);
    XanHashTable *instsToDelete = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    XanListItem *removeItem = xanList_first(removalList);
    XanHashTableItem *instItem;

    /* Add in all copy operations. */
    while (removeItem) {
        removal_entry_t *entry = removeItem->data;
        JITUINT32 duplicatedGenerators = xanList_length(entry->duplicated);
        assert(duplicatedGenerators > 0);

        /* Some instructions don't need copy operations. */
        if (generatorRequiresCopyOperation(entry->inst)) {
            XanListItem *generatorItem = xanList_first(entry->duplicated);
            IR_ITEM_VALUE throughVar = IRMETHOD_newVariableID(info->method);

            /* Check the method doesn't get too large. */
            if (info->maxMethodInsts > 0 &&
                    numMethodInsts + duplicatedGenerators > info->maxMethodInsts) {
                break;
            }

            /* Add copy operations after the generators. */
            while (generatorItem) {
                ir_instruction_t *generator = generatorItem->data;
                insertCopyFromResultToVarAfterInstruction(info->method, generator, throughVar);
                generatorItem = generatorItem->next;
            }

            /* A new copy into the eliminated variable. */
            insertCopyFromVarToResultBeforeInstruction(info->method, entry->inst, throughVar);
            numMethodInsts += duplicatedGenerators;
        }

        /* Record this instruction to delete. */
        assert(xanHashTable_lookup(instsToDelete, entry->inst) == NULL);
        xanHashTable_insert(instsToDelete, entry->inst, entry->inst);
        removeItem = removeItem->next;
    }

    /* Actually remove the generators now. */
    instItem = xanHashTable_first(instsToDelete);
    while (instItem) {
        ir_instruction_t *inst = instItem->element;
        IRMETHOD_deleteInstruction(info->method, inst);
        instItem = xanHashTable_next(instsToDelete, instItem);
    }

    /* Debugging output. */
    if (xanHashTable_elementsInside(instsToDelete) > 0) {
        PDEBUG(0, "Eliminated %u instructions from %s\n", xanHashTable_elementsInside(instsToDelete), IRMETHOD_getCompleteMethodName(info->method));
    }
    return instsToDelete;
}


/**
 * Delete redundant loads from a list.
 **/
static void
eliminateRedundantLoads(cse_info_t *info, XanList *removalList, XanHashTable *deletedInsts) {
    JITUINT32 eliminatedLoads = 0;
    JITUINT32 numMethodInsts = IRMETHOD_getInstructionsNumber(info->method);
    XanListItem *removeItem = xanList_first(removalList);
    while (removeItem) {
        remove_load_t *entry = removeItem->data;
        if (xanHashTable_lookup(deletedInsts, entry->load) != NULL || xanHashTable_lookup(deletedInsts, entry->store) != NULL) {

            /* Check the method doesn't get too large. */
            if (info->maxMethodInsts > 0 &&
                    numMethodInsts + 1 > info->maxMethodInsts) {
                break;
            }

            /* OK to go ahead with the removal. */
            removeRedundantLoad(info->method, entry);
            xanHashTable_insert(deletedInsts, entry->load, entry->load);
            eliminatedLoads += 1;
        }
        removeItem = removeItem->next;
    }

    /* Debugging output. */
    if (eliminatedLoads > 0) {
        PDEBUG(0, "Eliminated %u loads from %s\n", eliminatedLoads, IRMETHOD_getCompleteMethodName(info->method));
    }
}


/**
 * Free memory used by a removal list.
 **/
static void
freeRemovalList(XanList *removalList, JITBOOLEAN removalEntry) {
    XanListItem *removalItem = xanList_first(removalList);
    while (removalItem) {
        if (removalEntry) {
            removal_entry_t *entry = removalItem->data;
            xanList_destroyList(entry->duplicated);
        }
        freeFunction(removalItem->data);
        removalItem = removalItem->next;
    }
    xanList_destroyList(removalList);
}


/**
 * Create the structure containing unchanging information for this pass.
 **/
static cse_info_t *
init(ir_method_t *method) {
    char *env;
    cse_info_t *info = allocFunction(sizeof(cse_info_t));
    info->method = method;
    env = getenv("CSE_MAX_METHOD_INSTRUCTIONS");
    if (env) {
        info->maxMethodInsts = atoi(env);
    } else {
        info->maxMethodInsts = 0;
    }
    env = getenv("CSE_MAX_GROWTH_FACTOR");
    if (env) {
        JITUINT32 maxInstsGrowth = IRMETHOD_getInstructionsNumber(method);
        info->maxGrowthFactor = atof(env);
        maxInstsGrowth = ceil(maxInstsGrowth * info->maxGrowthFactor);
        if (info->maxMethodInsts == 0 || info->maxMethodInsts > maxInstsGrowth) {
            info->maxMethodInsts = maxInstsGrowth;
        }
    } else {
        info->maxGrowthFactor = 0.0;
    }
    env = getenv("CSE_DUMP_CFG");
    if (env) {
        info->dumpCfg = atoi(env);
    } else {
        info->dumpCfg = 0;
    }
    env = getenv("CSE_PRINT_DEBUG_LEVEL");
    if (env) {
        PDEBUGLEVEL(atoi(env));
    }
    return info;
}


/**
 * The main part of the common subexpression elimination pass.  The algorithm
 * passes over all instructions recording the expressions that they generate.
 * A second pass identifies instructions duplicating expression generation and
 * inserts the code to bypass them and remove them.
 **/
static void
commonSubexpressionElimination(ir_method_t *method) {
    cse_info_t *info;
    XanHashTable *deletedInsts;
    XanList *removeGeneratorsList;
    XanList *removeLoadsList;
    char *prevDotPrinterName;

    /* Create the structure containing all information for this pass. */
    info = init(method);

    /* Debugging method dumping. */
    prevDotPrinterName = getenv("DOTPRINTER_FILENAME");
    PDEBUGNL(0, "Optimising %s\n", IRMETHOD_getCompleteMethodName(method));
    setenv("DOTPRINTER_FILENAME", "cse", 1);
    if (info->dumpCfg > 0) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
    }

    /* Create the list of expression generators to remove. */
    removeGeneratorsList = xanList_new(allocFunction, freeFunction, NULL);
    removeLoadsList = xanList_new(allocFunction, freeFunction, NULL);
    identifyGeneratorsToRemove(info, removeGeneratorsList, removeLoadsList);

    /* Remove the generators. */
    deletedInsts = eliminateGenerators(info, removeGeneratorsList);

    /* Remove any loads. */
    eliminateRedundantLoads(info, removeLoadsList, deletedInsts);

    /* Debugging method dumping. */
    if (info->dumpCfg > 0) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
    }

    /* Clean up. */
    freeRemovalList(removeGeneratorsList, JITTRUE);
    freeRemovalList(removeLoadsList, JITFALSE);
    xanHashTable_destroyTable(deletedInsts);
    freeFunction(info);

    if (prevDotPrinterName != NULL) {
        setenv("DOTPRINTER_FILENAME", prevDotPrinterName, 1);
    }
}


/**
 * Entry to the common subexpression elimination pass.  Checks there is work
 * to be done and then starts the main algorithm.
 */
static inline void cse_do_job (ir_method_t *method) {
    char *env;

    /* Checks. */
    assert(method != NULL);

    /* Ensure there is work to be done. */
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return;
    }

    /* See if we need to control the maximum number of instructions. */
    env = getenv("CSE_MAX_METHOD_INSTRUCTIONS");
    if (env) {
        JITUINT32 maxInstructions = atoi(env);
        if (IRMETHOD_getInstructionsNumber(method) > maxInstructions) {
            return;
        }
    }

    /* Don't bother with some methods (for debugging). */
    /* if (IRMETHOD_getCompleteMethodName(method)[0] != 'v') { */
    /*   PDEBUG(0, "Skipping %s\n", IRMETHOD_getCompleteMethodName(method)); */
    /*   return; */
    /* } */

    /* Eliminate common subexpressions. */
    commonSubexpressionElimination(method);

    /* Return				*/
    return;
}

void cse_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    /* Assertions            */
    assert(lib != NULL);
    assert(optimizer != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    irOutputPrefix = outputPrefix;
}

void cse_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    irOutputPrefix = NULL;
}

static inline JITUINT64 get_ID_job (void) {
    return COMMON_SUBEXPRESSIONS_ELIMINATOR;
}

JITUINT64 get_dependences (void) {
    return AVAILABLE_EXPRESSIONS_ANALYZER | REACHING_DEFINITIONS_ANALYZER | PRE_DOMINATOR_COMPUTER | REACHING_INSTRUCTIONS_ANALYZER;
}

char * get_version (void) {
    return VERSION;
}

char * get_informations (void) {
    return INFORMATIONS;
}

char * get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 cse_get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

static inline void cse_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void cse_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
