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
/**
 * @file compiler_memory_manager.h
 */
#ifndef COMPILER_MEMORY_MANAGER_H
#define COMPILER_MEMORY_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>

// My headers
#include <private_memory_pool.h>
#include <shared_memory_pool.h>
// End

/**
 * \ingroup CompilerMemory
 * @brief Memory Manager
 *
 * This structure describe the memory manager used for the dynamic memory needed by the compiler for the translation/optimization of the bytecode program in execution.
 */
typedef struct MemoryManager {
} MemoryManager;

/**
 * \ingroup CompilerMemory
 * @brief Memory allocation
 *
 * Allocate size bytes of new memory
 *
 * @param size Number of Bytes to allocate
 * @return First address of the allocated memory
 */
void * allocFunction (size_t size);

/**
 * \ingroup CompilerMemory
 * @brief Memory allocation
 *
 * Allocate size bytes and return it.
 *
 * The address of the allocated memory will be a multiple of <code> alignment </code>, which must be a power of two and a multiple of <code> sizeof(void *) </code>.
 *
 * If size is 0, then this function returns NULL.
 *
 * @param size Number of Bytes to allocate
 * @param alignment Memory alignment to use
 * @return First address of the allocated memory
 */
void * allocAlignedMemoryFunction (size_t alignment, size_t size);

/**
 * \ingroup CompilerMemory
 * @brief Free memory previously allocated
 *
 * Free the memory pointed by the element parameter.
 *
 * @param element Address of the first Byte of the memory to free
 */
void freeFunction (void *element);

/**
 * \ingroup CompilerMemory
 * @brief Memory expansion
 *
 * Resize the memory pointed by the addr parameter without moving the previous piece of memory.
 * This action is not always feasable and when it is not, an internal abort will be executed.\n
 * Most of the time you do not need this function, use \ref dynamicReallocFunction instead.\n
 * Note that the behavior of this function is different from the behavior of the realloc function provided by the libc library.
 *
 * @param addr Address of the first Byte of the memory
 * @result The same address given as input inside the parameter addr
 */
void * reallocFunction (void *addr, size_t newSize);

/**
 * \ingroup CompilerMemory
 * @brief Memory reallocation
 *
 * This method changes the size of the memory block pointed to by addr to newSize bytes.\n
 * The contents will be unchanged to the minimum of the old and new sizes; newly allocated memory will be uninitialized.
 * If addr is NULL, then the call is equivalent to allocFunction(size), for all values of size; if newSize is equal to zero, and addr is not NULL, then the call is equivalent to freeFunction(ptr).\n
 * Unless addr is NULL, it must have been returned by an earlier call to allocFunction().
 * If the area pointed to was moved, a freeFunction(addr) is done.\n
 * Notice that this method has the same behavior of the usual realloc function provided by the libgc library.
 *
 * @result A pointer to the newly allocated memory.
 */
void * dynamicReallocFunction (void *addr, size_t newSize);

/**
 * \ingroup CompilerMemory
 * @brief Memory allocation
 *
 * Enable the use of the shared memory pool
 *
 * @return 0 if everything went fine
 */
JITINT32 MEMORY_enableSharedMemory(MemoryManager *self);

/**
 * \ingroup CompilerMemory
 * @brief Share a mutex to every process
 *
 * Share mutexes created by using the attributes <code> mutex_attr </code> to every ILDJIT process created
 *
 */
void MEMORY_shareMutex (pthread_mutexattr_t *mutex_attr);

/**
 * \ingroup CompilerMemory
 * @brief Share a condition variable to every process
 *
 * Share condition variables created by using the attributes <code> cond_attr </code> to every ILDJIT process created
 *
 */
void MEMORY_shareConditionVariable (pthread_condattr_t *cond_attr);

/**
 * \ingroup CompilerMemory
 * @brief Memory allocation
 *
 * Enable the use of the private memory pool
 *
 * @return 0 if everything went fine
 */
JITINT32 MEMORY_enablePrivateMemory(MemoryManager *self);

/**
 * \ingroup CompilerMemory
 * @brief Memory allocation
 *
 * Reserve a private address to be used for the double pointer trick
 *
 * @return A reserved memory address
 */
void * MEMORY_obtainPrivateAddress (void);

/**
 * \ingroup CompilerMemory
 * @brief Memory allocation
 *
 * Allocate size bytes of new memory from the shared memory pool
 *
 * @param size Number of Bytes to allocate
 * @return First address of the allocated memory
 */
void * sharedAllocFunction (size_t size);

/**
 * \ingroup CompilerMemory
 *
 * Allocate a new memory manager
 */
void MEMORY_initMemoryManager (MemoryManager *self);

/**
 * \ingroup CompilerMemory
 *
 * Shutdown the memory manager <code> self </code>
 *
 * @param self Memory manager to shutdown
 */
void MEMORY_shutdown (MemoryManager *self);

/**
 * \ingroup CompilerMemory
 *
 * Return the version of the Libcompilermemorymanager library
 */
char * libcompilermemorymanagerVersion ();

/**
 * \ingroup CompilerMemory
 *
 * Write the compilation flags to the buffer given as input.
 */
void libcompilermemorymanagerCompilationFlags (char *buffer, int bufferLength);

/**
 * \ingroup CompilerMemory
 *
 * Write the compilation time to the buffer given as input.
 */
void libcompilermemorymanagerCompilationTime (char *buffer, int bufferLength);

#define allocMemory(size) calloc((size_t) size, 1)
#define freeMemory(ptr) free(ptr)
#define reallocMemory(ptr, newSize) realloc(ptr, newSize)

#ifdef __cplusplus
};
#endif

#endif
