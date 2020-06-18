
/* For assert */
#include <assert.h>

/* For NULL */
#include <stdlib.h>

#include <gc_memory.h>

/* For memory routines and debug macros */
#include <gc_copy_utils.h>

/* Used later in this file */
static void gcm_update_max(gc_memory_t* memory);

/* Initialize a memory segment */
void gcm_init(gc_memory_t* memory, JITNUINT size) {

  /* Loop counter */
  JITNUINT i;

  /* Used to cache a value */
  JITNUINT half_size;

  DEBUG_ENTER("gcm_init");
  DEBUG_LOG("Params: this: %p, size: %lu", memory, size);

  /* Check for pointer */
  assert(memory != NULL);

  /* Size must be even */
  if(size % 2 != 0)
    size++;

  /*
   * Size must be greather than 0. This check is done here to avoid overflow
   * related to previous instruction.
   */
  assert(size > 0);

  /* Alloc raw memory */
  half_size = size / 2;
  memory->segment = xcalloc(sizeof(void*), 2);
  for(i = 0; i < 2; i++)
    memory->segment[i] = xalloc(half_size);
  /* Alloc memory for semispaces */
  memory->online_semi_space = xalloc(sizeof(semi_space_t));
  memory->offline_semi_space = xalloc(sizeof(semi_space_t));
  /* Give memory to semispaces */
  sm_init(memory->online_semi_space, memory->segment[0], half_size);
  sm_init(memory->offline_semi_space, memory->segment[1], half_size);
  /* Initialize profile informations */
  GC_PROFILE(memory->max_size = 0);
  GC_PROFILE(memory->total_alloc_time = 0);

  DEBUG_EXIT("gcm_init");

}

/* Memory destructor */
void gcm_destroy(gc_memory_t* memory) {

  /* Loop counter */
  JITNUINT i;

  /* Check for pointers */
  assert(memory != NULL);

  /* Release semi_space_t memory */
  xfree(memory->online_semi_space);
  xfree(memory->offline_semi_space);
  /* Release memory segments */
  for(i = 0; i < 2; i++)
    xfree(memory->segment[i]);
  /* Release segments array */
  xfree(memory->segment);

}

/* Alloc a chunk of memory */
void* gcm_alloc(gc_memory_t* memory, JITNUINT size) {

  /* Base address of new segment */
  void* address;

  DEBUG_ENTER("gcm_alloc");
  DEBUG_LOG("Params: this: %p, size: %lu", memory, size);

  /* Check for pointers */
  assert(memory != NULL);

  /* Initialize timer */
  GC_PROFILE_TIME();
  GC_PROFILE_TIME_START();
  
  /* Ask to current semispace needed memory */
  address = sm_alloc_memory(memory->online_semi_space, size);

  /* Update profiler info */
  GC_PROFILE(gcm_update_max(memory));
  GC_PROFILE_TIME_END(&memory->total_alloc_time);

  DEBUG_LOG("Return: heap_address: %p", address);
  DEBUG_EXIT("gcm_alloc");

  return address;

}

/* Update max_size field of given memory */
static void gcm_update_max(gc_memory_t* memory) {

  /* Currently used memory */
  JITNUINT used_memory;

  /* Check pointer */
  assert(memory != NULL);

  /* Update maximum */
  used_memory = gcm_used(memory);
  if(used_memory > memory->max_size)
    memory->max_size = used_memory;

}

/* Copy memory between semispaces */
void gcm_move(gc_memory_t* memory, void* address, JITNUINT size) {

  DEBUG_ENTER("gcm_move");
  DEBUG_LOG("Params: this: %p, address: %p, size: %lu", memory, address, size);

  /* Check for pointers */
  assert(memory != NULL && address != NULL);

  /* Test if address is inside the online semispace */
  assert(sm_valid_address(memory->online_semi_space, address));
  /* Copy the memory */
  sm_add(memory->offline_semi_space, address, size);

  DEBUG_EXIT("gcm_move");

}

/* Invert semispaces roles */
void gcm_swap_semispaces(gc_memory_t* memory) {

  void* space;

  /* Check for pointers */
  assert(memory != NULL);

  /* Switch semispaces */
  space = memory->online_semi_space;
  memory->online_semi_space = memory->offline_semi_space;
  memory->offline_semi_space = space;
  /* Reset the offline semispace (It must be free for the next swap) */
  sm_reset(memory->offline_semi_space, memory->offline_semi_space->bottom);

}

/* Get memory size */
JITNUINT gcm_size(gc_memory_t* memory) {

  /* Check for pointer */
  assert(memory != NULL);

  return 2 * memory->online_semi_space->size;

}

/* Get used memory count */
JITNUINT gcm_used(gc_memory_t* memory) {

  /* Check for pointer */
  assert(memory != NULL);

  return sm_used_memory(memory->online_semi_space);

}

/* Get free memory count */
JITNUINT gcm_free(gc_memory_t* memory) {

  /* Check pointers */
  assert(memory != NULL);

  return sm_free_memory(memory->online_semi_space);

}

/* Check if address is inside the online semispace */
JITBOOLEAN gcm_inside_heap(gc_memory_t* memory, void* address) {

  /* Check pointers */
  assert(memory != NULL && address != NULL);

  return sm_valid_address(memory->online_semi_space, address);

}

/* Get maximum used memory size */
JITNUINT gcm_max_used(gc_memory_t* memory) {

  /* Check pointer */
  assert(memory != NULL);

  return memory->max_size;

}

/* Get over head bytes count */
JITNUINT gcm_over_head(gc_memory_t* memory) {

  /* Over head memory counter */
  JITNUINT count;

  /* Check pointer */
  assert(memory != NULL);

  count = sizeof(memory) +
	  /* Each semi space "waste" some memory */
	  sm_over_head(memory->online_semi_space) +
	  sm_over_head(memory->offline_semi_space) +
	  /* Offline semi space size waste half of heap */
	  memory->offline_semi_space->size;

  return count;

}

/* Get total allocation time */
JITFLOAT32 gcm_total_alloc_time(gc_memory_t* memory) {

  /* Check pointer */
  assert(memory != NULL);

  return memory->total_alloc_time;

}

