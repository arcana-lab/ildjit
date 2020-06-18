/*
 * Copyright (C) 2010  Campanoni Simone
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
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>
#include <ir_optimization_interface.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
// End

static inline void startProgram (void);

static JITUINT64 programStartTicks = 0;

void PROGRAM_profileStartExecutionByUsingSymbol (ir_symbol_t *sym) {
    ir_method_t 		*m;
    ir_instruction_t 	*firstInst;

    /* Fetch the entry point of the program	*/
    m   = METHODS_getMainMethod();
    if (m == NULL){
        m = IRPROGRAM_getEntryPointMethod();
    }
    assert(m != NULL);

    /* Fetch the first instruction after the*
     * nops					*/
    firstInst = IRMETHOD_getFirstInstructionNotOfType(m, IRNOP);
    assert(firstInst != NULL);
    assert(firstInst->type != IRNOP);

    /* Instrument the code			*/
    IRMETHOD_newNativeCallInstructionByUsingSymbolsBefore(m, firstInst, NULL, sym, NULL, NULL);

    return ;
}

void PROGRAM_profileStartExecution (void (*startExecution) (void)) {
    ir_method_t 		*m;
    ir_instruction_t 	*firstInst;

    /* Fetch the entry point of the program	*/
    m   = METHODS_getMainMethod();
    if (m == NULL){
        m = IRPROGRAM_getEntryPointMethod();
    }
    assert(m != NULL);

    /* Fetch the first instruction after the*
     * nops					*/
    firstInst = IRMETHOD_getFirstInstruction(m);
    assert(firstInst != NULL);
    while (firstInst->type == IRNOP) {
        firstInst = IRMETHOD_getNextInstruction(m, firstInst);
        assert(firstInst != NULL);
    }
    assert(firstInst != NULL);
    assert(firstInst->type != IRNOP);

    /* Instrument the code			*/
    IRMETHOD_newNativeCallInstructionBefore(m, firstInst, "profileStartProgram", startExecution, NULL, NULL);

    return ;
}

void PROGRAM_profileTotalExecutionTime (void) {
    PROGRAM_profileStartExecution(startProgram);
}

JITUINT64 PROGRAM_profileTotalExecutionTimeEnd (void) {
    JITUINT64 totalTicks;

    /* Compute the elapsed time		*/
    totalTicks = (TIME_getClockTicks() - programStartTicks);

    /* Return				*/
    return totalTicks;
}

static inline void startProgram (void) {
    programStartTicks = TIME_getClockTicks();
}
