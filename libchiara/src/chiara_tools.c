/*
 * Copyright (C) 2010 - 2013  Campanoni Simone
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
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
// End

JITBOOLEAN TOOLS_isInstructionADominator (ir_method_t *method, JITUINT32 numInsts, ir_instruction_t *inst, ir_instruction_t *dominated, JITINT32 **doms, JITBOOLEAN(*dominance)(ir_method_t *, ir_instruction_t *, ir_instruction_t *)) {
    if (doms[inst->ID] == NULL) {
        doms[inst->ID]	= allocFunction(sizeof(JITINT32) * (numInsts+1));
        for (JITUINT32 count=0; count <= numInsts; count++) {
            doms[inst->ID][count]	= -1;
        }
    }
    assert(doms[inst->ID] != NULL);
    assert(dominated->ID <= numInsts);
    if (doms[inst->ID][dominated->ID] == -1) {
        doms[inst->ID][dominated->ID]	= (*dominance)(method, inst, dominated);
    }
    return doms[inst->ID][dominated->ID];
}
