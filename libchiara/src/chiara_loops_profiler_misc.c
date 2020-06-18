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
#include <ir_method.h>
#include <compiler_memory_manager.h>
#include <ir_optimization_interface.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
#include <chiara_loop_transformations.h>
// End

XanList * get_profiled_loops (XanList *loops, XanHashTable *loopsNames) {
    XanListItem *item;
    XanList *list;

    /* Assertions			*/
    assert(loops != NULL);

    /* Allocate the necessary 	*
     * memories			*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Allocate the profile	data	*
     * the loops			*/
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t 		*loop;
        loop_profile_t 	*profile;
        XanListItem	*item2;

        /* Fetch the loop			*/
        loop = item->data;
        assert(loop != NULL);
        assert(loop->header != NULL);
        assert(loop->header->type == IRLABEL);

        /* Allocate the profile data		*/
        profile = allocFunction(sizeof(loop_profile_t));

        /* Fill up the profile structure	*/
        profile->loop = loop;
        profile->headerID = loop->header->ID;
        profile->headerLabel = IRMETHOD_getInstructionParameter1Value(loop->header);
        profile->times = xanList_new(allocFunction, freeFunction, NULL);
        profile->iterationsTimes = xanList_new(allocFunction, freeFunction, NULL);
        if (loopsNames != NULL) {
            profile->loopName	= (JITINT8 *)xanHashTable_lookup(loopsNames, loop);
        }

        /* Get the first child of the current	*
         * loop	from the list we are building	*/
        item2	= xanList_first(list);
        while (item2 != NULL) {
            loop_profile_t	*oldP;
            oldP	= item2->data;
            if (	(oldP->loop->method == profile->loop->method)		&&
                    (LOOPS_isASubLoop(loops, profile->loop,  oldP->loop))	) {
                break;
            }
            item2	= item2->next;
        }

        /* Add the profile data to the list	*/
        if (item2 == NULL) {
            xanList_append(list, profile);
        } else {
            xanList_insertBefore(list, item2, profile);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }
    assert(xanList_length(list) == xanList_length(loops));

    /* Return			*/
    return list;
}

void profileLoop (ir_optimizer_t *irOptimizer, loop_profile_t *profile, void *startLoopFunction, ir_item_t *startLoopFunctionReturnItem, XanList *startLoopFunctionParameters, void *endLoopFunction, ir_item_t *endLoopFunctionReturnItem, XanList *endLoopFunctionParameters, void *newIterationFunction, ir_item_t *newIterationFunctionReturnItem, XanList *newIterationFunctionParameters, XanList *loopSuccessors, XanList *loopPredecessors, void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst)) {
    XanList *successors;
    XanList *predecessors;
    XanListItem *item;
    ir_instruction_t *firstInst;

    /* Assertions.
     */
    assert(irOptimizer != NULL);
    assert(profile != NULL);
    assert(profile->loop != NULL);
    assert(profile->loop->method != NULL);

    /* Fetch the successors of the loop.
     */
    if (loopSuccessors == NULL) {
        successors = LOOPS_getSuccessorsOfLoop(profile->loop);
    } else {
        successors = loopSuccessors;
    }
    assert(successors != NULL);

    /* Fetch the predecessors of the header.
     */
    if (loopPredecessors == NULL) {
        predecessors = IRMETHOD_getInstructionPredecessors(profile->loop->method, profile->loop->header);
    } else {
        predecessors = loopPredecessors;
    }
    assert(predecessors != NULL);
    assert(xanList_length(predecessors) > 1);

    /* Add the pre-header.
     */
    firstInst = LOOPS_addPreHeader(profile->loop, predecessors);
    assert(firstInst != NULL);
    assert(IRMETHOD_getInstructionType(firstInst) == IRLABEL);

    /* Inject code to notify the start of a loop.
     */
    if (startLoopFunction != NULL) {
        IRMETHOD_newNativeCallInstructionAfter(profile->loop->method, firstInst, "start_loop", startLoopFunction, startLoopFunctionReturnItem, startLoopFunctionParameters);
    }

    /* Notify the end of single loop*
     * iterations			*/
    assert(IRMETHOD_getInstructionType(profile->loop->header) == IRLABEL);
    if (newIterationFunction != NULL) {
        ir_instruction_t	*newLoopIterationCall;
        newLoopIterationCall	= IRMETHOD_newNativeCallInstructionAfter(profile->loop->method, profile->loop->header, "new_loop_iteration", newIterationFunction, newIterationFunctionReturnItem, newIterationFunctionParameters);
        if (addCodeFunction != NULL) {
            (*addCodeFunction)(profile->loop, newLoopIterationCall);
        }
    }
    item = xanList_first(successors);
    while (item != NULL) {
        ir_instruction_t *succ;
        ir_instruction_t *end_call;

        /* Fetch the successor of the loop	*/
        succ = item->data;
        assert(succ != NULL);

        /* Notify the end of the loop iteration	*/
        if (IRMETHOD_getPredecessorsNumber(profile->loop->method, succ) > 1) {
            ir_instruction_t *newLabel;
            ir_instruction_t *pred;
            assert(IRMETHOD_getInstructionType(succ) == IRLABEL);
            newLabel = IRMETHOD_newLabel(profile->loop->method);
            IRMETHOD_newBranchToLabelAfter(profile->loop->method, succ, newLabel);

            /* Update the predecessors within the loop	*/
            pred = IRMETHOD_getPredecessorInstruction(profile->loop->method, succ, NULL);
            while (pred != NULL) {
                if (xanList_find(profile->loop->instructions, pred) != NULL) {
                    IRMETHOD_substituteLabel(profile->loop->method, pred, (succ->param_1).value.v, (newLabel->param_1).value.v);
                }
                pred = IRMETHOD_getPredecessorInstruction(profile->loop->method, succ, pred);
            }
            succ = newLabel;
        }
        if (IRMETHOD_getInstructionType(succ) == IRLABEL) {
            end_call = IRMETHOD_newInstructionOfTypeAfter(profile->loop->method, succ, IRNOP);
        } else {
            end_call = IRMETHOD_newInstructionOfTypeBefore(profile->loop->method, succ, IRNOP);
        }
        assert(end_call != NULL);

        /* Notify the end of the loop		*/
        if (endLoopFunction != NULL) {
            IRMETHOD_newNativeCallInstructionAfter(profile->loop->method, end_call, "end_loop", endLoopFunction, endLoopFunctionReturnItem, endLoopFunctionParameters);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(successors);
    xanList_destroyList(predecessors);

    return ;
}

void profileLoopByUsingSymbols (ir_optimizer_t *irOptimizer, loop_profile_t *profile, ir_symbol_t *startLoopFunction, ir_item_t *startLoopFunctionReturnItem, XanList *startLoopFunctionParameters, ir_symbol_t *endLoopFunction, ir_item_t *endLoopFunctionReturnItem, XanList *endLoopFunctionParameters, ir_symbol_t *newIterationFunction, ir_item_t *newIterationFunctionReturnItem, XanList *newIterationFunctionParameters, XanList *successors, XanList *predecessors, XanList *addedInstructions) {
    XanListItem 		*item;
    ir_instruction_t 	*firstInst;

    /* Assertions.
     */
    assert(irOptimizer != NULL);
    assert(profile != NULL);
    assert(profile->loop != NULL);
    assert(profile->loop->method != NULL);
    assert(successors != NULL);
    assert(predecessors != NULL);
    assert(xanList_length(predecessors) > 1);

    /* Add the pre-header.
     */
    firstInst = LOOPS_addPreHeader(profile->loop, predecessors);
    assert(firstInst != NULL);
    assert(IRMETHOD_getInstructionType(firstInst) == IRLABEL);

    /* Start a new list of profiled	times.
     */
    if (startLoopFunction != NULL) {
        ir_instruction_t	*nativeCall;
        nativeCall	= IRMETHOD_newNativeCallInstructionByUsingSymbolsAfter(profile->loop->method, firstInst, NULL, startLoopFunction, startLoopFunctionReturnItem, startLoopFunctionParameters);
        assert(nativeCall != NULL);
        if (addedInstructions != NULL) {
            xanList_append(addedInstructions, nativeCall);
        }
    }

    /* Notify the end of single loop iterations.
     */
    assert(IRMETHOD_getInstructionType(profile->loop->header) == IRLABEL);
    if (newIterationFunction != NULL) {
        ir_instruction_t	*nativeCall;
        nativeCall	= IRMETHOD_newNativeCallInstructionByUsingSymbolsAfter(profile->loop->method, profile->loop->header, NULL, newIterationFunction, newIterationFunctionReturnItem, newIterationFunctionParameters);
        assert(nativeCall != NULL);
        if (addedInstructions != NULL) {
            xanList_append(addedInstructions, nativeCall);
        }
    }
    item = xanList_first(successors);
    while (item != NULL) {
        ir_instruction_t *succ;
        ir_instruction_t *end_call;

        /* Fetch the successor of the loop.
         */
        succ = item->data;
        assert(succ != NULL);

        /* Notify the end of the loop iteration.
         */
        if (IRMETHOD_getPredecessorsNumber(profile->loop->method, succ) > 1) {
            ir_instruction_t *newLabel;
            ir_instruction_t *pred;
            assert(IRMETHOD_getInstructionType(succ) == IRLABEL);
            newLabel = IRMETHOD_newLabel(profile->loop->method);
            IRMETHOD_newBranchToLabelAfter(profile->loop->method, succ, newLabel);

            /* Update the predecessors within the loop.
             */
            pred = IRMETHOD_getPredecessorInstruction(profile->loop->method, succ, NULL);
            while (pred != NULL) {
                if (xanList_find(profile->loop->instructions, pred) != NULL) {
                    IRMETHOD_substituteLabel(profile->loop->method, pred, (succ->param_1).value.v, (newLabel->param_1).value.v);
                }
                pred = IRMETHOD_getPredecessorInstruction(profile->loop->method, succ, pred);
            }
            succ = newLabel;
        }
        if (IRMETHOD_getInstructionType(succ) == IRLABEL) {
            end_call = IRMETHOD_newInstructionOfTypeAfter(profile->loop->method, succ, IRNOP);
        } else {
            end_call = IRMETHOD_newInstructionOfTypeBefore(profile->loop->method, succ, IRNOP);
        }
        assert(end_call != NULL);

        /* Notify the end of the loop		*/
        if (endLoopFunction != NULL) {
            ir_instruction_t	*nativeCall;
            nativeCall	= IRMETHOD_newNativeCallInstructionByUsingSymbolsAfter(profile->loop->method, end_call, NULL, endLoopFunction, endLoopFunctionReturnItem, endLoopFunctionParameters);
            assert(nativeCall != NULL);
            if (addedInstructions != NULL) {
                xanList_append(addedInstructions, nativeCall);
            }
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(successors);
    xanList_destroyList(predecessors);

    return ;
}

XanList * instrument_loops (	ir_optimizer_t *irOptimizer,
                                XanList *loops,
                                JITBOOLEAN noSystemLibraries,
                                XanHashTable *loopsNames,
                                void (*newLoop)(void *loopName),
                                void (*endLoop)(void),
                                void (*newIteration)(void),
                                void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst),
                                IR_ITEM_VALUE (*fetchNewLoopParameter)(loop_profile_t *p)
                           ) {
    XanList			*p;
    XanListItem		*item;
    XanHashTable 		*loopsSuccessors;
    XanHashTable 		*loopsPredecessors;
    void 			*newLoopfunctionPointer;
    void 			*endLoopFunctionPointer;
    void			*newLoopIterationFunctionPointer;

    /* Assertions.
     */
    assert(irOptimizer != NULL);
    assert(loops != NULL);
    assert(loopsNames != NULL);
    assert(fetchNewLoopParameter != NULL);

    /* Allocate the profile data structures.
     */
    p			= get_profiled_loops(loops, loopsNames);
    assert(p != NULL);

    /* Compute the successors of the loops to profile.
     */
    loopsSuccessors 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Set the function pointers	*/
    newLoopfunctionPointer 		= newLoop;
    endLoopFunctionPointer 		= endLoop;
    newLoopIterationFunctionPointer	= newIteration;

    /* Profile the loops.
     */
    item 				= xanList_first(p);
    while (item != NULL) {
        loop_profile_t			*profile;
        XanList 			*start_new_outermost_loop_parameters;
        ir_item_t 			currentItem;

        /* Fetch the loop to profile.
         */
        profile			= item->data;
        assert(profile != NULL);
        assert(profile->loopName != NULL);

        /* Check if we should consider the current loop.
         */
        if (	(noSystemLibraries == JITFALSE)					||
                (!IRPROGRAM_doesMethodBelongToALibrary(profile->loop->method))	) {
            XanList 	*successors;
            XanList 	*predecessors;

            /* Fetch the successors and predecessors of the loop.
             */
            successors 				= xanHashTable_lookup(loopsSuccessors, profile->loop);
            assert(successors != NULL);
            predecessors 				= xanHashTable_lookup(loopsPredecessors, profile->loop);
            assert(predecessors != NULL);

            /* Create the parameters.
             */
            start_new_outermost_loop_parameters 	= xanList_new(allocFunction, freeFunction, NULL);
            memset(&currentItem, 0, sizeof(ir_item_t));
            currentItem.value.v 			= (*fetchNewLoopParameter)(profile);
            currentItem.type 			= IRNUINT;
            currentItem.internal_type 		= IRNUINT;
            xanList_append(start_new_outermost_loop_parameters, &currentItem);

            /* Profile the loop.
             */
            profileLoop	 (	irOptimizer,
                            profile,
                            newLoopfunctionPointer, NULL, start_new_outermost_loop_parameters,
                            endLoopFunctionPointer, NULL, NULL,
                            newLoopIterationFunctionPointer, NULL, NULL,
                            successors, predecessors, addCodeFunction);

            /* Free the memory.
             */
            xanList_destroyList(start_new_outermost_loop_parameters);
        }

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    /* Free the memory.
     */
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);

    return p;
}
