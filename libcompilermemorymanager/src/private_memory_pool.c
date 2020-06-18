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
#include <stdlib.h>
#include <string.h>

// My headers
#include <private_memory_pool.h>
// End

inline int8_t getBit(int8_t byte, int8_t position);
inline void setBit(int8_t *byte, int8_t position);

PrivateMemoryPool_t* pmp_openPool(int addresses, void* (*allocFunction)(size_t size), void* (*sharedAllocFunction)(size_t size)) {
    PrivateMemoryPool_t	*privateMemoryPool;
    size_t			registry_size;
    pthread_mutexattr_t	attr;

    /* Adjust registry_size to be enough to contain all the requested addresses		*/
    registry_size = ( (addresses % 8) == 0 ? (addresses / 8) : (addresses / 8) + 1 );

    /* Save the PrivateMemoryPool_t structure using the requested allocation function	*/
    if ( sharedAllocFunction != NULL ) {
        privateMemoryPool = (PrivateMemoryPool_t*) sharedAllocFunction(sizeof(PrivateMemoryPool_t));
        privateMemoryPool->registry = (int8_t*) sharedAllocFunction(registry_size);
    } else {
        privateMemoryPool = (PrivateMemoryPool_t*) malloc(sizeof(PrivateMemoryPool_t));
        privateMemoryPool->registry = (int8_t*) malloc(registry_size);
    }

    /* Prepare the registry									*/
    privateMemoryPool->registry_size = registry_size;
    memset(privateMemoryPool->registry, 0, registry_size);

    /* Save the address table using the requested (non-shared) allocation function		*/
    privateMemoryPool->table = (void**) allocFunction(sizeof(void*) * addresses);
    memset(privateMemoryPool->table, 0, sizeof(void*) * addresses);

    /* Initialize the cursor to the start position						*/
    privateMemoryPool->cursor = 0;

    /* Initialize the mutex									*/
    PLATFORM_initMutexAttr(&attr);
    PLATFORM_setMutexAttr_pshared(&attr, PTHREAD_PROCESS_SHARED);
    PLATFORM_initMutex(&(privateMemoryPool->mutex), &attr);
    PLATFORM_destroyMutexAttr(&attr);

    /* Assertions										*/
    assert(privateMemoryPool		!= NULL);
    assert(privateMemoryPool->registry	!= NULL);
    assert(privateMemoryPool->table		!= NULL);

    /* Return										*/
    return privateMemoryPool;
}

void pmp_closePool(PrivateMemoryPool_t *privateMemoryPool) {
    // TODO
    // Implement according to release address policy chosen
}

void* pmp_obtainAddress(PrivateMemoryPool_t *privateMemoryPool) {
    int32_t	old_position;

    /* Lock the mutex									*/
    PLATFORM_lockMutex(&(privateMemoryPool->mutex));

    /* Scan registry for available addresses						*/
    for ( old_position = privateMemoryPool->cursor; privateMemoryPool->cursor < privateMemoryPool->registry_size; privateMemoryPool->cursor++ ) {
        int8_t	bit;
        for ( bit = 0; bit < 8; bit++ ) {
            if ( !getBit((privateMemoryPool->registry)[privateMemoryPool->cursor], bit) ) {
                setBit(&((privateMemoryPool->registry)[privateMemoryPool->cursor]), bit);
                PLATFORM_unlockMutex(&(privateMemoryPool->mutex));
                return privateMemoryPool->table + 8 * privateMemoryPool->cursor + bit;
            }
        }
    }

    for ( privateMemoryPool->cursor = 0; privateMemoryPool->cursor < old_position; privateMemoryPool->cursor++ ) {
        int8_t	bit;
        for ( bit = 0; bit < 8; bit++ ) {
            if ( !getBit((privateMemoryPool->registry)[privateMemoryPool->cursor], bit) ) {
                setBit(&((privateMemoryPool->registry)[privateMemoryPool->cursor]), bit);
                PLATFORM_unlockMutex(&(privateMemoryPool->mutex));
                return privateMemoryPool->table + 8 * privateMemoryPool->cursor + bit;
            }
        }
    }

    /* Unlock the mutex									*/
    PLATFORM_unlockMutex(&(privateMemoryPool->mutex));

    return NULL;
}

void pmp_releaseAddress(PrivateMemoryPool_t *privateMemoryPool, void *ptr) {
    // TODO
    // Implement according to release address policy chosen (e.g.: refcount)
    //unsetBit(&((privateMemoryPool->registry)[(ptr - privateMemoryPool->table) / 8]), (ptr - privateMemoryPool->table) % 8);
    //*ptr = NULL;
}

inline int8_t getBit(int8_t byte, int8_t position) {
    if ( (position < 0) || (position > 7) ) {
        return 0;
    }
    return (byte >> position) & 0x1;
}

inline void setBit(int8_t *byte, int8_t position) {
    if ( (position < 0) || (position > 7) ) {
        return;
    }
    *byte |= (0x1 << position);
}
