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
/**
 * @file cil_stack.h
 *
 * This file implements the stack used for the translation from CIL bytecode to IR language
 */
#ifndef CIL_STACK_H
#define CIL_STACK_H

#include <jitsystem.h>
#include <ir_method.h>
#include <metadata/metadata_types.h>

#define MAX_STACK_SIZE                          128

/**
 * @brief Stack
 *
 * This struct contains the stack used to translate the CIL method into the IR equivalent method.
 */
typedef struct t_stack {
    ir_item_t       *stack;                                                                         /**< Stack						*/
    JITUINT32 top;                                                                                  /**< Top of the stack					*/
    JITUINT32 max_stack_size;                                                                       /**< Max size of the stack				*/
    void (*dupTop)(struct t_stack *_this);                                                          /**< Duplicate the top of the stack			*/
    void (*push)(struct t_stack *_this, ir_item_t *var);                                            /**< Push the element var to the top of the stack	*/
    void (*pop)(struct t_stack *_this, ir_item_t *var);                                             /**< Pop the element on the top of the stack and store it into the item element if it is not NULL	*/
    void (*adjustSize)(struct t_stack *_this);
    void (*cleanTop)(struct t_stack *_this);                                                        /**< Clean the top of the stack                         */
} t_stack;

typedef struct _CILStack {
    XanList         *stackes;               /**< List of stackes allocated			*/

    t_stack *       (*newStack)(struct _CILStack *cilStack);
    t_stack *       (*mergeStackes)(struct _CILStack *cilStack, t_stack *stack1, t_stack *stack2, ir_method_t *method, ir_instruction_t *beforeInst);
    t_stack *       (*cloneStack)(struct _CILStack *cilStack, t_stack *stack);                                      /**< Clone the stack	*/
    void (*destroy)(struct _CILStack *cilStack);
} _CILStack;

typedef struct _CILStack * CILStack;

CILStack newCILStack ();

#endif
