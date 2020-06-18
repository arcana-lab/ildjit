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
#include <stdio.h>
#include <math.h>

// My header
#include <internal_calls_math.h>
#include <internal_calls_utilities.h>
// End

extern t_system *ildjitSystem;

JITFLOAT64 System_Math_Sin (JITFLOAT64 num) {
    METHOD_BEGIN(ildjitSystem, "System.Math.Sin");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Sin");
    return sin(num);
}

JITFLOAT64 System_Math_Sinh (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Sinh");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Sinh");
    return sinh(num);
}

JITFLOAT64 System_Math_Cos (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Cos");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Cos");
    return cos(num);
}

JITFLOAT64 System_Math_Cosh (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Cosh");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Cosh");
    return cosh(num);
}

JITFLOAT64 System_Math_Exp (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Exp");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Exp");
    return exp(num);
}

JITFLOAT64 System_Math_Sqrt (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Sqrt");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Sqrt");
    return sqrt(num);
}

JITFLOAT64 System_Math_Tan (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Tan");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Tan");
    return tan(num);
}

JITFLOAT64 System_Math_Tanh (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Tanh");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Tanh");
    return tanh(num);
}

JITFLOAT64 System_Math_Acos (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Acos");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Acos");
    return acos(num);
}

JITFLOAT64 System_Math_Asin (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Asin");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Asin");
    return asin(num);
}

JITFLOAT64 System_Math_Atan (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Atan");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Atan");
    return atan(num);
}

JITFLOAT64 System_Math_RoundDouble (JITFLOAT64 num, JITINT32 digits) {
    JITFLOAT64 power;
    JITFLOAT64 rounded;
    JITFLOAT64 value;

    METHOD_BEGIN(ildjitSystem, "System.Math.RoundDouble");

    rounded = round(num);
    if (digits == 0 || rounded == num) {

        /* Simple rounding, or the value is already an integer */
        value = rounded;
    } else {
        power = pow(10, (JITFLOAT64) digits);
        value = (round(((JITFLOAT64) num) * power) / power);
    }

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.RoundDouble");
    return value;
}

JITFLOAT64 System_Math_Round (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Round");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Round");
    return round(num);
}

JITFLOAT64 System_Math_IEEERemainder (JITFLOAT64 num1, JITFLOAT64 num2) {

    METHOD_BEGIN(ildjitSystem, "System.Math.IEEERemainder");

    /* Return			*/
    METHOD_BEGIN(ildjitSystem, "System.Math.IEEERemainder");
    return remainder(num1, num2);
}

JITFLOAT64 System_Math_Ceiling (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Ceiling");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Ceiling");
    return ceil(num);
}

JITFLOAT64 System_Math_Atan2 (JITFLOAT64 num1, JITFLOAT64 num2) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Atan2");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Atan2");
    return atan2(num1, num2);
}

JITFLOAT64 System_Math_Log (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Log");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Log");
    return log(num);
}

JITFLOAT64 System_Math_Log10 (JITFLOAT64 num) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Log10");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Log10");
    return log10(num);
}

JITFLOAT64 System_Math_Floor (JITFLOAT64 num) {
    JITFLOAT64 result;

    METHOD_BEGIN(ildjitSystem, "System.Math.Floor");

    result = floor(num);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Floor");
    return result;
}

JITFLOAT64 System_Math_Pow (JITFLOAT64 base, JITFLOAT64 exponent) {

    METHOD_BEGIN(ildjitSystem, "System.Math.Pow");

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Math.Pow");
    return pow(base, exponent);
}

JITBOOLEAN System_Single_IsNaN (JITFLOAT32 value) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "System.Single.IsNaN");

    result = isnan(value);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Single.IsNaN");
    return result;
}

JITINT32 System_Single_TestInfinity (JITFLOAT32 value) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.Single.TestInfinity");

    result = isinf(value);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Single.TestInfinity");
    return result;
}

JITBOOLEAN System_Double_IsNaN (JITFLOAT64 value) {
    JITBOOLEAN result;

    METHOD_BEGIN(ildjitSystem, "System.Double.IsNaN");

    result = isnan(value);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Double.IsNaN");
    return result;
}

JITINT32 System_Double_TestInfinity (JITFLOAT64 value) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.Double.TestInfinity");

    result = isinf(value);

    /* Return			*/
    METHOD_END(ildjitSystem, "System.Double.TestInfinity");
    return result;
}
