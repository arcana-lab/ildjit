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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// My headers
#include <memory_manager.h>
#include <sweeper.h>
#include <iljit_gc_shifter.h>
#include <iljit-utils.h>
#include <internal_memory_manager.h>
#include <gc_shifter_general_tools.h>
#include <time.h>
// End

#define MEMORY_FREEBYTES(memory) 		(((JITNUINT)memory->top) - ((JITNUINT)memory->heap))
#define INITIAL_OBJECTS_REFERENCE_SIZE(arg)	arg
#define INITIAL_MEMORY_BLOCK_SIZE(arg)		arg
#define INITIAL_RAWMEMORY_SIZE(arg)		arg
#define INITIAL_FREE_RAWMEMORY_SIZE(arg)	arg
#define OBJECTS_SIZE_STEP			500

JITINT16 memory_shutdown (t_memory *memory);
void * memory_fetchFreeMemory (t_memory *memory, JITNUINT size, t_gc_behavior *gc);
void memory_addObjectReference (t_memory *memory, void *newObject);
void memory_collect(t_memory *memory, t_gc_behavior *gc);
JITUINT32 markMemoryByObject (t_memory *memory, void *obj, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets, XanList *objectsLived);
void printBitmap (char *prefix, JITNUINT *bitmap, JITUINT32 size);
JITUINT32 mark_the_heap(t_memory *memory, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets, XanList *objectsLived);
JITBOOLEAN isMarked (JITNUINT *objectsReferenceMark, JITUINT32 objectID);
void markObject(JITNUINT *objectsReferenceMark, JITUINT32 objectID);
int internal_isInRootSets(XanList *rootSets, void *obj);
JITUINT32 internal_freeMemorySize (t_memory *memory);
void * internal_cloneLiveObjects (void *object);
JITINT16 internal_areLiveObjectsCorrupted(XanList *objectsLived, XanList *oldObjectsLived);

JITINT16 init_memory(t_memory *memory, JITUINT32 size){
	JITINT32	sizeRequest;
	JITINT32	heapOccupanceSize;

	/* Assertions		*/
	assert(memory != NULL);
	assert((size % getpagesize()) == 0);
	PDEBUG("GC Shifter: init_memory: Start\n");
	
	/* Alloc the heap memory		*/
	sizeRequest			= size;
	memory->heap			= gcAlloc(sizeRequest);
	memset(memory->heap, 0, sizeRequest);
	size				= sizeRequest;
	assert(memory->heap != NULL);
	PDEBUG("GC Shifter: init_memory: 	Heap size			= %u Bytes\n", size);

	/* Initialize the heap			*/
	memory->bottom			= memory->heap;
	memory->top			= memory->heap + size;
	memory->maxHeap			= 0;
	memory->shutdown		= memory_shutdown;
	memory->fetchFreeMemory		= memory_fetchFreeMemory;
	memory->collect			= memory_collect;
	memory->addObjectReference	= memory_addObjectReference;
	memory->freeMemorySize		= internal_freeMemorySize;
	assert((memory->top - memory->bottom) == size);

	/* Compute the size of the heap	*
	 * occupance			*/
	if (((memory->top - memory->bottom) % (sizeof(JITNUINT)*8)) == 0){
		memory->heapOccupanceWords	= ((memory->top - memory->bottom) / (sizeof(JITNUINT)*8));
	} else {
		memory->heapOccupanceWords	= ((memory->top - memory->bottom) / (sizeof(JITNUINT)*8)) + 1;
	}
	heapOccupanceSize		= (memory->heapOccupanceWords) * sizeof(JITNUINT);
	assert(heapOccupanceSize > 0);

	/* Alloc the heap occupance	*/
	memory->heapOccupance		= (JITNUINT *) gcAlloc (heapOccupanceSize);
	assert(memory->heapOccupance != NULL);

	/* Initialize the objects reference	*/
	sizeRequest				= INITIAL_OBJECTS_REFERENCE_SIZE(size);
	(memory->objectsReference).objects	= gcAlloc(sizeRequest);
	(memory->objectsReference).top		= 0;
	(memory->objectsReference).max		= 0;
	(memory->objectsReference).allocated	= INITIAL_OBJECTS_REFERENCE_SIZE(size) / sizeof(JITNUINT);
	assert((memory->objectsReference).objects != NULL);
	PDEBUG("GC Shifter: init_memory: 	Objects reference size		= %u Bytes\n", sizeRequest);
	PDEBUG("GC Shifter: init_memory: 	Objects reference length	= %u Slots\n", (memory->objectsReference).allocated);

	/* Return				*/
	PDEBUG("GC Shifter: init_memory: Exit\n");
	return 0;
}

JITINT16 memory_shutdown (t_memory *memory){

	/* Assertions		*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert(memory->heapOccupance != NULL);
	assert((memory->objectsReference).objects != NULL);
	PDEBUG("GC Shifter: memory_shutdown: Start\n");

	/* Free the heap	*/
	PDEBUG("GC Shifter: memory_shutdown: 	Free the heap\n");
	gcFree(memory->heap);
	gcFree(memory->heapOccupance);

	/* Free the objects 	*
	 * references		*/
	PDEBUG("GC Shifter: memory_shutdown: 	Free the objects reference\n");
	gcFree((memory->objectsReference).objects);
	
	/* Return		*/
	PDEBUG("GC Shifter: memory_shutdown: Exit\n");
	return 0;
}

void * memory_fetchFreeMemory (t_memory *memory, JITNUINT size, t_gc_behavior *gc){
	void 	*startFreeMemory;

	/* Assertions			*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert(gc != NULL);
	assert(size > 0);
	assert(MEMORY_FREEBYTES(memory) >= 0);
	PDEBUG("GC Shifter: fetchFreeMemory: Start\n");

	/* Initialize the variables	*/
	startFreeMemory	= NULL;

	/* Check to see if there are memory to	*
	 * store the new object on the top of	*
	 * the heap				*/
	if (MEMORY_FREEBYTES(memory) >= size){
		
		PDEBUG("GC Shifter: fetchFreeMemory:	There is enought memory\n");
		startFreeMemory	= memory->heap;
		memory->heap	+= size;
		assert(MEMORY_FREEBYTES(memory) >= 0);
	} else {
		
		/* The object does not fill into the memory; I 	*
		 * have to run the garbage collection		*/
		PDEBUG("GC Shifter: fetchFreeMemory:	Run the collector\n");
		memory->collect(memory, gc);
		PDEBUG("GC Shifter: fetchFreeMemory:	The collector has finished\n");
		
		/* Check if now there is a free memory		*/
		PDEBUG("GC Shifter: fetchFreeMemory:	Fetch the best fit block\n");
		if (MEMORY_FREEBYTES(memory) >= size){
		
			PDEBUG("GC Shifter: fetchFreeMemory: 	Now there is enought memory\n");
			startFreeMemory	= memory->heap;
			memory->heap	+= size;
			assert(MEMORY_FREEBYTES(memory) >= 0);
		} else {
			PDEBUG("GC Shifter: fetchFreeMemory: 	There isn't again enought memory\n");
		}
	}
	PDEBUG("GC Shifter: fetchFreeMemory:	Free heap	= %u Bytes\n", (JITUINT32) MEMORY_FREEBYTES(memory));
	
	/* Return			*/
	PDEBUG("GC Shifter: fetchFreeMemory: Exit\n");
	return startFreeMemory;
}

void memory_addObjectReference (t_memory *memory, void *newObject){
	JITNUINT	newSize;
	JITUINT32	count;

	/* Assertions			*/
	assert(newObject != NULL);
	assert(memory != NULL);
	assert((memory->objectsReference).objects != NULL);
	assert((memory->objectsReference).top <= (memory->objectsReference).allocated);

	/* Initialize the variables	*/
	newSize	= 0;
	count	= 0;

	/* Check that there is not the	*
	 * object already in the 	*
	 * objects list			*/
	#ifdef DEBUG
	for (count=0; count < (memory->objectsReference).top; count++){
		assert((memory->objectsReference).objects[count] != newObject);
	}
	#endif

	/* Check if the memory 	of the 	*
	 * objects references is enought*/
	if ((memory->objectsReference).top == (memory->objectsReference).allocated){
		newSize					= ((memory->objectsReference).allocated + OBJECTS_SIZE_STEP) * sizeof(void *);
		(memory->objectsReference).objects	= gcRealloc((memory->objectsReference).objects, newSize);
		(memory->objectsReference).allocated	+= OBJECTS_SIZE_STEP;
		assert((memory->objectsReference).objects != NULL);
	}
	assert((memory->objectsReference).top < (memory->objectsReference).allocated);

	/* Add the reference of the new *
	 * object			*/
	(memory->objectsReference).objects[(memory->objectsReference).top]	= newObject;
	((memory->objectsReference).top)++;
	#ifdef DEBUG
	for (count=0; count < ((memory->objectsReference).top - 1); count++){
		assert((memory->objectsReference).objects[count] != newObject);
	}
	assert((memory->objectsReference).objects[count] == newObject);
	#endif

	/* Return			*/
	return;
}

void memory_collect(t_memory *memory, t_gc_behavior *gc){
	JITNUINT	*objectsReferenceMark;
	JITINT32	objectsReferenceSize;
	JITUINT32	objectsReferenceLength;
	JITUINT32	count;
	JITUINT32	bytesSweeped;
	JITUINT32	bytesMarked;
	JITUINT32	bytesMarkedByStaticRootSets;
	JITUINT32	bytesFree;
	JITUINT32	objReferencesSweeped;
	JITUINT32	objReferencesOld;
	JITUINT32	maxHeap;
	struct timespec	startTime;
	struct timespec	stopTime;
	XanList		*rootSets;
	XanList		*objectsLived;
	XanList		*oldObjectsLived;
	
	/* Assertions			*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert(gc != NULL);

	/* Fetch the start time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &startTime);
	}
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC Shifter: Start collection at %s", ctime(&t));
	}
	PDEBUG("GC Shifter: collect: Start\n");
	PDEBUG("GC Shifter: collect: 	Memory top			= %p\n", memory->top);
	PDEBUG("GC Shifter: collect: 	Memory Bottom			= %p\n", memory->bottom);
	PDEBUG("GC Shifter: collect: 	Heap size			= %u Bytes\n", (JITUINT32) (JITNUINT) (memory->top - memory->bottom));
	PDEBUG("GC Shifter: collect:	Objects allocated		= %u\n", (memory->objectsReference).top);

	/* Initialize the variables	*/
	objectsReferenceMark		= NULL;
	rootSets			= NULL;
	oldObjectsLived			= NULL;
	bytesSweeped			= 0;
	bytesMarked			= 0;
	bytesFree			= 0;
	objectsReferenceSize		= 0;
	objectsReferenceLength		= 0;
	bytesMarkedByStaticRootSets	= 0;
	count				= 0;

	/* Compute the heap informations*/
	if (behavior.profile){
		maxHeap	= ((JITNUINT)(memory->top - memory->bottom)) - MEMORY_FREEBYTES(memory);
		if (maxHeap > memory->maxHeap) memory->maxHeap = maxHeap;
	}

	/* Compute the size of the 	*
	 * objects reference bitmap	*/
	objectsReferenceLength	= ((memory->objectsReference).top / (sizeof(JITNUINT) * 8)) + 1;
	objectsReferenceSize	= objectsReferenceLength * sizeof(JITNUINT);
	assert(objectsReferenceSize > 0);
	PDEBUG("GC Shifter: collect: 	Objects reference bitmap 	= %u words	%u Bytes\n", objectsReferenceLength, objectsReferenceSize);
	
	/* Alloc the objects reference	*
	 * bitmap			*/
	objectsReferenceMark	= (JITNUINT *) gcAlloc(objectsReferenceSize);
	assert(objectsReferenceMark != NULL);

	/* Initialize the heap occupance*/
	PDEBUG("GC Shifter: collect: 	Initialize the heap bitmap\n");
	memset(memory->heapOccupance, 0, sizeof(JITNUINT) * (memory->heapOccupanceWords));
	
	/* Initialize the objects 	*
	 * reference bitmap		*/
	PDEBUG("GC Shifter: collect: 	Initialize the objects reference bitmap\n");
	memset(objectsReferenceMark, 0, objectsReferenceSize);

	/* Fetch the root sets		*/
	PDEBUG("GC Shifter: collect: 	Fetch the root set\n");
	rootSets	= gc->getRootSet();
	assert(rootSets != NULL);
	PDEBUG("GC Shifter: collect: 		Root set cardinality 	= %u\n", rootSets->length(rootSets));

	/* Set the occupance of the heap*
	 * MARK phase			*/
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC Shifter: 	Starting mark phase at %s", ctime(&t));
	}
	PDEBUG("GC Shifter: collect: 	Begin the Mark phase of the collection\n");
	objectsLived			= xanListNew(gcAlloc, gcFree, internal_cloneLiveObjects);
	assert(objectsLived != NULL);
	bytesMarked			= mark_the_heap(memory, memory->heapOccupance, objectsReferenceMark, gc, rootSets, objectsLived);
	PDEBUG("GC Shifter: collect: 		Totally %u Bytes of the heap was marked for %u live objects\n", bytesMarked, objectsLived->length(objectsLived));
	assert(objectsLived != NULL);
	#ifdef DEBUG
	oldObjectsLived			= objectsLived->cloneList(objectsLived);
	assert(oldObjectsLived != NULL);
	assert(! internal_areLiveObjectsCorrupted(objectsLived, oldObjectsLived));
	#endif

	/* Make the memory blocks of 	*
	 * the memory not marked.	*
	 * SWEEP phase			*/
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC Shifter: 	Starting sweep phase at %s", ctime(&t));
	}
	PDEBUG("GC Shifter: collect: 	Begin the Sweep phase of the collection\n");
	PDEBUG("GC Shifter: collect: 		Sweep the memory heap\n");
	bytesSweeped			= sweep_the_heap_from_top_to_bottom(memory, rootSets, objectsLived, gc);
	assert(bytesSweeped >= 0);
	assert(bytesSweeped <= ((JITNUINT)(memory->top - memory->bottom)));
	assert(((JITNUINT)(memory->heap - memory->bottom)) == bytesMarked);
	assert(((JITNUINT)(memory->top - memory->heap)) == bytesSweeped);
	PDEBUG("GC Shifter: collect: 		Sweeped %u Bytes on %u Bytes\n", bytesSweeped, (JITUINT32) (JITNUINT) (memory->top - memory->bottom));

	/* Check the live objects	*/
	assert(! internal_areLiveObjectsCorrupted(objectsLived, oldObjectsLived));

	/* Sweep the objects references	*/
	PDEBUG("GC Shifter: collect: 		Sweep the objects references\n");
	objReferencesOld	= (memory->objectsReference).top;
	objReferencesSweeped	= sweep_the_objects_references(memory, objectsReferenceMark, objectsReferenceLength);
	PDEBUG("GC Shifter: collect: 		Sweeped %u objects reference on %u\n", objReferencesSweeped, objReferencesOld);

	/* Check the live objects	*/
	assert(! internal_areLiveObjectsCorrupted(objectsLived, oldObjectsLived));

	/* Print the free blocks	*/
	#ifdef DEBUG
	PDEBUG("GC Shifter: collect: 	Heap size			= %u Bytes\n", (JITUINT32) (JITNUINT) (memory->top - memory->bottom));
	PDEBUG("GC Shifter: collect: 	Bytes free on the heap		= %u Bytes\n", (JITUINT32) MEMORY_FREEBYTES(memory));
	PDEBUG("GC Shifter: collect: 	Bytes marked on the heap	= %u Bytes\n", bytesMarked);
	PDEBUG("GC Shifter: collect: 	Objects reference tracer size	= %u Bytes\n", (JITUINT32) ((memory->objectsReference).top * sizeof(void *)));
	PDEBUG("GC Shifter: collect: 	Objects reference heap length	= %u\n", (memory->objectsReference).top);
	assert((MEMORY_FREEBYTES(memory) + bytesMarked) == (memory->top - memory->bottom));
	#endif

	/* Free the bitmap of the heap	*/
	objectsLived->destroyList(objectsLived);
	gcFree(objectsReferenceMark);
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC Shifter: End collection at %s", ctime(&t));
	}
	#ifdef DEBUG
	oldObjectsLived->destroyListAndData(oldObjectsLived);
	#endif

	/* Check the heap compacted	*/
	#ifdef DEBUG
	objectsLived			= xanListNew(gcAlloc, gcFree, NULL);
	assert(objectsLived != NULL);
	objectsReferenceMark	= (JITNUINT *) gcAlloc(objectsReferenceSize);
	assert(objectsReferenceMark != NULL);
	memset(objectsReferenceMark, 0, objectsReferenceSize);
	memset(memory->heapOccupance, 0, sizeof(JITNUINT) * (memory->heapOccupanceWords));
	bytesMarked	= mark_the_heap(memory, memory->heapOccupance, objectsReferenceMark, gc, rootSets, objectsLived);
	assert(bytesMarked == (memory->heap - memory->bottom));
	for (count=0; count < (memory->heap - memory->bottom); count++){
		assert(isMarkedByteOfHeap(memory, count));
	}
	for (; count < (memory->top - memory->bottom); count++){
		assert(!isMarkedByteOfHeap(memory, count));
	}
	objectsLived->destroyList(objectsLived);
	gcFree(objectsReferenceMark);
	#endif

	/* Fetch the stop time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &stopTime);
	
		/* Store profiling informations		*/
		gcTime.collectTime	+= diff_time(&startTime, &stopTime);
	}

	/* Return			*/
	PDEBUG("GC Shifter: collect: Exit\n");
	return;
}

JITUINT32 markMemoryByObject (t_memory *memory, void *obj, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets, XanList *objectsLived){
	JITUINT32		objectByteCount;
	JITUINT32		count;
	JITUINT32		upstairFieldsNumber;
	JITUINT32		bytesMarked;
	XanList			*objectsReachable;
	XanListItem		*item;
	void			*startAddress;

	/* Assertions				*/
	assert(memory != NULL);
	assert(obj != NULL);
	assert(objectsLived != NULL);
	assert(heapOccupance != NULL);
	assert(objectsReferenceMark != NULL);
	assert(gc != NULL);
	assert(rootSets != NULL);
	PDEBUG("GC Shifter: markMemoryByObject: Start\n");
	PDEBUG("GC Shifter: markMemoryByObject: 	Object	= %u\n", (JITUINT32)(JITNUINT)(obj - memory->bottom));
		
	/* Initialize the variables		*/
	objectsReachable	= NULL;
	item			= NULL;
	bytesMarked		= 0;
	objectByteCount		= 0;
	upstairFieldsNumber	= 0;
	count			= 0;
	
	/* Check if the object is valid		*/
	PDEBUG("GC Shifter: markMemoryByObject: 	Check if it is a valid object\n");
	if (!isAnObjectAllocated(memory, obj, &count)){
		PDEBUG("GC Shifter: markMemoryByObject: 		The object is not valid\n");
		PDEBUG("GC Shifter: markMemoryByObject: Exit\n");
		return 0;
	}
	assert(obj >= memory->bottom);
	assert(obj <= memory->top);
	PDEBUG("GC Shifter: markMemoryByObject: 		The object is valid\n");

	/* Check if the object was already 	*
	 * marked				*/
	if (isMarked(objectsReferenceMark, count)) {
		PDEBUG("GC Shifter: markMemoryByObject: 	The object is marked\n");
		PDEBUG("GC Shifter: markMemoryByObject: Exit\n");
		return 0;
	}

	/* Mark the current object		*/
	PDEBUG("GC Shifter: markMemoryByObject: 	Mark the object\n");
	markObject(objectsReferenceMark, count);

	/* Compute the start address of the 	*
	 * memory occupied by the object	*/
	startAddress	= obj - HEADER_FIXED_SIZE;
	assert(startAddress != NULL);
	assert(startAddress >= memory->bottom);
	assert(startAddress <= memory->top);

	/* Add the current object to the list	*
	 * of live objects			*/
	PDEBUG("GC Shifter: markMemoryByObject: 	Add the object to the list of live objects\n");
	assert(objectsLived->find(objectsLived, obj) == NULL);
	objectsLived->insert(objectsLived, obj);
	assert(objectsLived->find(objectsLived, obj) != NULL);
	PDEBUG("GC Shifter: markMemoryByObject: 		Objects live	= %d\n", objectsLived->length(objectsLived));

	/* Set the occupance in the memory of 	*
	 * the current object			*/
	PDEBUG("GC Shifter: markMemoryByObject: 	Mark the heap used by the object\n");
	PDEBUG("GC Shifter: markMemoryByObject: 		Mark the bytes of the heap from %u to %u\n", (JITUINT32)(startAddress - memory->bottom), (JITUINT32) ((((JITUINT32)(startAddress - memory->bottom)) + gc->sizeObject(obj)) - 1));
	for (objectByteCount=0; objectByteCount < behavior.sizeObject(obj); objectByteCount++){
		JITUINT32	bytesToMark;
		JITUINT32	trigger;
		JITUINT32	heapFlag;
		assert((startAddress + objectByteCount) >= memory->bottom);
		assert((startAddress + objectByteCount) <= memory->top);
		bytesToMark	= (startAddress - memory->bottom) + objectByteCount;
		trigger		= bytesToMark / (sizeof(JITNUINT) * 8);
		heapFlag	= 0x1;
		heapFlag	= heapFlag << (bytesToMark % (sizeof(JITNUINT)*8));
		assert((heapOccupance[trigger] & heapFlag) == 0);
		heapOccupance[trigger] |= heapFlag;
	}
	assert(behavior.sizeObject(obj) == objectByteCount);
	bytesMarked += objectByteCount;
	
	/* Run the mark phase for each object 	*
	 * reference stored inside the current	*
	 * object				*/
	PDEBUG("GC Shifter: markMemoryByObject: 	Fetch the objects reachable by one step from the current one\n");
	objectsReachable	= behavior.getReferencedObject(obj);
	assert(objectsReachable != NULL);
	PDEBUG("GC Shifter: markMemoryByObject: 	Check all the %u objects reachable from this one\n", objectsReachable->length(objectsReachable));
	if (objectsReachable->length(objectsReachable) > 0){
		assert(objectsReachable->length(objectsReachable) > 0);
		item	= objectsReachable->first(objectsReachable);
		assert(item != NULL);
		while (item != NULL){
			void *obj2;
			void **obj2Ptr;
			obj2Ptr	= objectsReachable->data(objectsReachable, item);
			assert(obj2Ptr != NULL);
			obj2	= (*obj2Ptr);
			if (	(obj2 != NULL)			&&
				(obj2 - HEADER_FIXED_SIZE >= memory->bottom)	&&
				(obj2 - HEADER_FIXED_SIZE <= memory->top)		){
				bytesMarked	+= markMemoryByObject (memory, obj2, heapOccupance, objectsReferenceMark, gc, rootSets, objectsLived);
			}
			item	= objectsReachable->next(objectsReachable, item);
		}
	}

	/* Free the memory			*/
	objectsReachable->destroyList(objectsReachable);

	/* Return the bytes marked		*/
	PDEBUG("GC Shifter: markMemoryByObject: Exit\n");
	return bytesMarked;
}
	
void printBitmap (char *prefix, JITNUINT *bitmap, JITUINT32 size){
	JITUINT32	count;

	/* Assertions		*/
	assert(prefix != NULL);
	assert(bitmap != NULL);
	PDEBUG("%s: Start\n", prefix);
	
	/* Print the bitmap	*/
	for (count=0; count<size; count++){
		PDEBUG("%s:	Word %d	= 0x%X\n", prefix, count, (JITUINT32) bitmap[count]);
	}

	/* Return		*/
	PDEBUG("%s: Exit\n", prefix);
	return;
}

JITUINT32 mark_the_heap(t_memory *memory, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets, XanList *objectsLived){
	XanListItem	*item;
	JITUINT32	bytesMarked;
	JITUINT32	currentBytesMarked;
	void		**objPtr;

	/* Assertions						*/
	assert(memory != NULL);
	assert(gc != NULL);
	assert(heapOccupance != NULL);
	assert(objectsReferenceMark != NULL);
	assert(rootSets != NULL);
	assert(objectsLived != NULL);
	PDEBUG("GC Shifter: mark_the_heap: Start\n");
	PDEBUG("GC Shifter: mark_the_heap: 	Root set cardinality	= %d\n", rootSets->length(rootSets));

	/* Initialize the variables				*/
	bytesMarked	= 0;
	objPtr		= NULL;

	/* Mark the heap starting from the root set		*/
	PDEBUG("GC Shifter: mark_the_heap: 	Mark the heap\n");
	item	= rootSets->first(rootSets);
	while (item != NULL){
		
		/* Fetch the object pointer			*/
		objPtr			= rootSets->data(rootSets, item);
		assert(objPtr != NULL);
		assert(objectsLived->equalsInstancesNumber(objectsLived, *objPtr) <= 1);

		/* Check if the current root point to an object	*
		 * not already visited				*/
		if (	((*objPtr) != NULL)					&&
			(objectsLived->find(objectsLived, *objPtr) == NULL)	){

			/* Mark the heap occupied by the object		*/
			PDEBUG("GC Shifter: mark_the_heap:		Mark the heap occupied by the object %u\n", (JITUINT32)(JITNUINT)((*objPtr) - memory->bottom));
			currentBytesMarked	= markMemoryByObject(memory, *objPtr, heapOccupance, objectsReferenceMark, gc, rootSets, objectsLived);
			PDEBUG("GC Shifter: mark_the_heap:			Marked %u bytes\n", currentBytesMarked);
			bytesMarked		+= currentBytesMarked;
			PDEBUG("GC Shifter: mark_the_heap:			Totally marked bytes	= %u\n", bytesMarked);
		}
		assert(objectsLived->equalsInstancesNumber(objectsLived, *objPtr) <= 1);

		/* Fetch the next object			*/
		item			= rootSets->next(rootSets, item);
	}
	PDEBUG("GC Shifter: mark_the_heap: 	Total marked bytes	= %u\n", bytesMarked);
	
	/* Print the objects live			*/
	#ifdef DEBUG
	PDEBUG("GC Shifter: mark_the_heap: 	Print the objects live\n");
	printLiveObjects(objectsLived, gc);
	#endif

	/* Return the bytes marked				*/
	PDEBUG("GC Shifter: mark_the_heap: Exit\n");
	return bytesMarked;
}

JITBOOLEAN isMarked (JITNUINT *objectsReferenceMark, JITUINT32 objectID){
	JITUINT32	trigger;
	JITUINT32	objFlag;

	/* Assertions			*/
	assert(objectsReferenceMark != NULL);

	/* Check the object		*/
	trigger		= objectID / (sizeof(JITNUINT) * 8);
	objFlag		= 0x1;
	objFlag		= objFlag << (objectID % (sizeof(JITNUINT)*8));
	return ((objectsReferenceMark[trigger] & objFlag) != 0);
}

void markObject(JITNUINT *objectsReferenceMark, JITUINT32 objectID){
	JITUINT32	trigger;
	JITUINT32	objFlag;

	/* Assertions			*/
	assert(objectsReferenceMark != NULL);

	/* Mark the object		*/
	trigger		= objectID / (sizeof(JITNUINT) * 8);
	objFlag		= 0x1;
	objFlag		= objFlag << (objectID % (sizeof(JITNUINT)*8));
	objectsReferenceMark[trigger] |= objFlag;

	/* Return			*/
	return;
}
			
int internal_isInRootSets(XanList *rootSets, void *obj){
	XanListItem	*item;
	void		**slot;

	/* Assertions			*/
	assert(rootSets != NULL);
	
	if (obj == NULL) return 0;
	item	= rootSets->first(rootSets);
	while (item != NULL){
		slot	= (void **) rootSets->data(rootSets, item);
		assert(slot != NULL);
		if ((*slot) == obj) return 1;
		item	= rootSets->next(rootSets, item);
	}
	return 0;
}

JITUINT32 internal_freeMemorySize (t_memory *memory){
	
	/* Assertions			*/
	assert(memory != NULL);

	return (JITUINT32) MEMORY_FREEBYTES(memory);
}

void * internal_cloneLiveObjects (void *object){
	JITNUINT	size;
	JITNUINT	size2;
	void		*newObject;

	/* Assertions			*/
	assert(object != NULL);
	
	size	= behavior.sizeObject(object);
	assert(size > 0);
	newObject	= gcAlloc(size);
	assert(newObject != NULL);
	memcpy(newObject, object, size);
	size2	= behavior.sizeObject(newObject);
	assert(size2 > 0);
	assert(size == size2);
	return newObject;
}
	
JITINT16 internal_areLiveObjectsCorrupted(XanList *objectsLived, XanList *oldObjectsLived){
	XanListItem	*item;
	XanListItem	*item2;
	JITNUINT	size1;
	JITNUINT	size2;
	JITUINT32	count;
	JITUINT32	count2;
	void		*obj1;
	void		*obj2;

	/* Assertions			*/
	assert(objectsLived != NULL);
	assert(oldObjectsLived != NULL);
	assert(objectsLived->length(objectsLived) == oldObjectsLived->length(oldObjectsLived));

	count	= 0;
	item	= objectsLived->first(objectsLived);
	while (item != NULL){
		obj1	= objectsLived->data(objectsLived, item);
		assert(obj1 != NULL);
		item2	= oldObjectsLived->first(oldObjectsLived);
		assert(item2 != NULL);
		for (count2=0; count2 < count; count2++){
			assert(item2 != NULL);
			item2	= oldObjectsLived->next(oldObjectsLived, item2);
		}
		assert(item2 != NULL);
		obj2	= oldObjectsLived->data(oldObjectsLived, item2);
		assert(obj2 != NULL);
		size1	= behavior.sizeObject(obj1);
		assert(size1 > 0);
		size2	= behavior.sizeObject(obj2);
		assert(size2 > 0);
		assert(size1 == size2);
		count++;
		item	= objectsLived->next(objectsLived, item);
	}

	return 0;
}
