/*
 * Copyright (C) 2011 - 2012 Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine_stack_trace.cpp - This file handles stack trace functions that belong to the backend API.

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

#include <ir_virtual_machine.h>
#include <compiler_memory_manager.h>
#include <iostream>
#include <fstream>
#include <ir_optimizer.h>
#include <ir_optimization_interface.h>
#include <xanlib.h>

// My headers
#include <utilities.h>
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <config.h>
// End

JITUINT32 IRVM_getStackTraceOffset (IRVM_t *IRVM, IRVM_stackTrace *stack, JITUINT32 position){
	//TODO: this is important only to handle exceptions
	abort();

	return 0;
}

void * IRVM_getStackTraceFunctionAt (IRVM_t *IRVM, IRVM_stackTrace *stack, JITUINT32 position){
	//TODO: this is important only to handle exceptions
	abort();

	return NULL;
}

JITUINT32 IRVM_getStackTraceSize (IRVM_stackTrace *stack){
	//TODO: this is important only to handle exceptions
	abort();

	return 0;
}

IRVM_stackTrace * IRVM_getStackTrace (void){
	IRVM_stackTrace *s;

	s 		= (IRVM_stackTrace *)allocFunction(sizeof(IRVM_stackTrace));
	//s->stack_trace = jit_exception_get_stack_trace(); TODO

	return s;
}

void IRVM_destroyStackTrace (IRVM_stackTrace *stack){
	//TODO: this is important only to handle exceptions
	abort();

	return ;
}
