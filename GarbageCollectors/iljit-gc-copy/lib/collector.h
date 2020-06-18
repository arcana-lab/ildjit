
#ifndef COLLECTOR_H
#define COLLECTOR_H

/* For JIT* */
#include <jitsystem.h>

/* For XanHashTable */
#include <xanlib.h>

/* For gc_memory_t */
#include <gc_memory.h>

/* For boolean values */
#include <gc_copy_utils.h>

/* For recursion_stack_t */
#include <recursion_stack.h>

/**
 * Number of entries on collector object table at creation
 */
#define DEFAULT_OBJECT_TABLE_SIZE 16

/**
 * Returned by some functions if all is ok
 */
#define CO_ALL_OK      0x0

/**
 * Returned by marking functions if someone try to double mark an object
 */
#define CO_DOUBLE_MARK 0x1

/**
 * Collection algorithm
 *
 * This module implements a simple stop and wait copy collector. It provide
 * functions to collect memory and to mark an object collected or verify if it
 * has just been collected. This collector must be used only one time. When you
 * need to collect a second time you need to alloc a new collector.
 *
 * To work collector needs to know heap location. It uses an hash to save info
 * about objects. This info are used during collect process to manage object
 * addresses before and after their collection.
 * 
 * @todo collector must collect more that one time (rewrite object table)
 *
 * @author speziale.ettore@gmail.com
 */
typedef struct {

  /**
   * Program heap
   */
  gc_memory_t* heap;

  /**
   * Objects addresses (pre and after collection)
   */
  XanHashTable* object_table;

  /**
   * Used to emulate recursive function call
   */
  recursion_stack_t* stack;
} collector_t;

/**
 * Initialize a collector
 *
 * Build a new collector with an empty object table.
 *
 * @param collector      collector to initialize
 * @param heap           memory to be managed by collector
 */
void co_init(collector_t* collector, gc_memory_t* heap);

/**
 * Destructor
 *
 * Clean memory used by collector.
 *
 * @param collector this collector
 */
void co_destroy(collector_t* collector);

/**
 * Collect a single object
 *
 * This is the garbage collector core routine. The object pointed by given
 * reference is moved to the offline semispace if needed and reference pointed
 * address is updated. Then all objects reacheable by reference_pointer are
 * collected.
 *
 * @param collector this collector
 * @param reference_pointer a pointer to a reference to an object to collect
 *
 * @return collected object new address
 */
void* co_collect_object(collector_t* collector, void** reference_pointer);

/**
 * Collect all memory
 *
 * Apply collection algorithm on every reacheable object.
 *
 * @param collector this collector
 */
void co_collect(collector_t* collector);

/**
 * Signal that an object has been moved on offline semispace
 *
 * Call this function to update info about object in new_address location.
 *
 * @param collector   this collector
 * @param old_address object pre-collection address
 * @param new_address object after-collection address
 * 
 * @return CO_ALL_OK if object has been marked, CO_DOUBLE_MARK is object has
 *                   been marked in the past
 */
JITNUINT co_mark_collected(collector_t* collector, void* old_address,
 	  	           void* new_address);

/**
 * Search an object in object table
 *
 * Try to find the offline address of object that has old_address on online
 * semispace. Return NULL if search fails.
 *
 * @param collector   this collector
 * @param old_address object address in online semispace
 *
 * @return NULL if object isn't found; object address in offline semispace
 *         otherwise
 */
void* co_lookup_object(collector_t* collector, void* old_address);

/**
 * Check if an object has just been marked
 *
 * Return TRUE if object whose address in online semispace is old_address has
 * just been processes by garbage collector. FALSE otherwise.
 *
 * @param collector   this collector
 * @param old_address object address in online semispace
 *
 * @return TRUE if object has just been visited; FALSE otherwise
 */
JITBOOLEAN co_marked(collector_t* collector, void* old_address);

/**
 * Get marked objects count
 *
 * @param collector this collector
 *
 * @return the number of objects in collector object table
 */
JITNUINT co_objects_count(collector_t* collector);

#endif /* COLLECTOR_H */
