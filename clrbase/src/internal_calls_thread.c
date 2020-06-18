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
#include <jitsystem.h>

// My headers
#include <internal_calls_utilities.h>
#include <internal_calls_thread.h>
// End

extern t_system *ildjitSystem;

/* Get this thread state */
JITINT32 System_Threading_Thread_InternalGetState (void* self) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* This thread state */
    JITINT32 state;

    /* Get some pointers */
    threadManager = &((ildjitSystem->cliManager).CLR.threadsManager);

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.InternalGetState");

    /* Get current thread state */
    state = threadManager->getThreadState(self);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Thread.InternalGetState");
    return state;
}

/* Join to the self thread */
JITBOOLEAN System_Threading_Thread_InternalJoin (void* self, JITINT32 timeout) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Set to JITTRUE iff the self thread is terminated */
    JITBOOLEAN terminated;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.InternalJoin");

    /* Wait for the specified timeout */
    terminated = threadManager->joinToThread(self, timeout);

    METHOD_END(ildjitSystem, "System.Threading.Thread.InternalJoin");
    return terminated;
}

/* Launch the given thread */
void System_Threading_Thread_Start (void* self) {

    /* Thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.Start");

    /* Launch the thread */
    threadManager->startThread(self);

    /* here starts the test*/
    /*	static int count=1;
            if(count==2){
                    XanList *foregroundThreads = (ildjitSystem->cliManager).CLR.threadsManager.foregroundThreads;
                    assert(foregroundThreads->length(foregroundThreads) == 2);
                    XanListItem *item = foregroundThreads->first(foregroundThreads);
                    CLRThread *firstThread = item->data;
                    item = item->next;
                    CLRThread *secondThread = item->data;
                    (ildjitSystem->cliManager).CLR.threadsManager.constrain(firstThread, secondThread);
            }
            count++;*/
    /*		if(count==8){
                                    char ch;
                                    int k;
                                    int a=0;
                                    int b=0;
                                    XanList *l;
                                    do{
                                    fprintf(stderr,"inserire comando\n");
                                    ch=(char)getchar();
                                    k=8;
                                    switch (ch){
                                            case 'm':
                                                    foregroundThreads = threadManager->foregroundThreads;
                                                    threadContainer = foregroundThreads->first(foregroundThreads);
                                                fprintf(stderr,"inserire il core da spostare\n");
                                                scanf("%i",&a);
                                                fprintf(stderr,"inserire il core di destinazione\n");
                                                scanf("%i",&b);
                                                do{
                                                            currentThreadInfos = threadContainer->data;
                                                            if(CPU_ISSET(b,&currentThreadInfos->affinity)!=0){
                                                                            lastThreadInfos=currentThreadInfos;
                                                                            fprintf(stderr,"settato\n");
                                                                    }
                                                            else{
                                                                    threadContainer=threadContainer->next;
                                                            }
                                                }while(CPU_ISSET(b,&currentThreadInfos->affinity)==0);
                                                    threadContainer = foregroundThreads->first(foregroundThreads);
                                                    while (k!=0) {
                                                            currentThreadInfos = threadContainer->data;
                                                            if(CPU_ISSET(a,&currentThreadInfos->affinity)!=0){
                                                                    fprintf(stderr,"trovato\n");
                                                                    tm_constrain(currentThreadInfos,lastThreadInfos);
                                                            }
                                                            threadContainer=threadContainer->next;
                                                            k--;
                                                    }
                                                    break;
                                            case 'a':
                                                    foregroundThreads = threadManager->foregroundThreads;
                                                fprintf(stderr,"inserire il core da spostare\n");
                                                scanf("%i",&a);
                                                fprintf(stderr,"inserire il core di destinazione\n");
                                                scanf("%i",&b);
                                                    threadContainer = foregroundThreads->first(foregroundThreads);
                                                    while (k!=0) {
                                                            currentThreadInfos = threadContainer->data;
                                                            if(CPU_ISSET(a,&currentThreadInfos->affinity)!=0){
                                                                    fprintf(stderr,"trovato\n");
                                                                    tm_movethread(currentThreadInfos,b);
                                                            }
                                                            threadContainer=threadContainer->next;
                                                            k--;
                                                    }
                                                    break;
                                            case 'r':
                                                    l=tm_getFreeCPUs();
                                                    cpu=l->first(l);
                                                    for(k=0;k<l->length(l);k++){
                                                            fprintf(stderr,"La CPU %i è libera\n",(cpu->data-1));
                                                            cpu=cpu->next;
                                                            }
                                                    l->destroyList(l);
                                                    break;
                                            case 'q':
                                                    fprintf(stderr,"exiting\n");
                                                    break;
                                            default:
                                                    break;
                                            return 0;
                                            }
                                    }while(ch!='q');
                            }*/
    /* here it finish*/

    /* Return		*/
    METHOD_END(ildjitSystem, "System.Threading.Thread.Start");
    return;
}

/* Free thread compiler structures */
void System_Threading_Thread_FinalizeThread (void* self) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.FinalizeThread");

    threadManager->finalizeThread(self);

    METHOD_END(ildjitSystem, "System.Threading.Thread.FinalizeThread");

}

/* Initialize a new thread */
void  System_Threading_Thread_InitializeThread (void* self) {

    /* The thread manager */
    t_threadsManager* threadManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.InitializeThread");

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Initialize the thread	*/
    threadManager->initializeThread(self);

    /* Return		*/
    METHOD_END(ildjitSystem, "System.Threading.Thread.InitializeThread");
    return;
}

/* Get current thread description */
void* System_Threading_Thread_InternalCurrentThread () {

    /* Thread manager */
    t_threadsManager* threadManager;

    /* An IL object representing the current thread */
    void* currentThread;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.InternalCurrentThread");

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Retrieve current thread from the thread manager */
    currentThread = threadManager->getCurrentThread();

    /* Return		*/
    METHOD_END(ildjitSystem, "System.Threading.Thread.InternalCurrentThread");
    return currentThread;
}

/* Force all memory operation to commit */
void System_Threading_Thread_MemoryBarrier (void) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.MemoryBarrier ");

    threadManager->memoryBarrier();

    METHOD_END(ildjitSystem, "System.Threading.Thread.MemoryBarrier ");

}

/* Sleep for some time */
void System_Threading_Thread_InternalSleep (JITINT32 time) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.InternalSleep");

    threadManager->sleep(time);

    METHOD_END(ildjitSystem, "System.Threading.Thread.InternalSleep");

}

/* Volatile read of a System.Int32 */
JITINT32 System_Threading_Thread_VolatileRead_i (JITINT32* address) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The read value */
    JITINT32 readValue;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.VolatileRead");

    threadManager->memoryBarrier();
    readValue = *((volatile JITINT32*) address);

    METHOD_END(ildjitSystem, "System.Threading.Thread.VolatileRead");

    return readValue;

}

/* Volatile write of a System.Int32 */
void System_Threading_Thread_VolatileWrite_i (JITINT32* address, JITINT32 value) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.VolatileWrite");

    *((volatile JITINT32*) address) = value;
    threadManager->memoryBarrier();

    METHOD_END(ildjitSystem, "System.Threading.Thread.VolatielWrite");

}

/* Get the cached current culture */
void* System_Threading_Thread_GetCachedCurrentCulture (void* self) {
    t_cultureManager* cultureManager;
    void* cachedCurrentCulture;

    cultureManager = &((ildjitSystem->cliManager).CLR.cultureManager);

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.GetCachedCurrentCulture");
    cachedCurrentCulture = cultureManager->getInvariantCulture();
    METHOD_END(ildjitSystem, "System.Threading.Thread.GetCachedCurrentCulture");

    return cachedCurrentCulture;
}

/* Lock the given object */
void System_Threading_Monitor_Enter (void* object) {
    t_threadsManager* threadManager;

    /* Get some pointers */
    METHOD_BEGIN(ildjitSystem, "System.Threading.Monitor.Enter");
    threadManager = &((ildjitSystem->cliManager).CLR.threadsManager);

    threadManager->lockObject(object, WAIT_UNTIMED);

    METHOD_END(ildjitSystem, "System.Threading.Monitor.Enter");
    return ;
}

/* Unlock given object */
void System_Threading_Monitor_Exit (void* object) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &((ildjitSystem->cliManager).CLR.threadsManager);

    METHOD_BEGIN(ildjitSystem, "System.Threading.Monitor.Exit");

    threadManager->unlockObject(object);

    METHOD_END(ildjitSystem, "System.Threading.Monitor.Exit");
    return;
}

/* Try locking an object */
JITBOOLEAN System_Threading_Monitor_InternalTryEnter (void* object, JITINT32 timeout) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* JITTRUE if lock was acquired */
    JITBOOLEAN lockAcquired;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Monitor.TryEnter");

    lockAcquired = threadManager->lockObject(object, timeout);

    METHOD_END(ildjitSystem, "System.Threading.Monitor.TryEnter");

    /* Return			*/
    return lockAcquired;
}

/* Wait for a pulse signal on object */
JITBOOLEAN System_Threading_Monitor_InternalWait (void* object, JITINT32 timeout) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Indicates whether pulse signal has been detected before timeout */
    JITBOOLEAN pulseOccursBeforeTimeout;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Monitor.InternalWait");

    pulseOccursBeforeTimeout = threadManager->waitForPulse(object, timeout);

    METHOD_END(ildjitSystem, "System.Threading.Monitor.InternalWait");

    /* Return			*/
    return pulseOccursBeforeTimeout;
}

/* Send a pulse signal on object */
void System_Threading_Monitor_Pulse (void* object) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Monitor.Pulse");

    threadManager->signalPulse(object);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Monitor.Pulse");
    return;
}

/* Send a pulse all signal on object */
void System_Threading_Monitor_PulseAll (void* object) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Monitor.PulseAll");

    threadManager->signalPulseAll(object);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Monitor.PulseAll");
    return;
}

/* Build or get a mutex */
JITNINT System_Threading_Mutex_InternalCreateMutex (JITBOOLEAN initiallyOwned, void* name, JITINT8* gotOwnership) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The string manager */
    t_stringManager* stringManager;

    /* The new mutex */
    JITNINT mutex;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;
    stringManager = &((ildjitSystem->cliManager).CLR.stringManager);

    METHOD_BEGIN(ildjitSystem, "System.Threading.Mutex.InternalCreateMutex");

    /* Null name: local mutex */
    if (name == NULL) {
        mutex = (JITNINT) threadManager->createLocalMutex(
                    initiallyOwned);
        *gotOwnership = JITTRUE;

        /* Zero length name: local mutex */
    } else if (stringManager->getLength(name) == 0) {
        mutex = (JITNINT) threadManager->createLocalMutex(
                    initiallyOwned);
        *gotOwnership = JITTRUE;

        /* Interprocess mutex */
    } else {
        print_err("Interprocess mutexes not yet implemented", 0);
        abort();
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Mutex.InternalCreateMutex");
    return mutex;
}

/* Release a mutex */
void System_Threading_Mutex_InternalReleaseMutex (void* mutex) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Given mutex as a local one */
    CLRLocalMutex* localMutex;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Mutex.InternalReleaseMutex");

    /* Which is given mutex type? */
    localMutex = (CLRLocalMutex*) mutex;
    switch (localMutex->type) {

            /* Local mutex */
        case CLRLOCALMUTEX:
            threadManager->unlockLocalMutex((CLRLocalMutex*) mutex);
            break;

            /* Unknown mutex */
        default:
            print_err("Unknown mutex type found", 0);
            abort();

    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Mutex.InternalReleaseMutex");
    return;
}

/* Atomic Compare And Swap */
JITINT32 System_Threading_Interlocked_CompareExchange (JITINT32* location, JITINT32 value, JITINT32 comparand) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The value stored at location before the eventually swap */
    JITINT32 oldValue;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Interlocked.CompareExchange");

    /* Do an atomic CAS operation on software */
    threadManager->lockLocation(location);
    oldValue = *location;
    if (oldValue == comparand) {
        *location = value;
    }
    threadManager->unlockLocation(location);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Interlocked.CompareExchange");
    return oldValue;
}

/* Atomic exchange */
JITINT32 System_Threading_Interlocked_Exchange (JITINT32* location, JITINT32 value) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Value stored in location before the exchange is done */
    JITINT32 oldValue;

    /* Get pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Interlocked.Exchange");

    /* Save old value and overwrite it with the new one */
    threadManager->lockLocation(location);
    oldValue = *location;
    *location = value;
    threadManager->unlockLocation(location);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Interlocked.Exchange");
    return oldValue;
}

/* Atomic increment */
JITINT32 System_Threading_Interlocked_Increment (JITINT32* location) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The value stored at location incremented by 1 */
    JITINT32 newValue;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Interlocked.Increment");

    /* Do an atomic increment in software */
    threadManager->lockLocation(location);
    newValue = ++(*location);
    threadManager->unlockLocation(location);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Interlocked.Increment");
    return newValue;
}

/* Atomic decrement */
JITINT32 System_Threading_Interlocked_Decrement (JITINT32* location) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The value stored at location incremented by 1 */
    JITINT32 newValue;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Interlocked.Decrement");

    /* Do an atomic decrement in software */
    threadManager->lockLocation(location);
    newValue = --(*location);
    threadManager->unlockLocation(location);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Interlocked.Decrement");
    return newValue;
}

/* Create a new event */
JITNINT System_Threading_WaitEvent_InternalCreateEvent (JITBOOLEAN manualReset, JITBOOLEAN initialState) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* The new event */
    JITNINT event;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.WaitEvent.InternalCreateEvent");

    /* Build a new event */
    event = (JITNINT) threadManager->createEvent(manualReset, initialState);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.WaitEvent.InternalCreateEvent");
    return event;
}

/* Set the event identified by handle to signalled */
JITBOOLEAN System_Threading_WaitEvent_InternalSetEvent (JITNINT handle) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Whether the event was signalled */
    JITBOOLEAN signalled;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.WaitEvent.InternalSetEvent");

    /* Signal event */
    signalled = threadManager->signalEvent((CLRWaitEvent*) handle);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.WaitEvent.InternalSetEvent");
    return signalled;
}

/* Destroy given handle */
void System_Threading_WaitHandle_InternalClose (JITNINT handle) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.WaitHandle.InternalClose");

    threadManager->destroyEvent((CLRWaitEvent*) handle);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.WaitHandle.InternalClose");
    return;
}

/* Wait for signals on the event identified by handle */
JITBOOLEAN System_Threading_WaitHandle_InternalWaitOne (JITNINT handle, JITINT32 timeout) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Typed handler */
    CLRWaitEvent* synchPrimitive;

    /* Identify whether the event is signalled */
    JITBOOLEAN signalled;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.WaitHandle.InternalWaitOne");

    /* Which kind of synch primitive handle is? */
    synchPrimitive = (CLRWaitEvent*) handle;
    switch (synchPrimitive->type) {

            /* Event: wait */
        case CLRWAITEVENT:
            signalled = threadManager->waitOnEvent((CLRWaitEvent*) handle,
                                                   timeout);
            break;

            /* Local mutex: lock */
        case CLRLOCALMUTEX:
            threadManager->lockLocalMutex((CLRLocalMutex*) handle);
            signalled = JITTRUE;
            break;

            /* Unknown synchronization primitive */
        default:
            print_err("Unknown synchronization primitive detected", 0);
            abort();
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.WaitHandle.InternalWaitOne");
    return signalled;
}

/* Wait for all events in handles */
JITBOOLEAN System_Threading_WaitHandle_InternalWaitAll (JITNINT* handles, JITINT32 timeout, JITBOOLEAN exitContext) {

    /* Thread manager */
    t_threadsManager* threadManager;

    /* Whether all events have been signalled */
    JITBOOLEAN allSignalled;

    /* Get some pointers */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    METHOD_BEGIN(ildjitSystem, "System.Threading.WaitHandle.InternalWaitAll");

    allSignalled = threadManager->waitAllEvents(handles, timeout, exitContext);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.WaitHandle.InternalWaitAll");
    return allSignalled;
}

JITBOOLEAN System_Threading_Thread_CanStartThreads (void) {
    JITBOOLEAN canStart;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.CanStartThreads()");

    canStart = JITFALSE;

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Threading.Thread.CanStartThreads()");
    return canStart;
}

void* System_Threading_Thread_CurrentThread_internal (void) {
    void *currentThread;

    METHOD_BEGIN(ildjitSystem, "System.Threading.Thread.CurrentThread_internal");

    currentThread = System_Threading_Thread_InternalCurrentThread();


    METHOD_END(ildjitSystem, "System.Threading.Thread.CurrentThread_internal");
    return currentThread;
}
