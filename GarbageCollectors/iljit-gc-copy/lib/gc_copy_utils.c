
/* For assert */
#include <assert.h>

/* For malloc/free/... */
#include <stdlib.h>

/* For diff_time */
#include <iljit-utils.h>

/* For TRUE/FALSE */
#include <gc_copy_types.h>

/* Include prototypes */
#include <gc_copy_utils.h>

/* Start a timer */
struct timespec start_timer(void) {

  /* Holds timer start time */
  struct timespec start_time;

  /* Init and start timer */
  clock_gettime(CLOCK_REALTIME, &start_time);

  return start_time;

}

/* Stop a timer */
void stop_timer(struct timespec* start_time, JITFLOAT32* total) {

  /* Store timer stop time */
  struct timespec end_time;

  /* Check pointers */
  assert(start_time != NULL && total != NULL);

  /* Get end time and update global counter */
  clock_gettime(CLOCK_REALTIME, &end_time);
  *total += diff_time(start_time, &end_time);

}

/* "Safe" allocation routine */
void* xalloc(JITNUINT size) {

  /* Allocated memory pointer */
  void* memory;

  /* Alloc memory */
  memory = malloc(size);

  /* Check return value */
  assert(memory != NULL);

  return memory;

}

/* Array allocation routine */
void* xcalloc(JITNUINT count, JITNUINT size) {

  /* Array pointer */
  void* array;

  /* Alloc memory */
  array = calloc(count, size);

  /* Check returned value */
  assert(array != NULL);

  return array;

}

/* Reallocation routine */
void* xrealloc(void* address, JITNUINT new_size) {

  /* Memory pointer */
  void* memory;

  /* Realloc memory */
  memory = realloc(address, new_size);

  /* Check returned value */
  assert(memory != NULL);

  return memory;

}

/* Deallocation routine */
void xfree(void* memory) {

  free(memory);
  /* Logically reset memory */
  memory = NULL;

}

/* Allocation routine for libxan */
void* xan_alloc(int size) {

  /* Check args */
  assert(size >= 0);

  /* Alloc memory with xalloc */
  return xalloc((JITNUINT) size);

}

/* Reallocation routine for libxan */
void* xan_realloc(void* address, int new_size) {

  /* Check args */
  assert(address != NULL && new_size >= 0);

  /* Realloc memory with xrealloc */
  return xrealloc(address, (JITNUINT) new_size);

}
