/*
 * Copyright (C) 2012 - 2013 Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ir_language.h>
#include <metadata_manager.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <base_symbol.h>

// My headers
#include <ir_method.h>
#include <codetool_types.h>
#include <ir_data.h>
// End

static inline void internal_append_loops_without_duplications (loop_t *loop, XanList *l, XanList *loopsToAdd);
static inline XanList * internal_remove_false_loops (XanList *list);
static inline JITINT32 loops_equal_functions (void *key1, void *key2);
static inline JITUINT32 loops_hash_function (void *element);
static inline XanHashTable * internal_create_the_loops_table (ir_method_t *mainMethod, XanList *escapedMethods, XanList *reachableMethods, void (*analyzeCircuits)(ir_method_t *m));
static inline XanHashTable * internal_create_empty_loops_table (void);
static inline void internal_add_loops_of_method (ir_method_t *method, XanHashTable *loops, XanList *escapedMethods, XanList *reachableMethods, void (*analyzeCircuits)(ir_method_t *m));
static inline void _IRLOOP_computeLoopInformation (loop_t *loop, JITBOOLEAN updateVariablesInformation, XanList *escapedMethods, XanList *reachableMethods);
static inline void internal_analyze_method (ir_method_t *method, XanHashTable *loops, XanHashTable *methodsAlreadyConsidered, XanHashTable *outermostLoops, XanList *escapedMethods, XanList *reachableMethods);
static inline JITBOOLEAN internal_check_loops (XanHashTable *loops);
static inline JITBOOLEAN internal_check_loop (loop_t *loop);

loop_t * IRLOOP_getLoopFromInstruction (ir_method_t *method, ir_instruction_t *inst, XanHashTable *loops) {
    loop_t key;
    loop_t                  *loop;
    circuit_t                  *ir_loop;
    ir_instruction_t        *header;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(loops != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Fetch the IR loop		*/
    ir_loop = IRMETHOD_getTheMoreNestedCircuitOfInstruction(method, inst);
    if (ir_loop == NULL) {
        return NULL;
    }

    /* Fetch the header		*/
    header = IRMETHOD_getInstructionAtPosition(method, ir_loop->header_id);
    assert(header != NULL);

    /* Make the key			*/
    memset(&key, 0, sizeof(loop_t));
    key.method = method;
    key.header = header;

    /* Lookup the loop		*/
    loop = xanHashTable_lookup(loops, &key);
#ifdef DEBUG
    if (loop != NULL) {
        assert(key.method == loop->method);
        assert(key.header == loop->header);
    }
#endif

    /* Return			*/
    return loop;
}

void IRLOOP_refreshLoopsInformation (XanHashTable *loops) {
    XanHashTableItem 	*item;
    XanList			*escapedMethods;
    XanList			*reachableMethods;
    ir_method_t             *mainMethod;

    /* Assertions.
     */
    assert(loops != NULL);

    /* Fetch escaped methods.
     */
    escapedMethods 		= IRPROGRAM_getEscapedMethods();
    assert(escapedMethods != NULL);

    /* Fetch the entry point.
     */
    mainMethod          = IRPROGRAM_getEntryPointMethod();
    assert(mainMethod != NULL);

    /* Fetch the reachable methods.
     */
    reachableMethods    = IRPROGRAM_getReachableMethods(mainMethod);
    assert(reachableMethods != NULL);

    /* Refresh information about loops.
     */
    item = xanHashTable_first(loops);
    while (item != NULL) {
        loop_t *l;
        l = item->element;
        assert(l != NULL);
        IRLOOP_computeLoopInformation(l, JITTRUE, escapedMethods, reachableMethods);
        item = xanHashTable_next(loops, item);
    }

    /* Free the memory.
     */
    xanList_destroyList(escapedMethods);
    xanList_destroyList(reachableMethods);

    return ;
}

loop_t * IRLOOP_getLoop (ir_method_t *method, circuit_t *loop, XanHashTable *loops) {
    loop_t                  *loopDescription;
    ir_instruction_t        *header;

    /* Assertions			*/
    assert(method != NULL);
    assert(loop != NULL);
    assert(loops != NULL);

    /* Fetch the header		*/
    header = IRMETHOD_getInstructionAtPosition(method, loop->header_id);
    assert(header != NULL);

    /* Fetch the loop		*/
    loopDescription = IRLOOP_getLoopFromInstruction(method, header, loops);
#ifdef DEBUG
    if (loopDescription != NULL) {
        assert(loopDescription->header != NULL);
        assert(loopDescription->header->ID == loop->header_id);
    }
#endif

    return loopDescription;
}

void IRLOOP_destroyLoops (XanHashTable *loops) {
    XanHashTableItem *item;

    /* Assertions			*/
    assert(loops != NULL);

    /* Free the memory		*/
    item = xanHashTable_first(loops);
    while (item != NULL) {
        loop_t *loop;

        /* Fetch the loop		*/
        loop = item->element;
        assert(loop != NULL);

        /* Destroy the loop		*/
        IRLOOP_destroyLoop(loop);

        /* Fetch the next element to 	*
         * free				*/
        item = xanHashTable_next(loops, item);
    }
    xanHashTable_destroyTable(loops);

    /* Return			*/
    return;
}

void IRLOOP_destroyLoop (loop_t *loop) {

    /* Assertions			*/
    assert(loop != NULL);

    /* Destroy the loop		*/
    if (loop->subLoops != NULL) {
        xanList_destroyList(loop->subLoops);
    }
    if (loop->instructions != NULL) {
        xanList_destroyList(loop->instructions);
    }
    if (loop->exits != NULL) {
        xanList_destroyList(loop->exits);
    }
    if (loop->invariants != NULL) {
        xanList_destroyList(loop->invariants);
    }
    if (loop->inductionVariables != NULL) {
        xanList_destroyListAndData(loop->inductionVariables);
    }
    if (loop->backedges != NULL) {
        xanList_destroyList(loop->backedges);
    }
    freeFunction(loop);

    return;
}

void IRLOOP_computeLoopInformation (loop_t *loop, JITBOOLEAN updateVariablesInformation, XanList *escapedMethods, XanList *reachableMethods) {
    JITBOOLEAN	tofreeEscapedMethods;
    JITBOOLEAN	tofreeReachableMethods;

    /* Assertions.
     */
    assert(loop != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(loop->method, loop->header));

    /* Initialize the variables.
     */
    tofreeEscapedMethods	= JITFALSE;
    tofreeReachableMethods	= JITFALSE;

    /* Fetch escaped methods.
     */
    if (escapedMethods == NULL) {
        escapedMethods 		= IRPROGRAM_getEscapedMethods();
        tofreeEscapedMethods	= JITTRUE;
    }
    assert(escapedMethods != NULL);

    /* Fetch the reachable methods.
     */
    if (reachableMethods == NULL) {
        ir_method_t	*mainMethod;

        /* Fetch the entry point.
         */
        mainMethod 		= IRPROGRAM_getEntryPointMethod();
        assert(mainMethod != NULL);

        /* Fetch the reachable methods.
         */
        reachableMethods 	= IRPROGRAM_getReachableMethods(mainMethod);
        tofreeReachableMethods	= JITTRUE;
    }
    assert(reachableMethods != NULL);

    /* Analyze the loop.
     */
    _IRLOOP_computeLoopInformation(loop, updateVariablesInformation, escapedMethods, reachableMethods);

    /* Free the memory.
     */
    if (tofreeEscapedMethods) {
        xanList_destroyList(escapedMethods);
    }
    if (tofreeReachableMethods) {
        xanList_destroyList(reachableMethods);
    }

    return;
}

XanHashTable * IRLOOP_analyzeMethodLoops (ir_method_t *method, void (*analyzeCircuits)(ir_method_t *m)) {
    XanHashTable	*loops;
    XanList		*escapedMethods;
    XanList		*reachableMethods;
    ir_method_t	*mainMethod;

    /* Assertions.
     */
    assert(method != NULL);

    /* Fetch escaped methods.
     */
    escapedMethods = IRPROGRAM_getEscapedMethods();
    assert(escapedMethods != NULL);

    /* Fetch the entry point.
     */
    mainMethod = IRPROGRAM_getEntryPointMethod();
    assert(mainMethod != NULL);

    /* Fetch the reachable methods.
     */
    reachableMethods = IRPROGRAM_getReachableMethods(mainMethod);
    assert(reachableMethods != NULL);

    /* Allocate the table of loops.
     */
    loops = internal_create_empty_loops_table();
    assert(loops != NULL);

    /* Analyze the loops of the method.
     */
    internal_add_loops_of_method(method, loops, escapedMethods, reachableMethods, analyzeCircuits);

    /* Free the memory.
     */
    xanList_destroyList(escapedMethods);

    return loops;
}

XanList * IRLOOP_getOutermostLoopsWithinMethod (ir_method_t *method, XanHashTable *loops) {
    XanList         *l;
    XanList         *outLoops;
    XanListItem     *item;

    /* Assertions			*/
    assert(method != NULL);
    assert(loops != NULL);

    /* Allocate the list		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the list of outermost	*
     * loops			*/
    outLoops = IRMETHOD_getOutermostCircuits(method);
    assert(outLoops != NULL);

    /* Fill up the list		*/
    item = xanList_first(outLoops);
    while (item != NULL) {
        circuit_t  *loop;
        loop_t  *loopDes;

        /* Fetch the loop		*/
        loop = (circuit_t *) item->data;
        assert(loop != NULL);
        loopDes = IRLOOP_getLoop(method, loop, loops);
        assert(loopDes != NULL);
        assert(loopDes->instructions != NULL);
        assert(xanList_length(loopDes->instructions) > 0);

        /* Append the loop to the list	*/
        if (xanList_find(l, loopDes) == NULL) {
            xanList_insert(l, loopDes);
        }

        /* Fetch the next element from	*
         * the list			*/
        item = item->next;
    }

    /* Check that there is no 	*
     * shared instructions across	*
     * outermost loops		*/
#ifdef DEBUG
    item = xanList_first(l);
    while (item != NULL) {
        XanListItem *item2;
        item2 = item->next;
        while (item2 != NULL) {
            loop_t *l1;
            loop_t *l2;
            l1 = item->data;
            l2 = item2->data;
            assert(l1 != l2);
            assert(!xanList_shareSomeElements(l1->instructions, l2->instructions));
            item2 = item2->next;
        }
        item = item->next;
    }
#endif

    /* Free the memory		*/
    xanList_destroyList(outLoops);

    /* Return			*/
    return l;
}

XanHashTable * IRLOOP_analyzeLoops (void (*analyzeCircuits)(ir_method_t *m)) {
    XanList 		*escapedMethods;
    XanList			*reachableMethods;
    XanHashTable            *methodsConsidered;
    XanHashTable            *loops;
    XanHashTable            *outermostLoops;
    XanHashTableItem        *item;
    ir_method_t             *mainMethod;

    /* Allocate the required memory.
     */
    methodsConsidered = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the entry point.
     */
    mainMethod = IRPROGRAM_getEntryPointMethod();
    assert(mainMethod != NULL);

    /* Fetch the reachable methods.
     */
    reachableMethods = IRPROGRAM_getReachableMethods(mainMethod);
    assert(reachableMethods != NULL);

    /* Fetch escaped methods.
     */
    escapedMethods = IRPROGRAM_getEscapedMethods();
    assert(escapedMethods != NULL);

    /* Fetch the loops		*/
    loops = internal_create_the_loops_table(mainMethod, escapedMethods, reachableMethods, analyzeCircuits);
    assert(loops != NULL);

    /* Allocate the table of 	*
     * outermost loops per method	*/
    outermostLoops = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(outermostLoops != NULL);

    /* Start the analysis		*/
    internal_analyze_method(mainMethod, loops, methodsConsidered, outermostLoops, escapedMethods, reachableMethods);

    /* Check the loops.
     */
    assert(internal_check_loops(loops));

    /* Free the memory.
     */
    xanHashTable_destroyTable(methodsConsidered);
    item = xanHashTable_first(outermostLoops);
    while (item != NULL) {
        XanList *list;
        list = (XanList *) item->element;
        assert(list != NULL);
        xanList_destroyList(list);
        item = xanHashTable_next(outermostLoops, item);
    }
    xanHashTable_destroyTable(outermostLoops);
    xanList_destroyList(escapedMethods);
    xanList_destroyList(reachableMethods);
    assert(internal_check_loops(loops));

    return loops;
}

static inline XanHashTable * internal_create_the_loops_table (ir_method_t *mainMethod, XanList *escapedMethods, XanList *reachableMethods, void (*analyzeCircuits)(ir_method_t *m)) {
    XanList                 *l;
    XanListItem             *item;
    XanHashTable            *loops;

    /* Allocate the table of loops	*/
    loops = internal_create_empty_loops_table();
    assert(loops != NULL);
    assert(internal_check_loops(loops));

    /* Fetch the list of all IR methods reacable from the main method.
     */
    l	= IRPROGRAM_getReachableMethods(mainMethod);
    assert(l != NULL);

    /* Add loops that belong to these methods.
     */
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t     *method;

        /* Fetch the main method	*/
        method = (ir_method_t *) item->data;
        assert(method != NULL);
        assert(xanList_equalsInstancesNumber(l, method) == 1);

        /* Add the loops of the current	*
         * method			*/
        internal_add_loops_of_method(method, loops, escapedMethods, reachableMethods, analyzeCircuits);

        /* Fetch the next element of the*
         * list				*/
        item = item->next;
    }
    assert(internal_check_loops(loops));

    /* Free the memory		*/
    xanList_destroyList(l);

    return loops;
}

static inline XanHashTable * internal_create_empty_loops_table (void) {
    XanHashTable    *loops;

    /* Allocate the table of loops	*/
    loops = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, loops_hash_function, loops_equal_functions);
    assert(loops != NULL);

    /* Return			*/
    return loops;
}

static inline void internal_add_loops_of_method (ir_method_t *method, XanHashTable *loops, XanList *escapedMethods, XanList *reachableMethods, void (*analyzeCircuits)(ir_method_t *m)) {
    XanList                 *loopList;
    XanListItem             *item2;

    /* Assertions.
     */
    assert(method != NULL);
    assert(escapedMethods != NULL);
    assert(reachableMethods != NULL);
    assert(internal_check_loops(loops));

    /* Check if the methods has     *
     * loops			*/
    if (analyzeCircuits != NULL) {
        (*analyzeCircuits)(method);
    }
    loopList = internal_remove_false_loops(method->loop);
    if (    (loopList == NULL)                      ||
            (xanList_length(loopList) == 0)       ) {

        /* Free the memory		*/
        if (loopList != NULL) {
            xanList_destroyList(loopList);
        }

        return;
    }
    assert(loopList != NULL);
    assert(xanList_length(loopList) > 0);

    /* Free the memory		*/
    xanList_destroyList(loopList);

    /* Fetch the set of loops	*/
    loopList = internal_remove_false_loops(method->loop);
    assert(loopList != NULL);
    assert(xanList_length(loopList) > 0);

    /* Make the loops		*/
    item2 = xanList_first(loopList);
    assert(item2 != NULL);
    while (item2 != NULL) {
        circuit_t          *loop;
        loop_t          *loop_par;
        loop_t          *old_loop;

        /* Fetch the loop			*/
        loop = (circuit_t *) item2->data;
        assert(loop != NULL);
        assert(loop->loop_id != 0);

        /* Print the loop			*/
#ifdef PRINTDEBUG
        if (method->loopNestTree != NULL) {
            XanNode         *node;
            XanNode         *child;
            node = method->loopNestTree->find(method->loopNestTree, loop);
            assert(node != NULL);
            child = node->getNextChildren(node, NULL);
            while (child != NULL) {
                circuit_t  *subLoop;
                subLoop = child->data;
                assert(subLoop != NULL);
                child = node->getNextChildren(node, child);
            }
        }
#endif

        /* Allocate a new loop descriptor	*/
        loop_par = allocFunction(sizeof(loop_t));
        assert(loop_par != NULL);

        /* Fill up the loop descriptor		*/
        loop_par->header 	= IRMETHOD_getInstructionAtPosition(method, loop->header_id);
        loop_par->method 	= method;
        loop_par->instructions 	= IRMETHOD_getCircuitInstructions(method, loop);
        loop_par->subLoops 	= xanList_new(allocFunction, freeFunction, NULL);
        if (loop->loopExits != NULL) {
            loop_par->exits = xanList_cloneList(loop->loopExits);
        } else {
            loop_par->exits = xanList_new(allocFunction, freeFunction, NULL);
        }
        assert(loop_par->header == (loop->backEdge).dst);
        loop_par->backedges	= xanList_new(allocFunction, freeFunction, NULL);
        xanList_append(loop_par->backedges, (loop->backEdge).src);

        /* Check the loop			*/
        assert(loop_par->exits != NULL);
        assert(internal_check_loop(loop_par));

        /* Check if there is another loop with the same header.
         * In this case we normalize them in one single loop.
         */
        old_loop = xanHashTable_lookup(loops, loop_par);
        if (old_loop != NULL) {

            /* Merge the new loop with the old one	*/
            _IRLOOP_computeLoopInformation(old_loop, JITTRUE, escapedMethods, reachableMethods);
            xanList_appendList(old_loop->backedges, loop_par->backedges);
            xanList_deleteClones(old_loop->backedges);

            /* Free the memory			*/
            IRLOOP_destroyLoop(loop_par);

        } else {

            /* Compute the information of the loop	*/
            _IRLOOP_computeLoopInformation(loop_par, JITTRUE, escapedMethods, reachableMethods);

            /* Insert the new loop to the table	*/
            xanHashTable_insert(loops, loop_par, loop_par);
            assert(xanHashTable_lookup(loops, loop_par) != NULL);
            assert(IRLOOP_getLoop(method, loop, loops) != NULL);
        }

        /* Check the loop			*/
#ifdef DEBUG
        loop_par = IRLOOP_getLoop(method, loop, loops);
        assert(loop_par != NULL);
        assert(internal_check_loop(loop_par));
#endif

        /* Fetch the next element from the list	*/
        item2 = item2->next;
    }
    assert(internal_check_loops(loops));

    /* Make the hierarchy of the loops of the	*
     * method					*/
    if (method->loopNestTree != NULL) {
        item2 = xanList_first(loopList);
        while (item2 != NULL) {
            circuit_t 	 	*loop;
            XanNode         *node;
            XanNode         *parent;

            /* Fetch the loop			*/
            loop = (circuit_t *) item2->data;
            assert(loop != NULL);
            assert(loop->loop_id != 0);

            /* Fetch the node of the tree		*/
            node = method->loopNestTree->find(method->loopNestTree, loop);
            assert(node != NULL);

            /* Fetch the parent of the node		*/
            parent = node->getParent(node);
            if (parent != NULL) {
                circuit_t          *parentLoop;
                loop_t          *loopDescription;
                loop_t          *parentLoopDescription;

                /* Add the current loop as a sub-loop	*
                 * of the current parent one		*/
                assert(parent != node);
                parentLoop = (circuit_t *) parent->getData(parent);
                assert(parentLoop != NULL);
                assert(parentLoop != loop);
                if (parentLoop->loop_id != 0) {
                    loopDescription = IRLOOP_getLoop(method, loop, loops);
                    assert(loopDescription != NULL);
                    parentLoopDescription = IRLOOP_getLoop(method, parentLoop, loops);
                    assert(parentLoopDescription != NULL);
                    assert(loopDescription != parentLoopDescription);
                    assert(internal_check_loops(loops));
                    if (xanList_find(parentLoopDescription->subLoops, loopDescription) == NULL) {
                        xanList_append(parentLoopDescription->subLoops, loopDescription);
                    }
                    assert(internal_check_loops(loops));
                }
            }

            /* Fetch the next element from the list	*/
            item2 = item2->next;
        }
    }
    assert(internal_check_loops(loops));

    /* Free the memory.
     */
    xanList_destroyList(loopList);

    return;
}

static inline void _IRLOOP_computeLoopInformation (loop_t *loop, JITBOOLEAN updateVariablesInformation, XanList *escapedMethods, XanList *reachableMethods) {
    XanListItem     *item;

    /* Assertions.
     */
    assert(escapedMethods != NULL);
    assert(loop != NULL);
    assert(loop->header != NULL);
    assert(loop->method != NULL);
    assert(loop->method->loop != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(loop->method, loop->header));

    /* Free the previous information	*/
    if (loop->instructions != NULL) {
        xanList_destroyList(loop->instructions);
        loop->instructions	= NULL;
    }
    if (loop->exits != NULL) {
        xanList_destroyList(loop->exits);
        loop->exits = NULL;
    }
    if (updateVariablesInformation) {
        if (loop->invariants != NULL) {
            xanList_destroyList(loop->invariants);
            loop->invariants = NULL;
        }
        if (loop->inductionVariables != NULL) {
            xanList_destroyListAndData(loop->inductionVariables);
            loop->inductionVariables = NULL;
        }
    }

    /* Allocate the lists			*/
    loop->exits = xanList_new(allocFunction, freeFunction, NULL);
    if (updateVariablesInformation) {
        loop->inductionVariables = xanList_new(allocFunction, freeFunction, NULL);
        loop->invariants = xanList_new(allocFunction, freeFunction, NULL);
    }

    /* Compute the set of instructions that	*
     * compose the loop			*/
    loop->instructions = IRMETHOD_getInstructionsWithinAnyCircuitsOfSameHeader(loop->method, loop->header);
    assert(loop->instructions != NULL);

    /* Compute the exit points of the loop	*/
    item = xanList_first(loop->instructions);
    while (item != NULL) {
        ir_instruction_t        *inst;
        ir_instruction_t        *succ;
        JITBOOLEAN		found;

        /* Fetch the instruction		*/
        inst = item->data;
        assert(inst != NULL);
        assert(IRMETHOD_doesInstructionBelongToMethod(loop->method, inst));
        found = JITFALSE;

        /* Check if there is a successor outside the loop, which is not	due to an exception thrown.
         */
        succ = IRMETHOD_getSuccessorInstruction(loop->method, inst, NULL);
        while (succ != NULL) {
            if (	(succ->type != IREXITNODE)				&&
                    (succ->type != IRSTARTCATCHER)				&&
                    ((xanList_find(loop->instructions, succ) == NULL))	) {
                found = JITTRUE;
                break;
            }
            succ = IRMETHOD_getSuccessorInstruction(loop->method, inst, succ);
        }

        /* Add the instruction as exit		*/
        if (found) {
            assert(xanList_find(loop->exits, inst) == NULL);
            xanList_append(loop->exits, inst);
        }

        item = item->next;
    }

    /* Check whether we need to update the variables information also.
     */
    if (!updateVariablesInformation) {
        return ;
    }

    /* Compute the invariants and the       *
     * induction variables			*/
    item = xanList_first(loop->method->loop);
    while (item != NULL) {
        circuit_t  *l;

        /* Fetch the loop		*/
        l = item->data;
        assert(l != NULL);

        /* Check if it is the loop we	*
         * are looking for		*/
        if (l->header_id == loop->header->ID) {
            XanList         *invariants;
            XanList         *inductionVariables;
            XanListItem     *item2;

            /* Add the invariants		*/
            invariants = IRMETHOD_getCircuitInvariants(loop->method, l);
            assert(invariants != NULL);
            item2 = xanList_first(invariants);
            while (item2 != NULL) {
                if (xanList_find(loop->invariants, item2->data) == NULL) {
                    xanList_insert(loop->invariants, item2->data);
                }
                assert(xanList_find(loop->invariants, item2->data) != NULL);
                item2 = item2->next;
            }
            xanList_destroyList(invariants);

            /* Add the induction variables	*/
            inductionVariables = IRMETHOD_getCircuitInductionVariables(loop->method, l);
            assert(inductionVariables != NULL);
            item2 = xanList_first(inductionVariables);
            while (item2 != NULL) {
                induction_variable_t *v;
                induction_variable_t *v_clone;

                /* Fetch the variable		*/
                v = item2->data;
                assert(v != NULL);

                /* Clone the information	*/
                v_clone = allocFunction(sizeof(induction_variable_t));
                memcpy(v_clone, v, sizeof(induction_variable_t));
                assert(v_clone->coordinated == NULL);

                /* Add the variable		*/
                xanList_insert(loop->inductionVariables, v_clone);

                /* Fetch the next element from	*
                 * the list			*/
                item2 = item2->next;
            }

            /* Free the memory		*/
            xanList_destroyList(inductionVariables);
        }

        /* Fetch the next element from	*
         * the list			*/
        item = item->next;
    }

    return;
}

static inline void internal_analyze_method (ir_method_t *method, XanHashTable *loops, XanHashTable *methodsAlreadyConsidered, XanHashTable *outermostLoops, XanList *escapedMethods, XanList *reachableMethods) {
    JITUINT32 count;
    XanList *outermostLoopsOfMethod;
    JITUINT32 instructionsNumber;

    /* Assertions.
     */
    assert(method != NULL);
    assert(loops != NULL);
    assert(methodsAlreadyConsidered != NULL);
    assert(outermostLoops != NULL);

    /* Check if we have already considered the method given as input.
     */
    if (xanHashTable_lookup(methodsAlreadyConsidered, method) != NULL) {
        assert(xanHashTable_lookup(outermostLoops, method) != NULL);
        return;
    }
    xanHashTable_insert(methodsAlreadyConsidered, method, method);
    assert(xanHashTable_lookup(outermostLoops, method) == NULL);

    /* Get the list of outermost loops of the method*/
    outermostLoopsOfMethod = IRLOOP_getOutermostLoopsWithinMethod(method, loops);
    assert(outermostLoopsOfMethod != NULL);
    xanHashTable_insert(outermostLoops, method, outermostLoopsOfMethod);

    /* Fetch the number of instructions of the	*
     * method					*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Analyse the methods reachable with one step	*
     * from the current one that are not yet        *
     * considered					*/
    for (count = 0; count < instructionsNumber; count++) {
        IR_ITEM_VALUE methodID;
        ir_instruction_t        *inst;
        ir_method_t             *calledMethod;
        loop_t                  *currentLoop;
        XanList                 *oneStepLoops;
        XanList			*callableMethods;
        XanListItem		*item;

        /* Fetch the instruction			*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check the instruction			*/
        if (!IRMETHOD_isACallInstruction(inst)) {
            continue;
        }

        /* Take the callable methods 			*/
        callableMethods = IRPROGRAM_getCallableMethods(inst, escapedMethods, reachableMethods);
        if (callableMethods == NULL) {
            continue;
        }
        assert(callableMethods != NULL);

        /* Fetch the most nested loop that includes the current call instruction.
         */
        currentLoop = IRLOOP_getLoopFromInstruction(method, inst, loops);

        /* Analyze every callable method.
         */
        item = xanList_first(callableMethods);
        while (item != NULL) {

            /* Fetch the callable method			*/
            methodID = (IR_ITEM_VALUE)(JITNUINT)item->data;
            assert(methodID != 0);
            calledMethod = IRMETHOD_getIRMethodFromMethodID(method, methodID);

            /* Analyze the callable method			*/
            if (calledMethod != NULL) {

                /* Analyse the called method			*/
                if (xanHashTable_lookup(methodsAlreadyConsidered, calledMethod) == NULL) {
                    internal_analyze_method(calledMethod, loops, methodsAlreadyConsidered, outermostLoops, escapedMethods, reachableMethods);
                }

                /* Take the loops within the called method	*/
                oneStepLoops = xanHashTable_lookup(outermostLoops, calledMethod);
                assert(oneStepLoops != NULL);

                /* Take the loop that include the current call	*
                 * instruction					*/
                if (currentLoop != NULL) {

                    /* Add the loops of the current method to the	*
                     * list of loops included inside the loop that	*
                     * includes the current call instruction	*/
                    assert(currentLoop->method == method);
                    assert(xanList_find(currentLoop->instructions, inst) != NULL);
                    assert(internal_check_loop(currentLoop));
                    internal_append_loops_without_duplications(currentLoop, currentLoop->subLoops, oneStepLoops);
                    assert(internal_check_loop(currentLoop));

                } else {

                    /* The call is not inside any loop of the       *
                     * current method, which means that the		*
                     * outermost loops of the called method are      *
                     * also outermost loops of the current one	*/
                    internal_append_loops_without_duplications(NULL, outermostLoopsOfMethod, oneStepLoops);
                }
            }

            /* Fetch the next element from the list		*/
            item = item->next;
        }

        /* Free the memory			*/
        xanList_destroyList(callableMethods);
    }

    /* Return					*/
    return;
}

static inline JITBOOLEAN internal_check_loops (XanHashTable *loops) {
#ifdef DEBUG
    XanHashTableItem        *hashItem;

    assert(loops != NULL);
    hashItem = xanHashTable_first(loops);
    while (hashItem != NULL) {
        loop_t  *currentLoop;
        currentLoop = hashItem->element;
        assert(currentLoop != NULL);
        assert(currentLoop->method != NULL);
        assert(currentLoop->method->instructions != NULL);
        assert(internal_check_loop(currentLoop));
        if (currentLoop->subLoops != NULL) {
            XanListItem	*item;
            item	= xanList_first(currentLoop->subLoops);
            while (item != NULL) {
                loop_t	*subloop;
                subloop	= item->data;
                assert(subloop != NULL);
                assert(internal_check_loop(subloop));
                item	= item->next;
            }
        }
        hashItem = xanHashTable_next(loops, hashItem);
    }
#endif

    return JITTRUE;
}

static inline JITUINT32 loops_hash_function (void *element) {
    loop_t  *l;

    if (element == NULL) {
        return 0;
    }
    l = element;
    assert(l != NULL);
    return (JITUINT32)(JITNUINT) l->header;
}

static inline JITINT32 loops_equal_functions (void *key1, void *key2) {
    loop_t  *l1;
    loop_t  *l2;

    /* Assertions			*/
    assert(key1 != NULL);
    assert(key2 != NULL);

    /* Check the basic conditions	*/
    if (key1 == key2) {
        return JITTRUE;
    }

    /* Fetch the loops		*/
    l1 = key1;
    l2 = key2;
    assert(l1 != NULL);
    assert(l2 != NULL);

    /* Check the loops              */
    if (    (l1->header != l2->header)      ||
            (l1->method != l2->method)      ) {

        /* Return			*/
        return JITFALSE;
    }

    /* Return			*/
    return JITTRUE;
}

static inline XanList * internal_remove_false_loops (XanList *list) {
    XanList         *l;
    XanListItem     *item;

    if (list == NULL) {
        return NULL;
    }

    /* Allocate the memory		*/
    l = xanList_new(allocFunction, freeFunction, NULL);

    item = xanList_first(list);
    while (item != NULL) {
        circuit_t  *loop;
        loop = item->data;
        assert(loop != NULL);
        if (loop->loop_id != 0) {
            xanList_append(l, loop);
        }
        item = item->next;
    }

    /* Return			*/
    return l;
}

static inline JITBOOLEAN internal_check_loop (loop_t *loop) {
    XanListItem     *item;

    /* Assertions					*/
    assert(loop != NULL);
    assert(loop->exits != NULL);
    assert(IRMETHOD_doInstructionsBelongToMethod(loop->method, loop->instructions));
    assert(IRMETHOD_doInstructionsBelongToMethod(loop->method, loop->exits));
    assert(IRMETHOD_doesInstructionBelongToMethod(loop->method, loop->header));

    /* Check the exit points			*/
    item = xanList_first(loop->exits);
    while (item != NULL) {
        ir_instruction_t        *exit;
        ir_instruction_t        *succ;
        exit = item->data;
        assert(exit != NULL);
        succ = IRMETHOD_getSuccessorInstruction(loop->method, exit, NULL);
        while (succ != NULL) {
            if (xanList_find(loop->instructions, succ) == NULL) {
                break;
            }
            succ = IRMETHOD_getSuccessorInstruction(loop->method, exit, succ);
        }
        item = item->next;
    }

    /* Check the sub-loops				*/
    if (loop->subLoops != NULL) {
        item = xanList_first(loop->subLoops);
        while (item != NULL) {
            loop_t          *subloop;
            subloop = item->data;
            assert(subloop != NULL);
            if (subloop == NULL) {
                abort();
            }
            assert(xanList_equalsInstancesNumber(loop->subLoops, subloop) == 1);
            item = item->next;
        }
    }

    /* Return					*/
    return JITTRUE;
}

static inline void internal_append_loops_without_duplications (loop_t *loop, XanList *l, XanList *loopsToAdd) {
    XanListItem *item;

    /* Assertions			*/
    assert(l != NULL);
    assert(loopsToAdd != NULL);

    /* Check if the two lists are	*
     * the same one			*/
    if (loopsToAdd == l) {

        /* In this case, there is 	*
         * nothing to add		*/
        return ;
    }
    assert(loopsToAdd != l);

    /* Append the loops		*/
    item = xanList_first(loopsToAdd);
    while (item != NULL) {
        loop_t *sub;
        sub = item->data;
        assert(sub != NULL);
        if (xanList_find(l, sub) == NULL) {
            if (	(loop == NULL)						||
                    (sub->method != loop->method)				||
                    (xanList_find(sub->subLoops, loop) == NULL)	) {
                xanList_append(l, sub);
            }
        }
        item = item->next;
    }

    /* Return			*/
    return ;
}
