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
#include <loops_invocations.h>
#include <chiara_loops_profiler_misc.h>
// End

//#define PRINT_LOOPS_EXECUTION
//#define DUMP_LOOP_STACK

static inline void internal_dumpEdgeInformation (loop_invocations_t *inv, FILE **fileToWrite, FILE *summaryFile, JITUINT32 *bytesDumped, JITUINT32 *suffixNumber, JITINT8 *prefixName, JITBOOLEAN splittablePoint);
static inline void loop_invocation_external_profiler_new_loop (loop_profile_t *loop);
static inline void loop_invocation_external_profiler_end_loop (void);
static inline void loop_invocation_profiler_new_loop (loop_profile_t *loop);
static inline void loop_invocation_profiler_end_loop (void);
static inline void internal_allocate_start_times (XanList *list);
static inline XanList * loop_iterations_new_subloop_at_nesting_level (loop_profile_t *profile, XanList *parentLoops, JITUINT32 nestingLevel);
static inline void loop_iterations_end_subloop_at_nesting_level (void);
static inline void loop_iterations_new_subloop_iteration (XanList *l);
static inline void loop_iterations_new_loop (loop_profile_t *profile);
static inline void loop_iterations_end_loop (void);
static inline void loop_iterations_new_iteration_data_forwarded (void);
static inline void loop_time_iterations_new_loop (loop_profile_t *profile);
static inline void loop_time_iterations_end_loop (void);
static inline void loop_time_iterations_new_iteration (void);
static inline JITUINT64 start_new_outermost_loop (loop_profile_t *profile);
static inline void new_subloop_at_nesting_level (loop_profile_t *profile, XanList *parentLoops, JITUINT32 nestingLevel);
static inline void end_outermost_loop (void);
static inline void end_outermost_loop_baseline (void);
static inline void end_total_time_subloop_at_nesting_level (JITUINT32 nestingLevel);
static inline void end_total_time_subloop_at_nesting_level_baseline (JITUINT32 nestingLevel);
static inline void end_subloop_at_nesting_level (JITUINT32 nestingLevel);
static inline void internal_store_performed (void *start, void *end);
static inline void internal_load_performed (void *start, void *end);
static inline JITBOOLEAN internal_was_the_memory_written (memory_t *m, JITUINT32 top, void *start, void *end);
static inline JITBOOLEAN internal_was_the_memory_written_locally (void *start, void *end);
static inline JITBOOLEAN internal_was_the_memory_written_globally (void *start, void *end);
static inline JITBOOLEAN internal_was_the_memory_read_locally (void *start, void *end);
static inline void internal_inject_code_to_compute_forwarded_bytes_across_iterations (XanList *loops, ir_method_t *method, XanList *instructions);
static inline void internal_erase_memory_written (void);
static inline XanList * internal_LOOPS_profileSubloopsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel, IR_ITEM_VALUE newLoopfunctionPointer, IR_ITEM_VALUE endLoopFunctionPointer, IR_ITEM_VALUE newIterationFunctionPointer);
static inline void end_loop (void);
static inline void start_loop (loop_profile_t *profile);
static inline void internal_update_clock_ticks (loop_profile_t *profile, JITBOOLEAN updateLoopNestingLevel);
static inline void internal_add_memory_written (memory_t **memory, JITUINT32 *top, JITUINT32 *size, void *start, void *end);
static inline void internal_add_local_memory_to_global_memory (void);
static inline void internal_erase_local_memory_written (void);
static inline void internal_write_variable (ir_method_t *method, JITUINT32 varID, JITUINT32 bytes, JITUINT32 locallyProduced);
static inline void internal_read_variable (ir_method_t *method, JITUINT32 varID, JITUINT32 bytes, JITUINT32 locallyProduced);
static inline void internal_inject_code_for_variable_accesses (ir_method_t *method, XanList *inductionOrInvariantVariables, ir_instruction_t *beforeInst, ir_item_t *param, IR_ITEM_VALUE nativeFunction, IR_ITEM_VALUE nativeFunctionName);
static inline JITBOOLEAN internal_fetch_name (JITINT8 **fileName, FILE *summaryFile, JITUINT32 *fileNameSize, JITINT8 endCh);
static inline void internal_dumpEdgeInformationAndResetTop (loop_invocations_t *inv);
static inline void internal_closeEdgesInformationFiles (XanGraph *g);
static inline void internal_dumpAndResizeEdgesInformation (XanGraph *g);
static inline IR_ITEM_VALUE internal_fetchLoopName (loop_profile_t *p);
static inline IR_ITEM_VALUE internal_fetchLoopProfile (loop_profile_t *p);

static XanGraph *g = NULL;
static XanHashTable *startTimes = NULL;
static JITUINT64 loopStartTime = 0;
static JITUINT32 loopExecutionNesting = 0;
static XanStack *loopStack = NULL;
static loop_profile_t *outermostLoop = NULL;
static loop_profile_data_t *profileDataToUse = NULL;
static XanList *outermostLoopsToConsider = NULL;
static memory_t *storesPerformed = NULL;
static memory_t *localStoresPerformed = NULL;
static memory_t *localReadsPerformed = NULL;
static JITUINT32 storesPerformedSize = 0;
static JITUINT32 localStoresPerformedSize = 0;
static JITUINT32 localReadsPerformedSize = 0;
static JITUINT32 storesPerformedTop = 0;
static JITUINT32 localStoresPerformedTop = 0;
static JITUINT32 localReadsPerformedTop = 0;
static JITUINT32 *methodVariables = NULL;
static JITUINT32 *globalMethodVariables = NULL;
static JITUINT32 *methodVariablesRead = NULL;
static JITUINT32 methodVariablesNumber = 0;
static void (*enterLoopFunction) (loop_profile_t *loop);
static void (*exitLoopFunction) (void);

void LOOPS_profileLoopsWithNamesByUsingSymbols (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, ir_symbol_t *newLoop, ir_symbol_t *endLoop, ir_symbol_t *newIteration, XanHashTable *loopsPredecessors, XanHashTable *loopsSuccessors, XanList *addedInstructions) {
    XanList			*p;
    XanListItem		*item;
    JITBOOLEAN		computePredecessorsSuccessors;

    /* Assertions.
     */
    assert(irOptimizer != NULL);
    assert(loops != NULL);
    assert(loopsNames != NULL);

    /* Initialize the variables.
     */
    computePredecessorsSuccessors	= JITFALSE;

    /* Allocate the profile data structures.
     */
    p				= get_profiled_loops(loops, NULL);
    assert(p != NULL);

    /* Compute the successors of the loops to profile.
     */
    if (loopsSuccessors == NULL) {
        loopsSuccessors 		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        computePredecessorsSuccessors	= JITTRUE;
    }
    if (loopsPredecessors == NULL) {
        loopsPredecessors 		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        computePredecessorsSuccessors	= JITTRUE;
    }
    if (computePredecessorsSuccessors) {
        LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);
    }
    assert(loopsPredecessors != NULL);
    assert(loopsSuccessors != NULL);

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

        /* Check if we should consider the current loop.
         */
        if (	(noSystemLibraries == JITFALSE)					||
                (!IRPROGRAM_doesMethodBelongToALibrary(profile->loop->method))	) {
            XanList 	*successors;
            XanList 	*predecessors;
            ir_symbol_t	*nameSym;

            /* Fetch the successors and predecessors of the loop.
             */
            successors 				= xanHashTable_lookup(loopsSuccessors, profile->loop);
            assert(successors != NULL);
            predecessors 				= xanHashTable_lookup(loopsPredecessors, profile->loop);
            assert(predecessors != NULL);

            /* Fetch the symbol.
             */
            nameSym					= xanHashTable_lookup(loopsNames, profile->loop);
            assert(nameSym != NULL);

            /* Create the parameters.
             */
            start_new_outermost_loop_parameters 	= xanList_new(allocFunction, freeFunction, NULL);
            IRSYMBOL_storeSymbolToIRItem(&currentItem, nameSym, IRNUINT, NULL);
            xanList_append(start_new_outermost_loop_parameters, &currentItem);

            /* Profile the loop.
             */
            profileLoopByUsingSymbols	 (
                irOptimizer,
                profile,
                newLoop, NULL, start_new_outermost_loop_parameters,
                endLoop, NULL, NULL,
                newIteration, NULL, NULL,
                successors, predecessors, addedInstructions);

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
    if (computePredecessorsSuccessors) {
        xanHashTable_destroyTable(loopsSuccessors);
        xanHashTable_destroyTable(loopsPredecessors);
    }

    return ;
}

void LOOPS_profileLoopsWithNames (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, void (*newLoop)(JITINT8 *loopName), void (*endLoop)(void), void (*newIteration)(void), void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst)) {
    instrument_loops(irOptimizer, loops, noSystemLibraries, loopsNames, (void (*)(void*))newLoop, endLoop, newIteration, addCodeFunction, internal_fetchLoopName);

    return ;
}

void LOOPS_profileOutermostLoopsEnd (void) {
    if (loopExecutionNesting > 0) {
        end_outermost_loop();
    }
}

void LOOPS_profileTotalExecutionTimeOfSubloopsAtNestingLevelEnd (JITUINT32 nestingLevel) {
    if (loopExecutionNesting > 0) {
        end_total_time_subloop_at_nesting_level(nestingLevel);
    }
}

void LOOPS_profileLoopsIterationsEnd (void) {

    /* Free the memory		*/
    if (outermostLoopsToConsider != NULL) {
        xanList_destroyList(outermostLoopsToConsider);
        outermostLoopsToConsider = NULL;
    }
    if (loopStack != NULL) {
        xanStack_destroyStack(loopStack);
        loopStack = NULL;
    }
    if (startTimes != NULL) {
        XanHashTableItem *hashItem;
        hashItem = xanHashTable_first(startTimes);
        while (hashItem != NULL) {
            XanStack *startTimesStack;
            startTimesStack = hashItem->element;
            assert(startTimesStack != NULL);
            if (xanStack_getSize(startTimesStack) > 0) {
                loop_profile_t *profile;
                profile = hashItem->elementID;
                fprintf(stderr, "CHIARA: profileLoops: Error = Loop %s:%u seems to be still in execution\n", IRMETHOD_getSignatureInString(profile->loop->method), profile->loop->header->ID);
                abort();
            }
            xanStack_destroyStack(startTimesStack);
            hashItem = xanHashTable_next(startTimes, hashItem);
        }
        xanHashTable_destroyTable(startTimes);
        startTimes = NULL;
    }

    /* Return			*/
    return ;
}

XanList * LOOPS_profileTotalExecutionTimeOfSubloopsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel) {
    XanListItem *item;
    XanList *list;
    XanList *loops;
    XanList *cloneOutermostLoops;
    XanHashTable *loopsSuccessors;
    XanHashTable *loopsPredecessors;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(allLoops != NULL);
    assert(outermostLoops != NULL);
    assert(xanList_length(outermostLoops) > 0);

    /* Allocate the necessary 	*
     * memories			*/
    loopStack = xanStack_new (allocFunction, freeFunction, NULL);
    assert(loopStack != NULL);
    startTimes = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(startTimes != NULL);

    /* Fetch the loops to profile	*/
    loops = xanList_cloneList(outermostLoops);
    LOOPS_addProgramSubLoopsMayBeAtNestingLevels(outermostLoops, 1, nestingLevel, loops);

    /* Allocate the profile	data for*
     * the loops			*/
    list = get_profiled_loops(loops, NULL);
    assert(list != NULL);

    /* Allocate the startTimes 	*
     * hash table			*/
    internal_allocate_start_times(list);

    /* Clone the outermost loops	*
     * This is necessary because the*
     * caller can free this list 	*
     * after the code 		*
     * instrumentation, but this 	*
     * profile will use it at 	*
     * runtime			*/
    cloneOutermostLoops = xanList_cloneList(outermostLoops);

    /* Compute the successors of	*
     * the loops to profile		*/
    loopsSuccessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Profile the loops		*/
    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        ir_item_t *currentItem;
        XanList *start_new_loop_parameters;
        XanList *end_subloop_parameters;
        XanList *successors;
        XanList *predecessors;

        /* Fetch the profile data		*/
        profile = item->data;
        assert(profile != NULL);

        /* Specify the arguments		*/
        start_new_loop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = (IR_ITEM_VALUE)(JITNUINT) profile;
        currentItem->type = IRNUINT;
        currentItem->internal_type = IRNUINT;
        xanList_append(start_new_loop_parameters, currentItem);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = (IR_ITEM_VALUE)(JITNUINT) cloneOutermostLoops;
        currentItem->type = IRNUINT;
        currentItem->internal_type = IRNUINT;
        xanList_append(start_new_loop_parameters, currentItem);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = nestingLevel;
        currentItem->type = IRUINT32;
        currentItem->internal_type = IRUINT32;
        xanList_append(start_new_loop_parameters, currentItem);
        end_subloop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = nestingLevel;
        currentItem->type = IRUINT32;
        currentItem->internal_type = IRUINT32;
        xanList_append(end_subloop_parameters, currentItem);

        /* Fetch the successors of the loop	*/
        successors = xanHashTable_lookup(loopsSuccessors, profile->loop);
        assert(successors != NULL);

        /* Fetch the predecessors of the loop	*
         * header				*/
        predecessors = xanHashTable_lookup(loopsPredecessors, profile->loop);
        assert(predecessors != NULL);

        /* Profile the loop			*/
        profileLoop	 (	irOptimizer,
                        profile,
                        new_subloop_at_nesting_level, NULL, start_new_loop_parameters,
                        end_total_time_subloop_at_nesting_level, NULL, end_subloop_parameters,
                        NULL, NULL, NULL,
                        successors, predecessors, NULL);

        /* Free the memory			*/
        xanList_destroyListAndData(start_new_loop_parameters);
        xanList_destroyListAndData(end_subloop_parameters);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);
    xanList_destroyList(loops);

    /* Return 			*/
    return list;
}

XanList * LOOPS_profileSubloopsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel) {
    XanListItem *item;
    XanList *list;
    XanList *loops;
    XanList *cloneOutermostLoops;
    XanHashTable *loopsSuccessors;
    XanHashTable *loopsPredecessors;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(allLoops != NULL);
    assert(outermostLoops != NULL);
    assert(xanList_length(outermostLoops) > 0);

    /* Allocate the necessary 	*
     * memories			*/
    loopStack = xanStack_new (allocFunction, freeFunction, NULL);
    assert(loopStack != NULL);
    startTimes = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(startTimes != NULL);

    /* Fetch the loops to profile	*/
    loops = xanList_cloneList(outermostLoops);
    LOOPS_addProgramSubLoopsMayBeAtNestingLevels(outermostLoops, 1, nestingLevel, loops);

    /* Allocate the profile	data for*
     * the loops			*/
    list = get_profiled_loops(loops, NULL);
    assert(list != NULL);

    /* Allocate the startTimes 	*
     * hash table			*/
    internal_allocate_start_times(list);

    /* Clone the outermost loops	*
     * This is necessary because the*
     * caller can free this list 	*
     * after the code 		*
     * instrumentation, but this 	*
     * profile will use it at 	*
     * runtime			*/
    cloneOutermostLoops = xanList_cloneList(outermostLoops);

    /* Compute the successors of	*
     * the loops to profile		*/
    loopsSuccessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Profile the loops		*/
    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        ir_item_t *currentItem;
        XanList *start_new_loop_parameters;
        XanList *end_subloop_parameters;
        XanList *successors;
        XanList *predecessors;

        /* Fetch the profile data		*/
        profile = item->data;
        assert(profile != NULL);

        /* Specify the arguments		*/
        start_new_loop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = (IR_ITEM_VALUE)(JITNUINT) profile;
        currentItem->type = IRNUINT;
        currentItem->internal_type = IRNUINT;
        xanList_append(start_new_loop_parameters, currentItem);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = (IR_ITEM_VALUE)(JITNUINT) cloneOutermostLoops;
        currentItem->type = IRNUINT;
        currentItem->internal_type = IRNUINT;
        xanList_append(start_new_loop_parameters, currentItem);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = nestingLevel;
        currentItem->type = IRUINT32;
        currentItem->internal_type = IRUINT32;
        xanList_append(start_new_loop_parameters, currentItem);
        end_subloop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        currentItem = allocFunction(sizeof(ir_item_t));
        currentItem->value.v = nestingLevel;
        currentItem->type = IRUINT32;
        currentItem->internal_type = IRUINT32;
        xanList_append(end_subloop_parameters, currentItem);

        /* Fetch the successors of the loop	*/
        successors = xanHashTable_lookup(loopsSuccessors, profile->loop);
        assert(successors != NULL);

        /* Fetch the predecessors of the loop	*
         * header				*/
        predecessors = xanHashTable_lookup(loopsPredecessors, profile->loop);
        assert(predecessors != NULL);

        /* Profile the loop			*/
        profileLoop	 (	irOptimizer,
                        profile,
                        new_subloop_at_nesting_level, NULL, start_new_loop_parameters,
                        end_subloop_at_nesting_level, NULL, end_subloop_parameters,
                        NULL, NULL, NULL,
                        successors, predecessors, NULL);

        /* Free the memory			*/
        xanList_destroyListAndData(start_new_loop_parameters);
        xanList_destroyListAndData(end_subloop_parameters);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);
    xanList_destroyList(loops);

    /* Return 			*/
    return list;
}

XanList * LOOPS_profileSubloopsIterationsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel) {
    XanList *list;
    IR_ITEM_VALUE newLoopfunctionPointer;
    IR_ITEM_VALUE endLoopFunctionPointer;
    IR_ITEM_VALUE newIterationFunctionPointer;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(allLoops != NULL);
    assert(outermostLoops != NULL);
    assert(xanList_length(outermostLoops) > 0);

    /* Set the function pointers	*/
    newLoopfunctionPointer = (IR_ITEM_VALUE)(JITNUINT) loop_iterations_new_subloop_at_nesting_level;
    endLoopFunctionPointer = (IR_ITEM_VALUE)(JITNUINT) loop_iterations_end_subloop_at_nesting_level;
    newIterationFunctionPointer = (IR_ITEM_VALUE)(JITNUINT) loop_iterations_new_subloop_iteration ;

    /* Profile the loops		*/
    list = internal_LOOPS_profileSubloopsAtNestingLevel(irOptimizer, allLoops, outermostLoops, nestingLevel, newLoopfunctionPointer, endLoopFunctionPointer, newIterationFunctionPointer);

    /* Return			*/
    return list;
}

static inline XanList * internal_LOOPS_profileSubloopsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel, IR_ITEM_VALUE newLoopfunctionPointer, IR_ITEM_VALUE endLoopFunctionPointer, IR_ITEM_VALUE newIterationFunctionPointer) {
    XanList *list;
    XanList *loopsToProfile;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(allLoops != NULL);
    assert(outermostLoops != NULL);
    assert(xanList_length(outermostLoops) > 0);

    /* Fetch the list of loops that	*
     * may be within the nesting	*
     * level specified in input	*/
    loopsToProfile = LOOPS_getSubLoopsMayBeAtNestingLevel(outermostLoops, nestingLevel);
    assert(loopsToProfile != NULL);

    /* Allocate the necessary 	*
     * memories			*/
    loopStack = xanStack_new(allocFunction, freeFunction, NULL);
    assert(loopStack != NULL);
    outermostLoopsToConsider = xanList_cloneList(outermostLoops);

    /* Allocate the profile	data	*
     * the loops			*/
    list = get_profiled_loops(loopsToProfile, NULL);
    assert(list != NULL);
    abort();

    /* Return			*/
    return list;
}

XanList * LOOPS_profileDataForwardedAcrossLoopsIterations (ir_optimizer_t *irOptimizer, XanList *loops) {
    XanListItem *item;
    XanList *list;
    XanList *toInject;
    void *newLoopfunctionPointer;
    void *endLoopFunctionPointer;
    void *newIterationFunctionPointer;
    JITUINT32 initialSize;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);

    /* Allocate the profile	data	*
     * the loops			*/
    list = get_profiled_loops(loops, NULL);
    assert(list != NULL);

    /* Allocate the list of data 	*
     * structures that describe the	*
     * memory addresses written	*/
    initialSize 			= 1000;
    storesPerformed 		= allocFunction(sizeof(memory_t) * initialSize);
    storesPerformedSize 		= initialSize;
    localStoresPerformed 		= allocFunction(sizeof(memory_t) * initialSize);
    localStoresPerformedSize 	= initialSize;
    localReadsPerformed 		= allocFunction(sizeof(memory_t) * initialSize);
    localReadsPerformedSize 	= initialSize;

    /* Set the function pointers	*/
    newLoopfunctionPointer 		= loop_iterations_new_loop;
    endLoopFunctionPointer 		= loop_iterations_end_loop;
    newIterationFunctionPointer 	= loop_iterations_new_iteration_data_forwarded;

    /* Profile the loops		*/
    toInject = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        ir_method_t *m;
        XanListItem *item2;
        XanList *l;
        XanList *start_new_outermost_loop_parameters;
        ir_item_t currentItem;

        /* Fetch the loop to profile		*/
        profile = item->data;
        assert(profile != NULL);

        /* Fetch the method			*/
        m = profile->loop->method;
        assert(m != NULL);

        /* Fetch the list of reachable methods	*/
        l = IRPROGRAM_getReachableMethods(m);
        item2 = xanList_first(l);
        while (item2 != NULL) {
            if (xanList_find(toInject, item2->data) == NULL) {
                xanList_append(toInject, item2->data);
            }
            item2 = item2->next;
        }
        xanList_destroyList(l);

        /* Create the parameters		*/
        start_new_outermost_loop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        memset(&currentItem, 0, sizeof(ir_item_t));
        currentItem.value.v 		= (IR_ITEM_VALUE)(JITNUINT) profile;
        currentItem.type 		= IRNUINT;
        currentItem.internal_type 	= IRNUINT;
        xanList_append(start_new_outermost_loop_parameters, &currentItem);

        /* Profile the loop			*/
        profileLoop 	(	irOptimizer,
                            profile,
                            newLoopfunctionPointer, NULL, start_new_outermost_loop_parameters,
                            endLoopFunctionPointer, NULL, NULL,
                            newIterationFunctionPointer, NULL, NULL,
                            NULL, NULL, NULL);

        /* Free the memory			*/
        xanList_destroyList(start_new_outermost_loop_parameters);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Inject code			*/
    item = xanList_first(toInject);
    while (item != NULL) {
        ir_method_t *methodToConsider;
        methodToConsider = item->data;
        internal_inject_code_to_compute_forwarded_bytes_across_iterations(loops, methodToConsider, NULL);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(toInject);

    /* Return			*/
    return list;
}

XanList * LOOPS_profileLoopsIterations (ir_optimizer_t *irOptimizer, XanList *loops, XanHashTable *loopsNames) {
    XanListItem *item;
    XanList *list;
    void *newLoopfunctionPointer;
    void *endLoopFunctionPointer;
    void *newIterationFunctionPointer;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);
    assert(loopsNames != NULL);

    /* Allocate the profile	data	*
     * the loops			*/
    list = get_profiled_loops(loops, NULL);
    assert(list != NULL);

    /* Set the function pointers	*/
    newLoopfunctionPointer 		= loop_time_iterations_new_loop;
    endLoopFunctionPointer 		= loop_time_iterations_end_loop;
    newIterationFunctionPointer 	= loop_time_iterations_new_iteration;

    /* Profile the loops		*/
    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        XanList *start_new_outermost_loop_parameters;
        ir_item_t currentItem;

        /* Fetch the loop to profile		*/
        profile = item->data;
        assert(profile != NULL);
        profile->loopName = xanHashTable_lookup(loopsNames, profile->loop);

        /* Create the parameters		*/
        start_new_outermost_loop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        memset(&currentItem, 0, sizeof(ir_item_t));
        currentItem.value.v = (IR_ITEM_VALUE)(JITNUINT) profile;
        currentItem.type = IRNUINT;
        currentItem.internal_type = IRNUINT;
        xanList_append(start_new_outermost_loop_parameters, &currentItem);

        /* Profile the loop			*/
        profileLoop 	(	irOptimizer,
                            profile,
                            newLoopfunctionPointer, NULL, start_new_outermost_loop_parameters,
                            endLoopFunctionPointer, NULL, NULL,
                            newIterationFunctionPointer, NULL, NULL,
                            NULL, NULL, NULL);

        /* Free the memory			*/
        xanList_destroyList(start_new_outermost_loop_parameters);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Return			*/
    return list;
}

XanList * LOOPS_profileNestingLevelsOfLoops (ir_optimizer_t *irOptimizer, XanList *loops) {
    XanListItem *item;
    XanList *list;
    XanHashTable *loopsSuccessors;
    XanHashTable *loopsPredecessors;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);

    /* Allocate the profile	data	*
     * the loops			*/
    list = get_profiled_loops(loops, NULL);
    assert(list != NULL);

    /* Compute the successors of	*
     * the loops to profile		*/
    loopsSuccessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Profile the loops		*/
    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        ir_item_t currentItem;
        XanList * start_new_outermost_loop_parameters;
        XanList *successors;
        XanList *predecessors;

        /* Fetch the profile data		*/
        profile = item->data;
        assert(profile != NULL);
        assert(profile->loop != NULL);
        assert(profile->loop->method != NULL);
        assert(profile->loop->method->instructions != NULL);

        /* Specify the arguments		*/
        start_new_outermost_loop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        memset(&currentItem, 0, sizeof(ir_item_t));
        currentItem.value.v = (IR_ITEM_VALUE)(JITNUINT) profile;
        currentItem.type = IRNUINT;
        currentItem.internal_type = IRNUINT;
        xanList_append(start_new_outermost_loop_parameters, &currentItem);

        /* Fetch the successors of the loop	*/
        successors = xanHashTable_lookup(loopsSuccessors, profile->loop);
        assert(successors != NULL);

        /* Fetch the predecessors of the loop	*
         * header				*/
        predecessors = xanHashTable_lookup(loopsPredecessors, profile->loop);
        assert(predecessors != NULL);

        /* Profile the loop			*/
        profileLoop	 (	irOptimizer,
                        profile,
                        start_loop, NULL, start_new_outermost_loop_parameters,
                        end_loop, NULL, NULL,
                        NULL, NULL, NULL,
                        successors, predecessors, NULL);

        /* Free the memory			*/
        xanList_destroyList(start_new_outermost_loop_parameters);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);

    /* Return 			*/
    return list;
}

XanList * LOOPS_profileOutermostLoops (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN runBaseline, XanList *loopsBaseline) {
    XanListItem *item;
    XanList *list;
    XanList *toDelete;
    XanHashTable *loopsSuccessors;
    XanHashTable *loopsPredecessors;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);

    /* Allocate the necessary 	*
     * memories			*/
    startTimes = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(startTimes != NULL);
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Allocate the profile	data	*
     * the loops			*/
    list = get_profiled_loops(loops, NULL);
    assert(list != NULL);

    /* Allocate the startTimes 	*
     * hash table			*/
    internal_allocate_start_times(list);

    /* Compute the successors of	*
     * the loops to profile		*/
    loopsSuccessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Profile the loops		*/
    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        ir_item_t start_new_outermost_loop_return;
        ir_item_t currentItem;
        XanList * start_new_outermost_loop_parameters;
        XanList *successors;
        XanList *predecessors;
        void *endFunction;

        /* Fetch the profile data		*/
        profile = item->data;
        assert(profile != NULL);

        /* Set the end function			*/
        endFunction	= NULL;
        if (!runBaseline) {
            if (	(loopsBaseline == NULL)						||
                    (xanList_find(loopsBaseline, profile->loop) == NULL)	) {
                endFunction	= end_outermost_loop;
            }
        }
        if (endFunction == NULL) {
            endFunction 	= end_outermost_loop_baseline;
        }
        assert(endFunction != NULL);

        /* Allocate the necessary memory	*/
        if (xanList_length(profile->times) == 0) {
            loop_profile_data_t *data;
            data = allocMemory(sizeof(loop_profile_data_t));
            assert(data != NULL);
            xanList_append(profile->times, data);
        }

        /* Specify the arguments		*/
        memset(&start_new_outermost_loop_return, 0, sizeof(ir_item_t));
        start_new_outermost_loop_return.value.v = IRMETHOD_newVariableID(profile->loop->method);
        start_new_outermost_loop_return.type = IROFFSET;
        start_new_outermost_loop_return.internal_type = IRUINT64;
        start_new_outermost_loop_parameters = xanList_new(allocFunction, freeFunction, NULL);
        memset(&currentItem, 0, sizeof(ir_item_t));
        currentItem.value.v = (IR_ITEM_VALUE)(JITNUINT) profile;
        currentItem.type = IRNUINT;
        currentItem.internal_type = IRNUINT;
        xanList_append(start_new_outermost_loop_parameters, &currentItem);

        /* Fetch the successors of the loop	*/
        successors = xanHashTable_lookup(loopsSuccessors, profile->loop);
        assert(successors != NULL);

        /* Fetch the predecessors of the loop	*
         * header				*/
        predecessors = xanHashTable_lookup(loopsPredecessors, profile->loop);
        assert(predecessors != NULL);

        /* Profile the loop			*/
        profileLoop	 (	irOptimizer,
                        profile,
                        start_new_outermost_loop, &start_new_outermost_loop_return, start_new_outermost_loop_parameters,
                        endFunction, NULL, NULL,
                        NULL, NULL, NULL,
                        successors, predecessors, NULL);

        /* If the current loop is considered as	*
         * baseline, it cannot be included 	*
         * inside the list of loops profiled 	*
         * which is later returned		*/
        if (endFunction == end_outermost_loop_baseline) {
            xanList_append(toDelete, profile);
        }

        /* Free the memory			*/
        xanList_destroyList(start_new_outermost_loop_parameters);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Remove the loops not profiled*/
    item = xanList_first(toDelete);
    while (item != NULL) {
        xanList_removeElement(list, item->data, JITTRUE);
        item = item->next;
    }

    /* Free the memory		*/
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);
    xanList_destroyList(toDelete);

    /* Return 			*/
    return list;
}

static inline void internal_allocate_start_times (XanList *list) {
    XanListItem *item;

    /* Assertions			*/
    assert(list != NULL);

    item = xanList_first(list);
    while (item != NULL) {
        loop_profile_t *profile;
        XanStack *startTimesStack;

        /* Fetch the loop			*/
        profile = item->data;
        assert(profile != NULL);

        /* Allocate the stack times		*/
        startTimesStack = xanStack_new(allocFunction, freeFunction, NULL);
        xanHashTable_insert(startTimes, profile, startTimesStack);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Return			*/
    return ;
}

static inline void loop_iterations_new_subloop_iteration (XanList *l) {
    static JITUINT64 ticks = 0;
    JITUINT64 newticks;

    /* Check if we need to profile	*/
    if (l == NULL) {
        return ;
    }

    /* Profile the iteration	*/
    newticks = TIME_getClockTicks();
    assert(newticks > ticks);

    /* Store the ticks		*/
    if (ticks != 0) {
        loop_profile_data_t *data;
        data = allocMemory(sizeof(loop_profile_data_t));
        data->elapsedTicks = (newticks - ticks);
        xanList_append(l, data);
    }
    ticks = newticks;

    /* Return			*/
    return ;
}

static inline void loop_iterations_new_iteration_data_forwarded (void) {
    XanList			*l;
    XanListItem 		*item;
    loop_profile_data_t 	*data;

    /* Check the nesting level	*/
    if (loopExecutionNesting != 1) {

        /* Return			*/
        return ;
    }

    /* This is an iteration of the	*
     * outermost loop.		*
     * Update the global memory	*/
    internal_add_local_memory_to_global_memory();

    /* Erase memory written by the	*
     * past iteration		*/
    internal_erase_local_memory_written();

    /* Fetch the profile data	*/
    assert(outermostLoop != NULL);
    item 			= xanList_last(outermostLoop->iterationsTimes);
    assert(item != NULL);
    l 			= item->data;
    assert(l != NULL);
    item 			= xanList_first(l);
    if (item == NULL) {
        data 	= allocFunction(sizeof(loop_profile_data_t));
        xanList_insert(l, data);
    } else {
        data 	= item->data;
    }
    assert(data != NULL);
    assert(xanList_length(l) == 1);
    profileDataToUse 	= data;

    /* Return			*/
    return ;
}

static inline void loop_time_iterations_new_iteration (void) {
    static JITUINT64 ticks = 0;

    /* Assertions			*/
    assert(outermostLoop != NULL);
    assert(profileDataToUse != NULL);

    /* Check the nesting level	*/
    if (loopExecutionNesting == 1) {
        JITUINT64 newticks;
        newticks = TIME_getClockTicks();
        if (ticks != 0) {
            JITUINT64	deltaTicks;

            /* Compute the delta ticks	*/
            deltaTicks				= (newticks - ticks);

            /* Update the profile		*/
            (profileDataToUse->executionTimes)	++;
            (profileDataToUse->elapsedTicks)	+= deltaTicks;
            (profileDataToUse->elapsedTicksSquare)	+= (deltaTicks * deltaTicks);
        }
        ticks = newticks;
    }

    /* Return			*/
    return ;
}

static inline void loop_iterations_end_subloop_at_nesting_level (void) {

    /* Assertions			*/
    assert(loopExecutionNesting > 0);

    /* Decrease the level of nesting*/
    loopExecutionNesting--;
    xanStack_pop(loopStack);

    /* Return			*/
    return ;
}

static inline void loop_time_iterations_end_loop (void) {

    /* Assertions			*/
    assert(loopExecutionNesting > 0);
    assert(outermostLoop != NULL);
    assert(profileDataToUse != NULL);

    /* Decrease the level of nesting*/
    loopExecutionNesting--;
    if (loopExecutionNesting == 0) {
        JITINT8 fileName[1024];
        FILE *iterations_output;

        /* Dump the profile value	*/
        SNPRINTF(fileName, 1024, "%s_loops_iterations.data", IRPROGRAM_getProgramName());
        iterations_output = fopen((char *)fileName, "a");
        if (iterations_output == NULL) {
            abort();
        }
        fprintf(iterations_output, "%s\n", outermostLoop->loopName);
        fprintf(iterations_output, "%llu %llu %llu\n", profileDataToUse->executionTimes, profileDataToUse->elapsedTicks, profileDataToUse->elapsedTicksSquare);
        fclose(iterations_output);

        /* Erase global pointers	*/
        profileDataToUse = NULL;
        outermostLoop = NULL;
    }

    /* Return			*/
    return ;
}

static inline void loop_iterations_end_loop (void) {

    /* Assertions			*/
    assert(loopExecutionNesting > 0);
    assert(methodVariables != NULL);
    assert(methodVariablesRead != NULL);
    assert(globalMethodVariables != NULL);

    /* Decrease the level of nesting*/
    loopExecutionNesting--;
    if (loopExecutionNesting == 0) {
        freeFunction(methodVariables);
        freeFunction(methodVariablesRead);
        freeFunction(globalMethodVariables);
        methodVariables 	= NULL;
        methodVariablesRead 	= NULL;
        globalMethodVariables 	= NULL;
        methodVariablesNumber 	= 0;
        outermostLoop 		= NULL;
        profileDataToUse 	= NULL;
    }

    /* Return			*/
    return ;
}

static inline void end_total_time_subloop_at_nesting_level_baseline (JITUINT32 nestingLevel) {
    JITUINT64 ticks;
    loop_profile_t *profile;
    loop_profile_data_t *data;

    /* Assertions			*/
    assert(loopExecutionNesting > 0);

    /* Decrease the nesting level	*/
    loopExecutionNesting--;

    /* Check the nesting level	*/
    if (loopExecutionNesting != nestingLevel) {

        /* Return			*/
        return ;
    }

    /* Fetch the profiled loop	*/
    profile = xanStack_pop(loopStack);
    if (profile == NULL) {

        /* Return			*/
        return ;
    }
    assert(profile != NULL);

    /* Get the ticks		*/
    ticks = nestingLevel;

    /* Fetch the data		*/
    if (xanList_length(profile->times) == 0) {
        data = allocMemory(sizeof(loop_profile_data_t));
        assert(data != NULL);
        xanList_append(profile->times, data);
    }
    assert(xanList_length(profile->times) > 0);
    data = xanList_first(profile->times)->data;
    assert(data != NULL);

    /* Fill up the data		*/
    data->nestingLevel = loopExecutionNesting;
    (data->executionTimes)++;
    data->elapsedTicks += (ticks - loopStartTime);
    assert(data->elapsedTicks > 0);

    /* Return			*/
    return ;
}

static inline void end_total_time_subloop_at_nesting_level (JITUINT32 nestingLevel) {
    loop_profile_t *profile;

    /* Assertions			*/
    assert(loopExecutionNesting > 0);

    /* Decrease the nesting level	*/
    loopExecutionNesting--;

    /* Check the nesting level	*/
    if (loopExecutionNesting != nestingLevel) {

        /* Return			*/
        return ;
    }

    /* Fetch the profiled loop	*/
    profile = xanStack_pop(loopStack);
    if (profile == NULL) {

        /* Return			*/
        return ;
    }
    assert(profile != NULL);

    /* Fetch the data		*/
    if (xanList_length(profile->times) == 0) {
        loop_profile_data_t *data;
        data = allocMemory(sizeof(loop_profile_data_t));
        assert(data != NULL);
        xanList_append(profile->times, data);
    }
    internal_update_clock_ticks(profile, JITTRUE);

    /* Return			*/
    return ;
}

static inline void internal_update_clock_ticks (loop_profile_t *profile, JITBOOLEAN updateLoopNestingLevel) {
    JITUINT64 ticks;
    loop_profile_data_t *data;

    /* Assertions			*/
    assert(profile != NULL);
    assert(profile->times != NULL);
    assert(xanList_length(profile->times) > 0);

    /* Get the ticks		*/
    ticks = TIME_getClockTicks();

    /* Fetch the data		*/
    data = xanList_first(profile->times)->data;
    assert(data != NULL);

    /* Fill up the data		*/
    if (updateLoopNestingLevel) {
        data->nestingLevel = loopExecutionNesting;
    }
    (data->executionTimes)++;
    (data->elapsedTicks) += (ticks - loopStartTime);
    assert(data->elapsedTicks > 0);

    /* Return			*/
    return ;
}

static inline void end_subloop_at_nesting_level (JITUINT32 nestingLevel) {
    JITUINT64 ticks;
    JITUINT64 *startTime;
    loop_profile_t *profile;
    XanStack *t;
    loop_profile_data_t *data;

    /* Assertions			*/
    assert(loopExecutionNesting > 0);

    /* Decrease the nesting level	*/
    loopExecutionNesting--;

    /* Check the nesting level	*/
    if (loopExecutionNesting != nestingLevel) {

        /* Return			*/
        return ;
    }

    /* Fetch the profiled loop	*/
    profile = xanStack_pop(loopStack);
    if (profile == NULL) {

        /* Return			*/
        return ;
    }
    assert(profile != NULL);

    /* Get the ticks		*/
    ticks = TIME_getClockTicks();

    /* Pop the current start time	*/
    t = xanHashTable_lookup(startTimes, profile);
    startTime = xanStack_pop(t);
    assert(startTime != NULL);

    /* Allocate the data		*/
    data = allocMemory(sizeof(loop_profile_data_t));
    assert(data != NULL);

    /* Fill up the data		*/
    data->nestingLevel = loopExecutionNesting;
    data->elapsedTicks = ticks - (*startTime);
    assert(data->elapsedTicks > 0);

    /* Keep track of the data	*/
    xanList_append(profile->times, data);

    /* Free the memory		*/
    freeMemory(startTime);

    /* Return			*/
    return ;
}

static inline void new_subloop_at_nesting_level (loop_profile_t *profile, XanList *parentLoops, JITUINT32 nestingLevel) {
    static loop_t *parentLoop = NULL;

    /* Assertions			*/
    assert(profile != 0);
    assert(profile->iterationsTimes != NULL);
    assert(loopStack != NULL);
    assert(parentLoops != NULL);
    assert(xanList_length(parentLoops) > 0);

    /* Check if we are going to	*
     * execute an outermost loop	*/
    if (loopExecutionNesting == 0) {
        parentLoop = profile->loop;
    }
    assert(parentLoop != NULL);

    /* Increase the nesting level	*/
    loopExecutionNesting++;

    /* Check the nesting level	*/
    if ((loopExecutionNesting - 1) != nestingLevel) {

        /* Return			*/
        return ;
    }

    /* Check the loop stack		*/
    if (xanList_find(parentLoops, parentLoop) == NULL) {
        return ;
    }

    /* Add the new subloop to the 	*
     * loop stack			*/
    xanStack_push(loopStack, profile);

    /* Push the current start time	*/
    loopStartTime = TIME_getClockTicks();

    /* Return			*/
    return ;
}

static inline XanList * loop_iterations_new_subloop_at_nesting_level (loop_profile_t *profile, XanList *parentLoops, JITUINT32 nestingLevel) {
    XanList *newList;
    JITUINT32 currentNestingLevel;
    static loop_t *parentLoop = NULL;

    /* Assertions			*/
    assert(profile != 0);
    assert(profile->iterationsTimes != NULL);
    assert(loopStack != NULL);
    assert(parentLoops != NULL);
    assert(xanList_length(parentLoops) > 0);

    /* Check if we are going to	*
     * execute an outermost loop	*/
    if (loopExecutionNesting == 0) {
        parentLoop = profile->loop;
    }
    assert(parentLoop != NULL);

    /* Add the new level of nesting	*/
    currentNestingLevel = loopExecutionNesting;
    loopExecutionNesting++;
    xanStack_push(loopStack, profile);

    /* Check the nesting level	*/
    if (currentNestingLevel != nestingLevel) {
        return NULL;
    }

    /* Check the loop stack		*/
    if (xanList_find(parentLoops, parentLoop) == NULL) {
        return NULL;
    }

    /* Allocate the new list	*/
    newList = xanList_new(allocFunction, freeFunction, NULL);
    assert(newList != NULL);

    /* Append the new list		*/
    xanList_append(profile->iterationsTimes, newList);

    /* Return			*/
    return newList;
}

static inline void loop_time_iterations_new_loop (loop_profile_t *profile) {

    /* Assertions			*/
    assert(profile != 0);
    assert(profile->iterationsTimes != NULL);

    /* Check if we are entering an 	*
     * outermost loop		*/
    if (loopExecutionNesting == 0) {
        XanList	*l;
        XanListItem *item;
        loop_profile_data_t *data;

        /* Allocate the necessary 	*
         * memory			*/
        if (xanList_length(profile->iterationsTimes) == 0) {
            XanList *newList;

            /* Allocate a new list		*/
            newList = xanList_new(allocFunction, freeFunction, NULL);

            /* Append the new list		*/
            xanList_append(profile->iterationsTimes, newList);
        }
        assert(xanList_length(profile->iterationsTimes) == 1);
        item = xanList_first(profile->iterationsTimes);
        assert(item != NULL);
        l = item->data;
        assert(l != NULL);
        if (xanList_length(l) == 0) {
            data = allocFunction(sizeof(loop_profile_data_t));
            assert(data != NULL);
            xanList_insert(l, data);
        }
        assert(xanList_length(l) == 1);
        item = xanList_first(l);
        data = item->data;
        assert(data != NULL);

        /* Erase the memory		*/
        memset(data, 0, sizeof(loop_profile_data_t));

        /* Store some pointers		*/
        outermostLoop = profile;
        profileDataToUse = data;
    }

    /* Add the new level of nesting	*/
    loopExecutionNesting++;

    /* Return			*/
    return ;
}

static inline void loop_iterations_new_loop (loop_profile_t *profile) {
    XanList 	*newList;

    /* Assertions			*/
    assert(storesPerformed != NULL);
    assert(profile != 0);
    assert(profile->iterationsTimes != NULL);

    /* Initialize the variables	*/
    newList		= NULL;

    /* Check if we are entering an 	*
     * outermost loop		*/
    if (loopExecutionNesting == 0) {

        /* Erase the memory written	*/
        internal_erase_memory_written();

        /* Allocate the necessary 	*
         * memory			*/
        if (xanList_length(profile->iterationsTimes) == 0) {

            /* Allocate the new list	*/
            newList = xanList_new(allocFunction, freeFunction, NULL);

            /* Append the new list		*/
            xanList_append(profile->iterationsTimes, newList);

        } else {
            XanListItem *item;

            /* Fetch the previous list	*/
            item = xanList_last(profile->iterationsTimes);
            assert(item != NULL);
            newList = item->data;
        }
        assert(newList != NULL);

        /* Store some pointers		*/
        outermostLoop 		= profile;
        methodVariablesNumber 	= IRMETHOD_getNumberOfVariablesNeededByMethod(profile->loop->method);
        methodVariables 	= allocFunction(sizeof(JITUINT32) * methodVariablesNumber);
        methodVariablesRead 	= allocFunction(sizeof(JITUINT32) * methodVariablesNumber);
        globalMethodVariables 	= allocFunction(sizeof(JITUINT32) * methodVariablesNumber);
    }

    /* Add the new level of nesting	*/
    loopExecutionNesting++;

    /* Return			*/
    return ;
}

static inline void end_outermost_loop_baseline (void) {
    JITUINT64 ticks;
    loop_profile_data_t *data;

    /* Assertions			*/
    assert(outermostLoop != NULL);
    assert(loopExecutionNesting > 0);

    /* Decrease the nesting level	*/
    loopExecutionNesting--;

    /* Check the nesting level	*/
    if (loopExecutionNesting != 0) {

        /* Return			*/
        return ;
    }

    /* Update the overall number of	*
     * clock ticks spent inside the	*
     * loop				*/
    ticks = 0;
    data = xanList_first(outermostLoop->times)->data;

    /* Fill up the data		*/
    (data->executionTimes)++;
    (data->elapsedTicks) += (ticks - loopStartTime);

    /* Return			*/
    return ;
}

static inline void end_outermost_loop (void) {

    /* Assertions			*/
    assert(outermostLoop != NULL);
    assert(loopExecutionNesting > 0);

    /* Decrease the nesting level	*/
    loopExecutionNesting--;

    /* Check the nesting level	*/
    if (loopExecutionNesting != 0) {

        /* Return			*/
        return ;
    }

    /* Update the overall number of	*
     * clock ticks spent inside the	*
     * loop				*/
    internal_update_clock_ticks(outermostLoop, JITFALSE);

    /* Return			*/
    return ;
}

static inline void end_loop (void) {

    /* Assertions			*/
    assert(loopExecutionNesting > 0);

    /* Decrease the nesting level	*/
    loopExecutionNesting--;

    /* Return			*/
    return ;
}

static inline void start_loop (loop_profile_t *profile) {
    XanListItem *item;
    loop_profile_data_t *data;

    /* Try to see if the loop was	*
     * already present at the	*
     * current nesting level	*/
    item = xanList_first(profile->times);
    while (item != NULL) {
        data = item->data;
        if (data->nestingLevel == loopExecutionNesting) {

            /* Increase the frequency of the*
             * level			*/
            (data->executionTimes)++;

            /* Increase the nesting level	*/
            loopExecutionNesting++;

            /* Return			*/
            return ;
        }
        item = item->next;
    }

    /* Fetch the data		*/
    data = allocMemory(sizeof(loop_profile_data_t));
    assert(data != NULL);
    xanList_append(profile->times, data);
    assert(xanList_length(profile->times) > 0);

    /* Fill up the data		*/
    data->nestingLevel = loopExecutionNesting;
    data->executionTimes = 1;

    /* Increase the nesting level	*/
    loopExecutionNesting++;

    return ;
}

static inline JITUINT64 start_new_outermost_loop (loop_profile_t *profile) {
    JITUINT64 ticks;

    /* Check the nesting level	*/
    if (loopExecutionNesting == 0) {

        /* Push the current profile	*/
        outermostLoop = profile;

        /* Fetch the current start time	*/
        loopStartTime = TIME_getClockTicks();
        ticks = loopStartTime;

    } else {

        /* Erase the ticks		*/
        ticks = 0;
    }

    /* Increase the nesting level	*/
    loopExecutionNesting++;

    /* Return			*/
    return ticks;
}

static inline void internal_store_performed (void *start, void *end) {

    /* Assertions			*/
    assert(start != NULL);
    assert(end != NULL);
    assert((((JITNUINT)end) >= ((JITNUINT)start)));

    /* Check if we are executing 	*
     * a loop			*/
    if (loopExecutionNesting == 0) {
        return ;
    }

    /* Check if there will be a 	*
     * store effectively		*/
    if (end == start) {
        return ;
    }

    /* Check if it was already	*
     * written by the current 	*
     * iteration			*/
    if (internal_was_the_memory_written_locally(start, end)) {

        /* Return			*/
        return ;
    }

    /* Add the new memory region	*
     * written			*/
    internal_add_memory_written(&localStoresPerformed, &localStoresPerformedTop, &localStoresPerformedSize, start, end);

    /* Return			*/
    return ;
}

static inline void internal_load_performed (void *start, void *end) {

    /* Assertions			*/
    assert(start != NULL);
    assert(end != NULL);
    assert((((JITNUINT)end) >= ((JITNUINT)start)));
    assert(profileDataToUse != NULL);

    /* Check if we are executing 	*
     * a loop			*/
    if (loopExecutionNesting == 0) {

        /* Return			*/
        return ;
    }

    /* Check if there will be a 	*
     * load effectively		*/
    if (end == start) {

        /* Return			*/
        return ;
    }

    /* Check if the memory was 	*
     * written by the current loop	*
     * iteration.			*
     * In case the data was produced*
     * locally, there is no need	*
     * of data transfer		*/
    if (	(internal_was_the_memory_written_locally(start, end))	||
            (internal_was_the_memory_read_locally(start, end))	) {

        /* Update the profile		*/
        (profileDataToUse->data_read).produced_by_iteration += (((JITNUINT)end) - ((JITNUINT)start));

        /* Return			*/
        return;
    }

    /* Increase the number of bytes	*
     * that need to be forwarded	*
     * from previous loop iterations*/
    (profileDataToUse->data_read).not_produced_by_iteration += (((JITNUINT)end) - ((JITNUINT)start));

    /* Check if these values were 	*
     * produced outside the loop	*/
    if (internal_was_the_memory_written_globally(start, end)) {
        (profileDataToUse->data_read).produced_outside_loop	+= (((JITNUINT)end) - ((JITNUINT)start));
    }

    /* Add the new memory region	*
     * read				*/
    internal_add_memory_written(&localReadsPerformed, &localReadsPerformedTop, &localReadsPerformedSize, start, end);

    /* Return			*/
    return ;
}

static inline JITBOOLEAN internal_was_the_memory_read_locally (void *start, void *end) {
    JITUINT32	count;

    /* Check if the memory was 	*
     * already written		*/
    for (count = 0; count < localReadsPerformedTop; count++) {
        if (	(start >= (localReadsPerformed[count].start))	&&
                (end <= localReadsPerformed[count].end)		) {
            return JITTRUE;
        }
    }

    /* Return			*/
    return JITFALSE;
}

static inline JITBOOLEAN internal_was_the_memory_written_globally (void *start, void *end) {
    return internal_was_the_memory_written(storesPerformed, storesPerformedTop, start, end);
}

static inline JITBOOLEAN internal_was_the_memory_written_locally (void *start, void *end) {
    return internal_was_the_memory_written(localStoresPerformed, localStoresPerformedTop, start, end);
}

static inline JITBOOLEAN internal_was_the_memory_written (memory_t *m, JITUINT32 top, void *start, void *end) {
    JITUINT32	count;

    /* Check if the memory was 	*
     * already written		*/
    for (count = 0; count < top; count++) {
        if (	(start >= (m[count].start))	&&
                (end <= m[count].end)		) {
            return JITTRUE;
        }
    }

    /* Return			*/
    return JITFALSE;
}

static inline void internal_inject_code_to_compute_forwarded_bytes_across_iterations (XanList *loops, ir_method_t *method, XanList *instructions) {
    XanList *insts;
    XanList *inductionOrInvariantVariables;
    XanListItem *item;

    /* Assertions				*/
    assert(method != NULL);

    /* Get the list of induction variables	*/
    inductionOrInvariantVariables = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t *loop;
        loop = item->data;
        if (loop->method == method) {
            JITUINT32 count;
            for (count=0; count < IRMETHOD_getNumberOfVariablesNeededByMethod(loop->method); count++) {
                if (	(LOOPS_isAnInductionVariable(loop, count))	||
                        (LOOPS_isAnInvariantVariable(loop, count))	) {
                    xanList_append(inductionOrInvariantVariables, (void *)(JITNUINT)(count+1));
                }
            }
        }
        item = item->next;
    }

    /* Fetch the instructions to consider	*/
    if (instructions == NULL) {
        insts = IRMETHOD_getInstructions(method);
    } else {
        insts = xanList_cloneList(instructions);
    }
    assert(insts != NULL);

    /* Inject code to compute the number of	*
     * bytes forwarded 			*/
    item = xanList_first(insts);
    while (item != NULL) {
        ir_instruction_t *inst;
        ir_item_t *offsetItem;
        IR_ITEM_VALUE nativeFunction;
        IR_ITEM_VALUE nativeFunctionName;
        ir_item_t *baseAddressItem;
        ir_item_t baseAddressResolvedItem;
        ir_item_t *startItem;
        ir_item_t *endItem;
        ir_item_t *param;
        ir_instruction_t *addInst;
        ir_instruction_t *addInst2;
        ir_instruction_t *callInst;
        JITINT32 bytesToConsider;
        JITUINT32 count;

        /* Fetch the instruction		*/
        inst = item->data;
        assert(inst != NULL);

        /* Inject code to check memory accesses	*/
        switch (inst->type) {
            case IRGETADDRESS:

                /* Escaped variables are initialized 	*
                 * only if they are variables declared	*
                 * by the user				*/
                if (IRMETHOD_isTheVariableAMethodDeclaredVariable(method, IRMETHOD_getInstructionParameter1Value(inst))) {

                    /* Compute the size of the escaped 	*
                     * variable				*/
                    bytesToConsider = IRDATA_getSizeOfType(IRMETHOD_getInstructionParameter1(inst));
                    assert(bytesToConsider > 0);

                    /* Consider the memory written		*/
                    nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_store_performed;
                    nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"store_performed";

                    /* Fetch the end address		*/
                    addInst = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRADD);
                    IRMETHOD_cpInstructionParameter(method, inst, 0, addInst, 1);
                    IRMETHOD_setInstructionParameter2(method, addInst, bytesToConsider, 0, IRUINT32, IRUINT32, NULL);
                    IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                    endItem = IRMETHOD_getInstructionResult(addInst);

                    /* Call the profiler			*/
                    baseAddressItem = IRMETHOD_getInstructionResult(inst);
                    callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRNATIVECALL);
                    IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                    IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
                    IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                    IRMETHOD_addInstructionCallParameter(method, callInst, baseAddressItem);
                    IRMETHOD_addInstructionCallParameter(method, callInst, endItem);
                }
                break;

            case IRALLOC:
            case IRALLOCA:
            case IRALLOCALIGN:

                /* Consider the memory written		*/
                nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_store_performed;
                nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"store_performed";

                /* Fetch the end address		*/
                addInst = IRMETHOD_newInstructionOfTypeAfter(method, inst, IRADD);
                IRMETHOD_cpInstructionParameter(method, inst, 0, addInst, 1);
                IRMETHOD_cpInstructionParameter(method, inst, 1, addInst, 2);
                IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                endItem = IRMETHOD_getInstructionResult(addInst);

                /* Call the profiler			*/
                baseAddressItem = IRMETHOD_getInstructionResult(inst);
                callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRNATIVECALL);
                IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
                IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                IRMETHOD_addInstructionCallParameter(method, callInst, baseAddressItem);
                IRMETHOD_addInstructionCallParameter(method, callInst, endItem);
                break;

            case IRMEMCPY:

                /* Consider the memory written		*/
                nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_load_performed;
                nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"load_performed";

                /* Fetch the start address		*/
                baseAddressItem = IRMETHOD_getInstructionParameter2(inst);
                if (baseAddressItem->type == IRSYMBOL) {
                    IRSYMBOL_resolveSymbolFromIRItem(baseAddressItem, &baseAddressResolvedItem);
                } else {
                    memcpy(&baseAddressResolvedItem, baseAddressItem, sizeof(ir_item_t));
                }

                /* Fetch the end address		*/
                addInst = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
                IRMETHOD_cpInstructionParameter1(addInst, &baseAddressResolvedItem);
                IRMETHOD_cpInstructionParameter(method, inst, 3, addInst, 2);
                IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                endItem = IRMETHOD_getInstructionResult(addInst);

                /* Call the profiler			*/
                callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRNATIVECALL);
                IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                IRMETHOD_setInstructionParameter2(method, callInst, 0, 0.0, IRSTRING, IRSTRING, NULL);
                IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                IRMETHOD_addInstructionCallParameter(method, callInst, &baseAddressResolvedItem);
                IRMETHOD_addInstructionCallParameter(method, callInst, endItem);

                /* Consider the memory read		*/
                nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_store_performed;

                /* Fetch the start address		*/
                baseAddressItem = IRMETHOD_getInstructionParameter1(inst);
                if (baseAddressItem->type == IRSYMBOL) {
                    IRSYMBOL_resolveSymbolFromIRItem(baseAddressItem, &baseAddressResolvedItem);
                } else {
                    memcpy(&baseAddressResolvedItem, baseAddressItem, sizeof(ir_item_t));
                }

                /* Fetch the end address		*/
                addInst = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
                IRMETHOD_cpInstructionParameter1(addInst, &baseAddressResolvedItem);
                IRMETHOD_cpInstructionParameter(method, inst, 3, addInst, 2);
                IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                endItem = IRMETHOD_getInstructionResult(addInst);

                /* Call the profiler			*/
                callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRNATIVECALL);
                IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
                IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                IRMETHOD_addInstructionCallParameter(method, callInst, &baseAddressResolvedItem);
                IRMETHOD_addInstructionCallParameter(method, callInst, endItem);
                break;

            case IRINITMEMORY:

                /* Fetch the function pointer		*/
                nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_store_performed;
                nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"store_performed";

                /* Fetch the start address		*/
                baseAddressItem = IRMETHOD_getInstructionParameter1(inst);
                if (baseAddressItem->type == IRSYMBOL) {
                    IRSYMBOL_resolveSymbolFromIRItem(baseAddressItem, &baseAddressResolvedItem);
                } else {
                    memcpy(&baseAddressResolvedItem, baseAddressItem, sizeof(ir_item_t));
                }

                /* Fetch the end address		*/
                addInst = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
                IRMETHOD_cpInstructionParameter1(addInst, &baseAddressResolvedItem);
                IRMETHOD_cpInstructionParameter(method, inst, 3, addInst, 2);
                IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                endItem = IRMETHOD_getInstructionResult(addInst);

                /* Call the profiler			*/
                callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRNATIVECALL);
                IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
                IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                IRMETHOD_addInstructionCallParameter(method, callInst, &baseAddressResolvedItem);
                IRMETHOD_addInstructionCallParameter(method, callInst, endItem);
                break;

            case IRSTOREREL:
            case IRLOADREL:

                /* If the offset is negative, then this	*
                 * memory operation read/write to an	*
                 * object header			*/
                offsetItem = IRMETHOD_getInstructionParameter2(inst);
                assert(offsetItem->type != IROFFSET);
                assert(offsetItem->type != IRSYMBOL);
                if ((offsetItem->value).v >= 0) {
                    if (inst->type == IRSTOREREL) {
                        nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_store_performed;
                        nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"store_performed";
                    } else {
                        nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_load_performed;
                        nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"load_performed";
                    }

                    /* Fetch the start address		*/
                    baseAddressItem = IRMETHOD_getInstructionParameter1(inst);
                    if (baseAddressItem->type == IRSYMBOL) {
                        IRSYMBOL_resolveSymbolFromIRItem(baseAddressItem, &baseAddressResolvedItem);
                    } else {
                        memcpy(&baseAddressResolvedItem, baseAddressItem, sizeof(ir_item_t));
                    }
                    addInst = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
                    IRMETHOD_cpInstructionParameter1(addInst, &baseAddressResolvedItem);
                    IRMETHOD_cpInstructionParameter(method, inst, 2, addInst, 2);
                    IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                    startItem = IRMETHOD_getInstructionResult(addInst);

                    /* Fetch the end address		*/
                    bytesToConsider = IRDATA_getSizeOfType(IRMETHOD_getInstructionParameter3(inst));
                    assert(bytesToConsider > 0);
                    addInst2 = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRADD);
                    IRMETHOD_cpInstructionParameter1(addInst2, startItem);
                    IRMETHOD_setInstructionParameter2(method, addInst2, bytesToConsider, 0, IRNUINT, IRNUINT, NULL);
                    IRMETHOD_setInstructionParameterWithANewVariable(method, addInst2, IRNUINT, NULL, 0);
                    endItem = IRMETHOD_getInstructionResult(addInst2);

                    /* Call the profiler			*/
                    callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst2, IRNATIVECALL);
                    IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                    IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
                    IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                    IRMETHOD_addInstructionCallParameter(method, callInst, startItem);
                    IRMETHOD_addInstructionCallParameter(method, callInst, endItem);
                }
                break;

            case IRCHECKNULL:
                nativeFunction = (IR_ITEM_VALUE)(JITNUINT)internal_load_performed;
                nativeFunctionName = (IR_ITEM_VALUE)(JITNUINT)"load_performed";

                /* Fetch the start and end address	*/
                baseAddressItem = IRMETHOD_getInstructionParameter1(inst);
                if (baseAddressItem->type == IRSYMBOL) {
                    IRSYMBOL_resolveSymbolFromIRItem(baseAddressItem, &baseAddressResolvedItem);
                } else {
                    memcpy(&baseAddressResolvedItem, baseAddressItem, sizeof(ir_item_t));
                }
                addInst = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
                IRMETHOD_cpInstructionParameter1(addInst, &baseAddressResolvedItem);
                IRMETHOD_setInstructionParameter2(method, addInst, sizeof(JITNUINT), 0.0, baseAddressResolvedItem.internal_type, baseAddressResolvedItem.internal_type, NULL);
                IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, IRNUINT, NULL, 0);
                startItem = IRMETHOD_getInstructionParameter1(addInst);
                endItem = IRMETHOD_getInstructionResult(addInst);

                /* Call the profiler			*/
                callInst = IRMETHOD_newInstructionOfTypeAfter(method, addInst, IRNATIVECALL);
                IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
                IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
                IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
                IRMETHOD_addInstructionCallParameter(method, callInst, startItem);
                IRMETHOD_addInstructionCallParameter(method, callInst, endItem);
                break ;
            case IRNEWOBJ:
            case IRNEWARR:
                //TODO
                break ;
        }

        /* Inject code to check variable accesses	*/
        switch (IRMETHOD_getInstructionParametersNumber(inst)) {
            case 3:
                param = IRMETHOD_getInstructionParameter(inst, 3);
                internal_inject_code_for_variable_accesses(method, inductionOrInvariantVariables, inst, param, (IR_ITEM_VALUE)(JITNUINT)internal_read_variable, (IR_ITEM_VALUE)(JITNUINT)"read_variable");
            case 2:
                param = IRMETHOD_getInstructionParameter(inst, 2);
                internal_inject_code_for_variable_accesses(method, inductionOrInvariantVariables, inst, param, (IR_ITEM_VALUE)(JITNUINT)internal_read_variable, (IR_ITEM_VALUE)(JITNUINT)"read_variable");
            case 1:
                param = IRMETHOD_getInstructionParameter(inst, 1);
                internal_inject_code_for_variable_accesses(method, inductionOrInvariantVariables, inst, param, (IR_ITEM_VALUE)(JITNUINT)internal_read_variable, (IR_ITEM_VALUE)(JITNUINT)"read_variable");
        }
        for (count=0; count < IRMETHOD_getInstructionCallParametersNumber(inst); count++) {
            param = IRMETHOD_getInstructionCallParameter(inst, count);
            internal_inject_code_for_variable_accesses(method, inductionOrInvariantVariables, inst, param, (IR_ITEM_VALUE)(JITNUINT)internal_read_variable, (IR_ITEM_VALUE)(JITNUINT)"read_variable");
        }
        param = IRMETHOD_getInstructionResult(inst);
        if (param->type == IROFFSET) {
            assert(inst->type != IRLABEL);
            internal_inject_code_for_variable_accesses(method, inductionOrInvariantVariables, inst, param, (IR_ITEM_VALUE)(JITNUINT)internal_write_variable, (IR_ITEM_VALUE)(JITNUINT)"write_variable");
        }

        /* Fetch the next element from the list		*/
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(insts);
    xanList_destroyList(inductionOrInvariantVariables);

    /* Return				*/
    return ;
}

static inline void internal_write_variable (ir_method_t *method, JITUINT32 varID, JITUINT32 bytes, JITUINT32 locallyProduced) {

    /* Check if we are executing 	*
     * a loop			*/
    if (loopExecutionNesting == 0) {

        /* Return			*/
        return ;
    }

    /* Check if the method we are 	*
     * executing is the one that 	*
     * contains the loop		*/
    if (method != outermostLoop->loop->method) {

        /* Return			*/
        return ;
    }
    assert(method == outermostLoop->loop->method);

    /* Set the variable as written	*/
    methodVariables[varID] = JITTRUE;
    globalMethodVariables[varID] = JITTRUE;

    /* Return			*/
    return ;
}

static inline void internal_read_variable (ir_method_t *method, JITUINT32 varID, JITUINT32 bytes, JITUINT32 locallyProduced) {

    /* Assertions			*/
    assert(profileDataToUse != NULL);

    /* Check if we are executing 	*
     * a loop			*/
    if (loopExecutionNesting == 0) {

        /* Return			*/
        return ;
    }

    /* Check if the method we are 	*
     * executing is the one that 	*
     * contains the loop		*/
    if (method != outermostLoop->loop->method) {

        /* The value has not to be 	*
         * forwarded			*/
        (profileDataToUse->data_read).produced_by_iteration += bytes;

        /* Return			*/
        return ;
    }
    assert(method == outermostLoop->loop->method);

    /* Update the profile		*/
    if (	(locallyProduced) 			||
            (methodVariables[varID]) 		||
            (methodVariablesRead[varID])		) {

        /* The value has not to be forwarded	*/
        (profileDataToUse->data_read).produced_by_iteration 	+= bytes;

    } else {

        /* The value has to be forwarded	*/
        (profileDataToUse->data_read).not_produced_by_iteration 	+= bytes;
        methodVariablesRead[varID]			= JITTRUE;

        /* Check if the value was set outside 	*
         * the loop				*/
        if (globalMethodVariables[varID] == JITFALSE) {
            (profileDataToUse->data_read).produced_outside_loop 	+= bytes;
        }
    }

    /* Return			*/
    return ;
}

static inline void internal_erase_memory_written (void) {

    /* Erase the memory			*/
    memset(storesPerformed, 0, sizeof(memory_t) * storesPerformedSize);
    storesPerformedTop = 0;

    /* Return				*/
    return ;
}

static inline void internal_erase_local_memory_written (void) {

    /* Erase the memory			*/
    memset(methodVariables, 0, sizeof(JITUINT32) * methodVariablesNumber);
    memset(methodVariablesRead, 0, sizeof(JITUINT32) * methodVariablesNumber);
    memset(localStoresPerformed, 0, sizeof(memory_t) * localStoresPerformedSize);
    memset(localReadsPerformed, 0, sizeof(memory_t) * localReadsPerformedSize);
    localStoresPerformedTop = 0;
    localReadsPerformedTop = 0;

    /* Return				*/
    return ;
}

static inline void internal_add_local_memory_to_global_memory (void) {
    JITUINT32	count;

    /* Add the memory			*/
    for (count = 0; count < localStoresPerformedTop; count++) {
        internal_add_memory_written(&storesPerformed, &storesPerformedTop, &storesPerformedSize, localStoresPerformed[count].start, localStoresPerformed[count].end);
    }

    /* Return				*/
    return ;
}

static inline void internal_add_memory_written (memory_t **memory, JITUINT32 *top, JITUINT32 *size, void *start, void *end) {
    JITUINT32	count;
    JITBOOLEAN	found;

    /* Assertions				*/
    assert(memory != NULL);
    assert((*memory) != NULL);
    assert(top != NULL);
    assert(size != NULL);
    assert(start != NULL);
    assert(end != NULL);
#ifdef DEBUG
    for (count = 0; count < (*top); count++) {
        assert((*memory)[count].start != NULL);
        assert((*memory)[count].end != NULL);
    }
#endif

    /* Initialize the variables		*/
    found = JITFALSE;

    /* Check that there is enough space	*/
    if ((*top) == (*size)) {
        (*size) 	+= 10;
        (*memory)		= dynamicReallocFunction((*memory), sizeof(memory_t) * (*size));
    }
    assert((*top) < (*size));

    /* Add the memory written		*/
    for (count = 0; count < (*top); count++) {

        /* Check if we need to insert the	*
         * element				*/
        if (start < ((*memory)[count].start)) {

            /* The element has to be inserted 	*
             * before the current one		*/
            if ((*top) > 0) {
                memmove(&((*memory)[1]), &((*memory)[0]), sizeof(memory_t) * (*top));
            }
            (*memory)[0].start = start;
            (*memory)[0].end = end;
            (*top)++;

            /* The memory has merged		*/
            found = JITTRUE;
            break ;
        }
        assert(start >= ((*memory)[count].start));

        /* Check if the current element is 	*
         * fully included within the current one*/
        if (end <= ((*memory)[count].end)) {

            /* The element is included		*/
            assert(start <= ((*memory)[count].end));
            found = JITTRUE;
            break ;
        }
        assert(start >= ((*memory)[count].start));
        assert(end > ((*memory)[count].end));

        /* Check if there is an overlap with	*
         * the current element			*/
        if (start <= ((*memory)[count].end)) {

            /* The element has to be merged with	*
             * the current one			*/
            assert(start >= ((*memory)[count].start));
            assert(end > ((*memory)[count].end));
            (*memory)[count].end	= end;

            /* The memory has merged		*/
            found = JITTRUE;
            break ;
        }
        assert(start >= ((*memory)[count].start));
        assert(start > ((*memory)[count].end));
        assert(end > ((*memory)[count].end));
    }
    if (!found) {

        /* The element has to be inserted at 	*
         * the end of the list			*/
        (*memory)[*top].start = start;
        (*memory)[*top].end = end;
        (*top)++;
    }

    /* Check the memory			*/
#ifdef DEBUG
    for (count = 0; count < (*top); count++) {
        assert((*memory)[count].start != NULL);
        assert((*memory)[count].end != NULL);
    }
#endif

    /* Return				*/
    return ;
}

static inline void internal_inject_code_for_variable_accesses (ir_method_t *method, XanList *inductionOrInvariantVariables, ir_instruction_t *beforeInst, ir_item_t *param, IR_ITEM_VALUE nativeFunction, IR_ITEM_VALUE nativeFunctionName) {
    ir_instruction_t *callInst;
    ir_item_t callParam;
    ir_item_t callParam1;
    ir_item_t callParam2;
    ir_item_t callParam3;

    /* Check the parameter		*/
    if (param->type != IROFFSET) {

        /* Return			*/
        return ;
    }

    /* Initialize the variables	*/
    memset(&callParam, 0, sizeof(ir_item_t));
    memset(&callParam1, 0, sizeof(ir_item_t));
    memset(&callParam2, 0, sizeof(ir_item_t));
    memset(&callParam3, 0, sizeof(ir_item_t));

    /* Prepare the call parameters	*/
    callParam.value.v = param->value.v;
    callParam.type = IRUINT32;
    callParam.internal_type = IRUINT32;

    callParam1.value.v = (IR_ITEM_VALUE)(JITNUINT) method;
    callParam1.type = IRNUINT;
    callParam1.internal_type = IRNUINT;

    callParam2.value.v = IRDATA_getSizeOfType(param);
    callParam2.type = IRUINT32;
    callParam2.internal_type = IRUINT32;

    callParam3.value.v = 0;
    if (xanList_find(inductionOrInvariantVariables, (void *)(JITNUINT)(param->value.v + 1)) != NULL) {
        callParam3.value.v = 1;
    }
    callParam3.type = IRUINT32;
    callParam3.internal_type = IRUINT32;

    /* Inject the code		*/
    callInst = IRMETHOD_newInstructionOfTypeBefore(method, beforeInst, IRNATIVECALL);
    IRMETHOD_setInstructionParameter1(method, callInst, IRVOID, 0.0, IRTYPE, IRTYPE, NULL);
    IRMETHOD_setInstructionParameter2(method, callInst, nativeFunctionName, 0.0, IRSTRING, IRSTRING, NULL);
    IRMETHOD_setInstructionParameter3(method, callInst, nativeFunction, 0.0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
    IRMETHOD_addInstructionCallParameter(method, callInst, &callParam1);
    IRMETHOD_addInstructionCallParameter(method, callInst, &callParam);
    IRMETHOD_addInstructionCallParameter(method, callInst, &callParam2);
    IRMETHOD_addInstructionCallParameter(method, callInst, &callParam3);

    /* Return			*/
    return ;
}

void LOOPS_dumpLoopsInvocationsForestTreesDotFile (XanGraph *g, XanHashTable *loopsNames, FILE *fileToWrite, JITBOOLEAN dumpEdges, void (*dumpNodeAnnotations) (loop_invocations_information_t *node, FILE *fileToWrite)) {
    XanHashTableItem	*item;
    XanHashTable            *loopsIDs;
    JITUINT32		count;

    /* Assertions			*/
    assert(g != NULL);
    assert(loopsNames != NULL);
    assert(fileToWrite != NULL);

    /* Allocate the necessary memory*/
    loopsIDs 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Dump the header		*/
    fprintf(fileToWrite, "digraph \"%s\" {\n", IRPROGRAM_getProgramName());
    fprintf(fileToWrite, "label=\"Loops invocations forest trees\"\n");

    /* Dump the nodes		*/
    count	= 1;
    item	= xanHashTable_first(g->nodes);
    while (item != NULL) {
        XanGraphNode			*n;
        loop_invocations_information_t 	*l;

        /* Fetch the node		*/
        n	= item->element;
        assert(n != NULL);
        l	= n->data;
        assert(l != NULL);

        /* Check if the loop has been	*
         * executed			*/
        if (l->minInvocationsNumber != -1) {

            /* Dump the loop		*/
            fprintf(fileToWrite, "node [ fontsize=12 label=\"");
            fprintf(fileToWrite, "%s\\n", (char *)xanHashTable_lookup(loopsNames, l->loop->loop));
            fprintf(fileToWrite, "%llu - %llu", l->minInvocationsNumber, l->maxInvocationsNumber);
            if (dumpNodeAnnotations != NULL) {
                (*dumpNodeAnnotations)(l, fileToWrite);
            }
            fprintf(fileToWrite, "\" shape=box");
            fprintf(fileToWrite, " ] ;\n");
            fprintf(fileToWrite, "n%u ;\n", count);
            xanHashTable_insert(loopsIDs, n, (void *) (JITNUINT)(count));
            count++;
        }

        /* Fetch the next element	*/
        item	= xanHashTable_next(g->nodes, item);
    }

    /* Dump the edges		*/
    item	= xanHashTable_first(g->nodes);
    while (item != NULL) {
        XanGraphNode			*n;
        XanHashTableItem		*item2;
        loop_invocations_information_t 	*l;

        /* Fetch the node		*/
        n	= item->element;
        assert(n != NULL);
        l	= n->data;
        assert(l != NULL);

        /* Check if the loop has been	*
         * executed			*/
        if (l->minInvocationsNumber != -1) {

            /* Dump the outgoing edges	*/
            item2	= xanHashTable_first(n->outgoingEdges);
            while (item2 != NULL) {
                XanGraphEdge			*edge;
                XanGraphNode			*dst;
                loop_invocations_t		*inv;
                JITUINT32			count2;

                /* Fetch the edge		*/
                edge	= item2->element;
                assert(edge != NULL);
                assert(edge->p1 != edge->p2);
                inv	= edge->data;
                assert(inv != NULL);

                /* Fetch the destination	*/
                if (edge->p1 != n) {
                    dst	= edge->p1;
                } else {
                    dst	= edge->p2;
                }
                assert(dst != NULL);
                assert(dst != n);

                /* Dump the edge		*/
                count	= (JITUINT32) (JITNUINT)xanHashTable_lookup(loopsIDs, n);
                count2	= (JITUINT32) (JITNUINT)xanHashTable_lookup(loopsIDs, dst);
                assert(count > 0);
                assert(count2 > 0);
                fprintf(fileToWrite, "n%u -> n%u [ label=\"", count, count2);
                if (dumpEdges) {
                    for (JITUINT32 count=0; count < inv->invocationsTop; count++) {
                        JITUINT64	startInvocation;
                        JITUINT64	endInvocation;
                        assert((inv->invocations[count]).adjacentInvocationsNumber > 0);
                        endInvocation	= (inv->invocations[count]).startInvocation;
                        endInvocation	+= (inv->invocations[count]).adjacentInvocationsNumber - 1;
                        startInvocation	= (inv->invocations[count]).startInvocation;
                        fprintf(fileToWrite, "%llu - %llu\\n", startInvocation, endInvocation);
                    }
                }
                fprintf(fileToWrite, "\"] ;\n");

                /* Fetch the next element	*/
                item2	= xanHashTable_next(n->outgoingEdges, item2);
            }
        }

        /* Fetch the next element	*/
        item	= xanHashTable_next(g->nodes, item);
    }

    /* Print the foot of the graph  */
    fprintf(fileToWrite, "}\n");

    /* Flush the output channel     */
    fflush(fileToWrite);

    /* Free the memory              */
    xanHashTable_destroyTable(loopsIDs);

    /* Return			*/
    return ;
}

XanGraph * LOOPS_loadLoopsInvocationsForestTrees (XanHashTable *loopsNames, JITINT8 *prefixName, XanHashTable *loops) {
    XanGraph		*g;
    JITINT8			buf[DIM_BUF];
    JITINT8			*fileName;
    JITUINT32		fileNameSize;
    FILE			*summaryFile;

    /* Assertions			*/
    assert(loopsNames != NULL);
    assert(prefixName != NULL);
    assert(loops != NULL);

    /* Allocate the graph		*/
    g		= xanGraph_new(allocFunction, freeFunction, dynamicReallocFunction, NULL);
    assert(g != NULL);

    /* Open the summary file	*/
    SNPRINTF(buf, DIM_BUF, "%s_files", prefixName);
    summaryFile	= fopen((char *)buf, "r");
    if (summaryFile == NULL) {
        fprintf(stderr, "The file %s is missing. The LIF cannot be loaded.\n", prefixName);
        abort();
    }

    /* Load the graph		*/
    fileNameSize	= 100;
    fileName	= allocFunction(fileNameSize);
    while (JITTRUE) {
        FILE			*fileToRead;

        /* Fetch the next name		*/
        if (internal_fetch_name(&fileName, summaryFile, &fileNameSize, '\0')) {
            break;
        }

        /* Open the file		*/
        fileToRead	= fopen((char *)fileName, "r");
        if (fileToRead == NULL) {
            fprintf(stderr, "The file %s is missing. The LIF cannot be loaded.\n", fileName);
            abort();
        }

        /* Parse the file		*/
        loops_invocationsin	= fileToRead;
        loops_invocationslex(g, loops, loopsNames);

        /* Close the file		*/
        fclose(fileToRead);
    }

    /* Close the files		*/
    fclose(summaryFile);

    /* Free the memory		*/
    freeFunction(fileName);

    /* Return			*/
    return g;
}

void LOOPS_dumpLoopsInvocationsForestTrees (XanGraph *g, XanHashTable *loopsNames, JITINT8 *prefixName) {
    XanHashTableItem	*item;
    JITUINT32		suffixNumber;
    JITUINT32		bytesDumped;
    JITINT8			buf[DIM_BUF];
    FILE 			*fileToWrite;
    FILE			*summaryFile;

    /* Assertions			*/
    assert(g != NULL);
    assert(loopsNames != NULL);
    assert(prefixName != NULL);

    /* Initialize the variables	*/
    suffixNumber	= 0;
    bytesDumped	= 0;

    /* Open the files		*/
    SNPRINTF(buf, DIM_BUF, "%s_files", prefixName);
    summaryFile	= fopen((char *)buf, "w+");
    if (summaryFile == NULL) {
        abort();
    }
    SNPRINTF(buf, DIM_BUF, "%s_%u", prefixName, suffixNumber);
    suffixNumber++;
    fileToWrite	= fopen((char *)buf, "w+");
    if (fileToWrite == NULL) {
        abort();
    }
    fprintf(summaryFile, "%s\n", buf);

    /* Dump the graphs		*/
    item	= xanHashTable_first(g->nodes);
    while (item != NULL) {
        XanGraphNode			*n;
        XanHashTableItem		*item2;
        loop_invocations_information_t 	*l;

        /* Fetch the node		*/
        n	= item->element;
        assert(n != NULL);
        l	= n->data;
        assert(l != NULL);

        /* Check if the loop has been	*
         * executed			*/
        if (l->minInvocationsNumber != -1) {

            /* Dump the loop		*/
            SNPRINTF(buf, DIM_BUF, "Node\n");
            MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITTRUE);

            SNPRINTF(buf, DIM_BUF, "%s\n", (char *)xanHashTable_lookup(loopsNames, l->loop->loop));
            MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITFALSE);

            SNPRINTF(buf, DIM_BUF, "%llu\n", l->minInvocationsNumber);
            MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITFALSE);

            SNPRINTF(buf, DIM_BUF, "%llu\n", l->maxInvocationsNumber);
            MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITFALSE);

            /* Dump the outgoing edges	*/
            item2	= xanHashTable_first(n->outgoingEdges);
            while (item2 != NULL) {
                XanGraphEdge			*edge;
                XanGraphNode			*dst;
                loop_invocations_information_t 	*dstL;
                loop_invocations_t		*inv;

                /* Fetch the edge		*/
                edge	= item2->element;
                assert(edge != NULL);
                assert(edge->p1 != edge->p2);
                inv	= edge->data;
                assert(inv != NULL);

                /* Fetch the destination	*/
                if (edge->p1 != n) {
                    dst	= edge->p1;
                } else {
                    dst	= edge->p2;
                }
                assert(dst != NULL);
                assert(dst != n);
                dstL	= dst->data;
                assert(dstL != NULL);

                /* Dump the edge		*/
                SNPRINTF(buf, DIM_BUF, "Edge\n");
                MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITFALSE);

                SNPRINTF(buf, DIM_BUF, "%s\n", (char *)xanHashTable_lookup(loopsNames, dstL->loop->loop));
                MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITFALSE);

                SNPRINTF(buf, DIM_BUF, "%llu\n", dstL->minInvocationsNumber);
                MISC_dumpString(&fileToWrite, summaryFile, buf, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITFALSE);

                /* Append files			*/
                if (inv->summaryFile != NULL) {
                    JITUINT32	fileNameSize;
                    JITINT8		*fileName;

                    /* Close the last file		*/
                    fclose(inv->fileToWrite);
                    inv->fileToWrite	= NULL;

                    /* Free the memory		*/
                    assert(inv->prefixName != NULL);
                    freeFunction(inv->prefixName);
                    inv->prefixName		= NULL;

                    /* Seek to the beginning of the	*
                     * file				*/
                    fseek(inv->summaryFile, 0, SEEK_SET);

                    /* Load the graph		*/
                    fileNameSize	= 100;
                    fileName	= allocFunction(fileNameSize);
                    while (JITTRUE) {
                        FILE			*fileToRead;

                        /* Fetch the next name		*/
                        if (internal_fetch_name(&fileName, inv->summaryFile, &fileNameSize, '\0')) {
                            break;
                        }


                        /* Open the file		*/
                        fileToRead	= fopen((char *)fileName, "r");
                        if (fileToRead == NULL) {
                            abort();
                        }

                        /* Append the file		*/
                        while (!internal_fetch_name(&fileName, fileToRead, &fileNameSize, '\n')) {
                            MISC_dumpString(&fileToWrite, summaryFile, fileName, &bytesDumped, &suffixNumber, prefixName, JITMAXINT32 / 2, JITTRUE);
                        }

                        /* Close the file		*/
                        fclose(fileToRead);
                    }

                    /* Close the files		*/
                    fclose(inv->summaryFile);
                    inv->summaryFile	= NULL;

                    /* Free the memory		*/
                    freeFunction(fileName);
                }

                /* Dump the edge values		*/
                internal_dumpEdgeInformation(inv, &fileToWrite, summaryFile, &bytesDumped, &suffixNumber, prefixName, JITTRUE);

                /* Fetch the next element	*/
                item2	= xanHashTable_next(n->outgoingEdges, item2);
            }
        }

        /* Fetch the next element	*/
        item	= xanHashTable_next(g->nodes, item);
    }

    /* Close the file		*/
    fclose(summaryFile);
    fclose(fileToWrite);

    /* Return			*/
    return ;
}

XanList * LOOPS_profileLoopBoundaries (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, void (*newLoop) (loop_profile_t *loop), void (*endLoop) (void), void (*newIteration)(void), void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst)) {
    return instrument_loops(irOptimizer, loops, noSystemLibraries, loopsNames, (void(*)(void*))newLoop, endLoop, newIteration, addCodeFunction, internal_fetchLoopProfile);
}

XanGraph * LOOPS_profileLoopsInvocationsForestTrees (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames) {
    XanList			*p;
    XanList			*alreadyOptimized;
    XanListItem		*item;
    XanHashTable 		*loopsSuccessors;
    XanHashTable 		*loopsPredecessors;
    void 			*newLoopfunctionPointer;
    void 			*endLoopFunctionPointer;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);

    /* Allocate the graph		*/
    g		= xanGraph_new(allocFunction, freeFunction, dynamicReallocFunction, NULL);
    assert(g != NULL);

    /* Allocate the necessary 	*
     * memories			*/
    loopStack 	= xanStack_new (allocFunction, freeFunction, NULL);
    assert(loopStack != NULL);

    /* Fill up the graph		*/
    p		= get_profiled_loops(loops, loopsNames);
    assert(p != NULL);

    /* Compute the successors of	*
     * the loops to profile		*/
    loopsSuccessors 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Set the function pointers	*/
    newLoopfunctionPointer 		= loop_invocation_profiler_new_loop;
    endLoopFunctionPointer 		= loop_invocation_profiler_end_loop;

    /* Profile the loops		*/
    item = xanList_first(p);
    while (item != NULL) {
        loop_profile_t			*profile;
        XanList 			*start_new_outermost_loop_parameters;
        ir_item_t 			currentItem;

        /* Fetch the loop to profile		*/
        profile		= item->data;
        assert(profile != NULL);

        /* Check if we should consider the 	*
         * current loop				*/
        if (	(noSystemLibraries == JITFALSE)					||
                (!IRPROGRAM_doesMethodBelongToALibrary(profile->loop->method))	) {
            XanList *successors;
            XanList *predecessors;

            /* Fetch the successors of the loop	*/
            successors = xanHashTable_lookup(loopsSuccessors, profile->loop);
            assert(successors != NULL);

            /* Fetch the predecessors of the loop	*
             * header				*/
            predecessors = xanHashTable_lookup(loopsPredecessors, profile->loop);
            assert(predecessors != NULL);

            /* Create the parameters		*/
            start_new_outermost_loop_parameters 	= xanList_new(allocFunction, freeFunction, NULL);
            memset(&currentItem, 0, sizeof(ir_item_t));
            currentItem.value.v 			= (IR_ITEM_VALUE)(JITNUINT) profile;
            currentItem.type 			= IRNUINT;
            currentItem.internal_type 		= IRNUINT;
            xanList_append(start_new_outermost_loop_parameters, &currentItem);

            /* Profile the loop			*/
            profileLoop	 (	irOptimizer,
                            profile,
                            newLoopfunctionPointer, NULL, start_new_outermost_loop_parameters,
                            endLoopFunctionPointer, NULL, NULL,
                            NULL, NULL, NULL,
                            successors, predecessors, NULL);

            /* Free the memory			*/
            xanList_destroyList(start_new_outermost_loop_parameters);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Optimize the code		*/
    alreadyOptimized	= xanList_new(allocFunction, freeFunction, NULL);
    item 			= xanList_first(p);
    while (item != NULL) {
        loop_profile_t			*profile;

        /* Fetch the loop 		*/
        profile		= item->data;
        assert(profile != NULL);

        /* Optimize the method that	*
         * contains the loop		*/
        if (xanList_find(alreadyOptimized, profile->loop->method) == NULL) {
            IROPTIMIZER_optimizeMethod(irOptimizer, profile->loop->method);
            xanList_insert(alreadyOptimized, profile->loop->method);
        }

        /* Fetch the next element 	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);
    xanList_destroyList(alreadyOptimized);

    /* Return			*/
    return g;
}

static inline void loop_invocation_external_profiler_end_loop (void) {

    /* Assertions			*/
    assert(xanStack_getSize(loopStack) > 0);

    /* Pop the loop from the stack	*/
    xanStack_pop(loopStack);

    /* Call the external function	*/
    if (exitLoopFunction != NULL) {
        (*exitLoopFunction)();
    }

    /* Return			*/
    return ;
}

static inline void loop_invocation_external_profiler_new_loop (loop_profile_t *loop) {

    /* Assertions			*/
    assert(loop != NULL);

    /* Add the new loop to the 	*
     * loop stack			*/
    xanStack_push(loopStack, loop);

    /* Call the external function	*/
    if (enterLoopFunction != NULL) {
        (*enterLoopFunction)(loop);
    }

    /* Update the invocation number	*/
    (loop->invocationsNumber)++;

    /* Return			*/
    return ;
}

static inline void loop_invocation_profiler_new_loop (loop_profile_t *loop) {
    XanGraphNode			*dst;
    loop_invocations_information_t 	*dstData;

    /* Assertions			*/
    assert(loop != NULL);

    /* Check if the current node is	*
     * a root			*/
    if (xanStack_getSize(loopStack) > 0) {
        XanGraphNode			*src;
        XanGraphEdge			*e;
        loop_invocations_t		*inv;
        loop_invocations_information_t	*srcData;

        /* Fetch the current parent loop*
         * in execution			*/
        src	= xanStack_top(loopStack);
        assert(src != NULL);
        srcData	= src->data;
        assert(srcData != NULL);
        assert(loop->loopName != NULL);

        /* Fetch the node of the graph	*
         * to use for the current loop	*/
        assert(srcData->subloops != NULL);
        dst	= xanHashTable_lookup(srcData->subloops, loop);
        if (dst == NULL) {
            loop_invocations_information_t 	*n;

            /* Create a new node			*/
            n				= allocFunction(sizeof(loop_invocations_information_t));
            n->loop				= loop;
            n->minInvocationsNumber		= loop->invocationsNumber;
            n->maxInvocationsNumber		= loop->invocationsNumber;
            n->subloops			= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
            dst				= xanGraph_addANewNode(g, n);

            /* Update the table			*/
            xanHashTable_insert(srcData->subloops, loop, dst);
            assert(xanHashTable_lookup(srcData->subloops, loop) == dst);

            /* Add the edge				*/
            assert(src != dst);
            inv			= allocFunction(sizeof(loop_invocations_t));
            inv->invocationsSize	= 1;
            inv->invocations	= allocFunction((inv->invocationsSize) * sizeof(loop_invocations_range_t));
            inv->closed		= JITTRUE;
            e			= xanGraph_addDirectedEdge(g, src, dst, inv);

        } else {

            /* Fetch the edge			*/
            e			= xanGraph_getDirectedEdge(src, dst);
        }
        assert(dst != NULL);
        assert(xanHashTable_lookup(srcData->subloops, loop) == dst);
        assert(e != NULL);
        assert(e->data != NULL);

        /* Fetch the slots of the edge	*/
        inv	= e->data;
        assert(inv != NULL);

        /* Check if there is an open	*
         * slot on the edge		*/
        if (inv->closed == JITTRUE) {

            /* Create a new open slot	*/
            if (inv->invocationsTop == inv->invocationsSize) {
                JITUINT32			delta;
                JITUINT32			oldSize;
                loop_invocations_range_t	*oldInvocations;

                /* Cache the memory			*/
                oldInvocations		= inv->invocations;
                oldSize			= inv->invocationsSize;

                /* Allocate more memory			*/
                assert(inv->invocationsSize > 0);
                delta			= (inv->invocationsSize / 2);
                if (delta == 0) {
                    delta	= 10;
                }
                (inv->invocationsSize)	+= delta;
                inv->invocations	= allocFunction((inv->invocationsSize) * sizeof(loop_invocations_range_t));

                /* Check if we are running out of 	*
                 * memory				*/
                if (inv->invocations == NULL) {

                    /* Get back the old memory		*/
                    inv->invocations	= oldInvocations;
                    inv->invocationsSize	= oldSize;

                    /* Dump the edges			*/
                    internal_dumpEdgeInformationAndResetTop(inv);

                    /* Resize the graph			*/
                    internal_dumpAndResizeEdgesInformation(g);

                } else {

                    /* Copy the old memory inside the new 	*
                     * one					*/
                    memcpy(inv->invocations, oldInvocations, oldSize * sizeof(loop_invocations_range_t));

                    /* Free the old memory			*/
                    freeMemory(oldInvocations);
                }
            }
            assert(inv->invocationsTop < inv->invocationsSize);
            assert(inv->invocations != NULL);

            /* Initialize the slot		*/
            (inv->invocations[inv->invocationsTop]).startInvocation			= loop->invocationsNumber;
            if ((loop->invocationsNumber) >= JITMAXUINT32) {
                fprintf(stderr, "ERROR: Maximum value reached for startInvocation\n");
                abort();
            }
            (inv->invocations[inv->invocationsTop]).fatherInvocation		= srcData->maxInvocationsNumber;
            if ((srcData->maxInvocationsNumber) >= JITMAXUINT32) {
                fprintf(stderr, "ERROR: Maximum value reached for fatherInvocation\n");
                abort();
            }
            (inv->invocations[inv->invocationsTop]).adjacentInvocationsNumber	= 1;
#ifdef DEBUG
            for (JITUINT32 count=0; count < (inv->invocationsTop); count++) {
                assert((inv->invocations[count].fatherInvocation) != (srcData->maxInvocationsNumber));
            }
#endif
            (inv->invocationsTop)++;
            inv->closed	= JITFALSE;

        } else {

            /* Update the invocations	*
             * profile			*/
            assert(inv->invocationsTop > 0);
            if (((inv->invocations[inv->invocationsTop - 1]).adjacentInvocationsNumber) == JITMAXUINT32) {
                fprintf(stderr, "ERROR: Maximum value reached for adjacentInvocationsNumber\n");
                abort();
            }
            ((inv->invocations[inv->invocationsTop - 1]).adjacentInvocationsNumber)++;
        }
        assert(inv->closed == JITFALSE);

    } else {
        loop_invocations_information_t 	*n;

        /* The current loop is an 	*
         * outermost one.		*
         * Create a new node in the 	*
         * graph for it.		*/
        n				= allocFunction(sizeof(loop_invocations_information_t));
        n->loop				= loop;
        n->minInvocationsNumber		= loop->invocationsNumber;
        n->maxInvocationsNumber		= loop->invocationsNumber;
        n->subloops			= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        dst				= xanGraph_addANewNode(g, n);
    }
    assert(dst != NULL);

    /* Add the new loop to the 	*
     * loop stack			*/
    xanStack_push(loopStack, dst);

    /* Fetch the node		*/
    dstData					= dst->data;
    assert(dstData != NULL);
    assert(dstData->minInvocationsNumber <= loop->invocationsNumber);
    assert(dstData->maxInvocationsNumber <= loop->invocationsNumber);

    /* Update the profiled loop	*/
    dstData->maxInvocationsNumber		= loop->invocationsNumber;
    (loop->invocationsNumber)++;

    /* Return			*/
    return ;
}

static inline void loop_invocation_profiler_end_loop (void) {
    XanGraphNode		*n;
    XanHashTableItem	*item;

    /* Pop the executed loop	*/
    n	= xanStack_pop(loopStack);
    assert(n != NULL);
    assert(xanHashTable_elementsInside(n->incomingEdges) <= 1);

    /* Close every outgoing edges	*
     * of the just concluded loop	*/
    item	= xanHashTable_first(n->outgoingEdges);
    while (item != NULL) {
        loop_invocations_t	*inv;
        XanGraphEdge		*e;

        /* Fetch the edge		*/
        e		= item->element;
        assert(e != NULL);
        inv		= e->data;
        assert(inv != NULL);

        /* Close the edge		*/
        inv->closed	= JITTRUE;

        /* Fetch the next element	*/
        item	= xanHashTable_next(n->outgoingEdges, item);
    }

    /* Return			*/
    return ;
}

static inline void internal_dumpEdgeInformation (loop_invocations_t *inv, FILE **fileToWrite, FILE *summaryFile, JITUINT32 *bytesDumped, JITUINT32 *suffixNumber, JITINT8 *prefixName, JITBOOLEAN splittablePoint) {
    JITINT8			buf[DIM_BUF];
    JITUINT64		startInvocation;
    JITUINT64		endInvocation;

    /* Assertions				*/
    assert(inv != NULL);
    assert(fileToWrite != NULL);
    assert(summaryFile != NULL);
    assert(bytesDumped != NULL);
    assert(suffixNumber != NULL);
    assert(prefixName != NULL);

    /* Dump the edges			*/
    for (JITUINT32 count=0; count < inv->invocationsTop; count++) {

        /* Fetch the invocations		*/
        assert((inv->invocations[count]).adjacentInvocationsNumber > 0);
        startInvocation	= (inv->invocations[count]).startInvocation;
        endInvocation	= startInvocation + ((inv->invocations[count]).adjacentInvocationsNumber) - 1;

        /* Dump the values			*/
        SNPRINTF(buf, DIM_BUF, "%llu\n", startInvocation);
        MISC_dumpString(fileToWrite, summaryFile, buf, bytesDumped, suffixNumber, prefixName, JITMAXINT32 / 2, splittablePoint);

        SNPRINTF(buf, DIM_BUF, "%llu\n", endInvocation);
        MISC_dumpString(fileToWrite, summaryFile, buf, bytesDumped, suffixNumber, prefixName, JITMAXINT32 / 2, splittablePoint);

        SNPRINTF(buf, DIM_BUF, "%llu\n", (JITUINT64)(inv->invocations[count]).fatherInvocation);
        MISC_dumpString(fileToWrite, summaryFile, buf, bytesDumped, suffixNumber, prefixName, JITMAXINT32 / 2, splittablePoint);
    }

    /* Return				*/
    return ;
}

static inline JITBOOLEAN internal_fetch_name (JITINT8 **fileName, FILE *summaryFile, JITUINT32 *fileNameSize, JITINT8 endCh) {
    JITUINT32	count;

    /* Assertions			*/
    assert(fileName != NULL);
    assert(*fileName != NULL);
    assert(fileNameSize > 0);

    /* Fetch the next name		*/
    count		= 0;
    while (JITTRUE) {
        JITUINT32	reads;
        reads	= fread(&((*fileName)[count]), 1, 1, summaryFile);
        if (	(reads != 1)			||
                ((*fileName)[count] == '\n')	) {
            (*fileName)[count]	= endCh;
            (*fileName)[count+1]	= '\0';
            break ;
        }
        count++;
        if ((count + 1) >= (*fileNameSize)) {
            (*fileNameSize)	*= 2;
            (*fileName)	= dynamicReallocFunction(*fileName, *fileNameSize);
        }
        assert(count < (*fileNameSize));
    }
    if (count == 0) {
        return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

static inline void internal_dumpEdgeInformationAndResetTop (loop_invocations_t *inv) {
    JITUINT32	newSize;
    JITINT8		buf[DIM_BUF];

    /* Assertions				*/
    assert(inv != NULL);

    /* Check if we need to open files	*/
    if (inv->fileToWrite == NULL) {
        static JITUINT32 	tempFilePrefix = 0;

        /* Set the prefix			*/
        inv->prefixName	= allocFunction(DIM_BUF);
        SNPRINTF(inv->prefixName, DIM_BUF, "temp_%u", tempFilePrefix);
        tempFilePrefix++;

        /* Open the summary file		*/
        SNPRINTF(buf, DIM_BUF, "%s_files", inv->prefixName);
        inv->summaryFile	= fopen((char *)buf, "w+");
        if (inv->summaryFile == NULL) {

            /* Close the files			*/
            internal_closeEdgesInformationFiles(g);

            /* Reopen the file			*/
            inv->summaryFile	= fopen((char *)buf, "w+");
            if (inv->summaryFile == NULL) {
                print_err("ERROR on opening the LIF partial summary output file. ", errno);
                abort();
            }
        }
        assert(inv->summaryFile != NULL);

        /* Open the file to write		*/
        SNPRINTF(buf, DIM_BUF, "%s_%u", inv->prefixName, inv->suffixNumber);
        (inv->suffixNumber)++;
        inv->fileToWrite	= fopen((char *)buf, "w");
        if (inv->fileToWrite == NULL) {

            /* Close the files			*/
            internal_closeEdgesInformationFiles(g);

            /* Reopen the file			*/
            inv->fileToWrite	= fopen((char *)buf, "w");
            if (inv->fileToWrite == NULL) {
                print_err("ERROR on opening the LIF partial output file. ", errno);
                abort();
            }
        }
        assert(inv->fileToWrite != NULL);

        /* Dump the first file to the summary	*
         * file					*/
        fprintf(inv->summaryFile, "%s\n", buf);
    }
    assert(inv->prefixName != NULL);
    assert(inv->summaryFile != NULL);
    assert(inv->fileToWrite != NULL);

    /* Open the summary file		*/
    if (inv->summaryFile == NULL) {
        SNPRINTF(buf, DIM_BUF, "%s_files", inv->prefixName);
        inv->summaryFile	= fopen((char *)buf, "w+");
    }
    assert(inv->summaryFile != NULL);

    /* Dump the values			*/
    internal_dumpEdgeInformation(inv, &(inv->fileToWrite), inv->summaryFile, &(inv->bytesDumped), &(inv->suffixNumber), inv->prefixName, JITTRUE);

    /* Free the memory			*/
    inv->invocationsTop	= 0;
    freeFunction(inv->invocations);

    /* Resize the memory of the current node*/
    newSize	= (inv->invocationsSize / 2);
    if (newSize == 0) {
        newSize	= 10;
    }
    inv->invocations	= allocFunction((inv->invocationsSize) * sizeof(loop_invocations_range_t));

    /* Return				*/
    return ;
}

static inline void internal_closeEdgesInformationFiles (XanGraph *g) {
    XanHashTableItem	*item;

    /* Assertions				*/
    assert(g != NULL);

    /* Resize the memory of every edge	*/
    item	= xanHashTable_first(g->edges);
    while (item != NULL) {
        XanGraphEdge		*e;
        loop_invocations_t 	*currentInv;

        /* Fetch the current edge		*/
        e		= item->element;
        assert(e != NULL);
        currentInv	= e->data;
        assert(currentInv != NULL);

        /* Close the file			*/
        if (currentInv->fileToWrite != NULL) {
            fclose(currentInv->fileToWrite);
            fclose(currentInv->summaryFile);
            currentInv->summaryFile	= NULL;
            currentInv->fileToWrite	= NULL;
        }

        /* Fetch the next element		*/
        item	= xanHashTable_next(g->edges, item);
    }

    /* Return				*/
    return ;
}

static inline void internal_dumpAndResizeEdgesInformation (XanGraph *g) {
    XanHashTableItem	*item;

    /* Assertions				*/
    assert(g != NULL);

    /* Resize the memory of every edge	*/
    item	= xanHashTable_first(g->edges);
    while (item != NULL) {
        XanGraphEdge		*e;
        loop_invocations_t 	*currentInv;

        /* Fetch the current edge		*/
        e		= item->element;
        assert(e != NULL);
        currentInv	= e->data;
        assert(currentInv != NULL);

        /* Resize the edge			*/
        if (	(currentInv->closed)			&&
                (currentInv->invocationsTop > 0)	) {

            /* Dump the edge			*/
            internal_dumpEdgeInformationAndResetTop(currentInv);

            /* Resize the edge			*/
            currentInv->invocationsSize	= 2;
            freeFunction(currentInv->invocations);
            currentInv->invocations		= allocFunction((currentInv->invocationsSize) * sizeof(loop_invocations_range_t));
        }

        /* Fetch the next element		*/
        item	= xanHashTable_next(g->edges, item);
    }

    /* Return				*/
    return ;
}

static inline IR_ITEM_VALUE internal_fetchLoopProfile (loop_profile_t *p) {

    /* Assertions.
     */
    assert(p != NULL);

    return (IR_ITEM_VALUE)(JITNUINT) p;
}

static inline IR_ITEM_VALUE internal_fetchLoopName (loop_profile_t *p) {

    /* Assertions.
     */
    assert(p != NULL);

    return (IR_ITEM_VALUE)(JITNUINT) p->loopName;
}
