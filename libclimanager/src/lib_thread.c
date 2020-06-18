/*
 * Copyright (C) 2006  Campanoni Simone
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
#include <lib_thread.h>
#include <errno.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <ir_optimization_interface.h>
#include <platform_API.h>

// My headers
#include <climanager_system.h>
// End

/* Location locks hash table default size */
#define DEFAULT_LOCATION_LOCKS_SIZE 11

/* Default size of per thread stack (4 Mb) */
#define DEFAULT_THREAD_STACK_SIZE 1024 * 1024 * 4

/* Macros to check for thread state attribute */
#define THREAD_UNSTARTED(info) \
	((tm_readThreadState(info) & THREAD_STATE_UNSTARTED) ==	\
	 THREAD_STATE_UNSTARTED)
#define THREAD_STOPPED(info) \
	((tm_readThreadState(info) & THREAD_STATE_STOPPED) == \
	 THREAD_STATE_STOPPED)
#define THREAD_RUNNING(info) \
	((tm_readThreadState(info) & THREAD_STATE_RUNNING) == \
	 THREAD_STATE_RUNNING)
#define THREAD_ABORTED(info) \
	((tm_readThreadState(info) & THREAD_STATE_ABORTED) == \
	 THREAD_STATE_ABORTED)
#define THREAD_ABORTREQUESTED(info) \
	((tm_readThreadState(info) & THREAD_STATE_ABORTREQUESTED) == \
	 THREAD_STATE_ABORTREQUESTED)

/* Macros to check for event properties */
#define EVENT_SIGNALLED(event) (event->signalled == JITTRUE)
#define AUTORESET_EVENT(event) (event->manualReset == JITFALSE)

/* Function casts */
#define TM_NATIVETHREADRUN (void* (*)(void*))tm_nativeThreadRun

/* Current thread description access macro */
#define GET_CURRENT_THREAD_DESCRIPTION() (currentThreadDescription)
#define SET_CURRENT_THREAD_DESCRIPTION(description) \
	(currentThreadDescription = description)

/* Check what BCL is loaded */
#define MONO_BCL_IS_LOADED(self) \
	(self->privateCtor == NULL)

extern CLIManager_t *cliManager;
static pthread_once_t tm_metadataLock = PTHREAD_ONCE_INIT;

static void tm_initialize (void);
static void tm_shutdownThreadManager (t_threadsManager *self);
static void* tm_getCurrentThread (void);
static void tm_waitThreads (void);
static void tm_lockLocation (void* location);
static void tm_unlockLocation (void* location);
static void tm_memoryBarrier (void);
static void tm_sleep (JITINT32 milliseconds);
static JITBOOLEAN tm_lockObject (void* object, JITINT32 milliseconds);
static void tm_unlockObject (void* object);
static JITBOOLEAN tm_waitForPulse (void* object, JITINT32 milliseconds);
static void tm_signalPulse (void* object);
static void tm_signalPulseAll (void* object);
static void tm_initializeThread (void* thread);
static void tm_finalizeThread (void* thread);
static void tm_startThread (void* thread);
static JITBOOLEAN tm_joinToThread (void* thread, JITINT32 milliseconds);
static JITINT32 tm_getThreadState (void* thread);
static void tm_setPrivateData (void* thread, CLRThread* threadInfos);
static CLRThread* tm_getPrivateData (void* thread);
static void tm_setStartDelegate (void* thread, void* startRoutine);
static void* tm_getStartDelegate (void* thread);
static CLRWaitEvent* tm_createEvent (JITBOOLEAN manualReset, JITBOOLEAN initialState);
static void tm_destroyEvent (CLRWaitEvent* event);
static JITBOOLEAN tm_signalEvent (CLRWaitEvent* event);
static JITBOOLEAN tm_waitOnEvent (CLRWaitEvent* event, JITINT32 timeout);
static JITBOOLEAN tm_waitAllEvents (void* handles, JITINT32 timeout, JITBOOLEAN exitContext);
static CLRLocalMutex* tm_createLocalMutex (JITBOOLEAN initiallyOwned);
static void tm_lockLocalMutex (CLRLocalMutex* mutex);
static void tm_unlockLocalMutex (CLRLocalMutex* mutex);
static void tm_constrain (CLRThread* threadInfo_src, CLRThread* threadInfo_dest);
static void tm_movethread (CLRThread* threadInfo_src, JITUINT32 core);
static XanList* tm_getFreeCPUs (void);
/* Private functions prototypes */
static CLRThread* tm_buildCurrentThreadDescription (void);
static JITBOOLEAN tm_initialize_mono_thread (void* thread, CLRThread* threadinfo);
static JITBOOLEAN tm_initialize_pnet_thread (void* thread, CLRThread* threadinfo);
static void tm_waitForegroundThread (CLRThread* threadInfos);
static void tm_killUnstartedThread (CLRThread* threadInfos);
static void tm_initRecursiveMutexDescription (void);
static void tm_initThreadDefaultAttributes (void);
static void tm_initThreadStateMonitor (CLRThread* threadInfos);
static void tm_destroyThreadStateMonitor (CLRThread* threadInfo);
static void tm_lockThreadState (CLRThread* threadInfos);
static void tm_unlockThreadState (CLRThread* threadInfos);
static void tm_broadcastSignalThreadState (CLRThread* threadInfos);
static void tm_waitThreadState (CLRThread* threadInfos);
static void tm_initEventMonitor (CLRWaitEvent* event);
static void tm_destroyEventMonitor (CLRWaitEvent* event);
static void tm_lockEvent (CLRWaitEvent* event);
static void tm_unlockEvent (CLRWaitEvent* event);
static void tm_unicastSignalEvent (CLRWaitEvent* event);
static void tm_broadcastSignalEvent (CLRWaitEvent* event);
static void tm_waitEventSignal (CLRWaitEvent* event);
static void tm_initLocalMutexLock (CLRLocalMutex* mutex, JITBOOLEAN locked);
static void tm_lockLocalMutexLock (CLRLocalMutex* mutex);
static void tm_unlockLocalMutexLock (CLRLocalMutex* mutex);
static pthread_mutex_t* tm_getLock (void* location);
static void tm_loadMetadata (void);
static void tm_createNativeThread (CLRThread* threadInfos);
static void tm_joinToNativeThread (CLRThread* threadInfos);
static void* tm_nativeThreadRun (CLRThread* threadInfos);
static JITBOOLEAN tm_waitForLaunch (CLRThread* threadInfos);
static void tm_changeThreadState (CLRThread* threadInfos, JITINT32 state);
static inline void tm_writeThreadState (CLRThread* threadInfo, JITINT32 state);
static inline JITINT32 tm_readThreadState (CLRThread* threadInfo);

/* Current thread description */
static __thread CLRThread* currentThreadDescription = NULL;

/* Initialize thread manager */
void init_threadsManager (t_threadsManager *self) {

    /* Link functions */
    self->initialize = tm_initialize;
    self->destroy = tm_shutdownThreadManager;
    self->getCurrentThread = tm_getCurrentThread;
    self->waitThreads = tm_waitThreads;
    self->lockLocation = tm_lockLocation;
    self->unlockLocation = tm_unlockLocation;
    self->memoryBarrier = tm_memoryBarrier;
    self->sleep = tm_sleep;
    self->lockObject = tm_lockObject;
    self->unlockObject = tm_unlockObject;
    self->waitForPulse = tm_waitForPulse;
    self->signalPulse = tm_signalPulse;
    self->signalPulseAll = tm_signalPulseAll;
    self->initializeThread = tm_initializeThread;
    self->finalizeThread = tm_finalizeThread;
    self->startThread = tm_startThread;
    self->joinToThread = tm_joinToThread;
    self->getThreadState = tm_getThreadState;
    self->setPrivateData = tm_setPrivateData;
    self->getPrivateData = tm_getPrivateData;
    self->getStartDelegate = tm_getStartDelegate;
    self->createEvent = tm_createEvent;
    self->destroyEvent = tm_destroyEvent;
    self->signalEvent = tm_signalEvent;
    self->waitOnEvent = tm_waitOnEvent;
    self->waitAllEvents = tm_waitAllEvents;
    self->createLocalMutex = tm_createLocalMutex;
    self->lockLocalMutex = tm_lockLocalMutex;
    self->unlockLocalMutex = tm_unlockLocalMutex;
    self->constrain = tm_constrain;
    self->movethread = tm_movethread;
    self->getFreeCPUs = tm_getFreeCPUs;
    /* Initialize internal state */
    tm_initRecursiveMutexDescription();
    tm_initThreadDefaultAttributes();
    self->locationLocks = xanHashTable_new(DEFAULT_LOCATION_LOCKS_SIZE, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    self->unstartedThreads = xanList_new(allocFunction, freeFunction, NULL);
    self->foregroundThreads = xanList_new(allocFunction, freeFunction, NULL);

}

static void tm_shutdownThreadManager (t_threadsManager *self) {
    xanHashTable_destroyTable(self->locationLocks);
    xanList_destroyList(self->unstartedThreads);
    xanList_destroyList(self->foregroundThreads);

    return ;
}

/* Force metadata loading */
static void tm_initialize (void) {
    PLATFORM_pthread_once(&tm_metadataLock, tm_loadMetadata);
}

/* Get current thread */
static void* tm_getCurrentThread (void) {

    /* The current thread description */
    CLRThread* currentThread;

    /*
     * Look if the IL object representing current thread has just been
     * built. If no build a new one
     */
    currentThread = GET_CURRENT_THREAD_DESCRIPTION();
    if (currentThread == NULL) {
        currentThread = tm_buildCurrentThreadDescription();
    }

    return currentThread->object;

}

/* Wait all CIL threads */
static void tm_waitThreads (void) {
    /* The thread manager */
    t_threadsManager* threadManager;

    /* Set of currently active foreground threads */
    XanList* foregroundThreads;

    /* Set of currently unstarted threads */
    XanList* unstartedThreads;

    /* A CLRThread container */
    XanListItem* threadContainer;

    /* Description of current thread in the iteration */
    CLRThread* currentThreadInfos;

    /* Get active foreground threads set */
    threadManager = &(cliManager->CLR).threadsManager;
    foregroundThreads = threadManager->foregroundThreads;
    unstartedThreads = threadManager->unstartedThreads;

    /* Wait for all foreground threads */
    xanList_lock(foregroundThreads);
    while (xanList_length(foregroundThreads) != 0) {

        /* Extract a foreground thread */
        threadContainer = xanList_first(foregroundThreads);
        currentThreadInfos = threadContainer->data;
        xanList_unlock(foregroundThreads);

        /* Wait for current thread */
        tm_waitForegroundThread(currentThreadInfos);

        /* Relock the set, in order to get the next thread */
        xanList_lock(foregroundThreads);
    }
    xanList_unlock(foregroundThreads);

    /* Background threads will be killed here, before unstarted ones */

    /* Kill all unstarted threads */
    while (xanList_length(unstartedThreads) != 0) {

        /* Extract an unstarted thread */
        threadContainer = xanList_first(unstartedThreads);
        currentThreadInfos = threadContainer->data;

        /* Kill it */
        tm_killUnstartedThread(currentThreadInfos);
    }
    assert(xanList_length(unstartedThreads) == 0);

    /* Return			*/
    return;
}

/* Lock given location */
static void tm_lockLocation (void* location) {

    PLATFORM_lockMutex(tm_getLock(location));

}

/* Unlock given location */
static void tm_unlockLocation (void* location) {

    PLATFORM_unlockMutex(tm_getLock(location));

}

/* Commit all memory operations */
static void tm_memoryBarrier (void) {
    __sync_synchronize();
}

/* Sleep for some time */
static void tm_sleep (JITINT32 milliseconds) {

    /* Current thread description */
    CLRThread* selfThreadInfos;

    /* Result of an integer division */
    div_t result;

    /* The milliseconds arguments converted in a suitable format */
    struct timespec sleepTime;

    /* Milliseconds arguments discriminates between different behaviors */
    switch (milliseconds) {

            /* Infinite sleeping */
        case WAIT_UNTIMED:
            print_err("Infinite sleeping not yet implemented", 0);
            abort();
            break;

            /* Yield processor */
        case YIELD_PROCESSOR:
            PLATFORM_sched_yield();
            break;

            /* Timed wait */
        default:
            /* Convert milliseconds to (seconds, nanoseconds) pair */
            result = div(milliseconds, 1000);
            sleepTime.tv_sec = result.quot;
            sleepTime.tv_nsec = result.rem * 1000l * 1000l;

            /* Get current thread description */
            selfThreadInfos = tm_getPrivateData(tm_getCurrentThread());

            /* Update current thread state */
            tm_writeThreadState(selfThreadInfos,
                                THREAD_STATE_WAITSLEEPJOIN);

            /* Sleep */
            PLATFORM_nanosleep(&sleepTime, NULL);

            /* Now I'm running */
            tm_writeThreadState(selfThreadInfos, THREAD_STATE_RUNNING);
    }

}

/* Lock given object */
static JITBOOLEAN tm_lockObject (void* object, JITINT32 milliseconds) {
    void *threadObject;

    /* The garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Current thread internal representation */
    CLRThread* threadInfos;

    /* Whether lock has been acquired */
    JITBOOLEAN lockAcquired;

    /* Get the garbage collector */
    garbageCollector = cliManager->gc;

    /* Get current thread internal representation */
    threadObject = tm_getCurrentThread();
    assert(threadObject != NULL);
    threadInfos = tm_getPrivateData(threadObject);
    assert(threadInfos != NULL);

    /* We are logically blocked */
    tm_writeThreadState(threadInfos, THREAD_STATE_WAITSLEEPJOIN);

    /* Lock the object */
    lockAcquired = garbageCollector->lockObject(object, milliseconds);

    /* OK, return to run */
    tm_writeThreadState(threadInfos, THREAD_STATE_RUNNING);

    return lockAcquired;
}

/* Unlock given object */
static void tm_unlockObject (void* object) {
    /* The garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Get the garbage collector */
    garbageCollector = cliManager->gc;

    /* Unlock the object */
    garbageCollector->unlockObject(object);

}

/* Wait for pulse signal on object */
static JITBOOLEAN tm_waitForPulse (void* object, JITINT32 timeout) {

    /* The system garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Current thread description */
    CLRThread* selfThreadInfos;

    /* Set to JITTRUE if the pulse signal is detected before timeout */
    JITBOOLEAN pulseOccursBeforeTimeout;

    /* Get some pointers */
    garbageCollector = cliManager->gc;

    /* Update thread state */
    selfThreadInfos = tm_getPrivateData(tm_getCurrentThread());
    tm_writeThreadState(selfThreadInfos, THREAD_STATE_WAITSLEEPJOIN);

    /* Wait for pulse */
    pulseOccursBeforeTimeout = garbageCollector->waitForPulse(object,
                               timeout);
    /* Now I'm running */
    tm_writeThreadState(selfThreadInfos, THREAD_STATE_RUNNING);

    return pulseOccursBeforeTimeout;

}

/* Notify a pulse signal */
static void tm_signalPulse (void* object) {

    /* The system garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Get the garbage collector */
    garbageCollector = cliManager->gc;

    /* Let it notify the pulse signal */
    garbageCollector->signalPulse(object);
}

/* Notify a pulse all signal */
static void tm_signalPulseAll (void* object) {

    /* The system garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Get the garbage collector */
    garbageCollector = cliManager->gc;

    /* Let it notify the pulse all signal */
    garbageCollector->signalPulseAll(object);
}

/* Initialize the given thread */
static void tm_initializeThread (void* thread) {

    /* Infos about new thread */
    CLRThread* threadInfos;

    /* Build a new OS thread */
    threadInfos = (CLRThread*) allocMemory(sizeof(CLRThread));
    threadInfos->object = thread;
    threadInfos->joined = JITFALSE;
    tm_writeThreadState(threadInfos, THREAD_STATE_UNSTARTED);
    tm_initThreadStateMonitor(threadInfos);
    tm_createNativeThread(threadInfos);
    tm_setPrivateData(thread, threadInfos);
}

/* Finalize the given thread */
static void tm_finalizeThread (void* thread) {

    /* Given thread internal representation */
    CLRThread* threadInfo;

    threadInfo = tm_getPrivateData(thread);
    tm_destroyThreadStateMonitor(threadInfo);
    freeMemory(threadInfo);

}

/* Start given thread */
static void tm_startThread (void* thread) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Set of unstarted threads */
    XanList* unstartedThreads;

    /* Set of foreground threads */
    XanList* foregroundThreads;

    /* Info about the thread to launch */
    CLRThread* threadInfos;

    /* Get some pointers */
    threadManager = &(cliManager->CLR).threadsManager;
    unstartedThreads = threadManager->unstartedThreads;
    foregroundThreads = threadManager->foregroundThreads;

    /* Retrieve the thread internal representation */
    threadInfos = tm_getPrivateData(thread);
    assert(tm_readThreadState(threadInfos) == THREAD_STATE_UNSTARTED);
    assert(xanList_find(unstartedThreads, threadInfos) != NULL);

    /* Move to running set */
    xanList_lock(unstartedThreads);
    xanList_lock(foregroundThreads);
    xanList_removeElement(unstartedThreads, threadInfos, JITTRUE);
    xanList_insert(foregroundThreads, threadInfos);
    xanList_unlock(foregroundThreads);
    xanList_unlock(unstartedThreads);

    /* Signal that it can run */
    tm_changeThreadState(threadInfos, THREAD_STATE_RUNNING);

    /* Return		*/
    return;
}

/* Join to a thread */
static JITBOOLEAN tm_joinToThread (void* thread, JITINT32 timeout) {

    /* Thread internal representation */
    CLRThread* threadInfos;

    /* Current thread internal representation */
    CLRThread* selfThreadInfos;

    /* Store thread termination status */
    JITBOOLEAN terminated;

    /* Get thread internal representation */
    threadInfos = tm_getPrivateData(thread);

    /* Get current thread internal representation */
    selfThreadInfos = tm_getPrivateData(tm_getCurrentThread());

    /* Examine thread state */
    tm_lockThreadState(threadInfos);
    switch (timeout) {

            /* Wait for thread termination */
        case JOIN_UNTIMED:

            /* Update current thread state */
            tm_writeThreadState(selfThreadInfos, THREAD_STATE_WAITSLEEPJOIN);

            /* Wait until the joined thread exit */
            while (!THREAD_STOPPED(threadInfos)) {
                tm_waitThreadState(threadInfos);
            }

            /* OK, I'm running now and the joined thread is stopped */
            tm_writeThreadState(selfThreadInfos, THREAD_STATE_RUNNING);
            terminated = JITTRUE;
            break;

            /* Check if thread is terminated */
        case JOIN_NO_WAIT:
            print_err("Check for a thread termination isn't yet "
                      "implemented", 0);
            abort();
            break;

            /* Timed join */
        default:
            print_err("Thread time join isn't yet implemented", 0);
            abort();
    }

    /* Free native thread resources if thread is terminated */
    if (terminated) {
        tm_joinToNativeThread(threadInfos);
    }

    tm_unlockThreadState(threadInfos);

    return terminated;

}

/* Get given thread state */
static JITINT32 tm_getThreadState (void* thread) {

    /* Given thread internal representation */
    CLRThread* threadInfos;

    /* Given thread current state */
    JITINT32 state;

    /* Get the thread current state */
    threadInfos = tm_getPrivateData(thread);
    state = tm_readThreadState(threadInfos);

    /* Return			*/
    return state;
}

/* Store threadInfos inside given thread privateData field */
static void tm_setPrivateData (void* thread, CLRThread* threadInfos) {
    /* The thread manager */
    t_threadsManager* threadManager;

    /* Absolute address of the privateData field */
    CLRThread** field;

    /* Get the thread manager */
    threadManager = &(cliManager->CLR).threadsManager;

    /* Load metadata */
    PLATFORM_pthread_once(&tm_metadataLock, tm_loadMetadata);

    /* Save infos inside System.Threading.Thread object */
    field = (CLRThread**) (thread + threadManager->privateDataFieldOffset);
    *field = threadInfos;
}

/* Get given thread private data */
static CLRThread* tm_getPrivateData (void* thread) {

    /* The thread manager */
    t_threadsManager* threadManager;
    CLRThread *threadInfo;

    /* A pointer to privateData field inside thread */
    CLRThread** field;

    /* Get the thread manager */
    threadManager = &(cliManager->CLR).threadsManager;

    /* Load metadata */
    PLATFORM_pthread_once(&tm_metadataLock, tm_loadMetadata);

    /* Retrieve thread infos */
    field = (CLRThread**) (thread + threadManager->privateDataFieldOffset);
    threadInfo = NULL;
    if (field != NULL) {
        threadInfo = *field;
    }
    return threadInfo;
}

/* Set given thread start function to the given delegate */
static void tm_setStartDelegate (void* thread, void* startRoutine) {
    t_threadsManager* threadManager;
    void** field;

    threadManager = &(cliManager->CLR).threadsManager;

    PLATFORM_pthread_once(&tm_metadataLock, tm_loadMetadata);

    field = thread + threadManager->startFieldOffset;
    *field = startRoutine;
}

/* Get given thread start function as a delegate */
static void* tm_getStartDelegate (void* thread) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Pointer to start field into thread */
    void** field;

    /* Get the thread manager */
    threadManager = &(cliManager->CLR).threadsManager;

    /* Load metadata */
    PLATFORM_pthread_once(&tm_metadataLock, tm_loadMetadata);

    /* Retrieve the ThreadStart delegate */
    field = thread + threadManager->startFieldOffset;

    /* Return				*/
    return *field;
}

/* Create a new event */
static CLRWaitEvent* tm_createEvent (JITBOOLEAN manualReset, JITBOOLEAN initialState) {

    /* The new event */
    CLRWaitEvent* event;

    /* Build a new event and initialize its state */
    event = (CLRWaitEvent*) allocMemory(sizeof(CLRWaitEvent));
    event->type = CLRWAITEVENT;
    event->manualReset = manualReset;
    event->signalled = initialState;

    /* Also initialize the associated monitor */
    tm_initEventMonitor(event);

    return event;

}

/* Destroy given event */
static void tm_destroyEvent (CLRWaitEvent* event) {

    tm_destroyEventMonitor(event);
    freeMemory(event);

}

/* Signal an event */
static JITBOOLEAN tm_signalEvent (CLRWaitEvent* event) {

    /* Enter critical section */
    tm_lockEvent(event);

    /* OK, signal event */
    event->signalled = JITTRUE;

    /* Wake up one or all threads */
    if (AUTORESET_EVENT(event)) {
        tm_unicastSignalEvent(event);
    } else {
        tm_broadcastSignalEvent(event);
    }

    /* End of critical section */
    tm_unlockEvent(event);

    return JITTRUE;

}

/* Wait for an event */
static JITBOOLEAN tm_waitOnEvent (CLRWaitEvent* event, JITINT32 timeout) {

    /* Current thread description */
    CLRThread* selfThreadInfos;

    /* Event state */
    JITBOOLEAN signalled;

    /* Get current thread description */
    selfThreadInfos = tm_getPrivateData(tm_getCurrentThread());

    tm_lockEvent(event);

    /* What kind of wait we must do? */
    switch (timeout) {

            /* Classical untimed wait */
        case WAIT_UNTIMED:

            /* Update current thread state */
            tm_writeThreadState(selfThreadInfos,
                                THREAD_STATE_WAITSLEEPJOIN);

            /* Wait until the event is signalled */
            while (!EVENT_SIGNALLED(event)) {
                tm_waitEventSignal(event);
            }

            /* OK, now I'm running and I have been signalled */
            tm_writeThreadState(selfThreadInfos, THREAD_STATE_RUNNING);
            signalled = JITTRUE;
            break;

            /* Don't wait, just check event state */
        case WAIT_NO_WAIT:
            print_err("Event state check not yet implemented", 0);
            abort();
            break;

            /* Timed wait */
        default:
            print_err("Timed wait on events not yet implemented", 0);
            abort();
    }

    /* If in auto reset mode reset the signal */
    if (AUTORESET_EVENT(event)) {
        event->signalled = JITFALSE;
    }

    tm_unlockEvent(event);

    return signalled;

}

/* Wait for all events in handles array */
static JITBOOLEAN tm_waitAllEvents (void* handles, JITINT32 timeout, JITBOOLEAN exitContext) {
    /* The array manager */
    t_arrayManager* arrayManager;

    /* Current thread description */
    CLRThread* selfThreadInfos;

    /* Pointer to an event in the array */
    CLRWaitEvent** eventPointer;

    /* Current event in iteration */
    CLRWaitEvent* event;

    /* Whether all events has been signalled */
    JITBOOLEAN allSignalled;

    /* Loop counter */
    JITUINT32 i;

    /* Get the array manager */
    arrayManager = &((cliManager->CLR).arrayManager);

    /* Declare that I'm sleeping */
    selfThreadInfos = tm_getPrivateData(tm_getCurrentThread());
    tm_writeThreadState(selfThreadInfos, THREAD_STATE_WAITSLEEPJOIN);

    switch (timeout) {

            /* Classical untimed wait */
        case WAIT_UNTIMED:

            /* Wait on all events */
            for (i = 0; i < arrayManager->getArrayLength(handles, 0); i++) {

                /* Retrieve current event */
                eventPointer = arrayManager->getArrayElement(handles,
                               i);
                event = *eventPointer;

                tm_lockEvent(event);

                /* Wait for signalling on current event */
                while (!EVENT_SIGNALLED(event)) {
                    tm_waitEventSignal(event);
                }

                /* Reset event if needed */
                if (AUTORESET_EVENT(event)) {
                    event->signalled = JITFALSE;
                }

                tm_unlockEvent(event);
            }

            /* All events signalled */
            allSignalled = JITTRUE;
            break;

            /* Don't wait, just check events states */
        case WAIT_NO_WAIT:
            print_err("Events state check not yet implemented", 0);
            abort();
            break;

            /* Timed wait */
        default:
            print_err("Timed wait on events not yet implemented", 0);
            abort();
    }

    /* Waiting done, now I'm running */
    tm_writeThreadState(selfThreadInfos, THREAD_STATE_RUNNING);

    /* Return			*/
    return allSignalled;
}

/* Create an intraprocess mutex */
static CLRLocalMutex* tm_createLocalMutex (JITBOOLEAN initiallyOwned) {

    /* New local mutex */
    CLRLocalMutex* mutex;

    /* Build a fresh mutex */
    mutex = allocMemory(sizeof(CLRLocalMutex));
    mutex->type = CLRLOCALMUTEX;

    /* Remember mutex state */
    if (initiallyOwned) {
        mutex->owner = tm_getPrivateData(tm_getCurrentThread());
        mutex->recursionDeep = 1;
    } else {
        mutex->owner = NULL;
        mutex->recursionDeep = 0;
    }
    /* Init low level lock */
    tm_initLocalMutexLock(mutex, initiallyOwned);

    /* Return		*/
    return mutex;
}

/* Lock given mutex */
static void tm_lockLocalMutex (CLRLocalMutex* mutex) {

    /* Current thread informations */
    CLRThread* threadInfos;

    /* Get current thread description */
    threadInfos = tm_getPrivateData(tm_getCurrentThread());

    /* Acquire lock */
    tm_writeThreadState(threadInfos, THREAD_STATE_WAITSLEEPJOIN);
    tm_lockLocalMutexLock(mutex);

    /* Lock acquired */
    tm_writeThreadState(threadInfos, THREAD_STATE_RUNNING);
    mutex->owner = threadInfos;
    mutex->recursionDeep++;
}

/* Unlock given mutex */
static void tm_unlockLocalMutex (CLRLocalMutex* mutex) {

    /* Sanity check */
    if (mutex->owner != tm_getPrivateData(tm_getCurrentThread())) {
        print_err("Unlocking from not holding thread forbidden", 0);
        abort();
    }

    /* Release the mutex */
    if (--mutex->recursionDeep == 0) {
        mutex->owner = NULL;
    }
    tm_unlockLocalMutexLock(mutex);
}

/* Threads Merge  */
static void tm_constrain (CLRThread* threadInfo_src, CLRThread* threadInfo_dest) {

    /* Check whether the two logical        *
     * threads have already been mapped on	*
     * the same physical thread		*/
    if (memcmp(&(threadInfo_src->affinity), &(threadInfo_dest->affinity), sizeof(cpu_set_t)) == 0) {
        print_err("Cannot merge two logical threads already mapped to the same physical thread", 0);
        abort();
    }

    /* Change threadInfos_src's affinity    */
    if (PLATFORM_pthread_setaffinity_np(threadInfo_src->id, sizeof(cpu_set_t), &(threadInfo_dest->affinity)) != 0) {
        print_err("Unable to change source thread's affinity", 0);
        abort();
    }
    memcpy(&(threadInfo_src->affinity), &(threadInfo_dest->affinity), sizeof(cpu_set_t));

    /* Return				*/
    return;
}

static void tm_movethread (CLRThread* threadInfo_src, JITUINT32 core) {

    /* Assertions				*/
    assert(threadInfo_src != NULL);

    if (core >= PLATFORM_getProcessorsNumber()) {
        print_err("Core number not correct", 0);
        abort();
    }
    PLATFORM_CPU_ZERO(&(threadInfo_src->affinity));
    PLATFORM_CPU_SET(core, &(threadInfo_src->affinity));
    if (PLATFORM_pthread_setaffinity_np(threadInfo_src->id, sizeof(cpu_set_t), &(threadInfo_src->affinity)) != 0) {
        print_err("Unable to change source thread's affinity", 0);
        abort();
    }
}

static XanList* tm_getFreeCPUs (void) {
    XanList *l;
    JITUINT32 i;
    JITUINT32 j;
    XanList *foregroundThreads;
    t_threadsManager *threadManager;
    XanListItem *threadContainer;
    CLRThread *currentThreadInfos;
    JITUINT32 *cpulist;
    JITUINT32 ncores;

    /* Fetch the number of cores		*/
    ncores = PLATFORM_getProcessorsNumber();

    /* Allocate the necessary memory	*/
    cpulist = (JITUINT32 *) allocFunction(ncores*sizeof(JITUINT32));
    l = xanList_new(allocFunction, freeFunction, NULL);

    /* Cache some pointers			*/
    threadManager = &((cliManager->CLR).threadsManager);
    foregroundThreads = threadManager->foregroundThreads;

    /* Get the list of free processors	*/
    threadContainer = xanList_first(foregroundThreads);
    for (i = 0; i<xanList_length(foregroundThreads); i++) {
        currentThreadInfos = threadContainer->data;
        for (j = 0; j < ncores; j++) {
            if (PLATFORM_CPU_ISSET(j,&(currentThreadInfos->affinity))) {
                cpulist[j] = JITTRUE;
                break;
            }
        }
        threadContainer = threadContainer->next;
    }
    for (i = 0; i<ncores; i++) {
        if (cpulist[i] == JITFALSE) {
            xanList_append(l, (void*) (JITNUINT) (i+1));
        }
    }

    /* Free the memory		*/
    freeFunction(cpulist);

    /* Return			*/
    return l;
}

/* Build the current thread description */
static CLRThread* tm_buildCurrentThreadDescription (void) {

    /* The thread manager */
    t_threadsManager* self;

    /* The garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Description of System.Threading.Thread */
    TypeDescriptor* threadType;

    /* An instance of System.Threading.Thread */
    void* thread;

    /* Thread internal representation */
    CLRThread* threadInfos;

    /*
     * Used to check errors after the initialization of a thread
     */
    JITBOOLEAN initializationDone;

    /* Cache some pointers */
    self = &(cliManager->CLR).threadsManager;
    garbageCollector = cliManager->gc;

    /* Load metadata */
    PLATFORM_pthread_once(&tm_metadataLock, tm_loadMetadata);

    /* Call the garbage collector to request a new memory segment */
    threadType = self->threadType;
    assert(threadType != NULL);
    thread = garbageCollector->allocPermanentObject(threadType, 0);

    /* Build thread internal representation */
    threadInfos = (CLRThread*) allocMemory(sizeof(CLRThread));
    threadInfos->id = PLATFORM_getSelfThreadID();
    threadInfos->object = thread;
    threadInfos->joined = JITFALSE;
    tm_writeThreadState(threadInfos, THREAD_STATE_RUNNING);
    tm_initThreadStateMonitor(threadInfos);

    /* Initialize the Thread object */
    if (MONO_BCL_IS_LOADED(self)) {
        initializationDone = tm_initialize_mono_thread(thread, threadInfos);
    } else {
        initializationDone = tm_initialize_pnet_thread(thread, threadInfos);
    }

    /* All OK, add the thread to the cache */
    if (initializationDone) {
        SET_CURRENT_THREAD_DESCRIPTION(threadInfos);

        /* Something bad happens: cleanup and report to caller */
    } else {
        freeMemory(threadInfos);
        print_err("Failed initializing a thread", 0);
        abort();
    }

    /* Return				*/
    return threadInfos;
}

/* Initialize a System.Threading.Thread from the MONO BCL */
JITBOOLEAN tm_initialize_mono_thread (void* thread, CLRThread* threadInfos) {
    tm_setPrivateData(thread, threadInfos);
    tm_setStartDelegate(thread, NULL);
    return JITTRUE;
}

/* Initialize a System.Threading.Thread from the PNET BCL */
JITBOOLEAN tm_initialize_pnet_thread (void* thread, CLRThread* threadInfos) {
    t_threadsManager* self;
    Method ctor;
    ir_method_t *irCtor;
    void* args[2];
    JITBOOLEAN noException;

    /* Get some pointers.
    */
    self = &((cliManager->CLR).threadsManager);

    /* Fetch the method to run.
     */
    ctor = self->privateCtor;
    assert(ctor != NULL);
    irCtor = ctor->getIRMethod(ctor);
    assert(irCtor != NULL);
    assert(IRMETHOD_getMethodMetadata(irCtor, METHOD_METADATA) == ctor);

    /* Compile thread constructor, if not yet done.
    */
    IRMETHOD_translateMethodToMachineCode(irCtor);
    IRMETHOD_tagMethodAsProfiled(irCtor);

    /* Invoke thread constructor.
    */
    args[0] = &thread;
    args[1] = &threadInfos;
    noException = IRVM_run(cliManager->IRVM, *(ctor->jit_function), args, NULL);

    /* Print the stack trace in case of exception */
    if (!noException) {
        //print_stack_trace(ildjitSystem); FIXME
    }

    return noException;
}

/* Wait that the given foreground thread exit */
void tm_waitForegroundThread (CLRThread* threadInfos) {

    /* Current thread infos */
    CLRThread* selfThreadInfos;

    /* Get current thread */
    selfThreadInfos = tm_getPrivateData(tm_getCurrentThread());

    /* I'm waiting */
    tm_writeThreadState(selfThreadInfos, THREAD_STATE_WAITSLEEPJOIN);

    /* Wait for thread exit */
    tm_lockThreadState(threadInfos);
    while (!THREAD_STOPPED(threadInfos)) {
        tm_waitThreadState(threadInfos);
    }
    tm_joinToNativeThread(threadInfos);
    tm_unlockThreadState(threadInfos);

    /* Now, I'm running */
    tm_writeThreadState(selfThreadInfos, THREAD_STATE_RUNNING);
}

/* Abort given unstarted thread */
void tm_killUnstartedThread (CLRThread* threadInfos) {

    /* Send abort signal */
    tm_changeThreadState(threadInfos, THREAD_STATE_ABORTREQUESTED);

    /* Wait for thread termination */
    tm_lockThreadState(threadInfos);
    while (!THREAD_ABORTED(threadInfos)) {
        tm_waitThreadState(threadInfos);
    }
    tm_joinToNativeThread(threadInfos);
    tm_unlockThreadState(threadInfos);
}

/* Build description of a recursive mutex */
static void tm_initRecursiveMutexDescription (void) {
    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get thread manager */
    threadManager = &(cliManager->CLR).threadsManager;

    /* Build recursive mutex description */
    PLATFORM_initMutexAttr(&threadManager->recursiveMutexDescription);
    PLATFORM_setMutexAttr_type(&threadManager->recursiveMutexDescription, PTHREAD_MUTEX_RECURSIVE_NP);
}

/* Initialize native thread default attributes */
static void tm_initThreadDefaultAttributes (void) {
    /* The thread manager */
    t_threadsManager* threadManager;

    /* Size of each thread stack */
    size_t roundedStackSize;

    /* Get the thread manager */
    threadManager = &(cliManager->CLR).threadsManager;

    /* Set default attributes */
    PLATFORM_initThreadAttr(&threadManager->threadDefaultAttributes);

    /* Set stack size */
#if _POSIX_THREAD_ATTR_STACKSIZE > 0
    roundedStackSize = DEFAULT_THREAD_STACK_SIZE +
                       DEFAULT_THREAD_STACK_SIZE % PLATFORM_getSystemPageSize();
    if (roundedStackSize < PTHREAD_STACK_MIN) {
        roundedStackSize = PTHREAD_STACK_MIN;
    }
    PLATFORM_setThreadAttr_stackSize(&threadManager->threadDefaultAttributes,
                                     roundedStackSize);
#endif

}

/* Initialize the thread state monitor */
static void tm_initThreadStateMonitor (CLRThread* threadInfos) {

    pthread_mutexattr_t mutex_attr;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&threadInfos->stateLock, &mutex_attr);
    PLATFORM_initCondVar(&threadInfos->stateCondition, NULL);

}

/* Destroy thread state monitor */
static void tm_destroyThreadStateMonitor (CLRThread* threadInfo) {

    PLATFORM_destroyCondVar(&threadInfo->stateCondition);
    PLATFORM_destroyMutex(&threadInfo->stateLock);

}

/* Lock the thread state */
static void tm_lockThreadState (CLRThread* threadInfos) {

    while (PLATFORM_trylockMutex(&threadInfos->stateLock) == EBUSY) {
        ;
    }

}

/* Unlock the thread state */
static void tm_unlockThreadState (CLRThread* threadInfos) {
    PLATFORM_unlockMutex(&threadInfos->stateLock);
}

/* Broadcast signal for thread state change */
static void tm_broadcastSignalThreadState (CLRThread* threadInfos) {
    PLATFORM_broadcastCondVar(&threadInfos->stateCondition);

}

/* Wait for thread state changes */
static void tm_waitThreadState (CLRThread* threadInfos) {
    PLATFORM_waitCondVar(&threadInfos->stateCondition, &threadInfos->stateLock);
}

/* Initialize an event monitor */
static void tm_initEventMonitor (CLRWaitEvent* event) {

    pthread_mutexattr_t mutex_attr;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&event->lock, &mutex_attr);
    PLATFORM_initCondVar(&event->signalCondition, NULL);

}

/* Destroy an event monitor */
static void tm_destroyEventMonitor (CLRWaitEvent* event) {

    PLATFORM_destroyCondVar(&event->signalCondition);
    PLATFORM_destroyMutex(&event->lock);

}

/* Lock given event */
static void tm_lockEvent (CLRWaitEvent* event) {

    PLATFORM_lockMutex(&event->lock);

}

/* Unlock given event */
static void tm_unlockEvent (CLRWaitEvent* event) {

    PLATFORM_unlockMutex(&event->lock);

}

/* Wake up a thread blocked on an event */
static void tm_unicastSignalEvent (CLRWaitEvent* event) {

    PLATFORM_signalCondVar(&event->signalCondition);

}

/* Wake up all threads blocked on an event */
static void tm_broadcastSignalEvent (CLRWaitEvent* event) {

    PLATFORM_broadcastCondVar(&event->signalCondition);

}

/* Wait for event change */
static void tm_waitEventSignal (CLRWaitEvent* event) {

    PLATFORM_waitCondVar(&event->signalCondition, &event->lock);

}

/* Initialize given mutex low level lock */
static void tm_initLocalMutexLock (CLRLocalMutex* mutex, JITBOOLEAN locked) {
    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get thread manger */
    threadManager = &(cliManager->CLR).threadsManager;

    /* Initialize low level lock */
    PLATFORM_initMutex(&mutex->lock, &threadManager->recursiveMutexDescription);

    /* Lock it if needed */
    if (locked) {
        PLATFORM_lockMutex(&mutex->lock);
    }

}

/* Lock given local mutex low level lock */
static void tm_lockLocalMutexLock (CLRLocalMutex* mutex) {

    PLATFORM_lockMutex(&mutex->lock);

}

/* Unlock given local mutex low level lock */
static void tm_unlockLocalMutexLock (CLRLocalMutex* mutex) {

    PLATFORM_unlockMutex(&mutex->lock);

}

/* Get the lock associated with the given memory location */
static pthread_mutex_t* tm_getLock (void* location) {
    /* Contains locks for memory locations */
    XanHashTable* locationLocks;

    /* The looked lock */
    pthread_mutex_t* lock;

    /* Get the locks container */
    locationLocks = (cliManager->CLR).threadsManager.locationLocks;

    /* Search for the lock; if not found create a new one */
    xanHashTable_lock(locationLocks);
    lock = xanHashTable_lookup(locationLocks, location);
    if (lock == NULL) {
        lock = (pthread_mutex_t*) allocMemory(sizeof(pthread_mutex_t));
        pthread_mutexattr_t mutex_attr;
        PLATFORM_initMutexAttr(&mutex_attr);
        PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        PLATFORM_initMutex(lock, &mutex_attr);
        xanHashTable_insert(locationLocks, location, lock);
    }
    xanHashTable_unlock(locationLocks);

    return lock;
}

/* Load System.Threading.Thread metadata from assemblies */
static void tm_loadMetadata (void) {

    /* The thread manager */
    t_threadsManager* self;

    /* Collection of all known methods */
    t_methods* methods;

    /* Garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Name of the privateData field */
    JITINT8* privateDataFieldName;

    /* Name of the start field */
    JITINT8* startFieldName;

    /* Description of a System.Threading.Thread field */
    FieldDescriptor *fieldDescription;

    /* Offset of a System.Threding.Thread field */
    JITINT32 fieldOffset;

    /* System.Threading.Thread private constructor identifier */
    MethodDescriptor *ctorID;

    /* System.Threading.Thread private constructor */
    Method ctor;

    /* Get system description and thread manager */
    self = &((cliManager->CLR).threadsManager);

    /* Get some pointers */
    methods = &(cliManager->methods);
    garbageCollector = cliManager->gc;

    /* Find System.Threading.Thread in assemblies */
    self->threadType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System.Threading", (JITINT8 *) "Thread");

    if (self->threadType == NULL) {
        print_err("Unable to find System.Threading.Thread type", 0);
        abort();
    }

    /* Find System.Threading.Thread private constructor */
    ctorID = NULL;
    ctor = NULL;
    XanList *methods_list = self->threadType->getConstructors(self->threadType);
    XanListItem *item = xanList_first(methods_list);
    TypeDescriptor *intPtrType = cliManager->metadataManager->getTypeDescriptorFromElementType(cliManager->metadataManager, ELEMENT_TYPE_I);
    while (item != NULL) {
        ctorID = (MethodDescriptor *) item->data;
        ctor = methods->fetchOrCreateMethod(methods, ctorID, JITTRUE);
        /* Ignore multi parameters constructors (if any) */
        if (ctor->getParametersNumber(ctor) == 2) {
            /*
             * Get constructor first parameter. Its type identify
             * the right constructor
             */
            TypeDescriptor *paramType = ctor->getParameterILType(ctor, 1);
            if (paramType == intPtrType) {
                break;
            }
        }
        item = item->next;
    }
    self->privateCtor = ctor;
    assert(IRMETHOD_getMethodMetadata(&(self->privateCtor->IRMethod), METHOD_METADATA) == self->privateCtor);

    /* Tag the method as callable externally.
     */
    IRMETHOD_setMethodAsCallableExternally(&(self->privateCtor->IRMethod), JITTRUE);

    /* Look for privateData field offset */
    if (MONO_BCL_IS_LOADED(self)) {
        privateDataFieldName = MONO_PRIVATEDATAFIELDNAME;
        startFieldName = MONO_STARTFIELDNAME;
    } else {
        privateDataFieldName = PNET_PRIVATEDATAFIELDNAME;
        startFieldName = PNET_STARTFIELDNAME;
    }
    fieldDescription = self->threadType->getFieldFromName(self->threadType, privateDataFieldName);
    fieldOffset = garbageCollector->fetchFieldOffset(fieldDescription);
    self->privateDataFieldOffset = fieldOffset;

    /* Look for start field offset */
    fieldDescription = self->threadType->getFieldFromName(self->threadType, startFieldName);
    fieldOffset = garbageCollector->fetchFieldOffset(fieldDescription);
    self->startFieldOffset = fieldOffset;

    /* Look for start_obj field offset */
    fieldOffset = -1;
    fieldDescription = self->threadType->getFieldFromName(self->threadType, (JITINT8*) "start_obj");
    if (fieldDescription != NULL) {
        fieldOffset = garbageCollector->fetchFieldOffset(fieldDescription);
    }
    self->start_objFieldOffset = fieldOffset;

    /* Propagate the values		*/
    tm_memoryBarrier();

    /* Return			*/
    return;
}

/* Create a native thread */
static void tm_createNativeThread (CLRThread* threadInfos) {
    static JITUINT32 aff = 0;

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* Thread default attributes */
    pthread_attr_t* threadDefaultAttributes;

    /* Set of all unstarted threads */
    XanList* unstartedThreads;

    /* Thread creation status */
    JITINT32 threadStatus;

    /* Get some pointers */
    threadManager = &(cliManager->CLR).threadsManager;
    garbageCollector = cliManager->gc;
    threadDefaultAttributes = &threadManager->threadDefaultAttributes;
    unstartedThreads = threadManager->unstartedThreads;

    /* Get a new thread from it */
    threadStatus = garbageCollector->threadCreate(&threadInfos->id, threadDefaultAttributes, TM_NATIVETHREADRUN, threadInfos);

    /*  Set just created thread's affinity  */
    PLATFORM_CPU_ZERO(&(threadInfos->affinity));
    PLATFORM_CPU_SET(aff % PLATFORM_getProcessorsNumber(), &(threadInfos->affinity));
    PLATFORM_pthread_setaffinity_np(threadInfos->id, sizeof(cpu_set_t), &(threadInfos->affinity));
    aff++;

    if (threadStatus != 0) {
        print_err("Unable to create a thread. From standard an exception must be later raised on System.Threading.Thread.Start call", 0);
        abort();
    }

    /* Remember it was created */
    xanList_syncInsert(unstartedThreads, threadInfos);

    /* Return			*/
    return;
}

/* Join to a native thread, in order to free resources, if needed */
static void tm_joinToNativeThread (CLRThread* threadInfos) {
    /* The thread manager */
    t_threadsManager* threadManager;

    /* Set of unstarted threads */
    XanList* unstartedThreads;

    /* Currently active foreground threads */
    XanList* foregroundThreads;

    /* Get thread sets*/
    threadManager = &(cliManager->CLR).threadsManager;
    unstartedThreads = threadManager->unstartedThreads;
    foregroundThreads = threadManager->foregroundThreads;

    /* Free native OS resources, if needed */
    if (!threadInfos->joined) {
        PLATFORM_joinThread(threadInfos->id, NULL);
        threadInfos->joined = JITTRUE;
        if (THREAD_UNSTARTED(threadInfos)) {
            xanList_syncRemoveElement(unstartedThreads, threadInfos);
        } else {
            xanList_syncRemoveElement(foregroundThreads, threadInfos);
        }
    }
}

/* Thread main routine */
static void* tm_nativeThreadRun (CLRThread* threadInfos) {
    /* The delegates manager */
    t_delegatesManager* delegatesManager;

    /* The garbage collector */
    t_running_garbage_collector* garbageCollector;

    /* The thread entry point as a delegate */
    void* threadStart;

    /* Thread entry point */
    Method entryPoint;
    ir_method_t *irEntryPoint;

    /* Delegate argument */
    void* args[2];

    /* Whether thread must be aborted before start */
    JITBOOLEAN abortRequested;

    /* Get some pointers */
    delegatesManager = &((cliManager->CLR).delegatesManager);
    garbageCollector = cliManager->gc;

    /* Save thread description */
    SET_CURRENT_THREAD_DESCRIPTION(threadInfos);

    /* Get the delegate to invoke */
    threadStart = tm_getStartDelegate(threadInfos->object);
    entryPoint = delegatesManager->getDynamicInvoke(delegatesManager);
    irEntryPoint = entryPoint->getIRMethod(entryPoint);

    /* Make sure the method is compiled */
    IRMETHOD_translateMethodToMachineCode(irEntryPoint);

    /* Append the method to the partial AOT	*/
    IRMETHOD_tagMethodAsProfiled(irEntryPoint);

    /* Wait for launch */
    abortRequested = tm_waitForLaunch(threadInfos);

    /* OK, normal run */
    if (!abortRequested) {
        void *ar;
        void* start_objPtr;
        TypeDescriptor *objectType;
        JITINT32 error;

        /* Fetch the oject type				*/
        objectType = cliManager->metadataManager->getTypeDescriptorFromIRType(cliManager->metadataManager, IROBJECT);
        assert(objectType != NULL);

        /* Check if the BCL provides parametrized	*
         * start functions for threads                  */
        ar = NULL;
        if ((cliManager->CLR).threadsManager.start_objFieldOffset != -1) {

            /* Fetch the parameter				*/
            start_objPtr = (threadInfos->object) + ((cliManager->CLR).threadsManager.start_objFieldOffset);

            /* Check if there is a parameter to use		*/
            if ((*((void **) start_objPtr)) != 0) {

                /* Allocate the array of parameters             */
                ar = ((cliManager->CLR).arrayManager).newInstanceFromType(objectType, 1);

                /* Store the single parameter inside the array  *
                 * of parameters				*/
                ((cliManager->CLR).arrayManager).setValueArrayElement(ar, 0, start_objPtr);
            }
        }
        if (ar == NULL) {
            ar = ((cliManager->CLR).arrayManager).newInstanceFromType(objectType, 0);
        }
        assert(ar != NULL);

        /* Run the thread */
        args[0] = &threadStart;
        args[1] = &ar;
        error = IRVM_run(cliManager->IRVM, *(entryPoint->jit_function), args, NULL);

        /* Check the execution	*/
        if (error == 0) {
            printf("LIBCLIMANAGER: lib_thread.c: An exception has been thrown\n");
        }

        /* Free the memory */
        cliManager->gc->freeObject(ar);

        /* Change state */
        tm_changeThreadState(threadInfos, THREAD_STATE_STOPPED);

    } else {

        /* Change state */
        tm_changeThreadState(threadInfos, THREAD_STATE_ABORTED);
    }

    /* Remove root set */
    garbageCollector->threadExit(threadInfos->id);

    /* Return		*/
    return NULL;
}

/* Wait for thread launching */
static JITBOOLEAN tm_waitForLaunch (CLRThread* threadInfo) {

    /* Wait until some one tell me to run or abort */
    tm_lockThreadState(threadInfo);
    while (THREAD_UNSTARTED(threadInfo) &&
            !THREAD_ABORTREQUESTED(threadInfo)) {
        tm_waitThreadState(threadInfo);
    }
    tm_unlockThreadState(threadInfo);

    return THREAD_ABORTREQUESTED(threadInfo);

}

/* Change thread state in a thread safe way */
static void tm_changeThreadState (CLRThread* threadInfos, JITINT32 state) {

    tm_lockThreadState(threadInfos);
    tm_writeThreadState(threadInfos, state);
    tm_broadcastSignalThreadState(threadInfos);
    tm_unlockThreadState(threadInfos);

}

/* Set thread state in a thread unsafe way */
static void inline tm_writeThreadState (CLRThread* threadInfo, JITINT32 state) {

    switch (state) {

            /* Thread just created, initialize the state */
        case THREAD_STATE_UNSTARTED:
            threadInfo->state = THREAD_STATE_UNSTARTED;
            __sync_synchronize();
            break;

            /* Thread launched or just created, initialize the state */
        case THREAD_STATE_RUNNING:
            threadInfo->state = THREAD_STATE_RUNNING;
            __sync_synchronize();
            break;

            /* Thread terminated */
        case THREAD_STATE_STOPPED:
            threadInfo->state = THREAD_STATE_STOPPED;
            __sync_synchronize();
            break;

            /* Thread blocked on something */
        case THREAD_STATE_WAITSLEEPJOIN:
            threadInfo->state = THREAD_STATE_WAITSLEEPJOIN;
            __sync_synchronize();
            break;

            /* Abort requested */
        case THREAD_STATE_ABORTREQUESTED:
#ifdef __i386__
            __sync_or_and_fetch(&threadInfo->state, THREAD_STATE_ABORTREQUESTED);
#endif
            break;

            /* Thread aborted */
        case THREAD_STATE_ABORTED:
#ifdef __i386__
            __sync_or_and_fetch(&threadInfo->state, THREAD_STATE_ABORTED);
#endif
            break;

            /* Default: print an error */
        default:
            print_err("Unknown thread state attribute", 0);
            abort();
    }
}

/* Read current thread state */
static inline JITINT32 tm_readThreadState (CLRThread* threadInfo) {

    /* Given thread current state */
    JITINT32 state;

    /* Perform the read */
    __sync_synchronize();
    state = threadInfo->state;

    return state;
}
