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
#include <stdio.h>
#include <assert.h>
#include <xanlib.h>
#include <garbage_collector.h>

// My headers
#include <gc_shifter_general_tools.h>
#include <iljit_gc_shifter.h>
// End

void printRootSets(XanList *rootSets, t_memory *memory){
	XanListItem	*item;
	void	 	**objPtrPtr;

	/* Assertions			*/
	assert(rootSets != NULL);
	assert(memory != NULL);
	PDEBUG("GC Shifter: printRootSets: Start\n");

	item	= rootSets->first(rootSets);
	while (item != NULL){
		objPtrPtr	= rootSets->data(rootSets, item);
		assert(objPtrPtr != NULL);
		PDEBUG("GC Shifter: printRootSets: 	%u\n", (JITUINT32)(JITNUINT)((*objPtrPtr) - memory->bottom));
		item		= rootSets->next(rootSets, item);
	}
	
	/* Return			*/
	PDEBUG("GC Shifter: printRootSets: Exit\n");
	return;
}

void printShallowLiveObjects (XanList *objectsLived, t_gc_behavior *gc){
	XanListItem	*item;
	void		*obj;

	/* Assertions					*/
	assert(objectsLived != NULL);
	assert(gc != NULL);
	PDEBUG("GC Shifter: printShallowLiveObjects: Start\n");

	/* Print the objects live			*/
	item	= objectsLived->first(objectsLived);
	while (item != NULL){
		obj	= objectsLived->data(objectsLived, item);
		assert(obj != NULL);
		PDEBUG("GC Shifter: printShallowLiveObjects: 	Object	= %u\n", (JITUINT32) (JITNUINT) (obj - memory.bottom));
		item	= objectsLived->next(objectsLived, item);
	}
	
	/* Return					*/
	PDEBUG("GC Shifter: printShallowLiveObjects: Exit\n");
	return ;
}

void printLiveObjects (XanList *objectsLived, t_gc_behavior *gc){
	XanListItem	*item;
	void		*obj;

	/* Assertions					*/
	assert(objectsLived != NULL);
	assert(gc != NULL);
	PDEBUG("GC Shifter: printLiveObjects: Start\n");

	/* Dump the heap			*/
	PDEBUG("GC Shifter: printLiveObjects: 	Dump the heap\n");
	dumpHeap(&memory);

	/* Print the objects live			*/
	item	= objectsLived->first(objectsLived);
	while (item != NULL){
		obj	= objectsLived->data(objectsLived, item);
		assert(obj != NULL);
		printLiveObject (obj, gc);
		item	= objectsLived->next(objectsLived, item);
	}
	
	/* Return					*/
	PDEBUG("GC Shifter: printLiveObjects: Exit\n");
	return ;
}

JITINT16 isAnObjectAllocated(t_memory *memory, void *obj, JITUINT32 *objectID){
	JITUINT32	count;

	/* Assertions			*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert(objectID != NULL);
	PDEBUG("GC Shifter: isAnObjectAllocated: Start\n");

	/* Check if the obj is NULL	*/
	if (obj == NULL) {
		PDEBUG("GC Shifter: isAnObjectAllocated: 	The object is NULL\n");
		PDEBUG("GC Shifter: isAnObjectAllocated: Exit\n");
		return 0;
	}
	PDEBUG("GC Shifter: isAnObjectAllocated: 	The object is not NULL\n");

	/* Check if the obj points into *
	 * the heap			*/
	if (	(obj < (memory->bottom))	||
		(obj >= (memory->top))		){
		PDEBUG("GC Shifter: isAnObjectAllocated: 	The object does not point into the heap	(Heap = %p	<->	%p)\n", memory->bottom, memory->top);
		PDEBUG("GC Shifter: isAnObjectAllocated: Exit\n");
		return 0;
	}
	PDEBUG("GC Shifter: isAnObjectAllocated: 	The object point into the heap\n");

	/* Check if the obj is inside 	*
	 * the objects references of the*
	 * memory			*/
	for (count=0; count < (memory->objectsReference).top; count++){
		if ((memory->objectsReference).objects[count] == obj){
			PDEBUG("GC Shifter: isAnObjectAllocated: 	The object has been allocated\n");
			PDEBUG("GC Shifter: isAnObjectAllocated: Exit\n");
			(*objectID)	= count;
			return 1;
		}
	}
	PDEBUG("GC Shifter: isAnObjectAllocated: 	The object has been not allocated\n");

	/* The object reference is not 	*
	 * allocated			*/
	PDEBUG("GC Shifter: isAnObjectAllocated: Exit\n");
	return 0;
}

JITINT16 isMarkedByteOfHeap (t_memory *memory, JITINT32 byteNumber){
	JITNUINT	flag;
	JITINT32	count;
	JITINT32	count2;

	/* Assertions				*/
	assert(memory != NULL);
	assert(byteNumber >= 0);
	assert(byteNumber <= ((JITINT32)(memory->top - memory->bottom)));

	count	= byteNumber / (sizeof(JITNUINT) * 8);
	assert(count >= 0);
	assert(count < memory->heapOccupanceWords);
	count2	= byteNumber - (count * (sizeof(JITNUINT) * 8));
	assert(count2 >= 0);
	assert(count2 < (sizeof(JITNUINT) * 8));
	assert(byteNumber == ((count * sizeof(JITNUINT) * 8) + count2));
	flag	= 0x1;
	flag	= flag << count2;
	if ((memory->heapOccupance[count] & flag) != 0x0){
		return 1;
	}
	return 0;
}

void printLiveObject (void *obj, t_gc_behavior *gc){
	XanList		*references;
	XanListItem	*item2;
	void		**objRef;

	/* Assertions				*/
	assert(obj != NULL);
	assert(gc != NULL);
	PDEBUG("GC Shifter: printLiveObject: Start\n");
	
	/* Print the object			*/
	PDEBUG("GC Shifter: printLiveObject: 	Object	= %u\n", (JITUINT32) (JITNUINT) (obj - memory.bottom));
	references	= gc->getReferencedObject(obj);
	assert(references != NULL);
	item2	= references->first(references);
	while (item2 != NULL){
		objRef	= (void **) references->data(references, item2);
		assert(objRef != NULL);
		PDEBUG("GC Shifter: printLiveObject: 		Object referenced[%u]	= %u\n", (JITUINT32)(JITNUINT)(((void *)objRef) - memory.bottom), (JITUINT32) (JITNUINT) ((*objRef) - memory.bottom));
		item2	= references->next(references, item2);
	}
	references->destroyList(references);

	/* Return				*/
	PDEBUG("GC Shifter: printLiveObject: Exit\n");
	return;
}

void dumpHeap(t_memory *memory){
	JITUINT32	count;

	/* Assertions			*/
	assert(memory != NULL);
	PDEBUG("GC Shifter: dumpHeap: Start\n");

	for (count=0; count < (memory->top - memory->bottom); count++){
		if (count == memory->heap - memory->bottom){
			PDEBUG("GC Shifter: dumpHeap: 	[%u]	0x%X	[HEAP]\n", count, *((JITUINT8 *)(memory->bottom + count)));
		} else {
			PDEBUG("GC Shifter: dumpHeap: 	[%u]	0x%X\n", count, *((JITUINT8 *)(memory->bottom + count)));
		}
	}
	
	/* Return			*/
	PDEBUG("GC Shifter: dumpHeap: Exit\n");
	return;
}
