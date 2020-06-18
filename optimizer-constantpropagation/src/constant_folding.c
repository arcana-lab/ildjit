/*
 * Copyright (C) 2008 - 2012  Campanoni Simone
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
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <constant_propagation.h>
#include <misc.h>
// End

static inline void internal_folding (ir_method_t *method, ir_instruction_t *opt_inst);
static inline void folding_arithmetic_1_operand (ir_method_t *method, ir_instruction_t *opt_inst);
static inline void folding_arithmetic_2_operands (ir_method_t *method, ir_instruction_t *opt_inst);
static inline void internal_transformConditionalBranchToUnconditional (ir_method_t *method, ir_instruction_t *inst);

void constant_folding (ir_method_t *method) {
    JITUINT32 		instID;
    ir_instruction_t        *inst;

    /* Assertions                   */
    assert(method != NULL);

    /* Perform the constant folding */
    for (instID = 0; instID < IRMETHOD_getInstructionsNumber(method); instID++) {
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        internal_folding(method, inst);
    }

    return;
}

static inline void internal_folding (ir_method_t *method, ir_instruction_t *opt_inst) {
    JITUINT32 type;

    /* Assertions			*/
    assert(method != NULL);
    assert(opt_inst != NULL);

    /* Fetch the instruction type	*/
    type = IRMETHOD_getInstructionType(opt_inst);

    /* Handle arithmetics with two operands.
     */
    if (     (type == IRADD)         ||
             (type == IRMUL)         ||
             (type == IRSUB)         ||
             (type == IRAND)         ||
             (type == IROR)          ||
             (type == IRXOR)         ||
             (type == IRSHL)         ||
             (type == IRSHR)         ||
             (type == IRREM)         ||
             (type == IRPOW)         ||
             (type == IRGT)          ||
             (type == IRLT)          ||
             (type == IREQ)          ||
             (type == IRDIV)         ) {

        /* Fold the arithmetic operation		*/
        folding_arithmetic_2_operands(method, opt_inst);
        return;
    }

    /* Handle arithmetics with one operand.
     */
    if (            (type == IRNEG)         ||
                    (type == IRBITCAST)     ||
                    (type == IRNOT)         ||
                    (type == IRISNAN)       ||
                    (type == IRISINF)       ||
                    (type == IRSIN)         ||
                    (type == IRCOS)         ||
                    (type == IRACOS)        ||
                    (type == IRCOSH)        ||
                    (type == IRCEIL)        ||
                    (type == IRFLOOR)       ||
                    (type == IREXP)       	||
                    (type == IRLOG10)      	||
                    (type == IRSIZEOF)      ||
                    (type == IRSQRT)        ) {

        /* Fold the arithmetic operation		*/
        folding_arithmetic_1_operand(method, opt_inst);
        return;
    }

    /* Handle conditional branches.
     */
    if (type == IRBRANCHIF) {
        ir_item_t *par1;
        ir_item_t resolvedSymbol;

        /* Fetch the branch condition				*/
        par1 = IRMETHOD_getInstructionParameter1(opt_inst);
        assert(par1 != NULL);

        /* Check if the branch condition is a constant		*/
        if (par1->type == IROFFSET) {
            return;
        }
        if (par1->type == IRSYMBOL) {
            IRSYMBOL_resolveSymbolFromIRItem(par1, &resolvedSymbol);
            par1 = &resolvedSymbol;
        }
        assert(par1->type != IRSYMBOL);

        /* Check if the condition is a pointer	*/
        if (    (par1->type != IROBJECT)        &&
                (par1->type != IRMPOINTER)      &&
                (par1->type != IRNINT)          ) {

            /* The condition is not a pointer	*/
            if (par1->value.v == 0) {

                /* This branch is never taken.
                 * The instruction can be deleted.
                 */
                IRMETHOD_deleteInstruction(method, opt_inst);

            } else {

                /* This branch is always taken.
                 * Transform the conditional branch to an unconditional one.
                 */
                internal_transformConditionalBranchToUnconditional(method, opt_inst);
            }

        } else {

            /* The condition is on a pointer.
             * It means that null != 0 and therefore the branch is always taken.
             * Transform the conditional branch to an unconditional one.
             */
            internal_transformConditionalBranchToUnconditional(method, opt_inst);
        }
        return;
    }
    if (type == IRBRANCHIFNOT) {
        ir_item_t *par1;
        ir_item_t resolvedSymbol;

        /* Fetch the branch condition				*/
        par1 = IRMETHOD_getInstructionParameter1(opt_inst);
        assert(par1 != NULL);

        /* Check if the branch condition is a constant		*/
        if (par1->type == IROFFSET) {
            return;
        }
        if (par1->type == IRSYMBOL) {
            IRSYMBOL_resolveSymbolFromIRItem(par1, &resolvedSymbol);
            par1 = &resolvedSymbol;
        }
        assert(par1->type != IRSYMBOL);

        /* The condition is not a pointer	*/
        if (par1->value.v != 0) {

            /* This branch is never taken	*/
            IRMETHOD_deleteInstruction(method, opt_inst);

        } else {

            /* This branch is always taken.
             * Transform the conditional branch to an unconditional one.
             */
            internal_transformConditionalBranchToUnconditional(method, opt_inst);
        }
        return;
    }

    /* Handle conversions.
     */
    if (type == IRCONV) {
        ir_item_t *parameter;
        ir_item_t valueToConvert;
        ir_item_t *parameter2;
        JITUINT32 valueType;
        JITUINT32 typeToConvert;
        JITBOOLEAN modified;

        /* Fetch the value to convert                           */
        parameter = IRMETHOD_getInstructionParameter1(opt_inst);
        assert(parameter != NULL);

        /* Check if the value to convert is a constant		*/
        if (parameter->type == IROFFSET) {
            return;
        }
        if (parameter->type == IRSYMBOL) {
            return;
        }

        /* Initialize the variables				*/
        modified = JITFALSE;

        /* Fetch the second parameter				*/
        parameter2 = IRMETHOD_getInstructionParameter2(opt_inst);

        /* Fetch the target type                                */
        typeToConvert = (parameter2->value).v;

        /* Copy the value to convert				*/
        memcpy(&valueToConvert, parameter, sizeof(ir_item_t));
        assert(valueToConvert.type != IROFFSET);

        /* Fetch the type of the value to convert               */
        valueType = valueToConvert.type;

        /* Check if we need to resolve a symbol			*/
        if (valueType == IRSYMBOL) {
            ir_item_t resolvedSymbol;
            IRSYMBOL_resolveSymbolFromIRItem(&valueToConvert, &resolvedSymbol);
            memcpy(&valueToConvert, &resolvedSymbol, sizeof(ir_item_t));
            valueType = valueToConvert.type;
        }
        assert(valueType != IRSYMBOL);

        /* Check if the checking overflow has to be performed   */
        if (	(IRMETHOD_isAnIntType(valueType))	||
                (IRMETHOD_isAPointerType(valueType))	) {
            JITUINT32	typeToConvertNotNative;
            typeToConvertNotNative	= typeToConvert;

            if (IRMETHOD_isAPointerType(typeToConvertNotNative)) {
                typeToConvertNotNative	= IRNUINT;
            }
            if (typeToConvertNotNative == IRNFLOAT) {
                if (IRDATA_getSizeOfIRType(IRNFLOAT) == 4) {
                    typeToConvertNotNative	= IRFLOAT32;
                } else {
                    typeToConvertNotNative	= IRFLOAT64;
                }

            } else if (typeToConvertNotNative == IRNUINT) {
                if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
                    typeToConvertNotNative	= IRUINT32;
                } else {
                    typeToConvertNotNative	= IRUINT64;
                }

            } else if (typeToConvertNotNative == IRNINT) {
                if (IRDATA_getSizeOfIRType(IRNINT) == 4) {
                    typeToConvertNotNative	= IRINT32;
                } else {
                    typeToConvertNotNative	= IRINT64;
                }
            }

            switch (typeToConvertNotNative) {
                case IRINT8:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITINT8) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRINT16:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITINT16) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRINT32:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITINT32) (JITINT64)(valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRINT64:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITINT64) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRUINT8:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITUINT8) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRUINT16:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITUINT16) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRUINT32:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITUINT32) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRUINT64:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, (JITUINT64) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRFLOAT64:
                    IRMETHOD_setInstructionParameter1FValue(method, opt_inst, (JITFLOAT64) (JITINT64) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
                case IRFLOAT32:
                    IRMETHOD_setInstructionParameter1FValue(method, opt_inst, (JITFLOAT32) (JITINT64) (valueToConvert.value).v);
                    modified = JITTRUE;
                    break;
            }

        } else if (IRMETHOD_isAFloatType(valueType)) {
            if (	(IRMETHOD_isAPointerType(typeToConvert))	||
                    (typeToConvert == IRNINT)			||
                    (typeToConvert == IRNUINT)			) {
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, (IR_ITEM_VALUE) (JITNUINT) (valueToConvert.value).f);
                modified = JITTRUE;

            } else if (IRMETHOD_isAFloatType(typeToConvert)) {
                JITUINT32	typeToConvertNotNative;
                typeToConvertNotNative	= typeToConvert;
                if (typeToConvertNotNative == IRNFLOAT) {
                    if (IRDATA_getSizeOfIRType(IRNFLOAT) == 4) {
                        typeToConvertNotNative	= IRFLOAT32;
                    } else {
                        typeToConvertNotNative	= IRFLOAT64;
                    }
                }
                switch (typeToConvertNotNative) {
                    case IRFLOAT32:
                        IRMETHOD_setInstructionParameter1FValue(method, opt_inst, (JITFLOAT32) (valueToConvert.value).f);
                        break;
                    case IRFLOAT64:
                        IRMETHOD_setInstructionParameter1FValue(method, opt_inst, (JITFLOAT64) (valueToConvert.value).f);
                        break;
                }
                modified = JITTRUE;
            }
        }

        /* Conclude the modification		*/
        if (modified) {
            IRMETHOD_setInstructionType(method, opt_inst, IRMOVE);
            IRMETHOD_eraseInstructionParameter(method, opt_inst, 2);
            parameter->type = typeToConvert;
            parameter->internal_type = typeToConvert;
        }
    }

    /* Return                       */
    return;
}

static inline void folding_arithmetic_2_operands (ir_method_t *method, ir_instruction_t *opt_inst) {
    JITUINT32 	type;
    ir_item_t 	*par1Item;
    ir_item_t 	*par2Item;
    ir_item_t 	*resultItem;
    JITUINT64 	prevRes;
    JITUINT32 	prevResType;
    TypeDescriptor 	*prevResTypeInfos;

    /* Assertions			*/
    assert(method != NULL);
    assert(opt_inst != NULL);
    assert(IRMETHOD_getInstructionParametersNumber(opt_inst) == 2);

    /* Fetch the parameters					*/
    par1Item = IRMETHOD_getInstructionParameter1(opt_inst);
    par2Item = IRMETHOD_getInstructionParameter2(opt_inst);

    /* Check if the two parameters are both constants	*/
    if (    IRDATA_isAVariable(par1Item)	||
            IRDATA_isAVariable(par2Item) 	) {
        return;
    }
    if (    IRDATA_isASymbol(par1Item)	||
            IRDATA_isASymbol(par2Item) 	) {
        return;
    }

    /* Fetch the result					*/
    resultItem = IRMETHOD_getInstructionResult(opt_inst);

    /* Fetch the instruction type				*/
    type = IRMETHOD_getInstructionType(opt_inst);

    /* Fetch the result of the operation			*/
    prevRes = IRMETHOD_getInstructionResult(opt_inst)->value.v;
    prevResType = IRMETHOD_getInstructionResult(opt_inst)->internal_type;
    prevResTypeInfos = IRMETHOD_getInstructionResult(opt_inst)->type_infos;

    /* Check if the parameters are integers.
     */
    if (    (!IRMETHOD_hasAFloatType(par1Item))      &&
            (!IRMETHOD_hasAFloatType(par2Item))      ) {
        IR_ITEM_VALUE	unsignedPar1;
        IR_ITEM_VALUE	unsignedPar2;
        JITBOOLEAN	parametersAreSigned;

        /* Change the instruction to a move operation.
         */
        IRMETHOD_setInstructionType(method, opt_inst, IRMOVE);

        /* Fetch the values of the parameters.
         */
        if (par1Item->type == IRSYMBOL) {
            ir_item_t temp;
            IRSYMBOL_resolveSymbolFromIRItem(par1Item, &temp);
            unsignedPar1 = temp.value.v;
        } else {
            unsignedPar1 = (par1Item->value).v;
        }
        if (par2Item->type == IRSYMBOL) {
            ir_item_t temp;
            IRSYMBOL_resolveSymbolFromIRItem(par2Item, &temp);
            unsignedPar2 = temp.value.v;
        } else {
            unsignedPar2 = (par2Item->value).v;
        }

        /* Check if the parameters are signed.
         */
        parametersAreSigned	= IRMETHOD_hasASignedType(par1Item);

        /* Set the IR type of the source of the move instruction.
         */
        par1Item->type = prevResType;
        par1Item->internal_type = prevResType;
        par1Item->type_infos = prevResTypeInfos;

        /* Set the result of the move instruction.
         */
        resultItem->type = IROFFSET;
        resultItem->value.v = prevRes;
        resultItem->type_infos = prevResTypeInfos;

        /* Apply the folding depending on the sign of the parameters.
         */
        if (parametersAreSigned) {
            JITINT64	par1;
            JITINT64	par2;

            /* Set the values.
             */
            par1	= (JITINT64) unsignedPar1;
            par2	= (JITINT64) unsignedPar2;

            /* Perform the operation.
             */
            switch (type) {
                case IRADD:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 + par2);
                    break;
                case IRMUL:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 * par2);
                    break;
                case IRSUB:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 - par2);
                    break;
                case IRDIV:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 == 0 ? 0 : par1 / par2);
                    break;
                case IRREM:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 % par2);
                    break;
                case IRAND:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 & par2);
                    break;
                case IROR:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 | par2);
                    break;
                case IRXOR:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 ^ par2);
                    break;
                case IRSHL:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 << par2);
                    break;
                case IRSHR:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 >> par2);
                    break;
                case IRGT:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 > par2);
                    break;
                case IRLT:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 < par2);
                    break;
                case IREQ:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 == par2);
                    break;
                default:
                    print_err("OPTIMIZER_CONSTANTPROPAGATION: Error on constant folding: instruction type is not correct. ", 0);
                    abort();
            }

        } else {
            IR_ITEM_VALUE	par1;
            IR_ITEM_VALUE	par2;

            /* Set the values.
             */
            par1	= unsignedPar1;
            par2	= unsignedPar2;

            /* Perform the operation                                */
            switch (type) {
                case IRADD:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 + par2);
                    break;
                case IRMUL:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 * par2);
                    break;
                case IRSUB:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 - par2);
                    break;
                case IRDIV:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 / par2);
                    break;
                case IRREM:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 % par2);
                    break;
                case IRAND:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 & par2);
                    break;
                case IROR:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 | par2);
                    break;
                case IRXOR:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 ^ par2);
                    break;
                case IRSHL:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 << par2);
                    break;
                case IRSHR:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 >> par2);
                    break;
                case IRGT:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 > par2);
                    break;
                case IRLT:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 < par2);
                    break;
                case IREQ:
                    IRMETHOD_setInstructionParameter1Value(method, opt_inst, par1 == par2);
                    break;
                default:
                    print_err("OPTIMIZER_CONSTANTPROPAGATION: Error on constant folding: instruction type is not correct. ", 0);
                    abort();
            }
        }
        IRMETHOD_eraseInstructionParameter(method, opt_inst, 2);

        return;
    }

    /* Check if the parameters are floating			*/
    if (    (IRMETHOD_hasAFloatType(par1Item))       &&
            (IRMETHOD_hasAFloatType(par2Item))       ) {
        IR_ITEM_FVALUE	f_par1;
        IR_ITEM_FVALUE 	f_par2;
        IR_ITEM_FVALUE 	f_result;
        JITUINT32	dataSize;

        /* Fetch the number of bytes of the data type.
         */
        dataSize	= IRDATA_getSizeOfType(par1Item);

        /* Fetch the values of the parameters.
         */
        f_par1 = (par1Item->value).f;
        f_par2 = (par2Item->value).f;

        /* Set the type of the result of the constant folding.
         */
        par1Item->type = prevResType;
        par1Item->internal_type = prevResType;
        par1Item->type_infos = prevResTypeInfos;

        /* Change the instruction to IRMOVE.
         */
        IRMETHOD_setInstructionType(method, opt_inst, IRMOVE);

        /* Set the variable to set.
         */
        resultItem->type = IROFFSET;
        resultItem->value.v = prevRes;
        resultItem->type_infos = prevResTypeInfos;

        /* Perform the operation                */
        switch (type) {
            case IRADD:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_par1 + f_par2);
                break;
            case IRAND:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, ((IR_ITEM_VALUE) f_par1) & ((IR_ITEM_VALUE) f_par2));
                break;
            case IROR:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, ((IR_ITEM_VALUE) f_par1) | ((IR_ITEM_VALUE) f_par2));
                break;
            case IRXOR:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, ((IR_ITEM_VALUE) f_par1) ^ ((IR_ITEM_VALUE) f_par2));
                break;
            case IRSHL:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, ((IR_ITEM_VALUE) f_par1) << ((IR_ITEM_VALUE) f_par2));
                break;
            case IRSHR:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, ((JITINT64) f_par1) >> ((JITINT64) f_par2));
                break;
            case IRMUL:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_par1 * f_par2);
                break;
            case IRSUB:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_par1 - f_par2);
                break;
            case IRDIV:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_par1 / f_par2);
                break;
            case IRREM:
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, ((IR_ITEM_VALUE) f_par1) % ((IR_ITEM_VALUE) f_par2));
                break;
            case IRPOW:
                if (dataSize == 4) {
                    f_result	= (IR_ITEM_FVALUE)powf(f_par1, f_par2);
                } else {
                    f_result	= (IR_ITEM_FVALUE)pow(f_par1, f_par2);
                }
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_result);
                break;
            case IREXP:
                if (dataSize == 4) {
                    f_result	= (IR_ITEM_FVALUE)expf(f_par1);
                } else {
                    f_result	= (IR_ITEM_FVALUE)exp(f_par1);
                }
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_result);
                break;
            case IRLOG10:
                if (dataSize == 4) {
                    f_result	= (IR_ITEM_FVALUE)log10f(f_par1);
                } else {
                    f_result	= (IR_ITEM_FVALUE)log10(f_par1);
                }
                IRMETHOD_setInstructionParameter1FValue(method, opt_inst, f_result);
                break;
            case IRGT:
                IRMETHOD_setInstructionParameter1Value(method, opt_inst, f_par1 > f_par2);
                break;
            case IRLT:
                IRMETHOD_setInstructionParameter1Value(method, opt_inst, f_par1 < f_par2);
                break;
            case IREQ:
                IRMETHOD_setInstructionParameter1Value(method, opt_inst, f_par1 == f_par2);
                break;
            default:
                print_err("OPTIMIZER_CONSTANTPROPAGATION: Error on constant folding: instruction type is not correct. ", 0);
                abort();
        }
        IRMETHOD_eraseInstructionParameter(method, opt_inst, 2);

        return;
    }

    return;
}

static inline void folding_arithmetic_1_operand (ir_method_t *method, ir_instruction_t *opt_inst) {
    ir_item_t       *par1;
    ir_item_t       *par2;
    ir_item_t 	par1Copy;
    ir_item_t 	resultCopy;
    JITUINT32 	type;
    IR_ITEM_VALUE	valueToCast;

    /* Assertions			*/
    assert(method != NULL);
    assert(opt_inst != NULL);
    assert(IRMETHOD_getInstructionParametersNumber(opt_inst) == 1);

    /* Fetch the type of the parameter		*/
    par1 = IRMETHOD_getInstructionParameter1(opt_inst);
    assert(par1 != NULL);
    assert(par1->type != NOPARAM);

    /* Check if the parameter is a constant		*/
    if (    IRDATA_isAVariable(par1)	        ||
            IRDATA_isASymbol(par1)	            ||
            (par1->internal_type == IRMETHODID) ){
        return;
    }

    /* Copy the parameter				*/
    memcpy(&par1Copy, par1, sizeof(ir_item_t));

    /* Copy the result				*/
    memcpy(&resultCopy, IRMETHOD_getInstructionResult(opt_inst), sizeof(ir_item_t));

    /* Fetch the instruction type			*/
    type = IRMETHOD_getInstructionType(opt_inst);

    /* Transform the instruction to a store		*
     * operation		                        */
    IRMETHOD_eraseInstructionFields(method, opt_inst);
    opt_inst->type = IRMOVE;
    IRMETHOD_cpInstructionResult(opt_inst, &resultCopy);
    par2 = IRMETHOD_getInstructionParameter1(opt_inst);
    memcpy(par2, &resultCopy, sizeof(ir_item_t));
    par2->type = par2->internal_type;

    /* Perform the operation			*/
    switch (type) {
        case IRNEG:
            if (IRMETHOD_hasAFloatType(&par1Copy)) {
                (par2->value).f = -((par1Copy.value).f);
            } else {
                par2->value.v = -((par1Copy.value).v);
            }
            break;
        case IRNOT:
            (par2->value).v = ~((par1Copy.value).v);
            break;
        case IRISNAN:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).v = isnan((par1Copy.value).f);
            break;
        case IRISINF:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).v = isinf((par1Copy.value).f);
            break;
        case IRSIN:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = sin((par1Copy.value).f);
            break;
        case IRCOS:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = cos((par1Copy.value).f);
            break;
        case IRCOSH:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = cosh((par1Copy.value).f);
            break;
        case IRCEIL:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = ceil((par1Copy.value).f);
            break;
        case IRACOS:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = acos((par1Copy.value).f);
            break;
        case IRSQRT:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = sqrt((par1Copy.value).f);
            break;
        case IRFLOOR:
            assert(IRMETHOD_hasAFloatType(&par1Copy));
            (par2->value).f = floor((par1Copy.value).f);
            break;
        case IRBITCAST:
            memcpy(par2, &par1Copy, sizeof(ir_item_t));
            par2->type_infos	 = NULL;
            par2->internal_type	= resultCopy.internal_type;
            par2->type 		= par2->internal_type;
            valueToCast		= 0;
            switch (IRDATA_getSizeOfType(par2)) {
                case 8:
                    valueToCast		= 0xFFFFFFFFFFFFFFFF;
                    break;
                case 4:
                    valueToCast		= 0xFFFFFFFF;
                    break;
                case 3:
                    valueToCast		= 0xFFFFFF;
                    break;
                case 2:
                    valueToCast		= 0xFFFF;
                    break;
                case 1:
                    valueToCast		= 0xFF;
                    break;
            }
            (par2->value).v	&= valueToCast;
            break;
        case IRSIZEOF:
            if (	((par1Copy.value).v == IROBJECT)	||
                    ((par1Copy.value).v == IRVALUETYPE)	) {
                (par2->value).v	= IRDATA_getSizeOfObject(&par1Copy);
            } else {
                (par2->value).v	= IRDATA_getSizeOfType(&par1Copy);
            }
            break;
        default:
            abort();
    }

    return;
}

static inline void internal_transformConditionalBranchToUnconditional (ir_method_t *method, ir_instruction_t *inst) {
    ir_item_t	*par2;
    ir_item_t	labelToJump;

    /* This branch is always taken.
     * Fetch the target label.
     */
    par2	= IRMETHOD_getInstructionParameter2(inst);
    assert(par2 != NULL);
    assert(par2->type == IRLABELITEM);
    memcpy(&labelToJump, par2, sizeof(ir_item_t));

    /* Transform the conditional branch to an unconditional one.
     */
    IRMETHOD_eraseInstructionFields(method, inst);
    IRMETHOD_setInstructionType(method, inst, IRBRANCH);
    IRMETHOD_cpInstructionParameter1(inst, &labelToJump);

    return ;
}
