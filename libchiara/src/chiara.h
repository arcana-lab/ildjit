/*
 * Copyright (C) 2010 - 2011  Campanoni Simone
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
#ifndef CHIARA_H
#define CHIARA_H

#include <xanlib.h>
#include <ir_method.h>
#include <ir_optimizer.h>

void setIrOptimizerInChiara(ir_optimizer_t *opt);

typedef struct {
    JITUINT32		ID;
    XanList			*waitInsns;
    XanList			*signalInsns;
    ir_item_t		currentIterationMutexVar;
    ir_item_t		nextIterationMutexVar;
} sequential_segment_description_t;

typedef struct {
    ir_instruction_t	*memoryUse;		/**< It could be IRSTOREREL, IRLOADREL                              				*/
    XanList			*memoryAllocators;	/**< List of (ir_instruction_t *) that could allocate the memory used by memoryUse	*/
} data_dependence_memory_allocators_t;

/**
 *
 * Information about instructions that read or store from/to the memory and instructions of the same method that can lead to the allocation of that piece of	*
 * memory.
 * Only the memory allocated explicitly are considered (e.g. by instructions IRALLOC, IRALLOCA, IRALLOCALIGN, IRNEWOBJ, IRNEWARR or returned by IRLIBRARYCALL).
 */
typedef struct {
    data_dependence_memory_allocators_t	*relations;	/**< Set of relation between memory uses and possible memory allocations	*/
    JITUINT32				relationsNum;	/**< Number of relations							*/
} data_dependences_memory_allocators_t;

typedef struct {
    JITUINT64 elapsedTicks;
    JITUINT64 elapsedTicksSquare;
    JITUINT32 nestingLevel;
    JITUINT64 executionTimes;
    struct {
        JITUINT64 not_produced_by_iteration;	/**< Bytes used, which are not produced by the current iteration	*/
        JITUINT64 produced_by_iteration;	/**< Bytes used, which are produced by the current interation		*/
        JITUINT64 produced_outside_loop;	/**< Bytes used, which are produced outside the loop			*/
    } data_read;
    XanList *loopStack;		/**< List of (loop_profile_t *)								*/
} loop_profile_data_t;

typedef struct {
    loop_t *loop;
    JITINT8 *loopName;
    JITUINT32 headerID;
    IR_ITEM_VALUE headerLabel;	/**< Label number of the header of the loop									*/
    XanList *iterationsTimes;	/**< List of (XanList *). Every element of the sublist is (loop_profile_data_t *).				*
					  *  The meaning depends on the profiler used. 									*
					  *  For time profilers, this list are the times spent within single iterations of single invocations of loops. *
					  *  For data profilers, this list are the data size forwarded.							*/
    XanList 	*times; 		/**< Time spent within single invocation of the loops. List of (loop_profile_data_t) 			*/
    JITFLOAT64 	totalTime;		/**< Total time spent within the loop									*/
    JITFLOAT64 	totalOutermostTime; 	/**< Total time spent within the loop when it was run as an outermost loop				*/
    struct {
        JITUINT64       memoryClocks;
        JITUINT64       cpuClocks;
        JITFLOAT64      averageCycles;
        JITFLOAT64      varianceCycles;
        JITFLOAT64      M2n;
        JITUINT64       samples;
        XanStack        *cycles;
    } IR;
    JITUINT64	invocationsNumber;	/**< Number of invocations of the loop									*/
    JITUINT64	iterationsNumber;	/**< Number of iterations of the loop									*/
    JITBOOLEAN	flag1;
    void		*userData;
} loop_profile_t;

typedef struct {
    JITUINT32	fatherInvocation;
    JITUINT32	startInvocation;
    JITUINT32	adjacentInvocationsNumber;
} loop_invocations_range_t;

typedef struct {
    loop_invocations_range_t	*invocations;
    JITUINT32			invocationsTop;
    JITUINT32			invocationsSize;
    JITBOOLEAN			closed;
    JITINT8				*prefixName;
    JITUINT32			bytesDumped;
    JITUINT32			suffixNumber;
    FILE				*summaryFile;
    FILE				*fileToWrite;
} loop_invocations_t;

typedef struct {
    loop_profile_t			*loop;			/**< Static loop								*/
    JITUINT64			minInvocationsNumber;
    JITUINT64			maxInvocationsNumber;
    XanHashTable			*subloops;		/**< Elements are (XanGraphNode *). Keys are (loop_profile_t *).		*/
} loop_invocations_information_t;

enum data_dependence_category {FALSE_ESCAPES_MEMORY, FALSE_MEMORY_MANAGEMENT, FALSE_MEMORY_RENAMING, FALSE_INDUCTIVE_VARIABLE, FALSE_UNIQUE_VALUE_PER_ITERATION, ALWAYS_TRUE, GLOBAL_VARIABLE, LIBRARY_TRUE, RUNTIME_CHECK_TRUE, FALSE_REGISTER_REASSOCIATION, FLOATINGPOINT_REGISTER_REASSOCIATION, WAW_REGISTER_REASSOCIATION, MEMORY_WAW_REASSOCIATION, FALSE_LOCALIZABLE, FALSE_PRIVATE_MEMORY, FALSE_CODE_HOISTING, INPUT_OUTPUT, UNIQUE_VALUE_PER_ITERATION, ALL_DD };
enum condition_t { NO_CONDITION, MEMORY_ALIAS_CONDITION };

typedef struct {
    union {
        struct {
            ir_item_t var1;
            ir_item_t var2;
        } alias;
    } condition;
    enum condition_t conditionType;
} condition_t;

typedef struct {
    ir_instruction_t                        *inst1;				/**< This is the source of the edge of DDG							*/
    ir_instruction_t                        *inst2;				/**< This is the destination of the edge of DDG. 						*
										  *  In other words, this instruction has to be executed before inst1 in order to not break the	*
										  *  dependence											*/
    struct {
        ir_instruction_t *from;
        ir_instruction_t *to;
    } inst1Boundary;
    struct {
        ir_instruction_t *from;
        ir_instruction_t *to;
    } inst2Boundary;
    JITUINT16 type;
    enum data_dependence_category category;
    condition_t runtime_condition;
    JITUINT32 originalInductiveVariableID;
    JITUINT32 originalDerivedInductiveVariableID;
    struct {
        JITUINT32 origInst1ID;
        JITUINT32 origInst2ID;
        struct {
            JITUINT64 executed;                                     /**< Number of times the instruction is executed					*/
            JITUINT64 trueDependence;                               /**< Number of times the data dependence was true just before executing the instruction	*/
        } inst1;
        struct {
            JITUINT64 executed;                                     /**< Number of times the instruction is executed					*/
            JITUINT64 trueDependence;                               /**< Number of times the data dependence was true just before executing the instruction	*/
        } inst2;
    } frequency;
    JITBOOLEAN isBasedOnInductiveVariable;                                  /**< JITTRUE if the data dependence can be deleted by exploiting the inductive variable. *
										  *  The instruction inst2 always define the inductive variable				 */
} loop_inter_interation_data_dependence_t;

/****************** Loops analysis *************************************************************************/
XanGraph * LOOPS_loadLoopsInvocationsForestTrees (XanHashTable *loopsNames, JITINT8 *prefixName, XanHashTable *loops);
XanList * LOOPS_loadLoopNames (JITINT8 *fileName);
XanGraphNode * LOOPS_getLoopInvocation (XanGraph *g, loop_t *loop, JITUINT32 startInvocation);
XanList * LOOPS_getOutermostProgramLoops (XanHashTable *loops);
XanList * LOOPS_getProgramLoops (XanHashTable *loops);
XanList * LOOPS_getLoopsWithinMethod (XanHashTable *loops, ir_method_t *method);

/**
 *
 * Return the list of loops that can be proved to be outermost.
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getOutermostLoops (XanHashTable *loops, XanList *loopsToConsider);

/**
 *
 * Return the list of loops that may be outermost.
 *
 * In other words, the returned loops cannot be proved to be not outermost.
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getMayBeOutermostLoops (XanHashTable *loops, XanList *loopsToConsider);

/**
 *
 * Return the list of loops that may be at nesting level <code> nestingLevel </code>.
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getLoopsMayBeAtNestingLevel (XanHashTable *loops, JITUINT32 nestingLevel);

/**
 *
 * Return the list of loops that may be at nesting level <code> nestingLevel </code> starting from the loops <code> loops </code>
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getSubLoopsMayBeAtNestingLevel (XanList *loops, JITUINT32 nestingLevel);

/**
 *
 * Return the list of loops that may be at nesting level <code> nestingLevel </code>
 *
 * Every returned loop belongs to the program and not to any external library.
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getProgramSubLoopsMayBeAtNestingLevel (XanHashTable *loops, JITUINT32 nestingLevel);

/**
 *
 * Return the list of loops that may be at nesting level <code> nestingLevel </code> starting from the loops <code> loops </code>.
 *
 * Every returned loop belongs to the program and not to any external library.
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getProgramSubLoopsMayBeAtNestingLevelFromParentLoops (XanList *loops, JITUINT32 nestingLevel);

void LOOPS_addProgramSubLoopsMayBeAtNestingLevels (XanList *parentLoops, JITUINT32 fromNestingLevel, JITUINT32 toNestingLevel, XanList *subloops);

/**
 *
 * Return the list of loops that may be at nesting level <code> nestingLevel </code>.
 *
 * @return List of (loop_t *)
 */
XanList * LOOPS_getProgramLoopsMayBeAtNestingLevel (XanHashTable *loops, JITUINT32 nestingLevel);

void LOOPS_destroyLoopsNames (XanHashTable *loopsNames);
void LOOPS_removeLibraryLoops (XanList *loops);

/**
 * @brief Get names of loops
 *
 * Collect names of loops previously analyzed
 *
 * @return Hash table where (loop_t *) is the key and (JITINT8 *) is the value, which is the string name of the loop
 */
XanHashTable * LOOPS_getLoopsNames (XanHashTable *loops);

/**
 *
 * @return Tree of elements (loop_t *)
 */
XanNode * LOOPS_getMethodLoopsNestingTree (XanHashTable *loops, ir_method_t *method);

XanList * LOOPS_getImmediateParentsOfLoop (XanList *loops, loop_t *loop);

JITUINT32 LOOPS_numberOfBackedges (loop_t *loop);
JITBOOLEAN LOOPS_isAnInvariantInstruction (loop_t *loop, ir_instruction_t *inst);
JITBOOLEAN LOOPS_isAnInductionVariable (loop_t *loop, IR_ITEM_VALUE varID);
JITBOOLEAN LOOPS_isAnInvariantVariable (loop_t *loop, IR_ITEM_VALUE varID);
JITBOOLEAN LOOPS_hasASharedParentInductionVariable (loop_t *loop, IR_ITEM_VALUE varID1, IR_ITEM_VALUE varID2);
induction_variable_t * LOOPS_getInductionVariable (loop_t *loop, IR_ITEM_VALUE varID);
JITBOOLEAN LOOPS_isASubLoop (XanList *allLoops, loop_t *loop, loop_t *subloop);
JITBOOLEAN LOOPS_isASelfCalledLoop (XanList *allLoops, loop_t *loop);
JITBOOLEAN LOOPS_isAWhileLoop (ir_optimizer_t *irOptimizer, loop_t *loop);
JITBOOLEAN LOOPS_isAnExitInstruction (loop_t *loop, ir_instruction_t *inst);
XanList * LOOPS_getSuccessorsOfLoop (loop_t *loop);

/**
 *
 * @param loopsSuccessors Empty table, which will be filled up by this function. The hash table is <key, element> = <(loop_t *), (XanList *)>.
 * @param loopsPredecessors Empty table, which will be filled up by this function. The hash table is <key, element> = <(loop_t *), (XanList *)>.
 * @param loops List of (loop_t *)
 */
void LOOPS_getSuccessorsAndPredecessorsOfLoops (XanHashTable *loopsSuccessors, XanHashTable *loopsPredecessors, XanList *loops);
void LOOPS_destroySuccessorsAndPredecessorsOfLoops (XanHashTable *loopsSuccessors, XanHashTable *loopsPredecessors);

XanList * LOOPS_getCallInstructionsWithinLoop (loop_t *loop);
XanList * LOOPS_getBackedges (loop_t *loop);
XanList * LOOPS_getExitInstructions (loop_t *loop);
XanList * LOOPS_getMethods (XanList *loops);
XanList * LOOPS_getNotLibraryLoops (XanHashTable *loops);
loop_t * LOOPS_getLoopFromName (XanHashTable *loops, XanHashTable *loopsNames, JITINT8 *name);

/**
 *
 * 0: \c inst does not belong to any loop or sub-loop
 * 1: \c inst belongs directly to the loop given as input
 * 2: \c inst belongs to the direct sub-loop of the one given as input
 * n: \c inst belongs to a n-1 sub-loop of the loop given as input
 *
 * @return Nesting level of the instruction \c inst
 */
JITUINT32 LOOPS_getInstructionLoopNesting (loop_t *loop, ir_instruction_t *inst);
loop_t * LOOPS_getParentLoopWithinMethod (XanHashTable *loops, loop_t *loop);
void LOOPS_sortInstructions (ir_method_t *method, XanList *instructions, ir_instruction_t *header);

/****************** Loops profiling *************************************************************************/

/**
 *
 * @param loop Loop to consider
 * @return List of (loop_inter_interation_data_dependence_t *)
 */
XanList * LOOPS_profileDataDependenciesAcrossLoopIterations (loop_t *loop);

XanGraph * LOOPS_profileLoopsInvocationsForestTrees (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames);
void LOOPS_profileLoopsWithNamesByUsingSymbols (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, ir_symbol_t *newLoop, ir_symbol_t *endLoop, ir_symbol_t *newIteration, XanHashTable *loopsPredecessors, XanHashTable *loopsSuccessors, XanList *addedInstructions);
void LOOPS_profileLoopsWithNames (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, void (*newLoop)(JITINT8 *loopName), void (*endLoop)(void), void (*newIteration)(void), void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst));
void LOOPS_profileLoopsInvocationsIterationsAndInstructions (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst));
void LOOPS_profileLoopsInvocationsIterationsAndInstructionsEnd (void);

void LOOPS_profileLoopForDataDependences(ir_optimizer_t *irOptimizer, ir_method_t *parallelLoopMethod, ir_instruction_t *loopEntryPoint, JITINT8 *loopName, XanList *sequentialSegments);

/**
 *
 * @param loops List of (loop_t *)
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileLoopBoundaries (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, void (*newLoop) (loop_profile_t *loop), void (*endLoop) (void), void (*newIteration)(void), void (*addCodeFunction)(loop_t *loop, ir_instruction_t *afterInst));

/**
 *
 * @param loops List of (loop_t *)
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileLoops (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN noSystemLibraries, XanHashTable *loopsNames, JITUINT32 (*executionModel)(ir_method_t *method, ir_instruction_t *inst));

XanList * LOOPS_profileNestingLevelsOfLoops (ir_optimizer_t *irOptimizer, XanList *loops);

/**
 *
 *
 * @param loops List of (loop_t *)
 * @param runBaseline JITTRUE or JITFALSE
 * @param loopsBaseline NULL or a list of loops to not profile
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileOutermostLoops (ir_optimizer_t *irOptimizer, XanList *loops, JITBOOLEAN runBaseline, XanList *loopsBaseline);
void LOOPS_profileOutermostLoopsEnd (void);

/**
 *
 * @param loops List of (loop_t *)
 * @param loopsNames Hash table of loop names. The key is (loop_t *) and the value is (JITINT8 *).
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileLoopsIterations (ir_optimizer_t *irOptimizer, XanList *loops, XanHashTable *loopsNames);

/**
 *
 * @param loops List of (loop_t *)
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileDataForwardedAcrossLoopsIterations (ir_optimizer_t *irOptimizer, XanList *loops);

/**
 *
 * @param allLoops Every loop of the program
 * @param outermostLoops List of (loop_t *)
 * @param nestingLevel First nested level is 1
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileSubloopsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel);

XanList * LOOPS_profileTotalExecutionTimeOfSubloopsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel);
void LOOPS_profileTotalExecutionTimeOfSubloopsAtNestingLevelEnd (JITUINT32 nestingLevel);

/**
 *
 * @param allLoops Every loop of the program
 * @param outermostLoops List of (loop_t *)
 * @param nestingLevel First nested level is 1
 * @return List of (loop_profile_t *)
 */
XanList * LOOPS_profileSubloopsIterationsAtNestingLevel (ir_optimizer_t *irOptimizer, XanHashTable *allLoops, XanList *outermostLoops, JITUINT32 nestingLevel);

void LOOPS_profileLoopsIterationsEnd (void);
void LOOPS_profileLoopsEnd (void);

void LOOPS_profileDoPipeParallelism(ir_optimizer_t *irOptimizer, XanList *loops, XanHashTable *ddgs);
void LOOPS_profileDoPipeParallelismEnd(void);


/****************** Loops dumping *******************************************************************/
void LOOPS_dumpAllLoops (XanHashTable *loops, FILE *fileToWrite, char *programName);
void LOOPS_dumpLoops (XanHashTable *allLoops, XanList *loops, FILE *fileToWrite, char *programName);
void LOOPS_dumpLoopsInvocationsForestTrees (XanGraph *g, XanHashTable *loopsNames, JITINT8 *prefixName);
void LOOPS_dumpLoopsInvocationsForestTreesDotFile (XanGraph *g, XanHashTable *loopsNames, FILE *fileToWrite, JITBOOLEAN dumpEdges, void (*dumpNodeAnnotations) (loop_invocations_information_t *node, FILE *fileToWrite));


/****************** Loops transformations *******************************************************************/

/**
 *
 * Add the pre-header of the loop given as input.
 *
 * This new basic block is inserted even if another pre-header already exist.
 *
 * In this case, the new one is inserted just before the old one.
 *
 * The returned instruction is always a label (i.e., IRLABEL).
 *
 * The parameter <code> predecessors </code> can be NULL; in this case, they will be computed inside this function.
 *
 * @param loop Loop to consider
 * @predecessors Predecessors of the loop given as input
 * @return First instruction of the pre-header of the loop
 */
ir_instruction_t * LOOPS_addPreHeader (loop_t *loop, XanList *predecessors);
JITBOOLEAN LOOPS_transformToWhileLoop (ir_optimizer_t *irOptimizer, XanHashTable *loops, loop_t *loop);
void LOOPS_adjustLoopToHaveOneBackedge (loop_t *loop);
void LOOPS_transformToHeaderBodyLoop (ir_optimizer_t *irOptimizer, loop_t *loop);
void LOOPS_peelOutFirstIteration (loop_t *loop);

/****************** Data dependencies ***********************************************************************/

/**
 *
 * @param loop Loop to consider
 * @return List of (loop_inter_interation_data_dependence_t *)
 */
XanList * LOOPS_getDataDependencesAcrossLoopIterations (loop_t *loop, JITINT32 **predoms, JITINT32 **postdoms, XanHashTable ***depToCheckTable, JITUINT32 numInsts, XanHashTable *executedInsts);
XanList * LOOPS_getDataDependencesWithinLoop (loop_t *loop, XanList *loopCarriedDependences, JITINT32 **predoms, JITINT32 **postdoms, XanHashTable ***depToCheckTable, JITUINT32 numInsts, XanHashTable *executedInsts);
XanList * LOOPS_getDataDependenciesWithinMethod (ir_method_t *m);
XanHashTable ** LOOPS_getDDGFromDependences (XanList *deps, JITUINT32 numInsts);
void LOOPS_destroyDDG (XanHashTable **ddg, JITUINT32 numInsts);
loop_inter_interation_data_dependence_t * LOOPS_getDataDependence (XanList *depToCheck, ir_instruction_t *inst1, ir_instruction_t *inst2);
loop_inter_interation_data_dependence_t * LOOPS_getDirectDataDependence (XanList *depToCheck, ir_instruction_t *inst1, ir_instruction_t *inst2);
XanList * LOOPS_getRAWInstructionChainFromTheSameLoop (loop_t *loop, ir_instruction_t *startInst);

/**
 *
 * Let say that there are two instructions, inst1 and inst2, and we want to see whether inst1 depends on inst2 (i.e., the value generated by inst2 is used by inst1 in a later loop iteration).
 *
 * @code
 * XanHashTable *t;
 * t = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
 *
 * t2 = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
 * xanHashTable_insert(t, inst1, t2);
 *
 * depType = DEP_RAW | DEP_WAR;
 * l = xanList_new(allocFunction, freeFunction, NULL);
 * xanList_append(l, inst2);
 * xanHashTable_insert(t2, (void *)(JITNUINT)depType, l);
 *
 * LOOPS_keepOnlyLoopCarriedDataDependences(loop, predoms, postdoms, IRMETHOD_getInstructionsNumber(loop->method), t);
 * @endcode
 *
 * @param deps Each key, say k, is a (ir_instruction_t *); each element is (XanHashTable *). Each elementID of this last table is the dependence type, say depType (i.e., JITUINT16, for example DEP_RAW), and each element is (XanList *), which is the set of instructions (i.e., ir_instruction_t *) that the key k depends on with the type of dependence depType.
 */
void LOOPS_keepOnlyLoopCarriedDataDependences (loop_t *loop, JITINT32 **predoms, JITINT32 **postdoms, JITUINT32 numInsts, XanHashTable *deps);

void LOOPS_analyzeCircuits (ir_method_t *m);


/****************** Program ************************************************************************/
void PROGRAM_profileStartExecution (void (*startExecution) (void));
void PROGRAM_profileStartExecutionByUsingSymbol (ir_symbol_t *sym);
void PROGRAM_profileTotalExecutionTime (void);

/**
 *
 * @return Number of clock cycles spent within the program
 */
JITUINT64 PROGRAM_profileTotalExecutionTimeEnd (void);


/****************** Methods ************************************************************************/

/**
 *
 * @param minimumCallersNumber If zero, then this threshold is not considered
 */
JITBOOLEAN METHODS_inlineCalledMethods (ir_optimizer_t *irOptimizer, ir_method_t *method, XanList *callInstructions, void (*removeCalls)(ir_method_t *method, XanList *newCalls), JITUINT32 minimumCallersNumber);
JITBOOLEAN METHODS_inlineCalledMethodsNotInSubloops (ir_optimizer_t *irOptimizer, XanHashTable *loops, loop_t *loop, void (*removeCalls)(ir_method_t *method, XanList *newCalls));
JITBOOLEAN METHODS_inlineCalledMethodsNoRecursion (ir_optimizer_t *irOptimizer, ir_method_t *method, XanList *callInstructions, void (*removeCalls)(ir_method_t *method, XanList *newCalls));
ir_method_t * METHODS_getMainMethod (void);
void METHODS_addEmptyBasicBlocksJustBeforeInstructionsWithMultiplePredecessors (ir_method_t *method);

/**
 *
 * @param toConsider If it is not null, this function is invoked to check if the call instruction given as parameter should be considered on the creation of the call graph. Otherwise every call instruction is considered
 */
JITBOOLEAN METHODS_isReachableWithoutGoingThroughMethods (ir_method_t *startMethod, ir_method_t *endMethod, XanList *methodsToNotGoThrough, JITBOOLEAN (*toConsider)(ir_method_t *method, ir_instruction_t *callInst));

/**
 *
 * If the list pointed by <code> escapedMethods </code> is NULL, then this function will compute it internally.
 *
 * If the list pointed by <code> reachableMethodsFromEntryPoint </code> is NULL, then this function will compute it internally.
 *
 * @param inst Instruction to consider.
 * @param escapedMethods List of (ir_method_t *).
 * @param reachableMethodsFromEntryPoint List of (ir_method_t *).
 * @return List of IRMETHODID
 */
XanList * METHODS_getCallableMethods (ir_instruction_t *inst, XanList *escapedMethods, XanList *reachableMethodsFromEntryPoint);

/**
 * Get a list of IR methods callable by @c inst.  If @c escapedMethods is
 * non-NULL then it should be a list of (ir_method_t *) and will be used as the
 * list of escaped methods.  Otherwise this function will compute the list
 * internally.
 *
 * @param inst get all methods callable by this instruction.
 * @param escapedMethods A list of escaped methods, or NULL.
 * @return A list of methods (ir_method_t *) callable by @c inst.
 */
XanList * METHODS_getCallableIRMethods(ir_instruction_t *inst, XanList *escapedMethods);

/**
 * Get a list of methods callable by @c method.  If @c escapedMethods is
 * non-NULL then it should be a list of (ir_method_t *) and will be used as the
 * list of escaped methods.  Otherwise this function will compute the list
 * internally.
 *
 * @param method Get all methods callable by this method.
 * @param escapedMethods A list of escaped methods, or NULL.
 * @return A list of methods (ir_method_t *) callable by @c method.
 */
XanList * METHODS_getCallableIRMethodsFromMethod(ir_method_t *method, XanList *escapedMethods);

/**
 * Get a map between IR methods and lists of IR methods that they can call,
 * starting at the call graph rooted at @c startMethod.
 *
 * @param startMethod Build a map from the call graph rooted here.
 * @return A map between IR methods and lists of IR methods they can call.
 */
XanHashTable * METHODS_getCallableIRMethodsMap(ir_method_t *startMethod);

/**
 * Get a map between IR methods and lists of IR methods that can call them,
 * starting at the call graph rooted at @c startMethod.  If @c callMap is
 * non-NULL then the internally-built mapping from @c METHODS_getCallableIRMethodsMap
 * will be stored in it, not destroyed.
 *
 * @param startMethod Build a map from the call graph rooted here.
 * @param callMap The methods to callable methods mapping will be stored here, if non-NULL.
 * @return A map between IR methods and lists of IR methods that can call them.
 */
XanHashTable * METHODS_getCallableByIRMethodsMap(ir_method_t *startMethod, XanHashTable **callMap);

/**
 * Destroy a map between IR methods and lists of IR methods.
 *
 * @param callMap The map to destroy.
 */
void METHODS_destroyCallableIRMethodsMap(XanHashTable *callMap);

/**
 *
 * Translate every method that compose the program to machine code
 */
void METHODS_translateProgramToMachineCode (void);

/**
 *
 * @return Hash table where the key is the method and the value is the list of caller of that method
 */
XanHashTable * METHODS_getMethodsCallerGraph (JITBOOLEAN directCallsOnly);

void METHODS_destroyMethodsCallerGraph (XanHashTable *callerGraph);

/**
 * Get a list of strongly connected components within the call graph starting at @c method.
 *
 * @param method The method to start with.
 * @param includeLibraryCalls Whether to include library calls in the strongly connected components.
 * @return A list of strongly connected components (which are each a list of methods).
 */
XanList * METHODS_getStronglyConnectedComponentsList(ir_method_t *method, JITBOOLEAN includeLibraryCalls);

/**
 * Get a map between methods and the strongly connected component that they reside in within the call graph starting at @c method.
 *
 * @param method The method to start with.
 * @param includeLibraryCalls Whether to include library calls in the strongly connected components.
 * @return A map between each method and its strongly connected component (which is a list of methods).
 */
XanHashTable * METHODS_getStronglyConnectedComponentsMap(ir_method_t *method, JITBOOLEAN includeLibraryCalls);

/**
 * Destroy @c sccList that is a list of strongly-connected components.
 *
 * @param sccList The list of strongly-connected components.
 */
void METHODS_destroyStronglyConnectedComponentsList(XanList *sccList);

/**
 * Destroy @c sccMap that is a map between methods and the strongly-connected components that they reside in.
 *
 * @param sccMap The map between methods and their strongly-connected components.
 */
void METHODS_destroyStronglyConnectedComponentsMap(XanHashTable *sccMap);

/****************** Memory **********************************************************************************/
JITUINT64 MEMORY_getMemoryUsed (void);

/****************** Time ************************************************************************************/
JITUINT64 TIME_getClockTicks (void);
JITUINT64 TIME_getClockTicksBaseline (void);
JITFLOAT64 TIME_toSeconds (JITUINT64 deltaTicks, JITUINT32 clockFrequencyToUse);

/****************** Instructions ****************************************************************************/

/**
 * Add profiling code to a load or store instruction.
 **/
void INSTS_profileLoadOrStore(ir_method_t *method, ir_instruction_t *inst, void *nativeMethod);
void INSTS_addBasicInstructionProfileCall(ir_method_t *method, ir_instruction_t *inst, char *profileName, void *profileFunc, XanList *otherParams);


/****************** Instruction scheduling ******************************************************************/

/**
 * A structure holding everything need for scheduling.
 **/
typedef struct {
    ir_method_t *method;       /**< The method to be scheduled inside. */
    JITUINT32 numInsts;        /**< Cached number of instructions in the method. */
    JITUINT32 numBlocks;       /**< Cached number of basic blocks in the method. */
    ir_instruction_t **insts;
    XanList **predecessors;    /**< Cached predecessors of each method instruction. */
    XanList **successors;      /**< Cached successors of each method instruction. */
    XanList *loops;            /**< Loops within the method. */
    XanList *backEdges;        /**< Back edges for loops within the method. */
    XanBitSet **dataDepGraph;  /**< Data dependences between instructions within the method. */
    XanBitSet *regionInsts;    /**< The region of instructions to schedule instructions from. */
    XanBitSet *regionBlocks;   /**< The blocks containing the region of instructions. */
} schedule_t;

/**
 * An loop for scheduling.
 **/
typedef struct {
    JITUINT32 headerID;
    XanBitSet *exits;
    XanBitSet *loop;
} schedule_loop_t;

/**
 * An edge for scheduling.
 **/
typedef struct {
    ir_instruction_t *pred;
    ir_instruction_t *succ;
} schedule_edge_t;

schedule_t * SCHEDULE_initialiseScheduleForMethod(ir_method_t *method);
void SCHEDULE_freeSchedule(schedule_t *schedule);

/* XanList * SCHEDULE_findBackEdgesInMethod(ir_method_t *method, XanList **predecessors); */
/* XanList * SCHEDULE_findLoopsInMethod(ir_method_t *method, XanList **predecessors, XanList **successors); */
/* void SCHEDULE_freeBackEdgeList(XanList *edgeList); */
/* void SCHEDULE_freeLoopList(XanList *loopList); */

/* XanBitSet * SCHEDULE_findInstSetBetweenInstAndList(ir_method_t *method, ir_instruction_t *inst, XanList *instList, XanList *backEdges, XanList **predecessors, XanList **successors); */
/* XanBitSet * SCHEDULE_findInstSetBetweenTwoInstsNoLoops(ir_method_t *method, ir_instruction_t *first, ir_instruction_t *second, XanList *backEdges, XanList **predecessors, XanList **successors); */

void SCHEDULE_addContainedLoopsToInstRegion(schedule_t *schedule, XanBitSet *regionInsts);
void SCHEDULE_addRegionPreDominator(schedule_t *schedule, XanBitSet *regionInsts);
XanBitSet * SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule_t *schedule, ir_instruction_t *startInst);
XanBitSet * SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule_t *schedule, ir_instruction_t *startInst);
XanBitSet * SCHEDULE_findInstSetBetweenTwoInstsWithLoops(schedule_t *schedule, ir_instruction_t *first, ir_instruction_t *second);

XanBitSet * SCHEDULE_findInstsToCloneWhenMoving(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *outerInsts, JITBOOLEAN upwards, XanHashTable **reachableInstructions, XanNode **preDomTree, XanNode **postDomTree);
void SCHEDULE_filterInstsCantBeMoved(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *outerInsts, JITBOOLEAN upwards, JITBOOLEAN before);
XanBitSet * SCHEDULE_findInstsCantBeCloned(schedule_t *schedule, XanBitSet *instsToClone, XanBitSet *instsToMove, JITBOOLEAN upwards, JITBOOLEAN before);

/* XanBitSet * SCHEDULE_getInstsReachedUpwards(ir_method_t *method, ir_instruction_t *startInst, XanList **predecessors); */
/* XanBitSet * SCHEDULE_getInstsReachedDownwards (ir_method_t *method, ir_instruction_t *startInst, XanList **predecessors, XanList **successors); */

JITBOOLEAN SCHEDULE_moveInstructionsAfter(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *afterInsts, XanHashTable *clones);
JITBOOLEAN SCHEDULE_moveInstructionsBefore(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *beforeInsts, XanHashTable *clones);

JITBOOLEAN SCHEDULE_moveInstructionOnlyAsHighAsPossible(schedule_t *schedule, ir_instruction_t *instToMove, XanHashTable *clones);
JITBOOLEAN SCHEDULE_moveInstructionOnlyAsLowAsPossible(schedule_t *schedule, ir_instruction_t *instToMove, XanHashTable *clones);


/****************** Dominance ************************************************************************************/
void DOMINANCE_computePostdominance (ir_optimizer_t *irOptimizer, ir_method_t *method);


/****************** Misc ************************************************************************************/
void MISC_sortInstructions (ir_method_t *method, XanList *instructions);

/**
 *
 * The returned table contains all instructions listed in the dictionary stored in the file with name @c fileName .
 * Each element of the table has position number of the instruction as element and pointer to the (ir_instruction_t *) as key.
 *
 * If @c methodsThatIncludeInstructionsLoaded is not NULL, then this function add all methods that contain all instructions of the dictionary.
 */
XanHashTable * MISC_loadInstructionDictionary (XanHashTable *methodsThatIncludeInstructionsLoaded, char *fileName, XanHashTable **instIDs, XanHashTable **instructionToMethod);

/**
 *
 * The returned table contains all loops listed in the dictionary stored in the file with name @c fileName .
 * Each element of the table has position number of the instruction as element and pointer to the loop name string as key.
 * If @c loopDictionary is non-NULL then loops are added to this hash table, otherwise a new hash table is created.
 */
XanHashTable * MISC_loadLoopDictionary (char *fileName, XanHashTable *loopDictionary);

/**
 *
 * @param profiledLoops List of (loop_profile_t *)
 */
void MISC_sortProfiledLoops (XanList *profiledLoops);

void MISC_dumpString (FILE **fileToWrite, FILE *summaryFile, JITINT8 *buf, JITUINT32 *bytesDumped, JITUINT32 *suffixNumber, JITINT8 *prefixName, JITUINT64 maximumBytesPerFile, JITBOOLEAN splitPoint);

/**
 * Create a new hash table with strings as keys.
 **/
XanHashTable * MISC_newHashTableStringKey(void);

/**
 * Filter a list of loops.  If there is a *_enabled_loops file for the current
 * program then loops that are not in it are discarded.  By setting
 * CHIARA_FILTER_LOOP_ID a specific loop can be selected by its ID.  Otherwise,
 * loops from the *_enabled_loops file can be selected by using the environment
 * variables PARALLELIZER_LOWEST_LOOPS_TO_PARALLELIZE and
 * PARALLELIZER_MAXIMUM_LOOPS_TO_PARALLELIZE, as in HELIX.
 *
 * @param loopList The list of (loop_t *) loops to filter.
 * @param loopNames A mapping of (loop_t *) keys to string names of the loops.
 * @param loopDict A mapping of loop name string keys to IDs of the loops.
 **/
void MISC_chooseLoopsToProfile(XanList *loopList, XanHashTable *loopNames, XanHashTable *loopDict);

#endif
