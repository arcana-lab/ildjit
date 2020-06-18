/*
 * Copyright (C) 2014  Campanoni Simone
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
#include <assert.h>
#include <jitsystem.h>
#include <platform_API.h>
#include <cam.h>
#include <chiara.h>

// My headers
#include <optimizer_memorytracer.h>
#include <code_injector.h>
// End

extern XanHashTable				*methodNamesTable;
static JITUINT32				nestingLevel 	= 0;
static JITBOOLEAN				enablePrinter	= JITFALSE;

void RUNTIME_init (void) {
    CAM_init(CAM_MEMORY_PROFILE);
    nestingLevel	= 0;

    return ;
}

void RUNTIME_shutdown (void) {
    CAM_shutdown(CAM_MEMORY_PROFILE);

    return ;
}


void RUNTIME_enter_loop (loop_profile_t *l) {
    nestingLevel++;
    enablePrinter	= JITTRUE;

    return ;
}

void RUNTIME_end_loop (void) {

    /* Assertions.
     */
    assert(nestingLevel > 0);

    nestingLevel--;

    if (nestingLevel == 0) {
        enablePrinter	= JITFALSE;
    }

    return ;
}

void dump_call_instruction (JITUINT32 instructionID) {

    return ;
}

void dump_os_instruction (JITUINT32 instructionPosition) {
    if (enablePrinter){
    	CAM_mem(instructionPosition, 0, JITMAXUINT32, 0, 0, 0, JITMAXUINT32);
    }

    return ;
}

void dump_start_method (char *methodName) {
    return ;
}

void dump_end_method (void) {
    return ;
}

void dump_memory_instruction (JITUINT32 instructionPosition, void *input1, JITUINT32 input1Size, void *input2, JITUINT32 input2Size, void *output, JITUINT32 outputSize) {
    if (enablePrinter) {
        CAM_mem(instructionPosition, (JITNUINT)input1, input1Size, (JITNUINT)input2, input2Size, (JITNUINT)output, outputSize);
    }

    return ;
}
