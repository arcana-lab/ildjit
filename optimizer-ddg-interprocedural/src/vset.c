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
 * This file implements a set for the VLLPA algorithm.
 */

#include <vset.h>


/**
 * Create the union of two sets.  Creates a new set if the given flag is
 * set, otherwise inserts all items from the second set into the first.
 */
VSet *
vSetUnion(VSet *first, VSet *second, JITBOOLEAN newSet) {
    VSet *unionSet;
    VSetItem *item;
    assert(first);
    assert(second);

    /* Create a new set if required. */
    if (newSet) {
        unionSet = vSetClone(first);
    } else {
        unionSet = first;
    }

    /* Copy into the union set. */
    item = vSetFirst(second);
    while (item) {
        if (!vSetContains(unionSet, item->element)) {
            vSetInsert(unionSet, item->element);
        }
        item = vSetNext(second, item);
    }

    /* Return this set. */
    return unionSet;
}


/**
 * Functions for use with XanHashTableSets.
 */
#if defined(VLLPA_VSET_XANHASHTABLE)


/**
 * Create a new set.
 */
VSet *
vSetNew(void) {
    VSet *set = xanHashTable_new(16, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(set);
    return set;
}


/**
 * Add a data item to the given set.  Only adds it if the item is not
 * already there.
 */
void
vSetInsert(VSet *set, void *data) {
    assert(set);
    if (!xanHashTable_lookup(set, data)) {
        xanHashTable_insert(set, data, data);
    }
}


/**
 * Add a data item to the given set with a give key associated with it.  Only
 * adds the item if it is not already there.
 */
void
vSetInsertWithKey(VSet *set, void *key, void *data) {
    assert(set);
    if (!xanHashTable_lookup(set, key)) {
        xanHashTable_insert(set, key, data);
    }
}


/**
 * Remove a data item from the given set.
 */
void
vSetRemove(VSet *set, void *data) {
    assert(set);
    if (xanHashTable_lookup(set, data)) {
        xanHashTable_removeElement(set, data);
    }
}


#if 0
/**
 * Create the union of two sets.  Creates a new set if the given flag is
 * set, otherwise inserts all items from the second set into the first.
 */
VSet *
vSetUnion(VSet *first, VSet *second, JITBOOLEAN newSet) {
    VSet *unionSet;
    VSetItem *item;
    assert(first);
    assert(second);

    /* Create a new set if required. */
    if (newSet) {
        unionSet = vSetClone(first);
    } else {
        unionSet = first;
    }

    /* Copy into the union set. */
    item = xanHashTable_first(second);
    while (item) {
        if (!xanHashTable_lookup(unionSet, item->elementID)) {
            xanHashTable_insert(unionSet, item->elementID, item->element);
        }
        item = xanHashTable_next(second, item);
    }

    /* Return this set. */
    return unionSet;
}
#endif


/**
 * Empty a set of all items.
 */
void
vSetEmpty(VSet *set) {
    assert(set);
    xanHashTable_emptyOutTable(set);
}


/**
 * Check whether two sets are identical, meaning that they contain the
 * same data items.
 */
JITBOOLEAN
vSetAreIdentical(VSet *set1, VSet *set2) {
    VSetItem *item1;
    assert(set1);
    assert(set2);

    /* The obvious check first. */
    if (vSetSize(set1) != vSetSize(set2)) {
        return JITFALSE;
    }

    /* Check each data item. */
    item1 = xanHashTable_first(set1);
    while (item1) {
        if (!xanHashTable_lookup(set2, item1->elementID)) {
            return JITFALSE;
        }
        item1 = xanHashTable_next(set1, item1);
    }

    /* Everything found. */
    return JITTRUE;
}


/**
 * Check whether two sets share at least one data item.
 */
JITBOOLEAN
vSetShareSomeData(VSet *set1, VSet *set2) {
    VSetItem *item1;
    assert(set1);
    assert(set2);

    /* Check each data item. */
    item1 = xanHashTable_first(set1);
    while (item1) {
        if (xanHashTable_lookup(set2, item1->elementID)) {
            return JITTRUE;
        }
        item1 = xanHashTable_next(set1, item1);
    }

    /* No common data found. */
    return JITFALSE;
}


#elif defined(VLLPA_VSET_SMALLHASHTABLE)


/**
 * Remove a data item from the given set.
 */
void
vSetRemove(SmallHashTable *table, void *data) {
    if (smallHashTableContains(table, data)) {
        smallHashTableRemove(table, data);
    }
}


/**
 * Check whether two sets are identical, meaning that they contain the
 * same data items.
 */
JITBOOLEAN
vSetAreIdentical(SmallHashTable *table1, SmallHashTable *table2) {
    SmallHashTableItem *item1;
    assert(table1);
    assert(table2);

    /* The obvious check first. */
    if (table1->size != table2->size) {
        return JITFALSE;
    }

    /* Check each data item. */
    item1 = smallHashTableFirst(table1);
    while (item1) {
        if (!smallHashTableContains(table2, item1->key)) {
            return JITFALSE;
        }
        item1 = smallHashTableNext(table1, item1);
    }

    /* Everything found. */
    return JITTRUE;
}


/**
 * Check whether two sets share at least one data item.
 */
JITBOOLEAN
vSetShareSomeData(SmallHashTable *table1, SmallHashTable *table2) {
    SmallHashTableItem *item1;
    assert(table1);
    assert(table2);

    /* Check each data item. */
    item1 = smallHashTableFirst(table1);
    while (item1) {
        if (smallHashTableContains(table2, item1->key)) {
            return JITTRUE;
        }
        item1 = smallHashTableNext(table1, item1);
    }

    /* No common data found. */
    return JITFALSE;
}


#endif  /* ifdef VLLPA_VSET_XANHASHTABLE */
