/*
 * Copyright (C) 2006  Campanoni Simone
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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <platform_API.h>

// My headers
#include <lib_decimal.h>
#include <climanager_system.h>
// End


//Determine if a decimal value is negative.
#define DECIMAL_IS_NEG(value)   ((internal_getflags(value) & (JITUINT32) 0x80000000) != 0)

//Get the decimal point position from a decimal value.
#define DECIMAL_GETPT(value)    ((JITINT32) ((internal_getflags(value) >> 16) & (JITUINT32) 0xFF))

//Make a "flags" value from a sign and a decimal point position.
#define DECIMAL_MKFLAGS(sign, decpt)     ((((JITUINT32) (decpt)) << 16) | ((sign) ? (JITUINT32) 0x80000000 : (JITUINT32) 0))

typedef struct {
    JITUINT32 flags;
    JITUINT32 low;
    JITUINT32 middle;
    JITUINT32 high;
} decimalType;

/*
 * Powers of 10 as double values.
 */
static JITFLOAT64 const powOfTen[29] = {
    1.0,
    10.0,
    100.0,
    1000.0,
    10000.0,
    100000.0,
    1000000.0,
    10000000.0,
    100000000.0,
    1000000000.0,
    10000000000.0,
    100000000000.0,
    1000000000000.0,
    10000000000000.0,
    100000000000000.0,
    1000000000000000.0,
    10000000000000000.0,
    100000000000000000.0,
    1000000000000000000.0,
    10000000000000000000.0,
    100000000000000000000.0,
    1000000000000000000000.0,
    10000000000000000000000.0,
    100000000000000000000000.0,
    1000000000000000000000000.0,
    10000000000000000000000000.0,
    100000000000000000000000000.0,
    1000000000000000000000000000.0,
    10000000000000000000000000000.0
};

/* Names of the fields inside the PNET BCL */
#define PNET_DECIMAL_FIELD_FLAGS (JITINT8 *) "flags"
#define PNET_DECIMAL_FIELD_HIGH (JITINT8 *) "high"
#define PNET_DECIMAL_FIELD_MIDDLE (JITINT8 *) "middle"
#define PNET_DECIMAL_FIELD_LOW (JITINT8 *) "low"

/* Names of the fields inside the MONO BCL */
#define MONO_DECIMAL_FIELD_FLAGS (JITINT8 *) "flags"
#define MONO_DECIMAL_FIELD_HIGH (JITINT8 *) "hi"
#define MONO_DECIMAL_FIELD_MIDDLE (JITINT8 *) "mid"
#define MONO_DECIMAL_FIELD_LOW (JITINT8 *) "lo"

extern CLIManager_t *cliManager;

//funzioni private
void init_decimalManager ();
static inline void internal_convertToDecimalObject (void *object, decimalType* d);
static inline void internal_copyDecimalObject (void *destination, void *source);
static inline void *internal_newDecimalObject (decimalType *d);
static inline void *internal_newDecimalZero ();
static inline void *internal_newDecimalOne ();
static inline void internal_libdecimal_check_metadata (void);
static inline void internal_setflags (void *object, JITUINT32 flags);
static inline void internal_setmiddle (void *object, JITUINT32 value);
static inline void internal_sethigh (void *object, JITUINT32 value);
static inline void internal_setlow (void *object, JITUINT32 value);
static inline void internal_lib_decimal_initialize (void);
static inline JITUINT32 internal_getflags (void *object);
static inline JITUINT32 internal_getlow (void *object);
static inline JITUINT32 internal_getmiddle (void *object);
static inline JITUINT32 internal_gethigh (void *object);
static inline JITINT32 internal_DivByTen (JITUINT32 *result, JITUINT32 *value, JITINT32 size);
static inline JITINT32 internal_Normalize (JITUINT32 *value, JITINT32 size, JITINT32 decpt, JITINT32 roundMode);
static inline JITUINT64 internal_FloatToUInt64 (JITNFLOAT value);
static inline JITINT32 internal_isFinite (JITNFLOAT value);
static inline JITINT32 internal_MulByPowOfTen (JITUINT32 *value, JITINT32 power, JITUINT32 digit);
static inline JITINT32 internal_CmpAbs (void *valuea, void *valueb, JITINT32 sign);
static inline JITINT32 internal_add (JITUINT32 *tempa, void *valuea, void *valueb);
static inline JITINT32 internal_sub (JITUINT32 *tempa, void *valuea, void *valueb);
static inline JITINT32 internal_div (decimalType *result, void *valuea, void *valueb, JITINT32 roundMode);
static inline JITINT32 internal_divide (JITUINT32 *quotient, void *valuea, void *valueb);
static inline JITINT32 internal_addback (JITUINT32 *valuea, JITINT32 sizea, JITUINT32 *valueb, JITINT32 sizeb);
static inline JITINT32 internal_mulandsub (JITUINT32 *valuea, JITINT32 sizea, JITUINT32 *valueb, JITINT32 sizeb, JITUINT32 valuec);
static inline void internal_shiftleft (JITUINT32 *value, JITINT32 size, JITINT32 shift);
static inline void internal_mulbyword (JITUINT32 *result, JITINT32 base, void *valuea, JITUINT32 valueb);
static inline JITINT8 internal_decimalFromFloat (void *_this, JITFLOAT32 value);
static inline JITINT8 internal_decimalFromDouble (void *_this, JITFLOAT64 value);
static inline JITFLOAT64 internal_decimalToDouble (void *value);
static inline void *internal_decimalAdd (void *x, void *y, JITINT32 roundMode);
static inline void internal_decimalAddWithoutObjectAllocation (void *valuea, void *valueb, JITINT32 roundMode, void *objResult);
static inline JITINT32 internal_decimalCmp (void *valuea, void *valueb);
static inline void internal_decimalDivide (void *x, void *y, void *objResult);
static inline void internal_decimalFloor (void *x, void *objResult);
static inline void internal_decimalRemainder (void* x, void* y, void *objResult);
static inline void internal_decimalMultiply (void* x, void* y, void *objResult);
static inline void internal_decimalNegate (void* x, void *objResult);
static inline void internal_decimalRound (void* x, JITINT32 decimals, void *objResult);
static inline void internal_decimalSubtract (void* valuea, void* valueb, JITINT32 roundMode, void *objResult);
static inline void internal_decimalTruncate (void* x, void *objResult);

pthread_once_t decimalMetadataLock = PTHREAD_ONCE_INIT;

void init_decimalManager (t_decimalManager *decimalManager) {

    /* Initialize the functions			*/
    decimalManager->decimalNew = internal_newDecimalZero;
    decimalManager->decimalFromFloat = internal_decimalFromFloat;
    decimalManager->decimalFromDouble = internal_decimalFromDouble;
    decimalManager->decimalToDouble = internal_decimalToDouble;
    decimalManager->decimalAdd = internal_decimalAdd;
    decimalManager->decimalAddWithoutObjectAllocation = internal_decimalAddWithoutObjectAllocation;
    decimalManager->decimalCompare = internal_decimalCmp;
    decimalManager->decimalDivide = internal_decimalDivide;
    decimalManager->decimalFloor = internal_decimalFloor;
    decimalManager->decimalRemainder = internal_decimalRemainder;
    decimalManager->decimalMultiply = internal_decimalMultiply;
    decimalManager->decimalNegate = internal_decimalNegate;
    decimalManager->decimalRound = internal_decimalRound;
    decimalManager->decimalSubtract = internal_decimalSubtract;
    decimalManager->decimalTruncate = internal_decimalTruncate;
    decimalManager->initialize = internal_lib_decimal_initialize;
}

static inline void internal_libdecimal_check_metadata (void) {

    /* Check if we have to load the metadata	*/
    if (((cliManager->CLR).decimalManager).DecimalType == NULL) {

        /* Fetch the System.Decimal type		*/
        ((cliManager->CLR).decimalManager).DecimalType = cliManager->metadataManager->getTypeDescriptorFromName(cliManager->metadataManager, (JITINT8 *) "System", (JITINT8 *) "Decimal");

        /* Fetch the flags field ID			*/
        ((cliManager->CLR).decimalManager).flagsField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType, (JITINT8 *) "flags");

        /* Fetch the offset				*/
        ((cliManager->CLR).decimalManager).flagsFieldOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).decimalManager).flagsField);

        /* Fetch the high field ID			*/
        ((cliManager->CLR).decimalManager).highField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType,  PNET_DECIMAL_FIELD_HIGH);

        /* Mono BCL loaded */
        if (((cliManager->CLR).decimalManager).highField == NULL) {
            (cliManager->CLR).decimalManager.highField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType, MONO_DECIMAL_FIELD_HIGH);
        }

        /* Fetch the offset				*/
        ((cliManager->CLR).decimalManager).highFieldOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).decimalManager).highField);

        /* Fetch the middle field ID			*/
        ((cliManager->CLR).decimalManager).middleField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType, PNET_DECIMAL_FIELD_MIDDLE);

        /* Mono BCL loaded */
        if (((cliManager->CLR).decimalManager).middleField == NULL) {
            ((cliManager->CLR).decimalManager).middleField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType, MONO_DECIMAL_FIELD_MIDDLE);
        }

        /* Fetch the offset				*/
        ((cliManager->CLR).decimalManager).middleFieldOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).decimalManager).middleField);

        /* Fetch the low field ID		*/
        ((cliManager->CLR).decimalManager).lowField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType, PNET_DECIMAL_FIELD_LOW);

        /* Mono BCL loaded */
        if ((cliManager->CLR).decimalManager.lowField == NULL) {
            (cliManager->CLR).decimalManager.lowField = ((cliManager->CLR).decimalManager).DecimalType->getFieldFromName(((cliManager->CLR).decimalManager).DecimalType,  MONO_DECIMAL_FIELD_LOW);
        }


        /* Fetch the offset				*/
        ((cliManager->CLR).decimalManager).lowFieldOffset = cliManager->gc->fetchFieldOffset(((cliManager->CLR).decimalManager).lowField);
    }

    /* Final assertions				*/
    assert(((cliManager->CLR).decimalManager).flagsField != NULL);
    assert(((cliManager->CLR).decimalManager).highField != NULL);
    assert(((cliManager->CLR).decimalManager).middleField != NULL);
    assert(((cliManager->CLR).decimalManager).lowField != NULL);
    assert(((cliManager->CLR).decimalManager).DecimalType != NULL);

    /* Return					*/
    return;
}

static inline void internal_setflags (void *object, JITUINT32 flags) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).flagsFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &flags, sizeof(JITINT32));

    /* Return							*/
    return;
}

static inline void internal_setlow (void *object, JITUINT32 value) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).lowFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &value, sizeof(JITINT32));

    /* Return							*/
    return;
}

static inline void internal_setmiddle (void *object, JITUINT32 value) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).middleFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &value, sizeof(JITINT32));

    /* Return							*/
    return;
}

static inline void internal_sethigh (void *object, JITUINT32 value) {
    JITINT32 fieldOffset;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).highFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(object + fieldOffset, &value, sizeof(JITINT32));

    /* Return							*/
    return;
}

static inline JITUINT32 internal_getflags (void *object) {
    JITINT32 fieldOffset;
    JITINT32 value;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).flagsFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(&value, object + fieldOffset, sizeof(JITINT32));

    /* Return							*/
    return value;
}

static inline JITUINT32 internal_getlow (void *object) {
    JITINT32 fieldOffset;
    JITINT32 value;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).lowFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(&value, object + fieldOffset, sizeof(JITINT32));

    /* Return							*/
    return value;
}

static inline JITUINT32 internal_getmiddle (void *object) {
    JITINT32 fieldOffset;
    JITINT32 value;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).middleFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(&value, object + fieldOffset, sizeof(JITINT32));

    /* Return							*/
    return value;
}

static inline JITUINT32 internal_gethigh (void *object) {
    JITINT32 fieldOffset;
    JITINT32 value;

    /* Assertions							*/
    assert(object != NULL);

    assert(cliManager->entryPoint.binary != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Fetch the length offset					*/
    fieldOffset = ((cliManager->CLR).decimalManager).highFieldOffset;

    /* Store the length inside the field of the object		*/
    memcpy(&value, object + fieldOffset, sizeof(JITINT32));

    /* Return							*/
    return value;
}

static inline void internal_convertToDecimalObject (void *object, decimalType* d) {

    /* Assertions			*/
    assert(object != NULL);

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    internal_setflags(object, d->flags);
    internal_setlow(object, d->low);
    internal_setmiddle(object, d->middle);
    internal_sethigh(object, d->high);

    /* Return				*/
    return;
}

static inline void *internal_newDecimalObject (decimalType* d) {
    void            *object;
    TypeDescriptor *type;

    /* Check if we have to load the String	*
     * type information			*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Make the System.Decimal		*/
    type = ((cliManager->CLR).decimalManager).DecimalType;
    object = cliManager->gc->allocObject(type, 0);
    if (object == NULL) {
        cliManager->throwExceptionByName((JITINT8 *) "System", (JITINT8 *) "OutOfMemoryException");
    }
    assert(object != NULL);

    /* Fill the object			*/
    internal_convertToDecimalObject(object, d);

    /* Return				*/
    return object;
}

static inline void *internal_newDecimalZero () {
    decimalType d;

    d.high = 0;
    d.middle = 0;
    d.low = 0;
    d.flags = 0;

    return internal_newDecimalObject(&d);
}

static inline void *internal_newDecimalOne () {
    decimalType d;

    d.high = 0;
    d.middle = 0;
    d.low = 1;
    d.flags = 0;

    return internal_newDecimalObject(&d);
}


/*
 * Normalize a decimal value and return the new position
 * of the decimal point within the representation.  Returns
 * -1 if an overflow has occurred.
 */
static inline JITINT32 internal_Normalize (JITUINT32 *value, JITINT32 size, JITINT32 decpt, JITINT32 roundMode) {
    JITUINT32 intermediate[6];
    JITINT32 remainder;
    JITINT32 temp;
    JITINT32 carry;

    /* Strip leading zeros */
    while (size > 0 && *value == 0) {
        ++value;
        --size;
    }

    /* If the result is zero, then bail out now */
    if (!size) {
        return 0;
    }

    /* Keep dividing by 10 until the size is 3 or less, or
       until the decimal point returns to a useful position */
    remainder = 0;
    while ((size > 3 && decpt > 0) || decpt > 28) {
        remainder = internal_DivByTen(intermediate, value, size);
        --decpt;
        for (temp = 0; temp < size; ++temp) {
            value[temp] = intermediate[temp];
        }
        while (size > 0 && *value == 0) {
            ++value;
            --size;
        }
    }

    /* We need at least 3 words when rounding */
    if (size < 3) {
        value -= (3 - size);
        size = 3;
    }

    /* Round the value according to the rounding mode */
    if ((roundMode == IL_DECIMAL_ROUND_HALF_UP && remainder >= 5) ||
            (roundMode == IL_DECIMAL_ROUND_HALF_EVEN && remainder > 5) ||
            (roundMode == IL_DECIMAL_ROUND_HALF_EVEN && remainder == 5 &&
             (value[size - 1] & ((JITUINT32) 1)) != 0)) {
        /* Perform the rounding operation */
        carry = 1;
        for (temp = size - 1; temp >= 0; --temp) {
            if ((value[temp] += 1) != (JITUINT32) 0) {
                carry = 0;
                break;
            }
        }

        /* If we have a carry out, then we must divide by 10 again.
           If the decimal point is already at the right-most
           position, then the mantissa has overflowed */
        if (carry) {
            if (decpt <= 0) {
                return -1;
            }
            remainder = internal_DivByTen(intermediate, value, size);
            --decpt;
            for (temp = 0; temp < size; ++temp) {
                value[temp] = intermediate[temp];
            }
            while (size > 0 && *value == 0) {
                ++value;
                --size;
            }
        }
    }

    /* Remove trailing zeroes from the fractional part */
    while (decpt > 0 && size > 0) {
        remainder = internal_DivByTen(intermediate, value, size);
        if (remainder != 0) {
            break;
        }
        --decpt;
        for (temp = 0; temp < size; ++temp) {
            value[temp] = intermediate[temp];
        }
        while (size > 0 && *value == 0) {
            ++value;
            --size;
        }
    }

    /* If we have more than 3 words, then the result is too big */
    if (size > 3) {
        return -1;
    }

    /* If the size is zero, then the answer was rounded down to zero */
    if (!size) {
        return 0;
    }

    /* Done */
    return decpt;
}

/*
 * Divide a value by ten, returning the result and a remainder.
 */
static inline JITINT32 internal_DivByTen (JITUINT32 *result, JITUINT32 *value, JITINT32 size) {
    JITUINT32 remainder;
    JITUINT64 product;
    JITINT32 posn;

    remainder = 0;
    for (posn = 0; posn < size; ++posn) {
        product = (((JITUINT64) remainder) << 32) + ((JITUINT64) (value[posn]));
        result[posn] = (JITUINT32) (product / (JITUINT64) 10);
        remainder = (JITUINT32) (product % (JITUINT64) 10);
    }
    return (JITINT32) remainder;
}

static inline JITINT32 internal_isFinite (JITNFLOAT value) {
    return !isnan(value) && isinf(value) == 0;
}

/*
 * Convert "native float" into "ulong".
 *
 * Some platforms cannot perform the conversion directly,
 * so we need to do it in stages.
 */
static inline JITUINT64 internal_FloatToUInt64 (JITNFLOAT value) {

    if (internal_isFinite(value)) {
        if (value >= (JITNFLOAT) 0.0) {
            if (value < (JITNFLOAT) 9223372036854775808.0) {
                return (JITUINT64) (JITINT64) value;
            } else if (value < (JITNFLOAT) 18446744073709551616.0) {
                JITINT64 temp = (JITINT64) (value - 9223372036854775808.0);
                return (JITUINT64) (temp - IL_MIN_INT64);
            } else {
                return IL_MAX_UINT64;
            }
        } else {
            return 0;
        }
    } else if (isnan(value)) {
        return 0;
    } else if (value < (JITNFLOAT) 0.0) {
        return 0;
    }

    return IL_MAX_UINT64;
}



/*
 * Multiply a 192-bit value by power of ten.  Returns non-zero on carry out.
 */
static inline JITINT32 internal_MulByPowOfTen (JITUINT32 *value, JITINT32 power, JITUINT32 digit) {
    JITUINT32 powersOf10[9] = {
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000
    };
    JITUINT32 temp[6];
    JITUINT64 multiplier;
    JITUINT64 product;
    JITUINT32 prev;
    JITINT32 posn;

    while (power > 0) {
        /* Get the multiplier to use on this iteration */
        if (power > 9) {
            multiplier = (JITUINT64) (powersOf10[8]);
            power -= 9;
        } else {
            multiplier = (JITUINT64) (powersOf10[power - 1]);
            power = 0;
        }

        /* Compute "value * multiplier" and put it into "temp" */
        product = (JITUINT64) digit;
        temp[0] = temp[1] = temp[2] = temp[3] = temp[4] = temp[5] = 0;
        for (posn = 5; posn >= 0; --posn) {
            product += ((JITUINT64) (value[posn])) * multiplier;
            prev = temp[posn];
            if ((temp[posn] += (JITUINT32) product) < prev) {
                product = ((product >> 32) + 1);
            } else {
                product >>= 32;
            }
        }
        if (product != 0) {
            return 1;
        }

        /* Copy "temp" into "value" */
        value[0] = temp[0];
        value[1] = temp[1];
        value[2] = temp[2];
        value[3] = temp[3];
        value[4] = temp[4];
        value[5] = temp[5];
    }
    return 0;
}

/*
 * Compare the absolute magnitude of two decimal values.
 */
static inline JITINT32 internal_CmpAbs (void *valuea, void *valueb, JITINT32 sign) {
    JITUINT32 tempa[6];
    JITUINT32 tempb[6];
    JITINT32 decpta, decptb;
    JITINT32 posn;

    /* Load "valuea" and "valueb" into 192-bit temporary registers */
    tempa[0] = tempa[1] = tempa[2] = 0;
    tempa[3] = internal_gethigh(valuea);
    tempa[4] = internal_getmiddle(valuea);
    tempa[5] = internal_getlow(valuea);
    decpta = DECIMAL_GETPT(valuea);
    tempb[0] = tempb[1] = tempb[2] = 0;
    tempb[3] = internal_gethigh(valueb);
    tempb[4] = internal_getmiddle(valueb);
    tempb[5] = internal_getlow(valueb);
    decptb = DECIMAL_GETPT(valueb);

    /* Adjust for the decimal point positions */
    if (decpta < decptb) {
        /* Shift "valuea" up until the decimal points align */
        internal_MulByPowOfTen(tempa, (decptb - decpta), 0);
    } else if (decpta > decptb) {
        /* Shift "valueb" up until the decimal points align */
        internal_MulByPowOfTen(tempb, (decpta - decptb), 0);
    }

    /* Compare the 192-bit adjusted values */
    for (posn = 0; posn < 6; ++posn) {
        if (tempa[posn] > tempb[posn]) {
            return sign ? -1 : 1;
        } else if (tempa[posn] < tempb[posn]) {
            return sign ? 1 : -1;
        }
    }
    return 0;
}

/*
 * Add two 96-bit decimal values to get a 192-bit result.
 * Return the new position of the decimal point.
 */
static inline JITINT32 internal_add (JITUINT32 *tempa, void *valuea, void *valueb) {
    JITUINT32 tempb[6];
    JITUINT32 prev;
    JITINT32 posn, carry;
    JITINT32 decpta, decptb;

    /* Load "valuea" and "valueb" into 192-bit temporary registers */
    tempa[0] = tempa[1] = tempa[2] = 0;
    tempa[3] = internal_gethigh(valuea);
    tempa[4] = internal_getmiddle(valuea);
    tempa[5] = internal_getlow(valuea);
    decpta = DECIMAL_GETPT(valuea);
    tempb[0] = tempb[1] = tempb[2] = 0;
    tempb[3] = internal_gethigh(valueb);
    tempb[4] = internal_getmiddle(valueb);
    tempb[5] = internal_getlow(valueb);
    decptb = DECIMAL_GETPT(valueb);

    /* Adjust for the decimal point positions */
    if (decpta < decptb) {
        /* Shift "valuea" up until the decimal points align */
        internal_MulByPowOfTen(tempa, (decptb - decpta), 0);
        decpta = decptb;
    } else if (decpta > decptb) {
        /* Shift "valueb" up until the decimal points align */
        internal_MulByPowOfTen(tempb, (decpta - decptb), 0);
        decptb = decpta;
    }

    /* Add the two values */
    carry = 0;
    for (posn = 5; posn >= 0; --posn) {
        prev = tempa[posn];
        if (carry) {
            if ((tempa[posn] += tempb[posn] + 1) <= prev) {
                carry = 1;
            } else {
                carry = 0;
            }
        } else {
            if ((tempa[posn] += tempb[posn]) < prev) {
                carry = 1;
            } else {
                carry = 0;
            }
        }
    }
    return decpta;
}

/*
 * Subtract two 96-bit decimal values to get a 192-bit result.
 * Return the new position of the decimal point.  "valuea" will
 * always be greater than "valueb" on entry.
 */
static inline JITINT32 internal_sub (JITUINT32 *tempa, void *valuea, void *valueb) {
    JITUINT32 tempb[6];
    JITUINT32 prev;
    JITINT32 posn, carry;
    JITINT32 decpta, decptb;

    /* Load "valuea" and "valueb" into 192-bit temporary registers */
    tempa[0] = tempa[1] = tempa[2] = 0;
    tempa[3] = internal_gethigh(valuea);
    tempa[4] = internal_getmiddle(valuea);
    tempa[5] = internal_getlow(valuea);
    decpta = DECIMAL_GETPT(valuea);
    tempb[0] = tempb[1] = tempb[2] = 0;
    tempb[3] = internal_gethigh(valueb);
    tempb[4] = internal_getmiddle(valueb);
    tempb[5] = internal_getlow(valueb);
    decptb = DECIMAL_GETPT(valueb);

    /* Adjust for the decimal point positions */
    if (decpta < decptb) {
        /* Shift "valuea" up until the decimal points align */
        internal_MulByPowOfTen(tempa, (decptb - decpta), 0);
        decpta = decptb;
    } else if (decpta > decptb) {
        /* Shift "valueb" up until the decimal points align */
        internal_MulByPowOfTen(tempb, (decpta - decptb), 0);
        decptb = decpta;
    }

    /* Subtract the two values */
    carry = 0;
    for (posn = 5; posn >= 0; --posn) {
        prev = tempa[posn];
        if (carry) {
            if ((tempa[posn] -= tempb[posn] + 1) >= prev) {
                carry = 1;
            } else {
                carry = 0;
            }
        } else {
            if ((tempa[posn] -= tempb[posn]) > prev) {
                carry = 1;
            } else {
                carry = 0;
            }
        }
    }
    return decpta;
}

/*
 * Shift a value left by a number of bits.
 */
static inline void internal_shiftleft (JITUINT32 *value, JITINT32 size, JITINT32 shift) {
    JITUINT64 carry = 0;

    while (size > 0) {
        --size;
        carry |= (((JITUINT64) (value[size])) << shift);
        value[size] = (JITUINT32) carry;
        carry >>= 32;
    }
}

/*
 * Compute "valuea -= valueb * valuec".  Returns non-zero
 * if the result went negative.  We assume that sizeb is
 * greater than 0 and less than sizea.
 */
static inline JITINT32 internal_mulandsub (JITUINT32 *valuea, JITINT32 sizea, JITUINT32 *valueb, JITINT32 sizeb, JITUINT32 valuec) {
    JITINT32 posn;
    JITUINT64 carry = 0;
    JITUINT64 valuec64 = (JITUINT64) valuec;
    JITUINT32 temp;

    for (posn = sizeb; posn > 0; --posn) {
        carry += ((JITUINT64) (valueb[posn - 1])) * valuec64;
        temp = valuea[posn] - (JITUINT32) carry;
        if (temp > valuea[posn]) {
            carry = ((carry >> 32) + 1);
        } else {
            carry >>= 32;
        }
        valuea[posn] = temp;
    }
    /* process word 0 */
    temp = valuea[0] - (JITUINT32) carry;
    if (temp > valuea[0]) {
        carry = ((carry >> 32) + 1);
    } else {
        carry >>= 32;
    }
    valuea[0] = temp;
    return carry != 0;
}

/*
 * Compute "valuea += valueb".  Returns non-zero if
 * the result is still negative.  We assume that sizeb
 * is greater than 0 and less than sizea.
 */
static inline JITINT32 internal_addback (JITUINT32 *valuea, JITINT32 sizea, JITUINT32 *valueb, JITINT32 sizeb) {
    JITINT32 posn;
    JITUINT64 carry = 0;

    for (posn = sizeb; posn > 0; --posn) {
        carry += (((JITUINT64) (valuea[posn])) +
                  ((JITUINT64) (valueb[posn - 1])));
        valuea[posn] = (JITUINT32) carry;
        carry >>= 32;
    }
    /* process word 0 */
    carry += (JITUINT64) valuea[0];
    valuea[0] = (JITUINT32) carry;
    carry >>= 32;
    return carry != 0;
}



/*
 * Divide valuea by valueb giving a 228-bit quotient.
 * Returns zero if attempting to divide by zero.
 */
static inline JITINT32 internal_divide (JITUINT32 *quotient, void *valuea, void *valueb) {
    JITUINT32 tempa[7];
    JITUINT32 tempb[3];
    JITINT32 bsize, shift;
    JITINT32 posn, isneg, limit;
    JITUINT32 bittest;
    JITUINT32 testquot;
    JITUINT32 b_high, b_middle, b_low;


    /* Expand valuea to a 228-bit value, shifted up by 29 places */
    tempa[0] = tempa[1] = tempa[2] = tempa[3] = 0;
    tempa[4] = internal_gethigh(valuea);
    tempa[5] = internal_getmiddle(valuea);
    tempa[6] = internal_getlow(valuea);
    internal_MulByPowOfTen(tempa + 1, 29, 0);

    /* Convert valueb into its intermediate form */
    b_high = internal_gethigh(valueb);
    b_middle = internal_getmiddle(valueb);
    b_low = internal_getlow(valueb);
    if (b_high != 0) {
        tempb[0] = b_high;
        tempb[1] = b_middle;
        tempb[2] = b_low;
        bsize = 3;
    } else if (b_middle != 0) {
        tempb[0] = b_middle;
        tempb[1] = b_low;
        bsize = 2;
    } else if (b_low != 0) {
        tempb[0] = b_low;
        bsize = 1;
    } else {
        return 0;
    }

    /* Shift tempa and tempb so that tempb[0] >= 0x80000000 */
    shift = 0;
    bittest = tempb[0];
    while ((bittest & (JITUINT32) 0x80000000) == 0) {
        ++shift;
        bittest <<= 1;
    }
    if (shift != 0) {
        internal_shiftleft(tempa, 7, shift);
        internal_shiftleft(tempb, bsize, shift);
    }

    /* Perform the division */
    limit = 7 - bsize;
    for (posn = 0; posn < bsize; ++posn) {
        quotient[posn] = 0;
    }
    for (posn = 0; posn < limit; ++posn) {
        /* Get the test quotient for the current word */
        if (tempa[posn] >= tempb[0]) {
            testquot = (JITUINT32) 0xFFFFFFFF;
        } else if (posn < 6) {
            testquot = (JITUINT32)
                       (((((JITUINT64) (tempa[posn])) << 32) |
                         ((JITUINT64) (tempa[posn + 1]))) / (JITUINT64) (tempb[0]));
        } else {
            testquot = (JITUINT32)
                       ((((JITUINT64) (tempa[posn])) << 32) / (JITUINT64) (tempb[0]));
        }

        /* Multiply tempb by testquot and subtract from tempa */
        isneg = internal_mulandsub(tempa + posn, 7 - posn, tempb, bsize, testquot);

        /* Add back if the result went negative.  This loop will
           iterate for no more than 2 iterations */
        while (isneg) {
            --testquot;
            isneg = internal_addback(tempa + posn, 7 - posn, tempb, bsize);
        }

        /* Set the current quotient word to the test quotient */
        quotient[posn + bsize] = testquot;
    }

    /* Done */
    return 1;
}


static inline JITINT32 internal_div (decimalType *result, void *valuea, void *valueb, JITINT32 roundMode) {
    JITUINT32 quotient[7];
    JITINT32 decpt, sign;

    /* Compute the 228-bit quotient of the fractional parts */
    if (!internal_divide(quotient, valuea, valueb)) {
        /* Division by zero error */
        return 0;
    }

    /* Normalize and return the result */
    sign = (DECIMAL_IS_NEG(valuea) ^ DECIMAL_IS_NEG(valueb));
    decpt = internal_Normalize(quotient, 7, DECIMAL_GETPT(valuea) - DECIMAL_GETPT(valueb) + 29,  roundMode);
    if (decpt < 0) {
        return 0;
    }
    result->high = quotient[4];
    result->middle = quotient[5];
    result->low = quotient[6];
    if (result->high == 0 && result->middle == 0 && result->low == 0) {
        /* The sign of zero must always be positive */
        sign = 0;
    }
    result->flags = DECIMAL_MKFLAGS(sign, decpt);
    return 1;
}

/*
 * Multiply a value by a single word and add it to an accumulated result.
 */
static inline void internal_mulbyword (JITUINT32 *result, JITINT32 base, void *valuea, JITUINT32 valueb) {
    JITUINT64 product;

    /* valuea->low * valueb */
    product = ((JITUINT64) (internal_getlow(valuea))) * ((JITUINT64) valueb);
    product += (JITUINT64) (result[base]);
    result[base] = (JITUINT32) product;
    product >>= 32;
    --base;

    /* valuea->middle * valueb */
    product += ((JITUINT64) (internal_getmiddle(valuea))) * ((JITUINT64) valueb);
    product += (JITUINT64) (result[base]);
    result[base] = (JITUINT32) product;
    product >>= 32;
    --base;

    /* valuea->high * valueb */
    product += ((JITUINT64) (internal_gethigh(valuea))) * ((JITUINT64) valueb);
    product += (JITUINT64) (result[base]);
    result[base] = (JITUINT32) product;
    product >>= 32;
    --base;

    /* Propagate the carry */
    while (base >= 0 && product != 0) {
        product += (JITUINT64) (result[base]);
        result[base] = (JITUINT32) product;
        product >>= 32;
        --base;
    }
}

static inline JITINT32 internal_mul (decimalType *result, void *valuea, void *valueb, JITINT32 roundMode) {
    JITUINT32 temp[6];
    JITINT32 decpt;
    JITINT32 sign;

    /* Calculate the intermediate result */
    temp[0] = temp[1] = temp[2] = temp[3] = temp[4] = temp[5] = 0;
    internal_mulbyword(temp, 5, valuea, internal_getlow(valueb));
    internal_mulbyword(temp, 4, valuea, internal_getmiddle(valueb));
    internal_mulbyword(temp, 3, valuea, internal_gethigh(valueb));

    /* Build the result value */
    sign = (DECIMAL_IS_NEG(valuea) ^ DECIMAL_IS_NEG(valueb));
    decpt = internal_Normalize(temp, 6, DECIMAL_GETPT(valuea) + DECIMAL_GETPT(valueb),  roundMode);
    if (decpt < 0) {
        return 0;
    }
    result->high = temp[3];
    result->middle = temp[4];
    result->low = temp[5];
    if (result->high == 0 && result->middle == 0 && result->low == 0) {
        /* The sign of zero must always be positive */
        sign = 0;
    }
    result->flags = DECIMAL_MKFLAGS(sign, decpt);
    return 1;
}


static inline JITINT32 internal_round (decimalType *result, void *value, JITINT32 places, JITINT32 roundMode) {
    JITUINT32 tempa[3];
    JITUINT32 tempb[3];
    JITINT32 remainder, posn, carry;
    JITINT32 decpt = DECIMAL_GETPT(value);

    /* Bail out early if we already have fewer than the requested places */
    if (places < 0 || decpt <= places) {
        result->high = internal_gethigh(value);
        result->middle = internal_getmiddle(value);
        result->low = internal_getlow(value);
        result->flags = internal_getflags(value);
        return 1;
    }

    /* Keep dividing by 10 until we have the requested number of places */
    tempa[0] = internal_gethigh(value);
    tempa[1] = internal_getmiddle(value);
    tempa[2] = internal_getlow(value);
    remainder = 0;
    while (decpt > places) {
        remainder = internal_DivByTen(tempb, tempa, 3);
        tempa[0] = tempb[0];
        tempa[1] = tempb[1];
        tempa[2] = tempb[2];
        --decpt;
    }

    /* Round according to the rounding mode */
    if ((roundMode == IL_DECIMAL_ROUND_HALF_UP && remainder >= 5) ||
            (roundMode == IL_DECIMAL_ROUND_HALF_EVEN && remainder > 5) ||
            (roundMode == IL_DECIMAL_ROUND_HALF_EVEN && remainder == 5 &&
             (tempa[2] & ((JITUINT32) 1)) != 0)) {
        /* Perform the rounding operation */
        carry = 1;
        for (posn = 2; posn >= 0; --posn) {
            if ((tempa[posn] += 1) != (JITUINT32) 0) {
                carry = 0;
                break;
            }
        }

        /* If we have a carry out, then we must divide by 10 again.
           If the decimal point is already at the right-most
           position, then the mantissa has overflowed */
        if (carry) {
            if (decpt <= 0) {
                return 0;
            }
            remainder = internal_DivByTen(tempb, tempa, 3);
            tempa[0] = tempb[0];
            tempa[1] = tempb[1];
            tempa[2] = tempb[2];
            --decpt;
        }
    }

    /* Remove trailing zeros after the decimal point */
    while (decpt > 0) {
        remainder = internal_DivByTen(tempb, tempa, 3);
        if (remainder != 0) {
            break;
        }
        tempa[0] = tempb[0];
        tempa[1] = tempb[1];
        tempa[2] = tempb[2];
        --decpt;
    }

    /* Build and return the result to the caller */
    result->high = tempa[0];
    result->middle = tempa[1];
    result->low = tempa[2];
    if (result->high == 0 && result->middle == 0 && result->low == 0) {
        /* The sign of zero must always be positive */
        result->flags = 0;
    } else {
        result->flags = DECIMAL_MKFLAGS(DECIMAL_IS_NEG(value), decpt);
    }
    return 1;
}

static inline JITINT8 internal_decimalFromFloat (void *_this, JITFLOAT32 value) {

    /* Assertions		*/
    assert(_this != NULL);

    return internal_decimalFromDouble(_this, (JITFLOAT64) value);
}

static inline JITINT8 internal_decimalFromDouble (void *_this, JITFLOAT64 value) {
    JITINT32 isNeg;
    JITINT32 scale;
    JITUINT64 temp;
    JITUINT32 values[3];

    /* Assertions		*/
    assert(_this != NULL);

    /* Bail out if the number is out of range */
    if (     isnan((JITNFLOAT) value)                 ||
             value >= MAX_DECIMAL_AS_DOUBLE          ||
             value <= -MAX_DECIMAL_AS_DOUBLE) {
        return 1;
    }

    /* Extract the sign */
    isNeg = (value < (JITFLOAT64) 0.0);
    if (isNeg) {
        value = -value;
    }

    /* Determine the scale factor to use with the number */
    for (scale = 0; scale < 28; ++scale) {
        if (value < powOfTen[scale]) {
            break;
        }
    }
    scale = 28 - scale;

    /* Re-scale the value to convert it into an integer */
    value *= powOfTen[scale];

    /* Extract the 96-bit integer component */
    values[0] = (JITUINT32) internal_FloatToUInt64(value / TWO_TO_64);
    value = fmod(value, TWO_TO_64);
    temp = internal_FloatToUInt64(value);
    values[1] = (JITUINT32) (temp >> 32);
    values[2] = (JITUINT32) temp;

    /* Normalize the result and return it */
    scale = internal_Normalize(values, 3, scale, IL_DECIMAL_ROUND_HALF_UP);
    if (scale < 0) {
        return 1;
    }
    internal_setflags(_this, DECIMAL_MKFLAGS(isNeg, scale));
    internal_sethigh(_this, values[0]);
    internal_setmiddle(_this, values[1]);
    internal_setlow(_this, values[2]);

    /* Return				*/
    return 0;
}

static inline JITFLOAT64 internal_decimalToDouble (void *value) {
    JITFLOAT64 temp;
    JITINT32 decpt;

    /* Convert the fractional part of the value */
    temp = (JITFLOAT64) (internal_getlow(value));
    temp += ((JITFLOAT64) (internal_getmiddle(value))) * 4294967296.0;                              /* 2^32 */
    temp += ((JITFLOAT64) (internal_gethigh(value))) * 18446744073709551616.0;                      /* 2^64 */

    /* Adjust for the position of the decimal point */
    decpt = DECIMAL_GETPT(value);
    temp /= powOfTen[decpt];

    /* Apply the sign and return */
    if (DECIMAL_IS_NEG(value)) {
        return -temp;
    } else {
        return temp;
    }
}

static inline void internal_decimalAddWithoutObjectAllocation (void *valuea, void *valueb, JITINT32 roundMode, void *objResult) {
    JITUINT32 temp[6];
    JITINT32 decpt;
    JITINT32 sign;
    decimalType result;

    /* Assertions                           */
    assert(valuea != NULL);
    assert(valueb != NULL);
    assert(objResult != NULL);

    /* Determine how to perform the addition */
    if (!DECIMAL_IS_NEG(valuea) && !DECIMAL_IS_NEG(valueb)) {

        /* Both values are positive */
        decpt = internal_add(temp, valuea, valueb);
        sign = 0;
    } else if (DECIMAL_IS_NEG(valuea) && DECIMAL_IS_NEG(valueb)) {

        /* Both values are negative */
        decpt = internal_add(temp, valuea, valueb);
        sign = 1;
    } else if (DECIMAL_IS_NEG(valuea)) {

        /* The first value is negative, and the second is positive */
        if (internal_CmpAbs(valuea, valueb, 0) >= 0) {

            /* Negative result */
            decpt = internal_sub(temp, valuea, valueb);
            sign = 1;
        } else {

            /* Positive result */
            decpt = internal_sub(temp, valueb, valuea);
            sign = 0;
        }
    } else {

        /* The first value is positive, and the second is negative */
        if (internal_CmpAbs(valuea, valueb, 0) <= 0) {

            /* Negative result */
            decpt = internal_sub(temp, valueb, valuea);
            sign = 1;
        } else {

            /* Positive result */
            decpt = internal_sub(temp, valuea, valueb);
            sign = 0;
        }
    }

    /* Normalize and return the result to the caller */
    decpt = internal_Normalize(temp, 6, decpt, roundMode);
    if (decpt < 0) {
        return;
    }

    result.high = temp[3];
    result.middle = temp[4];
    result.low = temp[5];
    if (result.high == 0 && result.middle == 0 && result.low == 0) {

        /* The sign of zero must always be positive */
        sign = 0;
    }
    result.flags = DECIMAL_MKFLAGS(sign, decpt);

    internal_convertToDecimalObject(objResult, &result);
    return;
}

static inline void * internal_decimalAdd (void *valuea, void *valueb, JITINT32 roundMode) {
    JITUINT32 temp[6];
    JITINT32 decpt;
    JITINT32 sign;
    decimalType result;
    void            *objResult;

    /* Assertions                           */
    assert(valuea != NULL);
    assert(valueb != NULL);

    /* Determine how to perform the addition */
    if (!DECIMAL_IS_NEG(valuea) && !DECIMAL_IS_NEG(valueb)) {

        /* Both values are positive */
        decpt = internal_add(temp, valuea, valueb);
        sign = 0;
    } else if (DECIMAL_IS_NEG(valuea) && DECIMAL_IS_NEG(valueb)) {

        /* Both values are negative */
        decpt = internal_add(temp, valuea, valueb);
        sign = 1;
    } else if (DECIMAL_IS_NEG(valuea)) {

        /* The first value is negative, and the second is positive */
        if (internal_CmpAbs(valuea, valueb, 0) >= 0) {

            /* Negative result */
            decpt = internal_sub(temp, valuea, valueb);
            sign = 1;
        } else {

            /* Positive result */
            decpt = internal_sub(temp, valueb, valuea);
            sign = 0;
        }
    } else {

        /* The first value is positive, and the second is negative */
        if (internal_CmpAbs(valuea, valueb, 0) <= 0) {

            /* Negative result */
            decpt = internal_sub(temp, valueb, valuea);
            sign = 1;
        } else {

            /* Positive result */
            decpt = internal_sub(temp, valuea, valueb);
            sign = 0;
        }
    }

    /* Normalize and return the result to the caller */
    decpt = internal_Normalize(temp, 6, decpt, roundMode);
    if (decpt < 0) {
        return NULL;
    }

    result.high = temp[3];
    result.middle = temp[4];
    result.low = temp[5];
    if (result.high == 0 && result.middle == 0 && result.low == 0) {

        /* The sign of zero must always be positive */
        sign = 0;
    }
    result.flags = DECIMAL_MKFLAGS(sign, decpt);

    objResult = internal_newDecimalObject(&result);
    assert(objResult != NULL);
    return objResult;
}

static inline JITINT32 internal_decimalCmp (void *valuea, void *valueb) {
    if (DECIMAL_IS_NEG(valuea) && !DECIMAL_IS_NEG(valueb)) {
        return -1;
    } else if (!DECIMAL_IS_NEG(valuea) && DECIMAL_IS_NEG(valueb)) {
        return 1;
    } else {
        return internal_CmpAbs(valuea, valueb, DECIMAL_IS_NEG(valuea));
    }
}

static inline void internal_decimalDivide (void *x, void *y, void *objResult) {
    JITINT32 divResult;
    decimalType result;


    memset(&result, 0, sizeof(decimalType));
    divResult = internal_div(&result, x, y, DECIMAL_ROUND_MODE);
    if (!divResult) {
        cliManager->throwExceptionByName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowDivbyZero");
    } else if (divResult < 0) {
        cliManager->throwExceptionByName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowOverflow");
    }

    internal_convertToDecimalObject(objResult, &result);
    return;
}

static inline void internal_decimalFloor (void *x, void *objResult) {
    void *one;

    if (DECIMAL_IS_NEG(x)) {

        /* The value is negative */
        if (DECIMAL_GETPT(x)) {

            /* We have decimals, so truncate and subtract 1 */
            one = internal_newDecimalOne();
            assert(one != NULL);

            internal_decimalTruncate(x, objResult);
            internal_decimalSubtract(objResult, one, IL_DECIMAL_ROUND_DOWN, objResult);
        } else {

            /* No decimals, so the result is the same as the value */
            internal_copyDecimalObject(objResult, x);
        }
    } else {

        /* The value is positive, so floor is the same as truncate */
        internal_decimalTruncate(x, objResult);
    }

    /* Return		*/
    return;
}

static inline void internal_decimalRemainder (void* x, void* y, void *objResult) {

    /* Divide x by y to get the quotient            */
    internal_decimalDivide(x, y, objResult);

    /* Truncate the quotient q to get its integer part */
    internal_decimalTruncate(objResult, objResult);

    /* Compute "x - y * q" to get the remainder     */
    internal_decimalMultiply(y, objResult, objResult);
    internal_decimalSubtract(x, objResult, DECIMAL_ROUND_MODE, objResult);

    /* Return					*/
    return;
}

static inline void internal_decimalMultiply (void* x, void* y, void *objResult) {
    decimalType result;

    memset(&result, 0, sizeof(decimalType));
    if (!internal_mul(&result, x, y, DECIMAL_ROUND_MODE)) {
        cliManager->throwExceptionByName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowOverflow");
    }

    internal_convertToDecimalObject(objResult, &result);
    return;
}

static inline void internal_decimalNegate (void* x, void *objResult) {
    decimalType result;

    JITUINT32 flags = internal_getflags(x);

    result.high = internal_gethigh(x);
    result.middle = internal_getmiddle(x);
    result.low = internal_getlow(x);

    /* Invert the sign if the value is not zero (zero is always positive) */
    if ( result.high != 0 || result.middle != 0 || result.low != 0) {
        result.flags = (flags ^ (JITUINT32) 0x80000000);
    } else {
        result.flags = flags;
    }

    internal_convertToDecimalObject(objResult, &result);
    return;
}

static inline void internal_decimalRound (void* x, JITINT32 decimals, void *objResult) {
    decimalType result;

    memset(&result, 0, sizeof(decimalType));
    if (decimals < 0 || decimals > 28) {
        cliManager->throwExceptionByName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowDecimals");
    } else if (!internal_round(&result, x, decimals, DECIMAL_ROUND_MODE)) {
        cliManager->throwExceptionByName((JITINT8 *) "System.Decimal", (JITINT8 *) "ThrowOverflow");
    }

    internal_convertToDecimalObject(objResult, &result);
    return;
}

static inline void internal_decimalSubtract (void* valuea, void* valueb, JITINT32 roundMode, void *objResult) {
    decimalType result;
    JITUINT32 temp[6];
    JITINT32 decpt;
    JITINT32 sign;

    /* Determine how to perform the subtraction */
    if (!DECIMAL_IS_NEG(valuea) && DECIMAL_IS_NEG(valueb)) {
        /* The first value is positive, and the second is negative */
        decpt = internal_add(temp, valuea, valueb);
        sign = 0;
    } else if (DECIMAL_IS_NEG(valuea) && !DECIMAL_IS_NEG(valueb)) {
        /* The first value is negative, and the second is positive */
        decpt = internal_add(temp, valuea, valueb);
        sign = 1;
    } else if (DECIMAL_IS_NEG(valuea)) {
        /* Both values are negative */
        if (internal_CmpAbs(valuea, valueb, 0) >= 0) {
            /* Negative result */
            decpt = internal_sub(temp, valuea, valueb);
            sign = 1;
        } else {
            /* Positive result */
            decpt = internal_sub(temp, valueb, valuea);
            sign = 0;
        }
    } else {
        /* Both values are positive */
        if (internal_CmpAbs(valuea, valueb, 0) <= 0) {
            /* Negative result */
            decpt = internal_sub(temp, valueb, valuea);
            sign = 1;
        } else {
            /* Positive result */
            decpt = internal_sub(temp, valuea, valueb);
            sign = 0;
        }
    }

    /* Normalize and return the result to the caller */
    decpt = internal_Normalize(temp, 6, decpt, roundMode);
    if (decpt < 0) {
        return;
    }
    result.high = temp[3];
    result.middle = temp[4];
    result.low = temp[5];
    if (result.high == 0 && result.middle == 0 && result.low == 0) {
        /* The sign of zero must always be positive */
        sign = 0;
    }
    result.flags = DECIMAL_MKFLAGS(sign, decpt);

    internal_convertToDecimalObject(objResult, &result);
    return;
}

static inline void internal_decimalTruncate (void* x, void *objResult) {
    JITUINT32 tempa[3];
    JITUINT32 tempb[3];
    JITINT32 decpt = DECIMAL_GETPT(x);
    decimalType result;

    if (decpt) {

        /* Expand the value to the intermediate form */
        tempa[0] = internal_gethigh(x);
        tempa[1] = internal_getmiddle(x);
        tempa[2] = internal_getlow(x);

        /* Keep dividing by 10 until the decimal point position is reached */
        while (decpt > 0) {
            internal_DivByTen(tempb, tempa, 3);
            tempa[0] = tempb[0];
            tempa[1] = tempb[1];
            tempa[2] = tempb[2];
            --decpt;
        }

        /* Build the result value */
        result.high = tempa[0];
        result.middle = tempa[1];
        result.low = tempa[2];
        result.flags = (internal_getflags(x) & (JITUINT32) 0x80000000);
        internal_convertToDecimalObject(objResult, &result);
    } else {

        /* No decimals, so the result is the same as the value */
        internal_copyDecimalObject(objResult, x);
    }

    /* Return			*/
    return;
}

static inline void internal_copyDecimalObject (void *destination, void *source) {

    /* Assertions			*/
    assert(destination != NULL);
    assert(source != NULL);

    /* Copy the destination		*/
    internal_setflags(destination, internal_getflags(source));
    internal_setlow(destination, internal_getlow(source));
    internal_setmiddle(destination, internal_getmiddle(source));
    internal_sethigh(destination, internal_gethigh(source));

    /* Return			*/
    return;
}

static inline void internal_lib_decimal_initialize (void) {

    /* Initialize the module	*/
    PLATFORM_pthread_once(&decimalMetadataLock, internal_libdecimal_check_metadata);

    /* Return			*/
    return;
}
