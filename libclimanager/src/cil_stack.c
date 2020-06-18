/*
 * Copyright (C) 2006  Campanoni Simone
 *
 * iljit - This is a Just-in-time for the CIL language specified with the ECMA-335
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xanlib.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <cil_stack.h>
#include <ilmethod.h>
#include <climanager_system.h>
// End

static inline void internal_init_stack (t_stack *stack);
static inline void internal_dupTop (t_stack *_this);
static inline void internal_push (t_stack *_this, ir_item_t *var);
static inline void internal_pop (t_stack *_this, ir_item_t *var);
static inline void internal_adjustSize (t_stack *_this);
static inline void internal_cleanTop (t_stack *_this);
static inline t_stack * internal_cloneStack (CILStack cilStack, t_stack *stack);
static inline t_stack * merge_stackes (CILStack cilStack, t_stack *stack1, t_stack *stack2, ir_method_t *method, ir_instruction_t *beforeInst);
static inline void internal_destroy (CILStack cilStack);

static inline t_stack * merge_stackes (CILStack cilStack, t_stack *stack1, t_stack *stack2, ir_method_t *method, ir_instruction_t *beforeInst) {
    JITUINT32 stackTop;
    JITUINT32 count;
    ir_instruction_t        *instruction;
    ir_instruction_t        *instruction2;

    /* Assertions			*/
    assert(stack1 != NULL);
    assert(stack2 != NULL);
    assert(method != NULL);
    PDEBUG("CILSTACK: mergeStackes: Start\n");

    /* Take the top			*
     * We consider the top of the   *
     * stack "stack2" because it is	*
     * the stack state that is more	*
     * far away from the current	*
     * CIL->IR translation.		*
     * The standard ECMA-335 says	*
     * that this stack should be    *
     * used for paths merging	*/
    stackTop = stack2->top;

    /* Merge the stackes		*/
    for (count = 0; count < stackTop; count++) {
        if ((stack1->stack[count]).value.v != (stack2->stack[count]).value.v) {
            PDEBUG("CILSTACK: mergeStackes:         Difference in position %u\n", count);
            if (    ((stack1->stack[count]).type == (stack2->stack[count]).type)    &&
                    ((stack1->stack[count]).type == IROFFSET)                       ) {
                if ((stack1->stack[count]).internal_type == (stack2->stack[count]).internal_type) {
                    PDEBUG("CILSTACK: mergeStackes:                 They are two variables with the same internal type\n");
                    if (beforeInst != NULL) {
                        instruction = IRMETHOD_newInstructionBefore(method, beforeInst);
                        instruction->byte_offset = beforeInst->byte_offset;
                    } else {
                        instruction = IRMETHOD_newInstruction(method);
                    }
                    instruction->type = IRMOVE;
                    memcpy(&(instruction->result), &(stack2->stack[count]), sizeof(ir_item_t));
                    memcpy(&(instruction->param_1), &(stack1->stack[count]), sizeof(ir_item_t));
                } else {
                    PDEBUG("CILSTACK: mergeStackes:                 They are two variables with different internal type\n");
                    if (beforeInst != NULL) {
                        instruction = IRMETHOD_newInstructionBefore(method, beforeInst);
                        instruction->byte_offset = beforeInst->byte_offset;
                    } else {
                        instruction = IRMETHOD_newInstruction(method);
                    }
                    instruction->type = IRCONV;
                    memcpy(&(instruction->param_1), &(stack1->stack[count]), sizeof(ir_item_t));
                    (instruction->param_2).value.v = (stack2->stack[count]).internal_type;
                    (instruction->param_2).type = IRTYPE;
                    (instruction->param_2).internal_type = IRTYPE;
                    (instruction->result).value.v = IRMETHOD_getNumberOfVariablesNeededByMethod(method);
                    (instruction->result).type = IROFFSET;
                    (instruction->result).internal_type = (stack2->stack[count]).internal_type;
                    (instruction->result).type_infos = (stack2->stack[count]).type_infos;
                    IRMETHOD_setNumberOfVariablesNeededByMethod(method, (instruction->result).value.v + 1);
                    if (beforeInst != NULL) {
                        instruction2 = IRMETHOD_newInstructionBefore(method, beforeInst);
                        instruction2->byte_offset = beforeInst->byte_offset;
                    } else {
                        instruction2 = IRMETHOD_newInstruction(method);
                    }
                    instruction2->type = IRMOVE;
                    memcpy(&(instruction2->result), &(stack2->stack[count]), sizeof(ir_item_t));
                    memcpy(&(instruction2->param_1), &(instruction->result), sizeof(ir_item_t));
                }
            }
        }
    }

    /* Return			*/
    PDEBUG("CILSTACK: mergeStackes: Exit\n");
    return stack2;
}

static inline t_stack * new_stack (CILStack cilStack) {
    t_stack *stack;

    /* Assertions			*/
    assert(cilStack != NULL);
    assert(cilStack->stackes != NULL);

    /* Make the new stack		*/
    stack = (t_stack *) allocMemory(sizeof(t_stack));
    stack->stack = (ir_item_t *) allocMemory(sizeof(ir_item_t) * MAX_STACK_SIZE);
    stack->top = 0;
    stack->max_stack_size = MAX_STACK_SIZE;
    internal_init_stack(stack);
    stack->dupTop = internal_dupTop;
    stack->push = internal_push;
    stack->pop = internal_pop;
    stack->adjustSize = internal_adjustSize;
    stack->cleanTop = internal_cleanTop;

    /* Insert the new stack inside  *
     * the list			*/
    xanList_insert(cilStack->stackes, stack);

    /* Return			*/
    return stack;
}

CILStack newCILStack () {
    CILStack newCILStack;

    /* Make the new CIL stack manager	*/
    newCILStack = (CILStack) allocMemory(sizeof(_CILStack));
    assert(newCILStack != NULL);

    /* Make the list of stackes		*/
    newCILStack->stackes = xanList_new(allocFunction, freeFunction, NULL);
    assert(newCILStack->stackes != NULL);

    /* Initialize the functions		*/
    newCILStack->mergeStackes = merge_stackes;
    newCILStack->newStack = new_stack;
    newCILStack->cloneStack = internal_cloneStack;
    newCILStack->destroy = internal_destroy;

    /* Return				*/
    return newCILStack;
}

static inline void internal_destroy (CILStack cilStack) {
    XanListItem     *item;
    t_stack         *stack;

    /* Assertions			*/
    assert(cilStack != NULL);
    assert(cilStack->stackes != NULL);

    item = xanList_first(cilStack->stackes);
    while (item != NULL) {
        stack = (t_stack *) xanList_getData(item);
        assert(stack != NULL);
        freeMemory(stack->stack);
        stack->stack = NULL;
        freeMemory(stack);
        item = item->next;
    }
    xanList_destroyList(cilStack->stackes);
    cilStack->stackes = NULL;
    freeMemory(cilStack);
}

static inline t_stack * internal_cloneStack (CILStack cilStack, t_stack *stack) {
    t_stack         *new_stack;
    JITUINT32 stackSize;

#ifdef DEBUG
    JITUINT32 count;
#endif

    /* Assertions			*/
    assert(cilStack != NULL);
    assert(cilStack->stackes != NULL);
    assert(stack != NULL);
    assert(stack->stack != NULL);
    assert(stack->top < stack->max_stack_size);

    /* Fetch the stack size		*/
    stackSize = stack->max_stack_size;

    /* Clone the stack		*/
    new_stack = (t_stack *) allocMemory(sizeof(t_stack));
    assert(new_stack != NULL);
    memcpy(new_stack, stack, sizeof(t_stack));
    new_stack->stack = (ir_item_t *) allocMemory(sizeof(ir_item_t) * stackSize);
    assert(new_stack->stack != NULL);
    memcpy(new_stack->stack, stack->stack, sizeof(ir_item_t) * stackSize);

    /* Check the new stack		*/
#ifdef DEBUG
    assert(stackSize == stack->max_stack_size);
    assert(new_stack->top == stack->top);
    assert(new_stack->max_stack_size == stack->max_stack_size);
    assert(new_stack->stack != stack->stack);
    for (count = 0; count < stackSize; count++) {
        assert(memcmp(&(new_stack->stack[count]), &(stack->stack[count]), sizeof(ir_item_t)) == 0);
    }

#endif

    /* Insert the cloned stack to	*
     * the list of stackes		*/
    xanList_insert(cilStack->stackes, new_stack);

    /* Return			*/
    return new_stack;
}

static inline void internal_pop (t_stack *_this, ir_item_t *var) {

    /* Assertions			*/
    assert(_this != NULL);
    assert(_this->stack != NULL);
    assert(_this->top > 0);

    /* Pop the top element		*/
    (_this->top)--;
    if (var != NULL) {
        memcpy(var, &(_this->stack[_this->top]), sizeof(ir_item_t));
    }
    memset(&(_this->stack[_this->top]), 0, sizeof(ir_item_t));

    /* Return			*/
    return;
}

static inline void internal_push (t_stack *_this, ir_item_t *var) {

    /* Assertions			*/
    assert(_this != NULL);
    assert(_this->stack != NULL);
    assert(var != NULL);

    memcpy(&(_this->stack[_this->top]), var, sizeof(ir_item_t));
    (_this->top)++;
}

static inline void internal_dupTop (t_stack *_this) {

    /* Assertions			*/
    assert(_this != NULL);
    assert(_this->stack != NULL);

    memcpy(&(_this->stack[_this->top]), &(_this->stack[_this->top - 1]), sizeof(ir_item_t));
    (_this->top)++;
}

static inline void internal_init_stack (t_stack *stack) {
    JITUINT32 count;

    /* Assertions		*/
    assert(stack != NULL);

    for (count = 0; count < stack->max_stack_size; count++) {
        (stack->stack[count]).type = NOPARAM;
        (stack->stack[count]).internal_type = NOPARAM;
        (stack->stack[count]).type_infos = NULL;
    }
}

static inline void internal_cleanTop (t_stack *_this) {

    /* Assertions		*/
    assert(_this != NULL);
    assert(_this->max_stack_size > _this->top);

    /* Clean the top        */
    memset(&(_this->stack[_this->top]), 0, sizeof(ir_item_t));

    /* Return               */
    return;
}

static inline void internal_adjustSize (t_stack *_this) {
    JITUINT32 count;

    /* Assertions		*/
    assert(_this != NULL);

    if (    (_this->max_stack_size - _this->top < MAX_STACK_SIZE)   ||
            (_this->top >= _this->max_stack_size)                   ) {
        _this->max_stack_size = _this->top    + MAX_STACK_SIZE;
        _this->stack = (ir_item_t *) dynamicReallocFunction(_this->stack, sizeof(ir_item_t) * _this->max_stack_size);
        assert(_this->stack != NULL);
        for (count = _this->top; count < _this->max_stack_size; count++) {
            memset(&(_this->stack[count]), 0, sizeof(ir_item_t));
            (_this->stack[count]).type = NOPARAM;
            (_this->stack[count]).internal_type = NOPARAM;
        }
    }
    assert(_this->top < _this->max_stack_size);
}
