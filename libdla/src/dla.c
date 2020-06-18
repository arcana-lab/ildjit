/*
 * Copyright (C) 2009 Campanoni Simone
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
 * @file dla.c
 */
#include <stdio.h>
#include <assert.h>
#include <compiler_memory_manager.h>
#include <ir_optimization_interface.h>
#include <ilmethod.h>
#include <jit_metadata.h>

// My headers
#include <dla.h>
#include <config.h>
// End

static inline void internal_shutdown (DLA_t *_this);
static inline XanList * internal_getMethodsToCompile (DLA_t *_this, Method method, JITBOOLEAN *toBeCompiled);
static inline void internal_updateMethodsToCompile (DLA_t * _this, Method method, XanList *methods, JITBOOLEAN *toBeCompiled);

void DLANew (DLA_t *_this, IRVM_t *IRVM) {
    /* Assertions                   */
    assert(_this != NULL);

    _this->IRVM                     = IRVM;
    _this->shutdown                 = internal_shutdown;
    _this->getMethodsToCompile      = internal_getMethodsToCompile;
    _this->updateMethodsToCompile   = internal_updateMethodsToCompile;
}

static inline void internal_shutdown (DLA_t *_this) {

    /* Assertions                   */
    assert(_this != NULL);

}

static inline XanList * internal_getMethodsToCompile (DLA_t *_this, Method method, JITBOOLEAN *toBeCompiled) {
    ir_method_t *irMethod;
    XanList                 *instrs;
    ir_instruction_t        *inst;
    JITUINT32 count;
    JITFLOAT32 executionProbability;
    JITUINT32 instructionsNumber;
    ir_optimizer_t          *optimizer;

    /* Assertions                                   */
    assert(_this != NULL);
    assert(method != NULL);

    /* Initialize the variables                     */
    instrs          = NULL;
    inst            = NULL;

    method->lock(method);
    executionProbability = method->executionProbability;
    method->unlock(method);

    /* Check the Boundaries				*/
    (*toBeCompiled) = !((executionProbability <= _this->probabilityBoundary) || (executionProbability == -1));

    /* Cache some pointers				*/
    optimizer = &(_this->IRVM->optimizer);
    irMethod = method->getIRMethod(method);

    /* Make the list of methods callable		*/
    instrs          = xanList_new(allocFunction, freeFunction, NULL);
    assert(instrs != NULL);

    IRMETHOD_lock(irMethod);
    /* Call the branch predictor			*/
    IROPTIMIZER_callMethodOptimization(optimizer, irMethod, BRANCH_PREDICTOR);
    /* Call the call distance computer		*/
    IROPTIMIZER_callMethodOptimization(optimizer, irMethod, CALL_DISTANCE_COMPUTER);
    IRMETHOD_unlock(irMethod);

    /* Fetch the instructions number		*/
    instructionsNumber      = IRMETHOD_getInstructionsNumber(irMethod);

    /* Make the list of IR call instructions	*/
    for (count = 0; count < instructionsNumber; count++) {

        /* Fetch the instruction			*/
        inst    = IRMETHOD_getInstructionAtPosition(irMethod, count);
        assert(inst != NULL);

        if ((inst->type == IRCALL || inst->type == IRICALL) && (inst->executionProbability > 0) ) {
            JITFLOAT32	executionDistance;

            /* Fetch the instruction distance		*/
            executionDistance	= (JITFLOAT32) IRMETHOD_getInstructionDistance(irMethod, inst);

            /* Apply prediction Model */
            JITFLOAT32 callableMethodExecutionProbability = inst->executionProbability * executionProbability;
            JITFLOAT32 callableMethodIntermediatePriority = inst->executionProbability / executionDistance;
            JITFLOAT32 callableMethodPriority = callableMethodExecutionProbability / executionDistance;
            assert(((callableMethodPriority > 0.0 && callableMethodPriority <= 1.0) || !(*toBeCompiled)));

            /* Get all callable Methods		*/
            XanList *callableMethods = IRMETHOD_getCallableMethods(inst);
            XanListItem *item = xanList_first(callableMethods);
            while (item != NULL) {
                Method newMethod = (Method) item->data;

                if (newMethod != method) {
                    newMethod->lock(newMethod);
                    /* Update the execution probability for callable method		*/
                    if (((newMethod->executionProbability < callableMethodExecutionProbability) || newMethod->executionProbability < 0)) {
                        newMethod->executionProbability    = callableMethodExecutionProbability;
                    }
                    newMethod->unlock(newMethod);

                    JITBOOLEAN found = JITFALSE;
                    XanListItem *item2 = xanList_first(instrs);
                    while (item2 != NULL) {
                        dla_method_t *currentDLAMethod = (dla_method_t *) item2->data;
                        if (currentDLAMethod->method == newMethod && callableMethodPriority > currentDLAMethod->priority) {
                            /* Method exists update execution propability	*/
                            currentDLAMethod->callInstructionProbability = inst->executionProbability;
                            currentDLAMethod->priority = callableMethodPriority;
                            currentDLAMethod->intermediatePriority = callableMethodIntermediatePriority;
                            found = JITTRUE;
                            break;
                        }
                        item2 = item2->next;
                    }
                    if (!found) {
                        dla_method_t *newDLAMethod = (dla_method_t *) allocFunction(sizeof(dla_method_t));
                        newDLAMethod->method = newMethod;
                        newDLAMethod->priority = callableMethodPriority;
                        newDLAMethod->callInstructionProbability = inst->executionProbability;
                        newDLAMethod->intermediatePriority = callableMethodIntermediatePriority;
                        xanList_append(instrs, newDLAMethod);
                    }
                }

                item = item->next;
            }

            /* Free the memory                      */
            xanList_destroyList(callableMethods);
        }
    }

    /* Return                                       */
    return instrs;
}

static inline void internal_updateMethodsToCompile (DLA_t * _this, Method method, XanList *methods, JITBOOLEAN *toBeCompiled) {
    XanListItem *item;
    JITFLOAT32 executionProbability;

    /* Assertions                                   */
    assert(_this != NULL);
    assert(method != NULL);
    assert(methods != NULL);

    method->lock(method);
    executionProbability = method->executionProbability;
    method->unlock(method);

    /* Check the Boundaries				*/
    (*toBeCompiled) = !((executionProbability <= _this->probabilityBoundary) || (executionProbability == -1));

    item = xanList_first(methods);
    while (item != NULL) {
        dla_method_t *currentDLAMethod = (dla_method_t *) item->data;

        JITFLOAT32 callableMethodExecutionProbability = currentDLAMethod->callInstructionProbability * executionProbability;
        Method newMethod = currentDLAMethod->method;

        newMethod->lock(newMethod);
        /* Update the execution probability for callable method		*/
        if (((newMethod->executionProbability < callableMethodExecutionProbability) || newMethod->executionProbability < 0)) {
            newMethod->executionProbability    = callableMethodExecutionProbability;
        }
        newMethod->unlock(newMethod);

        /* Update the priority for callable method		*/
        currentDLAMethod->priority = currentDLAMethod->intermediatePriority * executionProbability;
        item = item->next;
    }
}

void libdlaCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf((char *) buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat((char *) buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat((char *) buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat((char *) buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

void libdlaCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libdlaVersion () {
    return VERSION;
}
