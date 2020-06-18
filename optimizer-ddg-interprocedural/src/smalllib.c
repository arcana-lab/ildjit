/*
 * Copyright (C) 2007 - 2010 Campanoni Simone, Luca Rocchini, Michele Tartara
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

/**
 * Assertions have to be explicitly enabled for this file to avoid excessive
 * slowdowns that come from checking things on each access.
 */
#ifdef SMALLLIB_DEBUG
#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif
#else  /* ifdef SMALLLIB_DEBUG */
#ifdef DEBUG
#undef DEBUG
#endif
#ifndef NDEBUG
#define NDEBUG
#endif
#endif  /* ifdef SMALLLIB_DEBUG */
#include <assert.h>

#include <xanlib.h>
#include <platform_API.h>
#include <compiler_memory_manager.h>

/* My headers	*/
#include <smalllib.h>
#include <config.h>


#ifdef PRINTDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, "SMALL_LIB: " fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif  /* ifdef PRINTDEBUG */


/* Primes we can use as sizes for the small hash table. */
static const size_t primes[] = {
    3,	   7,	       13,	    29,
    53,	   97,	       193,	    389,
    769,	   1543,       3079,	    6151,
    12289,	   24593,      49157,	    98317,
    196613,	   393241,     786433,	    1572869,
    3145739,   6291469,    12582917,    25165843,
    50331653,  100663319,  201326611,   402653189,
    805306457, 1610612741
};
#define PRIMES_TABLE_LENGTH (sizeof(primes)/sizeof(primes[0]))
#define SMALLLIB_MAX_LOAD_FACTOR 0.75

/* A cache of hash table items to avoid constant allocation and freeing. */
static SmallHashTableItem **itemCache = NULL;
static unsigned int itemsCached;
#define SMALLLIB_MAX_ITEMS_CACHED 1000

/* For debugging memory usage. */
#ifdef MEMUSE_DEBUG
static unsigned int tablesAllocated = 0;
static unsigned int itemsAllocated = 0;

void
smallHashTablePrintMemUse(void) {
    /* Directly print, since PRINT_DEBUG might not be enabled. */
    fprintf(stderr, "Smalllib tables: %8u : %10llu\n", tablesAllocated, (long long unsigned int)tablesAllocated * sizeof(SmallHashTable));
    fprintf(stderr, "Smalllib items:  %8u : %10llu\n", itemsAllocated, (long long unsigned int)itemsAllocated * sizeof(SmallHashTableItem));
}
#endif  /* ifdef MEMUSE_DEBUG */

static unsigned int
smallLibImproveHash(void *element) {
    unsigned int i = (unsigned int)element;
    i += ~(i << 9);
    i ^= ((i >> 14) | (i << 18));
    i += (i << 4);
    i ^= ((i >> 10) | (i << 22));
    return i;
}

static size_t
smallHashTableFindPrime(size_t len) {
    unsigned int count;
    for (count = 0; count < PRIMES_TABLE_LENGTH; count++) {
        if (primes[count] >= len) {
            return primes[count];
        }
    }
    abort();
}

static SmallHashTableItem *
smallHashTableAllocItem(void) {
    if (itemsCached > 0) {
        itemsCached -= 1;
        return itemCache[itemsCached];
    }
#ifdef MEMUSE_DEBUG
    itemsAllocated += 1;
#endif
    return allocFunction(sizeof(SmallHashTableItem));
}

static void
smallHashTableFreeItem(SmallHashTableItem *item) {
    if (itemsCached < SMALLLIB_MAX_ITEMS_CACHED) {
        itemCache[itemsCached] = item;
        itemsCached += 1;
        return;
    }
    freeFunction(item);
#ifdef MEMUSE_DEBUG
    assert(itemsAllocated > 0);
    itemsAllocated -= 1;
#endif
}

static void
smallHashTableExpand(SmallHashTable* table) {
    SmallHashTableItem **newTable;
    SmallHashTableItem *bucket;
    size_t newLength;
    size_t i;
    size_t newIndex;
    size_t elementsCount;

    /* Alloc a new empty table */
    newLength = smallHashTableFindPrime(table->length + 1);
    assert(newLength > 0);
    newTable = (SmallHashTableItem**) allocFunction(sizeof(SmallHashTableItem *) * newLength);
    assert(newTable != NULL);
    memset(newTable, 0, sizeof(SmallHashTableItem *) * newLength);
    elementsCount = 0;

    /* Rehash the table */
    for (i = 0; i < table->length; i++) {
        bucket = table->elements[i];

        /* Insert the element to the new table	*/
        while (bucket != NULL) {
            SmallHashTableItem *item;
            SmallHashTableItem *next;
            newIndex = smallLibImproveHash(bucket->key) % newLength;

            item = smallHashTableAllocItem();
            assert(item != NULL);
            item->bin = newIndex;
            item->key = bucket->key;
            item->element = bucket->element;
            item->next = NULL;
            if (newTable[newIndex] == NULL) {
                newTable[newIndex] = item;
                elementsCount++;
            } else {
                SmallHashTableItem *newBucket = newTable[newIndex];
                while (newBucket->next!=NULL) {
                    newBucket = newBucket->next;
                }
                newBucket->next = item;
            }

            next = bucket->next;
            smallHashTableFreeItem(bucket);
            bucket = next;
        }
    }

    /* Update table statistics */
    freeFunction(table->elements);
    table->elements = newTable;
    table->length = newLength;
}

void
smallHashTableInit(void) {
    itemCache = allocFunction(SMALLLIB_MAX_ITEMS_CACHED * sizeof(SmallHashTableItem *));
    assert(itemCache);
    itemsCached = 0;
}

void
smallHashTableFinish(void) {
    unsigned int i;
    for (i = 0; i < itemsCached; ++i) {
        freeFunction(itemCache[i]);
#ifdef MEMUSE_DEBUG
        assert(itemsAllocated > 0);
        itemsAllocated -= 1;
#endif
    }
    freeFunction(itemCache);
}

SmallHashTable *
smallHashTableNew(unsigned int length) {
    SmallHashTable *table = allocFunction(sizeof(SmallHashTable));
    assert(table != NULL);
    table->length = smallHashTableFindPrime(length);
    table->elements = allocFunction(sizeof(SmallHashTableItem *) * table->length);
    assert(table->elements != NULL);
    table->size = 0;
#ifdef MEMUSE_DEBUG
    tablesAllocated += 1;
#endif
    return table;
}

static void
smallHashTableInsertAtPosition(SmallHashTable *table, void *key, void *element, unsigned int position) {
    SmallHashTableItem *item;

    /* Whether the load factor is too high. */
    if ((1.0 * table->size / table->length) >= SMALLLIB_MAX_LOAD_FACTOR) {
        smallHashTableExpand(table);
        position = smallLibImproveHash(key);
        position = position % (table->length);
    }

    /* Insert the new element. */
    item = smallHashTableAllocItem();
    assert(item != NULL);
    item->bin = position;
    item->key = key;
    item->element = element;
    item->next = table->elements[position];
    table->elements[position] = item;
    table->size++;
}

void
smallHashTableInsert(SmallHashTable *table, void *key, void *element) {
    unsigned int position;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);
    assert(!smallHashTableContains(table, key));

    /* Add the new element */
    position = smallLibImproveHash(key);
    position = position % (table->length);
    smallHashTableInsertAtPosition(table, key, element, position);

    /* Sanity check. */
    assert(smallHashTableContains(table, key));
}

bool
smallHashTableUniqueInsert(SmallHashTable *table, void *key, void *element) {
    unsigned int position;
    SmallHashTableItem *bucket;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* See if this element is in the table. */
    position = smallLibImproveHash(key);
    position %= (table->length);
    bucket = table->elements[position];
    while (bucket) {
        if (bucket->key == key) {
            return false;
        }
        bucket = bucket->next;
    }

    /* Add the new element */
    smallHashTableInsertAtPosition(table, key, element, position);

    /* Sanity check. */
    assert(smallHashTableContains(table, key));
    return true;
}

void
smallHashTableReplace(SmallHashTable *table, void *key, void *element) {
    unsigned int position;
    SmallHashTableItem *bucket;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);
    assert(smallHashTableContains(table, key));

    /* Find the element and replace */
    position = smallLibImproveHash(key);
    position %= (table->length);
    bucket = table->elements[position];
    while (bucket) {
        if (bucket->key == key) {
            bucket->element = element;
            return;
        }
        bucket = bucket->next;
    }
}

bool
smallHashTableContains(SmallHashTable *table, void *key) {
    SmallHashTableItem *bucket;
    unsigned int position;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* See if this element is in the table. */
    position = smallLibImproveHash(key);
    position %= (table->length);
    bucket = table->elements[position];
    while (bucket) {
        if (bucket->key == key) {
            return true;
        }
        bucket = bucket->next;
    }
    return false;
}

void *
smallHashTableLookup(SmallHashTable *table, void *key) {
    SmallHashTableItem *bucket;
    unsigned int position;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* See if this element is in the table. */
    position = smallLibImproveHash(key);
    position %= (table->length);
    bucket = table->elements[position];
    while (bucket) {
        if (bucket->key == key) {
            return bucket->element;
        }
        bucket = bucket->next;
    }
    return NULL;
}

SmallHashTableItem *
smallHashTableLookupItem(SmallHashTable *table, void *key) {
    SmallHashTableItem *bucket;
    unsigned int position;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* See if this element is in the table. */
    position = smallLibImproveHash(key);
    position %= (table->length);
    bucket = table->elements[position];
    while (bucket) {
        if (bucket->key == key) {
            return bucket;
        }
        bucket = bucket->next;
    }
    return NULL;
}

void
smallHashTableRemove(SmallHashTable *table, void *key) {
    SmallHashTableItem* bucket;
    SmallHashTableItem* prev;
    unsigned int position;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);
    assert(smallHashTableContains(table, key));

    /* Initialize the variables	*/
    prev = NULL;

    /* Find this element. */
    position = smallLibImproveHash(key);
    position %= (table->length);
    bucket = table->elements[position];
    while (bucket) {
        if (bucket->key == key) {
            if (prev == NULL) {
                table->elements[position] = bucket->next;
            } else {
                prev->next = bucket->next;
            }
            smallHashTableFreeItem(bucket);
            table->size--;
            return;
        }
        prev = bucket;
        bucket = bucket->next;
    }

    /* Shouldn't get here. */
    abort();
}

void
smallHashTableRemoveItem(SmallHashTable *table, SmallHashTableItem *item) {
    SmallHashTableItem* bucket;
    SmallHashTableItem* prev;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);
    assert(item != NULL);
    assert(smallHashTableContains(table, item->key));

    /* Initialize the variables	*/
    prev = NULL;

    /* Find this element. */
    bucket = table->elements[item->bin];
    while (bucket) {
        if (bucket == item) {
            if (prev == NULL) {
                table->elements[item->bin] = bucket->next;
            } else {
                prev->next = bucket->next;
            }
            smallHashTableFreeItem(bucket);
            table->size--;
            return;
        }
        prev = bucket;
        bucket = bucket->next;
    }

    /* Shouldn't get here. */
    abort();
}

void
smallHashTableDestroy(SmallHashTable *table) {
    unsigned int count;
    SmallHashTableItem *bucket;
    SmallHashTableItem *next;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* Delete the table		*/
    for (count = 0; count < table->length; count++) {
        bucket = table->elements[count];
        while (bucket!=NULL) {
            next = bucket->next;
            smallHashTableFreeItem(bucket);
            bucket = next;
        }
    }
    freeFunction(table->elements);
    freeFunction(table);
#ifdef MEMUSE_DEBUG
    assert(tablesAllocated > 0);
    tablesAllocated -= 1;
#endif
}

int
smallHashTableSize(SmallHashTable *table) {
    assert(table != NULL);
    return table->size;
}

SmallHashTableItem *
smallHashTableFirst(SmallHashTable* table) {
    unsigned int count;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* Empty table. */
    if (table->size == 0) {
        return NULL;
    }

    /* Find the first element. */
    for (count = 0; count < table->length; count++) {
        if (table->elements[count] != NULL) {
            return table->elements[count];
        }
    }

    /* Never get here. */
    abort();
    return NULL;
}

SmallHashTableItem *
smallHashTableNext(SmallHashTable *table, SmallHashTableItem *item) {
    unsigned int count;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->size > 0);
    assert(table->elements != NULL);
    assert(item != NULL);
    assert(smallHashTableContains(table, item->key));

    /* Another in the same chain. */
    if (item->next != NULL) {
        return item->next;
    }

    /* In a different bucket. */
    for (count = item->bin + 1; count < table->length; count++) {
        if (table->elements[count] != NULL) {
            return table->elements[count];
        }
    }

    /* No items left. */
    return NULL;
}


SmallHashTable *
smallHashTableClone(SmallHashTable *table) {
    SmallHashTable *newTable;
    unsigned int count;

    /* Assertions			*/
    assert(table != NULL);

    /* Clone the table.		*/
    newTable = smallHashTableNew(table->length);
    assert(newTable != NULL);
    for (count = 0; count < table->length; count++) {
        SmallHashTableItem *bucket = table->elements[count];
        while (bucket != NULL) {
            smallHashTableInsert(newTable, bucket->key, bucket->element);
            bucket = bucket->next;
        }
    }
    assert(smallHashTableSize(newTable) == smallHashTableSize(table));
    return newTable;
}


XanList *
smallHashTableToKeyList(SmallHashTable *table) {
    SmallHashTableItem *bucket;
    XanList *list;
    unsigned int count;

    /* Assertions			*/
    assert(table != NULL);

    /* Make a new list		*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Fill the list		*/
    for (count = 0; count < table->length; count++) {
        bucket = table->elements[count];
        while (bucket!=NULL) {
            xanList_insert(list, bucket->key);
            bucket = bucket->next;
        }
    }

    /* Return the list		*/
    return list;
}


XanList *
smallHashTableToDataList(SmallHashTable *table) {
    SmallHashTableItem *bucket;
    XanList *list;
    unsigned int count;

    /* Assertions			*/
    assert(table != NULL);

    /* Make a new list		*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Fill the list		*/
    for (count = 0; count < table->length; count++) {
        bucket = table->elements[count];
        while (bucket!=NULL) {
            xanList_insert(list, bucket->element);
            bucket = bucket->next;
        }
    }

    /* Return the list		*/
    return list;
}


void **
smallHashTableToKeyArray(SmallHashTable *table) {
    SmallHashTableItem *bucket;
    void **array;
    unsigned int count;
    unsigned int index;

    /* Assertions			*/
    assert(table != NULL);

    /* Make a new array		*/
    array = allocFunction(sizeof(void *) * table->size);
    assert(array != NULL);

    /* Fill the array		*/
    index = 0;
    for (count = 0; count < table->length; count++) {
        bucket = table->elements[count];
        while (bucket!=NULL) {
            assert(index < table->size);
            array[index] = bucket->key;
            bucket = bucket->next;
            index += 1;
        }
    }

    /* Return the array		*/
    return array;
}


void **
smallHashTableToDataArray(SmallHashTable *table) {
    SmallHashTableItem *bucket;
    void **array;
    unsigned int count;
    unsigned int index;

    /* Assertions			*/
    assert(table != NULL);

    /* Make a new array		*/
    array = allocFunction(sizeof(void *) * table->size);
    assert(array != NULL);

    /* Fill the array		*/
    index = 0;
    for (count = 0; count < table->length; count++) {
        bucket = table->elements[count];
        while (bucket!=NULL) {
            assert(index < table->size);
            array[index] = bucket->element;
            bucket = bucket->next;
            index += 1;
        }
    }

    /* Return the array		*/
    return array;
}


void
smallHashTableEmpty(SmallHashTable *table) {
    unsigned int count;
    SmallHashTableItem *bucket;
    SmallHashTableItem *next;

    /* Assertions			*/
    assert(table != NULL);
    assert(table->length > 0);
    assert(table->elements != NULL);

    /* Empty the table		*/
    for (count = 0; count < table->length; count++) {
        bucket = table->elements[count];
        if (bucket != NULL) {
            while (bucket != NULL) {
                next = bucket->next;
                smallHashTableFreeItem(bucket);
                bucket = next;
            }
            table->elements[count] = NULL;
        }
    }

    /* Table now empty		*/
    table->size = 0;
}
