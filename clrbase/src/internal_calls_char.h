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

#ifndef INTERNAL_CALLS_CHAR_H
#define INTERNAL_CALLS_CHAR_H

#include <jitsystem.h>

/**
 * Initialize pointers to char conversion tables
 *
 * This is a Mono internal call.
 *
 * @param categoryData address of the category data table pointer
 * @param numericData address of the numeric data table pointer
 * @param numericDataValues address of the numeric data values table pointer
 * @param toLowerDataLow address of the lower data low table pointer
 * @param toLowerDataHigh address of the lower data high table pointer
 * @oaram toUpperDataLow address of the upper data low table pointer
 * @param toUpperDataHigh address of the upper data high table pointer
 */
void System_Char_GetDataTablePointers (JITINT8** categoryData,
                                       JITINT8** numericData,
                                       JITFLOAT64** numericDataValues,
                                       JITUINT16** toLowerDataLow,
                                       JITUINT16** toLowerDataHigh,
                                       JITUINT16** toUpperDataLow,
                                       JITUINT16** toUpperDataHigh);

#endif /* INTERNAL_CALLS_CHAR_H */
