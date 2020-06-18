/*
 * Copyright (C) 2009 - 2012  Campanoni Simone
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
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>
#include <chiara.h>

// My headers
#include <optimizer_methodinliner.h>
#include <inline_heuristics.h>
#include <inline_algorithm.h>
#include <inline_printer.h>
#include <inliner_configuration.h>
#include <config.h>
// End

#define INFORMATIONS    "This is a methodinliner plugin"
#define AUTHOR          "Campanoni Simone"

static inline JITUINT64 methodinliner_get_ID_job (void);
static inline char * methodinliner_get_version (void);
static inline void methodinliner_do_job (ir_method_t * method);
static inline char * methodinliner_get_informations (void);
static inline char * methodinliner_get_author (void);
static inline JITUINT64 methodinliner_get_dependences (void);
static inline void methodinliner_shutdown (JITFLOAT32 totalTime);
static inline void methodinliner_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 methodinliner_get_invalidations (void);
static inline void methodinliner_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void methodinliner_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t        	*irLib = NULL;
ir_optimizer_t  	*irOptimizer = NULL;
char            	*prefix = NULL;
JITUINT64 		threshold_insns_with_loops		= 0;
JITUINT64 		threshold_insns_without_loops		= 5000;
JITUINT64		threshold_caller_insns			= 50000;
JITUINT64		threshold_program_insns			= 500000;			// One instruction takes almost 100 Bytes. 10000000 takes almost 1 GB
JITBOOLEAN 		disableLibraries			= JITTRUE;
JITBOOLEAN		disableNotWorthFunctions		= JITTRUE;
JITBOOLEAN		avoidMethodsNotIncludedInLoops		= JITTRUE;

static JITBOOLEAN 	dumpFinalCallGraph 	= JITFALSE;

ir_optimization_interface_t plugin_interface = {
    methodinliner_get_ID_job,
    methodinliner_get_dependences,
    methodinliner_init,
    methodinliner_shutdown,
    methodinliner_do_job,
    methodinliner_get_version,
    methodinliner_get_informations,
    methodinliner_get_author,
    methodinliner_get_invalidations,
    NULL,
    methodinliner_getCompilationFlags,
    methodinliner_getCompilationTime
};

static inline void methodinliner_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    char	*env;

    /* Assertions.
     */
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    /* Initialize the global variables.
     */
    irLib 		= lib;
    irOptimizer 	= optimizer;
    prefix 		= outputPrefix;

    /* Configure the plugin.
     */
    env = getenv("INLINER_DUMP_CALL_GRAPH");
    if (env != NULL) {
        dumpFinalCallGraph = (JITBOOLEAN) atoi(env);
    }
    env = getenv("INLINER_DISABLE_LIBRARIES");
    if (env != NULL) {
        disableLibraries = (JITBOOLEAN) atoi(env);
    }
    env = getenv("INLINER_TH_INSNS_WITH_LOOPS");
    if (env != NULL) {
        threshold_insns_with_loops = (JITUINT64) atoi(env);
    }
    env = getenv("INLINER_TH_INSNS_WITHOUT_LOOPS");
    if (env != NULL) {
        threshold_insns_without_loops = (JITUINT64) atoi(env);
    }
    env = getenv("INLINER_TH_CALLER_INSNS");
    if (env != NULL) {
        threshold_caller_insns		= (JITUINT64) atoi(env);
    }
    env = getenv("INLINER_TH_PROGRAM_INSNS");
    if (env != NULL) {
        threshold_program_insns = (JITUINT64) atoi(env);
    }
    env = getenv("INLINER_AVOID_METHODS_NOT_IN_LOOPS");
    if (env != NULL) {
        avoidMethodsNotIncludedInLoops		= (JITBOOLEAN)atoi(env);
    }

    return ;
}

static inline void methodinliner_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

static inline JITUINT64 methodinliner_get_ID_job (void) {
    return METHOD_INLINER;
}

static inline JITUINT64 methodinliner_get_dependences (void) {
    return 0;
}

static inline void methodinliner_do_job (ir_method_t *method) {
    static JITINT32 	inUse = JITFALSE;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if the plugin is in use.
     */
    if (inUse) {
        return;
    }

    /* Record that we are using the plugin.
     */
    inUse = JITTRUE;

    /* Dump the call graph.
     */
    if (dumpFinalCallGraph) {
        dump_call_graph("before_");
    }

    /* Run the inline algorithm.
     */
    inline_methods();

    /* Dump the call graph.
     */
    if (dumpFinalCallGraph) {
        dump_call_graph("after_");
    }

    /* Unrecord that we are using the plugin.
     */
    inUse = JITFALSE;

    return;
}

static inline char * methodinliner_get_version (void) {
    return VERSION;
}

static inline char * methodinliner_get_informations (void) {
    return INFORMATIONS;
}

static inline char * methodinliner_get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 methodinliner_get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void methodinliner_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void methodinliner_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
