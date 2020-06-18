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
 * @file private_memory_pool.h
 */
#ifndef PRIVATE_MEMORY_POOL_H
#define PRIVATE_MEMORY_POOL_H

#include <platform_API.h>
#include <stdlib.h>

typedef struct {
    int8_t		*registry;
    int32_t		registry_size;
    void		**table;
    int32_t		cursor;
    pthread_mutex_t	mutex;
} PrivateMemoryPool_t;

PrivateMemoryPool_t* pmp_openPool(int addresses, void* (*allocFunction)(size_t size), void* (*sharedAllocFunction)(size_t size));
void pmp_closePool(PrivateMemoryPool_t *privateMemoryPool);
void* pmp_obtainAddress(PrivateMemoryPool_t *privateMemoryPool);
void pmp_releaseAddress(PrivateMemoryPool_t *privateMemoryPool, void *ptr);


int PMP_openPool(int addresses, void* (*allocFunction)(size_t size), void* (*sharedAllocFunction)(size_t size));
void PMP_closePool();
void* PMP_obtainAddress();
void PMP_releaseAddress(void *ptr);

#endif
