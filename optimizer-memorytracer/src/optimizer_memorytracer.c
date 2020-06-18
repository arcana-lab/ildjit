/*
 * Copyright (C) 2012  Campanoni Simone
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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <ildjit.h>

// My headers
#include <optimizer_memorytracer.h>
#include <code_injector.h>
#include <runtime.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a memorytracer plugin"
#define	AUTHOR		"Simone Campanoni"
#define JOB            	DDG_PROFILE

static inline JITUINT64 memorytracer_get_ID_job (void);
static inline char * memorytracer_get_version (void);
static inline void memorytracer_do_job (ir_method_t * method);
static inline char * memorytracer_get_informations (void);
static inline char * memorytracer_get_author (void);
static inline JITUINT64 memorytracer_get_dependences (void);
static inline void memorytracer_shutdown (JITFLOAT32 totalTime);
static inline void memorytracer_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 memorytracer_get_invalidations (void);
static inline void memorytracer_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void memorytracer_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t		*irLib		= NULL;
ir_optimizer_t		*irOptimizer	= NULL;
char 	 	       	*prefix		= NULL;
static JITBOOLEAN	initialized	= JITFALSE;

ir_optimization_interface_t plugin_interface = {
    memorytracer_get_ID_job		,
    memorytracer_get_dependences	,
    memorytracer_init		,
    memorytracer_shutdown		,
    memorytracer_do_job		,
    memorytracer_get_version		,
    memorytracer_get_informations	,
    memorytracer_get_author		,
    memorytracer_get_invalidations	,
    NULL				,
    memorytracer_getCompilationFlags	,
    memorytracer_getCompilationTime
};

static inline void memorytracer_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib		= lib;
    irOptimizer	= optimizer;
    prefix		= outputPrefix;
    executedCycles	= 0;

    return ;
}

static inline void memorytracer_shutdown (JITFLOAT32 totalTime) {
    irLib		= NULL;
    irOptimizer	= NULL;
    prefix		= NULL;

    if (initialized) {
        RUNTIME_shutdown();
    }

    return ;
}

static inline JITUINT64 memorytracer_get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 memorytracer_get_dependences (void) {
    return 0;
}

static inline void memorytracer_do_job (ir_method_t * method) {
    compilation_scheme_t 	cs;

    /* Assertions.
     */
    assert(method != NULL);

    /* Initialize the runtime.
     */
    if (!initialized) {
        initialized	= JITTRUE;
        RUNTIME_init();
    }

    /* Fetch the compilation scheme currently in use.
     */
    cs	= ILDJIT_compilationSchemeInUse();

    /* Check whether we are statically compiling the program.
     */
    if (!ILDJIT_isAStaticCompilationScheme(cs)) {
        return ;
    }
    if (IRPROGRAM_getEntryPointMethod() != method) {
        return ;
    }

    /* We are compiling the code with a static compilation scheme.
     * Inject the code to profile the execution of the entire program.
     */
    inject_code_to_program();

    return;
}

static inline char * memorytracer_get_version (void) {
    return VERSION;
}

static inline char * memorytracer_get_informations (void) {
    return INFORMATIONS;
}

static inline char * memorytracer_get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 memorytracer_get_invalidations (void) {
    return INVALIDATE_ALL;
}

static inline void memorytracer_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void memorytracer_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
