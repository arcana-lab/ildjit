/*
 * Copyright (C) 2006 - 2012 Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <jitsystem.h>
#include <compiler_memory_manager.h>
#include <base_symbol.h>

// My headers
#include <ir_method.h>
// End

extern ir_lib_t * ir_library;

static inline void internal_check_and_add_escaped_method (ir_method_t *method, ir_item_t *par, XanList *l);
static inline void _IRPROGRAM_getReachableMethods (ir_method_t *startMethod, XanHashTable *l, XanHashTable *escapedMethods);
static inline JITBOOLEAN internal_IRPROGRAM_isInstructionTypeReachable (ir_method_t *startMethod, JITUINT16 instructionType, XanList *alreadyConsidered, XanList *escapedMethods, XanList *reachableMethodsFromEntryPoint);
static inline void internal_addReachableMethods (XanList *methods, XanList *l);

XanList * IRPROGRAM_fetchILBinaries (void) {
    return *ir_library->ilBinaries;
}

XanList * IRPROGRAM_getMethodsThatInitializeGlobalMemories (void) {
    XanList		*l;
    XanList		*methods;
    XanListItem	*item;

    /* Allocate the necessary memory.
     */
    l	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the methods.
     */
    methods	= IRPROGRAM_getIRMethods();

    /* Add the methods that are used to initialize the global memories to the list.
     */
    item	= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*m;
        m	= item->data;
        assert(m != NULL);
        if (IRMETHOD_doesMethodInitializeGlobalMemory(m) && IRMETHOD_hasMethodInstructions(m)) {
            xanList_append(l, m);
        }
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(methods);

    return l;
}

XanList * IRPROGRAM_getReachableMethodsNeededByProgram (void) {
    ir_method_t	*entryMethod;
    XanList		*l;
    XanList		*cctors;
    XanList		*allMethods;
    XanList		*toDelete;
    XanListItem	*item;

    /* Allocate the necessary memory.
     */
    toDelete	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the entry point of the program.
     */
    entryMethod	= IRPROGRAM_getEntryPointMethod();
    assert(entryMethod != NULL);

    /* Fetch the methods reachable from the entry point.
     */
    l		= IRPROGRAM_getReachableMethods(entryMethod);
    assert(l != NULL);

    /* Add methods reachable from methods that initialize global memories.
     */
    cctors		= IRPROGRAM_getMethodsThatInitializeGlobalMemories();
    internal_addReachableMethods(cctors, l);

    /* Add methods that can be called from outside the IR code.
     */
    allMethods = IRPROGRAM_getIRMethods();
    item		= xanList_first(allMethods);
    while (item != NULL) {
        ir_method_t	*m;
        m	= item->data;
        if (!IRMETHOD_isCallableExternally(m)) {
            xanList_append(toDelete, m);
        }
        item	= item->next;
    }
    xanList_removeElements(allMethods, toDelete, JITTRUE);
    internal_addReachableMethods(allMethods, l);

    /* Free the memory.
     */
    xanList_destroyList(toDelete);
    xanList_destroyList(cctors);
    xanList_destroyList(allMethods);

    return l;
}

JITBOOLEAN IRPROGRAM_isInstructionTypeReachable (ir_method_t *startMethod, JITUINT16 instructionType, XanList *escapedMethods, XanList *reachableMethods) {
    JITBOOLEAN 	found;
    XanList 	*alreadyConsidered;

    /* Allocate the memory.
     */
    alreadyConsidered = xanList_new(allocFunction, freeFunction, NULL);

    /* Check if it is reachable.
     */
    found = internal_IRPROGRAM_isInstructionTypeReachable(startMethod, instructionType, alreadyConsidered, escapedMethods, reachableMethods);

    /* Free the memory.
     */
    xanList_destroyList(alreadyConsidered);

    return found;
}

XanList * IRPROGRAM_getCallableMethods (ir_instruction_t *inst, XanList *escapedMethods, XanList *reachableMethodsFromEntryPoint) {
    XanHashTable	*r;
    XanHashTable 	*e;
    XanList 	*l;
    XanList 	*temp;
    XanList 	*toDelete;
    XanListItem 	*item;

    /* Assertions			*/
    assert(inst != NULL);

    /* Fetch the list of callable	*
     * methods			*/
    l = IRMETHOD_getCallableMethods(inst);
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

    /* Fetch the list of reachable methods.
     */
    if (reachableMethodsFromEntryPoint == NULL) {
        ir_method_t *mainMethod;
        mainMethod 	= IRPROGRAM_getEntryPointMethod();
        assert(mainMethod != NULL);
        temp 		= IRPROGRAM_getReachableMethods(mainMethod);
    } else {
        temp		= reachableMethodsFromEntryPoint;
    }
    assert(temp != NULL);
    r 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    item 	= xanList_first(temp);
    while (item != NULL) {
        ir_method_t *m;
        IR_ITEM_VALUE id;
        m = item->data;
        assert(m != NULL);
        id = IRMETHOD_getMethodID(m);
        if (id != 0) {
            xanHashTable_insert(r, (void *)(JITNUINT)id, (void *)(JITNUINT)id);
        }
        item = item->next;
    }
    assert(r != NULL);

    /* Free the memory.
     */
    if (reachableMethodsFromEntryPoint == NULL) {
        xanList_destroyList(temp);
    }
    temp	= NULL;

    /* Fetch the list of methods that are escaped (they could be called indirectly).
     */
    if (escapedMethods == NULL) {
        temp = IRPROGRAM_getEscapedMethods();
    } else {
        temp = escapedMethods;
    }
    assert(temp != NULL);
    e 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    item 	= xanList_first(temp);
    while (item != NULL) {
        ir_method_t *m;
        IR_ITEM_VALUE id;
        m = item->data;
        assert(m != NULL);
        id = IRMETHOD_getMethodID(m);
        if (id != 0) {
            xanHashTable_insert(e, (void *)(JITNUINT)id, (void *)(JITNUINT)id);
        }
        item = item->next;
    }
    if (escapedMethods == NULL) {
        xanList_destroyList(temp);
    }

    /* Remove those methods that are both not reachable from the entry point of the program and not escaped.
     */
    toDelete = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(l);
    while (item != NULL) {
        void *id;
        id = item->data;
        if (	(id == 0)				||
                (xanHashTable_lookup(r, id) == NULL)	||
                (xanHashTable_lookup(e, id) == NULL)	) {
            xanList_append(toDelete, id);
        }
        item = item->next;
    }
    xanList_removeElements(l, toDelete, JITTRUE);

    /* Free the memory.
     */
    xanHashTable_destroyTable(r);
    xanHashTable_destroyTable(e);
    xanList_destroyList(toDelete);

    return l;
}

XanList * IRPROGRAM_getReachableMethods (ir_method_t *startMethod) {
    XanHashTable 	*l;
    XanList 	*list;
    XanList		*escapedMethods;
    XanHashTable 	*escapedMethodsTable;

    /* Assertions				*/
    assert(startMethod != NULL);

    /* Allocate the list of reachable	*
     * methods				*/
    l 		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(l != NULL);

    /* Fetch the escaped methods	*/
    escapedMethods 		= IRPROGRAM_getEscapedMethods();
    escapedMethodsTable	= xanList_toHashTable(escapedMethods);

    /* Fill up the hash table		*/
    xanHashTable_insert(l, startMethod, startMethod);
    _IRPROGRAM_getReachableMethods(startMethod, l, escapedMethodsTable);

    /* Fetch the list of reachable methods 	*/
    list = xanHashTable_toList(l);

    /* Free the memory			*/
    xanHashTable_destroyTable(l);
    xanList_destroyList(escapedMethods);
    xanHashTable_destroyTable(escapedMethodsTable);

    return list;
}

XanList * IRPROGRAM_getEscapedMethods (void) {
    XanList *l;
    XanList *all;
    XanListItem *item;

    /* Allocate the list of escaped		*
     * methods				*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch all IR methods			*/
    all = IRPROGRAM_getIRMethods();
    assert(all != NULL);

    /* Check every method			*/
    item = xanList_first(all);
    while (item != NULL) {
        ir_method_t *m;
        JITUINT32 i;
        m = item->data;
        assert(m != NULL);
        for (i=0; i < IRMETHOD_getInstructionsNumber(m); i++) {
            ir_instruction_t *inst;
            ir_item_t *par;
            JITUINT32 j;
            inst = IRMETHOD_getInstructionAtPosition(m, i);
            assert(inst != NULL);
            switch (IRMETHOD_getInstructionParametersNumber(inst)) {
                case 3:
                    par = IRMETHOD_getInstructionParameter3(inst);
                    internal_check_and_add_escaped_method(m, par, l);
                case 2:
                    par = IRMETHOD_getInstructionParameter2(inst);
                    internal_check_and_add_escaped_method(m, par, l);
                case 1:
                    par = IRMETHOD_getInstructionParameter1(inst);
                    internal_check_and_add_escaped_method(m, par, l);
                case 0:
                    par = IRMETHOD_getInstructionResult(inst);
                    internal_check_and_add_escaped_method(m, par, l);
            }
            for (j=0; j < IRMETHOD_getInstructionCallParametersNumber(inst); j++) {
                ir_item_t *call;
                call = IRMETHOD_getInstructionCallParameter(inst, j);
                assert(call != NULL);
                internal_check_and_add_escaped_method(m, call, l);
            }
        }
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(all);

    return l;
}

ir_method_t * IRPROGRAM_getMethodWithSignatureInString (JITINT8 *signatureInString) {
    ir_method_t	*foundM;
    XanList		*methods;
    XanListItem	*item;

    /* Initialize the variables.
     */
    foundM	= NULL;

    /* Fetch all the methods.
     */
    methods	= ir_library->getIRMethods();
    assert(methods != NULL);

    /* Create the list.
     */
    item	= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*m;
        JITINT8		*currentName;
        m	= item->data;
        assert(m != NULL);
        currentName	= IRMETHOD_getSignatureInString(m);
        if (	(currentName != NULL)					&&
                (STRCMP(currentName, signatureInString) == 0)		) {
            foundM	= m;
            break ;
        }
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(methods);

    return foundM;
}

XanList * IRPROGRAM_getMethodsWithName (JITINT8 *methodName) {
    XanList		*methods;
    XanList		*l;
    XanListItem	*item;

    /* Allocate the necessary memory.
     */
    l	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch all the methods.
     */
    methods	= ir_library->getIRMethods();
    assert(methods != NULL);

    /* Create the list.
     */
    item	= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*m;
        JITINT8		*currentName;
        m	= item->data;
        assert(m != NULL);
        currentName	= IRMETHOD_getMethodName(m);
        if (STRCMP(currentName, methodName) == 0) {
            xanList_append(l, m);
        }
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(methods);

    return l;
}

void IRPROGRAM_deleteDataFlowInformation (void) {
    XanList		*l;
    XanListItem	*item;

    /* Fetch the methods.
     */
    l	= ir_library->getIRMethods();

    /* Free the memory used to store circuit infomration
     */
    item	= xanList_first(l);
    while (item != NULL) {
        ir_method_t	*m;
        m	= item->data;
        IRMETHOD_deleteDataFlowInformation(m);
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(l);

    return ;
}

void IRPROGRAM_deleteCircuitsInformation (void) {
    XanList		*l;
    XanListItem	*item;

    /* Fetch the methods.
     */
    l	= ir_library->getIRMethods();

    /* Free the memory used to store circuit infomration
     */
    item	= xanList_first(l);
    while (item != NULL) {
        ir_method_t	*m;
        m	= item->data;
        IRMETHOD_deleteCircuitsInformation(m);
        if (m->preDominatorTree != NULL) {
            (m->preDominatorTree)->destroyTree(m->preDominatorTree);
            m->preDominatorTree 	= NULL;
        }
        if (m->postDominatorTree != NULL) {
            (m->postDominatorTree)->destroyTree(m->postDominatorTree);
            m->postDominatorTree 	= NULL;
        }

        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(l);

    return ;
}

XanList * IRPROGRAM_getIRMethods (void) {
    return ir_library->getIRMethods();
}

ir_method_t * IRPROGRAM_getEntryPointMethod (void) {
    ir_method_t     *entryPoint;

    /* Fetch the entry point	*/
    entryPoint = ir_library->getEntryPoint();
    assert(entryPoint != NULL);

    /* Return			*/
    return entryPoint;
}

JITINT8 * IRPROGRAM_getProgramName (void) {
    return ir_library->getProgramName();
}

XanList * IRPROGRAM_getCompatibleMethods (ir_signature_t *signature) {
    XanList *result;

    /* Fetch the List			*/
    if (signature != NULL) {
        result 	= ir_library->getCompatibleMethods(signature);

    } else {
        XanList		*l;
        XanListItem	*item;
        result	= xanList_new(allocFunction, freeFunction, NULL);
        l	= ir_library->getIRMethods();
        item	= xanList_first(l);
        while (item != NULL) {
            ir_method_t		*m;
            MethodDescriptor	*methodDesc;

            /* Fetch the method.
             */
            m		= item->data;
            methodDesc	= m->ID;

            /* Check the method.
             */
            if (	(methodDesc != NULL)			&&
                    (methodDesc->isCtor(methodDesc))	) {
                IR_ITEM_VALUE	methodID;
                methodID	= ir_library->getIRMethodID(m);
                if (methodID != 0) {
                    xanList_append(result, (void *)(JITNUINT)methodID);
                }
            }

            item	= item->next;
        }
        xanList_destroyList(l);
    }
    assert(result != NULL);

    return result;
}

XanList * IRPROGRAM_getImplementationsOfMethod (IR_ITEM_VALUE irMethodID) {
    XanList *result;

    /* Fetch the List			*/
    result = ir_library->getImplementationsOfMethod(irMethodID);
    assert(result != NULL);

    /* Return			*/
    return result;
}

JITBOOLEAN IRPROGRAM_doesMethodBelongToProgram (ir_method_t *method) {
    BasicAssembly           *assembly;
    t_binary_information    *binary;
    JITINT8                 *programName;

    /* Assertions.
     */
    assert(method != NULL);

    if (method->ID == NULL) {
        return JITFALSE;
    }

    assembly = method->ID->owner->getAssembly(method->ID->owner);
    assert(assembly != NULL);
    binary = assembly->binary;
    programName = ir_library->getProgramName();
    if (STRCMP(binary->name, programName) == 0) {
        return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRPROGRAM_doesMethodBelongToALibrary (ir_method_t *method) {
    BasicAssembly           *assembly;
    t_binary_information    *binary;
    JITINT8                 *programName;

    /* Assertions.
     */
    assert(method != NULL);

    if (method->ID == NULL) {
        return JITFALSE;
    }

    assembly = method->ID->owner->getAssembly(method->ID->owner);
    assert(assembly != NULL);
    binary = assembly->binary;
    programName = ir_library->getProgramName();
    if (STRCMP(binary->name, programName) != 0) {
        return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRPROGRAM_doesMethodBelongToTheBaseClassLibrary (ir_method_t *method) {
    BasicAssembly           *assembly;
    t_binary_information    *binary;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check whethere there is metadata.
     */
    if (method->ID == NULL) {
        return JITFALSE;
    }

    assembly = method->ID->owner->getAssembly(method->ID->owner);
    assert(assembly != NULL);
    binary = assembly->binary;
    if (STRCMP(binary->name, "mscorlib") == 0) {
        return JITTRUE;
    }

    return JITFALSE;
}

void IRPROGRAM_saveProgram (JITINT8 *directoryName) {
    ir_library->saveProgram(directoryName);
}

static inline void internal_check_and_add_escaped_method (ir_method_t *method, ir_item_t *par, XanList *l) {
    ir_method_t *ifunc;

    /* Assertions.
     */
    assert(par != NULL);
    assert(l != NULL);

    /* Initialize the variables.
     */
    ifunc   = NULL;

    /* Check whether is an entry point.
     */
    if (	(IRDATA_isAFunctionPointer(par))		||
            (par->internal_type == IRMETHODENTRYPOINT)	||
            (par->internal_type == IRFPOINTER)		) {
        void *entryPoint;
        ir_item_t itemID;
        if (par->type == IRSYMBOL) {
            ir_symbol_t     *sym;
            IR_ITEM_VALUE   methodID;
            sym         = IRSYMBOL_getSymbol(par);
            assert(sym != NULL);
            if (sym->tag == FUNCTION_POINTER_SYMBOL){
                methodID    = (IR_ITEM_VALUE)(JITNUINT)IRSYMBOL_unresolveSymbolFromIRItem(par);
                if (methodID != 0){
                    ifunc       = IRMETHOD_getIRMethodFromMethodID(method, methodID);
                }
            }

        } else {
            memcpy(&itemID, par, sizeof(ir_item_t));
            entryPoint = (void *)(JITNUINT)(itemID.value.v);
            if (entryPoint != NULL) {
                ifunc = IRMETHOD_getIRMethodFromEntryPoint(entryPoint);
            }
        }
        if (	(ifunc != NULL)			&&
                (xanList_find(l, ifunc) == NULL)	) {
            xanList_insert(l, ifunc);
        }

        return ;
    }

    /* Check whether is a method ID.
     */
    if (IRDATA_isAMethodID(par)) {
        IR_ITEM_VALUE	methodID;

        /* Fetch the method ID.
         */
        methodID	= IRDATA_getMethodIDFromItem(par);
        assert(methodID == 0);

        /* Fetch the method.
         */
        ifunc	= IRMETHOD_getIRMethodFromMethodID(method, methodID);

        /* Add the method.
         */
        if (	(ifunc != NULL)			&&
                (xanList_find(l, ifunc) == NULL)	) {
            xanList_insert(l, ifunc);
        }

        return ;
    }

    return ;
}

static inline void _IRPROGRAM_getReachableMethods (ir_method_t *startMethod, XanHashTable *l, XanHashTable *escapedMethods) {
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions.
     */
    assert(l != NULL);
    assert(startMethod != NULL);
    assert(escapedMethods != NULL);

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(startMethod);

    /* Fill up the list.
     */
    for (count = 0; count < instructionsNumber; count++) {
        XanList                 *tmpList;
        XanListItem             *item;
        ir_instruction_t        *inst;

        /* Fetch the next call instruction.
         */
        inst = IRMETHOD_getInstructionAtPosition(startMethod, count);
        assert(inst != NULL);
        if (!IRMETHOD_isACallInstruction(inst)) {
            continue;
        }

        /* Fetch the list of methods callable by the instruction.
         */
        tmpList = IRMETHOD_getCallableIRMethods(inst);
        assert(tmpList != NULL);

        /* Filter out the impossible callees.
         * Check whether the call is indirect.
         */
        if (xanList_length(tmpList) > 1) {
            XanList		*toDelete;

            /* The call is indrect.
             * Filter out the callees that are not escaped.
             */
            toDelete	= xanList_new(allocFunction, freeFunction, NULL);
            item 		= xanList_first(tmpList);
            while (item != NULL) {
                ir_method_t     *m;
                m = item->data;
                assert(m != NULL);
                if (xanHashTable_lookup(escapedMethods, m) == NULL) {
                    xanList_append(toDelete, m);
                }
                item = item->next;
            }
            xanList_removeElements(tmpList, toDelete, JITTRUE);
            //assert(xanList_length(tmpList) > 0);

            /* Free the memory.
             */
            xanList_destroyList(toDelete);
        }

        /* Add the callees.
         */
        item = xanList_first(tmpList);
        while (item != NULL) {
            ir_method_t     *m;
            m = item->data;
            assert(m != NULL);
            if (xanHashTable_lookup(l, m) == NULL) {
                xanHashTable_insert(l, m, m);
                _IRPROGRAM_getReachableMethods(m, l, escapedMethods);
            }
            item = item->next;
        }
        xanList_destroyList(tmpList);
    }

    return;
}

static inline JITBOOLEAN internal_IRPROGRAM_isInstructionTypeReachable (ir_method_t *startMethod, JITUINT16 instructionType, XanList *alreadyConsidered, XanList *escapedMethods, XanList *reachableMethodsFromEntryPoint) {
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    JITBOOLEAN found;

    /* Check if the method was already considered	*/
    if (xanList_find(alreadyConsidered, startMethod) != NULL) {
        return JITFALSE;
    }
    xanList_append(alreadyConsidered, startMethod);

    /* Check if the method has a IR body		*/
    if (!IRMETHOD_hasMethodInstructions(startMethod)) {
        return JITFALSE;
    }

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(startMethod);

    /* Analyze the method.
     */
    found = JITFALSE;
    for (count=0; (count < instructionsNumber) && (!found); count++) {
        ir_instruction_t 	*calledInst;
        XanList 		*calledMethods;
        XanListItem 		*item;

        /* Fetch the instruction.
         */
        calledInst = IRMETHOD_getInstructionAtPosition(startMethod, count);
        assert(calledInst != NULL);

        /* Check whether it is an instruction of the type requested.
         */
        if (calledInst->type == instructionType) {
            found	= JITTRUE;
            break;
        }

        /* Check whether the current instruction can jump to another method.
         */
        if (!IRMETHOD_isACallInstruction(calledInst)) {
            continue ;
        }

        /* Fetch the possible callees.
         */
        calledMethods = IRPROGRAM_getCallableMethods(calledInst, escapedMethods, reachableMethodsFromEntryPoint);
        assert(calledMethods != NULL);

        /* Check the callees.
         */
        item = xanList_first(calledMethods);
        while (item != NULL) {
            ir_method_t *calledMethod;
            IR_ITEM_VALUE calledMethodID;
            calledMethodID = (IR_ITEM_VALUE)(JITNUINT) item->data;
            if (calledMethodID != 0) {
                calledMethod = IRMETHOD_getIRMethodFromMethodID(startMethod, calledMethodID);
                assert(calledMethod != NULL);
                if (internal_IRPROGRAM_isInstructionTypeReachable(calledMethod, instructionType, alreadyConsidered, escapedMethods, reachableMethodsFromEntryPoint)) {
                    found = JITTRUE;
                    break ;
                }
            }
            item = item->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(calledMethods);
    }

    return found;
}

static inline void internal_addReachableMethods (XanList *methods, XanList *l) {
    XanListItem	*item;

    /* Assertions.
     */
    assert(methods != NULL);

    /* Add methods that can be called from outside the IR code.
     */
    item		= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*m;

        /* Fetch the method.
         */
        m		= item->data;
        assert(m != NULL);

        /* Check whether the method can be invoked outside the IR code.
         */
        if (xanList_find(l, m) == NULL) {
            XanList		*lC;
            XanListItem	*item2;

            /* Fetch methods reachable from the current method.
             */
            xanList_append(l, m);
            lC		= IRPROGRAM_getReachableMethods(m);
            assert(lC != NULL);

            /* Add methods that is reachable from the current method.
             */
            item2		= xanList_first(lC);
            while (item2 != NULL) {
                ir_method_t	*m2;
                m2		= item2->data;
                if (xanList_find(l, m2) == NULL) {
                    xanList_append(l, m2);
                }
                item2		= item2->next;
            }

            /* Free the memory.
             */
            xanList_destroyList(lC);
        }

        /* Fetch the next element from the list.
         */
        item		= item->next;
    }

    return ;
}
