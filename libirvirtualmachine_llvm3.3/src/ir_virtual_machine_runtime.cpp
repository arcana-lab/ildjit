/*
 * Copyright (C) 2011 - 2012 Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine.cpp - This is a translator from the IR language into the assembly language.

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
#include <math.h>
#include <ir_virtual_machine.h>
#include <compiler_memory_manager.h>
#include <ir_optimizer.h>

// My headers
#include <utilities.h>
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
// End

extern "C" {

extern IRVM_t *localVM;

JITINT32 ir_isnanf (JITFLOAT32 value){
	return isnan(value);
}

JITINT32 ir_isnan (JITFLOAT64 value){
	return isnan(value);
}

JITINT32 ir_isinff (JITFLOAT32 value){
	return isinf(value);
}

JITINT32 ir_isinf (JITFLOAT64 value){
	return isinf(value);
}

void ir_leaveExecution (JITINT32 exitCode){
	assert(localVM != NULL);
	localVM->leaveExecution(exitCode);
	abort();
}

size_t ir_strlen (char *str){
	return strlen(str);
}

void * ir_memcpy (void *s1, void *s2, JITINT32 n){
	assert(s1 != NULL);
	assert(s2 != NULL);
	return memcpy(s1, s2, n);
}

void ir_memset (void *s, int c, int n){
	memset(s, c, n);

	return ;
}

int ir_memcmp (char *s1, char *s2, JITINT32 n){
	return memcmp(s1, s2, n);
}

int ir_strcmp (char *s1, char *s2){
	return strcmp(s1, s2);
}

char * ir_strchr (char *s, JITINT32 c){
	return strchr(s, c);
}

void * ir_malloc (size_t size){
	void *ptr;
	if (size == 0){
		size	= 4;
	}
	ptr	= allocMemory(size);
	return ptr;
}

void * ir_calloc (size_t nmemb, size_t size){
	return calloc(nmemb, size);
}

}
