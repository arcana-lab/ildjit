/*
 * Copyright (C) 2009 - 2011  Campanoni Simone
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
#include <lib_lock.h>
#include <platform_API.h>

// My headers
#include <system_manager.h>
// End

/* Private function prototypes */
static void ll_toAbsoluteTime (JITINT32 milliseconds, struct timespec* absoluteTime);
static JITBOOLEAN ll_timeBoundReached (struct timespec* timeBound);
static JITBOOLEAN ll_monitorOwned (ILJITMonitor* monitor, CLRThread* thread);

extern t_system *ildjitSystem;

/* Module initialization */
void ILJITLibLockInit (void) {

}

/* Monitor initialization */
void ILJITMonitorInit (ILJITMonitor* monitor) {

    /* Init lock and condition variable */
    pthread_mutexattr_t mutex_attr;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PLATFORM_initMutex(&monitor->lock, &mutex_attr);
    PLATFORM_initCondVar(&monitor->condition, NULL);

    /* No owner by default */
    monitor->owner = NULL;
}

/* Destroy monitor */
void ILJITMonitorDestroy (ILJITMonitor* monitor) {
    PLATFORM_destroyCondVar(&monitor->condition);
    PLATFORM_destroyMutex(&monitor->lock);
}

/* Monitor lock*/
JITBOOLEAN ILJITMonitorLock (ILJITMonitor* monitor, JITINT32 timeout) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Current thread description */
    CLRThread* currentThreadInfo;

    /* Whether lock has been acquired */
    JITBOOLEAN lockAcquired;

    /* Time limit for bounded lock acquisitions */
    struct timespec endTime;

    /* Get the thread manager */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Get current thread */
    currentThreadInfo = threadManager->getPrivateData(threadManager->getCurrentThread());

    /*
     * First acquisition: timeout parameter discriminates between different
     * behaviors
     */
    if (!ll_monitorOwned(monitor, currentThreadInfo)) {
        switch (timeout) {

                /* Classical lock */
            case WAIT_FOR_LOCK_FOREVER:
                PLATFORM_lockMutex(&monitor->lock);
                monitor->owner = currentThreadInfo;
                monitor->locksCount = 1;
                lockAcquired = JITTRUE;
                break;

                /* Get the lock without waiting */
            case CHECK_FOR_LOCK:
                if (PLATFORM_trylockMutex(&monitor->lock) == 0) {
                    monitor->owner = currentThreadInfo;
                    monitor->locksCount = 1;
                    lockAcquired = JITTRUE;
                } else {
                    lockAcquired = JITFALSE;
                }
                break;

                /* Time bounded acquisition */
            default:
                lockAcquired = JITFALSE;
                ll_toAbsoluteTime(timeout, &endTime);
                do {
                    if (PLATFORM_trylockMutex(&monitor->lock) == 0) {
                        monitor->owner = currentThreadInfo;
                        monitor->locksCount = 1;
                        lockAcquired = JITTRUE;
                    }
                } while (!lockAcquired && !ll_timeBoundReached(&endTime));
        }

    } else {

        /* Recursive acquisition */
        monitor->locksCount++;
        lockAcquired = JITTRUE;
    }
    __sync_synchronize();

    return lockAcquired;
}

/* Monitor unlock */
void ILJITMonitorUnlock (ILJITMonitor* monitor) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Current thread description */
    CLRThread* currentThread;

    /* Get the thread manager */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Get current thread */
    currentThread = threadManager->getPrivateData(threadManager->getCurrentThread());

    /* Monitor owning check */
    if (!ll_monitorOwned(monitor, currentThread)) {
        print_err("Unlocking a non owned lock: exception must be raised", 0);
        abort();
    }

    /* Decrement locks count */
    monitor->locksCount--;

    /* Last unlock, free OS resources */
    if (monitor->locksCount == 0) {
        monitor->owner = NULL;
        PLATFORM_unlockMutex(&monitor->lock);
        __sync_synchronize();
    }
}

/* Wait for pulse signal on monitor */
JITBOOLEAN ILJITMonitorWaitForPulse (ILJITMonitor* monitor, JITINT32 timeout) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Current thread description */
    CLRThread* currentThread;

    /* Number of monitor acquisition by current thread */
    JITUINT32 locksCount;

    /* Set to JITTRUE if pulse signal is detected before timeout */
    JITBOOLEAN pulseOccursBeforeTimeout;

    /* Get the thread manager */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Get current thread */
    currentThread = threadManager->getPrivateData(
                        threadManager->getCurrentThread());

    /* Lock owning check */
    if (!ll_monitorOwned(monitor, currentThread)) {
        print_err("Monitor not owned! An exception must be raised", 0);
        abort();
    }

    /* The timeout parameter identify different behaviors */
    switch (timeout) {

            /* Normal untimed wait */
        case WAIT_FOR_PULSE_FOREVER:

            /* Save state and potentially sleep */
            locksCount = monitor->locksCount;
            PLATFORM_waitCondVar(&monitor->condition, &monitor->lock);

            /* Restore state after sleep */
            monitor->owner = currentThread;
            monitor->locksCount = locksCount;

            /* Lock acquired */
            pulseOccursBeforeTimeout = JITTRUE;
            break;

            /* Check for pulse signal without waiting */
        case CHECK_FOR_PULSE:
            print_err("Checking for pulse signal non yet "
                      "implemented", 0);
            abort();
            break;

            /* Timed wait */
        default:
            print_err("Timed waiting not yet implemented", 0);
            abort();
    }

    return pulseOccursBeforeTimeout;
}

/* Signal pulse */
void ILJITMonitorSignalPulse (ILJITMonitor* monitor) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Current thread description */
    CLRThread* currentThread;

    /* Get the thread manager */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Get current thread */
    currentThread = threadManager->getPrivateData(
                        threadManager->getCurrentThread());

    /* Owning check */
    if (!ll_monitorOwned(monitor, currentThread)) {
        print_err("Monitor not owned! An exception must be raised!", 0);
        abort();
    }

    /* Signal the pulse */
    PLATFORM_signalCondVar(&monitor->condition);
}

/* Signal pulse all */
void ILJITMonitorSignalPulseAll (ILJITMonitor* monitor) {

    /* The thread manager */
    t_threadsManager* threadManager;

    /* Current thread description */
    CLRThread* currentThread;

    /* Get the thread manager */
    threadManager = &(ildjitSystem->cliManager).CLR.threadsManager;

    /* Get current thread */
    currentThread = threadManager->getPrivateData(threadManager->getCurrentThread());

    /* Owning check */
    if (!ll_monitorOwned(monitor, currentThread)) {
        print_err("Monitor not owned! An exception must be raised!", 0);
        abort();
    }

    /* Signal the pulse all */
    PLATFORM_broadcastCondVar(&monitor->condition);

    /* Return		*/
    return;
}

/* Translate milliseconds time interval into absolute (sec, nsec) pair */
static void ll_toAbsoluteTime (JITINT32 milliseconds,
                               struct timespec* absoluteTime) {

    /* Current time */
    struct timespec currentTime;

    /* Integer division result */
    div_t result;

    /* Second part of time */
    time_t seconds;

    /* Nanoseconds part of time */
    long nanoseconds;

    /* Get all what we need */
    PLATFORM_clock_gettime(CLOCK_MONOTONIC, &currentTime);
    result = div(milliseconds, 1000);

    /* Compute time */
    seconds = result.quot + currentTime.tv_sec;
    nanoseconds = result.rem * 1000l * 1000l + currentTime.tv_nsec;
    if (nanoseconds >= 1000l * 1000l * 1000l) {
        seconds++;
        nanoseconds -= 1000l * 1000l * 1000l;
    }

    /* Save bound */
    absoluteTime->tv_sec = seconds;
    absoluteTime->tv_nsec = nanoseconds;
}

/* Check whether timeBound has been reached */
static JITBOOLEAN ll_timeBoundReached (struct timespec* timeBound) {

    /* Current time */
    struct timespec currentTime;

    /* Whether bound has been reached */
    JITBOOLEAN reached;

    /* Get current time */
    PLATFORM_clock_gettime(CLOCK_MONOTONIC, &currentTime);

    /* Second bound not reached */
    if (currentTime.tv_sec < timeBound->tv_sec) {
        reached = JITFALSE;
    }
    /* Next second bound passed */
    else if (currentTime.tv_sec > timeBound->tv_sec) {
        reached = JITTRUE;
    }
    /* After both second and nanosecond bound */
    else if (currentTime.tv_nsec >= timeBound->tv_nsec) {
        reached = JITTRUE;
    }
    /* Between second and nanosecond bound */
    else {
        reached = JITFALSE;
    }

    return reached;
}

/* Check whether monitor is currently owned by caller thread */
static JITBOOLEAN ll_monitorOwned (ILJITMonitor* monitor, CLRThread* thread) {
    /* Whether monitor is owned by current thread */
    JITBOOLEAN monitorOwned;

    /* Owning check */
    if (thread == monitor->owner) {
        monitorOwned = JITTRUE;
    } else {
        monitorOwned = JITFALSE;
    }

    return monitorOwned;
}
