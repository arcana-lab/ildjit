/*
 * Copyright (C) 2011 - 2012 Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine_exception.cpp - This file handles functions that belong to the backend API related to exceptions.

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


void IRVM_throwBuiltinException (IRVM_t *self, JITUINT32 exceptionType){
    fprintf(stderr, "IRVM_throwBuiltinException: not supported.\n");
	//TODO: this is important only to handle exceptions
	abort();

	return ;
}

void IRVM_throwException (IRVM_t *self, void *object){
    fprintf(stderr, "IRVM_throwException: not supported.\n");

	//TODO: this is important only to handle exceptions
	abort();

	return ;
}

void IRVM_setExceptionHandler (IRVM_t *self, void * (*exceptionHandler)(int exception_type)){

	//TODO: this is important only to handle exceptions

	return ;
}
