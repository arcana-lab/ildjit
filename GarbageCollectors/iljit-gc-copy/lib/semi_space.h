
#ifndef SEMI_SPACE_H
#define SEMI_SPACE_H

/* For basic type declaration, such as JITUINT32 */
#include <jitsystem.h>

/* Some usefull type macro */
#include <gc_copy_types.h>

/**
 * A resizable memory zone called "semispace"
 *
 * A "semispace" is a partition of the heap used by the garbage collector as
 * allocation area. It provide functions to alloc and reset memory (required by
 * garbage collector algorithm when swapping semispaces).
 *
 * In the heap the user program can allocate also raw memory. The size of this
 * memory is lesser than the size of "dynamic memory" (allocated via new
 * operator), so when raw memory is required it must be asked at semispaces.
 * When the user finish to use raw memory it can be released to semispaces.
 * The semispace module struct provide special functions to get and relase
 * memory for raw memory allocation and deallocation.
 *
 * @author speziale.ettore@gmail.com
 */
typedef struct {

  /**
   * The bottom of semispace
   */
  void* bottom;

  /**
   * Semispace size, in bytes
   */
  JITNUINT size;

  /**
   * A pointer to first non allocated byte
   */
  void* free_zone;

} semi_space_t;

/**
 * Semispace constructor
 *
 * @param space         this semispace
 * @param start_address the bottom of new semispace
 * @param size          semispace size
 */
void sm_init(semi_space_t* space, void* start_address, JITNUINT size);

/**
 * Require a new memory segment
 *
 * @param space this semispace
 * @param size  the required memory size
 *
 * @return a pointer to a free memory segment or NULL if something goes wrong
 */
void* sm_alloc_memory(semi_space_t* space, JITNUINT size);

/**
 * Append some bytes to space
 *
 * This function copy size bytes starting from address into space. Bytes are
 * writed starting from semispace free zone pointer.
 *
 * @param space   this semispace
 * @param address source address
 * @param size    number of bytes to copy
 */
void sm_add(semi_space_t* space, void* address, JITNUINT size);

/**
 * Reset semispace logical top
 *
 * Semispace logical top (free_zone) must be resetted during garbage
 * collection. This method do that.
 *
 * @param space     this semispace
 * @param free_zone the new free_zone address
 */
void sm_reset(semi_space_t* space, void* free_zone);

/**
 * Get semispace top pointer
 *
 * @param space this semispace
 *
 * @return space top pointer
 */
void* sm_top(semi_space_t* space);

/**
 * Get current semispace used memory
 *
 * @param space this semispace
 *
 * @return the current used memory size
 */
JITNUINT sm_used_memory(semi_space_t* space);

/**
 * Get current semispace unused memory
 *
 * @param space this semispace
 *
 * @return the semispace free memory size
 */
JITNUINT sm_free_memory(semi_space_t* space);

/**
 * Tests if given address is in this semispace
 *
 * Verify that address if logically between semispace bottom and semispace top.
 *
 * @param space   this semispace
 * @param address address to test
 *
 * @return TRUE if address is in this semispace, FALSE otherwise
 */
JITBOOLEAN sm_valid_address(semi_space_t* space, void* address);

/**
 * Get over head memory size
 *
 * Get size of data structures used to manage semi space.
 *
 * @param space this space
 *
 * @return over head memory count
 */
JITNUINT sm_over_head(semi_space_t* space);

#endif /* SEMI_SPACE_H */
