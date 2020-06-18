/*
 * Copyright (C) 2009 - 2010 Campanoni Simone
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
#include <string.h>
#include <assert.h>
#include <xanlib.h>
#include <platform_API.h>

#if INTERNAL_GC
#define GC_THREADS
#include <gc.h>
#endif

// My headers
#include <config.h>
#include <compiler_memory_manager.h>
#include <shared_memory_pool.h>
// End

void MEMORY_shutdown (MemoryManager *self) {
}

void MEMORY_initMemoryManager (MemoryManager *self) {

    /* Assertions                   */
    assert(self != NULL);

#ifdef INTERNAL_GC
    GC_INIT();
#endif
}

void * allocAlignedMemoryFunction (size_t alignment, size_t size) {
    void *newMemory;

    newMemory = NULL;
    if (size == 0) {
        return NULL;
    }
    if (PLATFORM_posix_memalign(&newMemory, alignment, size) == 0) {
        memset(newMemory, 0, size);
    }
    return newMemory;
}

void * allocFunction (size_t size) {
    void            *newMemory;

    /* Assertions                   */
    assert(size > 0);

    /* Allocate a new memory	*/
#if INTERNAL_GC
    newMemory = GC_MALLOC(size);
#else
    newMemory = calloc(size, 1);
#endif
    assert(newMemory != NULL);

    /* Return			*/
    return newMemory;
}

void freeFunction (void *element) {
#if INTERNAL_GC
    GC_FREE(element);
#elif defined MULTIAPP
    if ( SMP_belongsToPool(element) ) {
        SMP_free(element);
    } else {
        free(element);
    }
#else
    free(element);
#endif
}

void * dynamicReallocFunction (void *addr, size_t newSize) {
    void            *newMemory;

    /* Assertions           */
    assert(newSize > 0);

    /* Realloc the memory	*/
    if (addr == NULL) {
        newMemory = allocFunction(newSize);
        assert(newMemory != NULL);
    } else {
#if INTERNAL_GC
        newMemory = GC_REALLOC(addr, newSize);
#elif defined MULTIAPP
        if ( SMP_belongsToPool(addr) )	{
            newMemory	= SMP_realloc(addr, newSize);
        } else {
            newMemory	= realloc(addr, newSize);
        }
#else
        newMemory = realloc(addr, newSize);
#endif
        assert(newMemory != NULL);
    }
    assert(newMemory != NULL);

    /* Return		*/
    return newMemory;
}

void * reallocFunction (void *addr, size_t newSize) {
    void            *newMemory;

    /* Assertion            */
    assert(newSize > 0);

    /* Realloc the memory	*/
    if (addr == NULL) {
        newMemory = allocFunction(newSize);
        assert(newMemory != NULL);
    } else {
#if INTERNAL_GC
        newMemory = GC_REALLOC(addr, newSize);
#else
        newMemory = realloc(addr, newSize);
#endif
        assert(newMemory != NULL);
    }
    assert(newMemory != NULL);
    if (newMemory != addr) {
        print_err("CompilerMemoryManager: ERROR = cannot resize the memory. ", 0);
        abort();
    }

    /* Return		*/
    return newMemory;
}

JITINT32 MEMORY_enableSharedMemory(MemoryManager *self) {

    /* Assertions                   */
    assert(self != NULL);

    /* Open the shared memory pool	*/
    return SMP_openPool(573741824);
}

JITINT32 MEMORY_enablePrivateMemory(MemoryManager *self) {

    /* Assertions                   */
    assert(self != NULL);

    /* Open the private memory pool	*/
    return PMP_openPool(1000000, allocFunction, sharedAllocFunction);
}

void * sharedAllocFunction (size_t size) {
    void		*newMemory;

    /* Assertions                   */
    assert(size > 0);

    /* Allocate a new memory	*/
#ifdef MULTIAPP
    newMemory       = SMP_calloc(size, 1);
#else
    newMemory	= allocFunction(size);
#endif
    assert(newMemory != NULL);

    /* Return			*/
    return newMemory;
}

void * MEMORY_obtainPrivateAddress (void) {
    void		*newAddress;

#ifdef MULTIAPP
    /* Reserve a private address	*/
    newAddress	= PMP_obtainAddress();

    assert(newAddress != NULL);
    assert(*((void**)newAddress) == NULL);
#else
    newAddress	= calloc(1, sizeof(void**));
#endif

    /* Return			*/
    return newAddress;
}

void libcompilermemorymanagerCompilationFlags (char *buffer, int bufferLength) {

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

void libcompilermemorymanagerCompilationTime (char *buffer, int bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libcompilermemorymanagerVersion () {
    return VERSION;
}

void MEMORY_shareMutex (pthread_mutexattr_t *mutex_attr) {

    /* Assertions		*/
    assert(mutex_attr != NULL);

    /* Set the attributes	*/
#ifdef MULTIAPP
    PLATFORM_setMutexAttr_pshared(mutex_attr, PTHREAD_PROCESS_SHARED);
#endif

    /* Return		*/
    return ;
}

void MEMORY_shareConditionVariable (pthread_condattr_t *cond_attr) {

    /* Assertions		*/
    assert(cond_attr != NULL);

    /* Set the attributes	*/
#ifdef MULTIAPP
    PLATFORM_setCondVarAttr_pshared(cond_attr, PTHREAD_PROCESS_SHARED);
#endif

    /* Return		*/
    return ;
}
