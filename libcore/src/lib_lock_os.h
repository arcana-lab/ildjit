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

#ifndef LIB_LOCK_OS_H
#define LIB_LOCK_OS_H

#include <jitsystem.h>
#include <lib_thread.h>
#include <platform_API.h> /* Multi-Platform support */

/**
 * Direct mapped monitor
 */
typedef struct {

    /**
     * Used for mutual exclusion access
     */
    pthread_mutex_t lock;

    /**
     * Used for pulse/wait implementation
     */
    pthread_cond_t condition;

    /**
     * Thread currently owning the monitor
     */
    volatile CLRThread* owner;

    /**
     * How many locks the owner threads has acquired
     */
    JITUINT32 locksCount;
} ILJITMonitor;

#endif /* LIB_LOCK_OS_H */
