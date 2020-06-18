/*
 * Copyright (C) 2008  Campanoni Simone
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
#ifndef INTERNAL_CALLS_DECIMAL_H
#define INTERNAL_CALLS_DECIMAL_H

#include <jitsystem.h>


/****************************************************************************************************************************
*                                               DECIMAL									    *
****************************************************************************************************************************/
void            System_Decimal_ctor_f (void *_this, JITFLOAT32 value);
void            System_Decimal_ctor_d (void *_this, JITFLOAT64 value);
JITFLOAT32      System_Decimal_ToSingle (void* value);
JITFLOAT64      System_Decimal_ToDouble (void* value);
JITINT32        System_Decimal_Compare (void* x, void* y);
void            System_Decimal_Add (void *x, void *y, void *result);
void            System_Decimal_Divide (void* x, void* y, void *result);
void            System_Decimal_Floor (void* x, void *result);
void            System_Decimal_Truncate (void* x, void *result);
void            System_Decimal_Subtract (void* x, void* y, void *result);
void            System_Decimal_Multiply (void* x, void* y, void *result);
void            System_Decimal_Negate (void* x, void *result);
void            System_Decimal_Round (void* x, JITINT32 decimals, void *result);
void            System_Decimal_Remainder (void* x, void* y, void *result);

/**
 * Performs x += y
 *
 * This is a Mono internal call.
 *
 * @param x pointer to the first operand
 * @param y pointer to the second operand
 *
 * @return 0 if all went ok, an error code otherwise
 */
JITINT32 System_Decimal_decimalIncr (void* x, void* y);

/**
 * Compare x and y
 *
 * This is a Mono internal call.
 *
 * @param x pointer to the first operand
 * @param y pointer to the second operand
 *
 * @return a negative number if x is less than y, 0 if x is equal to y, a
 *         positive number otherwise
 */
JITINT32 System_Decimal_decimalCompare (void* x, void* y);

#endif
