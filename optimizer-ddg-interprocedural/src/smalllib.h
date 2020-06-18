/*
 * Copyright (C)  2010 Simone Campanoni
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

#ifndef SMALL_LIB_H
#define SMALL_LIB_H

#include <platform_API.h>
#include <stddef.h>
#include <stdbool.h>

#include <xanlib.h>

/**
 * @brief An element within a hash table.
 */
typedef struct SmallHashTableItem {
    unsigned int bin;
    void *key;
    void *element;
    struct SmallHashTableItem *next;
} SmallHashTableItem;

/**
 * @brief A space-efficient hash table.
 */
typedef struct SmallHashTable {
    SmallHashTableItem **elements;
    unsigned int length;
    unsigned int size;
} SmallHashTable;

void smallHashTableInit(void);
void smallHashTableFinish(void);
SmallHashTable * smallHashTableNew(unsigned int length);
void smallHashTableInsert(SmallHashTable *table, void *key, void *element);
bool smallHashTableUniqueInsert(SmallHashTable *table, void *key, void *element);
void smallHashTableReplace(SmallHashTable *table, void *key, void *element);
bool smallHashTableContains(SmallHashTable *table, void *key);
void * smallHashTableLookup(SmallHashTable *table, void *key);
SmallHashTableItem * smallHashTableLookupItem(SmallHashTable *table, void *key);
void smallHashTableRemove(SmallHashTable *table, void *key);
void smallHashTableRemoveItem(SmallHashTable *table, SmallHashTableItem *item);
void smallHashTableDestroy(SmallHashTable *table);
int smallHashTableSize(SmallHashTable *table);
SmallHashTableItem * smallHashTableFirst(SmallHashTable *table);
SmallHashTableItem * smallHashTableNext(SmallHashTable *table, SmallHashTableItem *item);
SmallHashTable * smallHashTableClone(SmallHashTable *table);
XanList * smallHashTableToKeyList(SmallHashTable *table);
XanList * smallHashTableToDataList(SmallHashTable *table);
void ** smallHashTableToKeyArray(SmallHashTable *table);
void ** smallHashTableToDataArray(SmallHashTable *table);
void smallHashTableEmpty(SmallHashTable *table);

#ifdef MEMUSE_DEBUG
void smallHashTablePrintMemUse(void);
#else
#define smallHashTablePrintMemUse()
#endif

#endif  /* ifndef SMALL_LIB_H */
