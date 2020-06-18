/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
#include <errno.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>

// My headers
#include <gc_root_sets.h>
#include <iljit-utils.h>
// End

#define ROOT_SETS_ALLOCATION_MEMORY_STEP        400

static inline JITINT16 shutdown_sets (t_root_sets *rootSets);
static inline void addNewRootSetSlot (t_root_sets *rootSets, void **varPointer);
static inline void addNewRootSet (t_root_sets *rootSets);
static inline void _popLastRootSet (t_root_sets *rootSets);
static inline void check_memory_for_one_slot (t_root_sets *rootSets);
static inline void ** internal_getRootSetSlot (t_root_sets *rootSets, JITUINT32 *slotID);
static inline JITUINT32 getRootSetsSize (t_root_sets *rootSets);
static inline JITBOOLEAN rootSetIsInRootSets (t_root_sets *rootSets, void *object);
static inline void internal_lockRootSet (t_root_sets *rootSets);
static inline void internal_unlockRootSet (t_root_sets *rootSets);

void init_root_sets (t_root_sets *rootSets) {

    /* Assertions		*/
    assert(rootSets != NULL);

    /* Init the field	*/
    memset(rootSets, 0, sizeof(t_root_sets));

    /* Init the rootSets		*/
    rootSets->shutdown = shutdown_sets;
    rootSets->addNewRootSetSlot = addNewRootSetSlot;
    rootSets->addNewRootSet = addNewRootSet;
    rootSets->popLastRootSet = _popLastRootSet;
    rootSets->getRootSetSlot = internal_getRootSetSlot;
    rootSets->length = getRootSetsSize;
    rootSets->isInRootSets = rootSetIsInRootSets;
    rootSets->lock = internal_lockRootSet;
    rootSets->unlock = internal_unlockRootSet;
    pthread_mutexattr_t mutex_attr;
    PLATFORM_initMutexAttr(&mutex_attr);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(rootSets->mutex), &mutex_attr);
}

static inline void addNewRootSetSlot (t_root_sets *rootSets, void **varPointer) {

    /* Assertions			*/
    assert(rootSets != NULL);
    assert(varPointer != NULL);
    assert(rootSets->top <= rootSets->allocated);

    /* Check if the memory is       *
     * enought			*/
    check_memory_for_one_slot(rootSets);
    assert(rootSets->top < rootSets->allocated);

    /* Add the new pointer of the   *
     * variable which can store an	*
     * object reference		*/
    rootSets->rootSets[rootSets->top] = varPointer;
    (rootSets->top)++;
    assert(rootSets->top <= rootSets->allocated);

    /* Return			*/
    return;
}

static inline void addNewRootSet (t_root_sets *rootSets) {

    /* Assertions		*/
    assert(rootSets != NULL);
    assert(rootSets->top <= rootSets->allocated);

    /* Check if the memory is       *
     * enought			*/
    check_memory_for_one_slot(rootSets);
    assert(rootSets->top < rootSets->allocated);

    /* Add a NULL to the top of the	*
     * root sets, to set that a new	*
     * root set is beginning	*/
    rootSets->rootSets[rootSets->top] = NULL;
    (rootSets->top)++;

    /* Return			*/
    return;
}

static inline void _popLastRootSet (t_root_sets *rootSets) {
    JITINT32 count;

    /* Assertions					*/
#ifdef DEBUG
    assert(rootSets != NULL);
    //  assert(rootSets->top > 0);
    assert(rootSets->top <= rootSets->allocated);
    assert(rootSets->rootSets != NULL);
    if (rootSets->top > 0) {
        assert((rootSets->rootSets[rootSets->top - 1]) != NULL);
    }

#endif

    /* Find the first NULL into the root sets from  *
     * the top of the stack to the bottom of it	*/
    count = 0;
    if (rootSets->top > 0) {
        for (count = (rootSets->top - 1); count >= 0; count--) {
            if (rootSets->rootSets[count] == NULL) {
                break;
            }
        }
    }

    /* Set the top of the stack                     */
    assert(rootSets->top <= rootSets->allocated);
    rootSets->top = count;
    assert(rootSets->top <= rootSets->allocated);

    /* Return					*/
    return;
}

static inline void check_memory_for_one_slot (t_root_sets *rootSets) {

    /* Assertions		*/
    assert(rootSets != NULL);

    if (rootSets->top == rootSets->allocated) {
        JITNUINT newSize;

        /* Compute the size of the new	*
         * root sets			*/
        newSize = (rootSets->allocated + ROOT_SETS_ALLOCATION_MEMORY_STEP) * sizeof(void *);
        assert(newSize > 0);
        rootSets->rootSets = reallocMemory(rootSets->rootSets, newSize);
        rootSets->allocated += ROOT_SETS_ALLOCATION_MEMORY_STEP;
        rootSets->_size = newSize;
        assert(rootSets->rootSets != NULL);
    }
    assert(rootSets->top < rootSets->allocated);
}

static inline JITINT16 shutdown_sets (t_root_sets *rootSets) {

    /* Assertions			*/
    assert(rootSets != NULL);

    /* Free the root set memory	*/
    if (rootSets->rootSets != NULL) {
        assert(rootSets->allocated > 0);
        freeMemory(rootSets->rootSets);
        rootSets->rootSets = NULL;
    }
    rootSets->top = 0;
    rootSets->allocated = 0;

    /* Return			*/
    return 0;
}

static inline void ** internal_getRootSetSlot (t_root_sets *rootSets, JITUINT32 *slotID) {
    void **slot;

    /* Assertions				*/
    assert(rootSets != NULL);
    assert(slotID != NULL);

    /* Initialize the variables		*/
    slot = NULL;

    /* Check if the slotID is correct	*/
    if (rootSets->top <= (*slotID)) {
        return NULL;
    }

    /* Fetch the slot			*/
    slot = rootSets->rootSets[(*slotID)];
    if (slot == NULL) {
        /* We are in a slot between two	*
         * different root set		*/
        if (rootSets->top <= ((*slotID) + 1)) {
            return NULL;
        }

        /* Fetch the next slot		*/
        (*slotID)++;
        slot = rootSets->rootSets[(*slotID)];
    }
    assert(slot != NULL);

    /* Return				*/
    return slot;
}

static inline JITUINT32 getRootSetsSize (t_root_sets *rootSets) {

    /* Assertions		*/
    assert(rootSets != NULL);

    return rootSets->top;
}

static inline void internal_lockRootSet (t_root_sets *rootSets) {

    /* Assertions		*/
    assert(rootSets != NULL);

    PLATFORM_lockMutex(&(rootSets->mutex));
}

static inline void internal_unlockRootSet (t_root_sets *rootSets) {

    /* Assertions		*/
    assert(rootSets != NULL);

    PLATFORM_unlockMutex(&(rootSets->mutex));
}

static inline JITBOOLEAN rootSetIsInRootSets (t_root_sets *rootSets, void *object) {
    JITUINT32 count;
    void            *current_object;
    void            **slot;

    /* Assertions			*/
    assert(rootSets != NULL);
    assert(object != NULL);

    /* Initialize the variables	*/
    current_object = NULL;
    slot = NULL;
    count = 0;

    slot = rootSets->getRootSetSlot(rootSets, &count);
    count++;
    while (slot != NULL) {
        /* Fetch the object stored inside the   *
         * slot					*/
        current_object = (*slot);
        if (    (current_object == NULL)        &&
                (current_object == object)      ) {
            return 1;
        }

        /* Fetch the next slot			*/
        slot = rootSets->getRootSetSlot(rootSets, &count);
        count++;
    }

    /* Return			*/
    return 0;
}
