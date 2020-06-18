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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <platform_API.h>

// My headers
#include <shared_memory_pool.h>
#include <compiler_memory_manager_system.h>
// End

static inline void allocChunks(SharedMemoryPool_t *pool, int start_chunk, size_t nchunks, int operation);
static inline int findFreeChunks(SharedMemoryPool_t *pool, size_t chunks_requested);
static inline int getIndex(int absolute);
static inline int getOffset(int absolute);
static inline void* getAddress(void *startAddress, int absolute);
static inline int getAbsolute(void *startAddress, void *address);

SharedMemoryPool_t* smp_openPool(size_t size) {
    SharedMemoryPool_t	*pool;
    size_t			total_size;
    pthread_mutexattr_t	attr;

    /* Adjust size to be multiple of 8 * CHUNK_SIZE					*/
    size = ( (size % (8 * CHUNK_SIZE)) == 0 ? size : ((size / (8 * CHUNK_SIZE)) + 1) * 8 * CHUNK_SIZE );

    /* Adjust total size of memory for the pool to be multiple of the page size	*/
    total_size = ( ((size + sizeof(SharedMemoryPool_t)) % PLATFORM_getSystemPageSize()) == 0 ? size + sizeof(SharedMemoryPool_t) : (((size + sizeof(SharedMemoryPool_t)) / PLATFORM_getSystemPageSize()) + 1) * PLATFORM_getSystemPageSize() );

    /* Mmap (with MAP_SHARED | MAP_ANONYMOUS) the requested amount of memory	*/
    if ( (pool = (SharedMemoryPool_t*) PLATFORM_mmap(NULL, total_size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == (void*) -1 ) {
        perror("mmap");
        return NULL;
    }

    /* Prepare the registry of the pool						*/
    pool->registry = (int8_t*)((unsigned long)pool + (unsigned long)sizeof(SharedMemoryPool_t));
    memset(pool->registry, 0, size / (8 * CHUNK_SIZE));

    /* Save the available size of the pool						*/
    pool->size = size;
    pool->total_size = total_size;

    /* Set the pointer to the data pool						*/
    pool->poolPtr = pool->registry + size / (8 * CHUNK_SIZE);

    /* Set the cursor start position						*/
    pool->cursor = 0;

    /* Initialize the mutex								*/
    PLATFORM_initMutexAttr(&attr);
    PLATFORM_setMutexAttr_pshared(&attr, PTHREAD_PROCESS_SHARED);
    PLATFORM_initMutex(&(pool->mutex), &attr);
    PLATFORM_destroyMutexAttr(&attr);

    /* Assertions									*/
    assert(pool		!= NULL);
    assert(pool->poolPtr	!= NULL);
    assert(pool->registry	!= NULL);

    /* Return									*/
    return pool;
}

int smp_closePool(SharedMemoryPool_t **pool) {
    /* Check parameter								*/
    if ( (pool == NULL) | (*pool == NULL) ) {
        return -1;
    }

    /* Unmap the pool								*/
    if ( PLATFORM_munmap(*pool, (*pool)->total_size) == -1 ) {
        perror("munmap");
    }

    /* Return									*/
    return 0;
}

int smp_clearPool(SharedMemoryPool_t *pool) {
    /* Check parameter								*/
    if ( pool == NULL ) {
        return -1;
    }

    /* Clear the registry								*/
    PLATFORM_lockMutex(&(pool->mutex));
    memset(pool->registry, 0, pool->size / (8 * CHUNK_SIZE));
    PLATFORM_unlockMutex(&(pool->mutex));

    /* Return									*/
    return 0;
}

void* smp_calloc(SharedMemoryPool_t *pool, size_t nmemb, size_t size) {
    void *ptr;

    /* Check parameter								*/
    if ( pool == NULL ) {
        return NULL;
    }

    /* Check the requested quantity of memory					*/
    if ( (nmemb == 0) || (size == 0) ) {
        return NULL;
    }

    /* Translate into a call to smpMalloc						*/
    if ( (ptr = smp_malloc(pool, nmemb * size)) == NULL ) {
        return NULL;
    }

    /* Set obtained memory to 0 (calloc semantic)					*/
    memset(ptr, 0, nmemb * size);

    /* Return the address of the required memory area				*/
    return ptr;
}

void* smp_malloc(SharedMemoryPool_t *pool, size_t size) {
    int chunks_requested;
    int start_chunk;

    /* Check parameter								*/
    if ( pool == NULL ) {
        return NULL;
    }

    /* Check the requested quantity of memory					*/
    if ( size == 0 ) {
        return NULL;
    }

    /* Obtain the number of consecutive memory chunks required			*/
    chunks_requested = ( (size % CHUNK_SIZE) == 0 ? (size / CHUNK_SIZE) : (size / CHUNK_SIZE) + 1 );

    /* Add a guard chunk								*/
    chunks_requested++;

    /* Lock the mutex								*/
    PLATFORM_lockMutex(&(pool->mutex));

    /* Obtain the first chunk from where to start allocation			*/
    /* or a negative number if there aren't any available				*/
    if ( (start_chunk = findFreeChunks(pool, chunks_requested)) < 0 ) {
        PLATFORM_unlockMutex(&(pool->mutex));
        return NULL;
    }

    /* Allocate the chunks								*/
    allocChunks(pool, start_chunk, chunks_requested, SMP_ALLOCATION);

    /* Release the mutex								*/
    PLATFORM_unlockMutex(&(pool->mutex));

    /* Write into the guard the number of chunks that have been requested for data	*/
    *((int*)getAddress(pool->poolPtr, start_chunk)) = chunks_requested - 1;

    /* Return the address of the required memory area				*/
    return getAddress(pool->poolPtr, start_chunk + 1);
}

void* smp_realloc(SharedMemoryPool_t *pool, void *ptr, size_t size) {
    int chunks_current;
    int chunks_requested;
    int chunks_to_be_removed;
    int start_chunk;

    /* Check parameter								*/
    if ( pool == NULL ) {
        return NULL;
    }

    /* If ptr is NULL then realloc behaves like malloc				*/
    if ( ptr == NULL ) {
        return smp_malloc(pool, size);
    }

    /* If ptr is not NULL and size is 0 then realloc behaves like free		*/
    if ( size == 0 ) {
        smp_free(pool, ptr);
        return NULL;
    }

    /* Obtain the number of consecutive memory chunks required			*/
    /* including the guard chunk							*/
    chunks_requested	= ( (size % CHUNK_SIZE) == 0 ? (size / CHUNK_SIZE) : (size / CHUNK_SIZE) + 1 ) + 1;
    chunks_current		= *((int*)(ptr - CHUNK_SIZE)) + 1;

    /* No action is taken if it is not requested to work with chunks		*/
    if ( chunks_requested == chunks_current ) {
        return ptr;
    }

    /* Handle the request for new chunks						*/
    if ( chunks_requested > chunks_current ) {

        /* Check if it is possible to extend the current sequence of chunks	*/
        pool->cursor = getAbsolute(pool->poolPtr, ptr);
        PLATFORM_lockMutex(&(pool->mutex));
        if ( (start_chunk = findFreeChunks(pool, chunks_requested - chunks_current)) < 0 ) {

            /* Not enough memory available					*/
            PLATFORM_unlockMutex(&(pool->mutex));
            return NULL;
        }

        /* If the start_chunk is the first one following the previous		*/
        /* allocated area, then it can be simply extended			*/
        if ( start_chunk == getAbsolute(pool->poolPtr, ptr) + chunks_current - 1 ) {

            /* Allocate the new chunks					*/
            allocChunks(pool, start_chunk, chunks_requested - chunks_current, SMP_ALLOCATION);
            PLATFORM_unlockMutex(&(pool->mutex));
        }

        /* Else memory must be moved						*/
        else {
            /* Restart examining memory register from a suitable point	*/
            pool->cursor = start_chunk;

            /* Obtain the first chunk from where to start allocation	*/
            /* or a negative number if there aren't any available		*/
            if ( (start_chunk = findFreeChunks(pool, chunks_requested)) < 0 ) {
                return NULL;
            }

            /* Allocate the new chunks					*/
            allocChunks(pool, start_chunk, chunks_requested, SMP_ALLOCATION);

            /* Copy the memory content					*/
            memmove(getAddress(pool->poolPtr, start_chunk + 1), ptr, size);

            /* Deallocate the old chunks (including the guard)		*/
            chunks_to_be_removed = chunks_current;
            allocChunks(pool, getAbsolute(pool->poolPtr, ptr) - 1, chunks_to_be_removed, SMP_DEALLOCATION);
            PLATFORM_unlockMutex(&(pool->mutex));

            /* set ptr to be the return value				*/
            ptr = getAddress(pool->poolPtr, start_chunk + 1);
        }
    }

    /* Handle request for remove exceedind chunks					*/
    else {
        /* Deallocate exceeding chunks						*/
        PLATFORM_lockMutex(&(pool->mutex));
        allocChunks(pool, getAbsolute(pool->poolPtr, ptr) + chunks_requested - 1, chunks_current - chunks_requested, SMP_DEALLOCATION);
        PLATFORM_unlockMutex(&(pool->mutex));
    }

    /* Update the guard chunk							*/
    *((int*)(ptr - CHUNK_SIZE)) = chunks_requested - 1;

    /* Return the address of the required memory area				*/
    return ptr;
}

void smp_free(SharedMemoryPool_t *pool, void *ptr) {
    int chunks_requested;
    int start_chunk;

    /* Check parameter								*/
    if ( pool == NULL ) {
        return;
    }

    /* If ptr is NULL no action is taken (free semantic)				*/
    if ( ptr == NULL ) {
        return;
    }

    /* Obtain from the guard's chunk and the total amount of chunks to be freed	*/
    start_chunk		= getAbsolute(pool->poolPtr, ptr) - 1;
    chunks_requested	= *((int*)(ptr - CHUNK_SIZE)) + 1;

    /* Deallocate the chunks							*/
    PLATFORM_lockMutex(&(pool->mutex));
    allocChunks(pool, start_chunk, chunks_requested, SMP_DEALLOCATION);
    PLATFORM_unlockMutex(&(pool->mutex));
}

int smp_belongsToPool(SharedMemoryPool_t *pool, void *ptr) {
    if ( ((unsigned long)ptr >= (unsigned long)pool->poolPtr ) && ((unsigned long)ptr < (unsigned long)(pool->poolPtr) + (unsigned long)(pool->size)) ) {
        return SMP_BELONGS_TO_POOL;
    }

    return SMP_DONT_BELONGS_TO_POOL;
}

int findFreeChunks(SharedMemoryPool_t *pool, size_t chunks_requested) {
    int			chunks_obtained;
    int			candidate_position;
    int			current_position;

    /* Check parameter								*/
    if ( pool == NULL ) {
        return -1;
    }

    chunks_obtained		= 0;
    candidate_position	= pool->cursor;
    current_position	= pool->cursor;

    PDEBUG("SHARED MEMORY POOL: findFreeChunks(): requested %d chunks\n", chunks_requested);
    PDEBUG("SHARED MEMORY POOL: findFreeChunks(): candidate_position is %d\n", candidate_position);

    do {
        /* Check if it is possible to find enough consecutive chunks		*/
        if ( !(((pool->registry)[getIndex(current_position)] >> getOffset(current_position)) & 0x1) ) {
            chunks_obtained++;
            current_position++;
        } else {
            chunks_obtained = 0;
            candidate_position = ++current_position;
        }

        /* Handle the case with the current position over the registry boundary	*/
        if (current_position >= (pool->size / CHUNK_SIZE)) {
            current_position = 0;

            if ( chunks_obtained < chunks_requested ) {
                candidate_position = 0;
                chunks_obtained = 0;
                PDEBUG("SHARED MEMORY POOL: findFreeChunks(): arrived to the last chunk, restarting from scratch from the first\n");
            }
        }

        /* If the current position corresponds to the cursor, all registry has	*/
        /* been scanned								*/
        if ( current_position == pool->cursor ) {
            PDEBUG("SHARED MEMORY POOL: findFreeChunks(): no enough consecutive chunks\n");
            return -1;
        }
    } while (chunks_obtained < chunks_requested);

    /* Update the cursor position							*/
    pool->cursor = current_position;

    /* Return									*/
    return candidate_position;
}

void smp_printRegistry(SharedMemoryPool_t *pool) {
    int index;
    int offset;

    /* Check parameter								*/
    if ( pool == NULL ) {
        fprintf(stderr, "ERROR: pool is NULL\n");
        return;
    }

    /* Scan bit-a-bit the content of the registry					*/
    PLATFORM_lockMutex(&(pool->mutex));
    for( index = 0; index < (pool->size)/(8 * CHUNK_SIZE); index++ ) {
        /* Print the state of the single chunks:				*/
        /* 0=free, 1=occupied							*/
        for ( offset = 7; offset >= 0; offset-- ) {
            if ( ( (pool->registry)[index] >> offset) & 0x1 ) {
                printf("1");
            } else {
                printf("0");
            }

            if ( (offset % 4) == 0 ) {
                printf(" ");
            }
        }

        /* Print the address within pool					*/
        printf("\t");
        printf("0x%X", index * 8 * CHUNK_SIZE);
        printf("\n");
    }
    PLATFORM_unlockMutex(&(pool->mutex));

    /* Print the legend								*/
    printf("1 bit = %d bytes\n", CHUNK_SIZE);
}

void allocChunks(SharedMemoryPool_t *pool, int start_chunk, size_t nchunks, int operation) {
    int		index;
    int		offset;
    int		value;

    index	= getIndex(start_chunk);
    offset	= getOffset(start_chunk);

#ifdef PRINTDEBUG
    if ( operation == SMP_ALLOCATION ) {
        PDEBUG("SHARED MEMORY POOL: allocChunks(): ALLOCATION\n");
    } else {
        PDEBUG("SHARED MEMORY POOL: allocChunks(): DEALLOCATION\n");
    }
    PDEBUG("SHARED MEMORY POOL: allocChunks(): index	= %d\n", index);
    PDEBUG("SHARED MEMORY POOL: allocChunks(): offset	= %d\n", offset);
    PDEBUG("SHARED MEMORY POOL: allocChunks(): nchunks	= %d\n", nchunks);
#endif

    /* Pre-allocation / Pre-deallocation						*/
    if ( offset != 0 ) {
        int		i;
        int8_t		mask;
        mask = 0;
        for (i = offset; (i < 8) && ((i - offset) < nchunks); i++ ) {
            mask = (++mask) << 1;
        }

        mask			= ( operation == SMP_ALLOCATION ? mask << (offset - 1)			: ~(mask << (offset - 1)) );

        (pool->registry)[index]	= ( operation == SMP_ALLOCATION ? (pool->registry)[index] | mask	: (pool->registry)[index] & mask );
        nchunks -= (i - offset);
        index++;
        PDEBUG("SHARED MEMORY POOL: allocChunks(): pre-allocation mask	= 0x%X\n", mask);
    }

    /* Allocation / Deallocation							*/
    value = ( operation == SMP_ALLOCATION ? -1 : 0 );
    if ( nchunks >= 8 ) {
        memset(pool->registry + index, value, nchunks / 8);
        index	+= (nchunks / 8);
        nchunks	-= (nchunks / 8) * 8;
    }
    assert(nchunks < 8);

    /* Post-allocation / Post-deallocation						*/
    if ( nchunks != 0 ) {
        int		i;
        int8_t		mask = 0;
        for ( i = nchunks; i > 0; i-- ) {
            mask = ((i != 1) ? ((++mask) << 1) : (++mask));
        }
        mask			= ( operation == SMP_ALLOCATION ? mask					: ~mask );
        (pool->registry)[index]	= ( operation == SMP_ALLOCATION ? (pool->registry)[index] | mask	: (pool->registry)[index] & mask );
        nchunks -= nchunks;
        PDEBUG("SHARED MEMORY POOL: allocChunks(): post-allocation mask	= 0x%X\n", mask);
    }

    assert(nchunks == 0);
}

static inline int getIndex(int absolute) {
    return absolute / 8;
}

inline int getOffset(int absolute) {
    return absolute % 8;
}

inline void* getAddress(void *startAddress, int absolute) {
    return (void*)(startAddress + (absolute * CHUNK_SIZE));
}

inline int getAbsolute(void *startAddress, void *address) {
    return (address - startAddress) / CHUNK_SIZE;
}
