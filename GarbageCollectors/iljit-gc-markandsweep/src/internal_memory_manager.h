#ifndef INTERNAL_MEMORY_MANAGEMENT_H
#define INTERNAL_MEMORY_MANAGEMENT_H

// My headers
#include <jitsystem.h>
// End

void * gcAlloc (size_t size);
void * gcRealloc (void *start, size_t newSize);
void gcFree(void *start);

#endif
