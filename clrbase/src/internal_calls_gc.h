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
#ifndef INTERNAL_CALLS_GC_H
#define INTERNAL_CALLS_GC_H

#include <jitsystem.h>

void            System_GC_Collect (void);
void            System_GC_SuppressFinalize (void *object);
void            System_GC_WaitForPendingFinalizers (void);

/**
 * Return the maximum generation number
 *
 * This is a Mono internal call. Always returns 0, until I start coding a
 * real GC.
 *
 * @return the maximum generation number
 */
JITINT32 System_GC_get_MaxGeneration (void);

/**
 * Collect the given generation
 *
 * This is a Mono internal call.
 *
 * @param generation the generation to collect
 */
void System_GC_InternalCollect (JITINT32 generation);

#endif
