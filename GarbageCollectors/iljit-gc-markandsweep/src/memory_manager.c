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
#include <iljit_gc_markandsweep.h>
#include <iljit-utils.h>
#include <internal_memory_manager.h>
#include <time.h>
// End

#define INITIAL_OBJECTS_REFERENCE_SIZE(arg)	arg
#define INITIAL_MEMORY_BLOCK_SIZE(arg)		arg
#define INITIAL_RAWMEMORY_SIZE(arg)		arg
#define INITIAL_FREE_RAWMEMORY_SIZE(arg)	arg
#define OBJECTS_SIZE_STEP			500
#define MEMORY_BLOCK_SIZE_STEP			20

JITINT16 memory_shutdown (t_memory *memory, t_gc_behavior *gc);
void * memory_fetchFreeMemory (t_memory *memory, JITNUINT size, t_gc_behavior *gc);
void memory_resizeMemory (t_memory *memory, void *obj, JITUINT32 newSize,  t_gc_behavior *gc);
void memory_addObjectReference (t_memory *memory, void *newObject);
void memory_collect(t_memory *memory, t_gc_behavior *gc);
JITUINT32 mark_the_heap(t_memory *memory, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets);
JITUINT32 markMemoryByObject (t_memory *memory, void *obj, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets);
JITINT16 isAnObjectAllocated(t_memory *memory, void *obj, JITUINT32 *objectID);
void * fetchBestFitMemoryBlock (t_memory *memory, JITUINT32 size);
void check_MemoryBlocks_for_one_slot (t_memory_blocks *memoryBlocks);
void printBitmap (char *prefix, JITNUINT *bitmap, JITUINT32 size);
JITUINT32 insertNewFreeBlock (t_memory *memory, t_memory_block *newBlock);
void deleteMemoryBlock(t_memory_blocks *memoryBlocks, JITUINT32 blockID);
JITBOOLEAN isMarked (JITNUINT *objectsReferenceMark, JITUINT32 objectID);
void markObject(JITNUINT *objectsReferenceMark, JITUINT32 objectID);
int internal_isInRootSets(XanList *rootSets, void *obj);
JITUINT32 printFreeBlocks (t_memory *memory);
void eraseFreeBlocks (t_memory *memory);

JITINT16 init_memory(t_memory *memory, JITUINT32 size){
	JITINT32	sizeRequest;
	JITINT32	heapOccupanceSize;

	/* Assertions		*/
	assert(memory != NULL);
	assert((size % getpagesize()) == 0);
	PDEBUG("GC MarkAndSweep: init_memory: Start\n");
	
	/* Alloc the heap memory		*/
	sizeRequest			= size;
	memory->heap			= gcAlloc(sizeRequest);
	memset(memory->heap, 0, sizeRequest);
	size				= sizeRequest;
	assert(memory->heap != NULL);
	PDEBUG("GC MarkAndSweep: init_memory: 	Heap size			= %u Bytes\n", size);

	/* Initialize the heap			*/
	memory->bottom			= memory->heap;
	memory->top			= memory->heap + size;
	memory->maxHeap			= 0;
	memory->shutdown		= memory_shutdown;
	memory->fetchFreeMemory		= memory_fetchFreeMemory;
	memory->resizeMemory		= memory_resizeMemory;
	memory->collect			= memory_collect;
	memory->addObjectReference	= memory_addObjectReference;
	memory->freeMemorySize		= printFreeBlocks;
	assert((memory->top - memory->bottom) == size);

	/* Compute the size of the heap	*
	 * occupance			*/
	memory->heapOccupanceWords	= ((memory->top - memory->bottom) / (sizeof(JITNUINT)*8));
	if((memory->top - memory->bottom) % (sizeof(JITNUINT)*8) != 0){
		memory->heapOccupanceWords++;
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
	(memory->objectsReference).size		= sizeRequest;
	assert((memory->objectsReference).objects != NULL);
	PDEBUG("GC MarkAndSweep: init_memory: 	Objects reference size		= %u Bytes\n", sizeRequest);
	PDEBUG("GC MarkAndSweep: init_memory: 	Objects reference length	= %u Slots\n", (memory->objectsReference).allocated);

	/* Initialize the memory blocks		*/
	sizeRequest				= INITIAL_MEMORY_BLOCK_SIZE(size);
	(memory->memoryBlocks).block		= gcAlloc(sizeRequest);
	(memory->memoryBlocks).top		= 0;
	(memory->memoryBlocks).max		= 0;
	(memory->memoryBlocks).allocated	= INITIAL_MEMORY_BLOCK_SIZE(size) / sizeof(t_memory_block);
	(memory->memoryBlocks).size		= sizeRequest;
	assert((memory->memoryBlocks).block != NULL);
	PDEBUG("GC MarkAndSweep: init_memory: 	Free memory block size		= %u Bytes\n", sizeRequest);
	PDEBUG("GC MarkAndSweep: init_memory: 	Free memory block length	= %u Slots\n", (memory->memoryBlocks).allocated);

	/* Add the entire heap as the first free*
	 * big block				*/
	(memory->memoryBlocks).block[0].start	= memory->heap;
	(memory->memoryBlocks).block[0].size	= size;
	(memory->memoryBlocks).top		= 1;

	/* Return				*/
	PDEBUG("GC MarkAndSweep: init_memory: Exit\n");
	return 0;
}

JITINT16 memory_shutdown (t_memory *memory, t_gc_behavior *gc){
	JITINT32	count;

	/* Assertions		*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert((memory->memoryBlocks).block != NULL);
	assert((memory->objectsReference).objects != NULL);
	assert((memory->objectsReference).size > 0);
	assert(gc != NULL);
	PDEBUG("GC MarkAndSweep: memory_shutdown: Start\n");

	/* Finalize the objects	*/
	for (count=0; count < (memory->objectsReference).top; count++){
		//gc->callFinalizer((memory->objectsReference).objects[count]); TODO da decommentare
	}

	/* Free the heap	*/
	PDEBUG("GC MarkAndSweep: memory_shutdown: 	Free the heap\n");
	gcFree(memory->heap);
	gcFree(memory->heapOccupance);

	/* Free the list of the	*
	 * old blocks		*/
	PDEBUG("GC MarkAndSweep: memory_shutdown: 	Free the memory blocks\n");
	gcFree((memory->memoryBlocks).block);

	/* Free the objects 	*
	 * references		*/
	PDEBUG("GC MarkAndSweep: memory_shutdown: 	Free the objects reference\n");
	gcFree((memory->objectsReference).objects);
	
	/* Return		*/
	PDEBUG("GC MarkAndSweep: memory_shutdown: Exit\n");
	return 0;
}

void  memory_resizeMemory (t_memory *memory, void *obj, JITUINT32 newSize,  t_gc_behavior *gc){
	JITINT32	count;
	JITINT32	size;
	JITINT32	diff;
	void		*start;

	/* Assertions			*/
	assert(memory != NULL);
	assert(obj != NULL);

	/* Compute the diff		*/
	size	= gc->sizeObject(obj);
	diff	= newSize - size;
	if (diff <= 0) return;
	assert(diff > 0);

        /* Fetch the start address of   *
         * the object.                  */
        obj     -= HEADER_FIXED_SIZE;

	/* Search the memory block	*/
	for (count=0; count < (memory->memoryBlocks).top; count++){
		assert((memory->memoryBlocks).block[count].start != NULL);
		assert((memory->memoryBlocks).block[count].start >= memory->bottom);
		assert((memory->memoryBlocks).block[count].start <= memory->top);
		assert((memory->memoryBlocks).block[count].size > 0);
		start	= (memory->memoryBlocks).block[count].start;
		if (	(start > obj) 						&&
			(start == obj + size)					&&
			(diff <= (memory->memoryBlocks).block[count].size)	){
			(memory->memoryBlocks).block[count].size -= diff;
			(memory->memoryBlocks).block[count].start += diff;
	
			/* Check if we have to delete 	*
			 * the block			*/
			if ((memory->memoryBlocks).block[count].size == 0){
				deleteMemoryBlock(&(memory->memoryBlocks), count);
			}
			return;
		}
	}
	print_err("GC: ERROR = Cannot resize the live object. ", 0);
	abort();
}

void * memory_fetchFreeMemory (t_memory *memory, JITNUINT size, t_gc_behavior *gc){
	void 	*startFreeMemory;

	/* Assertions			*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert((memory->memoryBlocks).block != NULL);
	assert(gc != NULL);
	assert(size > 0);
	PDEBUG("GC MarkAndSweep: fetchFreeMemory: Start\n");

	/* Initialize the variables	*/
	startFreeMemory	= NULL;

	/* Check to see if there are memory to	*
	 * store the new object on the top of	*
	 * the heap				*/
	startFreeMemory	= fetchBestFitMemoryBlock (memory, size);
	if (startFreeMemory == NULL){
		PDEBUG("GC MarkAndSweep: fetchFreeMemory:	There isn't enough memory\n");

		/* The object does not fill into the memory; I 	*
		 * have to run the garbage collection		*/
		PDEBUG("GC MarkAndSweep: fetchFreeMemory:	Run the collector\n");
		memory->collect(memory, gc);
		PDEBUG("GC MarkAndSweep: fetchFreeMemory:	The collector has finished\n");

		/* Check if now there is a free memory		*/
		PDEBUG("GC MarkAndSweep: fetchFreeMemory:	Fetch the best fit block\n");
		startFreeMemory	= fetchBestFitMemoryBlock (memory, size);
		if (startFreeMemory != NULL){
			PDEBUG("GC MarkAndSweep: fetchFreeMemory: 	Now there is enough memory\n");
		} else {
			PDEBUG("GC MarkAndSweep: fetchFreeMemory: 	There isn't again enough memory\n");
		}
	} else {
		PDEBUG("GC MarkAndSweep: fetchFreeMemory:	There is enough memory\n");
	}
	
	/* Print the free blocks	*/
	#ifdef DEBUG
	PDEBUG("GC MarkAndSweep: fetchFreeMemory:	Free memory blocks:\n");
	PDEBUG("GC MarkAndSweep: fetchFreeMemory:	Total free memory	= %u Bytes\n", printFreeBlocks (memory));
	PDEBUG("GC MarkAndSweep: fetchFreeMemory: 	Heap size		= %u Bytes	%.3f KBytes\n", (memory->top - memory->bottom), (JITFLOAT32) ((memory->top - memory->bottom) / 1024));
	#endif

	/* Return			*/
	PDEBUG("GC MarkAndSweep: fetchFreeMemory: Exit\n");
	return startFreeMemory;
}

void memory_addObjectReference (t_memory *memory, void *newObject){
	JITNUINT	newSize;

	/* Assertions			*/
	assert(newObject != NULL);
	assert(memory != NULL);
	assert((memory->memoryBlocks).block != NULL);
	assert((memory->objectsReference).objects != NULL);
	assert((memory->objectsReference).top <= (memory->objectsReference).allocated);

	/* Initialize the variables	*/
	newSize	= 0;

	/* Check if the memory 	of the 	*
	 * objects references is enough*/
	if ((memory->objectsReference).top == (memory->objectsReference).allocated){
		newSize					= ((memory->objectsReference).allocated + OBJECTS_SIZE_STEP) * sizeof(void *);
		(memory->objectsReference).objects	= gcRealloc((memory->objectsReference).objects, newSize);
		(memory->objectsReference).allocated	+= OBJECTS_SIZE_STEP;
		(memory->objectsReference).size		= newSize;
		assert((memory->objectsReference).objects != NULL);
	}
	assert((memory->objectsReference).top < (memory->objectsReference).allocated);

	/* Add the reference of the new *
	 * object			*/
	(memory->objectsReference).objects[(memory->objectsReference).top]	= newObject;
	(memory->objectsReference).top++;

	/* Return			*/
	return;
}

void memory_collect(t_memory *memory, t_gc_behavior *gc){
	JITNUINT	*objectsReferenceMark;
	JITINT32	objectsReferenceSize;
	JITUINT32	objectsReferenceLength;
	JITUINT32	count;
	JITUINT32	count2;
	JITUINT32	count3;
	JITUINT32	count4;
	JITUINT32	bytesSwept;
	JITUINT32	bytesMarked;
	JITUINT32	bytesMarkedByStaticRootSets;
	JITUINT32	bytesFree;
	JITUINT32	objReferencesSwept;
	JITUINT32	objReferencesOld;
	JITUINT32	maxHeap;
	JITBOOLEAN	modified;
	struct timespec	startTime;
	struct timespec	stopTime;
	t_memory_block 	newBlock;
	XanList		*rootSets;

	/* Assertions			*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert((memory->memoryBlocks).block != NULL);
	assert(gc != NULL);

	/* Fetch the start time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &startTime);
	}
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC MarkAndSweep: Start collection at %s", ctime(&t));
	}
	PDEBUG("GC MarkAndSweep: collect: Start\n");
	PDEBUG("GC MarkAndSweep: collect: 	Memory top			= %p\n", memory->top);
	PDEBUG("GC MarkAndSweep: collect: 	Memory Bottom			= %p\n", memory->bottom);
	PDEBUG("GC MarkAndSweep: collect: 	Heap size			= %u Bytes\n", memory->top - memory->bottom);
	PDEBUG("GC MarkAndSweep: collect:	Objects allocated		= %u\n", (memory->objectsReference).top);

	/* Initialize the variables	*/
	objectsReferenceMark		= NULL;
	newBlock.start			= NULL;
	rootSets			= NULL;
	newBlock.size			= 0;
	bytesSwept			= 0;
	bytesMarked			= 0;
	bytesFree			= 0;
	objectsReferenceSize		= 0;
	objectsReferenceLength		= 0;
	bytesMarkedByStaticRootSets	= 0;
	count				= 0;
	count2				= 0;
	count3				= 0;
	count4				= 0;

	/* Compute the heap informations*/
	if (behavior.profile){
		maxHeap	= ((JITUINT32)(memory->top - memory->bottom)) - printFreeBlocks(memory);
		if (maxHeap > memory->maxHeap) memory->maxHeap = maxHeap;
	}

	/* Delete each free blocks	*/
	count2	= (memory->memoryBlocks).top;
	for (count=0; count < count2; count++){
		deleteMemoryBlock(&(memory->memoryBlocks), 0);
	}
	assert((memory->memoryBlocks).top == 0);

	/* Compute the size of the 	*
	 * objects reference bitmap	*/
	objectsReferenceLength	= ((memory->objectsReference).top / (sizeof(JITNUINT) * 8)) + 1;
	objectsReferenceSize	= objectsReferenceLength * sizeof(JITNUINT);
	assert(objectsReferenceSize > 0);
	PDEBUG("GC MarkAndSweep: collect: 	Object references bitmap 	= %u words	%u Bytes\n", objectsReferenceLength, objectsReferenceSize);
	
	/* Alloc the objects reference	*
	 * bitmap			*/
	objectsReferenceMark	= (JITNUINT *) gcAlloc(objectsReferenceSize);
	assert(objectsReferenceMark != NULL);

	/* Initialize the heap occupance*/
	PDEBUG("GC MarkAndSweep: collect: 	Initialize the heap bitmap\n");
	memset(memory->heapOccupance, 0, sizeof(JITNUINT) * (memory->heapOccupanceWords));
	
	/* Initialize the objects 	*
	 * reference bitmap		*/
	PDEBUG("GC MarkAndSweep: collect: 	Initialize the object references bitmap\n");
	memset(objectsReferenceMark, 0, objectsReferenceSize);

	/* Fetch the root sets		*/
	PDEBUG("GC MarkAndSweep: collect: 	Fetch the root set\n");
	rootSets	= gc->getRootSet();
	assert(rootSets != NULL);
	PDEBUG("GC MarkAndSweep: collect: 		Root set cardinality 	= %u\n", rootSets->length(rootSets));

	/* Set the occupance of the heap*
	 * MARK phase			*/
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC MarkAndSweep: 	Starting mark phase at %s", ctime(&t));

	}
	PDEBUG("GC MarkAndSweep: collect: 	Begin the Mark phase of the collection\n");
	bytesMarked			= mark_the_heap(memory, memory->heapOccupance, objectsReferenceMark, gc, rootSets);
	PDEBUG("GC MarkAndSweep: collect: 		Totally %u Bytes of the heap was marked\n", bytesMarked);

	/* Make the memory blocks of 	*
	 * the memory not marked.	*
	 * SWEEP phase			*/
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC MarkAndSweep: 	Starting sweep phase at %s", ctime(&t));
	}
	PDEBUG("GC MarkAndSweep: collect: 	Begin the Sweep phase of the collection\n");
	PDEBUG("GC MarkAndSweep: collect: 		Sweep the memory heap\n");
	for (count=0, count4=0; count < memory->heapOccupanceWords; count++){
		for (count2=0; count2 < (sizeof(JITNUINT) * 8); count2++, count4++){
			JITUINT32	flag;

			/* Check if there a memory to check	*/
			if (count4 >= (memory->top - memory->bottom)) {
				break;
			}

			flag	= 0x1;
			flag	= flag << count2;
			if ((memory->heapOccupance[count] & flag) == 0x0){
				/* The byte (count * 8) + count2 is not mark	*/
				void	*startFreeBlock;
				startFreeBlock	= (memory->bottom + count4);
				assert(startFreeBlock != NULL);
				assert(startFreeBlock >= memory->bottom);
				assert(startFreeBlock <= memory->top);
				if (newBlock.start == NULL){
					newBlock.start 	= startFreeBlock;
					newBlock.size	= 1;
				} else {
					newBlock.size++;
				}
			} else {
				/* The byte (count * 8) + count2 is mark, so I 	*
				 * have to check if I have to add a new free	*
				 * block of memory				*/
				if (newBlock.start != NULL){
					PDEBUG("GC MarkAndSweep: collect: 			Found a free memory at %p (from heap bottom = %u) with size %u\n", newBlock.start, newBlock.start - memory->bottom, newBlock.size);
			
					/* Insert the new block into the list 	*
					 * of free blocks			*/
					bytesSwept += insertNewFreeBlock (memory, &newBlock);

					/* Set the new block as NULL		*/
					newBlock.start 	= NULL;
					newBlock.size	= 0;
				}
			}
		}
	}
	/* Check if exist a block at the top of the heap	*/
	if (newBlock.start != NULL){
		PDEBUG("GC MarkAndSweep: collect: 			Found a free memory at %p (from heap bottom = %u) with size %u\n", newBlock.start, newBlock.start - memory->bottom, newBlock.size);
		
		bytesSwept += insertNewFreeBlock (memory, &newBlock);
	}
	PDEBUG("GC MarkAndSweep: collect: 		Swept %u of %u Bytes\n", bytesSwept, memory->top - memory->bottom);
	PDEBUG("GC MarkAndSweep: collect: 		Sweep the object references\n");
	objReferencesOld	= (memory->objectsReference).top;
	for (count=0, objReferencesSwept=0; count < (memory->objectsReference).top; count++){
		JITUINT32	trigger;
		JITUINT32	objFlag;
		trigger		= count / (sizeof(JITNUINT) * 8);
		objFlag		= 0x1;
		objFlag		= objFlag << (count % (sizeof(JITNUINT)*8));
		assert(trigger < objectsReferenceLength);

		/* Check if the current object reference is marked	*/
		if ((objectsReferenceMark[trigger] & objFlag) == 0x0){
			PDEBUG("GC MarkAndSweep: collect: 			The object reference %u is not marked\n", count);
			objReferencesSwept++;
		
			//gc->callFinalizer((memory->objectsReference).objects[count]); TODO da decommentare
			(memory->objectsReference).objects[count]	= NULL;
		}
	}
	/* Delete the object references not marked		*/
	modified	= 1;
	PDEBUG("GC MarkAndSweep: collect: 			Delete the object references not marked\n");
	while (modified){
		modified	= 0;
		for (count=0; count < (memory->objectsReference).top; count++){
			if ((memory->objectsReference).objects[count] == NULL){
				JITUINT32	delta;
		
				/* Compute the delta			*/
				delta	= 0;
				for (count2=count; count2 < (memory->objectsReference).top; count2++){
					if ((memory->objectsReference).objects[count2] == NULL) delta++;
					else break;
				}
				assert(delta > 0);

				/* Move all the objects references after*
				 * the delta objects equals to NULL, 	*
				 * delta slot before the their old	*
				 * position				*/
				for (count2=count; (count2 + delta) < (memory->objectsReference).top; count2++){
					(memory->objectsReference).objects[count2]	= (memory->objectsReference).objects[count2 + delta];
				}
				(memory->objectsReference).top	-= delta;
				modified	= 1;
				break;
			}
		}

	}
	PDEBUG("GC MarkAndSweep: collect: 		Swept %u of %u object references.\n", objReferencesSwept, objReferencesOld);

	/* Print the free blocks	*/
	#ifdef PRINTDEBUG
	bytesFree	= printFreeBlocks (memory);
	PDEBUG("GC MarkAndSweep: collect: 	Heap size			= %u Bytes\n", memory->top - memory->bottom);
	PDEBUG("GC MarkAndSweep: collect: 	Bytes free on the heap		= %u Bytes\n", bytesFree);
	PDEBUG("GC MarkAndSweep: collect: 	Bytes marked on the heap	= %u Bytes\n", bytesMarked);
	PDEBUG("GC MarkAndSweep: collect: 	Objects reference heap size	= %u Bytes\n", (memory->objectsReference).top * sizeof(void *));
	PDEBUG("GC MarkAndSweep: collect: 	Objects reference heap length	= %u\n", (memory->objectsReference).top);
	assert(bytesFree + bytesMarked	== (memory->top - memory->bottom));
	#endif

	/* Erase the free blocks	*/
	#ifdef DEBUG
	eraseFreeBlocks(memory);
	#endif

	/* Free the bitmap of the heap	*/
	gcFree(objectsReferenceMark);
	if (gc->verbose){
		time_t	t;
		time(&t);
		printf("GC MarkAndSweep: End collection at %s", ctime(&t));
	}

	/* Fetch the stop time		*/
	if (behavior.profile){
		clock_gettime(CLOCK_REALTIME, &stopTime);
	
		/* Store profiling informations		*/
		gcTime.collectTime	+= diff_time(startTime, stopTime);
	}

	/* Return			*/
	PDEBUG("GC MarkAndSweep: collect: Exit\n");
	return;
}

JITUINT32 markMemoryByObject (t_memory *memory, void *obj, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets){
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
	assert(heapOccupance != NULL);
	assert(objectsReferenceMark != NULL);
	assert(gc != NULL);
	assert(rootSets != NULL);
	PDEBUG("GC MarkAndSweep: markMemoryByObject: Start\n");
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 	Object	= %p\n", obj);
		
	/* Initialize the variables		*/
	objectsReachable	= NULL;
	item			= NULL;
	startAddress		= NULL;
	bytesMarked		= 0;
	objectByteCount		= 0;
	upstairFieldsNumber	= 0;
	count			= 0;
	
	/* Check if the object is valid		*/
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 	Check if it is a valid object\n");
	if (	(!isAnObjectAllocated(memory, obj, &count))		||
		(!internal_isInRootSets(rootSets, obj))			){
		PDEBUG("GC MarkAndSweep: markMemoryByObject: 		The object is not valid\n");
		PDEBUG("GC MarkAndSweep: markMemoryByObject: Exit\n");
		return 0;
	}
	assert(obj >= memory->bottom);
	assert(obj <= memory->top);
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 		The object is valid\n");

	/* Check if the object was already 	*
	 * marked				*/
	if (isMarked(objectsReferenceMark, count)) {
		PDEBUG("GC MarkAndSweep: markMemoryByObject: 	The object is marked\n");
		PDEBUG("GC MarkAndSweep: markMemoryByObject: Exit\n");
		return 0;
	}

	/* Mark the current object		*/
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 	Mark the object\n");
	markObject(objectsReferenceMark, count);
        assert(isMarked(objectsReferenceMark, count));

	/* Compute the start address of the 	*
	 * memory occupied by the object	*/
	startAddress	= obj - HEADER_FIXED_SIZE;
	assert(startAddress != NULL);
	assert(startAddress >= memory->bottom);
	assert(startAddress < memory->top);

	/* Set the occupance in the memory of 	*
	 * the current object			*/
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 	Mark the heap used by the object\n");
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 		Mark the bytes of the heap from %u to %u\n", startAddress - memory->bottom, (((JITUINT32)(startAddress - memory->bottom)) + gc->sizeObject(obj)) - 1);
	for (objectByteCount=0; objectByteCount < behavior.sizeObject(obj); objectByteCount++){
		JITUINT32	bytesToMark;
		JITUINT32	trigger;
		JITUINT32	heapFlag;
		assert((startAddress + objectByteCount) >= memory->bottom);
		assert((startAddress + objectByteCount) <= memory->top);
		bytesToMark	= (startAddress - memory->bottom) + objectByteCount;
		trigger		= bytesToMark / (sizeof(JITNUINT) * 8);
		assert(trigger >= 0);
		assert(trigger < memory->heapOccupanceWords);
		heapFlag	= 0x1;
		heapFlag	= heapFlag << (bytesToMark % (sizeof(JITNUINT)*8));
		assert((heapOccupance[trigger] & heapFlag) == 0);
		heapOccupance[trigger] |= heapFlag;
		assert((heapOccupance[trigger] & heapFlag) != 0);
	}
	assert(behavior.sizeObject(obj) == objectByteCount);
	bytesMarked += objectByteCount;
	
	/* Run the mark phase for each object 	*
	 * reference stored inside the current	*
	 * object				*/
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 	Fetch the objects reachable by one step from the current one\n");
	objectsReachable	= behavior.getReferencedObject(obj);
	assert(objectsReachable != NULL);
	PDEBUG("GC MarkAndSweep: markMemoryByObject: 	Check all the %u objects reachable from this one\n", objectsReachable->length(objectsReachable));
	if (objectsReachable->length(objectsReachable) > 0){
		assert(objectsReachable->length(objectsReachable) > 0);
		item	= objectsReachable->first(objectsReachable);
		assert(item != NULL);
		while (item != NULL){
			void    *obj2;
			void    **obj2Ptr;
                        void    *obj2StartAddress;

                        /* Initialize the variables             */
                        obj2                    = NULL;
                        obj2Ptr                 = NULL;
                        obj2StartAddress        = NULL;

                        /* Fetch the object referenced          */
			obj2Ptr	                = objectsReachable->data(objectsReachable, item);
			assert(obj2Ptr != NULL);
			obj2	                = (*obj2Ptr);
                        if (obj2 != NULL){
                                obj2StartAddress        = obj2 - HEADER_FIXED_SIZE;
                        }
                
                        /* Mark the heap occupied by the object *
                         * referenced by the current one.       */
			if (	(obj2 != NULL)                          &&
				(obj2StartAddress >= memory->bottom)    &&
				(obj2StartAddress <= memory->top)       ){
				bytesMarked	+= markMemoryByObject (memory, obj2, heapOccupance, objectsReferenceMark, gc, rootSets);
			}
			item	= objectsReachable->next(objectsReachable, item);
		}
	}

	/* Free the memory			*/
	objectsReachable->destroyList(objectsReachable);

	/* Return the bytes marked		*/
	PDEBUG("GC MarkAndSweep: markMemoryByObject: Exit\n");
	return bytesMarked;
}
	
JITINT16 isAnObjectAllocated(t_memory *memory, void *obj, JITUINT32 *objectID){
	JITUINT32	count;

	/* Assertions			*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert(objectID != NULL);
	PDEBUG("GC MarkAndSweep: isAnObjectAllocated: Start\n");

	/* Check if the obj is NULL	*/
	if (obj == NULL) {
		PDEBUG("GC MarkAndSweep: isAnObjectAllocated: 	The object is NULL\n");
		PDEBUG("GC MarkAndSweep: isAnObjectAllocated: Exit\n");
		return 0;
	}
	PDEBUG("GC MarkAndSweep: isAnObjectAllocated: 	The object is not NULL\n");

	/* Check if the obj points into *
	 * the heap			*/
	if (	(obj <= (memory->bottom))	||
		(obj >= (memory->top))		){
		PDEBUG("GC MarkAndSweep: isAnObjectAllocated: 	The object does not point into the heap\n");
		PDEBUG("GC MarkAndSweep: isAnObjectAllocated: Exit\n");
		return 0;
	}
	PDEBUG("GC MarkAndSweep: isAnObjectAllocated: 	The object point into the heap\n");

	/* Check if the obj is inside 	*
	 * the objects references of the*
	 * memory			*/
	for (count=0; count < (memory->objectsReference).top; count++){
		if ((memory->objectsReference).objects[count] == obj){
			PDEBUG("GC MarkAndSweep: isAnObjectAllocated: 	The object has been allocated\n");
			PDEBUG("GC MarkAndSweep: isAnObjectAllocated: Exit\n");
			(*objectID)	= count;
			return 1;
		}
	}
	PDEBUG("GC MarkAndSweep: isAnObjectAllocated: 	The object has been not allocated\n");

	/* The object reference is not 	*
	 * allocated			*/
	PDEBUG("GC MarkAndSweep: isAnObjectAllocated: Exit\n");
	return 0;
}

void * fetchBestFitMemoryBlock (t_memory *memory, JITUINT32 size){
	JITUINT32	count;
	JITUINT32	champion;
	JITUINT32	championOverSize;
	JITUINT16	found;
	void		*championStartPoint;

	/* Assertions				*/
	assert(memory != NULL);
	assert(memory->heap != NULL);
	assert((memory->memoryBlocks).block != NULL);
	assert((memory->memoryBlocks).top <= (memory->memoryBlocks).allocated);
	PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock: Start\n");
	PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock: 	Memory top	= %p\n", memory->top);
	PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock: 	Memory Bottom	= %p\n", memory->bottom);
	PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock: 	Request %u Bytes\n", size);

	/* Initialize the variables		*/
	count			= 0;
	champion		= 0;
	championOverSize	= 0;
	found			= 0;
	championStartPoint	= NULL;

	/* Search the best fit memory block	*/
	for (count=0; count < (memory->memoryBlocks).top; count++){
		PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:		Start current block	= %p\n", (memory->memoryBlocks).block[count].start);
		PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:		Current block size	= %u Bytes\n", (memory->memoryBlocks).block[count].size);
		assert((memory->memoryBlocks).block[count].start != NULL);
		assert((memory->memoryBlocks).block[count].start >= memory->bottom);
		assert((memory->memoryBlocks).block[count].start <= memory->top);
		assert((memory->memoryBlocks).block[count].size > 0);
		if ((memory->memoryBlocks).block[count].size >= size){
			PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:			The size is enough\n");
			if (	(((memory->memoryBlocks).block[count].size - size) < championOverSize)	&&
				(found == 1)								){
				championOverSize	= ((memory->memoryBlocks).block[count].size - size);
				champion		= count;
				PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:			This is the current champion\n");
			}
			if (found == 0){
				championOverSize	= ((memory->memoryBlocks).block[count].size - size);
				champion		= count;
				PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:			This is the current champion\n");
			}
			found			= 1;
		} else {
			PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:			The size is not enough\n");
		}
	}
	
	if (found){
		/* We have found a memory block	*/
		assert((memory->memoryBlocks).block[champion].size >= size);

		/* Fetch the start point of the	*
		 * memory block found		*/
		championStartPoint	= (memory->memoryBlocks).block[champion].start;
		assert(championStartPoint != NULL);

		/* Compute the result of the 	*
		 * size of the memory block	*/
		(memory->memoryBlocks).block[champion].size	-= size;
		(memory->memoryBlocks).block[champion].start	+= size;
		assert((memory->memoryBlocks).block[champion].size >= 0);

		/* Check if we have to delete 	*
		 * the block			*/
		if ((memory->memoryBlocks).block[champion].size == 0){
			PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock:			The free block is empty, I have to delete it\n");
			/* Delete the block found	*/
			deleteMemoryBlock(&(memory->memoryBlocks), champion);
		}

		/* Return the start point of 	*
		 * the memory block found	*/
		PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock: Exit\n");
		return championStartPoint;
	} 
	/* We have not found a memory block	*/

	/* Return 				*/
	PDEBUG("GC MarkAndSweep: fetchBestFitMemoryBlock: Exit\n");
	return NULL;
}

void check_MemoryBlocks_for_one_slot (t_memory_blocks *memoryBlocks){
	JITNUINT	newSize;

	/* Assertions		*/
	assert(memoryBlocks != NULL);
	assert(memoryBlocks->top <= memoryBlocks->allocated);
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: Start\n");
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: 	Top		= %u\n", memoryBlocks->top);
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: 	Allocated	= %u\n", memoryBlocks->allocated);
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: 	Size		= %u Bytes\n", memoryBlocks->size);
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot:	Memory blocks address = %p\n", memoryBlocks->block);

	/* Initialize the variables	*/
	newSize	= 0;

	/* Check the max slot used	*/
	if (memoryBlocks->top > memoryBlocks->max){
		memoryBlocks->max	= memoryBlocks->top;
	}

	/* Check one slot free		*/
	if (memoryBlocks->top == memoryBlocks->allocated){
		
		/* I have to allocate a new slot	*/
		newSize	= (memoryBlocks->allocated + MEMORY_BLOCK_SIZE_STEP) * sizeof (t_memory_block);
		assert(newSize > 0);
		if (newSize > memoryBlocks->size){
			memoryBlocks->block		= gcRealloc (memoryBlocks->block, newSize);
			memoryBlocks->size		= newSize;
		}
		memoryBlocks->allocated		+= MEMORY_BLOCK_SIZE_STEP;
		
		/* Check the new memory			*/
		#ifdef DEBUG
		JITUINT32	count;
		PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot:		Memory blocks address = %p\n", memoryBlocks->block);
		PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot:		Erasing the new blocks\n");
		for (count=memoryBlocks->top; count < memoryBlocks->allocated; count++){
			PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot:			Erasing the new block %u\n", count);
			(memoryBlocks->block[count]).start	= NULL;
			(memoryBlocks->block[count]).size	= 0;
		}
		#endif
	}
	assert(memoryBlocks->block != NULL);
	assert(memoryBlocks->top < memoryBlocks->allocated);
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: 	Exit Allocated	= %u\n", memoryBlocks->allocated);
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: 	Exit Size	= %u Bytes\n", memoryBlocks->size);

	/* Return			*/
	PDEBUG("GC MarkAndSweep: check_MemoryBlocks_for_one_slot: Exit\n");
	return;
}

void printBitmap (char *prefix, JITNUINT *bitmap, JITUINT32 size){
	JITUINT32	count;

	/* Assertions		*/
	assert(prefix != NULL);
	assert(bitmap != NULL);
	PDEBUG("%s: Start\n", prefix);
	
	/* Print the bitmap	*/
	for (count=0; count<size; count++){
		PDEBUG("%s:	Word %d	= 0x%X\n", prefix, count, bitmap[count]);
	}

	/* Return		*/
	PDEBUG("%s: Exit\n", prefix);
	return;
}

void eraseFreeBlocks (t_memory *memory){
	JITUINT32	count;

	/* Assertions			*/
	assert(memory != NULL);
	assert((memory->memoryBlocks).block != NULL);
	
	/* Initialize the variables	*/
	count		= 0;

	/* Print the memory free blocks	*/
	for (count=0; count < (memory->memoryBlocks).top; count++){
		assert((memory->memoryBlocks).block[count].start != NULL);
		assert((memory->memoryBlocks).block[count].start >= memory->bottom);
		assert((memory->memoryBlocks).block[count].start <= memory->top);
		assert((memory->memoryBlocks).block[count].size > 0);
		memset((memory->memoryBlocks).block[count].start, 0, (memory->memoryBlocks).block[count].size);
	}

	/* Return		*/
	return ;
}

JITUINT32 printFreeBlocks (t_memory *memory){
	JITUINT32	count;
	JITUINT32	bytesFree;

	/* Assertions			*/
	assert(memory != NULL);
	assert((memory->memoryBlocks).block != NULL);
	PDEBUG("GC MarkAndSweep: printFreeBlocks: Start\n");
	
	/* Initialize the variables	*/
	count		= 0;
	bytesFree	= 0;

	/* Print the memory free blocks	*/
	for (count=0; count < (memory->memoryBlocks).top; count++){
		assert((memory->memoryBlocks).block[count].start != NULL);
		assert((memory->memoryBlocks).block[count].start >= memory->bottom);
		assert((memory->memoryBlocks).block[count].start <= memory->top);
		assert((memory->memoryBlocks).block[count].size > 0);
		PDEBUG("GC MarkAndSweep: printFreeBlocks: 	Free block %u\n", count);
		PDEBUG("GC MarkAndSweep: printFreeBlocks: 		Start point in memory			= %p\n", (memory->memoryBlocks).block[count].start);
		PDEBUG("GC MarkAndSweep: printFreeBlocks: 		Start point from the heap bottom	= %u\n", (memory->memoryBlocks).block[count].start - memory->bottom);
		PDEBUG("GC MarkAndSweep: printFreeBlocks: 		Size					= %u\n", (memory->memoryBlocks).block[count].size);
		bytesFree += (memory->memoryBlocks).block[count].size;
	}
	PDEBUG("GC MarkAndSweep: printFreeBlocks: 	Free bytes	= %u\n", bytesFree);

	/* Return		*/
	PDEBUG("GC MarkAndSweep: printFreeBlocks: Exit\n");
	return bytesFree;
}

JITUINT32 insertNewFreeBlock (t_memory *memory, t_memory_block *newBlock){
	JITUINT32	count;
	JITUINT32	count2;
	JITUINT32 	bytesSwept;

	/* Assertions			*/
	assert(memory != NULL);
	assert(newBlock != NULL);
	PDEBUG("GC MarkAndSweep: insertNewFreeBlock: Start\n");

	/* Initialize the variables	*/
	bytesSwept	= 0;
	count		= 0;
	count2		= 0;

	/* There is a potentially free block of	*
	 * memory. I have to check if it is not	*
	 * included in another one		*/
	for (count=0; count < (memory->memoryBlocks).top; count++){
		if (	(newBlock->start >= (memory->memoryBlocks).block[count].start)	&&
			(newBlock->start <= ((memory->memoryBlocks).block[count].start + (memory->memoryBlocks).block[count].size)) ){
			void 	*endNewBlock;
			void	*endOldBlock;
			PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 	The new block can be inserted into the following block\n");
			PDEBUG("GC MarkAndSweep: insertNewFreeBlock:			Start at %u from the heap\n", (memory->memoryBlocks).block[count].start - memory->bottom);
			PDEBUG("GC MarkAndSweep: insertNewFreeBlock:			Size	= %u\n", (memory->memoryBlocks).block[count].size);
			endNewBlock	= newBlock->start + newBlock->size;
			endOldBlock	= (memory->memoryBlocks).block[count].start + (memory->memoryBlocks).block[count].size;
			assert(endNewBlock != NULL);
			assert(endOldBlock != NULL);
			assert(endNewBlock >= memory->bottom);
			assert(endNewBlock <= memory->top);
			assert(endOldBlock >= memory->bottom);
			assert(endOldBlock <= memory->top);
			if (endNewBlock > endOldBlock){
				JITUINT32 overSize;
				/* I have to enlarge the old free block of memory	*/
				PDEBUG("GC MarkAndSweep: insertNewFreeBlock:			I have to enlarge the old free block\n");
						
				/* Compute the over size				*/
				overSize	= endNewBlock - endOldBlock;
				assert(overSize > 0);
				(memory->memoryBlocks).block[count].size += overSize;
				bytesSwept += overSize;
			}
			break;
		}
	}
	if (count == (memory->memoryBlocks).top){

		/* Add a new free block of memory	*/
		PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 	The new free block is inserted into the list\n");

		/* Check if there is a free slot in the	*
		 * stack of the free blocks		*/
		check_MemoryBlocks_for_one_slot (&(memory->memoryBlocks));
									
		/* Fill information of the new free block	*
		 * in the new slot of the stack of the free 	*
		 * blocks of memory				*/
		(memory->memoryBlocks).block[(memory->memoryBlocks).top].start	= newBlock->start;
		(memory->memoryBlocks).block[(memory->memoryBlocks).top].size	= newBlock->size;
		(memory->memoryBlocks).top++;
		bytesSwept += newBlock->size;
	}

	/* Delete redundant free blocks		*/
	PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 	Delete not usefull free blocks\n");
	for (count=0; count < (memory->memoryBlocks).top; count++){

		/* Check if the current memory block is not usefull.	*
		 * A memory block is not usefull if it can be included	*
		 * by another one					*/
		for (count2=0; count2 < (memory->memoryBlocks).top; count2++){
			if (	((memory->memoryBlocks).block[count].start >= (memory->memoryBlocks).block[count2].start) &&
				( count != count2 )									  &&
				((memory->memoryBlocks).block[count].start <= ((memory->memoryBlocks).block[count2].start + (memory->memoryBlocks).block[count2].size)) ){
				JITINT32	overSize;
				PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 		Found a not usefull free block\n");
				PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 			Start	= %u\n", (memory->memoryBlocks).block[count].start - memory->bottom);
				PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 			Size	= %u\n", (memory->memoryBlocks).block[count].size);

				overSize	= (memory->memoryBlocks).block[count2].size;
				overSize	-= ((memory->memoryBlocks).block[count].start - (memory->memoryBlocks).block[count2].start);
				overSize	-= (memory->memoryBlocks).block[count].size;
				overSize	= -overSize;
				if (overSize <= 0){
					/* The count free block is included into the count2 free block	*/
					PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 		It is totally included in the following free block:\n");
					PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 			Start	= %u\n", (memory->memoryBlocks).block[count2].start - memory->bottom);
					PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 			Size	= %u\n", (memory->memoryBlocks).block[count2].size);
					deleteMemoryBlock(&(memory->memoryBlocks), count);
				} else {
					/* The count free block is partially overlapped of the count2	*
					 * free block							*/
					assert(overSize > 0);
					PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 		It is partially overlapped of the following free block:\n");
					PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 			Start	= %u\n", (memory->memoryBlocks).block[count2].start - memory->bottom);
					PDEBUG("GC MarkAndSweep: insertNewFreeBlock: 			Size	= %u\n", (memory->memoryBlocks).block[count2].size);
					(memory->memoryBlocks).block[count2].size	+= overSize;
					deleteMemoryBlock(&(memory->memoryBlocks), count);
				}
			}
		}
	}

	/* Return				*/
	PDEBUG("GC MarkAndSweep: insertNewFreeBlock: Exit\n");
	return bytesSwept;
}

void deleteMemoryBlock(t_memory_blocks *memoryBlocks, JITUINT32 blockID){
	JITUINT32	count;

	/* Assertions		*/
	assert(memoryBlocks != NULL);
	assert(memoryBlocks->top <= memoryBlocks->allocated);
	assert(blockID < memoryBlocks->top);

	/* Delete the block	*/
	for (count=blockID; count < (memoryBlocks->top - 1); count++){
		memcpy(&(memoryBlocks->block[count]), &(memoryBlocks->block[count+1]), sizeof(t_memory_block));
	}
	(memoryBlocks->top)--;

	/* Return		*/
	return;
}
	
JITUINT32 mark_the_heap(t_memory *memory, JITNUINT *heapOccupance, JITNUINT *objectsReferenceMark, t_gc_behavior *gc, XanList *rootSets){
	XanListItem	*item;
	JITUINT32	count;
	JITUINT32	bytesMarked;
	JITUINT32	currentBytesMarked;
	JITUINT32	objectID;
	void		**objPtr;

	/* Assertions						*/
	assert(memory != NULL);
	assert(gc != NULL);
	assert(heapOccupance != NULL);
	assert(objectsReferenceMark != NULL);
	assert(rootSets != NULL);
	PDEBUG("GC MarkAndSweep: mark_the_heap: Start\n");

	/* Initialize the variables				*/
	bytesMarked	= 0;
	count		= 0;
	objectID	= 0;
	objPtr		= NULL;

	/* Delete the pointer within the root set equal to zero	*/
	item	= rootSets->first(rootSets);
	while (item != NULL){
		objPtr	= rootSets->data(rootSets, item);
		assert(objPtr != NULL);
		if ((*objPtr) == NULL) {
			XanListItem	*item2;
			item2		= rootSets->next(rootSets, item);
			rootSets->deleteItem(rootSets, item);
			item		= item2;
			continue;
		}
		item	= rootSets->next(rootSets, item);
	}

	/* Mark the heap starting from the root set		*/
	item	= rootSets->first(rootSets);
	while (item != NULL){
		XanListItem	*item2;
		
		/* Fetch the object pointer			*/
		objPtr			= rootSets->data(rootSets, item);
		assert(objPtr != NULL);
		assert((*objPtr) != NULL);

		/* Delete the clones of the current root from 	*
		 * the root sets				*/
		item2	= rootSets->next(rootSets, item);
		while (item2 != NULL){
			void	**objPtr2;
			objPtr2	= rootSets->data(rootSets, item2);
			assert(objPtr2 != NULL);
			if ((*objPtr2) == (*objPtr)){
				/*XanListItem	*item3;
				item3	= rootSets->next(rootSets, item2);
				rootSets->deleteItem(rootSets, item2);
				item2	= item3;*/
				rootSets->deleteItem(rootSets, item2);
				item2	= rootSets->next(rootSets, item);
				continue;
			}
			item2	= rootSets->next(rootSets, item2);
		}

		/* Mark the heap occupied by the object		*/
		PDEBUG("GC MarkAndSweep: mark_the_heap:	Mark the heap occupied by the object\n");
		currentBytesMarked	= markMemoryByObject(memory, *objPtr, heapOccupance, objectsReferenceMark, gc, rootSets);
		PDEBUG("GC MarkAndSweep: mark_the_heap:		Marked %u bytes\n", currentBytesMarked);
		bytesMarked		+= currentBytesMarked;
		PDEBUG("GC MarkAndSweep: mark_the_heap:		Totally marked bytes	= %u\n", bytesMarked);

		/* Fetch the next object			*/
		item			= rootSets->next(rootSets, item);
	}

	/* Return the bytes marked				*/
	PDEBUG("GC MarkAndSweep: mark_the_heap: Exit\n");
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
