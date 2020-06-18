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

#ifndef INTERNAL_CALLS_THREAD_H
#define INTERNAL_CALLS_THREAD_H

#include <jitsystem.h>

/**
 * Lock given object
 *
 * @param object the object to lock
 */
void System_Threading_Monitor_Enter (void* object);

/**
 * Unlock given object
 *
 * @param object the object to lock
 */
void System_Threading_Monitor_Exit (void* object);

/**
 * Try locking given object
 *
 * @param object the object to lock
 * @param timeout milliseconds to wait for lock acquisition
 *
 * @return JITTRUE if lock was acquired, JITFALSE otherwise
 */
JITBOOLEAN System_Threading_Monitor_InternalTryEnter (void* object, JITINT32 timeout);

/**
 * Wait for a pulse signal on given object for the specified timeout
 *
 * @param object the object where wait for pulse
 * @param timeout milliseconds to wait for pulse signal. If -1 wait until a
 *                pulse is detected, if 0 don't wait
 *
 * @return JITTRUE if pulse signal occurs before timeout, JITFALSE otherwise
 */
JITBOOLEAN System_Threading_Monitor_InternalWait (void* object, JITINT32 timeout);

/**
 * Awake a thread waiting on object
 *
 * @param object where send the pulse signal
 */
void System_Threading_Monitor_Pulse (void* object);

/**
 * Awake all threads waiting on object
 *
 * @param object where send the pulse all signal
 */
void System_Threading_Monitor_PulseAll (void* object);

/**
 * Build a new mutex
 *
 * @param initiallyOwner JITTRUE if mutex initial state is owned
 * @param name name of the mutex, if interprocess one
 * @param gotOwnership setted by routine to JITTRUE if mutex was created. Setted
 *                     to JITFALSE if mutex has just already been created in
 *                     another process
 *
 * @return mutex internal representation
 */
JITNINT System_Threading_Mutex_InternalCreateMutex (JITBOOLEAN initiallyOwned, void* name, JITINT8* gotOwnership);

/**
 * Release a mutex
 *
 * @param mutex mutex internal representation
 */
void System_Threading_Mutex_InternalReleaseMutex (void* mutex);

/**
 * Get current thread
 *
 * @return an instance of System.Threading.Thread representing current thread
 */
void * System_Threading_Thread_InternalCurrentThread (void);

/**
 * Initialize a new thread
 *
 * @param self the thread to be initialized
 */
void System_Threading_Thread_InitializeThread (void* self);

void System_Threading_Thread_FinalizeThread (void* self);

/**
 * Start this thread
 *
 * @param self the thread to be started
 *
 * @todo error checking
 */
void System_Threading_Thread_Start (void* self);

/**
 * Get thread state
 *
 * @param self this thread
 *
 * @return this thread state
 */
JITINT32 System_Threading_Thread_InternalGetState (void* self);

/**
 * Join to given thread
 *
 * Wait for self thread termination for timeout milleseconds. If timeout is -1
 * wait until termination. If timeout is 0 don't wait.
 *
 * @param self the thread to join
 * @param timeout milliseconds to wait
 *
 * @return JITTRUE if self is terminated, JITFALSE otherwise
 */
JITBOOLEAN System_Threading_Thread_InternalJoin (void* self, JITINT32 timeout);

/**
 * Force all memory operations to commit
 */
void System_Threading_Thread_MemoryBarrier (void);

/**
 * Sleep or yield current thread
 *
 * Put current thread into sleep state for time milliseconds. If time is -1
 * sleep forever, if 0 yield execution quantum.
 *
 * @param time how many milliseconds sleep
 */
void System_Threading_Thread_InternalSleep (JITINT32 time);

/**
 * Do a volatile read of a System.Int32 at given address
 *
 * @param address where to read
 *
 * @return read value
 */
JITINT32 System_Threading_Thread_VolatileRead_i (JITINT32* address);

/**
 * Do a volatile write of a System.Int32 at given address
 *
 * @param address where to write
 * @param value what to write
 */
void System_Threading_Thread_VolatileWrite_i (JITINT32* address, JITINT32 value);

/**
 * Get the cached current culture of the self thread
 *
 * This is a Mono internal call.
 *
 * @param self like this in C++
 *
 * @return the cached current culture of the self thread
 */
void* System_Threading_Thread_GetCachedCurrentCulture (void* self);

/**
 * Atomically exchange the value stored at location with value if it is equals
 * to comparand and then return it
 *
 * @param location the memory address to overwrite
 * @param value the value to be stored at location
 * @param comparand used to check whether to do the swap
 *
 * @return the value stored at location before the eventual exchange
 */
JITINT32 System_Threading_Interlocked_CompareExchange (JITINT32* location, JITINT32 value, JITINT32 comparand);

/**
 * Atomically exchange the value stored at location with value and return the
 * old one
 *
 * @param location the memory address to overwrite
 * @param value the value to be stored at location
 *
 * @return the value stored at location before the exchange
 */
JITINT32 System_Threading_Interlocked_Exchange (JITINT32* location, JITINT32 value);

/**
 * Atomically increment the value stored at location and return it
 *
 * @param location the memory address where there is the value to increment
 *
 * @return the incremented value
 */
JITINT32 System_Threading_Interlocked_Increment (JITINT32* location);

/**
 * Atomically decrement the value stored at location and return it
 *
 * @param location the memory address where there is the value to decrement
 *
 * @return the decremented value
 */
JITINT32 System_Threading_Interlocked_Decrement (JITINT32* location);

/**
 * Create a new event
 *
 * @param manualReset if false the event will automatically reset itself
 * @param initialState if JITTRUE the event start state is signalled
 *
 * @return the event internal description
 */
JITNINT System_Threading_WaitEvent_InternalCreateEvent (JITBOOLEAN manualReset, JITBOOLEAN intialState);

/**
 * Set the event identified by handle to signalled
 *
 * @param handle an event description
 *
 * @return JITTRUE if the event was signalled
 */
JITBOOLEAN System_Threading_WaitEvent_InternalSetEvent (JITNINT handle);

void System_Threading_WaitHandle_InternalClose (JITNINT handle);

/**
 * Wait for event activation
 *
 * Wait for a signal on the event described by handle. Wait for maximum timeout
 * milliseconds. If timeout is -1 wait until the event is signalled. If timeout
 * is 0 test and return immediately.
 *
 * @param handle event internal representation
 * @param timeout how much time to wait
 *
 * @return JITTRUE if the event is signalled, JITFALSE otherwise.
 */
JITBOOLEAN System_Threading_WaitHandle_InternalWaitOne (JITNINT handle, JITINT32 timeout);

/**
 * Wait that all events in handle array are signalled.
 *
 * @param handles an array of events
 * @param timeout how much time to wait; if -1 wait until all event are
 *                signalled, if 0 don't wait, only check
 * @param exitContext whether exit from a synchronization context while waiting
 *
 * @return JITTRUE if all events in handle has been signalled, JITFALSE
 *                 otherwise
 */
JITBOOLEAN System_Threading_WaitHandle_InternalWaitAll (JITNINT* handles, JITINT32 timeout, JITBOOLEAN exitContext);

JITBOOLEAN System_Threading_Thread_CanStartThreads (void);

/**
 * Get the current Thread
 *
 * This is a Mono internal call.
 *
 * @return the pointer to the current thread
 */

void* System_Threading_Thread_CurrentThread_internal (void);

#endif /* INTERNAL_CALLS_THREAD_H */
