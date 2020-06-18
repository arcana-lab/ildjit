/*
 * Copyright (C) 2009  Simone Campanoni, Luca Rocchini
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

#include <compiler_memory_manager.h>
#include <generic_type_rules.h>
#include <iljitmm-system.h>
#include <stdlib.h>
#include <assert.h>

JITBOOLEAN is_IR_ReferenceType (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITBOOLEAN is_IR_ObjectType (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITBOOLEAN is_IR_ManagedPointer (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITBOOLEAN is_IR_UnmanagedPointer (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITBOOLEAN is_IR_TransientPointer (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITBOOLEAN is_IR_ValueType (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITBOOLEAN is_IR_ValidType (t_generic_type_checking_rules *rules, JITUINT32 typeID);
JITUINT32 get_return_IRType_For_Unary_Numeric_Operation (JITUINT32 param);
JITUINT32 get_return_IRType_For_Binary_Comparison (JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp, JITBOOLEAN *verifiable_code);
JITUINT32 get_return_IRType_For_Binary_Numeric_operation (JITUINT32 param1, JITUINT32 param2, JITUINT32 irOP, JITBOOLEAN *verified_code);
JITUINT32 get_return_IRType_For_Shift_operation (JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp);
JITUINT32 get_return_IRType_For_Overflow_Arythmetic_operation (JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp, JITBOOLEAN *verified_code);
JITUINT32 get_return_IRType_For_Integer_operation (JITUINT32 param1, JITUINT32 param2);
JITUINT32 get_conversion_infos_For_Conv_operation (JITUINT32 fromType, JITUINT32 toType, JITBOOLEAN *verified_code);
JITUINT32 coerce_CLI_Type (JITUINT32 type_to_coerce);
JITBOOLEAN is_CLI_Type (JITUINT32 type_to_coerce);
JITBOOLEAN is_unsigned_IRType (JITUINT32 type);
JITUINT32 to_unsigned_IRType (JITUINT32 type);


JITBOOLEAN is_IR_ReferenceType (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* unchecked preconditions : the value type is a valid type */

    /*assertions */
    assert(rules != NULL);


    /* verify if the typeID identifies a referenceType location according to the ECMA specification */
    return (typeID == IRMPOINTER)
           || (typeID == IRTPOINTER)
           || (typeID == IROBJECT);
}

JITBOOLEAN is_Reference_or_Object_IRType (JITUINT32 IRType) {
    return (IRType == IRMPOINTER)
           || (IRType == IRTPOINTER)
           || (IRType == IROBJECT);
}

JITBOOLEAN is_IR_ValueType (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* unchecked preconditions : the value type is a valid type */

    /* assertions */
    assert(rules != NULL);

    /* verify if the typeID identifies a valueType location according to the ECMA specification */
    return (typeID == IRINT8)
           || (typeID == IRINT16)
           || (typeID == IRINT32)
           || (typeID == IRINT64)
           || (typeID == IRUINT8)
           || (typeID == IRUINT16)
           || (typeID == IRUINT32)
           || (typeID == IRUINT64)
           || (typeID == IRNINT)
           || (typeID == IRNUINT)
           || (typeID == IRFLOAT32)
           || (typeID == IRFLOAT64)
           || (typeID == IRNFLOAT)
           || (typeID == IRVOID);
}

JITBOOLEAN is_IR_ObjectType (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* assertions */
    assert(rules != NULL);

    return typeID == IROBJECT;
}

JITBOOLEAN is_IR_ManagedPointer (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* assertions */
    assert(rules != NULL);

    return typeID == IRMPOINTER;
}

JITBOOLEAN is_IR_UnmanagedPointer (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* assertions */
    assert(rules != NULL);

    return typeID == IRNUINT;
}

JITBOOLEAN is_IR_TransientPointer (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* assertions */
    assert(rules != NULL);

    return typeID == IRTPOINTER;
}

JITBOOLEAN is_IR_ValidType (t_generic_type_checking_rules *rules, JITUINT32 typeID) {
    /* assertions */
    assert(rules != NULL);

    return rules->is_IR_ReferenceType(rules, typeID)
           || rules->is_IR_ValueType(rules, typeID);
}

t_generic_type_checking_rules *new_generic_type_checking_rules () {
    /* initialize the function pointers */
    t_generic_type_checking_rules *checker = sharedAllocFunction(sizeof(t_generic_type_checking_rules));

    checker->is_IR_ReferenceType = is_IR_ReferenceType;
    checker->is_IR_ObjectType = is_IR_ObjectType;
    checker->is_IR_ManagedPointer = is_IR_ManagedPointer;
    checker->is_IR_UnmanagedPointer = is_IR_UnmanagedPointer;
    checker->is_IR_TransientPointer = is_IR_TransientPointer;
    checker->is_IR_ValueType = is_IR_ValueType;
    checker->is_IR_ValidType = is_IR_ValidType;

    /* function pointers used in comparison between types */
    checker->get_return_IRType_For_Binary_Numeric_operation = get_return_IRType_For_Binary_Numeric_operation;
    checker->get_return_IRType_For_Unary_Numeric_Operation = get_return_IRType_For_Unary_Numeric_Operation;
    checker->get_return_IRType_For_Binary_Comparison = get_return_IRType_For_Binary_Comparison;
    checker->get_return_IRType_For_Overflow_Arythmetic_operation = get_return_IRType_For_Overflow_Arythmetic_operation;
    checker->get_return_IRType_For_Integer_operation = get_return_IRType_For_Integer_operation;
    checker->get_return_IRType_For_Shift_operation = get_return_IRType_For_Shift_operation;
    checker->get_conversion_infos_For_Conv_operation = get_conversion_infos_For_Conv_operation;
    checker->coerce_CLI_Type = coerce_CLI_Type;
    checker->is_CLI_Type = is_CLI_Type;
    checker->is_unsigned_IRType = is_unsigned_IRType;
    checker->to_unsigned_IRType = to_unsigned_IRType;

    return checker;
}

JITUINT32 get_return_IRType_For_Binary_Numeric_operation (JITUINT32 param1, JITUINT32 param2, JITUINT32 irOP, JITBOOLEAN *verified_code) {

    /* Assertions   */
    assert(verified_code != NULL);

    if (param1 == IRINT32 ) {
        switch (param2) {
            case IRINT32:
                return IRINT32;
            case IRNINT:
                return IRNINT;
        }
        if (    (param2 == IRMPOINTER) && (irOP == IRADD) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Numeric_operation"
                   ": WARNING! --> trying to execute a not verifiable operation"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOP, IRMPOINTER);
            (*verified_code) = 0;
            return IRMPOINTER;
        }
    } else if ( param1 == IRINT64 ) {
        if ( param2 == IRINT64 ) {
            return IRINT64;
        }
    } else if (param1 == IRNINT ) {
        if (    (param2 == IRINT32)
                || (param2 == IRNINT) ) {
            return IRNINT;
        }
        if (    (param2 == IRMPOINTER) && (irOP == IRADD) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Numeric_operation: WARNING! --> trying to execute an operation not verifiable" ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n", param1, param2, irOP, IRMPOINTER);
            (*verified_code) = 0;
            return IRMPOINTER;
        }
    } else if ( param1 == IRNFLOAT ) {
        if ( param2 == IRNFLOAT ) {
            return IRNFLOAT;
        }
        if ( param2 == IRFLOAT64) {
            return IRFLOAT64;
        }
        if ( param2 == IRFLOAT32 ) {
            return IRNFLOAT;
        }
    } else if ( param1 == IRFLOAT64 ) {
        if (    (param2 == IRFLOAT32)      ||
                (param2 == IRFLOAT64)      ||
                (  param2 == IRNFLOAT)       ) {
            return IRFLOAT64;
        }
    } else if ( param1 == IRFLOAT32 ) {
        if ( param2 == IRNFLOAT ) {
            return IRNFLOAT;
        }
        if ( param2 == IRFLOAT64) {
            return IRFLOAT64;
        }
        if ( param2 == IRFLOAT32 ) {
            return IRFLOAT32;
        }
    } else if ( param1 == IRMPOINTER ) {
        if (    (( param2 == IRINT32  ) || ( param2 == IRNINT))
                && ( (irOP == IRADD) || (irOP == IRSUB) ) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Numeric_operation "
                   ": WARNING! --> trying to execute an operation not verifiable"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOP, IRMPOINTER);
            (*verified_code) = 0;
            return IRMPOINTER;
        }
        if ( (param2 == IRMPOINTER) && (irOP == IRSUB) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Numeric_operation"
                   ": WARNING! --> trying to execute an operation not verifiable"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOP, IRNINT);
            (*verified_code) = 0;
            return IRNINT;
        }
    } else {
        PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Numeric_operation: WARNING! --> invalid binary operation: param1=%d, param2=%d, irOP=%d --> RETURNING %d \n", param1, param2, irOP, NOPARAM);
        return NOPARAM;
    }

    /* Return			*/
    PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Numeric_operation : WARNING! --> invalid binary operation : param1=%d, param2=%d, irOP=%d --> RETURNING %d \n", param1, param2, irOP, NOPARAM);
    return NOPARAM;
}

JITUINT32 get_conversion_infos_For_Conv_operation (JITUINT32 fromType, JITUINT32 toType, JITBOOLEAN *verified_code) {
    /* assertions */
    assert(verified_code != NULL);

    if (fromType == IRINT32) {
        switch (toType) {
            case IRINT8:
            case IRUINT8:
            case IRINT16:
            case IRUINT16:
                return TRUNCATE;
            case IRINT32:
            case IRUINT32:
                return NOP;
            case IRINT64:
            case IRNINT:
                return SIGN_EXTENDED;
            case IRUINT64:
            case IRNUINT:
                return ZERO_EXTENDED;
            case IRFLOAT32:
            case IRFLOAT64:
            case IRNFLOAT:
                return TO_FLOAT;
            case IROBJECT:
                return ZERO_EXTENDED;
        }
    } else if (fromType == IRINT64) {
        if (    (toType == IRINT8)
                || (toType == IRUINT8)
                || (toType == IRINT16)
                || (toType == IRUINT16)
                || (toType == IRINT32)
                || (toType == IRUINT32)
                || (toType == IRNINT)
                || (toType == IRNUINT) ) {
            return TRUNCATE;
        }
        if (    (toType == IRINT64)
                || (toType == IRUINT64)) {
            return NOP;
        }
        if (    (toType == IRFLOAT32)
                || (toType == IRFLOAT64)
                || (toType == IRNFLOAT) ) {
            return TO_FLOAT;
        }
    } else if (fromType == IRNINT) {
        if (    (toType == IRINT8)
                || (toType == IRUINT8)
                || (toType == IRINT16)
                || (toType == IRUINT16)
                || (toType == IRINT32)
                || (toType == IRUINT32)) {
            return TRUNCATE;
        }
        if (toType == IRINT64) {
            return SIGN_EXTENDED;
        }
        if (toType == IRUINT64) {
            return ZERO_EXTENDED;
        }
        if (    (toType == IRNINT)		||
                (toType == IRNUINT) 		||
                (toType == IRFPOINTER)		) {
            return NOP;
        }
        if (    (toType == IRFLOAT32)
                || (toType == IRFLOAT64)
                || (toType == IRNFLOAT) ) {
            return TO_FLOAT;
        }
    } else if (fromType == IRNFLOAT || fromType == IRFLOAT64 || fromType == IRFLOAT32) {
        if (    (toType == IRINT8)
                || (toType == IRUINT8)
                || (toType == IRINT16)
                || (toType == IRUINT16)
                || (toType == IRINT32)
                || (toType == IRUINT32)
                || (toType == IRINT64)
                || (toType == IRUINT64)
                || (toType == IRNINT)
                || (toType == IRNUINT) ) {
            return TRUNCATE_TO_ZERO;
        }
        if (    (toType == IRFLOAT32)
                || (toType == IRFLOAT64)
                || (toType == IRNFLOAT) ) {
            return CHANGE_PRECISION;
        }
    } else if ((fromType == IRMPOINTER) || (fromType == IROBJECT)) {
        if (    (toType == IRINT64)                                     ||
                (toType == IRUINT64)                                    ||
                (toType == IRNINT)                                      ||
                (toType == IRNUINT)                                     ||
                ((toType == IRUINT32) && (getIRSize(IRMPOINTER) == 4))  ||
                ((toType == IRINT32) && (getIRSize(IRMPOINTER) == 4))   ) {
            (*verified_code) = 0;
            return STOP_GC_TRACKING;
        }
    } else {
        PDEBUG("TYPE_CHECKER : get_conversion_infos_For_Conv_operation :"
               "WARNING! --> Conv Operation not valid :"
               "from=%d, to=%d --> RETURNING %d \n"
               , fromType, toType, INVALID_CONVERSION);
        return INVALID_CONVERSION;
    }

    PDEBUG("TYPE_CHECKER : get_conversion_infos_For_Conv_operation :"
           "WARNING! --> Conv Operation not valid :"
           "from=%d, to=%d --> RETURNING %d \n"
           , fromType, toType, INVALID_CONVERSION);
    return INVALID_CONVERSION;
}

JITUINT32 get_return_IRType_For_Shift_operation (JITUINT32 param1, JITUINT32 param2, JITUINT32 irOp) {
    if (param1 == param2) {
        return param1;
    }
    if (param1 == IRINT32) {
        if ((param2 != IRINT32) && (param2 != IRNINT)) {
            print_err("TYPE_CHECKER : get_return_IRType_For_Shift_operation"
                      ": ERROR! : Shift Operation not valid ", 0);
            PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                   ": ERROR! : invalid Right Operand\n");
            PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                   ": ERROR! : operand must be IRINT32 or IRNINT. Actually is %d\n", param2);
            abort();
        }
        return IRINT32;
    }
    if (param1 == IRINT64) {
        if ((param2 != IRINT32) && (param2 != IRNINT)) {
            print_err("TYPE_CHECKER : get_return_IRType_For_Shift_operation"
                      ": ERROR! : Shift Operation not valid ", 0);
            PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                   ": ERROR! : invalid Right Operand\n");
            PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                   ": ERROR! : operand must be IRINT32 or IRNINT. Actually is %d\n", param2);
            abort();
        }
        return IRINT64;
    }
    if (param1 == IRNINT) {
        if ((param2 != IRINT32) && (param2 != IRNINT)) {
            print_err("TYPE_CHECKER : get_return_IRType_For_Shift_operation"
                      ": ERROR! : Shift Operation not valid ", 0);
            PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                   ": ERROR! : invalid Right Operand\n");
            PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                   ": ERROR! : operand must be IRINT32 or IRNINT. Actually is %d\n", param2);
            abort();
        }
        return IRNINT;
    }

    print_err("TYPE_CHECKER : get_return_IRType_For_Shift_operation: ERROR! : Shift Operation not valid ", 0);
    PDEBUG("TYPE_CHECKER : get_return_IRType_For_Shift_operation: invalid SHIFT Operation\n");
    PDEBUG("OPERAND 1 == %d , OPERAND 2 == %d, irOP == %d \n", param1, param2, irOp);
    abort();
}

JITUINT32 get_return_IRType_For_Integer_operation (JITUINT32 param1, JITUINT32 param2) {
    if (param1 == IRINT32) {
        if (param2 == IRNINT) {
            return IRNINT;
        }
        if (param2 == IRINT32) {
            return IRINT32;
        }
    } else if ( (param1 == IRINT64) && (param2 == IRINT64)) {
        return IRINT64;
    } else if (param1 == IRNINT) {
        if ((param2 == IRINT32) || (param2 == IRNINT)) {
            return IRNINT;
        }
    } else {
        print_err("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
                  ": ERROR : invalid Integer Operation", 0);
        PDEBUG("TYPE_CHECKER: get_return_IRType_For_Shift_operation"
               ": ERROR : invalid Integer Operation\n");
        PDEBUG("OPERAND 1 == %d , OPERAND 2 == %d \n", param1, param2);
        abort();
        return NOPARAM;
    }

    return 1;
}

JITUINT32 get_return_IRType_For_Overflow_Arythmetic_operation (JITUINT32 param1
        , JITUINT32 param2, JITUINT32 irOp, JITBOOLEAN *verified_code) {
    if (param1 == IRINT32) {
        if (param2 == IRINT32) {
            return IRINT32;
        }
        if (param2 == IRNINT) {
            return IRNINT;
        }
        if ((param2 == IRMPOINTER) && (irOp == IRADDOVF)) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Overflow_Arythmetic_operation"
                   ": WARNING! --> trying to execute a not verifiable operation"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOp, IRMPOINTER);
            (*verified_code) = 0;
            return IRMPOINTER;
        }
    } else if ( (param1 == IRINT64) && (param2 == IRINT64) ) {
        return IRINT64;
    } else if (param1 == IRNINT) {
        if (param2 == IRINT32) {
            return IRNINT;
        }
        if (param2 == IRNINT) {
            return IRNINT;
        }
        if ((param2 == IRMPOINTER) && (irOp == IRADDOVF)) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Overflow_Arythmetic_operation"
                   ": WARNING! --> trying to execute a not verifiable operation"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOp, IRMPOINTER);
            (*verified_code) = 0;
            return IRMPOINTER;
        }
        return NOPARAM;
    } else if (param1 == IRMPOINTER) {
        if (    ((param2 == IRINT32) || (param2 == IRNINT))
                && ((irOp == IRADDOVF) || (irOp == IRSUBOVF)) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Overflow_Arythmetic_operation"
                   ": WARNING! --> trying to execute a not verifiable operation"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOp, IRMPOINTER);
            (*verified_code) = 0;
            return IRMPOINTER;
        }
        if ((param2 == IRMPOINTER) && (irOp == IRSUBOVF)) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Overflow_Arythmetic_operation"
                   ": WARNING! --> trying to execute a not verifiable operation"
                   ": param1=%d, param2=%d, irOP=%d --> RETURNING %d \n"
                   , param1, param2, irOp, IRNINT);
            (*verified_code) = 0;
            return IRNINT;
        }
    } else {
        print_err("TYPE_CHECKER : get_return_IRType_For_Overflow_Arythmetic_operation: ERROR! Operation not verifiable! ", 0);
        PDEBUG("GENERAL_TOOLS: get_return_IRType_For_Overflow_Arythmetic_operation: invalid Operation \n");
        PDEBUG("OPERAND 1 == %d , OPERAND 2 == %d, irOP == %d \n", param1, param2, irOp);
        abort();
    }

    print_err("TYPE_CHECKER : get_return_IRType_For_Overflow_Arythmetic_operation"
              ": ERROR! Operation not verifiable! ", 0);
    PDEBUG("GENERAL_TOOLS: get_return_IRType_For_Overflow_Arythmetic_operation: invalid Operation \n");
    PDEBUG("OPERAND 1 == %d , OPERAND 2 == %d, irOP == %d \n", param1, param2, irOp);
    abort();
}

JITUINT32 get_return_IRType_For_Binary_Comparison (JITUINT32 param1, JITUINT32 param2,
        JITUINT32 irOp, JITBOOLEAN *verified_code) {
    /* assertions */
    assert(verified_code != NULL);

    if (param1 == IRINT32) {
        if ((param2 != IRINT32) && (param2 != IRNINT)) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison : "
                   "WARNING! --> Operation not valid : param1=%d, param2=%d, "
                   "irOp=%d --> RETURNING %d \n"
                   , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
    } else if (param1 == IRNINT) {
        if (    ((param2 != IRINT32)
                 && (param2 != IRNINT)
                 && (param2 != IRMPOINTER))
                || ((param2 == IRMPOINTER) && (irOp != IREQ)) ) {
            PDEBUG( "TYPE_CHECKER : get_return_IRType_For_Binary_Comparison : "
                    "WARNING! --> Operation not valid : param1=%d, param2=%d, "
                    "irOp=%d --> RETURNING %d \n"
                    , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
        if ((param2 == IRMPOINTER) && (irOp == IREQ)) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison : "
                   "WARNING! --> trying to execute a not verifiable operation : "
                   "param1=%d, param2=%d, irOP=%d \n"
                   , param1, param2, irOp);
            (*verified_code) = 0;
        }

        /* verify the post-conditions */
        assert( (param2 == IRNINT) || (param2 == IRINT32) || (param2 == IRMPOINTER));
    } else if (param1 == IRINT64) {
        if (param2 != IRINT64) {
            PDEBUG( "TYPE_CHECKER : get_return_IRType_For_Binary_Comparison : "
                    "WARNING! --> Operation not valid : param1=%d, param2=%d, "
                    "irOp=%d --> RETURNING %d \n"
                    , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
    } else if (param1 == IRNFLOAT) {
        if (param2 != IRNFLOAT) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison "
                   ": WARNING! --> Operation not valid : param1=%d, param2=%d"
                   ", irOp=%d --> RETURNING %d \n"
                   , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
    } else if (param1 == IRFLOAT64) {
        if (param2 != IRFLOAT64) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison "
                   ": WARNING! --> Operation not valid : param1=%d, param2=%d"
                   ", irOp=%d --> RETURNING %d \n"
                   , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
    } else if (param1 == IRFLOAT32) {
        if (param2 != IRFLOAT32) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison "
                   ": WARNING! --> Operation not valid : param1=%d, param2=%d"
                   ", irOp=%d --> RETURNING %d \n"
                   , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
    } else if (param1 == IRMPOINTER) {
        if (    ((param2 != IRMPOINTER) && (param2 != IRNINT))
                || ((param2 == IRNINT) && (irOp != IREQ)) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison "
                   ": WARNING! --> Operation not valid : param1=%d, param2=%d"
                   ", irOp=%d --> RETURNING %d \n"
                   , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        } else if ( (param2 == IRNINT) && (irOp == IREQ) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison : "
                   "WARNING! --> trying to execute a not verifiable operation "
                   ": param1=%d, param2=%d, irOP=%d \n"
                   , param1, param2, irOp);
            (*verified_code) = 0;
        }
    } else if (param1 == IROBJECT) {
        if (    (param2 != IROBJECT)
                || ((param2 == IROBJECT) && (irOp != IRGT) && (irOp != IREQ)) ) {
            PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison "
                   ": WARNING! --> Operation not valid : param1=%d, param2=%d, "
                   "irOp=%d --> RETURNING %d \n"
                   , param1, param2, irOp, NOPARAM);
            return NOPARAM;
        }
    } else {
        PDEBUG("TYPE_CHECKER : get_return_IRType_For_Binary_Comparison : WARNING! --> "
               "Operation not valid : param1=%d, param2=%d, irOp=%d --> RETURNING %d \n"
               , param1, param2, irOp, NOPARAM);
        return NOPARAM;
    }

    /* the operation is valid. RETURN a IRINT32 type */
    return IRINT32;
}

JITUINT32 get_return_IRType_For_Unary_Numeric_Operation (JITUINT32 param) {
    if (    (param == IRINT32)
            || (param == IRINT64)
            || (param == IRNINT)
            || (param == IRFLOAT32)
            || (param == IRFLOAT64)
            || (param == IRNFLOAT)) {
        return param;
    }

    PDEBUG("TYPE_CHECKER: get_return_IRType_For_Unary_Numeric_Operation: "
           "WARNING: invalid unary numeric operation --> invalid parameter type (type == %d)  \n", param);
    PDEBUG("TYPE_CHECKER: get_return_IRType_For_Unary_Numeric_Operation: "
           "WARNING! returning \"NOPARAM\" as result \n");

    return NOPARAM;
}

JITBOOLEAN is_CLI_Type (JITUINT32 type_to_coerce) {

    if (    type_to_coerce == IRINT32
            || type_to_coerce == IRINT64
            || type_to_coerce == IRNINT
            || type_to_coerce == IRNFLOAT
            || type_to_coerce == IRFLOAT32
            || type_to_coerce == IRFLOAT64
            || type_to_coerce == IROBJECT
            || type_to_coerce == IRMPOINTER ) {
        return 1;
    }

    return 0;
}

JITUINT32 coerce_CLI_Type (JITUINT32 type_to_coerce) {

    switch (type_to_coerce) {
        case IRINT32:
        case IRINT64:
        case IRNINT:
        case IRNFLOAT:
        case IROBJECT:
        case IRMPOINTER:
        case IRFPOINTER:
        case IRFLOAT64:
        case IRFLOAT32:
        case IRVALUETYPE:
            return type_to_coerce;
        case IRINT8:
        case IRINT16:
        case IRUINT8:
        case IRUINT16:
        case IRUINT32:
            return IRINT32;
        case IRUINT64:
            return IRINT64;
        case IRNUINT:
            return IRNINT;
        case IRTPOINTER:
            print_err("TODO ... TYPE_CHECKER: coerce_CLI_Type : NOT YET IMPLEMENTED :( SORRY! ", 0);
            abort();
        default:
            print_err("TYPE_CHECKER : coerce_CLI_Type : ERROR! COERCION NOT POSSIBLE", 0);
            PDEBUG("TYPE_CHECKER : coerce_CLI_Type : TRIED TO COERCE %u, ABORTING...\n", type_to_coerce);
            abort();
    }
}

JITBOOLEAN is_unsigned_IRType (JITUINT32 type) {
    return type == IRUINT8
           || type == IRUINT16
           || type == IRUINT32
           || type == IRUINT64
           || type == IRNUINT;
}

JITUINT32 to_unsigned_IRType (JITUINT32 type) {
    JITUINT32 unsigned_IRtype;

    /* local variables initialization */
    unsigned_IRtype = NOPARAM;

    /* check if the type is already an unsigned type */
    if (is_unsigned_IRType(type)) {
        return type;
    }

    /* if type identifies a floating point type, no equivalent type is defined */
    if (    type == IRFLOAT32
            || type == IRFLOAT64
            || type == IRNFLOAT) {
        return type;
    }

    /* find the corrispondent unsigned value */
    switch (type) {
        case IRINT8:
            unsigned_IRtype = IRUINT8;
            break;
        case IRINT16:
            unsigned_IRtype = IRUINT16;
            break;
        case IRINT32:
            unsigned_IRtype = IRUINT32;
            break;
        case IRINT64:
            unsigned_IRtype = IRUINT64;
            break;
        case IRNINT:
        case IROBJECT:
        case IRMPOINTER:
            unsigned_IRtype = IRNUINT;
            break;
        default:
            break;
    }

    /* return the unsigned IR type. This function returns NOPARAM only if
     * an invalid parameter is passed as input */
    return unsigned_IRtype;
}
