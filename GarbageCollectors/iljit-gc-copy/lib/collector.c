
/* For assert */
#include <assert.h>

/* For NULL */
#include <stdlib.h>

/* For HEADER_FIXED_SIZE */
#include <garbage_collector.h>

#include <collector.h>

/* For debug macro */
#include <gc_copy_utils.h>

/*
 * Some file must define the following function in order to link this module
 */
extern JITNUINT (*object_size)(void* object);
extern XanList* (*object_children)(void* object);
extern XanList* (*get_root_set)(void);

/* Prototypes of some private functions */
static JITBOOLEAN co_is_collectable(collector_t* collector, void* object);
static void co_collect_children(collector_t* collector, void* object);

/* Constructor */
void co_init(collector_t* collector, gc_memory_t* heap) {

  /* Pointers check */
  assert(collector != NULL && heap != NULL);

  /* Initialize internal state */
  collector->heap = heap;
  collector->object_table = xanHashTableNew(DEFAULT_OBJECT_TABLE_SIZE, 0,
		                            xan_alloc, xan_realloc, xfree,
					    NULL, NULL);
  collector->stack = rs_init(NULL);

}

/* Destructor */
void co_destroy(collector_t* collector) {

  /* Pointer check */
  assert(collector != NULL);

  /* Free memory used by object table */
  collector->object_table->destroy(collector->object_table);

  /* Free memory used by stack */
  collector->stack->destroyList(collector->stack);

}

/* Visit an object and apply garbage collecting algorithm to it and childs */
inline void* co_collect_object(collector_t* collector,
		               void** reference_pointer) {

  /* Object address in online semi space */
  void* old_address;
  
  /* Object address in offline semi space */
  void* new_address;

  DEBUG_ENTER("co_collect_object");
  DEBUG_LOG("Params: this: %p, reference_pointer: %p",
            collector, reference_pointer);

  /* Check pointers */
  assert(collector != NULL && reference_pointer != NULL);

  /* Save object old address */
  old_address = *reference_pointer;

  /* Do collection only if object is collectable */
  if(co_is_collectable(collector, old_address)) {

    DEBUG_LOG("Object %p collectable", old_address);

    /* Test if object has just been visited */
    new_address = co_lookup_object(collector, old_address);
    if(new_address != NULL) {
      *reference_pointer = new_address;
    }
    /* Object never visited */
    else {
      /* Update root reference */
      new_address = collector->heap->offline_semi_space->free_zone +
		    HEADER_FIXED_SIZE;
      *reference_pointer = new_address;

      DEBUG_LOG("Object new address is %p", new_address);

      /* Put object addresses in object table */
      co_mark_collected(collector, old_address, new_address);

      /* Copy object: if not enought memory, program will be killed */
      gcm_move(collector->heap,
	       old_address - HEADER_FIXED_SIZE,
	       object_size(old_address));

      /* We still need to visit children */
      co_collect_children(collector, new_address);
    }
  }

  DEBUG_EXIT("co_collect_object");

  return new_address;

}

/* Check that an object is collectable */
static JITBOOLEAN co_is_collectable(collector_t* collector, void* object) {

  /* Function return value */
  JITBOOLEAN retval;

  /* Check pointer */
  assert(collector != NULL);

  /* If object is NULL don't collect it */
  if(object == NULL)
    retval = FALSE;
  /* An object is collectable if is inside the heap */
  else
    retval = gcm_inside_heap(collector->heap, object);

  return retval;

}

/* Visit given object children */
static void co_collect_children(collector_t* collector, void* object) {

  /* A list of objects */
  XanList* children;

  DEBUG_ENTER("co_collect_children");
  DEBUG_LOG("Params: this: %p, object: %p", collector, object);

  /* Check pointers */
  assert(collector != NULL && object != NULL);

  /* Put all children on stack top */
  children = object_children(object);
  rs_push_list(collector->stack, children);

  /* Release resources after collection */
  children->destroyList(children);

  DEBUG_EXIT("co_collect_children");

}

/* Run collect algorithm */
void co_collect(collector_t* collector) {

  /* Current root reference in iteration */
  XanListItem* item;

  /* Used to emulate recursive function call */
  recursion_stack_t* stack;

  DEBUG_ENTER("co_collect");
  DEBUG_LOG("Params: this: %p", collector);
  DEBUG_LOG("Free memory: %lu", gcm_free(collector->heap));

  /* Check pointers */
  assert(collector != NULL);

  /* Add root set to stack */
  stack = collector->stack;
  rs_push_list(stack, get_root_set());

  /* Visit the tree */
  while(!rs_is_empty(stack))
    /* Call core routine to visit current object */
    co_collect_object(collector, rs_pop(stack));

  /* Swap semispaces */
  gcm_swap_semispaces(collector->heap);

  DEBUG_LOG("Free memory: %lu", gcm_free(collector->heap));
  DEBUG_EXIT("co_collect");

}

/* Mark object with old address old_address visited */
JITNUINT co_mark_collected(collector_t* collector, void* old_address,
		       void* new_address) {

  /* Return value */
  JITNUINT retval;

  /* Alias for collector object table */
  XanHashTable* object_table;

  DEBUG_ENTER("co_mark_collected");
  DEBUG_LOG("Params: this: %p, old_address: %p, new_address: %p",
	    collector, old_address, new_address);

  /* Pointers check */
  assert(collector != NULL && old_address != NULL && new_address != NULL);

  if(co_marked(collector, old_address))
    /* Error: object already marked */
    retval = CO_DOUBLE_MARK;
  else {
    /* Inset new_address in object table, with old_address as key */
    object_table = collector->object_table;
    object_table->insert(object_table, old_address, new_address);
    retval = CO_ALL_OK;
  }

  DEBUG_EXIT("co_mark_collected");

  return retval;

}

/* Get object new address. The search key is object old address */
void* co_lookup_object(collector_t* collector, void* old_address) {

  /* New object address */
  void* new_address;

  /* Alias for collector object table */
  XanHashTable* object_table;

  DEBUG_ENTER("co_lookup_object");
  DEBUG_LOG("Params: this: %p, old_address: %p", collector, old_address);

  /* Check pointers */
  assert(collector != NULL && old_address != NULL);

  /* Lookup object in object table */
  object_table = collector->object_table;
  new_address = object_table->lookup(object_table, old_address);

  DEBUG_LOG("Return: %p", new_address);
  DEBUG_EXIT("co_lookup_object");

  return new_address;

}

/* Test if an object has been visited */
JITBOOLEAN co_marked(collector_t* collector, void* old_address) {

  /* Search result */
  void* new_address;

  DEBUG_ENTER("co_marked");
  DEBUG_LOG("Params: this: %p, old_address: %p", collector, old_address);

  /* Pointers check */
  assert(collector!= NULL && old_address != NULL);

  /* Try searching object: if not found NULL is returned */
  new_address = co_lookup_object(collector, old_address);

  DEBUG_EXIT("co_marked");

  return new_address != NULL;

}

/* Get current number of visited objects */
JITNUINT co_objects_count(collector_t* collector) {

  /* A table of current known objects */
  XanHashTable* object_table;

  /* Pointer check */
  assert(collector != NULL);

  /* This alias is only for beauty */
  object_table = collector->object_table;

  return object_table->elementsInside(object_table);

}
