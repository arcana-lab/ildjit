
#ifndef RECURSION_STACK_H
#define RECURSION_STACK_H

/* For JIT* type declarations */
#include <jitsystem.h>

/* For XanList */
#include <xanlib.h>

/**
 * Returned by some routines when all is ok
 */
#define RS_ALL_OK 0x0

/**
 * Returned by rs_push when trying to push a NULL pointer
 */
#define RS_PUSH_NULL 0x1

/**
 * A stack used to emulate recursion
 *
 * This data structure allow to pop and push pointers like a stack. It is used
 * to emulate recursive function call because it is faster than normal function
 * call. To use it simply put needed arguments on recursion stack top and call
 * your function. To increase performance is better that called routinr id an
 * inline function.
 */
typedef XanList recursion_stack_t;

/**
 * Initialize a new recursion stack
 *
 * Build a new recursion stack. If pointers is not NULL then all pointers in
 * given XanList are pushed on stack; if a NULL pointer is founded in given
 * it isn't added to the stack. If pointers is NULL the new stack is empty.
 *
 * @param pointers NULL or a pointer to a XanList
 *
 * @return a new recursion_stack_t instance
 */
recursion_stack_t* rs_init(XanList* pointers);

/**
 * Destructor
 *
 * Destroy given recursion stack. Stored elements aren't destroyed.
 *
 * @param stack this stack
 */
void rs_destroy(recursion_stack_t* stack);

/**
 * Push operation
 *
 * Put given pointer on stack top. If pointer is NULL RS_NULL_PUSH is returned.
 * Otherwise RS_ALL_OK is returned.
 *
 * @param stack   this stack
 * @param pointer the pointer to push
 *
 * @return RS_NULL_PUSH if pointer is NULL. RS_ALL_OK otherwise
 */
JITNUINT rs_push(recursion_stack_t* stack, void* pointer);

/**
 * Another push operation
 *
 * Put all pointers in given list on stack top. If pointers is NULL RS_NULL_PUSH
 * is returned. If there are some NULL elements inside pointers list they
 * aren't added to the stack. On success RS_ALL_OK is returned.
 *
 * @param stack    this stack
 * @param pointers a list of pointers to push
 *
 * @return RS_NULL_PUSH if pointers is NULL. Otherwise RS_ALL_OK
 */
JITNUINT rs_push_list(recursion_stack_t* stack, XanList* pointers);

/**
 * Pop operation
 *
 * Remove last added pointer to stack top. If stack is empty NULL is returned.
 *
 * @param stack this stack
 *
 * @return stack top or NULL if stack is empty
 */
void* rs_pop(recursion_stack_t* stack);

/**
 * Test if given pointer is inside the stack
 *
 * @param stack   this stack
 * @param pointer pointer to check
 *
 * @return TRUE if given pointer is inside the stack. FALSE otherwise
 */
JITBOOLEAN rs_contains(recursion_stack_t* stack, void* pointer);

/**
 * Get stack size
 *
 * @param stack this stack
 *
 * @return stack size
 */
JITNUINT rs_size(recursion_stack_t* stack);

/**
 * Check if stack is empty
 *
 * @param stack this stack
 *
 * @return TRUE if stack size is 0, FALSE otherwise
 */
JITBOOLEAN rs_is_empty(recursion_stack_t* stack);

#endif /* RECURSION_STACK_H */
