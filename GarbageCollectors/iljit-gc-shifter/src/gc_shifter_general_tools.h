/*
 * Copyright (C) 2007  Campanoni Simone
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
#ifndef GC_SHIFTER_GENERAL_TOOLS_H
#define GC_SHIFTER_GENERAL_TOOLS_H

#include <jitsystem.h>
#include <xanlib.h>
#include <garbage_collector.h>

// My headers
#include <memory_manager.h>
// End

void printLiveObjects (XanList *objectsLived, t_gc_behavior *gc);
void printLiveObject (void *obj, t_gc_behavior *gc);
void printShallowLiveObjects (XanList *objectsLived, t_gc_behavior *gc);
void printRootSets(XanList *rootSets, t_memory *memory);
JITINT16 isAnObjectAllocated(t_memory *memory, void *obj, JITUINT32 *objectID);
JITINT16 isMarkedByteOfHeap (t_memory *memory, JITINT32 byteNumber);
void dumpHeap(t_memory *memory);

#endif
