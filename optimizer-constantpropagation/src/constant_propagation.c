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
#include <constant_propagation_codetool.h>
#include <misc.h>
// End

static inline void internal_constant_propagation_apply (ir_method_t *method, ir_instruction_t *store_inst, ir_instruction_t *use_inst);

void constant_propagation (ir_method_t *method) {
    ir_instruction_t        *inst;
    ir_instruction_t        *defInst;
    JITUINT32 instID;
    JITINT32 blockBits;
    JITNUINT inst_use;
    JITNUINT temp;
    JITINT32 count;
    JITINT32 count2;
    XanHashTable            *defsTable;

    /* Assertions                   */
    assert(method != NULL);

    if (sizeof(JITNINT) == 4) {
        blockBits = 32;
    } else {
        blockBits = 64;
    }

    /* Fetch all definitions of all variables.
     */
    defsTable = IRMETHOD_getAllVariableDefinitions(method);

    /* Looking for instructions      */
    for (instID = 0; instID < IRMETHOD_getInstructionsNumber(method); instID++) {
        IR_ITEM_VALUE used_var;

        /* Fetch the instruction		*/
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);

        /* Apply the constant propagation       */
        for (count = 0; count < method->livenessBlockNumbers; count++) {
            inst_use = livenessGetUse(method, inst, count);
            for (count2 = 0; count2 < blockBits; count2++) {
                used_var = count * blockBits;
                used_var = used_var + count2;
                temp = 0x1;
                temp = temp << count2;
                if ((inst_use & temp) == temp) {

                    /* Check if the variable used has only  *
                    * one definition reached in            */
                    defInst = internal_getSingleDefinitionReachedIn(method, used_var, inst, defsTable);
                    if (defInst != NULL) {
                        IR_ITEM_VALUE	definedVar;

                        /* Fetch the defined variable.
                         */
                        definedVar	= IRMETHOD_getInstructionResultValue(defInst);

                        /* defInst is the only definition that reaches the instruction inst.
                         */
                        if (    (IRMETHOD_getInstructionType(defInst) == IRMOVE)                &&
                                (IRMETHOD_getInstructionParameter1Type(defInst) != IROFFSET)   	&&
                                (!IRMETHOD_isAnEscapedVariable(method, definedVar))       	) {

                            /* Check if the variable defined by the IRMOVE is a parameter.		*/
                            if (definedVar < IRMETHOD_getMethodParametersNumber(method)) {
                                continue;
                            }

                            assert(used_var == IRMETHOD_getInstructionResultValue(defInst));
                            internal_constant_propagation_apply(method, defInst, inst);
                        }
                    }
                }
            }
        }
    }

    /* Free the memory.
     */
    internal_destroyDefsTable(defsTable);

    return;
}

static inline void internal_constant_propagation_apply (ir_method_t *method, ir_instruction_t *store_inst, ir_instruction_t *use_inst) {
    JITUINT32 inner_type;
    JITUINT64 definedValue;
    JITUINT32 parNumb;
    JITUINT32 totalPar;
    ir_item_t *valueToPropagate;

    /* Assertions			*/
    assert(method != NULL);
    assert(use_inst != NULL);
    assert(store_inst != NULL);
    assert(IRMETHOD_getInstructionType(store_inst) == IRMOVE);
    assert(IRMETHOD_getInstructionParameter2Type(store_inst) != IROFFSET);

    /* Fetch the instruction type	*/
    inner_type = IRMETHOD_getInstructionType(use_inst);

    /* Check if the instruction     *
     * takes the address of a       *
     * variable			*/
    if (inner_type == IRGETADDRESS) {
        return;
    }

    /* Fetch the parameter type	*/
    valueToPropagate = IRMETHOD_getInstructionParameter1(store_inst);

    /* Check if the instruction is	*
     * a call			*/
    if (    (inner_type == IRCALL)          ||
            (inner_type == IRVCALL)         ||
            (inner_type == IRLIBRARYCALL)   ||
            (inner_type == IRNATIVECALL)    ||
            (inner_type == IRICALL)         ) {
        JITUINT32 callParNumb;

        //Prendo il numero dei parametri della funzione.
        totalPar = IRMETHOD_getInstructionCallParametersNumber(use_inst);

        //Ciclo sui parametri alla ricerca della variabile da sostituire (NB: potrebbe non esserci più!)
        for (callParNumb = 0; callParNumb < totalPar; callParNumb++) {
            JITUINT32 parType = IRMETHOD_getInstructionCallParameterType(use_inst, callParNumb);

            //Ho trovato una variabile, controllo che sia quella corretta.
            if (parType == IROFFSET) {
                ir_item_t *callParameter;
                definedValue = IRMETHOD_getInstructionResultValue(store_inst);
                callParameter = IRMETHOD_getInstructionCallParameter(use_inst, callParNumb);

                /* Check the variable defined by IRMOVE is the one used as a call parameter in the call instruction.
                 */
                if ((callParameter->value).v == definedValue) {

                    /* Forward the constant				*/
                    memcpy(callParameter, valueToPropagate, sizeof(ir_item_t));
                }
            }
        }

        /* Return				*/
        return;
    }

    /* The instruction is not a call		*/
    totalPar = IRMETHOD_getInstructionParametersNumber(use_inst);
    definedValue = IRMETHOD_getInstructionResultValue(store_inst);

    //Ciclo sui parametri alla ricerca della variabile da sostituire (NB: potrebbe non esserci più!)
    for (parNumb = 0; parNumb < totalPar; parNumb++) {
        ir_item_t *parameter;
        JITUINT32 parType;
        JITUINT64 parValue;
        switch (parNumb) {
            case 2:
                parType = IRMETHOD_getInstructionParameter3Type(use_inst);
                if (parType != IROFFSET) {
                    break;
                }

                //Ho trovato una variabile, controllo che sia quella corretta.
                parValue = IRMETHOD_getInstructionParameter3Value(use_inst);

                //Contronto la variabile definita dalla IRMOVE sia la stessa usata come parametro.
                if (parValue != definedValue) {
                    break;
                }

                /* Forward the constant             */
                parameter = IRMETHOD_getInstructionParameter3(use_inst);
                memcpy(parameter, valueToPropagate, sizeof(ir_item_t));
                break;
            case 1:
                parType = IRMETHOD_getInstructionParameter2Type(use_inst);
                if (parType != IROFFSET) {
                    break;
                }

                //Ho trovato una variabile, controllo che sia quella corretta.
                parValue = IRMETHOD_getInstructionParameter2Value(use_inst);

                //Contronto la variabile definita dalla IRMOVE sia la stessa usata come parametro.
                if (parValue != definedValue) {
                    break;
                }

                /* Forward the constant             */
                parameter = IRMETHOD_getInstructionParameter2(use_inst);
                memcpy(parameter, valueToPropagate, sizeof(ir_item_t));
                break;
            case 0:
                parType = IRMETHOD_getInstructionParameter1Type(use_inst);
                if (parType != IROFFSET) {
                    break;
                }

                //Ho trovato una variabile, controllo che sia quella corretta.
                parValue = IRMETHOD_getInstructionParameter1Value(use_inst);

                //Contronto la variabile definita dalla IRMOVE sia la stessa usata come parametro.
                if (parValue != definedValue) {
                    break;
                }

                /* Forward the constant             */
                parameter = IRMETHOD_getInstructionParameter1(use_inst);
                memcpy(parameter, valueToPropagate, sizeof(ir_item_t));
                break;
        }
    }

    return;
}
