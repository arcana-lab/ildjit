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
#ifndef INTERNAL_CALLS_MATH_H
#define INTERNAL_CALLS_MATH_H

#include <jitsystem.h>

/****************************************************************************************************************************
*                                               MATH									    *
****************************************************************************************************************************/
JITFLOAT64      System_Math_Acos (JITFLOAT64 num);
JITFLOAT64      System_Math_Asin (JITFLOAT64 num);
JITFLOAT64      System_Math_Atan (JITFLOAT64 num);
JITFLOAT64      System_Math_Atan2 (JITFLOAT64 num1, JITFLOAT64 num2);
JITFLOAT64      System_Math_Ceiling (JITFLOAT64 num);
JITFLOAT64      System_Math_Cos (JITFLOAT64 num);
JITFLOAT64      System_Math_Cosh (JITFLOAT64 num);
JITFLOAT64      System_Math_Exp (JITFLOAT64 num);
JITFLOAT64      System_Math_Floor (JITFLOAT64 num);
JITFLOAT64      System_Math_IEEERemainder (JITFLOAT64 num1, JITFLOAT64 num2);
JITFLOAT64      System_Math_Log (JITFLOAT64 num);
JITFLOAT64      System_Math_Log10 (JITFLOAT64 num);
JITFLOAT64      System_Math_Pow (JITFLOAT64 base, JITFLOAT64 exponent);
JITFLOAT64      System_Math_Round (JITFLOAT64 num);
JITFLOAT64      System_Math_RoundDouble (JITFLOAT64 num, JITINT32 digits);
JITFLOAT64      System_Math_Sin (JITFLOAT64 num);
JITFLOAT64      System_Math_Sinh (JITFLOAT64 num);
JITFLOAT64      System_Math_Sqrt (JITFLOAT64 num);
JITFLOAT64      System_Math_Tan (JITFLOAT64 num);
JITFLOAT64      System_Math_Tanh (JITFLOAT64 num);

/****************************************************************************************************************************
*                                               DOUBLE									    *
****************************************************************************************************************************/
JITBOOLEAN      System_Double_IsNaN (JITFLOAT64 value);
JITINT32        System_Double_TestInfinity (JITFLOAT64 value);

/****************************************************************************************************************************
*                                               SINGLE									    *
****************************************************************************************************************************/
JITBOOLEAN      System_Single_IsNaN (JITFLOAT32 value);
JITINT32        System_Single_TestInfinity (JITFLOAT32 value);

#endif
