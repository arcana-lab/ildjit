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
#include <assert.h>
#include <errno.h>
#include <ir_method.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>
#include <chiara.h>

// My headers
#include <optimizer_methodinliner.h>
#include <inline_heuristics.h>
#include <inliner_configuration.h>
// End

extern ir_lib_t 		*irLib;
extern ir_optimizer_t  		*irOptimizer;
extern JITBOOLEAN		avoidMethodsNotIncludedInLoops;

static inline void internal_addReachableCallers (ir_method_t *m, XanHashTable *callGraph, XanHashTable *markedMethods);
static inline void internal_addMethodsWithoutReachableLoops (XanHashTable *set);
static inline void compute_method_information (ir_method_t *method);
static inline JITUINT64 internal_get_program_insns_number (XanList *methods);
static inline JITBOOLEAN methodinliner_inline_called_method (ir_method_t *method, JITUINT32 max_insns_without_loops, JITUINT32 max_insns_with_loops, JITUINT32 max_insns, XanHashTable *methodsToAvoid);

void inline_methods (void) {
    JITBOOLEAN		modified;
    JITBOOLEAN		thereIsSpace;
    XanHashTable		*methodsToAvoid;

    /* Initialize the variables.
     */
    thereIsSpace	= JITTRUE;

    /* Allocate the necessary memory.
     */
    methodsToAvoid	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Avoid methods where no loop are reachable from them.
     */
    internal_addMethodsWithoutReachableLoops(methodsToAvoid);

    /* Inline till a fixed point is reached.
     */
    do {
        XanList                 *list;
        JITUINT32		max_insns_with_loops;

        /* Flush the set of calls to avoid.
         */
        if (!avoidMethodsNotIncludedInLoops) {
            xanHashTable_emptyOutTable(methodsToAvoid);
        }

        /* Delete dead methods.
         */
        setenv("DEADCODE_REMOVE_DEADMETHODS", "1", JITTRUE);
        IROPTIMIZER_callMethodOptimization(irOptimizer, IRPROGRAM_getEntryPointMethod(), DEADCODE_ELIMINATOR);
        setenv("DEADCODE_REMOVE_DEADMETHODS", "0", JITTRUE);

        /* Fetch the list of methods.
         */
        list = fetchMethodsToConsider();
        assert(list != NULL);
        fprintf(stderr, "XAN: %u methods\n", xanList_length(list));

        /* Initialized the variables.
         */
        modified						= JITFALSE;
        inlinedBlockedForMaxInsnsWithLoopsInputConstraints	= JITTRUE;

        /* Inline till there is something to inline with the current constraints.
         */
        for (max_insns_with_loops=0; inlinedBlockedForMaxInsnsWithLoopsInputConstraints && (max_insns_with_loops <= threshold_insns_with_loops); max_insns_with_loops = (max_insns_with_loops < minInsnsWithLoopsInputConstraintsToUnblockInline) ? minInsnsWithLoopsInputConstraintsToUnblockInline + 1 : max_insns_with_loops * 2) {
            JITUINT32	max_caller_insns;

            /* Initialize the variables.
             */
            minInsnsWithLoopsInputConstraintsToUnblockInline		= 0;
            inlinedBlockedForMaxInsnsWithLoopsInputConstraints		= JITFALSE;
            inlinedBlockedForMaxCallerInsnsInputConstraints			= JITTRUE;

            /* Inline till there is something to inline with the current constraints.
             */
            for (max_caller_insns=10; (internal_get_program_insns_number(list) <= threshold_program_insns) && (max_caller_insns <= threshold_caller_insns) && inlinedBlockedForMaxCallerInsnsInputConstraints; max_caller_insns = (max_caller_insns < minCallerInsnsToUnblockInline) ? minCallerInsnsToUnblockInline + 1 : max_caller_insns * 2) {
                JITUINT32	max_insns_without_loops;

                /* Initialize the variables.
                 */
                minCallerInsnsToUnblockInline					= 0;
                inlinedBlockedForMaxCallerInsnsInputConstraints			= JITFALSE;
                inlinedBlockedForMaxInsnsWithoutLoopsInputConstraints		= JITTRUE;

                /* Inline till there is something to inline with the current constraints.
                 */
                for (max_insns_without_loops=100; inlinedBlockedForMaxInsnsWithoutLoopsInputConstraints && (max_insns_without_loops <= threshold_insns_without_loops); max_insns_without_loops = (max_insns_without_loops < minInsnsWithoutLoopsInputConstraintsToUnblockInline) ? minInsnsWithoutLoopsInputConstraintsToUnblockInline + 1 : max_insns_without_loops * 2) {
                    XanListItem             *item;

                    /* Inline methods.
                     */
                    minInsnsWithoutLoopsInputConstraintsToUnblockInline	= 0;
                    inlinedBlockedForMaxInsnsWithoutLoopsInputConstraints	= JITFALSE;
                    item 							= xanList_first(list);
                    while (item != NULL) {
                        ir_method_t     *tempMethod;

                        /* Fetch the method.
                         */
                        tempMethod 			= (ir_method_t *) item->data;
                        assert(tempMethod != NULL);

                        /* Inline the called methods.
                         */
                        modified			|= methodinliner_inline_called_method(tempMethod, max_insns_without_loops, max_insns_with_loops, max_caller_insns, methodsToAvoid);
                        assert(IRMETHOD_getInstructionsNumber(tempMethod) > 0);

                        /* Fetch the next element from the list.
                         */
                        item 				= item->next;
                    }
                }
            }

            /* Check if there is no more space for inlining methods.
             */
            if (	(max_caller_insns <= threshold_caller_insns)		&&
                    (inlinedBlockedForMaxCallerInsnsInputConstraints)	) {
                thereIsSpace	= JITFALSE;
                break ;
            }
        }

        /* Free the memory.
         */
        xanList_destroyList(list);

    } while (modified && thereIsSpace);

    /* Remove dead methods.
     */
    setenv("DEADCODE_REMOVE_DEADMETHODS", "1", JITTRUE);
    IROPTIMIZER_callMethodOptimization(irOptimizer, IRPROGRAM_getEntryPointMethod(), DEADCODE_ELIMINATOR);
    setenv("DEADCODE_REMOVE_DEADMETHODS", "0", JITTRUE);

    /* Free the memory.
     */
    xanHashTable_destroyTable(methodsToAvoid);

    return ;
}

static inline JITUINT64 internal_get_program_insns_number (XanList *methods) {
    JITUINT64	programInstsNumber;
    XanListItem	*item;

    programInstsNumber	= 0;
    item			= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*m;
        m			= item->data;
        programInstsNumber	+= IRMETHOD_getInstructionsNumber(m);
        item			= item->next;
    }
    fprintf(stderr, "XAN: Prog insns %llu\n", programInstsNumber);

    return programInstsNumber;
}

static inline JITBOOLEAN methodinliner_inline_called_method (ir_method_t *method, JITUINT32 max_caller_insns_without_loops, JITUINT32 max_insns_with_loops, JITUINT32 max_insns, XanHashTable *methodsToAvoid) {
    JITBOOLEAN 	modified;
    XanList 	*methodsToInline;

    /* Assertions.
     */
    assert(method != NULL);
    PDEBUG("METHOD INLINER: Start\n");
    PDEBUG("METHOD INLINER:         Method %s\n", IRMETHOD_getSignatureInString(method));
    PDEBUG("METHOD INLINER:         Instructions number = %u\n", IRMETHOD_getInstructionsNumber(method));

    /* Initialize the variables.
     */
    modified	= JITFALSE;

    /* Print the input method.
     */
#ifdef DUMP_ALL_CFG
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
#endif

    /* Fetch the list of call instruciton to inline.
     */
    methodsToInline = fetchCallInstructions(method, max_caller_insns_without_loops, max_insns_with_loops, max_insns, methodsToAvoid);
    assert(methodsToInline != NULL);

    /* Remove methods to avoid.
     */
    if (xanHashTable_elementsInside(methodsToAvoid) > 0) {
        XanList		*toDelete;
        XanListItem	*item;

        toDelete	= xanList_new(allocFunction, freeFunction, NULL);
        item		= xanList_first(methodsToInline);
        while (item != NULL) {
            ir_method_t	*m;
            m	= item->data;
            assert(m != NULL);
            if (xanHashTable_lookup(methodsToAvoid, m) != NULL) {
                xanList_append(toDelete, m);
            }
            item	= item->next;
        }
        xanList_removeElements(methodsToInline, toDelete, JITTRUE);
        xanList_destroyList(toDelete);
    }

    /* Check if we should inline something.
     */
    if (xanList_length(methodsToInline) > 0) {

        /* Inline methods.
         */
        PDEBUG("METHOD INLINER:         Inline methods\n");
        modified 	= METHODS_inlineCalledMethods(irOptimizer, method, methodsToInline, remove_calls, 0);

        /* Recompute method information.
         */
        if (modified) {
            PDEBUG("METHOD INLINER:         Compute method information\n");
            compute_method_information(method);

        } else {

            /* The instruction calls given as input are not inlined.
             * Add these calls to the list of ones to avoid.
             */
            xanHashTable_addList(methodsToAvoid, methodsToInline);
        }
    }

    /* Free the memory.
     */
    xanList_destroyList(methodsToInline);

    /* Optimize the escapes.
     */
    IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

    /* Print the output method.
     */
#ifdef DUMP_ALL_CFG
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
#endif

    PDEBUG("METHOD INLINER: Exit\n");
    return modified;
}

static inline void compute_method_information (ir_method_t *method) {

    /* Assertions				*/
    assert(method != NULL);

    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, PRE_DOMINATOR_COMPUTER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LOOP_IDENTIFICATION);

    /* Return				*/
    return;
}

static inline void internal_addMethodsWithoutReachableLoops (XanHashTable *set) {
    XanHashTable 		*loops;
    XanHashTable 		*callGraph;
    XanHashTable 		*markedMethods;
    XanList			*programLoops;
    XanList			*methodsWithProgramLoops;
    XanList			*allMethods;
    XanListItem		*item;

    /* Assertions.
     */
    assert(set != NULL);

    /* Analyze the loops of the program.
     */
    loops 			= IRLOOP_analyzeLoops(LOOPS_analyzeCircuits);
    assert(loops != NULL);

    /* Fetch the loops defined by the program and not by external libraries.
     */
    programLoops		= LOOPS_getProgramLoops(loops);
    assert(programLoops != NULL);

    /* Fetch the methods that contain the program loops.
     */
    methodsWithProgramLoops	= LOOPS_getMethods(programLoops);
    assert(methodsWithProgramLoops != NULL);

    /* Fetch the call graph.
     */
    callGraph	= METHODS_getMethodsCallerGraph(JITTRUE);
    assert(callGraph != NULL);

    /* Mark all methods that can lead to a program loop or that are contained in loops.
     */
    markedMethods	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    item		= xanList_first(methodsWithProgramLoops);
    while (item != NULL) {
        ir_method_t	*m;
        XanList		*l;
        XanListItem	*item2;

        /* Fetch a method that contains a program loop.
         */
        m	= item->data;
        assert(m != NULL);

        /* Fetch the methods that are reachable from the current one.
         */
        l	= IRPROGRAM_getReachableMethods(m);
        assert(l != NULL);

        /* Mark all methods that can lead to a program loop.
         */
        internal_addReachableCallers(m, callGraph, markedMethods);

        /* Mark all methods that are contained within a loop at least.
         */
        item2	= xanList_first(l);
        while (item2 != NULL) {
            ir_method_t	*m2;
            m2	= item2->data;
            assert(m2 != NULL);
            if (xanHashTable_lookup(markedMethods, m2) == NULL) {
                xanHashTable_insert(markedMethods, m2, m2);

            }
            item2	= item2->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(l);

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Tag as methods to not inline the ones from which no program loop can be reached.
     * These methods are the unmarked ones.
     */
    allMethods	= IRPROGRAM_getIRMethods();
    assert(allMethods != NULL);
    item		= xanList_first(allMethods);
    while (item != NULL) {
        ir_method_t	*m;
        m	= item->data;
        if (xanHashTable_lookup(markedMethods, m) == NULL) {
            fprintf(stderr, "NOINLINE: %s: %s\n", IRPROGRAM_doesMethodBelongToProgram(m) ? "program" : "library", (char *) IRMETHOD_getSignatureInString(m));
            xanHashTable_insert(set, m, m);
        }
        item	= item->next;
    }

    /* Free the memory.
     */
    IRLOOP_destroyLoops(loops);
    xanList_destroyList(programLoops);
    METHODS_destroyMethodsCallerGraph(callGraph);
    xanList_destroyList(methodsWithProgramLoops);
    xanHashTable_destroyTable(markedMethods);
    xanList_destroyList(allMethods);

    return ;
}

static inline void internal_addReachableCallers (ir_method_t *m, XanHashTable *callGraph, XanHashTable *markedMethods) {
    XanListItem	*item;
    XanList		*callees;

    /* Assertions.
     */
    assert(m != NULL);
    assert(callGraph != NULL);
    assert(markedMethods != NULL);

    /* Check whether the method has been already marked.
     */
    if (xanHashTable_lookup(markedMethods, m) != NULL) {
        return ;
    }
    xanHashTable_insert(markedMethods, m, m);

    /* Fetch the callees.
     */
    callees		= xanHashTable_lookup(callGraph, m);
    if (callees == NULL) {
        return ;
    }

    /* Propagate the analysis through the callees.
     */
    item	= xanList_first(callees);
    while (item != NULL) {
        ir_method_t	*c;
        c	= item->data;
        assert(c != NULL);
        internal_addReachableCallers(c, callGraph, markedMethods);
        item	= item->next;
    }

    return ;
}
