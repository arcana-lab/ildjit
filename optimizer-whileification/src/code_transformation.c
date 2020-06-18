/*
 * Copyright (C) 2012  Campanoni Simone
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
#include <platform_API.h>
#include <chiara.h>

// My headers
#include <optimizer_whileification.h>
#include <config.h>
// End

extern ir_optimizer_t	*irOptimizer;

static inline XanList * internal_fetchDoWhileLoops (XanHashTable *loops);
static inline JITBOOLEAN internal_whileify (XanHashTable *loops, loop_t *loop);
static inline JITBOOLEAN internal_is_method_valid (ir_method_t *method);
static inline void internal_identify_prologue_and_body (loop_t *loop, XanList *prologue, XanList *body);

JITBOOLEAN transform_loops_to_while (XanHashTable *loops) {
    XanList			*loopsToTransform;
    XanListItem		*item;
    JITBOOLEAN		modified;

    /* Initialize the variables.
     */
    modified		= JITFALSE;

    /* Fetch the list of loops to transform.
     */
    loopsToTransform	= internal_fetchDoWhileLoops(loops);

    /* Transform the loops.
     */
    item			= xanList_first(loopsToTransform);
    while (item != NULL) {
        loop_t	*loop;
        loop		= item->data;
        modified	|= internal_whileify(loops, loop);
        item		= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(loopsToTransform);

    return modified;
}

static inline XanList * internal_fetchDoWhileLoops (XanHashTable *loops) {
    XanList			*methods;
    XanList			*loopsList;
    XanList			*loopsToTransform;
    XanListItem		*item;

    /* Allocate the necessary memory.
     */
    loopsToTransform	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the list of loops.
     */
    loopsList		= xanHashTable_toList(loops);
    assert(loopsList != NULL);

    /* Fetch the set of methods that include loops.
     */
    methods			= LOOPS_getMethods(loopsList);

    /* Analyze just the loops that belong to not library method.
     */
    item			= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*method;

        /* Fetch the method.
         */
        method		= item->data;
        assert(method != NULL);

        /* Check the method.
         */
        if (!IRPROGRAM_doesMethodBelongToALibrary(method)) {
            XanList		*outermostLoops;
            XanListItem	*item2;

            /* Fetch the set of outermost loops of the current method.
             */
            outermostLoops	= IRLOOP_getOutermostLoopsWithinMethod(method, loops);
            assert(outermostLoops != NULL);

            /* Consider just the do-while loops.
             */
            item2		= xanList_first(outermostLoops);
            while (item2 != NULL) {
                loop_t	*loop;

                /* Fetch the loop.
                 */
                loop	= item2->data;
                assert(loop != NULL);

                /* Check if it is a while loop.
                 */
                if (!LOOPS_isAWhileLoop(irOptimizer, loop)) {
                    xanList_append(loopsToTransform, loop);
                }

                /* Fetch the next element.
                 */
                item2	= item2->next;
            }

            /* Free the memory.
             */
            xanList_destroyList(outermostLoops);
        }

        /* Fetch the next element.
         */
        item		= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(loopsList);
    xanList_destroyList(methods);

    return loopsToTransform;
}

static inline JITBOOLEAN internal_whileify (XanHashTable *loops, loop_t *loop) {
    XanListItem		*item;
    XanList			*prologue;
    XanList			*body;
    XanList			*originalLoop;
    XanList			*newLoopInstructions;
    ir_instruction_t	*whileHeader;
    ir_instruction_t	*whileBackedge;
    ir_instruction_t	*lastInst;
    ir_instruction_t	*firstPrologueInst;
    ir_instruction_t	**labelMapping;
    JITUINT32		labelMappingElements;

    /* Assertions.
     */
    assert(loops != NULL);
    assert(loop != NULL);
    assert(internal_is_method_valid(loop->method));

    /* Check the loop.
     */
    if (LOOPS_isAWhileLoop(irOptimizer, loop)) {
        return JITFALSE;
    }

    /* Allocate the necessary memory.
     */
    prologue		= xanList_new(allocFunction, freeFunction, NULL);
    body			= xanList_new(allocFunction, freeFunction, NULL);

    /* Compute the post-dominance relation.
     */
    DOMINANCE_computePostdominance(irOptimizer, loop->method);

    /* Identify the prologue and the body of the loop.
     */
    internal_identify_prologue_and_body(loop, prologue, body);

    /* Check if there is any body.
     */
    if (xanList_length(body) == 0) {

        /* Free the memory.
         */
        xanList_destroyList(prologue);
        xanList_destroyList(body);

        return JITFALSE;
    }
    fprintf(stderr, "DOWHILE %s\n", IRMETHOD_getSignatureInString(loop->method));

    /* Create the boundary of the new while loop that will contain iterations of the do-while loop starting from the second one.
     */
    newLoopInstructions	= xanList_new(allocFunction, freeFunction, NULL);
    whileHeader		= IRMETHOD_newLabel(loop->method);
    whileBackedge		= IRMETHOD_newBranchToLabelAfter(loop->method, whileHeader, whileHeader);
    xanList_append(newLoopInstructions, whileHeader);
    xanList_append(newLoopInstructions, whileBackedge);

    /* Allocate the mapping from the original label, to the new one.
     */
    labelMappingElements	= IRMETHOD_getMaxLabelID(loop->method) + 1;
    labelMapping		= allocFunction(sizeof(ir_instruction_t) * labelMappingElements);

    /* Create the list of sorted instructions of the original loop.
     */
    LOOPS_sortInstructions(loop->method, prologue, NULL);
    LOOPS_sortInstructions(loop->method, body, NULL);
    assert((xanList_length(prologue) + xanList_length(body)) == xanList_length(loop->instructions));
    originalLoop		= xanList_cloneList(prologue);
    xanList_appendList(originalLoop, body);
    assert(xanList_length(originalLoop) == xanList_length(loop->instructions));

    /* Add the prologue and the body to the while loop.
     */
    lastInst		= whileHeader;
    item			= xanList_first(originalLoop);
    firstPrologueInst	= item->data;
    while (item != NULL) {
        ir_instruction_t	*inst;
        ir_instruction_t	*newInst;
        ir_instruction_t	*nextInst;

        /* Fetch the original instruction.
         */
        inst		= item->data;
        assert(inst != NULL);

        /* Clone the instruction.
         */
        newInst		= IRMETHOD_cloneInstruction(loop->method, inst);
        IRMETHOD_moveInstructionAfter(loop->method, newInst, lastInst);
        lastInst	= newInst;
        xanList_append(newLoopInstructions, lastInst);

        /* Check if the current instruction is the last body instruction.
         */
        nextInst	= IRMETHOD_getNextInstruction(loop->method, inst);
        if (nextInst == firstPrologueInst) {
            lastInst	= IRMETHOD_newBranchToLabelAfter(loop->method, whileHeader, newInst);
            xanList_append(newLoopInstructions, lastInst);
        }

        /* Adjust the label.
         */
        for (JITUINT32 count=1; count <= IRMETHOD_getInstructionParametersNumber(newInst); count++) {
            ir_item_t		*param;
            ir_instruction_t	*destination;
            ir_instruction_t	*outsideLabel;

            /* Check whether the parameter is a label or not.
             */
            param		= IRMETHOD_getInstructionParameter(newInst, count);
            if (param->type != IRLABELITEM) {
                continue;
            }
            assert(param->internal_type == IRLABELITEM);

            /* Check if the instruction is a label.
             */
            if (newInst->type == IRLABEL) {
                ir_instruction_t	*clonedLabel;
                clonedLabel	= labelMapping[(inst->param_1).value.v];
                if (clonedLabel == NULL) {
                    (param->value).v			= IRMETHOD_newLabelID(loop->method);
                    labelMapping[(inst->param_1).value.v]	= newInst;
                    continue;
                }
                IRMETHOD_moveInstructionBefore(loop->method, clonedLabel, newInst);
                IRMETHOD_deleteInstruction(loop->method, newInst);
                xanList_removeElement(newLoopInstructions, newInst, JITTRUE);
                lastInst	= clonedLabel;
                break ;
            }
            assert(newInst->type != IRLABEL);
            assert(IRMETHOD_isABranchInstruction(newInst));

            /* The instruction is a branch.
             * Fetch the destination of the branch in the original loop.
             */
            destination	= IRMETHOD_getBranchDestination(loop->method, inst);
            assert(destination != NULL);
            assert(destination->type == IRLABEL);

            /* Check whether the destination of the branch is outside the original loop.
             * In case the destination is outside the loop, we don't need to change anything to the cloned instruction.
             */
            if (xanList_find(loop->instructions, destination) == NULL) {

                /* the destination is outside the loop.
                 */
                continue;
            }

            /* The destination of the branch is within the loop.
             * Check if the current original instruction is the last one before the prologue.
             */
            outsideLabel	= IRMETHOD_getNextInstruction(loop->method, inst);
            if (outsideLabel != firstPrologueInst) {

                /* The current original instruction is not in the boundary between the body and the prologue.
                 * Check if the fall through instruction does not belong to the loop.
                 * If this is the case, we need to add the branch after the current one to jump outside the loop in case the branch will not be taken.
                 */
                if (xanList_find(loop->instructions, outsideLabel) == NULL) {
                    if (outsideLabel->type != IRLABEL) {
                        outsideLabel	= IRMETHOD_newLabelBefore(loop->method, outsideLabel);
                    }
                    assert(outsideLabel->type == IRLABEL);
                    lastInst	= IRMETHOD_newBranchToLabelAfter(loop->method, outsideLabel, newInst);
                }
            }

            /* We need to adjust the cloned branch instruction to point to the cloned destination of the branch.
             * Check if the cloned destination of the branch has been already created.
             */
            if (labelMapping[(destination->param_1).value.v] != NULL) {
                ir_instruction_t	*clonedLabel;

                /* The cloned destination of the branch has been already created.
                 * Adjust the cloned branch to point to the cloned label.
                 */
                clonedLabel		= labelMapping[(destination->param_1).value.v];
                (param->value).v	= IRMETHOD_getInstructionParameter1Value(clonedLabel);
                assert(IRMETHOD_getBranchDestination(loop->method, newInst) == clonedLabel);
                continue ;
            }

            /* The branch destination is not created in the while loop yet.
             * We need to create it and later when we will find the position, we will schedule this label in the proper place.
             */
            labelMapping[(destination->param_1).value.v]	= IRMETHOD_newLabel(loop->method);
            xanList_append(newLoopInstructions, labelMapping[(destination->param_1).value.v]);

            /* Adjust the current cloned branch to point to the cloned label.
             */
            (param->value).v				= IRMETHOD_getInstructionParameter1Value(labelMapping[(destination->param_1).value.v]);
            assert(IRMETHOD_getBranchDestination(loop->method, newInst) == labelMapping[(destination->param_1).value.v]);
        }

        /* Fetch the next instruction.
         */
        item		= item->next;
    }
    assert(internal_is_method_valid(loop->method));

    /* Adjust the branches to the original labels mapped to the new ones.
     */
    for (JITUINT32 count=0; count < labelMappingElements; count++) {
        IR_ITEM_VALUE		newLabel;
        ir_instruction_t	*labelInst;

        /* Fetch the next label to substitute.
         */
        if (labelMapping[count] == NULL) {
            continue;
        }
        newLabel	= IRMETHOD_getInstructionParameter1Value(labelMapping[count]);
        assert(labelMapping[count]->type == IRLABEL);
        assert(xanList_find(loop->instructions, labelMapping[count]) == NULL);

        /* Check if the label belongs to the original prologue.
         */
        labelInst	= IRMETHOD_getLabelInstruction(loop->method, count);
        assert(labelInst != NULL);
        if (xanList_find(prologue, labelInst) == NULL) {
            continue;
        }

        /* Substitute the label.
         */
        item		= xanList_first(body);
        while (item != NULL) {
            ir_instruction_t	*inst;

            /* Fetch the instruction.
             */
            inst	= item->data;
            assert(inst != NULL);
            assert(IRMETHOD_doesInstructionBelongToMethod(loop->method, inst));

            /* Check if the current instruction uses the old label.
             */
            if (	(IRMETHOD_isABranchInstruction(inst))				&&
                    (IRMETHOD_doesInstructionUseLabel(loop->method, inst, count))	) {

                /* The current instruction refers to the old label.
                 * Redirect it to the new label.
                 */
                IRMETHOD_substituteLabel(loop->method, inst, count, newLabel);
                assert(IRMETHOD_getBranchDestination(loop->method, inst) == labelMapping[count]);
            }

            /* Fetch the next instruction to consider.
             */
            item	= item->next;
        }
    }
    assert(internal_is_method_valid(loop->method));
#ifdef DEBUG
    for (JITUINT32 count=0; count < IRMETHOD_getInstructionsNumber(loop->method); count++) {
        ir_instruction_t	*inst;
        ir_instruction_t	*dest;
        inst	= IRMETHOD_getInstructionAtPosition(loop->method, count);
        if (!IRMETHOD_isABranchInstruction(inst)) {
            continue ;
        }
        if (xanList_find(prologue, inst) != NULL) {
            continue;
        }
        dest	= IRMETHOD_getBranchDestination(loop->method, inst);
        assert(dest != NULL);
        assert(xanList_find(prologue, dest) == NULL);
    }
#endif

    /* Delete the prologue of the original loops.
     */
    if (firstPrologueInst->type == IRLABEL) {
        IRMETHOD_newBranchToLabelAfter(loop->method, whileHeader, firstPrologueInst);
    } else {
        IRMETHOD_newBranchToLabelBefore(loop->method, whileHeader, firstPrologueInst);
    }
    assert(internal_is_method_valid(loop->method));
    item			= xanList_first(prologue);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst		= item->data;
        IRMETHOD_deleteInstruction(loop->method, inst);
        item		= item->next;
    }
    assert(internal_is_method_valid(loop->method));

    /* Check the new loop.
     */
#ifdef DEBUG
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, PRE_DOMINATOR_COMPUTER);
    item	= xanList_first(newLoopInstructions);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (inst->type != IRBRANCH) {
            assert(IRMETHOD_isInstructionAPredominator(loop->method, whileHeader, inst));
        }
        item	= item->next;
    }
#endif

    /* Free the memory.
     */
    freeFunction(labelMapping);
    xanList_destroyList(prologue);
    xanList_destroyList(body);
    xanList_destroyList(originalLoop);
    xanList_destroyList(newLoopInstructions);

    return JITTRUE;
}

static inline JITBOOLEAN internal_is_method_valid (ir_method_t *method) {
    if (method == NULL) {
        return JITFALSE;
    }
    for (JITUINT32 count=0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        ir_instruction_t	*inst;
        inst	= IRMETHOD_getInstructionAtPosition(method, count);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));
        if (IRMETHOD_isABranchInstruction(inst)) {
            assert(IRMETHOD_getBranchDestination(method, inst) != NULL);
        }
    }
    return JITTRUE;
}

static inline void internal_identify_prologue_and_body (loop_t *loop, XanList *prologue, XanList *body) {
    XanStack		*workingList;
    XanList			*exits;
    XanList			*toRemove;
    XanList			*toKeep;
    XanList			*alreadyConsidered;
    XanList			*backedges;
    XanListItem		*item;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(prologue != NULL);
    assert(body != NULL);

    /* Allocate the necessary memory.
     */
    toRemove		= xanList_new(allocFunction, freeFunction, NULL);
    alreadyConsidered	= xanList_new(allocFunction, freeFunction, NULL);
    toKeep			= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the backedges of the loop.
     */
    backedges		= LOOPS_getBackedges(loop);

    /* Fetch the exit points of the loop.
     */
    exits			= LOOPS_getExitInstructions(loop);
    item			= xanList_first(exits);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (IRMETHOD_isACallInstruction(inst)) {
            xanList_append(toRemove, inst);
        }
        item	= item->next;
    }
    item		= xanList_first(toRemove);
    while (item != NULL) {
        xanList_removeElement(exits, item->data, JITTRUE);
        item	= item->next;
    }
    xanList_emptyOutList(toRemove);

    /* Remove exit points that do not belong to the while clause.
     * Only exit points that predominates all exit points that are also backedges belong to the while clause.
     */
    item			= xanList_first(exits);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (xanList_find(backedges, inst) != NULL) {
            xanList_append(toKeep, inst);
        }
        item	= item->next;
    }
    item			= xanList_first(exits);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (xanList_find(toKeep, inst) == NULL) {
            XanListItem	*item2;
            item2	= xanList_first(toKeep);
            while (item2 != NULL) {
                ir_instruction_t	*inst2;
                inst2	= item2->data;
                if (!IRMETHOD_isInstructionAPredominator(loop->method, inst, inst2)) {
                    xanList_append(toRemove, inst);
                    break ;
                }
                item2	= item2->next;
            }
        }
        item	= item->next;
    }
    item		= xanList_first(toRemove);
    while (item != NULL) {
        assert(xanList_find(toKeep, item->data) == NULL);
        xanList_removeElement(exits, item->data, JITTRUE);
        item	= item->next;
    }

    /* Add the exit points to the stack of instructions to check to identify the prologue.
     */
    workingList	= xanStack_new(allocFunction, freeFunction, NULL);
    item		= xanList_first(exits);
    while (item != NULL) {
        xanStack_push(workingList, item->data);
        item	= item->next;
    }

    /* Identify the prologue.
     */
    while (xanStack_getSize(workingList) > 0) {
        ir_instruction_t	*inst;
        ir_instruction_t	*succ;

        /* Fetch the instruction to analyze.
         */
        inst		= xanStack_pop(workingList);
        assert(inst != NULL);
        xanList_append(alreadyConsidered, inst);

        /* Add the current instruction to belong to the prologue since it can be reached by an exit instruction.
         */
        assert(xanList_find(prologue, inst) == NULL);
        xanList_append(prologue, inst);

        /* Look at the successor of the instruction.
         */
        succ	= IRMETHOD_getSuccessorInstruction(loop->method, inst, NULL);
        while (succ != NULL) {
            if (	(succ != loop->header)					&&
                    (xanList_find(loop->instructions, succ) != NULL)	&&
                    (xanList_find(alreadyConsidered, succ) == NULL)		&&
                    (!xanStack_contains(workingList, succ))			) {
                xanStack_push(workingList, succ);
            }
            succ	= IRMETHOD_getSuccessorInstruction(loop->method, inst, succ);
        }
    }

    /* Identify the body.
     */
    item		= xanList_first(loop->instructions);
    while (item != NULL) {
        ir_instruction_t	*inst;

        /* Fetch the instruction to analyze.
         */
        inst		= item->data;
        assert(inst != NULL);

        /* If the instruction does not belong to the prologue, it belongs to the body.
         */
        if (xanList_find(prologue, inst) == NULL) {
            xanList_append(body, inst);
        }

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }
    assert((xanList_length(prologue) + xanList_length(body)) == xanList_length(loop->instructions));

    /* Free the memory.
     */
    xanStack_destroyStack(workingList);
    xanList_destroyList(toRemove);
    xanList_destroyList(toKeep);
    xanList_destroyList(exits);
    xanList_destroyList(backedges);

    return ;
}
