/*
 * Copyright (C) 2008  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
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
#include <base_symbol.h>
#include <platform_API.h>

// My headers
#include <ilmethod.h>
#include <climanager_system.h>
// End

extern CLIManager_t *cliManager;

static inline void internal_destroyILMethod (Method method);
static inline JITUINT32 getRootSetTop (Method method);
static inline JITUINT32 getRootSetSlot (Method method, JITUINT32 slotID);
static inline void addVariableToRootSet (Method method, JITUINT32 varID);
static inline JITBOOLEAN isCctor (Method method);
static inline JITBOOLEAN isFinalizer (Method method);
static inline JITBOOLEAN isGeneric (Method method);
static inline JITBOOLEAN internal_requireConservativeTypeInitialization (Method self);
static inline JITBOOLEAN internal_isEntryPoint (Method self);
static inline JITBOOLEAN internal_isCtor (Method self);
static inline JITBOOLEAN internal_isStatic (Method self);
static inline JITINT32 internal_isCilImplemented (Method method);
static inline JITBOOLEAN internal_isIrImplemented (Method self);
static inline JITBOOLEAN internal_isRuntimeManaged (Method self);
static inline ir_signature_t * getSignature (Method method);
static inline TypeDescriptor *getType (Method method);
static inline BasicAssembly * getAssembly (Method method);
static inline JITUINT32 getState (Method method);
static inline JITUINT32 getGlobalState (Method method);
static inline JITINT8 * getName (Method method);
static inline JITUINT32 getMaxVariables (Method method);
static inline JITUINT32 getLocalsNumber (Method method);
static inline t_jit_function  *getJITFunction (Method method);
static inline void * getFunctionPointer (Method method);
static inline ir_symbol_t * getFunctionPointerSymbol (Method method);
static inline void setResultType (Method method, JITUINT32 value);
static inline JITUINT32 getParametersNumber (Method method);
static inline JITUINT32 getInstructionsNumber (Method method);
static inline TypeDescriptor *getParameterILType (Method method, JITINT32 parameterNumber);
static inline TypeDescriptor *getReturnILType (Method method);
static inline XanList *getLocals (Method method);
static inline JITUINT32 incMaxVariablesReturnOld (Method method);
static inline void increaseMaxVariables (Method method);
static inline ir_local_t *insertLocal (Method method, JITUINT32 internal_type, TypeDescriptor *type);
static inline void setState (Method method, JITUINT32 state);
static inline void setGlobalState (Method method, JITUINT32 gstate);
static inline void waitTillGlobalState(Method method, JITUINT32 gstate);
static inline void setID (Method method, MethodDescriptor *ID);
static inline XanList * internal_getTryBlocks (Method method);
static inline void internal_setTryBlocks (Method method, XanList *blocks);
static inline JITINT16 hasCatcher (Method method);
static inline ir_method_t * getIRMethod (Method method);
static inline void internal_unlock (Method method);
static inline void internal_lock (Method method);
static inline JITBOOLEAN insertAnHandler (Method method, t_try_block *block, t_try_handler *handler);
static inline ir_instruction_t * ir_instruction_insert (Method method);
static inline ir_instruction_t * ir_instruction_insert_before (Method method, ir_instruction_t * instr);
static inline JITBOOLEAN insertABlock_helpFunction (Method method, t_try_block *parent, t_try_block *block, t_try_handler *handler);
static inline t_try_block * findTheOwnerTryBlock (Method method, t_try_block *parent, JITUINT32 offset);
static inline t_try_handler * findOwnerHandler (t_try_block *parent_block, JITUINT32 offset);
static inline JITBOOLEAN isAnInnerTryBlock (t_try_block *parent, t_try_block *target);
static inline ir_instruction_t * getIRInstructionFromPC (Method method, JITNUINT pc);
static inline JITINT8 * internal_getFullName (Method method);

static inline void internal_lock (Method method) {

    /* Assertions			*/
    assert(method != NULL);

    PLATFORM_lockMutex(&(method->mutex));
}

static inline void internal_unlock (Method method) {

    /* Assertions			*/
    assert(method != NULL);

    PLATFORM_unlockMutex(&(method->mutex));
}

void addToTrampolinesSet (struct _ILMethod *method, struct _ILMethod *caller, ir_instruction_t *inst) {
    /* Assertions			*/
    assert(method != NULL);

    XanListItem *item = xanList_first(method->trampolinesSet);
    while (item != NULL) {
        Trampoline *trampoline = (Trampoline *) item->data;
        if (trampoline->caller == caller && trampoline->inst == inst) {
            return;
        }
        item = item->next;
    }
    Trampoline *newTrampoline = (Trampoline *)sharedAllocFunction(sizeof(Trampoline));
    newTrampoline->caller = caller;
    newTrampoline->inst = inst;
    xanList_append(method->trampolinesSet, newTrampoline);
}

XanList * getTrampolinesSet (struct _ILMethod *method) {

    /* Assertions			*/
    assert(method != NULL);

    return method->trampolinesSet;
}

static inline XanList * getCctorMethodsToCall (Method method) {

    /* Assertions			*/
    assert(method != NULL);

    return method->cctorMethods;
}

static inline void addCctorToCall (Method method, Method cctor) {

    /* Assertions			*/
    assert(method != NULL);
    assert(cctor != NULL);

    if (cctor != method) {
        JITBOOLEAN found = JITFALSE;
        XanListItem *item = xanList_first(method->cctorMethods);
        while (item != NULL) {
            Method currentCctor = (Method) item->data;
            assert(currentCctor != NULL);
            if (currentCctor == cctor) {
                found = JITTRUE;
                break;
            }
            item = item->next;
        }
        if (!found) {
            xanList_append(method->cctorMethods, cctor);
            assert(xanList_length(method->cctorMethods) > 0);
        }
    }

}
static inline ir_method_t * getIRMethod (Method method) {

    /* Assertions		*/
    assert(method != NULL);

    return &(method->IRMethod);
}

static inline JITUINT32 getRootSetTop (Method method) {
    JITUINT32 	top;

    top = 0;
    abort();

    return top;
}

static inline JITUINT32 getRootSetSlot (Method method, JITUINT32 slotID) {
    JITUINT32 slot;

    slot = 0;
    abort();

    return slot;
}

static inline void addVariableToRootSet (Method method, JITUINT32 varID) {
    //TODO

    return;
}

static inline JITBOOLEAN isAnonymous (Method method) {
    ir_method_t     *ir_method;
    MethodDescriptor        *methodID;

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    methodID = IRMETHOD_getMethodDescription(ir_method);

    return methodID == NULL;
}

static inline MethodDescriptor *getID (Method method) {
    ir_method_t     *ir_method;
    MethodDescriptor        *methodID;

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    methodID = IRMETHOD_getMethodDescription(ir_method);

    return methodID;
}

static inline ir_signature_t * getSignature (Method method) {
    ir_method_t     *ir_method;
    ir_signature_t  *signature;

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    signature = IRMETHOD_getSignature(ir_method);
    IRMETHOD_unlock(ir_method);

    return signature;
}

static inline TypeDescriptor * getType (Method method) {

    /* Assertions		*/
    assert(method != NULL);

    return ((method->IRMethod).ID)->owner;
}

static inline BasicAssembly * getAssembly (Method method) {
    BasicAssembly   *assembly;

    /* Assertions		*/
    assert(method != NULL);

    TypeDescriptor *owner = ((method->IRMethod).ID)->owner;
    assembly = owner->getAssembly(owner);

    return assembly;
}

static inline GenericContainer *getContainer (Method method) {
    MethodDescriptor *cilMethod = ((method->IRMethod).ID);

    return cilMethod->getResolvingContainer(cilMethod);
}

static inline t_binary_information *getBinary (Method method) {
    return (t_binary_information *) ((method->IRMethod).ID)->row->binary;
}

static inline JITUINT32 getState (Method method) {

    /* Assertions		*/
    assert(method != NULL);

    /* Return		*/
    return method->state;
}

static inline JITUINT32 getGlobalState (Method method) {

    /* Assertions		*/
    assert(method != NULL);

    return method->globalState;
}

static inline JITINT32 internal_isCilImplemented (Method method) {
    JITINT32 result;
    BasicMethodAttributes   *attributes;

    /* Assertions                           */
    assert(method != NULL);

    /* Initialize the variables		*/
    result = JITTRUE;

    /* Fetch the metadata			*/
    attributes = ((method->IRMethod).ID)->attributes;
    assert(attributes != NULL);

    /* Check the method                     */
    if (IRVM_isANativeFunction(cliManager->IRVM, *(method->jit_function))) {

        /* The method is internal			*/
        result = JITFALSE;
    } else if (attributes->is_abstract) {

        /* The method is abstract			*/
        result = JITFALSE;
    } else if (attributes->is_provided_by_the_runtime) {

        /* The method is provided by the run time       */
        result = JITFALSE;
    } else if (attributes->is_pinvoke) {

        /* The method is provided by the external library       */
        result = JITFALSE;
    }

    /* Return				*/
    return result;
}

/* Check if this method is implemented in IR language */
static inline JITBOOLEAN internal_isIrImplemented (Method self) {
    t_jit_function	*jitF;

    if (isAnonymous(self)) {
        return JITTRUE;
    }

    /* Check if the method is abstract	*/
    if (!IRMETHOD_hasIRMethodBody(&(self->IRMethod))) {
        return JITFALSE;
    }

    /* Check if the method is internal */
    jitF	= *(self->jit_function);
    if (	(jitF->data == NULL)					||
            (IRVM_isANativeFunction(cliManager->IRVM, jitF))	) {
        return JITFALSE;
    }

    MethodDescriptor *methodID;
    methodID	= getID(self);
    if (	(methodID != NULL)				&&
            (methodID->attributes->is_internal_call)	) {
        return JITFALSE;
    }

    /* This method has an IR body		*/
    return JITTRUE;
}

/* Check if this method is provided by the runtime */
static inline JITBOOLEAN internal_isRuntimeManaged (Method self) {

    /* Check if the method is provideg by the runtime */
    if (((self->IRMethod).ID)->attributes->is_provided_by_the_runtime == 1) {
        return JITTRUE;
    }

    /* This method isn't managed by runtime */
    return JITFALSE;

}

static inline JITBOOLEAN isCctor (Method method) {
    JITBOOLEAN result;

    /* Assertions                           */
    assert(method != NULL);

    if (isAnonymous(method)) {
        return JITFALSE;
    }

    /* Initialize the local variables       */

    MethodDescriptor *methodID = ((method->IRMethod).ID);
    result = methodID->isCctor(methodID);

    return result;
}

static inline JITBOOLEAN isFinalizer (Method method) {
    JITBOOLEAN result;

    /* Assertions                           */
    assert(method != NULL);

    if (isAnonymous(method)) {
        return JITFALSE;
    }

    /* Initialize the local variables       */

    MethodDescriptor *methodID = ((method->IRMethod).ID);
    result = methodID->isFinalizer(methodID);

    return result;
}

static inline JITBOOLEAN isGeneric (Method method) {
    JITBOOLEAN result;

    /* Assertions                           */
    assert(method != NULL);

    /* Initialize the local variables       */
    MethodDescriptor *methodID = ((method->IRMethod).ID);
    result = (methodID->attributes->param_count > 0);

    return result;
}

/* Whether self method call must trigger a conservative type initialization */
static inline JITBOOLEAN internal_requireConservativeTypeInitialization (Method self) {

    if (isAnonymous(self)) {
        return JITFALSE;
    }

    MethodDescriptor *method = (self->IRMethod).ID;

    return method->requireConservativeOwnerInitialization(method);
}

/* Whether self method is the application entry point */
static inline JITBOOLEAN internal_isEntryPoint (Method self) {

    /* Assetions			*/
    assert(self != NULL);
    assert(cliManager != NULL);

    if (self == (cliManager->entryPoint).method) {
        return JITTRUE;
    }

    return JITFALSE;
}

/* Whether self method is a .ctor */
static inline JITBOOLEAN internal_isCtor (Method self) {

    JITBOOLEAN result;

    /* Assertions                           */
    assert(self != NULL);

    if (isAnonymous(self)) {
        return JITFALSE;
    }

    /* Initialize the local variables       */

    MethodDescriptor *methodID = ((self->IRMethod).ID);
    result = methodID->isCtor(methodID);

    return result;

}

/* Whether self method is static */
static inline JITBOOLEAN internal_isStatic (Method self) {

    if (isAnonymous(self)) {
        return JITFALSE;
    }

    return ((self->IRMethod).ID)->attributes->is_static;

}

static inline XanList * internal_getTryBlocks (Method method) {
    XanList *blocks;

    /* Assertions	*/
    assert(method != NULL);

    blocks = method->try_blocks;

    return blocks;
}

static inline void internal_setTryBlocks (Method method, XanList *blocks) {

    /* Assertions	*/
    assert(method != NULL);

    method->try_blocks = blocks;
}

static inline JITINT8 * internal_getFullName (Method method) {
    JITINT8                     *name;
    ir_method_t             *irMethod;

    /* Assertions           */
    assert(method != NULL);

    /* Fetch the IR method	*/
    irMethod = getIRMethod(method);
    assert(irMethod != NULL);

    /* Fetch the name	*/
    name = (JITINT8 *) IRMETHOD_getCompleteMethodName(irMethod);
    assert(name != NULL);

    /* Return		*/
    return name;
}

static inline JITINT8 * getName (Method method) {
    ir_method_t     *ir_method;
    JITINT8         *name;

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    name = (JITINT8 *) IRMETHOD_getMethodName(ir_method);
    assert(name != NULL);
    IRMETHOD_unlock(ir_method);

    return name;
}

static inline JITUINT32 getMaxVariables (Method method) {
    ir_method_t     *ir_method;
    JITUINT32 max;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    max = IRMETHOD_getNumberOfVariablesNeededByMethod(ir_method);
    IRMETHOD_unlock(ir_method);

    return max;
}

static inline JITUINT32 getLocalsNumber (Method method) {
    ir_method_t     *ir_method;
    JITUINT32 num;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    num = IRMETHOD_getMethodLocalsNumber(ir_method);
    IRMETHOD_unlock(ir_method);

    return num;
}

static inline void * getFunctionPointer (Method method) {
    void    *entryPoint;
    XanHashTable *table;

    /* Assertions				*/
    assert(method != NULL);

    /* Cache some pointer	*/
    table = (cliManager->methods).functionsEntryPoint;

    if (IRVM_isANativeFunction(cliManager->IRVM, *(method->jit_function))) {
        entryPoint = IRVM_getEntryPoint(cliManager->IRVM, *(method->jit_function));
    } else {
        t_jit_function *jitFunction = getJITFunction(method);
        entryPoint = IRVM_getFunctionPointer(cliManager->IRVM, jitFunction);
    }

    xanHashTable_lock(table);
    if (xanHashTable_lookup(table, entryPoint) == NULL) {
        xanHashTable_insert(table, entryPoint, (void *) method);
    }
    xanHashTable_unlock(table);

    /* Return		*/
    return entryPoint;
}

ir_value_t functionPointerResolve (ir_symbol_t *symbol) {
    ir_value_t value;
    Method method;
    void *entryPoint;

    /* Initialize the variables				*/
    method = (Method) symbol->data;
    assert(method != NULL);

    /* Insert the entry_point in the hash table, so that we can find the jit function associated with it later on */
    entryPoint = getFunctionPointer(method);

    value.v = (JITNUINT) entryPoint;
    return value;
}

void functionPointerSerialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    Method 		method;
    JITUINT32	bytesWritten;

    /* Assertions.
     */
    assert(cliManager != NULL);

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);

    /* Fetch the method.
     */
    method = (Method) symbol->data;
    assert(method != NULL);

    /* Serialize the method.
     */
    ir_symbol_t *methodSymbol 	= IRSYMBOL_createSymbol(METHOD_SYMBOL, (void *) method);
    bytesWritten			= ILDJIT_writeIntegerValueInMemory(*mem, 0, *memBytesAllocated, methodSymbol->id, JITFALSE);
    (*memBytesAllocated)		= bytesWritten;

    return ;
}

void functionPointerDump (ir_symbol_t *symbol, FILE *fileToWrite) {
    Method method;

    /* Initialize the variables				*/
    method = (Method) symbol->data;
    assert(method != NULL);

    fprintf(fileToWrite, "Pointer to method %s", method->getFullName(method));

    return ;
}

ir_symbol_t * getFunctionPointerSymbol (Method method) {
    return IRSYMBOL_createSymbol(FUNCTION_POINTER_SYMBOL,  (void *) method);
}

ir_symbol_t *functionPointerDeserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 	methodSymbolID;

    /* Assertions.
     */
    assert(mem != NULL);
    assert(memBytes > 0);

    /* Deserialize the symbol ID.
     */
    ILDJIT_readIntegerValueFromMemory(mem, 0, &methodSymbolID);

    ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol(methodSymbolID);
    Method method = (Method) (JITNUINT) (IRSYMBOL_resolveSymbol(methodSymbol).v);

    return getFunctionPointerSymbol(method);
}

static inline t_jit_function * getJITFunction (Method method) {

    /* Assertions		*/
    assert(method != NULL);
    assert(cliManager != NULL);

    if ( *(method->jit_function) == NULL ) {
        cliManager->ensureCorrectJITFunction(method);
    }

    return *(method->jit_function);
}

static inline void setResultType (Method method, JITUINT32 value) {
    ir_method_t     *ir_method;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    IRMETHOD_lock(ir_method);
    IRMETHOD_setResultType(ir_method, value);
    IRMETHOD_unlock(ir_method);
}

static inline JITUINT32 getParametersNumber (Method method) {
    ir_method_t     *ir_method;
    JITUINT32 num;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    IRMETHOD_lock(ir_method);
    num = IRMETHOD_getMethodParametersNumber(ir_method);
    IRMETHOD_unlock(ir_method);

    return num;
}

static inline JITUINT32 getInstructionsNumber (Method method) {
    ir_method_t     *ir_method;
    JITUINT32 num;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    num = IRMETHOD_getInstructionsNumber(ir_method);
    IRMETHOD_unlock(ir_method);

    return num;
}

static inline XanList *getLocals (Method method) {
    XanList *locals;
    ir_method_t     *ir_method;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    locals = ir_method->locals;
    IRMETHOD_unlock(ir_method);

    /* Return */
    return locals;
}

/**
 * Increase the MaxVariables variable and return the non-increased value.
 * It works as if it were MaxVariables++ when used in an instruction like
 * a = method->incMaxVariablesReturnOld(method);
 * The value of MaxVariables for "method" is increase, and "a" gets the value it had before the increasing
 * @param method the name of the Method whose MaxVariables value will be returned and increased
 * @return the value MaxVariables had before being increased
 */
static inline JITUINT32 incMaxVariablesReturnOld (Method method) {
    ir_method_t     *ir_method;
    JITUINT32 old;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    old = IRMETHOD_getNumberOfVariablesNeededByMethod(ir_method);
    IRMETHOD_setNumberOfVariablesNeededByMethod(ir_method, old + 1);
    IRMETHOD_unlock(ir_method);

    return old;
}

/**
 * Increase the MaxVariables variable by 1.
 * @param method the name of the Method whose MaxVariables value will be increased
 */
static inline void increaseMaxVariables (Method method) {
    ir_method_t     *ir_method;
    JITUINT32 old;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    old = IRMETHOD_getNumberOfVariablesNeededByMethod(ir_method);
    IRMETHOD_setNumberOfVariablesNeededByMethod(ir_method, old + 1);
    IRMETHOD_unlock(ir_method);
}

static inline ir_local_t *insertLocal (Method method, JITUINT32 internal_type, TypeDescriptor *type) {
    ir_local_t *result;
    ir_method_t     *ir_method;

    /* Assertions				*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    result = IRMETHOD_insertLocalVariableToMethod(ir_method, internal_type, type);
    IRMETHOD_unlock(ir_method);

    /* Return */
    return result;
}

static inline void setState (Method method, JITUINT32 state) {

    /* Assertions		*/
    assert(method != NULL);

    /* set the new state */
    method->state = state;
}

static inline void setGlobalState (Method method, JITUINT32 gstate) {

    /* Assertions		*/
    assert(method != NULL);

    /* Set the new global state		*/
    method->globalState = gstate;
    pthread_cond_broadcast(&(method->notify));
}

static inline void waitTillGlobalState(Method method, JITUINT32 gstate) {

    /* Assertions		*/
    assert(method != NULL);
    assert(	(gstate == CIL_GSTATE)			||
            (gstate == CILIR_ONGOING_GSTATE)	||
            (gstate == DLA_PRE_GSTATE)		||
            (gstate == DLA_ONGOING_GSTATE)		||
            (gstate == IROPT_PRE_GSTATE)		||
            (gstate == IROPT_ONGOING_GSTATE)	||
            (gstate == IR_GSTATE));

    /* Wait until method reaches requested state	*/
    while ( method->globalState < gstate ) {
        pthread_cond_wait(&(method->notify), &(method->mutex));
    }
}

static inline void setID (Method method, MethodDescriptor *ID) {
    ir_method_t     *ir_method;

    /* Assertions		*/
    assert(method != NULL);
    assert(ID != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    IRMETHOD_setMethodDescriptor(ir_method, ID);
    IRMETHOD_unlock(ir_method);
}

static inline ir_instruction_t * ir_instruction_insert (Method method) {
    ir_method_t             *ir_method;
    ir_instruction_t        *instr;

    /* Assertions		*/
    assert(method != NULL);

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    instr = IRMETHOD_newInstruction(ir_method);
    assert(instr != NULL);
    IRMETHOD_unlock(ir_method);

    return instr;
}

static inline ir_instruction_t * ir_instruction_insert_before (Method method, ir_instruction_t * instr) {
    ir_method_t             *ir_method;
    ir_instruction_t        *instr2;

    ir_method = getIRMethod(method);
    assert(ir_method != NULL);
    IRMETHOD_lock(ir_method);
    instr2 = IRMETHOD_newInstructionBefore(ir_method, instr);
    assert(instr2 != NULL);
    IRMETHOD_unlock(ir_method);

    return instr2;
}

static inline JITBOOLEAN insertAnHandler (Method method, t_try_block *block, t_try_handler *handler) {
    JITBOOLEAN res;

    PDEBUG("SYSTEM_MANAGER: insertAnHandler\n");

    /* Assertions		*/
    assert(method != NULL);
    assert(block != NULL);
    assert(handler != NULL);

    res = insertABlock_helpFunction(method, NULL, block, handler);

    return res;
}

static inline t_try_block * findTheOwnerTryBlock (Method method, t_try_block *parent, JITUINT32 offset) {
    XanList         *blocks;
    XanListItem     *current_block;

    /* ASSERTIONS */
    assert(method != NULL);

    /* PRINT DEBUG INFORMATIONS */
#ifdef PRINTDEBUG
    if (parent != NULL) {
        PDEBUG("\n\nDEBUG:	parent %p \n", parent);
        PDEBUG("DEBUG:	parent begin_offset %d \n", parent->begin_offset);
        PDEBUG("DEBUG:	parent length %d \n", parent->length);
        PDEBUG("DEBUG:	parent inner_try_blocks %p \n", parent->inner_try_blocks);
        PDEBUG("DEBUG:	parent catch_handlers %p \n", parent->catch_handlers);
        PDEBUG("DEBUG:	parent finally_handlers %p \n", parent->finally_handlers);
        PDEBUG("DEBUG:	parent stack_to_restore %p \n", parent->stack_to_restore);
        PDEBUG("DEBUG:	parent parent_block %p \n", parent->parent_block);
        PDEBUG("DEBUG:	parent try_start_label_ID %d \n", parent->try_start_label_ID);
        PDEBUG("DEBUG:	parent try_end_label_ID %d \n", parent->try_end_label_ID);
        if (parent->inner_try_blocks != NULL) {
            PDEBUG("DEBUG:	parent inner_try_blocks - length %d \n\n\n", xanList_length(parent->inner_try_blocks));
        }
    }
#endif

    /* INITIALIZATION OF THE LOCAL VARIABLES */
    current_block = NULL;
    blocks = (parent == NULL) ?  internal_getTryBlocks(method) : parent->inner_try_blocks;

    /* TEST IF THERE AREN'T PROTECTED BLOCKS IN THIS METHOD */
    if (    (blocks == NULL)                ||
            (blocks->len) == 0              ) {
        return parent;
    }

    /* RETRIEVE THE FIRST BLOCK */
    current_block = xanList_first(blocks);
    assert(current_block != NULL);

    /* SEARCH FOR THE SMALLEST BLOCK THAT SURROUNDS THE OFFSET ADDRESS */
    while (current_block != NULL) {
        t_try_block *current_try_block;
        current_try_block = (t_try_block *) current_block->data;
        assert(current_try_block != NULL);
        assert(current_try_block != parent);

        if (    (current_try_block->begin_offset > offset)                                      ||
                ((current_try_block->begin_offset +  current_try_block->length) <= offset)      ) {

            /* BEFORE CONTINUE WITH THE RESEARCH, WE MUST VERIFY IF THE OFFSET IS A VALID
             * ADDRESS FOR A CATCH BLOCK ASSOCIATED WITH THE CURRENT TRY-BLOCK */
            if (current_try_block->catch_handlers != NULL) {
                XanListItem     *current_handler;
                t_try_handler   *current_try_handler;

                /* INITIALIZE THE LOCAL VARIABLES */
                current_handler = NULL;
                current_try_handler = NULL;

                current_handler = xanList_first(current_try_block->catch_handlers);
                while (current_handler != NULL) {
                    JITUINT32 begin_offset;

                    current_try_handler = (t_try_handler *) current_handler->data;
                    assert(current_try_handler != NULL);

                    begin_offset = (current_try_handler->type == FILTER_TYPE) ?  current_try_handler->filter_offset : current_try_handler->handler_begin_offset;
                    if (    begin_offset > offset
                            || ( (begin_offset + current_try_handler->handler_length)
                                 <= offset ) ) {
                        current_handler = current_handler->next;
                        continue;
                    }
                    return current_try_block;
                }
            }
            if (current_try_block->finally_handlers != NULL) {
                XanListItem     *current_handler;
                t_try_handler   *current_try_handler;

                /* INITIALIZE THE LOCAL VARIABLES */
                current_try_handler = NULL;
                current_handler = xanList_first(current_try_block->finally_handlers);
                while (current_handler != NULL) {
                    JITUINT32 begin_offset;

                    current_try_handler = (t_try_handler *) current_handler->data;
                    assert(current_try_handler != NULL);

                    begin_offset = current_try_handler->handler_begin_offset;

                    if (    begin_offset > offset
                            || ( (begin_offset + current_try_handler->handler_length)
                                 <= offset ) ) {
                        current_handler = current_handler->next;
                        continue;
                    }
                    return current_try_block;
                }
            }
        } else {
            return findTheOwnerTryBlock(method, current_try_block, offset);
        }

        current_block = current_block->next;
    }

    return parent;
}

static inline t_try_handler * findOwnerHandler (t_try_block *parent_block, JITUINT32 offset) {

    if (parent_block == NULL) {
        return NULL;
    }
    assert(!((parent_block->catch_handlers == NULL) && (parent_block->finally_handlers == NULL)));

    if (parent_block->catch_handlers != NULL) {
        XanListItem     *current_handler;
        t_try_handler   *current_try_handler;

        /* INITIALIZE THE LOCAL VARIABLES */
        current_handler = NULL;
        current_try_handler = NULL;

        current_handler = xanList_first(parent_block->catch_handlers);
        while (current_handler != NULL) {
            JITUINT32 begin_offset;

            current_try_handler = (t_try_handler *) current_handler->data;
            assert(current_try_handler != NULL);

            begin_offset = (current_try_handler->type == FILTER_TYPE) ?
                           current_try_handler->filter_offset
                           : current_try_handler->handler_begin_offset;

            if (    begin_offset > offset
                    || (    (begin_offset + current_try_handler->handler_length)
                            <= offset ) ) {
                current_handler = current_handler->next;
                continue;
            }
            return current_try_handler;
        }
    }
    if (parent_block->finally_handlers != NULL) {
        XanListItem     *current_handler;
        t_try_handler   *current_try_handler;

        /* INITIALIZE THE LOCAL VARIABLES */
        current_try_handler = NULL;
        current_handler = xanList_first(parent_block->finally_handlers);
        while (current_handler != NULL) {
            JITUINT32 begin_offset;

            current_try_handler = (t_try_handler *) current_handler->data;
            assert(current_try_handler != NULL);

            begin_offset = current_try_handler->handler_begin_offset;

            if (    begin_offset > offset
                    || ( (begin_offset + current_try_handler->handler_length)
                         <= offset ) ) {
                current_handler = current_handler->next;
                continue;
            }
            return current_try_handler;
        }
    }
    return NULL;
}

static inline JITBOOLEAN isAnInnerTryBlock (t_try_block *parent, t_try_block *target) {
    XanListItem             *current_inner_block;
    t_try_block             *current_inner_try_block;

    /*	ASSERTIONS */
    assert(parent != NULL);
    assert(target != NULL);

    /*	INITIALIZE THE LOCAL VARIABLES */
    current_inner_block = NULL;
    current_inner_try_block = NULL;

    if (parent == target) {
        return 1;
    }
    if (parent->inner_try_blocks == NULL) {
        return 0;
    }

    current_inner_block = xanList_first(parent->inner_try_blocks);
    assert(current_inner_block != NULL);

    while (current_inner_block != NULL) {
        current_inner_try_block = (t_try_block *) current_inner_block->data;
        assert(current_inner_try_block != NULL);

        if (isAnInnerTryBlock(current_inner_try_block, target) == 1) {
            return 1;
        }

        /* WE MUST CONTINUE THE RESEARCH */
        current_inner_block = current_inner_block->next;
    }

    return 0;
}

static inline JITINT16 hasCatcher (Method method) {
    JITINT16 result;

    /* Assertions	*/
    assert(method != NULL);

    result = (internal_getTryBlocks(method) != NULL);

    return result;
}

static inline ir_instruction_t * getIRInstructionFromPC (Method method, JITNUINT pc) {
    JITUINT32 instID;
    ir_method_t             *irMethod;
    ir_instruction_t        *inst;

    /* Assertion			*/
    assert(method != NULL);

    /* Initialize the variables	*/
    instID = 0;
    inst = NULL;
    irMethod = NULL;

    /* Fetch the IR Method		*/
    irMethod = getIRMethod(method);
    assert(irMethod != NULL);
    assert(cliManager != NULL);

    /* Take the instruction ID	*/
    //instID	= jit_function_get_offset_from_pc ((cliManager->IRVM).context, method->getJITFunction(method), (void *) pc);
    abort();
    if (instID >= IRMETHOD_getInstructionsNumber(irMethod)) {
        return NULL;
    }

    /* Take the IR instruction	*/
    inst = IRMETHOD_getInstructionAtPosition(irMethod, instID);
    assert(inst != NULL);

    /* Return			*/
    return inst;
}

static inline TypeDescriptor *getReturnILType (Method self) {
    TypeDescriptor *type;
    ir_method_t     *ir_method;

    ir_method = getIRMethod(self);
    assert(ir_method != NULL);
    type = IRMETHOD_getResultILType(ir_method);

    return type;
}

static inline void internal_destroyILMethod_empty (Method method) {
}

static inline void internal_destroyILMethod (Method method) {
    ir_method_t     *irMethod;

    /* Assertions			*/
    assert(method != NULL);
    assert(method->jit_function != NULL);
    assert((*(method->jit_function)) != NULL);

    /* Redirect the call to an empty*
     * method			*/
    method->destroyMethod = internal_destroyILMethod_empty;

    /* Fetch the IR method		*/
    irMethod = getIRMethod(method);
    assert(irMethod != NULL);

    /* Destroy the IR method	*/
    IRMETHOD_destroyMethod(irMethod);

    /* Sanity checks		*/
#ifdef DEBUG
    if ((cliManager->methods).container != NULL) {
        assert(xanList_find((cliManager->methods).container, method) == NULL);
    }
#endif

    /* Destroy the mutexes		*/
    PLATFORM_destroyMutex(&(method->mutex));

    /* Destroy the JIT information	*/
    IRVM_destroyFunction(cliManager->IRVM, *(method->jit_function));
    freeFunction(*(method->jit_function));
    freeFunction(method->jit_function);

    /* Free the cctor list		*/
    if (method->cctorMethods != NULL) {
        xanList_destroyList(method->cctorMethods);
    }

    /* Free the list of trampolines.
     */
    if (method->trampolinesSet != NULL) {
        xanList_destroyList(method->trampolinesSet);
    }

    /* Free the try blocks		*/
    if (method->try_blocks != NULL){
/*        XanListItem *item;
item = method->try_blocks->first(method->try_blocks);
while (item != NULL){
t_try_block *b;
b = item->data;
assert(b != NULL);
if (b->inner_try_blocks != NULL){
    b->inner_try_blocks->destroyList(b->inner_try_blocks);
}
if (b->catch_handlers != NULL){
    b->catch_handlers->destroyListAndData(b->catch_handlers);
}
if (b->finally_handlers != NULL){
    b->finally_handlers->destroyListAndData(b->finally_handlers);
}
if (b->stack_to_restore != NULL){
    freeMemory(b->stack_to_restore->stack);
    freeMemory(b->stack_to_restore);
}
item = item->next;
}*/
        //xanList_destroyListAndData(method->try_blocks);
    }

    /* Free the structure memory	*/
    freeFunction(method);

    /* Return			*/
    return;
}

static inline TypeDescriptor *getParameterILType (Method self, JITINT32 parameterNumber) {
    TypeDescriptor *type;
    ir_method_t     *ir_method;

    ir_method = getIRMethod(self);
    assert(ir_method != NULL);
    type = IRMETHOD_getParameterILType(ir_method, parameterNumber);

    return type;
}

static inline JITBOOLEAN insertABlock_helpFunction (Method method, t_try_block *parent, t_try_block *block, t_try_handler *handler) {
    t_try_block     *block_found;
    t_try_block     *current_block;
    XanList         *blocks;

    /*	INITIALIZE THE LOCALS */
    block_found = NULL;
    current_block = NULL;
    blocks = (parent != NULL) ? parent->inner_try_blocks : internal_getTryBlocks(method);

    /* assertions */
    assert(block != NULL);
    PDEBUG("SYSTEM_MANAGER.C : InsertABlock : begin...\n");
    PDEBUG("SYSTEM_MANAGER.C : InsertABlock : We have method=%p, parent=%p, block=%p, handler=%p\n", method, parent, block, handler);

    if (blocks != NULL) {
        XanListItem             *current_element;
        current_element = xanList_first(blocks);

        PDEBUG("SYSTEM_MANAGER.C : InsertABlock : starting the analysis of each blocks \n");

        while (current_element != NULL) {
            /*	CHECK IF THE CURRENT BLOCK IS THE CORRECT BLOCK */
            current_block = (t_try_block *) current_element->data;
            assert(current_block != NULL);

            /*	CASE : blocks begin at the same address */
            if (current_block->begin_offset == block->begin_offset ) {
                PDEBUG("InsertABlock : CASE : blocks begin at the same address \n");
                PDEBUG("InsertABlock : current_block->begin_offset == block->begin_offset \n");
                PDEBUG("InsertABlock : %d == %d \n", current_block->begin_offset, block->begin_offset);

                if (current_block->length == block->length ) {

                    /*	OFFSET & SIZE ARE EQUALS */
                    PDEBUG("InsertABlock : blocks have same dimension and both of them start at the same label \n");
                    PDEBUG("InsertABlock : current_block->length == block->length \n");
                    PDEBUG("InsertABlock : %d == %d \n", current_block->length, block->length);
                    block_found = current_block;
                    break;
                } else if (current_block->length < block->length ) {
                    XanListItem     *nextElement;

                    /* retrieve the next element */
                    nextElement = current_element->next;

                    /* We have to insert the current_block between
                     * the block's inner_try_blocks */
                    PDEBUG("InsertABlock : We have to insert the current_block between the block's inner_try_blocks \n");
                    PDEBUG("InsertABlock : current_block->length < block->length \n");
                    PDEBUG("InsertABlock : %d == %d \n", current_block->length, block->length);
                    if (parent != NULL) {
                        PDEBUG("InsertABlock : adding current_block to parent \n");
                        PDEBUG("DEBUG: BEFORE: length of parent->inner_try_blocks == %d \n", xanList_length(parent->inner_try_blocks));
                        PDEBUG("DEBUG: BEFORE: length of blocks == %d \n", xanList_length(blocks));
                        xanList_deleteItem(blocks, current_element);
                        parent->inner_try_blocks = blocks;
                        PDEBUG("DEBUG: AFTER: length of parent->inner_try_blocks == %d \n", xanList_length(parent->inner_try_blocks));
                    } else {
                        PDEBUG("InsertABlock : adding current_block to method \n");
                        PDEBUG("DEBUG: method->tryBlocks == %p, blocks == %p \n", method->getTryBlocks(method), blocks);
                        PDEBUG("DEBUG: BEFORE: length of method->tryBlocks == %d \n", xanList_length(method->getTryBlocks(method)));
                        PDEBUG("DEBUG: BEFORE: length of blocks == %d \n", xanList_length(blocks));
                        xanList_deleteItem(blocks, current_element);
                        internal_setTryBlocks(method, blocks);
                        blocks = internal_getTryBlocks(method);
                        PDEBUG("DEBUG: AFTER: length of method->tryBlocks == %d \n", xanList_length(method->getTryBlocks(method)));
                        PDEBUG("DEBUG: AFTER: length of blocks == %d \n", xanList_length(blocks));
                        PDEBUG("DEBUG: method->tryBlocks == %p, blocks == %p \n", method->getTryBlocks(method), blocks);
                    }

                    PDEBUG("InsertABlock : recall InsertABlock recursively using block=%p and c_block=%p and NULL \n", block, current_block);
                    insertABlock_helpFunction(method, block, current_block, NULL);
                    PDEBUG(" [...]a ----> RETURNING FROM A RECURSIVE CALL...\n");
                    PDEBUG(" [...]a ----> now the state is : block=%p, current_block=%p, handler=%p \n", block, current_block, handler);
                    current_element = nextElement;
                    continue;
                } else {
                    PDEBUG("InsertABlock : current_block->length > block->length");
                    PDEBUG("InsertABlock : recall and return InsertABlock recursively using current_block=%p and block=%p and handler=%p\n", current_block, block, handler);
                    /*	current_block->length > block->length */
                    return insertABlock_helpFunction(method, current_block, block, handler);
                }
            }
            /*	CASE : block is prior in the code.*/
            else if (current_block->begin_offset  >  block->begin_offset) {
                PDEBUG("InsertABlock : CASE : block comes prior in the code \n");

                /*	Blocks are disjointed */
                if (    current_block->begin_offset
                        >= (block->begin_offset + block->length)) {
                    PDEBUG("InsertABlock : blocks are disjointed \n");
                    PDEBUG("InsertABlock : c_block->b_offset=%d ; "
                           "b->b_offset+length=%d \n"
                           , current_block->begin_offset
                           , (block->begin_offset + block->length));

                    /*	UPDATE current_element TO THE NEXT ELEMENT */
                    current_element = current_element->next;
                    continue;
                }
                /*	The current_block is a sibling of block */
                else {
                    PDEBUG("InsertABlock : blocks are not disjointed "
                           "and current_block is a sibling of block... \n");
                    PDEBUG("InsertABlock : where current_block = %p "
                           "and block = %p \n", current_block, block);
                    PDEBUG("InsertABlock : We have to insert the "
                           "current_block between the block's inner_try_blocks \n");
                    /* We have to insert the current_block between
                     * the block's inner_try_blocks */
                    XanListItem *newElement = current_element->next;

                    if (parent != NULL) {
                        PDEBUG("InsertABlock : parent %p is not NULL "
                               "and so we delete the current_element from the "
                               "parent->inner_try_blocks list \n", parent);
                        xanList_deleteItem(blocks, current_element);
                        parent->inner_try_blocks = blocks;
                    } else {
                        PDEBUG("InsertABlock : parent %p is NULL "
                               "and so we delete the current_element from the "
                               "METHOD->try_blocks list\n", parent);
                        xanList_deleteItem(blocks, current_element);
                        internal_setTryBlocks(method, blocks);
                        blocks = internal_getTryBlocks(method);
                    }

                    PDEBUG("InsertABlock : finally we call recursively InsertABlock "
                           "on block=%p and current_block=%p and NULL\n"
                           , block, current_block);
                    insertABlock_helpFunction(method, block, current_block, NULL);
                    PDEBUG(" [...]b ----> RETURNING FROM A RECURSIVE CALL...\n");
                    PDEBUG(" [...]b ----> now the state is : block=%p, current_block=%p"
                           ", handler=%p\n", block, current_block, handler);
                    PDEBUG(" [...]b ----> passing to the new element ... newElement=%p\n"
                           , newElement);
                    current_element = newElement;
                    continue;
                }
            } else {
                PDEBUG("InsertABlock : CASE : block comes after in the code \n");
                /* CASE : block isn't prior in the code */
                /*	Blocks are disjointed */
                if (    block->begin_offset
                        >= (current_block->begin_offset + current_block->length)) {
                    PDEBUG("InsertABlock : blocks are disjointed \n");
                    PDEBUG("InsertABlock : block->begin_offset >= "
                           "(current_block->begin_offset + current_block->length) \n");
                    PDEBUG("InsertABlock : %d >= %d \n", block->begin_offset
                           , (current_block->begin_offset + current_block->length));
                    /*	UPDATE current_element TO THE NEXT ELEMENT */
                    current_element = current_element->next;
                    continue;
                }
                /*	block is a inner-try-block of current_block */
                else {
                    PDEBUG("InsertABlock : block is a inner-try-block of current_block \n");
                    PDEBUG("InsertABlock : finally we call and return recursively "
                           "InsertABlock on current_block=%p and block=%p and handler=%p\n"
                           , current_block, block, handler);
                    return insertABlock_helpFunction(method, current_block, block, handler);
                }
            }
        }
    }

    /*	NO MATCHING BLOCK FOUND */
    if (block_found == NULL) {
        PDEBUG("InsertABlock : NO MATCHING BLOCK FOUND \n");
        PDEBUG("SYSTEM_MANAGER.C : InsertABlock : THIS BLOCK ISN'T CONTAINED IN ANY OTHER INNER-BLOCK OF parent \n");

        /*	THIS BLOCK ISN'T CONTAINED IN ANY OTHER INNER-BLOCK OF parent */
        if (parent != NULL) {
            if (parent->inner_try_blocks == NULL) {
                parent->inner_try_blocks = xanList_new(allocFunction, freeFunction, NULL);
            }
            xanList_append(parent->inner_try_blocks, block);
        } else {
            XanList *tempList;
            tempList = internal_getTryBlocks(method);
            if (tempList == NULL) {
                tempList = xanList_new(allocFunction, freeFunction, NULL);
            }
            assert(tempList != NULL);
            xanList_append(tempList, block);
            internal_setTryBlocks(method, tempList);
        }

        block->parent_block = parent;
        block_found = block;
    }
    if (handler != NULL) {
        /* preconditions */
        assert(block_found != NULL);
        if (block_found != block) {
            /* update the handler informations */
            handler->owner_block = block_found;
        }

        if (            handler->type == EXCEPTION_TYPE
                        ||      handler->type == FILTER_TYPE ) {
            PDEBUG("SYSTEM_MANAGER: insertABlock : Found a Catch_or_filter block for this handler \n");
            if (block_found->catch_handlers == NULL) {
                block_found->catch_handlers = xanList_new(allocFunction, freeFunction, NULL);
            }
            xanList_append(block_found->catch_handlers, handler);
        } else {
            PDEBUG("SYSTEM_MANAGER: insertABlock : Found a Finally_or_fault block for this handler \n");
            if (block_found->finally_handlers == NULL) {
                block_found->finally_handlers = xanList_new(allocFunction, freeFunction, NULL);
            }

            xanList_append(block_found->finally_handlers, handler);
        }
    }

    PDEBUG("SYSTEM_MANAGER: insertABlock ----- returning!\n");
    return 1;
}

void initializeMethod (Method self) {

    /* Allocate memory for the jit function		*/
    self->jit_function = MEMORY_obtainPrivateAddress();
    assert(*(self->jit_function) == NULL);
    *(self->jit_function) = allocFunction(sizeof(t_jit_function));

    /* Initialize the function pointers		*/
    self->getIRMethod = getIRMethod;
    self->getRootSetTop = getRootSetTop;
    self->getRootSetSlot = getRootSetSlot;
    self->addVariableToRootSet = addVariableToRootSet;
    self->getCctorMethodsToCall = getCctorMethodsToCall;
    self->addCctorToCall = addCctorToCall;
    self->getID = getID;
    self->isAnonymous = isAnonymous;
    self->getSignature = getSignature;
    self->destroyMethod = internal_destroyILMethod;
    self->getType = getType;
    self->getAssembly = getAssembly;
    self->getState = getState;
    self->getGlobalState = getGlobalState;
    self->getName = getName;
    self->getMaxVariables = getMaxVariables;
    self->getLocalsNumber = getLocalsNumber;
    self->getJITFunction = getJITFunction;
    self->getFunctionPointer = getFunctionPointer;
    self->getFunctionPointerSymbol = getFunctionPointerSymbol;
    self->setResultType = setResultType;
    self->getParametersNumber = getParametersNumber;
    self->getParameterILType = getParameterILType;
    self->getReturnILType = getReturnILType;
    self->getIRInstructionFromPC = getIRInstructionFromPC;
    self->getInstructionsNumber = getInstructionsNumber;
    self->getLocals = getLocals;
    self->incMaxVariablesReturnOld = incMaxVariablesReturnOld;
    self->increaseMaxVariables = increaseMaxVariables;
    self->insertLocal = insertLocal;
    self->setState = setState;
    self->setGlobalState = setGlobalState;
    self->waitTillGlobalState = waitTillGlobalState;
    self->setID = setID;
    self->newIRInstr = ir_instruction_insert;
    self->newIRInstrBefore = ir_instruction_insert_before;
    self->insertAnHandler = insertAnHandler;
    self->findTheOwnerTryBlock = findTheOwnerTryBlock;
    self->findOwnerHandler = findOwnerHandler;
    self->isAnInnerTryBlock = isAnInnerTryBlock;
    self->hasCatcher = hasCatcher;
    self->getTryBlocks = internal_getTryBlocks;
    self->setTryBlocks = internal_setTryBlocks;
    self->isCctor = isCctor;
    self->isFinalizer = isFinalizer;
    self->isGeneric = isGeneric;
    self->requireConservativeTypeInitialization = internal_requireConservativeTypeInitialization;
    self->isEntryPoint = internal_isEntryPoint;
    self->isCtor = internal_isCtor;
    self->isStatic = internal_isStatic;
    self->isCilImplemented = internal_isCilImplemented;
    self->isIrImplemented = internal_isIrImplemented;
    self->isRuntimeManaged = internal_isRuntimeManaged;
    self->lock = internal_lock;
    self->unlock = internal_unlock;
    self->getFullName = internal_getFullName;
    self->getContainer = getContainer;
    self->getBinary = getBinary;
    self->addToTrampolinesSet = addToTrampolinesSet;
    self->getTrampolinesSet = getTrampolinesSet;
}
