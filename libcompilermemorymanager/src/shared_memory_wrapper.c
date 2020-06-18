/*
 * Copyright (C) 2009 - 2011 Campanoni Simone
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

// My headers
#include <shared_memory_pool.h>
// End

static SharedMemoryPool_t *pool = NULL;

int SMP_openPool(size_t size) {
    /* Remove an eventual already allocated pool		*/
    if ( pool != NULL ) {
        smp_closePool(&pool);
    }

    /* Allocate the new pool				*/
    if ( (pool = smp_openPool(size)) == NULL ) {
        return -1;
    }

    /* Return						*/
    return 0;
}

int SMP_closePool() {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return -1;
    }

    return smp_closePool(&pool);
}

int SMP_clearPool(SharedMemoryPool_t *pool) {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return -1;
    }

    return smp_clearPool(pool);
}

void SMP_printRegistry() {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return;
    }

    smp_printRegistry(pool);
}

void* SMP_calloc(size_t nmemb, size_t size) {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return NULL;
    }

    return smp_calloc(pool, nmemb, size);
}

void* SMP_malloc(size_t size) {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return NULL;
    }

    return smp_malloc(pool, size);
}

void* SMP_realloc(void *ptr, size_t size) {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return NULL;
    }

    return smp_realloc(pool, ptr, size);
}

void SMP_free(void *ptr) {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return;
    }

    /* smp_free() does not return anything			*/
    smp_free(pool, ptr);
}

int SMP_belongsToPool(void *ptr) {
    /* Check static parameter				*/
    if ( pool == NULL ) {
        return SMP_DONT_BELONGS_TO_POOL;
    }

    return smp_belongsToPool(pool, ptr);
}
