/*
 * Copyright (C) 2009 - 2010  Campanoni Simone
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// My header
#include <iljit_gc_allocator.h>
#include <config.h>
// End

t_garbage_collector_plugin plugin_interface = {
    allocator_init,
    allocator_shutdown,
    allocator_getSupport,
    allocator_allocObject,
    allocator_freeObject,
    allocator_collect,
    allocator_threadCreate,
    allocator_gcInformations,
    allocator_getName,
    allocator_getVersion,
    allocator_getAuthor,
    allocator_getCompilationFlags,
    allocator_getCompilationTime
};

static inline void allocator_init (t_gc_behavior *gcBehavior, JITNUINT sizeMemory) {
    return;
}
static inline JITINT16 allocator_shutdown (void) {
    return 0;
}

static inline JITINT32 allocator_getSupport (void) {
    return 0;
}

static inline void allocator_collect (void) {
    return;
}

static inline void allocator_threadCreate (pthread_t *thread, pthread_attr_t *attr, void *(*startRouting)(void *), void *arg) {
    abort();
}

static inline char * allocator_getName (void) {
    return "Allocator";
}

static inline char * allocator_getAuthor (void) {
    return "Simone Campanoni";
}

static inline char * allocator_getVersion (void) {
    return VERSION;
}

static inline void allocator_gcInformations (t_gc_informations *gcInfo) {

}

static inline void * allocator_allocObject (JITNUINT size) {
    return allocMemory((size_t) size);
}

static inline void allocator_freeObject (void *obj) {
    freeMemory(obj);
}

static inline void allocator_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

static inline void allocator_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
