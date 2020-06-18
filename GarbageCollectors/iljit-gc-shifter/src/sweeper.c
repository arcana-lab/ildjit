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
#include <internal_memory_manager.h>
#include <memory_manager.h>
#include <iljit_gc_shifter.h>
#include <iljit-utils.h>
#include <gc_shifter_general_tools.h>
#include <sweeper.h>
// End

JITINT16 internal_isReferenceAlreadyRefreshed(JITUINT32 byteNumber, JITNUINT *heapBitmap);
JITINT16 internal_checkLiveObjects (t_memory *memory, XanList *objectsLived);
void internal_setReferenceAsRefreshed (JITUINT32 byteNumber, JITNUINT *heapBitmap);

/**
 * @param lastByte From bottom to lastByte the heap is unmodified.
 */
void internal_adjustReferences (t_memory *memory, void *lastByte, JITNUINT shifting, XanList *objectsLived, t_gc_behavior *gc, XanList *rootSets);

JITUINT32 sweep_the_heap_from_top_to_bottom (t_memory *memory, XanList *rootSets, XanList *objectsLived, t_gc_behavior *gc){
	JITUINT32	bytesSweeped;
	JITINT32	count;
	JITINT32	count2;
	JITINT32	count4;
	JITINT32	startFreeBytes;
	JITNUINT	flag;
	void		*startMarkBlock;
	void		*endMarkBlock;

	/* Assertions			*/
	assert(memory != NULL);
	assert(rootSets != NULL);
	assert(objectsLived != NULL);
	assert(gc != NULL);
	PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: Start\n");
	PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 	Heap size	= %u bytes\n", (JITUINT32) (JITNUINT) (memory->top - memory->bottom));

	/* Print the marked map		*/
	#ifdef PRINTDEBUG
	PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 	Marked map\n");
	startFreeBytes	= -1;
	for (count=0, count4=0; count < memory->heapOccupanceWords; count++){
		for (count2=0; count2 < (sizeof(JITNUINT) * 8); count2++, count4++){
			if (count4 >= (memory->top - memory->bottom)) {
				break;
			}
			flag	= 0x1;
			flag	= flag << count2;
			if ((memory->heapOccupance[count] & flag) == 0x0){
				assert(!isMarkedByteOfHeap(memory, count4));
				if (startFreeBytes == -1){
					startFreeBytes	= count4;
				}
				assert(startFreeBytes >= 0);
				assert(startFreeBytes <= (memory->top - memory->bottom));
			} else {
				assert(isMarkedByteOfHeap(memory, count4));
				if (startFreeBytes >= 0){
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 		Free blocks: 	byte %u -> byte %u\n", startFreeBytes, count4 - 1);
					startFreeBytes	= -1;
				}
				assert(startFreeBytes == -1);
			}
		}
	}
	if (startFreeBytes != -1){
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 		Free blocks: 	byte %u -> byte %u\n", startFreeBytes, count4 - 1);
	}
	#endif

	/* Sweep the heap		*/
	bytesSweeped		= 0;
	startFreeBytes		= -1;
	startMarkBlock		= NULL;
	endMarkBlock		= NULL;
	count4			= ((sizeof(JITNUINT)*8) * (memory->heapOccupanceWords)) - 1;
	PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 	Sweep the heap\n");
	for (count=(memory->heapOccupanceWords - 1); count >=  0; count--){
		for (count2=((sizeof(JITNUINT)*8) - 1); (count2 >= 0) && (count4 >= 0); count2--, count4--){
			
			/* Check that we are in the heap	*/
			if (count4 >= (memory->top - memory->bottom)){
				JITUINT32	overflow;
				PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			It is out of the heap\n");
				overflow	= count4 - (memory->top - memory->bottom);
				PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Overflow 	= %u bytes\n", overflow);
				assert(overflow > 0);
				assert(overflow <= ((sizeof(JITNUINT)*8) - 2));
				count2		-= overflow;
				count4		-= overflow;
				assert(count2 > 0);
				continue;
			}
			assert(count4 < (memory->top - memory->bottom));
			assert(((count * sizeof(JITNUINT) * 8) + count2) == count4);

			/* Checking if the byte number count4 of*
			 * the heap is marked			*/
			flag	= 0x1;
			flag	= flag << count2;
			if ((memory->heapOccupance[count] & flag) != 0x0){
				
				/* The byte (count * 8) + count2 is mark	*/
				if (	(startMarkBlock == NULL)	&&
					(endMarkBlock == NULL)		){

					/* This is the first byte marked	*/
					assert(isMarkedByteOfHeap(memory, count4));
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 		First byte marked (byte %u)\n", count4);
					assert(endMarkBlock == NULL);
					startMarkBlock	= (memory->bottom + count4 - bytesSweeped);
					assert(isMarkedByteOfHeap(memory, startMarkBlock - memory->bottom + bytesSweeped));
					if (bytesSweeped == 0){
						assert(startFreeBytes == -1);
						startFreeBytes	= count4 + 1;
						assert(startFreeBytes >= 0);
						assert(startFreeBytes <= (memory->top - memory->bottom));
						assert(!isMarkedByteOfHeap(memory, startFreeBytes));
						memory->heap	= memory->bottom + startFreeBytes;
						assert(!isMarkedByteOfHeap(memory, memory->heap - memory->bottom));
					}
					assert(startMarkBlock != NULL);
					assert(endMarkBlock == NULL);
				} else if (endMarkBlock != NULL){
					JITUINT32	sizeRight;	/* Store how many bytes there are from the	*
									 * current first free byte of the heap to the 	*
									 * end of the marked heap.			*/
					JITUINT32	shifting;	/* Store how many bytes the right hand of the 	*
									 * heap has to shift left			*/
					void		*startFreeBlock;

					/* This is the first byte marked after another marked block and a following	*
					 * free block									*/
					assert(startMarkBlock == NULL);
					assert(isMarkedByteOfHeap(memory, count4));
					assert(isMarkedByteOfHeap(memory, endMarkBlock - memory->bottom + bytesSweeped));
					assert(!isMarkedByteOfHeap(memory, endMarkBlock - 1 - memory->bottom + bytesSweeped));
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 		Found another new block of byte marked (byte %u)\n", count4);

					/* Compact the heap				*/
					assert((memory->bottom + count4 - bytesSweeped) < endMarkBlock);
					sizeRight	= (JITUINT32) (memory->top - endMarkBlock + bytesSweeped);
					assert(sizeRight > 0);
					assert(sizeRight < (memory->top - memory->bottom));
					assert(sizeRight == (memory->top - endMarkBlock));
					startFreeBlock	= memory->bottom + count4 + 1 - bytesSweeped;
					assert(startFreeBlock != NULL);
					assert(startFreeBlock >= memory->bottom);
					assert(startFreeBlock < memory->top);
					assert(!isMarkedByteOfHeap(memory, startFreeBlock - memory->bottom + bytesSweeped));
					shifting	= (endMarkBlock - startFreeBlock);
					assert(shifting > 0);
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 		Shifting the memory block %u of %u bytes to the left (%u --> %u)\n", (JITUINT32) (JITNUINT) (endMarkBlock - memory->bottom), shifting, (JITUINT32) (JITNUINT) (endMarkBlock - memory->bottom), (JITUINT32) (JITNUINT) (startFreeBlock - memory->bottom));
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Memory block with heap expanded (%u --> %u)\n", (JITUINT32) (JITNUINT) (endMarkBlock - memory->bottom + bytesSweeped), (JITUINT32) (JITNUINT) (startFreeBlock - memory->bottom + bytesSweeped));
					assert((memory->bottom + count4) >= memory->bottom);
					assert((memory->bottom + count4) <= memory->top);
					assert(isMarkedByteOfHeap(memory, endMarkBlock - memory->bottom + bytesSweeped));
					assert(startFreeBlock < endMarkBlock);
					assert(endMarkBlock - shifting == startFreeBlock);
					assert(startFreeBlock >= memory->bottom);
					assert(startFreeBlock <= memory->top);
					#ifdef DEBUG
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Check the live objects\n");
					assert(internal_checkLiveObjects(memory, objectsLived));
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Print the live objects\n");
					printLiveObjects (objectsLived, gc);
					#endif

					/* Refresh the pointers between objects	*/
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Adjust the references of the live objects\n");
					assert((endMarkBlock - shifting) >= memory->bottom);
					internal_adjustReferences(memory, endMarkBlock - 1, shifting, objectsLived, gc, rootSets);

					/* Shift the right side of the heap of shifting bytes to left	*/
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Compact the heap (Copy %u bytes from %u to %u)\n", sizeRight, (JITUINT32)(JITNUINT)(endMarkBlock - memory->bottom), (JITUINT32)(JITNUINT)(startFreeBlock - memory->bottom));
					assert(endMarkBlock > startFreeBlock);
					assert((endMarkBlock - startFreeBlock) == shifting);
					#ifdef PRINTDEBUG
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 				Heap before compacting\n");
					dumpHeap(memory);
					#endif
					memmove(startFreeBlock, endMarkBlock, sizeRight);
					memory->heap	-= shifting;
					#ifdef PRINTDEBUG
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 				Heap after compacting\n");
					dumpHeap(memory);
					#endif
					assert(memory->heap >= memory->bottom);
					assert(memory->heap <= memory->top);
					bytesSweeped	+= shifting;
					assert(bytesSweeped > 0);
					assert(bytesSweeped <= ((JITNUINT)(memory->top - memory->bottom)));
					endMarkBlock	= NULL;
					startMarkBlock	= NULL;
					#ifdef DEBUG
					/* Print the live objects		*/
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Print the live objects\n");
					printLiveObjects (objectsLived, gc);

					/* Check the live objects			*/
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Check the live objects\n");
					assert(internal_checkLiveObjects(memory, objectsLived));				
					#endif
				}
			} else {

				/* The byte (count * 8) + count2 is not mark. 	*/
				if (startMarkBlock != NULL){
					assert(endMarkBlock == NULL);
					PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 		Last byte marked (byte %u)\n", count4 + 1);
					endMarkBlock	= (memory->bottom + count4 + 1 - bytesSweeped);
					assert(isMarkedByteOfHeap(memory, endMarkBlock - memory->bottom + bytesSweeped));
					startMarkBlock	= NULL;
				}
			}
		}
	}
	assert(internal_checkLiveObjects(memory, objectsLived));

	/* Check if exist a block at the top of the heap	*/
	if (endMarkBlock != NULL){
		JITUINT32	shifting;
		JITUINT32	sizeRight;
		assert(startMarkBlock == NULL);
		assert(isMarkedByteOfHeap(memory, endMarkBlock - memory->bottom + bytesSweeped));
		sizeRight	= (JITUINT32) (memory->top - endMarkBlock - bytesSweeped + 1);
		assert(sizeRight > 0);
		shifting	= (endMarkBlock - memory->bottom);
		assert(shifting > 0);
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Found the last trunk of the heap as free\n");
		#ifdef DEBUG
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Check the live objects\n");
		assert(internal_checkLiveObjects(memory, objectsLived));
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Print the live objects\n");
		printLiveObjects (objectsLived, gc);
		#endif

		/* Refresh the pointers between objects	*/
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 				Adjust the references of the live objects\n");
		assert(memory->bottom >= ((endMarkBlock - 1) - shifting));
		internal_adjustReferences(memory, endMarkBlock - 1, shifting, objectsLived, gc, rootSets);
	
		/* Shift the right side of the heap of shifting bytes to left	*/
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 				Compact the heap\n");
		memmove(memory->bottom, endMarkBlock, sizeRight);
		endMarkBlock	= NULL;
		memory->heap	-= shifting;
		bytesSweeped	+= shifting;
		assert(bytesSweeped > 0);
		assert(bytesSweeped <= ((JITNUINT)(memory->top - memory->bottom)));
		assert(memory->heap >= memory->bottom);
		assert(memory->heap <= memory->top);
		assert(internal_checkLiveObjects(memory, objectsLived));
		#ifdef DEBUG
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Check the live objects\n");
		assert(internal_checkLiveObjects(memory, objectsLived));
		PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 			Print the live objects\n");
		printLiveObjects (objectsLived, gc);
		#endif
	}
	if (startFreeBytes != -1){
		bytesSweeped	+= ((memory->top - memory->bottom) - startFreeBytes);
	}
	assert(internal_checkLiveObjects(memory, objectsLived));
	PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: 	Sweeped %u bytes\n", bytesSweeped);
	assert(endMarkBlock == NULL);

	/* Erase the free memory				*/
	assert((memory->heap + bytesSweeped == memory->top));
	memset(memory->heap, 0, bytesSweeped);
	assert(internal_checkLiveObjects(memory, objectsLived));

	/* Return						*/
	PDEBUG("GC Shifter: sweep_the_heap_from_top_to_bottom: Exit\n");
	return bytesSweeped;
}

JITUINT32 sweep_the_heap_from_bottom_to_top (t_memory *memory, XanList *rootSets, XanList *objectsLived, t_gc_behavior *gc){
	JITUINT32	bytesSweeped;
	JITUINT32	count;
	JITUINT32	count2;
	JITUINT32	count4;
	JITNUINT	flag;
	void		*startFreeBlock;

	/* Assertions			*/
	assert(memory != NULL);
	assert(rootSets != NULL);
	assert(objectsLived != NULL);
	assert(gc != NULL);

	bytesSweeped		= 0;
	startFreeBlock		= NULL;
	for (count=0, count4=0; count < memory->heapOccupanceWords; count++){
		for (count2=0; count2 < (sizeof(JITNUINT) * 8); count2++, count4++){

			/* Check if there a memory to check	*/
			if (count4 >= (memory->top - memory->bottom)) {
				break;
			}
			
			/* Checking if the byte number count4 of*
			 * the heap is marked			*/
			flag	= 0x1;
			flag	= flag << count2;
			if ((memory->heapOccupance[count] & flag) == 0x0){
				
				/* The byte (count * 8) + count2 is not mark	*/
				if (startFreeBlock == NULL){
					startFreeBlock	= (memory->bottom + count4 - bytesSweeped);
				}
			} else {

				/* The byte (count * 8) + count2 is mark, so I 	*
				 * have to check if I have to shift some block	*
				 * of memory					*/
				if (startFreeBlock != NULL){
					JITNUINT	sizeRight;	/* Store how many bytes there are from the	*
									 * current first marked byte of the heap to the *
									 * end of the heap				*/
					JITNUINT	shifting;	/* Store how many bytes the right hand of the 	*
									 * heap has to shift left			*/
					assert((memory->bottom + count4 - bytesSweeped) > startFreeBlock);
					sizeRight	= (JITNUINT) (memory->top - (memory->bottom + count4 - bytesSweeped));
					assert(sizeRight > 0);
					shifting	= (memory->bottom + count4 - bytesSweeped - startFreeBlock);
					assert(shifting > 0);
					PDEBUG("GC Shifter: sweep_the_heap_from_bottom_to_top: 			Found a free memory at %p (from heap bottom = %u) with size %u\n", startFreeBlock, (JITUINT32) (JITNUINT) (startFreeBlock - memory->bottom), (JITUINT32) shifting);
					assert((memory->bottom + count4) >= memory->bottom);
					assert((memory->bottom + count4) <= memory->top);
					assert((memory->bottom + count4 + sizeRight - bytesSweeped) >= memory->bottom);
					assert((memory->bottom + count4 + sizeRight - bytesSweeped) <= memory->top);

					/* Refresh the pointers between objects	*/
					PDEBUG("GC Shifter: sweep_the_heap_from_bottom_to_top: 				Adjust the references of the live objects\n");
					internal_adjustReferences(memory, startFreeBlock - 1, shifting, objectsLived, gc, rootSets);

					/* Shift the right side of the heap of shifting bytes to left	*/
					PDEBUG("GC Shifter: sweep_the_heap_from_bottom_to_top: 				Compact the heap\n");
					memmove(startFreeBlock, (memory->bottom + count4 - bytesSweeped), sizeRight);
					memory->heap	-= shifting;
					assert(memory->heap >= memory->bottom);
					assert(memory->heap <= memory->top);
					bytesSweeped	+= shifting;
					assert(bytesSweeped > 0);
					assert(bytesSweeped <= ((JITNUINT)(memory->top - memory->bottom)));
					startFreeBlock	= NULL;
		
					/* Print the live objects		*/
					#ifdef DEBUG
					PDEBUG("GC Shifter: sweep_the_heap_from_bottom_to_top: 			 	Print the live objects\n");
					printLiveObjects (objectsLived, gc);
					#endif
				}
				assert(startFreeBlock == NULL);
			}
		}
	}

	/* Check if exist a block at the top of the heap	*/
	if (startFreeBlock != NULL){
		JITNUINT	shifting;
		memory->heap	= startFreeBlock;
		shifting	= (memory->bottom + count4 - bytesSweeped - startFreeBlock);
		assert(shifting > 0);
		bytesSweeped	+= shifting;
		assert(bytesSweeped > 0);
		assert(bytesSweeped <= ((JITNUINT)(memory->top - memory->bottom)));
		PDEBUG("GC Shifter: sweep_the_heap_from_bottom_to_top: 			Found the last trunk of the heap (starting from %p, from heap bottom = %u) as free\n", startFreeBlock, (JITUINT32) (JITNUINT) (startFreeBlock - memory->bottom));
		assert(memory->heap >= memory->bottom);
		assert(memory->heap <= memory->top);
		startFreeBlock	= NULL;
	}
	assert(startFreeBlock == NULL);

	/* Erase the free memory				*/
	memset(memory->heap, 0, bytesSweeped);

	/* Return						*/
	return bytesSweeped;
}

JITUINT32 sweep_the_objects_references (t_memory *memory, JITNUINT *objectsReferenceMark, JITUINT32 objectsReferenceLength){
	JITUINT32	count;
	JITUINT32	count2;
	JITUINT32	trigger;
	JITUINT32	objFlag;
	JITUINT32	objReferencesSweeped;
	JITUINT16	modified;
	#ifdef DEBUG
	JITUINT32	oldTop;
	#endif

 	/* Assertions						*/
	assert(memory != NULL);
	assert(objectsReferenceMark != NULL);
	PDEBUG("GC Shifter: sweep_the_objects_references: Start\n");

	/* Set NULL the objects dead				*/
	#ifdef DEBUG
	oldTop	= (memory->objectsReference).top;
	#endif
	PDEBUG("GC Shifter: sweep_the_objects_references: 	Set to NULL the objects reference not marked\n");
	for (count=0, objReferencesSweeped=0; count < (memory->objectsReference).top; count++){
		assert((memory->objectsReference).objects[count] != NULL);
		trigger		= count / (sizeof(JITNUINT) * 8);
		assert(trigger < objectsReferenceLength);
		objFlag		= 0x1;
		objFlag		= objFlag << (count % (sizeof(JITNUINT)*8));

		/* Check if the current object reference is marked	*/
		if ((objectsReferenceMark[trigger] & objFlag) == 0x0){
			PDEBUG("GC Shifter: sweep_the_objects_references: 		The object reference %u is not mark\n", count);
			objReferencesSweeped++;
			(memory->objectsReference).objects[count]	= NULL;
		}
	}
	
	/* Delete the objects reference not NULL		*/
	modified	= 1;
	PDEBUG("GC Shifter: sweep_the_objects_references: 	Delete the objects reference not marked\n");
	while (modified){
		modified	= 0;
		for (count=0; count < (memory->objectsReference).top; count++){
			if ((memory->objectsReference).objects[count] == NULL){
				JITUINT32	delta;
		
				/* Compute the delta			*/
				delta	= 1;
				for (count2=count+1; count2 < (memory->objectsReference).top; count2++){
					if ((memory->objectsReference).objects[count2] == NULL) delta++;
					else break;
				}
				assert(delta > 0);

				/* Move all the objects references after*
				 * the delta objects equals to NULL, 	*
				 * delta slot before the their old	*
				 * position				*/
				#ifdef DEBUG
				assert((memory->objectsReference).objects[count] == NULL);
				if (count + delta < (memory->objectsReference).top){
					assert((memory->objectsReference).objects[count+delta] != NULL);
				}
				#endif
				memmove(&((memory->objectsReference).objects[count]), &((memory->objectsReference).objects[count + delta]), sizeof(void *) * ((memory->objectsReference).top - (count+delta)));
				(memory->objectsReference).top	-= delta;
				modified	= 1;
				break;
			}
		}
	}
	#ifdef DEBUG
	assert((oldTop - (memory->objectsReference).top) == objReferencesSweeped);
	#endif

	/* Return						*/
	PDEBUG("GC Shifter: sweep_the_objects_references: Exit\n");
	return objReferencesSweeped;
}

void internal_adjustReferences (t_memory *memory, void *lastByte, JITNUINT shifting, XanList *objectsLived, t_gc_behavior *gc, XanList *rootSets){
	XanList		*referencesPtr;
	XanListItem	*item;
	XanListItem	*item2;
	JITUINT32	count;
	JITNUINT	*heapBitmap;
	JITUINT32	heapBitmapSize;
	void		*objPtr;
	#ifdef DEBUG
	XanList		*referencesList;
	void		**reference;
	XanList		*oldObjectsList;
	void		*oldCloneFunction;
	#endif

	/* Assertions				*/
	assert(memory != NULL);
	assert(rootSets != NULL);
	assert(lastByte != NULL);
	assert(gc != NULL);
	assert(objectsLived != NULL);
	assert(shifting > 0);
	assert((lastByte - shifting) >= memory->bottom);
	PDEBUG("GC Shifter: adjustReferences: Start\n");
	PDEBUG("GC Shifter: adjustReferences: 	Shift references that points from %u to the top of the heap\n", (JITUINT32) (JITNUINT) ((lastByte + 1) - memory->bottom));
	PDEBUG("GC Shifter: adjustReferences: 	Bytes to shift	= %u\n", (JITUINT32) shifting);

	/* Allocate the heap bitmap		*/
	heapBitmapSize	= (JITUINT32) (((JITUINT32)(memory->top - memory->bottom)) / sizeof(JITNUINT)) + 1;
	assert(heapBitmapSize > 0);
	heapBitmap	= gcAlloc(heapBitmapSize);
	assert(heapBitmap != NULL);
	memset(heapBitmap, 0, heapBitmapSize);

	/* Print the live objects		*/
	#ifdef PRINTDEBUG
	PDEBUG("GC Shifter: adjustReferences: 	Print the live objects\n");
	printShallowLiveObjects(objectsLived, gc);
	PDEBUG("GC Shifter: adjustReferences: 	Print the root sets\n");
	printRootSets(rootSets, memory);
	#endif

	/* Allocate the list of originally 	*
	 * object				*/
	#ifdef DEBUG
	oldCloneFunction	= objectsLived->getCloneFunction(objectsLived);
	objectsLived->setCloneFunction(objectsLived, NULL);
	oldObjectsList	= objectsLived->cloneList(objectsLived);
	assert(oldObjectsList != NULL);
	objectsLived->setCloneFunction(objectsLived, oldCloneFunction);
	#endif

	/* Check the live objects		*/
	assert(internal_checkLiveObjects(memory, objectsLived));
	
	/* Adjust the reference on root sets	*/
	PDEBUG("GC Shifter: adjustReferences: 	Refresh the references of the root sets\n");
	count	= 0;
	item	= rootSets->first(rootSets);
	while (item != NULL){
		void 	**objPtrPtr;
		objPtrPtr	= rootSets->data(rootSets, item);
		assert(objPtrPtr != NULL);
		assert(rootSets->equalsInstancesNumber(rootSets, objPtrPtr) == 1);
		if ((*objPtrPtr) != NULL){
			if (	((lastByte != NULL) && ((*objPtrPtr) > lastByte))	||
				(lastByte == NULL)					){
					if (	(START_ADDRESS(*objPtrPtr) >= memory->bottom)		&&
						(START_ADDRESS(*objPtrPtr) <= memory->top)		){

						/* Check the live objects		*/
						assert(internal_checkLiveObjects(memory, objectsLived));

						/* Refresh the reference		*/
						(*objPtrPtr)	-= shifting;

						/* Check the live objects		*/
						assert(internal_checkLiveObjects(memory, objectsLived));
						if (	(START_ADDRESS((void *) objPtrPtr) >= memory->bottom)	&&
							(START_ADDRESS((void *) objPtrPtr) <= memory->top)		){
							PDEBUG("GC Shifter: adjustReferences: 		Refresh the reference %u of the heap\n", (JITUINT32)(JITNUINT)(START_ADDRESS((void *)objPtrPtr) - memory->bottom));
							assert(!internal_isReferenceAlreadyRefreshed((JITUINT32)(((void *) objPtrPtr) - memory->bottom), heapBitmap));
							internal_setReferenceAsRefreshed((JITUINT32)(((void *) objPtrPtr) - memory->bottom), heapBitmap);
							assert(internal_isReferenceAlreadyRefreshed((JITUINT32)(((void *)objPtrPtr) - memory->bottom), heapBitmap));
						}
						assert(START_ADDRESS(*objPtrPtr) >= memory->bottom);
						assert(START_ADDRESS(*objPtrPtr) <= memory->top);
					}
			}
			assert((*objPtrPtr) != NULL);
		}
		item		= rootSets->next(rootSets, item);
	}

	/* Print the live objects		*/
	#ifdef PRINTDEBUG
	PDEBUG("GC Shifter: adjustReferences: 	Print the live objects\n");
	printShallowLiveObjects(objectsLived, gc);
	PDEBUG("GC Shifter: adjustReferences: 	Print the root sets\n");
	printRootSets(rootSets, memory);
	#endif

	/* Check the live objects		*/
	assert(internal_checkLiveObjects(memory, objectsLived));

	/* Adjust the reference between objects	*/
	PDEBUG("GC Shifter: adjustReferences: 	Refresh the references between live objects\n");
	item	= objectsLived->first(objectsLived);
	while (item != NULL){

		/* Fetch the object pointer			*/
		objPtr			= objectsLived->data(objectsLived, item);
		PDEBUG("GC Shifter: adjustReferences: 		Live object	= %u\n", (JITUINT32)(JITNUINT)(START_ADDRESS(objPtr) - memory->bottom));
		assert(objPtr != NULL);
		assert(START_ADDRESS(objPtr) >= memory->bottom);
		assert(START_ADDRESS(objPtr) <= memory->top);
		
		/* Adjust the reference stored inside the 	*
		 * current object				*/
		PDEBUG("GC Shifter: adjustReferences: 			Adjust the reference stored inside the current object\n");
		referencesPtr		= gc->getReferencedObject(objPtr);
		assert(referencesPtr != NULL);
		#ifdef DEBUG
		referencesList	= xanListNew(gcAlloc, gcFree, NULL);
		assert(referencesList != NULL);
		#endif
		item2			= referencesPtr->first(referencesPtr);
		while (item2 != NULL){
			void	**objPtrPtr;
			objPtrPtr	= referencesPtr->data(referencesPtr, item2);
			assert(objPtrPtr != NULL);
			assert(((void *)objPtrPtr) >= objPtr);
			assert(((void *)objPtrPtr) < (objPtr + gc->sizeObject(objPtr)));
			PDEBUG("GC Shifter: adjustReferences: 				Check the reference %u\n", (JITUINT32)(JITNUINT)((*objPtrPtr) - memory->bottom));
			#ifdef DEBUG
			reference	= (void **) gcAlloc(sizeof(void *));
			assert(reference != NULL);
			(*reference)	= (*objPtrPtr);
			referencesList->append(referencesList, reference);
			#endif
			if ((*objPtrPtr) != NULL){
				if (	((lastByte != NULL) && ((*objPtrPtr) > lastByte))	||
					(lastByte == NULL)					){
					
					/* Check if we have already refresh this pointer	*/
					if (	(((void *) objPtrPtr) >= memory->bottom)	&&
						(((void *) objPtrPtr) <= memory->top)		){
						if (!internal_isReferenceAlreadyRefreshed((JITUINT32)(((void *)objPtrPtr) - memory->bottom), heapBitmap)){
							PDEBUG("GC Shifter: adjustReferences: 					Refresh the reference %u\n", (JITUINT32)(JITNUINT)((*objPtrPtr) - memory->bottom));
							(*objPtrPtr)	-= shifting;
							PDEBUG("GC Shifter: adjustReferences: 						New value	= %u\n", (JITUINT32)(JITNUINT)((*objPtrPtr) - memory->bottom));
							internal_setReferenceAsRefreshed((JITUINT32)(((void *)objPtrPtr) - memory->bottom), heapBitmap);
						}
						assert(internal_isReferenceAlreadyRefreshed((JITUINT32)(((void *)objPtrPtr) - memory->bottom), heapBitmap));
					}
				}
				assert((*objPtrPtr) >= memory->bottom);
				assert((*objPtrPtr) <= memory->top);
			}
			item2		= referencesPtr->next(referencesPtr, item2);
		}
		#ifdef DEBUG
		XanListItem	*item3;
		PDEBUG("GC Shifter: adjustReferences: 			Check the reference refreshed stored inside the current object\n");
		assert(referencesList->length(referencesList) == referencesPtr->length(referencesPtr));
		referencesPtr->destroyList(referencesPtr);
		referencesPtr		= gc->getReferencedObject(objPtr);
		assert(referencesPtr != NULL);
		item3			= referencesList->first(referencesList);
		item2			= referencesPtr->first(referencesPtr);
		while (item2 != NULL){
			void	**objPtrPtr;
			void	**oldObjPtrPtr;
			assert(item3 != NULL);
			objPtrPtr	= referencesPtr->data(referencesPtr, item2);
			assert(objPtrPtr != NULL);
			oldObjPtrPtr	= referencesList->data(referencesList, item3);
			assert(oldObjPtrPtr != NULL);
			assert(oldObjPtrPtr != objPtrPtr);
			assert(((void *)objPtrPtr) >= objPtr);
			assert(((void *)objPtrPtr) < (objPtr + gc->sizeObject(objPtr)));
			if ((*objPtrPtr) != NULL){
				if (	((lastByte != NULL) && ((*objPtrPtr) > lastByte))	||
					(lastByte == NULL)					){
					if (	(((void *) objPtrPtr) >= memory->bottom)	&&
						(((void *) objPtrPtr) <= memory->top)		){
						assert((*objPtrPtr) + shifting == (*oldObjPtrPtr));
					}
				}
			}
			item2		= referencesPtr->next(referencesPtr, item2);
			item3		= referencesList->next(referencesList, item3);
		}
		referencesList->destroyListAndData(referencesList);
		#endif
		referencesPtr->destroyList(referencesPtr);

		/* Print the object references			*/
		#ifdef PRINTDEBUG
		PDEBUG("GC Shifter: adjustReferences: 			Print the current object references\n");
		printLiveObject(objPtr, gc);
		#endif

		/* Adjust the current object pointer		*/
		PDEBUG("GC Shifter: adjustReferences: 			Adjust the current object reference\n");
		assert(objPtr >= memory->bottom);
		assert(objPtr <= memory->top);
		if (	((lastByte != NULL) && (objPtr > lastByte))	||
			(lastByte == NULL)				){
			void		**objPtrPtr;
			objPtrPtr	= objectsLived->getSlotData(objectsLived, item);
			assert(objPtrPtr != NULL);
			assert((*objPtrPtr) == objPtr);
			assert(objPtr >= memory->bottom);
			assert(objPtr <= memory->top);
			#ifdef PRINTDEBUG
			PDEBUG("GC Shifter: adjustReferences: 				Print the live objects\n");
			printShallowLiveObjects(objectsLived, gc);
			#endif
			(*objPtrPtr)	-= shifting;
			#ifdef PRINTDEBUG
			PDEBUG("GC Shifter: adjustReferences: 				Print the live objects\n");
			printShallowLiveObjects(objectsLived, gc);
			#endif
		}
		#ifdef DEBUG
		objPtr			= objectsLived->data(objectsLived, item);
		PDEBUG("GC Shifter: adjustReferences: 			New reference of the current object	= %u\n", (JITUINT32)(JITNUINT)(objPtr - memory->bottom));
		assert(objPtr != NULL);
		assert(objPtr >= memory->bottom);
		assert(objPtr <= memory->top);
		#endif

		/* Fetch the next object			*/
		item			= objectsLived->next(objectsLived, item);
	}
	assert(internal_checkLiveObjects(memory, objectsLived));

	/* Print the live objects		*/
	#ifdef PRINTDEBUG
	PDEBUG("GC Shifter: adjustReferences: 	Print the live objects\n");
	printShallowLiveObjects(objectsLived, gc);
	#endif

	/* Adjust the references stored inside the 	*
	 * objects allocated memory			*/
	PDEBUG("GC Shifter: adjustReferences: 	Refresh the references stored inside the objects allocated memory\n");
	for (count=0; count < (memory->objectsReference).top; count++){
		objPtr	= (memory->objectsReference).objects[count];
		assert(objPtr != NULL);
		assert(objPtr >= memory->bottom);
		assert(objPtr <= memory->top);
		if (	((lastByte != NULL) && (objPtr > lastByte))	||
			(lastByte == NULL)				){
			PDEBUG("GC Shifter: adjustReferences: 		Refresh the object reference from %u to %u\n", (JITUINT32)(JITNUINT)(objPtr - memory->bottom), (JITUINT32)(JITNUINT)(objPtr - shifting - memory->bottom));
			((memory->objectsReference).objects[count])	-= shifting;
			assert((objPtr - shifting) == ((memory->objectsReference).objects[count]));
		}
	}

	/* Check the live objects			*/
	assert(internal_checkLiveObjects(memory, objectsLived));
	
	/* Print the live objects		*/
	#ifdef PRINTDEBUG
	PDEBUG("GC Shifter: adjustReferences: 	Print the live objects\n");
	printShallowLiveObjects(objectsLived, gc);
	PDEBUG("GC Shifter: adjustReferences: 	Print the root sets\n");
	printRootSets(rootSets, memory);
	PDEBUG("GC Shifter: adjustReferences: 	Print the new references of the old address of live objects\n");
	printLiveObjects(oldObjectsList, gc);
	#endif

	/* Free the memory			*/
	gcFree(heapBitmap);
	#ifdef DEBUG
	oldObjectsList->destroyList(oldObjectsList);
	#endif

	/* Return				*/
	PDEBUG("GC Shifter: adjustReferences: Exit\n");
	return;
}

void internal_setReferenceAsRefreshed (JITUINT32 byteNumber, JITNUINT *heapBitmap){
	JITNUINT	flag;
	JITINT32	count;
	JITINT32	count2;

	/* Assertions				*/
	assert(heapBitmap != NULL);

	count	= byteNumber / (sizeof(JITNUINT) * 8);
	assert(count >= 0);
	count2	= byteNumber - (count * (sizeof(JITNUINT) * 8));
	assert(count2 >= 0);
	assert(count2 < (sizeof(JITNUINT) * 8));
	assert(byteNumber == ((count * sizeof(JITNUINT) * 8) + count2));
	flag	= 0x1;
	flag	= flag << count2;
	heapBitmap[count] 	|= flag;
}

JITINT16 internal_isReferenceAlreadyRefreshed (JITUINT32 byteNumber, JITNUINT *heapBitmap){
	JITNUINT	flag;
	JITINT32	count;
	JITINT32	count2;

	/* Assertions				*/
	assert(heapBitmap != NULL);

	count	= byteNumber / (sizeof(JITNUINT) * 8);
	assert(count >= 0);
	count2	= byteNumber - (count * (sizeof(JITNUINT) * 8));
	assert(count2 >= 0);
	assert(count2 < (sizeof(JITNUINT) * 8));
	assert(byteNumber == ((count * sizeof(JITNUINT) * 8) + count2));
	flag	= 0x1;
	flag	= flag << count2;
	if ((heapBitmap[count] & flag) != 0x0){
		return 1;
	}
	return 0;
}

JITINT16 internal_checkLiveObjects (t_memory *memory, XanList *objectsLived){
	XanListItem	*item;
	void		*objPtr;
	JITUINT32	objID;

	/* Assertions			*/
	assert(memory != NULL);
	assert(objectsLived != NULL);
	item	= objectsLived->first(objectsLived);
	while (item != NULL){

		/* Fetch the object pointer			*/
		objPtr			= objectsLived->data(objectsLived, item);
		
		/* Check the liveness of the object		*/
		if (objPtr == NULL) return 0;
		if(START_ADDRESS(objPtr) < memory->bottom) return 0;
		if (START_ADDRESS(objPtr) > memory->top) return 0;
		if (!isAnObjectAllocated(memory, objPtr, &objID)) return 0;
		if (objectsLived->equalsInstancesNumber(objectsLived, objPtr) != 1) return 0;
		
		/* Fetch the next object			*/
		item			= objectsLived->next(objectsLived, item);
	}

	return 1;
}
