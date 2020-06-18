/*
 * Copyright (C) 2010-2011  Timothy M. Jones and Simone Campanoni
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
#include <stdlib.h>
#include <xanlib.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
#include <chiara_profile.h>
#include <chiara_system.h>
// End


/**
 * Create a new profiling instruction before or after the given instruction
 * depending on whether this is a label or not.  The profiling function that
 * will be called takes only the instruction as an argument, but this can be
 * supplemented with additional parameters from the given list.
 */
void
INSTS_addBasicInstructionProfileCall(ir_method_t *method, ir_instruction_t *inst, char *profileName, void *profileFunc, XanList *otherParams) {
    ir_item_t *param;
    XanList *pCallParams;

    /* Create the native call back to this library. */
    pCallParams = xanList_new(allocFunction, freeFunction, NULL);
    param = allocFunction(sizeof(ir_item_t));
    param->value.v = (JITNINT)inst;
    param->type = IRNUINT;
    param->internal_type = param->type;
    xanList_append(pCallParams, param);
    if (otherParams) {
        xanList_appendList(pCallParams, otherParams);
    }

    /* Add the profile instruction after a label, before others. */
    if (inst->type == IRLABEL) {
        IRMETHOD_newNativeCallInstructionAfter(method, inst, profileName, profileFunc, NULL, pCallParams);
    } else {
        IRMETHOD_newNativeCallInstructionBefore(method, inst, profileName, profileFunc, NULL, pCallParams);
    }

    /* Clean up. */
    xanList_destroyListAndData(pCallParams);
}


/**
 * Profile the exits from the given loop.  If a loop successor has a non-loop
 * predecessor then the profile calls will be inserted in a new basic block
 * that is created along the edge between loop instructions and the successor.
 */
void
LOOPS_profileLoopExits(ir_method_t *method, loop_t *loop, char *exitFuncName, void *exitFunc) {
    XanList *succs = LOOPS_getSuccessorsOfLoop(loop);
    XanListItem *succItem = xanList_first(succs);

    /* Profile each successor. */
    while (succItem) {
        ir_instruction_t *succ;
        ir_instruction_t *pred;
        JITBOOLEAN nonLoopPred;

        /* Check whether a new basic block needs to be created. */
        succ = succItem->data;
        assert(succ);
        nonLoopPred = JITFALSE;
        pred = NULL;
        while (!nonLoopPred && (pred = IRMETHOD_getPredecessorInstruction(method, succ, pred))) {
            if (!xanList_find(loop->instructions, pred)) {
                nonLoopPred = JITTRUE;
            }
        }

        /* Add extra basic block if required. */
        if (nonLoopPred) {
            ir_instruction_t *newLabel;

            /* Create the new label and branch. */
            newLabel = IRMETHOD_newLabel(method);
            IRMETHOD_newBranchToLabelAfter(method, succ, newLabel);
            PDEBUG("Added new basic block before successor (%u) in %s\n", succ->ID, IRMETHOD_getMethodName(method));

            /* Update non-loop predecessors. */
            pred = NULL;
            while ((pred = IRMETHOD_getPredecessorInstruction(method, succ, pred))) {
                if (!xanList_find(loop->instructions, pred)) {
                    IRMETHOD_substituteLabel(method, pred, succ->param_1.value.v, newLabel->param_1.value.v);
                }
            }

            /* Loop successor is now the new label. */
            succ = newLabel;
        }

        /* Add the profiling call. */
        INSTS_addBasicInstructionProfileCall(method, succ, exitFuncName, exitFunc, NULL);
        PDEBUG("Added exit function at instruction (%u) in %s\n", succ->ID, IRMETHOD_getMethodName(method));
        succItem = succItem->next;
    }

    /* Clean up. */
    xanList_destroyList(succs);
}
