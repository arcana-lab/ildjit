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

typedef struct {
    JITUINT64	currentInvocation;
    FILE		*summaryFile;
    FILE		*fileToWrite;
    JITUINT32	bytesDumped;
    JITUINT32	suffixNumber;
    JITINT8		prefixName[DIM_BUF];
    JITINT8		*loopName;
} loop_profile_invocations_information_t;

typedef struct {
    loop_profile_t	*loop;
    JITUINT64	invocationNumber;
    JITUINT64	data1;
    JITUINT64	data2;
    JITUINT64	data3;
} loop_profile_invocation_number_t;

static inline void internal_closeLoopInvocationFiles (void);
static inline void internal_compute_CPI (ir_method_t *method, ir_instruction_t *startInst, ir_instruction_t *endInst, JITUINT64	*memoryCPI, JITUINT64 *cpuCPI, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst));
static inline void loop_invocation_iterations_new_instructions (JITUINT32 memoryCPI, JITUINT32 cpuCPI);
static inline void loop_new_instructions (JITUINT32 memoryCPI, JITUINT32 cpuCPI);
static inline void loop_new_iteration (void);
static inline void loop_new_loop (loop_profile_t *loop);
static inline void loop_end_loop (void);
static inline void loop_invocation_iterations_profiler_new_loop (loop_profile_t *loop);
static inline void loop_invocation_iterations_new_iteration (void);
static inline void loop_invocation_iterations_profiler_end_loop (void);
static inline void loop_invocation_iterations_profiler_end_loop_wrapper (loop_profile_t *loop);
static inline void internal_update_counters (ir_method_t *method, ir_instruction_t *inst, JITUINT64 *memoryCPI, JITUINT64 *cpuCPI, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst));
static inline void internal_injectCodeBetweenInstructions (ir_optimizer_t *irOptimizer, XanList *toInject, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst), void (*new_instructions) (JITUINT32 memoryCPI, JITUINT32 cpuCPI));
static inline XanList * internal_computeMethodsToProfileExecutedInstruction (XanList *p, JITBOOLEAN noSystemLibraries);

static XanStack *loopStack = NULL;
static XanHashTable *loopsInformation = NULL;

XanList * LOOPS_profileLoops (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst)) {
    XanList			*p;
    XanList			*toInject;
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
    assert(executionModel != NULL);

    /* Allocate the necessary memory.
     */
    loopStack 		= xanStack_new(allocFunction, freeFunction, NULL);
    assert(loopStack != NULL);

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
    newLoopfunctionPointer 		= loop_new_loop;
    endLoopFunctionPointer 		= loop_end_loop;
    newLoopIterationFunctionPointer	= loop_new_iteration;

    /* Compute the set of methods to profile.
     */
    toInject			= internal_computeMethodsToProfileExecutedInstruction(p, noSystemLibraries);

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

            /* Allocate the stack of cycles.
             */
            (profile->IR).cycles    = xanStack_new(allocFunction, freeFunction, NULL);

            /* Fetch the successors and predecessors of the loop.
             */
            successors 				= xanHashTable_lookup(loopsSuccessors, profile->loop);
            assert(successors != NULL);
            predecessors 			= xanHashTable_lookup(loopsPredecessors, profile->loop);
            assert(predecessors != NULL);

            /* Create the parameters.
             */
            memset(&currentItem, 0, sizeof(ir_item_t));
            start_new_outermost_loop_parameters 	= xanList_new(allocFunction, freeFunction, NULL);
            currentItem.value.v 	    		    = (IR_ITEM_VALUE)(JITNUINT) profile;
            currentItem.type 			            = IRNUINT;
            currentItem.internal_type 	    	    = IRNUINT;
            xanList_append(start_new_outermost_loop_parameters, &currentItem);

            /* Profile the loop.
             */
            profileLoop	 (	irOptimizer,
                            profile,
                            newLoopfunctionPointer, NULL, start_new_outermost_loop_parameters,
                            endLoopFunctionPointer, NULL, NULL,
                            newLoopIterationFunctionPointer, NULL, NULL,
                            successors, predecessors, NULL);

            /* Free the memory.
             */
            xanList_destroyList(start_new_outermost_loop_parameters);
        }

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    /* Inject the code to profile executed instructions.
     */
    internal_injectCodeBetweenInstructions(irOptimizer, toInject, executionModel, loop_new_instructions);

    /* Free the memory.
     */
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);
    xanList_destroyList(toInject);

    return p;
}

void LOOPS_profileLoopsEnd (void) {

    /* Free the memory.
     */
    xanStack_destroyStack(loopStack);

    return ;
}

void LOOPS_profileLoopsInvocationsIterationsAndInstructions (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst)) {
    XanList			*p;
    XanList			*toInject;
    XanListItem		*item;
    XanHashTable 		*loopsSuccessors;
    XanHashTable 		*loopsPredecessors;
    JITUINT32		loopID;
    void 			*newLoopfunctionPointer;
    void 			*endLoopFunctionPointer;
    void 			*newIterationFunctionPointer;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);
    assert(loopsNames != NULL);
    assert(executionModel != NULL);

    /* Allocate the necessary 	*
     * memories			*/
    loopStack 		= xanStack_new(allocFunction, freeFunction, NULL);
    assert(loopStack != NULL);
    loopsInformation	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(loopsInformation != NULL);

    /* Allocate the loops 		*
     * information table		*/
    loopID		= 0;
    item 		= xanList_first(loops);
    while (item != NULL) {
        loop_t					*loop;
        loop_profile_invocations_information_t	*info;
        loop			= item->data;
        assert(loop != NULL);
        info			= allocFunction(sizeof(loop_profile_invocations_information_t));
        assert(info != NULL);
        info->loopName		= xanHashTable_lookup(loopsNames, loop);
        SNPRINTF(info->prefixName, DIM_BUF, "%s_loops_memory_cpu_per_invocation_loop_%u", IRPROGRAM_getProgramName(), loopID);
        xanHashTable_insert(loopsInformation, loop, info);
        loopID++;
        item			= item->next;
    }

    /* Allocate the profile data	*
     * structures			*/
    p		= get_profiled_loops(loops, loopsNames);
    assert(p != NULL);

    /* Compute the successors of	*
     * the loops to profile		*/
    loopsSuccessors 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopsPredecessors 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    LOOPS_getSuccessorsAndPredecessorsOfLoops(loopsSuccessors, loopsPredecessors, loops);

    /* Set the function pointers	*/
    newLoopfunctionPointer 		= loop_invocation_iterations_profiler_new_loop;
    endLoopFunctionPointer 		= loop_invocation_iterations_profiler_end_loop;
    newIterationFunctionPointer 	= loop_invocation_iterations_new_iteration;
#ifdef DEBUG
    endLoopFunctionPointer 		= loop_invocation_iterations_profiler_end_loop_wrapper;
#endif

    /* Compute the set of methods to profile.
     */
    toInject			= internal_computeMethodsToProfileExecutedInstruction(p, noSystemLibraries);

    /* Profile the loops		*/
    item 		= xanList_first(p);
    while (item != NULL) {
        loop_profile_t			*profile;
        XanList 			*start_new_outermost_loop_parameters;
        ir_item_t 			currentItem;

        /* Fetch the loop to profile		*/
        profile			= item->data;
        assert(profile != NULL);

        /* Check if we should consider the 	*
         * current loop				*/
        if (	(noSystemLibraries == JITFALSE)					||
                (!IRPROGRAM_doesMethodBelongToALibrary(profile->loop->method))	) {
            XanList 	*successors;
            XanList 	*predecessors;
            XanList		*end_loop_parameters;

            /* Initialize the variables		*/
            end_loop_parameters			= NULL;

            /* Fetch the successors of the loop	*/
            successors 				= xanHashTable_lookup(loopsSuccessors, profile->loop);
            assert(successors != NULL);

            /* Fetch the predecessors of the loop	*
             * header				*/
            predecessors 				= xanHashTable_lookup(loopsPredecessors, profile->loop);
            assert(predecessors != NULL);

            /* Create the parameters		*/
            start_new_outermost_loop_parameters 	= xanList_new(allocFunction, freeFunction, NULL);
            memset(&currentItem, 0, sizeof(ir_item_t));
            currentItem.value.v 			= (IR_ITEM_VALUE)(JITNUINT) profile;
            currentItem.type 			= IRNUINT;
            currentItem.internal_type 		= IRNUINT;
            xanList_append(start_new_outermost_loop_parameters, &currentItem);
#ifdef DEBUG
            end_loop_parameters			= start_new_outermost_loop_parameters;
#endif

            /* Profile the loop			*/
            profileLoop 	(	irOptimizer,
                                profile,
                                newLoopfunctionPointer, NULL, start_new_outermost_loop_parameters,
                                endLoopFunctionPointer, NULL, end_loop_parameters,
                                newIterationFunctionPointer, NULL, NULL,
                                successors, predecessors, NULL);

            /* Free the memory			*/
            xanList_destroyList(start_new_outermost_loop_parameters);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Inject the code to profile executed instructions.
     */
    internal_injectCodeBetweenInstructions(irOptimizer, toInject, executionModel, loop_invocation_iterations_new_instructions);

    /* Free the memory		*/
    xanHashTable_destroyTable(loopsSuccessors);
    xanHashTable_destroyTable(loopsPredecessors);
    xanList_destroyList(toInject);

    return ;
}

void LOOPS_profileLoopsInvocationsIterationsAndInstructionsEnd (void) {
    XanHashTableItem	*item;

    /* Assertions			*/
    assert(loopsInformation != NULL);

    /* End the running loops 	*/
    while (xanStack_getSize(loopStack) > 0) {
        loop_invocation_iterations_profiler_end_loop();
    }
    assert(xanStack_getSize(loopStack) == 0);

    /* Close the file		*/
    item	= xanHashTable_first(loopsInformation);
    while (item != NULL) {
        loop_profile_invocations_information_t	*i;
        i	= item->element;
        assert(i != NULL);
        if (i->fileToWrite != NULL) {
            fclose(i->fileToWrite);
            i->fileToWrite	= NULL;
        }
        if (i->summaryFile != NULL) {
            fclose(i->summaryFile);
            i->summaryFile	= NULL;
        }
        item	= xanHashTable_next(loopsInformation, item);
    }

    /* Free the memory		*/
    xanHashTable_destroyTableAndData(loopsInformation);

    /* Return			*/
    return ;
}

/*****************************************************************************************************************
						Help functions
*****************************************************************************************************************/
static inline void loop_new_instructions (JITUINT32 memoryCPI, JITUINT32 cpuCPI) {
    XanListItem	*item;

    /* Assertions.
     */
    assert(loopStack != NULL);

    /* Check if are inside a loop.
     */
    if (xanStack_getSize(loopStack) == 0) {
        return ;
    }

    /* Erase the flags.
     */
    item	= xanList_first(loopStack->internalList);
    while (item != NULL) {
        loop_profile_t	*l;
        l		= item->data;
        assert(l != NULL);
        l->totalTime	= 0;
        item		= item->next;
    }

    /* Update the profile.
     */
    item	= xanList_first(loopStack->internalList);
    while (item != NULL) {
        loop_profile_t	*l;

        /* Fetch the profile.
         */
        l	= item->data;
        assert(l != NULL);

        /* Update the profile.
         */
        if (l->totalTime == 0) {
            XanListItem *item2;
            ((l->IR).memoryClocks)	+= (JITUINT64) memoryCPI;
            ((l->IR).cpuClocks)	    += (JITUINT64) cpuCPI;
            item2	= xanList_first((l->IR).cycles->internalList);
            while (item2 != NULL) {
                JITUINT64   *currentCycles;
                currentCycles       = item2->data;
                (*currentCycles)	+= ((JITUINT64) cpuCPI) + ((JITUINT64) memoryCPI);
                item2               = item2->next;
            }
            l->totalTime		    = 1;
        }

        /* Fetch the next element to update.
         */
        item	= item->next;
    }

    return ;
}

static inline void loop_invocation_iterations_new_instructions (JITUINT32 memoryCPI, JITUINT32 cpuCPI) {
    XanListItem			*item;

    /* Assertions			*/
    assert(loopStack != NULL);

    /* Check if are inside a loop	*/
    if (xanStack_getSize(loopStack) == 0) {
        return ;
    }

    /* Update the profile		*/
    item	= xanList_first(loopStack->internalList);
    while (item != NULL) {
        loop_profile_invocation_number_t	*l;

        /* Fetch the profile		*/
        l	= item->data;
        assert(l != NULL);
        assert(l->loop != NULL);
        assert(l->loop->loopName != NULL);

        /* Update the profile		*/
        (l->data1)	+= (JITUINT64) memoryCPI;
        (l->data2)	+= (JITUINT64) cpuCPI;

        /* Fetch the next element	*/
        item	= item->next;
    }

    return ;
}

static inline void internal_compute_CPI (ir_method_t *method, ir_instruction_t *startInst, ir_instruction_t *endInst, JITUINT64	*memoryCPI, JITUINT64 *cpuCPI, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst)) {

    /* Assertions.
     */
    assert(memoryCPI != NULL);
    assert(cpuCPI != NULL);
    assert(startInst != NULL);
    assert(endInst != NULL);
    assert(startInst->ID <= endInst->ID);

    /* Initialize the variables.
     */
    (*memoryCPI)		= 0;
    (*cpuCPI)		= 0;

    /* Compute the CPI of the instructions in the straight block.
     */
    while (startInst != endInst) {

        /* Add the CPI of the current instruction.
         */
        internal_update_counters(method, startInst, memoryCPI, cpuCPI, executionModel);

        /* Fetch the next instruction of the block.
         */
        startInst	= IRMETHOD_getNextInstruction(method, startInst);
        assert(startInst != NULL);
    }
    assert(startInst == endInst);

    /* Add the CPI of the last of the block.
     */
    internal_update_counters(method, endInst, memoryCPI, cpuCPI, executionModel);

    return ;
}

static inline void loop_new_iteration (void) {
    loop_profile_t 	*loop;

    /* Fetch the loop in execution.
     */
    loop	= xanStack_top(loopStack);
    assert(loop != NULL);

    /* Update the counter of invocations executed of the loop.
     */
    (loop->iterationsNumber)++;

    return ;
}

static inline void loop_new_loop (loop_profile_t *loop) {

    /* Assertions.
     */
    assert(loop != NULL);

    /* Update the counter of invocations executed of the loop.
     */
    (loop->invocationsNumber)++;

    /* Push a new cycle value.
     */
    xanStack_push((loop->IR).cycles, allocFunction(sizeof(JITUINT64)));

    /* Add the new loop to the loop stack.
     */
    xanStack_push(loopStack, loop);

    return ;
}

static inline void loop_end_loop (void) {
    loop_profile_t  *p;
    JITUINT64       *currentCycles;
    JITFLOAT64      currentCyclesValue;
    JITFLOAT64      previousAverage;
    JITFLOAT64      floatSamples;

    /* Assertions.
     */
    assert(xanStack_getSize(loopStack) > 0);

    /* Pop the loop from the stack.
     */
    p   = xanStack_pop(loopStack);
    assert(p != NULL);

    /* Fetch the current value of the current loop just ended.
     */
    currentCycles       = xanStack_pop((p->IR).cycles);
    assert(currentCycles != NULL);
    currentCyclesValue  = (JITFLOAT64)(*currentCycles);

    /* Update the statistics.
     *
     * Compute the average.
     */
    (p->IR).samples++;
    floatSamples                = (JITFLOAT64)(p->IR).samples;
    previousAverage             = (p->IR).averageCycles;
    (p->IR).averageCycles       = (((floatSamples - 1) * previousAverage) + currentCyclesValue) / floatSamples;

    /* Compute the M2n and the variance.
     */
    if ((p->IR).samples >= 2){
        (p->IR).M2n              += (   
                                        (currentCyclesValue - previousAverage) * (currentCyclesValue - (p->IR).averageCycles)
                                    );
        (p->IR).varianceCycles   = (p->IR).M2n / (floatSamples - 1);
    }

    /* Free the memory.
     */
    freeFunction(currentCycles);

    return ;
}

static inline void loop_invocation_iterations_profiler_new_loop (loop_profile_t *loop) {
    loop_profile_invocation_number_t	*l;
    loop_profile_invocations_information_t	*info;

    /* Assertions			*/
    assert(loop != NULL);

    /* Allocate the memory		*/
    l			= allocFunction(sizeof(loop_profile_invocation_number_t));

    /* Fill up the memory		*/
    info			= xanHashTable_lookup(loopsInformation, loop->loop);
    assert(info != NULL);
    l->loop			= loop;
    l->invocationNumber	= info->currentInvocation;

    /* Update the invocation number	*/
    (info->currentInvocation)++;

    /* Add the new loop to the 	*
     * loop stack			*/
    xanStack_push(loopStack, l);

    /* Return			*/
    return ;
}

static inline void loop_invocation_iterations_profiler_end_loop (void) {
    JITINT8					buf[DIM_BUF];
    loop_profile_invocations_information_t	*info;
    loop_profile_invocation_number_t	*l;

    /* Assertions			*/
    assert(xanStack_getSize(loopStack) > 0);

    /* Pop the loop from the stack	*/
    l		= xanStack_pop(loopStack);
    assert(l != NULL);

    /* Update the profile		*/
    (l->data3)--;
    assert(l->data3 > 0);

    /* Fetch the loop information	*/
    info		= xanHashTable_lookup(loopsInformation, l->loop->loop);
    assert(info != NULL);

    /* Open the files		*/
    if (info->summaryFile == NULL) {
        JITINT8	buf2[DIM_BUF];

        /* Sanity checks		*/
        assert(info->fileToWrite == NULL);
        assert(info->suffixNumber == 0);

        /* Open the files		*/
        SNPRINTF(buf, DIM_BUF, "%s_%u", info->prefixName, info->suffixNumber);
        (info->suffixNumber)++;
        info->fileToWrite	= fopen((char *)buf, "w");
        if (info->fileToWrite == NULL) {

            /* Close the files		*/
            internal_closeLoopInvocationFiles();

            /* Open the new file		*/
            info->fileToWrite	= fopen((char *)buf, "w");
            if (info->fileToWrite == NULL) {
                print_err("ERROR on opening the output file. ", errno);
                abort();
            }
        }
        assert(info->fileToWrite != NULL);
        SNPRINTF(buf2, DIM_BUF, "%s_summary", info->prefixName);
        info->summaryFile	= fopen((char *)buf2, "w");
        if (info->summaryFile == NULL) {
            print_err("ERROR on opening the summary outputs file. ", errno);
            abort();
        }

        /* Write the summary file	*/
        fprintf(info->summaryFile, "%s\n", buf);

        /* Write the data file		*/
        SNPRINTF(buf, DIM_BUF, "%s\n", info->loopName);
        MISC_dumpString(&(info->fileToWrite), (info->summaryFile), buf, &(info->bytesDumped), &(info->suffixNumber), info->prefixName, JITMAXINT32 / 2, JITTRUE);
        SNPRINTF(buf, DIM_BUF, "InvocationNumber LoopMemoryInstructions LoopCPUInstructions LoopIterations\n");
        MISC_dumpString(&(info->fileToWrite), (info->summaryFile), buf, &(info->bytesDumped), &(info->suffixNumber), info->prefixName, JITMAXINT32 / 2, JITTRUE);
    }
    assert(info->summaryFile != NULL);
    if (info->fileToWrite == NULL) {
        SNPRINTF(buf, DIM_BUF, "%s_%u", info->prefixName, info->suffixNumber);
        info->fileToWrite	= fopen((char *)buf, "w");
        if (info->fileToWrite == NULL) {
            print_err("ERROR on opening the output file. ", errno);
            abort();
        }
    }
    assert(info->fileToWrite != NULL);

    /* Dump the information		*/
    assert((l->data1 + l->data2) > 0);
    SNPRINTF(buf, DIM_BUF, "%llu %llu %llu %llu\n", l->invocationNumber, l->data1, l->data2, l->data3);
    MISC_dumpString(&(info->fileToWrite), (info->summaryFile), buf, &(info->bytesDumped), &(info->suffixNumber), info->prefixName, JITMAXINT32 / 2, JITTRUE);

    /* Remove the not more useful	*
     * memory			*/
    freeFunction(l);

    /* Return			*/
    return ;
}

static inline void loop_invocation_iterations_profiler_end_loop_wrapper (loop_profile_t *loop) {
    loop_profile_invocation_number_t	*l;

    /* Assertions			*/
    assert(xanStack_getSize(loopStack) > 0);
    assert(loop->loopName != NULL);

    /* Check the loop		*/
    if (loop == NULL) {
        fprintf(stderr, "NULL parameter\n");
        abort();
    }

    /* Check the top		*/
    l	= xanStack_top(loopStack);
    assert(l != NULL);
    assert(l->loop != NULL);
    if (l->loop != loop) {
        fprintf(stderr, "Corrupted stack\n");
        fprintf(stderr, "Loop is ending: %s\n", loop->loopName);
        fprintf(stderr, "	Header label %llu\n", loop->headerLabel);
        fprintf(stderr, "Loop at the top of the stack: %s\n", l->loop->loopName);
        fprintf(stderr, "	Header label %llu\n", l->loop->headerLabel);
        abort();
    }

    /* End the loop			*/
    loop_invocation_iterations_profiler_end_loop();

    /* Return			*/
    return ;
}

static inline void loop_invocation_iterations_new_iteration (void) {
    loop_profile_invocation_number_t	*l;

    /* Assertions			*/
    assert(xanStack_getSize(loopStack) > 0);

    /* Fetch the profile		*/
    l	= xanStack_top(loopStack);
    assert(l != NULL);

    /* Update the profile		*/
    (l->data3)++;

    /* Return			*/
    return ;
}

static inline void internal_update_counters (ir_method_t *method, ir_instruction_t *inst, JITUINT64 *memoryCPI, JITUINT64 *cpuCPI, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst)) {
    JITUINT64	CPI;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(memoryCPI != NULL);
    assert(cpuCPI != NULL);
    assert(executionModel != NULL);

    /* Update the counters		*/
    CPI	= 0;
    if (inst->type != IRNATIVECALL) {

        /* Fetch the CPI		*/
        CPI		= (*executionModel)(method, inst);

        /* Update the right counter	*/
        if (IRMETHOD_isAMemoryInstruction(inst)) {
            (*memoryCPI)	+= CPI;
        } else {
            (*cpuCPI)	+= CPI;
        }
    }

    /* Return			*/
    return ;
}

static inline void internal_closeLoopInvocationFiles (void) {
    XanHashTableItem	*item;

    /* Assertions				*/
    assert(loopsInformation != NULL);

    /* Resize the memory of every edge	*/
    item	= xanHashTable_first(loopsInformation);
    while (item != NULL) {
        loop_profile_invocations_information_t	*info;

        /* Fetch the current edge		*/
        info		= item->element;
        assert(info != NULL);

        /* Close the file			*/
        if (info->fileToWrite != NULL) {
            fclose(info->fileToWrite);
            info->fileToWrite	= NULL;
        }

        /* Fetch the next element		*/
        item	= xanHashTable_next(loopsInformation, item);
    }

    /* Return				*/
    return ;
}

static inline void internal_injectCodeBetweenInstructions (ir_optimizer_t *irOptimizer, XanList *toInject, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst), void (*new_instructions) (JITUINT32 memoryCPI, JITUINT32 cpuCPI)) {
    XanList			*parameters;
    XanListItem		*item;
    ir_item_t		param1;
    ir_item_t		param2;

    /* Make the parameters required for code injection.
     */
    parameters		= xanList_new(allocFunction, freeFunction, NULL);
    memset(&param1, 0, sizeof(ir_item_t));
    memset(&param2, 0, sizeof(ir_item_t));
    param1.type			= IRUINT32;
    param1.internal_type		= param1.type;
    param2.type			= IRUINT32;
    param2.internal_type		= param2.type;
    xanList_append(parameters, &param1);
    xanList_append(parameters, &param2);

    /* Inject code.
     */
    item 		= xanList_first(toInject);
    while (item != NULL) {
        ir_method_t 		*methodToConsider;
        ir_instruction_t 	*inst;
        IRBasicBlock		*bb;
        XanList			*points;
        XanListItem		*item2;

        /* Fetch the method to profile.
         */
        methodToConsider 	= item->data;
        assert(methodToConsider != NULL);

        /* Optimize the method.
         */
//		IROPTIMIZER_optimizeMethod(irOptimizer, methodToConsider);
        IROPTIMIZER_callMethodOptimization(irOptimizer, methodToConsider, DEADCODE_ELIMINATOR);

        /* Compute the basic blocks.
         */
        IROPTIMIZER_callMethodOptimization(irOptimizer, methodToConsider, BASIC_BLOCK_IDENTIFIER);

        /* Find the points to inject code.
         */
        points	= xanList_new(allocFunction, freeFunction, NULL);
        bb	= IRMETHOD_getFirstBasicBlock(methodToConsider);
        while (bb != NULL) {
            memory_t	*point;

            /* Fetch the first instruction of the basic block.
             */
            inst			= IRMETHOD_getFirstInstructionWithinBasicBlock(methodToConsider, bb);
            assert(inst != NULL);

            /* Check if the basic block should be considered.
             */
            if (inst->type != IREXITNODE) {

                /* Fetch the start point.
                 */
                while (	(inst != NULL) 						&&
                        ((inst->type == IRLABEL) || (inst->type == IRNOP))	) {
                    inst	= IRMETHOD_getNextInstructionWithinBasicBlock(methodToConsider, bb, inst);
                }

                /* Check if we need to consider the basic block.
                 */
                if (inst != NULL) {
                    assert(inst->type != IRLABEL);
                    assert(inst->type != IRNOP);

                    /* Allocate the point.
                     */
                    point			= allocFunction(sizeof(memory_t));

                    /* Set the boundaries of the block.
                     */
                    point->start		= inst;
                    point->end		= IRMETHOD_getLastInstructionWithinBasicBlock(methodToConsider, bb);

                    /* Inject the code to profile the last part of the basic block.
                     */
                    xanList_append(points, point);
                }
            }

            /* Fetch the next basic block.
             */
            bb			= IRMETHOD_getNextBasicBlock(methodToConsider, bb);
        }

        /* Inject the code.
         */
        item2			= xanList_first(points);
        while (item2 != NULL) {
            memory_t			*point;
            ir_instruction_t		*pointToInject;
            JITUINT64			memoryCPI;
            JITUINT64			cpuCPI;

            /* Fetch the point.
             */
            point			= item2->data;
            assert(point != NULL);
            pointToInject		= (ir_instruction_t *)point->start;
            assert(pointToInject != NULL);

            /* Compute the CPIs.
             */
            internal_compute_CPI(methodToConsider, point->start, point->end, &memoryCPI, &cpuCPI, executionModel);

            /* Inject code.
             */
            if (	(memoryCPI > 0)	||
                    (cpuCPI > 0)	) {
                param1.value.v	= (IR_ITEM_VALUE)memoryCPI;
                param2.value.v	= (IR_ITEM_VALUE)cpuCPI;
                if (pointToInject->type == IRLABEL) {
                    IRMETHOD_newNativeCallInstructionAfter(methodToConsider, pointToInject, "instructions_executed", new_instructions, NULL, parameters);
                } else {
                    IRMETHOD_newNativeCallInstructionBefore(methodToConsider, pointToInject, "instructions_executed", new_instructions, NULL, parameters);
                }
            }

            /* Fetch the next element to consider.
             */
            item2			= item2->next;
        }

        /* Free the memory.
         */
        xanList_destroyListAndData(points);

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(parameters);

    return ;
}

static inline XanList * internal_computeMethodsToProfileExecutedInstruction (XanList *p, JITBOOLEAN noSystemLibraries) {
    XanList		*toInject;
    XanListItem	*item;

    /* Allocate the memory.
     */
    toInject 	= xanList_new(allocFunction, freeFunction, NULL);

    /* Compute the set of methods to consider for code injection.
     */
    item 		= xanList_first(p);
    while (item != NULL) {
        loop_profile_t			*profile;

        /* Fetch the loop to profile		*/
        profile		= item->data;
        assert(profile != NULL);

        /* Check if we should consider the 	*
         * current loop				*/
        if (	(noSystemLibraries == JITFALSE)					||
                (!IRPROGRAM_doesMethodBelongToALibrary(profile->loop->method))	) {
            XanList		*l;
            XanListItem	*item2;

            /* Add the list of reachable methods	*/
            l = IRPROGRAM_getReachableMethods(profile->loop->method);
            item2 = xanList_first(l);
            while (item2 != NULL) {
                ir_method_t	*m;

                /* Fetch the method.
                 */
                m	= item2->data;
                assert(m != NULL);

                /* Check if the method has a body and it has not been considered yet.
                 */
                if (	(IRMETHOD_hasMethodInstructions(m))	&&
                        (xanList_find(toInject, m) == NULL)	) {

                    /* Tag the method		*/
                    xanList_append(toInject, m);
                }

                /* Fetch the next element	*/
                item2 = item2->next;
            }

            /* Free the memory			*/
            xanList_destroyList(l);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    return toInject;
}
