/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
#ifndef LIB_DECIMAL_H
#define LIB_DECIMAL_H

#include <jitsystem.h>
#include <metadata/metadata_types.h>

typedef struct {
    TypeDescriptor          *DecimalType;
    FieldDescriptor         *flagsField;
    FieldDescriptor         *highField;
    FieldDescriptor         *middleField;
    FieldDescriptor         *lowField;
    JITINT32 flagsFieldOffset;
    JITINT32 highFieldOffset;
    JITINT32 middleFieldOffset;
    JITINT32 lowFieldOffset;

    void *          (*decimalNew)(void);
    JITINT8 (*decimalFromFloat)(void *_this, JITFLOAT32 value);
    JITINT8 (*decimalFromDouble)(void *_this, JITFLOAT64 value);
    JITFLOAT64 (*decimalToDouble)(void *value);
    void *          (*decimalAdd)(void *x, void *y, JITINT32 roundMode);
    void (*decimalAddWithoutObjectAllocation)(void *valuea, void *valueb, JITINT32 roundMode, void *objResult);
    JITINT32 (*decimalCompare)(void *valuea, void *valueb);
    void (*decimalDivide)(void *x, void *y, void *objResult);
    void (*decimalFloor)(void *x, void *objResult);
    void (*decimalTruncate)(void* x, void *objResult);
    void (*decimalSubtract)(void* x, void* y, JITINT32 roundMode, void *objResult);
    void (*decimalRemainder)(void *x, void *y, void *objResult);
    void (*decimalMultiply)(void* x, void* y, void *objResult);
    void (*decimalNegate)(void* x, void *objResult);
    void (*decimalRound)(void* x, JITINT32 decimals, void *objResult);
    void (*initialize)(void);
} t_decimalManager;

void init_decimalManager (t_decimalManager *decimalManager);

/*
 * Useful min/max constants for the above types.
 */
#define IL_MIN_INT32                            ((JITINT32) 0x80000000L)
#define IL_MAX_INT32                            ((JITINT32) 0x7FFFFFFFL)
#define IL_MAX_UINT32                           ((JITUINT32) 0xFFFFFFFFL)
#define IL_MIN_INT64                            ((JITINT64) 0x8000000000000000LL)
#define IL_MAX_INT64                            ((JITINT64) 0x7FFFFFFFFFFFFFFFLL)
#define IL_MAX_UINT64                           ((JITUINT64) 0xFFFFFFFFFFFFFFFFLL)

#define DECIMAL_ROUND_MODE              IL_DECIMAL_ROUND_HALF_EVEN

/*
 * Permitted decimal rounding modes.
 */
#define IL_DECIMAL_ROUND_DOWN                   0
#define IL_DECIMAL_ROUND_HALF_UP                1
#define IL_DECIMAL_ROUND_HALF_EVEN              2

//The value 2^96 as a double value.
#define MAX_DECIMAL_AS_DOUBLE   ((JITFLOAT64) 79228162514264337593543950336.0)

//The value 2^64 as a double value.
#define TWO_TO_64               ((JITFLOAT64) 18446744073709551616.0)

#endif
