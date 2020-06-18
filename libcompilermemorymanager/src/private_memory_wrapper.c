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

// My headers
#include <private_memory_pool.h>
// End

static PrivateMemoryPool_t *privatePool = NULL;

int PMP_openPool(int addresses, void* (*allocFunction)(size_t size), void* (*sharedAllocFunction)(size_t size)) {
    /* Remove an eventual already allocated pool	*/
    if ( privatePool != NULL ) {
        pmp_closePool(privatePool);
    }

    /* Allocate the new pool			*/
    if ( (privatePool = pmp_openPool(addresses, allocFunction, sharedAllocFunction) ) == NULL ) {
        return -1;
    }

    return 0;
}

void PMP_closePool() {
}

void* PMP_obtainAddress() {
    return pmp_obtainAddress(privatePool);
}

void PMP_releaseAddress(void *ptr) {
}
