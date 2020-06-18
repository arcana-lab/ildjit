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
#include <chiara.h>

// My headers
#include <optimizer_memorytracer.h>
#include <code_injector.h>
#include <runtime.h>
// End

static inline void internal_injectCodeToDumpDependenceTrace (ir_method_t *method, XanHashTable *insts, XanHashTable *seenMethods);
static inline void internal_inject_code_to_dump_trace_in_method (ir_method_t *m, XanHashTable *instsToConsider);

extern ir_optimizer_t                   *irOptimizer;
XanHashTable				*methodNamesTable	= NULL;

void inject_code_to_program (void) {
    XanHashTable		*insts;
    XanHashTable		*methodsToConsider;
    XanHashTable		*seenMethods;
    XanHashTable		*loops;
    XanHashTable		*loopNames;
    XanHashTable		*loopDict;
    XanList			*loopsList;
    XanListItem			*loopItem;

    /* Fetch the set of instructions to consider.
     */
    methodsToConsider	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    insts			= MISC_loadInstructionDictionary(methodsToConsider, "instruction_IDs.txt", NULL, NULL);

    /* Analyze the loops.
     */
    loops			= IRLOOP_analyzeLoops(LOOPS_analyzeCircuits);
    assert(loops != NULL);
    loopNames 		= LOOPS_getLoopsNames(loops);
    assert(loopNames != NULL);

    /* Load the loop dictionary.
     */
    loopDict 		= MISC_loadLoopDictionary("loop_IDs.txt", NULL);
    assert(loopDict != NULL);

    /* Choose the loops to analyze.
     */
    loopsList		= xanHashTable_toList(loops);
    MISC_chooseLoopsToProfile(loopsList, loopNames, loopDict);
    if (xanList_length(loopsList) == 0) {
        fprintf(stderr, "MEMORY TRACER: ERROR = no loop has been selected to profile\n");
        fprintf(stderr, "Please check that the file loop_IDs.txt exists in the working directory and that the environment variables PARALLELIZER_LOWEST_LOOPS_TO_PARALLELIZE and PARALLELIZER_MAXIMUM_LOOPS_TO_PARALLELIZE have been set properly.\n");
        abort();
    } else if (xanList_length(loopsList) > 1) {
        fprintf(stderr, "MEMORY TRACER: Warning: More than one loop selected to profile\n");
        fprintf(stderr, "This plugin may not work correctly with more than one loop\n");
    }

    /* Profile the boundaries of the selected loops.
     */
    LOOPS_profileLoopBoundaries(irOptimizer, loopsList, JITFALSE, loopNames, RUNTIME_enter_loop, RUNTIME_end_loop, NULL, NULL);

    /* Get each loop method and profile instructions.
     */
    seenMethods = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    loopItem = xanList_first(loopsList);
    while (loopItem) {
        loop_t *loop = loopItem->data;

        /* Inject the code.
         */
        internal_injectCodeToDumpDependenceTrace(loop->method, insts, seenMethods);
        loopItem = loopItem->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(loopsList);
    xanHashTable_destroyTable(insts);
    xanHashTable_destroyTable(loopDict);
    xanHashTable_destroyTable(seenMethods);
    xanHashTable_destroyTable(methodsToConsider);

    return ;
}

static inline void internal_injectCodeToDumpDependenceTrace (ir_method_t *method, XanHashTable *insts, XanHashTable *seenMethods) {
    XanList *reachableMethods;
    XanListItem	*item;

    /* Get all methods reachable from this one.
     */
    reachableMethods = IRPROGRAM_getReachableMethods(method);

    /* Inject code to profile specified instructions.
     */
    item = xanList_first(reachableMethods);
    while (item != NULL) {
        ir_method_t		*m;

        /* Fetch the method.
         */
        m		= item->data;
        assert(m != NULL);

        /* Inject code for memory instructions.
         */
        if (!xanHashTable_lookup(seenMethods, m)) {
            JITINT8 *methodName = IRMETHOD_getSignatureInString(m);
            if (methodName != NULL &&
                IRMETHOD_hasMethodInstructions(m) &&
                STRCMP(IRMETHOD_getMethodName(m), ".cctor") != 0) {
                internal_inject_code_to_dump_trace_in_method(m, insts);
            }
            xanHashTable_insert(seenMethods, m, m);
        }

        /* Fetch the next element.
         */
        item		= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(reachableMethods);

    return ;
}

static inline void internal_inject_code_to_dump_trace_in_method (ir_method_t *m, XanHashTable *instsToConsider) {
    XanList			*insts;
    XanList			*callInsts;
    XanList			*params;
    XanListItem		*item;
    XanList			*l;
    ir_item_t		methodNameParam;
    ir_instruction_t	*firstInst;
    ir_item_t		instPosParam;
    ir_item_t		input1Param;
    ir_item_t		input1SizeParam;
    ir_item_t		input2Param;
    ir_item_t		input2SizeParam;
    ir_item_t		outputParam;
    ir_item_t		outputSizeParam;
    JITINT8			*methodName;
    fprintf(stderr, "Profiling method %s\n", IRMETHOD_getCompleteMethodName(m));

    /* Assertions.
     */
    assert(m != NULL);
    assert(instsToConsider != NULL);

    /* Fetch the name of the method.
     */
    methodName	= IRMETHOD_getSignatureInString(m);
    assert(methodName != NULL);

    /* Fetch the first instruction.
     */
    firstInst	= IRMETHOD_getFirstInstructionNotOfType(m, IRNOP);
    if (firstInst->type == IRLABEL) {
        firstInst	= IRMETHOD_newInstructionOfTypeBefore(m, firstInst, IRNOP);
    }
    assert(firstInst != NULL);
    assert(firstInst->type != IRLABEL);

    /* Fetch the memory instructions that belong to the method.
     */
    insts		= IRMETHOD_getMemoryInstructions(m);
    assert(insts != NULL);

    /* Fetch the call instructions that belong to the method.
     */
    callInsts	= IRMETHOD_getCallInstructions(m);
    assert(callInsts != NULL);

    /* Allocate the necessary memory.
     */
    params		= xanList_new(allocFunction, freeFunction, NULL);

    /* Inject the call for the begin of the method.
     */
    memset(&methodNameParam, 0, sizeof(ir_item_t));
    methodNameParam.value.v		= (IR_ITEM_VALUE)(JITNUINT)methodName;
    methodNameParam.internal_type	= IRNUINT;
    methodNameParam.type		= methodNameParam.internal_type;
    xanList_insert(params, &methodNameParam);
    IRMETHOD_newNativeCallInstructionBefore(m, firstInst, "MemoryInstructionDumperBeginMethod", dump_start_method, NULL, params);
    xanList_emptyOutList(params);

    /* Inject the call for the end of the method.
     */
    l	= IRMETHOD_getInstructionsOfType(m, IRRET);
    item	= xanList_first(l);
    while (item != NULL) {
        ir_instruction_t	*retInst;

        /* Fetch the return instruction.
         */
        retInst	= item->data;
        assert(retInst != NULL);

        /* Inject the code.
         */
        IRMETHOD_newNativeCallInstructionBefore(m, retInst, "MemoryInstructionDumperEndMethod", dump_end_method, NULL, NULL);

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(l);

    /* Fill up the call parameters.
     */
    memset(&instPosParam, 0, sizeof(ir_item_t));
    memset(&input1Param, 0, sizeof(ir_item_t));
    memset(&input1SizeParam, 0, sizeof(ir_item_t));
    memset(&input2Param, 0, sizeof(ir_item_t));
    memset(&input2SizeParam, 0, sizeof(ir_item_t));
    memset(&outputParam, 0, sizeof(ir_item_t));
    memset(&outputSizeParam, 0, sizeof(ir_item_t));
    xanList_append(params, &instPosParam);
    xanList_append(params, &input1Param);
    xanList_append(params, &input1SizeParam);
    xanList_append(params, &input2Param);
    xanList_append(params, &input2SizeParam);
    xanList_append(params, &outputParam);
    xanList_append(params, &outputSizeParam);
    instPosParam.internal_type	= IRUINT32;
    instPosParam.type		= instPosParam.internal_type;

    /* Inject the code.
     */
    item					= xanList_first(insts);
    while (item != NULL) {
        ir_instruction_t	*inst;
        ir_instruction_t	*instPrev;

        /* Fetch the memory instruction.
         */
        inst		= item->data;
        assert(inst != NULL);

        /* Fetch the previous instruction.
         */
        instPrev	= IRMETHOD_getPrevInstruction(m, inst);
        assert(instPrev != NULL);

        /* Check if the instruction is used to forward variables shared among loop iterations.
         */
        if (    (inst->type != IRARRAYLENGTH)   &&
                (inst->type != IRSTRINGCHR)     ){
            ir_item_t		*par1;
            ir_item_t		*par2;
            ir_item_t		*par3;
            ir_item_t		*res;
            ir_instruction_t	*strLen1;
            ir_instruction_t	*strLen2;
            ir_instruction_t	*add1;
            ir_instruction_t	*add2;

            /* Fetch the parameters of the instruction.
             */
            par1	= IRMETHOD_getInstructionParameter1(inst);
            par2	= IRMETHOD_getInstructionParameter2(inst);
            par3	= IRMETHOD_getInstructionParameter3(inst);
            res	= IRMETHOD_getInstructionResult(inst);

            /* Fetch the position of the memory instruction.
             */
            instPosParam.value.v		= ((JITUINT32)(JITNUINT)xanHashTable_lookup(instsToConsider, inst));

            /* Check if the current instruction needs to be considered.
             */
            if (instPosParam.value.v != 0) {

                /* Set the types of the parameters of the call.
                 */
                input1Param.internal_type	= IRNUINT;
                input1Param.type		= input1Param.internal_type;
                input1SizeParam.internal_type	= IRUINT32;
                input1SizeParam.type		= input1SizeParam.internal_type;
                input2Param.internal_type	= input1Param.internal_type;
                input2Param.type		= input2Param.internal_type;
                input2SizeParam.internal_type	= input1SizeParam.internal_type;
                input2SizeParam.type		= input2SizeParam.internal_type;
                outputParam.internal_type	= IRNUINT;
                outputParam.type		= outputParam.internal_type;
                outputSizeParam.internal_type	= IRUINT32;
                outputSizeParam.type		= outputSizeParam.internal_type;

                /* Inject the code to dump the instruction at run time.
                 */
                input1Param.value.v		= 0;
                input1SizeParam.value.v		= 0;
                input2Param.value.v		= 0;
                input2SizeParam.value.v		= 0;
                outputParam.value.v		= 0;
                outputSizeParam.value.v		= 0;
                switch (inst->type) {
                    case IRLOADREL:
                        if (par2->value.v == 0) {
                            memcpy(&input1Param, par1, sizeof(ir_item_t));

                        } else {
                            ir_instruction_t	*addInst;
                            ir_item_t		*addInstResult;
                            addInst		= IRMETHOD_newInstructionOfTypeBefore(m, inst, IRADD);
                            IRMETHOD_cpInstructionParameter1(addInst, par1);
                            IRMETHOD_cpInstructionParameter2(addInst, par2);
                            IRMETHOD_setInstructionParameterWithANewVariable(m, addInst, par1->internal_type, par1->type_infos, 0);
                            addInstResult	= IRMETHOD_getInstructionResult(addInst);
                            memcpy(&input1Param, addInstResult, sizeof(ir_item_t));
                        }
                        input1SizeParam.value.v	= IRDATA_getSizeOfType(res);
                        break;
                    case IRSTOREREL:
                        if (par2->value.v == 0) {
                            memcpy(&outputParam, par1, sizeof(ir_item_t));

                        } else {
                            ir_instruction_t	*addInst;
                            ir_item_t		*addInstResult;
                            addInst		= IRMETHOD_newInstructionOfTypeBefore(m, inst, IRADD);
                            IRMETHOD_cpInstructionParameter1(addInst, par1);
                            IRMETHOD_cpInstructionParameter2(addInst, par2);
                            IRMETHOD_setInstructionParameterWithANewVariable(m, addInst, par1->internal_type, par1->type_infos, 0);
                            addInstResult	= IRMETHOD_getInstructionResult(addInst);
                            memcpy(&outputParam, addInstResult, sizeof(ir_item_t));
                        }
                        outputSizeParam.value.v	= IRDATA_getSizeOfType(par3);
                        break;
                    case IRMEMCPY:
                        memcpy(&input1Param, par2, sizeof(ir_item_t));
                        memcpy(&outputParam, par1, sizeof(ir_item_t));
                        outputSizeParam.value.v	= IRDATA_getSizeOfType(par3);
                        input1SizeParam.value.v	= outputSizeParam.value.v;
                        break;
                    case IRMEMCMP:
                        memcpy(&input1Param, par1, sizeof(ir_item_t));
                        memcpy(&input2Param, par2, sizeof(ir_item_t));
                        input1SizeParam.value.v	= IRDATA_getSizeOfType(par3);
                        input2SizeParam.value.v	= input1SizeParam.value.v;
                        break;
                    case IRINITMEMORY:
                        memcpy(&outputParam, par1, sizeof(ir_item_t));
                        outputSizeParam.value.v	= IRDATA_getSizeOfType(par3);
                        break;
                    case IRSTRINGCMP:
                        memcpy(&input1Param, par1, sizeof(ir_item_t));
                        strLen1	= IRMETHOD_newInstructionOfTypeAfter(m, instPrev, IRSTRINGLENGTH);
                        IRMETHOD_cpInstructionParameter(m, inst, 1, strLen1, 1);
                        IRMETHOD_setInstructionParameterWithANewVariable(m, strLen1, IRUINT32, NULL, 0);

                        add1	= IRMETHOD_newInstructionOfTypeAfter(m, strLen1, IRADD);
                        IRMETHOD_cpInstructionParameter(m, strLen1, 0, add1, 1);
                        IRMETHOD_setInstructionParameter2(m, add1, 1, 0, IRUINT32, IRUINT32, NULL);
                        IRMETHOD_cpInstructionParameter(m, add1, 1, add1, 0);

                        IRMETHOD_cpInstructionParameterToItem(m, add1, 0, &input1SizeParam);

                        memcpy(&input2Param, par2, sizeof(ir_item_t));
                        strLen2	= IRMETHOD_newInstructionOfTypeAfter(m, instPrev, IRSTRINGLENGTH);
                        IRMETHOD_cpInstructionParameter(m, inst, 2, strLen2, 1);
                        IRMETHOD_setInstructionParameterWithANewVariable(m, strLen2, IRUINT32, NULL, 0);

                        add2	= IRMETHOD_newInstructionOfTypeAfter(m, strLen2, IRADD);
                        IRMETHOD_cpInstructionParameter(m, strLen2, 0, add2, 1);
                        IRMETHOD_setInstructionParameter2(m, add2, 1, 0, IRUINT32, IRUINT32, NULL);
                        IRMETHOD_cpInstructionParameter(m, add2, 1, add2, 0);

                        IRMETHOD_cpInstructionParameterToItem(m, add2, 0, &input2SizeParam);
                        break ;
                    case IRSTRINGLENGTH:
                        memcpy(&input1Param, par1, sizeof(ir_item_t));
                        strLen1	= IRMETHOD_newInstructionOfTypeAfter(m, instPrev, IRSTRINGLENGTH);
                        IRMETHOD_cpInstructionParameter(m, inst, 1, strLen1, 1);
                        IRMETHOD_setInstructionParameterWithANewVariable(m, strLen1, IRUINT32, NULL, 0);

                        add1	= IRMETHOD_newInstructionOfTypeAfter(m, strLen1, IRADD);
                        IRMETHOD_cpInstructionParameter(m, strLen1, 0, add1, 1);
                        IRMETHOD_setInstructionParameter2(m, add1, 1, 0, IRUINT32, IRUINT32, NULL);
                        IRMETHOD_cpInstructionParameter(m, add1, 1, add1, 0);

                        IRMETHOD_cpInstructionParameterToItem(m, add1, 0, &input1SizeParam);
                        break ;
                    default:
                        fprintf(stderr, "Instruction %s is not handled.\n", IRMETHOD_getInstructionTypeName(inst->type));
                        abort();
                }
                IRMETHOD_newNativeCallInstructionBefore(m, inst, "MemoryInstructionDumper", dump_memory_instruction, NULL, params);
            }
        }

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Inject code to dump the call instructions.
     */
    instPosParam.internal_type	= IRUINT32;
    instPosParam.type		    = instPosParam.internal_type;
    item		                = xanList_first(callInsts);
    while (item != NULL) {
        ir_instruction_t	*inst;

        /* Fetch the memory instruction.
         */
        inst	= item->data;
        assert(inst != NULL);

        /* Check if the current instruction has to be considered.
         */
        if (xanHashTable_lookup(instsToConsider, inst) != NULL) {
            ir_instruction_t	*nativeCall;

            /* Fetch the position of the call instruction.
             */
            instPosParam.value.v		= ((JITUINT32)(JITNUINT)xanHashTable_lookup(instsToConsider, inst));

            /* If the call instruction is a call to OS, then inject the code.
             */
            if (inst->type == IRLIBRARYCALL) {

                /* Inject the code.
                 */
                nativeCall  = IRMETHOD_newNativeCallInstructionBefore(m, inst, "OSDumper", dump_os_instruction, NULL, NULL);
                IRMETHOD_addInstructionCallParameter(m, nativeCall, &instPosParam);

            } else if (inst->type != IRNATIVECALL) {

                /* Inject the code.
                 */
                nativeCall	= IRMETHOD_newNativeCallInstructionBefore(m, inst, "CallDumper", dump_call_instruction, NULL, NULL);
                IRMETHOD_addInstructionCallParameter(m, nativeCall, &instPosParam);
            }
        }

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(params);
    xanList_destroyList(insts);
    xanList_destroyList(callInsts);

    return ;
}
