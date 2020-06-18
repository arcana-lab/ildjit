#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/mman.h>

// My headers
#include <jitsystem.h>
#include <iljit-utils.h>
#include <garbage_collector.h>
#include <iljit_gc_shifter.h>
// End

JITNUINT getSizeAligned (JITNUINT size);

void * gcAlloc (JITINT32 size){
	void 		*mem;

	/* Assertions				*/
	assert(size > 0);
	PDEBUG("GC Shifter: gcAlloc: Start\n");
	PDEBUG("GC Shifter: gcAlloc: 	Request size	= %u Bytes\n", size);

	/* Check the flags			*/
	mem	= malloc(size);
	if (mem == NULL){
		print_err("GC Shifter: gcAlloc: ERROR = Malloc return an error. ", errno);
		abort();
	}
	PDEBUG("GC Shifter: gcAlloc: Exit\n");
	return mem;
}

void * gcRealloc (void *start, JITINT32 newSize){
	void *mem;
	
	/* Assertions				*/
	assert(newSize > 0);
	PDEBUG("GC Shifter: gcRealloc: Start\n");
	PDEBUG("GC Shifter: gcRealloc: 	New size 	= %u Bytes\n", newSize);

	/* Initialize the variables		*/
	mem	= NULL;

	/* Execute a mapping of the memory	*/
	if (start == NULL){
		PDEBUG("GC Shifter: gcRealloc: 	Alloc a new memory block\n");
		mem	= gcAlloc(newSize);
		start	= mem;
		assert(mem != NULL);
	} else {
		assert(start != NULL);
		PDEBUG("GC Shifter: gcRealloc: 	Realloc the memory block\n");
		mem	= realloc(start, newSize);
		if (mem == NULL){
			print_err("GC Shifter: gcRealloc: ERROR = mremap return an error. ", errno);
			abort();
		} else if (mem == ((void *) -1)){
			print_err("GC Shifter: gcRealloc: ERROR = mremap return an error. ", errno);
			abort();
		}
	} 
	assert(mem != NULL);

	/* Return				*/
	PDEBUG("GC Shifter: gcRealloc: Exit\n");
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
