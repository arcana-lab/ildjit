/*
 * Copyright (C) 2006-2012  Rocchini Luca, Simone Campanoni
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
#include <optimizer_distance.h>
#include <config.h>
// End

#define INFORMATIONS    "Compute call distance from first instruction"
#define AUTHOR          "Luca Rocchini, Simone Campanoni"

static inline JITUINT64 distance_get_ID_job (void);
static inline void distance_do_job (ir_method_t *method);
static inline char * distance_get_version (void);
static inline char * distance_get_informations (void);
static inline char * distance_get_author (void);
static inline JITUINT64 distance_get_dependences (void);
static inline JITUINT64 distance_get_invalidations (void);
static inline void distance_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void distance_shutdown (JITFLOAT32 totalTime);
static inline void markInstruction (ir_method_t *method, ir_instruction_t *instr, ir_instruction_t *succ, XanList *workList);
static inline JITBOOLEAN containInstructionType (ir_instruction_t *inst);

ir_optimization_interface_t plugin_interface = {
    distance_get_ID_job,
    distance_get_dependences,
    distance_init,
    distance_shutdown,
    distance_do_job,
    distance_get_version,
    distance_get_informations,
    distance_get_author,
    distance_get_invalidations

};

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;

static inline void distance_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);

    irLib = lib;
    irOptimizer = optimizer;
}

static inline void distance_shutdown (JITFLOAT32 totalTime) {

    irLib = NULL;
}

static inline JITUINT64 distance_get_ID_job (void) {
    return CALL_DISTANCE_COMPUTER;
}

static inline JITUINT64 distance_get_dependences (void) {
    return 0;
}

static inline JITUINT64 distance_get_invalidations (void) {
    return INVALIDATE_NONE;
}


static inline void distance_do_job (ir_method_t *method) {
    XanList                 *workList;
    XanListItem             *item;
    ir_instruction_t        *instr;
    ir_instruction_t        *succ1;
    ir_instruction_t        *succ2;
    JITUINT32 count;

    /* Assertions				*/
    assert(method != NULL);
    assert(irLib != NULL);

    /* Erase the distance	*/
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        instr = IRMETHOD_getInstructionAtPosition(method, count);
        assert(instr != NULL);
        IRMETHOD_setInstructionDistance(method, instr, -1);
    }
    workList = xanList_new(allocFunction, freeFunction, NULL);
    assert(workList != NULL);

    /* Set the probability to execute the	*
     * first instruction			*/
    instr = IRMETHOD_getFirstInstruction(method);
    if (instr != NULL) {
        if (containInstructionType(instr)) {
            IRMETHOD_setInstructionDistance(method, instr, 1);
        } else {
            IRMETHOD_setInstructionDistance(method, instr, 0);
        }
        xanList_insert(workList, instr);
    }

    /* Compute the execution probability	*
     * of the instructions			*/
    while (xanList_length(workList)  > 0) {
        succ1 = NULL;
        succ2 = NULL;
        item = xanList_first(workList);
        assert(item != NULL);
        instr = item->data;
        assert(instr != NULL);
        xanList_deleteItem(workList, item);

        /* Fetch the successors			*/
        succ1 = IRMETHOD_getSuccessorInstruction(method, instr, NULL);
        if (succ1 == NULL) {
            continue;
        }
        markInstruction(method, instr, succ1, workList);
        succ2 = IRMETHOD_getSuccessorInstruction(method, instr, succ1);
        if (succ2 == NULL) {
            continue;
        }
        markInstruction(method, instr, succ2, workList);
    }

    /* Check the instructions		*/
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        instr = IRMETHOD_getInstructionAtPosition(method, count);
        assert(instr != NULL);
        if (IRMETHOD_getInstructionDistance(method, instr) == -1) {
            IRMETHOD_setInstructionDistance(method, instr, 0);
        }
    }

    /* Free the memory			*/
    xanList_destroyList(workList);

    /* Return				*/
    return;
}

static inline void markInstruction (ir_method_t *method, ir_instruction_t *instr, ir_instruction_t *succ, XanList *workList) {
    JITUINT32 	increment;
    JITINT16	d;
    JITINT16	instr_d;

    /* Assertions			*/
    assert(instr != NULL);
    assert(workList != NULL);

    if (containInstructionType(succ)) {
        increment = 1;
    } else {
        increment = 0;
    }

    d	= IRMETHOD_getInstructionDistance(method, succ);
    instr_d	= IRMETHOD_getInstructionDistance(method, instr);
    if ((d < 0) || (d > instr_d + increment)) {
        IRMETHOD_setInstructionDistance(method, succ, instr_d + increment);
        xanList_append(workList, succ);
    }

    return ;
}

static inline JITBOOLEAN containInstructionType (ir_instruction_t *instr) {

    /* Assertions			*/
    assert(instr != NULL);

    if (IRMETHOD_getInstructionType(instr) == IRCALL) {
        return JITTRUE;
    }
    if (IRMETHOD_getInstructionType(instr) == IRVCALL) {
        return JITTRUE;
    }
    if (IRMETHOD_getInstructionType(instr) == IRICALL) {
        return JITTRUE;
    }

    return JITFALSE;
}


static inline char * distance_get_version (void) {
    return VERSION;
}

static inline char * distance_get_informations (void) {
    return INFORMATIONS;
}

static inline char * distance_get_author (void) {
    return AUTHOR;
}
