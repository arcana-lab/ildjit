/*
 * Copyright (C) 2006 - 2014 Campanoni Simone,
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
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>
#include <ir_virtual_machine.h>
#include <garbage_collector.h>
#include <jit_metadata.h>
#include <platform_API.h>
#include <ilmethod.h>

// My header
#include <exception_manager.h>
#include <recompiler.h>
#include <system_manager.h>
#include <initial_checker.h>
#include <system_initializer.h>
#include <lib_compilerNativeCall.h>
#include <loader.h>
#include <runtime.h>
#include <iljit_plugin_manager.h>
#include <cil_ir_translator.h>
#include <general_tools.h>
#include <static_memory.h>
#include <ildjit_profiler.h>
#include <garbage_collector_interactions.h>
#include <iljit.h>
#include <translation_pipeline.h>
#include <layout_manager.h>
#include <lib_lock.h>
#include <code_generator.h>
#include <clr_manager.h>
// End

/**
 *
 * Free the last memory block allocated by the system manager
 */
static inline void setup_garbage_collector (t_system *system);
static inline void setup_library_path (char **libraryPath, char *envVar, char *defaultPath);
static inline void internal_system_shutdown (t_system *system);

/* t_system functions		*/
t_methods * getMethods (t_system *system);

/* t_methods functions		*/
static inline Method fetchOrCreateMethod (t_methods *methods, MethodDescriptor *methodID, JITBOOLEAN isExternallyLinkable);
static inline Method libclimanager_fetchOrCreateMethod (MethodDescriptor *methodID);

static char prefix[DIM_BUF], library_path[DIM_BUF], path_plugin[DIM_BUF], path_garbage_collector_plugin[DIM_BUF], cil_path[DIM_BUF], path_clr_plugin[DIM_BUF], machinecode_path[DIM_BUF];

t_system *ildjitSystem;

char * ILDJIT_getPrefix (void) {
    return get_prefix();
}

void * ILDJIT_boxNullPrimitiveType (JITUINT32 IRType) {
    return runtimeBoxNullPrimitiveType(ildjitSystem, IRType);
}

void * ILDJIT_boxNullValueType (TypeDescriptor *valueType) {
    return runtimeBoxNullValueType(ildjitSystem, valueType);
}

void * ILDJIT_getCurrentThreadException (void) {
    return getCurrentThreadException();
}

void ILDJIT_throwException (void *exceptionObject) {
    throw_thread_exception(exceptionObject);

    return ;
}

void ILDJIT_throwExceptionWithName (JITINT8 *typeNameSpace, JITINT8 *typeName) {
    throw_thread_exception_ByName(typeNameSpace, typeName);

    return ;
}

void ILDJIT_quit (JITINT32 exitCode) {

    /* Store the time elapsed.
     */
    if (    ((ildjitSystem->IRVM).behavior.profiler >= 1)         ||
            ((ildjitSystem->IRVM).behavior.aot)                   ) {
        (ildjitSystem->profiler).execution_time = profilerGetTimeElapsed(&((ildjitSystem->profiler).startExecutionTime), &((ildjitSystem->profiler).wallTime.execution_time));
    }

    /* Shutdown the system.
     */
    internal_system_shutdown(ildjitSystem);

    /* Quit the program.
     */
    exit(exitCode);

    return ;
}

static inline void iljit_tagIRMethodAsProfiled (ir_method_t *irMethod) {
    Method method;

    /* Assertions			*/
    assert(irMethod != NULL);

    /* Fetch the method		*/
    method = (Method) IRMETHOD_getMethodMetadata(irMethod, METHOD_METADATA);
    assert(method != NULL);

    /* Tag the method		*/
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), method);
    }

    /* Return			*/
    return;
}

static inline IR_ITEM_VALUE iljit_getIRMethodID (ir_method_t *irMethod) {
    Method method;

    /* Fetch the method		*/
    method = (Method) IRMETHOD_getMethodMetadata(irMethod, METHOD_METADATA);
    assert(method != NULL);

    /* Return			*/
    return (IR_ITEM_VALUE) (JITNUINT) method;
}

static inline JITBOOLEAN iljit_unregisterIRMethod (ir_method_t *method) {
    Method m;
    t_methods *methods;

    m = (Method) (JITNUINT) iljit_getIRMethodID(method);
    assert(m != NULL);

    /* Fetch the system             */
    methods = &((ildjitSystem->cliManager).methods);
    assert(methods != NULL);

    /* Unregister the method	*/
    if (methods->deleteMethod(methods, m)) {

        /* Destroy the method		*/
        m->destroyMethod(m);

        /* Return			*/
        return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

static inline JITBOOLEAN iljit_hasIRBody (ir_method_t *method) {
    Method m;

    m = (Method) (JITNUINT) iljit_getIRMethodID(method);
    assert(m != NULL);
    return m->isIrImplemented(m);
}

static inline void iljit_saveProgram (JITINT8 *name) {
    MANFRED_saveProgram(&(ildjitSystem->manfred), name);
}

static inline ir_method_t * iljit_getCallerOfMethodInExecution (ir_method_t *m) {
    ir_method_t *caller;

    if (m == NULL) {
        caller = IRVM_getRunningMethod(&(ildjitSystem->IRVM));
    } else {
        caller = IRVM_getCallerOfMethodInExecution(&(ildjitSystem->IRVM), m);
    }
    return caller;
}

static inline void iljit_addToSymbolToSave (ir_symbol_t *symbol) {
    if ((ildjitSystem->IRVM).behavior.aot || (ildjitSystem->IRVM).behavior.pgc == 1) {
        MANFRED_addToSymbolToSave(&(ildjitSystem->manfred), symbol);
    }
}

static inline ir_symbol_t  * iljit_loadSymbol (JITUINT32 number) {
    return MANFRED_loadSymbol(&(ildjitSystem->manfred), number);
}

static inline XanList * iljit_getCompatibleMethods (ir_signature_t *signature) {
    XanList 	*list;
    XanList 	*compatibleMethods;
    t_methods 	*methods;

    /* Assertions.
     */
    assert(signature != NULL);

    /* Fetch the methods.
     */
    methods = getMethods(ildjitSystem);
    assert(methods != NULL);

    /* Allocate the list.
     */
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Fetch the compatible methods.
     */
    compatibleMethods = methods->findCompatibleMethods(methods, signature);
    if (compatibleMethods != NULL) {
        XanListItem 	*item;

        /* Fetch the subset that has IR body.
         */
        xanList_lock(compatibleMethods);
        item = xanList_first(compatibleMethods);
        while (item != NULL) {
            Method newMethod = (Method) item->data;
            assert(newMethod != NULL);
            if (newMethod->isIrImplemented(newMethod)) {
                xanList_append(list, newMethod);
            }
            item = item->next;
        }
        xanList_unlock(compatibleMethods);
    }

    return list;
}

static inline XanList * iljit_getImplementationsOfMethod (IR_ITEM_VALUE irMethodID) {
    XanList         *list;
    t_system        *system;
    Method method;
    MethodDescriptor *methodID;
    MethodDescriptor *methodDefinition;
    XanListItem     *item;
    MethodImplementations *implementationInformation;
    GenericContainer *container;

    /* Fetch the system             */
    system = getSystem(NULL);
    assert(system != NULL);

    /* Initialize */
    method = (Method) (JITNUINT) irMethodID;
    assert(method != NULL);
    methodID = method->getID(method);
    assert(methodID != NULL);
    if (methodID->isGeneric(methodID)) {
        methodDefinition = methodID->getGenericDefinition(methodID);
        container = methodID->container;
    } else {
        methodDefinition = methodID;
        container = NULL;
    }

    /* Allocate the list		*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    implementationInformation = (system->cliManager).layoutManager.getMethodImplementations(&((system->cliManager).layoutManager), methodID);

    xanList_lock(implementationInformation->implementor);
    item = xanList_first(implementationInformation->implementor);
    while (item != NULL) {
        Method newMethod;
        MethodDescriptor *bodyID;
        ILLayout *layout = (ILLayout *) item->data;

        if (container != NULL) {
            VTableSlot *slot = (VTableSlot *) xanHashTable_lookup(layout->methods, methodDefinition);
            bodyID = slot->body->getInstance(slot->body, container);
        } else {
            VTableSlot *slot = (VTableSlot *) xanHashTable_lookup(layout->methods, methodDefinition);
            bodyID = slot->body;
        }
        assert(!bodyID->attributes->is_abstract);
        newMethod = fetchOrCreateMethod(&((system->cliManager).methods), bodyID, JITFALSE);
        if (newMethod->isIrImplemented(newMethod)) {
            xanList_append(list, newMethod);
        }

        item = item->next;
    }
    xanList_unlock(implementationInformation->implementor);

    /* Return			*/
    return list;
}

static inline XanList * iljit_getIRMethods (void) {
    XanList         	*l;
    Method 			method;
    t_methods		*methods;
    XanListItem    	 	*item;
    XanHashTable		*anonymousMethods;
    XanHashTableItem	*hashItem;

    /* Allocate the list		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fill up the list		*/
    methods = &((ildjitSystem->cliManager).methods);
    item = xanList_first(methods->container);
    while (item != NULL) {

        /* Fetch the method		*/
        method = (Method) item->data;
        assert(method != NULL);

        /* Check if the method is in	*
         * CIL				*/
        if (method->isIrImplemented(method)) {
            xanList_append(l, method->getIRMethod(method));
        }

        /* Fetch the next element from  *
         * the list			*/
        item = item->next;
    }
    anonymousMethods	= ((ildjitSystem->cliManager).methods).anonymousContainer;
    hashItem  		= xanHashTable_first(anonymousMethods);
    while (hashItem != NULL) {
        Method method;
        method = (Method) hashItem->element;
        assert(method != NULL);
        if (method->isIrImplemented(method)) {
            xanList_append(l, method->getIRMethod(method));
        }
        hashItem  = xanHashTable_next(anonymousMethods, hashItem);
    }

    return l;
}

static inline ir_method_t * iljit_getIRMethodFromEntryPoint (void *entryPointAddress) {
    t_system        *system;
    Method method;
    XanHashTable *functionsEntryPoint;

    /* Fetch the system             */
    system = getSystem(NULL);
    assert(system != NULL);

    /* Cache some pointers		*/
    functionsEntryPoint = (system->cliManager).methods.functionsEntryPoint;
    assert(functionsEntryPoint != NULL);

    /* Fetch the JIT method		*/
    method = (Method) xanHashTable_lookup(functionsEntryPoint, entryPointAddress);
    if (method == NULL) {
        return NULL;
    }

    /* Return			*/
    return &(method->IRMethod);
}

static inline JITUINT32 iljit_getTypeSize (TypeDescriptor *typeDescriptor) {
    t_system        *system;
    IRVM_type       *jit_type;

    if (typeDescriptor == NULL) {
        return 0;
    }

    /* Fetch the system             */
    system = getSystem(NULL);
    assert(system != NULL);

    /* Layout the type		*/
    jit_type = (system->cliManager).layoutManager.layoutJITType(&(system->cliManager).layoutManager, typeDescriptor);
    assert(jit_type != NULL);

    /* Return			*/
    return IRVM_getIRVMTypeSize(&(ildjitSystem->IRVM), jit_type);
}

static inline ir_method_t * iljit_getIRMethodFromMethod (IR_ITEM_VALUE methodID) {
    Method method;

    /* Fetch the method		*/
    method = (Method) (JITNUINT) methodID;
    assert(method != NULL);

    return &(method->IRMethod);
}

t_jit_function * iljit_getJITMethodFromMethod (IR_ITEM_VALUE methodID) {
    Method method;

    /* Fetch the method		*/
    method = (Method) (JITNUINT) methodID;
    assert(method != NULL);

    return method->getJITFunction(method);
}

static inline void iljit_ensureCorrectJITFunction(Method method) {
    t_system *system;

    /* Fetch the system             */
    system = getSystem(NULL);
    assert(system != NULL);

    PDEBUG("SYSTEM_MANAGER: iljit_ensureCorrectJITFunction: method=%s\n", method->getFullName(method));
    if ( *(method->jit_function) == NULL ) {
        /* Allocate space for the JIT function		*/
        PDEBUG("SYSTEM_MANAGER: iljit_ensureCorrectJITFunction:    allocating space\n");
        *(method->jit_function) = allocFunction(sizeof(t_jit_function));

        if (!((method->IRMethod).ID->attributes->is_internal_call)) {
            IRVM_translateIRMethodSignatureToBackendSignature(&(system->IRVM), &((method->IRMethod).signature), *(method->jit_function));
            IRVM_newBackendMethod(&(system->IRVM), method, *(method->jit_function), !(system->IRVM).behavior.staticCompilation, JITTRUE);
        } else {
            IRVM_translateIRMethodSignatureToBackendSignature(&(system->IRVM), &((method->IRMethod).signature), *(method->jit_function));
        }

        /* Build method body, if needed			*/
        if ((method->IRMethod).ID->attributes->is_provided_by_the_runtime) {
            method->setState(method, IR_STATE);
        } else if ((method->IRMethod).ID->attributes->is_pinvoke) {
            method->setState(method, IR_STATE);
        } else if ((method->IRMethod).ID->attributes->is_internal_call) {
            CLR_buildMethod(method);
            method->setState(method, EXECUTABLE_STATE);
        } else {
            JITUINT32 gstate = method->getGlobalState(method);
            if ( gstate < IR_GSTATE ) {
                method->setState(method, CIL_STATE);
            } else {
                method->setState(method, IR_STATE);
            }
        }
    }
}

static inline ir_method_t * iljit_getEntryPoint (void) {
    Method m;
    ir_method_t     *irM;
    t_system        *system;

    /* Fetch the system             */
    system = getSystem(NULL);
    assert(system != NULL);

    /* Fetch the method		*/
    m = (system->program).entry_point;
    if (m == NULL) {
        return NULL;
    }
    assert(m != NULL);

    /* Fetch the IR method		*/
    irM = &(m->IRMethod);
    assert(irM != NULL);

    /* Return			*/
    return irM;
}

static inline void checkAnonymousMethodInited (Method method) {
    t_system        *system;
    ir_signature_t  *irSignature;

    /* Fetch the system             */
    system = getSystem(NULL);
    assert(system != NULL);

    if (!IRVM_hasTheSignatureBeenTranslated(&(ildjitSystem->IRVM), *(method->jit_function))) {

        /* Prepare JIT Signature  and function */
        irSignature = IRMETHOD_getSignature(&(method->IRMethod));
        IRVM_translateIRMethodSignatureToBackendSignature(&(system->IRVM), irSignature, *(method->jit_function));
        IRVM_newBackendMethod(&(system->IRVM), method, *(method->jit_function), JITTRUE, JITTRUE);
    }
}

static inline void iljit_createTrampoline (ir_method_t *method) {
    Method bytecodeMethod;

    /* Initialize the variables	*/
    bytecodeMethod = (Method) IRMETHOD_getMethodMetadata(method, METHOD_METADATA);

    /* Ensure that signature is initialized */
    if (bytecodeMethod->isAnonymous(bytecodeMethod)) {
        checkAnonymousMethodInited(bytecodeMethod);
    }
}

static inline void * iljit_getFunctionPointer (ir_method_t *method) {
    Method bytecodeMethod;

    /* Initialize the variables	*/
    bytecodeMethod = (Method) IRMETHOD_getMethodMetadata(method, METHOD_METADATA);

    /* Ensure that signature is initialized */
    if (bytecodeMethod->isAnonymous(bytecodeMethod)) {
        checkAnonymousMethodInited(bytecodeMethod);
    }

    /* Return			*/
    return bytecodeMethod->getFunctionPointer(bytecodeMethod);
}


static inline void iljit_translateIRMethod (ir_method_t *method) {
    Method	bytecodeMethod;

    /* Initialize the variables.
     */
    bytecodeMethod = (Method) IRMETHOD_getMethodMetadata(method, METHOD_METADATA);
    if (bytecodeMethod == NULL) {
        fprintf(stderr, "ERROR: The IR method %s does not have the Method structure. \n", IRMETHOD_getSignatureInString(method));
        abort();
    }

    /* Check if the method has been already compiled.
     */
    if (bytecodeMethod->getState(bytecodeMethod) == EXECUTABLE_STATE) {
        return ;
    }

    /* Ensure that signature is initialized */
    if (bytecodeMethod->isAnonymous(bytecodeMethod)) {
        checkAnonymousMethodInited(bytecodeMethod);
    }

    /* Translate the method.
     */
    switch (bytecodeMethod->getState(bytecodeMethod)) {
        case CIL_STATE:
        case MACHINE_CODE_STATE:
            break;
        default:
            bytecodeMethod->setState(bytecodeMethod, IR_STATE);
    }
    (ildjitSystem->pipeliner).synchInsertMethod(&(ildjitSystem->pipeliner), bytecodeMethod, MIN_METHOD_PRIORITY);

    return;
}

static inline JITINT32 iljit_runMethod (ir_method_t *method, void **args, void *returnArea) {
    Method	 	bytecodeMethod;
    JITINT32	returnValue;

    /* Initialize the variables.
     */
    bytecodeMethod = (Method) IRMETHOD_getMethodMetadata(method, METHOD_METADATA);
    assert(bytecodeMethod != NULL);

    /* Ensure that method is traslated.
     */
    iljit_translateIRMethod(method);

    /* Save Trace Profile.
     */
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), bytecodeMethod);
    }

    /* Check if we need to execute the code.
     */
    if ((ildjitSystem->program).disableExecution) {
        return 0;
    }

    /* Run the method.
     */
    returnValue	= IRVM_run(&(ildjitSystem->IRVM), *(bytecodeMethod->jit_function), args, returnArea);

    return returnValue;
}

static inline ir_method_t *iljit_newIRMethod (JITINT8 *name) {
    t_methods               *methods;

    methods = &((ildjitSystem->cliManager).methods);

    return &(methods->fetchOrCreateAnonymousMethod(methods, name)->IRMethod);
}

static inline JITINT8 * iljit_getNativeMethodName (JITINT8 *internalName) {
    JITINT8	*functionName;
    void	*fp;

    CLR_getFunctionNameAndFunctionPointer(internalName, &functionName, &fp);

    return functionName;
}

static inline ir_method_t * iljit_getIRMethod (ir_method_t *method, IR_ITEM_VALUE irMethodID, JITBOOLEAN ensureIRTranslation) {
    Method methodToFetch;
    t_system        *system;
    ir_method_t     *irMethod;

    /* Assertions			*/
    assert(irMethodID != 0);

    /* Fetch the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    /* Fetch the method             */
    methodToFetch = (Method) (JITNUINT) irMethodID;
    assert(methodToFetch != NULL);

    /* Fetch the IR method		*/
    irMethod = &(methodToFetch->IRMethod);
    assert(irMethod != NULL);

    /* Anonymous method not require translation */
    if (methodToFetch->isAnonymous(methodToFetch)) {
        return irMethod;
    }

    /* Check that the method is     *
     * available in CIL		*/
    if (!methodToFetch->isIrImplemented(methodToFetch)) {
        return irMethod;
    }

    /* Check if we need to ensure   *
    * that the method is already   *
    * available in IR              */
    if (!ensureIRTranslation) {
        return irMethod;
    }
    assert(ensureIRTranslation);

    /* Check if are executing a	*
     * static compilation.		*
     * In this case, every method   *
     * that we wanted to translate  *
     * are already available	*/
    if ((system->IRVM).behavior.onlyPrecompilation) {
        return irMethod;
    }

    /* Check if the method is the   *
     * same as the one given as     *
     * input			*/
    if (method != irMethod) {
        JITBOOLEAN toUnlock;

        /* Unlock the method in         *
         * compilation (in order to	*
         * avoid deadlocks		*/
        if (method != NULL) {
            toUnlock = IRMETHOD_tryLock(method);
            IRMETHOD_unlock(method);
        }

        /* Fetch the method requested	*/
        //(system->pipeliner).synchTillIRMethod(&(system->pipeliner), methodToFetch, MIN_METHOD_PRIORITY);
        (system->pipeliner).synchInsertMethod(&(system->pipeliner), methodToFetch, MIN_METHOD_PRIORITY);

        /* Lock back the method in      *
         * compilation			*/
        if (    (toUnlock)              &&
                (method != NULL)        ) {
            IRMETHOD_lock(method);
        }
    }

    /* Return			*/
    return irMethod;
}

IRVM_type * iljit_lookupValueType (TypeDescriptor *type) {
    t_system        *system;

    /* Assertions			*/
    assert(type != NULL);

    /* Fetch the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    /* Return			*/
    return (system->cliManager).layoutManager.layoutJITType(&((system->cliManager).layoutManager), type);
}

JITINT8 * iljit_getProgramName (void) {
    return (JITINT8 *)(ildjitSystem->programName);
}

void iljit_compileToIR (void *method_void) {
    Method method;
    t_system        *system;

    /* Assertions			*/
    assert(method_void != NULL);

    /* Fetch the system		*/
    system = getSystem(NULL);
    assert(system != NULL);

    /* Fetch the method		*/
    method = (Method) method_void;
    assert(method != NULL);

    /* Compile to IR the method	*/
    (system->pipeliner).synchTillIRMethod(&(system->pipeliner), method, MAX_METHOD_PRIORITY);

    /* Return			*/
    return;
}

t_system * generator_bootstrapper (int argc, char **argv) {

    /* Allocate and initialize the system.
     */
    allocateAndInitializeSystem(JITTRUE, JITTRUE, JITTRUE, argc, argv, JITTRUE, JITTRUE);

    return ildjitSystem;
}

t_system * allocateAndInitializeSystem (JITBOOLEAN loadPlugins, JITBOOLEAN initMemoryManager, JITBOOLEAN parseArguments, int argc, char *argv[], JITBOOLEAN enableCodeGenerator, JITBOOLEAN enableDynamicCLRs) {
    t_system                *system;
    IR_ITEM_VALUE		buildDelegateFunctionPointer;
    compilation_scheme_t	compilationScheme;

    /* Create the system.
     */
    system  	= (t_system *) allocFunction(sizeof(t_system));
    if (system == NULL) {
        print_err("BOOTSTRAPPER: Cannot allocate memory for the system. ", errno);
        return NULL;
    }
    memset(system, 0, sizeof(t_system));
    assert(system != NULL);
    ildjitSystem	= system;

    /* Setup the Compiler Memory manager.
     */
    if (initMemoryManager) {
        MEMORY_initMemoryManager(&(system->compilerMemoryManager));
        MEMORY_enableSharedMemory(&(system->compilerMemoryManager));
        MEMORY_enablePrivateMemory(&(system->compilerMemoryManager));
    }

    /* Load the garbage collectors of the system.
     */
    if (loadPlugins) {
        (system->garbage_collectors).plugins = load_garbage_collectors();
        assert((system->garbage_collectors).plugins != NULL);
    }

    /* Init the ILDJIT environment.
     */
    compilationScheme	= jitScheme;
    if (parseArguments) {

        /* Parse the arguments.
         */
        if (system_initializer(system, argc, argv) != 0) {
            freeMemory(system);
            return NULL;
        }

        /* Set the compilation scheme.
         */
        compilationScheme	= jitScheme;
        if ((system->IRVM).behavior.dla) {
            compilationScheme	= dlaScheme;
        } else if ((system->IRVM).behavior.staticCompilation) {
            compilationScheme	= staticScheme;
        } else if ((system->IRVM).behavior.aot) {
            compilationScheme	= aotScheme;
        }
    }

    /* Setup the garbage collection.
     */
    setup_garbage_collector(system);

    /* Setup the library path.
     */
    setup_library_path(&(system->cilLibraryPath), "ILDJIT_PATH", get_cil_path());
    setup_library_path((char **)&(system->machineCodeLibraryPath), "ILDJIT_MACHINECODE_PATH", get_machinecode_path());

    /* Fill up the system structure.
     */
    (system->cliManager).methods.fetchOrCreateMethod = fetchOrCreateMethod;

    /* INITIALIZE THE system->program STRUCTURE.
     */
    (system->program).thread_exception_infos			= (t_thread_exception_infos *) allocFunction(sizeof(t_thread_exception_infos));
    ((system->program).thread_exception_infos)->stackTrace		= NULL;
    ((system->program).thread_exception_infos)->exception_thrown	= NULL;
    ((system->program).thread_exception_infos)->isConstructed	= 1;

    /* Load the decoder plugins of the system.
     */
    if (loadPlugins) {
        system->plugins = load_plugin(system);
        if (system->plugins == NULL) {
            print_err("BOOTSTRAPPER: ERROR= No plugins loaded. ", 0);
            freeFunction(system);
            abort();
        }
    }

    /* Allocate the IR Manager.
     */
    IRLibNew(		&((system->IRVM).ir_system),
                    compilationScheme,
                    iljit_newIRMethod,
                    iljit_createTrampoline,
                    iljit_getIRMethod,
                    iljit_getProgramName,
                    iljit_translateIRMethod,
                    iljit_runMethod,
                    iljit_getTypeSize,
                    iljit_getEntryPoint,
                    iljit_getIRMethodFromEntryPoint,
                    iljit_getIRMethodID,
                    iljit_getIRMethods,
                    iljit_getImplementationsOfMethod,
                    iljit_getCompatibleMethods,
                    iljit_getFunctionPointer,
                    iljit_loadSymbol,
                    iljit_addToSymbolToSave,
                    iljit_getCallerOfMethodInExecution,
                    iljit_saveProgram,
                    iljit_hasIRBody,
                    iljit_unregisterIRMethod,
                    iljit_tagIRMethodAsProfiled,
                    iljit_getNativeMethodName,
                    &((system->staticMemoryManager).symbolTable),
                    &((system->cliManager).binaries));

    /* Initialize Compiler Call Manager.
     */
    init_compilerCallManager();

    /* Initialize Manfred (AOT compiler).
     */
    MANFRED_init(&(system->manfred), &(system->cliManager));

    /* Start the CLI manager.
     */
    buildDelegateFunctionPointer = (IR_ITEM_VALUE) (JITNUINT) COMPILERCALL_getSymbol((JITINT8 *) "BuildDelegate");
    init_CLIManager(	&(system->cliManager),
                        &(system->IRVM),
                        &((system->garbage_collectors).gc),
                        libclimanager_fetchOrCreateMethod,
                        throw_thread_exception_ByName,
                        iljit_ensureCorrectJITFunction,
                        (system->cliManager).CLR.runtimeChecks,
                        buildDelegateFunctionPointer,
                        system->machineCodeLibraryPath);

    /* Initialize the IR virtual machine		*/
    if ((ildjitSystem->IRVM).behavior.profiler) {
        IRVM_init(	&(system->IRVM),
                    &((system->garbage_collectors).gc),
                    iljit_lookupValueType,
                    iljit_getIRMethodFromMethod,
                    ILDJIT_quit,
                    (JITINT8 *)"ILDJIT_quit",
                    iljit_getJITMethodFromMethod,
                    (JITINT32 (*)(void*, void*, ir_instruction_t *))recompiler_with_profile
                 );
    } else {
        IRVM_init(	&(system->IRVM),
                    &((system->garbage_collectors).gc),
                    iljit_lookupValueType,
                    iljit_getIRMethodFromMethod,
                    ILDJIT_quit,
                    (JITINT8 *)"ILDJIT_quit",
                    iljit_getJITMethodFromMethod,
                    (JITINT32 (*)(void*, void*, ir_instruction_t *))recompiler
                 );
    }

    /* Create the methods hash table.
     */
    PDEBUG("BOOTSTRAPPER: Create the <token, binary_info> <-> IR method Hash table\n");
    ((system->cliManager).methods).methods = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, (JITUINT32 (*)(void *))IRMETHOD_hashIRSignature, (JITINT32 (*)(void *, void *))IRMETHOD_equalsIRSignature);
    ((system->cliManager).methods).container = xanList_new(sharedAllocFunction, freeFunction, NULL);
    ((system->cliManager).methods).anonymousContainer = xanHashTable_new(11, 0, sharedAllocFunction, dynamicReallocFunction, freeFunction, hashString, equalsString);
    if (((system->cliManager).methods).container == NULL) {
        print_err("BOOTSTRAPPER: ERROR = Cannot create the hash table to contain all the methods. ", 0);
        freeFunction(system);
        freeFunction((system->cliManager).entryPoint.binary);
        abort();
    }

    /* Initialize the optimizer.
     */
    if (enableCodeGenerator) {
        IROptimizerNew(		&((system->IRVM).optimizer),
                            &((system->IRVM).ir_system),
                            (system->IRVM).behavior.verbose,
                            (system->IRVM).behavior.optimizations,
                            (system->IRVM).behavior.outputPrefix
                      );
    }

    /* Initialize the DLA compiler.
     */
    DLANew(&(system->DLA), &((system->IRVM)));

    /* Initialize the pipeliner.
     */
    if (enableCodeGenerator) {
        init_pipeliner(&(system->pipeliner));
        (system->pipeliner).startPipeliner(&(system->pipeliner));
    }

    /* Make the static memory manager.
     */
    STATICMEMORY_initStaticMemoryManager(&(system->staticMemoryManager));

    /* Initialize lock library.
     */
    ILJITLibLockInit();

    /* Create the code generator.
     */
    if (enableCodeGenerator) {
        CODE_init(&(system->codeGenerator));
    }

    /* Load the CLRs.
     */
    if (enableDynamicCLRs) {
        CLR_loadCLRs();
    }

    return system;
}

t_system * system_bootstrapper_multiapp (t_system *system, int argc, char **argv) {
    ProfileTime startTime;

    /* Check the initial condition		*/
    PDEBUG("BOOTSTRAPPER: Check the initial condition\n");
    initial_checker(argc, argv);

    /* Store the start time			*/
    profilerStartTime(&startTime);

    /* Init the Just in time environment	*/
    PDEBUG("BOOTSTRAPPER: Initializing the system\n");
    if (system_initializer(system, argc, argv) != 0) {
        print_err("BOOTSTRAPPER: ERROR from the system initializer. ", 0);
        freeFunction(system);
        return NULL;
    }
    PDEBUG("BOOTSTRAPPER: Initialized the system\n");

    /* Setup the garbage collector	*/
    setup_garbage_collector(system);

    /* Initialize the profiler		*/
    if ((system->IRVM).behavior.profiler >= 1) {
        memset(&(system->profiler), 0, sizeof(t_profiler));
    }

    /* Initialize the optimizer			*/
    IROptimizerNew(&((system->IRVM).optimizer), &((system->IRVM).ir_system), (system->IRVM).behavior.verbose, (system->IRVM).behavior.optimizations, (system->IRVM).behavior.outputPrefix);

    /* Start the pipeliner				*/
    (system->pipeliner).startPipeliner(&(system->pipeliner));

    /* Profiling				*/
    if ((system->IRVM).behavior.profiler >= 1) {
        JITFLOAT32 wallTime;

        /* Store the stop time			*/
        (system->profiler).bootstrap_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (system->profiler).wallTime.bootstrap_time += wallTime;
    }

    return system;
}

t_system * system_bootstrapper (int argc, char **argv) {
    t_system		*system;
    ProfileTime		startTime;

    /* Store the start time.
     */
    profilerStartTime(&startTime);

    /* Check the initial condition.
     */
    PDEBUG("BOOTSTRAPPER: Check the initial condition\n");
    initial_checker(argc, argv);

    /* Create the system.
     */
    system	= allocateAndInitializeSystem(JITTRUE, JITTRUE, JITTRUE, argc, argv, JITTRUE, JITTRUE);
    assert(system != NULL);

    /* Profiling.
     */
    if ((system->IRVM).behavior.profiler >= 1) {
        JITFLOAT32 wallTime;
        (system->profiler).bootstrap_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (system->profiler).wallTime.bootstrap_time += wallTime;
    }

    return system;
}

static inline void setup_library_path (char **libraryPath, char *envVar, char *defaultPath) {
    char    *path;
    char    *buf;

    /* Assertions.
     */
    assert(libraryPath != NULL);
    assert(envVar != NULL);
    assert(defaultPath != NULL);

    path	= getenv(envVar);
    if (path == NULL) {
        (*libraryPath) 	= strdup(defaultPath);
    } else {
        buf = allocFunction(strlen(path) + strlen(defaultPath) + 5);
        sprintf(buf, "%s", path);
        (*libraryPath)	= buf;
    }
    assert((*libraryPath) != NULL);

    return ;
}

static inline void setup_garbage_collector (t_system *system) {
    t_garbage_collector_plugin      *gc;
    JITNUINT heapSize;
    XanListItem                     *item;
    t_gc_behavior gcBehavior;

    /* Assertions				*/
    assert(system != NULL);

    /* Init the variables			*/
    gc = NULL;
    item = NULL;

    /* Choose one garbage collector		*/
    if (	((system->garbage_collectors).plugins != NULL)			&&
            (xanList_length((system->garbage_collectors).plugins) > 0)	) {
        item = xanList_first((system->garbage_collectors).plugins);
        assert(item != NULL);
        if ((system->IRVM).behavior.gcName != NULL) {
            while (item != NULL) {
                gc = ((t_garbage_collector *) xanList_getData(item))->plugin;
                assert(gc != NULL);
                if (strcmp(gc->getName(), (system->IRVM).behavior.gcName) == 0) {
                    break;
                }
                item = item->next;
            }
        }
        if (item == NULL) {
            print_err("No Garbage collector found. ", 0);
            exit(1);
        }
        assert(item != NULL);
        gc = ((t_garbage_collector *) xanList_getData(item))->plugin;
        assert(gc != NULL);
        assert(gc->allocObject != NULL);
        assert(gc->init != NULL);
        assert(gc->shutdown != NULL);
        assert(gc->collect != NULL);
        assert(gc->gcInformations != NULL);
        (system->garbage_collectors).gc.init = gc->init;
        (system->garbage_collectors).gc.shutdown = gc->shutdown;
        (system->garbage_collectors).gc.getSupport = gc->getSupport;
        (system->garbage_collectors).gc.gcInformations = gc->gcInformations;
        (system->garbage_collectors).gc.gc_plugin = gc;
        assert((system->garbage_collectors).gc.gc_plugin != NULL);
        assert(((system->garbage_collectors).gc.gc_plugin)->allocObject != NULL);

    } else {

        /* No garbage collector is going to be used.
         */
        (system->garbage_collectors).gc.gc_plugin			= (t_garbage_collector_plugin *) allocFunction(sizeof(t_garbage_collector_plugin));
        ((system->garbage_collectors).gc.gc_plugin)->allocObject	= (void * (*) (JITNUINT))allocFunction;
    }

    /* Set the heap size.
     */
    if ((system->IRVM).behavior.heapSize > 0) {
        heapSize = (system->IRVM).behavior.heapSize;
    } else {
        heapSize = DEFAULT_HEAP_SIZE;
    }
    if ((system->IRVM).behavior.profiler >= 2) {
        gcBehavior.profile = 1;
    } else {
        gcBehavior.profile = 0;
    }
    gcBehavior.verbose = (system->IRVM).behavior.verbose;
    gcBehavior.isCollectable = isCollectable;
    gcBehavior.sizeObject = sizeObject;
    gcBehavior.getReferencedObject = getReferencedObject;
    gcBehavior.getRootSet = getRootSet;
    gcBehavior.callFinalizer = callFinalizer;

    /* Call the initialization of the chosen garbage collector.
     */
    if (gc != NULL) {
        (system->garbage_collectors).gc.init(&gcBehavior, heapSize);
    }

    /* Setup the garbage collectors interactions.
     */
    setup_garbage_collector_interaction();

    return;
}

static inline void internal_system_shutdown (t_system *system) {
    XanList 			*methods;
    XanHashTable 			*anonymousMethods;
    XanListItem     		*item;
    XanHashTableItem 		*hashItem;
    JITFLOAT32 			totalTime;

    /* Assertions.
     */
    assert(system != NULL);
    PDEBUG("SYSTEM MANAGER: system_shutdown: Start\n");

    /* Profiling.
     */
    if ((system->IRVM).behavior.profiler >= 1) {
        (system->profiler).total_time = profilerGetTimeElapsed(&((system->profiler).startTime), &((system->profiler).wallTime.total_time));
    }

    /* Fetch the total time.
     */
    if ((system->IRVM).behavior.aot) {
        totalTime = (system->profiler).wallTime.execution_time;
    } else {
        totalTime = (system->profiler).total_time;
    }

    /* Destroy the optimizer.
     */
    IROPTIMIZER_shutdown(&((system->IRVM).optimizer), totalTime);

    /* Unload the CLRs.
     */
    CLR_unloadCLRs();

    /* Fetch the set of methods.
     */
    methods = ((system->cliManager).methods).container;
    assert(methods != NULL);
    anonymousMethods = ((system->cliManager).methods).anonymousContainer;
    assert(anonymousMethods != NULL);

    /* Free the objects	*/
    PDEBUG("SYSTEM MANAGER: system_shutdown:        Shutdown the Garbage Collectors\n");
    if ((system->garbage_collectors).gc.shutdown != NULL) {
        (system->garbage_collectors).gc.shutdown();
    }
    if ((system->exception_system)._OutOfMemoryException != NULL) {
        (ildjitSystem->garbage_collectors).gc.freeObject((system->exception_system)._OutOfMemoryException);
    }

    /* Destroy the pipeline			*/
    PDEBUG("SYSTEM MANAGER: system_shutdown:        Shutdown the translation pipeliner\n");
    if ((system->pipeliner).destroyThePipeline != NULL) {
        (system->pipeliner).destroyThePipeline(&(system->pipeliner));
    }

    /* Close the files			*/
    if ((system->IRVM).behavior.dumpAssembly.dumpAssembly) {
        PDEBUG("SYSTEM MANAGER: system_shutdown:        Close the dump files\n");
        fclose((system->IRVM).behavior.dumpAssembly.dumpFileIR);
        fclose((system->IRVM).behavior.dumpAssembly.dumpFileJIT);
        fclose((system->IRVM).behavior.dumpAssembly.dumpFileAssembly);
    }

    /* Save Trace Profile	*/
    if (    (!(system->IRVM).behavior.aot)          &&
            ((system->IRVM).behavior.pgc == 1)      ) {
        MANFRED_saveProfile(&(system->manfred), NULL);
    }

    /* Print the profiling informations	*/
    if ((system->IRVM).behavior.profiler >= 1) {
        printProfilerInformation();
    }

    /* Destroy manfred.
     */
    MANFRED_shutdown(&(system->manfred));

    /* Delete the memory allocated for the managers.
     */
    CLIMANAGER_shutdown(&(system->cliManager));

    /* free the memory associated with the thread_exception_infos instance */
    if ((system->program).thread_exception_infos != NULL) {
        freeMemory((system->program).thread_exception_infos);
    }

    /* Destroy the static memory manager	*/
    STATICMEMORY_shutdownMemoryManager(&(system->staticMemoryManager));

#ifndef MULTIAPP

    /* Free methods compatibility map.
     */
    hashItem = xanHashTable_first(((system->cliManager).methods).methods);
    while (hashItem != NULL) {
        XanList         *compatibleMethods;
        ir_signature_t 	*irSignature;

        /* Fetch the element.
         */
        compatibleMethods = hashItem->element;
        assert(compatibleMethods != NULL);
        irSignature	= hashItem->elementID;
        assert(irSignature != NULL);

        /* Free the memory.
         */
        xanList_destroyList(compatibleMethods);
        if (irSignature->parameterTypes != NULL) {
            freeFunction(irSignature->parameterTypes);
        }
        freeFunction(irSignature);

        hashItem = xanHashTable_next(((system->cliManager).methods).methods, hashItem);
    }
    xanHashTable_destroyTable(((system->cliManager).methods).methods);
    ((system->cliManager).methods).methods = NULL;

    /* Free the anonymous methods.
     */
    hashItem  = xanHashTable_first(anonymousMethods);
    while (hashItem != NULL) {
        Method method;
        method = (Method) hashItem->element;
        assert(method != NULL);
        assert(xanList_find(methods, method) == NULL);
        method->destroyMethod(method);
        hashItem  = xanHashTable_first(anonymousMethods);
    }
    xanHashTable_destroyTable(anonymousMethods);
    ((system->cliManager).methods).anonymousContainer	= NULL;

    /* Free the methods.
     */
    item = xanList_first(methods);
    while (item != NULL) {
        Method method;
        method = (Method) item->data;
        assert(method != NULL);
        method->destroyMethod(method);
        assert(xanList_find(methods, method) == NULL);
        item = xanList_first(methods);
    }
    xanList_destroyList(methods);
    ((system->cliManager).methods).container	= NULL;

    /* Shutdown the IR virtual machine.
     */
    IRVM_shutdown(&(system->IRVM));

    /* Free the system decoders		*/
    unload_plugins(system);
#endif

    /* Free the garbage collectors		*/
    if ((system->garbage_collectors).plugins != NULL) {
        xanList_destroyListAndData((system->garbage_collectors).plugins);
        (system->garbage_collectors).plugins	= NULL;
    }

    /* Shutdown the compiler memory manager */
    MEMORY_shutdown(&(system->compilerMemoryManager));

    /* Destroy the code generator.
     */
    CODE_shutdown(&(system->codeGenerator));
    freeFunction(system->cilLibraryPath);
    freeFunction(system->machineCodeLibraryPath);

    /* Shutdown the IR manager.
     */
#ifndef MULTIAPP
    IRLibDestroy(&((system->IRVM).ir_system));
#endif

    /* Free system memory	*/
    PDEBUG("SYSTEM MANAGER: system_shutdown:        Free the system memory\n");
#ifdef DEBUG
    memset(system, 0, sizeof(t_system));
#endif
    freeFunction(system->programName);
    if ((system->IRVM).behavior.outputPrefix != NULL) {
        freeFunction((system->IRVM).behavior.outputPrefix);
    }
    freeFunction(system);

    PDEBUG("SYSTEM MANAGER: system_shutdown: Exit\n");
    return;
}

t_methods * getMethods (t_system *system) {
    return &((system->cliManager).methods);
}

static inline Method libclimanager_fetchOrCreateMethod (MethodDescriptor *methodID) {
    t_methods	*methods;
    methods	= &((ildjitSystem->cliManager).methods);
    return fetchOrCreateMethod(methods, methodID, JITFALSE);
}

static inline Method fetchOrCreateMethod (t_methods *methods, MethodDescriptor *methodID, JITBOOLEAN isExternallyLinkable) {
    DescriptorMetadataTicket 	*ticket;
    Method 				method;

    /* Assertions.
     */
    assert(methods != NULL);
    assert(methodID != NULL);
    PDEBUG("SYSTEM_MANAGER: fetchOrCreateMethod: Start\n");

    /* Initialize the variables	*/
    method = NULL;

    /* Fetch the method		*/
    ticket = methodID->createDescriptorMetadataTicket(methodID, METHOD_METADATA);

    /* Check if the method is already made	*/
    if (ticket->data == NULL) {
        PDEBUG("SYSTEM_MANAGER: fetchOrCreateMethod:            The method is not found\n");

        /* Create a temporany method			*/
        PDEBUG("SYSTEM_MANAGER: fetchOrCreateMethod:    Make it\n");
        method = (ildjitSystem->cliManager).translator.newMethod(&(ildjitSystem->cliManager), methodID, isExternallyLinkable);
        assert(method != NULL);
        assert(IRMETHOD_getMethodMetadata(&(method->IRMethod), METHOD_METADATA) == method);

        /* Insert the new method into the container	*/
        PDEBUG("SYSTEM_MANAGER: fetchOrCreateMethod:    Insert the new method into the container\n");
        xanList_syncAppend(methods->container, method);
        xanHashTable_lock(methods->methods);
        XanList *compatibleMethods = xanHashTable_lookup(methods->methods, &((method->IRMethod).signature));
        if (compatibleMethods == NULL) {
            ir_signature_t *irSignature;
            irSignature = sharedAllocFunction(sizeof(ir_signature_t));
            memcpy(irSignature, &((method->IRMethod).signature), sizeof(ir_signature_t));
            if (irSignature->parametersNumber > 0) {
                irSignature->parameterTypes = sharedAllocFunction(sizeof(JITUINT32) * (irSignature->parametersNumber));
                memcpy(irSignature->parameterTypes, (method->IRMethod).signature.parameterTypes, sizeof(JITUINT32) * (irSignature->parametersNumber));
            }
            compatibleMethods = xanList_new(sharedAllocFunction, freeFunction, NULL);
            xanHashTable_insert(methods->methods, irSignature, compatibleMethods);
        }
        assert(compatibleMethods != NULL);
        assert(xanList_find(compatibleMethods, method) == NULL);
        xanList_append(compatibleMethods, method);
        xanHashTable_unlock(methods->methods);

        if (methodID->attributes->is_internal_call) {
            CLR_buildMethod(method);

        } else if (!(ildjitSystem->manfred).isLoading) {
            /* Prepare method body, if needed                 */

            ir_method_t *body = method->getIRMethod(method);
            IRMETHOD_addMethodDummyInstructions(body);

            if (methodID->attributes->is_provided_by_the_runtime) {
                IRMETHOD_addMethodDummyInstructions(body);
                (ildjitSystem->cliManager).CLR.delegatesManager.buildMethod(&((ildjitSystem->cliManager).CLR.delegatesManager), method);

            } else if (methodID->attributes->is_pinvoke) {
                IRMETHOD_addMethodDummyInstructions(body);
                (ildjitSystem->cliManager).CLR.pinvokeManager.buildMethod(&((ildjitSystem->cliManager).CLR.pinvokeManager), method);
            }
        }

        methodID->broadcastDescriptorMetadataTicket(methodID, ticket, method);

        /* If we are in AOT compilation	we insert the	*
         * method into the compilation pipeline		*/
        if (    (!(ildjitSystem->manfred).isLoading)                                  &&
                ((ildjitSystem->IRVM).behavior.aot)                                   &&
                (!(ildjitSystem->IRVM).behavior.pgc)                                  &&
                (!MANFRED_isProgramPreCompiled(&(ildjitSystem->manfred)))		&&
                (method->isIrImplemented(method))		 		&&
                (!method->isCctor(method))   					) {

            /* Compile the method				*/
            (ildjitSystem->pipeliner).insertMethod(&(ildjitSystem->pipeliner), method, MIN_METHOD_PRIORITY);
        }
    }
    assert(ticket->data != NULL);
    assert(IRMETHOD_getMethodMetadata(&(((Method)ticket->data)->IRMethod), METHOD_METADATA) == ticket->data);

    /* Return the new method			*/
    PDEBUG("SYSTEM_MANAGER: fetchOrCreateMethod: Exit\n");
    return (Method) ticket->data;
}

char * get_prefix () {
    return prefix;
}
char * get_library_path () {
    return library_path;
}
char * get_path_plugin () {
    return path_plugin;
}
char * get_path_garbage_collector_plugin () {
    return path_garbage_collector_plugin;
}
char * get_cil_path () {
    return cil_path;
}

char * get_machinecode_path () {
    return machinecode_path;
}

char * get_path_clr_plugin () {
    return path_clr_plugin;
}

void init_paths () {
    JITINT8		*pathToUse;
    JITINT8		**pathToUsePtr;

#ifdef WIN32    // Windows
    strcpy(prefix, WIN_DEFAULT_PREFIX);

    sprintf(library_path, "%s\\lib\\cscc\\lib", prefix);
    sprintf(path_plugin, "%s\\lib\\iljit\\loaders\\", prefix);
    sprintf(path_garbage_collector_plugin, "%s\\lib\\iljit\\gc\\", prefix);
    sprintf(cil_path, ".;%s\\lib\\cscc\\lib\\", prefix);
    sprintf(machinecode_path, ".");
#else
    if (PLATFORM_readlink("/proc/self/exe", prefix, sizeof(prefix)) == -1) {
        abort();
    }
    PLATFORM_dirname(PLATFORM_dirname(prefix));

    sprintf(library_path, "%s/lib/cscc/lib", prefix);
    sprintf(path_plugin, "%s/lib/iljit/loaders/", prefix);
    sprintf(path_garbage_collector_plugin, "%s/lib/iljit/gc/", prefix);
    sprintf(cil_path, ".:%s/lib/cscc/lib", prefix);
    sprintf(machinecode_path, ".");
#endif

    pathToUse	= (JITINT8 *)path_clr_plugin;
    pathToUsePtr	= &pathToUse;
    PLUGIN_loadPluginDirectoryNamesToUse((JITINT8 *)get_prefix(), pathToUsePtr, (JITINT8 *)"ILDJIT_CLR_PLUGINS", (JITINT8 *)"clrs");

    return ;
}

t_binary_information * ILDJIT_loadAssembly (JITINT8 *assemblyName) {
    return loadAssembly(ildjitSystem, assemblyName);
}
