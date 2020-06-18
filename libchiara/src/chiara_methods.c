/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
// End

//#define DUMP_CFG

/**
 * Information required for Tarjan's stronly connected components algorithm.
 */
typedef struct {
    int index;
    int lowlink;
} tarjan_scc_t;

#define MIN(a, b) ((a) > (b) ? (b) : (a))

static inline JITBOOLEAN internal_considerOnlyCallInstructions (ir_method_t *method, ir_instruction_t *callInst);
static inline void internal_add_empty_basic_block (ir_method_t *method, ir_instruction_t *basic_block_predecessor, ir_instruction_t *basic_block_successor);
static inline JITINT32 internal_deepInlineMethodWithoutRecursion (ir_optimizer_t *irOptimizer, ir_method_t *method, ir_instruction_t *inst, XanList *methodsAlreadyInlined, void (*removeCalls)(ir_method_t *method, XanList *newCalls), XanHashTable *callerGraph, JITUINT32 minimumCallersNumber, JITBOOLEAN inlineEmptyMethods);
static inline JITINT32 internal_thereIsRecursionWithMethod (ir_method_t *methodToInline, XanList *methodsAlreadyConsidered);
static inline void internal_inlineMethod (ir_optimizer_t *irOptimizer, ir_method_t *method, ir_instruction_t *inst, ir_method_t *methodToInline, JITBOOLEAN optimizeMethod);
static inline JITINT32 internal_findDirectRecursion (ir_method_t *method, ir_method_t *startMethod, ir_instruction_t *inst, XanList *methodsAlreadyConsidered);
static inline JITINT32 internal_findRecursion (ir_method_t *method, ir_instruction_t *inst, XanList *methodsAlreadyConsidered);
static inline void internal_adjustInlinedInstruction (ir_method_t *method, ir_instruction_t *inst, JITUINT32 variablesOffset, JITUINT32 labelsOffset, JITUINT32 *variablesMapping, JITUINT32 parametersNumber);
static inline void internal_adjust_label (ir_item_t *item, IR_ITEM_VALUE labelsOffset);
static inline void internal_adjust_variable (ir_item_t *item, JITUINT32 variablesOffset, JITUINT32 *variablesMapping, JITUINT32 parametersNumber);
static inline JITBOOLEAN internal_METHODS_isReachableWithoutGoingThroughAMethod (ir_method_t *startMethod, ir_method_t *endMethod, XanList *methodsToNotGoThrough, XanList *alreadyConsidered, XanList *trace, XanList *escapedMethods, JITBOOLEAN (*toConsider)(ir_method_t *method, ir_instruction_t *callInst));
static inline JITBOOLEAN internal_isCalledDirectly (ir_method_t *caller, ir_method_t *callee);

XanList * METHODS_getCallableIRMethods (ir_instruction_t *inst, XanList *escapedMethods) {
    XanList *l;
    XanList *temp;
    XanList *r;
    XanList *e;
    XanList *toDelete;
    XanListItem *item;
    ir_method_t *mainMethod;

    /* Assertions			*/
    assert(inst != NULL);

    /* Fetch the list of callable	*
     * methods			*/
    l = IRMETHOD_getCallableIRMethods(inst);
    if (l == NULL) {
        return NULL;
    }
    assert(l != NULL);

    /* Check if the list is empty	*
     * Notice that this is possible	*
     * when a not IR method is 	*
     * called			*/
    if (xanList_length(l) == 0) {
        return l;
    }
    assert(xanList_length(l) > 0);

    /* Check if the call is direct	*/
    switch(inst->type) {
        case IRCALL:
        case IRLIBRARYCALL:
        case IRNATIVECALL:
            return l;
    }

    /* Check if the indirect call	*
     * can call not escaped methods	*/
    switch(inst->type) {
        case IRVCALL:
            return l;
    }

    /* Fetch the list of reachable 	*
     * methods			*/
    mainMethod = METHODS_getMainMethod();
    assert(mainMethod != NULL);
    temp = IRPROGRAM_getReachableMethods(mainMethod);
    assert(temp != NULL);
    r = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(temp);
    while (item != NULL) {
        ir_method_t *m;
        IR_ITEM_VALUE id;
        m = item->data;
        assert(m != NULL);
        id = IRMETHOD_getMethodID(m);
        if (id != 0) {
            xanList_append(r, m);
        }
        item = item->next;
    }
    xanList_destroyList(temp);
    assert(r != NULL);

    /* Fetch the list of methods	*
     * that are escaped (they could	*
     * be called indirectly)	*/
    if (escapedMethods == NULL) {
        temp = IRPROGRAM_getEscapedMethods();
    } else {
        temp = escapedMethods;
    }
    assert(temp != NULL);
    e = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(temp);
    while (item != NULL) {
        ir_method_t *m;
        IR_ITEM_VALUE id;
        m = item->data;
        assert(m != NULL);
        id = IRMETHOD_getMethodID(m);
        if (id != 0) {
            xanList_append(e, m);
        }
        item = item->next;
    }
    if (escapedMethods == NULL) {
        xanList_destroyList(temp);
    }

    /* Consider only methods that 	*
     * are both reachable and 	*
     * escaped			*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t *m;
        m = item->data;
        if (	(xanList_find(r, m) == NULL)	||
                (xanList_find(e, m) == NULL)	) {
            xanList_append(toDelete, m);
        }
        item = item->next;
    }
    item = xanList_first(toDelete);
    while (item != NULL) {
        xanList_removeElement(l, item->data, JITTRUE);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(r);
    xanList_destroyList(e);
    xanList_destroyList(toDelete);

    /* Return			*/
    return l;
}

static inline void internal_METHODS_inlineCalledMethodsNotInSubloops_removeCalls(ir_method_t *method, XanList *newCalls) {
    static loop_t *loop = NULL;
    static XanHashTable *loops = NULL;
    static void (*removeCalls)(ir_method_t *, XanList *) = NULL;
    static ir_optimizer_t *irOptimizer = NULL;
    XanList *toDelete;
    XanListItem *item;

    /* Assertions				*/
    assert(method != NULL);

    /* Initialize the function		*/
    if (newCalls == NULL) {
        if (loops == NULL) {
            loops = (XanHashTable *) method;
            assert(loops != NULL);
        } else if (loop == NULL) {
            loop = (loop_t *) method;
            assert(loop != NULL);
        } else if (removeCalls == NULL) {
            removeCalls = (void (*)(ir_method_t *, XanList *)) method;
            assert(removeCalls != NULL);
        } else {
            irOptimizer = (ir_optimizer_t *) method;
            assert(irOptimizer != NULL);
        }
        return ;
    }
    assert(loops != NULL);
    assert(loop != NULL);
    assert(removeCalls != NULL);
    assert(method != NULL);
    assert(newCalls != NULL);
    assert(irOptimizer != NULL);

    /* Remove every call that belongs	*
     * to subloops				*/
    if (xanList_length(newCalls) > 0) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, LOOP_IDENTIFICATION);
        toDelete = xanList_new(allocFunction, freeFunction, NULL);
        item = xanList_first(newCalls);
        while (item != NULL) {
            ir_instruction_t *inst;
            loop_t *instLoop;
            inst = item->data;
            instLoop = IRLOOP_getLoopFromInstruction(method, inst, loops);
            if (	(instLoop != NULL)	&&
                    (instLoop != loop)	) {
                xanList_append(toDelete, inst);
            }
            item = item->next;
        }
        item = xanList_first(toDelete);
        while (item != NULL) {
            xanList_removeElement(newCalls, item->data, JITTRUE);
            item = item->next;
        }
        xanList_destroyList(toDelete);
    }

    /* Remove every other call		*/
    (*removeCalls)(method, newCalls);

    /* Return				*/
    return ;
}

JITBOOLEAN METHODS_inlineCalledMethodsNotInSubloops (ir_optimizer_t *irOptimizer, XanHashTable *loops, loop_t *loop, void (*removeCalls)(ir_method_t *method, XanList *newCalls)) {
    JITUINT32 		instructionsNumber;
    JITUINT32 		count;
    JITBOOLEAN		inlined;
    XanList 		*callInstructions;

    /* Assertions				*/
    assert(irOptimizer != NULL);
    assert(loops != NULL);
    assert(loop != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(loop->method);

    /* Fetch the set of calls to consider	*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, LOOP_IDENTIFICATION);
    callInstructions = xanList_new(allocFunction, freeFunction, NULL);
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *tmp_inst;
        loop_t 			*instLoop;
        tmp_inst = IRMETHOD_getInstructionAtPosition(loop->method, count);
        assert(tmp_inst != NULL);
        if (tmp_inst->type != IRCALL) {
            continue;
        }

        instLoop = IRLOOP_getLoopFromInstruction(loop->method, tmp_inst, loops);
        if (instLoop == loop) {
            xanList_append(callInstructions, tmp_inst);
        }
    }

    /* Initialize the removeCalls function	*/
    internal_METHODS_inlineCalledMethodsNotInSubloops_removeCalls((ir_method_t *)loops, NULL);
    internal_METHODS_inlineCalledMethodsNotInSubloops_removeCalls((ir_method_t *)loop, NULL);
    internal_METHODS_inlineCalledMethodsNotInSubloops_removeCalls((ir_method_t *)removeCalls, NULL);
    internal_METHODS_inlineCalledMethodsNotInSubloops_removeCalls((ir_method_t *)irOptimizer, NULL);

    /* Inline the methods			*/
    inlined = METHODS_inlineCalledMethods(irOptimizer, loop->method, callInstructions, internal_METHODS_inlineCalledMethodsNotInSubloops_removeCalls, 1);

    /* Free the memory			*/
    xanList_destroyList(callInstructions);

    /* Return				*/
    return inlined;
}


/**
 * Determine whether a method is definitely recursive.  This means it calls
 * itself via an IRCALL instruction.
 **/
static inline JITBOOLEAN
internal_isDefinitelyRecursiveMethod(ir_method_t *method) {
    JITUINT32 i, numInsts;

    /* Checks. */
    assert(method != NULL);

    /* Find calls and check them. */
    numInsts = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        if (inst->type == IRCALL) {
            ir_method_t *callee = IRMETHOD_getCalledMethod(method, inst);
            if (callee == method) {
                return JITTRUE;
            }
        }
    }

    /* No recursion found. */
    return JITFALSE;
}


/**
 * Inlines the method called by @c call into @c method, providing that this
 * method to inline is either alone in its strongly connected component or
 * is in the same strongly connected component as @c method and is not a
 * recursive method.
 **/
static inline JITBOOLEAN
internal_inlineMethodNoRecursion(ir_optimizer_t *irOptimizer, ir_method_t *method, ir_instruction_t *call, XanHashTable *sccMap, void (*removeCalls)(ir_method_t *method, XanList *newCalls), JITUINT32 maxMethodInsts) {
    ir_method_t *methodToInline;
    XanList *scc;

    /* Checks. */
    assert(method != NULL);
    assert(call != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, call));

    /* Check for the right type of instruction. */
    if (IRMETHOD_getInstructionType(call) != IRCALL) {
        return JITFALSE;
    }

    /* Get the method to inline. */
    methodToInline = IRMETHOD_getCalledMethod(method, call);
    assert(methodToInline != NULL);
    PDEBUG("CHIARA_INLINE:   Attempting to inline %s into %s\n", IRMETHOD_getMethodName(methodToInline), IRMETHOD_getMethodName(method));

    /* Check this is not part of a large strongly connected component (i.e. one
       containing more than one method.  However, if both the method to inline
       and the method to inline into are in the same strongly connected component
       then this is ok. */
    scc = xanHashTable_lookup(sccMap, methodToInline);
    if (!scc) {
        return JITFALSE;
    } else if (xanList_length(scc) > 1 && !xanList_find(scc, method)) {
        return JITFALSE;
    }

    /* Check there is something to inline. */
    if (!IRMETHOD_hasMethodInstructions(methodToInline)) {
        return JITFALSE;
    } else if (maxMethodInsts && IRMETHOD_getInstructionsNumber(method) + IRMETHOD_getInstructionsNumber(methodToInline) > maxMethodInsts) {
        return JITFALSE;
    }

    /* Check we don't inline ourselves. */
    if (method == methodToInline) {
        return JITFALSE;
    }

    /* Check we don't inline a recursive method. */
    if (internal_isDefinitelyRecursiveMethod(methodToInline)) {
        return JITFALSE;
    }

    /* Make sure this call instruction can be removed. */
    if (removeCalls) {
        XanList *callList = xanList_new(allocFunction, freeFunction, NULL);
        JITBOOLEAN inlineMethod;
        xanList_append(callList, call);
        (*removeCalls)(method, callList);
        inlineMethod = (xanList_length(callList) > 0);
        xanList_destroyList(callList);
        if (!inlineMethod) {
            return JITFALSE;
        }
    }

    /* Do the inlining. */
    internal_inlineMethod(irOptimizer, method, call, methodToInline, JITFALSE);
    assert(IRMETHOD_getInstructionsNumber(method) > 0);

    /* Finished. */
    return JITTRUE;
}


JITBOOLEAN
METHODS_inlineCalledMethodsNoRecursion(ir_optimizer_t *irOptimizer, ir_method_t *method, XanList *callInstructions, void (*removeCalls)(ir_method_t *method, XanList *newCalls)) {
    static JITINT32 invocations = 0;
    XanList *callsToInline;
    XanHashTable *sccMap;
    JITBOOLEAN inlined;
    JITUINT32 maxMethodInsts;
    char *env;

    /* Checks. */
    assert(irOptimizer != NULL);
    assert(method != NULL);
    assert(callInstructions != NULL);

    /* See if we've done enough inlining. */
    invocations += 1;
    env = getenv("CHIARA_INLINER_MAX_INVOCATIONS");
    if (env != NULL && atoi(env) <= invocations) {
        return JITFALSE;
    }
    PDEBUG("CHIARA_INLINE: Inlining into %s\n", IRMETHOD_getMethodName(method));

    /* Get the maximum size of the method. */
    maxMethodInsts = 0;
    env = getenv("CHIARA_INLINER_MAXIMUM_METHOD_INSTRUCTIONS");
    if (env != NULL) {
        maxMethodInsts = atoi(env);
    }

    /* Get strongly connected components. */
    sccMap = METHODS_getStronglyConnectedComponentsMap(method, JITTRUE);

    /* Set up calls to inline. */
    callsToInline = xanList_cloneList(callInstructions);

    /* Inline up to strongly connected component containing multiple methods. */
    inlined = JITFALSE;
    while (xanList_length(callsToInline) > 0) {
        ir_instruction_t *call = xanList_first(callsToInline)->data;
        xanList_removeElement(callsToInline, call, JITTRUE);
        if (IRMETHOD_doesInstructionBelongToMethod(method, call)) {
            JITBOOLEAN didInline;
            PDEBUG("CHIARA_INLINE: Considering call %d\n", call->ID);
            didInline = internal_inlineMethodNoRecursion(irOptimizer, method, call, sccMap, removeCalls, maxMethodInsts);

            /* Append additional calls to the list of calls to process. */
            if (didInline) {
                XanList *newCalls = IRMETHOD_getInstructionsOfType(method, IRCALL);
                if (removeCalls) {
                    (*removeCalls)(method, newCalls);
                }
                xanList_appendList(callsToInline, newCalls);
                xanList_destroyList(newCalls);
                inlined = JITTRUE;
            }
        }
    }

    /* Optimise the new method. */
    if (inlined) {
        PDEBUG("CHIARA_INLINE: Optimising %s\n", IRMETHOD_getMethodName(method));
        IROPTIMIZER_optimizeMethod(irOptimizer, method);
        PDEBUG("CHIARA_INLINE: Finished optimising\n");
    }

    /* Clean up. */
    xanList_destroyList(callsToInline);
    METHODS_destroyStronglyConnectedComponentsMap(sccMap);

    /* Indicate whether inlining occurred. */
    PDEBUG("CHIARA_INLINE: Finished (%d invocations)\n", invocations);
    return inlined;
}

JITBOOLEAN METHODS_inlineCalledMethods (ir_optimizer_t *irOptimizer, ir_method_t *method, XanList *callInstructions, void (*removeCalls)(ir_method_t *method, XanList *newCalls), JITUINT32 minimumCallersNumber) {
    XanList                 *methodsAlreadyInlined;
    XanListItem             *item;
    JITINT32 		modified;
    XanHashTable		*callerGraph;

    /* Assertions			*/
    assert(method != NULL);
    assert(callInstructions != NULL);

    /* Compute the caller graph		*/
    callerGraph = METHODS_getMethodsCallerGraph(JITFALSE);
    assert(callerGraph != NULL);

    /* Allocate the list of methods	*
     * already inlined		*/
    methodsAlreadyInlined = xanList_new(allocFunction, freeFunction, NULL);

    /* Add the current method	*/
    xanList_append(methodsAlreadyInlined, method);

    /* Inline the calls		*/
    modified = JITFALSE;
    item = xanList_first(callInstructions);
    while (item != NULL) {
        ir_instruction_t        *call;

        /* Fetch the current call		*/
        call = item->data;
        assert(call != NULL);

        /* Check if this call has been deleted	*/
        if (IRMETHOD_doesInstructionBelongToMethod(method, call)) {

            /* Inline recursively the call		*/
            modified |= internal_deepInlineMethodWithoutRecursion(irOptimizer, method, call, methodsAlreadyInlined, removeCalls, callerGraph, minimumCallersNumber, JITTRUE);
        }

        /* Check the end condition		*/
#ifdef MAX_INSTRUCTIONS
        if (IRMETHOD_getInstructionsNumber(method) > MAX_INSTRUCTIONS) {
            break;
        }
#endif

        /* Fetch the next top level call	*/
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(methodsAlreadyInlined);
    METHODS_destroyMethodsCallerGraph(callerGraph);

    return modified;
}

static inline JITINT32 internal_deepInlineMethodWithoutRecursion (ir_optimizer_t *irOptimizer, ir_method_t *method, ir_instruction_t *inst, XanList *methodsAlreadyInlined, void (*removeCalls)(ir_method_t *method, XanList *newCalls), XanHashTable *callerGraph, JITUINT32 minimumCallersNumber, JITBOOLEAN inlineEmptyMethods) {
    ir_method_t             *methodToInline;
    XanList                 *newCalls;
    XanList 		*callerMethods;
    XanListItem             *item;
    static JITUINT32 level = 0;

    /* Assertions			*/
    assert(irOptimizer != NULL);
    assert(method != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));
    assert(methodsAlreadyInlined != NULL);
    PDEBUG("METHOD INLINER: Inline method call till recursion: Start (level %u)\n", level);
    PDEBUG("METHOD INLINER: Inline method call till recursion: 	Start method %s\n", (char *)IRMETHOD_getSignatureInString(method));

    /* Check the instruction	*/
    if (IRMETHOD_getInstructionType(inst) != IRCALL) {
        PDEBUG("METHOD INLINER: Inline method call till recursion: 	The instruction is not a IRCALL\n");
        PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
        return JITFALSE;
    }
    assert(IRMETHOD_getInstructionType(inst) == IRCALL);

    /* Fetch the callee		*/
    methodToInline = IRMETHOD_getCalledMethod(method, inst);
    assert(methodToInline != NULL);
    PDEBUG("METHOD INLINER: Inline method call till recursion:      Method to inline = %s\n", IRMETHOD_getSignatureInString(methodToInline));

    /* Check the end condition	*/
#ifdef MAX_INSTRUCTIONS
    if (IRMETHOD_getInstructionsNumber(method) > MAX_INSTRUCTIONS) {
        return JITTRUE;
    }
#endif

    /* Check if the method to inline*
     * is empty, which means that 	*
     * its IR body is not yet known	*/
    if (!IRMETHOD_hasMethodInstructions(methodToInline)) {
        JITBOOLEAN	inlined;
        PDEBUG("METHOD INLINER: Inline method call till recursion:      The IR callee body is not available yet\n");

        /* Check whether inlining empty methods is allowed.
         */
        inlined	= JITFALSE;
        if (inlineEmptyMethods) {
            ir_item_t	*retVar;
            PDEBUG("METHOD INLINER: Inline method call till recursion:     	Inline the empty method\n");

            /* Fetch the return value.
             */
            retVar	= IRMETHOD_getInstructionResult(inst);
            if (IRDATA_isAVariable(retVar)) {
                ir_item_t	retVarClone;
                ir_item_t	valueToStore;

                /* Set the parameters of the move instruction.
                 */
                memcpy(&retVarClone, retVar, sizeof(ir_item_t));
                memcpy(&valueToStore, &retVarClone, sizeof(ir_item_t));
                valueToStore.type	= valueToStore.internal_type;
                valueToStore.value.v	= 0;

                /* Add the move instruction.
                 */
                IRMETHOD_eraseInstructionFields(method, inst);
                inst->type	= IRMOVE;
                IRMETHOD_cpInstructionResult(inst, &retVarClone);
                IRMETHOD_cpInstructionParameter1(inst, &valueToStore);

            } else {

                /* Delete the call instruction.
                 */
                IRMETHOD_deleteInstruction(method, inst);
            }
            inlined	= JITTRUE;
        }

        PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
        return inlined;
    }

    /* Check if there is a recursion that involves the method given as input.
     */
    if (	(internal_thereIsRecursionWithMethod(methodToInline, methodsAlreadyInlined))		||
            (internal_isCalledDirectly(methodToInline, methodToInline))				||
            (method == methodToInline)								) {
        PDEBUG("METHOD INLINER: Inline method call till recursion:      There is a recursion\n");
        PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
        return JITFALSE;
    }

    /* Delete the calls of methods  *
     * that does not satisfy the	*
     * constraints			*/
    if (removeCalls != NULL) {
        XanList *tmp;
        JITBOOLEAN inlineMethod;
        tmp = xanList_new(allocFunction, freeFunction, NULL);
        xanList_append(tmp, inst);
        (*removeCalls)(method, tmp);
        inlineMethod = (xanList_length(tmp) > 0);
        xanList_destroyList(tmp);
        if (!inlineMethod) {
            PDEBUG("METHOD INLINER: Inline method call till recursion:      The method cannot be inlined\n");
            PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
            return JITFALSE;
        }
    }

    /* Optimize the method		*/
    IROPTIMIZER_optimizeMethod(irOptimizer, methodToInline);

    /* Add the current method in the list of methods inlined.
     */
    xanList_append(methodsAlreadyInlined, methodToInline);

    /* Fetch the methods called by the current called method.
     */
    newCalls = IRMETHOD_getInstructionsOfType(methodToInline, IRCALL);
    assert(newCalls != NULL);

    /* Delete the calls of methods that does not satisfy the constraints.
     */
    if (removeCalls != NULL) {
        (*removeCalls)(methodToInline, newCalls);
    }

    /* Inline the methods called by the current method first.
     */
    item = xanList_first(newCalls);
    while (item != NULL) {
        ir_instruction_t        *newCall;

        /* Fetch the current call.
         */
        newCall = item->data;
        assert(newCall != NULL);

        /* Check if the call still exist.
         */
        if (IRMETHOD_doesInstructionBelongToMethod(methodToInline, newCall)) {

            /* Inline the called methods.
             */
            level += 1;
            internal_deepInlineMethodWithoutRecursion(irOptimizer, methodToInline, newCall, methodsAlreadyInlined, removeCalls, callerGraph, minimumCallersNumber, inlineEmptyMethods);
            level -= 1;
        }

        /* New loops in the call graph could have been created by the deep inlining just finished.
         * Check if this is the case.
         */
        if (internal_isCalledDirectly(methodToInline, methodToInline)) {
            PDEBUG("METHOD INLINER: Inline method call till recursion:      The method calls itself (first check)\n");
            PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
            return JITFALSE;
        }

        /* Fetch the next element of the list.
         */
        item = item->next;
    }
    xanList_destroyList(newCalls);

    /* Check the end condition.
     */
#ifdef MAX_INSTRUCTIONS
    if (IRMETHOD_getInstructionsNumber(method) > MAX_INSTRUCTIONS) {
        xanList_removeElement(methodsAlreadyInlined, methodToInline, JITTRUE);
        return JITTRUE;
    }
#endif

    /* Do not inline methods called less than the threshold.
     */
    callerMethods = xanHashTable_lookup(callerGraph, methodToInline);
    if (	(minimumCallersNumber > 0)					&&
            (callerMethods != NULL)						&&
            (xanList_length(callerMethods) <= minimumCallersNumber)	) {
        xanList_removeElement(methodsAlreadyInlined, methodToInline, JITTRUE);
        PDEBUG("METHOD INLINER: Inline method call till recursion:      The threshold of inlining has been reached\n");
        PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
        return JITFALSE;
    }

    /* New loops in the call graph could have been created by the deep inlining just finished.
     * Check if this is the case.
     */
    if (internal_isCalledDirectly(methodToInline, methodToInline)) {
        PDEBUG("METHOD INLINER: Inline method call till recursion:      The method calls itself (second check)\n");
        PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
        return JITFALSE;
    }

    /* Inline the method.
     */
    PDEBUG("METHOD INLINER: Inline method call till recursion:      Start inlining\n");
    PDEBUG("METHOD INLINER: Inline method call till recursion:		Instructions of method to inline = %d\n", IRMETHOD_getInstructionsNumber(methodToInline));
    PDEBUG("METHOD INLINER: Inline method call till recursion:		Instruction call = %d\n", inst->ID);
    internal_inlineMethod(irOptimizer, method, inst, methodToInline, JITTRUE);
    assert(IRMETHOD_getInstructionsNumber(method) > 0);
    PDEBUG("METHOD INLINER: Inline method call till recursion:		Instructions after inlining = %d\n", IRMETHOD_getInstructionsNumber(method));

    /* Delete the current top level call already inlined.
     */
    xanList_removeElement(methodsAlreadyInlined, methodToInline, JITTRUE);

    PDEBUG("METHOD INLINER: Inline method call till recursion: Exit\n");
    return JITTRUE;
}

static inline JITINT32 internal_thereIsRecursionWithMethod (ir_method_t *methodToInline, XanList *methodsAlreadyConsidered) {

    /* Assertions			*/
    assert(methodToInline != NULL);
    assert(methodsAlreadyConsidered != NULL);

    /* Check if exist a recursion	*
     * that involves the method     *
     * given as input		*/
    return (xanList_find(methodsAlreadyConsidered, methodToInline) != NULL);
}

static inline JITINT32 internal_findDirectRecursion (ir_method_t *method, ir_method_t *startMethod, ir_instruction_t *inst, XanList *methodsAlreadyConsidered) {
    JITUINT32 instructionsNumber;
    JITUINT32 instID;
    ir_method_t             *methodToInline;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(startMethod != NULL);
    assert(IRMETHOD_getInstructionType(inst) == IRCALL);
    assert(IRMETHOD_getInstructionParameter1Type(inst) == IRMETHODID);
    assert(methodsAlreadyConsidered != NULL);

    /* Fetch the callee			*/
    methodToInline = IRMETHOD_getIRMethodFromMethodID(method, IRMETHOD_getInstructionParameter1Value(inst));
    assert(methodToInline != NULL);
    IRMETHOD_translateMethodToIR(methodToInline);

    /* Check if the calling method was      *
     * already considered			*/
    if (xanList_find(methodsAlreadyConsidered, methodToInline) != NULL) {

        /* Found a call recursion. Check if the method	*
         * is involved in the recursion			*/
        if (methodToInline == startMethod) {
            return JITTRUE;
        }
        return JITFALSE;
    }

    /* Add the current method to the list	*/
    xanList_append(methodsAlreadyConsidered, methodToInline);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(methodToInline);

    /* Find the recursion			*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *newCall;

        /* Fetch the instruction	*/
        newCall = IRMETHOD_getInstructionAtPosition(methodToInline, instID);
        assert(newCall != NULL);

        /* Check if it is a call	*/
        if (newCall->type != IRCALL) {
            continue;
        }

        /* Check if there is a recursion*
         * from this point		*/
        if (internal_findDirectRecursion(method, startMethod, newCall, methodsAlreadyConsidered)) {

            /* Found a call recursion		*/
            return JITTRUE;
        }
    }

    /* Delete the current method from the	*
     * list					*/
    xanList_removeElement(methodsAlreadyConsidered, methodToInline, JITTRUE);

    /* Return			*/
    return JITFALSE;
}

static inline JITINT32 internal_findRecursion (ir_method_t *method, ir_instruction_t *inst, XanList *methodsAlreadyConsidered) {
    JITUINT32 instructionsNumber;
    JITUINT32 instID;
    ir_method_t             *methodToInline;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_getInstructionType(inst) == IRCALL);
    assert(IRMETHOD_getInstructionParameter1Type(inst) == IRMETHODID);
    assert(methodsAlreadyConsidered != NULL);

    /* Fetch the callee			*/
    methodToInline = IRMETHOD_getIRMethodFromMethodID(method, IRMETHOD_getInstructionParameter1Value(inst));
    assert(methodToInline != NULL);
    IRMETHOD_translateMethodToIR(methodToInline);

    /* Check if the calling method was      *
     * already considered			*/
    if (xanList_find(methodsAlreadyConsidered, methodToInline) != NULL) {

        /* Found a call recursion		*/
        return JITTRUE;
    }

    /* Add the current method to the list	*/
    xanList_append(methodsAlreadyConsidered, methodToInline);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(methodToInline);

    /* Find the recursion			*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *newCall;

        /* Fetch the instruction	*/
        newCall = IRMETHOD_getInstructionAtPosition(methodToInline, instID);
        assert(newCall != NULL);

        /* Check if it is a call	*/
        if (newCall->type != IRCALL) {
            continue;
        }

        /* Check if there is a recursion*
         * from this point		*/
        if (internal_findRecursion(methodToInline, newCall, methodsAlreadyConsidered)) {

            /* Found a call recursion		*/
            return JITTRUE;
        }
    }

    /* Delete the current method from the	*
     * list					*/
    xanList_removeElement(methodsAlreadyConsidered, methodToInline, JITTRUE);

    /* Return			*/
    return JITFALSE;
}

static inline void internal_inlineMethod (ir_optimizer_t *irOptimizer, ir_method_t *method, ir_instruction_t *inst, ir_method_t *methodToInline, JITBOOLEAN optimizeMethod) {
    ir_instruction_t        *inst2;
    ir_instruction_t        *inst3;
    ir_instruction_t        *endLabel;
    ir_instruction_t        *lastInst;
    ir_item_t               *resultVar;
    JITUINT32	  	count;
    JITUINT32	  	calleeInstructionsNumber;
    JITUINT32	  	parametersNumber;
    JITUINT32	  	variablesOffset;
    JITUINT32	  	labelsOffset;
    JITUINT32	  	callParameterNumber;
    JITUINT32               *variablesMapping;
#ifdef DEBUG
    JITUINT32	  	catchersNumber;
#endif

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(methodToInline != NULL);
    assert(IRMETHOD_getInstructionType(inst) == IRCALL);
    assert(IRMETHOD_hasMethodInstructions(methodToInline));
    PDEBUG("METHOD INLINER: Inline method: Start\n");
    fprintf(stderr, "INLINE: %s -> %s\n", (char *)IRMETHOD_getSignatureInString(method), (char *)IRMETHOD_getSignatureInString(methodToInline));

    /* Print the method to inline	*/
#ifdef DUMP_CFG
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, methodToInline, METHOD_PRINTER);
#endif

    /* Add a new label after the    *
     * call instruction		*/
    endLabel	= IRMETHOD_newLabelAfter(method, inst);

    /* Fetch the result variable of *
     * the call			*/
    resultVar 	= IRMETHOD_getInstructionResult(inst);
    assert(resultVar != NULL);

    /* Fetch the number of call	*
     * parameters			*/
    parametersNumber = IRMETHOD_getInstructionCallParametersNumber(inst);
    assert(IRMETHOD_getMethodParametersNumber(methodToInline) == parametersNumber);

    /* Allocate the variables       *
     * mapping rules of the call	*
     * parameters			*/
    variablesMapping = NULL;
    if (parametersNumber > 0) {
        variablesMapping = allocMemory(sizeof(JITUINT32) * parametersNumber);
        assert(variablesMapping != NULL);
    }

    /* Store into variables the     *
     * constant parameters		*/
    PDEBUG("METHOD INLINER: Inline method: Storing call parameters\n");
    for (callParameterNumber = 0; callParameterNumber < parametersNumber; callParameterNumber++) {
        ir_item_t               *callParameter;
        ir_instruction_t        *newStore;
        IR_ITEM_VALUE		newVarID;

        /* Fetch the call parameter		*/
        callParameter 		= IRMETHOD_getInstructionCallParameter(inst, callParameterNumber);
        assert(callParameter != NULL);

        /* Allocate a new variable		*/
        newVarID		= IRMETHOD_newVariableID(method);

        /* Add a new IRMOVE instruction to store the constant  *
         * given as the current call parameter			*/
        newStore 		= IRMETHOD_newInstructionOfTypeBefore(method, inst, IRMOVE);
        IRMETHOD_cpInstructionParameter1(newStore, callParameter);
        IRMETHOD_cpInstructionResult(newStore, callParameter);
        IRMETHOD_setInstructionResultValue(method, newStore, newVarID);
        IRMETHOD_setInstructionResultType(method, newStore, IROFFSET);

        /* Change the call instruction in order to use the	*
         * variable instead of the constant			*/
        variablesMapping[callParameterNumber] = newVarID;
    }
    IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

    /* Fetch the offset to use for	*
     * variables of the callee      *
     * method			*/
    variablesOffset = IRMETHOD_getNumberOfVariablesNeededByMethod(method) + 1;

    /* Fetch the offset to use for	*
     * labels of the callee method  */
    labelsOffset 	= IRMETHOD_getMaxLabelID(method) + 1;

    /* Check the method			*/
#ifdef DEBUG
    PDEBUG("METHOD INLINER: Inline method: Checking method\n");
    for (count = 0, catchersNumber = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        ir_instruction_t        *tempInst;

        /* Fetch the instruction			*/
        tempInst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(tempInst != NULL);

        if (tempInst->type == IRSTARTCATCHER) {
            catchersNumber++;
        }
    }
    assert(catchersNumber <= 1);
#endif

    /* Inline the method.
     */
    calleeInstructionsNumber = IRMETHOD_getInstructionsNumber(methodToInline);
    PDEBUG("METHOD INLINER: Inline method: Inlining %d instructions\n", calleeInstructionsNumber);
    for (count = 0; count < calleeInstructionsNumber; count++) {

        /* Fetch the instruction of the callee to add to the caller method.
         */
        inst2 = IRMETHOD_getInstructionAtPosition(methodToInline, count);
        assert(inst2 != NULL);

        /* Clone the callee instruction within the caller.
         */
        switch (inst2->type) {
            case IRNOP:
                break;

            case IRRET:

                /* Store the result.
                 */
                if (resultVar->type == IROFFSET) {
                    ir_item_t       *var;

                    /* Fetch the variable to return			*/
                    assert((IRMETHOD_getResultType(methodToInline)) != IRVOID);
                    var = IRMETHOD_getInstructionParameter1(inst2);
                    assert(var != NULL);
                    assert(var->type != NOPARAM);

                    inst3 = IRMETHOD_newInstructionOfTypeBefore(method, endLabel, IRMOVE);
                    IRMETHOD_cpInstructionParameter1(inst3, var);
                    internal_adjustInlinedInstruction(method, inst3, variablesOffset, labelsOffset, variablesMapping, parametersNumber);
                    IRMETHOD_cpInstructionResult(inst3, resultVar);
                }

                /* Branch to the end of the inlined method.
                 */
                inst3	= IRMETHOD_newBranchToLabelBefore(method, endLabel, endLabel);
                break;
            case IRSTARTCATCHER:

                /* For every method we can have only one catcher.	*
                 * Hence we need to merge two catchers and move them at	*
                 * the end of the method.				*/
                lastInst = IRMETHOD_getLastInstruction(method);
                assert(lastInst != NULL);
                if (IRMETHOD_getCatcherInstruction(method) == NULL) {

                    /* Clone and add the instruction to the caller	*
                     * method					*/
                    inst3 = IRMETHOD_cloneInstructionAndInsertAfter(method, inst2, lastInst);
                    lastInst = inst3;
                }

                /* Move the rest of the inlined method after the start	*
                 * catcher instruction					*/
                count++;
                for (; count < calleeInstructionsNumber; count++) {
                    inst2 = IRMETHOD_getInstructionAtPosition(methodToInline, count);
                    assert(inst2 != NULL);
                    inst3 = IRMETHOD_cloneInstructionAndInsertAfter(method, inst2, lastInst);
                    internal_adjustInlinedInstruction(method, inst3, variablesOffset, labelsOffset, variablesMapping, parametersNumber);
                    lastInst = inst3;
                }
                break;
            default:

                /* Clone and add the instruction to the caller method.
                 */
                inst3 = IRMETHOD_cloneInstructionAndInsertBefore(method, inst2, endLabel);

                /* Adjust the variable IDs as well as the labels of the new instruction.
                 */
                internal_adjustInlinedInstruction(method, inst3, variablesOffset, labelsOffset, variablesMapping, parametersNumber);
        }
    }

    /* Check the method.
     */
#ifdef DEBUG
    PDEBUG("METHOD INLINER: Inline method: Checking method (second check)\n");
    for (count = 0, catchersNumber = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        ir_instruction_t        *tempInst;

        /* Fetch the instruction			*/
        tempInst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(tempInst != NULL);

        if (tempInst->type == IRSTARTCATCHER) {
            catchersNumber++;
        }
    }
    assert(catchersNumber <= 1);
#endif

    /* Set the number of variables.
     */
    IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

    /* Free the memory.
     */
    if (parametersNumber > 0) {
        freeMemory(variablesMapping);
    }

    /* Delete the call instruction.
     */
    assert(inst->type == IRCALL);
    PDEBUG("METHOD INLINER: Inline method: Removing call instruction\n");
    IRMETHOD_deleteInstruction(method, inst);

    /* Print the method.
     */
#ifdef DUMP_CFG
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
#endif

    /* Optimize the method.
     */
    if (optimizeMethod) {
        PDEBUG("METHOD INLINER: Inline method: Optimising method (%u instructions)\n", IRMETHOD_getInstructionsNumber(method));
        IROPTIMIZER_optimizeMethod(irOptimizer, method);
    }

    /* Print the method.
     */
#ifdef DUMP_CFG
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
#endif

    PDEBUG("METHOD INLINER: Inline method: Exit\n");
    return ;
}

static inline void internal_adjustInlinedInstruction (ir_method_t *method, ir_instruction_t *inst, JITUINT32 variablesOffset, JITUINT32 labelsOffset, JITUINT32 *variablesMapping, JITUINT32 parametersNumber) {
    JITUINT32 callParameterNumber;

    /* Assertions					*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Adjust the variable IDs of the new           *
     * instruction					*/
    internal_adjust_variable(IRMETHOD_getInstructionParameter1(inst), variablesOffset, variablesMapping, parametersNumber);
    internal_adjust_variable(IRMETHOD_getInstructionParameter2(inst), variablesOffset, variablesMapping, parametersNumber);
    internal_adjust_variable(IRMETHOD_getInstructionParameter3(inst), variablesOffset, variablesMapping, parametersNumber);
    internal_adjust_variable(IRMETHOD_getInstructionResult(inst), variablesOffset, variablesMapping, parametersNumber);
    for (callParameterNumber = 0; callParameterNumber < IRMETHOD_getInstructionCallParametersNumber(inst); callParameterNumber++) {
        internal_adjust_variable(IRMETHOD_getInstructionCallParameter(inst, callParameterNumber), variablesOffset, variablesMapping, parametersNumber);
    }

    /* Adjust the labels				*/
    internal_adjust_label(IRMETHOD_getInstructionParameter1(inst), labelsOffset);
    internal_adjust_label(IRMETHOD_getInstructionParameter2(inst), labelsOffset);
    internal_adjust_label(IRMETHOD_getInstructionParameter3(inst), labelsOffset);
    internal_adjust_label(IRMETHOD_getInstructionResult(inst), labelsOffset);
    for (callParameterNumber = 0; callParameterNumber < IRMETHOD_getInstructionCallParametersNumber(inst); callParameterNumber++) {
        internal_adjust_label(IRMETHOD_getInstructionCallParameter(inst, callParameterNumber), labelsOffset);
    }

    return;
}

static inline void internal_adjust_variable (ir_item_t *item, JITUINT32 variablesOffset, JITUINT32 *variablesMapping, JITUINT32 parametersNumber) {

    /* Assertions			*/
    assert(item != NULL);

    /* Check for constants		*/
    if (item->type != IROFFSET) {
        return;
    }

    /* Check for call parameters	*/
    if ((item->value).v < parametersNumber) {
        assert(variablesMapping != NULL);
        (item->value).v = variablesMapping[item->value.v];
        return;
    }

    /* Normal variable		*/
    (item->value).v += variablesOffset;

    return;
}

static inline void internal_adjust_label (ir_item_t *item, IR_ITEM_VALUE labelsOffset) {

    /* Assertions			*/
    assert(item != NULL);

    /* Check if it is a label	*/
    if (    (labelsOffset == 0)             ||
            (item->type != IRLABELITEM)     ) {

        /* Return				*/
        return;
    }
    assert(item->internal_type == IRLABELITEM);

    /* Adjust the label		*/
    (item->value).v += labelsOffset;

    /* Return			*/
    return;
}

ir_method_t * METHODS_getMainMethod (void) {
    XanList         *l;
    ir_method_t     *mainMethod;
    XanListItem     *item;

    /* Initialize the variables	*/
    mainMethod = NULL;

    /* Fetch the list of all IR	*
     * methods			*/
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);

    /* Search the main method	*/
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t     *method;
        JITINT8         *name;

        /* Fetch the main method	*/
        method = (ir_method_t *) item->data;
        assert(method != NULL);

        /* Fetch the name of the method	*/
        name = IRMETHOD_getMethodName(method);
        assert(name != NULL);

        /* Check the name of the method	*/
        if (    (STRNCMP(name, "main", 5) == 0) ||
                (STRNCMP(name, "Main", 5) == 0) ) {
            mainMethod = method;
            break;
        }

        /* Fetch the next element of the*
         * list				*/
        item = item->next;
    }
    assert(mainMethod != NULL);
    assert(item != NULL);

    /* Free the memory		*/
    xanList_destroyList(l);

    /* Return			*/
    return mainMethod;
}

JITBOOLEAN METHODS_isReachableWithoutGoingThroughMethods (ir_method_t *startMethod, ir_method_t *endMethod, XanList *methodsToNotGoThrough, JITBOOLEAN (*toConsider)(ir_method_t *method, ir_instruction_t *callInst)) {
    XanList *l;
    XanList *trace;
    XanList *escapedMethods;
    JITBOOLEAN found;

    /* Assertions			*/
    assert(startMethod != NULL);
    assert(endMethod != NULL);
    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Start\n");

    /* Print the methods to avoid	*/
#ifdef PRINTDEBUG
    if (methodsToNotGoThrough != NULL) {
        XanListItem *item;
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	Methods to avoid:\n");
        item = xanList_first(methodsToNotGoThrough);
        while (item != NULL) {
            ir_method_t *m;
            m = item->data;
            assert(m != NULL);
            PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods:		%s\n", IRMETHOD_getSignatureInString(m));
            item = item->next;
        }
    }
#endif

    /* Allocate the list of methods	*
     * already considered as start	*
     * points			*/
    l = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the escaped methods	*/
    escapedMethods =  IRPROGRAM_getEscapedMethods();

    /* Allocate the trace		*/
    trace = xanList_new(allocFunction, freeFunction, NULL);

    /* Check if the end method is	*
     * reachable			*/
    found = internal_METHODS_isReachableWithoutGoingThroughAMethod(startMethod, endMethod, methodsToNotGoThrough, l, trace, escapedMethods, toConsider);

    /* Print the trace		*/
#ifdef PRINTDEBUG
    if (found) {
        XanListItem *item;
        PDEBUG("Found the trace\n");
        PDEBUG("	From %s\n", IRMETHOD_getSignatureInString(startMethod));
        PDEBUG("	To %s\n", IRMETHOD_getSignatureInString(endMethod));
        PDEBUG("	Avoiding the following methods:\n");
        if (methodsToNotGoThrough != NULL) {
            item = xanList_first(methodsToNotGoThrough);
            while (item != NULL) {
                ir_method_t *m;
                m = item->data;
                assert(m != NULL);
                PDEBUG("		%s\n", IRMETHOD_getSignatureInString(m));
                item = item->next;
            }
        }
        item = xanList_last(trace);
        while (item != NULL) {
            ir_method_t *m;
            m = item->data;
            PDEBUG("	%s\n", IRMETHOD_getSignatureInString(m));
            item = item->prev;
        }
    }
#endif

    /* Free the memory		*/
    xanList_destroyList(l);
    xanList_destroyList(trace);
    xanList_destroyList(escapedMethods);

    /* Return			*/
    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
    return found;
}

static inline JITBOOLEAN internal_METHODS_isReachableWithoutGoingThroughAMethod (ir_method_t *startMethod, ir_method_t *endMethod, XanList *methodsToNotGoThrough, XanList *alreadyConsidered, XanList *trace, XanList *escapedMethods, JITBOOLEAN (*toConsider)(ir_method_t *method, ir_instruction_t *callInst)) {
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions			*/
    assert(alreadyConsidered != NULL);
    assert(startMethod != NULL);
    assert(trace != NULL);
    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Start\n");
    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	Start method %s\n", IRMETHOD_getSignatureInString(startMethod));

    /* Check to see if the start	*
     * method was already considered*/
    if (xanList_find(alreadyConsidered, startMethod) != NULL) {

        /* Return			*/
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	The start method was already considered\n");
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
        return JITFALSE;
    }
    xanList_insert(alreadyConsidered, startMethod);
    xanList_insert(trace, startMethod);

    /* Check if we reached the 	*
     * end method			*/
    if (startMethod == endMethod) {
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	The end method is reachable\n");
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
        return JITTRUE;
    }

    /* Check if we reached the 	*
     * method to avoid		*/
    if (	(methodsToNotGoThrough != NULL)					&&
            (xanList_find(methodsToNotGoThrough, startMethod) != NULL)	) {
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	The start method has to be avoided\n");
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
        xanList_removeElement(trace, startMethod, JITTRUE);
        return JITFALSE;
    }

    /* Fetch the number of          *
     * instructions of the method	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(startMethod);

    /* Check if the end method is	*
     * reachable from the start	*
     * one without going through 	*
     * methodToNotGoThrough		*/
    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	Check the reachable methods from the start one\n");
    for (count = 0; count < instructionsNumber; count++) {
        XanList                 *tmpList;
        XanListItem             *item;
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(startMethod, count);
        assert(inst != NULL);
        if (!IRMETHOD_isACallInstruction(inst)) {
            continue;
        }
        if (	(toConsider != NULL)			&&
                (!(*toConsider)(startMethod, inst))	) {
            continue;
        }
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	Found the following call\n");
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods:		Instruction %u\n", inst->ID);
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods:		From method %s\n", IRMETHOD_getSignatureInString(startMethod));
        tmpList = IRPROGRAM_getCallableMethods(inst, escapedMethods, NULL);
        assert(tmpList != NULL);
        PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	Check the callable methods\n");
        item = xanList_first(tmpList);
        while (item != NULL) {
            ir_method_t     *m;
            IR_ITEM_VALUE 	methodID;

            /* Fetch the callable method	*/
            methodID = (IR_ITEM_VALUE)(JITNUINT)item->data;
            m = IRMETHOD_getIRMethodFromMethodID(startMethod, methodID);
            assert(m != NULL);
            PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 		Check %s\n", IRMETHOD_getSignatureInString(m));

            /* Check if we reached the 	*
             * end method			*/
            if (m == endMethod) {

                /* Free the memory		*/
                xanList_destroyList(tmpList);

                /* Return			*/
                PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	Found the end method\n");
                PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
                return JITTRUE;
            }

            /* Check if m is the method to 	*
             * avoid			*/
            if (	(methodsToNotGoThrough == NULL)				||
                    (xanList_find(methodsToNotGoThrough, m) == NULL) 	) {

                /* Check if the end method is 	*
                 * reachable from m 		*/
                if (internal_METHODS_isReachableWithoutGoingThroughAMethod(m, endMethod, methodsToNotGoThrough, alreadyConsidered, trace, escapedMethods, toConsider)) {

                    /* Free the memory		*/
                    xanList_destroyList(tmpList);

                    /* Return			*/
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 	The end method is reachable within the following trace\n");
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 		Start method = %s\n", IRMETHOD_getSignatureInString(startMethod));
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 		One step called method = %s\n", IRMETHOD_getSignatureInString(m));
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
                    return JITTRUE;

                } else {
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 			The end method is not reachable from the following trace\n");
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 				Start method = %s\n", IRMETHOD_getSignatureInString(startMethod));
                    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 				One step called method = %s\n", IRMETHOD_getSignatureInString(m));
                }
            } else {
                PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: 			The callable method has to be avoided\n");
            }

            /* Fetch the next element of the*
             * list				*/
            item = item->next;
        }

        /* Free the memory		*/
        xanList_destroyList(tmpList);
    }
    xanList_removeElement(trace, startMethod, JITTRUE);

    /* Return			*/
    PDEBUG("CHIARA METHODS: isReachableWithoutGoingThroughMethods: Exit\n");
    return JITFALSE;
}

void METHODS_destroyMethodsCallerGraph (XanHashTable *callerGraph) {
    XanHashTableItem *item;

    /* Assertions				*/
    assert(callerGraph != NULL);

    /* Free the content of the table	*/
    item = xanHashTable_first(callerGraph);
    while (item != NULL) {
        XanList *l;
        l = item->element;
        xanList_destroyList(l);
        item = xanHashTable_next(callerGraph, item);
    }

    /* Free the hash table			*/
    xanHashTable_destroyTable(callerGraph);

    /* Return				*/
    return ;
}

XanHashTable * METHODS_getMethodsCallerGraph (JITBOOLEAN directCallsOnly) {
    ir_method_t *mainMethod;
    XanHashTable *callerGraphMethods;
    XanList *escapedMethods;
    XanList *reachableMethods;
    XanListItem *item;

    /* Fetch the entry point.
     */
    mainMethod = IRPROGRAM_getEntryPointMethod();
    assert(mainMethod != NULL);

    /* Fetch the reachable methods.
     */
    reachableMethods = IRPROGRAM_getReachableMethods(mainMethod);
    assert(reachableMethods != NULL);

    /* Fetch the escaped methods.
     */
    escapedMethods = IRPROGRAM_getEscapedMethods();
    assert(escapedMethods != NULL);

    /* Initialize the caller methods hash table.
     */
    callerGraphMethods = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(callerGraphMethods != NULL);
    item = xanList_first(reachableMethods);
    while (item != NULL) {
        ir_method_t *m;
        XanList *callerMethods;
        m = item->data;
        callerMethods = xanList_new(allocFunction, freeFunction, NULL);
        xanHashTable_insert(callerGraphMethods, m, callerMethods);
        item = item->next;
    }

    /* Compute the list of single called methods.
     */
    item = xanList_first(reachableMethods);
    while (item != NULL) {
        ir_method_t *m;
        JITUINT32 count;
        JITUINT32 instructionsNumber;

        /* Fetch the method			*/
        m = item->data;
        assert(m != NULL);

        /* Analyze the callable methods		*/
        instructionsNumber = IRMETHOD_getInstructionsNumber(m);
        for (count = 0; count < instructionsNumber; count++) {
            ir_instruction_t *inst;
            XanList *calledMethods;
            XanListItem *item2;

            /* Fetch the instruction		*/
            inst = IRMETHOD_getInstructionAtPosition(m, count);
            assert(inst != NULL);
            if (	(directCallsOnly)		&&
                    (inst->type != IRCALL)		) {
                continue;
            }

            /* Fetch the callable methods		*/
            calledMethods = IRPROGRAM_getCallableMethods(inst, escapedMethods, reachableMethods);
            assert(calledMethods != NULL);

            /* Update the caller graph		*/
            item2 = xanList_first(calledMethods);
            while (item2 != NULL) {
                ir_method_t *calledMethod;
                IR_ITEM_VALUE calledMethodID;
                XanList *callerMethods;
                calledMethodID = (IR_ITEM_VALUE) (JITNUINT)item2->data;
                assert(calledMethodID != 0);
                calledMethod = IRMETHOD_getIRMethodFromMethodID(m, calledMethodID);
                callerMethods = xanHashTable_lookup(callerGraphMethods, calledMethod);
                if (callerMethods == NULL) {
                    callerMethods = xanList_new(allocFunction, freeFunction, NULL);
                    xanHashTable_insert(callerGraphMethods, calledMethod, callerMethods);
                }
                assert(callerMethods != NULL);
                xanList_append(callerMethods, m);
                item2 = item2->next;
            }

            /* Free the memory			*/
            xanList_destroyList(calledMethods);
        }

        /* Fetch the next element of the list.
         */
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(reachableMethods);
    xanList_destroyList(escapedMethods);

    return callerGraphMethods;
}

void METHODS_translateProgramToMachineCode (void) {
    XanList         *l;
    XanListItem     *item;

    /* Fetch the list of all IR	*
     * methods			*/
    l = IRPROGRAM_getIRMethods();
    assert(l != NULL);

    /* Search the main method	*/
    item = xanList_first(l);
    while (item != NULL) {
        ir_method_t     *method;

        /* Fetch the method		*/
        method = (ir_method_t *) item->data;
        assert(method != NULL);

        /* Translate the method to 	*
         * machine code			*/
        if (IRMETHOD_canHaveIRBody(method)) {
            IRMETHOD_translateMethodToMachineCode(method);
        }

        /* Fetch the next element of the*
         * list				*/
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(l);

    /* Return			*/
    return;
}

void METHODS_addEmptyBasicBlocksJustBeforeInstructionsWithMultiplePredecessors (ir_method_t *method) {
    JITUINT32           count;
    JITUINT32           instsNumber;
    ir_instruction_t    **insts;
    XanList             *todos;
    XanList             *toAddNops;
    XanListItem         *item;

    /* Assertions.
     */
    assert(method != NULL);

    /* Fetch the number of instructions.
     */
    instsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Fetch the instructions.
     */
    insts       = IRMETHOD_getInstructionsWithPositions(method);

    /* Compute the list of instructions to consider.
     */
    todos = xanList_new(allocFunction, freeFunction, NULL);
    toAddNops = xanList_new(allocFunction, freeFunction, NULL);
    for (count = 0; count < instsNumber; count++) {
        ir_instruction_t        *inst;
        ir_instruction_t        *instNext;
        JITBOOLEAN              handled;

        /* Fetch the instruction.
         */
        inst = insts[count];
        assert(inst != NULL);

        /* Check the number of predecessors and its type.
         * Currently, we need to handle just labels.
         */
        handled = JITFALSE;
        switch (inst->type){
            case IRLABEL:
            case IRSTARTCATCHER:
            case IRSTARTFINALLY:
                if (IRMETHOD_getPredecessorsNumber(method, inst) > 1){
                    ir_instruction_t        *instNext;

                    /* Fetch the next instruction		*/
                    instNext = insts[count+1];
                    assert(instNext != NULL);

                    /* Check if the current instruction is not the beginning of an empty basic block.
                     * In this case, we don't need to add an additional empty basic block.
                     */
                    if (    (inst->type != IRLABEL)         ||
                            (instNext->type != IRBRANCH)    ) {

                        /* Consider the instruction.
                         */
                        xanList_append(todos, inst);
                    }
                    handled = JITTRUE;
                }
                break ;
        }
        if (handled){
            continue ;
        }

        /* Fetch the next instruction.
         */
        instNext = insts[count+1];
        assert(instNext != NULL);

        /* Consider the edge between two branches as critical.
         */
        if (    (IRMETHOD_mayHaveNotFallThroughInstructionAsSuccessor(method, inst))     &&
                (IRMETHOD_mayHaveNotFallThroughInstructionAsSuccessor(method, instNext)) ) {
            xanList_append(toAddNops, instNext);
        }
    }

    /* Insert the empty basic blocks*/
    item = xanList_first(todos);
    while (item != NULL) {
        ir_instruction_t        *inst;
        XanListItem             *item2;
        XanList                 *l;
        ir_item_t               *labelToJump;

        /* Fetch the instruction		*/
        inst = item->data;
        assert(inst != NULL);
        assert(IRMETHOD_getPredecessorsNumber(method, inst) > 1);

        /* We handle only labels		*/
        if (inst->type == IRLABEL) {

            /* Fetch the list of predecessors	*/
            l = IRMETHOD_getInstructionPredecessors(method, inst);
            assert(l != NULL);
            assert(IRMETHOD_getPredecessorsNumber(method, inst) == xanList_length(l));

            /* Fetch the label			*/
            labelToJump = IRMETHOD_getInstructionParameter1(inst);
            assert(labelToJump != NULL);
            assert(labelToJump->type == IRLABELITEM);

            /* Insert the empty basic block		*/
            item2 = xanList_first(l);
            while (item2 != NULL) {
                ir_instruction_t        *pred;
                JITBOOLEAN fixed;
                ir_item_t               *label;

                /* Fetch the predecessor		*/
                pred = item2->data;
                assert(pred != NULL);

                /* Check if the predecessor is a jump   *
                 * to the label				*/
                fixed = JITFALSE;
                label = NULL;
                switch (pred->type) {
                    case IRBRANCH:
                        label = IRMETHOD_getInstructionParameter1(pred);
                        break;
                    case IRBRANCHIF:
                    case IRBRANCHIFNOT:
                        label = IRMETHOD_getInstructionParameter2(pred);
                        break;
                }
                if (label != NULL) {
                    assert(label->type == IRLABELITEM);
                    if (labelToJump->value.v == label->value.v) {

                        /* The jumps target the label	*
                         * Adds an empty block		*/
                        internal_add_empty_basic_block(method, pred, inst);
                        fixed = JITTRUE;
                    }
                }

                /* Check if the predecessor is the      *
                 * instruction stored just before the   *
                 * label				*/
                if (    (!fixed)                        &&
                        (pred->ID == (inst->ID - 1))    ) {
                    ir_instruction_t        *newBranch;
                    assert(pred->type != IRBRANCH);
                    newBranch = IRMETHOD_newBranchToLabel(method, inst);
                    assert(newBranch != NULL);
                    IRMETHOD_moveInstructionAfter(method, newBranch, pred);
                    internal_add_empty_basic_block(method, newBranch, inst);
                    fixed = JITTRUE;
                }
//				assert(fixed);

                /* Fetch the next element from the list	*/
                item2 = item2->next;
            }

            /* Free the memory			*/
            xanList_destroyList(l);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Insert the nops		*/
    item = xanList_first(toAddNops);
    while (item != NULL) {
        ir_instruction_t        *inst;
        ir_instruction_t        *nop;

        /* Fetch the instruction		*/
        inst = item->data;
        assert(inst != NULL);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

        /* Add the nop				*/
        nop = IRMETHOD_newInstructionBefore(method, inst);
        IRMETHOD_setInstructionType(method, nop, IRNOP);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(todos);
    xanList_destroyList(toAddNops);
    freeFunction(insts);

    return;
}

static inline void internal_add_empty_basic_block (ir_method_t *method, ir_instruction_t *basic_block_predecessor, ir_instruction_t *basic_block_successor) {
    ir_instruction_t        *newLabelInst;

    /* Assertions			*/
    assert(method != NULL);
    assert(basic_block_successor != NULL);
    assert(basic_block_predecessor != NULL);

    /* The jumps target the label	*
     * Adds an empty block		*/
    newLabelInst = IRMETHOD_newLabel(method);
    assert(newLabelInst != NULL);
    IRMETHOD_newBranchToLabel(method, basic_block_successor);
    IRMETHOD_substituteLabels(method, basic_block_predecessor, (newLabelInst->param_1).value.v);

    /* Return			*/
    return;
}

static inline JITBOOLEAN internal_considerOnlyCallInstructions (ir_method_t *method, ir_instruction_t *callInst) {
    return callInst->type == IRCALL;
}

static inline JITBOOLEAN internal_isCalledDirectly (ir_method_t *caller, ir_method_t *callee) {
    JITBOOLEAN 	found;
    XanList		*l;
    XanListItem	*item;

    /* Initialize the variables.
     */
    found	= JITFALSE;

    /* Fetch the list of call instructions.
     */
    l	= IRMETHOD_getInstructionsOfType(caller, IRCALL);

    /* Search the callee.
     */
    item	= xanList_first(l);
    while (item != NULL) {
        ir_instruction_t	*i;
        ir_method_t		*tempMethod;
        i		= item->data;
        assert(i != NULL);
        tempMethod	= IRMETHOD_getCalledMethod(caller, i);
        if (tempMethod == callee) {
            found		= JITTRUE;
            break;
        }
        item		= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(l);

    return found;
}


/**
 * Recursive function to compute strongly connected components.
 */
static void
internal_getSccs(ir_method_t *method, XanHashTable *tarjanInfo, XanStack *nodeStack, XanList *allSccs, JITBOOLEAN includeLibraryCalls, int *index) {
    tarjan_scc_t *tarjan;
    JITUINT32 i, numInsts;

    /* Assign an index. */
    assert(xanHashTable_lookup(tarjanInfo, method) == NULL);
    tarjan = allocFunction(sizeof(tarjan_scc_t));
    tarjan->index = *index;
    tarjan->lowlink = *index;
    *index = *index + 1;
    xanHashTable_insert(tarjanInfo, method, tarjan);

    /* Save onto the stack and process callees. */
    xanStack_push(nodeStack, method);
    numInsts = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        if (inst->type == IRCALL || inst->type == IRVCALL || inst->type == IRICALL) {
            XanList *calleeList = IRMETHOD_getCallableMethods(inst);
            XanListItem *calleeItem = xanList_first(calleeList);
            while (calleeItem) {
                IR_ITEM_VALUE calleeMethodId = (IR_ITEM_VALUE)(JITNUINT)calleeItem->data;
                ir_method_t *callee = IRMETHOD_getIRMethodFromMethodID(method, calleeMethodId);

                /* Possibly ignore library calls. */
                if (includeLibraryCalls || !IRPROGRAM_doesMethodBelongToALibrary(callee)) {
                    tarjan_scc_t *calleeTarjan = xanHashTable_lookup(tarjanInfo, callee);

                    /* Check whether this method has been seen. */
                    if (calleeTarjan == NULL) {
                        internal_getSccs(callee, tarjanInfo, nodeStack, allSccs, includeLibraryCalls, index);
                        calleeTarjan = xanHashTable_lookup(tarjanInfo, callee);
                        assert(calleeTarjan != NULL);
                        tarjan->lowlink = MIN(tarjan->lowlink, calleeTarjan->lowlink);
                    } else if (xanStack_contains(nodeStack, callee)) {
                        tarjan->lowlink = MIN(tarjan->lowlink, calleeTarjan->index);
                    }
                }

                /* Next callee. */
                calleeItem = xanList_next(calleeItem);
            }
        }
    }

    /* Check for a stronly connected component. */
    if (tarjan->lowlink == tarjan->index) {
        ir_method_t *callee = NULL;
        XanList *scc = xanList_new(allocFunction, freeFunction, NULL);

        /* Add the SCC to a new list. */
        while (callee != method) {
            callee = xanStack_pop(nodeStack);
            xanList_append(scc, callee);
        }

        /* Remember this SCC. */
        xanList_append(allSccs, scc);
    }
}


/**
 * Find strongly connected components within the call graph starting at 'method'
 * using the algorithm from
 *
 * Depth-first search and linear graph algorithms.  Robert E. Tarjan.
 * SIAM Journal on Computing 1(2), pages 146-160, 1972.
 */
XanList *
METHODS_getStronglyConnectedComponentsList(ir_method_t *method, JITBOOLEAN includeLibraryCalls) {
    int index;
    XanList *allSccs;
    XanStack *nodeStack;
    XanHashTable *tarjanInfo;
    XanHashTableItem *infoItem;

    /* Initialisation. */
    allSccs = xanList_new(allocFunction, freeFunction, NULL);
    nodeStack = xanStack_new(allocFunction, freeFunction, NULL);
    tarjanInfo = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(allSccs);
    assert(nodeStack);
    assert(tarjanInfo);

    /* Perform a depth-first traversal of the graph. */
    index = 0;
    internal_getSccs(method, tarjanInfo, nodeStack, allSccs, includeLibraryCalls, &index);

    /* Free all created tarjan structures. */
    infoItem = xanHashTable_first(tarjanInfo);
    while (infoItem) {
        tarjan_scc_t *tarjan = infoItem->element;
        assert(tarjan);
        freeFunction(tarjan);
        infoItem = xanHashTable_next(tarjanInfo, infoItem);
    }

    /* Clean up. */
    xanStack_destroyStack(nodeStack);
    xanHashTable_destroyTable(tarjanInfo);

    return allSccs;
}


/**
 * Get a table mapping methods to their strongly-connected components.
 **/
XanHashTable *
METHODS_getStronglyConnectedComponentsMap(ir_method_t *method, JITBOOLEAN includeLibraryCalls) {
    XanList *sccList;
    XanHashTable *sccMap;
    XanListItem *sccItem;

    /* Get the strongly connected components as a list. */
    sccList = METHODS_getStronglyConnectedComponentsList(method, includeLibraryCalls);

    /* Create the map. */
    sccMap = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(sccMap);

    /* Populate the map. */
    sccItem = xanList_first(sccList);
    while (sccItem) {
        XanList *scc = sccItem->data;
        XanListItem *methodItem = xanList_first(scc);
        while (methodItem) {
            ir_method_t *sccMethod = methodItem->data;
            xanHashTable_insert(sccMap, sccMethod, scc);
            methodItem = xanList_next(methodItem);
        }
        sccItem = xanList_next(sccItem);
    }

    /* Clean up. */
    xanList_destroyList(sccList);

    /* Return the created map. */
    return sccMap;
}


/**
 * Destroy a list of strongly-connected components.
 **/
void
METHODS_destroyStronglyConnectedComponentsList(XanList *sccList) {
    XanListItem *listItem;

    assert(sccList);

    listItem = xanList_first(sccList);
    while (listItem) {
        xanList_destroyList(listItem->data);
        listItem = xanList_next(listItem);
    }
    xanList_destroyList(sccList);
}


/**
 * Destroy a table mapping methods to their strongly-connected components.
 **/
void
METHODS_destroyStronglyConnectedComponentsMap(XanHashTable *sccMap) {
    XanList *sccsToDelete;
    XanListItem *listItem;
    XanHashTableItem *mapItem;

    assert(sccMap);

    /* Each list can occur multiple times, so keep track of those seen here. */
    sccsToDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Populate the list of sccs to delete. */
    mapItem = xanHashTable_first(sccMap);
    while (mapItem) {
        XanList *scc = mapItem->element;
        if (!xanList_find(sccsToDelete, scc)) {
            xanList_append(sccsToDelete, scc);
        }
        mapItem = xanHashTable_next(sccMap, mapItem);
    }

    /* Delete the sccs. */
    listItem = xanList_first(sccsToDelete);
    while (listItem) {
        XanList *scc = listItem->data;
        xanList_destroyList(scc);
        listItem = xanList_next(listItem);
    }

    /* Free remaining memory. */
    xanList_destroyList(sccsToDelete);
}


/**
 * Get a list of methods called by 'method'.
 **/
XanList * METHODS_getCallableIRMethodsFromMethod(ir_method_t *method, XanList *escapedMethods) {
    JITUINT32 numInsts, i;
    XanList *callees;

    /* Initialisation. */
    numInsts = IRMETHOD_getInstructionsNumber(method);
    callees = xanList_new(allocFunction, freeFunction, NULL);

    /* Find call instructions. */
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *call = IRMETHOD_getInstructionAtPosition(method, i);
        if (IRMETHOD_isACallInstruction(call)) {
            XanList *callableMethods = METHODS_getCallableIRMethods(call, NULL);
            XanListItem *calleeItem = xanList_first(callableMethods);
            while (calleeItem) {
                ir_method_t *callee = calleeItem->data;

                /* Avoid duplicates in the callee list. */
                if (!xanList_find(callees, callee)) {
                    xanList_append(callees, callee);
                }
                calleeItem = calleeItem->next;
            }
            xanList_destroyList(callableMethods);
        }
    }

    /* Return the list of callable methods. */
    return callees;
}


/**
 * Build a mapping between methods and lists of methods they can call,
 * starting at 'startMethod'.
 **/
XanHashTable *
METHODS_getCallableIRMethodsMap(ir_method_t *startMethod) {
    XanHashTable *callTable;
    XanList *worklist;
    XanListItem *methodItem;

    worklist = xanList_new(allocFunction, freeFunction, NULL);
    callTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    xanList_append(worklist, startMethod);
    methodItem = xanList_first(worklist);
    while (methodItem) {
        XanList *callees;
        ir_method_t *method = methodItem->data;

        callees = METHODS_getCallableIRMethodsFromMethod(method, NULL);
        xanHashTable_insert(callTable, method, callees);

        XanListItem *calleeItem = xanList_first(callees);
        while (calleeItem) {
            ir_method_t *callee = calleeItem->data;
            if (!xanList_find(worklist, callee)) {
                xanList_append(worklist, callee);
            }
            calleeItem = calleeItem->next;
        }

        methodItem = xanList_next(methodItem);
    }

    xanList_destroyList(worklist);
    return callTable;
}


/**
 * Build a mapping between methods and lists of methods can call them,
 * starting at 'startMethod'.
 **/
XanHashTable *
METHODS_getCallableByIRMethodsMap(ir_method_t *startMethod, XanHashTable **callMap) {
    XanHashTable *callTable;
    XanHashTable *calledByTable;
    XanHashTableItem *methodItem;

    /* A mapping between methods and those they can call. */
    callTable = METHODS_getCallableIRMethodsMap(startMethod);

    /* The map to populate. */
    calledByTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Invert the call map. */
    methodItem = xanHashTable_first(callTable);
    while (methodItem) {
        ir_method_t *method = methodItem->elementID;
        XanList *callees = methodItem->element;
        XanListItem *calleeItem = xanList_first(callees);
        while (calleeItem) {
            ir_method_t *callee = calleeItem->data;
            XanList *callers;
            if (!(callers = xanHashTable_lookup(calledByTable, callee))) {
                callers = xanList_new(allocFunction, freeFunction, NULL);
                xanHashTable_insert(calledByTable, callee, callers);
            }
            xanList_append(callers, method);
            calleeItem = calleeItem->next;
        }
        methodItem = xanHashTable_next(callTable, methodItem);
    }

    /* Clean up or store. */
    if (callMap) {
        *callMap = callTable;
    } else {
        METHODS_destroyCallableIRMethodsMap(callTable);
    }

    /* Return the mapping between methods and those that can call them. */
    return calledByTable;
}


/**
 * Free a map between methods and lists methods.
 **/
void
METHODS_destroyCallableIRMethodsMap(XanHashTable *callMap) {
    XanHashTableItem *methodItem = xanHashTable_first(callMap);
    while (methodItem) {
        xanList_destroyList(methodItem->element);
        methodItem = xanHashTable_next(callMap, methodItem);
    }
    xanHashTable_destroyTable(callMap);
}
