#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

// My headers
#include <jitsystem.h>
#include <garbage_collector.h>
// End

typedef struct {
	void 		*start;
	JITUINT32	size;
} t_memory_block;

typedef struct {
	t_memory_block	*block;
	JITUINT32	top;
	JITUINT32	allocated;
	JITUINT32	max;
	JITNUINT	size;
} t_memory_blocks;

typedef struct {
	void 		**objects;
	JITUINT32	top;
	JITUINT32	allocated;
	JITNUINT	size;
	JITUINT32	max;
} t_objects;

typedef struct t_memory{
	void		*heap;		/**< First byte free of the heap							*/
	void		*top;		/**< First byte outside the heap (i.e. access to this memory location is not legal.)	*/
	void 		*bottom;	/**< First byte of the heap								*/
	JITNUINT 	*heapOccupance;	/**< Bitmap of bytes occupied by live objects.						*/
	JITNUINT 	heapOccupanceWords;
	JITUINT32	maxHeap;
	t_memory_blocks	memoryBlocks;		/**< List of memory block in decrease order of size	*/
	t_objects	objectsReference;
	JITINT16	(*shutdown)		(struct t_memory *memory, t_gc_behavior *gc);
	void *		(*fetchFreeMemory)	(struct t_memory *memory, JITNUINT size, t_gc_behavior *gc);
	void 		(*resizeMemory)		(struct t_memory *memory, void *obj, JITUINT32 newSize, t_gc_behavior *gc);
	void 		(*collect)		(struct t_memory *memory, t_gc_behavior *gc);
	void		(*addObjectReference)	(struct t_memory *memory, void *newObject);
	JITUINT32 	(*freeMemorySize) 	(struct t_memory *memory);
} t_memory;

JITINT16 init_memory(t_memory *memory, JITUINT32 size);

#endif
