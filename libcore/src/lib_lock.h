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

#ifndef LIB_LOCK_H
#define LIB_LOCK_H

#include <jitsystem.h>
#include <lib_thread.h>
#include <platform_API.h>

/* Include the right lock library: first is lib_lock_os */
#ifdef USE_LIB_LOCK_OS
#include <lib_lock_os.h>

/* Use thin lock library */
#elif USE_LIB_LOCK_THIN
#include <lib_lock_thin.h>

/* Default, use lib_lock_os */
#else
#include <lib_lock_os.h>
#endif

/**
 * Wait until lock is acquired flag
 */
#define WAIT_FOR_LOCK_FOREVER -1

/**
 * Try to get lock without waiting
 */
#define CHECK_FOR_LOCK 0

/**
 * Wait until a monitor is pulsed
 */
#define WAIT_FOR_PULSE_FOREVER  -1

/**
 * Check that a monitor has been pulsed flag
 */
#define CHECK_FOR_PULSE         0

/**
 * Initialize lib lock
 */
void ILJITLibLockInit (void);

/**
 * Build a new monitor
 *
 * @param monitor the monitor to initialize
 */
void ILJITMonitorInit (ILJITMonitor* monitor);

/**
 * Destroy given monitor
 *
 * @param monitor the monitor to destroy
 */
void ILJITMonitorDestroy (ILJITMonitor* monitor);

/**
 * Lock given monitor
 *
 * @param monitor monitor to lock
 * @param timeout if WAIT_FOR_LOCK_FOREVER wait until lock is aquired, if
 *                CHECK_FOR_LOCK try lock acquisition without waiting, otherwise
 *                try lock acquisition for timeout milliseconds
 *
 * @return JITTTRUE JITTRUE if lock has been acquired, JITFALSE otherwise
 */
JITBOOLEAN ILJITMonitorLock (ILJITMonitor* monitor, JITINT32 timeout);

/**
 * Unlock given monitor
 *
 * @param monitor monitor to unlock
 */
void ILJITMonitorUnlock (ILJITMonitor* monitor);

/**
 * Wait for pulse signal on monitor
 *
 * @param monitor where wait for pulse signal
 * @param timeout discriminates between different behaviours. If
 *                WAIT_FOR_PULSE_FOREVER wait until pulse signal is raised, if
 *                CHECK_FOR_PULSE check if signal has been raised without
 *                waiting, otherwise wait for timeout milliseconds
 *
 * @return JITTRUE if pulse signal has been detected, JITFALSE otherwise
 */
JITBOOLEAN ILJITMonitorWaitForPulse (ILJITMonitor* monitor, JITINT32 timeout);

/**
 * Send pulse signal to monitor
 *
 * @param monitor the monitor to signal
 */
void ILJITMonitorSignalPulse (ILJITMonitor* monitor);

/**
 * Send pulse all signal to monitor
 *
 * @param monitor the monitor to signal
 */
void ILJITMonitorSignalPulseAll (ILJITMonitor* monitor);

#endif /* LIB_LOCK_H */
