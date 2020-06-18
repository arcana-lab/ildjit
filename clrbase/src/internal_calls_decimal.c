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
#include <ildjit_locale.h>
#include <framework_garbage_collector.h>
#include <ildjit.h>

// My headers
#include <internal_calls_utilities.h>
#include <internal_calls_decimal.h>
// End

/* Errors from Mono internal calls */
#define MONO_DECIMAL_NO_ERROR 0
#define MONO_DECIMAL_FINISHED 1
#define MONO_DECIMAL_OVERFLOW 2
#define MONO_DECIMAL_INVALID_CHARACTER 2
#define MONO_DECIMAL_INTERNAL_ERROR 3
#define MONO_DECIMAL_INVALID_BITS 4
#define MONO_DECIMAL_DIVIDE_BY_ZERO 5
#define MONO_DECIMAL_BUFFER_OVERFLOW 6

extern t_system *ildjitSystem;

void System_Decimal_ctor_f (void *_this, JITFLOAT32 value) {

    /* Assertions			*/
    assert(_this != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.ctor(float value)");

    /* Make the decimal		*/
    if ((((ildjitSystem->cliManager).CLR.decimalManager)).decimalFromFloat(_this, value) != 0) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowOverflow");
    }

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.ctor(float value)");
    return;
}

void System_Decimal_ctor_d (void *_this, JITFLOAT64 value) {
    METHOD_BEGIN(ildjitSystem, " System.Decimal.ctor(double value)");

    /* Make the decimal		*/
    if ((((ildjitSystem->cliManager).CLR.decimalManager)).decimalFromDouble(_this, value) != 0) {
        ILDJIT_throwExceptionWithName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowOverflow");
    }
    assert(_this != NULL);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.ctor(double value)");
    return;
}

JITFLOAT32 System_Decimal_ToSingle (void* value) {
    JITFLOAT32 result;

    assert(value != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.ToSingle");

    result = (JITFLOAT32) ((((ildjitSystem->cliManager).CLR.decimalManager)).decimalToDouble(value));

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.ToSingle");
    return result;
}

JITFLOAT64 System_Decimal_ToDouble (void *value) {
    JITFLOAT64 result;

    /* Assertions			*/
    assert(value != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.ToDouble");

    result = (((ildjitSystem->cliManager).CLR.decimalManager)).decimalToDouble(value);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.ToDouble");
    return result;
}

void System_Decimal_Add (void *x, void *y, void *result) {

    /* Assertions			*/
    assert(result != NULL);
    assert(x != NULL);
    assert(y != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Add");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalAddWithoutObjectAllocation(x, y, DECIMAL_ROUND_MODE, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Add");
    return;
}

JITINT32 System_Decimal_Compare (void* x, void* y) {
    JITINT32 result;

    assert(x != NULL);
    assert(y != NULL);

    METHOD_BEGIN(ildjitSystem, " System.Decimal.Compare");

    result = (((ildjitSystem->cliManager).CLR.decimalManager)).decimalCompare(x, y);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Compare");
    return result;
}

void System_Decimal_Divide (void* x, void* y, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(y != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Divide");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalDivide(x, y, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Divide");
    return;
}

void System_Decimal_Floor (void* x, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Floor");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalFloor(x, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Floor");
    return;
}

void System_Decimal_Remainder (void* x, void* y, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(y != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Remainder");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalRemainder(x, y, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Remainder");
    return;
}

void System_Decimal_Multiply (void* x, void* y, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(y != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Multiply");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalMultiply(x, y, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Multiply");
    return;
}

void System_Decimal_Negate (void* x, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Negate");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalNegate(x, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Negate");
    return;
}

void System_Decimal_Round (void* x, JITINT32 decimals, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Round");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalRound(x, decimals, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Round");
    return;
}

void System_Decimal_Subtract (void* x, void* y, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(y != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Subtract");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalSubtract(x, y, DECIMAL_ROUND_MODE, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Subtract");
    return;
}

void System_Decimal_Truncate (void* x, void *result) {

    /* Assertions			*/
    assert(x != NULL);
    assert(result != NULL);
    METHOD_BEGIN(ildjitSystem, " System.Decimal.Truncate");

    (((ildjitSystem->cliManager).CLR.decimalManager)).decimalTruncate(x, result);

    /* Return			*/
    METHOD_END(ildjitSystem, " System.Decimal.Truncate");
    return;
}

/* Performs x += y */
JITINT32 System_Decimal_decimalIncr (void* x, void* y) {
    t_running_garbage_collector* garbageCollector;
    void* sum;
    JITNUINT decimalSize;
    JITINT32 status;

    garbageCollector = &ildjitSystem->garbage_collectors.gc;

    METHOD_BEGIN(ildjitSystem, "System.Decimal.decimalIncr");

    System_Decimal_Add(x, y, &sum);
    if (sum == NULL) {
        status = MONO_DECIMAL_INTERNAL_ERROR;
    } else {
        status = MONO_DECIMAL_NO_ERROR;
        decimalSize = garbageCollector->getSize(sum) - HEADER_FIXED_SIZE;
        memcpy(x, sum, decimalSize);
    }

    METHOD_END(ildjitSystem, "System.Decimal.decimalIncr");

    return status;
}

/* Compares x and y */
JITINT32 System_Decimal_decimalCompare (void* x, void* y) {
    JITINT32 result;

    METHOD_BEGIN(ildjitSystem, "System.Decimal.decimalCompare");

    result = System_Decimal_Compare(x, y);

    METHOD_END(ildjitSystem, "System.Decimal.decimalCompare");

    return result;
}
