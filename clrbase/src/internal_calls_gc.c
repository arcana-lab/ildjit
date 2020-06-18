/*
 * Copyright (C) 2009  Campanoni Simone
 *
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

// My headers
#include <internal_calls_gc.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

void System_GC_WaitForPendingFinalizers (void) {
    METHOD_BEGIN(ildjitSystem, "System.GC.WaitForPendingFinalizers");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.GC.WaitForPendingFinalizers");
    return;
}

void System_GC_SuppressFinalize (void *object) {

    METHOD_BEGIN(ildjitSystem, "System.GC.SuppressFinalize");

    (ildjitSystem->garbage_collectors).gc.setFinalizedFlag(object);

    /* Return		*/
    METHOD_END(ildjitSystem, "System.GC.SuppressFinalize");
    return;
}

void System_GC_Collect (void) {

    /* Assertions		*/
    METHOD_BEGIN(ildjitSystem, "System.GC.Collect");

    /* Call the GC		*/
    if (!(ildjitSystem->IRVM).behavior.noExplicitGarbageCollection) {
        (ildjitSystem->garbage_collectors).gc.collect();
    }

    /* Return		*/
    METHOD_END(ildjitSystem, "System.GC.Collect");
    return;
}

/* Get maximum generation number */
JITINT32 System_GC_get_MaxGeneration (void) {
    JITINT32 maximumGeneration;

    METHOD_BEGIN(ildjitSystem, "System.GC.get_MaxGeneration");

    maximumGeneration = 0;

    METHOD_END(ildjitSystem, "System.GC.get_MaxGeneration");

    return maximumGeneration;
}

/* Collect given generation */
void System_GC_InternalCollect (JITINT32 generation) {
    METHOD_BEGIN(ildjitSystem, "System.GC.InternalCollect");

    System_GC_Collect();

    METHOD_END(ildjitSystem, "System.GC.InternalCollect");

}
