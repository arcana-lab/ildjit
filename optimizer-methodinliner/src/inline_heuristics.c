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
#include <assert.h>
#include <errno.h>
#include <ir_method.h>
#include <xanlib.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <ir_optimization_interface.h>
#include <chiara.h>

// My headers
#include <optimizer_methodinliner.h>
#include <inline_heuristics.h>
#include <inliner_configuration.h>
// End

extern ir_lib_t *irLib;

static inline JITBOOLEAN internal_doesMethodPerformIO (ir_method_t *method);
static inline void internal_remove_not_profitable_to_inline_calls (ir_method_t *startMethod, XanList *calls);
static inline void internal_remove_calls_to_large_methods (ir_method_t *startMethod, XanList *calls);
static inline void internal_remove_calls_to_methods_with_catch_blocks (ir_method_t *startMethod, XanList *calls);
static inline void internal_check_caller (ir_method_t *startMethod, XanList *calls);
static inline void internal_remove_calls_to_avoid (XanList *calls);
static inline void internal_remove_calls_to_library_methods (ir_method_t *startMethod, XanList *calls);
static inline void internal_remove_calls_to_not_worth_methods (ir_method_t *startMethod, XanList *calls);
static inline JITUINT32 internal_get_weight (ir_method_t *method);
static inline XanList * internal_fetchCallInstructionsToSmallMethods (ir_method_t *method);

static JITUINT32 current_max_insns_without_loops	= 0;
static JITUINT32 current_max_insns_with_loops		= 0;
static JITUINT32 current_max_insns			= 0;
static XanHashTable *current_methodsToAvoid		= NULL;

extern ir_optimizer_t *irOptimizer;

XanList * fetchMethodsToConsider (void) {
    XanList		*l;
    XanList		*toDelete;
    XanListItem	*item;
    ir_method_t	*mainMethod;

    /* Remove dead methods.
     */
    IROPTIMIZER_callMethodOptimization(irOptimizer, IRPROGRAM_getEntryPointMethod(), DEADCODE_ELIMINATOR);

    /* Allocate the necessary memory.
     */
    toDelete	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the main method.
     */
    mainMethod	= METHODS_getMainMethod();
    assert(mainMethod != NULL);

    /* Fetch the list of methods reachable from the main method.
     */
    l = IRPROGRAM_getReachableMethods(mainMethod);
    assert(l != NULL);

    /* Filter out methods that belong to libraries.
     */
    item	= xanList_first(l);
    while (item != NULL) {
        ir_method_t	*m;
        m	= item->data;
        assert(m != NULL);
        if (IRPROGRAM_doesMethodBelongToALibrary(m)) {
            xanList_insert(toDelete, m);
        }
        item	= item->next;
    }
    xanList_removeElements(l, toDelete, JITTRUE);

    /* Free the memory.
     */
    xanList_destroyList(toDelete);

    return l;
}

XanList * fetchCallInstructions (ir_method_t *method, JITUINT32 max_insns_without_loops, JITUINT32 max_insns_with_loops, JITUINT32 max_insns, XanHashTable *methodsToAvoid) {
    XanList                 *list;

    /* Set the thresholds to use.
     */
    current_max_insns_without_loops				= max_insns_without_loops;
    current_max_insns_with_loops				= max_insns_with_loops;
    current_max_insns					= max_insns;
    current_methodsToAvoid					= methodsToAvoid;

    /* Fetch the corner case		*/
    if (IRMETHOD_hasCatchBlocks(method)) {
        return xanList_new(allocFunction, freeFunction, NULL);
    }

    /* Fetch the list of calls to inline	*/
    list = internal_fetchCallInstructionsToSmallMethods(method);

    /* Check the list			*/
#ifdef DEBUG
    XanListItem     *item;
    item = xanList_first(list);
    while (item != NULL) {
        ir_instruction_t        *inst;
        inst 		= item->data;
        assert(inst != NULL);
        assert(IRMETHOD_getInstructionType(inst) == IRCALL);
        item 		= item->next;
    }
#endif

    return list;
}

void remove_calls (ir_method_t *startMethod, XanList *calls) {

    /* Assertions.
     */
    assert(startMethod != NULL);
    assert(calls != NULL);

    /* Remove calls to methods with catch blocks.
     */
    internal_remove_not_profitable_to_inline_calls(startMethod, calls);

    return;
}

static inline void internal_remove_calls_to_not_worth_methods (ir_method_t *startMethod, XanList *calls) {
    XanList         *toDelete;
    XanListItem     *item;

    /* Assertions.
     */
    assert(startMethod != NULL);
    assert(calls != NULL);

    /* Allocate the necessary memories.
     */
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Remove calls to methods that belong to libraries.
     */
    item = xanList_first(calls);
    while (item != NULL) {
        ir_instruction_t        *call;
        ir_method_t             *methodToInline;
        JITINT8	                *name;

        /* Fetch the call		*/
        call = item->data;
        assert(call != NULL);

        /* Fetch the method		*/
        assert(irLib != NULL);
        methodToInline = IRMETHOD_getCalledMethod(startMethod, call);
        assert(methodToInline != NULL);

        /* Fetch the name of the method.
         */
        name	= IRMETHOD_getMethodName(methodToInline);

        /* Check if we know this method.
         */
        if (name != NULL) {

            /* Check if the method belongs to the base class library.
             */
            if (IRPROGRAM_doesMethodBelongToALibrary(methodToInline)) {

                /* Check whether the method is known to be not worth it to inline.
                 */
                if (	(STRCMP(name, "strlen") == 0)	||
                        (STRCMP(name, "getenv") == 0)	||
                        (STRCMP(name, "rand") == 0)	||
                        (STRCMP(name, "srand") == 0)	||
                        (STRCMP(name, "setbuf") == 0)	||
                        (STRCMP(name, "atoi") == 0)	||
                        (STRCMP(name, "setjmp") == 0)	||
                        (STRCMP(name, "malloc") == 0)	||
                        (STRCMP(name, "realloc") == 0)	||
                        (STRCMP(name, "calloc") == 0)	||
                        (STRCMP(name, "free") == 0)	||
                        (STRCMP(name, "exit") == 0)	||
                        (STRCMP(name, "read") == 0)	||
                        (STRCMP(name, "write") == 0)	||
                        (STRCMP(name, "clock") == 0)	||
                        (STRCMP(name, "ctime") == 0)	||
                        (STRCMP(name, "strstr") == 0)	||
                        (STRCMP(name, "strtok") == 0)	||
                        (STRCMP(name, "strcspn") == 0)	||
                        (STRCMP(name, "strcmp") == 0)	||
                        (STRCMP(name, "strncmp") == 0)  ||
                        (STRCMP(name, "strcpy") == 0)   ||
                        (STRCMP(name, "strncpy") == 0)  ||
                        (STRCMP(name, "sprintf") == 0)  ||
                        (STRCMP(name, "snprintf") == 0) ||
                        (STRCMP(name, "scanf") == 0)    ||
                        (STRCMP(name, "sscanf") == 0)   ||
                        (STRCMP(name, "main") == 0)     ||
                        (STRCMP(name, "Main") == 0)     ||
                        (STRCMP(name, "memcmp") == 0)   ||
                        (STRCMP(name, "memcpy") == 0)	) {
                    xanList_append(toDelete, call);
                }

            } else {

                /* Check whether the method is known to be not worth it to inline.
                 */
                if (	(STRCMP(name, "spec_write") == 0)	||
                        (STRCMP(name, "spec_putc") == 0)	||
                        (STRCMP(name, "spec_reset") == 0)	||
                        (STRCMP(name, "spec_rewind") == 0)	||
                        (STRCMP(name, "spec_ungetc") == 0)	||
                        (STRCMP(name, "spec_getc") == 0)	||
                        (STRCMP(name, "spec_read") == 0)	||
                        (STRCMP(name, "spec_load") == 0)	||
                        (STRCMP(name, "spec_random_load") == 0)	||
                        (STRCMP(name, "BUFFER_setStream") == 0)	                ||
                        (STRCMP(name, "BUFFER_bsR1") == 0)	                ||
                        (STRCMP(name, "BUFFER_needToReadNumberOfBits") == 0)	                ||
                        (STRCMP(name, "BUFFER_setNumberOfBytesWritten") == 0)	                ||
                        (STRCMP(name, "BUFFER_getNumberOfBytesWritten") == 0)	                ||
                        (STRCMP(name, "BUFFER_readBits") == 0)	                ||
                        (STRCMP(name, "BUFFER_writeBit") == 0)	                ||
                        (STRCMP(name, "BUFFER_writeBits") == 0)	                ||
                        (STRCMP(name, "BUFFER_writeBitsFromValue") == 0)	                ||
                        (STRCMP(name, "BUFFER_writeUChar") == 0)	                ||
                        (STRCMP(name, "BUFFER_writeUInt") == 0)	                ||
                        (STRCMP(name, "BUFFER_flush") == 0)	                ){
                    xanList_append(toDelete, call);
                }
            }
        }

        /* Fetch the next element			*/
        item = item->next;
    }
    xanList_removeElements(calls, toDelete, JITTRUE);

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    return ;
}

static inline void internal_remove_calls_to_library_methods (ir_method_t *startMethod, XanList *calls) {
    XanList         *toDelete;
    XanListItem     *item;

    /* Assertions.
     */
    assert(startMethod != NULL);
    assert(calls != NULL);

    /* If the caller is a library method, do not inline anything.
     */
    if (IRPROGRAM_doesMethodBelongToALibrary(startMethod)) {
        xanList_emptyOutList(calls);
        return ;
    }

    /* Allocate the necessary memories.
     */
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Remove calls to methods that belong to libraries.
     */
    item = xanList_first(calls);
    while (item != NULL) {
        ir_instruction_t        *call;
        ir_method_t             *methodToInline;

        /* Fetch the call		*/
        call = item->data;
        assert(call != NULL);

        /* Fetch the method		*/
        assert(irLib != NULL);
        methodToInline = IRMETHOD_getCalledMethod(startMethod, call);
        assert(methodToInline != NULL);

        /* Check if the method belongs to the base class library.
         */
        if (IRPROGRAM_doesMethodBelongToALibrary(methodToInline)) {
            xanList_append(toDelete, call);
        }

        /* Fetch the next element			*/
        item = item->next;
    }
    xanList_removeElements(calls, toDelete, JITTRUE);

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    return;
}

static inline void internal_remove_calls_to_special_methods (ir_method_t *startMethod, XanList *calls) {
    XanList         *toDelete;
    XanListItem     *item;

    /* Assertions.
     */
    assert(startMethod != NULL);
    assert(calls != NULL);

    /* Allocate the necessary memories.
     */
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Remove calls to methods with	the catcher.
     */
    item = xanList_first(calls);
    while (item != NULL) {
        ir_instruction_t        *call;
        ir_method_t             *methodToInline;

        /* Fetch the call.
         */
        call = item->data;
        assert(call != NULL);

        /* Fetch the method.
         */
        assert(irLib != NULL);
        methodToInline = IRMETHOD_getCalledMethod(startMethod, call);
        assert(methodToInline != NULL);

        /* Check if the method performs I/O.
         */
        if (internal_doesMethodPerformIO(methodToInline)) {
            xanList_append(toDelete, call);
        }

        /* Fetch the next element.
         */
        item = item->next;
    }
    xanList_removeElements(calls, toDelete, JITTRUE);

    /* Free the memory.
     */
    xanList_destroyList(toDelete);

    return;
}

static inline void internal_remove_calls_to_large_methods (ir_method_t *startMethod, XanList *calls) {
    XanListItem     *item;
    XanList         *toDelete;

    /* Assertions.
     */
    assert(startMethod != NULL);
    assert(calls != NULL);

    /* Allocate the necessary memories.
     */
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    item = xanList_first(calls);
    while (item != NULL) {
        ir_instruction_t        *call;
        ir_method_t             *methodToInline;
        JITUINT32		methodWeight;
        JITBOOLEAN		thresholdWithoutLoops;
        JITBOOLEAN		thresholdWithLoops;

        /* Fetch the call.
         */
        call = item->data;
        assert(call != NULL);

        /* Fetch the method.
         */
        assert(irLib != NULL);
        methodToInline = IRMETHOD_getIRMethodFromMethodID(startMethod, IRMETHOD_getInstructionParameter1Value(call));
        assert(methodToInline != NULL);

        /* Fetch the weight of the called method.
         */
        methodWeight	= internal_get_weight(methodToInline);

        /* Check if the method should be inlined.
         */
        thresholdWithoutLoops	= (methodWeight > current_max_insns_without_loops);
        thresholdWithLoops	= hasLoop(methodToInline) && (methodWeight > current_max_insns_with_loops);
        if (	(thresholdWithoutLoops) 	||
                (thresholdWithLoops)		) {

            /* Update the feedbacks.
             */
            if (thresholdWithLoops) {
                inlinedBlockedForMaxInsnsWithLoopsInputConstraints	= JITTRUE;
                if (	(minInsnsWithLoopsInputConstraintsToUnblockInline == 0)			||
                        (methodWeight < minInsnsWithLoopsInputConstraintsToUnblockInline)	) {
                    minInsnsWithLoopsInputConstraintsToUnblockInline	= methodWeight;
                }
            } else if (thresholdWithoutLoops) {
                inlinedBlockedForMaxInsnsWithoutLoopsInputConstraints	= JITTRUE;
                if (	(minInsnsWithoutLoopsInputConstraintsToUnblockInline == 0)		||
                        (methodWeight < minInsnsWithoutLoopsInputConstraintsToUnblockInline)	) {
                    minInsnsWithoutLoopsInputConstraintsToUnblockInline	= methodWeight;
                }
            }

            /* The current callee should not be inlined in the caller.
             */
            xanList_append(toDelete, call);
        }

        /* Fetch the next element in the list.
         */
        item = item->next;
    }
    xanList_removeElements(calls, toDelete, JITTRUE);

    /* Free the memory.
     */
    xanList_destroyList(toDelete);

    return;
}

static inline JITUINT32 internal_get_weight (ir_method_t *method) {

    /* Assertions.
     */
    assert(method != NULL);

    /* Set the method dummy instructions.
     */
    IRMETHOD_addMethodDummyInstructions(method);

    /* Update the number of variables used by the method.
     */
    IRMETHOD_updateNumberOfVariablesNeededByMethod(method);
    assert(IRMETHOD_getInstructionsNumber(method) >= IRMETHOD_getMethodParametersNumber(method));

    return IRMETHOD_getInstructionsNumber(method) - IRMETHOD_getMethodParametersNumber(method);
}

static inline XanList * internal_fetchCallInstructionsToSmallMethods (ir_method_t *method) {
    XanList                 *list;

    /* Get every call			*/
    list = IRMETHOD_getInstructionsOfType(method, IRCALL);
    assert(list != NULL);

    /* Remove calls 			*/
    internal_remove_not_profitable_to_inline_calls(method, list);

    return list;
}

static inline void internal_remove_calls_to_avoid (XanList *calls) {
    XanList		*toDelete;
    XanListItem	*item;

    if (xanHashTable_elementsInside(current_methodsToAvoid) == 0) {
        return ;
    }

    toDelete	= xanList_new(allocFunction, freeFunction, NULL);
    item		= xanList_first(calls);
    while (item != NULL) {
        if (xanHashTable_lookup(current_methodsToAvoid, item->data) != NULL) {
            xanList_append(toDelete, item->data);
        }
        item	= item->next;
    }
    xanList_removeElements(calls, toDelete, JITTRUE);
    xanList_destroyList(toDelete);

    return ;
}

static inline void internal_check_caller (ir_method_t *startMethod, XanList *calls) {
    JITUINT32	currentMethodInsns;

    /* Check whether the method performs IO.
     */
    if (internal_doesMethodPerformIO(startMethod)) {
        xanList_emptyOutList(calls);
        return ;
    }

    /* Check the constraint.
     */
    currentMethodInsns	= IRMETHOD_getInstructionsNumber(startMethod);
    if (currentMethodInsns <= current_max_insns) {
        return ;
    }

    inlinedBlockedForMaxCallerInsnsInputConstraints	= JITTRUE;
    if (	(minCallerInsnsToUnblockInline == 0)			||
            (currentMethodInsns < minCallerInsnsToUnblockInline)	) {
        minCallerInsnsToUnblockInline	= currentMethodInsns;
    }
    xanList_emptyOutList(calls);

    return ;
}

static inline void internal_remove_calls_to_methods_with_catch_blocks (ir_method_t *startMethod, XanList *calls) {
    XanListItem     *item;
    XanList         *toDelete;

    /* Assertions			*/
    assert(startMethod != NULL);
    assert(calls != NULL);

    /* Allocate the necessary	*
     * memories			*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    item = xanList_first(calls);
    while (item != NULL) {
        ir_instruction_t        *call;
        ir_method_t             *methodToInline;

        /* Fetch the call		*/
        call = item->data;
        assert(call != NULL);

        /* Fetch the method		*/
        assert(irLib != NULL);
        methodToInline = IRMETHOD_getIRMethodFromMethodID(startMethod, IRMETHOD_getInstructionParameter1Value(call));
        assert(methodToInline != NULL);

        /* Check if the method has a body	*/
        if (IRMETHOD_hasCatchBlocks(methodToInline)) {
            xanList_append(toDelete, call);
        }

        /* Get the method		*/
        item = item->next;
    }
    xanList_removeElements(calls, toDelete, JITTRUE);

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    /* Return			*/
    return;
}

static inline void internal_remove_not_profitable_to_inline_calls (ir_method_t *startMethod, XanList *calls) {

    /* Filter out elements to avoid.
     */
    internal_remove_calls_to_avoid(calls);

    /* Check whether there are elements to inline.
     */
    if (xanList_length(calls) == 0) {
        return ;
    }

    /* Check the caller.
     */
    internal_check_caller(startMethod, calls);

    /* Remove calls to methods with catch blocks.
     */
    internal_remove_calls_to_methods_with_catch_blocks(startMethod, calls);

    /* Remove calls to small methods.
     */
    internal_remove_calls_to_large_methods(startMethod, calls);

    /* Remove calls to methods that do not belong to the original program.
     */
    if (disableLibraries) {
        internal_remove_calls_to_library_methods(startMethod, calls);
    }

    /* Remove calls to methods that are not worth it.
     */
    if (disableNotWorthFunctions) {
        internal_remove_calls_to_not_worth_methods(startMethod, calls);
    }

    /* Remove calls to specific methods.
     */
    internal_remove_calls_to_special_methods(startMethod, calls);

    return ;
}

JITBOOLEAN hasLoop (ir_method_t *m) {
    JITUINT32	instructionsNumber;
    JITBOOLEAN	loopFound;
    JITUINT32 	count;

    /* Assertions.
     */
    assert(m != NULL);

    /* Initialize the variables.
     */
    loopFound		= JITFALSE;

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber	= IRMETHOD_getInstructionsNumber(m);

    /* Check if the method contain a loop.
     */
    for (count=0; count < instructionsNumber; count++) {
        ir_instruction_t	*i;
        ir_instruction_t	*takenBranch;

        /* Fetch the instruction.
         */
        i	= IRMETHOD_getInstructionAtPosition(m, count);

        /* Check the instruction in case it is a branch one.
         */
        if (!IRMETHOD_isAJumpInstruction(i)) {
            continue;
        }
        if (IRMETHOD_isACallInstruction(i)) {
            continue;
        }

        /* The instruction is a branch.
         * Fetch the taken branch instruction.
         */
        takenBranch	= IRMETHOD_getBranchDestination(m, i);
        if (takenBranch == NULL) {
            continue;
        }

        /* Check if the current edge of the CFG is a backedge.
         */
        if (takenBranch->ID < i->ID) {
            loopFound	= JITTRUE;
            break;
        }
    }

    return loopFound;
}

static inline JITBOOLEAN internal_doesMethodPerformIO (ir_method_t *method) {
    JITINT8	*name;

    /* Assertions.
     */
    assert(method != NULL);

    /* Fetch the name of the method.
     */
    name = IRMETHOD_getMethodName(method);
    assert(name != NULL);

    /* Check the name of the method.
     */
    if (    (STRCMP(name, "fflush") == 0)                   ||
            (STRCMP(name, "fclose") == 0)                   ||
            (STRCMP(name, "fprintf") == 0)                  ||
            (STRCMP(name, "fwrite") == 0)                   ||
            (STRCMP(name, "fscanf") == 0)                   ||
            (STRCMP(name, "fseek") == 0)                    ||
            (STRCMP(name, "rewind") == 0)                   ||
            (STRCMP(name, "__device_write") == 0)           ||
            (STRCMP(name, "__device_read") == 0)            ||
            (STRCMP(name, "__io_flush") == 0)               ||
            (STRCMP(name, "__io_ftable_close_all") == 0)    ||
            (STRCMP(name, "__io_fread") == 0)               ||
            (STRCMP(name, "_") == 0)                        ||
            (STRCMP(name, "Obj?27?__do__atexit") == 0)      ||
            (STRCMP(name, "fputc") == 0)                    ||
            (STRCMP(name, "fputs") == 0)                    ||
            (STRCMP(name, "putchar") == 0)                  ||
            (STRCMP(name, "putc") == 0)                     ||
            (STRCMP(name, "puts") == 0)                     ||
            (STRCMP(name, "fgets") == 0)                    ||
            (STRCMP(name, "fgetc") == 0)                    ||
            (STRCMP(name, "getc") == 0)                     ||
            (STRCMP(name, "ungetc") == 0)                   ||
            (STRCMP(name, "feof") == 0)                     ||
            (STRCMP(name, "fopen") == 0)			        ||
            (STRCMP(name, "open") == 0)			            ||
            (STRCMP(name, "fclose") == 0)			        ||
            (STRCMP(name, "close") == 0)			        ||
            (STRCMP(name, "__io_ftable_get_entry") == 0)    ||
        	(STRCMP(name, "spec_write") == 0)	||
            (STRCMP(name, "spec_putc") == 0)	||
            (STRCMP(name, "spec_reset") == 0)	||
            (STRCMP(name, "spec_rewind") == 0)	||
            (STRCMP(name, "spec_ungetc") == 0)	||
            (STRCMP(name, "spec_getc") == 0)	||
            (STRCMP(name, "spec_read") == 0)	||
            (STRCMP(name, "spec_load") == 0)	||
            (STRCMP(name, "spec_random_load") == 0)	||
            (STRCMP(name, "BUFFER_setStream") == 0)	                ||
            (STRCMP(name, "BUFFER_bsR1") == 0)	                ||
            (STRCMP(name, "BUFFER_needToReadNumberOfBits") == 0)	                ||
            (STRCMP(name, "BUFFER_setNumberOfBytesWritten") == 0)	                ||
            (STRCMP(name, "BUFFER_getNumberOfBytesWritten") == 0)	                ||
            (STRCMP(name, "BUFFER_readBits") == 0)	                ||
            (STRCMP(name, "BUFFER_writeBit") == 0)	                ||
            (STRCMP(name, "BUFFER_writeBits") == 0)	                ||
            (STRCMP(name, "BUFFER_writeBitsFromValue") == 0)	                ||
            (STRCMP(name, "BUFFER_writeUChar") == 0)	                ||
            (STRCMP(name, "BUFFER_writeUInt") == 0)	                ||
            (STRCMP(name, "BUFFER_flush") == 0)	                ||
            (STRCMP(name, "printf") == 0)                   ) {
        return JITTRUE;
    }

    return JITFALSE;
}
