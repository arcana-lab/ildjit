/*
 * Copyright (C) 2006-2009  Campanoni Simone
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
#include <ir_language.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_branchpredictors.h>
#include <config.h>
// End

#define INFORMATIONS    "Makes the static branch prediction"
#define AUTHOR          "Simone Campanoni"

static inline void branch_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void branch_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline JITUINT64 branch_get_ID_job (void);
static inline void branch_do_job (ir_method_t *method);
static inline char * branch_get_version (void);
static inline char * branch_get_informations (void);
static inline char * branch_get_author (void);
static inline JITUINT64 branch_get_dependences (void);
static inline JITUINT64 branch_get_invalidations (void);
static inline void branch_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void branch_shutdown (JITFLOAT32 totalTime);
static inline void update_probability (ir_instruction_t *inst, JITFLOAT32 prob, XanList *workList);
static inline void set_probability (ir_instruction_t *inst, JITFLOAT32 prob, XanList *workList);
static inline JITBOOLEAN containInstructionType (ir_instruction_t *inst);
static inline JITBOOLEAN containExceptionInstructionsType (ir_instruction_t *start, ir_method_t *method, JITINT8 *haveExceptionInstrAsSuccessor, XanList **successors);

ir_optimization_interface_t plugin_interface = {
    branch_get_ID_job,
    branch_get_dependences,
    branch_init,
    branch_shutdown,
    branch_do_job,
    branch_get_version,
    branch_get_informations,
    branch_get_author,
    branch_get_invalidations,
    NULL,
    branch_getCompilationFlags,
    branch_getCompilationTime
};

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;

static inline void branch_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);

    irLib = lib;
    irOptimizer = optimizer;
}

static inline void branch_shutdown (JITFLOAT32 totalTime) {

    irLib = NULL;
}

static inline JITUINT64 branch_get_ID_job (void) {
    return BRANCH_PREDICTOR;
}

static inline JITUINT64 branch_get_dependences (void) {
    return 0;
}

static inline JITUINT64 branch_get_invalidations (void) {
    return INVALIDATE_NONE;
}


static inline void branch_do_job (ir_method_t *method) {
    XanList                 *workList;
    XanListItem             *item;
    ir_instruction_t        *inst;
    ir_instruction_t        *succ1;
    ir_instruction_t        *succ2;
    JITUINT32 		count;
    JITUINT32 		instructionsNumber;
    JITFLOAT32 		prob;
    JITFLOAT32 		succ1Prob;
    JITFLOAT32 		succ2Prob;
    JITINT8                 *haveExceptionInstrAsSuccessor;
    XanList			**successors;

    /* Assertions				*/
    assert(method != NULL);
    assert(irLib != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);
    if (instructionsNumber == 0) {
        return;
    }

    /* Fetch the successors.
     */
    successors	= IRMETHOD_getInstructionsSuccessors(method);

    /* Allocate the necessary memory	*/
    haveExceptionInstrAsSuccessor = allocFunction(sizeof(JITINT8) * (instructionsNumber + 1));

    /* Erase the execution probability	*/
    for (count = 0; count < instructionsNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        inst->executionProbability = -1;
        haveExceptionInstrAsSuccessor[count] = -1;
    }
    workList = xanList_new(allocFunction, freeFunction, NULL);
    assert(workList != NULL);

    /* Set the probability to execute the	*
     * first instruction			*/
    inst = IRMETHOD_getFirstInstruction(method);
    if (inst != NULL) {
        inst->executionProbability = 1;
        xanList_insert(workList, inst);
    }

    /* Compute the execution probability	*
     * of the instructions			*/
    while (xanList_length(workList)  > 0) {
        XanListItem	*item2;
        succ1 = NULL;
        succ2 = NULL;
        item = xanList_first(workList);
        assert(item != NULL);
        inst = xanList_getData(item);
        assert(inst != NULL);
        xanList_deleteItem(workList, item);

        /* Check the instruction.
         */
        if (IRMETHOD_getInstructionType(inst) == IRSTARTCATCHER) {
            set_probability(inst, 0, workList);
            continue;
        }

        /* Fetch the first successor.
         */
        item2	= xanList_first(successors[inst->ID]);
        if (item2 == NULL) {
            continue;
        }
        succ1	= item2->data;
        assert(succ1 != NULL);

        /* Fetch the second successor.
         */
        item2	= item2->next;
        if (item2 == NULL) {

            /* There is only one successor.
             */
            update_probability(succ1, inst->executionProbability, workList);
            continue;
        }
        succ2	= item2->data;
        assert(succ2 != NULL);

        /* There are two successors.			*
         * Update the execution probability of the      *
         * successors					*/
        if (inst->executionProbability == 0) {

            /* Propagate the zero probability	*/
            set_probability(succ1, 0, workList);
            set_probability(succ2, 0, workList);

        } else if (containExceptionInstructionsType(succ1, method, haveExceptionInstrAsSuccessor, successors)) {

            /* Set to zero the probability of the           *
             * instructions in a branch since there is a    *
             * throw					*/
            set_probability(succ1, 0, workList);
            if (containExceptionInstructionsType(succ2, method, haveExceptionInstrAsSuccessor, successors)) {
                set_probability(succ2, 0, workList);
            } else {
                prob = inst->executionProbability;
                update_probability(succ2, prob, workList);
            }
        } else if (containExceptionInstructionsType(succ2, method, haveExceptionInstrAsSuccessor, successors)) {

            /* Set to zero the probability of the           *
             * instructions in a branch since there is a    *
             * throw					*/
            set_probability(succ2, 0, workList);
            if (containExceptionInstructionsType(succ1, method, haveExceptionInstrAsSuccessor, successors)) {
                set_probability(succ1, 0, workList);
            } else {
                prob = inst->executionProbability;
                update_probability(succ1, prob, workList);
            }
        } else {
            if (    (IRMETHOD_getInstructionPosition(succ1) > IRMETHOD_getInstructionPosition(inst))        &&
                    (IRMETHOD_getInstructionPosition(succ2) > IRMETHOD_getInstructionPosition(inst))        ) {
                prob = inst->executionProbability / 2;
                update_probability(succ1, prob, workList);
                update_probability(succ2, prob, workList);
            } else {
                if (IRMETHOD_getInstructionPosition(succ1) > IRMETHOD_getInstructionPosition(inst)) {
                    succ1Prob = (inst->executionProbability * 0.1);
                    succ2Prob = (inst->executionProbability * 0.9);
                } else {
                    succ1Prob = (inst->executionProbability * 0.9);
                    succ2Prob = (inst->executionProbability * 0.1);
                }
                update_probability(succ1, succ1Prob, workList);
                update_probability(succ2, succ2Prob, workList);
            }
        }
    }

    /* Check the instructions		*/
    for (count = 0; count < instructionsNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (inst->executionProbability == -1) {
            inst->executionProbability = 0;
        }
    }

    /* Free the memory			*/
    xanList_destroyList(workList);
    freeFunction(haveExceptionInstrAsSuccessor);
    IRMETHOD_destroyInstructionsSuccessors(successors, instructionsNumber);

    return;
}

static inline char * branch_get_version (void) {
    return VERSION;
}

static inline char * branch_get_informations (void) {
    return INFORMATIONS;
}

static inline char * branch_get_author (void) {
    return AUTHOR;
}

static inline JITBOOLEAN containExceptionInstructionsType (ir_instruction_t *start, ir_method_t *method, JITINT8 *haveExceptionInstrAsSuccessor, XanList **successors) {

    /* Assertions.
     */
    assert(start != NULL);
    assert(method != NULL);
    assert(successors != NULL);

    if (haveExceptionInstrAsSuccessor[start->ID] == -1) {

        haveExceptionInstrAsSuccessor[start->ID] = JITFALSE;

        ir_instruction_t        *succ;
        ir_instruction_t        *inst;

        inst = start;
        while (1) {
            XanListItem	*item;

            /* Fetch the first successor.
             */
            item	= xanList_first(successors[inst->ID]);
            if (item == NULL) {
                break;
            }
            succ 	= item->data;
            assert(succ != NULL);

            /* Fetch the second successor.
             */
            item	= item->next;
            if (item != NULL) {
                break;
            }
            if (containInstructionType(succ)) {
                haveExceptionInstrAsSuccessor[start->ID] = JITTRUE;
                break;
            }
            inst = succ;
        }

    }

    return haveExceptionInstrAsSuccessor[start->ID];
}

static inline JITBOOLEAN containInstructionType (ir_instruction_t *inst) {

    /* Assertions			*/
    assert(inst != NULL);

    if (IRMETHOD_getInstructionType(inst) == IRTHROW) {
        return JITTRUE;
    }
    if (IRMETHOD_getInstructionType(inst) == IRSTARTFILTER) {
        return JITTRUE;
    }
    if (IRMETHOD_getInstructionType(inst) == IRENDFILTER) {
        return JITTRUE;
    }
    if (IRMETHOD_getInstructionType(inst) == IRSTARTCATCHER) {
        return JITTRUE;
    }

    return JITFALSE;
}

static inline void set_probability (ir_instruction_t *inst, JITFLOAT32 prob, XanList *workList) {

    /* Assertions			*/
    assert(prob >= 0);
    assert(prob <= 1);
    assert(inst != NULL);
    assert(workList != NULL);

    if (inst->executionProbability != prob) {
        xanList_append(workList, inst);
    }
    inst->executionProbability = prob;

    return;
}

static inline void update_probability (ir_instruction_t *inst, JITFLOAT32 prob, XanList *workList) {

    /* Assertions			*/
    assert(prob >= 0);
    assert(prob <= 1);
    assert(inst != NULL);
    assert(workList != NULL);

    if (    (inst->executionProbability < prob)     ||
            (inst->executionProbability == 0)       ) {
        if (inst->executionProbability != prob) {
            xanList_append(workList, inst);
        }
        inst->executionProbability = prob;
    }

    return;
}

static inline void branch_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void branch_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
