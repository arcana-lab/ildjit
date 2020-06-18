/*
 * Copyright (C) 2012  Simone Campanoni, Timothy M Jones
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
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_ddg.h>
// End

void declare_memory_aliases (ir_method_t *method) {
    JITUINT32	instructionsNumber;

    /* Destroy previous data		*/
    IRMETHOD_destroyMemoryAliasInformation(method);

    /* Declare memory aliases		*/
    instructionsNumber	= IRMETHOD_getInstructionsNumber(method);
    for (JITUINT32 count=0; count < instructionsNumber; count++) {
        ir_instruction_t	*inst;

        /* Fetch the instruction								*/
        inst	= IRMETHOD_getInstructionAtPosition(method, count);

        /* Set every memory aliases related to variables used by the current instruction	*/
        for (JITUINT32 count2=0; count2 < instructionsNumber; count2++) {
            ir_instruction_t	*inst2;
            ir_item_t		*var1;
            ir_item_t		*var2;

            /* Check if there is a data dependence between the current couple of instructions.
             * Notice that in case there isn't, there is no memory aliases between variables used by the current couple of instructions.
             */
            inst2	= IRMETHOD_getInstructionAtPosition(method, count2);
            if (	(!IRMETHOD_isInstructionDataDependentThroughMemory(method, inst, inst2))	&&
                    (!IRMETHOD_isInstructionDataDependentThroughMemory(method, inst2, inst))	) {
                continue;
            }

            /* Check if these instructions are memory operations.
             */
            if (	(!IRMETHOD_isAMemoryInstruction(inst))	||
                    (!IRMETHOD_isAMemoryInstruction(inst2))	) {
                continue;
            }

            /* Check if the base addresses are variables.
             * In case just one of them is not a variable, there is no alias.
             */
            var1	= IRMETHOD_getInstructionParameter1(inst);
            var2	= IRMETHOD_getInstructionParameter2(inst2);
            if (	(!IRDATA_isAVariable(var1))	&&
                    (!IRDATA_isAVariable(var2))	) {
                continue;
            }

            /* Set the memory aliases due to the current data dependence.
             */
            IRMETHOD_setAlias(method, inst, (var1->value).v, (var2->value).v);
        }
    }

    return ;
}
