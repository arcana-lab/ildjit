#ifndef INTERNAL_MEMORY_MANAGEMENT_H
#define INTERNAL_MEMORY_MANAGEMENT_H

// My headers
#include <jitsystem.h>
// End

void * gcAlloc (JITINT32 size);
void * gcRealloc (void *start, JITINT32 newSize);
void gcFree(void *start);

#endif
