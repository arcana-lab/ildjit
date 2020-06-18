/*
 * Copyright (C) 2012  Simone Campanoni
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
#include <stdlib.h>
#include <ir_optimization_interface.h>
#include <ilbinary.h>
#include <base_symbol.h>

// My headers
#include <clr_manager.h>
#include <iljit_plugin_manager.h>
#include <garbage_collector_interactions.h>
#include <system_manager.h>
#include <code_generator.h>
#include <runtime.h>
// End

typedef struct {
    ir_symbol_t	*sym;
    ir_global_t	*gloType;
    ir_global_t	*gloIndexType;
    ir_global_t	*gloUserString;
    ir_global_t	*gloField;
    ir_global_t	*gloArray;
    ir_global_t	*gloCLRType;
    ir_global_t	*gloIlTypeArray;
    ir_global_t	*gloSerializedSym;
    JITBOOLEAN	translateGlobals;
} symbol_globals_t;

static inline void internal_layout_types (t_system *system);
static inline void internal_addDependentCCTORS (ir_method_t *m, XanList *deps, XanHashTable *alreadyChecked);
static inline XanList * internal_sortCctors (XanList *cctors);
static inline void internal_addCallsToBootstrap (ir_method_t *m);
static inline t_plugins * internal_fetchDecoder (XanList *plugins, t_binary_information *b);
static inline ir_instruction_t * internal_storeValueToGlobal (ir_method_t *m, ir_instruction_t *afterInst, ir_global_t *glo, ir_item_t *irValueToStore);
static inline void internal_substituteSymbolsWithGlobals (ir_method_t *method, ir_instruction_t *i, ir_item_t *par, XanHashTable *symbolGlobalMap, XanHashTable *globalILTypeMap, XanHashTable *st);

extern t_system *ildjitSystem;

Method CODE_generateWrapperOfEntryPoint (CodeGenerator *self) {
    TypeDescriptor          *stringClassLocated;
    Method			ilWrapper;
    ir_method_t             *entryPoint;
    ir_method_t		*wrapper;
    ir_instruction_t	*notifyArgsInst;
    ir_instruction_t	*lastInst;
    ir_instruction_t	*callInst;
    ir_instruction_t	*freeInst;
    ir_instruction_t	*retInst;
    ir_item_t		argcIRItem;
    ir_item_t		argvIRItem;
    ir_item_t		arrayAllocatedIRItem;
    ir_item_t		returnItem;
    ir_item_t		*callReturnItem;
    XanList			*mains;
    XanListItem		*item;

    /* Assertions.
     */
    assert(self != NULL);

    /* Initialize variables.
     */
    ilWrapper	= NULL;
    wrapper		= NULL;

    /* Check whether we have already generated the wrapper or not.
     */
    if ((ildjitSystem->program).wrapperEntryPoint != NULL) {
        return (ildjitSystem->program).wrapperEntryPoint;
    }

    /* Check whether the wrapper has been generated in a previous compilation.
     * In this case, we need to destroy it because it contains pointers of previous runs.
     */
    mains	= IRPROGRAM_getMethodsWithName((JITINT8 *)"main");
    item	= xanList_first(mains);
    while (item != NULL) {
        ir_method_t *m;
        m 	= item->data;
        assert(STRCMP(IRMETHOD_getMethodName(m), "main") == 0);
        if (IRMETHOD_getSignatureInString(m) == NULL) {
            ilWrapper	= (Method)(JITNUINT)IRMETHOD_getMethodID(m);
            assert(ilWrapper != NULL);
            break ;
        }
        item     = item->next;
    }
    xanList_destroyList(mains);
    if (ilWrapper != NULL) {

        /* The wrapper method has been already created.
         * However, this means that it has been created by a previous run, which dumped the IR we have just loaded.
         * Therefore, we must erase the method and restart making it because it contains run-specific pointers in it.
         *
         * Notice that we cannot destroy the method and create a new one because the backend (e.g., LLVM) may assign to it a different symbol because the previous one has been already used (i.e., main43 instead of main).
         * A different symbol may lead to problems to the linker which cannot find pre-defined ones (e.g., main).
         */
        wrapper		= ilWrapper->getIRMethod(ilWrapper);
        IRMETHOD_deleteInstructions(wrapper);
        IRMETHOD_addMethodDummyInstructions(wrapper);
    }

    /* Fetch the entry point.
     */
    entryPoint			= IRPROGRAM_getEntryPointMethod();
    assert(entryPoint != NULL);
    assert(IRMETHOD_getMethodParametersNumber(entryPoint) == 1);

    /* Fetch the string data type.
     */
    stringClassLocated		= (ildjitSystem->cliManager).CLR.stringManager.fillILStringType();
    assert(stringClassLocated != NULL);

    /* Generate the IR symbol file for the string data type.
     */
    (ildjitSystem->cliManager).translator.getTypeDescriptorSymbol(&(ildjitSystem->cliManager), stringClassLocated);

    /* Create the wrapper.
     */
    if (wrapper == NULL) {
        JITINT8			*wrapperName;
        wrapperName			= (JITINT8 *)strdup("main");
        wrapper				= IRMETHOD_newMethod(wrapperName);
    }
    assert(wrapper != NULL);
    ilWrapper			= (Method) (JITNUINT)IRMETHOD_getMethodID(wrapper);
    assert(ilWrapper != NULL);

    /* Set the signature.
     */
    IRMETHOD_initSignature(&(wrapper->signature), 2);
    IRMETHOD_setParameterType(wrapper, 0, IRINT32);
    IRMETHOD_setParameterType(wrapper, 1, IRMPOINTER);
    IRMETHOD_setResultType(wrapper, IRINT32);
    IRMETHOD_addMethodDummyInstructions(wrapper);

    /* Define the return value.
     */
    memset(&returnItem, 0, sizeof(ir_item_t));
    if (((entryPoint->signature).resultType) == IRVOID) {
        returnItem.value.v		= 0;
        returnItem.internal_type	= IRINT32;
        returnItem.type			= returnItem.internal_type;
        callReturnItem			= NULL;

    } else {
        IRMETHOD_newVariable(wrapper, &returnItem, IRINT32, NULL);
        callReturnItem			= &returnItem;
    }

    /* Notify the input arguments to the system.
     */
    IRMETHOD_newVariable(wrapper, &arrayAllocatedIRItem, IRNUINT, NULL);
    notifyArgsInst		= IRMETHOD_newNativeCallInstruction(wrapper, "RUNTIME_setArguments", RUNTIME_setArguments, &arrayAllocatedIRItem, NULL);
    memset(&argcIRItem, 0, sizeof(ir_item_t));
    memset(&argvIRItem, 0, sizeof(ir_item_t));
    argcIRItem.value.v		= 0;
    argcIRItem.type			= IROFFSET;
    argcIRItem.internal_type	= IRINT32;
    argvIRItem.value.v		= 1;
    argvIRItem.type			= IROFFSET;
    argvIRItem.internal_type	= IRNUINT;
    IRMETHOD_addInstructionCallParameter(wrapper, notifyArgsInst, &argcIRItem);
    IRMETHOD_addInstructionCallParameter(wrapper, notifyArgsInst, &argvIRItem);
    lastInst		= notifyArgsInst;

    /* Invoke the actual entry point of the program.
     */
    callInst		= IRMETHOD_newCallInstructionAfter(wrapper, entryPoint, lastInst, callReturnItem, NULL);
    IRMETHOD_addInstructionCallParameter(wrapper, callInst, &arrayAllocatedIRItem);
    lastInst		= callInst;

    /* Free the allocated array.
     */
    freeInst		= IRMETHOD_newInstructionOfTypeAfter(wrapper, lastInst, IRFREEOBJ);
    IRMETHOD_cpInstructionParameter1(freeInst, &arrayAllocatedIRItem);
    lastInst		= freeInst;

    /* Return the value.
     */
    retInst			= IRMETHOD_newInstructionOfTypeAfter(wrapper, lastInst, IRRET);
    IRMETHOD_cpInstructionParameter1(retInst, &returnItem);

    /* Check whether we have to bootstrap the system before executing anything.
     */
    if ((ildjitSystem->IRVM).behavior.staticCompilation) {

        /* Initialize the system.
         */
        internal_addCallsToBootstrap(wrapper);
    }

    return ilWrapper;
}

void CODE_generateMachineCode (CodeGenerator *self, Method method) {
    ir_method_t			*irMethod;
    t_jit_function                  *jitFunction;

    /* Fetch the IR method					*/
    irMethod = method->getIRMethod(method);
    assert(irMethod != NULL);

    /* Fetch the JIT function				*/
    jitFunction = method->getJITFunction(method);
    assert(jitFunction != NULL);

    /* Insert the tracer					*/
    if ((ildjitSystem->IRVM).behavior.tracer) {
        JITBOOLEAN 		traceEnabled;

        /* Check the tracer options.
         */
        traceEnabled	= JITFALSE;
        switch ((ildjitSystem->IRVM).behavior.tracerOptions) {
            case 0:
                traceEnabled	= JITTRUE;
                break;
            case 1:
                traceEnabled 	= !IRPROGRAM_doesMethodBelongToALibrary(irMethod);
                break;
            case 2:
                traceEnabled 	= IRPROGRAM_doesMethodBelongToALibrary(irMethod);
                break;
            case 3:
                break;
        }
        if (traceEnabled) {
            ir_instruction_t		*firstInstruction;

            /* Fetch the first instruction				*/
            firstInstruction	= IRMETHOD_getFirstInstruction(irMethod);

            /* Check if we need to inject code			*/
            if (	(firstInstruction->type != IRNATIVECALL)						||
                    ((firstInstruction->param_3).value.v != (IR_ITEM_VALUE)(JITNUINT)tracer_startMethod)	) {
                XanList				*callParameters;
                XanList				*lastInstructions;
                XanListItem			*item;
                ir_item_t			param;

                /* Fetch the last instructions				*/
                lastInstructions	= IRMETHOD_getInstructionsOfType(irMethod, IRRET);

                /* Prepare the parameters				*/
                memset(&param, 0, sizeof(ir_item_t));
                callParameters		= xanList_new(allocFunction, freeFunction, NULL);
                xanList_append(callParameters, &param);
                param.type		= IRNUINT;
                param.internal_type	= param.type;
                param.value.v		= (IR_ITEM_VALUE)(JITNUINT)irMethod;

                /* Inject code 						*/
                IRMETHOD_newNativeCallInstructionBefore(irMethod, firstInstruction, "tracer_startMethod", tracer_startMethod, NULL, callParameters);
                item			= xanList_first(lastInstructions);
                while (item != NULL) {
                    ir_instruction_t	*inst;
                    inst	= item->data;
                    IRMETHOD_newNativeCallInstructionBefore(irMethod, inst, "tracer_exitMethod", tracer_exitMethod, NULL, callParameters);
                    item	= item->next;
                }

                /* Free the memory					*/
                xanList_destroyList(callParameters);
                xanList_destroyList(lastInstructions);
            }
        }
    }

    /* Check if we need to postpone the generation of machine code of the method or generate the machine code right now.
     */
    if (self->cacheCodeGeneration) {

        /* Cache the method.
         */
        if (xanList_find(self->cachedMethods, method) == NULL) {
            xanList_append(self->cachedMethods, method);
        }

    } else {

        /* Check whether we can use the backend.
         */
        if ((ildjitSystem->program).enableMachineCodeGeneration) {

            /* Translate the method into the backend-specific intermediate representation.
             */
            IRVM_lowerMethod(&(ildjitSystem->IRVM), irMethod, jitFunction);
        }
    }

    return ;
}

void CODE_linkMethodToProgram (CodeGenerator *self, Method method) {

    /* Check if we need to link method to the program right now.
     */
    if (self->cacheCodeGeneration) {

        /* Cache the method.
         */
        if (xanList_find(self->cachedMethods, method) == NULL) {
            xanList_append(self->cachedMethods, method);
        }

    } else {

        /* Link the method to the program.
         */
        if ((ildjitSystem->program).enableMachineCodeGeneration) {
            IRVM_generateMachineCode(&(ildjitSystem->IRVM), *(method->jit_function), method->getIRMethod(method));
        }
    }

    return ;
}

void CODE_init (CodeGenerator *self) {
    self->cacheCodeGeneration	= JITFALSE;
    self->cachedMethods		= xanList_new(allocFunction, freeFunction, NULL);

    return ;
}

void CODE_shutdown (CodeGenerator *self) {

    if (self->cachedMethods != NULL) {
        xanList_destroyList(self->cachedMethods);
        self->cachedMethods	= NULL;
    }

    return ;
}

void CODE_cacheCodeGeneration (CodeGenerator *self) {
    self->cacheCodeGeneration	= JITTRUE;
}

void CODE_generateAndLinkCodeForCachedMethods (CodeGenerator *self) {
    XanListItem			*item;
    XanList				*cctors;
    XanList				*newCctors;
    ILLayout_manager		*layoutManager;

    /* Check if we are caching methods.
     */
    if (!self->cacheCodeGeneration) {
        return ;
    }

    /* Unset the cache condition.
     */
    self->cacheCodeGeneration	= JITFALSE;

    /* Get layout manager.
     */
    layoutManager			= &((ildjitSystem->cliManager).layoutManager);

    /* Reset the static memory manager in order to reconsider cctors.
     * This is necessary because no cctor has been executed yet, but methods went through the compilation pipeline, which modify the static memory manager.
     * Before resetting, we clone the list of cctors to invoke in the order specified in the list. This list will be used later to finally invoke them.
     */
    cctors		= xanList_cloneList((ildjitSystem->staticMemoryManager).cachedConstructors);
    ildjitSystem->staticMemoryManager.cacheConstructors	= JITFALSE;
    xanList_emptyOutList((ildjitSystem->staticMemoryManager).cachedConstructors);
    xanHashTable_deleteAndFreeElements((ildjitSystem->staticMemoryManager).cctorMethodsExecuted);

    /* Sort the cctors to call.
     */
    newCctors	= internal_sortCctors(cctors);

    /* Handle cctors.
     */
    if ((ildjitSystem->IRVM).behavior.staticCompilation) {
        XanHashTable 		*st;
        ir_instruction_t	*placeToInsertCall;
        ir_instruction_t	*lastInst;
        ir_method_t		*wrapperIRMethod;
        Method			wrapper;

        /* Fetch the symbol table for global static memories.
         */
        st		= IRSYMBOL_getSymbolTableForStaticMemories();
        assert(st != NULL);

        /* Fetch the entry point of the program.
         */
        wrapper			= (ildjitSystem->program).wrapperEntryPoint;
        assert(wrapper != NULL);
        wrapperIRMethod		= wrapper->getIRMethod(wrapper);
        assert(wrapperIRMethod != NULL);

        /* Fetch where to insert the calls to cctors.
         */
        placeToInsertCall	= IRMETHOD_getFirstInstructionNotOfType(wrapperIRMethod, IRNOP);
        while (placeToInsertCall != NULL) {
            if (placeToInsertCall->type == IRNATIVECALL) {
                void	*fp;
                fp	= IRMETHOD_getCalledNativeMethod(placeToInsertCall);
                if (fp == RUNTIME_setArguments) {
                    break ;
                }
            }
            placeToInsertCall	= IRMETHOD_getNextInstruction(wrapperIRMethod, placeToInsertCall);
        }
        assert(placeToInsertCall != NULL);
        lastInst		= placeToInsertCall;

        /* Insert calls to cctors at the beginning of the entry point method.
         */
        item			= xanList_first(newCctors);
        while (item != NULL) {
            Method			cctor;
            TypeDescriptor		*cctorType;
            ir_method_t		*irCctor;
            ir_instruction_t	*pointerOfCctorObjectInst;
            ir_item_t		cctorObject;
            ir_item_t		*cctorObjectPointer;
            ir_symbol_t		*cctorObjectSymbol;
            ir_static_memory_t	*info;

            /* Fetch the cctor.
             */
            cctor				= item->data;
            irCctor				= cctor->getIRMethod(cctor);
            assert(irCctor != NULL);

            /* Fetch the symbol for the cctor object.
             */
            memset(&cctorObject, 0, sizeof(ir_item_t));
            cctorType			= cctor->getType(cctor);
            assert(cctorType != NULL);
            cctorObjectSymbol		= STATICMEMORY_fetchStaticObjectSymbol(&(ildjitSystem->staticMemoryManager), wrapper, cctorType);
            assert(cctorObjectSymbol != NULL);

            /* Fetch the global.
             */
            info				= xanHashTable_lookup(st, cctorObjectSymbol);
            assert(info != NULL);
            assert(info->global != NULL);
            IRGLOBAL_storeGlobalToIRItem(&cctorObject, info->global, NULL);

            /* Fetch the address of the static object.
             */
            pointerOfCctorObjectInst	= IRMETHOD_newInstructionOfTypeAfter(wrapperIRMethod, lastInst, IRGETADDRESS);
            IRMETHOD_cpInstructionParameter1(pointerOfCctorObjectInst, &cctorObject);
            IRMETHOD_setInstructionParameterWithANewVariable(wrapperIRMethod, pointerOfCctorObjectInst, IRNUINT, NULL, 0);
            cctorObjectPointer		= IRMETHOD_getInstructionResult(pointerOfCctorObjectInst);
            lastInst			= pointerOfCctorObjectInst;

            /* Insert the call.
             */
            placeToInsertCall		= IRMETHOD_newCallInstructionAfter(wrapperIRMethod, irCctor, lastInst, NULL, NULL);
            IRMETHOD_addInstructionCallParameter(wrapperIRMethod, placeToInsertCall, cctorObjectPointer);
            lastInst			= placeToInsertCall;

            /* Fetch the next element.
             */
            item		= item->next;
        }
    }

    /* Each method is now available in IR.
     */
    item	= xanList_first(self->cachedMethods);
    while (item != NULL) {
        Method		m;
        m	= item->data;
        m->setState(m, IR_STATE);
        item	= item->next;
    }

    /* Generate the machine code for every directly referenced methods.
     */
    item	= xanList_first(self->cachedMethods);
    while (item != NULL) {
        Method		m;

        /* Fetch the method.
         */
        m	= item->data;
        assert(m != NULL);

        /* Generate the machine code.
         */
        if (IRMETHOD_doesMethodHaveItsEntryPointDirectlyReferenced(m->getIRMethod(m))) {
            CODE_generateMachineCode(self, m);
            CODE_linkMethodToProgram(self, m);
            m->setState(m, EXECUTABLE_STATE);
        }

        /* Fetch the next element.
         */
        item	= item->next;
    }

    /* Generate the IRVM intermediate representation.
     */
    item	= xanList_first(self->cachedMethods);
    while (item != NULL) {
        Method		m;

        /* Fetch the method.
         */
        m	= item->data;
        assert(m != NULL);
        assert(xanList_equalsInstancesNumber(self->cachedMethods, m) == 1);

        /* Generate the machine code.
         */
        if (m->getState(m) == IR_STATE) {
            CODE_generateMachineCode(self, m);
            m->setState(m, MACHINE_CODE_STATE);
        }

        /* Fetch the next element.
         */
        item	= item->next;
    }

    /* Optimize the entire program.
     */
    IRVM_optimizeProgram(&(ildjitSystem->IRVM));

    /* Link the machine code previously generated all together.
     */
    item	= xanList_first(self->cachedMethods);
    while (item != NULL) {
        Method		m;

        /* Fetch the method.
         */
        m	= item->data;
        assert(m != NULL);

        /* Link the method.
         */
        if (m->getState(m) == MACHINE_CODE_STATE) {
            CODE_linkMethodToProgram(self, m);
            m->setState(m, EXECUTABLE_STATE);
        }

        /* Fetch the next element.
         */
        item	= item->next;
    }
#ifdef DEBUG
    item	= xanList_first(self->cachedMethods);
    while (item != NULL) {
        Method		m;
        m	= item->data;
        assert(m != NULL);
        assert(m->getState(m) == EXECUTABLE_STATE);
        item	= item->next;
    }
#endif

    /* Compile the wrapper of the entry point of the program.
     */
    CODE_generateMachineCode(self, (ildjitSystem->program).wrapperEntryPoint);
    CODE_linkMethodToProgram(self, (ildjitSystem->program).wrapperEntryPoint);

    /* Disable the backend code generation.
     */
    CODE_cacheCodeGeneration(self);

    /* Handle cctors.
     */
    if (!(ildjitSystem->IRVM).behavior.staticCompilation) {

        /* Layout the types.
         * Everything is available in machine code at this point.
         * Hence, we can compute all virtual tables.
         */
        internal_layout_types(ildjitSystem);

        /* Create the symbols.
         */
        item			= xanList_first(layoutManager->classesLayout);
        while (item != NULL) {
            ILLayout		*layout;
            layout			= item->data;
            assert(layout != NULL);
            assert(layout->type != NULL);
            (ildjitSystem->cliManager).translator.getTypeDescriptorSymbol(&(ildjitSystem->cliManager), layout->type);
            item			= item->next;
        }

        /* Create virtual methods.
         */
        LAYOUT_setCachingRequestsForVirtualMethodTables(layoutManager, JITFALSE);
        LAYOUT_createCachedVirtualMethodTables(layoutManager);

        /* Because we are going to execute the code, we want to enable possible future layouts.
         */
        LAYOUT_setCachingRequestsForVirtualMethodTables(layoutManager, JITTRUE);

        /* Execute all cctors.
         */
        xanList_appendList((ildjitSystem->staticMemoryManager).cachedConstructors, newCctors);
        assert(xanList_length(cctors) == xanList_length((ildjitSystem->staticMemoryManager).cachedConstructors));
        STATICMEMORY_callCachedConstructors(&(ildjitSystem->staticMemoryManager));
    }

    /* Free the memory.
     */
    xanList_destroyList(newCctors);
    xanList_destroyList(cctors);

    return;
}

JITUINT32 CODE_optimizeIR (CodeGenerator *self, Method method, JITUINT32 jobState, XanVar *checkPoint) {
    ir_method_t     *ir_method;
    JITUINT32	state;

    /* Fetch the IR method.
     */
    ir_method = method->getIRMethod(method);

    /* Optimize the method.
     */
    state = IROPTIMIZER_optimizeMethod_checkpointable(&((ildjitSystem->IRVM).optimizer), ir_method, jobState, checkPoint);

    return state;
}

static inline void internal_layout_types (t_system *system) {
    ILLayout_manager*       layoutManager;
    XanListItem 		*item;
    t_methods 		*methods;

    /* Get layout manager */
    layoutManager = &((system->cliManager).layoutManager);

    /* Fetch the methods					*/
    methods = &((system->cliManager).methods);

    /* Consider every method and take the IL types		*/
    item 	= xanList_first(methods->container);
    while (item!=NULL) {
        Method 			method;
        ir_method_t 		*irMethod;
        XanListItem 		*item2;
        XanList			*insts;
        XanList			*l;

        /* Fetch the method					*/
        method = (Method) item->data;
        assert(method != NULL);
        irMethod = method->getIRMethod(method);

        /* Take the instructions that can trigger a layout at runtime	*/
        l	= xanList_new(allocFunction, freeFunction, NULL);
        insts	= IRMETHOD_getInstructionsOfType(irMethod, IRNEWOBJ);
        item2	= xanList_first(insts);
        while (item2 != NULL) {
            ir_instruction_t	*inst;
            ir_item_t		*par1;
            ir_item_t		ilTypeIRItem;
            TypeDescriptor 		*ilType;

            /* Fetch the IL type						*/
            inst	= item2->data;
            par1		= IRMETHOD_getInstructionParameter1(inst);
            if (IRDATA_isASymbol(par1)) {
                IRSYMBOL_resolveSymbolFromIRItem(par1, &ilTypeIRItem);
                assert(IRDATA_isASymbol(par1));
            } else {
                memcpy(&ilTypeIRItem, par1, sizeof(ir_item_t));
            }
            ilType		= (TypeDescriptor *)(JITNUINT)ilTypeIRItem.value.v;
            assert(ilType != NULL);

            /* Add the type							*/
            xanList_append(l, ilType);

            /* Fetch the next element					*/
            item2	= item2->next;
        }
        xanList_destroyList(insts);
        insts	= IRMETHOD_getInstructionsOfType(irMethod, IRNEWARR);
        item2	= xanList_first(insts);
        while (item2 != NULL) {
            ir_instruction_t	*inst;
            TypeDescriptor 		*ilType;
            TypeDescriptor 		*arrayDescriptor;
            TypeDescriptor		*arrayTypeDescriptor;
            ArrayDescriptor 	*arrayType;
            ir_item_t		*par1;
            ir_item_t		ilTypeIRItem;

            /* Fetch the IL type						*/
            inst		= item2->data;
            par1		= IRMETHOD_getInstructionParameter1(inst);
            assert(par1->type != IRGLOBAL);
            if (IRDATA_isASymbol(par1)) {
                IRSYMBOL_resolveSymbolFromIRItem(par1, &ilTypeIRItem);
            } else {
                memcpy(&ilTypeIRItem, par1, sizeof(ir_item_t));
            }
            ilType		= (TypeDescriptor *)(JITNUINT)ilTypeIRItem.value.v;
            assert(ilType != NULL);
            xanList_append(l, ilType);

            /* Add the array descriptor					*/
            arrayDescriptor	= ilType->makeArrayDescriptor(ilType, 1);
            xanList_append(l, arrayDescriptor);

            /* Add the array type						*/
            arrayType 	= GET_ARRAY(arrayDescriptor);
            xanList_append(l, arrayType->type);

            /* Add the CIL type of System.Array				*/
            arrayTypeDescriptor = arrayType->getTypeDescriptor(arrayType);
            assert(arrayTypeDescriptor != NULL);
            xanList_append(l, arrayTypeDescriptor);

            /* Fetch the next element					*/
            item2	= item2->next;
        }
        xanList_destroyList(insts);

        /* Layout the IL types						*/
        item2	= xanList_first(l);
        while (item2 != NULL) {
            TypeDescriptor 		*ilType;

            /* Fetch the IL type						*/
            ilType	= (TypeDescriptor *)item2->data;
            assert(ilType != NULL);

            /* Layout the IL type						*/
            layoutManager->layoutType(layoutManager, ilType);

            /* Fetch the next element					*/
            item2	= item2->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(l);

        /* Fetch the next element				*/
        item = item->next;
    }

    return ;
}

static inline void internal_addDependentCCTORS (ir_method_t *m, XanList *deps, XanHashTable *alreadyChecked) {
    Method		ilM;
    XanList 	*cctors;
    XanList 	*reachableMethods;
    XanListItem	*item;

    /* Assertions.
     */
    assert(m != NULL);
    assert(deps != NULL);
    assert(alreadyChecked != NULL);
    assert(xanHashTable_lookup(alreadyChecked, m) == NULL);

    /* Tag the method as already checked.
     */
    xanHashTable_insert(alreadyChecked, m, m);

    /* Fetch the IL method.
     */
    ilM	= IRMETHOD_getMethodMetadata(m, METHOD_METADATA);
    assert(ilM != NULL);

    /* Fetch and append the cctors to call before this method.
     */
    cctors	= ilM->getCctorMethodsToCall(ilM);
    item	= xanList_first(cctors);
    while (item != NULL) {
        if (xanList_find(deps, item->data) == NULL) {
            xanList_append(deps, item->data);
        }
        item	= item->next;
    }

    /* Fetch the reachable methods.
     */
    reachableMethods	= IRPROGRAM_getReachableMethods(m);
    assert(reachableMethods != NULL);

    /* Check all reachable methods.
     */
    item	= xanList_first(reachableMethods);
    while (item != NULL) {
        ir_method_t	*callee;
        callee	= item->data;
        assert(callee != NULL);
        if (xanHashTable_lookup(alreadyChecked, callee) == NULL) {
            internal_addDependentCCTORS(callee, deps, alreadyChecked);
        }
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(reachableMethods);

    return ;
}

static inline XanList * internal_sortCctors (XanList *cctors) {
    XanHashTable			*alreadyChecked;
    XanList				*newCctors;
    XanListItem			*item;

    /* Assertions.
     */
    assert(cctors != NULL);

    /* Clone the list of cctors.
     */
    newCctors	= xanList_cloneList(cctors);
    assert(xanList_containsTheSameElements(cctors, newCctors));

    /* Sort the cctors.
     */
    alreadyChecked	= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    item		= xanList_first(cctors);
    while (item != NULL) {
        Method		cctor;
        ir_method_t	*cctorIR;
        XanList		*deps;
        XanListItem	*cctorItem;

        /* Fetch the cctor.
         */
        cctor		= item->data;
        assert(cctor != NULL);
        cctorIR		= cctor->getIRMethod(cctor);
        assert(cctorIR != NULL);
        cctorItem	= xanList_find(newCctors, cctor);
        assert(cctorItem != NULL);

        /* Fetch the dependences.
         */
        deps		= xanList_new(allocFunction, freeFunction, NULL);
        internal_addDependentCCTORS(cctorIR, deps, alreadyChecked);
        xanHashTable_emptyOutTable(alreadyChecked);
        xanList_removeElement(deps, cctor, JITFALSE);

        /* Check the dependences.
         */
        if (xanList_length(deps) > 0) {
            XanListItem	*item2;
            XanListItem	*firstDepItem;
            JITUINT32	firstDepItemPosition;

            /* Fetch the first dependent cctor element in the list.
             */
            firstDepItem		= NULL;
            firstDepItemPosition	= -1;
            item2		= xanList_first(deps);
            while (item2 != NULL) {
                Method		depCctor;
                XanListItem	*depCctorItem;
                depCctor	= item2->data;
                assert(depCctor != NULL);
                depCctorItem	= xanList_find(newCctors, depCctor);
                if (depCctorItem != NULL) {
                    JITUINT32	depCctorItemPosition;

                    /* The current dependent cctor belongs to the ones to execute.
                     */
                    depCctorItemPosition	= xanList_getPositionNumberFromElement(newCctors, depCctorItem->data);
                    if (firstDepItem == NULL) {
                        firstDepItem		= depCctorItem;
                        firstDepItemPosition	= depCctorItemPosition;
                        assert(firstDepItemPosition == xanList_getPositionNumberFromElement(newCctors, firstDepItem->data));

                    } else {
                        assert(firstDepItem != depCctorItem);
                        assert(firstDepItemPosition > depCctorItemPosition);
                        if (firstDepItemPosition > depCctorItemPosition) {

                            /* The first dependent item is the current dependent cctor.
                             */
                            firstDepItem		= depCctorItem;
                            firstDepItemPosition	= depCctorItemPosition;
                            assert(firstDepItemPosition == xanList_getPositionNumberFromElement(newCctors, firstDepItem->data));
                        }
                    }
                }
                item2		= item2->next;
            }
            assert(firstDepItem != NULL);
            assert(firstDepItemPosition == xanList_getPositionNumberFromElement(newCctors, firstDepItem->data));
            xanList_moveBeforeItem(newCctors, firstDepItem, cctorItem);
        }

        /* Free the memory.
         */
        xanList_destroyList(deps);

        /* Fetch the next element.
         */
        item		= item->next;
    }
    assert(xanList_containsTheSameElements(cctors, newCctors));

    /* Free the memory.
     */
    xanHashTable_destroyTable(alreadyChecked);

    return newCctors;
}

static inline void internal_addCallsToBootstrap (ir_method_t *m) {
    ir_instruction_t	*firstInst;
    ir_instruction_t	*initializeSystemInst;
    ir_instruction_t	*beforeLastInst;
    ir_instruction_t	*lastInst;
    ir_instruction_t	*getAddrProgramNameInst;
    ir_global_t		*globalFileOpenMode;
    ir_global_t		*globalProgramName;
    ir_item_t		globalProgramNameIR;
    ir_item_t		*pointerToProgramNameIR;
    XanList			*retInsts;
    XanList			*methods;
    XanListItem		*item;
    XanHashTable 		*st;
    XanHashTable		*symbolGlobalMap;
    XanHashTable		*globalILTypeMap;
    XanHashTable		*globalLayoutMap;
    XanHashTable		*allSymbols;
    XanHashTableItem	*hashItem;
    ILLayout_manager	*layoutManager;
    JITINT8			*programName;

    /* Assertions.
     */
    assert(m != NULL);

    /* Cache some pointers.
     */
    layoutManager		= &((ildjitSystem->cliManager).layoutManager);
    assert(layoutManager != NULL);

    /* Allocate the necessary memory.
     */
    symbolGlobalMap		= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    globalILTypeMap		= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    globalLayoutMap		= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the first instruction.
     */
    firstInst		= IRMETHOD_getFirstInstructionNotOfType(m, IRNOP);

    /* Generate the global for the name of the program.
     */
    programName		= IRPROGRAM_getProgramName();
    globalProgramName	= IRGLOBAL_createGlobal((JITINT8 *)"PROGRAM NAME", IRVALUETYPE, STRLEN(programName) + 1, JITTRUE, programName);
    IRGLOBAL_storeGlobalToIRItem(&globalProgramNameIR, globalProgramName, NULL);

    /* Get the pointer to the global used to store the name of the program.
     */
    getAddrProgramNameInst	= IRMETHOD_newInstructionOfTypeBefore(m, firstInst, IRGETADDRESS);
    IRMETHOD_cpInstructionParameter1(getAddrProgramNameInst, &globalProgramNameIR);
    IRMETHOD_setInstructionParameterWithANewVariable(m, getAddrProgramNameInst, IRNUINT, NULL, 0);
    pointerToProgramNameIR	= IRMETHOD_getInstructionResult(getAddrProgramNameInst);
    lastInst		= getAddrProgramNameInst;

    /* Insert a call to the runtime to initialize the system.
     */
    initializeSystemInst	= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "RUNTIME_standaloneBinaryIntializeSystem", RUNTIME_standaloneBinaryIntializeSystem, NULL, NULL);
    IRMETHOD_addInstructionCallParameter(m, initializeSystemInst, pointerToProgramNameIR);
    lastInst		= initializeSystemInst;

    /* Create the global for the string to describe a read-only mode to open files.
     */
    globalFileOpenMode	= IRGLOBAL_createGlobal((JITINT8 *)"globalFileOpenModeRead", IRVALUETYPE, 2, JITTRUE, (void *)"r");

    /* We have been configured as a static compiler.
     * Hence, we are going to dump the binary and therefore we need to create the globals for the static objects.
     * Fetch the symbol table for global static memories.
     */
    st		= IRSYMBOL_getSymbolTableForStaticMemories();
    assert(st != NULL);

    /* Create the globals related to the CIL static classes.
     */
    hashItem	= xanHashTable_first(st);
    while (hashItem != NULL) {
        ir_static_memory_t	*info;
        JITINT8			*globalName;
        JITUINT32		globalNameLength;

        /* Fetch the current global static memory.
         */
        info			= (ir_static_memory_t *)hashItem->element;
        assert(info != NULL);
        assert(info->allocatedMemory != NULL);

        /* Define the name
         */
        globalNameLength	= STRLEN(info->name) + 1 + STRLEN("STATIC MEMORY ");
        globalName		= allocFunction(globalNameLength);
        SNPRINTF(globalName, globalNameLength, "STATIC MEMORY %s", info->name);

        /* Generate an entry for the DATA section in the heap and link it into the module.
         * It has common linkage and therefore it has a zero initializer.
         */
        info->global	= IRGLOBAL_createGlobal(globalName, IRVALUETYPE, info->size, JITFALSE, info->allocatedMemory);
        assert(info->global != NULL);

        /* Free the memory.
         */
        freeFunction(globalName);

        /* Fetch the next element stored in the symbol table.
         */
        hashItem	= xanHashTable_next(st, hashItem);
    }

    /* Create the globals for CIL files.
     * Insert calls to decode embedded CIL files.
     */
    item			= xanList_first((ildjitSystem->cliManager).binaries);
    while (item != NULL) {
        t_binary_information	*ilBinary;
        FILE			*CILFile;
        JITNINT			CILFileDescriptor;
        char 			*CILFileData;
        char			*decodeILBinaryName;
        JITUINT64		CILFileDataBytes;
        JITUINT64		bytesRead;
        JITUINT64		decodeILBinaryNameLength;
        ir_item_t		filePointerItem;
        ir_item_t		memoryOfGlobal;
        ir_item_t		globalSize;
        ir_item_t		ilBinaryIRItem;
        ir_item_t		globalFileOpenModeIRItem;
        ir_item_t		ilBinaryNameItem;
        ir_item_t		*pointerToMemoryOfGlobal;
        ir_item_t		*pointerToGlobalFileOpenModeIRItem;
        ir_item_t		*pointerToIlBinaryName;
        ir_global_t		*global;
        ir_global_t		*ilBinaryName;
        ir_instruction_t	*getAddressOfMemoryGlobalInst;
        ir_instruction_t	*getAddressOfOpenModeGlobalInst;
        ir_instruction_t	*getAddressOfBinaryNameGlobalInst;
        ir_instruction_t	*fetchFilePointerInst;
        ir_instruction_t	*createILBinaryInst;
        ir_instruction_t	*setILBinaryInst;
        ir_instruction_t	*setILBinaryNameInst;
        ir_instruction_t	*decodeILBinaryInst;
        ir_instruction_t	*addDecodedBinaryInst;
        t_plugins		*decoder;

        /* Fetch the Input-Language file.
         */
        ilBinary			= (t_binary_information *)item->data;
        assert(ilBinary != NULL);
        CILFile				= (*((ilBinary->binary).reader))->stream;
        assert(CILFile != NULL);
        CILFileDescriptor		= fileno(CILFile);

        /* Allocate the memory to use to store the file in memory.
         */
        CILFileDataBytes		= PLATFORM_sizeOfFile(CILFile);
        CILFileData			= (char *)allocFunction(sizeof(char) * (CILFileDataBytes));
        assert(CILFileData != NULL);

        /* Copy CIL file to memory.
         */
        PLATFORM_lseek(CILFileDescriptor, 0, SEEK_SET);
        bytesRead			= (JITUINT64) PLATFORM_read(CILFileDescriptor, CILFileData, CILFileDataBytes);
        if (bytesRead != CILFileDataBytes) {
            abort();
        }

        /* Create the global to store the CIL file.
         */
        global					= IRGLOBAL_createGlobal(ilBinary->name, IRVALUETYPE, CILFileDataBytes, JITTRUE, CILFileData);
        assert(global != NULL);

        /* Create the global to store the name of the CIL file.
         */
        ilBinaryName				= IRGLOBAL_createGlobal((JITINT8 *)"ILBinary name", IRVALUETYPE, STRLEN(ilBinary->name) + 1, JITTRUE, (void *)ilBinary->name);

        /* Fetch the address of the global that stores the CIL file.
         */
        IRGLOBAL_storeGlobalToIRItem(&memoryOfGlobal, global, NULL);
        getAddressOfMemoryGlobalInst		= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRGETADDRESS);
        IRMETHOD_cpInstructionParameter1(getAddressOfMemoryGlobalInst, &memoryOfGlobal);
        IRMETHOD_setInstructionParameterWithANewVariable(m, getAddressOfMemoryGlobalInst, IRNUINT, NULL, 0);
        pointerToMemoryOfGlobal			= IRMETHOD_getInstructionResult(getAddressOfMemoryGlobalInst);
        lastInst				= getAddressOfMemoryGlobalInst;

        /* Fetch the address of the global used to specify the mode of opening the CIL file.
         */
        IRGLOBAL_storeGlobalToIRItem(&globalFileOpenModeIRItem, globalFileOpenMode, NULL);
        getAddressOfOpenModeGlobalInst		= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRGETADDRESS);
        IRMETHOD_cpInstructionParameter1(getAddressOfOpenModeGlobalInst, &globalFileOpenModeIRItem);
        IRMETHOD_setInstructionParameterWithANewVariable(m, getAddressOfOpenModeGlobalInst, IRNUINT, NULL, 0);
        pointerToGlobalFileOpenModeIRItem	= IRMETHOD_getInstructionResult(getAddressOfOpenModeGlobalInst);
        lastInst				= getAddressOfOpenModeGlobalInst;

        /* Fetch the address of the global used to specify the name of the binary.
         */
        IRGLOBAL_storeGlobalToIRItem(&ilBinaryNameItem, ilBinaryName, NULL);
        getAddressOfBinaryNameGlobalInst	= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRGETADDRESS);
        IRMETHOD_cpInstructionParameter1(getAddressOfBinaryNameGlobalInst, &ilBinaryNameItem);
        IRMETHOD_setInstructionParameterWithANewVariable(m, getAddressOfBinaryNameGlobalInst, IRNUINT, NULL, 0);
        pointerToIlBinaryName			= IRMETHOD_getInstructionResult(getAddressOfBinaryNameGlobalInst);
        lastInst				= getAddressOfBinaryNameGlobalInst;

        /* Create the IR item to store the size of the global.
         */
        memset(&globalSize, 0, sizeof(ir_item_t));
        globalSize.value.v			= CILFileDataBytes;
        globalSize.internal_type		= IRINT32;
        globalSize.type				= globalSize.internal_type;

        /* Insert a call to create a file pointer from the memory occupied by the global.
         */
        IRMETHOD_newVariable(m, &filePointerItem, IRNUINT, NULL);
        fetchFilePointerInst			= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "PLATFORM_fmemopen", PLATFORM_fmemopen, &filePointerItem, NULL);
        IRMETHOD_addInstructionCallParameter(m, fetchFilePointerInst, pointerToMemoryOfGlobal);
        IRMETHOD_addInstructionCallParameter(m, fetchFilePointerInst, &globalSize);
        IRMETHOD_addInstructionCallParameter(m, fetchFilePointerInst, pointerToGlobalFileOpenModeIRItem);
        lastInst				= fetchFilePointerInst;

        /* Create a new ILBinary.
         */
        IRMETHOD_newVariable(m, &ilBinaryIRItem, IRNUINT, NULL);
        createILBinaryInst	= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "ILBINARY_new", ILBINARY_new, &ilBinaryIRItem, NULL);
        lastInst		= createILBinaryInst;

        /* Set the file into the ILBinary.
         */
        setILBinaryInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "ILBINARY_setBinaryFile", ILBINARY_setBinaryFile, NULL, NULL);
        IRMETHOD_addInstructionCallParameter(m, setILBinaryInst, &ilBinaryIRItem);
        IRMETHOD_addInstructionCallParameter(m, setILBinaryInst, &filePointerItem);
        lastInst		= setILBinaryInst;

        /* Set the name of the ILBinary.
         */
        setILBinaryNameInst	= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "ILBINARY_setBinaryName", ILBINARY_setBinaryName, NULL, NULL);
        IRMETHOD_addInstructionCallParameter(m, setILBinaryNameInst, &ilBinaryIRItem);
        IRMETHOD_addInstructionCallParameter(m, setILBinaryNameInst, pointerToIlBinaryName);
        lastInst		= setILBinaryNameInst;

        /* Fetch the decoder we used for the current file.
         */
        decoder			= internal_fetchDecoder(ildjitSystem->plugins, ilBinary);
        if (decoder == NULL) {
            fprintf(stderr, "ILDJIT: ERROR = no decoder has been found for the file %s\n", ilBinary->name);
            abort();
        }

        /* Insert the call to decode the binary.
         */
        decodeILBinaryNameLength	= strlen(decoder->getName()) + strlen("_decode_image") + 1;
        decodeILBinaryName		= allocFunction(sizeof(char) * decodeILBinaryNameLength);
        snprintf(decodeILBinaryName, decodeILBinaryNameLength, "%s_decode_image", decoder->getName());
        decodeILBinaryInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, decodeILBinaryName, decoder->decode_image, NULL, NULL);
        IRMETHOD_addInstructionCallParameter(m, decodeILBinaryInst, &ilBinaryIRItem);
        lastInst			= decodeILBinaryInst;

        /* Notify the system about the new binary that has been just decoded.
         */
        addDecodedBinaryInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "CLIMANAGER_insertDecodedBinary", CLIMANAGER_insertDecodedBinary, NULL, NULL);
        IRMETHOD_addInstructionCallParameter(m, addDecodedBinaryInst, &ilBinaryIRItem);
        lastInst			= addDecodedBinaryInst;

        /* Free the memory.
         */
        freeFunction(CILFileData);

        /* Fetch the next element from the list.
         */
        item			= item->next;
    }

    /* Initialize each CLR module for force the layout of classes.
     */
    CLIMANAGER_initializeCLR();

    /* Dump into globals all the virtual tables and insert calls to register them to the layout manager.
     * To create all virtual tables, we need to layout all types.
     */
    internal_layout_types(ildjitSystem);

    /* Create virtual methods.
     */
    LAYOUT_setCachingRequestsForVirtualMethodTables(layoutManager, JITFALSE);
    LAYOUT_createCachedVirtualMethodTables(layoutManager);
    LAYOUT_setCachingRequestsForVirtualMethodTables(layoutManager, JITTRUE);

    /* Insert calls to update the globals to use the metadata data structure.
     * In order to do it, we first identify the symbols used in instructions, then we substitute them with equivalent globals that we create on the fly.
     * While we do this substitution, we keep track of the 1-1 mapping from globals to symbol identifiers (integer values).
     * Moreover, we inject calls into the wrapper method to translate these symbol identifiers to actual values (i.e., pointers to metadata data structures previously created at run time).
     * Finally, we update globals previously created with these actual values.
     *
     * Create globals and substitute symbols with them.
     */
    methods			= IRPROGRAM_getIRMethods();
    item			= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*currentMethod;
        JITUINT32 	instID;

        /* Fetch the method.
         */
        currentMethod		= item->data;
        assert(currentMethod != NULL);

        /* Substitute symbols with globals for this method.
         */
        for (instID=0; instID < IRMETHOD_getInstructionsNumber(currentMethod); instID++) {
            ir_instruction_t	*i;
            JITUINT32		parID;

            /* Fetch the instruction.
             */
            i	= IRMETHOD_getInstructionAtPosition(currentMethod, instID);
            assert(i != NULL);

            /* Check the parameters.
             */
            for (parID = 0; parID <= IRMETHOD_getInstructionParametersNumber(i); parID++) {
                ir_item_t	*par;

                /* Check whether the current IR item stores a symbol in it.
                 */
                par	= IRMETHOD_getInstructionParameter(i, parID);
                if (!IRDATA_isASymbol(par)) {
                    continue ;
                }

                /* Translate the symbol to a global.
                 */
                internal_substituteSymbolsWithGlobals(currentMethod, i, par, symbolGlobalMap, globalILTypeMap, st);
            }

            /* Check the call parameters.
             */
            for (parID = 0; parID < IRMETHOD_getInstructionCallParametersNumber(i); parID++) {
                ir_item_t	*par;

                /* Check whether the current IR item stores a symbol in it.
                 */
                par	= IRMETHOD_getInstructionCallParameter(i, parID);
                if (!IRDATA_isASymbol(par)) {
                    continue ;
                }

                /* Translate the symbol to a global.
                 */
                internal_substituteSymbolsWithGlobals(currentMethod, i, par, symbolGlobalMap, globalILTypeMap, st);
            }
            instID	= i->ID;
        }

        /* Fetch the next element from the list.
         */
        item			= item->next;
    }

    /* Add all types that have been layout.
     */
    item			= xanList_first(layoutManager->classesLayout);
    while (item != NULL) {
        ILLayout		*layout;
        ir_symbol_t		*ilTypeSymbol;
        symbol_globals_t	*symGlo;

        /* Fetch the layout.
         */
        layout				= item->data;
        assert(layout != NULL);
        assert(layout->type != NULL);

        /* Fetch the symbol of the type.
         */
        ilTypeSymbol			= (ildjitSystem->cliManager).translator.getTypeDescriptorSymbol(&(ildjitSystem->cliManager), layout->type);
        assert(ilTypeSymbol != NULL);

        /* Check if we have already created a global for it.
         */
        symGlo				= xanHashTable_lookup(symbolGlobalMap, ilTypeSymbol);
        if (symGlo == NULL) {
            symGlo				= allocFunction(sizeof(symbol_globals_t));
            symGlo->sym			= ilTypeSymbol;
            xanHashTable_insert(symbolGlobalMap, ilTypeSymbol, symGlo);
        }
        assert(symGlo != NULL);
        if (symGlo->gloType == NULL) {
            JITINT8		*gloName;
            JITINT8		*typeName;
            JITINT8		*prefixName;
            JITUINT32	gloNameLength;

            /* Make the name of the global.
             */
            typeName	= layout->type->getName(layout->type);
            assert(typeName != NULL);
            prefixName	= (JITINT8 *)"Type ";
            gloNameLength	= STRLEN(typeName) + STRLEN(prefixName) + 1;
            gloName		= allocFunction(gloNameLength);
            SNPRINTF(gloName, gloNameLength, "%s%s", prefixName, typeName);

            /* Create the global.
             */
            symGlo->gloType			= IRGLOBAL_createGlobal(gloName, IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
            xanHashTable_insert(globalILTypeMap, symGlo->gloType, ilTypeSymbol);

            /* Free the memory.
             */
            freeFunction(gloName);
        }
        symGlo->translateGlobals	= JITTRUE;

        /* Fetch the next element from the list.
         */
        item				= item->next;
    }

    /* Add all globals for all symbols.
     */
    allSymbols		= IRSYMBOL_getLoadedSymbols();
    assert(allSymbols != NULL);
    hashItem		= xanHashTable_first(allSymbols);
    while (hashItem != NULL) {
        ir_symbol_t	*currentSymbol;

        /* Fetch the symbol.
         */
        currentSymbol		= (ir_symbol_t *)hashItem->element;
        assert(currentSymbol != NULL);

        /* Check whether the symbol has been already considered.
         */
        if (xanHashTable_lookup(symbolGlobalMap, currentSymbol) == NULL) {
            symbol_globals_t	*symGlo;
            symGlo				= allocFunction(sizeof(symbol_globals_t));
            symGlo->sym			= currentSymbol;
            symGlo->translateGlobals	= JITTRUE;
            xanHashTable_insert(symbolGlobalMap, currentSymbol, symGlo);
        }

        /* Fetch the next element.
         */
        hashItem		= xanHashTable_next(allSymbols, hashItem);
    }

    /* We have all globals we need now.
     * Make space to inject code to cache serialization of symbols.
     */
    beforeLastInst		= lastInst;
    lastInst		= IRMETHOD_newInstructionOfTypeAfter(m, beforeLastInst, IRNOP);

    /* Inject calls into the wrapper method to update these globals with actual values when the binary executes.
     */
    hashItem		= xanHashTable_first(symbolGlobalMap);
    while (hashItem != NULL) {
        ir_item_t		pointerIRItem;
        ir_instruction_t	*resolveSymbolInst;
        symbol_globals_t	*symGlo;
        JITUINT32		symbolID;

        /* Fetch the symbol and the global.
         */
        symGlo				= hashItem->element;
        assert(symGlo != NULL);

        /* Check whether we have to do anything.
         */
        if (	(symGlo->translateGlobals)			&&
                (IRSYMBOL_hasSymbolIdentifier(symGlo->sym))	) {
            ir_item_t		memBytesIR;
            ir_item_t		symbolIDIRItem;
            ir_item_t		symbolTagIRItem;
            ir_item_t		*pointerToMem;
            ir_instruction_t	*getPointerToMemInst;
            ir_instruction_t	*cacheSymbolInst;
            JITUINT32		memBytes;
            JITUINT32		gloNameLength;
            void			*mem;
            char			*gloName;
            char			*gloNamePrefix;

            /* Fetch the identifier of the symbol.
             */
            symbolID			= IRSYMBOL_getSymbolIdentifier(symGlo->sym);

            /* Serialize the symbol in memory.
             */
            mem				= NULL;
            memBytes			= 0;
            if (IRSYMBOL_isSerializationOfSymbolCached(symbolID)) {
                JITUINT32	tag;
                IRSYMBOL_getSerializationOfSymbolCached(symbolID, &tag, &mem, &memBytes);
            } else {
                IRSYMBOL_serializeSymbol(symGlo->sym, &mem, &memBytes);
            }
            assert(mem != NULL);
            assert(memBytes > 0);

            /* Store the identifier of the symbol.
             */
            memset(&symbolIDIRItem, 0, sizeof(ir_item_t));
            symbolIDIRItem.value.v		= symbolID;
            symbolIDIRItem.internal_type	= IRUINT32;
            symbolIDIRItem.type		= symbolIDIRItem.internal_type;

            /* Store the tag of the symbol.
             */
            memset(&symbolTagIRItem, 0, sizeof(ir_item_t));
            symbolTagIRItem.value.v		= symGlo->sym->tag;
            symbolTagIRItem.internal_type	= IRUINT32;
            symbolTagIRItem.type		= symbolTagIRItem.internal_type;

            /* Store the number of bytes of the serialization in memory of the symbol.
             */
            memset(&memBytesIR, 0, sizeof(ir_item_t));
            memBytesIR.value.v		= memBytes;
            memBytesIR.internal_type	= IRUINT32;
            memBytesIR.type			= memBytesIR.internal_type;

            /* Create the correspondent global.
             */
            gloNamePrefix			= "Serialized symbol of ";
            gloNameLength			= strlen(gloNamePrefix) + ILDJIT_numberOfDigits(symbolID) + 1;
            gloName				= allocFunction(gloNameLength);
            snprintf(gloName, gloNameLength, "%s%u", gloNamePrefix, symbolID);
            symGlo->gloSerializedSym	= IRGLOBAL_createGlobal((JITINT8 *)gloName, IRVALUETYPE, memBytes, JITTRUE, mem);

            /* Fetch the pointer to the memory.
             */
            getPointerToMemInst		= IRMETHOD_newInstructionOfTypeAfter(m, beforeLastInst, IRGETADDRESS);
            IRGLOBAL_storeGlobalToInstructionParameter(m, getPointerToMemInst, 1, symGlo->gloSerializedSym, NULL);
            IRMETHOD_setInstructionParameterWithANewVariable(m, getPointerToMemInst, IRNUINT, NULL, 0);
            pointerToMem			= IRMETHOD_getInstructionResult(getPointerToMemInst);

            /* Cache the serialization of the symbol and inject the code for it before starting the code that will translate symbols into values.
             */
            cacheSymbolInst			= IRMETHOD_newNativeCallInstructionAfter(m, getPointerToMemInst, "IRSYMBOL_cacheSymbolSerialization", IRSYMBOL_cacheSymbolSerialization, NULL, NULL);
            IRMETHOD_addInstructionCallParameter(m, cacheSymbolInst, &symbolIDIRItem);
            IRMETHOD_addInstructionCallParameter(m, cacheSymbolInst, &symbolTagIRItem);
            IRMETHOD_addInstructionCallParameter(m, cacheSymbolInst, pointerToMem);
            IRMETHOD_addInstructionCallParameter(m, cacheSymbolInst, &memBytesIR);

            /* Insert the call to translate a symbol into the actual pointer.
             */
            if (	(symGlo->gloUserString != NULL)	||
                    (symGlo->gloField != NULL)	||
                    (symGlo->gloType != NULL)	||
                    (symGlo->gloArray != NULL)	||
                    (symGlo->gloCLRType != NULL)	) {
                IRMETHOD_newVariable(m, &pointerIRItem, IRMPOINTER, NULL);
                resolveSymbolInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "RUNTIME_fromSerializedSymbolToPointer", RUNTIME_fromSerializedSymbolToPointer, &pointerIRItem, NULL);
                IRMETHOD_addInstructionCallParameter(m, resolveSymbolInst, &symbolTagIRItem);
                IRMETHOD_addInstructionCallParameter(m, resolveSymbolInst, pointerToMem);
                IRMETHOD_addInstructionCallParameter(m, resolveSymbolInst, &memBytesIR);
                lastInst			= resolveSymbolInst;
            }

            /* Update the global with the pointer to the index of the metadata type.
             */
            if (symGlo->gloIndexType != NULL) {
                ir_item_t	indexIRItem;

                /* Insert the call to translate a symbol into the index.
                 */
                IRMETHOD_newVariable(m, &indexIRItem, IRINT32, NULL);
                resolveSymbolInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "RUNTIME_fromSerializedSymbolToInteger", RUNTIME_fromSerializedSymbolToInteger, &indexIRItem, NULL);
                memset(&symbolIDIRItem, 0, sizeof(ir_item_t));
                symbolIDIRItem.value.v		= symGlo->sym->tag;
                symbolIDIRItem.internal_type	= IRUINT32;
                symbolIDIRItem.type		= symbolIDIRItem.internal_type;
                IRMETHOD_addInstructionCallParameter(m, resolveSymbolInst, &symbolIDIRItem);
                IRMETHOD_addInstructionCallParameter(m, resolveSymbolInst, pointerToMem);
                IRMETHOD_addInstructionCallParameter(m, resolveSymbolInst, &memBytesIR);
                lastInst			= resolveSymbolInst;

                /* Update the global.
                 */
                lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloIndexType, &indexIRItem);
            }

            /* Update the global with the pointer to the user string.
             */
            if (symGlo->gloUserString != NULL) {
                lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloUserString, &pointerIRItem);
            }

            /* Update the global with the pointer to the field medata data structure.
             */
            if (symGlo->gloField != NULL) {
                lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloField, &pointerIRItem);
            }

            /* Update the global for the type.
             */
            if (symGlo->gloType != NULL) {
                lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloType, &pointerIRItem);
            }

            /* Update the global for the array type.
             */
            if (symGlo->gloArray != NULL) {
                ir_instruction_t	*getArrayDescInst;
                ir_item_t		pointerArrayDescIRItem;

                /* Translate the type into the array descriptor.
                 */
                IRMETHOD_newVariable(m, &pointerArrayDescIRItem, IRNUINT, NULL);
                getArrayDescInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "METADATA_fromTypeToArray", METADATA_fromTypeToArray, &pointerArrayDescIRItem, NULL);
                IRMETHOD_addInstructionCallParameter(m, getArrayDescInst, &pointerIRItem);
                lastInst			= getArrayDescInst;

                /* Store the value to the global.
                 */
                lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloArray, &pointerArrayDescIRItem);

                /* Translate the type array into the type descriptor.
                 */
                if (symGlo->gloIlTypeArray != NULL) {
                    ir_instruction_t	*getTypeDescInst;
                    ir_item_t		pointerTypeDescIRItem;

                    /* Translate the array descriptor into the IL type.
                     */
                    IRMETHOD_newVariable(m, &pointerTypeDescIRItem, IRNUINT, NULL);
                    getTypeDescInst		= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "METADATA_fromArrayToTypeDescriptor", METADATA_fromArrayToTypeDescriptor, &pointerTypeDescIRItem, NULL);
                    IRMETHOD_addInstructionCallParameter(m, getTypeDescInst, &pointerArrayDescIRItem);
                    lastInst		= getTypeDescInst;

                    /* Store the value to the global.
                     */
                    lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloIlTypeArray, &pointerTypeDescIRItem);
                }
            }

            /* Update the global for the CLR type.
             */
            if (symGlo->gloCLRType != NULL) {
                ir_instruction_t	*getCLRTypeInst;
                ir_item_t		pointerCLRTypeIRItem;

                /* Translate the type into the array descriptor.
                 */
                IRMETHOD_newVariable(m, &pointerCLRTypeIRItem, IRNUINT, NULL);
                getCLRTypeInst			= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "CLIMANAGER_getCLRType", CLIMANAGER_getCLRType, &pointerCLRTypeIRItem, NULL);
                IRMETHOD_addInstructionCallParameter(m, getCLRTypeInst, &pointerIRItem);
                lastInst			= getCLRTypeInst;

                /* Store the value to the global.
                 */
                lastInst			= internal_storeValueToGlobal(m, lastInst, symGlo->gloCLRType, &pointerCLRTypeIRItem);
            }

            /* Free the memory.
             */
            freeFunction(gloName);
        }

        /* Fetch the next element.
         */
        hashItem				= xanHashTable_next(symbolGlobalMap, hashItem);
    }

    /* Dump virtual tables.
     */
    item			= xanList_first(layoutManager->classesLayout);
    while (item != NULL) {
        ILLayout		*layout;
        ir_symbol_t		*ilTypeSymbol;

        /* Fetch the layout.
         */
        layout		= item->data;
        assert(layout != NULL);

        /* Fetch the symbol of the IL type.
         */
        ilTypeSymbol	= (ildjitSystem->cliManager).translator.getTypeDescriptorSymbol(&(ildjitSystem->cliManager), layout->type);
        assert(ilTypeSymbol != NULL);

        /* Check if we have performed the layout.
         * If we have not performed the layout, then this type is abstract and therefore we do not need to dump any virtual table.
         */
        if (layout->virtualTableMethods != NULL) {
            JITINT8			*gloName;
            JITUINT32		gloNameLength;
            JITINT8			*prefixName;
            JITINT8			*typeName;
            JITUINT32 		count;
            JITBOOLEAN		initializeVTs;
            ir_global_t		*gloVT;
            ir_item_t		vtIRItem;
            ir_item_t		*pointerToVtIRItem;
            ir_instruction_t	*getAddrVTInst;

            /* Initialize variables.
             */
            initializeVTs	= JITFALSE;

            /* Make the name of the global.
             */
            typeName	= layout->type->getName(layout->type);
            assert(typeName != NULL);
            prefixName	= (JITINT8 *)"Virtual table of ";
            gloNameLength	= STRLEN(typeName) + STRLEN(prefixName) + 1;
            gloName		= allocFunction(gloNameLength);
            SNPRINTF(gloName, gloNameLength, "%s%s", prefixName, typeName);

            /* Create the global for the virtual table.
             */
            gloVT		= IRGLOBAL_createGlobal(gloName, IRVALUETYPE, sizeof(JITNUINT) * layout->virtualTableLength, JITFALSE, NULL);
            xanHashTable_insert(globalLayoutMap, layout, gloVT);

            /* Store the virtual table global into the IR item.
             */
            IRGLOBAL_storeGlobalToIRItem(&vtIRItem, gloVT, NULL);

            /* Fetch the address of the virtual table stored into the associated global.
             */
            getAddrVTInst		= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRGETADDRESS);
            IRMETHOD_cpInstructionParameter1(getAddrVTInst, &vtIRItem);
            IRMETHOD_setInstructionParameterWithANewVariable(m, getAddrVTInst, IRNUINT, NULL, 0);
            pointerToVtIRItem	= IRMETHOD_getInstructionResult(getAddrVTInst);
            lastInst		= getAddrVTInst;

            /* Inject code to initialize the virtual table.
             */
            for (count=0; count < layout->virtualTableLength; count++) {
                Method			calleeM;
                ir_method_t		*calleeMIR;
                ir_instruction_t	*storeInst;

                /* Fetch the callee of the current slot of the virtual table.
                 */
                calleeM		= layout->virtualTableMethods[count];
                assert(calleeM != NULL);
                calleeMIR	= calleeM->getIRMethod(calleeM);
                assert(calleeMIR != NULL);

                /* Check whether the method has been translated or not.
                 */
                if (	IRMETHOD_hasMethodInstructions(calleeMIR)					||
                        ((!calleeM->isCilImplemented(calleeM)) && CLR_doesExistMethod(calleeM))		) {

                    /* Store the pointer to the method into the global to initialize the virtual table.
                     */
                    storeInst	= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRSTOREREL);
                    IRMETHOD_cpInstructionParameter1(storeInst, pointerToVtIRItem);
                    IRMETHOD_setInstructionParameter2(m, storeInst, count * sizeof(JITNUINT), 0, IRINT32, IRINT32, NULL);
                    IRMETHOD_setInstructionParameter3(m, storeInst, (IR_ITEM_VALUE)(JITNUINT)calleeM, 0, IRFPOINTER, IRFPOINTER, NULL);
                    lastInst	= storeInst;
                    initializeVTs	= JITTRUE;
                }
            }

            /* Inject code to initialize tables.
             */
            if (	initializeVTs					&&
                    IRSYMBOL_hasSymbolIdentifier(ilTypeSymbol)	) {
                ir_instruction_t	*initVTsInst;
                ir_item_t		ilTypeGlobal;
                ir_item_t		imtIRItem;
                symbol_globals_t	*symGlo;

                /* Fetch the symbol of the IL type.
                 */
                assert(layout != NULL);
                assert(layout->type != NULL);

                /* Fetch the associated global.
                 */
                symGlo			= xanHashTable_lookup(symbolGlobalMap, ilTypeSymbol);
                if (symGlo != NULL) {
                    assert(symGlo != NULL);
                    assert(symGlo->gloType != NULL);
                    assert(IRSYMBOL_hasSymbolIdentifier(symGlo->sym));
                    IRGLOBAL_storeGlobalToIRItem(&ilTypeGlobal, symGlo->gloType, NULL);

                    /* Store the IMT global into the IR item.
                     */
                    //TODO
                    memset(&imtIRItem, 0, sizeof(ir_item_t));
                    imtIRItem.internal_type	= IRMPOINTER;
                    imtIRItem.type		= IRMPOINTER;

                    /* Inject the call to set the tables.
                     */
                    initVTsInst	= IRMETHOD_newNativeCallInstructionAfter(m, lastInst, "LAYOUT_setMethodTables", LAYOUT_setMethodTables, NULL, NULL);
                    IRMETHOD_addInstructionCallParameter(m, initVTsInst, &ilTypeGlobal);
                    IRMETHOD_addInstructionCallParameter(m, initVTsInst, pointerToVtIRItem);
                    IRMETHOD_addInstructionCallParameter(m, initVTsInst, &imtIRItem);
                    lastInst	= initVTsInst;
                }
            }

            /* Free the memory.
             */
            freeFunction(gloName);
        }

        /* Fetch the next element.
         */
        item		= item->next;
    }

    /* Translate new instructions into native calls to the garbage collectors to use the globals as virtual tables.
     */
    item			= xanList_first(methods);
    while (item != NULL) {
        ir_method_t	*currentMethod;
        JITUINT32	instructionsNumber;
        JITUINT32 	instID;

        /* Fetch the method.
         */
        currentMethod		= item->data;
        assert(currentMethod != NULL);
        instructionsNumber	= IRMETHOD_getInstructionsNumber(currentMethod);

        /* Substitute symbols with globals for this method.
         */
        for (instID=0; instID < instructionsNumber; instID++) {
            ir_instruction_t	*i;
            ir_instruction_t	*getAddrOfGlobal;
            ir_instruction_t	*computeSizeInst;
            ir_item_t		*par1;
            ir_item_t		*par2;
            ir_item_t		*retValue;
            ir_item_t		overallSizeIR;
            ir_item_t		arraySizeIR;
            ir_item_t		ilTypeIR;
            ir_item_t		isValueTypeIR;
            ir_item_t		vTableIR;
            ir_item_t		IMTIR;
            ir_item_t		returnObjectIR;
            ir_item_t		lengthArrayIR;
            ir_symbol_t		*ilTypeSym;
            ir_global_t		*gloVT;
            symbol_globals_t	*symGlo;
            TypeDescriptor		*ilType;
            TypeDescriptor		*ilTypeOfElements;
            TypeDescriptor		*arrayType;
            ArrayDescriptor 	*arrayDescriptor;
            ILLayout		*layout;
            ILLayout		*arrayLayout;
            ILLayout		*elementLayout;
            JITBOOLEAN		isValueType;
            JITUINT32		slotSize;

            /* Fetch the instruction.
             */
            i	= IRMETHOD_getInstructionAtPosition(currentMethod, instID);
            assert(i != NULL);

            /* Transform the instruction.
             */
            switch (i->type) {
                case IRNEWOBJ:

                    /* Fetch the parameters.
                     */
                    par1		= IRMETHOD_getInstructionParameter1(i);
                    par2		= IRMETHOD_getInstructionParameter2(i);
                    retValue	= IRMETHOD_getInstructionResult(i);
                    assert(par1->type == IRGLOBAL);

                    /* Copy the return value.
                     */
                    memcpy(&returnObjectIR, retValue, sizeof(ir_item_t));

                    /* Fetch the IL type.
                     */
                    ilTypeSym	= xanHashTable_lookup(globalILTypeMap, (ir_global_t *)(JITNUINT)((par1->value).v));
                    ilType		= (TypeDescriptor *)(JITNUINT)IRSYMBOL_resolveSymbol(ilTypeSym).v;
                    assert(ilType != NULL);

                    /* Fetch the associated global.
                     */
                    symGlo			= xanHashTable_lookup(symbolGlobalMap, ilTypeSym);
                    assert(symGlo != NULL);

                    /* Fetch the address of the global.
                     */
                    IRGLOBAL_storeGlobalToIRItem(&ilTypeIR, symGlo->gloType, NULL);

                    /* Fetch the layout.
                     */
                    layout 		= layoutManager->layoutType(layoutManager, ilType);
                    assert(layout != NULL);

                    /* Compute the overall size.
                     */
                    memset(&overallSizeIR, 0, sizeof(ir_item_t));
                    overallSizeIR.internal_type	= IRUINT32;
                    if (IRDATA_isAConstant(par2)) {
                        overallSizeIR.value.v	= HEADER_FIXED_SIZE + layout->typeSize + (par2->value).v;
                        overallSizeIR.type	= overallSizeIR.internal_type;
                    } else {
                        ir_instruction_t	*addInst;
                        addInst			= IRMETHOD_newInstructionOfTypeBefore(currentMethod, i, IRADD);
                        IRMETHOD_cpInstructionParameter1(addInst, par2);
                        IRMETHOD_setInstructionParameter2(currentMethod, addInst, HEADER_FIXED_SIZE + layout->typeSize, 0, overallSizeIR.internal_type, overallSizeIR.internal_type, NULL);
                        IRMETHOD_setInstructionParameterWithANewVariable(currentMethod, addInst, overallSizeIR.internal_type, NULL, 0);
                    }

                    /* Fetch the virtual table.
                     */
                    gloVT			= xanHashTable_lookup(globalLayoutMap, layout);
                    assert(gloVT != NULL);
                    getAddrOfGlobal		= IRMETHOD_newInstructionOfTypeBefore(currentMethod, i, IRGETADDRESS);
                    IRGLOBAL_storeGlobalToInstructionParameter(currentMethod, getAddrOfGlobal, 1, gloVT, NULL);
                    IRMETHOD_setInstructionParameterWithANewVariable(currentMethod, getAddrOfGlobal, IRNUINT, NULL, 0);
                    memcpy(&vTableIR, IRMETHOD_getInstructionResult(getAddrOfGlobal), sizeof(ir_item_t));

                    /* Compute the IMT.
                     */
                    memset(&IMTIR, 0, sizeof(ir_item_t));
                    IMTIR.internal_type	= IRNUINT;
                    IMTIR.type		= IMTIR.internal_type;
                    //TODO

                    /* Transform the instruction.
                     */
                    IRMETHOD_setInstructionAsNativeCall(currentMethod, i, "GC_allocUntracedObject", GC_allocUntracedObject, &returnObjectIR, NULL);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &overallSizeIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &ilTypeIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &vTableIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &IMTIR);
                    break ;

                case IRNEWARR:

                    /* Fetch the parameters.
                     */
                    par1		= IRMETHOD_getInstructionParameter1(i);
                    par2		= IRMETHOD_getInstructionParameter2(i);
                    retValue	= IRMETHOD_getInstructionResult(i);
                    assert(par1->type == IRGLOBAL);

                    /* Copy the length of the array.
                     */
                    memcpy(&lengthArrayIR, par2, sizeof(ir_item_t));

                    /* Copy the return value.
                     */
                    memcpy(&returnObjectIR, retValue, sizeof(ir_item_t));

                    /* Fetch the IL type.
                     */
                    ilTypeSym		= xanHashTable_lookup(globalILTypeMap, (ir_global_t *)(JITNUINT)((par1->value).v));
                    assert(ilTypeSym != NULL);
                    ilTypeOfElements	= (TypeDescriptor *)(JITNUINT)IRSYMBOL_resolveSymbol(ilTypeSym).v;
                    assert(ilTypeOfElements != NULL);
                    arrayType 		= ilTypeOfElements->makeArrayDescriptor(ilTypeOfElements, 1);
                    assert(arrayType != NULL);
                    arrayDescriptor 	= GET_ARRAY(arrayType);
                    assert(arrayDescriptor != NULL);
                    ilType	 		= arrayDescriptor->getTypeDescriptor(arrayDescriptor);
                    assert(ilType != NULL);

                    /* Fetch the associated global.
                     */
                    symGlo			= xanHashTable_lookup(symbolGlobalMap, ilTypeSym);
                    assert(symGlo != NULL);

                    /* Fetch the address of the global.
                     */
                    memcpy(&ilTypeIR, par1, sizeof(ir_item_t));
                    IRGLOBAL_storeGlobalToIRItem(&ilTypeIR, symGlo->gloIlTypeArray, NULL);

                    /* Fetch the layout.
                     */
                    arrayLayout 		= layoutManager->layoutType(layoutManager, ilType);
                    assert(arrayLayout != NULL);
                    elementLayout 		= layoutManager->layoutType(layoutManager, arrayDescriptor->type);
                    assert(elementLayout != NULL);

                    /* Check if values stored inside the array are valuetypes.
                     */
                    isValueType 			= elementLayout->type->isValueType(elementLayout->type);
                    memset(&isValueTypeIR, 0, sizeof(ir_item_t));
                    isValueTypeIR.value.v		= isValueType;
                    isValueTypeIR.internal_type	= IRINT32;
                    isValueTypeIR.type		= isValueTypeIR.internal_type;

                    /* Set the slot size for the array.
                     */
                    if (isValueType) {
                        slotSize 	= elementLayout->typeSize;
                    } else {
                        slotSize 	= getIRSize(IRMPOINTER);
                    }

                    /* Set the array size.
                     */
                    if (IRDATA_isAConstant(par2)) {

                        /* Compute the size at compile time.
                         */
                        memset(&arraySizeIR, 0, sizeof(ir_item_t));
                        arraySizeIR.value.v 		= slotSize * arrayDescriptor->rank * (par2->value).v;
                        arraySizeIR.internal_type	= IRUINT32;
                        arraySizeIR.type		= arraySizeIR.internal_type;

                    } else {
                        ir_instruction_t	*computeArraySizeInst;

                        /* Compute the size at run time.
                         */
                        IRMETHOD_newVariable(currentMethod, &arraySizeIR, IRUINT32, NULL);
                        computeArraySizeInst		= IRMETHOD_newInstructionOfTypeBefore(currentMethod, i, IRMUL);
                        IRMETHOD_cpInstructionParameter1(computeArraySizeInst, par2);
                        IRMETHOD_setInstructionParameter2(currentMethod, computeArraySizeInst, slotSize * arrayDescriptor->rank, 0, IRUINT32, IRUINT32, NULL);
                        IRMETHOD_cpInstructionResult(computeArraySizeInst, &arraySizeIR);
                    }

                    /* Compute the overall size.
                     */
                    IRMETHOD_newVariable(currentMethod, &overallSizeIR, IRUINT32, NULL);
                    computeSizeInst		= IRMETHOD_newInstructionOfTypeBefore(currentMethod, i, IRADD);
                    IRMETHOD_cpInstructionParameter1(computeSizeInst, &arraySizeIR);
                    IRMETHOD_setInstructionParameter2(currentMethod, computeSizeInst, HEADER_FIXED_SIZE + getIRSize(IRMPOINTER), 0, IRUINT32, IRUINT32, NULL);
                    IRMETHOD_cpInstructionResult(computeSizeInst, &overallSizeIR);

                    /* Compute the virtual table.
                     */
                    gloVT			= xanHashTable_lookup(globalLayoutMap, arrayLayout);
                    assert(gloVT != NULL);
                    getAddrOfGlobal		= IRMETHOD_newInstructionOfTypeBefore(currentMethod, i, IRGETADDRESS);
                    IRGLOBAL_storeGlobalToInstructionParameter(currentMethod, getAddrOfGlobal, 1, gloVT, NULL);
                    IRMETHOD_setInstructionParameterWithANewVariable(currentMethod, getAddrOfGlobal, IRNUINT, NULL, 0);
                    memcpy(&vTableIR, IRMETHOD_getInstructionResult(getAddrOfGlobal), sizeof(ir_item_t));

                    /* Compute the IMT.
                     */
                    memset(&IMTIR, 0, sizeof(ir_item_t));
                    IMTIR.internal_type	= IRNUINT;
                    IMTIR.type		= IMTIR.internal_type;
                    //TODO

                    /* Transform the instruction.
                     */
                    IRMETHOD_setInstructionAsNativeCall(currentMethod, i, "GC_allocUntracedArray", GC_allocUntracedArray, &returnObjectIR, NULL);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &overallSizeIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &ilTypeIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &vTableIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &IMTIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &lengthArrayIR);
                    IRMETHOD_addInstructionCallParameter(currentMethod, i, &isValueTypeIR);
                    break ;
            }
            instID		= i->ID;
        }

        /* Fetch the next element from the list.
         */
        item			= item->next;
    }

    /* Insert a call to the runtime to shutdown the system.
     */
    retInsts		= IRMETHOD_getInstructionsOfType(m, IRRET);
    item			= xanList_first(retInsts);
    while (item != NULL) {
        ir_instruction_t	*retInst;
        retInst	= item->data;
        IRMETHOD_newNativeCallInstructionBefore(m, retInst, "RUNTIME_standaloneBinaryShutdownSystem", RUNTIME_standaloneBinaryShutdownSystem, NULL, NULL);
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(retInsts);
    xanList_destroyList(methods);
    xanHashTable_destroyTableAndData(symbolGlobalMap);
    xanHashTable_destroyTable(globalILTypeMap);
    xanHashTable_destroyTable(globalLayoutMap);

    return ;
}

static inline t_plugins * internal_fetchDecoder (XanList *plugins, t_binary_information *b) {
    XanListItem	*item;

    item = xanList_first(plugins);
    while (item != NULL) {
        t_plugins	*plugin;

        /* Fetch the plugin.
         */
        plugin = (t_plugins *) item->data;
        assert(plugin != NULL);

        /* Check the plugin.
         */
        if (xanList_find(plugin->assemblies_decoded, b) != NULL) {
            return plugin;
        }
        item = item->next;
    }

    return NULL;
}

static inline ir_instruction_t * internal_storeValueToGlobal (ir_method_t *m, ir_instruction_t *afterInst, ir_global_t *glo, ir_item_t *irValueToStore) {
    ir_instruction_t	*lastInst;
    ir_instruction_t	*getAddressOfMemoryGlobalInst;
    ir_instruction_t	*storeGlobalInst;
    ir_item_t		irGlobal;

    /* Set where to append instructions.
     */
    lastInst				= afterInst;

    /* Fetch the address of the global.
     */
    IRGLOBAL_storeGlobalToIRItem(&irGlobal, glo, NULL);
    getAddressOfMemoryGlobalInst		= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRGETADDRESS);
    IRMETHOD_cpInstructionParameter1(getAddressOfMemoryGlobalInst, &irGlobal);
    IRMETHOD_setInstructionParameterWithANewVariable(m, getAddressOfMemoryGlobalInst, IRNUINT, NULL, 0);
    lastInst				= getAddressOfMemoryGlobalInst;

    /* Set the value of the global.
     */
    storeGlobalInst				= IRMETHOD_newInstructionOfTypeAfter(m, lastInst, IRSTOREREL);
    IRMETHOD_cpInstructionParameter(m, getAddressOfMemoryGlobalInst, 0, storeGlobalInst, 1);
    IRMETHOD_setInstructionParameter2(m, storeGlobalInst, 0, 0.0, IRINT32, IRINT32, NULL);
    IRMETHOD_cpInstructionParameter3(storeGlobalInst, irValueToStore);
    lastInst				= storeGlobalInst;

    return lastInst;
}

static inline void internal_substituteSymbolsWithGlobals (ir_method_t *method, ir_instruction_t *i, ir_item_t *par, XanHashTable *symbolGlobalMap, XanHashTable *globalILTypeMap, XanHashTable *st) {
    ir_symbol_t		*sym;
    ir_global_t		*staticObjectGlobal;
    ir_instruction_t	*getAddr;
    symbol_globals_t	*symGlo;
    TypeDescriptor		*ilType;
    Method			ilMethod;

    /* Assertions.
     */
    assert(method != NULL);
    assert(i != NULL);
    assert(par != NULL);

    /* Fetch the symbol.
     */
    sym	= IRSYMBOL_getSymbol(par);
    assert(sym != NULL);

    /* Fetch the symbol global mapping.
     */
    symGlo	= xanHashTable_lookup(symbolGlobalMap, sym);
    if (symGlo == NULL) {
        symGlo		= allocFunction(sizeof(symbol_globals_t));
        symGlo->sym	= sym;
        xanHashTable_insert(symbolGlobalMap, sym, symGlo);
    }
    assert(symGlo != NULL);
    assert(symGlo->sym == sym);

    /* Handle the symbol.
     */
    switch (sym->tag) {
        case INDEX_OF_TYPE_DESCRIPTOR_SYMBOL:

            /* Notify that the symbol requires to be translated into globals.
             */
            symGlo->translateGlobals	= JITTRUE;

            if (symGlo->gloIndexType == NULL) {

                /* Create the global.
                 */
                symGlo->gloIndexType	 = IRGLOBAL_createGlobal((JITINT8 *)"Index of type ", IRINT32, sizeof(JITINT32), JITFALSE, NULL);
            }

            /* Update the IR to use the global instead of the symbol.
             */
            IRGLOBAL_storeGlobalToIRItem(par, symGlo->gloIndexType, NULL);
            break ;

        case FUNCTION_POINTER_SYMBOL:

            /* Fetch the callee.
             */
            ilMethod	= (Method) IRSYMBOL_unresolveSymbolFromIRItem(par);
            assert(ilMethod != NULL);

            /* Store the data structure that describes the callee in the IR item instead of the symbol.
             */
            memset(par, 0, sizeof(ir_item_t));
            if (	(ilMethod->isCilImplemented(ilMethod))					&&
                    (IRMETHOD_hasMethodInstructions(ilMethod->getIRMethod(ilMethod)))	) {
                par->value.v		= (IR_ITEM_VALUE)(JITNUINT)ilMethod;
            }
            par->type		= IRFPOINTER;
            par->internal_type	= IRFPOINTER;
            break ;

        case USER_STRING_SYMBOL:

            /* Notify that the symbol requires to be translated into globals.
             */
            symGlo->translateGlobals	= JITTRUE;

            if (symGlo->gloUserString == NULL) {

                /* Create the global.
                 */
                symGlo->gloUserString	 = IRGLOBAL_createGlobal((JITINT8 *)"User String ", IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
            }

            /* Update the IR to use the global instead of the symbol.
             */
            IRGLOBAL_storeGlobalToIRItem(par, symGlo->gloUserString, NULL);
            break ;

        case FIELD_DESCRIPTOR_SYMBOL:

            /* Notify that the symbol requires to be translated into globals.
             */
            symGlo->translateGlobals	= JITTRUE;

            if (symGlo->gloField == NULL) {

                /* Create the global.
                 */
                symGlo->gloField	 = IRGLOBAL_createGlobal((JITINT8 *)"Field ", IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
            }

            /* Update the IR to use the global instead of the symbol.
             */
            IRGLOBAL_storeGlobalToIRItem(par, symGlo->gloField, NULL);
            break ;

        case TYPE_DESCRIPTOR_SYMBOL:

            /* Notify that the symbol requires to be translated into globals.
             */
            symGlo->translateGlobals	= JITTRUE;

            /* Create the relative global.
             */
            switch (i->type) {
                case IRNEWARR:

                    /* Check if we have already created the relative global.
                     */
                    if (symGlo->gloArray == NULL) {

                        /* Create the global.
                         */
                        symGlo->gloArray	 = IRGLOBAL_createGlobal((JITINT8 *)"Array ", IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
                    }

                    /* Check if we have already created the global for the IL type to store within the header of the array.
                     */
                    if (symGlo->gloIlTypeArray == NULL) {

                        /* Create the global.
                         */
                        symGlo->gloIlTypeArray	 = IRGLOBAL_createGlobal((JITINT8 *)"Type ", IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
                    }

                    /* Update the IR to use the global instead of the symbol.
                     */
                    xanHashTable_insert(globalILTypeMap, symGlo->gloArray, sym);
                    IRGLOBAL_storeGlobalToIRItem(par, symGlo->gloArray, NULL);
                    break ;

                default :

                    /* Fetch the IL type.
                     */
                    ilType	= (TypeDescriptor *)(JITNUINT)IRSYMBOL_resolveSymbol(sym).v;
                    assert(ilType != NULL);

                    /* Check if we have already created the relative global.
                     */
                    if (symGlo->gloType == NULL) {
                        JITINT8		*gloName;
                        JITINT8		*typeName;
                        JITINT8		*prefixName;
                        JITUINT32	gloNameLength;

                        /* Make the name of the global.
                         */
                        typeName	= ilType->getName(ilType);
                        assert(typeName != NULL);
                        prefixName	= (JITINT8 *)"Type ";
                        gloNameLength	= STRLEN(typeName) + STRLEN(prefixName) + 1;
                        gloName		= allocFunction(gloNameLength);
                        SNPRINTF(gloName, gloNameLength, "%s%s", prefixName, typeName);

                        /* Create the global.
                         */
                        symGlo->gloType		 = IRGLOBAL_createGlobal(gloName, IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);

                        /* Free the memory.
                         */
                        freeFunction(gloName);
                    }

                    /* Update the IR to use the global instead of the symbol.
                     */
                    xanHashTable_insert(globalILTypeMap, symGlo->gloType, sym);
                    IRGLOBAL_storeGlobalToIRItem(par, symGlo->gloType, NULL);
                    break ;
            }
            break ;

        case CLR_TYPE_SYMBOL:

            /* Notify that the symbol requires to be translated into globals.
             */
            symGlo->translateGlobals	= JITTRUE;

            /* Check if we have already created the relative global.
             */
            if (symGlo->gloCLRType == NULL) {

                /* Create the global.
                 */
                symGlo->gloCLRType	 = IRGLOBAL_createGlobal((JITINT8 *)"CLR type ", IRNUINT, sizeof(JITNUINT), JITFALSE, NULL);
            }

            /* Update the IR to use the global instead of the symbol.
             */
            IRGLOBAL_storeGlobalToIRItem(par, symGlo->gloCLRType, NULL);
            break ;

        case STATIC_OBJECT_SYMBOL:

            /* Fetch the correspondent global.
             */
            staticObjectGlobal	= ((ir_static_memory_t *)xanHashTable_lookup(st, sym))->global;
            assert(staticObjectGlobal != NULL);

            /* Replace the pointer to the static memory with the pointer to the global.
             */
            getAddr			= IRMETHOD_newInstructionOfTypeBefore(method, i, IRGETADDRESS);
            IRGLOBAL_storeGlobalToInstructionParameter(method, getAddr, 1, staticObjectGlobal, NULL);
            IRMETHOD_setInstructionParameterWithANewVariable(method, getAddr, IRNUINT, NULL, 0);
            IRMETHOD_cpInstructionParameterToItem(method, getAddr, 0, par);
            break ;

        case COMPILER_CALL_SYMBOL:

            /* Internal calls are already handled by passing the name of the callee to the linker.
             */
            break ;

        default :
            fprintf(stderr, "ILDJIT: Error = symbol %u is not handled.\n", sym->tag);
            abort();
    }

    return ;
}
