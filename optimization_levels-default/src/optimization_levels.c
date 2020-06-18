/*
 * Copyright (C) 2011 - 2012  Campanoni Simone
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
#include <assert.h>
#include <errno.h>
#include <ir_optimization_levels_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>
#include <ir_optimization_interface.h>

// My headers
#include <optimization_levels.h>
#include <config.h>
// End

#define INFORMATIONS    "This is the default optimization_levels plugin for the definition of the optimization levels of ILDJIT"
#define AUTHOR          "Simone Campanoni"
#define TOOLNAME	"default"

static inline JITUINT32 optimization_levels_optimizeMethodAtLevel (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimization_levels_optimizeMethodAOT (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimization_levels_optimizeMethodPostAOT (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline void optimization_levels_init (ir_optimizer_t *optimizer, JITINT8 *outputPrefix);
static inline void optimization_levels_shutdown (JITFLOAT32 totalTime);
static inline char * optimization_levels_get_version (void);
static inline char * optimization_levels_get_information (void);
static inline char * optimization_levels_get_author (void);
static inline char * optimization_levels_get_name (void);
static inline JITUINT32 optimizeMethod_O0_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_O1_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_O2_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_O3_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_O4_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_O5_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_O6_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline JITUINT32 optimizeMethod_aggressive_optimizations (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint);
static inline void optimization_levels_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void optimization_levels_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void internal_checkIR (void);

static ir_optimizer_t  *irOptimizer = NULL;
static JITINT8         *prefix = NULL;

static JITINT32 continuousOptimizations;
static JITINT32 customOptimization;
static JITINT32 aggressiveOptimizations;
static JITINT32 ddgBasedOptimizations;
static JITINT32 continuousOptimizationsSaveProgram;

ir_optimization_levels_interface_t plugin_interface = {
    optimization_levels_optimizeMethodAtLevel,
    optimization_levels_optimizeMethodAOT,
    optimization_levels_optimizeMethodPostAOT,
    optimization_levels_init,
    optimization_levels_shutdown,
    optimization_levels_get_version,
    optimization_levels_get_information,
    optimization_levels_get_author,
    optimization_levels_get_name,
    optimization_levels_getCompilationFlags,
    optimization_levels_getCompilationTime
};

static inline void optimization_levels_init (ir_optimizer_t *optimizer, JITINT8 *outputPrefix) {
    char	*var;

    /* Assertions			*/
    assert(optimizer != NULL);

    irOptimizer 				= optimizer;
    prefix 					= outputPrefix;
    continuousOptimizations			= JITFALSE;
    customOptimization			= JITFALSE;
    aggressiveOptimizations			= JITFALSE;
    ddgBasedOptimizations			= JITFALSE;
    continuousOptimizationsSaveProgram	= JITFALSE;

    var	= getenv("ILDJIT_ITERATIVE_OPTIMIZATIONS");
    if (var != NULL) {
        continuousOptimizations	= atoi(var);
    }

    var	= getenv("ILDJIT_ITERATIVE_OPTIMIZATIONS_SAVE_PROGRAM");
    if (var != NULL) {
        continuousOptimizationsSaveProgram	= atoi(var);
    }

    var	= getenv("ILDJIT_CUSTOM_OPTIMIZATION");
    if (var != NULL) {
        customOptimization	= atoi(var);
    }

    var	= getenv("ILDJIT_AGGRESSIVE_OPTIMIZATIONS");
    if (var != NULL) {
        aggressiveOptimizations	= atoi(var);
    }

    var	= getenv("ILDJIT_DDG_BASED_OPTIMIZATIONS");
    if (var != NULL) {
        ddgBasedOptimizations	= atoi(var);
    }

    return ;
}

static inline void optimization_levels_shutdown (JITFLOAT32 totalTime) {
    irOptimizer = NULL;
    prefix = NULL;
}

static inline char * optimization_levels_get_version (void) {
    return VERSION;
}

static inline char * optimization_levels_get_information (void) {
    return INFORMATIONS;
}

static inline char * optimization_levels_get_name (void) {
    return TOOLNAME;
}

static inline char * optimization_levels_get_author (void) {
    return AUTHOR;
}

static inline JITUINT32 optimization_levels_optimizeMethodAtLevel (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 optimizationLevel, JITUINT32 state, XanVar *checkPoint) {
    switch (optimizationLevel) {
        case 0:
            return optimizeMethod_O0_checkpointable(lib, method, state, checkPoint);
        case 1:
            return optimizeMethod_O1_checkpointable(lib, method, state, checkPoint);
        case 2:
            return optimizeMethod_O2_checkpointable(lib, method, state, checkPoint);
        case 3:
            return optimizeMethod_O3_checkpointable(lib, method, state, checkPoint);
        case 4:
            return optimizeMethod_O4_checkpointable(lib, method, state, checkPoint);
        case 5:
            return optimizeMethod_O5_checkpointable(lib, method, state, checkPoint);
        default:
            return optimizeMethod_O6_checkpointable(lib, method, state, checkPoint);
    }
    abort();
}

static inline JITUINT32 optimization_levels_optimizeMethodAOT (ir_optimizer_t *lib, ir_method_t *startMethod, JITUINT32 state, XanVar *checkPoint) {
    XanList *l;
    XanListItem *item;

    /* Assertions				*/
    assert(lib != NULL);
    assert(startMethod != NULL);

    /* Check if optimizations are enabled	*/
    if (lib->optimizationLevel == 0) {
        return JOB_END;
    }
    setenv("DEADCODE_REMOVE_DEADMETHODS", "0", JITTRUE);

    /* Fetch the set of methods		*/
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);

    /* Inline the methods			*/
    if (aggressiveOptimizations) {
        IROPTIMIZER_callMethodOptimization(lib, startMethod, METHOD_INLINER);

        item	= xanList_first(l);
        while (item != NULL) {
            ir_method_t *method;
            method = item->data;
            IROPTIMIZER_callMethodOptimization(lib, method, INDIRECT_CALL_ELIMINATION);
            item	= item->next;
        }
    }

    /* Run DDG.
     */
    if (ddgBasedOptimizations) {
        IROPTIMIZER_callMethodOptimization(lib, startMethod, DDG_COMPUTER);
    }

    /* Optimize methods					*/
    item = xanList_getElementFromPositionNumber(l, state);
    while (item != NULL) {
        ir_method_t *method;

        /* Fetch the method					*/
        method = item->data;
        assert(method != NULL);

        /* Check if we should apply aggressive optimizations	*/
        if (ddgBasedOptimizations) {

            /* Apply aggressive optimizations	*/
            if (	(IRMETHOD_hasMethodInstructions(method))	&&
                    (!IRPROGRAM_doesMethodBelongToALibrary(method))	) {
                optimizeMethod_aggressive_optimizations(lib, method, state, checkPoint);
            }
        }

        /* Optimize the code                    */
        if (optimization_levels_optimizeMethodAtLevel(lib, method, lib->optimizationLevel, JOB_START, checkPoint) != JOB_END) {
            state = JOB_START;
            break;
        }

        /* Fetch the next element				*/
        item = item->next;
    }

    /* Remove dead methods.
     */
    setenv("DEADCODE_REMOVE_DEADMETHODS", "1", JITTRUE);
    IROPTIMIZER_callMethodOptimization(lib, startMethod, DEADCODE_ELIMINATOR);
    setenv("DEADCODE_REMOVE_DEADMETHODS", "0", JITTRUE);

    /* Free the memory			*/
    xanList_destroyList(l);

    return state;
}

static inline JITUINT32 optimization_levels_optimizeMethodPostAOT (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions				*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Apply the custom optimization	*/
    if (customOptimization) {

        /* Run the CUSTOM optimization		*/
        IROPTIMIZER_callMethodOptimization(lib, method, CUSTOM);
    }

    /* Check if the continuous optimizations is enabled	*/
    if (continuousOptimizations) {

        /* Optimize the code			*/
        optimization_levels_optimizeMethodAOT(lib, method, state, checkPoint);
    }

    /* Dump the IR code.
     */
    if (lib->verbose) {
        XanList		*l;
        XanListItem	*item;

        /* Fetch the set of methods.
         */
        l = IRPROGRAM_getIRMethods();
        assert(l != NULL);

        /* Dump the CFG.
         */
        item = xanList_first(l);
        while (item != NULL) {
            ir_method_t *method;

            /* Fetch the method.
             */
            method = item->data;
            assert(method != NULL);

            /* Print the CFG.
             */
            if (IRPROGRAM_doesMethodBelongToALibrary(method)) {
                setenv("DOTPRINTER_FILENAME", "Library_methods", JITTRUE);
            } else {
                setenv("DOTPRINTER_FILENAME", "Program_methods", JITTRUE);
            }
            IROPTIMIZER_callMethodOptimization(lib, method, METHOD_PRINTER);

            /* Fetch the next element.
             */
            item	= item->next;
        }

        /* Free the memory			*/
        xanList_destroyList(l);
    }

    /* Check the IR code.
     */
    if (lib->checkProducedIR) {
        internal_checkIR();
    }

    /* Save the code.
     */
    if (continuousOptimizationsSaveProgram) {
        IRPROGRAM_saveProgram(NULL);
    }

    return JOB_END;
}

static inline JITUINT32 optimizeMethod_O0_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions                                           */
    assert(lib != NULL);
    assert(method != NULL);

    /* Return                                               */
    return JOB_END;
}

static inline JITUINT32 optimizeMethod_O1_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions				*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Optimize the method                  */
    optimizeMethod_O0_checkpointable(lib, method, state, checkPoint);

    /* Constant propagation and constant folding            */
    IROPTIMIZER_callMethodOptimization(lib, method, CONSTANT_PROPAGATOR);

    /* Copy propagation                                     */
    IROPTIMIZER_callMethodOptimization(lib, method, COPY_PROPAGATOR);

    /* Dead code elimination                                */
    IROPTIMIZER_callMethodOptimization(lib, method, DEADCODE_ELIMINATOR);

    /* Common sub-expression elimination			*/
    IROPTIMIZER_callMethodOptimization(lib, method, COMMON_SUBEXPRESSIONS_ELIMINATOR);

    /* Remove the unusefull null check instructions         */
    IROPTIMIZER_callMethodOptimization(lib, method, NULL_CHECK_REMOVER);

    /* Instruction rescheduler                              */
    IROPTIMIZER_callMethodOptimization(lib, method, INSTRUCTIONS_SCHEDULER);

    /* Algebraic simplification                             */
    IROPTIMIZER_callMethodOptimization(lib, method, ALGEBRAIC_SIMPLIFICATION);

    /* Update the max variables				*/
    IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

    return JOB_END;
}

static inline JITUINT32 optimizeMethod_O2_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions				*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Reset the flags		*/
    method->modified = JITFALSE;

    /* Optimize the code at O1	*/
    optimizeMethod_O1_checkpointable(lib, method, state, checkPoint);

    /* Remove escapes		*/
    IROPTIMIZER_callMethodOptimization(lib, method, ESCAPES_ELIMINATION);

    /* Reduce the conversion	*
     * instructions			*/
    IROPTIMIZER_callMethodOptimization(lib, method, CONVERSION_MERGING);

    /* Rename the variables		*/
    IROPTIMIZER_callMethodOptimization(lib, method, VARIABLES_RENAMING);

    /* Inline the native methods	*/
    IROPTIMIZER_callMethodOptimization(lib, method, NATIVE_METHODS_ELIMINATION);

    /* Move loop invariants		*/
//	IROPTIMIZER_callMethodOptimization(lib, method, LOOP_INVARIANT_CODE_HOISTING);

    /* Optimize the code at O1	*/
    optimizeMethod_O1_checkpointable(lib, method, state, checkPoint);

    return JOB_END;
}

static inline JITUINT32 optimizeMethod_O3_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions.
     */
    assert(lib != NULL);
    assert(method != NULL);

    /* Apply simple optimizations first.
     */
    while (1) {

        /* Reset the flags.
         */
        method->modified = JITFALSE;

        /* Optimize the code.
         */
        IROPTIMIZER_callMethodOptimization(lib, method, CONSTANT_PROPAGATOR);
        IROPTIMIZER_callMethodOptimization(lib, method, COPY_PROPAGATOR);
        IROPTIMIZER_callMethodOptimization(lib, method, DEADCODE_ELIMINATOR);

        /* Check the exit condition.
         */
        if (method->modified == JITFALSE) {
            break;
        }
        if (	(checkPoint != NULL)					&&
                ((JITBOOLEAN) (JITNUINT) xanVar_read(checkPoint))	) {
            return JOB_START;
        }
    }

    /* Apply all optimizations.
     */
    while (1) {

        /* Reset the flags.
         */
        method->modified = JITFALSE;

        /* Optimize the code.
         */
        optimizeMethod_O2_checkpointable(lib, method, state, checkPoint);

        /* Check the exit condition.
         */
        if (method->modified == JITFALSE) {
            state = JOB_END;
            break;
        }
        if (	(checkPoint != NULL)					&&
                ((JITBOOLEAN) (JITNUINT) xanVar_read(checkPoint))	) {
            state = JOB_START;
            break;
        }
    }

    return state;
}

static inline JITUINT32 optimizeMethod_O4_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions				*/
    assert(method != NULL);

    /* Run the previous level		*/
    optimizeMethod_O3_checkpointable(lib, method, state, checkPoint);

    /* Run the custom plugin		*/
    IROPTIMIZER_callMethodOptimization(lib, method, CUSTOM);

    /* Return				*/
    return JOB_END;
}

static inline JITUINT32 optimizeMethod_O5_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {
    JITUINT32 instructionsNumber;
    JITUINT32 vars;
    JITUINT32 diff;

    /* Assertions				*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Optimize the method                  */
    while (1) {

        /* Fetch the number of IR instructions			*/
        instructionsNumber = IRMETHOD_getInstructionsNumber(method);
        vars = IRMETHOD_getNumberOfVariablesNeededByMethod(method);

        optimizeMethod_O4_checkpointable(lib, method, state, checkPoint);

        /* Check the exit condition				*/
        diff = abs(  abs(IRMETHOD_getInstructionsNumber(method) - instructionsNumber)         +
                     abs(IRMETHOD_getNumberOfVariablesNeededByMethod(method) - vars)          );
        if (diff == 0) {
            state = JOB_END;
            break;
        }

        if (	(checkPoint != NULL)					&&
                ((JITBOOLEAN) (JITNUINT) xanVar_read(checkPoint))	) {
            state = JOB_START;
            break;
        }
    }

    /* Return				*/
    return state;
}

static inline JITUINT32 optimizeMethod_O6_checkpointable (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {
    JITUINT32 instructionsNumber;
    JITUINT32 vars;
    JITUINT32 diff;

    /* Assertions				*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Optimize the method                  */
    while (1) {

        /* Fetch the number of IR instructions			*/
        instructionsNumber = IRMETHOD_getInstructionsNumber(method);
        vars = IRMETHOD_getNumberOfVariablesNeededByMethod(method);

        optimizeMethod_O5_checkpointable(lib, method, state, checkPoint);

        /* Check the exit condition				*/
        diff = abs(  abs(IRMETHOD_getInstructionsNumber(method) - instructionsNumber)         +
                     abs(IRMETHOD_getNumberOfVariablesNeededByMethod(method) - vars)          );
        if (diff == 0) {
            state = JOB_END;
            break;
        }

        if (	(checkPoint != NULL)					&&
                ((JITBOOLEAN) (JITNUINT) xanVar_read(checkPoint))	) {
            state = JOB_START;
            break;
        }
    }

    /* Return				*/
    return state;
}

static inline JITUINT32 optimizeMethod_aggressive_optimizations (ir_optimizer_t *lib, ir_method_t *method, JITUINT32 state, XanVar *checkPoint) {

    /* Assertions				*/
    assert(lib != NULL);
    assert(method != NULL);

    /* Optimize the method 		      */
    while (1) {

        /* Reset the flags			*/
        method->modified = JITFALSE;

        /* Apply the aggressive optimizations	*/
        IROPTIMIZER_callMethodOptimization(lib, method, COMMON_SUBEXPRESSIONS_ELIMINATOR);

        /* Check the exit condition		*/
        if (method->modified == JITFALSE) {
            state = JOB_END;
            break;
        }
        if (	(checkPoint != NULL)					&&
                ((JITBOOLEAN) (JITNUINT) xanVar_read(checkPoint))	) {
            state = JOB_START;
            break;
        }

        /* The code has been modified.
         * We need to re-analize the DDG.
         */
        IROPTIMIZER_callMethodOptimization(lib, method, DDG_COMPUTER);
    }

    return state;
}

static inline void optimization_levels_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void optimization_levels_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}

static inline void internal_checkIR (void) {
    XanListItem	*item;
    XanList		*l;

    /* Fetch the set of methods.
     */
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);

    /* Check the methods.
     */
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t *method;

        /* Fetch the method					*/
        method = item->data;
        assert(method != NULL);

        /* Check the code					*/
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, IR_CHECKER);

        /* Fetch the next element				*/
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(l);

    return ;
}
