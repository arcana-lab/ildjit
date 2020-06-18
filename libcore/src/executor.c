/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
#include <string.h>
#include <errno.h>
#include <ir_language.h>
#include <platform_API.h>

// My headers
#include <code_generator.h>
#include <exception_manager.h>
#include <iljit.h>
#include <system_manager.h>
#include <cil_ir_translator.h>
#include <executor.h>
#include <ildjit_profiler.h>
// End

static inline void compile_entry_point (t_system *system);
static inline void * execute (void *system_void);
static inline void internal_generate_machine_code (t_methods *methods, TranslationPipeline *pipeliner);

extern t_system *ildjitSystem;

void startExecutor (t_system *system) {

    /* Assertions.
     */
    assert(system != NULL);

    /* Compile the Main method.
     */
    compile_entry_point(system);

    /* Save Trace Profile.
     */
    if (	(!(system->IRVM).behavior.aot) 		&&
            ((system->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(system->manfred), (system->program).entry_point);
    }

    /* Create the thread object.
     */
    if (	(((system->IRVM).behavior.pgc == 1) || ((system->IRVM).behavior.aot))	&&
            (!(system->program).disableExecution)					) {
       (system->cliManager).CLR.threadsManager.getCurrentThread();
    }

    /* Notify the system that we are going to execute the entry point.
     */
    if (!(system->program).disableExecution) {
        IROPTIMIZER_startExecution(&((system->IRVM).optimizer));
    }

    /* Run the executor.
     */
    if ((system->program).spawnNewThreadAsMainCodeExecutor) {
        pthread_t executorThreadID;
        PLATFORM_createThread(&executorThreadID, NULL, execute, system);
        PLATFORM_joinThread(executorThreadID, NULL);

    } else {
        execute(system);
    }

    return;
}

static inline void * execute (void *system_void) {
    t_system                *system;
    JITINT32 		argc;
    char                    **argv;
    Method			method;
    JITINT32 		error;
    JITINT32 		resultInt;
    JITBOOLEAN		profileExecution;
    void                    *args_array[2];

    /* Assertions.
     */
    assert(system_void != NULL);
    PDEBUG("EXECUTOR: Start the executor thread\n");

    /* Init the variables.
     */
    profileExecution	= JITFALSE;
    error 			= 0;
    args_array[0] 		= NULL;
    resultInt 		= -1;

    /* Get the system.
     */
    system = (t_system *) system_void;
    assert(system != NULL);

    /* Check if we need to execute the code.
     */
    if ((system->program).disableExecution) {
        return NULL;
    }

    /* Lookup the IR method.
     */
    PDEBUG("EXECUTOR: Fetch the IR method\n");
    method		= (ildjitSystem->program).wrapperEntryPoint;

    /* Make the arguments.
     * Eliminate the name of the bytecode.
     */
    argc = (system->arg).argc;
    if (argc > 0) {
        argv = &((system->arg).argv[0]);
    } else {
        argv = NULL;
    }

    /* Profiling.
     */
    if (	((system->IRVM).behavior.aot)    	&&
            ((system->IRVM).behavior.profiler >= 1)	) {
        profilerStartTime(&((system->profiler).startExecutionTime));
        profileExecution	= JITTRUE;
    }

    /* Start the execution.
     */
    PDEBUG("EXECUTOR: Start the execution\n");
    args_array[0] 	= &argc;
    args_array[1] 	= &argv;
    error 		= IRVM_run(&(system->IRVM), *(method->jit_function), args_array, &(resultInt));

    /* Wait for other CIL threads.
     */
    (system->cliManager).CLR.threadsManager.waitThreads();

    /* Profiling.
     */
    if (profileExecution) {
        (system->profiler).execution_time = profilerGetTimeElapsed(&((system->profiler).startExecutionTime), &((system->profiler).wallTime.execution_time));
    }

    /* Check the return value		*/
    if (error == 0) {

        /* AN EXCEPTION WAS THROWN AND NEVER CATCHED */
        PDEBUG("EXECUTOR:       An exception occurs. \n");
        print_stack_trace(system);
        (system->program).result = -1;
    } else {

        PDEBUG("EXECUTOR:       Return value %d\n", resultInt);
        (system->program).result = resultInt;
    }

    PDEBUG("EXECUTOR: Exit\n");
    return NULL;
}

static inline void compile_entry_point (t_system *system) {
    TranslationPipeline     *pipeliner;
    ILLayout_manager	*layoutManager;
    Method 			entryPoint;
    ProfileTime 		startTime;

    /* Assertions.
     */
    assert(system != NULL);

    /* Fetch the start time.
     */
    profilerStartTime(&startTime);

    /* Fetch the pipeliner.
     */
    pipeliner 	= &(system->pipeliner);
    assert(pipeliner != NULL);

    /* Fetch the layout manager.
     */
    layoutManager	= &((system->cliManager).layoutManager);

    /* Fetch the entry point.
     */
    entryPoint 					= (system->program).entry_point;
    entryPoint->executionProbability 		= 1;

    /* Notify the IRVM that we are starting the pre-compilation.
     */
    IRVM_startPreCompilation(&(system->IRVM));

    /* Initialize each CLR module.
     */
    if (	((system->IRVM).behavior.aot)			||
            ((system->IRVM).behavior.staticCompilation)	||
            ((system->IRVM).behavior.pgc)			) {
        CLIMANAGER_initializeCLR();
    }

    /* AOT compiler.
     */
    if (	((system->IRVM).behavior.aot)			||
            ((system->IRVM).behavior.staticCompilation)	) {
        t_methods 	*methods;
        JITBOOLEAN	saveProgram;

        /* Initialize the variables.
         */
        saveProgram	= JITFALSE;
        assert(entryPoint != NULL);

        /* Fetch the methods.
         */
        methods = &((system->cliManager).methods);

        /* Check if we need to perform something.
         */
        if (!MANFRED_isProgramPreCompiled(&(system->manfred))) {

            /* Load all method of entry point assembly.
             */
            BasicAssembly *assembly = ((system->program).entry_point)->getAssembly((system->program).entry_point);
            assert(assembly != NULL);
            XanList *assemblyMethods = (system->cliManager).metadataManager->getMethodsFromAssembly((system->cliManager).metadataManager, assembly);
            assert(assemblyMethods != NULL);

            /* Compile all method of entry point assembly.
             */
            XanListItem *item = xanList_first(assemblyMethods);
            while (item != NULL) {
                MethodDescriptor *methodID = (MethodDescriptor *) item->data;
                assert(methodID != NULL);
                if (methodID->attributes->param_count == 0 && methodID->owner->attributes->param_count == 0) {
                    Method current_method = methods->fetchOrCreateMethod(methods, methodID, JITFALSE);
                    assert(current_method != NULL);
                    if (!current_method->isCctor(current_method) && current_method->isIrImplemented(current_method)) {
                        pipeliner->insertMethod(pipeliner, current_method, MIN_METHOD_PRIORITY);
                    }
                }
                item = item->next;
            }

            /* Wait the AOT compiler		*/
            pipeliner->waitEmptyPipeline(pipeliner);

            /* Optimize the method			*/
            IROPTIMIZER_optimizeMethodAtAOTLevel(&((system->IRVM).optimizer), entryPoint->getIRMethod(entryPoint));

            /* Translate the method to machine code	*/
            entryPoint->setState(entryPoint, IR_STATE);
            saveProgram	= JITTRUE;

        } else if ((system->IRVM).behavior.pgc) {

            /* Optimize the method			*/
            IROPTIMIZER_optimizeMethodAtAOTLevel(&((system->IRVM).optimizer), entryPoint->getIRMethod(entryPoint));

            /* Translate the method to machine code	*/
            entryPoint->setState(entryPoint, IR_STATE);
            saveProgram	= JITTRUE;
        }

        /* Erase the modified flags.
         */
        XanListItem *item = xanList_first(methods->container);
        while (item!=NULL) {
            Method method;
            ir_method_t *irMethod;
            method = (Method) item->data;
            assert(method != NULL);
            irMethod = method->getIRMethod(method);
            irMethod->modified = JITFALSE;
            item = item->next;
        }
        XanHashTableItem *hashItem = xanHashTable_first(methods->anonymousContainer);
        while (hashItem!=NULL) {
            Method 		method;
            ir_method_t	*irMethod;

            /* Fetch the method.
             */
            method 		= (Method) hashItem->element;
            assert(method != NULL);
            irMethod 	= method->getIRMethod(method);
            assert(irMethod != NULL);

            /* Set the flag.
             */
            irMethod->modified = JITFALSE;

            hashItem = xanHashTable_next(methods->anonymousContainer, hashItem);
        }

        /* Postponed the generation of machine code.
         */
        CODE_cacheCodeGeneration(&(system->codeGenerator));

        /* Postponed the execution of cctors.
         */
        STATICMEMORY_cacheConstructorsToCall(&(system->staticMemoryManager));

        /* Optimize the methods.
         */
        IROPTIMIZER_optimizeMethodAtPostAOTLevel(&((system->IRVM).optimizer), entryPoint->getIRMethod(entryPoint));

        /* Create the symbols (and correspondent files in the IR code cache) of all types.
         * This step is necessary to force the creation of all IR symbol files necessary for later use.
         */
        item			= xanList_first(layoutManager->classesLayout);
        while (item != NULL) {
            ILLayout		*layout;
            layout			= item->data;
            assert(layout != NULL);
            assert(layout->type != NULL);
            (system->cliManager).translator.getTypeDescriptorSymbol(&(system->cliManager), layout->type);
            item			= item->next;
        }

        /* Save the program.
         */
        if (saveProgram) {
            MANFRED_saveProgram(&(system->manfred), NULL);
        }

        /* Generate the wrapper of the entry point.
         */
        (system->program).wrapperEntryPoint		= CODE_generateWrapperOfEntryPoint(&(system->codeGenerator));

        /* Compile the IR code for all IR methods.
         */
        internal_generate_machine_code(methods, pipeliner);

        /* Generate the machine code and execute the cctors.
         */
        CODE_generateAndLinkCodeForCachedMethods(&(system->codeGenerator));

        /* Destroy the pipeline.
         */
        if ((system->IRVM).behavior.onlyPrecompilation) {
            (system->pipeliner).destroyThePipeline(&(system->pipeliner));
        }

        /* Erase the trampolines information.
         */
        (system->profiler).trampolinesTakenBeforeEntryPoint = (system->profiler).trampolinesTaken;
        (system->profiler).trampolinesTaken = 0;
        (system->profiler).trampolines_time = 0;

        /* Notify the IRVM that we finished the pre-compilation.
         */
        IRVM_finishPreCompilation(&(system->IRVM));

        return ;
    }

    if ((system->IRVM).behavior.pgc == 2) {
        XanList *methods = MANFRED_loadProfile(&(system->manfred), NULL);
        JITFLOAT32 priorityDec = 1.0 / (xanList_length(methods));
        JITFLOAT32 priority = 1.0;
        XanListItem *item = xanList_first(methods);
        while (item != NULL) {
            Method method = (Method) item->data;
            pipeliner->insertMethod(pipeliner, method, priority);
            item = item->next;
            priority -= priorityDec;
        }
        pipeliner->synchInsertMethod(pipeliner, entryPoint, MAX_METHOD_PRIORITY);

    } else {

        pipeliner->synchInsertMethod(pipeliner, entryPoint, MAX_METHOD_PRIORITY);
        assert(entryPoint->getState(entryPoint) == EXECUTABLE_STATE);

        /* Profiling				*/
        if ((system->IRVM).behavior.profiler >= 1) {
            JITFLOAT32 wallTime;

            /* Fetch the stop time			*/
            (system->profiler).cil_loading_time += profilerGetTimeElapsed(&startTime, &wallTime);
            (system->profiler).wallTime.cil_loading_time += wallTime;
        }
    }

    /* Generate the wrapper of the entry point.
     */
    (system->program).wrapperEntryPoint			= CODE_generateWrapperOfEntryPoint(&(system->codeGenerator));
    (system->program).wrapperEntryPoint->executionProbability	= 1;

    /* Compile the wrapper.
     */
    pipeliner->synchInsertMethod(pipeliner, (system->program).wrapperEntryPoint, MAX_METHOD_PRIORITY);

    /* Notify the IRVM that we finished the pre-compilation.
     */
    IRVM_finishPreCompilation(&(system->IRVM));

    return;
}

static inline void internal_generate_machine_code (t_methods *methods, TranslationPipeline *pipeliner) {
    XanListItem		*item;
    XanHashTableItem	*hashItem;

    /* Generate machine code for the entry point.
     */
    pipeliner->synchInsertMethod(pipeliner, (ildjitSystem->program).entry_point, MAX_METHOD_PRIORITY);

    item = xanList_last(methods->container);
    while (item!=NULL) {
        Method method;
        ir_method_t *irMethod;

        /* Fetch the method			*/
        method = (Method) item->data;
        irMethod = method->getIRMethod(method);

        /* Check if we need to compile the	*
         * method				*/
        if (    (method->isIrImplemented(method))       	&&
                (!method->isCctor(method))              	&&
                (IRMETHOD_hasMethodInstructions(irMethod))	) {

            /* Change the state			*/
            if (irMethod->modified) {
                assert((method->getState(method) == IR_STATE) || (method->getState(method) == EXECUTABLE_STATE));
                method->setState(method, IR_STATE);
            }

            /* Generate the code			*/
            pipeliner->insertMethod(pipeliner, method, MAX_METHOD_PRIORITY);
        }

        /* Fetch the next element		*/
        item = item->prev;
    }

    hashItem = xanHashTable_first(methods->anonymousContainer);
    while (hashItem!=NULL) {
        Method 		method;
        ir_method_t	*irMethod;

        /* Fetch the method.
         */
        method 		= (Method) hashItem->element;
        assert(method != NULL);
        irMethod = method->getIRMethod(method);

        if (    (method->isIrImplemented(method))       	&&
                (!method->isCctor(method))              	&&
                (IRMETHOD_hasMethodInstructions(irMethod))	) {

            /* Change the state			*/
            if (irMethod->modified) {
                assert((method->getState(method) == IR_STATE) || (method->getState(method) == EXECUTABLE_STATE));
                method->setState(method, IR_STATE);
            }

            /* Generate the code			*/
            pipeliner->insertMethod(pipeliner, method, MAX_METHOD_PRIORITY);
        }

        hashItem = xanHashTable_next(methods->anonymousContainer, hashItem);
    }

    /* Wait the AOT compiler		*/
    pipeliner->waitEmptyPipeline(pipeliner);

    return ;
}
