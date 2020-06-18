/*
 * Copyright (C) 2008 - 2012 Timothy M. Jones, Simone Campanoni
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

#ifndef VLLPA_MEMORY_H
#define VLLPA_MEMORY_H

#include <compiler_memory_manager.h>
#include <xanlib.h>

#include "vllpa_macros.h"
#include "vset.h"

/**
 * Many structures take up a fair amount of memory which isn't required when
 * they only hold a single item.  This structure optimises for that case by
 * only allocating the structure when more than one item needs storing.
 */
typedef struct vllpa_item_holder_t {
    JITUINT8 flags;
    union {
        void *single;
        VSet *set;
        XanList *list;
        XanBitSet *bitset;
    } value;
} vllpa_item_holder_t;

#define VLLPA_HOLDER_SINGLE            0x00
#define VLLPA_HOLDER_SET               0x01
#define VLLPA_HOLDER_LIST              0x10
#define VLLPA_HOLDER_BITSET            0x11
#define vllpaItemHolderType(h)         (((h)->flags) & 0x11)
#define vllpaItemHolderIsSingle(h)     ((((h)->flags) & 0x11) == VLLPA_HOLDER_SINGLE)
#define vllpaItemHolderIsSet(h)        ((((h)->flags) & 0x11) == VLLPA_HOLDER_SET)
#define vllpaItemHolderIsList(h)       ((((h)->flags) & 0x11) == VLLPA_HOLDER_LIST)
#define vllpaItemHolderIsBitSet(h)     ((((h)->flags) & 0x11) == VLLPA_HOLDER_BITSET)
#define vllpaItemHolderSetSingle(h)    (((h)->flags) &= (~0x11))
#define vllpaItemHolderSetSet(h)       (((h)->flags) = (((h)->flags) & (~0x11)) | 0x01)
#define vllpaItemHolderSetList(h)      (((h)->flags) = (((h)->flags) & (~0x11)) | 0x10)
#define vllpaItemHolderSetBitSet(h)    (((h)->flags) |= 0x11)


#ifdef MEMUSE_DEBUG
void * vllpaAllocFunction(size_t size, JITUINT32 type);
void vllpaFreeFunction(void *mem, size_t size);
void vllpaFreeFunctionNoSize(void *mem);
void vllpaFreeFunctionUnallocated(void *mem);
void vllpaWillNotFreeFunction(void *mem, size_t size);
void vllpaPrintMemAllocTypes(void);
void vllpaPrintMemAllocated(void);
void vllpaPrintMemUse(void);
#else
#define vllpaAllocFunction(size, type) allocFunction(size)
#define vllpaFreeFunction(mem, size) freeFunction(mem)
#define vllpaFreeFunctionNoSize(mem) freeFunction(mem)
#define vllpaFreeFunctionUnallocated(mem) freeFunction(mem)
#define vllpaWillNotFreeFunction(mem, size)
#define vllpaPrintMemUse()
#define vllpaPrintMemAllocated()
#define vllpaPrintMemAllocTypes()
#endif  /* ifdef MEMUSE_DEBUG */


/**
 * Initialise memory for this analysis.
 */
void vllpaInitMemory(void);

/**
 * Free up memory used by this analysis.
 */
void vllpaFinishMemory(void);

/**
 * Allocate a new set.
 */
VSet * allocateNewSet(JITUINT32 type);

/**
 * Allocate a new list.
 */
XanList * allocateNewList(void);

/**
 * Allocate a new hash table.
 */
SmallHashTable * allocateNewHashTable(void);

/**
 * Allocate a new bit set.
 */
XanBitSet * allocateNewBitSet(JITUINT32 length);

/**
 * Allocate a new item holder, but don't set any flags.
 */
vllpa_item_holder_t * allocateNewItemHolder(void);

/**
 * Clone a set.
 */
VSet * vllpaCloneSet(VSet *set);

/**
 * Clone a bit set.
 */
XanBitSet * cloneBitSet(XanBitSet *set);

/**
 * Free a set.
 */
void freeSet(VSet *set);

/**
 * Free a list.
 */
void freeList(XanList *list);

/**
 * Free a list that this pass didn't allocate.
 */
void freeListUnallocated(XanList *list);

/**
 * Mark a list that this pass has allocated as not going to be freed.
 */
void willNotFreeList(XanList *list);

/**
 * Free a hash table.
 */
void freeHashTable(SmallHashTable *table);

/**
 * Free a bit set.
 */
void freeBitSet(XanBitSet *set);

/**
 * Free an item holder.
 */
void freeItemHolder(vllpa_item_holder_t *holder);

#endif  /* VLLPA_MEMORY_H */
