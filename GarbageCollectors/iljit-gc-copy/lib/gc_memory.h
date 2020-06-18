
#ifndef GC_MEMORY_H
#define GC_MEMORY_H

/* For JITNUINT */
#include <jitsystem.h>

/* For semi_space_t */
#include <semi_space.h>

/**
 * The program memory
 *
 * This component is used to manage program memory. It expose functions to
 * manage collectable memory. Memory is allocated in two special memory areas
 * called semispaces. At a given time only one semispace contains live data. To
 * interact with semispaces the componet give functions to swap semispaces role
 * and to move memory from the online one to the other.
 *
 * @author speziale.ettore@gmail.com
 */
typedef struct {

  /**
   * Managed memory
   */
  void** segment;

  /**
   * Contains actually live data
   */
  semi_space_t* online_semi_space;

  /**
   * Currently unused semispace
   */
  semi_space_t* offline_semi_space;
  
  /**
   * Maximum number of bytes allocated
   */
  JITNUINT max_size;

  /**
   * Total allocation time
   */
  JITFLOAT32 total_alloc_time;

} gc_memory_t;

/**
 * Initialize a memory area
 *
 * @param memory this memory
 * @param size   the size of new memory segment
 */
void gcm_init(gc_memory_t* memory, JITNUINT size);

/**
 * Cleanup memory
 *
 * This functions acts as a destructor in OO languages.
 *
 * @param memory this memory
 */
void gcm_destroy(gc_memory_t* memory);

/**
 * Alloc size bytes of collectable memory
 *
 * @param memory this memory
 * @param size   the requested memory size
 *
 * @return the requested memory address. NULL if there aren't enougth memory or
 *         size is 0
 */
void *gcm_alloc(gc_memory_t* memory, JITNUINT size);

/**
 * Move a chunk of bytes from currently live semispace to the end of the
 * offline one
 *
 * @param memory  this memory
 * @param address pointer to memory to move
 * @param size    number of bytes to move
 *
 * @note This is a critical operation. If one of the parameter is wrong (e.g.
 *       address points to a bad memory location) the program will be killed.
 */
void gcm_move(gc_memory_t* memory, void* address, JITNUINT size);

/**
 * Swap semispaces role
 *
 * @param memory this memory
 */
void gcm_swap_semispaces(gc_memory_t* memory);

/**
 * Get memory size
 *
 * @param memory this memory
 *
 * @return memory size
 */
JITNUINT gcm_size(gc_memory_t* memory);

/**
 * Get currently used memory
 *
 * @param memory this memory
 *
 * @return used memory count
 */
JITNUINT gcm_used(gc_memory_t* memory);

/**
 * Get currently free memory count
 *
 * @param memory this memory
 *
 * @return count of free bytes
 */
JITNUINT gcm_free(gc_memory_t* memory);

/**
 * Check if given address is a valid address
 *
 * This function define a valid address an address inside online semispace.
 * Call this function to know if an object is collectable.
 *
 * @param memory  this memory
 * @param address an address to check
 *
 * @return TRUE if address is inside memory online semi space, FALSE otherwise
 */
JITBOOLEAN gcm_inside_heap(gc_memory_t* memory, void* address);

/**
 * Get maximum used memory
 *
 * @param memory this memory
 *
 * @return used memory count
 */
JITNUINT gcm_max_used(gc_memory_t* memory);

/**
 * Get over head bytes count
 *
 * This function returns the number of bytes used by gc_memory component to
 * store information about managed heap.
 *
 * @param memory this memory
 *
 * @return over head bytes count
 */
JITNUINT gcm_over_head(gc_memory_t* memory);

/**
 * Get total allocation time
 *
 * @param memory this memory
 *
 * @return total allocation time
 */
JITFLOAT32 gcm_total_alloc_time(gc_memory_t* memory);

#endif /* GC_MEMORY_H */
