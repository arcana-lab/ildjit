/*
 * Copyright (C) 2009 - 2011 Timothy M Jones, Simone Campanoni
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
 * This file implements a set for the VLLPA algorithm.  For now this is a
 * XanHashTable, but because the details are abstracted away, it should be
 * easy to change in the future, if necessary.
 */

#ifndef VLLPA_VSET_H
#define VLLPA_VSET_H

#include "compiler_memory_manager.h"

//#define VLLPA_VSET_XANHASHTABLE
#define VLLPA_VSET_SMALLHASHTABLE

#if defined(VLLPA_VSET_XANHASHTABLE)

#include "xanlib.h"

/**
 * Typedefs for VSet and VSetItem.
 */
typedef XanHashTable VSet;
typedef XanHashTableItem VSetItem;


/**
 * Get the data out of a set item.  When debugging, this provides an
 * assertion on the data before returning it.
 */
#define getSetItemData(item) (assert(item && item->element), item->element)


/**
 * Get a key from a set item.
 */
#define vSetItemKey(item) (assert(item), item->elementID)


/**
 * Create a new set.
 */
VSet *
vSetNew(void);


/**
 * Destroy the given set.
 */
#define vSetDestroy(set) (assert(set), xanHashTable_destroyTable(set))


/**
 * Get the first item in the given set.
 */
#define vSetFirst(set) (assert(set), xanHashTable_first(set))


/**
 * Get the next item in the given set following the given item.
 */
#define vSetNext(set, item) (assert(set), xanHashTable_next(set, item))


/**
 * Add a data item to the given set.  Only adds it if the item is not
 * already there.
 */
void
vSetInsert(VSet *set, void *data);


/**
 * Add a data item to the given set with a give key associated with it.  Only
 * adds the item if it is not already there.
 */
void
vSetInsertWithKey(VSet *set, void *key, void *data);


/**
 * Remove a data item from the given set.
 */
void
vSetRemove(VSet *set, void *data);


/**
 * Check whether a data item is already in the given set.
 */
#define vSetContains(set, data) (assert(set), xanHashTable_lookup(set, data) != NULL)


/**
 * Extract a data item from a set when held via a key.
 */
#define vSetFindWithKey(set, key) (assert(set), xanHashTable_lookup(set, key))


/**
 * Find the container holding a data item.
 */
#define vSetFindItem(set, data) (assert(set), xanHashTable_lookupItem(set, data))


/**
 * Create the union of two sets.  Creates a new set if the given flag is
 * set, otherwise inserts all items from the second set into the first.
 */
VSet *
vSetUnion(VSet *first, VSet *second, JITBOOLEAN newSet);


/**
 * Empty a set of all items.
 */
void
vSetEmpty(VSet *set);


/**
 * Get the number of items in the given set.
 */
#define vSetSize(set) (assert(set), xanHashTable_elementsInside(set))


/**
 * Remove duplicates from the given set.
 */
#define vSetDeleteClones(set)


/**
 * Check whether two sets are identical, meaning that they contain the
 * same data items.
 */
JITBOOLEAN
vSetAreIdentical(VSet *set1, VSet *set2);


/**
 * Check whether two sets share at least one data item.
 */
JITBOOLEAN
vSetShareSomeData(VSet *set1, VSet *set2);


/**
 * Create a XanList from the given set.
 */
#define vSetToList(set) (assert(set), xanHashTable_toList(set))


/**
 * Create an array from the given set.
 */
#define vSetToDataArray(set) xanHashTable_toDataArray(set)


/**
 * Clone a set.
 */
#define vSetClone(set) (assert(set), xanHashTable_cloneTable(set))


#elif defined(VLLPA_VSET_SMALLHASHTABLE)


#include "smalllib.h"

/**
 * Typedefs for VSet and VSetItem.
 */
typedef SmallHashTable VSet;
typedef SmallHashTableItem VSetItem;


/**
 * Get the data out of a set item.  When debugging, this provides an
 * assertion on the data before returning it.
 */
#define getSetItemData(item) (assert(item), item->element)


/**
 * Get a key from a set item.
 */
#define vSetItemKey(item) ((item)->key)


/**
 * Get data from a set item.
 */
#define vSetItemData(item) ((item)->element)


/**
 * Create a new set.
 */
#define vSetNew(void) smallHashTableNew(3)


/**
 * Destroy the given set.
 */
#define vSetDestroy(set) smallHashTableDestroy(set)


/**
 * Get the first item in the given set.
 */
#define vSetFirst(set) smallHashTableFirst(set)


/**
 * Get the next item in the given set following the given item.
 */
#define vSetNext(set, item) smallHashTableNext(set, item)


/**
 * Add a data item to the given set.  Only adds it if the item is not
 * already there.
 */
#define vSetInsert(set, data) smallHashTableUniqueInsert(set, data, data)


/**
 * Add a data item to the given set with a given key associated with it.  Only
 * adds the item if it is not already there.
 */
#define vSetInsertWithKey(set, key, data) smallHashTableUniqueInsert(set, key, data)


/**
 * Remove a data item from the given set.
 */
void vSetRemove(VSet *set, void *data);


/**
 * Remove a container holding an item from the given set.
 */
#define vSetRemoveItem(set, item) smallHashTableRemoveItem(set, item)


/**
 * Remove a data item from the given set using the given key associated with it.
 */
#define vSetRemoveWithKey(set, key) vSetRemove(set, key)


/**
 * Check whether a data item is already in the given set.
 */
#define vSetContains(set, data) smallHashTableContains(set, data)


/**
 * Extract a data item from a set when held via a key.
 */
#define vSetFindWithKey(set, key) smallHashTableLookup(set, key)


/**
 * Find the container holding a data item.
 */
#define vSetFindItem(set, data) smallHashTableLookupItem(set, data)


/**
 * Find the container holding a data item when held with a key.
 */
#define vSetFindItemWithKey(set, key) smallHashTableLookupItem(set, key)


/**
 * Replace a data item already in the given set with the given key associated
 * with it.
 */
#define vSetReplaceWithKey(set, key, data) smallHashTableReplace(set, key, data)


/**
 * Create the union of two sets.  Creates a new set if the given flag is
 * set, otherwise inserts all items from the second set into the first.
 */
VSet *
vSetUnion(VSet *first, VSet *second, JITBOOLEAN newSet);


/**
 * Empty a set of all items.
 */
#define vSetEmpty(set) smallHashTableEmpty(set)


/**
 * Get the number of items in the given set.
 */
#define vSetSize(set) smallHashTableSize(set)


/**
 * Remove duplicates from the given set.
 */
#define vSetDeleteClones(set)


/**
 * Check whether two sets are identical, meaning that they contain the
 * same data items.
 */
JITBOOLEAN
vSetAreIdentical(VSet *set1, VSet *set2);


/**
 * Check whether two sets share at least one data item.
 */
JITBOOLEAN
vSetShareSomeData(VSet *set1, VSet *set2);


/**
 * Create a XanList from the given set.
 */
#define vSetToList(set) smallHashTableToDataList(set)


/**
 * Create an array from the given set.
 */
#define vSetToDataArray(set) smallHashTableToDataArray(set)


/**
 * Clone a set.
 */
#define vSetClone(set) smallHashTableClone(set)


#endif  /* ifdef VLLPA_VSET_XANHASHTABLE */
#endif  /* ifdef VLLPA_VSET_H */
