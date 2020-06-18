#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

// My headers
#include <jitsystem.h>
#include <iljit-utils.h>
#include <garbage_collector.h>
#include <iljit_gc_markandsweep.h>
// End

JITNUINT getSizeAligned (JITNUINT size);

void * gcAlloc (size_t size){
	void 		*mem;

	/* Assertions				*/
	assert(size > 0);
	PDEBUG("GC MarkAndSweep: gcAlloc: Start\n");
	PDEBUG("GC MarkAndSweep: gcAlloc: 	Request size	= %u Bytes\n", size);

	/* Check the flags			*/
	PDEBUG("GC MarkAndSweep: gcAlloc: 	Alloc a fixed size of memory	= %u Bytes\n", size);
	mem	= malloc(size);
	if (mem == NULL){
		print_err("GC MarkAndSweep: gcAlloc: ERROR = Malloc return an error. ", errno);
		abort();
	}
	PDEBUG("GC MarkAndSweep: gcAlloc: Exit\n");
	return mem;
}

void * gcRealloc (void *start, size_t newSize){
	void *mem;
	
	/* Assertions				*/
	assert(newSize > 0);
	PDEBUG("GC MarkAndSweep: gcRealloc: Start\n");
	PDEBUG("GC MarkAndSweep: gcRealloc: 	New size 	= %u Bytes\n", newSize);

	/* Initialize the variables		*/
	mem	= NULL;

	/* Execute a mapping of the memory	*/
	if (start == NULL){
		PDEBUG("GC MarkAndSweep: gcRealloc: 	Alloc a new memory block\n");
		mem	= gcAlloc(newSize);
		start	= mem;
		assert(mem != NULL);
	} else {
		assert(start != NULL);
		PDEBUG("GC MarkAndSweep: gcRealloc: 	Realloc the memory block\n");
		mem	= realloc(start, newSize);
		if (mem == NULL){
			print_err("GC MarkAndSweep: gcRealloc: ERROR = mremap return an error. ", errno);
			abort();
		} else if (mem == ((void *) -1)){
			print_err("GC MarkAndSweep: gcRealloc: ERROR = mremap return an error. ", errno);
			abort();
		}
	} 
	assert(mem != NULL);

	/* Return				*/
	PDEBUG("GC MarkAndSweep: gcRealloc: Exit\n");
	return mem;
}

void gcFree(void *start){
	
	/* Assertions		*/
	assert(start != NULL);

	free(start);
}

JITNUINT getSizeAligned (JITNUINT size){
	JITUINT32	pages;

	/* Assertions		*/
	assert(size > 0);

	if ((size % getpagesize()) != 0){
		pages	= (size / getpagesize()) + 1;
		size	= pages * getpagesize();
	}

	/* Return		*/
	return size;
}
