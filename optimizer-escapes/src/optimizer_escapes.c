/*
 * Copyright (C) 2008 - 2009  Campanoni Simone
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

// My headers
#include <optimizer_escapes.h>
#include <config.h>
// End

#define INFORMATIONS    "This step mark the escapes variables"
#define AUTHOR          "Campanoni Simone"

static inline JITUINT64 escapes_get_ID_job (void);
static inline char * escapes_get_version (void);
static inline void escapes_do_job (ir_method_t *method);
static inline char * escapes_get_informations (void);
static inline char * escapes_get_author (void);
static inline JITUINT64 escapes_get_dependences (void);
static inline JITUINT64 escapes_get_invalidations (void);
static inline void init_def_use (ir_method_t *method);
static inline void mark_the_escapes_variables (ir_method_t *method);
static inline void escapes_shutdown (JITFLOAT32 totalTime);
static inline void escapes_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void escapes_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void escapes_getCompilationFlags (char *buffer, JITINT32 bufferLength);

static ir_optimizer_t *irOptimizer;

ir_optimization_interface_t plugin_interface = {
    escapes_get_ID_job,
    escapes_get_dependences,
    escapes_init,
    escapes_shutdown,
    escapes_do_job,
    escapes_get_version,
    escapes_get_informations,
    escapes_get_author,
    escapes_get_invalidations,
    NULL,
    escapes_getCompilationFlags,
    escapes_getCompilationTime
};

static inline void escapes_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    irOptimizer	= optimizer;

    return ;
}

static inline void escapes_shutdown (JITFLOAT32 totalTime) {

}

static inline JITUINT64 escapes_get_ID_job (void) {
    return ESCAPES_ANALYZER;
}

static inline JITUINT64 escapes_get_dependences (void) {
    return 0;
}

static inline JITUINT64 escapes_get_invalidations (void) {
    return INVALIDATE_NONE;
}


static inline void escapes_do_job (ir_method_t *method) {

    /* Assertions				*/
    assert(method != NULL);

    /* Update the number of variables used by the method.
     */
    IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

    /* Erase past information.
     */
    IRMETHOD_allocEscapedVariablesMemory(method, 0);

    /* Check if there are variables		*/
    if (IRMETHOD_getNumberOfVariablesNeededByMethod(method) == 0) {
        return;
    }

    /* Init the def, use sets		*/
    init_def_use(method);

    /* Mark the escapes variable		*/
    mark_the_escapes_variables(method);

    /* Return				*/
    return;
}

static inline void init_def_use (ir_method_t *method) {
    JITINT32 use_size;
    JITUINT32 blocksNumber;

    /* Assertions			*/
    assert(method != NULL);

    /* Init the variables		*/
    use_size = 0;
    blocksNumber = 0;

    /* Fetch the USE size		*/
    use_size = IRMETHOD_getNumberOfVariablesNeededByMethod(method);
    assert(use_size > 0);

    /* Compute how many block we	*
     * have to allocate for the	*
     * USE set			*/
    blocksNumber = (use_size / (sizeof(JITNUINT) * 8)) + 1;
    IRMETHOD_allocEscapedVariablesMemory(method, blocksNumber);

    /* Return			*/
    return ;
}

static inline void mark_the_escapes_variables (ir_method_t *method) {
    JITUINT32 inst_count;
    JITUINT32 varID;
    ir_instruction_t        *inst;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Init the variables			*/
    inst = NULL;

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Search the escaped variables		*/
    for (inst_count = 0; inst_count < instructionsNumber; inst_count++) {

        /* Fetch the instruction	*/
        inst = IRMETHOD_getInstructionAtPosition(method, inst_count);
        assert(inst != NULL);

        /* Check if the instruction take*
         * the address			*/
        if (inst->type == IRGETADDRESS) {
            assert(IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET);
            varID = IRMETHOD_getInstructionParameter1Value(inst);
            assert(varID < IRMETHOD_getNumberOfVariablesNeededByMethod(method));
            IRMETHOD_setVariableAsEscaped(method, varID);
        }
    }
}

static inline char * escapes_get_version (void) {
    return VERSION;
}

static inline char * escapes_get_informations (void) {
    return INFORMATIONS;
}

static inline char * escapes_get_author (void) {
    return AUTHOR;
}

static inline void escapes_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void escapes_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
