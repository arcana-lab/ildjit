/*
 * Copyright (C) 2006 - 2012  Campanoni Simone
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
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>
#include <ildjit.h>

// My headers
#include <optimizer_deadcode.h>
#include <instructions_cleaning.h>
#include <program_deadcode.h>
#include <method_deadcode.h>
#include <config.h>
// End

#define INFORMATIONS    "This step check for each instruction, if it define a variable, which is not live out"
#define AUTHOR          "Simone Campanoni"

static inline JITUINT64 get_ID_job (void);
static inline void deadcode_do_job (ir_method_t *method);
static inline char * get_version (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline JITUINT64 get_dependences (void);
static inline JITUINT64 get_invalidations (void);
static inline void de_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void de_shutdown (JITFLOAT32 totalTime);
static inline void de_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void de_getCompilationFlags (char *buffer, JITINT32 bufferLength);

JITBOOLEAN	removeDeadMethods	= JITFALSE;

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    de_init,
    de_shutdown,
    deadcode_do_job,
    get_version,
    get_informations,
    get_author,
    get_invalidations,
    NULL,
    de_getCompilationFlags,
    de_getCompilationTime
};

static ir_lib_t        *irLib;
ir_optimizer_t  *irOptimizer;
static char            *irOutputPrefix;

static inline void de_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    char	*env;

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    irOutputPrefix = outputPrefix;

    env = getenv("DEADCODE_REMOVE_DEADMETHODS");
    if (env != NULL) {
        removeDeadMethods	= (JITBOOLEAN) atoi(env);
    }

}

static inline void de_shutdown (JITFLOAT32 totalTime) {

    irLib = NULL;
    irOptimizer = NULL;
    irOutputPrefix = NULL;
}

static inline JITUINT64 get_ID_job (void) {
    return DEADCODE_ELIMINATOR;
}

static inline JITUINT64 get_dependences (void) {
    return LIVENESS_ANALYZER | REACHING_DEFINITIONS_ANALYZER | ESCAPES_ANALYZER;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

static inline void deadcode_do_job (ir_method_t *method) {

    /* Assertions.
     */
    assert(method != NULL);

    /* Print the DEF, USE sets.
     */
#ifdef PRINTDEBUG
    PDEBUG("OPTIMIZER_DEADCODE: Sets\n");
    print_sets(method);
#endif

    /* Check the instructions number.
     */
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return;
    }

    /* Eliminate dead code.
     */
    eliminate_deadcode_within_method(method);

    /* Remove useless parameters.
     */
    erase_useless_parameters(method);

    /* Eliminate dead methods if we are running static compilation schemes.
     */
    if (	(removeDeadMethods)					&&
            (ILDJIT_compilationSchemeInUse() == staticScheme)	&&
            (IRPROGRAM_getEntryPointMethod() == method)		) {
        eliminate_deadmethods();
    }

    return;
}

static inline char * get_version (void) {
    return VERSION;
}

static inline char * get_informations (void) {
    return INFORMATIONS;
}

static inline char * get_author (void) {
    return AUTHOR;
}

static inline void de_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void de_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
