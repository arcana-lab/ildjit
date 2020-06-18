/*
 * Copyright (C) 2006 - 2012  Simone Campanoni
 *
 * Compilation pipeline system. In this module the compilation tasks are dispatched and performed in parallel.
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
/**
 * @file translation_pipeline.c
 * @brief Pipeline system
 */
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <xanlib.h>
#include <iljit-utils.h>
#include <error_codes.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>
#include <dla.h>
#include <ir_virtual_machine.h>
#include <jit_metadata.h>
#include <platform_API.h>

// My headers
#include <iljit.h>
#include <system_manager.h>
#include <cil_ir_translator.h>
#include <static_memory.h>
#include <ildjit_profiler.h>
#include <code_generator.h>
// End

#define MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define NEED_HIGH_PRIORITY_PIPE(priority) (priority >= 0.75)
#define MIN_CCTORS_DELTA                        2
//#define PROFILE_METHODS_IN_COMPILATION
//#define PROFILE_CCTORS
//#define PROFILE_NEEDED_CCTORS
//#define NO_CCTORS_MANAGER
//#define DISABLE_PRIORITY_PIPE
//#define DISABLE_CHECKPOINTING

/**
 *
 * Ticket used for each method in pipe.
 */
typedef struct {
    pthread_mutex_t	mutex;
    XanList         *notifyTickets;
    XanList         *unoptimizedIR_notifyTickets;
    XanList         *machineCode_notifyTickets;
    ProfileTime startTime;
    ildjitPipe_t *inPipe;                /**< Show in which pipe the method bind to this ticket is currently inserted. It can be in CILPIPE, IRPIPE, IROPTIMIZERPIPE, CCTORPIPE, NOPIPE	*/
    JITBOOLEAN inHighPriorityPipe;
    XanQueueItem     *pipeItem;
    Method method;
    JITFLOAT32 priority;            /**< Current priority of the ticket.	*/
    JITUINT32 jobState;             /**< Job State for rescheduled ticket	*/
    JITBOOLEAN trampolinesSet;      /**< Trampoline is already backpatched.	*/
    JITBOOLEAN insertedByCctorThread;
    JITBOOLEAN 	runCCTORS;
    XanList         *dlaMethods;
} t_ticket;

typedef struct {
    XanVar *variable;
    XanQueueItem *pipeItem;
} checkPoint_t;

/* Threads functions			*/
static inline void internal_initSchedulerAttributes (pthread_attr_t *tattr, JITBOOLEAN useHighPriority);
static inline void internal_startWorkersSet (workerSet_t *workers, JITBOOLEAN isHighPriorityWorkersSet, void * (*startWorker)(void *));
static inline void internal_startILDJITPipe (ildjitPipe_t *pipe, void * (*startWorker)(void *));
static inline void internal_startPipeliner (TranslationPipeline *pipeline);
static inline JITINT32 insert_method_into_pipeline_async (TranslationPipeline *pipe, Method method, JITFLOAT32 priority);
static inline void internal_synchTillIRMethod (TranslationPipeline *pipe, Method method, JITFLOAT32 priority);
static inline void insert_method_into_pipeline (TranslationPipeline *pipe, Method method, JITFLOAT32 priority);
static inline void * start_cil_ir_translator (void *data);
static inline void * start_dla (void *data);
static inline void * start_ir_optimizer (void *data);
static inline void * start_ir_machinecode_translator (void *data);
static inline void * start_cctor (void *data);
static inline void * start_scheduler (void *data);
static inline void internal_worker_execution (ildjitPipe_t *fromPipe, ildjitPipe_t *toPipe, JITBOOLEAN isHighPriorityPipe,JITUINT32 (*compilationJob)(t_ticket *ticket, XanVar *checkPointVariable));
static inline t_ticket * internal_pipe_get_job (ildjitPipe_t *sourcePipe, JITBOOLEAN useHighPriorityPipe, checkPoint_t *checkPoint);
static inline void internal_pipe_end_job (ildjitPipe_t *sourcePipe, JITBOOLEAN useHighPriorityPipe, ildjitPipe_t *destinationPipe, JITBOOLEAN destinateToHighPriority, checkPoint_t *sourceCheckpoint, t_ticket *ticket);
static inline JITUINT32 DLA (t_ticket *ticket, XanVar *checkPoint);
static inline JITUINT32 cilIRTranslator (t_ticket *ticket, XanVar *checkPoint);
static inline JITUINT32 iROptimizer (t_ticket *ticket, XanVar *checkPoint);
static inline JITUINT32 iRMachineCodeTranslator (t_ticket *ticket, XanVar *checkPoint);
static inline JITUINT32 cctorTranslator (t_ticket *ticket, XanVar *checkPoint);
static inline void internal_printMethodsInPipe (TranslationPipeline *pipe);
static inline void internal_deleteMethodIntoPipelineSet (TranslationPipeline *pipe, t_ticket *ticket);
static inline t_ticket * internal_insertMethodIntoPipelineSet (TranslationPipeline *pipe, Method method, pthread_cond_t *notify, pthread_cond_t *unoptimizedIR_notify, pthread_cond_t *machineCode_notify);
static inline JITINT16 internal_isInPipe (TranslationPipeline *pipe, Method method);
static inline void internal_dumpPipe (XanPipe *pipe, JITINT8 *prefixString);
static inline void internal_destroyWorkersSet (workerSet_t *workers);
static inline void internal_destroyILDJITPipe (ildjitPipe_t *pipe);
static inline void internal_destroySchedulerPipe (schedPipe_t *pipe);
static inline void internal_initWokersSet (workerSet_t *workers, JITUINT32 threadsCount);
static inline void internal_initILDJITPipe (ildjitPipe_t *pipe, JITUINT32 highPriorityThreadCount, JITUINT32 lowPriorityThreadCount);
static inline void internal_initSchedulerPipe (schedPipe_t *pipe);
static inline void internal_changePriority (TranslationPipeline *pipe, ildjitPipe_t *fromPipe, t_ticket *ticket);
static inline void DLAScheduler (schedPipe_t *pipe);
static inline void internal_destroyThePipeline (TranslationPipeline *pipe);
static inline JITBOOLEAN isCCTORThread (TranslationPipeline* self, pthread_t thread);
static inline JITBOOLEAN internal_isCCTORThread (TranslationPipeline* self, pthread_t thread);
static inline void internal_waitEmptyPipeline (TranslationPipeline *pipe);
static inline t_ticket * internal_insert_method_into_the_right_pipeline (TranslationPipeline *pipe, Method method, JITFLOAT32 priority, pthread_cond_t *notify, pthread_cond_t *unoptimizedIR_notify, pthread_cond_t *machineCode_notify, JITUINT32 methodState, JITBOOLEAN runCCTORS);
static inline void internal_tryCheckPoint (workerSet_t *destinationWorkers, t_ticket *ticket);
static inline void internal_putToPipe (ildjitPipe_t *destinationPipe, JITBOOLEAN destinateToHighPriority,t_ticket *ticket);
static inline void internal_declareWaitingCCTOR (struct TranslationPipeline *pipeline);
static inline void internal_decreaseSteadyCCTORThreads (TranslationPipeline *pipeline);
static inline void internal_declareWaitingCCTORDone (struct TranslationPipeline *pipeline);
static inline void internal_increaseSteadyCCTORThreads (TranslationPipeline *pipeline);
static inline void internal_killWorkersSet (workerSet_t *workers);
static inline void internal_killILDJITPipe (ildjitPipe_t *pipe);
static inline void internal_killCCTORThreads (TranslationPipeline *pipeline);
static inline void internal_startMoreThreadsWorkersSet (workerSet_t *workers, JITBOOLEAN isHighPriorityWorkersSet, void * (*startWorker)(void *), JITINT32 num);
static inline void internal_startMoreThreadsILDJITPipe (ildjitPipe_t *pipe, void * (*startWorker)(void *), JITINT32 numLow, JITINT32 numHigh);
static inline void internal_startMoreCCTORThreads (TranslationPipeline *pipeline, JITINT32 numLow, JITINT32 numHigh);
static inline XanList * internal_getReachableMethods (ir_method_t *irMethod);
static inline void internal_add_functions (XanList *list, ir_item_t *item);
static inline void * waitEmptyPipelineThread (void *data);
static inline void internal_waitStartPipeline (void);
static inline void internal_notifyThePipelineIsStarted (void);
static inline void internal_declareWaitingCCTOR_dummy (struct TranslationPipeline *pipeline);
static inline void internal_declareWaitingCCTORDone_dummy (struct TranslationPipeline *pipeline);
static inline JITINT16 internal_isInPipe_dummy (TranslationPipeline *pipe, Method method);
static inline void internal_waitEmptyPipeline_dummy (TranslationPipeline *pipe);
static inline JITBOOLEAN isCCTORThread_dummy (TranslationPipeline* self, pthread_t thread);
static inline void internal_destroyThePipeline_dummy (TranslationPipeline *pipe);
static inline void insert_method_into_pipeline_dummy (TranslationPipeline *pipe, Method method, JITFLOAT32 priority);
static inline void internal_synchTillIRMethod_dummy (TranslationPipeline *pipe, Method method, JITFLOAT32 priority);
static inline JITINT32 insert_method_into_pipeline_async_dummy (TranslationPipeline *pipe, Method method, JITFLOAT32 priority);
static inline void internal_notifyCompilationEnd (t_ticket *ticket);
static inline void internal_destroyTicket (t_ticket *ticket);

static JITBOOLEAN exitPipeCondition = JITFALSE;
static pthread_mutex_t startPipeMutex;
static pthread_cond_t startPipeCondition;
static JITBOOLEAN pipelineIsStarted = JITFALSE;

static JITUINT32 cctors_delta;
#ifdef PROFILE_CCTORS
static FILE *cctorsProfile;
#endif
#ifdef PROFILE_NEEDED_CCTORS
static FILE *cctorsProfile;
#endif
#ifdef PROFILE_METHODS_IN_COMPILATION
static FILE *cilirProfile;
static FILE *irOptimizerProfile;
static FILE *irMachineCodeProfile;
static FILE *cctorsProfile;
#endif

extern t_system *ildjitSystem;

void PARALLELPIPELINER_init (TranslationPipeline *pipeline) {
    JITUINT32 irOptimizerLowPriorityThread;

    /* Assertions				*/
    assert(pipeline != NULL);
    PDEBUG("TRANSLATION PIPELINE: newPipeline: Start\n");
    PDEBUG("TRANSLATION PIPELINE: newPipeline:      Make the new pipeline\n");

    /* Fill up the new pipeline		*/
    pipeline->declareWaitingCCTOR = internal_declareWaitingCCTOR;
    pipeline->declareWaitingCCTORDone = internal_declareWaitingCCTORDone;
    pipeline->isInPipe = internal_isInPipe;
    pipeline->startPipeliner = internal_startPipeliner;
    pipeline->waitEmptyPipeline = internal_waitEmptyPipeline;
    pipeline->isCCTORThread = isCCTORThread;
    pipeline->destroyThePipeline = internal_destroyThePipeline;
    pipeline->synchInsertMethod = insert_method_into_pipeline;
    pipeline->synchTillIRMethod = internal_synchTillIRMethod;
    pipeline->insertMethod = insert_method_into_pipeline_async;
    cctors_delta = MIN_CCTORS_DELTA;

    /* Make the pipes			*/
    PDEBUG("TRANSLATION PIPELINE (%d): newPipeline:      Make the pipes\n", getpid());

    /* Make the CIL->IR pipes		*/
    internal_initILDJITPipe(&((pipeline->cilPipe)), MAX_CILIR_HIGH_THREADS, MAX_CILIR_LOW_THREADS);

    /* Make the DLA pipes			*/
    if (	(ildjitSystem->IRVM).behavior.dla 	||
            (ildjitSystem->IRVM).behavior.aot 	||
            (ildjitSystem->IRVM).behavior.jit	) {
        internal_initILDJITPipe(&((pipeline->dlaPipe)), MAX_DLA_HIGH_THREADS, MAX_DLA_LOW_THREADS);
    }

    /* Make the IR->Machinecode pipes	*/
    internal_initILDJITPipe(&((pipeline->irPipe)), MAX_IRMACHINECODE_HIGH_THREADS, MAX_IRMACHINECODE_LOW_THREADS);

    /* Make the IR optimizer pipes		*/
    irOptimizerLowPriorityThread = PLATFORM_getProcessorsNumber() - (MAX_IROPTIMIZER_HIGH_THREADS * !((ildjitSystem->IRVM).behavior.aot));
    if (irOptimizerLowPriorityThread <= 0) {
        irOptimizerLowPriorityThread = 1;
    }
    if (irOptimizerLowPriorityThread > MAX_IROPTIMIZER_LOW_THREADS) {
        irOptimizerLowPriorityThread = MAX_IROPTIMIZER_LOW_THREADS;
    }
    internal_initILDJITPipe(&((pipeline->irOptimizerPipe)), MAX_IROPTIMIZER_HIGH_THREADS, irOptimizerLowPriorityThread);

    /* Make the cctor pipes			*/
    internal_initILDJITPipe(&((pipeline->cctorPipe)), START_CCTOR_HIGH_THREADS, START_CCTOR_LOW_THREADS);
    pipeline->steady_cctor_threads = START_CCTOR_HIGH_THREADS + START_CCTOR_LOW_THREADS;

    /* Make the scheduler Pipes */
#ifndef DISABLE_PRIORITY_PIPE
    if (	((ildjitSystem->IRVM).behavior.dla)	||
            ((ildjitSystem->IRVM).behavior.jit)	) {
        internal_initSchedulerPipe(&((pipeline->schedulerPipe)));
    }
#endif

    /* Make the hash table of the	*
     * notifies			*/
    PDEBUG("TRANSLATION PIPELINE (%d): newPipeline:      Make the hash table for the tickets\n", getpid());
    pipeline->methodsInPipe = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(pipeline->methodsInPipe != NULL);
    pthread_mutexattr_t mutex_attr;
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    PLATFORM_initMutex(&(pipeline->mutex), &mutex_attr);
    PLATFORM_initCondVar(&(pipeline->emptyCondition), NULL);

    /* Initialize the notification that the	*
     * pipeline is ready			*/
    PLATFORM_initMutex(&startPipeMutex, NULL);
    PLATFORM_initCondVar(&startPipeCondition, NULL);

    /* Open the profiler files		*/
#ifdef PROFILE_CCTORS
    cctorsProfile = fopen("cctors_profile", "w");
    fprintf(cctorsProfile, "0 %d\n", pipeline->cctor_threads);
#endif
#ifdef PROFILE_NEEDED_CCTORS
    cctorsProfile = fopen("needed_cctors_profile", "w");
    fprintf(cctorsProfile, "0 0\n");
#endif
#ifdef PROFILE_METHODS_IN_COMPILATION
    cilirProfile = fopen("cilir_profile", "w");
    irOptimizerProfile = fopen("irOptimizer_profile", "w");
    irMachineCodeProfile = fopen("irMachineCode_profile", "w");
    cctorsProfile = fopen("cctors_profile", "w");
    fprintf(cilirProfile, "0 0\n");
    fprintf(irOptimizerProfile, "0 0\n");
    fprintf(irMachineCodeProfile, "0 0\n");
    fprintf(cctorsProfile, "0 0\n");
#endif

    /* Return				*/
    return;
}

static inline void internal_initSchedulerAttributes (pthread_attr_t *tattr, JITBOOLEAN useHighPriority) {
    struct sched_param param;

    if (useHighPriority) {
        if (    (PLATFORM_initThreadAttr(tattr) != 0)                       ||
                (pthread_attr_getschedparam(tattr, &param) != 0)      ) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot get the thread priority. ", errno);
            abort();
        }
        param.sched_priority = PLATFORM_sched_get_priority_max(SCHED_OTHER);
        if (param.sched_priority == -1) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot get the maximum thread priority for the SCHED_OTHER policy. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_inheritsched(tattr, PTHREAD_EXPLICIT_SCHED) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread inherits policy. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_scope(tattr, PTHREAD_SCOPE_SYSTEM) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread scope. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_schedpolicy(tattr, SCHED_OTHER) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread policy. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_schedparam(tattr, &param) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread parameters. ", errno);
            abort();
        }
    } else {
        if (    (PLATFORM_initThreadAttr(tattr) != 0)                       ||
                (pthread_attr_getschedparam(tattr, &param) != 0)      ) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread priority. ", errno);
            abort();
        }
        param.sched_priority = PLATFORM_sched_get_priority_min(SCHED_OTHER);
        if (param.sched_priority == -1) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot get the minimum thread priority for the SCHED_OTHER policy. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_inheritsched(tattr, PTHREAD_EXPLICIT_SCHED) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread inherits policy. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_scope(tattr, PTHREAD_SCOPE_SYSTEM) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread scope. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_schedpolicy(tattr, SCHED_OTHER) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread scheduling policy. ", errno);
            abort();
        }
        if (PLATFORM_setThreadAttr_schedparam(tattr, &param) != 0) {
            print_err("TRANSLATION PIPELINE: ERROR: Cannot set the thread priority. ", errno);
            abort();
        }
    }

}

static inline void internal_startWorkersSet (workerSet_t *workers, JITBOOLEAN isHighPriorityWorkersSet,  void * (*startWorker)(void *)) {
    JITUINT32 count;
    pthread_attr_t tattr;

    internal_initSchedulerAttributes(&tattr, isHighPriorityWorkersSet);
    for (count = 0; count < workers->threadsCount; count++) {

        /* Check if the current GC needs	*
         * a thread creation support		*/
        (ildjitSystem->garbage_collectors).gc.threadCreate(&((workers->threads)[count]), &tattr, startWorker, (void *) (JITNUINT) isHighPriorityWorkersSet);
    }
}

static inline void internal_startILDJITPipe (ildjitPipe_t *pipe, void * (*startWorker)(void *)) {

    /* Start the high priority threads			*/
    internal_startWorkersSet(&(pipe->highPriority), JITTRUE, startWorker);

    /* Start the low priority threads			*/
    internal_startWorkersSet(&(pipe->lowPriority), JITFALSE, startWorker);

}

static inline void internal_startSchedulerPipe (schedPipe_t *pipe) {
    pthread_attr_t tattr;

    internal_initSchedulerAttributes(&tattr, JITFALSE);
    ((ildjitSystem->garbage_collectors).gc.threadCreate(&(pipe->thread), &tattr, start_scheduler, (void *) system));
}

static inline void internal_startPipeliner (TranslationPipeline *pipeline) {
    pthread_attr_t tattr;

    /*	Start pipeline stage thread	*/
    internal_startILDJITPipe(&(pipeline->cilPipe), start_cil_ir_translator);
    if (	(ildjitSystem->IRVM).behavior.dla 	||
            (ildjitSystem->IRVM).behavior.jit 	||
            (ildjitSystem->IRVM).behavior.aot	) {
        internal_startILDJITPipe(&(pipeline->dlaPipe), start_dla);
    }
    internal_startILDJITPipe(&(pipeline->irOptimizerPipe), start_ir_optimizer);
    internal_startILDJITPipe(&(pipeline->irPipe), start_ir_machinecode_translator);
    internal_startILDJITPipe(&(pipeline->cctorPipe), start_cctor);

    /*      Start the scheduling threads	*/
#ifndef DISABLE_PRIORITY_PIPE
    if (	(ildjitSystem->IRVM).behavior.dla 	||
            (ildjitSystem->IRVM).behavior.jit	) {
        internal_startSchedulerPipe(&(pipeline->schedulerPipe));
    }
#endif

    /* Start the empty pipeline thread	*/
    internal_initSchedulerAttributes(&tattr, JITFALSE);
    ((ildjitSystem->garbage_collectors).gc.threadCreate(&(pipeline->waitPipelineThread), &tattr, waitEmptyPipelineThread, (void *) &(ildjitSystem->pipeliner)));

    /* Wait that the empty thread will start*/
    internal_waitStartPipeline();
    pipeline->running = JITTRUE;

    /* Return				*/
    return;
}

static inline JITINT32 insert_method_into_pipeline_async (TranslationPipeline *pipe, Method method, JITFLOAT32 priority) {
    JITUINT32 state;

    /* Assertions						*/
    assert(pipe != NULL);
    assert(method != NULL);
    assert(priority <= MAX_METHOD_PRIORITY);
    assert(priority >= MIN_METHOD_PRIORITY);

    /* Fetch the state of the method			*/
    method->lock(method);
    state = method->getState(method);
    method->unlock(method);

    /* Check the method state				*/
    if (state != EXECUTABLE_STATE) {
        t_ticket        *ticket	 __attribute__((unused));

        /* Insert the method into the pipe			*/
        PLATFORM_lockMutex(&(pipe->mutex));
        ticket = internal_insert_method_into_the_right_pipeline(pipe, method, priority, NULL, NULL, NULL, state, JITTRUE);
        assert(ticket != NULL);
        assert(ticket->method == method);
        assert(pipe->isInPipe(pipe, method));
        assert((pipe->isInPipe(pipe, method)));
        PLATFORM_unlockMutex(&(pipe->mutex));
    }

    /* Return to the callee		*/
    return 0;
}

static inline void internal_synchTillIRMethod (TranslationPipeline *pipe, Method method, JITFLOAT32 priority) {
    JITUINT32 	methodState;
    t_ticket        *ticket	__attribute__((unused));
    pthread_cond_t 	notify;

    /* Assertions						*/
    assert(pipe != NULL);
    assert(method != NULL);
    assert(priority <= MAX_METHOD_PRIORITY);
    assert(priority >= MIN_METHOD_PRIORITY);

    /* Initialize the variables				*/
    PLATFORM_initCondVar(&notify, NULL);

    /* Fetch the method state				*/
    method->lock(method);
    methodState = method->getState(method);
    method->unlock(method);

    /* Lock the set of methods present in the pipe		*/
    PLATFORM_lockMutex(&(pipe->mutex));

    /* Check the method state				*/
    switch (methodState) {
        case IR_STATE:
        case MACHINE_CODE_STATE:
        case EXECUTABLE_STATE:
            PLATFORM_unlockMutex(&(pipe->mutex));
            return;
    }

    /* Insert the method into the rigth pipeline    */
    ticket = internal_insert_method_into_the_right_pipeline(pipe, method, priority, NULL, &notify, NULL, methodState, JITTRUE);
    assert(ticket != NULL);
    assert(ticket->method == method);
    assert(pipe->isInPipe(pipe, method));

    /* Synch to the end compilation of the method		*/
    PLATFORM_waitCondVar(&notify, &(pipe->mutex));

    /* Unlock the pipe					*/
    PLATFORM_unlockMutex(&(pipe->mutex));

    PLATFORM_destroyCondVar(&notify);

#ifdef DEBUG
    method->lock(method);
    methodState = method->getState(method);
    method->unlock(method);
    assert((methodState == IR_STATE) || (methodState == MACHINE_CODE_STATE) || (methodState == EXECUTABLE_STATE));
#endif

    /* Return						*/
    return;
}

static inline void insert_method_into_pipeline (TranslationPipeline *pipe, Method method, JITFLOAT32 priority) {
    JITUINT32	methodState;
    t_ticket	*ticket;
    pthread_cond_t	notify;

    /* The static memory manager */
    StaticMemoryManager* staticMemoryManager;

    /* Current thread identifier */
    pthread_t currentThread;

    /* Whether current thread is a cctor one */
    JITBOOLEAN isCCTORThread;
    JITBOOLEAN isMethodInPipe;

    /* Assertions						*/
    assert(pipe != NULL);
    assert(method != NULL);
    assert(priority <= MAX_METHOD_PRIORITY);
    assert(priority >= MIN_METHOD_PRIORITY);
    PDEBUG("TRANSLATION PIPELINE (%d): synchInsertMethod: Start\n", getpid());

    /* Initialize the variables				*/
    ticket = NULL;
    PLATFORM_initCondVar(&notify, NULL);

    /* Get the static memory manager */
    staticMemoryManager = &(ildjitSystem->staticMemoryManager);

    /* Get current thread identifier */
    currentThread = PLATFORM_getSelfThreadID();

    /* Lock the set of methods present in the pipe		*/
    PLATFORM_lockMutex(&(pipe->mutex));

    /* Fetch the method state				*/
    method->lock(method);
    methodState = method->getState(method);
    method->unlock(method);

    /* Check the method state				*/
    if (methodState != EXECUTABLE_STATE) {

        /* If this thread is a cctor register it waits for a compilation 	*/
        isCCTORThread = internal_isCCTORThread(pipe, currentThread);

        /* Check whether the method is already inside the compilation pipeline	*/
        isMethodInPipe = internal_isInPipe(pipe, method);

        /* If thread is cctor and method is in pipeline update check if method does not create deadlock. If so update the wait for graph */
        if (	(isCCTORThread)												&&
                (isMethodInPipe)											&&
                (staticMemoryManager->compilationOfMethodLeadsToADeadlock(staticMemoryManager, currentThread, method))	) {

            /* Fetch the ticket				*/
            ticket = internal_insert_method_into_the_right_pipeline(pipe, method, priority, NULL, NULL, &notify, methodState, JITTRUE);
            assert(ticket != NULL);

            /* Check if we need to wait until the end of    *
             * the compilation of the method		*/
            if (    (methodState != MACHINE_CODE_STATE)     &&
                    (methodState != EXECUTABLE_STATE)       ) {

                /* Synch to the assembly code available of the	*
                 * method                                       */
                PLATFORM_waitCondVar(&notify, &(pipe->mutex));
            }

            /* Set the trampolines to the new compiled      *
             * method					*/
            CODE_linkMethodToProgram(&(ildjitSystem->codeGenerator), method);
            PLATFORM_lockMutex(&(ticket->mutex));
            ticket->trampolinesSet = JITTRUE;
            PLATFORM_unlockMutex(&(ticket->mutex));

            /* Check the method				*/
#ifdef DEBUG
            method->lock(method);
            methodState = method->getState(method);
            method->unlock(method);
            assert((methodState == EXECUTABLE_STATE) || (methodState == MACHINE_CODE_STATE));
#endif

        } else {

            /* if this thread is cctor and the method is    *
             * inserted for first time then register it     *
             * waits for a compilation (detection of deadlock has not run yet)		*/
            if (	(isCCTORThread) 	&&
                    (!isMethodInPipe)	) {
                staticMemoryManager->registerCompilationNeeded(staticMemoryManager, currentThread, method);
            }

            /* Insert the method into the rigth pipeline    */
            ticket = internal_insert_method_into_the_right_pipeline(pipe, method, priority, &notify, NULL, NULL, methodState, JITTRUE);
            assert(ticket != NULL);
            assert(ticket->method == method);
            assert(pipe->isInPipe(pipe, method));
            assert(xanList_find(ticket->notifyTickets, &notify) != NULL);
            assert(pipe->isInPipe(pipe, method));

            /* Check if we need to decrease the number of   *
             * threads available within the CCTOR stage	*/
            if (isCCTORThread) {
                internal_decreaseSteadyCCTORThreads(pipe);
            }

            /* Synch to the end of the compilation of the   *
             * method                                       */
            PDEBUG("TRANSLATION PIPELINE (%d): synchInsertMethod:        Wait for the method %s\n", getpid(), method->getFullName(method));
            PDEBUG("TRANSLATION PIPELINE: synchInsertMethod:        Wait for the method %s\n", method->getName(method));
            if (PLATFORM_waitCondVar(&notify, &(pipe->mutex)) != 0) {
                print_err("TRANSLATION PIPELINE: Error on waiting the compilation of a method. ", errno);
                abort();
            }

            /* Check the method				*/
#ifdef DEBUG
            method->lock(method);
            methodState = method->getState(method);
            method->unlock(method);
            assert((methodState == EXECUTABLE_STATE) || (methodState == MACHINE_CODE_STATE));
#endif

            /* Check if the current thread is a CCTOR thread	*/
            if (isCCTORThread) {

                /* Register the end the waiting of the current thread	*/
                internal_increaseSteadyCCTORThreads(pipe);
                staticMemoryManager->registerCompilationDone(staticMemoryManager, currentThread);
            }
        }

        /* Check the method		*/
#ifdef DEBUG
        method->lock(method);
        methodState = method->getState(method);
        method->unlock(method);
        assert((methodState == EXECUTABLE_STATE) || (methodState == MACHINE_CODE_STATE));
#endif
    }

    /* Unlock the pipe		*/
    PLATFORM_unlockMutex(&(pipe->mutex));
    PDEBUG("TRANSLATION PIPELINE (%d): synchInsertMethod:	Exit for the method %s\n", getpid(), method->getFullName(method));

#ifdef DEBUG
    method->lock(method);
    methodState = method->getState(method);
    method->unlock(method);
    assert((methodState == EXECUTABLE_STATE)          || (methodState == MACHINE_CODE_STATE)        );
#endif

    /* Free the memory		*/
    PLATFORM_destroyCondVar(&notify);

    /* Return to the callee		*/
    PDEBUG("TRANSLATION PIPELINE (%d): synchInsertMethod: Exit\n", getpid());
    return;
}

static inline void * start_cil_ir_translator (void *data) {
    JITBOOLEAN useHighPriorityPipe;

    /* Fetch the system	*/
    useHighPriorityPipe = (JITBOOLEAN) (JITNUINT) data;

    /* Run the execution	*/
    if (	(ildjitSystem->IRVM).behavior.dla 	||
            (ildjitSystem->IRVM).behavior.jit 	||
            (ildjitSystem->IRVM).behavior.aot	) {
        internal_worker_execution(&((ildjitSystem->pipeliner).cilPipe), &((ildjitSystem->pipeliner).dlaPipe), useHighPriorityPipe, cilIRTranslator);
    } else {
        internal_worker_execution(&((ildjitSystem->pipeliner).cilPipe), &((ildjitSystem->pipeliner).irOptimizerPipe), useHighPriorityPipe, cilIRTranslator);
    }
    return NULL;
}

static inline void * start_dla (void *data) {
    JITBOOLEAN useHighPriorityPipe;

    /* Fetch the system	*/
    useHighPriorityPipe = (JITBOOLEAN) (JITNUINT) data;

    /* Run the execution	*/
    internal_worker_execution(&((ildjitSystem->pipeliner).dlaPipe), &((ildjitSystem->pipeliner).irOptimizerPipe), useHighPriorityPipe, DLA);

    return NULL;
}

static inline void * start_ir_optimizer (void *data) {
    JITBOOLEAN useHighPriorityPipe;


    /* Fetch the system	*/
    useHighPriorityPipe = (JITBOOLEAN) (JITNUINT) data;

    /* Run the execution	*/
    internal_worker_execution(&((ildjitSystem->pipeliner).irOptimizerPipe), &((ildjitSystem->pipeliner).irPipe), useHighPriorityPipe, iROptimizer);

    /* Return		*/
    return NULL;
}

static inline void * start_ir_machinecode_translator (void *data) {
    JITBOOLEAN useHighPriorityPipe;

    /* Fetch the system	*/
    useHighPriorityPipe = (JITBOOLEAN) (JITNUINT) data;

    /* Run the execution	*/
    internal_worker_execution(&((ildjitSystem->pipeliner).irPipe), &((ildjitSystem->pipeliner).cctorPipe), useHighPriorityPipe, iRMachineCodeTranslator);

    return NULL;
}

static inline void * start_cctor (void *data) {
    JITBOOLEAN useHighPriorityPipe;

    /* Fetch the system	*/
    useHighPriorityPipe = (JITBOOLEAN) (JITNUINT) data;

    /* Run the execution	*/
    internal_worker_execution(&((ildjitSystem->pipeliner).cctorPipe), NULL, useHighPriorityPipe, cctorTranslator);

    return NULL;
}

static inline void * start_scheduler (void *data) {

    /* Run the execution	*/
    DLAScheduler(&((ildjitSystem->pipeliner).schedulerPipe));

    return NULL;
}

static inline void internal_worker_execution (ildjitPipe_t *fromPipe, ildjitPipe_t *toPipe, JITBOOLEAN isHighPriorityPipe,JITUINT32 (*compilationJob)(t_ticket *ticket, XanVar *checkPointVariable)) {
    t_ticket        *ticket;
    ildjitPipe_t    *destinationPipe;
    checkPoint_t checkPoint;
    JITBOOLEAN destinateToHighPriority;
    JITUINT32 jobState;

    /* Assertions		*/
    assert(fromPipe != NULL);
    assert(compilationJob != NULL);

    checkPoint.variable = xanVar_new(allocFunction, freeFunction);

    while (1) {

        /* Fetch the item from source pipe	*/
        ticket = internal_pipe_get_job(fromPipe, isHighPriorityPipe, &checkPoint);
        if (ticket == NULL) {
            break;
        }
        assert(ticket->method != NULL);

        /* Make the JOB			*/
        jobState = (*compilationJob)(ticket, checkPoint.variable);

        if (jobState != JOB_END) {

            /* Put the method to the source pipe      */
            destinateToHighPriority = isHighPriorityPipe;
            destinationPipe = fromPipe;

            /* Refresh the diagnostic		*/
            PLATFORM_lockMutex(&((ildjitSystem->pipeliner).mutex));
            (ildjitSystem->pipeliner).rescheduledTask++;
            PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).mutex));

            /* refresh ticket Job state		*/
            ticket->jobState = jobState;

        } else if (toPipe != NULL) {

            /* Check if the method is become*
             * high priority		*/
            PLATFORM_lockMutex(&(ticket->mutex));
            destinateToHighPriority = NEED_HIGH_PRIORITY_PIPE(ticket->priority);
            PLATFORM_unlockMutex(&(ticket->mutex));

            destinationPipe = toPipe;

            /* refresh ticket Job state		*/
            ticket->jobState = JOB_START;

        } else {
            destinationPipe = NULL;

            /* refresh ticket Job state		*/
            ticket->jobState = JOB_START;
        }

        /* Release Worker		*/
        internal_pipe_end_job(fromPipe, isHighPriorityPipe, destinationPipe, destinateToHighPriority, &checkPoint, ticket);
    }

    xanVar_destroyVar(checkPoint.variable);

    /* Return			*/
    return;
}

static inline t_ticket * internal_pipe_get_job (ildjitPipe_t *sourcePipe, JITBOOLEAN useHighPriorityPipe, checkPoint_t *checkPoint) {
    workerSet_t *workers;
    t_ticket *ticket;

    if (useHighPriorityPipe) {
        workers = &(sourcePipe->highPriority);
    } else {
        workers = &(sourcePipe->lowPriority);
    }

    /* Check if the thread has to go to sleep	*/
    PLATFORM_lockMutex(&(workers->mutex));

    workers->steadyThreadsCount++;

    while (xanQueue_isEmpty(workers->pipe)) {

        /* The thread has to wait that some method enters in one pipe at least	*/
        PLATFORM_waitCondVar(&(workers->wakeupCondition), &(workers->mutex));
    }

    /* Fetch the item on the low priority pipe	*/
    ticket = (t_ticket *) xanQueue_get(workers->pipe);

    if (ticket != NULL) {
        PLATFORM_lockMutex(&(ticket->mutex));

        /* Refresh the ticket state	*/
        ticket->inPipe = NULL;
        ticket->pipeItem = NULL;

        /* Declare Thread as Busy Worker	*/
        workers->steadyThreadsCount--;
        xanVar_write(checkPoint->variable, JITFALSE);
        checkPoint->pipeItem = xanQueue_put(workers->busyWorkers, checkPoint, MAX_METHOD_PRIORITY - ticket->priority);

        PLATFORM_unlockMutex(&(ticket->mutex));

    }

    PLATFORM_unlockMutex(&(workers->mutex));

    return ticket;
}

static inline void internal_pipe_end_job (ildjitPipe_t *sourcePipe, JITBOOLEAN useHighPriorityPipe, ildjitPipe_t *destinationPipe, JITBOOLEAN destinateToHighPriority, checkPoint_t *sourceCheckpoint, t_ticket *ticket) {
    workerSet_t *sourceWorkers;

    if (useHighPriorityPipe) {
        sourceWorkers = &(sourcePipe->highPriority);
    } else {
        sourceWorkers = &(sourcePipe->lowPriority);
    }

    /* Declare Thread as Steady Worker	*/
    PLATFORM_lockMutex(&(sourceWorkers->mutex));
    if (sourceCheckpoint->pipeItem != NULL) {
        /* worker has not received checkpoint request */
        xanQueue_removeItem(sourceWorkers->busyWorkers, sourceCheckpoint->pipeItem);
        sourceCheckpoint->pipeItem = NULL;
    }
    PLATFORM_unlockMutex(&(sourceWorkers->mutex));

    if (destinationPipe != NULL) {

        /* Put ticket to next stage pipe	*/
        internal_putToPipe(destinationPipe, useHighPriorityPipe, ticket);
    }
}

static inline JITUINT32 DLA (t_ticket *ticket, XanVar *checkPoint) {
    ProfileTime startTime;
    XanList                 *methods;
    XanListItem             *item;

    /* Assertions					*/
    assert(ticket != NULL);

    /* Profiling					*/
    if (ildjitSystem->IRVM.behavior.profiler >= 2) {

        /* Store the start time				*/
        profilerStartTime(&startTime);
    }

    /* Check if we are executing the partial AOT	*/
    if (    ((ildjitSystem->IRVM).behavior.aot)             &&
            ((ildjitSystem->IRVM).behavior.pgc)             ) {
        return JOB_END;
    }

    /* Check if we are executing the JIT mode	*/
    if ((ildjitSystem->IRVM).behavior.jit) {
        return JOB_END;
    }

    /* Check if we are executing the DLA compiler	*
     * or the AOT compiler.				*/
    if (    ((ildjitSystem->IRVM).behavior.aot)             &&
            (!(ildjitSystem->IRVM).behavior.pgc)            ) {

        ir_method_t             *irMethod;
        Method methodToCompile;
        XanList                 *list;
        XanListItem             *item;

        /* Fetch the IR method				*/
        irMethod = ticket->method->getIRMethod(ticket->method);
        assert(irMethod != NULL);

        /* Fetch the list of methods    *
         * reachable at one step from	*
         * the one given as input	*/
        list = internal_getReachableMethods(irMethod);
        assert(list != NULL);

        /* Print the methods		*/
        item = xanList_first(list);
        while (item != NULL) {
            methodToCompile = (Method) item->data;
            assert(methodToCompile != NULL);
            (ildjitSystem->pipeliner).insertMethod(&(ildjitSystem->pipeliner), methodToCompile, MIN_METHOD_PRIORITY);
            item = item->next;
        }

        /* Free the memory		*/
        xanList_destroyList(list);

    } else {
        dla_method_t            *DLAMethodToCompile;
        JITBOOLEAN 		toBeCompiled;

#ifdef MULTIAPP
        ticket->method->lock(ticket->method);
        assert( ticket->method->getGlobalState(ticket->method) >= DLA_PRE_GSTATE );
        if ( ticket->method->getGlobalState(ticket->method) < DLA_ONGOING_GSTATE ) {
            PDEBUG("TRANSLATION PIPELINE (%d): DLA:		Method %s (%p, MethodDescriptor %p) is not being DLA-ed by anyone, I'll take it\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
            ticket->method->setGlobalState(ticket->method, DLA_ONGOING_GSTATE);
            ticket->method->unlock(ticket->method);
#endif
            /* Call the DLA                                 */
            methods = (ildjitSystem->DLA).getMethodsToCompile(&(ildjitSystem->DLA), ticket->method, &toBeCompiled);

            /* Compile in ahead of time the methods         */
            if (toBeCompiled) {
#ifndef DISABLE_PRIORITY_PIPE
                assert(methods != NULL);
                item = xanList_first(methods);
                while (item != NULL) {

                    /* Fetch the method			*/
                    DLAMethodToCompile = (dla_method_t *) item->data;
                    assert(DLAMethodToCompile->method != NULL);

                    /* Check the method			*/
#ifdef DEBUG
                    MethodDescriptor *methodID = DLAMethodToCompile->method->getID(DLAMethodToCompile->method);
                    assert(!methodID->attributes->is_abstract);
#endif

                    /* Insert the method into the pipeline	*/
                    (ildjitSystem->pipeliner).insertMethod(&(ildjitSystem->pipeliner), DLAMethodToCompile->method, DLAMethodToCompile->priority);

                    /* Fetch the next element from the list	*/
                    item = item->next;
                }
#else
                /* sort methods here and insert method with mininum priority	*/
                while (methods->length(methods) > 0) {
                    XanListItem *selectedItem = NULL;
                    dla_method_t *selectedDLAMethodToCompile = NULL;

                    item = xanList_first(methods);
                    while (item != NULL) {
                        DLAMethodToCompile = (dla_method_t *) item->data;

                        if (selectedDLAMethodToCompile == NULL || selectedDLAMethodToCompile->priority < DLAMethodToCompile->priority) {
                            selectedDLAMethodToCompile = DLAMethodToCompile;
                            selectedItem = item;
                        }
                        /* Fetch the next element from the list	*/
                        item = item->next;
                    }
                    methods->deleteItem(methods, selectedItem);

                    (ildjitSystem->pipeliner).insertMethod(&(ildjitSystem->pipeliner), DLAMethodToCompile->method, MIN_METHOD_PRIORITY);
                }
                methods = NULL;
#endif
            }

#ifdef MULTIAPP
            ticket->method->lock(ticket->method);
            PDEBUG("TRANSLATION PIPELINE (%d): DLA:		DLA done of %s (%p)\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method);
            ticket->method->setGlobalState(ticket->method, IROPT_PRE_GSTATE);
#endif

            /* Save DLA methods in ticket			*/
            PLATFORM_lockMutex(&(ticket->mutex));
            ticket->dlaMethods = methods;
            PLATFORM_unlockMutex(&(ticket->mutex));

#ifdef MULTIAPP
        } else if ( ticket->method->getGlobalState(ticket->method) == DLA_ONGOING_GSTATE ) {
            PDEBUG("TRANSLATION PIPELINE (%d): DLA:		Method %s (%p, MethodDescriptor %p) is not being DLA-ed by another process, wait for it\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
            ticket->method->waitTillGlobalState(ticket->method, IROPT_PRE_GSTATE);
        } else {
            PDEBUG("TRANSLATION PIPELINE (%d): DLA:		Method %s (%p, MethodDescriptor %p) is after DLA state\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
            assert(ticket->method->getGlobalState(ticket->method) > DLA_ONGOING_GSTATE);
        }

        ticket->method->unlock(ticket->method);
#endif
    }

    /* Profiling					*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {
        JITFLOAT32 wallTime;

        /* Store the time elapsed			*/
        wallTime = 0;
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).dlaPipe.profileMutex));
        (ildjitSystem->profiler).dla_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (ildjitSystem->profiler).wallTime.dla_time += wallTime;
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).dlaPipe.profileMutex));
    }

    /* Return					*/
    return JOB_END;
}

static inline JITUINT32 cilIRTranslator (t_ticket *ticket, XanVar *checkPoint) {
    ProfileTime startTime;
    XanListItem             *item;

    /* Assertions					*/
    assert(ticket != NULL);

    /* Profiling					*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {

        /* Store the start time				*/
        profilerStartTime(&startTime);
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    JITFLOAT32 wallTime;
    JITUINT32 elementsInPipe;
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).cilPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).cilPipe.lowPriority.pipe)->size + 1;
    fprintf(cilirProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Translate the method from CIL to IR		*/
#ifndef MULTIAPP
    PDEBUG("TRANSLATION PIPELINE: CIL->IR:	Start CIL->IR translation of %s\n", ticket->method->getName(ticket->method));
    translate_method_from_cil_to_ir(ildjitSystem, ticket->method);

    /* Set the state of the method			*/
    ticket->method->lock(ticket->method);
    ticket->method->setState(ticket->method, IR_STATE);
    ticket->method->unlock(ticket->method);
    PDEBUG("TRANSLATION PIPELINE: CIL->IR:		Translation done of %s\n", ticket->method->getName(ticket->method));
#else
    ticket->method->lock(ticket->method);
    if  ( ticket->method->getGlobalState(ticket->method) < CILIR_ONGOING_GSTATE ) {
        PDEBUG("TRANSLATION PIPELINE (%d): CIL->IR:	Method %s (%p, MethodDescriptor %p) is not being CIL->IR by anyone, I'll take it\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
        ticket->method->setGlobalState(ticket->method, CILIR_ONGOING_GSTATE);
        ticket->method->unlock(ticket->method);

        PDEBUG("TRANSLATION PIPELINE (%d): CIL->IR:	Start CIL->IR translation of %s\n", getpid(), ticket->method->getFullName(ticket->method));
        translate_method_from_cil_to_ir(ildjitSystem, ticket->method);

        ticket->method->lock(ticket->method);
        ticket->method->setGlobalState(ticket->method, DLA_PRE_GSTATE);
    } else if ( ticket->method->getGlobalState(ticket->method) == CILIR_ONGOING_GSTATE ) {
        PDEBUG("TRANSLATION PIPELINE (%d): CIL->IR:	Method %s (%p) is being CIL->IR by another process, wait for it\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method);
        ticket->method->waitTillGlobalState(ticket->method, DLA_PRE_GSTATE);
    } else {
        PDEBUG("TRANSLATION PIPELINE (%d): CIL->IR:		Method %s (%p) is IR\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method);
        assert(ticket->method->getGlobalState(ticket->method) > CILIR_ONGOING_GSTATE);
    }

    ticket->method->setState(ticket->method, IR_STATE);
    ticket->method->unlock(ticket->method);
    PDEBUG("TRANSLATION PIPELINE (%d): CIL->IR:	Translation done of %s (%p)\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method);
#endif

    /* Wakeup every threads that are waiting for	*
     * this method in IR language			*/
    PLATFORM_lockMutex(&(ticket->mutex));
    item = xanList_first(ticket->unoptimizedIR_notifyTickets);
    while (item != NULL) {
        pthread_cond_t  *currentNotify;
        currentNotify = (pthread_cond_t *) item->data;
        assert(currentNotify != NULL);
        PLATFORM_signalCondVar(currentNotify);
        item = item->next;
    }
    PLATFORM_unlockMutex(&(ticket->mutex));

    /* Profiling					*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {
        JITFLOAT32 wallTime;

        /* Store the time elapsed			*/
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).cilPipe.profileMutex));
        (ildjitSystem->profiler).cil_ir_translation_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (ildjitSystem->profiler).wallTime.cil_ir_translation_time += wallTime;
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).cilPipe.profileMutex));
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).cilPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).cilPipe.lowPriority.pipe)->size;
    fprintf(cilirProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Return					*/
    return JOB_END;
}

static inline JITUINT32 iROptimizer (t_ticket *ticket, XanVar *checkPoint) {
    ProfileTime startTime;
    ir_method_t     *ir_method;
    JITUINT32 state;

    /* Assertions					*/
    assert(ticket != NULL);

    /* Profiling				*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {

        /* Store the start time		*/
        profilerStartTime(&startTime);
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    JITFLOAT32 wallTime;
    JITUINT32 elementsInPipe;
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).irOptimizerPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).irOptimizerPipe.lowPriority.pipe)->size + 1;
    fprintf(irOptimizerProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Fetch the IR method			*/
    ir_method = ticket->method->getIRMethod(ticket->method);

    /* Optimize the method			*/
#ifndef MULTIAPP
    PDEBUG("TRANSLATION PIPELINE: IR optimization:	Start IR optimization of %s\n", ticket->method->getName(ticket->method));
    state = IROPTIMIZER_optimizeMethod_checkpointable(&((ildjitSystem->IRVM).optimizer), ir_method, ticket->jobState, checkPoint);
    PDEBUG("TRANSLATION PIPELINE: IR optimization:		Optimization done of %s\n", ticket->method->getName(ticket->method));
#else
    ticket->method->lock(ticket->method);

    /* Sanity checks			*/
#ifdef DEBUG
    if (	(ildjitSystem->IRVM).behavior.dla 	||
            (ildjitSystem->IRVM).behavior.jit 	||
            (ildjitSystem->IRVM).behavior.aot	) {
        assert( ticket->method->getGlobalState(ticket->method) >= IROPT_PRE_GSTATE );
    } else {
        assert( ticket->method->getGlobalState(ticket->method) >= DLA_PRE_GSTATE );
    }
#endif

    if ( ticket->method->getGlobalState(ticket->method) < IROPT_ONGOING_GSTATE ) {
        PDEBUG("TRANSLATION PIPELINE (%d): IR optimization:	Method %s (%p, MethodDescriptor %p) is not being IR optimized by anyone, I'll take it\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
        ticket->method->setGlobalState(ticket->method, IROPT_ONGOING_GSTATE);
        ticket->method->unlock(ticket->method);

        PDEBUG("TRANSLATION PIPELINE (%d): IR optimization:	Start IR optimization of %s\n", getpid(), ticket->method->getFullName(ticket->method));
        state = IROPTIMIZER_optimizeMethod_checkpointable(&((ildjitSystem->IRVM).optimizer), ir_method, ticket->jobState, checkPoint);

        ticket->method->lock(ticket->method);
        ticket->method->setGlobalState(ticket->method, IR_GSTATE);
    } else if ( ticket->method->getGlobalState(ticket->method) == IROPT_ONGOING_GSTATE ) {
        PDEBUG("TRANSLATION PIPELINE (%d): IR optimization:	Method %s (%p, MethodDescriptor %p) is being IR optimized by another process, wait for it\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
        ticket->method->waitTillGlobalState(ticket->method, IR_GSTATE);
        state = JOB_END;
    } else {
        PDEBUG("TRANSLATION PIPELINE (%d): IR optimization:	Method %s (%p, MethodDescriptor %p) is after IR optimization state\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
        assert(ticket->method->getGlobalState(ticket->method) > IROPT_ONGOING_GSTATE);
        state = JOB_END;
    }
    PDEBUG("TRANSLATION PIPELINE (%d): IR optimization:	IR optimization done of %s (%p, MethodDescriptor %p)\n", getpid(), ticket->method->getFullName(ticket->method), ticket->method, (ticket->method->IRMethod).ID);
    ticket->method->unlock(ticket->method);
#endif

    /* Profiling				*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {
        JITFLOAT32 wallTime;

        /* Store the time elapsed			*/
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).irOptimizerPipe.profileMutex));
        (ildjitSystem->profiler).ir_optimization_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (ildjitSystem->profiler).wallTime.ir_optimization_time += wallTime;
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).irOptimizerPipe.profileMutex));
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).irOptimizerPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).irOptimizerPipe.lowPriority.pipe)->size;
    fprintf(irOptimizerProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Return					*/
    return state;
}

static inline JITUINT32 iRMachineCodeTranslator (t_ticket *ticket, XanVar *checkPoint) {
    ProfileTime 	startTime;
    Method 		method;
    XanListItem     *item;

    /* Assertions					*/
    assert(ticket != NULL);

    /* Profiling						*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {

        /* Store the start time					*/
        profilerStartTime(&startTime);
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    JITFLOAT32 wallTime;
    JITUINT32 elementsInPipe;
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).irPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).irPipe.lowPriority.pipe)->size + 1;
    fprintf(irMachineCodeProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Cache the method					*/
    method = ticket->method;
    assert(method != NULL);

    /* Translate the method into machine code		*/
    PDEBUG("TRANSLATION PIPELINE: IR->machineCode:	Start IR->machineCode translation of %s\n", ticket->method->getName(ticket->method));
    CODE_generateMachineCode(&(ildjitSystem->codeGenerator), method);
    PDEBUG("TRANSLATION PIPELINE: IR->machineCode:		Translation done of %s\n", ticket->method->getName(ticket->method));

    /* Set the is_jitted flag				*/
    method->lock(method);
    method->setState(method, MACHINE_CODE_STATE);
    method->unlock(method);

    /* Wakeup every threads that are waiting for this method*
     * in machine code					*/
    PLATFORM_lockMutex(&(ticket->mutex));
    item = xanList_first(ticket->machineCode_notifyTickets);
    while (item != NULL) {
        pthread_cond_t  *currentNotify;
        currentNotify = (pthread_cond_t *) item->data;
        assert(currentNotify != NULL);
        PLATFORM_signalCondVar(currentNotify);
        item = item->next;
    }
    PLATFORM_unlockMutex(&(ticket->mutex));

    /* Profiling						*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {
        JITFLOAT32 wallTime;

        /* Store the stop time					*/
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).irPipe.profileMutex));
        (ildjitSystem->profiler).ir_machine_code_translation_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (ildjitSystem->profiler).wallTime.ir_machine_code_translation_time += wallTime;
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).irPipe.profileMutex));
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).irPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).irPipe.lowPriority.pipe)->size;
    fprintf(irMachineCodeProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Return						*/
    return JOB_END;
}

static inline JITUINT32 cctorTranslator (t_ticket *ticket, XanVar *checkPoint) {
    XanList                 *cctorMethodsToCall;
    ProfileTime 		startTime;
    StaticMemoryManager	*staticMemoryManager;
    Method 			method;
    pthread_t 		currentThread;

    /* Check if we need to run CCTORS		*/
    if (!ticket->runCCTORS) {

        /* Notify the end of the compilation of the method	*/
        internal_notifyCompilationEnd(ticket);

        return JOB_END;
    }

    /* Profiling					*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {

        /* Store the start time			*/
        profilerStartTime(&startTime);
    }
#ifdef PROFILE_METHODS_IN_COMPILATION
    JITFLOAT32 wallTime;
    JITUINT32 elementsInPipe;
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).cctorPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).cctorPipe.lowPriority.pipe)->size + 1;
    fprintf(cctorsProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Get the method to "translate" */
    method = ticket->method;
    assert(method != NULL);

    /* Get the static memory manager */
    staticMemoryManager = &(ildjitSystem->staticMemoryManager);

    /* Get current thread */
    currentThread = PLATFORM_getSelfThreadID();

    /* Register "translation in progress" */
    STATICMEMORY_registerTranslator(staticMemoryManager, method, currentThread);

    /* Add entry point type .cctor if requested */
    if ((method->isEntryPoint(method)) && (method->requireConservativeTypeInitialization(method)) ) {
        STATICMEMORY_fetchStaticObject(staticMemoryManager, method, method->getType(method));
    }

    /* Get list of Cctor to call            */
    cctorMethodsToCall = method->getCctorMethodsToCall(method);

    /* Check if the method is ready to be executed			*/
    if (!STATICMEMORY_areStaticConstructorsExecutable(cctorMethodsToCall)) {
        STATICMEMORY_makeStaticConstructorsExecutable(staticMemoryManager, cctorMethodsToCall, ticket->priority);
    }

    /* Call the cctor method					*/
    STATICMEMORY_callStaticConstructors(staticMemoryManager, cctorMethodsToCall);

    /* Set the state of the method to executable			*/
    PLATFORM_lockMutex(&(ticket->mutex));
    if (!ticket->trampolinesSet) {
        CODE_linkMethodToProgram(&(ildjitSystem->codeGenerator), ticket->method);
        ticket->trampolinesSet = JITTRUE;
    }
    PLATFORM_unlockMutex(&(ticket->mutex));

    /* Unregister translator        				*/
    STATICMEMORY_unregisterTranslator(staticMemoryManager, method);

    /* Set the state of the method					*/
    method->lock(method);
    method->setState(method, EXECUTABLE_STATE);
    method->unlock(method);

    /* Notify the ready state for the execution of the method	*/
    internal_notifyCompilationEnd(ticket);

    /* Profiling							*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 1) {
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).cctorPipe.profileMutex));
        ((ildjitSystem->pipeliner).methodsCompiled)++;
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).cctorPipe.profileMutex));
    } else if ((ildjitSystem->IRVM).behavior.profiler >= 2) {
        JITFLOAT32 wallTime;
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).cctorPipe.profileMutex));
        (ildjitSystem->profiler).average_pipeline_latency += profilerGetTimeElapsed(&(ticket->startTime), &wallTime);
        (ildjitSystem->profiler).wallTime.average_pipeline_latency += wallTime;
        (ildjitSystem->profiler).static_memory_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (ildjitSystem->profiler).wallTime.static_memory_time += wallTime;
        ((ildjitSystem->pipeliner).methodsCompiled)++;
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).cctorPipe.profileMutex));
    }

    /* Profile							*/
#ifdef PROFILE_METHODS_IN_COMPILATION
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    elementsInPipe = ((ildjitSystem->pipeliner).cctorPipe.highPriority.pipe)->size + ((ildjitSystem->pipeliner).cctorPipe.lowPriority.pipe)->size;
    fprintf(cctorsProfile, "%.3f %u\n", wallTime, elementsInPipe);
#endif

    /* Destroy the ticket						*/
    internal_destroyTicket(ticket);

    /* Return							*/
    return JOB_END;
}

static inline void internal_notifyCompilationEnd (t_ticket *ticket) {
    XanListItem     	*item;

    /* Notify the ready state for the execution of the method	*/
    PLATFORM_lockMutex(&(ticket->mutex));
    item = xanList_first(ticket->notifyTickets);
    while (item != NULL) {
        pthread_cond_t  *currentNotify;
        currentNotify = (pthread_cond_t *) item->data;
        assert(currentNotify != NULL);
        PLATFORM_signalCondVar(currentNotify);
        item = item->next;
    }
    PLATFORM_unlockMutex(&(ticket->mutex));

    return ;
}

static inline void internal_destroyTicket (t_ticket *ticket) {
    JITBOOLEAN toBeFreed = JITTRUE;

    /* Detach the ticket from the specific pipe where it was residing 	*/
    ticket->inPipe = NULL;

    /* Unlock the compilation pipeline				*/
    PLATFORM_lockMutex(&((ildjitSystem->pipeliner).mutex));

    /* Check if ticket is currently in scheduling pipeline	*/
#ifndef DISABLE_PRIORITY_PIPE
    if (	(ildjitSystem->IRVM).behavior.dla	||
            (ildjitSystem->IRVM).behavior.jit 	) {
        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).schedulerPipe.mutex));
        XanQueueItem *queueItem = xanHashTable_lookup(((ildjitSystem->pipeliner).schedulerPipe.methodsInScheduling), ticket);
        if ( queueItem != NULL) {
            toBeFreed = JITFALSE;
        }
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).schedulerPipe.mutex));
    }
#endif

    /* Delete the methods from the pipe				*/
    xanHashTable_removeElement((ildjitSystem->pipeliner).methodsInPipe, ticket->method);
    if (toBeFreed) {
        internal_deleteMethodIntoPipelineSet(&(ildjitSystem->pipeliner), ticket);
    }

    /* Check if there is no method inside the compilation pipeline	*/
    if (xanHashTable_elementsInside((ildjitSystem->pipeliner).methodsInPipe) == 0) {

        /* Broadcast the event "compilation pipeline empty"		*/
        PLATFORM_broadcastCondVar(&((ildjitSystem->pipeliner).emptyCondition));
    }

    /* Unlock the compilation pipeline				*/
    PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).mutex));

    return ;
}

static inline void internal_printMethodsInPipe (TranslationPipeline *pipe) {
    XanHashTableItem        *item;
    t_ticket        *t;
    JITINT32 count;

    /* Assertions				*/
    assert(pipe != NULL);
    assert(PLATFORM_trylockMutex(&(pipe->mutex)) != 0);

    fprintf(stderr, "BEGIN\n");
    count = 0;
    item = xanHashTable_first(pipe->methodsInPipe);
    while (item != NULL) {
        t = (t_ticket *) item->element;
        fprintf(stderr, "%d)	%s\n", count, IRMETHOD_getCompleteMethodName(&(t->method->IRMethod)));
        count++;
        item = xanHashTable_next(pipe->methodsInPipe, item);
    }
    fprintf(stderr, "END\n");

    /* Return				*/
    return;
}

static inline void internal_deleteMethodIntoPipelineSet (TranslationPipeline *pipe, t_ticket *ticket) {

    /* Assertions				*/
    assert(pipe != NULL);
    assert(ticket != NULL);

    /* Free the ticket			*/
    if (ticket->dlaMethods != NULL) {
        xanList_destroyListAndData(ticket->dlaMethods);
    }
    xanList_destroyList(ticket->notifyTickets);
    xanList_destroyList(ticket->unoptimizedIR_notifyTickets);
    xanList_destroyList(ticket->machineCode_notifyTickets);
    freeMemory(ticket);

    /* Return				*/
    return;
}

static inline t_ticket * internal_insertMethodIntoPipelineSet (TranslationPipeline *pipe, Method method, pthread_cond_t *notify, pthread_cond_t *unoptimizedIR_notify, pthread_cond_t *machineCode_notify) {
    t_ticket        *ticket;

    /* Assertions				*/
    assert(pipe != NULL);
    assert(method != NULL);
    assert(!(pipe->isInPipe(pipe, method)));

    /* Make a new ticket			*/
    ticket = allocMemory(sizeof(t_ticket));

    /* Profiling				*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 2) {
        profilerStartTime(&(ticket->startTime));
    }

    /* Initialize the new ticket		*/
    ticket->notifyTickets = xanList_new(allocFunction, freeFunction, NULL);
    ticket->unoptimizedIR_notifyTickets = xanList_new(allocFunction, freeFunction, NULL);
    ticket->machineCode_notifyTickets = xanList_new(allocFunction, freeFunction, NULL);

    /* Add the notify			*/
    if (notify != NULL) {
        xanList_insert(ticket->notifyTickets, notify);
    }
    if (unoptimizedIR_notify != NULL) {
        xanList_insert(ticket->unoptimizedIR_notifyTickets, unoptimizedIR_notify);
    }
    if (machineCode_notify != NULL) {
        xanList_insert(ticket->machineCode_notifyTickets, machineCode_notify);
    }

    /* Initialize the fields		*/
    pthread_mutexattr_t mutex_attr;
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&(ticket->mutex), &mutex_attr);
    ticket->dlaMethods = NULL;
    ticket->inPipe = NULL;
    ticket->pipeItem = NULL;
    ticket->trampolinesSet = JITFALSE;
    ticket->method = method;

    /* Insert to the pipeline		*/
    assert(!pipe->isInPipe(pipe, method));
    xanHashTable_insert(pipe->methodsInPipe, method, ticket);
    assert(pipe->isInPipe(pipe, method));

    /* Return				*/
    return ticket;
}

static inline JITINT16 internal_isInPipe (TranslationPipeline *pipe, Method method) {
    t_ticket        *ticket;

    /* Assertions				*/
    assert(pipe != NULL);
    assert(method != NULL);

    ticket = xanHashTable_lookup(pipe->methodsInPipe, method);
    if (ticket == NULL) {
        return 0;
    }
    assert(ticket != NULL);
    return 1;
}

static inline void internal_dumpPipe (XanPipe *pipe, JITINT8 *prefixString) {
    XanListItem     *item;
    t_ticket        *ticket		__attribute__((unused));

    /* Assertions				*/
    assert(pipe != NULL);
    PDEBUG("TRANSLATION PIPELINE (%d): dumpPipe: %s: Start\n", getpid(), prefixString);

    PDEBUG("TRANSLATION PIPELINE (%d): dumpPipe: %s:	Dump the pipe from first item to exit to last one\n", getpid(), prefixString);
    item = xanList_first(pipe->items);
    while (item != NULL) {
        ticket = (t_ticket *) xanList_getData(item);
        assert(ticket != NULL);
        assert(ticket->method != NULL);
        PDEBUG("TRANSLATION PIPELINE (%d): dumpPipe: %s:		%s\n", getpid(), prefixString, ticket->method->getName(ticket->method));
        item = item->next;
    }

    /* Return				*/
    PDEBUG("TRANSLATION PIPELINE (%d): dumpPipe: %s: Exit\n", getpid(), prefixString);
    return;
}

static inline void internal_destroyWorkersSet (workerSet_t *workers) {
    JITUINT32 count;

    /* Assertions				*/
    assert(workers != NULL);

    /* Send Dead ticket		*/
    for (count = 0; count < workers->threadsCount; count++) {
        PLATFORM_lockMutex(&(workers->mutex));
        xanQueue_put(workers->pipe, NULL, 2.0f);
        PLATFORM_signalCondVar(&(workers->wakeupCondition));
        PLATFORM_unlockMutex(&(workers->mutex));
    }

    /* Join the threads		*/
    for (count = 0; count < workers->threadsCount; count++) {
        PLATFORM_joinThread(workers->threads[count], NULL);
    }

    /* Destroy synchronization primitive	*/
    PLATFORM_destroyMutex(&(workers->mutex));
    PLATFORM_destroyCondVar(&(workers->wakeupCondition));

    /* Destroy Threads List		*/
    freeFunction(workers->threads);
}

static inline void internal_destroyILDJITPipe (ildjitPipe_t *pipe) {

    /* Assertions				*/
    assert(pipe != NULL);

    internal_destroyWorkersSet(&(pipe->highPriority));
    internal_destroyWorkersSet(&(pipe->lowPriority));
    PLATFORM_destroyMutex(&(pipe->profileMutex));
}

static inline void internal_destroySchedulerPipe (schedPipe_t *pipe) {

    /* Assertions				*/
    assert(pipe != NULL);

    /* Scheduler threads		*/
    PLATFORM_lockMutex(&(pipe->mutex));
    xanQueue_put(pipe->pipe, NULL, 2.0f);
    PLATFORM_signalCondVar(&(pipe->wakeupCondition));
    PLATFORM_unlockMutex(&(pipe->mutex));

    /* Join the threads		*/
    PLATFORM_joinThread(pipe->thread, NULL);

    xanQueue_destroyQueue(pipe->pipe);
    xanHashTable_destroyTable(pipe->methodsInScheduling);
    PLATFORM_destroyMutex(&(pipe->mutex));
    PLATFORM_destroyCondVar(&(pipe->wakeupCondition));
}

static inline void internal_initWokersSet (workerSet_t *workers, JITUINT32 threadsCount) {
    pthread_mutexattr_t mutex_attr;

    /* Assertions				*/
    assert(workers != NULL);

    workers->pipe = xanQueue_new(allocFunction, dynamicReallocFunction, freeFunction);
    workers->busyWorkers = xanQueue_new(allocFunction, dynamicReallocFunction, freeFunction);
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&(workers->mutex), &mutex_attr);
    PLATFORM_initCondVar(&(workers->wakeupCondition), NULL);
    workers->threads = allocFunction(sizeof(pthread_t) * threadsCount);
    workers->threadsCount = threadsCount;
}

static inline void internal_initILDJITPipe (ildjitPipe_t *pipe, JITUINT32 highPriorityThreadCount, JITUINT32 lowPriorityThreadCount) {
    pthread_mutexattr_t mutex_attr;

    /* Assertions				*/
    assert(pipe != NULL);
    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&(pipe->profileMutex), &mutex_attr);
    internal_initWokersSet(&(pipe->highPriority), highPriorityThreadCount);
    internal_initWokersSet(&(pipe->lowPriority), lowPriorityThreadCount);
}

static inline void internal_initSchedulerPipe (schedPipe_t *pipe) {
    pthread_mutexattr_t mutex_attr;

    /* Assertions				*/
    assert(pipe != NULL);

    pipe->pipe = xanQueue_new(allocFunction, dynamicReallocFunction, freeFunction);
    pipe->methodsInScheduling = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&(pipe->mutex), &mutex_attr);
    PLATFORM_initCondVar(&(pipe->wakeupCondition), NULL);
}

static inline void internal_changePriority (TranslationPipeline *pipe, ildjitPipe_t *fromPipe, t_ticket *ticket) {
    workerSet_t *lowPriority;
    workerSet_t *highPriority;

    /* Assertions				*/
    assert(pipe != NULL);
    assert(ticket != NULL);

    lowPriority = &(fromPipe->lowPriority);
    highPriority = &(fromPipe->highPriority);

    PLATFORM_lockMutex(&(lowPriority->mutex));
    PLATFORM_lockMutex(&(highPriority->mutex));
    PLATFORM_lockMutex(&(ticket->mutex));

    if (ticket->inPipe == fromPipe) {
        if (!ticket->inHighPriorityPipe && NEED_HIGH_PRIORITY_PIPE(ticket->priority)) {
            /* Refresh the diagnostic		*/
            (pipe->methodsMovedToHighPriority)++;

            /* remove ticket from low priority pipe */
            assert(ticket->pipeItem != NULL);
            xanQueue_removeItem(lowPriority->pipe, ticket->pipeItem);

            /* move ticket to high priority pipe */
            ticket->pipeItem = xanQueue_put((fromPipe->highPriority).pipe, ticket, ticket->priority);
            assert(ticket->pipeItem != NULL);

            /* refresh ticket state */
            ticket->inPipe = fromPipe;
            ticket->inHighPriorityPipe = JITTRUE;

            /* Check for checkpointable worker	*/
            internal_tryCheckPoint(&(fromPipe->highPriority), ticket);

        } else {
#ifndef DISABLE_PRIORITY_PIPE
            /* Refresh the diagnostic		*/
            (pipe->reprioritizedTicket)++;

            /* move update ticket priority only */
            if (ticket->inHighPriorityPipe) {
                xanQueue_changeItemPriority((fromPipe->highPriority).pipe, ticket->pipeItem, ticket->priority);

                /* Check for checkpointable worker	*/
                internal_tryCheckPoint(&(fromPipe->highPriority), ticket);

            } else {
                xanQueue_changeItemPriority((fromPipe->lowPriority).pipe, ticket->pipeItem, ticket->priority);

                /* Check for checkpointable worker	*/
                internal_tryCheckPoint(&(fromPipe->lowPriority), ticket);
            }
#endif
        }
    }
    PLATFORM_unlockMutex(&(ticket->mutex));
    PLATFORM_unlockMutex(&((fromPipe->highPriority).mutex));
    PLATFORM_unlockMutex(&((fromPipe->lowPriority).mutex));
}

static inline void DLAScheduler (schedPipe_t *pipe) {
    t_ticket        *ticket;
    XanList                 *methods;
    XanListItem             *item;
    JITBOOLEAN toBeCompiled;

    /* Assertions		*/
    assert(pipe != NULL);

    while (1) {

        /* Check if the thread has to go to sleep	*/
        PLATFORM_lockMutex(&(pipe->mutex));
        while (xanQueue_isEmpty(pipe->pipe)) {

            /* The thread has to wait that some method enters in one pipe at least	*/
            PLATFORM_waitCondVar(&(pipe->wakeupCondition), &(pipe->mutex));
        }

        /* Fetch the item on the high priority pipe	*/
        ticket = (t_ticket *) xanQueue_get(pipe->pipe);
        PLATFORM_unlockMutex(&(pipe->mutex));
        if (ticket == NULL) {
            break;
        }
        assert(ticket->method != NULL);


        /* Update scheduling Pipe		*/
        PLATFORM_lockMutex(&(ticket->mutex));
        methods = ticket->dlaMethods;
        PLATFORM_unlockMutex(&(ticket->mutex));


        if (methods != NULL) {
            /* Update the priority of callable methods	*/
            (ildjitSystem->DLA).updateMethodsToCompile(&(ildjitSystem->DLA), ticket->method, methods, &toBeCompiled);
            if (toBeCompiled) {
                /* Reinsert methods in the pipeline			*/
                item = xanList_first(methods);
                while (item != NULL) {
                    dla_method_t *currentDLAMethod = (dla_method_t *) item->data;
                    (ildjitSystem->pipeliner).insertMethod(&(ildjitSystem->pipeliner), currentDLAMethod->method, currentDLAMethod->priority);
                    item = item->next;
                }
            }
        }

        PLATFORM_lockMutex(&((ildjitSystem->pipeliner).mutex));
        PLATFORM_lockMutex(&(pipe->mutex));
        if (!internal_isInPipe(&(ildjitSystem->pipeliner), ticket->method)) {
            internal_deleteMethodIntoPipelineSet(&(ildjitSystem->pipeliner), ticket);
        }
        xanHashTable_removeElement(pipe->methodsInScheduling, ticket);
        PLATFORM_unlockMutex(&(pipe->mutex));
        PLATFORM_unlockMutex(&((ildjitSystem->pipeliner).mutex));
    }

    /* Return			*/
    return;
}

static inline void internal_destroyThePipeline (TranslationPipeline *pipe) {

    /* Assertions			*/
    assert(pipe != NULL);

    /* Check if the pipeline is     *
     * running actually		*/
    if (!pipe->running) {
        return;
    }

    /* Wait the pipeline becomes    *
     * empty			*/
    internal_waitEmptyPipeline(pipe);

    /* Kill the wait empty pipeline	*
     * thread			*/
    PLATFORM_lockMutex(&(pipe->mutex));
    exitPipeCondition = JITTRUE;
    PLATFORM_broadcastCondVar(&(pipe->emptyCondition));
    PLATFORM_unlockMutex(&(pipe->mutex));

    /* Join the threads	empty pipeline thread	*/
    PLATFORM_joinThread(pipe->waitPipelineThread, NULL);

    /* Kill the Ticket Rescheduler thread		*/
#ifndef DISABLE_PRIORITY_PIPE
    if (	(ildjitSystem->IRVM).behavior.dla	||
            (ildjitSystem->IRVM).behavior.jit 	) {
        internal_destroySchedulerPipe(&((pipe->schedulerPipe)));
    }
#endif

    /* Destroy the pipes		*/
    internal_destroyILDJITPipe(&((pipe->cilPipe)));
    if (	(ildjitSystem->IRVM).behavior.dla 	||
            (ildjitSystem->IRVM).behavior.jit 	||
            (ildjitSystem->IRVM).behavior.aot	) {
        internal_destroyILDJITPipe(&((pipe->dlaPipe)));
    }
    internal_destroyILDJITPipe(&((pipe->irPipe)));
    internal_destroyILDJITPipe(&((pipe->irOptimizerPipe)));
    internal_destroyILDJITPipe(&((pipe->cctorPipe)));

    /* Destroy the notifies		*/
    PLATFORM_destroyMutex(&(pipe->mutex));
    PLATFORM_destroyCondVar(&(pipe->emptyCondition));
    xanHashTable_destroyTable(pipe->methodsInPipe);

    /* Close the profile files	*/
#ifdef PROFILE_CCTORS
    fclose(cctorsProfile);
#endif
#ifdef PROFILE_NEEDED_CCTORS
    fclose(cctorsProfile);
#endif
#ifdef PROFILE_METHODS_IN_COMPILATION
    fclose(cilirProfile);
    fclose(irOptimizerProfile);
    fclose(irMachineCodeProfile);
    fclose(cctorsProfile);
#endif
    pipe->running = JITFALSE;

    /* Redirect the calls to dummy	*
     * functions			*/
    pipe->declareWaitingCCTOR = internal_declareWaitingCCTOR_dummy;
    pipe->declareWaitingCCTORDone = internal_declareWaitingCCTORDone_dummy;
    pipe->isInPipe = internal_isInPipe_dummy;
    pipe->waitEmptyPipeline = internal_waitEmptyPipeline_dummy;
    pipe->isCCTORThread = isCCTORThread_dummy;
    pipe->destroyThePipeline = internal_destroyThePipeline_dummy;
    pipe->synchInsertMethod = insert_method_into_pipeline_dummy;
    pipe->synchTillIRMethod = internal_synchTillIRMethod_dummy;
    pipe->insertMethod = insert_method_into_pipeline_async_dummy;

    /* Return			*/
    return;
}

static inline JITBOOLEAN isCCTORThread (TranslationPipeline* self, pthread_t thread) {
    PLATFORM_lockMutex(&(self->mutex));
    JITBOOLEAN result = internal_isCCTORThread(self, thread);
    PLATFORM_unlockMutex(&(self->mutex));

    /* Return			*/
    return result;
}

/* Check if thread is one of the cctor stage */
static inline JITBOOLEAN internal_isCCTORThread (TranslationPipeline* self, pthread_t thread) {
    JITINT32 i;

    workerSet_t *lowPriority = &((self->cctorPipe).lowPriority);

    /* Check all cctor threads      */
    for (i = 0; i < lowPriority->threadsCount; i++) {
        if (PLATFORM_equalThread(thread, (lowPriority->threads)[i])) {
            return JITTRUE;
        }
    }

    workerSet_t *highPriority = &((self->cctorPipe).highPriority);
    for (i = 0; i < highPriority->threadsCount; i++) {
        if (PLATFORM_equalThread(thread, (highPriority->threads)[i])) {
            return JITTRUE;
        }
    }

    /* Return			*/
    return JITFALSE;
}

static inline void internal_waitEmptyPipeline (TranslationPipeline *pipe) {

    /* Assertions			*/
    assert(pipe != NULL);

    PLATFORM_lockMutex(&(pipe->mutex));
    while (xanHashTable_elementsInside(pipe->methodsInPipe) > 0) {
        PLATFORM_waitCondVar(&(pipe->emptyCondition), &(pipe->mutex));
    }
    PLATFORM_unlockMutex(&(pipe->mutex));
}

static inline t_ticket * internal_insert_method_into_the_right_pipeline (TranslationPipeline *pipe, Method method, JITFLOAT32 priority, pthread_cond_t *notify, pthread_cond_t *unoptimizedIR_notify, pthread_cond_t *machineCode_notify, JITUINT32 methodState, JITBOOLEAN runCCTORS) {
    t_ticket	*ticket;
    ildjitPipe_t	*inPipe;
    char		buf[DIM_BUF];

    /* Assertions                                           */
    assert(pipe != NULL);
    assert(method != NULL);

    /* Initialize the variables                             */
    ticket = NULL;

    /* Check if the method is already present into the pipe	*/
    if (pipe->isInPipe(pipe, method)) {

        /* Fetch the ticket of the method	*/
        ticket = xanHashTable_lookup(pipe->methodsInPipe, method);
        assert(ticket != NULL);

        /* Lock the ticket                      */
        PLATFORM_lockMutex(&(ticket->mutex));

        /* Fetch the pipe ID where the method is*/
        inPipe = ticket->inPipe;
        assert(ticket->method == method);

        /* Add the notify			*/
        if (notify != NULL) {
            xanList_insert(ticket->notifyTickets, notify);
        }
        if (unoptimizedIR_notify != NULL) {
            xanList_insert(ticket->unoptimizedIR_notifyTickets, unoptimizedIR_notify);
        }
        if (machineCode_notify != NULL) {
            xanList_insert(ticket->machineCode_notifyTickets, machineCode_notify);
        }

        /* Unlock the ticket                    */
        if (priority > ticket->priority) {

            /* Refresh the priority			*/
            ticket->priority = priority;

            /* Move the ticket in correct priority	*
             * queue.                               */
            PLATFORM_unlockMutex(&(ticket->mutex));

            /* Move at the end of the pipe the	*
             * method				*/
            if (inPipe != NULL) {
                internal_changePriority(pipe, inPipe, ticket);
            }

            /* Send ticket to scheduling */
#ifndef DISABLE_PRIORITY_PIPE
            if (	(ildjitSystem->IRVM).behavior.dla	||
                    (ildjitSystem->IRVM).behavior.jit	) {
                PLATFORM_lockMutex(&((pipe->schedulerPipe).mutex));
                PLATFORM_lockMutex(&(ticket->mutex));
                XanQueueItem *queueItem = xanHashTable_lookup((pipe->schedulerPipe).methodsInScheduling, ticket);
                if ( queueItem == NULL) {
                    queueItem = xanQueue_put((pipe->schedulerPipe).pipe, ticket, ticket->priority);
                    PLATFORM_signalCondVar(&((pipe->schedulerPipe).wakeupCondition));
                    xanHashTable_insert((pipe->schedulerPipe).methodsInScheduling, ticket, queueItem);
                } else {
                    xanQueue_changeItemPriority((pipe->schedulerPipe).pipe, queueItem, ticket->priority);
                }
                PLATFORM_unlockMutex(&(ticket->mutex));
                PLATFORM_unlockMutex(&((pipe->schedulerPipe).mutex));
            }
#endif
        } else {
            PLATFORM_unlockMutex(&(ticket->mutex));
        }
        assert(pipe->isInPipe(pipe, method));

    } else {
        assert(!(pipe->isInPipe(pipe, method)));

        /* Insert the method into the pipeline set		*/
        ticket = internal_insertMethodIntoPipelineSet(pipe, method, notify, unoptimizedIR_notify, NULL);
        assert(pipe->isInPipe(pipe, method));
        assert(ticket != NULL);
        assert(ticket->method == method);

        /* Set the ticket information				*/
        PLATFORM_lockMutex(&(ticket->mutex));
        ticket->priority 	= priority;
        ticket->runCCTORS	= runCCTORS;
        PLATFORM_unlockMutex(&(ticket->mutex));

        /* Move the method to the right pipeline		*/
        JITBOOLEAN useHighPriorityPipe = NEED_HIGH_PRIORITY_PIPE(priority);
        if (useHighPriorityPipe) {
            (pipe->methodsCompiledInHighPriority)++;
        }

        /* Put at the end of the right pipe the method		*/
        switch (methodState) {
            case CIL_STATE:
                internal_putToPipe(&(pipe->cilPipe), useHighPriorityPipe, ticket);
                break;
            case IR_STATE:
                internal_putToPipe(&(pipe->irPipe), useHighPriorityPipe, ticket);
                break;
            case MACHINE_CODE_STATE:
                internal_putToPipe(&(pipe->cctorPipe), useHighPriorityPipe, ticket);
                break;
            case NEEDS_RECOMPILE_STATE:
                internal_putToPipe(&(pipe->irOptimizerPipe), useHighPriorityPipe, ticket);
                break;
            default:
                snprintf(buf, sizeof(char) * 1024, "PIPELINER: ERROR: state %u is not correct for the %s CIL method. ", methodState, method->getName(method));
                print_err(buf, 0);
                abort();
        }
    }

    /* Return                       */
    return ticket;
}

static inline void internal_tryCheckPoint (workerSet_t *destinationWorkers, t_ticket *ticket) {

#ifndef DISABLE_CHECKPOINTING
    JITFLOAT32 lowestPriority = xanQueue_headPriority(destinationWorkers->busyWorkers);
    if (destinationWorkers->steadyThreadsCount >= destinationWorkers->pipe->size) {

        /* Exit at least one work that is steady	*/
        PLATFORM_signalCondVar(&(destinationWorkers->wakeupCondition));

    } else if (ticket->priority > (MAX_METHOD_PRIORITY - lowestPriority)) {

        /* Exist at least one worker that does not received checkpoint request
         * send checkpoint request to lowest priority worker
         */
        checkPoint_t *destinationCheckPoint = (checkPoint_t *) xanQueue_get(destinationWorkers->busyWorkers);
        destinationCheckPoint->pipeItem = NULL;
        xanVar_write(destinationCheckPoint->variable, (void *) (JITNUINT) JITTRUE);
    }
#else
    PLATFORM_signalCondVar(&(destinationWorkers->wakeupCondition));
#endif
}

static inline void internal_putToPipe (ildjitPipe_t *destinationPipe, JITBOOLEAN destinateToHighPriority,t_ticket *ticket) {
    workerSet_t *destinationWorkers;

    if (destinateToHighPriority) {
        destinationWorkers = &(destinationPipe->highPriority);
    } else {
        destinationWorkers = &(destinationPipe->lowPriority);
    }

    PLATFORM_lockMutex(&(destinationWorkers->mutex));
    PLATFORM_lockMutex(&(ticket->mutex));

    /* Insert ticket in pipe and refresh its state	*/
    ticket->pipeItem = xanQueue_put(destinationWorkers->pipe, ticket, ticket->priority);
    assert(ticket->pipeItem != NULL);
    ticket->inHighPriorityPipe = destinateToHighPriority;
    ticket->inPipe = destinationPipe;

    /* Check for checkpointable worker	*/
    internal_tryCheckPoint(destinationWorkers, ticket);

    PLATFORM_unlockMutex(&(ticket->mutex));
    PLATFORM_unlockMutex(&(destinationWorkers->mutex));
}

static inline void internal_declareWaitingCCTOR (struct TranslationPipeline *pipeline) {
    PLATFORM_lockMutex(&(pipeline->mutex));
    internal_decreaseSteadyCCTORThreads(pipeline);
    PLATFORM_unlockMutex(&(pipeline->mutex));
}

static inline void internal_decreaseSteadyCCTORThreads (TranslationPipeline *pipeline) {
    JITUINT32 busyThreads;

    /* Decrease the number of threads that	*
     * are steady				*/
    (pipeline->steady_cctor_threads)--;

    /* Compute the number of threads that	*
     * are busy				*/
    busyThreads = ((pipeline->cctorPipe).lowPriority.threadsCount + (pipeline->cctorPipe).highPriority.threadsCount - pipeline->steady_cctor_threads);

    /* Profile				*/
#ifdef PROFILE_NEEDED_CCTORS
    JITFLOAT32 wallTime;
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    fprintf(cctorsProfile, "%.3f %d\n", wallTime, busyThreads + 1);
#endif

    /* Check if we should spawn new threads	*/
    if (busyThreads >= (((pipeline->cctorPipe).lowPriority.threadsCount + (pipeline->cctorPipe).highPriority.threadsCount) / 2)) {
#ifdef NO_CCTORS_MANAGER
        cctors_delta = 1;
#endif
        internal_startMoreCCTORThreads(pipeline, cctors_delta, cctors_delta);
        cctors_delta *= 2;
#ifdef NO_CCTORS_MANAGER
        cctors_delta = 1;
#endif
    }

    /* Return				*/
    return;
}

static inline void internal_declareWaitingCCTORDone (struct TranslationPipeline *pipeline) {
    PLATFORM_lockMutex(&(pipeline->mutex));
    internal_increaseSteadyCCTORThreads(pipeline);
    PLATFORM_unlockMutex(&(pipeline->mutex));
}

static inline void internal_increaseSteadyCCTORThreads (TranslationPipeline *pipeline) {

    /* Decrease the number of threads that	*
     * are steady				*/
    (pipeline->steady_cctor_threads)++;

    /* Profile				*/
#ifdef PROFILE_NEEDED_CCTORS
    JITFLOAT32 wallTime;
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    fprintf(cctorsProfile, "%.3f %d\n", wallTime, pipeline->cctor_threads - pipeline->steady_cctor_threads + 1);
#endif

    /* Return				*/
    return;
}

static inline void internal_killWorkersSet (workerSet_t *workers) {
    JITINT32 count;

    for (count = 0; count < workers->threadsCount; count++) {
        PLATFORM_lockMutex(&(workers->mutex));
        xanQueue_put(workers->pipe, NULL, 2.0f);
        PLATFORM_signalCondVar(&(workers->wakeupCondition));
        PLATFORM_unlockMutex(&(workers->mutex));
    }

    /* Join the high priority threads			*/
    for (count = 0; count < workers->threadsCount; count++) {
        PLATFORM_joinThread(workers->threads[count], NULL);
    }

    workers->threadsCount = 0;
}

static inline void internal_killILDJITPipe (ildjitPipe_t *pipe) {
    internal_killWorkersSet(&(pipe->highPriority));
    internal_killWorkersSet(&(pipe->lowPriority));
}

static inline void internal_killCCTORThreads (TranslationPipeline *pipeline) {

    /* Assertions						*/
    assert(pipeline != NULL);

    /* Kill the threads			*/
    internal_killILDJITPipe(&(pipeline->cctorPipe));

    /* Refresh the number of cctor	*
     * threads.			*/
    pipeline->steady_cctor_threads = 0;
    cctors_delta = MIN_CCTORS_DELTA;

    /* Return			*/
    return;
}

static inline void internal_startMoreThreadsWorkersSet (workerSet_t *workers, JITBOOLEAN isHighPriorityWorkersSet, void * (*startWorker)(void *), JITINT32 num) {
    JITINT32 count;
    JITINT32 newThreadsCount;
    pthread_attr_t tattr;

    internal_initSchedulerAttributes(&tattr, isHighPriorityWorkersSet);

    /* Compute the new number of cctor threads		*/
    newThreadsCount = workers->threadsCount + num;

    /* Reallocate the memory for cctor threads		*/
    workers->threads = reallocMemory(workers->threads, sizeof(pthread_t) * newThreadsCount);
#ifdef DEBUG
    memset(&(workers->threads[workers->threadsCount]), 0, sizeof(pthread_t) * num);
#endif

    for (count = 0; count < num; count++) {
        (ildjitSystem->garbage_collectors).gc.threadCreate(&((workers->threads)[count + workers->threadsCount]), &tattr, startWorker, (void *) (JITNUINT) isHighPriorityWorkersSet);
    }

    workers->threadsCount = newThreadsCount;
}

static inline void internal_startMoreThreadsILDJITPipe (ildjitPipe_t *pipe, void * (*startWorker)(void *), JITINT32 numLow, JITINT32 numHigh) {

    /* Assertions						*/
    assert(pipe != NULL);

    /* Start the high priority threads			*/
    internal_startMoreThreadsWorkersSet(&(pipe->highPriority), JITTRUE, startWorker, numHigh);

    /* Start the low priority threads			*/
    internal_startMoreThreadsWorkersSet(&(pipe->lowPriority), JITFALSE, startWorker, numLow);
}

static inline void internal_startMoreCCTORThreads (TranslationPipeline *pipeline, JITINT32 numLow, JITINT32 numHigh) {

    /* Start new threads			*/
    internal_startMoreThreadsILDJITPipe(&(pipeline->cctorPipe), start_cctor, numLow, numHigh);

    /* Refresh the number of cctor	*
     * threads.			*/
    pipeline->steady_cctor_threads += numLow + numHigh;

    /* Profiling					*/
#ifdef PROFILE_CCTORS
    JITFLOAT32 wallTime;

    /* Fetch the elapsed time from the beginning	*/
    profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
    fprintf(cctorsProfile, "%.3f %d\n", wallTime, pipeline->cctor_threads);
#endif

    /* Return			*/
    return;
}

static inline XanList * internal_getReachableMethods (ir_method_t *irMethod) {
    ir_instruction_t        *inst;
    XanList                 *list;
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions			*/
    assert(irMethod != NULL);

    /* Lock the IR method		*/
    IRMETHOD_lock(irMethod);

    /* Allocate the list of methods	*
     * reachable at one step from	*
     * the one given as input.	*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Fetch the number of          *
     * instructions of the method	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(irMethod);

    for (count = 0; count < instructionsNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(irMethod, count);
        assert(inst != NULL);
        if (IRMETHOD_isACallInstruction(inst)) {
            XanList *tmpList;
            tmpList = IRMETHOD_getCallableMethods(inst);
            assert(tmpList != NULL);
            xanList_appendList(list, tmpList);
            xanList_destroyList(tmpList);
            continue;
        }
        switch (IRMETHOD_getInstructionParametersNumber(inst)) {
            case 3:
                internal_add_functions(list, IRMETHOD_getInstructionParameter3(inst));
            case 2:
                internal_add_functions(list, IRMETHOD_getInstructionParameter2(inst));
            case 1:
                internal_add_functions(list, IRMETHOD_getInstructionParameter1(inst));
        }
        if (inst->callParameters != NULL) {
            XanListItem     *item;
            item = xanList_first(inst->callParameters);
            while (item != NULL) {
                internal_add_functions(list, (ir_item_t *) item->data);
                item = item->next;
            }
        }
    }

    /* Unlock the IR method		*/
    IRMETHOD_unlock(irMethod);

    /* Return			*/
    return list;
}

static inline void internal_add_functions (XanList *list, ir_item_t *item) {
    void            *entryPoint;
    Method methodToCompile;

    if (item->internal_type == IRFPOINTER) {
        if (item->type == IRSYMBOL) {
            entryPoint = (void *) (JITNUINT) (IRSYMBOL_resolveSymbol((ir_symbol_t *) (JITNUINT) (item->value).v).v);
        } else {
            entryPoint = (void *) (JITNUINT) (item->value).v;
        }
        if (entryPoint != NULL) {
            methodToCompile = (Method) xanHashTable_lookup((ildjitSystem->cliManager).methods.functionsEntryPoint, entryPoint);
            if (methodToCompile != NULL) {
                xanList_append(list, methodToCompile);
            }
        }
    }
}

static inline void * waitEmptyPipelineThread (void *data) {
    TranslationPipeline *pipe;

    /* Fetch the system		*/
    pipe = (TranslationPipeline *) data;

    /* Lock the pipeline				*/
    PLATFORM_lockMutex(&(pipe->mutex));

    /* Perform the job		*/
    while (1) {

        /* Notify that the thread is started		*/
        if (!pipelineIsStarted) {
            internal_notifyThePipelineIsStarted();
        }
        assert(pipelineIsStarted);

        /* Wait that the pipeline is empty		*/
        PLATFORM_waitCondVar(&(pipe->emptyCondition), &(pipe->mutex));

        /* Check the exit condition                     */
        if (exitPipeCondition) {
            break;
        }

        /* Reduce the cctor threads			*/
#ifndef NO_CCTORS_MANAGER
        if (    (xanHashTable_elementsInside(pipe->methodsInPipe) == 0) &&
                ((pipe->cctorPipe).lowPriority.threadsCount +  (pipe->cctorPipe).highPriority.threadsCount> 2)                                  ) {

            /* Kill all cctor threads			*/
            internal_killCCTORThreads(pipe);

            /* Start 2 cctor threads			*/
            internal_startMoreCCTORThreads(pipe, 1, 1);

            /* Profiling					*/
#ifdef PROFILE_CCTORS
            JITFLOAT32 wallTime;

            /* Fetch the elapsed time from the beginning	*/
            profilerGetTimeElapsed(&((ildjitSystem->profiler).startTime), &wallTime);
            fprintf(cctorsProfile, "%.3f %d\n", wallTime, pipe->cctor_threads);
#endif
        }
#endif
    }

    /* Unlock the pipeline				*/
    PLATFORM_unlockMutex(&(pipe->mutex));

    /* Return		*/
    return NULL;
}

static inline void internal_waitStartPipeline (void) {
    PLATFORM_lockMutex(&startPipeMutex);
    while (!pipelineIsStarted) {
        PLATFORM_waitCondVar(&startPipeCondition, &startPipeMutex);
    }
    PLATFORM_unlockMutex(&startPipeMutex);
}

static inline void internal_notifyThePipelineIsStarted (void) {
    PLATFORM_lockMutex(&startPipeMutex);
    if (!pipelineIsStarted) {
        pipelineIsStarted = JITTRUE;
        PLATFORM_broadcastCondVar(&startPipeCondition);
    }
    PLATFORM_unlockMutex(&startPipeMutex);
}

/*********** Dummy functions ********************/
static inline void internal_declareWaitingCCTOR_dummy (struct TranslationPipeline *pipeline) {
    return;
}

static inline void internal_declareWaitingCCTORDone_dummy (struct TranslationPipeline *pipeline) {
    return;
}

static inline JITINT16 internal_isInPipe_dummy (TranslationPipeline *pipe, Method method) {
    return JITFALSE;
}

static inline void internal_waitEmptyPipeline_dummy (TranslationPipeline *pipe) {
    return;
}

static inline JITBOOLEAN isCCTORThread_dummy (TranslationPipeline* self, pthread_t thread) {
    return JITFALSE;
}

static inline void internal_destroyThePipeline_dummy (TranslationPipeline *pipe) {
    return;
}

static inline void insert_method_into_pipeline_dummy (TranslationPipeline *pipe, Method method, JITFLOAT32 priority) {
    return;
}

static inline void internal_synchTillIRMethod_dummy (TranslationPipeline *pipe, Method method, JITFLOAT32 priority) {
    return;
}

static inline JITINT32 insert_method_into_pipeline_async_dummy (TranslationPipeline *pipe, Method method, JITFLOAT32 priority) {
    return 0;
}
