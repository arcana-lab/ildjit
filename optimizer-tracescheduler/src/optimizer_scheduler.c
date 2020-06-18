/*
 * Copyright (C) 2006 - 2009 Campanoni Simone
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
#include <ir_language.h>
#include <jitsystem.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_scheduler.h>
#include <config.h>
// End

#define INFORMATIONS    "Schedule the IR instruction to minimize the static branches"
#define AUTHOR          "Simone Campanoni"

static inline JITUINT64 sched_get_ID_job (void);
static inline void sched_do_job (ir_method_t *method);
static inline char * sched_get_version (void);
static inline char * sched_get_informations (void);
static inline char * sched_get_author (void);
static inline JITUINT64 sched_get_dependences (void);
static inline JITUINT64 sched_get_invalidations (void);
static inline void sched_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void sched_shutdown (JITFLOAT32 totalTime);
static inline JITNINT remove_branch (ir_method_t *method);
static inline JITBOOLEAN internal_is_a_branch_needed (ir_method_t *method, ir_instruction_t *endBasicBlock);
static inline JITBOOLEAN internal_is_label_used_in_catch_blocks (ir_method_t *method, IR_ITEM_VALUE labelID);
static inline void sched_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void sched_getCompilationFlags (char *buffer, JITINT32 bufferLength);

/**
 * Move a basic block
 *
 * Move the basic block starting from `label` just after the instruction `branch`
 */
static inline void internal_moveBasicBlock (ir_method_t *method, ir_instruction_t *branch, ir_instruction_t *label);

ir_optimization_interface_t plugin_interface = {
    sched_get_ID_job,
    sched_get_dependences,
    sched_init,
    sched_shutdown,
    sched_do_job,
    sched_get_version,
    sched_get_informations,
    sched_get_author,
    sched_get_invalidations,
    NULL,
    sched_getCompilationFlags,
    sched_getCompilationTime
};

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;

static inline void sched_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);

    irLib = lib;
    irOptimizer = optimizer;
}

static inline void sched_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
}

static inline JITUINT64 sched_get_ID_job (void) {
    return INSTRUCTIONS_SCHEDULER;
}

static inline JITUINT64 sched_get_dependences (void) {
    return BASIC_BLOCK_IDENTIFIER;
}

static inline JITUINT64 sched_get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

static inline void sched_do_job (ir_method_t *method) {

    /* Assertions				*/
    assert(method != NULL);
    PDEBUG("OPTIMIZER_SCHEDULER: Start\n");

    /* Schedule the instructions		*/
    remove_branch(method);

    /* Return				*/
    PDEBUG("OPTIMIZER_SCHEDULER: Exit\n");
    return;
}

static inline JITNINT branch_prediction_reschedule (ir_method_t *method) {
    ir_instruction_t        *inst;
    ir_instruction_t        *instSucc;
    ir_instruction_t        *instSucc2;
    JITUINT32 count;
    JITUINT32 branchInstID;
    JITUINT32 labelInstID;
    JITUINT32 instSuccID;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Search instructions to reschedule	*/
    for (count = 0; count < instructionsNumber; count++) {

        /* Fetch the instruction                        */
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check if we are in the last block of the     *
         * code						*/
        if (IRMETHOD_getInstructionType(inst) == IRSTARTCATCHER) {
            return 0;
        }

        /* Check if the current instruction is a branch	*/
        if (IRMETHOD_getInstructionType(inst) == IRBRANCH) {
            instSucc = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            assert(instSucc != NULL);
            instSuccID = IRMETHOD_getInstructionPosition(instSucc);
            if (    (count < instSuccID)            &&
                    (count + 1 != instSuccID)       ) {

                /* Move the basic block starting from instSucc  *
                 * just after the current branch instruction	*/
                internal_moveBasicBlock(method, inst, instSucc);
            }
        } else if (IRMETHOD_mayHaveNotFallThroughInstructionAsSuccessor(method, inst)) {
            branchInstID = count;
            instSucc = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            assert(instSucc != NULL);
            instSucc2 = IRMETHOD_getSuccessorInstruction(method, inst, instSucc);
            if (instSucc2 != NULL) {
                instSuccID = IRMETHOD_getInstructionPosition(instSucc);
                if (labelInstID > (branchInstID + 1)) {

                    /* Move the basic block starting from instSucc  *
                     * just after the current branch instruction	*/
//                                        internal_moveBasicBlock(method, inst, instSucc);
                }
            }
        }
    }

    return 0;
}

static inline JITNINT remove_branch (ir_method_t *method) {
    ir_instruction_t        *inst;
    ir_instruction_t        *instNext;
    ir_instruction_t        *instSucc;
    JITUINT32 		count;
    XanList			*toDelete;
    XanListItem		*item;

    /* Assertions				*/
    assert(method != NULL);

    /* Allocate the memory.
     */
    toDelete	= xanList_new(allocFunction, freeFunction, NULL);

    /* Remove jumps to empty basic blocks	*/
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        JITUINT32 instType;
        ir_instruction_t        *labelInst;
        ir_instruction_t        *nextLabelInst;
        ir_instruction_t        *nextInst;
        IR_ITEM_VALUE nextLabelID;

        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Fetch the instruction type			*/
        instType = IRMETHOD_getInstructionType(inst);

        /* Check if the current instruction is a branch	*/
        if (    (instType != IRBRANCH)          &&
                (instType != IRBRANCHIF)        &&
                (instType != IRBRANCHIFNOT)     ) {
            continue;
        }

        /* Get the target basic block			*/
        labelInst = IRMETHOD_getBranchDestination(method, inst);
        assert(labelInst != NULL);
        assert(labelInst->type == IRLABEL);

        /* Check if the basic block is empty		*/
        nextInst = IRMETHOD_getNextInstruction(method, labelInst);
        assert(nextInst != NULL);
        if (nextInst->type != IRBRANCH) {
            continue;
        }
        nextLabelInst = IRMETHOD_getBranchDestination(method, nextInst);
        assert(nextLabelInst != NULL);
        assert(nextLabelInst->type == IRLABEL);
        if (nextLabelInst == labelInst) {
            continue;
        }

        /* The branch targets an empty basic block.	*
         * Shortcut the jump				*/
        nextLabelID = IRMETHOD_getInstructionParameter1Value(nextLabelInst);
        IRMETHOD_setBranchDestination(method, inst, nextLabelID);
    }

    /* Compute the method information		*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);

    /* Remove unnecessary conditional jumps 	*/
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        JITUINT32 instType;
        ir_instruction_t        *labelInst;
        ir_instruction_t        *nextLabelInst;
        ir_instruction_t        *nextInst;

        /* Fetch the instruction			*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        instType = IRMETHOD_getInstructionType(inst);

        /* Check if the current instruction is a branch	*/
        if (    (instType != IRBRANCHIF)        &&
                (instType != IRBRANCHIFNOT)     ) {
            continue;
        }

        /* Get the target basic block			*/
        labelInst = IRMETHOD_getBranchDestination(method, inst);
        assert(labelInst != NULL);
        assert(labelInst->type == IRLABEL);

        /* Check if the next instruction is an unconditional branch	*/
        nextInst = IRMETHOD_getNextInstruction(method, inst);
        assert(nextInst != NULL);
        if (nextInst->type != IRBRANCH) {
            continue;
        }
        assert(nextInst->type == IRBRANCH);

        /* Check if both branches point to the same label.
         */
        nextLabelInst = IRMETHOD_getBranchDestination(method, nextInst);
        assert(nextLabelInst != NULL);
        assert(nextLabelInst->type == IRLABEL);
        if (nextLabelInst != labelInst) {
            continue;
        }

        /* The conditional branch is useless because if it is not taken, the subsequent unconditional branch will jump to the same target.
         */
        xanList_append(toDelete, inst);
    }
    item	= xanList_first(toDelete);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        assert(inst != NULL);
        IRMETHOD_deleteInstruction(method, inst);
        item	= item->next;
    }

    /* Compute the method information		*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);

    /* Search instructions to reschedule	*/
    inst = IRMETHOD_getFirstInstruction(method);
    while (inst != NULL) {
        JITUINT32 instType;

        /* Fetch the next instruction                   */
        instNext = IRMETHOD_getNextInstruction(method, inst);

        /* Fetch the instruction type			*/
        instType = IRMETHOD_getInstructionType(inst);

        /* Check if we are in the last block of the     *
         * code						*/
        if (instType == IRSTARTCATCHER) {
            break ;
        }

        /* Check if the current instruction is a branch	*/
        if (instType == IRBRANCH) {

            /* Try to move the successor block. */
            instSucc = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            assert(instSucc != NULL);
            assert(IRMETHOD_getInstructionType(instSucc) == IRLABEL);
            assert(IRMETHOD_getInstructionPosition(inst) != IRMETHOD_getInstructionPosition(instSucc));

            /* Check the predecessors                       */
            if (    (instSucc->ID != 0)                                             &&
                    (IRMETHOD_getPredecessorsNumber(method, instSucc) == 1)         ) {
                assert(IRMETHOD_getPredecessorInstruction(method, instSucc, NULL) == inst);
                assert(IRMETHOD_getSuccessorInstruction(method, inst, NULL) == instSucc);
                assert(IRMETHOD_getSuccessorsNumber(method, inst) == 1);

                /* Move the basic block starting from instSucc  *
                 * just after the current branch instruction	*/
                internal_moveBasicBlock(method, inst, instSucc);

                /* Compute the method information		*/
                IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);
            }
        }
        inst = instNext;
    }

    /* Free the memory.
     */
    xanList_destroyList(toDelete);

    return 0;
}

static inline char * sched_get_version (void) {
    return VERSION;
}

static inline char * sched_get_informations (void) {
    return INFORMATIONS;
}

static inline char * sched_get_author (void) {
    return AUTHOR;
}

static inline void internal_moveBasicBlock (ir_method_t *method, ir_instruction_t *branch, ir_instruction_t *label) {
    ir_instruction_t        *instToMove;
    ir_instruction_t        *lastInstMoved;
    ir_instruction_t        *endBasicBlock;
    ir_instruction_t        *lastInstructionOfPrevBasicBlock;
    ir_instruction_t        *newInst;
    IRBasicBlock              *basicBlockToMove;
    JITNUINT labelID;
    XanList                 *instructionsToMove;
    XanListItem             *item;

    /* Assertions				*/
    assert(method != NULL);
    assert(branch != NULL);
    assert(label != NULL);
    assert(IRMETHOD_getInstructionType(label) == IRLABEL);
    assert(IRMETHOD_getInstructionType(branch) == IRBRANCH);
    assert(IRMETHOD_getSuccessorsNumber(method, branch) == 1);
    assert(IRMETHOD_getPredecessorsNumber(method, label) == 1);
    assert(IRMETHOD_getSuccessorInstruction(method, branch, NULL) == label);
    PDEBUG("OPTIMIZER_SCHEDULER: moveBasicBlock: Start\n");
    PDEBUG("OPTIMIZER_SCHEDULER: moveBasicBlock:	Method %s\n", IRMETHOD_getSignatureInString(method));

    /* Initialize the variables		*/
    instToMove = NULL;
    lastInstMoved = NULL;
    endBasicBlock = NULL;
    lastInstructionOfPrevBasicBlock = NULL;
    newInst = NULL;

    /* Fetch the the basic block to move	*/
    basicBlockToMove = IRMETHOD_getBasicBlockContainingInstruction(method, label);
    assert(basicBlockToMove != NULL);
    endBasicBlock = IRMETHOD_getInstructionAtPosition(method, basicBlockToMove->endInst);
    assert(endBasicBlock != NULL);
    assert(endBasicBlock->type != IREXITNODE);

    /* Check the last instruction of the    *
     * basic block to move			*/
    if (endBasicBlock->type == IRCALLFINALLY) {

        /* Currently we do not support this case*
         * The problem is to redirect the       *
         * execution to the successor of the    *
         * call finally instruction of the end	*
         * finally instruction that eventually  *
         * will be executed			*/

        /* Return				*/
        return;
    }

    /* Check if the label is used in some	*
     * catch blocks				*/
    if (internal_is_label_used_in_catch_blocks(method, (label->param_1).value.v)) {

        /* Return				*/
        return;
    }
    PDEBUG("OPTIMIZER_SCHEDULER: moveBasicBlock:	Move the basic block starting from %d (L%lld) just after %d\n", label->ID, (label->param_1).value, branch->ID);

    /* Check if the execution can go        *
    * through the next instruction of the  *
    * end of the basic block to move       */
    if (internal_is_a_branch_needed(method, endBasicBlock)) {
        ir_instruction_t        *firstInstructionOfNextBasicBlock;

        /* Add the branch instruction at the end*
         * of the basic block to move.		*
         * This branch has to jump to the next	*
         * basic block of the one to move	*/
        PDEBUG("OPTIMIZER_SCHEDULER: moveBasicBlock:		A branch at the end of the moving basic block is needed\n");
        firstInstructionOfNextBasicBlock = IRMETHOD_getNextInstruction(method, endBasicBlock);
        assert(firstInstructionOfNextBasicBlock != NULL);
#ifdef DEBUG
        ir_instruction_t        *tempInst;
        tempInst = IRMETHOD_getSuccessorInstruction(method, endBasicBlock, NULL);
        if (tempInst != firstInstructionOfNextBasicBlock) {
            assert(IRMETHOD_getSuccessorInstruction(method, endBasicBlock, tempInst) == firstInstructionOfNextBasicBlock);
        }
#endif
        if (IRMETHOD_getInstructionType(firstInstructionOfNextBasicBlock) != IRLABEL) {
            newInst = IRMETHOD_newInstructionBefore(method, firstInstructionOfNextBasicBlock);
            labelID = IRMETHOD_newLabelID(method);
            assert(newInst != NULL);
            IRMETHOD_setInstructionType(method, newInst, IRLABEL);
            IRMETHOD_setInstructionParameter1(method, newInst, labelID, 0, IRLABELITEM, IRLABELITEM, NULL);
            firstInstructionOfNextBasicBlock = newInst;
        }
        assert(firstInstructionOfNextBasicBlock != NULL);
        assert(IRMETHOD_getInstructionType(firstInstructionOfNextBasicBlock) == IRLABEL);
        assert(IRMETHOD_getInstructionParameter1Type(firstInstructionOfNextBasicBlock) == IRLABELITEM);

        /* Fetch the label to jump		*/
        labelID = IRMETHOD_getInstructionParameter1Value(firstInstructionOfNextBasicBlock);

        /* Add the branch			*/
        newInst = IRMETHOD_newInstructionAfter(method, endBasicBlock);
        assert(newInst != NULL);
        IRMETHOD_setInstructionType(method, newInst, IRBRANCH);
        IRMETHOD_setInstructionParameter1(method, newInst, labelID, 0, IRLABELITEM, IRLABELITEM, NULL);
        endBasicBlock = newInst;
    }

    /* Add the branch instruction at the    *
     * end of the basic block which is      *
     * before the one to move.		*
     * This branch has to jump to the first	*
     * instruction of the basic block to	*
     * move.				*/
    assert(label != NULL);
    lastInstructionOfPrevBasicBlock = IRMETHOD_getPrevInstruction(method, label);
    if (lastInstructionOfPrevBasicBlock != NULL) {
        assert(IRMETHOD_getInstructionPosition(lastInstructionOfPrevBasicBlock) == IRMETHOD_getInstructionPosition(label) - 1);
        if (IRMETHOD_canTheNextInstructionBeASuccessor(method, lastInstructionOfPrevBasicBlock)) {
            labelID = IRMETHOD_getInstructionParameter1Value(label);
            newInst = IRMETHOD_newInstructionBefore(method, label);
            assert(newInst != NULL);
            IRMETHOD_setInstructionType(method, newInst, IRBRANCH);
            IRMETHOD_setInstructionParameter1(method, newInst, labelID, 0, IRLABELITEM, IRLABELITEM, NULL);
        }
    }

    /* Make the list of instructions to     *
    * move                                 */
    instructionsToMove = xanList_new(allocFunction, freeFunction, NULL);
    instToMove = label;
    while (instToMove != endBasicBlock) {
#ifdef DEBUG
        assert(instToMove != NULL);
        assert(xanList_find(instructionsToMove, instToMove) == NULL);
        if (instToMove->ID < (endBasicBlock->ID - 1)) {
            assert(IRMETHOD_getSuccessorsNumber(method, instToMove) == 1);
        }
#endif
        xanList_append(instructionsToMove, instToMove);
        instToMove = IRMETHOD_getNextInstruction(method, instToMove);
    }
    assert(xanList_find(instructionsToMove, endBasicBlock) == NULL);
    xanList_append(instructionsToMove, endBasicBlock);

    /* Move the basic block starting	*
     * from "label" till "endBasicBlock"	*/
    PDEBUG("OPTIMIZER_SCHEDULER:            Basic block ready to be moved: from %d to %d\n", IRMETHOD_getInstructionPosition(label), IRMETHOD_getInstructionPosition(endBasicBlock));
    lastInstMoved = branch;
    item = xanList_first(instructionsToMove);
    assert(item != NULL);
    while (item != NULL) {
        instToMove = (ir_instruction_t *) item->data;
        assert(instToMove != NULL);
        IRMETHOD_moveInstructionAfter(method, instToMove, lastInstMoved);
        lastInstMoved = instToMove;
        item = item->next;
    }

    /* Delete the not more useful           *
    * instructions                         */
    assert(IRMETHOD_getInstructionAtPosition(method, branch->ID) == branch);
    IRMETHOD_deleteInstruction(method, branch);

    /* Free the memory                      */
    xanList_destroyList(instructionsToMove);

    /* Return				*/
    PDEBUG("OPTIMIZER_SCHEDULER: moveBasicBlock: Exit\n");
    return;
}

static inline JITBOOLEAN internal_is_a_branch_needed (ir_method_t *method, ir_instruction_t *endBasicBlock) {
    ir_instruction_t        *next;

    /* Assertions				*/
    assert(method != NULL);
    assert(endBasicBlock != NULL);

    if (!IRMETHOD_canTheNextInstructionBeASuccessor(method, endBasicBlock)) {
        return JITFALSE;
    }
    next = IRMETHOD_getNextInstruction(method, endBasicBlock);
    if (    (endBasicBlock->type == IRBRANCH)                               &&
            (next->type == IRLABEL)                                         &&
            ((endBasicBlock->param_1).value.v == (next->param_1).value.v)   ) {
        return JITFALSE;
    }
    return JITTRUE;
}

static inline JITBOOLEAN internal_is_label_used_in_catch_blocks (ir_method_t *method, IR_ITEM_VALUE labelID) {
    ir_instruction_t        *inst;
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Search instructions to reschedule	*/
    for (count = 0; count < instructionsNumber; count++) {

        /* Fetch the instruction                        */
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check the instruction			*/
        if (inst->type != IRBRANCHIFPCNOTINRANGE) {
            continue;
        }

        /* Check the labels used			*/
        if (    ((inst->param_1).value.v == labelID)    ||
                ((inst->param_2).value.v == labelID)    ||
                ((inst->param_3).value.v == labelID)    ) {

            /* Return			*/
            return JITTRUE;
        }
    }

    /* Return			*/
    return JITFALSE;
}

static inline void sched_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void sched_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
