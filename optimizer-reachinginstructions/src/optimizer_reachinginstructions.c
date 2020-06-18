/*
 * Copyright (C) 2012 - 2013  Campanoni Simone
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

// My headers
#include <optimizer_reachinginstructions.h>
#include <config.h>
// End

#define INFORMATIONS 	"This is a reachinginstructions plugin"
#define	AUTHOR		"Simone Campanoni"
#define JOB            	REACHING_INSTRUCTIONS_ANALYZER

static inline JITUINT64 reachinginstructions_get_ID_job (void);
static inline char * reachinginstructions_get_version (void);
static inline void reachinginstructions_do_job (ir_method_t * method);
static inline char * reachinginstructions_get_informations (void);
static inline char * reachinginstructions_get_author (void);
static inline JITUINT64 reachinginstructions_get_dependences (void);
static inline void reachinginstructions_shutdown (JITFLOAT32 totalTime);
static inline void reachinginstructions_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 reachinginstructions_get_invalidations (void);
static inline void reachinginstructions_startExecution (void);
static inline void reachinginstructions_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void reachinginstructions_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline data_flow_t * compute_reachable_instructions_data_flow (ir_method_t *method, JITBOOLEAN acrossBackEdges);
static inline void compute_gen_kill_reachable_instructions (ir_method_t *method, data_flow_t *data);
static inline void init_in_out_reachable_instructions (ir_method_t *method, data_flow_t *data);
static inline JITBOOLEAN compute_reachable_instructions_data_flow_step (data_flow_t *data, XanList **successors, XanHashTable *backEdges);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

ir_optimization_interface_t plugin_interface = {
    reachinginstructions_get_ID_job		,
    reachinginstructions_get_dependences	,
    reachinginstructions_init		,
    reachinginstructions_shutdown		,
    reachinginstructions_do_job		,
    reachinginstructions_get_version	,
    reachinginstructions_get_informations	,
    reachinginstructions_get_author		,
    reachinginstructions_get_invalidations	,
    reachinginstructions_startExecution	,
    reachinginstructions_getCompilationFlags,
    reachinginstructions_getCompilationTime
};

static inline void reachinginstructions_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib		= lib;
    irOptimizer	= optimizer;
    prefix		= outputPrefix;
}

static inline void reachinginstructions_shutdown (JITFLOAT32 totalTime) {
    irLib		= NULL;
    irOptimizer	= NULL;
    prefix		= NULL;
}

static inline JITUINT64 reachinginstructions_get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 reachinginstructions_get_dependences (void) {
    return 0;
}

static inline void reachinginstructions_do_job (ir_method_t * method) {
    data_flow_t	*df;
    char		*env;
    JITBOOLEAN	acrossBackEdges = JITTRUE;

    /* Assertions.
     */
    assert(method != NULL);

    /* Free the past information.
     */
    df	= IRMETHOD_getMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    if (df != NULL) {
        DATAFLOW_destroySets(df);
        IRMETHOD_removeMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    }

    /* Check for following back edges.
     */
    env	= getenv("REACHING_INSTRUCTIONS_ACROSS_BACK_EDGES");
    if (env != NULL) {
        acrossBackEdges = atoi(env) != 0;
    }

    /* Identify loops, if necessary.
     */
    if (!acrossBackEdges) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, LOOP_IDENTIFICATION);
    }

    /* Compute the data flow.
     */
    df	= compute_reachable_instructions_data_flow(method, acrossBackEdges);
    assert(df != NULL);

    /* Set the data flow.
     */
    IRMETHOD_setMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER, df);

    return;
}

static inline char * reachinginstructions_get_version (void) {
    return VERSION;
}

static inline char * reachinginstructions_get_informations (void) {
    return INFORMATIONS;
}

static inline char * reachinginstructions_get_author (void) {
    return AUTHOR;
}

static inline JITUINT64 reachinginstructions_get_invalidations (void) {
    return 0;
}

static inline void reachinginstructions_startExecution (void) {

}

static inline void reachinginstructions_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void reachinginstructions_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}

static inline data_flow_t * compute_reachable_instructions_data_flow (ir_method_t *method, JITBOOLEAN acrossBackEdges) {
    data_flow_t             *reachableInstructions;
    JITUINT32 		instructionsNumber;
    JITBOOLEAN 		modified;
    XanList                 **successors;
    XanHashTable		*backEdges;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);
    if (instructionsNumber == 0) {
        return NULL;
    }

    /* Allocate the memory for data-flow	*
     * information				*/
    reachableInstructions = DATAFLOW_allocateSets(instructionsNumber, instructionsNumber);
    assert(reachableInstructions != NULL);

    /* Compute the gen and kill sets	*/
    compute_gen_kill_reachable_instructions(method, reachableInstructions);

    /* Initialize the IN and OUT sets	*/
    init_in_out_reachable_instructions(method, reachableInstructions);

    /* Compute the set of successors	*/
    successors = IRMETHOD_getInstructionsSuccessors(method);
    assert(successors != NULL);

    /* Compute the set of circuits		*/
    if (!acrossBackEdges) {
        XanListItem *circuitItem;
        XanList *circuits = IRMETHOD_getMethodCircuits(method);
        assert(circuits != NULL);
        backEdges = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        assert(backEdges != NULL);
        circuitItem = xanList_first(circuits);
        while (circuitItem) {
            circuit_t *c = circuitItem->data;
            ir_instruction_t *branch = IRMETHOD_getTheSourceOfTheCircuitBackedge(c);
            ir_instruction_t *header = IRMETHOD_getTheDestinationOfTheCircuitBackedge(c);
            XanBitSet *branches = xanHashTable_lookup(backEdges, header);
            if (!branches) {
                branches = xanBitSet_new(instructionsNumber);
                xanHashTable_insert(backEdges, header, branches);
            }
            xanBitSet_setBit(branches, branch->ID);
            circuitItem = circuitItem->next;
        }
        xanList_destroyList(circuits);
    } else {
        backEdges = NULL;
    }

    /* Compute the reachable instructions	*
     * data-flow information		*/
    do {
        modified = compute_reachable_instructions_data_flow_step(reachableInstructions, successors, backEdges);
    } while (modified);

    /* Free the memory			*/
    IRMETHOD_destroyInstructionsSuccessors(successors, instructionsNumber);
    if (!acrossBackEdges) {
        XanHashTableItem *edgeItem;
        assert(backEdges != NULL);
        edgeItem = xanHashTable_first(backEdges);
        while (edgeItem) {
            XanBitSet *branches = edgeItem->element;
            xanBitSet_free(branches);
            edgeItem = xanHashTable_next(backEdges, edgeItem);
        }
        xanHashTable_destroyTable(backEdges);
    }

    return reachableInstructions;
}

static inline void compute_gen_kill_reachable_instructions (ir_method_t *method, data_flow_t *data) {
    JITUINT32 i;

    /* Assertions				*/
    assert(method != NULL);
    assert(data != NULL);

    /* Compute the gen sets			*/
    for (i = 0; i < (data->num); i++) {
        JITUINT32 trigger;
        JITNUINT temp;
        trigger = i / (sizeof(JITNUINT) * 8);
        assert(trigger < (data->elements));
        temp 	= 0x1;
        assert((data->data[i]).gen != NULL);
        (data->data[i]).gen[trigger] = temp << (i % (sizeof(JITNUINT) * 8));
    }

    /* Return				*/
    return;
}

static inline void init_in_out_reachable_instructions (ir_method_t *method, data_flow_t *data) {
    JITUINT32 i;
    JITUINT32 j;

    /* Assertions				*/
    assert(method != NULL);
    assert(data != NULL);

    /* Compute the gen sets			*/
    for (i = 0; i < (data->num); i++) {
        for (j = 0; j < (data->elements); j++) {
            (data->data[i]).in[j] = 0;
            (data->data[i]).out[j] = 0;
        }
    }

    /* Return				*/
    return;
}

static inline JITBOOLEAN compute_reachable_instructions_data_flow_step (data_flow_t *data, XanList **successors, XanHashTable *backEdges) {
    JITBOOLEAN modified;
    JITINT32 i;

    /* Assertions				*/
    assert(data != NULL);
    assert(successors != NULL);

    /* Initialize the variables		*/
    modified = JITFALSE;

    /* Compute the in and out sets		*/
    for (i = (data->num) - 1; i >= 0; i--) {
        JITUINT32 j;
        JITNUINT old;
        JITNUINT new;

        /* Compute the out set			*/
        for (j = 0; j < (data->elements); j++) {
            XanListItem     *item;
            old = (data->data[i]).out[j];
            item = xanList_first(successors[i]);
            while (item != NULL) {
                ir_instruction_t        *succ;
                XanBitSet		*branches = NULL;
                succ = item->data;
                assert(succ != NULL);
                assert(succ->ID < data->num);
                if (backEdges != NULL) {
                    branches = xanHashTable_lookup(backEdges, succ);
                }
                if (branches == NULL || !xanBitSet_isBitSet(branches, i)) {
                    ((data->data[i]).out[j]) |= ((data->data[succ->ID]).in[j]);
                }
                item = item->next;
            }
            new = (data->data[i]).out[j];
            if (new != old) {
                modified = JITTRUE;
            }
        }

        /* Compute the in set			*/
        for (j = 0; j < (data->elements); j++) {
            old = (data->data[i]).in[j];
            (data->data[i]).in[j] = (data->data[i]).gen[j] | (data->data[i]).out[j];
            new = (data->data[i]).in[j];
            if (new != old) {
                modified = JITTRUE;
            }
        }
    }

    /* Return				*/
    return modified;
}
