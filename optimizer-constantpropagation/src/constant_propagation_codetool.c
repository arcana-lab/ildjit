/*
 * Copyright (C) 2008 - 2012  Campanoni Simone
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
#include <math.h>
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <constant_propagation_codetool.h>
#include <constant_propagation.h>
#include <constant_folding.h>
#include <config.h>
#include <misc.h>
// End

#define INFORMATIONS    "This optimization step performs the constant propagation algorithm"
#define AUTHOR          "Simone Campanoni"
#define JOB             CONSTANT_PROPAGATOR

static inline void cp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void cp_shutdown (JITFLOAT32 totalTime);
static inline JITUINT64 get_ID_job (void);
static inline void cons_do_job (ir_method_t *method);
static inline char * cons_get_version (void);
static inline JITUINT64 get_dependences (void);
static inline JITUINT64 get_invalidations (void);
static inline char * cons_get_informations (void);
static inline char * cons_get_author (void);
static inline void cons_propagation_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void cons_propagation_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    cp_init,
    cp_shutdown,
    cons_do_job,
    cons_get_version,
    cons_get_informations,
    cons_get_author,
    get_invalidations,
    NULL,
    cons_propagation_getCompilationFlags,
    cons_propagation_getCompilationTime
};

static inline void cp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);

    irLib = lib;
    irOptimizer = optimizer;
}

static inline void cp_shutdown (JITFLOAT32 totalTime) {
    return ;
}

static inline JITUINT64 get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 get_dependences (void) {
    return REACHING_DEFINITIONS_ANALYZER | LIVENESS_ANALYZER | ESCAPES_ANALYZER;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

static inline void cons_do_job (ir_method_t *method) {

    /* Perform the constant propagation	*/
    if ((irOptimizer->enabledOptimizations).consprop) {
        constant_propagation(method);
    }

    /* Perform the constant folding		*/
    if ((irOptimizer->enabledOptimizations).cfold) {
        constant_folding(method);
    }
}

static inline char * cons_get_version (void) {
    return VERSION;
}

static inline char * cons_get_informations (void) {
    return INFORMATIONS;
}

static inline char * cons_get_author (void) {
    return AUTHOR;
}

static inline void cons_propagation_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void cons_propagation_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
