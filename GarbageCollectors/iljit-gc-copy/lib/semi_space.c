
/* For assert */
#include <assert.h>

/* For NULL */
#include <stdlib.h>

/* For memcpy */
#include <string.h>

#include <semi_space.h>

/* For debug macro */
#include <gc_copy_utils.h>

/* Initialize the given semispace */
void sm_init(semi_space_t* space, void* start_address, JITNUINT size) {

  DEBUG_ENTER("sm_init");
  DEBUG_LOG("Params: this: %p, start_address: %p, size : %lu",
	    space, start_address, size);

  /* Pointer checks */
  assert(space != NULL);
  assert(start_address != NULL);

  /* We can't initialize a semispace with size 0 bytes */
  assert(size > 0);

  /* Initialize a free semispace */
  space->bottom = start_address;
  space->size = size;
  space->free_zone = space->bottom;

  DEBUG_EXIT("sm_init");

}

/* Alloc size byte of memory, if possible */
void* sm_alloc_memory(semi_space_t* space, JITNUINT size) {

  /* Pointer to new allocated memory */
  void* memory;

  /* Check pointer */
  assert(space != NULL);

  /* We can't alloc 0 byte */
  if(size == 0)
    memory = NULL;
  /* OK there are free space */
  else if((sm_free_memory(space) >= size)) {
    space->free_zone += size;
    memory = space->free_zone - size;
  }
  /* No free size bytes in this semispace */
  else
    memory = NULL;

  return memory;

}

/* Copy some bytes into freee zone */
void sm_add(semi_space_t* space, void* address, JITNUINT size) {

  /* Check for pointers */
  assert(space != NULL && address != NULL);

  /* We have enough memory? */
  assert(sm_free_memory(space) >= size);
  /* Do the copy */
  memcpy(space->free_zone, address, size);
  space->free_zone += size;

}

/* Reset semispace logical top */
void sm_reset(semi_space_t* space, void* free_zone) {

  /* Check pointers */
  assert(space != NULL);
  assert(free_zone != NULL);

  /* Check if free_zone is an address inside semispace */
  assert(sm_valid_address(space, free_zone));
  /* Update logical pointer */
  space->free_zone = free_zone;

#ifdef DEBUG
  /* Reset semispace during debug */
  memset(space->bottom, 0, space->size);
#endif

}

/* Compute semispace top border */
void* sm_top(semi_space_t* space) {

  /* Check for pointers */
  assert(space != NULL);

  return space->bottom + space->size - 1;

}

/* Compute semispace currently used memory */
JITNUINT sm_used_memory(semi_space_t* space) {

  /* Check for pointers */
  assert(space != NULL);

  return space->free_zone - space->bottom;

}

/* Compute currently free memory */
JITNUINT sm_free_memory(semi_space_t* space) {

  /* Check for pointers */
  assert(space != NULL);

  return space->size - sm_used_memory(space);

}

/* Test if address is valid in semispace space */
JITBOOLEAN sm_valid_address(semi_space_t* space, void* address) {

  /* Check for pointers */
  assert(space != NULL && address != NULL);

  /* Address is valid if it is inside the semispace */
  return address >= space->bottom && address <= sm_top(space);

}

/* Get over head memory count */
JITNUINT sm_over_head(semi_space_t* space) {

  /* Check pointer */
  assert(space != NULL);

  return sizeof(space);

}
