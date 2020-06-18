/*
 * Copyright (C) 2009  Campanoni Simone
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
   *@file iljit_gc_allocator.h
 */
#ifndef ILJIT_GC_ALLOCATOR_H
#define ILJIT_GC_ALLOCATOR_H

#include <iljit-utils.h>
#include <jitsystem.h>
#include <garbage_collector.h>
#include <compiler_memory_manager.h>

#ifdef PRINTDEBUG
#define PDEBUG(fmt, args ...) fprintf(stderr, fmt, ## args)
#else
#define PDEBUG(fmt, args ...)
#endif

typedef struct {
    JITFLOAT32 collectTime;
    JITFLOAT32 virtualMethodsTime;
    JITFLOAT32 allocTime;
    JITFLOAT32 fieldsTime;
} t_gc_time;

static inline void              allocator_init (t_gc_behavior *gcBehavior, JITNUINT sizeMemory);
static inline JITINT16          allocator_shutdown (void);
static inline JITINT32          allocator_getSupport (void);
static inline void *            allocator_allocObject (JITNUINT size);
static inline void              allocator_freeObject (void *obj);
static inline void              allocator_collect (void);
static inline void              allocator_gcInformations (t_gc_informations *gcInfo);
static inline void allocator_threadCreate(pthread_t * thread, pthread_attr_t * attr, void *(*startRouting)(void *), void *arg);
static inline char *            allocator_getName (void);
static inline char *            allocator_getVersion (void);
static inline char *            allocator_getAuthor (void);
static inline void              allocator_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void              allocator_getCompilationTime (char *buffer, JITINT32 bufferLength);

#endif
