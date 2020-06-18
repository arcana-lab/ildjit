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
 * @file shared_memory_pool.h
 */
#ifndef SHARED_MEMORY_POOL_H
#define SHARED_MEMORY_POOL_H

#include <platform_API.h>
#include <stdlib.h>

#define SMP_DEALLOCATION			0
#define SMP_ALLOCATION				1

#define SMP_DONT_BELONGS_TO_POOL		0
#define SMP_BELONGS_TO_POOL			1

#define CHUNK_SIZE				8

typedef struct SharedMemoryPool_t {
    void		*poolPtr;
    size_t		size;
    size_t		total_size;
    int8_t		*registry;
    int		cursor;
    pthread_mutex_t	mutex;
} SharedMemoryPool_t;

SharedMemoryPool_t* smp_openPool(size_t size);
int smp_closePool(SharedMemoryPool_t **pool);
int smp_clearPool(SharedMemoryPool_t *pool);
void smp_printRegistry(SharedMemoryPool_t *pool);
void* smp_calloc(SharedMemoryPool_t *pool, size_t nmemb, size_t size);
void* smp_malloc(SharedMemoryPool_t *pool, size_t size);
void* smp_realloc(SharedMemoryPool_t *pool, void *ptr, size_t size);
void smp_free(SharedMemoryPool_t *pool, void *ptr);
int smp_belongsToPool(SharedMemoryPool_t *pool, void *ptr);

int SMP_openPool(size_t size);
int SMP_closePool();
int SMP_clearPool();
void SMP_printRegistry();
void* SMP_calloc(size_t nmemb, size_t size);
void* SMP_malloc(size_t size);
void* SMP_realloc(void *ptr, size_t size);
void SMP_free(void *ptr);
int SMP_belongsToPool(void *ptr);

#endif
