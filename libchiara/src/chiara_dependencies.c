/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#include <stdlib.h>
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
#include <chiara_tools.h>
// End

static inline JITBOOLEAN internal_isIOMethod (ir_method_t *m);
static inline JITBOOLEAN internal_isAnInvariantWithinLoop (loop_t *loop, IR_ITEM_VALUE varID, XanHashTable *variableInvariantsWithinLoop);
static inline void internal_addDataDependence_helpfunction (XanList *depToCheck, XanHashTable **depToCheckTable, loop_inter_interation_data_dependence_t *dep);
static inline JITINT32 internal_isALibraryCallDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep);
static inline JITBOOLEAN internal_doesCallALibrary (ir_method_t *m, ir_instruction_t *inst);
static inline JITBOOLEAN internal_variablesHaveDifferentValues (ir_method_t *m, IR_ITEM_VALUE var1, IR_ITEM_VALUE var2);
static inline JITBOOLEAN internal_doesInstructionAccessAGlobalVariable (ir_instruction_t *inst);
static inline JITINT32 internal_isGlobalVariableDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep);
static inline JITBOOLEAN internal_haveVariablesDifferentValuesAlways (loop_t *loop, IR_ITEM_VALUE var1, IR_ITEM_VALUE var2, JITBOOLEAN *cannotChangeVars, XanHashTable *variableDefinitionsWithinLoop);
static inline JITBOOLEAN internal_isDominationRelationCorrectForLocalization (loop_t *loop, ir_instruction_t *inst1, ir_instruction_t *inst2, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts);
static inline JITBOOLEAN * internal_getCannotBeAliasVariables (loop_t *loop);
static inline JITBOOLEAN * internal_getCannotChangeVariables (loop_t *loop, XanHashTable *variableDefinitionsWithinLoop);
static inline JITBOOLEAN internal_isMethodThreadSafe (ir_method_t *method);
static inline JITINT32 internal_isFalseMemoryManagement (loop_t *loop, ir_method_t *method, loop_inter_interation_data_dependence_t *dep);
static inline JITBOOLEAN internal_dependenceDueToThreadSafeCall (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dependentInst, JITUINT16 depType);
static inline JITBOOLEAN internal_directDependenceDueToThreadSafeCall (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2);
static inline XanHashTable * internal_getDefinitionsWithinLoop (loop_t *loop, IR_ITEM_VALUE varID, XanHashTable *variableDefinitionsWithinLoop);
static inline JITINT32 * internal_findMemoryLocationStatus (loop_t *loop);
static inline XanList * internal_getStraightRAWChainForVariableLocalization (loop_t *loop, ir_instruction_t *startInst);
static inline JITBOOLEAN internal_doesVariableStrictlyGrowWithIterationCount (loop_t *loop, IR_ITEM_VALUE varID, XanList *instsToCheck, JITINT32 *memoryLocations, JITBOOLEAN *cannotChangeVars, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop);
static inline JITBOOLEAN internal_doesVariableGrowWithIterationCount (loop_t *loop, IR_ITEM_VALUE varID, JITBOOLEAN *strict, XanList *instsToCheck, JITINT32 *memoryLocations, JITBOOLEAN *cannotChangeVars, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop);
static inline JITBOOLEAN internal_doesMemoryAddressChangeAtEveryIteration (loop_t *loop, IR_ITEM_VALUE varID, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop);
static inline JITBOOLEAN internal_doesMemoryAddressChangeAtEveryIterationForTheDefinition (loop_t *loop, IR_ITEM_VALUE varID, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, ir_instruction_t *def);
static inline void internal_checkAndAddDataDependence (ir_instruction_t *inst, data_dep_arc_list_t *dep, loop_t *loop, XanList *depToCheck, XanHashTable **depToCheckTable, JITBOOLEAN *uniqueValueVars, JITINT32 *memoryLocations, JITBOOLEAN *cannotBeAliasVars, JITBOOLEAN *cannotChangeVars, XanList *loopSuccessors, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop, XanHashTable *executedInsts);
static inline JITBOOLEAN internal_isThereALoopCarriedDataDependence (ir_instruction_t *instDst, ir_instruction_t *instSrc, JITUINT16 depType, loop_t *loop, JITBOOLEAN *uniqueValueVars, JITINT32 *memoryLocations, JITBOOLEAN *cannotBeAliasVars, JITBOOLEAN *cannotChangeVars, XanList *loopSuccessors, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, JITBOOLEAN dataDependencesAreCorrect, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop, XanHashTable *executedInsts);
static inline void freeDataDependenceToCheck (void *dataDep);
static inline void * cloneDataDependenceToCheck (void *dataDep);
static inline JITINT32 internal_isLiveOutAtExitPoint (loop_t *loop, ir_instruction_t *inst, XanList *loopSuccessors);
static inline JITINT32 internal_isLiveOutAtLoopEntryPoint (loop_t *loop, ir_instruction_t *inst);
static inline void internal_addDataDependence (loop_t *loop, ir_instruction_t *inst, data_dep_arc_list_t *dep, XanList *depToCheck, XanHashTable **depToCheckTable, JITBOOLEAN addSymmetry);
static inline JITBOOLEAN internal_doesExistADataDependenceOfTypeWithinLoop (loop_t *loop, ir_instruction_t *inst, JITUINT16 depType);
static inline loop_inter_interation_data_dependence_t * internal_exist_data_dependence (XanList *depToCheck, XanHashTable **depToCheckTable, ir_instruction_t *inst1, ir_instruction_t *inst2);
static inline loop_inter_interation_data_dependence_t * internal_exist_direct_data_dependence (XanList *depToCheck, XanHashTable **depToCheckTable, ir_instruction_t *inst1, ir_instruction_t *inst2);
static inline void internal_categorizeDataDependencies (loop_t *loop, XanList *depToCheck, JITBOOLEAN *uniqueValueVars, JITBOOLEAN *cannotBeAliasVariables, XanList *loopSuccessors, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *executedInsts);
static inline JITINT32 internal_isEscapesMemoryFalseDataDependence (ir_method_t *method, loop_inter_interation_data_dependence_t *dep);
static inline JITINT32 internal_isWAWRegisterReassociation (loop_t *loop, ir_method_t *method, loop_inter_interation_data_dependence_t *dep, XanList *loopSuccessors);
static inline JITINT32 internal_isInputOutput (ir_method_t *method, loop_inter_interation_data_dependence_t *dep);
static inline JITINT32 internal_isMemoryWAWRegisterReassociation (loop_t *loop, ir_method_t *method, loop_inter_interation_data_dependence_t *dep, JITBOOLEAN *cannotBeAliasVariables);
static inline JITINT32 internal_isUniqueValueVariableDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, JITBOOLEAN *uniqueValueVars);
static inline JITINT32 internal_isFalseUniqueValueVariableDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, JITBOOLEAN *uniqueValueVars);
static inline JITINT32 internal_isFalseMemoryRenamingDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, JITINT32 **predoms, JITUINT32 numInsts);
static inline JITINT32 internal_isFalseInductiveVariableDataDependence (ir_method_t *method, loop_inter_interation_data_dependence_t *dep);
static inline JITINT32 internal_isAlwaysTrueDataDependence (ir_method_t *method, loop_inter_interation_data_dependence_t *dep);
static inline JITINT32 internal_isLocalizableByRegisterReassociationDataDependence (ir_method_t *method, loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanHashTable *executedInsts, XanList *loopCarriedDeps);
static inline JITINT32 internal_isLocalizableByIntegerRegisterReassociationDataDependence (ir_method_t *method, loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanHashTable *executedInsts, XanList *loopCarriedDeps);
static inline JITINT32 internal_isLocalizableByFloatingPointRegisterReassociationDataDependence (ir_method_t *method, loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanHashTable *executedInsts, XanList *loopCarriedDeps);
static inline void internal_computeConditionToCheck (ir_method_t *method, loop_inter_interation_data_dependence_t *dep);
static inline JITINT32 internal_isEscapesMemoryFalseDataDependenceDirect (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2);
static inline JITBOOLEAN internal_isLocalizableDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanList *depToCheck, JITBOOLEAN *localizedVariables, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts);
static inline JITBOOLEAN internal_isLocalizableDirectDataDependence (loop_t *loop, ir_instruction_t *from, ir_instruction_t *to);
static inline JITBOOLEAN * internal_getUniqueValueVariables (loop_t *loop, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop);
static inline JITBOOLEAN internal_isNotAnArraySubscriptComputation (loop_t *loop, ir_item_t *parOfDefinerOfBaseAddress, XanHashTable *variableDefinitionsWithinLoop);
static inline JITBOOLEAN internal_isDefinedVariableDefinedByMallocAndPredominatesAllUses (loop_t *loop, ir_instruction_t *def, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts);
static inline JITBOOLEAN internal_isDefinedVariableFreeByFreeAndPredominatesBackedges (loop_t *loop, ir_instruction_t *def, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts);
static inline ir_instruction_t * internal_fetchFirstInstructionInBody (loop_t *loop);

extern ir_optimizer_t  	*irOptimizer;

loop_inter_interation_data_dependence_t * LOOPS_getDirectDataDependence (XanList *depToCheck, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    return internal_exist_direct_data_dependence(depToCheck, NULL, inst1, inst2);
}

loop_inter_interation_data_dependence_t * LOOPS_getDataDependence (XanList *depToCheck, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    return internal_exist_data_dependence(depToCheck, NULL, inst1, inst2);
}

XanList * LOOPS_getRAWInstructionChainFromTheSameLoop (loop_t *loop, ir_instruction_t *startInst) {
    return internal_getStraightRAWChainForVariableLocalization(loop, startInst);
}

XanList * LOOPS_getDataDependenciesWithinMethod (ir_method_t *m) {
    XanList                                 *depToCheck;
    JITUINT32				instructionsNumber;
    JITUINT32				instID;

    /* Allocate the memory.
     */
    depToCheck		= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the dependences.
     */
    instructionsNumber	= IRMETHOD_getInstructionsNumber(m);
    for (instID = 0; instID < instructionsNumber; instID++) {
        XanList       		*ddList;
        XanListItem		*item;
        ir_instruction_t	*inst;

        /* Fetch an instruction.
         */
        inst	= IRMETHOD_getInstructionAtPosition(m, instID);

        /* Fetch the dependences from the current instruction.
         */
        ddList 	= IRMETHOD_getDataDependencesFromInstruction(m, inst);
        if (ddList == NULL) {
            continue ;
        }

        /* Transform the list of dependences.
         */
        item 	= xanList_first(ddList);
        while (item != NULL) {
            data_dep_arc_list_t     		*dep;

            /* Fetch the dependence.
             */
            dep 	= item->data;
            assert(dep != NULL);
            assert(dep->inst != NULL);

            /* Add the data dependence.
             */
            if (internal_exist_data_dependence(depToCheck, NULL, inst, dep->inst) == NULL) {
                loop_inter_interation_data_dependence_t	*currentDep;

                /* Allocate the current data dependence.
                 */
                currentDep 		= allocMemory(sizeof(loop_inter_interation_data_dependence_t));
                currentDep->inst1 	= inst;
                currentDep->inst2 	= dep->inst;
                currentDep->type 	= dep->depType;

                /* Add the dependence.
                 */
                xanList_append(depToCheck, currentDep);
            }

            /* Fetch the next element.
             */
            item 	= item->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(ddList);
    }

    return depToCheck;
}

XanList * LOOPS_getDataDependencesWithinLoop (loop_t *loop, XanList *loopCarriedDependences, JITINT32 **predoms, JITINT32 **postdoms, XanHashTable ***depToCheckTable, JITUINT32 numInsts, XanHashTable *executedInsts) {
    XanList                                 *ddList;
    XanList                                 *depToCheck;
    XanList 		                		*loopSuccessors;
    XanListItem                             *item;
    XanListItem                             *item2;
    ir_instruction_t                        *inst;

    /* Assertions						*/
    assert(loop != NULL);
    assert(loop->method != NULL);
    assert(loop->instructions != NULL);

    /* Fetch the list of data dependences across loop iterations.
     */
    if (loopCarriedDependences == NULL) {
        depToCheck 	= LOOPS_getDataDependencesAcrossLoopIterations(loop, predoms, postdoms, depToCheckTable, numInsts, executedInsts);
    } else {
        depToCheck	= xanList_cloneList(loopCarriedDependences);
    }
    assert(depToCheck != NULL);

    /* Fetch the successors of the loop.
     */
    loopSuccessors	= LOOPS_getSuccessorsOfLoop(loop);
    assert(loopSuccessors != NULL);

    /* Check inter loop-iterations data dependences.
     */
    item = xanList_first(loop->instructions);
    while (item != NULL) {

        /* Fetch the instruction.
         */
        inst = item->data;
        assert(inst != NULL);

        /* Check if the instruction should be considered.
         */
        if (    (executedInsts == NULL)                                 ||
                (xanHashTable_lookup(executedInsts, inst) != NULL)      ){

            /* Check if there is a inter loop-iteration data dependence with the current instruction.
             */
            ddList 	= IRMETHOD_getDataDependencesFromInstruction(loop->method, inst);
            if (ddList != NULL) {
                data_dep_arc_list_t     *dep;
                item2 = xanList_first(ddList);
                while (item2 != NULL) {
                    dep = item2->data;
                    assert(dep != NULL);
                    assert(dep->inst != NULL);

                    /* Add the data dependence				*/
                    if (xanList_find(loop->instructions, dep->inst) != NULL) {
                        internal_addDataDependence(loop, inst, dep, depToCheck, *depToCheckTable, JITFALSE);
                    }

                    item2 = item2->next;
                }

                /* Free the memory.
                 */
                xanList_destroyList(ddList);
            }
        }

        /* Fetch the next instruction.
         */
        item = item->next;
    }

    /* Categorize data dependences.
     */
    internal_categorizeDataDependencies(loop, depToCheck, NULL, NULL, loopSuccessors, predoms, postdoms, numInsts, NULL);

    /* Free the memory.
     */
    xanList_destroyList(loopSuccessors);

    return depToCheck;
}

XanHashTable ** LOOPS_getDDGFromDependences (XanList *deps, JITUINT32 numInsts){
    XanHashTable                                    **ddg;
    loop_inter_interation_data_dependence_t         *dd;
    XanListItem					                    *item;

    /* Allocate the DDG.
     */
    ddg = allocFunction(sizeof(XanHashTable *) * (numInsts + 1));
    
    /* Add edges to the DDG.
     */
    item	= xanList_first(deps);
    while (item != NULL) {
        XanHashTable    *t;

        /* Fetch a dependence.
         */
        dd	= item->data;
        assert(dd != NULL);

        /* Fetch the node of the source of the dependence.
         */
        t   = ddg[dd->inst1->ID];
        if (t == NULL){
            t   = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
            ddg[dd->inst1->ID]  = t;
        }
        assert(t != NULL);

        /* Add an edge.
         */
        xanHashTable_insert(t, dd->inst2, dd);

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    return ddg;
}

void LOOPS_keepOnlyLoopCarriedDataDependences (loop_t *loop, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *deps) {
    XanList 				*loopSuccessors;
    XanList					*toDelete;
    XanHashTableItem	    *hashItem;
    XanHashTable            *variableDefinitionsWithinLoop;
    XanHashTable            *variableInvariantsWithinLoop;
    JITBOOLEAN				*uniqueValueVars;
    JITBOOLEAN				*cannotBeAliasVars;
    JITBOOLEAN				*cannotChangeVars;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(loop->method != NULL);
    assert(loop->instructions != NULL);
    PDEBUG("CHIARA: GetDataDependencesAcrossLoopIterations: Start\n");

    /* Allocate the necessary memory.
     */
    toDelete		= xanList_new(allocFunction, freeFunction, NULL);
    variableDefinitionsWithinLoop = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    variableInvariantsWithinLoop = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Compute the dominance relations.
     */
    DOMINANCE_computePostdominance(irOptimizer, loop->method);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, PRE_DOMINATOR_COMPUTER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, ESCAPES_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, REACHING_DEFINITIONS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, REACHING_INSTRUCTIONS_ANALYZER);

    /* Fetch the successors of the loop.
     */
    loopSuccessors		= LOOPS_getSuccessorsOfLoop(loop);
    assert(loopSuccessors != NULL);

    /* Identify the variables that change value at every iteration.
     */
    uniqueValueVars		= internal_getUniqueValueVariables(loop, predoms, postdoms, numInsts, variableDefinitionsWithinLoop);

    /* Identify variables that cannot have aliases.
     */
    cannotBeAliasVars	= internal_getCannotBeAliasVariables(loop);

    /* Identify variables that do not change after being computed in the loop.
     * Notice that this is different then invariant variables.
     */
    cannotChangeVars	= internal_getCannotChangeVariables(loop, variableDefinitionsWithinLoop);

    /* Filter out not loop-carried data dependences.
     */
    hashItem		= xanHashTable_first(deps);
    while (hashItem != NULL) {
        ir_instruction_t	*dependentInst;
        XanHashTable		*depListOfInstructionsTable;
        XanHashTableItem	*hashItem2;

        /* Fetch the dependences.
         */
        dependentInst			= hashItem->elementID;
        depListOfInstructionsTable	= hashItem->element;
        assert(dependentInst != NULL);
        assert(depListOfInstructionsTable != NULL);

        /* Find the dependences that are not loop-carried.
         */
        hashItem2			= xanHashTable_first(depListOfInstructionsTable);
        while (hashItem2 != NULL) {
            JITUINT16 		depType;
            XanList			*possibleDeps;
            XanListItem           	*item;

            /* Fetch the set of instructions that dependentInst might depends on and the type of dependence.
             */
            depType			= (JITUINT16) (JITNUINT) hashItem2->elementID;
            possibleDeps		= hashItem2->element;
            assert(possibleDeps != NULL);
            assert(depType != 0);

            /* Find the dependences that are not loop-carried.
             */
            xanList_emptyOutList(toDelete);
            item		= xanList_first(possibleDeps);
            while (item != NULL) {
                ir_instruction_t	*depInst;

                /* Fetch the instruction that dependentInst might depends from.
                 */
                depInst	= item->data;
                assert(depInst != NULL);

                /* Check the dependence.
                 */
                if (!internal_isThereALoopCarriedDataDependence(dependentInst, depInst, depType, loop, uniqueValueVars, NULL, cannotBeAliasVars, cannotChangeVars, loopSuccessors, predoms, postdoms, numInsts, JITFALSE, variableDefinitionsWithinLoop, variableInvariantsWithinLoop, NULL)) {
                    xanList_append(toDelete, depInst);
                }

                /* Fetch the next element.
                 */
                item	= item->next;
            }

            /* Remove dependences that are not loop-carried.
             */
            xanList_removeElements(possibleDeps, toDelete, JITTRUE);

            /* Fetch the next element.
             */
            hashItem2	= xanHashTable_next(depListOfInstructionsTable, hashItem2);
        }

        /* Fetch the next element.
         */
        hashItem	= xanHashTable_next(deps, hashItem);
    }

    /* Free the memory.
     */
    hashItem    = xanHashTable_first(variableDefinitionsWithinLoop);
    while (hashItem != NULL){
        XanHashTable    *ht;
        ht          = hashItem->element;
        assert(ht != NULL);
        xanHashTable_destroyTable(ht);
        hashItem    = xanHashTable_next(variableDefinitionsWithinLoop, hashItem);
    }
    xanHashTable_destroyTable(variableDefinitionsWithinLoop);
    xanHashTable_destroyTable(variableInvariantsWithinLoop);
    xanList_destroyList(loopSuccessors);
    xanList_destroyList(toDelete);
    freeFunction(uniqueValueVars);
    freeFunction(cannotBeAliasVars);
    freeFunction(cannotChangeVars);

    return ;
}

XanList * LOOPS_getDataDependencesAcrossLoopIterations (loop_t *loop, JITINT32 **predoms, JITINT32 **postdoms, XanHashTable ***depToCheckTable, JITUINT32 numInsts, XanHashTable *executedInsts) {
    XanList                                 *ddList;
    XanList                                 *depToCheck;
    XanList 				                *loopSuccessors;
    XanListItem                             *item;
    XanListItem                             *item2;
    XanHashTable                            *variableDefinitionsWithinLoop;
    XanHashTable                            *variableInvariantsWithinLoop;
    XanHashTableItem                        *hashItem;
    ir_instruction_t                        *inst;
    JITBOOLEAN				                *uniqueValueVars;
    JITBOOLEAN				                *cannotBeAliasVars;
    JITBOOLEAN				                *cannotChangeVars;
    JITINT32				                *memoryLocations;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(loop->method != NULL);
    assert(loop->instructions != NULL);
    assert(depToCheckTable != NULL);
    PDEBUG("CHIARA: GetDataDependencesAcrossLoopIterations: Start\n");

    /* Compute the dominance relations.
     */
    DOMINANCE_computePostdominance(irOptimizer, loop->method);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, PRE_DOMINATOR_COMPUTER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, ESCAPES_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, REACHING_DEFINITIONS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, REACHING_INSTRUCTIONS_ANALYZER);

    /* Allocate the necessary memory.
     */
    depToCheck 		= xanList_new(allocFunction, freeDataDependenceToCheck, cloneDataDependenceToCheck);
    (*depToCheckTable) = allocFunction(sizeof(XanHashTable *) * (numInsts + 1));
    variableDefinitionsWithinLoop = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    variableInvariantsWithinLoop = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the successors of the loop.
     */
    loopSuccessors		= LOOPS_getSuccessorsOfLoop(loop);
    assert(loopSuccessors != NULL);

    /* Identify the variables that change value at every iteration.
     */
    uniqueValueVars		= internal_getUniqueValueVariables(loop, predoms, postdoms, numInsts, variableDefinitionsWithinLoop);

    /* Identify variables that cannot have aliases.
     */
    cannotBeAliasVars	= internal_getCannotBeAliasVariables(loop);

    /* Analyze memory locations used by instructions of the loop.
     */
    memoryLocations		= internal_findMemoryLocationStatus(loop);

    /* Identify variables that do not change after being computed in the loop.
     * Notice that this is different then invariant variables.
     */
    cannotChangeVars	= internal_getCannotChangeVariables(loop, variableDefinitionsWithinLoop);

    /* Check inter loop-iterations data dependences.
     */
    PDEBUG("CHIARA: GetDataDependencesAcrossLoopIterations: 	There are %u loop instructions\n", xanList_length(loop->instructions));
    item = xanList_first(loop->instructions);
    while (item != NULL) {

        /* Fetch the instruction.
         */
        inst = item->data;
        assert(inst != NULL);

        /* Check if there is a inter loop-iteration data dependence with the current instruction.
         */
        ddList 	= IRMETHOD_getDataDependencesFromInstruction(loop->method, inst);
        if (ddList != NULL) {
            data_dep_arc_list_t     *dep;
            PDEBUG("CHIARA: GetDataDependencesAcrossLoopIterations:			Instruction %u has %u dependences\n", inst->ID, xanList_length(ddList));
            item2 = xanList_first(ddList);
            while (item2 != NULL) {

                /* Fetch the data dependence.
                 */
                dep = item2->data;
                assert(dep != NULL);
                assert(dep->inst != NULL);
                PDEBUG("CHIARA: GetDataDependencesAcrossLoopIterations: 			Checking the dependence %u -> %u\n", inst->ID, dep->inst->ID);

                /* Add the data dependence if it is across different loop iterations.
                 */
                internal_checkAndAddDataDependence(inst, dep, loop, depToCheck, *depToCheckTable, uniqueValueVars, memoryLocations, cannotBeAliasVars, cannotChangeVars, loopSuccessors, predoms, postdoms, numInsts, variableDefinitionsWithinLoop, variableInvariantsWithinLoop, executedInsts);

                item2 = item2->next;
            }

            /* Free the memory.
             */
            xanList_destroyList(ddList);
        }

        /* Fetch the next instruction.
         */
        item = item->next;
    }

    /* Categorize data dependencies.
     */
    internal_categorizeDataDependencies(loop, depToCheck, uniqueValueVars, cannotBeAliasVars, loopSuccessors, predoms, postdoms, numInsts, executedInsts);

    /* Free the memory.
     */
    hashItem    = xanHashTable_first(variableDefinitionsWithinLoop);
    while (hashItem != NULL){
        XanHashTable    *ht;
        ht          = hashItem->element;
        assert(ht != NULL);
        xanHashTable_destroyTable(ht);
        hashItem    = xanHashTable_next(variableDefinitionsWithinLoop, hashItem);
    }
    xanHashTable_destroyTable(variableDefinitionsWithinLoop);
    xanHashTable_destroyTable(variableInvariantsWithinLoop);
    xanList_destroyList(loopSuccessors);
    freeFunction(uniqueValueVars);
    freeFunction(cannotBeAliasVars);
    freeFunction(memoryLocations);
    freeFunction(cannotChangeVars);

    PDEBUG("CHIARA: GetDataDependencesAcrossLoopIterations: Exit\n");
    return depToCheck;
}

void LOOPS_destroyDDG (XanHashTable **ddg, JITUINT32 numInsts){

    if (ddg == NULL){
        return ;
    }

    for (JITUINT32 count=0; count <= numInsts; count++){
        if (ddg[count] == NULL){
            continue ;
        }
        xanHashTable_destroyTable(ddg[count]);
    }
    freeFunction(ddg);

    return ;
}

static inline void internal_checkAndAddDataDependence (ir_instruction_t *inst, data_dep_arc_list_t *dep, loop_t *loop, XanList *depToCheck, XanHashTable **depToCheckTable, JITBOOLEAN *uniqueValueVars, JITINT32 *memoryLocations, JITBOOLEAN *cannotBeAliasVars, JITBOOLEAN *cannotChangeVars, XanList *loopSuccessors, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop, XanHashTable *executedInsts){

    /* Assertions.
     */
    assert(dep != NULL);
    assert(depToCheck != NULL);

    /* Check whether the dependence from inst to dep->inst is a loop-carried one.
     */
    if (internal_isThereALoopCarriedDataDependence(inst, dep->inst, dep->depType, loop, uniqueValueVars, memoryLocations, cannotBeAliasVars, cannotChangeVars, loopSuccessors, predoms, postdoms, numInsts, JITTRUE, variableDefinitionsWithinLoop, variableInvariantsWithinLoop, executedInsts)) {

        /* The dependence from inst to dep->inst is a loop-carried one.
         * Add it to the list.
         */
        internal_addDataDependence(loop, inst, dep, depToCheck, depToCheckTable, JITTRUE);
    }

    return ;
}

static inline JITBOOLEAN internal_isThereALoopCarriedDataDependence (ir_instruction_t *instDst, ir_instruction_t *instSrc, JITUINT16 depType, loop_t *loop, JITBOOLEAN *uniqueValueVars, JITINT32 *memoryLocations, JITBOOLEAN *cannotBeAliasVars, JITBOOLEAN *cannotChangeVars, XanList *loopSuccessors, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, JITBOOLEAN dataDependencesAreCorrect, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop, XanHashTable *executedInsts) {
    IR_ITEM_VALUE   varDefinedID;

    /* Assertions.
     */
    assert(instDst != NULL);
    assert(instSrc != NULL);
    assert(loop != NULL);
    assert(loop->method != NULL);
    assert(loop->instructions != NULL);
    assert(cannotBeAliasVars != NULL);
    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Start\n");
    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	Dependence %u -> %u\n", instDst->ID, instSrc->ID);

    /* Fetch the variable defined by the source of the dependence.
     */
    varDefinedID    = livenessGetDef(loop->method, instSrc);

    /* Check whether both instructions belong to the loop or not.
     */
    if (	(xanList_find(loop->instructions, instDst) == NULL)	||
            (xanList_find(loop->instructions, instSrc) == NULL)	) {
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	Instructions do not belong to the loop\n");
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
        return JITFALSE;
    }
    assert(xanList_find(loop->instructions, instDst) != NULL);
    assert(xanList_find(loop->instructions, instSrc) != NULL);

    /* Check whether the instructions cannot be executed.
     */
    if (    (executedInsts != NULL)                                                                                             &&
            ((xanHashTable_lookup(executedInsts, instSrc) == NULL) || (xanHashTable_lookup(executedInsts, instDst) == NULL))    ){
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	Instructions did not get executed\n");
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
        return JITFALSE;
    }

    /* Check if this is a false RAW dependence through memory because the producer predominates the consumer.
     */
    if (	((depType & DEP_RAW) == 0)		&&
            ((depType & DEP_WAW) == 0)		&&
            ((depType & DEP_WAR) == 0)		&&
            ((depType & DEP_MWAW) == 0)		&&
            ((depType & DEP_MWAR) == 0)		&&
            ((depType & DEP_MRAW) == DEP_MRAW)	) {

        /* Check if the producer of the value predominates the consumer.
         */
        if (TOOLS_isInstructionADominator(loop->method, numInsts, instSrc, instDst, predoms, IRMETHOD_isInstructionAPredominator)) {

            /* Check if the producer does not read any value, it blindly writes to the memory.
             */
            switch (instSrc->type) {
                case IRSTOREREL:
                case IRINITMEMORY:

                    /* Since the producer predominates the consumer and the dependence is a RAW through memory, this is a false dependence.
                     */
                    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The producer predominates the consumer\n");
                    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                    return JITFALSE;
            }
        }

        /* Check if the consumer does not read any value from memory.
         */
        switch (instDst->type) {
            case IRSTOREREL:
            case IRINITMEMORY:

                /* Since the consumer does not read any value from memory, this is a false dependence.
                 */
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The producer predominates the consumer\n");
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                return JITFALSE;
        }
    }

    /* Check if this is a false RAW dependence through registers because the producer predominates the consumer.
     */
    if (	((depType & DEP_RAW) == DEP_RAW)	&&
            ((depType & DEP_WAW) == 0)	    	&&
            ((depType & DEP_WAR) == 0)		    &&
            ((depType & DEP_MWAW) == 0)		    &&
            ((depType & DEP_MWAR) == 0)		    &&
            ((depType & DEP_MRAW) == 0)	        ){

        /* Check if the producer of the value predominates the consumer.
         */
        if (TOOLS_isInstructionADominator(loop->method, numInsts, instSrc, instDst, predoms, IRMETHOD_isInstructionAPredominator)) {

            /* The producer generates the value before the consumer at every iteration.
             * Hence, there is no loop-carried data dependence.
             */
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The producer predominates the consumer\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }

        /* Check whether we need to use the executed instructions profile.
         */
        if (executedInsts != NULL){

            /* Check if the instruction that uses the shared variable cannot touch memory.
             */
            if (!IRMETHOD_canInstructionPerformAMemoryStore(instDst)){
                IR_ITEM_VALUE   consumerVarDefID;

                /* Check if the variable generated by the consumer is going to be used by an instruction that has been executed.
                 */
                consumerVarDefID    = livenessGetDef(loop->method, instDst);
                if (consumerVarDefID != NOPARAM){
                    XanHashTable        *reachableInsts;
                    XanHashTableItem    *hashItem;
                    JITBOOLEAN          depMayExist;

                    /* Fetch the instructions that can use the variable defined by instDst.
                     */
                    depMayExist     = JITFALSE;
                    reachableInsts  = IRMETHOD_getInstructionsThatCanBeReachedFromPosition(loop->method, instDst);
                    hashItem        = xanHashTable_first(reachableInsts);
                    while (hashItem != NULL){
                        ir_instruction_t    *reachableInst;

                        /* Fetch an instruction.
                         */
                        reachableInst   = hashItem->element;
                        assert(reachableInst != NULL);

                        /* Check whether the current instruction uses the variable defined by instDst.
                         */
                        if (IRMETHOD_doesInstructionUseVariable(loop->method, reachableInst, consumerVarDefID)){

                            /* The variable defined by instDst can be used by instruction reachableInst.
                             * Check whether reachableInst has been executed.
                             */
                            if (xanHashTable_lookup(executedInsts, reachableInst) != NULL){
                                depMayExist = JITTRUE;
                                break ;
                            }
                        }
                        
                        /* Fetch the next element.
                         */
                        hashItem        = xanHashTable_next(reachableInsts, hashItem);
                    }

                    /* Free the memory.
                     */
                    xanHashTable_destroyTable(reachableInsts);

                    /* Check if the variable defined by instDst can be used by an instruction that has been executed.
                     */
                    if (!depMayExist){
                        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The consumer generates the variable %llu that is not used by any instruction that has been executed\n", consumerVarDefID);
                        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                        return JITFALSE;
                    }
                }
            }
        }
    }

    /* Check whether this dependence is a true dependence based on an induction variable.
     * The dependence we are looking for is the following:
     * ...
     * a = definition of an induction variable
     * instruction that uses a
     *
     * In this case, we need to add this dependence.
     */
    if (	(depType == DEP_RAW)        					&&
            (IRMETHOD_doesInstructionDefineAVariable(loop->method, instSrc))	) {
        IR_ITEM_VALUE	defVar;
        defVar	= IRMETHOD_getVariableDefinedByInstruction(loop->method, instSrc);
        if (	LOOPS_isAnInductionVariable(loop, defVar)			&&
                IRMETHOD_doesInstructionUseVariable(loop->method, instDst, defVar)	) {
            assert(IRMETHOD_doesInstructionUseVariable(loop->method, instDst, defVar));
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is based on the induction variable %llu\n", defVar);
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITTRUE;
        }
    }

    /* Check if the self dependence is due to a free call.
     */
    if (	(instDst->type == IRFREEOBJ)	||
            (instDst->type == IRFREE)		||
            (instSrc->type == IRFREE)	||
            (instSrc->type == IRFREEOBJ)	) {
        ir_item_t	    	*varDefined;
        ir_instruction_t	*freeInst;
        XanHashTable    	*defs;
        XanHashTableItem	*hashItem;

        /* Check self-dependence.
         */
        if (instDst == instSrc) {
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The self dependence is a free\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }

        /* Check for dependences with other free instructions.
         */
        if (	((instDst->type == IRFREEOBJ) || (instDst->type == IRFREE))		&&
                ((instSrc->type == IRFREE) || (instSrc->type == IRFREEOBJ))	) {
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence does not exist because free are thread safe\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }

        /* Check if the free removes memory allocated at every iteration.
         */
        if ((instDst->type == IRFREEOBJ) || (instDst->type == IRFREE)) {
            freeInst	= instDst;
        } else {
            freeInst	= instSrc;
        }
        varDefined	= IRMETHOD_getInstructionParameter1(freeInst);
        assert(IRDATA_isAVariable(varDefined));
        defs        = internal_getDefinitionsWithinLoop(loop, (varDefined->value).v, variableDefinitionsWithinLoop);
        hashItem	= xanHashTable_first(defs);
        while (hashItem != NULL) {
            ir_instruction_t	*def;
            def	= hashItem->element;
            assert(def != NULL);
            if (!TOOLS_isInstructionADominator(loop->method, numInsts, def, freeInst, predoms, IRMETHOD_isInstructionAPredominator)) {
                break;
            }
            if (!IRMETHOD_isAMemoryAllocationInstruction(def)) {
                break ;
            }
            hashItem	= xanHashTable_next(defs, hashItem);
        }
        if (hashItem == NULL) {
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence does not exist because free deallocate memory allocated within the same iteration\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }
    }

    /* Check whether the dependence is due to a call to memory allocation functions which are thread safe.
     */
    if (internal_dependenceDueToThreadSafeCall(loop->method, instDst, instSrc, depType)) {

        /* Add the dependence.
         */
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is due to a call that can be thread-safe\n");
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
        return JITTRUE;
    }

    /* Check if the dependence can be       *
     * deleted by variables renaming	*/
    if (    ((depType & DEP_RAW) == 0)         		&&
            ((depType & DEP_MWAW) == 0)      			&&
            ((depType & DEP_MWAR) == 0)        		&&
            ((depType & DEP_MRAW) == 0)    	    		) {
        if (	((depType & DEP_WAW) == 0)						||
                (instSrc != instDst)							||
                (!internal_isLiveOutAtExitPoint(loop, instSrc, loopSuccessors))  	) {

            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is satisfied by registers renaming\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }
        assert(instSrc == instDst);
        assert((depType & DEP_WAW) == DEP_WAW);
        assert(internal_isLiveOutAtExitPoint(loop, instSrc, loopSuccessors));
        assert(IRMETHOD_doesInstructionDefineAVariable(loop->method, instSrc));
    }

    /* Check if the instruction dependent on predominates the other instruction and the dependence is through registers.
     */
    if (    (varDefinedID != -1)         								                                                                &&
            (internal_isLiveOutAtLoopEntryPoint(loop, instSrc))  	 				    			                                    &&
            (!internal_isLiveOutAtExitPoint(loop, instSrc, loopSuccessors)) 	        				                                &&
            (TOOLS_isInstructionADominator(loop->method, numInsts, instSrc, instDst, predoms, IRMETHOD_isInstructionAPredominator)) 	&&
            (instDst != instSrc)													                                                    &&
            ((depType & DEP_MWAW) == 0)											                                                        &&
            ((depType & DEP_MWAR) == 0)											                                                        &&
            ((depType & DEP_MRAW) == 0)											                                                        ) {

        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The value is always produced by %u before it is used by %u during the same iteration\n", instSrc->ID, instDst->ID);
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
        return JITFALSE;
    }

    /* Check if the dependence is through memory only and it is based on an impossible alias between variables.
     */
    if (	((depType & DEP_RAW) == 0)						&&
            ((instDst->type == IRLOADREL) || (instDst->type == IRSTOREREL))		&&
            ((instSrc->type == IRLOADREL) || (instSrc->type == IRSTOREREL))		) {
        ir_item_t	*base1;
        ir_item_t	*base2;

        /* Fetch the base addresses.
         */
        base1	= IRMETHOD_getInstructionParameter1(instDst);
        base2	= IRMETHOD_getInstructionParameter1(instSrc);

        /* Check if the base address is kept in different variables.
         */
        if (	(base1->type == IROFFSET)			&&
                (base2->type == IROFFSET)			&&
                ((base1->value).v != (base2->value).v)		) {

            /* Check if one of these variables cannot have alias.
             * In this case, the dependence is false.
             */
            if (	cannotBeAliasVars[(base1->value).v]	||
                    cannotBeAliasVars[(base2->value).v]	) {
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence does not exist because base addresses cannot be alias\n");
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                return JITFALSE;
            }

            /* Check if these two variables will always have different values.
             */
            if (internal_haveVariablesDifferentValuesAlways(loop, (base1->value).v, (base2->value).v, cannotChangeVars, variableDefinitionsWithinLoop)) {
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence does not exist because base addresses cannot be alias\n");
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                return JITFALSE;
            }
        }
    }

    /* Check if the instruction defines a value that is live across loop iterations.
     */
    if (    (livenessGetDef(loop->method, instSrc) != -1)                                   &&
            (IRMETHOD_doesInstructionUseVariable(loop->method, instDst, varDefinedID))      &&
            ((depType & DEP_RAW) == DEP_RAW)                                                &&
            ((depType & DEP_MWAW) == 0)                                                     &&
            ((depType & DEP_MWAR) == 0)                                                     &&
            ((depType & DEP_MRAW) == 0)                                                     &&
            (internal_isLiveOutAtLoopEntryPoint(loop, instSrc))                             ) {
        XanList			*reachingDefs;
        JITBOOLEAN		doesDefReachesUse;

        /* Check if the definition reaches the current use.
         */
        reachingDefs		= IRMETHOD_getVariableDefinitionsThatReachInstruction(loop->method, instDst, varDefinedID);
        doesDefReachesUse	= (xanList_find(reachingDefs, instSrc) != NULL);
        xanList_destroyList(reachingDefs);
        if (doesDefReachesUse) {

            /* Add the current data dependence.
             */
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	Added: the instruction %d defines a value, which is live across loop iterations\n", instSrc->ID);
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITTRUE;
        }

        /* The instruction instSrc defines a value, which will be stored in the same variable used by instDst, but it will be redefined before getting to instDst.
         * Hence, no loop-carried data dependence exists.
         */
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	Variable %llu defined by %u get redefined before reaching %u\n", varDefinedID, instSrc->ID, instDst->ID);
        PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
        return JITFALSE;
    }

    /* Check if the dependence can be across*
     * different loop iterations and it is	*
     * not through memory			*/
    if (    ((depType & DEP_MWAW) == 0)        &&
            ((depType & DEP_MWAR) == 0)        &&
            ((depType & DEP_MRAW) == 0)        ) {

        /* The dependence is through registers.		*
         * if the register defined is not live at the	*
         * entry point of the loop, this dependence     *
         * cannot be across different loop iterations	*/
        if (!internal_isLiveOutAtLoopEntryPoint(loop, instSrc)) {

            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is through registers and the defined variable is not live at entry point of the loop\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }
        assert(internal_isLiveOutAtLoopEntryPoint(loop, instSrc));
    }

    /* Check if the data dependence is through memory, the address is defined once during each iteration and this address grows strictly monotonically.
     */
    if (dataDependencesAreCorrect) {
        if (    ((depType & DEP_RAW) == 0)        	 						 &&
                ((depType & DEP_WAR) == 0)        							 &&
                ((depType & DEP_WAW) == 0)        							 &&
                (instDst->type == IRLOADREL || instDst->type == IRSTOREREL)	 &&
                (instSrc->type == IRLOADREL || instSrc->type == IRSTOREREL)	 ){
            ir_item_t	*base1;
            ir_item_t	*base2;

            /* Fetch the base addresses and the offsets.
             */
            base1	= IRMETHOD_getInstructionParameter1(instDst);
            base2	= IRMETHOD_getInstructionParameter1(instSrc);

            /* Check if the base address is kept in the same variable.
             */
            if (	(base1->type == IROFFSET)			&&
                    (base2->type == IROFFSET)			) {
                JITBOOLEAN	isFirstMono;
                JITBOOLEAN	isSecondMono;
                JITBOOLEAN	isIntraIteration;
                XanList		*instsToCheck;

                /* Check if the variable that keeps the base address monotonically increases with the iteration count.
                 */
                instsToCheck		= xanList_new(allocFunction, freeFunction, NULL);
                isFirstMono		    = internal_doesVariableStrictlyGrowWithIterationCount(loop, (base1->value).v, instsToCheck, memoryLocations, cannotChangeVars, predoms, postdoms, numInsts, variableDefinitionsWithinLoop, variableInvariantsWithinLoop);
                isSecondMono		= internal_doesVariableStrictlyGrowWithIterationCount(loop, (base2->value).v, instsToCheck, memoryLocations, cannotChangeVars, predoms, postdoms, numInsts, variableDefinitionsWithinLoop, variableInvariantsWithinLoop);
                xanList_destroyList(instsToCheck);

                /* Check if the dependence is loop-carried.
                 */
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	%d %d\n", isFirstMono == JITTRUE, isSecondMono == JITTRUE);
                isIntraIteration	= isFirstMono && isSecondMono;
                if (isIntraIteration) {
                    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is through a memory address that strictly grows with iteration count\n");
                    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                    return JITFALSE;
                }
            }
        }
    }

    /* Check if the data dependence is through memory but the address change at every iteration.
     */
    if (    ((depType & DEP_RAW) == 0)        	 						  	                                &&
            ((depType & DEP_WAR) == 0)        	 						  	                                &&
            ((depType & DEP_WAW) == 0)        	 						  	                                &&
            (instDst->type == IRLOADREL || instDst->type == IRSTOREREL || instDst->type == IRINITMEMORY)	&&
            (instSrc->type == IRLOADREL || instSrc->type == IRSTOREREL || instSrc->type == IRINITMEMORY)    ){
        ir_item_t	*base1;
        ir_item_t	*base2;

        /* Fetch the base addresses and the offsets.
         */
        base1	= IRMETHOD_getInstructionParameter1(instDst);
        base2	= IRMETHOD_getInstructionParameter1(instSrc);

        /* Check if the base address is kept in the same variable.
         */
        if (	(base1->type == IROFFSET)			&&
                (base2->type == IROFFSET)			&&
                ((base1->value).v == (base2->value).v)		) {

            /* Check if the dependence is false.
             */
            if (uniqueValueVars[(base1->value).v]) {
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is through a memory address that change at every iteration\n");
                PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
                return JITFALSE;
            }
        }
    }

    /* Check if the memory is because a memory location that change at every instruction.
     */
    if (    ((depType & DEP_RAW) == 0)        	 							&&
            ((depType & DEP_WAR) == 0)        	 							&&
            ((depType & DEP_WAW) == 0)        	 							&&
            (IRMETHOD_isAMemoryInstruction(instDst) || IRMETHOD_isACallInstruction(instDst))			&&
            (IRMETHOD_isAMemoryInstruction(instSrc) || IRMETHOD_isACallInstruction(instSrc))		&&
            (IRMETHOD_isAMemoryInstruction(instSrc) != IRMETHOD_isAMemoryInstruction(instDst))		) {
        ir_item_t	*base1;

        /* Fetch the base address.
         */
        if (IRMETHOD_isAMemoryInstruction(instSrc)) {
            assert(IRMETHOD_isAMemoryInstruction(instSrc));
            assert(!IRMETHOD_isAMemoryInstruction(instDst));
            base1	= IRMETHOD_getInstructionParameter1(instSrc);
        } else {
            assert(!IRMETHOD_isAMemoryInstruction(instSrc));
            assert(IRMETHOD_isAMemoryInstruction(instDst));
            base1	= IRMETHOD_getInstructionParameter1(instDst);
        }

        /* Check if the dependence is false.
         */
        if (	IRDATA_isAVariable(base1)			&&
                (uniqueValueVars[(base1->value).v])		) {
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	The dependence is through a memory address that change at every iteration\n");
            PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
            return JITFALSE;
        }
    }

    /* Since we are going to run one thread each iteration	*
     * we need to delete all data dependencies where the    *
     * writer instruction is post-dominated by another      *
     * writer instruction and there is no modification of   *
     * the base + offset address in the meanwhile.		*/
    //TODO

    /* Add the data dependence.
     */
    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: 	Added\n");
    PDEBUG("CHIARA: Data dependences: check_and_add_data_dependences: Exit\n");
    return JITTRUE;
}

static inline void * cloneDataDependenceToCheck (void *dataDep) {
    loop_inter_interation_data_dependence_t *dep;
    loop_inter_interation_data_dependence_t *clone;

    /* Assertions.
     */
    assert(dataDep != NULL);

    /* Clone.
     */
    dep 					= (loop_inter_interation_data_dependence_t *) dataDep;
    clone					= allocFunction(sizeof(loop_inter_interation_data_dependence_t));
    memcpy(clone, dep, sizeof(loop_inter_interation_data_dependence_t));

    return clone;
}

static inline void freeDataDependenceToCheck (void *dataDep) {
    loop_inter_interation_data_dependence_t *dep;

    /* Assertions			*/
    assert(dataDep != NULL);

    /* Free the memory		*/
    dep = (loop_inter_interation_data_dependence_t *) dataDep;
    freeFunction(dep);

    return;
}

static inline JITINT32 internal_isLiveOutAtLoopEntryPoint (loop_t *loop, ir_instruction_t *inst) {

    /* Assertions				*/
    assert(loop != NULL);
    assert(loop->header != NULL);
    assert(loop->method != NULL);
    assert(inst != NULL);

    /* Check if the instruction defines a   *
     * value which is live after the loop	*
     * header				*/
    if (IRMETHOD_isVariableLiveIN(loop->method, loop->header, livenessGetDef(method, inst))) {
        return JITTRUE;
    }

    /* Return				*/
    return JITFALSE;
}

static inline JITINT32 internal_isLiveOutAtExitPoint (loop_t *loop, ir_instruction_t *inst, XanList *loopSuccessors) {
    XanListItem             *item;
    ir_instruction_t        *exitPoint;
    JITBOOLEAN		found;
    IR_ITEM_VALUE		defVar;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(inst != NULL);
    assert(loop->method != NULL);
    assert(loopSuccessors != NULL);

    /* Fetch the variable defined by the instruction.
     */
    defVar	= livenessGetDef(loop->method, inst);

    /* Check the exit points.
     */
    found	= JITFALSE;
    item 	= xanList_first(loopSuccessors);
    while (item != NULL) {
        exitPoint = (ir_instruction_t *) item->data;
        assert(exitPoint != NULL);
        if (IRMETHOD_isVariableLiveIN(loop->method, exitPoint, defVar)) {
            found	= JITTRUE;
            break;
        }
        item = item->next;
    }

    return found;
}

static inline void internal_addDataDependence (loop_t *loop, ir_instruction_t *inst, data_dep_arc_list_t *dep, XanList *depToCheck, XanHashTable **depToCheckTable, JITBOOLEAN addSymmetry) {
    loop_inter_interation_data_dependence_t *currentDep;
    loop_inter_interation_data_dependence_t *currentDepClone;

    /* Assertions					*/
    assert(loop != NULL);
    assert(inst != NULL);
    assert(dep != NULL);
    assert(depToCheck != NULL);

    /* Check if the current dependence was already	*
     *  considered					*/
    if (internal_exist_data_dependence(depToCheck, depToCheckTable, inst, dep->inst) != NULL) {
        return;
    }

    /* Add the current data dependence		*/
    currentDep = allocMemory(sizeof(loop_inter_interation_data_dependence_t));
    assert(currentDep != NULL);
    currentDep->inst1 = inst;
    currentDep->inst2 = dep->inst;
    currentDep->type = dep->depType;

    /* Check if the dependence is through registers	*/
    if (    ((dep->depType & DEP_MWAW) == 0)        &&
            ((dep->depType & DEP_MWAR) == 0)        &&
            ((dep->depType & DEP_MRAW) == 0)        ) {
        IR_ITEM_VALUE inductiveVarID;

        /* Fetch the variable ID where we will store the produced value.
         */
        inductiveVarID	= livenessGetDef(loop->method, dep->inst);
        if (!IRMETHOD_doesInstructionUseVariable(loop->method, inst, inductiveVarID)) {
            ir_instruction_t	*tempInst;
            tempInst		= currentDep->inst1;
            currentDep->inst1	= currentDep->inst2;
            currentDep->inst2	= tempInst;
            inductiveVarID		= livenessGetDef(loop->method, inst);
        }

        /* The dependence is through registers			*
         * Check if the variable in conflict is inductive	*/
        if (    (inductiveVarID != -1)							&&
                (LOOPS_isAnInductionVariable(loop, inductiveVarID))                     ) {
            if (	((dep->depType & DEP_WAR) != 0) 				||
                    ((dep->depType & DEP_RAW) != 0) 				||
                    (((dep->depType & DEP_WAW) != 0) && (dep->inst == inst))	) {
                induction_variable_t	*varInfo;

                /* The variable that makes the data dependence is inductive, hence its	*
                 * value is predictable statically and therefore there is not this data	*
                 * dependence.								*/
                assert(inductiveVarID < IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method));
                assert(IRMETHOD_doesInstructionDefineVariable(loop->method, currentDep->inst2 , inductiveVarID));
                currentDep->originalInductiveVariableID = inductiveVarID;
                currentDep->isBasedOnInductiveVariable 	= JITTRUE;

                /* Set the derived induction variable.
                 */
                varInfo	= LOOPS_getInductionVariable(loop, inductiveVarID);
                assert(varInfo != NULL);
                assert(varInfo->ID == inductiveVarID);
                currentDep->originalDerivedInductiveVariableID	= varInfo->i;
            }
        }
    }

    /* Check if the dependence is between a store	*
     * and itself					*/
    if (	(currentDep->inst1 == currentDep->inst2)	&&
            ((dep->depType & DEP_MWAW) == DEP_MWAW)		&&
            (currentDep->inst1->type == IRSTOREREL)	    ){
        ir_item_t	*param1;

        /* Check if the base address change every iteration	*/
        param1	= IRMETHOD_getInstructionParameter1(currentDep->inst1);
        if (	(param1->type == IROFFSET)					&&
                (LOOPS_isAnInductionVariable(loop, (param1->value).v))        	) {
            currentDep->originalInductiveVariableID 	= (param1->value).v;
            currentDep->isBasedOnInductiveVariable 		= JITTRUE;
        }
    }

    /* Check if the dependence is false due to a false alias due to derived induction variables with a shared parent.
     * e.g. M[x] = ...  ; M[y] = ...; x != y always
     */
    if(	(!currentDep->isBasedOnInductiveVariable)	&&
            ((dep->depType & DEP_WAW) == 0)       		&&
            ((dep->depType & DEP_WAR) == 0)       		&&
            ((dep->depType & DEP_RAW) == 0)       		) {
        ir_item_t	*param1;
        ir_item_t	*param2;

        /* Fetch the base addresses			*/
        param1	= IRMETHOD_getInstructionParameter1(currentDep->inst1);
        param2	= IRMETHOD_getInstructionParameter1(currentDep->inst2);
        if (	(param1->type == IROFFSET)								&&
                (param2->type == IROFFSET)								&&
                (LOOPS_isAnInductionVariable(loop, (param1->value).v))        				&&
                (LOOPS_isAnInductionVariable(loop, (param2->value).v))        				&&
                (LOOPS_hasASharedParentInductionVariable(loop, (param1->value).v, (param2->value).v))	) {
            currentDep->originalInductiveVariableID 	= (param1->value).v;
            currentDep->isBasedOnInductiveVariable 		= JITTRUE;
        }
    }

    /* Add the dependence and its symmetric closure.
     */
    internal_addDataDependence_helpfunction(depToCheck, depToCheckTable, currentDep);
    if (	(addSymmetry)								&&
            (currentDep->inst1 != currentDep->inst2)				) {
        currentDepClone	= internal_exist_data_dependence(depToCheck, depToCheckTable, dep->inst, inst);
        if (currentDepClone == NULL) {

            /* Allocate the symmetric dependence.
             */
            currentDepClone = allocMemory(sizeof(loop_inter_interation_data_dependence_t));
            memcpy(currentDepClone, currentDep, sizeof(loop_inter_interation_data_dependence_t));
            currentDepClone->inst1 = currentDep->inst2;
            currentDepClone->inst2 = currentDep->inst1;
            internal_addDataDependence_helpfunction(depToCheck, depToCheckTable, currentDepClone);

        } else if (currentDep->isBasedOnInductiveVariable) {

            /* Copy the symmetric dependence.
             */
            memcpy(currentDepClone, currentDep, sizeof(loop_inter_interation_data_dependence_t));
            if (depToCheckTable != NULL){
                assert(xanHashTable_lookup(depToCheckTable[currentDepClone->inst1->ID], currentDepClone->inst2) != NULL);
                xanHashTable_removeElement(depToCheckTable[currentDepClone->inst1->ID], currentDepClone);
            }
            currentDepClone->inst1 = currentDep->inst2;
            currentDepClone->inst2 = currentDep->inst1;
            if (depToCheckTable != NULL){
                assert(xanHashTable_lookup(depToCheckTable[currentDepClone->inst1->ID], currentDepClone->inst2) == NULL);
                xanHashTable_insert(depToCheckTable[currentDepClone->inst1->ID], currentDepClone->inst2, currentDepClone);
            }
        }
        assert(internal_exist_data_dependence(depToCheck, depToCheckTable, dep->inst, inst) != NULL);
    }

    return;
}

static inline loop_inter_interation_data_dependence_t * internal_exist_data_dependence (XanList *depToCheck, XanHashTable **depToCheckTable, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    loop_inter_interation_data_dependence_t *dd;
    dd = internal_exist_direct_data_dependence(depToCheck, depToCheckTable, inst1, inst2);
    return dd;
}

static inline loop_inter_interation_data_dependence_t * internal_exist_direct_data_dependence (XanList *depToCheck, XanHashTable **depToCheckTable, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    loop_inter_interation_data_dependence_t         *foundDep;

    /* Assertions.
     */
    assert(inst1 != NULL);
    assert(inst2 != NULL);

    /* Check if the dependence exists inside the DDG.
     */
    foundDep    = NULL;
    if (depToCheckTable != NULL){
        if (depToCheckTable[inst1->ID] != NULL){
            foundDep  = xanHashTable_lookup(depToCheckTable[inst1->ID], inst2);
        }

    } else {
        loop_inter_interation_data_dependence_t         *currentDep;
        XanListItem                                     *item;
        assert(depToCheck != NULL);
        item = xanList_first(depToCheck);
        while (item != NULL) {
            currentDep = item->data;
            assert(currentDep != NULL);
            if (    (currentDep->inst1 == inst1)    &&
                    (currentDep->inst2 == inst2)    ) {
                foundDep    = currentDep;
                break ;
            }
            item = item->next;
        }
    }
    #ifdef DEBUG
    if (foundDep != NULL){
        assert(foundDep->inst1 == inst1);
        assert(foundDep->inst2 == inst2);
    }
    #endif

    return foundDep;
}

static inline void internal_categorizeDataDependencies (loop_t *loop, XanList *depToCheck, JITBOOLEAN *uniqueValueVars, JITBOOLEAN *cannotBeAliasVariables, XanList *loopSuccessors, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *executedInsts) {
    XanList					*toDelete;
    XanListItem                             *item;
    loop_inter_interation_data_dependence_t *currentDep;
    ir_method_t 				*method;
    JITBOOLEAN				*localizedVariables;
    JITBOOLEAN				modified;
    JITUINT32				variablesNumber;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(depToCheck != NULL);

    /* Cache some pointers.
     */
    method 			= loop->method;
    assert(method != NULL);

    /* Allocate the memory.
     */
    variablesNumber		= IRMETHOD_getNumberOfVariablesNeededByMethod(method);
    localizedVariables	= allocFunction(sizeof(JITBOOLEAN) * variablesNumber);

    /* Unset the tags of all dependences.
     */
    item = xanList_first(depToCheck);
    while (item != NULL) {

        /* Fetch the current data dependence.
         */
        currentDep = item->data;
        assert(currentDep != NULL);

        /* Unset the tags.
         */
        (currentDep->runtime_condition).conditionType 	= NO_CONDITION;
        currentDep->category 				= ALL_DD;

        /* Fetch the next data dependence.
         */
        item = item->next;
    }

    /* Categorize data dependences.
     */
    item = xanList_first(depToCheck);
    while (item != NULL) {

        /* Fetch the current data dependence.
         */
        currentDep = item->data;
        assert(currentDep != NULL);

        /* Check whether the dependence required to be categorized.
         */
        if ((currentDep->category) == ALL_DD) {

            /* Categorize the data dependence.
             */
            if (internal_isFalseMemoryManagement(loop, method, currentDep)) {
                currentDep->category = FALSE_MEMORY_MANAGEMENT;
            } else if (internal_isInputOutput(method, currentDep)) {
                currentDep->category = INPUT_OUTPUT;
            } else if (internal_isWAWRegisterReassociation(loop, method, currentDep, loopSuccessors)) {
                currentDep->category = WAW_REGISTER_REASSOCIATION;
            } else if (internal_isMemoryWAWRegisterReassociation(loop, method, currentDep, cannotBeAliasVariables)) {
                currentDep->category = MEMORY_WAW_REASSOCIATION;
            } else if (internal_isEscapesMemoryFalseDataDependence(method, currentDep)) {
                currentDep->category = FALSE_ESCAPES_MEMORY;
            } else if (internal_isFalseInductiveVariableDataDependence(method, currentDep)) {
                currentDep->category = FALSE_INDUCTIVE_VARIABLE;
            } else if (internal_isLocalizableByIntegerRegisterReassociationDataDependence(method, loop, currentDep, executedInsts, depToCheck)) {
                currentDep->category = FALSE_REGISTER_REASSOCIATION;
            } else if (internal_isLocalizableByFloatingPointRegisterReassociationDataDependence(method, loop, currentDep, executedInsts, depToCheck)) {
                currentDep->category = FLOATINGPOINT_REGISTER_REASSOCIATION;
            } else if (internal_isLocalizableDataDependence(loop, currentDep, depToCheck, localizedVariables, predoms, postdoms, numInsts)) {
                currentDep->category = FALSE_LOCALIZABLE;
            } else if (internal_isUniqueValueVariableDataDependence(loop, currentDep, uniqueValueVars)) {
                currentDep->category = UNIQUE_VALUE_PER_ITERATION;
            } else if (internal_isFalseUniqueValueVariableDataDependence(loop, currentDep, uniqueValueVars)) {
                currentDep->category = FALSE_UNIQUE_VALUE_PER_ITERATION;
            } else if (internal_isALibraryCallDataDependence(loop, currentDep)) {
                currentDep->category = LIBRARY_TRUE;
            } else if (internal_isGlobalVariableDependence(loop, currentDep)) {
                currentDep->category = GLOBAL_VARIABLE;
            } else if (internal_isFalseMemoryRenamingDataDependence(loop, currentDep, predoms, numInsts)) {
                currentDep->category = FALSE_MEMORY_RENAMING;
            } else if (internal_isAlwaysTrueDataDependence(method, currentDep)) {
                currentDep->category = ALWAYS_TRUE;
            } else {
                currentDep->category = RUNTIME_CHECK_TRUE;
                internal_computeConditionToCheck(method, currentDep);
            }
        }
        assert((currentDep->category) != ALL_DD);

        /* Fetch the next data dependence.
         */
        item = item->next;
    }

    /* Compute the set of variables that have unique values.
     */
    do {

        /* Initialize the variables.
         */
        modified	= JITFALSE;

        /* Expand the localized variables.
         */
        for (JITUINT32 count=0; count < variablesNumber; count++) {
            XanList 	*l;

            /* Check whether the current variable is localized.
             */
            if (!localizedVariables[count]) {
                continue ;
            }

            /* Fetch the instructions that use the localized variable.
             */
            l	= IRMETHOD_getVariableUsesWithinInstructions(loop->method, count, loop->instructions);
            assert(l != NULL);

            /* Expand the localization to variables defined based on the already localized ones.
             */
            item	= xanList_first(l);
            while (item != NULL) {
                ir_instruction_t	*inst;
                IR_ITEM_VALUE		defVar;
                inst	= item->data;
                defVar	= IRMETHOD_getVariableDefinedByInstruction(loop->method, inst);
                if (	(defVar != NOPARAM)		&&
                        (!localizedVariables[defVar])	) {
                    localizedVariables[defVar]	= JITTRUE;
                    modified			= JITTRUE;
                }
                item	= item->next;
            }

            /* Free the memory.
             */
            xanList_destroyList(l);
        }

    } while (modified);

    /* Remove dependences that are based on impossible aliases.
     */
    toDelete	= xanList_new(allocFunction, freeFunction, NULL);
    item 		= xanList_first(depToCheck);
    while (item != NULL) {

        /* Fetch the current data dependence.
         */
        currentDep = item->data;
        assert(currentDep != NULL);
        assert((currentDep->category) != ALL_DD);

        /* Check if the dependence has been already considered for localization.
         */
        if ((currentDep->category) == FALSE_LOCALIZABLE) {

            /* Check if the dependence is through memory instructions.
             */
            if (	((currentDep->type & DEP_RAW) == 0)			&&
                    ((currentDep->type & DEP_WAR) == 0)			&&
                    ((currentDep->type & DEP_WAW) == 0)			&&
                    (IRMETHOD_isAMemoryInstruction(currentDep->inst1))	&&
                    (IRMETHOD_isAMemoryInstruction(currentDep->inst2))	) {
                ir_item_t	*par1_inst1;
                ir_item_t	*par1_inst2;

                /* Check whether the memory address will be localized.
                 */
                par1_inst1	= IRMETHOD_getInstructionParameter1(currentDep->inst1);
                par1_inst2	= IRMETHOD_getInstructionParameter1(currentDep->inst2);
                if (	(IRDATA_isAVariable(par1_inst1))		&&
                        (IRDATA_isAVariable(par1_inst2))		&&
                        (localizedVariables[(par1_inst1->value).v])	&&
                        (localizedVariables[(par1_inst2->value).v])	&&
                        (xanList_find(toDelete, currentDep) == NULL)	) {
                    loop_inter_interation_data_dependence_t *currentDepSimm;
                    XanListItem				*item2;

                    /* Fetch the simmetric dependence of the current one.
                     */
                    currentDepSimm	= NULL;
                    item2 		= xanList_first(depToCheck);
                    while (item2 != NULL) {
                        loop_inter_interation_data_dependence_t *currentDep2;
                        currentDep2	= item2->data;
                        assert(currentDep2 != NULL);
                        if (	(currentDep2 != currentDep)			&&
                                (currentDep2->inst1 == currentDep->inst2)	&&
                                (currentDep2->inst2 == currentDep->inst1)	) {
                            currentDepSimm	= currentDep2;
                            break;
                        }
                        item2		= item2->next;
                    }
                    assert(currentDepSimm != NULL);

                    /* Check if the simmetric dependence is not localized as well.
                     */
                    if (currentDepSimm->category == FALSE_LOCALIZABLE) {
                        xanList_append(toDelete, currentDep);
                        xanList_append(toDelete, currentDepSimm);
                    }
                }
            }
        }

        /* Fetch the next data dependence.
         */
        item = item->next;
    }
    xanList_removeElements(depToCheck, toDelete, JITTRUE);
    xanList_destroyListAndData(toDelete);

    /* Free the memory.
     */
    freeFunction(localizedVariables);

    return;
}

static inline JITINT32 internal_isUniqueValueVariableDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, JITBOOLEAN *uniqueValueVars) {
    if (!internal_isFalseUniqueValueVariableDataDependence(loop, dep, uniqueValueVars)) {
        return JITFALSE;
    }
    return (dep->inst1 == dep->inst2);
}

static inline JITINT32 internal_isFalseUniqueValueVariableDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, JITBOOLEAN *uniqueValueVars) {
    IR_ITEM_VALUE	defVar;

    if (uniqueValueVars == NULL) {
        return JITFALSE;
    }
    if ((dep->type & DEP_RAW) == 0) {
        return JITFALSE;
    }
    assert((IRMETHOD_getVariableDefinedByInstruction(loop->method, dep->inst1) != NOPARAM) || (IRMETHOD_getVariableDefinedByInstruction(loop->method, dep->inst2) != NOPARAM));

    defVar	= IRMETHOD_getVariableDefinedByInstruction(loop->method, dep->inst2);
    if (	(defVar == NOPARAM)								||
            (!IRMETHOD_doesInstructionUseVariable(loop->method, dep->inst1, defVar))	) {
        defVar	= IRMETHOD_getVariableDefinedByInstruction(loop->method, dep->inst1);
        if (defVar == NOPARAM) {
            return JITFALSE;
        }
        if (!IRMETHOD_doesInstructionUseVariable(loop->method, dep->inst2, defVar)) {
            return JITFALSE;
        }
    }
    assert(defVar != NOPARAM);
    return uniqueValueVars[defVar];
}

static inline JITINT32 internal_isMemoryWAWRegisterReassociation (loop_t *loop, ir_method_t *method, loop_inter_interation_data_dependence_t *dep, JITBOOLEAN *cannotBeAliasVariables) {
    ir_item_t	*base;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(method != NULL);
    assert(dep != NULL);

    /* Check if we have enough information.
     */
    if (cannotBeAliasVariables == NULL) {
        return JITFALSE;
    }
    assert(cannotBeAliasVariables != NULL);

    /* Check if the dependence is between the same instructions.
     */
    if (dep->inst1 != dep->inst2) {
        return JITFALSE;
    }
    assert(dep->inst1 == dep->inst2);

    /* Check if the current dependence can be removed by memory renaming.
     */
    if (	((dep->type & DEP_RAW) != 0)	||
            ((dep->type & DEP_WAR) != 0)	||
            ((dep->type & DEP_WAW) != 0)	||
            ((dep->type & DEP_MRAW) != 0)	||
            ((dep->type & DEP_MWAR) != 0)	) {
        return JITFALSE;
    }
    assert((dep->type & DEP_MWAW) == DEP_MWAW);

    /* Check if there is any alias with the base address.
     */
    base	= IRMETHOD_getInstructionParameter1(dep->inst1);
    assert(base != NULL);
    if (!IRDATA_isAVariable(base)) {
        return JITFALSE;
    }
    if (!cannotBeAliasVariables[(base->value).v]) {
        return JITFALSE;
    }

    return JITTRUE;
}

static inline JITINT32 internal_isFalseMemoryManagement (loop_t *loop, ir_method_t *method, loop_inter_interation_data_dependence_t *dep) {
    if (	(internal_dependenceDueToThreadSafeCall(loop->method, dep->inst1, dep->inst2, dep->type))	||
            (internal_dependenceDueToThreadSafeCall(loop->method, dep->inst2, dep->inst1, dep->type))	) {
        return JITTRUE;
    }

    return JITFALSE;
}

static inline JITINT32 internal_isInputOutput (ir_method_t *method, loop_inter_interation_data_dependence_t *dep) {
    ir_method_t *callee1;
    ir_method_t *callee2;

    /* Assertions.
     */
    assert(method != NULL);
    assert(dep != NULL);
    assert(dep->inst1 != NULL);
    assert(dep->inst2 != NULL);

    /* Initialize the variables.
     */
    callee1 = NULL;
    callee2 = NULL;

    /* Fetch the callees.
     */
    if (dep->inst1->type == IRCALL){
        callee1 = IRMETHOD_getCalledMethod(method, dep->inst1);
    }
    if (dep->inst2->type == IRCALL){
        callee2 = IRMETHOD_getCalledMethod(method, dep->inst2);
    }

    /* Check if there is a call instruction at least.
     */
    if (    (callee1 == NULL)   &&
            (callee2 == NULL)   ){
        return JITFALSE;
    }

    /* Check if we know one of the callee.
     */
    if (    (!internal_isIOMethod(callee1)) &&
            (!internal_isIOMethod(callee2)) ){
        return JITFALSE;
    }

    return JITTRUE;
}

static inline JITBOOLEAN internal_isIOMethod (ir_method_t *m){
    JITINT8 *name;

    if (m == NULL){
        return JITFALSE;
    }

    name	= IRMETHOD_getMethodName(m);
    if (name == NULL){
        return JITFALSE;
    }
    if (	(STRCMP(name, "spec_write") == 0)	||
            (STRCMP(name, "spec_putc") == 0)	||
            (STRCMP(name, "spec_reset") == 0)	||
            (STRCMP(name, "spec_rewind") == 0)	||
            (STRCMP(name, "spec_ungetc") == 0)	||
            (STRCMP(name, "spec_getc") == 0)	||
            (STRCMP(name, "spec_read") == 0)	||
            (STRCMP(name, "spec_load") == 0)	||
            (STRCMP(name, "spec_random_load") == 0)	    ||
            (STRCMP(name, "BUFFER_setStream") == 0)	                ||
            (STRCMP(name, "BUFFER_bsR1") == 0)	                ||
            (STRCMP(name, "BUFFER_needToReadNumberOfBits") == 0)	                ||
            (STRCMP(name, "BUFFER_setNumberOfBytesWritten") == 0)	                ||
            (STRCMP(name, "BUFFER_getNumberOfBytesWritten") == 0)	                ||
            (STRCMP(name, "BUFFER_readBits") == 0)	                ||
            (STRCMP(name, "BUFFER_writeBit") == 0)	                ||
            (STRCMP(name, "BUFFER_writeBits") == 0)	                ||
            (STRCMP(name, "BUFFER_writeBitsFromValue") == 0)	                ||
            (STRCMP(name, "BUFFER_writeUChar") == 0)	                ||
            (STRCMP(name, "BUFFER_writeUInt") == 0)	                ||
            (STRCMP(name, "BUFFER_flush") == 0)	                ){
        return JITTRUE;
    }

    return JITFALSE;
}

static inline JITINT32 internal_isWAWRegisterReassociation (loop_t *loop, ir_method_t *method, loop_inter_interation_data_dependence_t *dep, XanList *loopSuccessors) {
    if (    ((dep->type & DEP_RAW) == 0)         					&&
            ((dep->type & DEP_MWAW) == 0)      					&&
            ((dep->type & DEP_MWAR) == 0)   	     				&&
            ((dep->type & DEP_MRAW) == 0)    	    				&&
            ((dep->type & DEP_WAW) == DEP_WAW)					&&
            (dep->inst1 == dep->inst2)						&&
            (internal_isLiveOutAtExitPoint(loop, dep->inst1, loopSuccessors))	&&
            (IRMETHOD_doesInstructionDefineAVariable(loop->method, dep->inst1))	) {
        XanList		*l;
        JITUINT32	num;
        IR_ITEM_VALUE	varID;

        /* Fetch the set of instructions within the loop that uses the defined variable.
         */
        varID	= IRMETHOD_getVariableDefinedByInstruction(method, dep->inst1);
        l	= IRMETHOD_getVariableUsesWithinInstructions(method, varID, loop->instructions);
        num	= xanList_length(l);

        /* Free the memory.
         */
        xanList_destroyList(l);

        return (num == 0);
    }

    return JITFALSE;
}

static inline JITINT32 internal_isEscapesMemoryFalseDataDependence (ir_method_t *method, loop_inter_interation_data_dependence_t *dep) {

    /* Assertions			*/
    assert(method != NULL);
    assert(dep != NULL);

    /* Check if one of the          *
     * instruction is a call.	*/
    switch (dep->inst1->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
            return JITFALSE;
    }
    switch (dep->inst2->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
            return JITFALSE;
    }

    /* Check that the dependence is	*
     * through memory		*/
    if (    ((dep->type & DEP_RAW) == 0)    &&
            ((dep->type & DEP_WAW) == 0)    &&
            ((dep->type & DEP_WAR) == 0)    ) {

        if (    (internal_isEscapesMemoryFalseDataDependenceDirect(method, dep->inst1, dep->inst2))     ||
                (internal_isEscapesMemoryFalseDataDependenceDirect(method, dep->inst2, dep->inst1))     ) {
            return JITTRUE;
        }
    }

    /* Return			*/
    return JITFALSE;
}

static inline JITINT32 internal_isEscapesMemoryFalseDataDependenceDirect (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    ir_item_t       *result;
    ir_item_t       *param1;
    ir_item_t       *param2;
    ir_item_t       *loadStoreBase;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst1 != NULL);
    assert(inst2 != NULL);

    /* Check if one of the instruction is a	*
     * call.				*/
    switch (inst1->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
            return JITFALSE;
    }
    switch (inst2->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
            return JITFALSE;
    }

    /* Check the directed dependence	*/
    if (    (inst2->type == IRSTOREREL)     ||
            (inst2->type == IRLOADREL)      ){
        loadStoreBase = IRMETHOD_getInstructionParameter1(inst2);
        if (loadStoreBase->type == IROFFSET) {
            switch (inst1->type) {
                case IRGETADDRESS:
                    result = IRMETHOD_getInstructionResult(inst1);
                    assert(result->type == IROFFSET);
                    if (loadStoreBase->value.v != result->value.v) {
                        return JITTRUE;
                    }
                    break;
                case IRMOVE:
                    param1 = IRMETHOD_getInstructionResult(inst1);
                    assert(param1->type == IROFFSET);
                    param2 = IRMETHOD_getInstructionParameter1(inst1);
                    if (    (loadStoreBase->value.v != param1->value.v)             &&
                            (IRMETHOD_hasInstructionEscapes(method, inst1)) ) {
                        if (param2->type == IROFFSET) {
                            if (loadStoreBase->value.v != param2->value.v) {
                                return JITTRUE;
                            }
                        } else {
                            return JITTRUE;
                        }
                    }
                    break;
            }
        }
    }

    /* Return				*/
    return JITFALSE;
}

static inline JITINT32 internal_isALibraryCallDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep) {
    if ((dep->type & DEP_RAW) != 0) {
        return JITFALSE;
    }

    if (	internal_doesCallALibrary(loop->method, dep->inst1)	||
            internal_doesCallALibrary(loop->method, dep->inst2)	) {
        return JITTRUE;
    }

    return JITFALSE;
}

static inline JITBOOLEAN internal_doesCallALibrary (ir_method_t *m, ir_instruction_t *inst) {
    ir_method_t 	*callee;
    if (!IRMETHOD_isACallInstruction(inst)) {
        return JITFALSE;
    }
    callee	= IRMETHOD_getCalledMethod(m, inst);
    if (callee == NULL) {
        return JITFALSE;
    }
    if (!IRPROGRAM_doesMethodBelongToALibrary(callee)) {
        return JITFALSE;
    }
    return JITTRUE;
}

static inline JITINT32 internal_isGlobalVariableDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep) {

    /* Assertions.
     */
    assert(loop != NULL);
    assert(dep != NULL);

    /* Check if one instruction at least is a memory one.
     */
    if (	(!IRMETHOD_isAMemoryInstruction(dep->inst1))	&&
            (!IRMETHOD_isAMemoryInstruction(dep->inst2))	) {
        return JITFALSE;
    }

    /* Check if the dependence is through memory.
     */
    if (!IRMETHOD_isAMemoryDataDependenceType(dep->type)) {
        return JITFALSE;
    }

    /* Check if one instruction at least access to a global variable.
     */
    if (	internal_doesInstructionAccessAGlobalVariable(dep->inst1)	||
            internal_doesInstructionAccessAGlobalVariable(dep->inst2)	) {
        return JITTRUE;
    }

    return JITFALSE;
}

static inline JITBOOLEAN internal_doesInstructionAccessAGlobalVariable (ir_instruction_t *inst) {
    if (	(inst->type == IRLOADREL)	||
            (inst->type == IRSTOREREL)	) {
        ir_item_t *par1;
        par1	= IRMETHOD_getInstructionParameter1(inst);
        if (IRDATA_isASymbol(par1)) {
            return JITTRUE;
        }
    }
    return JITFALSE;
}

static inline JITINT32 internal_isFalseMemoryRenamingDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, JITINT32 **predoms, JITUINT32 numInsts) {
    ir_item_t       *inst1_param1;
    ir_item_t       *inst2_param1;
    ir_item_t       *inst1_param2;
    ir_item_t       *inst2_param2;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(dep != NULL);

    /* If both dependences are not store instructions, then this is not a dependence solvable by memory renaming.
     */
    if (	(dep->inst1->type != IRSTOREREL)	&&
            (dep->inst2->type != IRSTOREREL)	) {
        return JITFALSE;
    }

    /* Check the instruction types.
     */
    if (	(dep->inst1->type != IRSTOREREL)	||
            (dep->inst2->type != IRSTOREREL)	) {
        ir_instruction_t	*storeInst;
        ir_instruction_t	*notStoreInst;

        /* Fetch the store instruction.
         */
        if (dep->inst1->type == IRSTOREREL) {
            storeInst	= dep->inst1;
            notStoreInst	= dep->inst2;
        } else {
            storeInst	= dep->inst2;
            notStoreInst	= dep->inst1;
        }
        assert(storeInst->type == IRSTOREREL);
        assert(notStoreInst->type != IRSTOREREL);

        return TOOLS_isInstructionADominator(loop->method, numInsts, storeInst, notStoreInst, predoms, IRMETHOD_isInstructionAPredominator);
    }

    /* Check the dependence.
     */
    switch (dep->inst1->type) {
        case IRSTOREREL:
            inst1_param1 = IRMETHOD_getInstructionParameter1(dep->inst1);
            inst2_param1 = IRMETHOD_getInstructionParameter1(dep->inst2);
            inst1_param2 = IRMETHOD_getInstructionParameter2(dep->inst1);
            inst2_param2 = IRMETHOD_getInstructionParameter2(dep->inst2);
            if (    (memcmp(inst1_param1, inst2_param1, sizeof(ir_item_t)) == 0)    &&
                    (memcmp(inst1_param2, inst2_param2, sizeof(ir_item_t)) == 0)    ) {
                if (IRDATA_isAVariable(inst1_param1)) {
                    return LOOPS_isAnInvariantVariable(loop, (inst1_param1->value).v);
                }
                return JITTRUE;
            }
            break;
    }

    return JITTRUE;
}

static inline JITINT32 internal_isFalseInductiveVariableDataDependence (ir_method_t *method, loop_inter_interation_data_dependence_t *dep) {

    /* Assertions			*/
    assert(method != NULL);
    assert(dep != NULL);

    return dep->isBasedOnInductiveVariable;
}

static inline JITINT32 internal_isLocalizableByIntegerRegisterReassociationDataDependence (ir_method_t *method, loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanHashTable *executedInsts, XanList *loopCarriedDeps) {
    ir_item_t	*defVar;

    /* Assertions.
     */
    assert(dep != NULL);

    /* Check if register reassociation is possible.
     */
    if (!internal_isLocalizableByRegisterReassociationDataDependence(method, loop, dep, executedInsts, loopCarriedDeps)) {
        return JITFALSE;
    }

    /* Fetch the defined variable.
     */
    defVar			= IRMETHOD_getInstructionResult(dep->inst1);

    /* If the defined variable is floating point, it cannot be reassociated.
     */
    if (IRMETHOD_hasAFloatType(defVar)) {
        return JITFALSE;
    }

    return JITTRUE;
}

static inline JITINT32 internal_isLocalizableByFloatingPointRegisterReassociationDataDependence (ir_method_t *method, loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanHashTable *executedInsts, XanList *loopCarriedDeps) {

    /* Assertions.
     */
    assert(dep != NULL);

    /* Check if register reassociation is possible.
     */
    if (!internal_isLocalizableByRegisterReassociationDataDependence(method, loop, dep, executedInsts, loopCarriedDeps)) {
        return JITFALSE;
    }

    return JITTRUE;
}

static inline JITINT32 internal_isLocalizableByRegisterReassociationDataDependence (ir_method_t *method, loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanHashTable *executedInsts, XanList *loopCarriedDeps) {
    XanList     *uses;
    XanListItem *item;
    ir_item_t	*defVar1;

    /* Assertions.
     */
    assert(method != NULL);
    assert(dep != NULL);
    assert(loop != NULL);

    /* Check if the dependence is through the registers.
     */
    if (    ((dep->type & DEP_MRAW) != 0)   ||
            ((dep->type & DEP_MWAW) != 0)   ||
            ((dep->type & DEP_MWAR) != 0) 	) {
        return JITFALSE;
    }

    /* Chck if it is a RAW or WAR data dependence.
     */
    if (    ((dep->type & DEP_RAW) == 0) 	&&
            ((dep->type & DEP_WAR) == 0)  	) {
        return JITFALSE;
    }

    /* Check the type of the instruction.
     */
    if (	(dep->inst1->type != IRADD)	&&
            (dep->inst1->type != IRSUB)	&&
            (dep->inst1->type != IRMUL)	&&
            (dep->inst1->type != IRAND)	&&
            (dep->inst1->type != IROR)	&&
            (dep->inst1->type != IRXOR)	) {
        return JITFALSE;
    }
    assert(IRMETHOD_getInstructionParametersNumber(dep->inst1) == 2);

    /* Fetch the defined variable.
     */
    defVar1			= IRMETHOD_getInstructionResult(dep->inst1);

    /* Fetch the uses within the loop of the variable defined.
     */
    uses            = IRMETHOD_getVariableUsesWithinInstructions(method, (defVar1->value).v, loop->instructions);
    assert(uses != NULL);

    /* Check if the second instruction belongs to the uses set.
     */
    if (xanList_find(uses, dep->inst2) == NULL){

        /* Free the memory.
         */
        xanList_destroyList(uses);

        return JITFALSE;
    }

    /* Check every use of the defined variable.
     */
    item            = xanList_first(uses);
    while (item != NULL){
        ir_instruction_t    *inst2;
        ir_item_t       	*defVar2;

        /* Fetch the current use.
         */
        inst2           = item->data;
        assert(inst2 != NULL);

        /* Check if the instruction should be considered.
         */
        if (    (executedInsts == NULL)                                 ||
                (xanHashTable_lookup(executedInsts, inst2) != NULL)     ){
            XanListItem *item2;

            /* Check if there is a dependence between the original instruction and the current use of the defined variable.
             */
            item2 = xanList_first(loopCarriedDeps);
            while (item2 != NULL) {
                loop_inter_interation_data_dependence_t	*currentDep;
                currentDep  = item2->data;
                assert(currentDep != NULL);
                if (    ((currentDep->inst1 == dep->inst1) && (currentDep->inst2 == inst2))     ||
                        ((currentDep->inst2 == dep->inst1) && (currentDep->inst1 == inst2))     ){
                    break ;
                }
                item2       = item2->next;
            }
            if (item2 != NULL){

                /* The current use of the defined variable is executed and there is a loop-carried dependence that involves it.
                 *
                 * Fetch the variable defined by the current use.
                 */
                defVar2			= IRMETHOD_getInstructionResult(inst2);

                /* Check if we can vectorize the accesses to the defined variable.
                 *
                 * Let's start by checking whether the instructions are different or not.
                 * If they are the same, then we can vectorize the defined variable. Otherwise, further checkings are needed.
                 */
                if (dep->inst1 != inst2){

                    /* The instructions are different.
                     * Check whether they have the same type.
                     */
                    if (dep->inst1->type != inst2->type){

                        /* Free the memory.
                         */
                        xanList_destroyList(uses);

                        /* The instructions have different operands. 
                         * We cannot vectorize the defined variable.
                         */
                        return JITFALSE;
                    }
                    assert(dep->inst1->type == inst2->type);

                    /* The instructions have the same operand.
                     * Check if they define the same variable.
                     */
                    assert(IRDATA_isAVariable(defVar1));
                    assert(IRDATA_isAVariable(defVar2));
                    if ((defVar1->value).v != (defVar2->value).v){

                        /* Free the memory.
                         */
                        xanList_destroyList(uses);

                        /* The instructions define different variables.
                         * Hence, we cannot vectorize the defined variables.
                         */
                        return JITFALSE;
                    }
                    assert((defVar1->value).v == (defVar2->value).v);
                }
                assert(IRMETHOD_doesInstructionUseVariable(method, inst2, IRMETHOD_getVariableDefinedByInstruction(method, dep->inst1)));
                assert(IRMETHOD_doesInstructionUseVariable(method, dep->inst1, IRMETHOD_getVariableDefinedByInstruction(method, inst2)));
            }
        }

        /* Fetch the next element.
         */
        item    = item->next;
    }
    assert(IRMETHOD_doesInstructionUseVariable(method, dep->inst2, IRMETHOD_getVariableDefinedByInstruction(method, dep->inst1)));
    assert(IRMETHOD_doesInstructionUseVariable(method, dep->inst1, IRMETHOD_getVariableDefinedByInstruction(method, dep->inst2)));

    /* Free the memory.
     */
    xanList_destroyList(uses);

    return JITTRUE;
}

static inline JITINT32 internal_isAlwaysTrueDataDependence (ir_method_t *method, loop_inter_interation_data_dependence_t *dep) {
    ir_item_t	ba1;
    ir_item_t	ba2;
    ir_item_t	o1;
    ir_item_t	o2;

    /* Assertions			*/
    assert(method != NULL);
    assert(dep != NULL);

    /* Check if one of the          *
     * instruction is a call.	*/
    switch (dep->inst1->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
            return JITFALSE;
    }
    switch (dep->inst2->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
            return JITFALSE;
    }

    /* Check if the dependence is through memory and the location is statically known.
     */
    IRMETHOD_baseAddressAndOffsetOfAccessedMemoryLocation(dep->inst1, &ba1, &o1);
    IRMETHOD_baseAddressAndOffsetOfAccessedMemoryLocation(dep->inst2, &ba2, &o2);
    if (	(ba1.type != NOPARAM)				&&
            (ba2.type != NOPARAM)				&&
            (!IRDATA_isAVariable(&ba1))			&&
            (!IRDATA_isAVariable(&ba2))			&&
            (!IRDATA_isAVariable(&o1))			&&
            (!IRDATA_isAVariable(&o2))			&&
            (memcmp(&ba1, &ba2, sizeof(ir_item_t)) == 0)	&&
            (memcmp(&o1, &o2, sizeof(ir_item_t)) == 0)	) {
        return JITTRUE;
    }

    /* Check if the dependence is through registers.
     */
    if (    ((dep->type & DEP_MRAW) == 0)    &&
            ((dep->type & DEP_MWAW) == 0)    &&
            ((dep->type & DEP_MWAR) == 0)    ) {
        return JITTRUE;
    }

    return JITFALSE;
}

static inline void internal_computeConditionToCheck (ir_method_t *method, loop_inter_interation_data_dependence_t *dep) {

    /* Assertions			*/
    assert(method != NULL);
    assert(dep != NULL);

    switch (dep->inst1->type) {
        case IRLOADREL:
        case IRSTOREREL:
            switch (dep->inst2->type) {
                case IRLOADREL:
                case IRSTOREREL:
                    (dep->runtime_condition).conditionType = MEMORY_ALIAS_CONDITION;
                    memcpy(&((dep->runtime_condition).condition.alias.var1), IRMETHOD_getInstructionParameter1(dep->inst1), sizeof(ir_item_t));
                    memcpy(&((dep->runtime_condition).condition.alias.var2), IRMETHOD_getInstructionParameter1(dep->inst2), sizeof(ir_item_t));
                    break;
            }
            break;
    }

    /* Return			*/
    return;
}

static inline JITBOOLEAN internal_doesVariableStrictlyGrowWithIterationCount (loop_t *loop, IR_ITEM_VALUE varID, XanList *instsToCheck, JITINT32 *memoryLocations, JITBOOLEAN *cannotChangeVars, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop) {
    JITBOOLEAN	monotonic;
    JITBOOLEAN	strict;

    strict		= JITFALSE;
    monotonic	= internal_doesVariableGrowWithIterationCount(loop, varID, &strict, instsToCheck, memoryLocations, cannotChangeVars, predoms, postdoms, numInsts, variableDefinitionsWithinLoop, variableInvariantsWithinLoop);

    return monotonic && strict;
}

static inline JITBOOLEAN internal_doesVariableGrowWithIterationCount (loop_t *loop, IR_ITEM_VALUE varID, JITBOOLEAN *strict, XanList *instsToCheck, JITINT32 *memoryLocations, JITBOOLEAN *cannotChangeVars, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop, XanHashTable *variableInvariantsWithinLoop) {
    JITBOOLEAN	monotonic;
    JITBOOLEAN	*analyzedVars;
    XanStack	*workStack;
    XanList		*toRemove;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(strict != NULL);
    assert(instsToCheck != NULL);
    assert(cannotChangeVars != NULL);

    /* Initialize the variables.
     */
    (*strict)	= JITFALSE;
    monotonic	= JITTRUE;

    /* Check whether we have the memory location analysis.
     */
    if (memoryLocations == NULL) {
        return JITFALSE;
    }
    assert(memoryLocations != NULL);

    /* Allocate the necessary memory.
     */
    toRemove	= xanList_new(allocFunction, freeFunction, NULL);
    workStack	= xanStack_new(allocFunction, freeFunction, NULL);
    analyzedVars	= allocFunction(sizeof(JITBOOLEAN) * IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method));

    /* Check the variable.
     */
    xanStack_push(workStack, (void *)(JITNUINT)(varID + 1));
    while ((xanStack_getSize(workStack) > 0) && (monotonic)) {
        IR_ITEM_VALUE	    currentVarID;
        JITBOOLEAN	        dominatorFound;
        XanHashTable	    *defs;
        XanHashTableItem    *hashItem;

        /* Fetch the variable.
         */
        currentVarID			= ((IR_ITEM_VALUE)(JITNUINT)xanStack_pop(workStack)) - 1;
        analyzedVars[currentVarID]	= JITTRUE;

        /* Check if it is an induction variable.
         */
        if (LOOPS_isAnInductionVariable(loop, currentVarID)) {
            (*strict)	= JITTRUE;
            continue ;
        }

        /* Fetch the definitions of the current variable within the loop.
         */
        defs		= internal_getDefinitionsWithinLoop(loop, currentVarID, variableDefinitionsWithinLoop);
        assert(defs != NULL);

        /* Check each definition.
         */
        dominatorFound	= JITFALSE;
        hashItem		= xanHashTable_first(defs);
        while ((hashItem != NULL) && (monotonic)) {
            ir_instruction_t	*def;
            IR_ITEM_VALUE		definedVar;
            XanListItem		*item2;

            /* Fetch the definition.
             */
            def		= hashItem->element;
            assert(def != NULL);

            /* Check if the instruction defines a variable that is constant.
             */
            definedVar	= IRMETHOD_getInstructionResultValue(def);
            if (definedVar != NOPARAM) {
                if (	(cannotChangeVars[definedVar])				                                            ||
                        (internal_isAnInvariantWithinLoop(loop, definedVar, variableInvariantsWithinLoop))		) {

                    /* The current definition is ok.
                     */
                    dominatorFound	= JITTRUE;
                    hashItem		= xanHashTable_next(defs, hashItem);
                    continue ;
                }
            }

            /* Check the definition.
             */
            if (	(def->type != IRADD)		&&
                    (def->type != IRMUL)		&&
                    (def->type != IRMOVE)		) {
                ir_item_t	*baseAddr;

                /* If the instruction is not a load operation and it is not an arithmetic operation as weel, then we cannot guarantee that the variable grows with the iteration count.
                 */
                if (def->type != IRLOADREL){
                    monotonic	= JITFALSE;
                    break ;
                }
                assert((def->type == IRLOADREL));

                /* The instruction is a load operation.
                 * Fetch the base address.
                 */
                baseAddr	= IRMETHOD_getInstructionParameter1(def);

                /* If the memory location pointed by the load operation is a constant, then without further analysis we cannot conclude that it can grow with iteration count.
                 */
                if (!IRDATA_isAVariable(baseAddr)) {
                    //TODO

                } else {

                    /* Check if the base address is used in instructions involved in RAW data dependences through memory.
                     */
                    if (	(IRDATA_isAVariable(baseAddr))		&&
                            (memoryLocations[(baseAddr->value).v])	&&
                            ((baseAddr->value).v != varID)		&&
                            (!(*strict))				) {
                        JITBOOLEAN	cannotGrow;

                        /* Check if the current load instruction is not due to fetch the sub-array reference of an array.
                         */
                        cannotGrow	= JITFALSE;
                        if (baseAddr->type_infos == NULL) {
                            cannotGrow	= JITTRUE;

                        } else {
                            XanHashTable		*baDefs;

                            /* Fetch the definitions of the base address.
                             */
                            baDefs		= internal_getDefinitionsWithinLoop(loop, (baseAddr->value).v, variableDefinitionsWithinLoop);
                            if (xanHashTable_elementsInside(baDefs) >= 1) {
                                if (xanHashTable_elementsInside(baDefs) > 1) {
                                    cannotGrow	= JITTRUE;

                                } else {
                                    ir_instruction_t	*definer;
                                    ir_item_t		*add1Par;
                                    ir_item_t		*add2Par;
                                    definer	= xanHashTable_first(baDefs)->element;
                                    assert(definer != NULL);
                                    switch (definer->type) {
                                        case IRADD:
                                            add1Par		= IRMETHOD_getInstructionParameter1(definer);
                                            add2Par		= IRMETHOD_getInstructionParameter2(definer);
                                            cannotGrow	|= internal_isNotAnArraySubscriptComputation(loop, add1Par, variableDefinitionsWithinLoop);
                                            cannotGrow	|= internal_isNotAnArraySubscriptComputation(loop, add2Par, variableDefinitionsWithinLoop);
                                            /* This fall through is intentional.
                                             */
                                        case IRMUL:
                                            if (IRMETHOD_doesInstructionUseVariable(loop->method, definer, (baseAddr->value).v)) {

                                                /* We need to skip cases where:
                                                 * z   = M[v]
                                                 * ... = M[z]
                                                 *
                                                 * w   = M[v]
                                                 * M[w] = ...
                                                 * v = v + 1
                                                 *
                                                 * because w and z may be alias.
                                                 */
                                                cannotGrow	= JITTRUE;
                                            }
                                            break;
                                        default:
                                            cannotGrow	= JITTRUE;
                                    }
                                }
                            }
                        }

                        /* Check if the variable cannot grow.
                         */
                        if (cannotGrow) {
                            monotonic	= JITFALSE;
                            break ;
                        }
                    }
                }
            }
            switch (def->type) {
                case IRADD:
                case IRMUL:
                    (*strict)	= JITTRUE;
                    break;
            }

            /* Check if the current definition predominates all instructions given as input.
             */
            item2	= xanList_first(instsToCheck);
            while (item2 != NULL) {
                ir_instruction_t	*currentInst;
                currentInst	= item2->data;
                if (!TOOLS_isInstructionADominator(loop->method, numInsts, def, currentInst, predoms, IRMETHOD_isInstructionAPredominator)) {
                    break ;
                }
                item2		= item2->next;
            }
            if (item2 == NULL) {
                dominatorFound	= JITTRUE;
            }

            /* Add each variable used in these definition to be checked.
             */
            for (JITUINT32 count=1; count <= IRMETHOD_getInstructionParametersNumber(def); count++) {
                ir_item_t	*par;
                par	= IRMETHOD_getInstructionParameter(def, count);
                if (IRDATA_isAConstant(par)) {
                    if (IRMETHOD_hasAFloatType(par)) {
                        if ((par->value).f < 0) {
                            monotonic	= JITFALSE;
                            break ;
                        }
                    } else {
                        if ((par->value).v < 0) {
                            monotonic	= JITFALSE;
                            break ;
                        }
                    }
                    continue ;
                }
                if (!IRDATA_isAVariable(par)) {
                    continue ;
                }
                assert(IRDATA_isAVariable(par));
                if (analyzedVars[par->value.v]) {
                    continue;
                }
                xanStack_push(workStack, (void *)(JITNUINT)((par->value).v + 1));
            }

            /* Fetch the next element from the list.
             */
            hashItem		= xanHashTable_next(defs, hashItem);
        }

        /* Check if at least one definition of the variable predominates the instructions to check.
         */
        if (	(monotonic)				                    &&
                (xanHashTable_elementsInside(defs) > 0)		&&
                (!dominatorFound)			                ) {

            /* There are definitions of the current variable and no one predominates the instructions to check.
             */
            monotonic	= JITFALSE;
        }
    }

    /* Free the memory.
     */
    xanStack_destroyStack(workStack);
    xanList_destroyList(toRemove);
    freeFunction(analyzedVars);

    return monotonic;
}

static inline JITBOOLEAN internal_isLocalizableDataDependence (loop_t *loop, loop_inter_interation_data_dependence_t *dep, XanList *depToCheck, JITBOOLEAN *localizedVariables, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts) {
    IR_ITEM_VALUE		defVar;
    JITBOOLEAN		correct;
    ir_instruction_t	*inst1;
    ir_instruction_t	*inst2;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(dep != NULL);
    assert(depToCheck != NULL);
    assert(xanList_find(depToCheck, dep) != NULL);

    /* Check if the data dependence is through memory.
     */
    if (	((dep->type & DEP_MWAW) != 0)        ||
            ((dep->type & DEP_MWAR) != 0)        ||
            ((dep->type & DEP_MRAW) != 0)        ) {
        return JITFALSE;
    }

    /* Check if the two instructions of the data dependence can be considered as control equivalent for the localization purpose.
     */
    correct	= internal_isDominationRelationCorrectForLocalization(loop, dep->inst1, dep->inst2, predoms, postdoms, numInsts);
    if (correct) {
        inst1	= dep->inst1;
        inst2	= dep->inst2;
    } else {
        correct	= internal_isDominationRelationCorrectForLocalization(loop, dep->inst2, dep->inst1, predoms, postdoms, numInsts);
        inst2	= dep->inst1;
        inst1	= dep->inst2;
    }
    if (!correct) {
        return JITFALSE;
    }

    /* Check that the destination of the dependence (i.e., inst2) effectively defines a variable.
     */
    defVar	= IRMETHOD_getVariableDefinedByInstruction(loop->method, inst2);
    if (defVar == NOPARAM) {
        return JITFALSE;
    }

    /* Check that the source of the dependence (i.e., inst1) effectively uses the defined variable.
     */
    if (!IRMETHOD_doesInstructionUseVariable(loop->method, inst1, defVar)) {
        return JITFALSE;
    }

    /* Check the data dependence.
     */
    if (!internal_isLocalizableDirectDataDependence(loop, inst1, inst2)) {
        return JITFALSE;
    }
    if (localizedVariables != NULL) {
        localizedVariables[defVar]	= JITTRUE;
    }

    return JITTRUE;
}

static inline JITBOOLEAN internal_isLocalizableDirectDataDependence (loop_t *loop, ir_instruction_t *from, ir_instruction_t *to) {
    IR_ITEM_VALUE		defVar;
    ir_item_t		*defVarItem;
    JITBOOLEAN		localizable;
    XanList			*l;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(from != NULL);
    assert(to != NULL);

    /* Fetch the variable defined by the instruction.
     */
    defVar	= IRMETHOD_getVariableDefinedByInstruction(loop->method, to);

    /* Check that the instruction defines a variable.
     */
    if (defVar == NOPARAM) {
        return JITFALSE;
    }

    /* Check that the defined variable is not a floating point.
     */
    defVarItem	= IRMETHOD_getInstructionResult(to);
    if (IRMETHOD_hasAFloatType(defVarItem)) {
        return JITFALSE;
    }

    /* Check the type of the instruction.
     */
    if (	(!IRMETHOD_isAMathInstruction(to)) 		&&
            (to->type != IRMOVE) 				&&
            (!IRMETHOD_isAConversionInstruction(to))	) {
        return JITFALSE;
    }

    /* Check that the defined variable is not induction.
     */
    if (LOOPS_isAnInductionVariable(loop, defVar)) {
        return JITFALSE;
    }

    /* Check the chain (we have to ensure that it is a chain, trees or graphs cannot be accepted) of RAW dependences from "from" to "to".
     * These instructions must be always executed every iteration and they cannot use any memory reference.
     * Moreover, they must be control equivalent.
     */
    l		= internal_getStraightRAWChainForVariableLocalization(loop, to);
    localizable	= (xanList_length(l) > 0);

    /* Free the memory.
     */
    xanList_destroyList(l);

    return localizable;
}

static inline XanList * internal_getStraightRAWChainForVariableLocalization (loop_t *loop, ir_instruction_t *startInst) {
    ir_instruction_t	*bodyStart;
    XanStack		*toCheckInstructions;
    XanList			*alreadyChecked;
    XanList			*backedges;
    JITBOOLEAN		localizable;

    /* Initialize the variables.
     */
    localizable		= JITTRUE;

    /* Allocate the necessary memory.
     */
    toCheckInstructions	= xanStack_new(allocFunction, freeFunction, NULL);
    alreadyChecked		= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the backedges of the loop.
     */
    backedges		= LOOPS_getBackedges(loop);

    /* Fetch the start of the body.
     */
    bodyStart		= internal_fetchFirstInstructionInBody(loop);

    /* Start the search.
     */
    xanStack_push(toCheckInstructions, startInst);
    xanList_append(alreadyChecked, startInst);
    while ((xanStack_getSize(toCheckInstructions) > 0) && (localizable)) {
        XanList			*ddList;
        XanListItem		*item;
        ir_instruction_t	*tempInst;

        /* Fetch the instruction.
         */
        tempInst	= xanStack_pop(toCheckInstructions);
        assert(tempInst != NULL);
        assert(xanList_find(loop->instructions, tempInst) != NULL);

        /* Check that the current instruction is control equivalent with the destination of the data dependence (i.e., startInst).
         * The chain is:
         * ...
         * tempInst
         * ...
         * startInst
         * ...
         */
        if (!IRMETHOD_isInstructionAPredominator(loop->method, tempInst, startInst)) {
            localizable	= JITFALSE;
            break ;
        }
        assert(IRMETHOD_isInstructionAPredominator(loop->method, tempInst, startInst));
        if (!IRMETHOD_isInstructionAPostdominator(loop->method, startInst, tempInst)) {

            /* There is the case where tempInst belongs to the prologue and startInst to the body.
             * In this case, we need to check whether tempInst is control-equivalent with the beginning of the body or the header of the loop.
             */
            if (	(bodyStart != NULL)								&&
                    (!IRMETHOD_isInstructionAPredominator(loop->method, tempInst, bodyStart))	) {
                localizable	= JITFALSE;
                break;
            }
            if (	(bodyStart == NULL)								&&
                    (!IRMETHOD_isInstructionAPostdominator(loop->method, tempInst, loop->header))	) {
                localizable	= JITFALSE;
                break;
            }
        }

        /* Check that the current instruction is executed just once.
         */
        item	= xanList_first(backedges);
        while (item != NULL) {
            ir_instruction_t	*backedge;

            /* Fetch the backedge.
             */
            backedge	= item->data;
            assert(backedge != NULL);

            /* Check whether tempInst is executed once per iteration.
             */
            if (!IRMETHOD_isInstructionAPredominator(loop->method, tempInst, backedge)) {
                localizable	= JITFALSE;
                break ;
            }
            assert(IRMETHOD_isInstructionAPredominator(loop->method, tempInst, backedge));
            if (!IRMETHOD_isInstructionAPostdominator(loop->method, backedge, tempInst)) {
                if (	(bodyStart != NULL)								&&
                        (!IRMETHOD_isInstructionAPredominator(loop->method, tempInst, bodyStart))	) {
                    localizable	= JITFALSE;
                    break;
                }
                if (	(bodyStart == NULL)								&&
                        (!IRMETHOD_isInstructionAPostdominator(loop->method, tempInst, loop->header))	) {
                    localizable	= JITFALSE;
                    break;
                }
            }

            /* Fetch the next element.
             */
            item		= item->next;
        }
        if (!localizable) {
            break;
        }

        /* Check that the instruction does not access any memory location.
         */
        if (	(IRMETHOD_isAMemoryAllocationInstruction(tempInst))							||
                (IRMETHOD_isAMemoryInstruction(tempInst) && (!LOOPS_isAnInvariantInstruction(loop, tempInst)))		||
                (IRMETHOD_isACallInstruction(tempInst))									) {
            localizable	= JITFALSE;
            break;
        }

        /* Check if the instruction defines an escaped variable.
         */
        if (IRMETHOD_doesInstructionDefineAVariable(loop->method, tempInst)) {
            IR_ITEM_VALUE	definedVariable;
            definedVariable	= IRMETHOD_getVariableDefinedByInstruction(loop->method, tempInst);
            if (IRMETHOD_isAnEscapedVariable(loop->method, definedVariable)) {
                localizable	= JITFALSE;
                break;
            }
        }

        /* Check if we have already checked the entire chain.
         */
        ddList 	= IRMETHOD_getDataDependencesFromInstruction(loop->method, tempInst);
        if (ddList == NULL) {
            continue;
        }

        /* Check if the instruction belongs to any subloop.
         */
        item	= xanList_first(loop->subLoops);
        while (item != NULL) {
            loop_t	*subloop;
            subloop	= item->data;
            if (xanList_find(subloop->instructions, tempInst) != NULL) {
                localizable	= JITFALSE;
                break ;
            }
            item	= item->next;
        }

        /* Check the data dependences that the current instruction depends from.
         */
        if (localizable) {
            XanList		*toDelete;

            /* Allocate the necessary memory.
             */
            toDelete	= xanList_new(allocFunction, freeFunction, NULL);

            /* Remove dependences to instructions outside the loop.
             */
            item = xanList_first(ddList);
            while (item != NULL) {
                data_dep_arc_list_t     *dep;
                dep 	= item->data;
                if (xanList_find(loop->instructions, dep->inst) == NULL) {
                    xanList_append(toDelete, dep);
                }
                item	= item->next;
            }
            xanList_removeElements(ddList, toDelete, JITTRUE);

            item = xanList_first(ddList);
            while (item != NULL) {
                data_dep_arc_list_t     *dep;

                /* Fetch the dependence.
                 */
                dep 	= item->data;
                assert(dep != NULL);
                assert(xanList_find(loop->instructions, dep->inst) != NULL);

                /* Check whether this is a dependence through memory.
                 */
                if (	((dep->depType & DEP_MWAW) != 0)        ||
                        ((dep->depType & DEP_MWAR) != 0)        ||
                        ((dep->depType & DEP_MRAW) != 0)        ) {

                    /* The dependence is through memory.
                     * Currently we do not handle this case.
                     */
                    localizable	= JITFALSE;
                    break;
                }

                /* The dependence is through registers only.
                 * Check if it is a RAW data dependence.
                 */
                if ((dep->depType & DEP_RAW) != 0) {

                    /* Check if the instruction has been considered already.
                     */
                    if (xanList_find(alreadyChecked, dep->inst) == NULL) {

                        /* Analyze the instruction that the current one depends on.
                         */
                        xanStack_push(toCheckInstructions, dep->inst);
                        xanList_append(alreadyChecked, dep->inst);
                    }
                }

                /* Fetch the next element.
                 */
                item	= item->next;
            }

            /* Free the memory.
            */
            xanList_destroyList(toDelete);
        }

        /* Free the memory.
         */
        xanList_destroyList(ddList);
    }

    /* Prepare the list.
     */
    if (!localizable) {
        xanList_emptyOutList(alreadyChecked);
    }

    /* Sort the list.
     */
    if (xanList_length(alreadyChecked) > 0) {
        XanList		*l;
        XanListItem	*item;

        /* Create the sorted list.
         */
        l	= xanList_new(allocFunction, freeFunction, NULL);
        item	= xanList_first(alreadyChecked);
        while (item != NULL) {
            ir_instruction_t	*tempInst;
            XanListItem		*item2;
            JITBOOLEAN		added;
            tempInst	= item->data;
            added		= JITFALSE;
            item2		= xanList_first(l);
            while (item2 != NULL) {
                ir_instruction_t	*addedInst;
                addedInst	= item2->data;
                assert(tempInst != addedInst);
                if (IRMETHOD_isInstructionAPredominator(loop->method, tempInst, addedInst)) {
                    xanList_insertBefore(l, item2, tempInst);
                    added	= JITTRUE;
                    break;
                }
                item2		= item2->next;
            }
            if (!added) {
                xanList_append(l, tempInst);
            }
            item		= item->next;
        }
        assert(xanList_length(l) == xanList_length(alreadyChecked));

        /* Swap the lists.
         */
        xanList_destroyList(alreadyChecked);
        alreadyChecked	= l;
    }

    /* Free the memory.
     */
    xanStack_destroyStack(toCheckInstructions);
    xanList_destroyList(backedges);

    return alreadyChecked;
}

static inline JITBOOLEAN internal_doesMemoryAddressChangeAtEveryIteration (loop_t *loop, IR_ITEM_VALUE varID, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop) {
    XanHashTable			*defs;
    XanHashTableItem		*item;
    JITBOOLEAN		        changeEveryIteration;

    /* Assertions.
     */
    assert(loop != NULL);

    /* Fetch the definitions of the variable within the loop.
     */
    defs		= internal_getDefinitionsWithinLoop(loop, varID, variableDefinitionsWithinLoop);
    assert(defs != NULL);

    /* Check if the variable is defined based on its value defined in the previous iteration.
     */
    changeEveryIteration	= JITFALSE;
    item			= xanHashTable_first(defs);
    while (item != NULL) {
        ir_instruction_t	*def;

        /* Fetch the definition.
         */
        def		= item->element;
        assert(def != NULL);

        /* Check if the current definition allows the memory address to stay the same among a couple of iterations.
         */
        if (!internal_doesMemoryAddressChangeAtEveryIterationForTheDefinition(loop, varID, predoms, postdoms, numInsts, def)) {
            break ;
        }

        /* Fetch the next element from the list.
         */
        item		= xanHashTable_next(defs, item);
    }
    if (	(item == NULL)			                &&
            (xanHashTable_elementsInside(defs) > 0)	){
        changeEveryIteration	= JITTRUE;
    }

    return changeEveryIteration;
}

static inline JITBOOLEAN internal_doesMemoryAddressChangeAtEveryIterationForTheDefinition (loop_t *loop, IR_ITEM_VALUE varID, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, ir_instruction_t *def) {
    ir_item_t		*otherPar;
    XanList			*backedges;
    XanList			*uses;
    XanListItem		*item;
    JITBOOLEAN		found;

    /* Check if the variable is defined based on its value defined in the previous iteration.
     */
    if (!IRMETHOD_doesInstructionUseVariable(loop->method, def, varID)) {
        XanList		*backedges;
        JITBOOLEAN	getFreed;

        /* The variable is defined not based on its value defined in the previous iteration.
         * Check if it is a value returned by well-known functions.
         */
        if (	(internal_isDefinedVariableDefinedByMallocAndPredominatesAllUses(loop, def, predoms, postdoms, numInsts))	||
                (internal_isDefinedVariableFreeByFreeAndPredominatesBackedges(loop, def, predoms, postdoms, numInsts))		) {
            return JITTRUE;
        }

        /* Fetch the backedges.
         */
        backedges	= LOOPS_getBackedges(loop);
        assert(backedges != NULL);

        /* Check if the memory is going to be freed before leaving the iteration.
         * Fetch the uses of the variable.
         */
        uses		= IRMETHOD_getVariableUses(loop->method, varID);
        assert(uses != NULL);

        /* Check if the current use frees the memory at every iteration.
         */
        getFreed	= JITFALSE;
        item		= xanList_first(uses);
        while (item != NULL) {
            ir_instruction_t	*use;
            XanListItem		*item2;
            use	= item->data;
            assert(use != NULL);
            switch (use->type) {
                case IRFREE:
                case IRFREEOBJ:
                    item2		= xanList_first(backedges);
                    while (item2 != NULL) {
                        ir_instruction_t	*backedge;
                        backedge	= item2->data;
                        assert(backedge != NULL);
                        if (!TOOLS_isInstructionADominator(loop->method, numInsts, use, backedge, predoms, IRMETHOD_isInstructionAPredominator)) {
                            break;
                        }
                        item2		= item2->next;
                    }
                    if (item2 == NULL) {
                        getFreed	= JITTRUE;
                    }
                    break;
            }
            item	= item->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(uses);
        xanList_destroyList(backedges);

        return getFreed;
    }

    /* Check if the definition is executed at every iteration.
     */
    found		= JITFALSE;
    backedges	= LOOPS_getBackedges(loop);
    item		= xanList_first(backedges);
    while (item != NULL) {
        ir_instruction_t	*back;
        back	= item->data;
        if (!TOOLS_isInstructionADominator(loop->method, numInsts, back, def, postdoms, IRMETHOD_isInstructionAPostdominator)) {
            found	= JITTRUE;
            break;
        }
        if (!TOOLS_isInstructionADominator(loop->method, numInsts, def, back, predoms, IRMETHOD_isInstructionAPredominator)) {
            found	= JITTRUE;
            break;
        }
        item	= item->next;
    }
    xanList_destroyList(backedges);
    if (found) {
        return JITFALSE;
    }

    /* Check that after the definition there is no uses otherwise adjacent iterations will share the same value for a bit.
     */
    uses		= IRMETHOD_getVariableUsesWithinInstructions(loop->method, varID, loop->instructions);
    found		= JITFALSE;
    item		= xanList_first(uses);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (	(!TOOLS_isInstructionADominator(loop->method, numInsts, def, inst, postdoms, IRMETHOD_isInstructionAPostdominator))	&&
                (!TOOLS_isInstructionADominator(loop->method, numInsts, inst, def, predoms, IRMETHOD_isInstructionAPredominator))	) {
            found	= JITTRUE;
            break;
        }
        item	= item->next;
    }
    xanList_destroyList(uses);
    if (found) {
        return JITFALSE;
    }

    /* Check the type of the unique definition.
     */
    otherPar	= IRMETHOD_getInstructionParameter1(def);
    if (	(otherPar->type == IROFFSET)	&&
            ((otherPar->value).v == varID)	) {
        otherPar	= IRMETHOD_getInstructionParameter2(def);
    }
    switch (def->type) {
        case IRADD:
        case IRSUB:
        case IRMUL:
        case IRDIV:
            if (	(!IRDATA_isAConstant(otherPar))		&&
                    (!IRDATA_isASymbol(otherPar))		) {
                return JITFALSE;
            }
            break;
        case IRLOADREL:
            if (	(!IRDATA_isAConstant(otherPar))		&&
                    (!IRDATA_isASymbol(otherPar))		) {
                return JITFALSE;
            }
            break;
        default:
            return JITFALSE;
    }

    return JITTRUE;
}

static inline JITBOOLEAN * internal_getCannotBeAliasVariables (loop_t *loop) {
    JITUINT32	vars;
    JITBOOLEAN 	*cannotBeAliasVariables;

    vars			= IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method);
    cannotBeAliasVariables	= allocFunction(sizeof(JITBOOLEAN) * (vars + 1));
    for (JITUINT32 count=0; count < vars; count++) {
        XanList		*defs;

        /* Fetch the definitions of the current variable.
         */
        defs		= IRMETHOD_getVariableDefinitions(loop->method, count);
        assert(defs != NULL);

        /* Check the definitions.
         */
        if (xanList_length(defs) == 1) {
            ir_instruction_t	*def;

            /* Fetch the single definition.
             */
            def	= (ir_instruction_t *)xanList_first(defs)->data;
            assert(def != NULL);

            /* Check if it is an allocation of dynamic memory outside the loop.
             * Notice that allocation within the loop are handled not here.
             */
            if (xanList_find(loop->instructions, def) == NULL) {
                ir_method_t	*callee;
                switch (def->type) {
                    case IRCALL:
                        callee	= IRMETHOD_getCalledMethod(loop->method, def);
                        if (	(callee == NULL)						||
                                (STRCMP(IRMETHOD_getMethodName(callee), "malloc") != 0)		) {
                            break;
                        }
                    case IRALLOC:
                    case IRALLOCALIGN:
                    case IRCALLOC:
                    case IRREALLOC:
                    case IRNEWOBJ:
                    case IRNEWARR:

                        /* The variable is defined by a call to malloc.
                         * Check if the variable is not an escaped one.
                         */
                        if (!IRMETHOD_isAnEscapedVariable(loop->method, count)) {
                            JITBOOLEAN	isACallParameter;

                            /* Check if the variable is passed to other methods.
                             */
                            isACallParameter	= JITFALSE;
                            for (JITUINT32 instID=0; instID < IRMETHOD_getInstructionsNumber(loop->method); instID++) {
                                ir_instruction_t	*inst;
                                inst	= IRMETHOD_getInstructionAtPosition(loop->method, instID);
                                assert(inst != NULL);
                                if (!IRMETHOD_isACallInstruction(inst)) {
                                    continue ;
                                }
                                for (JITUINT32 count2=0; count2 < IRMETHOD_getInstructionCallParametersNumber(inst); count2++) {
                                    ir_item_t 	*callPar;
                                    callPar	= IRMETHOD_getInstructionCallParameter(inst, count2);
                                    assert(callPar != NULL);
                                    if (!IRDATA_isAVariable(callPar)) {
                                        continue;
                                    }
                                    if ((callPar->value).v == count) {

                                        /* The variable has been passed to other method.
                                         * Therefore, it can be alias to other variables.
                                         */
                                        isACallParameter	= JITTRUE;
                                    }
                                }
                            }
                            if (!isACallParameter) {
                                cannotBeAliasVariables[count]	= JITTRUE;
                            }
                        }
                        break;
                }
            }
        }

        /* Free the memory.
         */
        xanList_destroyList(defs);
    }

    return cannotBeAliasVariables;
}

static inline JITBOOLEAN * internal_getUniqueValueVariables (loop_t *loop, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *variableDefinitionsWithinLoop) {
    JITUINT32	vars;
    JITBOOLEAN 	*uniqueValuesVariables;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(loop->method != NULL);

    /* Allocate the memory to store information about variables of the method that includes the loop.
     */
    vars			= IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method);
    uniqueValuesVariables	= allocFunction(sizeof(JITBOOLEAN) * (vars + 1));

    /* Analyze the variables to identify the ones that will have unique values per loop iteration.
     */
    for (JITUINT32 count=0; count < vars; count++) {
        if (internal_doesMemoryAddressChangeAtEveryIteration(loop, count, predoms, postdoms, numInsts, variableDefinitionsWithinLoop)) {
            uniqueValuesVariables[count]	= JITTRUE;
        }
    }

    return uniqueValuesVariables;
}

static inline JITBOOLEAN * internal_getCannotChangeVariables (loop_t *loop, XanHashTable *variableDefinitionsWithinLoop) {
    JITUINT32	vars;
    JITBOOLEAN 	*cannotChangeVariables;
    JITBOOLEAN	modified;

    /* Assertions.
     */
    assert(loop != NULL);

    /* Allocate the memory.
     */
    vars			= IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method);
    cannotChangeVariables	= allocFunction(sizeof(JITBOOLEAN) * (vars + 1));

    /* Analyze the variables.
     */
    do {
        modified	= JITFALSE;
        for (JITUINT32 count=0; count < vars; count++) {
            XanHashTable		*defs;
            XanHashTableItem	*item;

            /* If the variable escapes, then we cannot say anything.
             */
            if (IRMETHOD_isAnEscapedVariable(loop->method, count)) {
                continue ;
            }

            /* Fetch the definitions of the current variable.
             */
            defs		= internal_getDefinitionsWithinLoop(loop, count, variableDefinitionsWithinLoop);
            assert(defs != NULL);

            /* Check whether there are definitions of the variable within the loop.
             */
            if (xanHashTable_elementsInside(defs) == 0) {
                if (!cannotChangeVariables[count]) {
                    cannotChangeVariables[count]	= JITTRUE;
                    modified			= JITTRUE;
                }
                continue ;
            }

            /* Check the definition.
             */
            item	= xanHashTable_first(defs);
            while (item != NULL) {
                ir_instruction_t	*def;

                /* Fetch the single definition.
                 */
                def	= (ir_instruction_t *)item->element;
                assert(def != NULL);
                assert(xanList_find(loop->instructions, def) != NULL);

                /* Check that the definition rely on values that do not change.
                 */
                if (	IRMETHOD_isAMemoryInstruction(def)		||
                        IRMETHOD_isAMemoryAllocationInstruction(def)	||
                        IRMETHOD_isACallInstruction(def)		) {
                    break;
                }

                /* Check the trivial cases.
                 */
                if (def->type != IRGETADDRESS) {
                    JITBOOLEAN	canChange;

                    /* Check the parameter of the current variable.
                     */
                    canChange	= JITFALSE;
                    for (JITUINT32 par=1; par <= IRMETHOD_getInstructionParametersNumber(def); par++) {
                        ir_item_t	*parInst;
                        parInst	= IRMETHOD_getInstructionParameter(def, par);
                        if (!IRDATA_isAVariable(parInst)) {
                            continue ;
                        }
                        if (!cannotChangeVariables[(parInst->value).v]) {
                            canChange	= JITTRUE;
                            break;
                        }
                    }
                    if (canChange) {
                        break;
                    }
                }

                /* Fetch the next element from the list.
                 */
                item	= xanHashTable_next(defs, item);
            }
            if (item == NULL) {
                if (!cannotChangeVariables[count]) {
                    cannotChangeVariables[count]	= JITTRUE;
                    modified			= JITTRUE;
                }
            }
        }
    } while (modified);

    return cannotChangeVariables;
}

static inline JITINT32 * internal_findMemoryLocationStatus (loop_t *loop) {
    XanListItem	*item;
    JITINT32	*memoryLocations;
    JITUINT32	vars;

    /* Allocate the necessary memory.
     */
    vars		= IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method);
    memoryLocations	= allocFunction(sizeof(JITINT32) * vars);

    /* Tag the memory locations.
     */
    item		= xanList_first(loop->instructions);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (IRMETHOD_isAMemoryInstruction(inst)) {
            XanList  	*ddList;
            XanListItem	*item2;
            ir_item_t	*baseAddr;
            switch (inst->type) {
                case IRLOADREL:
                    baseAddr	= IRMETHOD_getInstructionParameter1(inst);
                    if (!IRDATA_isAVariable(baseAddr)) {
                        break;
                    }
                    ddList 	= IRMETHOD_getDataDependencesFromInstruction(loop->method, inst);
                    if (ddList == NULL) {
                        break;
                    }
                    item2		= xanList_first(ddList);
                    while (item2 != NULL) {
                        data_dep_arc_list_t     *dep;
                        dep	= item2->data;
                        if (	(xanList_find(loop->instructions, dep->inst) != NULL) 	&&
                                ((dep->depType & DEP_MRAW) == DEP_MRAW)        		) {
                            memoryLocations[(baseAddr->value).v]	= 1;
                            if (IRMETHOD_doesInstructionUseVariable(loop->method, dep->inst, (baseAddr->value).v)) {
                                memoryLocations[(baseAddr->value).v]	= 2;
                            }
                            break;
                        }
                        item2	= item2->next;
                    }
                    xanList_destroyList(ddList);
                    break;
                case IRSTOREREL:
                    ddList 	= IRMETHOD_getDataDependencesToInstruction(loop->method, inst);
                    if (ddList == NULL) {
                        break;
                    }
                    item2		= xanList_first(ddList);
                    while (item2 != NULL) {
                        data_dep_arc_list_t     *dep;
                        dep		= item2->data;
                        if (	(xanList_find(loop->instructions, dep->inst) != NULL) 	&&
                                (dep->inst->type == IRLOADREL)	                        ) {
                            baseAddr	= IRMETHOD_getInstructionParameter1(dep->inst);
                            if (	(IRDATA_isAVariable(baseAddr))		&&
                                    (memoryLocations[(baseAddr->value).v] == 0)	) {
                                memoryLocations[(baseAddr->value).v]	= 1;
                                if (IRMETHOD_doesInstructionUseVariable(loop->method, inst, (baseAddr->value).v)) {
                                    memoryLocations[(baseAddr->value).v]	= 2;
                                }
                            }
                        }
                        item2	= item2->next;
                    }
                    xanList_destroyList(ddList);
                    break;
            }
        }
        item	= item->next;
    }

    return memoryLocations;
}

static inline XanHashTable * internal_getDefinitionsWithinLoop (loop_t *loop, IR_ITEM_VALUE varID, XanHashTable *variableDefinitionsWithinLoop) {
    XanList 	    *defs;
    XanHashTable    *defsTable;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(variableDefinitionsWithinLoop != NULL);

    /* Check whether we have computed the variable already.
     */
    defsTable  = xanHashTable_lookup(variableDefinitionsWithinLoop, (void *)(JITNUINT)(varID + 1));
    if (defsTable != NULL){
        return defsTable;
    }

    /* Fetch the definitions of the current variable.
     */
    defs		= IRMETHOD_getVariableDefinitionsWithinInstructions(loop->method, varID, loop->instructions);

    /* Generate the hash table.
     */
    defsTable   = xanList_toHashTable(defs);
    xanHashTable_insert(variableDefinitionsWithinLoop, (void *)(JITNUINT)(varID + 1), defsTable);

    /* Free the memory.
     */
    xanList_destroyList(defs);

    return defsTable;
}

static inline JITBOOLEAN internal_isNotAnArraySubscriptComputation (loop_t *loop, ir_item_t *parOfDefinerOfBaseAddress, XanHashTable *variableDefinitionsWithinLoop) {
    XanHashTable		*add1Defs;
    JITBOOLEAN	        cannotGrow;

    /* Check the parameter.
     */
    if (!IRDATA_isAVariable(parOfDefinerOfBaseAddress)) {
        return JITFALSE;
    }

    /* Fetch the definer of the parameter within the loop.
     */
    cannotGrow	= JITFALSE;
    add1Defs	= internal_getDefinitionsWithinLoop(loop, (parOfDefinerOfBaseAddress->value).v, variableDefinitionsWithinLoop);
    if (xanHashTable_elementsInside(add1Defs) > 1) {
        cannotGrow	= JITTRUE;

    } else if (xanHashTable_elementsInside(add1Defs) == 1) {
        ir_instruction_t	*par1Definer;
        ir_item_t		*par1DefinerPar2;

        /* Fetch the single definer of the parameter within the loop.
         */
        par1Definer	= xanHashTable_first(add1Defs)->element;
        assert(par1Definer != NULL);
        par1DefinerPar2	= IRMETHOD_getInstructionParameter2(par1Definer);

        /* Check the single definer.
         */
        if (	(par1Definer->type == IRLOADREL)								&&
                (IRDATA_isAConstant(par1DefinerPar2))								&&
                (internal_doesExistADataDependenceOfTypeWithinLoop(loop, par1Definer, DEP_MRAW | DEP_MWAR))	) {
            cannotGrow	= JITTRUE;
        }
    }

    return cannotGrow;
}

static inline JITBOOLEAN internal_doesExistADataDependenceOfTypeWithinLoop (loop_t *loop, ir_instruction_t *inst, JITUINT16 depType) {
    XanListItem	*item;
    JITBOOLEAN	found;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(loop->instructions != NULL);

    found	= JITFALSE;
    item 	= xanList_first(loop->instructions);
    while ((item != NULL) && (!found)) {
        XanList		*ddList;

        /* Fetch the instruction.
         */
        inst = item->data;
        assert(inst != NULL);

        /* Check if there is a inter loop-iteration data dependence with the current instruction.
         */
        ddList 	= IRMETHOD_getDataDependencesFromInstruction(loop->method, inst);
        if (ddList != NULL) {
            XanListItem		*item2;
            data_dep_arc_list_t     *dep;
            item2 = xanList_first(ddList);
            while (item2 != NULL) {

                /* Fetch the data dependence.
                 */
                dep = item2->data;
                assert(dep != NULL);
                assert(dep->inst != NULL);

                /* Check if it is between instructions within the loop.
                 */
                if (xanList_find(loop->instructions, dep->inst) != NULL) {

                    /* We found a dependence between the input instruction and another one (i.e., dep->inst).
                     * Check whether it is of type specified as input.
                     */
                    if ((dep->depType & depType) == depType) {
                        found	= JITTRUE;
                        break;
                    }
                }

                /* Fetch the next element from the list.
                 */
                item2 = item2->next;
            }

            /* Free the memory.
             */
            xanList_destroyList(ddList);
        }

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    return found;
}

static inline JITBOOLEAN internal_haveVariablesDifferentValuesAlways (loop_t *loop, IR_ITEM_VALUE var1, IR_ITEM_VALUE var2, JITBOOLEAN *cannotChangeVars, XanHashTable *variableDefinitionsWithinLoop) {
    XanHashTable		*base1Defs;
    XanHashTable		*base2Defs;
    JITBOOLEAN	        theyAreDifferent;

    /* Assertions.
     */
    assert(loop != NULL);

    /* Check trivial cases.
     */
    if (var1 == var2) {
        return JITFALSE;
    }

    /* If variables cannot change over iterations and they have different values, they cannot store inside the same address.
     */
    if (	(cannotChangeVars[var1])	&&
            (cannotChangeVars[var2])	) {
        if (internal_variablesHaveDifferentValues(loop->method, var1, var2)) {
            return JITTRUE;
        }
    }

    /* Initialize the variables.
     */
    theyAreDifferent	= JITFALSE;

    /* Fetch the definitions.
     */
    base1Defs       = internal_getDefinitionsWithinLoop(loop, var1, variableDefinitionsWithinLoop);
    base2Defs       = internal_getDefinitionsWithinLoop(loop, var2, variableDefinitionsWithinLoop);
    assert(base1Defs != NULL);
    assert(base2Defs != NULL);

    /* We handle single definitions only.
     */
    if (	(xanHashTable_elementsInside(base1Defs) <= 1)	&&
            (xanHashTable_elementsInside(base2Defs) <= 1)	) {
        //theyAreDifferent	= JITTRUE; TODO
    }

    return theyAreDifferent;
}

static inline JITBOOLEAN internal_dependenceDueToThreadSafeCall (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dependentInst, JITUINT16 depType) {

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);
    assert(dependentInst != NULL);

    if (	((depType & DEP_RAW) == 0)							&&
            (internal_directDependenceDueToThreadSafeCall(method, inst, dependentInst))	) {
        return JITTRUE;
    }
    if (	((depType & DEP_WAR) == 0)							&&
            (internal_directDependenceDueToThreadSafeCall(method, dependentInst, inst))	) {
        return JITTRUE;
    }

    return JITFALSE;
}

static inline JITBOOLEAN internal_directDependenceDueToThreadSafeCall (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    ir_method_t 	*callee;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst1 != NULL);
    assert(inst2 != NULL);

    if (	(!IRMETHOD_isACallInstruction(inst1))		&&
            (!IRMETHOD_isACallInstruction(inst2))		) {
        return JITFALSE;
    }

    if (inst1->type == IRCALL) {
        callee	= IRMETHOD_getCalledMethod(method, inst1);
        if (internal_isMethodThreadSafe(callee)) {
            return JITTRUE;
        }
    }
    if (inst2->type == IRCALL) {
        callee	= IRMETHOD_getCalledMethod(method, inst2);
        if (internal_isMethodThreadSafe(callee)) {
            return JITTRUE;
        }
    }

    return JITFALSE;
}

static inline JITBOOLEAN internal_isMethodThreadSafe (ir_method_t *method) {
    JITINT8	*name;

    if (method == NULL) {
        return JITFALSE;
    }

    if (!IRPROGRAM_doesMethodBelongToALibrary(method)) {
        return JITFALSE;
    }
    name	= IRMETHOD_getMethodName(method);
    if (	(STRCMP(name, "malloc") == 0)	||
            (STRCMP(name, "free") == 0)	||
            (STRCMP(name, "realloc") == 0)	||
            (STRCMP(name, "exit") == 0)	) {
        return JITTRUE;
    }
    return JITFALSE;
}

static inline JITBOOLEAN internal_isDefinedVariableFreeByFreeAndPredominatesBackedges (loop_t *loop, ir_instruction_t *def, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts) {
    JITINT8		*name;
    XanListItem	*item;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(def != NULL);

    /* Check whether the instruction does not rely on dynamic memory allocation services.
     */
    switch (def->type) {
        case IRCALL:
            name	= IRMETHOD_getMethodName(loop->method);
            if (	(STRCMP(name, "free") != 0)	&&
                    (STRCMP(name, "realloc") != 0)	) {
                return JITFALSE;
            }
        case IRFREE:
        case IRFREEOBJ:
        case IRREALLOC:
            break;
        default:
            return JITFALSE;
    }

    /* Check whether the memory is freed before every backedge.
     */
    item		= xanList_first(loop->backedges);
    while (item != NULL) {
        ir_instruction_t 	*backedge;
        backedge	= item->data;
        assert(backedge != NULL);
        if (!TOOLS_isInstructionADominator(loop->method, numInsts, def, backedge, predoms, IRMETHOD_isInstructionAPredominator)) {
            break;
        }
        item	= item->next;
    }

    return (item == NULL);
}

static inline JITBOOLEAN internal_isDefinedVariableDefinedByMallocAndPredominatesAllUses (loop_t *loop, ir_instruction_t *def, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts) {
    IR_ITEM_VALUE	varID;
    JITINT8		*name;
    XanList		*toDelete;
    XanList		*uses;
    XanListItem	*item;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(def != NULL);

    /* Check whether the instruction does not rely on dynamic memory allocation services.
     */
    switch (def->type) {
        case IRCALL:
            name	= IRMETHOD_getMethodName(loop->method);
            if (	(STRCMP(name, "malloc") != 0)	&&
                    (STRCMP(name, "realloc") != 0)	) {
                return JITFALSE;
            }
        case IRALLOC:
        case IRALLOCALIGN:
        case IRREALLOC:
        case IRNEWOBJ:
        case IRNEWARR:
            break;
        default:
            return JITFALSE;
    }

    /* Fetch the defined variable.
     */
    varID	= IRMETHOD_getVariableDefinedByInstruction(loop->method, def);
    assert(varID != NOPARAM);

    /* Fetch the uses of the variable within the loop.
     */
    toDelete	= xanList_new(allocFunction, freeFunction, NULL);
    uses		= IRMETHOD_getVariableUses(loop->method, varID);
    item		= xanList_first(uses);
    while (item != NULL) {
        if (xanList_find(loop->instructions, item->data) == NULL) {
            xanList_append(toDelete, item->data);
        }
        item	= item->next;
    }
    xanList_removeElements(uses, toDelete, JITTRUE);

    /* Check whether all uses that the current definition reaches are predominated by the definition.
     */
    item		= xanList_first(uses);
    while (item != NULL) {
        XanList			*reachingDefs;
        ir_instruction_t 	*use;
        JITBOOLEAN		doesDefReachesUse;

        /* Fetch the use.
         */
        use		= item->data;
        assert(use != NULL);

        /* Check if the definition reaches the current use.
         */
        reachingDefs		= IRMETHOD_getVariableDefinitionsThatReachInstruction(loop->method, use, varID);
        doesDefReachesUse	= (xanList_find(reachingDefs, use) != NULL);
        xanList_destroyList(reachingDefs);

        /* If the definition reaches the current use and it does not predominate it, then the memory address can be shared among different iterations.
         */
        if (	(doesDefReachesUse)													&&
                (!TOOLS_isInstructionADominator(loop->method, numInsts, def, use, predoms, IRMETHOD_isInstructionAPredominator))	) {
            break;
        }

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(toDelete);
    xanList_destroyList(uses);

    return (item == NULL);
}

static inline JITBOOLEAN internal_isDominationRelationCorrectForLocalization (loop_t *loop, ir_instruction_t *inst1, ir_instruction_t *inst2, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts) {
    ir_instruction_t	*bodyStart;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(inst1 != NULL);
    assert(inst2 != NULL);
    assert(predoms != NULL);
    assert(postdoms != NULL);

    /* Check the dominance relation between instructions: if they are control independent then they are ok.
     */
    if (TOOLS_isInstructionADominator(loop->method, numInsts, inst1, inst2, predoms, IRMETHOD_isInstructionAPredominator)) {
        XanListItem	*item;

        if (!TOOLS_isInstructionADominator(loop->method, numInsts, inst2, inst1, postdoms, IRMETHOD_isInstructionAPostdominator)) {
            return JITFALSE;
        }
        item		= xanList_first(loop->backedges);
        while (item != NULL) {
            ir_instruction_t	*backedge;

            /* Fetch a backedge of the loop.
             */
            backedge	= item->data;
            assert(backedge != NULL);

            /* Check whether the current backedge is control equivalent to  inst2.
             */
            if (	(!TOOLS_isInstructionADominator(loop->method, numInsts, backedge, inst2, postdoms, IRMETHOD_isInstructionAPostdominator))	&&
                    (!TOOLS_isInstructionADominator(loop->method, numInsts, inst2, backedge, predoms, IRMETHOD_isInstructionAPredominator))		) {
                break;
            }

            /* Fetch the next element from the list.
             */
            item		= item->next;
        }
        if (item == NULL) {
            return JITTRUE;
        }
    }

    /* Check if we have a single bodyStart point.
     */
    bodyStart	= internal_fetchFirstInstructionInBody(loop);
    if (bodyStart == NULL) {
        return JITFALSE;
    }

    /* Check if one of the instruction is executed at every iteration.
     * The pattern we are looking for is:
     *
     * header
     * ...
     * inst1
     * ...
     * exitInst
     * ----- End prologue
     * startBody
     * ...
     * inst2
     */
    if (!TOOLS_isInstructionADominator(loop->method, numInsts, inst1, bodyStart, predoms, IRMETHOD_isInstructionAPredominator)) {
        return JITFALSE;
    }
    if (!TOOLS_isInstructionADominator(loop->method, numInsts, bodyStart, inst2, predoms, IRMETHOD_isInstructionAPredominator)) {
        return JITFALSE;
    }
    if (!TOOLS_isInstructionADominator(loop->method, numInsts, inst2, bodyStart, postdoms, IRMETHOD_isInstructionAPostdominator)) {
        return JITFALSE;
    }

    return JITTRUE;
}

static inline ir_instruction_t * internal_fetchFirstInstructionInBody (loop_t *loop) {
    ir_instruction_t	*bodyStart;
    XanListItem		*item;

    /* Fetch the first instruction of the body.
     */
    bodyStart	= NULL;
    item		= xanList_first(loop->exits);
    while (item != NULL) {
        ir_instruction_t	*singleExit;
        ir_instruction_t	*succ;
        XanListItem		*item2;

        /* Fetch an exit of the loop.
         */
        singleExit	= item->data;
        assert(singleExit != NULL);

        /* Fetch the successor of the current exit within the loop.
         */
        succ		= IRMETHOD_getSuccessorInstruction(loop->method, singleExit, NULL);
        while (succ != NULL) {
            if (xanList_find(loop->instructions, succ) != NULL) {
                break;
            }
            succ		= IRMETHOD_getSuccessorInstruction(loop->method, singleExit, succ);
        }
        assert(succ != NULL);

        /* Check if the current successor is the bodyStart.
         */
        item2		= xanList_first(loop->backedges);
        while (item2 != NULL) {
            ir_instruction_t	*backedge;

            /* Fetch a backedge of the loop.
             */
            backedge	= item2->data;
            assert(backedge != NULL);

            /* Check whether the current successor is postdominated by the backedge.
             */
            if (!IRMETHOD_isInstructionAPostdominator(loop->method, backedge, succ)) {
                succ	= NULL;
                break;
            }

            /* Fetch the next element.
             */
            item2		= item2->next;
        }
        if (succ != NULL) {
            bodyStart	= succ;
            break;
        }

        /* Fetch the next element.
         */
        item	= item->next;
    }

    return bodyStart;
}

static inline JITBOOLEAN internal_variablesHaveDifferentValues (ir_method_t *m, IR_ITEM_VALUE var1, IR_ITEM_VALUE var2) {
    XanList 		*defs1;
    XanList 		*defs2;
    JITUINT32		defs1Num;
    JITUINT32		defs2Num;
    ir_instruction_t	*def1;
    ir_instruction_t	*def2;

    /* Assertions.
     */
    assert(m != NULL);

    /* Initialize the variables.
     */
    def1	= NULL;
    def2	= NULL;

    /* Fetch the definitions.
     */
    defs1		= IRMETHOD_getVariableDefinitions(m, var1);
    defs2		= IRMETHOD_getVariableDefinitions(m, var2);
    defs1Num	= xanList_length(defs1);
    defs2Num	= xanList_length(defs2);

    /* Fetch the first definitions.
     * Currently we support only single definitions.
     */
    if (defs1Num == 1) {
        def1	= xanList_first(defs1)->data;
    }
    if (defs2Num == 1) {
        def2	= xanList_first(defs2)->data;
    }

    /* Free the memory.
     */
    xanList_destroyList(defs1);
    xanList_destroyList(defs2);

    /* Check if we have single definitions.
     */
    if (	(def1 == NULL)	||
            (def2 == NULL)	||
            (def1 == def2)	) {
        return JITFALSE;
    }

    /* If the two single definitions have different types, then the two variables will have different values.
     */
    if (def1->type != def2->type) {
        return JITTRUE;
    }

    /* Check the parameters.
     */
    for (JITUINT32 c=1; c <= IRMETHOD_getInstructionParametersNumber(def1); c++) {
        ir_item_t	*p1;
        ir_item_t	*p2;
        p1	= IRMETHOD_getInstructionParameter(def1, c);
        p2	= IRMETHOD_getInstructionParameter(def2, c);
        if (memcmp(p1, p2, sizeof(ir_item_t)) == 0) {
            continue ;
        }
        if (	IRDATA_isAConstant(p1)	&&
                IRDATA_isAConstant(p2)	) {
            if (IRMETHOD_hasAFloatType(p1)) {
                if ((p1->value).f != (p2->value).f) {
                    return JITTRUE;
                }
                continue ;
            }
            if ((p1->value).v != (p2->value).v) {
                return JITTRUE;
            }
            continue ;
        }
        if (	IRDATA_isAVariable(p1)	&&
                IRDATA_isAVariable(p2)	) {
            if ((p1->value).v == (p2->value).v) {
                continue ;
            }
            if (internal_variablesHaveDifferentValues(m, (p1->value).v, (p2->value).v)) {
                return JITTRUE;
            }
        }
    }

    return JITFALSE;
}

static inline void internal_addDataDependence_helpfunction (XanList *depToCheck, XanHashTable **depToCheckTable, loop_inter_interation_data_dependence_t *dep){
    XanHashTable    *t;

    /* Append the dependence to the list.
     */
    xanList_append(depToCheck, dep);

    /* Add the dependence to the table.
     */
    if (depToCheckTable != NULL){

            /* Fetch the table.
             */
            t   = depToCheckTable[dep->inst1->ID];
            if (t == NULL){
                t   = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
                depToCheckTable[dep->inst1->ID] = t;
            }
            assert(depToCheckTable[dep->inst1->ID] != NULL);

            /* Add the dependence to the table.
             */
            xanHashTable_insert(t, dep->inst2, dep);
    }

    return ;
}

static inline JITBOOLEAN internal_isAnInvariantWithinLoop (loop_t *loop, IR_ITEM_VALUE varID, XanHashTable *variableInvariantsWithinLoop){
    JITINT32    previousResult;

    previousResult  = (JITNUINT)xanHashTable_lookup(variableInvariantsWithinLoop, (void *)(JITNUINT)(varID + 1));
    if (previousResult != 0){
        return (previousResult != 1);
    }

    if (LOOPS_isAnInvariantVariable(loop, varID)){
        xanHashTable_insert(variableInvariantsWithinLoop, (void *)(JITNUINT)(varID + 1), (void *)(JITNUINT)(2));
        return JITTRUE;
    }

    xanHashTable_insert(variableInvariantsWithinLoop, (void *)(JITNUINT)(varID + 1), (void *)(JITNUINT)(1));
    return JITFALSE;
}
