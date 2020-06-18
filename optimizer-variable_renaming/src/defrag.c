/*
 * Copyright (C) 2010  Mastrandrea Daniele
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
 * @file defrag.c
 * @brief ILDJIT Optimizer: variable renaming
 * @author Mastrandrea Daniele
 * @version 0.1.0
 * @date 2010
 *
 * @section <description> (Description)
 * This module is a plugin to remove unused temporaries.
 * Sometimes it happens that a method `thinks` it is using a huge amount of
 * temporaries (often after the execution of some optimisations, such as copy
 * propagation, dead code elimination...), while temporaries' usage is very low.
 * Unfortunately such few temporaries use variables with a high ID, preventing
 * the updateRootSet() function to work properly, and wasting memory.
 * This plugin renames the temporaries in order for them to have the lowest IDs,
 * allowing the previously mentioned function to do its job.
 * In other words it performs some kind of variables' defragmentation.
 *
 * @section <algorithm> (Algorithm)
 * The plugin simply parses all the instructions in the method, and when it finds
 * a variable never found yet, it maps the variable ID to an incremntally generated
 * new ID, then performs the substitution in the current isntruction.
 * Every time an already known variable is found, the plugin fetches the associated
 * new value and substitutes.
 **/

// Standard headers
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>

// Standard ILDJIT headers
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include "config.h"
#include "defrag.h"
#ifdef PRINTDEBUG
#include "pretty_print.h"
#endif /* PRINTDEBUG */

// Plugin informations
#define INFORMATIONS    "Plugin to perform some memory defragmentation"
#define AUTHOR          "Simone Campanoni, Mastrandrea Daniele"
#define JOB             VARIABLES_RENAMING

/**
 * @defgroup Interface
 * Standard interface functions
 **/

/**
 * @ingroup Interface
 * @brief Get the ID of the job.
 **/
static inline JITUINT64 defrag_get_ID_job (void);


/**
 * @ingroup Interface
 * @brief Get the plugin version.
 **/
static inline char * defrag_get_version (void);

/**
 * @ingroup Interface
 * @brief Run the job of the plugin.
 **/
static inline void defrag_do_job (ir_method_t *method);

/**
 * @ingroup Interface
 * @brief Get informations about the plugin.
 **/
static inline char * defrag_get_informations (void);

/**
 * @ingroup Interface
 * @brief Get the author of the plugin.
 **/
static inline char * defrag_get_author (void);

/**
 * @ingroup Interface
 * @brief Get the dependences of the plugin.
 **/
static inline JITUINT64 defrag_get_dependences (void);

static inline JITUINT64 defrag_get_invalidations (void);

/**
 * @ingroup Interface
 * @brief Perform the shutdown of the plugin.
 **/
static inline void defrag_shutdown (JITFLOAT32 totalTime);

/**
 * @ingroup Interface
 * @brief Perform the initialisation of the plugin.
 **/
static inline void defrag_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);

/**
 * @defgroup Internals
 * Optimizer internal functions
 **/

/**
 * @ingroup Internals
 * @brief Start the plugin's task.
 **/
static inline JITUINT32 internal_defrag (ir_method_t * method);

static inline void internal_makeILTypeUniform (ir_method_t *m);
static inline void defrag_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void defrag_getCompilationFlags (char *buffer, JITINT32 bufferLength);


/*****************************************************************************************************************************************************/
/*                                                                                                                                                   */
/*****************************************************************************************************************************************************/

// Global variables
ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;
#ifdef PRINTDEBUG
int INDENTATION_LEVEL = 0;
#endif /* PRINTDEBUG */

// Plugin interface
ir_optimization_interface_t plugin_interface = {
    defrag_get_ID_job,
    defrag_get_dependences,
    defrag_init,
    defrag_shutdown,
    defrag_do_job,
    defrag_get_version,
    defrag_get_informations,
    defrag_get_author,
    defrag_get_invalidations,
    NULL,
    defrag_getCompilationFlags,
    defrag_getCompilationTime
};

static inline void defrag_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    // Assertions
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

static inline void defrag_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
#ifdef PRINTDEBUG
    assert(0 == INDENTATION_LEVEL);
#endif  /* PRINTDEBUG */
}

static inline JITUINT64 defrag_get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 defrag_get_dependences (void) {
#ifdef SUPERDEBUG
    return METHOD_PRINTER;
#else
    return 0;
#endif  /* SUPERDEBUG */
}

static inline JITUINT64 defrag_get_invalidations (void) {
    return INVALIDATE_ALL;
}

static inline char * defrag_get_version (void) {
    return VERSION;
}

static inline char * defrag_get_informations (void) {
    return INFORMATIONS;
}

static inline char * defrag_get_author (void) {
    return AUTHOR;
}

static inline void defrag_do_job (ir_method_t * method) {
    JITUINT32 variables_gain __attribute__((unused));

    // Assertions
    assert(method != NULL);

    PDEBUG("OPTIMIZER_VARIABLE_RENAMING: Start with method %s\n", METH_NAME(method));
    PP_INDENT();

#ifdef SUPERDEBUG
    // Print method's graph before optimisation
    SDEBUG("Dumping the method's control flow graph before any change is applied\n");
    irOptimizer->callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
#endif  /* SUPERDEBUG */

    // Print some debug information before plugin execution
    PDEBUG("Some information before the execution of variables' renaming\n");
    PP_INDENT();
    PDEBUG("Instructions number: %u\n", METH_N_INST(method));
    PDEBUG("Max variables: %u\n", METH_MAX_VAR(method));
    PDEBUG("Parameters number: %u\n", METH_N_PAR(method));
    PDEBUG("Locals number: %u\n", METH_N_LOCAL(method));
    PDEBUG("Temporaries number: %u\n", METH_N_TEMP(method));
    PP_UNINDENT();

    variables_gain = internal_defrag(method);

    internal_makeILTypeUniform(method);

    // Print some debug information after plugin execution
    PDEBUG("Some information before the execution of variables' renaming\n");
    PP_INDENT();
    SDEBUG("Instructions number: %u\n", METH_N_INST(method));
    PDEBUG("Max variables: %u\n", METH_MAX_VAR(method));
    SDEBUG("Parameters number: %u\n", METH_N_PAR(method));
    SDEBUG("Locals number: %u\n", METH_N_LOCAL(method));
    PDEBUG("Temporaries number: %u\n", METH_N_TEMP(method));
    PP_UNINDENT();

    PP_UNINDENT();
    PDEBUG("OPTIMIZER_VARIABLE_RENAMING: Finished with method %s, with a gain of %u variables\n", METH_NAME(method), variables_gain);

    return;
}

static inline void internal_makeILTypeUniform (ir_method_t *m) {
    JITUINT32	i;
    JITUINT32	instructionsNumber;

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber	= IRMETHOD_getInstructionsNumber(m);

    /* The IL type of the output of arithmetic operation must be the same as the input ones.
     */
    for (i=0; i < instructionsNumber; i++) {
        ir_instruction_t	*inst;
        ir_item_t		*res;
        ir_item_t		*p;

        /* Check the instruction.
         */
        inst	= IRMETHOD_getInstructionAtPosition(m, i);
        if (!IRMETHOD_isAMathInstruction(inst)) {
            continue ;
        }

        /* Check the IL data types.
         */
        res	= IRMETHOD_getInstructionResult(inst);
        p	= IRMETHOD_getInstructionParameter1(inst);
        if (!IRDATA_isAVariable(p)) {
            p	= IRMETHOD_getInstructionParameter2(inst);
        }
        if (	(IRDATA_isAVariable(p))			&&
                (p->type_infos != res->type_infos)	&&
                (p->type_infos != NULL)			) {
            res->type_infos	= p->type_infos;
            m->modified	= JITTRUE;
        }
    }

    /* The IL data types of a variable must be consistent among every uses.
     */
    //TODO

    return ;
}

static inline JITUINT32 internal_defrag (ir_method_t * method) {
    XanHashTable            *vars_map;
    ir_instruction_t        *curr_inst;
    JITUINT32 param_type;
    IR_ITEM_VALUE param_value;
    JITUINT32 current_var, mapped_var, max_variables_new, count;
    void                    *lookup_result;

    const JITUINT32 parameters_number = METH_N_PAR(method);
    const JITUINT32 locals_number = METH_N_LOCAL(method);
    const JITUINT32 max_variables = METH_MAX_VAR(method);
    const JITUINT32 temporaries_offset = parameters_number + locals_number;

    if (max_variables == temporaries_offset) {
        PDEBUG("No temporaries to rename\n");
        return 0;
    }

    PDEBUG("Starting variables renaming\n");
    PP_INDENT();

    // Initialise the hash map to be large enough to store the whole number of temporaries
    vars_map = xanHashTable_new(METH_N_TEMP(method), JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    // Initialise the index of current variable
    current_var = 0;

    // Get first instruction and...
    curr_inst = INST_AT(method, 0);

    // ... iterate over the whole method
    while (NULL != curr_inst) {

        SDEBUG("Examining instruction #%u\n", curr_inst->ID);
        PP_INDENT();

        switch (INST_N_PAR(method, curr_inst)) {
            case 3:
                // Third parameter
                assert(NULL != PAR_3(method, curr_inst));
                param_type = PAR_3_TYPE(method, curr_inst);
                param_value = PAR_3_VALUE(method, curr_inst);
                if (IROFFSET == param_type && param_value >= temporaries_offset) {
                    SDEBUG("Parameter %u is a temporary with id %u\n", 3, (JITUINT32) param_value);
                    PP_INDENT();
                    lookup_result = xanHashTable_lookup(vars_map, (void *) (JITNUINT) (param_value + 1));
                    if (NULL == lookup_result) {
                        SDEBUG("Variable id must be stored inside the variables' map\n");
                        mapped_var = temporaries_offset + current_var;
                        xanHashTable_insert(vars_map, (void *) (JITNUINT) (param_value + 1), (void *) (JITNUINT) (mapped_var + 1));
                        ++current_var;
                    } else {
                        SDEBUG("Variable is already present inside the variables' map\n");
                        mapped_var = ((JITNUINT) lookup_result) - 1;
                    }
                    // Replace the variable ID with the new one, if needed
                    if (mapped_var != (JITUINT32) param_value) {
                        SDEBUG("Old id %u maps to new id %u, updating...\n", (JITUINT32) param_value, mapped_var);
                        PAR_3_VALUE_SET(method, curr_inst, mapped_var);
                    }
                    PP_UNINDENT();
                }

            case 2:
                // Second parameter
                assert(NULL != PAR_2(method, curr_inst));
                param_type = PAR_2_TYPE(method, curr_inst);
                param_value = PAR_2_VALUE(method, curr_inst);
                if (IROFFSET == param_type && param_value >= temporaries_offset) {
                    SDEBUG("Parameter %u is a temporary with id %u\n", 2, (JITUINT32) param_value);
                    PP_INDENT();
                    lookup_result = xanHashTable_lookup(vars_map, (void *) (JITNUINT) (param_value + 1));
                    if (NULL == lookup_result) {
                        SDEBUG("Variable id must be stored inside the variables' map\n");
                        mapped_var = temporaries_offset + current_var;
                        xanHashTable_insert(vars_map, (void *) (JITNUINT) (param_value + 1), (void *) (JITNUINT) (mapped_var + 1));
                        ++current_var;
                    } else {
                        SDEBUG("Variable is already present inside the variables' map\n");
                        mapped_var = ((JITNUINT) lookup_result) - 1;
                    }
                    // Replace the variable ID with the new one, if needed
                    if (mapped_var != (JITUINT32) param_value) {
                        SDEBUG("Old id %u maps to new id %u, updating...\n", (JITUINT32) param_value, mapped_var);
                        PAR_2_VALUE_SET(method, curr_inst, mapped_var);
                    }
                    PP_UNINDENT();
                }

            case 1:
                // First parameter
                assert(NULL != PAR_1(method, curr_inst));
                param_type = PAR_1_TYPE(method, curr_inst);
                param_value = PAR_1_VALUE(method, curr_inst);
                if (IROFFSET == param_type && param_value >= temporaries_offset) {
                    SDEBUG("Parameter %u is a temporary with id %u\n", 1, (JITUINT32) param_value);
                    PP_INDENT();
                    lookup_result = xanHashTable_lookup(vars_map, (void *) (JITNUINT) (param_value + 1));
                    if (NULL == lookup_result) {
                        SDEBUG("Variable id must be stored inside the variables' map\n");
                        mapped_var = temporaries_offset + current_var;
                        xanHashTable_insert(vars_map, (void *) (JITNUINT) (param_value + 1), (void *) (JITNUINT) (mapped_var + 1));
                        ++current_var;
                    } else {
                        SDEBUG("Variable is already present inside the variables' map\n");
                        mapped_var = ((JITNUINT) lookup_result) - 1;
                    }
                    // Replace the variable ID with the new one, if needed
                    if (mapped_var != (JITUINT32) param_value) {
                        SDEBUG("Old id %u maps to new id %u, updating...\n", (JITUINT32) param_value, mapped_var);
                        PAR_1_VALUE_SET(method, curr_inst, mapped_var);
                    }
                    PP_UNINDENT();
                }

            default:
                // Result (optional)
                if (NULL != RES(method, curr_inst)) {
                    param_type = RES_TYPE(method, curr_inst);
                    param_value = RES_VALUE(method, curr_inst);
                    if (IROFFSET == param_type && param_value >= temporaries_offset) {
                        SDEBUG("Result is a temporary with id %u\n", (JITUINT32) param_value);
                        PP_INDENT();
                        lookup_result = xanHashTable_lookup(vars_map, (void *) (JITNUINT) (param_value + 1));
                        if (NULL == lookup_result) {
                            SDEBUG("Variable id must be stored inside the variables' map\n");
                            mapped_var = temporaries_offset + current_var;
                            xanHashTable_insert(vars_map, (void *) (JITNUINT) (param_value + 1), (void *) (JITNUINT) (mapped_var + 1));
                            ++current_var;
                        } else {
                            SDEBUG("Variable is already present inside the variables' map\n");
                            mapped_var = ((JITNUINT) lookup_result) - 1;
                        }
                        // Replace the variable ID with the new one, if needed
                        if (mapped_var != (JITUINT32) param_value) {
                            SDEBUG("Old id %u maps to new id %u, updating...\n", (JITUINT32) param_value, mapped_var);
                            RES_VALUE_SET(method, curr_inst, mapped_var);
                        }
                        PP_UNINDENT();
                    }
                }
                break;
        }

        // Call parameters (optional)
        for (count = 0; count < CALL_PAR_NUM(method, curr_inst); count++) {
            param_type = CALL_PAR_TYPE(method, curr_inst, count);
            param_value = CALL_PAR_VALUE(method, curr_inst, count);
            if (IROFFSET == param_type && param_value >= temporaries_offset) {
                SDEBUG("Call parameter %u is a temporary with id %u\n", count, (JITUINT32) param_value);
                PP_INDENT();
                lookup_result = xanHashTable_lookup(vars_map, (void *) (JITNUINT) (param_value + 1));
                if (NULL == lookup_result) {
                    SDEBUG("Variable id must be stored inside the variables' map\n");
                    mapped_var = temporaries_offset + current_var;
                    xanHashTable_insert(vars_map, (void *) (JITNUINT) (param_value + 1), (void *) (JITNUINT) (mapped_var + 1));
                    ++current_var;
                } else {
                    SDEBUG("Variable is already present inside the variables' map\n");
                    mapped_var = ((JITNUINT) lookup_result) - 1;
                }
                // Replace the variable ID with the new one, if needed
                if (mapped_var != (JITUINT32) param_value) {
                    SDEBUG("Old id %u maps to new id %u, updating...\n", (JITUINT32) param_value, mapped_var);
                    CALL_PAR_VALUE_SET(method, curr_inst, count, mapped_var);
                }
                PP_UNINDENT();
            }
        }
        PP_UNINDENT();

        // Repeat on the next instruction
        curr_inst = NEXT_INST(method, curr_inst);
    }
    PP_UNINDENT();

    // Free no longer used memory
    xanHashTable_destroyTable(vars_map);

    // Update the value of the max number of variables
    max_variables_new = temporaries_offset + current_var;
    METH_MAX_VAR_SET(method, max_variables_new);

    // Gain should be positive or zero
    assert(max_variables - max_variables_new >= 0);

    // Return the difference between the old and the new value of max_variables (gain)
    return max_variables - max_variables_new;
}

static inline void defrag_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void defrag_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
