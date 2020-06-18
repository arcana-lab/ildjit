/*
 * Copyright (C) 2009  Campanoni Simone
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
#include <jitsystem.h>

// My headers
#include <internal_calls_environment.h>
#include <internal_calls_utilities.h>
#include <internal_calls_misc.h>
// End

extern t_system *ildjitSystem;

/* Get current time in ticks */
JITINT64 System_DateTime_GetNow (void) {
    JITINT64 	now;

    METHOD_BEGIN(ildjitSystem, "System.DateTime.GetNow");

    now = Platform_TimeMethods_GetCurrentTime();

    METHOD_END(ildjitSystem, "System.DateTime.GetNow");
    return now;
}
