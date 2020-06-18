/*
 * Copyright (C) 2006 - 2011  Simone Campanoni
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
#ifndef IR_VIRTUAL_MACHINE_BACKEND_H
#define IR_VIRTUAL_MACHINE_BACKEND_H

#include <xanlib.h>
#include <jitsystem.h>
#include <ir_optimizer.h>
#include <garbage_collector.h>
#include <framework_garbage_collector.h>
#include <ir_virtual_machine.h>

// My headers
#include <jit/jit.h>
// End

typedef struct {
	jit_function_t function;                        /**< Backend-dependent function									*/
	jit_value_t     *locals;                        /**< Variables of the backend-dependent function						*/
	jit_type_t signature;                           /**< Backend-dependent signature								*/
	void            *nativeFunction;
	void            *entryPoint;                    /**< First memory address where the machine code of the function has been stored.		*/
} t_jit_function_internal;

typedef struct {
	jit_context_t context;                                                  /**< Libjit context						*/
	jit_type_t basicExceptionConstructorJITSignature;                       /**< JIT signature for the exception constructor.		*/
	jit_type_t leaveExecutionSignature;
	jit_type_t signAddRootSet;
	jit_type_t signPopLastRootSet;
	jit_type_t signAllocObject;
	jit_type_t signFreeObject;
	jit_type_t signCreateArray;
} IRVM_t_internal;

#endif
