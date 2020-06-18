/*
 * Copyright (C) 2006  Campanoni Simone, Di Biagio Andrea
 *
 * iljit - This is a Just-in-time for the CIL language specified with the
 *         ECMA-335
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <platform_API.h>

/* Module header */
#include <lib_lock.h>

/* Include fat lock library with rewritten names */
#define ILJITMonitor ILJITFatMonitor
#define ILJITLibLockInit ILJITFatLibLockInit
#define ILJITMonitorInit ILJITFatMonitorInit
#define ILJITMonitorDestroy ILJITFatMonitorDestroy
#define ILJITMonitorLock ILJITFatMonitorLock
#define ILJITMonitorUnlock ILJITFatMonitorUnlock
#define ILJITMonitorWaitForPulse ILJITFatMonitorWaitForPulse
#define ILJITMonitorSignalPulse ILJITFatMonitorSignalPulse
#define ILJITMonitorSignalPulseAll ILJITFatMonitorSignalPulseAll
#include <lib_lock_os.h>
#include <lib_lock_os.c>
#undef ILJITMonitor
#undef ILJITLibLockInit
#undef ILJITMonitorInit
#undef ILJITMonitorDestroy
#undef ILJITMonitorLock
#undef ILJITMonitorUnlock
#undef ILJITMonitorWaitForPulse
#undef ILJITMonitorSignalPulse
#undef ILJITMonitorSignalPulseAll

/* Null thread identifier */
#define NULL_THREAD_ID 0

/* Lock word if monitor unlocked */
#define UNLOCKED_THIN_LOCK_WORD 0

/* Mask for hashing thread id ~(2^6 - 1) */
#define THREAD_ID_HASH_MASK ~63u

/* Number of unused bits in raw thread id */
#define THREAD_ID_UNUSED_BITS_COUNT 6

/* Mask for monitor kind bit (2^31) */
#define LOCK_WORD_MASK 2147483648u

/* Mask for count field (2^5 - 1) */
#define COUNT_FIELD_MASK 31u

/* Mask for thread id field */
#define THREAD_ID_FIELD_MASK (~(LOCK_WORD_MASK | COUNT_FIELD_MASK))

/* Length, in bits, of count field */
#define COUNT_FIELD_LENGTH 5

/* Fat monitor footprint (2^31) */
#define FAT_MONITOR_PRINT 2147483648u

/* Maximum nesting deep with thin monitors (2^5 - 1) */
#define MAX_NESTING_DEEP 31u

/* Monitor memory alignment */
#define MONITOR_ALIGNMENT sizeof(void*)

/* Number of wasted bit due to alignment */
#define MONITOR_ALIGNMENT_BITS 2

/* Check if given monitor is one of fat kind */
#define IS_FAT_MONITOR(monitor) ((monitor->lockWord & LOCK_WORD_MASK) == \
				 FAT_MONITOR_PRINT)

/* Get the thin monitor lock word */
#define GET_THIN_MONITOR(monitor) (monitor->lockWord)

/* Get a pointer to the fat monitor */
#define GET_FAT_MONITOR(monitor) \
	((ILJITFatMonitor*) (monitor->lockWord << MONITOR_ALIGNMENT_BITS))

/* Store given fat monitor inside the lock word */
#define SET_FAT_MONITOR(monitor, fatMonitor) \
	(monitor->lockWord = ((JITNINT) fatMonitor >> \
			      MONITOR_ALIGNMENT_BITS) |	\
			     FAT_MONITOR_PRINT)

/* Get thread identifier stored inside a thin monitor */
#define GET_THREAD_ID(monitor) ((monitor->lockWord & THREAD_ID_FIELD_MASK) >> \
				COUNT_FIELD_LENGTH)

/* Shift given thread id to thread field */
#define SHIFT_TO_THREAD_FIELD(threadId) (threadId << COUNT_FIELD_LENGTH)

/* Increment counter field of a thin lock */
#define INCREMENT_THIN_COUNTER(monitor) (monitor->lockWord++)

/* Decrement counter filed of a thin lock */
#define DECREMENT_THIN_COUNTER(monitor) (monitor->lockWord--)

/* Build a thin lock word */
#define BUILD_THIN_LOCK_WORD(threadID, count) \
	((count & COUNT_FIELD_MASK) | \
	 (threadID << COUNT_FIELD_LENGTH) \
	)

/* Get current thread identifier */
#define GET_CURRENT_THREAD_IDENTIFIER()	\
	(threadID != NULL_THREAD_ID ? \
	 threadID : llt_buildCurrentThreadIdentifier())

/* Private function prototypes */
static JITBOOLEAN llt_tryLockUnlocked (ILJITMonitor* monitor);
static JITBOOLEAN llt_tryLockShallowlyNested (ILJITMonitor* monitor);
static JITBOOLEAN llt_tryLockFat (ILJITMonitor* monitor);
static JITBOOLEAN llt_tryUnlockSingleLocked (ILJITMonitor* monitor);
static JITBOOLEAN llt_tryUnlockShallowlyNested (ILJITMonitor* monitor);
static JITBOOLEAN llt_tryUnlockFat (ILJITMonitor* monitor);
static JITNINT llt_buildCurrentThreadIdentifier (void);
static void llt_inflateMonitor (ILJITMonitor* monitor, JITNINT startDeep);

/* Thread id, on TLS */
static __thread JITNINT threadID = NULL_THREAD_ID;

/* Initialize module */
void ILJITLibLockInit (void) {

    ILJITFatLibLockInit();

}

/* Initialize given monitor */
void ILJITMonitorInit (ILJITMonitor* monitor) {

    monitor->lockWord = UNLOCKED_THIN_LOCK_WORD;

}

/* Destroy given monitor */
void ILJITMonitorDestroy (ILJITMonitor* monitor) {

    /* A fat monitor */
    ILJITFatMonitor* fatMonitor;

    /* Free monitor, it it is a fat one */
    if (IS_FAT_MONITOR(monitor)) {
        fatMonitor = GET_FAT_MONITOR(monitor);
        ILJITFatMonitorDestroy(fatMonitor);
        freeFunction(fatMonitor);
    }

}

/* Acquire lock */
JITBOOLEAN ILJITMonitorLock (ILJITMonitor* monitor, JITINT32 timeout) {

    /* Whether lock has been acquired */
    JITBOOLEAN lockAcquired;

    /* Timeout parameter discriminates between different behaviours */
    switch (timeout) {

            /* Classical lock */
        case WAIT_FOR_LOCK_FOREVER:
            lockAcquired = llt_tryLockUnlocked(monitor);
            if (lockAcquired) {
                break;
            }
            lockAcquired = llt_tryLockShallowlyNested(monitor);
            if (lockAcquired) {
                break;
            }
            lockAcquired = llt_tryLockFat(monitor);
            break;

            /* Get the lock without waiting */
        case CHECK_FOR_LOCK:
            print_err("Lock acquisition without waiting not yet supported",
                      0);
            abort();
            break;

            /* Time bounded acquisition */
        default:
            print_err("Time bounded lock acquisition not yet implemented",
                      0);
            abort();
    }

    return lockAcquired;

}

/* Release lock */
void ILJITMonitorUnlock (ILJITMonitor* monitor) {

    /* Whether monitor has been unlocked */
    JITBOOLEAN unlocked;

    /* Try unlock a single locked lock */
    unlocked = llt_tryUnlockSingleLocked(monitor);
    if (unlocked) {
        return;
    }
    unlocked = llt_tryUnlockShallowlyNested(monitor);
    if (unlocked) {
        return;
    }
    llt_tryUnlockFat(monitor);

}

/* Wait for a pulse */
JITBOOLEAN ILJITMonitorWaitForPulse (ILJITMonitor* monitor, JITINT32 timeout) {

    print_err("Monitor waiting for pulse not yet implemented", 0);
    abort();

}

/* Signal a pulse to a thread */
void ILJITMonitorSignalPulse (ILJITMonitor* monitor) {

    print_err("Monitor signalling not yet implemented", 0);
    abort();

}

/* Signal pulse to all threads */
void ILJITMonitorSignalPulseAll (ILJITMonitor* monitor) {

    print_err("Monitor signalling (alls) not yet implemented", 0);
    abort();

}

/* Try locking a maybe-unlocked monitor */
static JITBOOLEAN llt_tryLockUnlocked (ILJITMonitor* monitor) {

    /* Current threadIdentifier */
    JITNINT currentThreadIdentifier;

    /* Whether lock has been acquired */
    JITBOOLEAN lockAcquired;

    /* Expected lock word before CAS operation */
    JITNINT oldLockWord;

    /* Expected lock word after CAS operation */
    JITNINT nextLockWord;

    /* Get current thread identifier */
    currentThreadIdentifier = GET_CURRENT_THREAD_IDENTIFIER();

    /* Try lock with a CAS */
    oldLockWord = UNLOCKED_THIN_LOCK_WORD;
    nextLockWord = BUILD_THIN_LOCK_WORD(currentThreadIdentifier, 0);
    lockAcquired = __sync_bool_compare_and_swap(&monitor->lockWord,
                   oldLockWord, nextLockWord);

    return lockAcquired;

}

/* Try a shallowly nested lock acquisition */
static JITBOOLEAN llt_tryLockShallowlyNested (ILJITMonitor* monitor) {

    /* Current thread identifier */
    JITNINT currentThreadIdentifier;

    /* Thin monitor lock word */
    JITNINT lockWord;

    /* Counter field of thin monitor lock word */
    JITNINT counter;

    /* Whether lock has been acquired */
    JITBOOLEAN acquired;

    /* Get current thread identifier */
    currentThreadIdentifier = GET_CURRENT_THREAD_IDENTIFIER();

    /*
     * Get as fast as possible counter and also preparing for monitor shape
     * check
     */
    lockWord = GET_THIN_MONITOR(monitor);
    counter = SHIFT_TO_THREAD_FIELD(currentThreadIdentifier) ^ lockWord;

    /* Shallowly lock acquisition success */
    if (counter < MAX_NESTING_DEEP) {
        INCREMENT_THIN_COUNTER(monitor);
        acquired = JITTRUE;

        /* Deeply lock acquisition success */
    } else if (counter == MAX_NESTING_DEEP) {
        llt_inflateMonitor(monitor, MAX_NESTING_DEEP + 2);
        acquired = JITTRUE;

        /* Locking acquisition failed */
    } else {
        acquired = JITFALSE;
    }

    return acquired;

}

/* Try locking using a fat monitor */
static JITBOOLEAN llt_tryLockFat (ILJITMonitor* monitor) {

    /* Current thread identifier */
    JITNINT currentThreadIdentifier;

    /* Lock word locked once by me */
    JITNINT nextLockWord;

    /* Whether monitor has been locked */
    JITBOOLEAN lockAcquired;

    /* Whether monitor, as a thin lock, has been locked */
    JITBOOLEAN thinLockAcquired;

    /* Prepare a new thin lock word */
    currentThreadIdentifier = GET_CURRENT_THREAD_IDENTIFIER();
    nextLockWord = BUILD_THIN_LOCK_WORD(currentThreadIdentifier, 0);

    /* Try locking using a fat monitor */
    lockAcquired = JITFALSE;
    do {

        /* I, or someone else, has inflated the monitor */
        if (IS_FAT_MONITOR(monitor)) {
            ILJITFatMonitorLock(GET_FAT_MONITOR(monitor),
                                WAIT_FOR_LOCK_FOREVER);
            lockAcquired = JITTRUE;

            /*
             * The monitor is a thin one, or it has been inflated but
             * pointer not yet updated
             */
        } else {

            /* Heuristic: monitor is thin */
            thinLockAcquired = __sync_bool_compare_and_swap(
                                   &monitor->lockWord,
                                   UNLOCKED_THIN_LOCK_WORD,
                                   nextLockWord);

            /* Speculation confirmed: inflate */
            if (thinLockAcquired) {
                llt_inflateMonitor(monitor, 1);
                lockAcquired = JITTRUE;
            }
        }
    } while (!lockAcquired);

    return JITTRUE;

}

/* Try unlock a maybe single locked lock */
static JITBOOLEAN llt_tryUnlockSingleLocked (ILJITMonitor* monitor) {

    /* Current thread identifier for thin locking library */
    JITNINT currentThreadIdentifier;

    /* Expected current lock word */
    JITNINT presumableCurrentWord;

    /* Whether unlocking successes */
    JITBOOLEAN unlocked;

    /* Build the expected current word */
    currentThreadIdentifier = GET_CURRENT_THREAD_IDENTIFIER();
    presumableCurrentWord = BUILD_THIN_LOCK_WORD(currentThreadIdentifier,
                            0);
    /* OK, unlock */
    if (presumableCurrentWord == GET_THIN_MONITOR(monitor)) {
        monitor->lockWord = UNLOCKED_THIN_LOCK_WORD;
        unlocked = JITTRUE;

        /* Something bad happens, unlocking failed */
    } else {
        unlocked = JITFALSE;
    }

    return unlocked;

}

/* Try unlocking a maybe fat monitor */
static JITBOOLEAN llt_tryUnlockFat (ILJITMonitor* monitor) {

    /* Given monitor is thin */
    if (!IS_FAT_MONITOR(monitor)) {
        print_err("Trying unlocking a non owned thin lock: an "
                  "exception must be raised!", 0);
        abort();
    }

    /* Unlock via fat monitor library */
    ILJITFatMonitorUnlock(GET_FAT_MONITOR(monitor));

    return JITTRUE;

}

/* Try unlocking a shallowly nested thin lock */
static JITBOOLEAN llt_tryUnlockShallowlyNested (ILJITMonitor* monitor) {

    /* Thin monitor lock word */
    JITNINT lockWord;

    /* Thin monitor counter field */
    JITNINT counter;

    /* Current thread identifier */
    JITNINT currentThreadIdentifier;

    /* Whether monitor has been released */
    JITBOOLEAN released;

    /* Get current thread identifier */
    currentThreadIdentifier = GET_CURRENT_THREAD_IDENTIFIER();

    /* Get thin monitor counter and prepare it for later checks */
    lockWord = GET_THIN_MONITOR(monitor);
    counter = SHIFT_TO_THREAD_FIELD(currentThreadIdentifier) ^ lockWord;

    /* OK to release */
    if (counter <= MAX_NESTING_DEEP) {
        DECREMENT_THIN_COUNTER(monitor);
        released = JITTRUE;

        /* Releasing failed */
    } else {
        released = JITFALSE;
    }

    return released;

}

/* Get current thread identifier */
static JITNINT llt_buildCurrentThreadIdentifier (void) {

    /* System description */
    t_system* system;

    /* The thread manager */
    t_threadManager* threadManager;

    /* Current thread, as a CLR object */
    void* currentThread;

    /* Current thread, internal representation */
    CLRThread* currentThreadInfo;

    /* Get the thread manager */
    system = getSystem(NULL);
    threadManager = &system->threadManager;

    /* Get current thread internal representation */
    currentThread = threadManager->getCurrentThread();
    currentThreadInfo = threadManager->getPrivateData(currentThread);

    /* Hash address to get thread ID */
    threadID = ((JITNINT) currentThreadInfo) & THREAD_ID_HASH_MASK >>
               THREAD_ID_UNUSED_BITS_COUNT;

    return threadID;

}

/* Inflate given monitor */
static void llt_inflateMonitor (ILJITMonitor* monitor, JITNINT startDeep) {

    /* Heavyweight monitor */
    ILJITFatMonitor* fatMonitor;

    /* Current nesting deep */
    JITNINT i;

    /* Allocate monitor aligned, so we save some bits */
    PLATFORM_posix_memalign((void**) &fatMonitor, MONITOR_ALIGNMENT,
                            sizeof(ILJITFatMonitor));

    /* Initialize it and get the lock */
    ILJITFatMonitorInit(fatMonitor);
    for (i = 0; i < startDeep; i++) {
        ILJITFatMonitorLock(fatMonitor, WAIT_FOR_LOCK_FOREVER);
    }

    /* Store it inside the lock word */
    SET_FAT_MONITOR(monitor, fatMonitor);

}
