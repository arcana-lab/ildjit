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

#ifndef LIB_THREAD_H
#define LIB_THREAD_H

#include <jitsystem.h>
#include <ilmethod.h>
#include <platform_API.h>

/**
 * Name of the privataData field inside a System.Threading.Thread object
 */
#define PNET_PRIVATEDATAFIELDNAME (JITINT8 *) "privateData"

/**
 * Equivalent name of the field privateData inside the Mono BCL
 */
#define MONO_PRIVATEDATAFIELDNAME (JITINT8 *) "system_thread_handle"

/**
 * Name of the start field inside a System.Threading.Thread object
 */
#define PNET_STARTFIELDNAME (JITINT8 *) "start"

/**
 * Equivalent name of the field start inside the Mono BCL
 */
#define MONO_STARTFIELDNAME (JITINT8 *) "threadstart"

/**
 * Thread is running state
 */
#define THREAD_STATE_RUNNING 0x0

/**
 * Thread has been requested to stop
 */
#define THREAD_STATE_STOPREQUESTED 0x1

/**
 * Thread has been requested to suspend execution
 */
#define THREAD_STATE_SUSPENDREQUESTED 0x2

/**
 * Thread is on background
 */
#define THREAD_STATE_BACKGROUND 0x4

/**
 * Thread isn't yet started
 */
#define THREAD_STATE_UNSTARTED 0x8

/**
 * Thread main routine has finished execution
 */
#define THREAD_STATE_STOPPED 0x10

/**
 * Thread is sleeping waiting for another thread join
 */
#define THREAD_STATE_WAITSLEEPJOIN 0x20

/**
 * Thread is suspend
 */
#define THREAD_STATE_SUSPENDED 0x40

/**
 * Thread has been requested to abort
 */
#define THREAD_STATE_ABORTREQUESTED 0x80

/**
 * Thread is aborted
 */
#define THREAD_STATE_ABORTED 0x100

/**
 * Wait for a thread termination until its exit
 */
#define JOIN_UNTIMED -1

/**
 * Check thread termination without waiting for its exit
 */
#define JOIN_NO_WAIT 0

/**
 * Wait for an event until it occurs
 */
#define WAIT_UNTIMED -1

/**
 * Check event state without waiting for its exit
 */
#define WAIT_NO_WAIT 0

/**
 * Yield processor
 */
#define YIELD_PROCESSOR 0

/**
 * The synch primitive is a CLRWaitEvent
 */
#define CLRWAITEVENT 0

/**
 * The synch primitive is a CLRLocalMutex
 */
#define CLRLOCALMUTEX 1

/**
 * Thread internal representation
 *
 * @author Speziale Ettore
 */
typedef struct {

    /**
     * Thread identifier
     */
    pthread_t id;

    /**
     * The corresponding System.Threading.Thread object
     */
    void* object;

    /**
     * Identify current thread state
     */
    volatile JITINT32 state;

    /**
     * Identify whether the native thread linked to this one has just been
     * joined
     */
    JITBOOLEAN joined;

    /**
     * Used to solve race conditions on thread state change
     */
    pthread_mutex_t stateLock;

    /**
     * Condition variable used to access thread state
     */
    pthread_cond_t stateCondition;

    /**
     * Physical thread's affinity
     */
    cpu_set_t affinity;

} CLRThread;

/**
 * Internal representation of an event
 */
typedef struct {

    /**
     * Synchronization primitive type
     */
    JITBOOLEAN type;

    /**
     * Whether this event must be automatically reseted
     */
    JITBOOLEAN manualReset;

    /**
     * Event current state
     */
    JITBOOLEAN signalled;

    /**
     * Protect event access
     */
    pthread_mutex_t lock;

    /**
     * Used to suspend threads waiting for this event
     */
    pthread_cond_t signalCondition;
} CLRWaitEvent;

/**
 * Internal representation of a intraprocess mutex
 */
typedef struct {

    /**
     * Synchronization primitive type
     */
    JITBOOLEAN type;

    /**
     * Low level lock
     */
    pthread_mutex_t lock;

    /**
     * Mutex onwer (NULL if not assigned)
     */
    volatile CLRThread* owner;

    /**
     * Number of lock invocations by current lock owner thread
     */
    JITUINT32 recursionDeep;
} CLRLocalMutex;

/**
 * The thread manager
 *
 * This module defines functions related to thread managements, such as
 * functions for acquiring or releasing locks.
 *
 * @author Speziale Ettore
 *
 * @todo threads exceptions management
 */
typedef struct t_threadsManager {

    /**
     * Recursive mutex description
     */
    pthread_mutexattr_t recursiveMutexDescription;

    /**
     * Thread default attributes
     */
    pthread_attr_t threadDefaultAttributes;

    /**
     * The description of System.Threading.Thread
     */
    TypeDescriptor *threadType;

    /**
     * The description of System.Threading.Thread private ctor
     */
    Method privateCtor;

    /**
     * Contains locks for memory addresses
     */
    XanHashTable* locationLocks;

    /**
     * Currently unstarted threads
     */
    XanList* unstartedThreads;

    /**
     * Currently active foreground threads
     */
    XanList* foregroundThreads;

    /**
     * Offset of the privateData field inside a System.Threading.Thread
     * object
     */
    JITINT32 privateDataFieldOffset;

    /**
     * Offset of the start field inside a System.Threading.Thread object
     */
    JITINT32 startFieldOffset;

    /**
     * Offset of the start_obj field inside a System.Threading.Thread object
     */
    JITINT32 start_objFieldOffset;

    /**
     * Force System.Threading.Thread metadata loading
     */
    void (*initialize)(void);

    void (*destroy)(struct t_threadsManager *self);

    /**
     * Get a IL representation of current thread
     *
     * @return an instance of System.Threading.Thread representing current
     *         thread
     */
    void* (*getCurrentThread)(void);

    /**
     * Wait that all CIL threads terminates
     *
     * @todo kill background threads
     */
    void (*waitThreads)(void);

    /**
     * Lock the given location
     *
     * @param location the location to be locked
     */
    void (*lockLocation)(void* location);

    /**
     * Unlock the given location
     *
     * @param location the location to unlock
     */
    void (*unlockLocation)(void* location);

    /**
     * Force all memory operations to commit
     */
    void (*memoryBarrier)(void);

    /**
     * Put current thread into sleep state
     *
     * Sleep for some time. If milliseonds is -1 sleep forever, if 0 yield
     * execution.
     *
     * @param milliseconds milliseconds to sleep
     *
     * @todo infinite sleeping
     */
    void(*sleep) (JITINT32 time);

    /**
     * Lock the given object
     *
     * Try locking object. If milliseconds is -1 wait until lock
     * acquisition. If 0 try to get the lock without waiting.
     *
     * @param object the object to be locked
     * @param milliseconds milliseconds to wait for lock acquisition
     *
     * @return JITTRUE if lock has been aquired, JITFALSE otherwise
     */
    JITBOOLEAN (*lockObject)(void* object, JITINT32 milliseconds);

    /**
     * Unlock the given object
     *
     * @param object the object to unlock
     */
    void (*unlockObject)(void* object);

    /**
     * Wait for a pulse signal on given object for timeout milliseconds
     *
     * @param object the object where wait for the pulse
     * @param timeout milliseconds to wait for the pulse. If -1 wait until
     *                the pulse is detected, if don't wait
     *
     * @return JITTRUE if pulse was detected, JITFALSE otherwise
     *
     * @todo implement timed and no wait behavior. Raise
     * System.Threading.SynchronizationLockException if current thread
     * doesn't own the lock for object
     */
    JITBOOLEAN (*waitForPulse)(void* object, JITINT32 timeout);

    /**
     * Send a pulse signal to a thread waiting on object
     *
     * @param object used as a monitor
     *
     * @todo raise System.ArgumentNullException if object is null; raise
     *       System.Threading.SychronizationLockException if current thread
     *       doesn't own the object lock
     */
    void (*signalPulse)(void* object);

    /**
     * Sen a pulse signal to all threads waiting on object
     *
     * @param object used as a monitor
     *
     * @todo raise System.ArgumentNullException if object is null; raise
     *       System.Threading.SynchronizationLockException if current thread
     *       doesn't own the object lock
     */
    void (*signalPulseAll)(void* object);

    /**
     * Initialize the given thread
     *
     * @param thread the thread to be initialized
     */
    void (*initializeThread)(void* thread);

    /**
     * Finalize the given thread
     *
     * @param thread the thread to be finalized
     */
    void (*finalizeThread)(void* thread);

    /**
     * Start given thread
     *
     * @param thread the thread to be launched
     */
    void (*startThread)(void* thread);

    /**
     * Join to given thread
     *
     * Wait for thread termination for timeout milliseconds. If timeout is
     * -1 wait until thread termination. If timeout is 0 don't wait
     *
     * @param thread the thread to join
     * @param timeout milliseconds to wait
     *
     * @return JITTRUE if thread is terminated, JITFALSE otherwise
     *
     * @todo implement timed and no wait join; raise
     * System.Threading.ThreadStateException if thread is is in
     * System.Threading.ThreadState.Unstarted state
     */
    JITBOOLEAN (*joinToThread)(void* thread, JITINT32 timeout);

    /**
     * Get given thread state
     *
     * @param thread a thread in an unknown state
     *
     * @return thread state
     */
    JITINT32 (*getThreadState)(void* thread);

    /**
     * Store threadInfos inside thread privateData field
     *
     * @param thread an instance of System.Threading.Thread
     * @param threadInfos info about a thread
     */
    void (*setPrivateData)(void* thread, CLRThread* threadInfos);

    /**
     * Get given thread internal representation
     *
     * @param thread a System.Threading.Thread instance
     *
     * @return thread internal representation
     */
    CLRThread* (*getPrivateData)(void* thread);

    /**
     * Get the delegate representing thread start routine
     *
     * @param thread an instance of System.Threading.Thread
     *
     * @return thread start routine as a delegate
     */
    void* (*getStartDelegate)(void* thread);

    /**
     * Build a new event
     *
     * @param manualReset if JITTRUE the event will automatically reset
     *                    itself
     * @param intialState the event initial state signalled (JITTRUE) or not
     *                    (JITFALSE)
     *
     * @return the new event
     */
    CLRWaitEvent* (*createEvent)(JITBOOLEAN manualReset, JITBOOLEAN initialState);

    /**
     * Destroy given event
     *
     * @param event the event to destroy
     */
    void (*destroyEvent)(CLRWaitEvent* event);

    /**
     * Signal that an event has been occurred
     *
     * @param event the event to signal
     *
     * @return JITTRUE if the event was successfully signalled, JITFALSE
     *                 otherwise
     */
    JITBOOLEAN (*signalEvent)(CLRWaitEvent* event);

    /**
     * Wait for an event signal for specified timeout
     *
     * @param event the event to wait
     * @param timeout how much time to wait for the event; if WAIT_UNTIMED
     *        wait until the event is signalled; if WAIT_NO_WAIT don't wait
     *
     * @return JITTRUE if the event was signalled, JITFALSE otherwise
     *
     * @todo implements timed and no wait behavior
     */
    JITBOOLEAN (*waitOnEvent)(CLRWaitEvent* event, JITINT32 timeout);

    /**
     * Wait that all events on handles are signalled
     *
     * @param handles a CIL array of CLRWaitEvent to wait
     * @param timeout how much time to wait for the events; if WAIT_UNTIMED
     *                wait until the event is signalled; if WAIT_NO_WAIT
     *                don't wait, only check
     * @param exitContext whether exit from a synchronization context during
     *                    waiting
     *
     * @return JITTRUE if all the events were signalled, JITFALSE otherwise
     *
     * @todo implements timed and no wait behavior; implement
     *                  synchronization contexts
     */
    JITBOOLEAN (*waitAllEvents)(void* handles, JITINT32 timeout, JITBOOLEAN exitContext);

    /**
     * Create an interprocess mutex
     *
     * @param intiallyOwned JITTRUE if mutex initial state is acquired
     *
     * @return mutex internal description
     */
    CLRLocalMutex* (*createLocalMutex)(JITBOOLEAN initiallyOwned);

    /**
     * Lock given local mutex
     *
     * @param mutex a local mutex to lock
     */
    void (*lockLocalMutex)(CLRLocalMutex* mutex);

    /**
     * Unlock given local mutex
     *
     * @param mutex a local mutex to unlock
     */
    void (*unlockLocalMutex)(CLRLocalMutex* mutex);

    /**
     * Constrain function is used both to implement merge and split behavior, depending on which logical threads are passed as parameters
     *
     * @param threadInfo_src
     * @param threadInfo_dest
     */
    void (*constrain)(CLRThread* threadInfo_src, CLRThread* threadInfo_dest);

    /**
     * Move the thread to the given core
     *
     * @param threadInfo_src
     * @param core from 0 to number of cores-1
     */
    void (*movethread)(CLRThread* threadInfo_src, JITUINT32 core);

    /**
     * Return a list containing the free CPUs. The list's elements type is unsigned integer 32. Every element corresponds to the equivalent free CPU id + 1.
     * To use a cpu from the list it is necessary to decrease the given CPU id by 1
     * The list is allocated inside the function. The caller is in charge to destroy the list after the usage
     * @code
     * l->destroyList(l);
     * @endcode
     */
    XanList *(*getFreeCPUs)(void);
} t_threadsManager;

/**
 * Initialize thread manager
 */
void init_threadsManager (t_threadsManager *threadsManager);

#endif /* LIB_THREAD_H */
