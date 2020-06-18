/*
 * Copyright (C) 2010 Campanoni Simone
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
#include <compiler_memory_manager.h>
#include <chiara.h>

// My headers
#include <optimizer_functionpointerselimination.h>
#include <config.h>
// End

#define INFORMATIONS    "This is a functionpointerselimination plugin"
#define AUTHOR          "Simone Campanoni and Timothy M. Jones"

static inline JITUINT64 functionpointerselimination_get_ID_job (void);
static inline char * functionpointerselimination_get_version (void);
static inline void functionpointerselimination_do_job (ir_method_t * method);
static inline char * functionpointerselimination_get_informations (void);
static inline char * functionpointerselimination_get_author (void);
static inline JITUINT64 functionpointerselimination_get_dependences (void);
static inline void functionpointerselimination_shutdown (JITFLOAT32 totalTime);
static inline void functionpointerselimination_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 functionpointerselimination_get_invalidations (void);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;


/**
 * Check whether there is an indirect call within the given method.
 */
static JITBOOLEAN hasIndirectCall (ir_method_t *method) {
    JITUINT32 i;

    /* Simply check all instructions for indirect calls. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        if (inst->type == IRICALL) {
            return JITTRUE;
        }
    }

    /* Checked all instructions but no indirect call. */
    return JITFALSE;
}


/**
 * Add an item to two lists if not already in the second list.
 */
static void
addUnseenItemToList (void *item, XanList *list, XanList *seenList) {
    if (!xanList_find(seenList, item)) {
        xanList_append(list, item);
        xanList_append(seenList, item);
    }
}


/**
 * Add an item to a list if not already there.
 */
static void
addUniqueItemToList (void *item, XanList *list) {
    if (!xanList_find(list, item)) {
        xanList_append(list, item);
    }
}


/**
 * Get the intersection between two lists.
 */
static XanList *
getIntersectionList (XanList *first, XanList *second) {
    XanList *iList = xanList_new(allocFunction, freeFunction, NULL);
    XanListItem *currItem = xanList_first(first);

    while (currItem) {
        if (xanList_find(second, currItem->data)) {
            xanList_append(iList, currItem->data);
        }
        currItem = currItem->next;
    }
    return iList;
}


/**
 * Convert a list of method IDs to a list of methods.
 */
static XanList *
convertMethodIdListToMethodList (ir_method_t *method, XanList *idList) {
    XanList *methodList = xanList_new(allocFunction, freeFunction, NULL);

    while (xanList_length(idList) > 0) {
        XanListItem *currItem = xanList_first(idList);
        IR_ITEM_VALUE methodId = (IR_ITEM_VALUE) (JITNUINT) currItem->data;
        ir_method_t *currMethod =
            IRMETHOD_getIRMethodFromMethodID(method, methodId);
        xanList_append(methodList, currMethod);
        xanList_deleteItem(idList, currItem);
    }
    xanList_destroyList(idList);
    return methodList;
}


/**
 * Add all non-seen methods that are called by the given method to the list
 * of methods to process.
 */
static void
addCalledMethodsToWorklist (ir_method_t *method, XanList *methodList,
                            XanList *seenMethods) {
    JITUINT32 i;

    /* Simply check all instructions for calls. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);

        /* Direct calls are easy. */
        if (inst->type == IRCALL) {
            ir_method_t *calledMethod =
                IRMETHOD_getCalledMethod(method, inst);
            if (IRMETHOD_hasIRMethodBody(calledMethod)) {
                addUnseenItemToList(calledMethod, methodList, seenMethods);
            }
        }
        /* Indirect calls take a little more work. */
        else if (inst->type == IRICALL || inst->type == IRVCALL) {
            XanList *callableMethodIds = IRMETHOD_getCallableMethods(inst);
            assert(callableMethodIds != NULL);
            XanList *callableMethods =
                convertMethodIdListToMethodList(method, callableMethodIds);
            while (xanList_length(callableMethods) > 0) {
                XanListItem *currItem = xanList_first(callableMethods);
                ir_method_t *calledMethod = (ir_method_t *) currItem->data;
                xanList_deleteItem(callableMethods, currItem);
                if (IRMETHOD_hasIRMethodBody(calledMethod)) {
                    addUnseenItemToList(calledMethod, methodList, seenMethods);
                }
            }
            xanList_destroyList(callableMethods);
        }
    }
}


/**
 * Check a parameter to see if it is a function pointer and, if so, add the
 * pointed-to method to the given list.
 */
static void checkAndAddParamFuncPtrToList (ir_item_t *param, XanList *ifuncList) {
    void	*fp;

    /* Check whether the parameter has a function pointer stored in it.
     */
    fp	= NULL;
    if (	IRDATA_isASymbol(param)			&&
            IRDATA_isAFunctionPointer(param)	) {
        fp	= IRSYMBOL_unresolveSymbolFromIRItem(param);

    } else if (	(param->type == IRMETHODENTRYPOINT) 	||
                (param->type == IRFPOINTER)		) {
        fp	= (void *) (JITNUINT) (param->value).v;
    }
    if (fp == NULL) {
        return ;
    }

    ir_method_t *ifunc = IRMETHOD_getIRMethodFromEntryPoint(fp);
    assert(ifunc != NULL);

    addUniqueItemToList(ifunc, ifuncList);

    return ;
}

/**
 * Add functions whose addresses are used (i.e. function pointers) to the
 * given list.  To be sure we get them all, we'll scan all source parameters
 * of most instruction types.
 */
static void addFuncPtrsToList (ir_method_t *method, XanList *ifuncList) {
    JITUINT32 i, c;

    /* Simply check all instructions. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);

        /* Parameters to check depends on the instruction. */
        switch (inst->type) {

                /* Two source parameters. */
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IROR:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRLT:
            case IRGT:
            case IREQ:
                checkAndAddParamFuncPtrToList(IRMETHOD_getInstructionParameter1(inst), ifuncList);
                checkAndAddParamFuncPtrToList(IRMETHOD_getInstructionParameter2(inst), ifuncList);
                break;

                /* One source parameter. */
            case IRNEG:
            case IRNOT:
            case IRCONV:
            case IRCONVOVF:
            case IRRET:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRMOVE:
                checkAndAddParamFuncPtrToList(IRMETHOD_getInstructionParameter1(inst), ifuncList);
                break;

                /* For calls we check the call parameters. */
            case IRCALL:
            case IRLIBRARYCALL:
            case IRVCALL:
            case IRICALL:
            case IRNATIVECALL:
                for (c = 0; c < IRMETHOD_getInstructionCallParametersNumber(inst); ++c) {
                    ir_item_t *param = IRMETHOD_getInstructionCallParameter(inst, c);
                    checkAndAddParamFuncPtrToList(param, ifuncList);
                }
                break;
        }
    }
}


/**
 * Build a list of methods that have their address taken.  To do this we
 * scan all instructions in all methods from the entry point looking for
 * stores to variables from function addresses.  We then simply add each
 * function to the list.
 */
static XanList * buildIndirectFunctionList (ir_method_t *entryMethod) {
    XanList *ifuncList;
    XanList *methodList;
    XanList *seenMethods;

    /* Initialise all lists. */
    ifuncList = xanList_new(allocFunction, freeFunction, NULL);
    methodList = xanList_new(allocFunction, freeFunction, NULL);
    seenMethods = xanList_new(allocFunction, freeFunction, NULL);

    /* Start with the entry method. */
    xanList_insert(methodList, entryMethod);
    xanList_insert(seenMethods, entryMethod);

    /* Iterate over all reachable methods. */
    while (xanList_length(methodList) > 0) {
        XanListItem *currItem = xanList_first(methodList);
        ir_method_t *currMethod = (ir_method_t *) currItem->data;
        xanList_deleteItem(methodList, currItem);

        /* Add called methods to the worklist. */
        addCalledMethodsToWorklist(currMethod, methodList, seenMethods);

        /* Check this method for function addresses being used. */
        addFuncPtrsToList(currMethod, ifuncList);
    }

    /* Clean up. */
    xanList_destroyList(methodList);
    xanList_destroyList(seenMethods);

    /* Return our created list. */
    return ifuncList;
}


/**
 * Convert a single indirect call to a direct one.
 */
static void
convertIndirectCall (ir_method_t *method, ir_instruction_t *inst,
                     ir_method_t *calledMethod) {
    IR_ITEM_VALUE calledId = IRMETHOD_getMethodID(calledMethod);

    assert(inst->type == IRICALL);
    IRMETHOD_setInstructionType(method, inst, IRCALL);
    IRMETHOD_setInstructionParameter1(method, inst, calledId, 0.0, IRMETHODID, IRFPOINTER, NULL);
    IRMETHOD_setInstructionParameter2(method, inst, 0, 0.0, IRINT32, IRINT32, NULL);
    IRMETHOD_setInstructionParameter3Type(method, inst, NOPARAM);
}


/**
 * Try to convert indirect calls to direct calls by seeing if only one
 * function can be called by any of the indirect calls in the method.  This
 * is done by taking the intersection between the list of callable methods
 * and the list of methods that have had their addresses taken somewhere in
 * the program.  If there is only one item, a conversion can be applied.
 */
static void tryConvertingIndirectCalls (ir_method_t *method, XanList *ifuncList) {
    JITUINT32 	i, converted = 0;
    XanList		*escapedMethods;

    /* Fetch the escaped methods.
     */
    escapedMethods	= IRPROGRAM_getEscapedMethods();

    /* Check all instructions for indirect calls. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t 	*inst;
        XanList			*callableMethods;

        /* Fetch the instruction.
         */
        inst 	= IRMETHOD_getInstructionAtPosition(method, i);
        if (inst->type != IRICALL) {
            continue ;
        }

        /* Get the list of callable methods.
         */
        callableMethods		= METHODS_getCallableIRMethods(inst, escapedMethods);
        assert(callableMethods != NULL);

        /* Get the intersection between the two lists of methods.
         */
        XanList *iList = getIntersectionList(callableMethods, ifuncList);
        assert(xanList_length(iList) > 0);

        /* Make the conversion if there's only one option.
         */
        if (xanList_length(iList) == 1) {
            ir_method_t *calledMethod = (ir_method_t *) xanList_first(iList)->data;
            convertIndirectCall(method, inst, calledMethod);
            ++converted;
        }

        /* Clean up.
         */
        xanList_destroyList(callableMethods);
        xanList_destroyList(iList);
    }

    /* Information. */
#ifdef PRINTDEBUG
    if (converted > 0) {
        PDEBUG("OPTIMIZER_FPE: Converted %u indirect calls in %s\n", converted, IRMETHOD_getCompleteMethodName(method));
    }
#endif

    /* Clean up.
     */
    xanList_destroyList(escapedMethods);

    return ;
}


/**
 * Entry point for the function pointer removal algorithm.  The basic idea
 * is to remove indirect calls (convert them to direct calls) if it can be
 * proven that there is only one possible function that they can call.
 */
static inline void functionpointerselimination_do_job (ir_method_t * method) {
    ir_method_t *entryMethod;
    XanList *ifuncList;

    /* Checks. */
    assert(method != NULL);

    /* See if there's work to be done. */
    if (!hasIndirectCall(method)) {
        return;
    }

    /* Build the list of methods that have their address taken. */
    entryMethod = IRPROGRAM_getEntryPointMethod();
    ifuncList = buildIndirectFunctionList(entryMethod);

    /* See if any function pointers can be removed in this method. */
    if (xanList_length(ifuncList) > 0) {
        tryConvertingIndirectCalls(method, ifuncList);
    }

    /* Clean up. */
    xanList_destroyList(ifuncList);

    return ;
}


ir_optimization_interface_t plugin_interface = {
    functionpointerselimination_get_ID_job,
    functionpointerselimination_get_dependences,
    functionpointerselimination_init,
    functionpointerselimination_shutdown,
    functionpointerselimination_do_job,
    functionpointerselimination_get_version,
    functionpointerselimination_get_informations,
    functionpointerselimination_get_author,
    functionpointerselimination_get_invalidations
};

static inline JITUINT64 functionpointerselimination_get_invalidations (void) {
    return INVALIDATE_ALL;
}

static inline void functionpointerselimination_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

static inline void functionpointerselimination_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

static inline JITUINT64 functionpointerselimination_get_ID_job (void) {
    return INDIRECT_CALL_ELIMINATION;
}

static inline JITUINT64 functionpointerselimination_get_dependences (void) {
    return 0;
}

static inline char * functionpointerselimination_get_version (void) {
    return VERSION;
}

static inline char * functionpointerselimination_get_informations (void) {
    return INFORMATIONS;
}

static inline char * functionpointerselimination_get_author (void) {
    return AUTHOR;
}
