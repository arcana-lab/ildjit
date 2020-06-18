/*
 * Copyright (C) 2006
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
#include <assert.h>
#include <jit_metadata.h>
#include <ir_virtual_machine.h>
#include <platform_API.h>

// My headers
#include <ildjit_profiler.h>
#include <iljit.h>
#include <translation_pipeline.h>
// End

extern t_system *ildjitSystem;

static inline JITINT32 internal_recompiler (Method method, Method caller, ir_instruction_t *inst);

JITINT32 recompiler_with_profile (Method method, Method caller, ir_instruction_t *inst) {
    ProfileTime startTime;
    JITINT32 isCCTORThread;
    JITINT32 returnValue;

    /* Initialize the variables		*/
    isCCTORThread = JITFALSE;

    /* Profiling					*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 1) {

        /* Check if the current thread is a     *
         * cctor.				*/
        isCCTORThread = (ildjitSystem->pipeliner).isCCTORThread(&(ildjitSystem->pipeliner), PLATFORM_getSelfThreadID());
        if (!isCCTORThread) {

            /* Store the start time			*/
            profilerStartTime(&startTime);
        }
    }

    /* Compile the method			*/
    returnValue	= internal_recompiler(method, caller, inst);

    /* Profiling					*/
    if ((ildjitSystem->IRVM).behavior.profiler >= 1) {

        /* Check if the current thread is a     *
         * cctor.				*/
        if (!isCCTORThread) {
            JITFLOAT32 wallTime;

            /* Store the time elapsed		*/
            wallTime = 0;
            (ildjitSystem->profiler).trampolines_time += profilerGetTimeElapsed(&startTime, &wallTime);
            (ildjitSystem->profiler).wallTime.trampolines_time += wallTime;
        }
        (ildjitSystem->profiler).trampolinesTaken++;
    }

    /* Return					*/
    return returnValue;
}

JITINT32 recompiler (Method method, Method caller, ir_instruction_t *inst) {
    return internal_recompiler(method, caller, inst);
}

static inline JITINT32 internal_recompiler (Method method, Method caller, ir_instruction_t *inst) {

    /* Assertions				*/
    assert(method != NULL);
    PDEBUG("RECOMPILER: recompiler: Start\n");

    /* Update the execution distance	*/
    if ((ildjitSystem->IRVM).behavior.dla) {
        if (!((ildjitSystem->pipeliner).isCCTORThread(&(ildjitSystem->pipeliner), PLATFORM_getSelfThreadID()))) {
            method->lock(method);
            method->executionProbability = 1;
            if (caller != NULL && inst != NULL) {
                method->addToTrampolinesSet(method, caller, inst);
            }
            method->unlock(method);
        }
    }

    /* Insert the method into the pipeline	*/
    PDEBUG("RECOMPILER: recompiler:		Recompile the method %s\n", method->getFullName(method));
    (ildjitSystem->pipeliner).synchInsertMethod(&(ildjitSystem->pipeliner), method, MAX_METHOD_PRIORITY);

    /* Save Trace Profile */
    if (	(!(ildjitSystem->IRVM).behavior.aot) 		&&
            ((ildjitSystem->IRVM).behavior.pgc == 1)	) {
        MANFRED_appendMethodToProfile(&(ildjitSystem->manfred), method);
    }

    /* Return				*/
    PDEBUG("RECOMPILER: recompiler: Exit\n");
    return JITTRUE;
}
