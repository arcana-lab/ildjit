/*
 * Copyright (C) 2008 - 2012 Campanoni Simone
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
/**
 * @file ir_optimizer.h
 */
#ifndef IR_OPTIMIZER_H
#define IR_OPTIMIZER_H

#include <xanlib.h>
#include <jitsystem.h>
#include <ir_method.h>

typedef struct {
    JITBOOLEAN	ddg;				/**< DDG analysis					*/
    JITBOOLEAN	cdg;				/**< CDG analysis					*/
    JITBOOLEAN	varLiv;				/**< Variable liveness analysis				*/
    JITBOOLEAN	reachExpr;			/**< Reaching expression analysis			*/
    JITBOOLEAN	reachDefs;			/**< Reaching definition analysis			*/
    JITBOOLEAN	avExpr;				/**< Available expression analysis			*/
    JITBOOLEAN 	deadce;                         /**< Dead code elimination                              */
    JITBOOLEAN	irCheck;			/**< Checking the IR code				*/
    JITBOOLEAN	loopInvariantsIdent;		/**< Identification of loop invariants			*/
    JITBOOLEAN	methodPrinter;			/**< Printer of code inside methods			*/
    JITBOOLEAN	scal;				/**< Scalarization					*/
    JITBOOLEAN 	cse;                            /**< Common Subexpression Elimination			*/
    JITBOOLEAN 	consprop;                       /**< Constant propagation				*/
    JITBOOLEAN	inductionVars;			/**< Induction variables identification			*/
    JITBOOLEAN	bb;				/**< Basic blocks identification			*/
    JITBOOLEAN	escapes;			/**< Variable escapes analysis				*/
    JITBOOLEAN 	sched;                          /**< Instruction scheduling                             */
    JITBOOLEAN	branchPredictor;		/**< Branch prediction analysis				*/
    JITBOOLEAN 	nullcheck;                      /**< Remove unusefull checks of null pointers           */
    JITBOOLEAN	post;				/**< Post-dominators analysis				*/
    JITBOOLEAN	pre;				/**< Pre-dominators analysis				*/
    JITBOOLEAN	loopUnroll;			/**< Loop unrolling					*/
    JITBOOLEAN	loopId;				/**< Identification of loops				*/
    JITBOOLEAN	loopInvCodeHoisting;		/**< Loop invariant code hoisting			*/
    JITBOOLEAN	loopInvVars;			/**< Loop invariant variables elimination		*/
    JITBOOLEAN	loopUnswitch;			/**< Loop unswitching					*/
    JITBOOLEAN	loopWhile;			/**< Transforming loops to while			*/
    JITBOOLEAN	loopPeeling;			/**< Loop peeling					*/
    JITBOOLEAN	boundsRem;			/**< Array bounds check remotion			*/
    JITBOOLEAN	strengthReduction;		/**< Strength reduction					*/
    JITBOOLEAN 	algebraic;                      /**< Algebraic simplification                           */
    JITBOOLEAN 	copyprop;                       /**< Copy propagation					*/
    JITBOOLEAN 	inlinemethod;                   /**< Method inlining					*/
    JITBOOLEAN 	variablesrenaming;              /**< Variables renaming					*/
    JITBOOLEAN	varsLiveSplit;			/**< Variables live range splitting			*/
    JITBOOLEAN	indirectCalls;			/**< Indirect calls elimination				*/
    JITBOOLEAN 	nativeinlinemethod;             /**< Native method inlining				*/
    JITBOOLEAN	dumpCode;			/**< Dump the code					*/
    JITBOOLEAN	toSSA;				/**< To SSA						*/
    JITBOOLEAN	fromSSA;			/**< From SSA						*/
    JITBOOLEAN	callDistance;			/**< Call distance analysis				*/
    JITBOOLEAN 	convmerging;                    /**< Conversion merging					*/
    JITBOOLEAN 	escapesRemove;                  /**< Escapes optimization				*/
    JITBOOLEAN	threadsExtraction;		/**< Extraction of threads				*/
    JITBOOLEAN 	cfold;                          /**< Constant folding					*/
    JITBOOLEAN 	ddgprofile;                     /**< DDG profile					*/
    JITBOOLEAN 	reachInst;                      /**< Reaching instructions				*/
    JITBOOLEAN 	loopProfile;                    /**< Loop profile					*/
    JITBOOLEAN 	custom;                         /**< Custom						*/
    JITINT8 	*optimizationsList;             /**< Input given by the user				*/
    JITUINT32	optimizationsListSize;		/**< Size of memory allocated for optimizationsList	*/
} optimizations_t;

typedef struct ir_optimizer_t {
    XanList                	 	*plugins;                       /**< List of (ir_optimization_t *)		*/
    XanList                	 	*optimizationLevelsPlugins;     /**< List of (ir_optimization_levels_t *)	*/
    JITINT32 			verbose;
    JITUINT32 			optimizationLevel;
    XanList                	 	*methods;
    optimizations_t 		enabledOptimizations;
    optimizations_t 		disabledOptimizations;
    JITBOOLEAN 			checkProducedIR;
    JITBOOLEAN 			enableMachineDependentOptimizations;
    XanHashTable			*jobCodeToolsMap;		/**< Map from the codetool type to the codetool that provides it. Each element is (ir_optimization_t *)			*/
    char 				*optLevelToolName;
} ir_optimizer_t;

#ifdef __cplusplus
extern "C" {
#endif

void IROptimizerNew (ir_optimizer_t *lib, ir_lib_t *irLib, JITINT32 verbose, JITUINT32 optimizationLevel, char *outputPrefix);
void IROPTIMIZER_startExecution (ir_optimizer_t *lib);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method
 *
 * Run every plugins the optimizer decides to use on the IR method <code> method </code> as input.
 *
 * @param lib IR optimizer
 * @param method IR method to optimize
 */
void IROPTIMIZER_optimizeMethod (ir_optimizer_t *lib, ir_method_t *method);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method
 *
 * Run every plugins the optimizer decides to use on the IR method <code> method </code> as input.
 * This version of method is checkpointable through checkPoint guard
 *
 * @param lib IR optimizer
 * @param method IR method to optimize
 * @param state Status of the optimization
 * @param checkPoint checkpoint guard
 */
JITUINT32 IROPTIMIZER_optimizeMethod_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method
 *
 * Run every plugins defined within the optimization level \c optimizationLevel using the IR method <code> method </code> as input.
 *
 * @param lib IR optimizer
 * @param method IR method to optimize
 * @param optimizationLevel Optimization level to use
 */
void IROPTIMIZER_optimizeMethodAtLevel (ir_optimizer_t *lib, ir_method_t *method, JITINT32 optimizationLevel);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method
 *
 * Run every plugins defined within the optimization level \c optimizationLevel using the IR method <code> method </code> as input.
 * This version of method is checkpointable through checkPoint guard
 *
 * @param lib IR optimizer
 * @param method IR method to optimize
 * @param optimizationLevel Optimization level to use
 * @param state Status of the optimization
 * @param checkPoint checkpoint guard
 */
JITUINT32 IROPTIMIZER_optimizeMethodAtLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint);

/**
 * \ingroup InvokeCodetools
 * @brief Call an optimization
 *
 * Run the plugin installed within the system that is able to perform the optimization specified by the parameter \c optimizationKind.
 *
 * @param lib IR optimizer
 * @param method IR method to optimize
 * @param optimizationKind Optimization kind to perform
 */
void IROPTIMIZER_callMethodOptimization (ir_optimizer_t *lib, ir_method_t *method, JITUINT64 optimizationKind);

/**
 * \ingroup IROptimizerInterface
 * @brief Check whether an information is valid
 *
 * Check whether an information is valid or not.
 *
 * For example, the following code checks if the information about loops of a given method is valid:
 * @code
 * if (IROPTIMIZER_isInformationValid(method, LOOP_IDENTIFICATION)){
 *    printf("Information about loops is valid.");
 * }
 * @endcode
 *
 * @param method IR method to consider
 * @param codetoolKind Type of information to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IROPTIMIZER_isInformationValid (ir_method_t *method, JITUINT64 codetoolKind);

/**
 * \ingroup IROptimizerInterface
 * @brief Set an information as valid
 *
 * Set an information to be valid for a method.
 *
 * For example
 * @code
 *
 * // Notify that loops have been identified
 * IROPTIMIZER_setInformationAsValid(method, LOOP_IDENTIFICATION);
 *
 * // Assert the information about loops is valid
 * assert(IROPTIMIZER_isInformationValid(method, LOOP_IDENTIFICATION));
 * @endcode
 *
 * @param method IR method to consider
 * @param codetoolKind Type of information to consider
 */
void IROPTIMIZER_setInformationAsValid (ir_method_t *method, JITUINT64 codetoolKind);

/**
 * \ingroup IROptimizerInterface
 * @brief Invalidate a given information
 *
 * Invalidate a given information.
 *
 * For example, the following code will not abort:
 * @code
 * IROPTIMIZER_invalidateInformation(method, LOOP_IDENTIFICATION);
 * assert(!IROPTIMIZER_isInformationValid(method, LOOP_IDENTIFICATION));
 * @endcode
 *
 * @param method IR method to consider
 * @param codetoolKind Type of information to invalidate
 */
void IROPTIMIZER_invalidateInformation (ir_method_t *method, JITUINT64 codetoolKind);

/**
 * \ingroup IROptimizerInterface
 * @brief Check whether a code tool can be used
 *
 * Check whether a code tool is installed in the system and it has been enabled.
 *
 * For example, the following code checks if the information about loops can be computed by invoking the \ref LOOP_IDENTIFICATION code tool.
 * @code
 * if (!IROPTIMIZER_canCodeToolBeUsed(irOptimizer, LOOP_IDENTIFICATION)){
 *    printf("Information about loops cannot be computed.\n");
 *    if (!IROPTIMIZER_isCodeToolInstalled(irOptimizer, LOOP_IDENTIFICATION)){
 *       printf("The plugin %s has not been installed.\n", IROPTIMIZER_jobToString(LOOP_IDENTIFICATION));
 *    } else {
 *       printf("The plugin %s has been installed, but it has been disabled.\n", IROPTIMIZER_jobToString(LOOP_IDENTIFICATION));
 *    }
 * }
 * @endcode
 *
 * @param lib IR optimizer library
 * @param codetoolKind Type of information to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IROPTIMIZER_canCodeToolBeUsed (ir_optimizer_t *lib, JITUINT64 codetoolKind);

/**
 * \ingroup IROptimizerInterface
 * @brief Check whether a code tool has been installed
 *
 * Check whether a code tool has been installed in the system.
 *
 * For example, the following code checks whether the code tool to identify loops has been installed in the system:
 * @code
 * if (IROPTIMIZER_isCodeToolInstalled(irOptimizer, LOOP_IDENTIFICATION)){
 *       printf("The plugin %s has not been installed.\n", IROPTIMIZER_jobToString(LOOP_IDENTIFICATION));
 * }
 * @endcode
 *
 * @param lib IR optimizer library
 * @param codetoolKind Type of information to check
 * @return JITTRUE or JITFALSE
 */
JITBOOLEAN IROPTIMIZER_isCodeToolInstalled (ir_optimizer_t *lib, JITUINT64 codetoolKind);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method with AOT code optimizations pre serialization
 *
 * Run every plugins used only in AOT execution mode (too expensive for JIT compilation).
 *
 * @param lib IR optimizer
 * @param startMethod Entry point of the program
 */
void IROPTIMIZER_optimizeMethodAtAOTLevel (ir_optimizer_t *lib, ir_method_t *startMethod);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method with AOT code optimizations pre serialization
 *
 * Run every plugins used only in AOT execution mode (too expensive for JIT compilation).
 * This version of method is checkpointable through checkPoint guard
 *
 * @param lib IR optimizer
 * @param startMethod Entry point of the program
 * @param state Status of the optimization
 * @param checkPoint checkpoint guard
 */
JITUINT32 IROPTIMIZER_optimizeMethodAtAOTLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method with AOT code optimizations after serialization
 *
 * Run every plugins used only in AOT execution mode (too expensive for JIT compilation).
 *
 * @param lib IR optimizer
 * @param method IR method to optimize
 */
void IROPTIMIZER_optimizeMethodAtPostAOTLevel (ir_optimizer_t *lib, ir_method_t *method);

/**
 * \ingroup InvokeCodetools
 * @brief Optimize a IR method with AOT code optimizations after serialization
 *
 * Run every plugins used only in AOT execution mode (too expensive for JIT compilation).
 * This version of method is checkpointable through checkPoint guard
 *
 * @param lib IR optimizer
 * @param startMethod Entry point of the program
 * @param state Status of the optimization
 * @param checkPoint checkpoint guard*
 */
JITUINT32 IROPTIMIZER_optimizeMethodAtPostAOTLevel_checkpointable (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint);

/**
 * \ingroup IROptimizerInterface
 * @brief Get the paths where codetools are installed
 *
 * Return the list of paths where ILDJIT found codetools
 *
 * @return List of paths (e.g. PATH1:PATH2:PATH3)
 */
JITINT8 * IROPTIMIZER_getCodetoolPaths (void);

/**
 * \ingroup IROptimizerInterface
 * @brief Get the paths where optimizationleveltools are installed
 *
 * Return the list of paths where ILDJIT found optimizationleveltools.
 *
 * @return List of paths (e.g. PATH1:PATH2:PATH3)
 */
JITINT8 * IROPTIMIZER_getOptimizationleveltoolPaths (void);

/**
 * \ingroup IROptimizerInterface
 * @brief Get the string version of a job type
 *
 * Return the string version of the job type <code> job </code>.
 *
 * @param job Type of codetool
 * @return String version of <code> job </code>
 */
char * IROPTIMIZER_jobToString (JITUINT64 job);

/**
 * \ingroup IROptimizerInterface
 * @brief Get the name of a job type
 *
 * Return the name of the job type <code> job </code>.
 *
 * This name can be used to identify codetools by command line.
 *
 * @param job Type of codetool
 * @return String version of <code> job </code>
 */
char * IROPTIMIZER_jobToShortName (JITUINT64 job);

/**
 * \ingroup IROptimizerInterface
 * @brief Shutdown
 *
 * Shutdown the codetools manager
 *
 * @param lib Library
 * @param totalTime Time spent at runtime
 */
void IROPTIMIZER_shutdown (ir_optimizer_t *lib, JITFLOAT32 totalTime);

/**
 *
 * Return the version of the Libiljitiroptimizer library
 */
char * libiljitiroptimizerVersion ();

/**
 *
 * Write the compilation flags to the buffer given as input.
 */
void libiljitiroptimizerCompilationFlags (char *buffer, JITINT32 bufferLength);

/**
 *
 * Write the compilation time to the buffer given as input.
 */
void libiljitiroptimizerCompilationTime (char *buffer, JITINT32 bufferLength);

#ifdef __cplusplus
};
#endif

#endif
