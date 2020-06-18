/*
 * Copyright (C) 2012  Campanoni Simone
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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_deadcode.h>
#include <config.h>
// End

void erase_useless_parameters (ir_method_t *method) {
    JITUINT32 	count;
    JITUINT32 	instructionsNumber;

    instructionsNumber	= IRMETHOD_getInstructionsNumber(method);
    for (count=0; count < instructionsNumber; count++) {
        ir_instruction_t	*inst;
        inst	= IRMETHOD_getInstructionAtPosition(method, count);
        switch (inst->type) {
            case IRLOADREL:
                if (IRMETHOD_getInstructionParametersNumber(inst) > 2) {
                    IRMETHOD_eraseInstructionParameter(method, inst, 3);
                }
                assert(IRMETHOD_getInstructionParametersNumber(inst) == 2);
                break;
        }
    }

    return ;
}
