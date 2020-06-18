
/* For assert */
#include <assert.h>

#include <recursion_stack.h>

/* For memory allocation routines */
#include <gc_copy_utils.h>

/* Constructor */
recursion_stack_t* rs_init(XanList* pointers) {

  /* Pointer to new stack */
  recursion_stack_t* stack;

  /* Build an empty stack */
  stack = xanListNew(xan_alloc, xfree, NULL);

  /* Fill it if pointers given */
  if(pointers != NULL)
    rs_push_list(stack, pointers);

  return stack;

}

/* Destructor */
void rs_destroy(recursion_stack_t* stack) {

  /* Check pointer */
  assert(stack != NULL);

  /* Free memory */
  stack->destroyList(stack);
	
}

/* Add a pointer to stack top */
JITNUINT rs_push(recursion_stack_t* stack, void* pointer) {

  /* Return value */
  JITNUINT retval;

  /* Pointer check */
  assert(stack != NULL);

  /* Add pointer only if it is not NULL */
  if(pointer != NULL) {
    stack->insert(stack, pointer);
    retval = RS_ALL_OK;
  }
  else
    retval = RS_PUSH_NULL;

  return retval;

}

/* Push  a set of pointers */
JITNUINT rs_push_list(recursion_stack_t* stack, XanList* pointers) {

  /* Routine return value */
  JITNUINT retval;

  /* Pointer container */
  XanListItem* item;

  /* Check pointer */
  assert(stack != NULL);

  /* Check if pointers is valid */
  if(pointers == NULL)
    retval = RS_PUSH_NULL;
  else {
    /* Some pointers to push */
    item = pointers->first(pointers);
    while(item != NULL) {
      rs_push(stack, item->data);
      item = pointers->next(pointers, item);
    }
    retval = RS_ALL_OK;
  }

  return retval;

}

/* Remove last added pointer */
void* rs_pop(recursion_stack_t* stack) {

  /* Stack top */
  void* pointer;

  /* Pointer container */
  XanListItem* item;

  /* Check pointer */
  assert(stack != NULL);

  /* Get stack top */
  item = stack->first(stack);
  if(item == NULL)
    /* Stack empty */
    pointer = NULL;
  else {
    /* Stack not empty */
    pointer = item->data;
    stack->delete(stack, pointer);
  }

  return pointer;

}

/* Check if pointer is inside the stack */
JITBOOLEAN rs_contains(recursion_stack_t* stack, void* pointer) {

  /* Check pointer */
  assert(stack != NULL);

  return stack->find(stack, pointer) != NULL;

}

/* Get stack size */
JITNUINT rs_size(recursion_stack_t* stack) {

  /* Check pointer */
  assert(stack != NULL);

  return stack->length(stack);

}

/* Check if stack is empty */
JITBOOLEAN rs_is_empty(recursion_stack_t* stack) {

  /* Check pointer */
  assert(stack != NULL);
 
  return rs_size(stack) == 0;

}
