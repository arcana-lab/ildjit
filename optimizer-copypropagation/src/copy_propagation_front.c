/*
 * Copyright (C) 2006 - 2013  Campanoni Simone
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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <copy_propagation.h>
// End

static inline JITINT32 internal_updateFrontPropagation (ir_method_t *method, ir_instruction_t *inst, JITBOOLEAN *toRename);
static inline JITINT32 internal_updateFrontPropagation2 (ir_method_t *method, ir_instruction_t *inst);
static inline JITBOOLEAN internal_applyFrontPropagation (ir_method_t *method);
static inline JITBOOLEAN internal_applyFrontPropagation2 (ir_method_t *method);
static inline JITINT32 internal_areInSameBasicBlock (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2);
static inline JITINT32 internal_canBasicBlocksBeMergedWithoutConsideringExceptionHandling (ir_method_t *method, IRBasicBlock *bb1, IRBasicBlock *bb2);

extern ir_optimizer_t  *irOptimizer;

JITBOOLEAN applyFrontPropagation (ir_method_t *method) {
    return internal_applyFrontPropagation(method) || internal_applyFrontPropagation2(method);
}

static inline JITBOOLEAN internal_applyFrontPropagation (ir_method_t *method) {
    ir_instruction_t        *instSucc;
    ir_instruction_t        *inst;
    XanListItem             *item;
    XanList                 *instToUpdate;
    XanList			*instFromWhichWeNeedToRenameVariable;
    JITUINT32 		instID;
    JITUINT32 		instructionsNumber;
    JITBOOLEAN		modified;

    /* Assertions			*/
    assert(method != NULL);
    assert(irOptimizer != NULL);

    /* Initialize the variables.
     */
    modified	= JITFALSE;

    /* Recompute the liveness	*
     * information			*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, ESCAPES_ANALYZER);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Search the instructions to update.
     */
    instToUpdate 				= xanList_new(allocFunction, freeFunction, NULL);
    instFromWhichWeNeedToRenameVariable	= xanList_new(allocFunction, freeFunction, NULL);
    for (instID = 0; instID < instructionsNumber; instID++) {
        JITBOOLEAN	toRename;
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (internal_updateFrontPropagation(method, inst, &toRename) == JITTRUE) {
            xanList_insert(instToUpdate, inst);
            if (toRename) {
                xanList_insert(instFromWhichWeNeedToRenameVariable, inst);
            }
            PDEBUG("Will update inst %u\n", inst->ID);
        }
    }

    /* Apply the front propagation	*/
    item = xanList_first(instToUpdate);
    while (item != NULL) {

        /* Take the instruction to	*
         * update			*/
        inst = (ir_instruction_t *) item->data;
        assert(inst != NULL);

        /* Take the next instruction	*/
        instSucc = IRMETHOD_getNextInstruction(method, inst);
        assert(instSucc != NULL);

        /* Check the instruction	*/
        assert(internal_areInSameBasicBlock(method, inst, instSucc));
        assert(IRMETHOD_getInstructionType(instSucc) == IRMOVE);
        assert(IRMETHOD_doesInstructionUseVariable(method, instSucc, IRMETHOD_getInstructionResultValue(inst)));
        assert(IRMETHOD_getInstructionParameter1Value(instSucc) == ir_instr_liveness_def_get(method, inst));
        assert(IRMETHOD_getInstructionResult(inst)->value.v == ir_instr_liveness_def_get(method, inst));
        assert(IRMETHOD_getInstructionResult(inst)->type == IROFFSET);
        assert(IRMETHOD_getSuccessorInstruction(method, inst, NULL) == instSucc);
        assert(IRMETHOD_getSuccessorInstruction(method, inst, instSucc) == NULL);
        assert(IRMETHOD_getPredecessorInstruction(method, instSucc, NULL) == inst);
        assert(IRMETHOD_getPredecessorInstruction(method, instSucc, inst) == NULL);
        assert(!IRMETHOD_isAnEscapedVariable(method, IRMETHOD_getInstructionParameter1Value(instSucc)));
        assert(!IRMETHOD_isAnEscapedVariable(method, IRMETHOD_getInstructionResultValue(instSucc)));

        /* Check if we need to rename the variable.
         */
        if (xanList_find(instFromWhichWeNeedToRenameVariable, inst)) {
            IRBasicBlock		*bbInst;
            ir_instruction_t	*tempInst;
            IR_ITEM_VALUE		varDefined;
            IR_ITEM_VALUE		varDefinedByMove;

            /* Substitute the variable that will be changed by the propagation.
             */
            varDefined		= IRMETHOD_getVariableDefinedByInstruction(method, inst);
            varDefinedByMove	= IRMETHOD_getVariableDefinedByInstruction(method, instSucc);
            bbInst 			= IRMETHOD_getBasicBlockContainingInstruction(method, inst);
            assert(bbInst != NULL);
            tempInst		= IRMETHOD_getNextInstructionWithinBasicBlock(method, bbInst, instSucc);
            while (tempInst != NULL) {
                IRMETHOD_substituteVariableInsideInstruction(method, tempInst, varDefined, varDefinedByMove);
                tempInst	= IRMETHOD_getNextInstructionWithinBasicBlock(method, bbInst, tempInst);
            }
        }

        /* Apply the propagation.
         */
        IRMETHOD_cpInstructionParameter(method, instSucc, 0, inst, 0);
        IRMETHOD_swapInstructionParameters(method, instSucc, 0, 1);
        modified	= JITTRUE;

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(instToUpdate);
    xanList_destroyList(instFromWhichWeNeedToRenameVariable);

    return modified;
}

static inline JITINT32 internal_updateFrontPropagation (ir_method_t *method, ir_instruction_t *inst, JITBOOLEAN *toRename) {
    ir_instruction_t        *instSucc;
    IR_ITEM_VALUE	 	unusefullVar;
    IR_ITEM_VALUE	 	definedVar;
    IRBasicBlock		*bbInst;
    ir_instruction_t	*lastBBInst;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize variables.
     */
    (*toRename)	= JITFALSE;

    /* Check if the instruction define some variable.
     */
    unusefullVar 	= IRMETHOD_getVariableDefinedByInstruction(method, inst);
    if (   	(unusefullVar == NOPARAM)       					||
            (inst->type == IRNOP)           					||
            (IRMETHOD_doesInstructionUseVariable(method, inst, unusefullVar))	) {
        return JITFALSE;
    }

    /* Fetch the basic block containing the instruction.
     */
    bbInst 		= IRMETHOD_getBasicBlockContainingInstruction(method, inst);
    assert(bbInst != NULL);
    lastBBInst	= IRMETHOD_getLastInstructionWithinBasicBlock(method, bbInst);
    assert(lastBBInst != NULL);

    /* Check the next instruction.
     */
    instSucc = IRMETHOD_getNextInstruction(method, inst);
    if (instSucc == NULL) {
        return JITFALSE;
    }
    if (!internal_areInSameBasicBlock(method, inst, instSucc)) {
        return JITFALSE;
    }
    if (IRMETHOD_getInstructionType(instSucc) != IRMOVE) {
        return JITFALSE;
    }
    if (IRMETHOD_getInstructionParameter1Value(instSucc) != unusefullVar) {
        return JITFALSE;
    }
    if (IRMETHOD_getInstructionParameter1Type(instSucc) != IROFFSET) {
        return JITFALSE;
    }
    if (IRMETHOD_getInstructionResult(inst)->value.v != unusefullVar) {
        return JITFALSE;
    }
    if (IRMETHOD_isAnEscapedVariable(method, IRMETHOD_getInstructionParameter1Value(instSucc))) {
        return JITFALSE;
    }
    if (IRMETHOD_isAnEscapedVariable(method, IRMETHOD_getInstructionResultValue(instSucc))) {
        return JITFALSE;
    }
    definedVar 	= IRMETHOD_getVariableDefinedByInstruction(method, instSucc);
    if (	(!IRMETHOD_doesInstructionUseVariable(method, inst, definedVar))	&&
            (IRMETHOD_isVariableLiveOUT(method, instSucc, unusefullVar))		) {
        ir_instruction_t	*tempInst;

        /* Check if renaming variables within the basic block is enough to allow the transformation.
         * If the variable unusefullVar is live at the end of the basic block, then we cannot do anything.
         */
        if (IRMETHOD_isVariableLiveOUT(method, lastBBInst, unusefullVar)) {
            return JITFALSE;
        }

        /* If the variable unusefullVar or the one defined by the IRMOVE instruction is redefined, then we cannot do anything.
         */
        assert(!IRMETHOD_isVariableLiveOUT(method, lastBBInst, unusefullVar));
        tempInst	= IRMETHOD_getNextInstructionWithinBasicBlock(method, bbInst, instSucc);
        while (tempInst != NULL) {
            IR_ITEM_VALUE	tempInstDefVar;
            tempInstDefVar	= IRMETHOD_getVariableDefinedByInstruction(method, tempInst);
            if (	(tempInstDefVar == definedVar)		||
                    (tempInstDefVar == unusefullVar)	) {
                return JITFALSE;
            }
            tempInst	= IRMETHOD_getNextInstructionWithinBasicBlock(method, bbInst, tempInst);
        }

        /* Renaming variables within the basic block is enough to allow the transformation.
         */
        (*toRename)	= JITTRUE;
    }

    /* Apply the propagation.
     */
    assert(instSucc != NULL);
    assert(internal_areInSameBasicBlock(method, inst, instSucc));
    assert(IRMETHOD_getInstructionType(instSucc) == IRMOVE);
    assert(IRMETHOD_getInstructionParameter1Value(instSucc) == ir_instr_liveness_def_get(method, inst));
    assert(IRMETHOD_getInstructionResult(inst)->value.v == ir_instr_liveness_def_get(method, inst));
    assert(IRMETHOD_getInstructionResult(inst)->type == IROFFSET);
    assert(IRMETHOD_getSuccessorInstruction(method, inst, NULL) == instSucc);
    assert(IRMETHOD_getSuccessorInstruction(method, inst, instSucc) == NULL);
    assert(IRMETHOD_doesInstructionUseVariable(method, instSucc, IRMETHOD_getInstructionResultValue(inst)));

    return JITTRUE;
}

static inline JITINT32 internal_areInSameBasicBlock (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    IRBasicBlock      *bb1;
    IRBasicBlock      *bb2;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst1 != NULL);
    assert(inst2 != NULL);

    /* Fetch the basic blocks	*/
    bb1 = IRMETHOD_getBasicBlockContainingInstruction(method, inst1);
    assert(bb1 != NULL);
    bb2 = IRMETHOD_getBasicBlockContainingInstruction(method, inst2);
    assert(bb2 != NULL);

    /* Check if the basic block is	*
     * the same			*/
    if (bb1 == bb2) {
        return JITTRUE;
    }

    /* Check if one is the          *
     * successor of the other	*/
    if (    	internal_canBasicBlocksBeMergedWithoutConsideringExceptionHandling(method, bb1, bb2)    ||
                internal_canBasicBlocksBeMergedWithoutConsideringExceptionHandling(method, bb2, bb1)    ) {
        return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

static inline JITINT32 internal_canBasicBlocksBeMergedWithoutConsideringExceptionHandling (ir_method_t *method, IRBasicBlock *bb1, IRBasicBlock *bb2) {
    ir_instruction_t        *endBB1;
    ir_instruction_t        *succEndBB1;

    /* Assertions			*/
    assert(method != NULL);
    assert(bb1 != NULL);
    assert(bb2 != NULL);

    endBB1 = IRMETHOD_getInstructionAtPosition(method, bb1->endInst);
    succEndBB1 = IRMETHOD_getSuccessorInstruction(method, endBB1, NULL);
    if (succEndBB1 != NULL) {
        if (IRMETHOD_getSuccessorInstruction(method, endBB1, succEndBB1) == NULL) {
            if (succEndBB1->ID == bb2->startInst) {
                return JITTRUE;
            }
        }
    }

    return JITFALSE;
}

static inline JITBOOLEAN internal_applyFrontPropagation2 (ir_method_t *method) {
    ir_instruction_t        *instSucc;
    ir_instruction_t        *inst;
    XanListItem             *item;
    XanList                 *instToUpdate;
    JITUINT32 		        instID;
    JITUINT32 		        instructionsNumber;
    JITBOOLEAN		        modified;

    /* Assertions.
     */
    assert(method != NULL);
    assert(irOptimizer != NULL);

    /* Initialize the variables.
     */
    modified	= JITFALSE;

    /* Recompute data flow information.
     */
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, ESCAPES_ANALYZER);

    /* Fetch the number of instructions.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Search the instructions to update.
     */
    instToUpdate 				= xanList_new(allocFunction, freeFunction, NULL);
    for (instID = 0; instID < instructionsNumber; instID++) {
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (internal_updateFrontPropagation2(method, inst) == JITTRUE) {
            xanList_insert(instToUpdate, inst);
        }
    }

    /* Apply the front propagation.
     */
    item = xanList_first(instToUpdate);
    while (item != NULL) {
        IR_ITEM_VALUE   usefullVar;
        IR_ITEM_VALUE   unusefullVar;

        /* Take the instruction to update.
         */
        inst            = (ir_instruction_t *) item->data;
        assert(inst != NULL);

        /* Take the next instruction.
         */
        instSucc        = IRMETHOD_getNextInstruction(method, inst);
        assert(instSucc != NULL);

        /* Fetch the variables.
         */
        unusefullVar 	= IRMETHOD_getVariableDefinedByInstruction(method, inst);
        usefullVar 	    = IRMETHOD_getVariableDefinedByInstruction(method, instSucc);
        assert(IRMETHOD_doesInstructionDefineVariable(method, inst, unusefullVar));
        assert(!IRMETHOD_doesInstructionUseVariable(method, inst, unusefullVar));
        assert(IRMETHOD_doesInstructionUseVariable(method, inst, usefullVar));
        assert(!IRMETHOD_doesInstructionDefineVariable(method, inst, usefullVar));
        assert(IRMETHOD_doesInstructionUseVariable(method, instSucc, unusefullVar));
        assert(!IRMETHOD_doesInstructionUseVariable(method, instSucc, usefullVar));

        /* Apply the propagation.
         */
        IRMETHOD_substituteVariableInsideInstruction(method, inst, unusefullVar, usefullVar);
        IRMETHOD_substituteVariableInsideInstruction(method, instSucc, unusefullVar, usefullVar);
        modified	    = JITTRUE;

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(instToUpdate);

    return modified;
}

static inline JITINT32 internal_updateFrontPropagation2 (ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t    *instSucc;
    IR_ITEM_VALUE	 	unusefullVar;
    IR_ITEM_VALUE	 	usefullVar;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);

    /* Check if the instruction define a variable that does not uses it.
     */
    unusefullVar 	= IRMETHOD_getVariableDefinedByInstruction(method, inst);
    if (   	(unusefullVar == NOPARAM)       					||
            (inst->type == IRNOP)           					||
            (IRMETHOD_doesInstructionUseVariable(method, inst, unusefullVar))	) {
        return JITFALSE;
    }

    /* Fetch the variable defined by the successive instruction.
     */
    instSucc = IRMETHOD_getNextInstruction(method, inst);
    usefullVar 	= IRMETHOD_getVariableDefinedByInstruction(method, instSucc);

    /* Check the next instruction.
     */
    if (instSucc == NULL) {
        return JITFALSE;
    }
    if (!internal_areInSameBasicBlock(method, inst, instSucc)) {
        return JITFALSE;
    }
    if (!IRMETHOD_isAMathInstruction(instSucc)){
        return JITFALSE;
    }
    if (!IRMETHOD_doesInstructionUseVariable(method, instSucc, unusefullVar)){
        return JITFALSE;
    }
    if (IRMETHOD_doesInstructionUseVariable(method, instSucc, usefullVar)){
        return JITFALSE;
    }
    if (!IRMETHOD_doesInstructionUseVariable(method, inst, usefullVar)){
        return JITFALSE;
    }
    if (!IRMETHOD_doesInstructionDefineVariable(method, inst, unusefullVar)){
        return JITFALSE;
    }
    if (IRMETHOD_isAnEscapedVariable(method, unusefullVar)){
        return JITFALSE;
    }
    if (IRMETHOD_isVariableLiveOUT(method, instSucc, unusefullVar)){
        return JITFALSE;
    }
    
    /* We can apply the propagation.
     */
    assert(instSucc != NULL);
    assert(internal_areInSameBasicBlock(method, inst, instSucc));

    return JITTRUE;
}
