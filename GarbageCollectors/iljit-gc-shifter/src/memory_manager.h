#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

// My headers
#include <jitsystem.h>
#include <garbage_collector.h>
// End

#define START_ADDRESS(obj) (obj - HEADER_FIXED_SIZE)

typedef struct {
	void 		**objects;
	JITUINT32	top;
	JITUINT32	allocated;
	JITUINT32	max;
} t_objects;

typedef struct t_memory{
	void		*heap;
	void		*top;
	void 		*bottom;
	JITNUINT 	*heapOccupance;
	JITNUINT 	heapOccupanceWords;
	JITUINT32	maxHeap;
	t_objects	objectsReference;
	JITINT16	(*shutdown)		(struct t_memory *memory);
	void *		(*fetchFreeMemory)	(struct t_memory *memory, JITNUINT size, t_gc_behavior *gc);
	void 		(*collect)		(struct t_memory *memory, t_gc_behavior *gc);
	void		(*addObjectReference)	(struct t_memory *memory, void *newObject);
	JITUINT32 	(*freeMemorySize) 	(struct t_memory *memory);
} t_memory;

JITINT16 init_memory(t_memory *memory, JITUINT32 size);

#endif
