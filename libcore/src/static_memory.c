/*
 * Copyright (C) 2008 - 2012 Campanoni Simone
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
#include <xanlib.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <ilmethod.h>
#include <base_symbol.h>

// My headers
#include <system_manager.h>
#include <static_memory.h>
#include <iljit.h>
// End

typedef struct {
    Method 		method;
    void            *obj;
    pthread_t 	owner;
    JITBOOLEAN 	executionDone;
    JITBOOLEAN 	inExecution;
    pthread_mutex_t lock;
    pthread_cond_t 	condition;
} t_methodToCall;

extern t_system *ildjitSystem;

static inline ir_symbol_t * internal_fetchStaticMemorySymbol (TypeDescriptor *type);
static inline JITINT32 internal_are_equal_threads (pthread_t *k1, pthread_t *k2);
static inline Method internal_fetchStaticConstructor (StaticMemoryManager *manager, MethodDescriptor *methodID);
static inline void * internal_create_static_object (StaticMemoryManager *manager, TypeDescriptor *type);
static inline void internal_registerCompilationNeeded (StaticMemoryManager* self, pthread_t thread, Method method);
static inline void internal_registerCompilationDone (StaticMemoryManager* self, pthread_t thread);
static inline void internal_callStaticConstructorIfNeeded (StaticMemoryManager* self, Method cctor);
static inline void internal_makeStaticConstructorExecutable (StaticMemoryManager* self, Method cctor, JITINT32 priority);
static inline void internal_callStaticConstructor (StaticMemoryManager* self, t_methodToCall* cctorDescription);
static inline void internal_waitStaticConstructor (StaticMemoryManager* self, t_methodToCall* cctorDescription);
static inline JITBOOLEAN internal_executionIsDone (StaticMemoryManager* self, t_methodToCall* cctorDescription);
static inline JITBOOLEAN internal_tryDeclaringWaiting (StaticMemoryManager* self, pthread_t waitingThread, Method method);
static inline ir_value_t static_object_resolve (ir_symbol_t *symbol);
static inline void static_object_serialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated);
static inline void static_object_dump (ir_symbol_t *symbol, FILE *fileToWrite);
static inline ir_symbol_t *  static_object_deserialize (void *mem, JITUINT32 memBytes);
static inline void internal_callCctorMethod (StaticMemoryManager* self, Method cctor);

void STATICMEMORY_callCctorMethod (StaticMemoryManager* self, Method cctor) {
    TranslationPipeline* pipeline;

    /* Cache some pointers.
     */
    pipeline = &(ildjitSystem->pipeliner);

    /* Generate the machine code.
     */
    pipeline->synchInsertMethod(pipeline, cctor, MAX_METHOD_PRIORITY);

    /* Invoke the method.
     */
    internal_callStaticConstructorIfNeeded(self, cctor);

    return ;
}

ir_symbol_t * STATICMEMORY_fetchStaticObjectSymbol (StaticMemoryManager *manager, Method method, TypeDescriptor *type) {
    ir_symbol_t 	*symbol;

    /* Assertions.
     */
    assert(manager != NULL);
    assert(type != NULL);
    assert(method != NULL);

    /* Fetch the symbol representing the static object of the class "type".
     */
    symbol 	= internal_fetchStaticMemorySymbol(type);
    assert(symbol != NULL);

    /* Fetch the static constructor of the current static object.
     */
    MethodDescriptor *cctorMethodID = type->getCctor(type);
    if (cctorMethodID != NULL) {
        Method cctor = internal_fetchStaticConstructor(manager, cctorMethodID);
        if (cctor != method) {
            method->lock(method);
            cctor->lock(cctor);
            if (cctor->executionProbability < 0 || (cctor->executionProbability < method->executionProbability) ) {
                cctor->executionProbability = method->executionProbability;
            }
            cctor->unlock(cctor);
            method->unlock(method);
        }
        method->addCctorToCall(method, cctor);
    }

    return symbol;
}

static inline void internal_callCctorMethod (StaticMemoryManager* self, Method cctor) {

    /* Type owning the CCTOR				*/
    TypeDescriptor  	*type;

    /* Static Object					*/
    void 			*obj;
    void                    *args[1];

    /* Prepare Arguments					*/
    type = cctor->getType(cctor);
    obj = internal_create_static_object(self, type);
    args[0] = &(obj);

    /* Tag the method					*/
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), cctor);
    }

    /* Check if the execution of code is allowed.
     */
    if ((ildjitSystem->program).disableExecution) {
        return ;
    }

    /* Initialize the static memory.
     */
    IRVM_run(&(ildjitSystem->IRVM), *(cctor->jit_function), args, NULL);

    return ;
}

void STATICMEMORY_resetStaticMemoryManager (StaticMemoryManager *manager) {

    /* Shutdown the memory manager.
     */
    STATICMEMORY_shutdownMemoryManager(manager);

    /* Create the memory manager.
     */
    manager->cctorMethodsExecuted = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    manager->fetchStaticConstructor = internal_fetchStaticConstructor;
    manager->registerCompilationNeeded = internal_registerCompilationNeeded;
    manager->registerCompilationDone = internal_registerCompilationDone;
    manager->compilationOfMethodLeadsToADeadlock = internal_tryDeclaringWaiting;

    /* Setup the static object					*/
    manager->staticObjects 	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    manager->symbolTable	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Setup wait method and translating set */
    manager->waitMethodSet = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, PLATFORM_threadHashFunc, (int (*)(void *, void *))internal_are_equal_threads);
    manager->translatingSet = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    manager->cachedConstructors = xanList_new(allocFunction, freeFunction, NULL);

    return ;
}

void STATICMEMORY_initStaticMemoryManager (StaticMemoryManager *manager) {
    STATICMEMORY_resetStaticMemoryManager(manager);

    IRSYMBOL_registerSymbolManager(STATIC_OBJECT_SYMBOL, static_object_resolve, static_object_serialize, static_object_dump, static_object_deserialize);

    return ;
}

/* Equal Function for xanHashTables used with pthread (needed for multi-platform compatibility) */
static inline JITINT32 internal_are_equal_threads (pthread_t *k1, pthread_t *k2) {
    pthread_t* t1 = (pthread_t  *) k1;
    pthread_t* t2 = (pthread_t *) k2;

    if (t1 == t2) {
        return JITTRUE;
    }
    if (t1 == NULL || t2 == NULL) {
        return JITFALSE;
    }
    return PLATFORM_equalThread(*t1, *t2);
}

/* Compile static constructors by try taking an initialization lock */
void STATICMEMORY_makeStaticConstructorsExecutable (StaticMemoryManager* self, XanList* cctors, JITFLOAT32 priority) {

    /* Contains a cctor description inside the 	*
     * XanList 					*/
    XanListItem* cctorContainer;

    /* Quit now if static constructiors are disabled*/
    if ((ildjitSystem->IRVM).behavior.disableStaticConstructors) {
        return ;
    }

    /* Make (if possible) all cctors executables    */
    cctorContainer = xanList_first(cctors);
    while (cctorContainer != NULL) {
        Method cctor;

        /* Fetch the method				*/
        cctor = cctorContainer->data;
        assert(cctor != NULL);

        /* Compile the cctor				*/
        internal_makeStaticConstructorExecutable(self, cctor, priority);

        /* Fetch the next element			*/
        cctorContainer = cctorContainer->next;
    }

    return;
}

void STATICMEMORY_callStaticConstructors (StaticMemoryManager* self, XanList* cctors) {

    /* Contains a cctor description */
    XanListItem* cctorContainer;

    /* Description of a cctor */
    Method cctor;

    /* Quit now if static constructiors are disabled*/
    if ((ildjitSystem->IRVM).behavior.disableStaticConstructors) {
        return ;
    }

    /* Call all needed type constructors */
    cctorContainer = xanList_first(cctors);
    while (cctorContainer != NULL) {
        cctor = (Method) cctorContainer->data;
        internal_callStaticConstructorIfNeeded(self, cctor);
        cctorContainer = cctorContainer->next;
    }

    return ;
}

JITINT32 STATICMEMORY_areStaticConstructorsExecutable (XanList *methods) {
    XanListItem     *item;
    Method cctor;
    JITINT32 executionReady;
    JITUINT32 methodState;

    /* Assertions					*/
    assert(methods != NULL);

    /* Quit now if static constructiors are disabled*/
    if ((ildjitSystem->IRVM).behavior.disableStaticConstructors) {
        return JITFALSE;
    }

    executionReady = 1;
    item = xanList_first(methods);
    while (item != NULL) {
        cctor = (Method) item->data;
        assert(cctor != NULL);
        cctor->lock(cctor);
        methodState = cctor->getState(cctor);
        cctor->unlock(cctor);
        if (methodState != EXECUTABLE_STATE) {
            executionReady = 0;
            break;
        }
        item = item->next;
    }

    return executionReady;
}

static inline Method internal_fetchStaticConstructor (StaticMemoryManager *manager, MethodDescriptor *methodID) {
    Method	 	cctor;
    t_methods       *methods;

    /* Assertions		*/
    assert(manager != NULL);
    assert(methodID != NULL);

    /* Search the cctor	*/
    methods = &((ildjitSystem->cliManager).methods);
    cctor = methods->fetchOrCreateMethod(methods, methodID, JITTRUE);
    assert(cctor != NULL);

    /* Return the cctor	*/
    return cctor;
}

static inline void * internal_create_static_object (StaticMemoryManager *manager, TypeDescriptor *type) {
    void            *newStaticObject;

    /* Assertions.
     */
    assert(manager != NULL);
    assert(type != NULL);

    xanHashTable_lock(manager->staticObjects);
    newStaticObject = xanHashTable_lookup(manager->staticObjects, type);
    if (newStaticObject == NULL) {
        newStaticObject = (ildjitSystem->garbage_collectors).gc.allocStaticObject(type, 0);
        assert(newStaticObject != NULL);
        xanHashTable_insert(manager->staticObjects, type, newStaticObject);
    }
    assert(newStaticObject != NULL);
    xanHashTable_unlock(manager->staticObjects);

    return newStaticObject;
}

static inline ir_value_t static_object_resolve (ir_symbol_t *symbol) {
    ir_value_t value;

    /* Assertions.
     */
    assert(symbol != NULL);
    assert(symbol->data != NULL);

    value.v = (IR_ITEM_VALUE) (JITNUINT) internal_create_static_object(&(ildjitSystem->staticMemoryManager), (TypeDescriptor *) symbol->data);
    assert(value.v != 0);

    return value;
}

static inline void static_object_serialize (ir_symbol_t * symbol, void **mem, JITUINT32 *memBytesAllocated) {
    JITUINT32		bytesWritten;
    TypeDescriptor *type = (TypeDescriptor *) symbol->data;
    ir_symbol_t *typeSymbol = (ildjitSystem->cliManager).translator.getTypeDescriptorSymbol(&(ildjitSystem->cliManager), type);

    /* Allocate the memory.
     */
    (*memBytesAllocated)	= (ILDJIT_maxNumberOfDigits(IRUINT32) + 1);
    (*mem)			= allocFunction(*memBytesAllocated);

    /* Serialize.
     */
    bytesWritten		= ILDJIT_writeIntegerValueInMemory(*mem, 0, *memBytesAllocated, typeSymbol->id, JITFALSE);
    (*memBytesAllocated)	= bytesWritten;

    return ;
}

static inline void static_object_dump (ir_symbol_t *symbol, FILE *fileToWrite) {
    TypeDescriptor *type;

    /* Initialize the variables				*/
    type = (TypeDescriptor *) symbol->data;
    assert(type != NULL);

    fprintf(fileToWrite, "Static Object of %s", type->getCompleteName(type));
}

static inline ir_symbol_t *  static_object_deserialize (void *mem, JITUINT32 memBytes) {
    JITINT32 typeSymbolID;

    ILDJIT_readIntegerValueFromMemory(mem, 0, &typeSymbolID);
    ir_symbol_t *symbol = IRSYMBOL_loadSymbol(typeSymbolID);
    TypeDescriptor *type = (TypeDescriptor *) (JITNUINT) (IRSYMBOL_resolveSymbol(symbol).v);

    return internal_fetchStaticMemorySymbol(type);
}

void * STATICMEMORY_fetchStaticObject (StaticMemoryManager *manager, Method method, TypeDescriptor *type) {

    /* Assertions						*/
    assert(manager != NULL);
    assert(type != NULL);

    /* Make the call to the existStaticObject method	*/
    void *newStaticObject = internal_create_static_object(manager, type);
    assert(newStaticObject != NULL);

    /* Search the .cctor method			*/
    MethodDescriptor *cctorMethodID = type->getCctor(type);
    if (	(cctorMethodID != NULL) 	&&
            (method != NULL)		) {
        Method cctor = internal_fetchStaticConstructor(manager, cctorMethodID);
        if (cctor != method) {
            method->lock(method);
            cctor->lock(cctor);
            if (cctor->executionProbability < 0 || (cctor->executionProbability < method->executionProbability) ) {
                cctor->executionProbability = method->executionProbability;
            }
            cctor->unlock(cctor);
            method->unlock(method);
        }
        method->addCctorToCall(method, cctor);
    }

    /* return						*/
    return newStaticObject;
}

/* Register that cctor thread is blocked waiting for method translation */
static inline void internal_registerCompilationNeeded (StaticMemoryManager* self, pthread_t thread, Method method) {
    XanStack	*waitingStack;
    XanHashTable	*waitMethodSet;

    /* Cache some pointers					*/
    waitMethodSet = self->waitMethodSet;

    /* Lock the waiting set					*/
    xanHashTable_lock(waitMethodSet);

    /* Allocate the waiting stack for the current thread	*/
    waitingStack	= xanHashTable_lookup(waitMethodSet, (void *) &thread);
    if (waitingStack == NULL) {
        pthread_t	*allocatedThreadId;
        allocatedThreadId 	= allocFunction(sizeof(pthread_t));
        (*allocatedThreadId) 	= thread;
        waitingStack		= xanStack_new(allocFunction, freeFunction, NULL);
        xanHashTable_insert(waitMethodSet, (void*) allocatedThreadId, waitingStack);
    }
    assert(xanHashTable_lookup(waitMethodSet, &thread) != NULL);

    /* Declare that thread is waiting for method translation */
    xanStack_push(waitingStack, method);

    /* Unlock the waiting set				*/
    xanHashTable_unlock(waitMethodSet);

    return ;
}

/* Register that cctor thread ends waiting for a method initialization */
static inline void internal_registerCompilationDone (StaticMemoryManager* self, pthread_t thread) {
    XanHashTable	*waitMethodSet;
    XanStack	*waitingStack;

    /* Cache some pointers					*/
    waitMethodSet = self->waitMethodSet;

    /* Lock the waiting set					*/
    xanHashTable_lock(waitMethodSet);

    /* Fetch the waiting stack of the current thread	*/
    waitingStack	= xanHashTable_lookup(waitMethodSet, (void *) &thread);
    assert(waitingStack != NULL);

    /* Declare that waiting is done 			*/
    xanStack_pop(waitingStack);

    /* Unlock the waiting set				*/
    xanHashTable_unlock(waitMethodSet);

    return ;
}

JITBOOLEAN STATICMEMORY_registerTranslator (StaticMemoryManager* self, Method method, pthread_t thread) {
    XanHashTable	*translatingSet;
    pthread_t	*oldThread;

    /* Cache some pointers				*/
    translatingSet = self->translatingSet;

    /* Lock the translating set			*/
    xanHashTable_lock(translatingSet);

    /* Declare a new translator 			*/
    oldThread	= xanHashTable_lookup(translatingSet, method);
    if (oldThread == NULL) {
        pthread_t	*allocatedThreadPointer;

        /* Allocate thread identifier into heap		*/
        allocatedThreadPointer = allocFunction(sizeof(pthread_t));
        (*allocatedThreadPointer) = thread;

        /* Insert the mapping				*/
        xanHashTable_insert(translatingSet, method, allocatedThreadPointer);
    }

    /* Unlock the translating set			*/
    xanHashTable_unlock(translatingSet);

    return (oldThread == NULL);
}

void STATICMEMORY_unregisterTranslator (StaticMemoryManager* self, Method method) {

    /* Contains (method, thread) translating relation */
    XanHashTable* translatingSet;
    void *elementToFree;

    /* Unregister translator */
    translatingSet 	= self->translatingSet;
    xanHashTable_lock(translatingSet);
    elementToFree	= xanHashTable_lookup(translatingSet, method);
    xanHashTable_removeElement(translatingSet, method);
    xanHashTable_unlock(translatingSet);
    freeMemory(elementToFree);

    /* Return					*/
    return;
}

/* Invoke/Bypass/Wait a cctor */
static inline void internal_callStaticConstructorIfNeeded (StaticMemoryManager* self, Method cctor) {

    /* Contains all executed/on-fly cctors */
    XanHashTable* executedCctors;

    /* Description of cctor inside the initialization module */
    t_methodToCall* assignedCctorDescription;

    /* Get some pointers */
    executedCctors = self->cctorMethodsExecuted;

    /* Check who has the initialization lock */
    assignedCctorDescription = xanHashTable_syncLookup(executedCctors, cctor);

    /* If the cctor is just called we don't need to wait */
    if (internal_executionIsDone(self, assignedCctorDescription)) {
        return;
    }

    /* Check if we need to call or just wait the execution of the cctor	*/
    if (	(!assignedCctorDescription->inExecution)						&&
            (PLATFORM_equalThread(assignedCctorDescription->owner, PLATFORM_getSelfThreadID())) 	) {

        /* Its mine! Call static constructor */
        internal_callStaticConstructor(self, assignedCctorDescription);

    } else {

        /* Not mine, safe wait or not wait */
        internal_waitStaticConstructor(self, assignedCctorDescription);
    }
}

/* Compile or bypass static constructor */
static inline void internal_makeStaticConstructorExecutable (StaticMemoryManager* self, Method cctor, JITINT32 priority) {
    /* Executed/On-fly cctors */
    XanHashTable* executedCctors;

    /* Description of an assigned cctor */
    t_methodToCall* assignedCctorDescription;

    /*
     * Whether this thread got the initialization lock for they type the
     * cctor belongs to
     */
    JITBOOLEAN initializationLockTaken;

    /* Current thread ID */
    pthread_t currentThread;

    /* Get some pointers */
    executedCctors = self->cctorMethodsExecuted;
    assert(cctor != NULL);
    assert(cctor->isCilImplemented(cctor));

    /* Critical operation, lock needed */
    xanHashTable_lock(executedCctors);

    /* See if someone else was or is initializing the type */
    assignedCctorDescription = xanHashTable_lookup(executedCctors, cctor);

    /*
     * Type initialization not yet done, current thread is the responsible
     * for cctor translation and invocation
     */
    if (assignedCctorDescription == NULL) {
        currentThread = PLATFORM_getSelfThreadID();
        assignedCctorDescription = allocMemory(sizeof(t_methodToCall));
        assignedCctorDescription->method = cctor;
        assignedCctorDescription->owner = currentThread;
        assignedCctorDescription->executionDone = JITFALSE;
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        PLATFORM_initMutex(&(assignedCctorDescription->lock), &mutex_attr);
        PLATFORM_initCondVar(&assignedCctorDescription->condition, NULL);
        xanHashTable_insert(executedCctors, cctor, assignedCctorDescription);
        assert(xanHashTable_lookup(executedCctors, cctor) == assignedCctorDescription);
        initializationLockTaken = JITTRUE;

        /* Initialization lock just taken by another thread, don't wait! */
    } else {
        initializationLockTaken = JITFALSE;
    }

    /* End of critical section */
    xanHashTable_unlock(executedCctors);

    /*
     * I've got the initialization lock, compile cctor (evades compile time
     * deadlocks)
     */
    if (initializationLockTaken) {
        TranslationPipeline* pipeline;
        pipeline = &(ildjitSystem->pipeliner);
        pipeline->synchInsertMethod(pipeline, cctor, priority);
    }
}

/* Call given static constructor */
static inline void internal_callStaticConstructor (StaticMemoryManager* self, t_methodToCall* cctorDescription) {

    /* Current thread identifier */
    pthread_t currentThread;

    /* CCTOR to call                        */
    Method cctor;

    /* Get current thread identifier */
    currentThread = PLATFORM_getSelfThreadID();

    /* Assertions				*/
    assert(self != NULL);
    assert(cctorDescription != NULL);
    assert(PLATFORM_equalThread(cctorDescription->owner,PLATFORM_getSelfThreadID()));

    /* Initialize the type                  */
    cctor = cctorDescription->method;

    /* Check if in the mean while the	*
     * method has been executed		*/
    PLATFORM_lockMutex(&cctorDescription->lock);
    if (	(!cctorDescription->executionDone) 	&&
            (!cctorDescription->inExecution)	) {
        JITBOOLEAN		toUnregister;
        cctorDescription->inExecution	= JITTRUE;
        PLATFORM_unlockMutex(&cctorDescription->lock);

        /* Declare I'm running cctor 		*/
        toUnregister	= STATICMEMORY_registerTranslator(self, cctor, currentThread);

        /* Check if we need to delay the 	*
         * execution of the cctor		*/
        if (!self->cacheConstructors) {

            /* Execute the cctor			*/
            internal_callCctorMethod(self, cctor);

        } else {

            /* Delay the execution of the cctor	*/
            assert(xanList_find(self->cachedConstructors, cctor) == NULL);
            xanList_append(self->cachedConstructors, cctor);
        }

        /* Declare I'm not still running cctor 	*/
        if (toUnregister) {
            STATICMEMORY_unregisterTranslator(self, cctor);
        }

        /* Declare initialization done          */
        PLATFORM_lockMutex(&cctorDescription->lock);
        cctorDescription->executionDone = JITTRUE;
        cctorDescription->inExecution	= JITFALSE;
        PLATFORM_broadcastCondVar(&cctorDescription->condition);
    }
    PLATFORM_unlockMutex(&cctorDescription->lock);

    /* Return			*/
    return;
}

/* Wait for given static constructor end, if safe */
static inline void internal_waitStaticConstructor (StaticMemoryManager* self, t_methodToCall* cctorDescription) {

    /* Current thread identifier */
    pthread_t currentThread;

    /* Whether a deadlock would occurs */
    JITBOOLEAN deadlock;

    /* Get current thread identifier */
    currentThread = PLATFORM_getSelfThreadID();

    /* Check if a deadlock would occurs */
    deadlock = internal_tryDeclaringWaiting(self, currentThread, cctorDescription->method);

    /* Safe to wait */
    if (!deadlock) {

        (ildjitSystem->pipeliner).declareWaitingCCTOR(&(ildjitSystem->pipeliner));

        /* Wait for execution done */
        PLATFORM_lockMutex(&cctorDescription->lock);
        while (!cctorDescription->executionDone) {
            PLATFORM_waitCondVar(&cctorDescription->condition, &cctorDescription->lock);
        }
        PLATFORM_unlockMutex(&cctorDescription->lock);

        (ildjitSystem->pipeliner).declareWaitingCCTORDone(&(ildjitSystem->pipeliner));

        /* Declare I'm not still waiting */
        internal_registerCompilationDone(self, currentThread);
    }
}

/* Try declaring that waiting thread is waiting for waitedThread */
static inline JITBOOLEAN internal_tryDeclaringWaiting (StaticMemoryManager* self, pthread_t waitingThread, Method method) {

    /* Maps threads to waited methods */
    XanHashTable* waitingSet;

    /* Maps methods to corresponding translators */
    XanHashTable* translatingSet;

    XanHashTable* executedCctors;

    /* A method waited by a thread */
    Method waitedMethod;

    /* A thread that I can wait */
    pthread_t waitedThread;

    /* A tread translating a method */
    pthread_t* translatorThreadPointer;
    XanStack	*waitingStack;
    t_methodToCall  *cctorToCall;

    /* Whether a deadlock has been found */
    JITBOOLEAN deadlock;

    /* Get the chain */
    waitingSet = self->waitMethodSet;
    translatingSet = self->translatingSet;
    executedCctors = self->cctorMethodsExecuted;

    /* Lock wait chain 			*/
    xanHashTable_lock(waitingSet);
    xanHashTable_lock(translatingSet);
    xanHashTable_lock(executedCctors);

    /* Get chain head 			*/
    waitedThread 	= 0;
    waitedMethod	= method;
    cctorToCall 	= (t_methodToCall *) xanHashTable_lookup(executedCctors, method);
    if (cctorToCall == NULL) {
        pthread_t	*waitedThreadPtr;
        waitedThreadPtr	= ((pthread_t*) xanHashTable_lookup(translatingSet, method));
        if (waitedThreadPtr != NULL) {
            waitedThread = *waitedThreadPtr;
        }
    } else {
        waitedThread = cctorToCall->owner;
    }

    /* Check if we have a self loop deadlock		*/
    if (waitedThread == waitingThread) {

        /* Unlock wait chain            					*/
        xanHashTable_unlock(executedCctors);
        xanHashTable_unlock(translatingSet);
        xanHashTable_unlock(waitingSet);

        return JITTRUE;
    }

    /* Fetch the waiting stack						*/
    waitingStack	= xanHashTable_lookup(waitingSet, &waitingThread);
    if (waitingStack == NULL) {
        pthread_t* allocatedWaitingThreadId;
        allocatedWaitingThreadId 	= allocFunction(sizeof(pthread_t));
        (*allocatedWaitingThreadId) 	= waitingThread;
        waitingStack			= xanStack_new(allocFunction, freeFunction, NULL);
        xanHashTable_insert(waitingSet, allocatedWaitingThreadId, waitingStack);
    }
    assert(xanHashTable_lookup(waitingSet, &waitingThread) != NULL);

    /* Declare thread waiting for method */
    xanStack_push(waitingStack, method);

    /* Cycle thought the chain to find a possible deadlock */
    deadlock = JITFALSE;
    do {
        XanStack	*waitedStack;

        /* Check if the thread is waiting for the method is the one     *
         * that is asking to wait for it.				*
         * In this case we have a deadlock				*/
        if (waitedThread) {
            if (PLATFORM_equalThread(waitedThread, waitingThread)) {
                xanStack_pop(waitingStack);
                deadlock = JITTRUE;
                break;
            }

            /* Fetch the waiting stack of the thread that is already waiting for the translation of the method, which is also in charge of its translation	*/
            waitedStack	= xanHashTable_lookup(waitingSet, &waitedThread);

            /* The waited thread isn't waiting any method translation */
            if (waitedStack == NULL) {
                deadlock = JITFALSE;
                break;
            }
            assert(waitedStack != NULL);
            if (xanStack_getSize(waitedStack) == 0) {
                deadlock = JITFALSE;
                break;
            }
            waitedMethod 	= xanStack_top(waitedStack);
        }

        /* Get the thread that is translating the method */
        translatorThreadPointer = (pthread_t *) xanHashTable_lookup(translatingSet, waitedMethod);

        /* No translator associated with the method */
        if (translatorThreadPointer == NULL) {
            deadlock = JITFALSE;
            break;
        }

        /* Continue on waiting chain */
        waitedThread = (*translatorThreadPointer);

    } while (waitedThread != 0);

    /* Unlock wait chain            */
    xanHashTable_unlock(executedCctors);
    xanHashTable_unlock(translatingSet);
    xanHashTable_unlock(waitingSet);

    /* Return			*/
    return deadlock;
}

/* Check whether cctorDescription reference an executed cctor */
static inline JITBOOLEAN internal_executionIsDone (StaticMemoryManager* self, t_methodToCall* cctorDescription) {

    /* Whether cctor execution has done */
    JITBOOLEAN executionDone;

    if (cctorDescription == NULL) {
        return JITTRUE;
    }

    /* Do the check in a thread safe way */
    PLATFORM_lockMutex(&cctorDescription->lock);
    executionDone	= cctorDescription->executionDone;
    PLATFORM_unlockMutex(&cctorDescription->lock);

    return executionDone;
}

void STATICMEMORY_flushCachedConstructors (StaticMemoryManager *manager) {

    /* Remove called methods.
     */
    xanList_emptyOutList(manager->cachedConstructors);

    return ;
}

void STATICMEMORY_cacheConstructorsToCall (StaticMemoryManager *manager) {

    /* Assertions			*/
    assert(manager != NULL);

    manager->cacheConstructors	= JITTRUE;

    /* Return			*/
    return ;
}

void STATICMEMORY_callCachedConstructors (StaticMemoryManager *manager) {
    XanListItem	*item;

    /* Assertions					*/
    assert(manager != NULL);

    /* Quit now if static constructiors are disabled*/
    if ((ildjitSystem->IRVM).behavior.disableStaticConstructors) {
        return ;
    }

    /* Call cctors					*/
    item	= xanList_first(manager->cachedConstructors);
    while (item != NULL) {
        Method cctor;

        /* Fetch the cctor				*/
        cctor = item->data;
        assert(cctor != NULL);

        /* Call the cctor				*/
        internal_callCctorMethod(manager, cctor);

        /* Fetch the next element			*/
        item = item->next;
    }

    /* Remove called methods			*/
    xanList_emptyOutList(manager->cachedConstructors);

    return ;
}

void STATICMEMORY_shutdownMemoryManager (StaticMemoryManager *manager) {
    if (manager->staticObjects != NULL) {
        XanHashTableItem	*hashItem;
        hashItem	= xanHashTable_first(manager->staticObjects);
        while (hashItem != NULL) {
            void	*obj;
            obj		= hashItem->element;
            (ildjitSystem->garbage_collectors).gc.freeObject(obj);
            hashItem	= xanHashTable_next(manager->staticObjects, hashItem);
        }
        xanHashTable_destroyTable(manager->staticObjects);
    }
    if (manager->symbolTable != NULL) {
        xanHashTable_destroyTableAndData(manager->symbolTable);
        manager->symbolTable	= NULL;
    }
    if (manager->cctorMethodsExecuted != NULL) {
        xanHashTable_destroyTableAndData(manager->cctorMethodsExecuted);
    }
    if (manager->cachedConstructors != NULL) {
        xanList_destroyList(manager->cachedConstructors);
    }
    if (manager->waitMethodSet != NULL) {
        XanHashTableItem	*item;
        item	= xanHashTable_first(manager->waitMethodSet);
        while (item != NULL) {
            XanStack	*s;
            s	= item->element;
            xanStack_destroyStack(s);
            item	= xanHashTable_next(manager->waitMethodSet, item);
        }
        xanHashTable_destroyTableAndKey(manager->waitMethodSet);
    }
    if (manager->translatingSet != NULL) {
        xanHashTable_destroyTable(manager->translatingSet);
    }
    memset(manager, 0, sizeof(StaticMemoryManager));

    return ;
}

JITBOOLEAN STATICMEMORY_isThreadTranslatingMethod (StaticMemoryManager* self, pthread_t thread, Method method) {
    XanHashTable	*translatingSet;
    pthread_t	*translatingThread;
    JITBOOLEAN	isTheOne;

    /* Cache some pointers				*/
    translatingSet 		= self->translatingSet;

    /* Lock the set					*/
    xanHashTable_lock(translatingSet);

    /* Fetch the thread that is translating the	*
     * method					*/
    isTheOne		= JITFALSE;
    translatingThread	= xanHashTable_lookup(translatingSet, method);
    if (	(translatingThread != NULL)				&&
            (PLATFORM_equalThread(*translatingThread, thread))	) {
        isTheOne		= JITTRUE;
    }

    /* Unlock the set				*/
    xanHashTable_unlock(translatingSet);

    return isTheOne;
}

static inline ir_symbol_t * internal_fetchStaticMemorySymbol (TypeDescriptor *type) {
    ir_symbol_t 	*symbol;

    /* Assertions.
     */
    assert(type != NULL);

    /* Fetch the symbol representing the static object of the class "type".
     */
    symbol 	= IRSYMBOL_createSymbol(STATIC_OBJECT_SYMBOL, (void *) type);
    assert(symbol != NULL);

    /* Populate the symbol table.
     */
    if (xanHashTable_lookup((ildjitSystem->staticMemoryManager).symbolTable, symbol) == NULL) {
        ir_static_memory_t	*desc;
        ILLayoutStatic		*staticInfos;
        ILLayout_manager	*layoutManager;
        void			*obj;

        /* Fetch the layout manager.
         */
        layoutManager 		= &((ildjitSystem->cliManager).layoutManager);

        /* Allocate the static object.
         */
        obj			= internal_create_static_object(&(ildjitSystem->staticMemoryManager), type);
        assert(obj != NULL);

        /* Fetch the static layout.
         */
        staticInfos 		= layoutManager->layoutStaticType(layoutManager, type);
        assert(staticInfos != NULL);
        assert(memcmp(staticInfos->initialized_obj, obj, staticInfos->typeSize) == 0);

        /* Create the static memory descriptor.
         */
        desc			= allocFunction(sizeof(ir_static_memory_t));
        desc->s			= symbol;
        desc->allocatedMemory	= obj;
        desc->name		= type->getCompleteName(type);
        desc->size 		= staticInfos->typeSize;

        /* Insert the descriptor into the symbol table.
         */
        xanHashTable_insert((ildjitSystem->staticMemoryManager).symbolTable, symbol, desc);
    }

    return symbol;
}
