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

#ifndef INTERNAL_CALLS_DATETIME_H
#define INTERNAL_CALLS_DATETIME_H

#include <jitsystem.h>

/**
 * Get the now time in ticks
 *
 * This is a Mono internal call.
 *
 * @return now, expressed in tick (100 nanoseconds)
 */
JITINT64 System_DateTime_GetNow (void);

#endif /* INTERNAL_CALLS_DATETIME_H */
