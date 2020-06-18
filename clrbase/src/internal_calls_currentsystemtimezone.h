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

#ifndef INTERNAL_CALLS_CURRENTSYSTEMTIMEZONE_H
#define INTERNAL_CALLS_CURRENTSYSTEMTIMEZONE_H

#include <jitsystem.h>

/**
 * Get info about the current time zone
 *
 * This is a Mono internal call.
 *
 * The timeInfo array will be filled with the following informations:
 *
 * [0] = start of daylight saving time (in ticks)
 * [1] = end of daylight saving time (in ticks)
 * [2] = UTC offset (in ticks)
 * [3] = additional offset when daylight saving (in ticks)
 *
 * The timeNames array will be filled
 * [0] = name of this timezone when not daylight saving
 * [1]:  name of this timezone when daylight saving
 *
 * Notice that the two arrays are created by the internal call.
 *
 * @param year the year for which get timezone info
 * @param timeInfo will be filled with time related info
 * @param timeNames will be filled with the names of system timezone
 *
 * @return JITTRUE on success, JITFALSE otherwise
 */
JITBOOLEAN System_CurrentSystemTimeZone_GetTimeZoneData (JITINT32 year,
        void** timeInfo,
        void** timeNames);

#endif /* INTERNAL_CALLS_CURRENTSYSTEMTIMEZONE_H */
