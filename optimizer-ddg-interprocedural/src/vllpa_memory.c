/*
 * Copyright (C) 2009 - 2012 Timothy M Jones, Simone Campanoni
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
 * This file implements memory management for the interprocedural pointer
 * analysis, including ways of detecting memory leaks, if necessary.
 */

#include <compiler_memory_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <xanlib.h>

// My headers
#include "optimizer_ddg.h"
#include "vllpa_memory.h"
// End


/**
 * A list of VSets that are free.  These sets are frequently used and having
 * this list saves the time spent initialising them and freeing their memory
 * again once finished.
 */
static XanList *freeSetList;


/**
 * For debugging memory leaks.
 */
#ifdef MEMUSE_DEBUG
static JITUINT32 setsAllocated = 0, setsFree = 0, maxSetsAllocated = 0;
static JITUINT32 hashTablesAllocated = 0, maxHashTablesAllocated = 0;
static JITUINT32 listsAllocated = 0, maxListsAllocated = 0;
static JITUINT32 bitSetsAllocated = 0, maxBitSetsAllocated = 0;
static JITINT64 totalMemAllocated = 0, maxTotalMemAllocated = 0;
static JITINT64 *memAllocatedByType;
static JITUINT32 allocTypes;
static SmallHashTable *memory;
static SmallHashTable *nonFreedSets;
static JITINT64 *setsAllocatedByType;
static JITUINT32 setAllocTypes;

typedef struct {
    size_t size;
    JITUINT32 type;
} mem_alloc_t;

void *
vllpaAllocFunction(size_t size, JITUINT32 type) {
    void *mem = allocFunction(size);
    mem_alloc_t *allocation = allocFunction(sizeof(mem_alloc_t));
    totalMemAllocated += size;
    if (type >= allocTypes) {
        JITUINT32 i, oldTypes = allocTypes;
        JITINT64 *newAllocs;
        allocTypes = type + 1;
        newAllocs = allocFunction(allocTypes * sizeof(JITINT64));
        for (i = 0; i < allocTypes; ++i) {
            if (i < oldTypes) {
                newAllocs[i] = memAllocatedByType[i];
            } else {
                newAllocs[i] = 0;
            }
        }
        freeFunction(memAllocatedByType);
        memAllocatedByType = newAllocs;
    }
    memAllocatedByType[type] += size;
    maxTotalMemAllocated = MAX(totalMemAllocated, maxTotalMemAllocated);
    assert(mem);
    allocation->size = size;
    allocation->type = type;
    smallHashTableInsert(memory, mem, allocation);
    /* fprintf(stderr, "Allocated mem at %p, size %u, total %8llu\n", mem, size, totalMemAllocated); */
    return mem;
}
void
vllpaFreeFunction(void *mem, size_t size) {
    mem_alloc_t *allocation;
    assert(mem);
    freeFunction(mem);
    allocation = (mem_alloc_t *)smallHashTableLookup(memory, mem);
    totalMemAllocated -= size;
    assert(totalMemAllocated >= 0);
    assert(allocation->size == size);
    memAllocatedByType[allocation->type] -= size;
    freeFunction(allocation);
    smallHashTableRemove(memory, mem);
    /* fprintf(stderr, "Freed mem at %p, size %u, total %8llu\n", mem, size, totalMemAllocated); */
}
void
vllpaFreeFunctionUnallocated(void *mem) {
    assert(mem);
    freeFunction(mem);
}
void
vllpaFreeFunctionNoSize(void *mem) {
    mem_alloc_t *allocation;
    assert(mem);
    freeFunction(mem);
    allocation = (mem_alloc_t *)smallHashTableLookup(memory, mem);
    totalMemAllocated -= allocation->size;
    assert(totalMemAllocated >= 0);
    memAllocatedByType[allocation->type] -= allocation->size;
    freeFunction(allocation);
    smallHashTableRemove(memory, mem);
    /* fprintf(stderr, "Freed mem at %p, no size, total %8llu\n", mem, totalMemAllocated); */
}
void
vllpaWillNotFreeFunction(void *mem, size_t size) {
    mem_alloc_t *allocation;
    assert(mem);
    allocation = (mem_alloc_t *)smallHashTableLookup(memory, mem);
    totalMemAllocated -= size;
    assert(totalMemAllocated >= 0);
    assert(allocation->size == size);
    memAllocatedByType[allocation->type] -= size;
    freeFunction(allocation);
    smallHashTableRemove(memory, mem);
    /* fprintf(stderr, "Will not free mem at %p, size %u, total %8llu\n", mem, size, totalMemAllocated); */
}
void
vllpaPrintSetsAllocated(void) {
    if (smallHashTableSize(nonFreedSets) > 0) {
        SmallHashTableItem *item = smallHashTableFirst(nonFreedSets);
        fprintf(stderr, "Still %d sets not freed\n", smallHashTableSize(nonFreedSets));
        while (item) {
            fprintf(stderr, "Still allocated set %p type %u\n", item->key, fromPtr(JITUINT32, item->element));
            item = smallHashTableNext(nonFreedSets, item);
        }
    }
}
void
vllpaPrintMemAllocated(void) {
    if (smallHashTableSize(memory) > 0) {
        SmallHashTableItem *item = smallHashTableFirst(memory);
        fprintf(stderr, "Still %d memory elements allocated\n", smallHashTableSize(memory));
        while (item) {
            mem_alloc_t *allocation = (mem_alloc_t *)item->element;
            fprintf(stderr, "Still allocated %p, size %u\n", item->key, allocation->size);
            item = smallHashTableNext(memory, item);
        }
    }
}
void
vllpaPrintMemAllocTypes(void) {
    JITUINT32 i;
    for (i = 0; i < allocTypes; ++i) {
        fprintf(stderr, "Type %2u: %8llu\n", i, memAllocatedByType[i]);
    }
}
void
vllpaPrintSetAllocTypes(void) {
    JITUINT32 i;
    for (i = 0; i < setAllocTypes; ++i) {
        if (setsAllocatedByType[i] != 0) {
            fprintf(stderr, "Set type %2u: %lld\n", i, setsAllocatedByType[i]);
        }
    }
}
void
vllpaPrintMemUse(void) {
    /* Directly print, since PRINT_DEBUG might not be enabled. */
    fprintf(stderr, "Sets:            %8u : %10llu\tMax: %8u : %10llu\n", setsAllocated, (JITUINT64)setsAllocated * sizeof(VSet), maxSetsAllocated, (JITUINT64)maxSetsAllocated * sizeof(VSet));
    fprintf(stderr, "Sets free:       %8u : %10llu\n", setsFree, (JITUINT64)setsFree * sizeof(VSet));
    fprintf(stderr, "Lists:           %8u : %10llu\tMax: %8u : %10llu\n", listsAllocated, (JITUINT64)listsAllocated * sizeof(XanList), maxListsAllocated, (JITUINT64)maxListsAllocated * sizeof(XanList));
    fprintf(stderr, "Hash Tables:     %8u : %10llu\tMax: %8u : %10llu\n", hashTablesAllocated, (JITUINT64)hashTablesAllocated * sizeof(SmallHashTable), maxHashTablesAllocated, (JITUINT64)maxHashTablesAllocated * sizeof(SmallHashTable));
    fprintf(stderr, "Bit Sets:        %8u : %10llu\tMax: %8u : %10llu\n", bitSetsAllocated, (JITUINT64)bitSetsAllocated * sizeof(XanBitSet), maxBitSetsAllocated, (JITUINT64)maxBitSetsAllocated * sizeof(XanBitSet));
    fprintf(stderr, "Total Mem:          %18llu\tMax: %21llu\n", totalMemAllocated, maxTotalMemAllocated);
    vllpaPrintMemAllocTypes();
    /* malloc_stats(); */
}
#endif  /* ifdef MEMUSE_DEBUG */


/**
 * Initialise memory for this analysis.
 */
void
vllpaInitMemory(void) {
    /* Must initialise all memory first. */
#ifdef MEMUSE_DEBUG
    memory = smallHashTableNew(29);
    nonFreedSets = smallHashTableNew(29);
    memAllocatedByType = allocFunction(1);
    setsAllocatedByType = allocFunction(1);
    allocTypes = 0;
    setAllocTypes = 0;
#endif

    /* Create global list of free VSets. */
    freeSetList = allocateNewList();
}


/**
 * Actually free a set.
 */
static void
actuallyFreeSet(VSet *set) {
    vSetDestroy(set);
#ifdef MEMUSE_DEBUG
    assert(setsAllocated != 0);
    setsAllocated -= 1;
    totalMemAllocated -= sizeof(VSet);
    assert(totalMemAllocated >= 0);
#ifdef DEBUG
    size_t allocated = (size_t)(JITNUINT)smallHashTableLookup(memory, set);
    assert(allocated == sizeof(VSet));
#endif
    smallHashTableRemove(memory, set);
#endif
}


/**
 * Empty a list of free sets.
 */
static void
emptyFreeSetList(XanList *setList) {
    XanListItem *listItem = xanList_first(setList);
    while (listItem) {
        VSet *set = listItem->data;
        actuallyFreeSet(set);
#ifdef MEMUSE_DEBUG
        assert(setsFree != 0);
        setsFree -= 1;
#endif
        listItem = xanList_next(listItem);
    }
    xanList_emptyOutList(setList);
}


/**
 * Free up memory used by this analysis.
 */
void
vllpaFinishMemory(void) {

    /* Early checks. */
#ifdef MEMUSE_DEBUG
    assert(setsFree == setsAllocated);
    assert(smallHashTableSize(nonFreedSets) == 0);
#endif

    /* Empty the list of free sets. */
    emptyFreeSetList(freeSetList);
    freeList(freeSetList);

    /* Make sure everything got deallocated. */
#ifdef MEMUSE_DEBUG
    assert(setsFree == 0);
    assert(setsAllocated == 0);
    assert(listsAllocated == 0);
    assert(hashTablesAllocated == 0);
    assert(totalMemAllocated == 0);
    smallHashTableDestroy(memory);
    smallHashTableDestroy(nonFreedSets);
    freeFunction(memAllocatedByType);
    freeFunction(setsAllocatedByType);
#endif
}


/**
 * Allocate a new set.  When debugging memory usage this increments the
 * global count of sets allocated.
 */
VSet *
allocateNewSet(JITUINT32 type) {
    VSet *set;
    if (xanList_length(freeSetList) > 0) {
        XanListItem *listItem = xanList_first(freeSetList);
        set = listItem->data;
        xanList_deleteItem(freeSetList, listItem);
#ifdef MEMUSE_DEBUG
        assert(setsFree != 0);
        setsFree -= 1;
        smallHashTableInsert(nonFreedSets, set, toPtr(type));
        /* fprintf(stderr, "Reused set %p, (%d free)\n", set, setsFree); */
#endif
    } else {
        set = vSetNew();
#ifdef MEMUSE_DEBUG
        setsAllocated += 1;
        maxSetsAllocated = MAX(setsAllocated, maxSetsAllocated);
        totalMemAllocated += sizeof(VSet);
        maxTotalMemAllocated = MAX(totalMemAllocated, maxTotalMemAllocated);
        smallHashTableInsert(memory, set, (void *)(JITNUINT)sizeof(VSet));
        smallHashTableInsert(nonFreedSets, set, toPtr(type));
        /* fprintf(stderr, "Allocated new set %p\n", set); */
#endif
    }
    assert(vSetSize(set) == 0);
#ifdef MEMUSE_DEBUG
    if (type >= setAllocTypes) {
        JITUINT32 i, oldTypes = setAllocTypes;
        JITINT64 *newAllocs;
        setAllocTypes = type + 1;
        newAllocs = allocFunction(setAllocTypes * sizeof(JITINT64));
        for (i = 0; i < setAllocTypes; ++i) {
            if (i < oldTypes) {
                newAllocs[i] = setsAllocatedByType[i];
            } else {
                newAllocs[i] = 0;
            }
        }
        freeFunction(setsAllocatedByType);
        setsAllocatedByType = newAllocs;
    }
    setsAllocatedByType[type] += 1;
#endif
    return set;
}


/**
 * Allocate a new list.  When debugging memory usage this increments the
 * global count of lists allocated.
 */
XanList *
allocateNewList(void) {
    XanList *list = xanList_new(allocFunction, freeFunction, NULL);
#ifdef MEMUSE_DEBUG
    listsAllocated += 1;
    maxListsAllocated = MAX(listsAllocated, maxListsAllocated);
    totalMemAllocated += sizeof(XanList);
    maxTotalMemAllocated = MAX(totalMemAllocated, maxTotalMemAllocated);
    smallHashTableInsert(memory, list, (void *)(JITNUINT)sizeof(XanList));
    /*   fprintf(stderr, "Allocated list at %p, size %u, total %8llu\n", list, sizeof(XanList), totalMemAllocated); */
#endif
    return list;
}


/**
 * Allocate a new hash table.  When debugging memory usage this increments
 * the global count of hash tables allocated.  With normal debugging it
 * provides an assertion on the memory allocated.
 */
SmallHashTable *
allocateNewHashTable(void) {
    SmallHashTable *table = smallHashTableNew(3);
    assert(table);
#ifdef MEMUSE_DEBUG
    hashTablesAllocated += 1;
    maxHashTablesAllocated = MAX(hashTablesAllocated, maxHashTablesAllocated);
    totalMemAllocated += sizeof(SmallHashTable);
    maxTotalMemAllocated = MAX(totalMemAllocated, maxTotalMemAllocated);
    smallHashTableInsert(memory, table, (void *)(JITNUINT)sizeof(SmallHashTable));
    /*     fprintf(stderr, "Allocated hash table at %p, size %u, total %8llu\n", table, sizeof(SmallHashTable), totalMemAllocated); */
#endif
    return table;
}


/**
 * Allocate a new bit set.  When debugging memory usage this increments the
 * global count of bit sets allocated.
 */
XanBitSet *
allocateNewBitSet(JITUINT32 length) {
    XanBitSet *set = xanBitSet_new(length);
#ifdef MEMUSE_DEBUG
    bitSetsAllocated += 1;
    bitSetsAllocated = MAX(bitSetsAllocated, maxBitSetsAllocated);
    totalMemAllocated += sizeof(XanBitSet);
    maxTotalMemAllocated = MAX(totalMemAllocated, maxTotalMemAllocated);
    smallHashTableInsert(memory, set, (void *)(JITNUINT)sizeof(XanBitSet));
    /*   fprintf(stderr, "Allocated bit set at %p, size %u, total %8llu\n", set, sizeof(XanBitSet), totalMemAllocated); */
#endif
    return set;
}


/**
 * Allocate a new item holder, but don't set any flags.
 */
vllpa_item_holder_t *
allocateNewItemHolder(void) {
    vllpa_item_holder_t *holder = vllpaAllocFunction(sizeof(vllpa_item_holder_t), 26);
    return holder;
}


/**
 * Clone a set.  When debugging memory usage this increments the global count
 * of sets allocated.
 */
VSet *
vllpaCloneSet(VSet *set) {
    VSet *clone = allocateNewSet(14);
    VSetItem *setItem = vSetFirst(set);
    while (setItem) {
        void *key = vSetItemKey(setItem);
        void *data = getSetItemData(setItem);
        vSetInsertWithKey(clone, key, data);
        setItem = vSetNext(set, setItem);
    }
    return clone;
}


/**
 * Clone a bit set.  When debugging memory usage this increments the global
 * count of bit sets allocated.
 */
XanBitSet *
cloneBitSet(XanBitSet *set) {
    XanBitSet *clone = xanBitSet_clone(set);
#ifdef MEMUSE_DEBUG
    bitSetsAllocated += 1;
    bitSetsAllocated = MAX(bitSetsAllocated, maxBitSetsAllocated);
    totalMemAllocated += sizeof(XanBitSet);
    maxTotalMemAllocated = MAX(totalMemAllocated, maxTotalMemAllocated);
    smallHashTableInsert(memory, clone, (void *)(JITNUINT)sizeof(XanBitSet));
#endif
    return clone;
}


/**
 * Free a set.  When debugging memory usage this decrements the global
 * count of sets allocated.
 */
void
freeSet(VSet *set) {
    if (xanList_length(freeSetList) < MAX_FREE_SETS) {
        vSetEmpty(set);
        assert(vSetSize(set) == 0);
        xanList_append(freeSetList, set);
#ifdef MEMUSE_DEBUG
        setsFree += 1;
#endif
        /* fprintf(stderr, "Freed set %p (%d free)\n", set, setsFree); */
    } else {
        actuallyFreeSet(set);
    }
#ifdef MEMUSE_DEBUG
    assert(smallHashTableContains(nonFreedSets, set));
    assert(setsAllocatedByType[fromPtr(JITUINT32, smallHashTableLookup(nonFreedSets, set))] > 0);
    setsAllocatedByType[fromPtr(JITUINT32, smallHashTableLookup(nonFreedSets, set))] -= 1;
    smallHashTableRemove(nonFreedSets, set);
#endif
}


/**
 * Free a list.  When debugging memory usage this decrements the global
 * count of lists allocated.
 */
void
freeList(XanList *list) {
    assert(list);
    xanList_destroyList(list);
#ifdef MEMUSE_DEBUG
    assert(listsAllocated != 0);
    listsAllocated -= 1;
    totalMemAllocated -= sizeof(XanList);
    assert(totalMemAllocated >= 0);
#ifdef DEBUG
    size_t allocated = (size_t)(JITNUINT)smallHashTableLookup(memory, list);
    assert(allocated == sizeof(XanList));
#endif
    smallHashTableRemove(memory, list);
#endif
}


/**
 * Free a list that this pass didn't allocate.
 */
void
freeListUnallocated(XanList *list) {
    assert(list);
    xanList_destroyList(list);
}


/**
 * Mark a list that this pass has allocated as not going to be freed.
 */
void
willNotFreeList(XanList *list) {
#ifdef MEMUSE_DEBUG
    assert(listsAllocated != 0);
    listsAllocated -= 1;
    totalMemAllocated -= sizeof(XanList);
    assert(totalMemAllocated >= 0);
#ifdef DEBUG
    size_t allocated = (size_t)(JITNUINT)smallHashTableLookup(memory, list);
    assert(allocated == sizeof(XanList));
#endif
    smallHashTableRemove(memory, list);
#endif
}


/**
 * Free a hash table.
 */
void
freeHashTable(SmallHashTable *table) {
    assert(table);
    smallHashTableDestroy(table);
#ifdef MEMUSE_DEBUG
    assert(hashTablesAllocated != 0);
    hashTablesAllocated -= 1;
    totalMemAllocated -= sizeof(SmallHashTable);
    assert(totalMemAllocated >= 0);
#ifdef DEBUG
    size_t allocated = (size_t)(JITNUINT)smallHashTableLookup(memory, table);
    assert(allocated == sizeof(SmallHashTable));
#endif
    smallHashTableRemove(memory, table);
#endif
}


/**
 * Free a bit set.  When debugging memory usage this decrements the global
 * count of bit sets allocated.
 */
void
freeBitSet(XanBitSet *set) {
    assert(set);
    xanBitSet_free(set);
#ifdef MEMUSE_DEBUG
    assert(bitSetsAllocated != 0);
    bitSetsAllocated -= 1;
    totalMemAllocated -= sizeof(XanBitSet);
    assert(totalMemAllocated >= 0);
#ifdef DEBUG
    size_t allocated = (size_t)(JITNUINT)smallHashTableLookup(memory, set);
    assert(allocated == sizeof(XanBitSet));
#endif
    smallHashTableRemove(memory, set);
#endif
}


/**
 * Free an item holder.
 */
void
freeSetHolder(vllpa_item_holder_t *holder) {
    if (vllpaItemHolderIsSet(holder)) {
        freeSet(holder->value.set);
    } else if (vllpaItemHolderIsList(holder)) {
        freeList(holder->value.list);
    } else if (vllpaItemHolderIsBitSet(holder)) {
        freeBitSet(holder->value.bitset);
    }
    vllpaFreeFunction(holder, sizeof(vllpa_item_holder_t));
}


