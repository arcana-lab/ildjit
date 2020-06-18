/*
 * Copyright (C) 2012 Campanoni Simone
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

// My headers
#include <optimizer_deadcode.h>
// End

static inline JITBOOLEAN internal_isEmpty (ir_method_t *method);
static inline void internal_inADeadMethod (char *methodName);

static JITUINT32 debug_deadMethods = 0;

extern ir_optimizer_t  *irOptimizer;

void eliminate_deadmethods (void) {
    XanList 	*l;
    XanList		*allMethods;
    XanListItem	*item;

    /* Fetch the set of methods stored in the system.
     */
    allMethods	= IRPROGRAM_getIRMethods();

    /* Fetch the set of methods needed by the program.
     */
    l 		= IRPROGRAM_getReachableMethodsNeededByProgram();

    /* Destroy methods that are not needed by the program (i.e., dead methods).
     */
    item		= xanList_first(allMethods);
    while (item != NULL) {
        ir_method_t	*m;

        /* Fetch the method.
         */
        m		= item->data;
        assert(m != NULL);

        /* Check if the method is dead.
         */
        if (	(!internal_isEmpty(m))					&&
                (!IRPROGRAM_doesMethodBelongToALibrary(m))		&&
                (xanList_find(l, m) == NULL)				) {
            XanList			*params;
            ir_instruction_t	*retInst;
            ir_instruction_t	retInstClone;
            ir_item_t		param;
            ir_item_t		*retInstParam;

            fprintf(stderr, "DEADMETHOD: %s\n", IRMETHOD_getSignatureInString(m));

            /* Check if we are debugging methods that we tag as dead.
             */
            if (debug_deadMethods) {

                /* Fetch a return instruction.
                 */
                retInst		= IRMETHOD_getFirstInstructionOfType(m, IRRET);
                assert(retInst != NULL);

                /* Adjust the instruction.
                 */
                retInstParam	= IRMETHOD_getInstructionParameter1(retInst);
                if (	(IRDATA_isAVariable(retInstParam))		&&
                        (retInstParam->internal_type != IRVALUETYPE)	) {
                    retInstParam->type	= retInstParam->internal_type;
                }
                memcpy(&retInstClone, retInst, sizeof(ir_instruction_t));
            }

            /* Reset the method.
             */
            IRMETHOD_deleteInstructions(m);
            IRMETHOD_addMethodDummyInstructions(m);

            /* Check if we are debugging methods that we tag as dead.
             */
            if (debug_deadMethods) {

                /* Add a call for debugging purpose.
                 */
                params			= xanList_new(allocFunction, freeFunction, NULL);
                memset(&param, 0, sizeof(ir_item_t));
                xanList_append(params, &param);
                param.value.v		= (IR_ITEM_VALUE)(JITNUINT)IRMETHOD_getSignatureInString(m);
                param.internal_type	= IRNUINT;
                param.type		= param.internal_type;
                IRMETHOD_newNativeCallInstruction(m, "inADeadMethod", internal_inADeadMethod, NULL, params);
                xanList_destroyList(params);

                /* Add the return instruction.
                 */
                IRMETHOD_cloneInstruction(m, &retInstClone);
            }
        }

        /* Fetch the next element from the list.
         */
        item		= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(allMethods);
    xanList_destroyList(l);

    return ;
}

static inline void internal_inADeadMethod (char *methodName) {
    ir_method_t *runningMethod;

    fprintf(stderr, "A deadmethod is required at run time: %s\n", methodName);
    fprintf(stderr, "Call stack:\n");
    runningMethod	= IRMETHOD_getRunningMethod();
    if (runningMethod != NULL) {
        ir_method_t	*caller;

        caller	= runningMethod;
        while (caller != NULL) {
            fprintf(stderr, "	%s\n", IRMETHOD_getSignatureInString(caller));
            caller	= IRMETHOD_getCallerOfMethodInExecution(caller);
        }
    }
    abort();
}

static inline JITBOOLEAN internal_isEmpty (ir_method_t *method) {
    ir_instruction_t	*inst;
    ir_instruction_t	*nextInst;

    if (!IRMETHOD_hasMethodInstructions(method)) {
        return JITTRUE;
    }
    inst		= IRMETHOD_getFirstInstructionNotOfType(method, IRNOP);
    if (inst == NULL) {
        return JITTRUE;
    }
    nextInst	= IRMETHOD_getNextInstruction(method, inst);
    assert(nextInst != NULL);
    if (	(inst->type == IRNATIVECALL)		&&
            (nextInst->type == IRRET)		) {
        return JITTRUE;
    }

    return JITFALSE;
}
