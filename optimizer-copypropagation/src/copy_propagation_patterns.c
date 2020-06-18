/*
 * Copyright (C) 2006 - 2012  Campanoni Simone
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
#include <assert.h>
#include <xanlib.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <copy_propagation.h>
#include <copy_propagation_patterns.h>
// End

typedef struct {
    ir_instruction_t	*loadInst;
    ir_instruction_t	*getaddrInst;
    ir_instruction_t	*memcpyInst;
} memcpy_insts_t;

extern ir_optimizer_t  *irOptimizer;

JITBOOLEAN apply_copy_propagation_for_value_types (ir_method_t *method) {
    XanList		*memcpyInsts;
    XanList		*l;
    XanListItem	*item;
    JITBOOLEAN	codeChanged;

    /* Assertions											*/
    assert(method != NULL);

    /* Run the necessary analysis									*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, REACHING_DEFINITIONS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);

    /* Fetch the list of instructions that transfer memory data between pointers to value types	*/
    memcpyInsts	= IRMETHOD_getInstructionsOfType(method, IRMEMCPY);

    /* Compute the list of instructions to optimize							*/
    l		= xanList_new(allocFunction, freeFunction, NULL);
    item		= xanList_first(memcpyInsts);
    while (item != NULL) {
        ir_instruction_t	*inst;
        ir_item_t		*srcMem;

        /* Fetch the instruction									*/
        inst		= item->data;
        assert(inst->type == IRMEMCPY);

        /* Fetch the pointer that points to the memory location to read from				*/
        srcMem		= IRMETHOD_getInstructionParameter2(inst);

        /* Check if it is a defined variable								*/
        if (IRDATA_isAVariable(srcMem)) {
            ir_instruction_t	*defInst;
            JITUINT32		numUses;
            XanList			*tempL;

            /* Fetch the definition of the variable that reaches the current instruction			*/
            defInst		= NULL;
            tempL	= IRMETHOD_getVariableDefinitionsThatReachInstruction(method, inst, (srcMem->value).v);
            if (xanList_length(tempL) == 1) {
                defInst	= xanList_first(tempL)->data;
                assert(defInst != NULL);
            }
            xanList_destroyList(tempL);

            /* Compute the number of uses of the defined variable						*/
            numUses		= 0;
            tempL		= IRMETHOD_getVariableUses(method, (srcMem->value).v);
            numUses		= xanList_length(tempL);
            assert(numUses > 0);
            xanList_destroyList(tempL);

            /* In case (1) there is a unique definition (2) it is a getaddress instruction (3) and this defined variable has only one use, continue the optimization	*/
            if (	(defInst != NULL)		&&
                    (defInst->type == IRGETADDRESS)	&&
                    (numUses == 1)			) {
                ir_instruction_t	*dataStructDefInst;
                ir_item_t		*dataStruct;
                JITUINT32		numUsesOfDataStruct;

                /* Fetch the escaped data structure 								*/
                dataStruct		= IRMETHOD_getInstructionParameter1(defInst);

                /* Fetch the definition of this data structure that reaches the current instruction		*/
                dataStructDefInst	= NULL;
                if (IRDATA_isAVariable(dataStruct)) {
                    tempL	= IRMETHOD_getVariableDefinitionsThatReachInstruction(method, defInst, (dataStruct->value).v);
                    if (xanList_length(tempL) == 1) {
                        dataStructDefInst	= xanList_first(tempL)->data;
                        assert(dataStructDefInst != NULL);
                    }
                    xanList_destroyList(tempL);
                }

                /* Compute the number of uses of the data structure						*/
                numUsesOfDataStruct	= 0;
                if (IRDATA_isAVariable(dataStruct)) {
                    tempL			= IRMETHOD_getVariableUses(method, (dataStruct->value).v);
                    numUsesOfDataStruct	= xanList_length(tempL);
                    xanList_destroyList(tempL);
                }

                /* In case (1) there is a unique definition (2) it is a load instruction (3) and this defined variable has only one use, tag the triple of instructions as "to optimize"*/
                if (	(dataStructDefInst != NULL)		&&
                        (dataStructDefInst->type == IRLOADREL)	&&
                        (numUsesOfDataStruct == 0)		) {
                    memcpy_insts_t	*m;
                    m	= allocFunction(sizeof(memcpy_insts_t));
                    m->getaddrInst	= defInst;
                    m->loadInst	= dataStructDefInst;
                    m->memcpyInst	= inst;
                    xanList_insert(l, m);
                }
            }
        }

        /* Fetch the next element from the list								*/
        item		= item->next;
    }
    codeChanged	= (xanList_length(l) > 0);

    /* Apply the optimization									*/
    item		= xanList_first(l);
    while (item != NULL) {
        memcpy_insts_t		*m;
        ir_instruction_t	*convInst;
        ir_instruction_t	*addInst;
        JITUINT32		typeToConvert;

        /* Fetch the triple to optimize									*/
        m			= item->data;

        /* Convert the base address to the appropriate integer						*/
        typeToConvert		= IRMETHOD_getInstructionParameter2(m->loadInst)->internal_type;
        convInst		= IRMETHOD_newInstructionOfTypeAfter(method, m->loadInst, IRCONV);
        IRMETHOD_cpInstructionParameter(method, m->loadInst, 1, convInst, 1);
        IRMETHOD_setInstructionParameter2(method, convInst, typeToConvert, 0, IRTYPE, IRTYPE, NULL);
        IRMETHOD_setInstructionParameterWithANewVariable(method, convInst, typeToConvert, NULL, 0);

        /* Compute the effective address to use								*/
        addInst			= IRMETHOD_newInstructionOfTypeAfter(method, convInst, IRADD);
        IRMETHOD_cpInstructionParameter(method, convInst, 0, addInst, 1);
        IRMETHOD_cpInstructionParameter(method, m->loadInst, 2, addInst, 2);
        IRMETHOD_setInstructionParameterWithANewVariable(method, addInst, typeToConvert, NULL, 0);

        /* Convert the result to the appropriate type							*/
        typeToConvert		= IRMETHOD_getInstructionResult(m->getaddrInst)->internal_type;
        m->getaddrInst->type 	= IRCONV;
        IRMETHOD_cpInstructionParameter(method, addInst, 0, m->getaddrInst, 1);
        IRMETHOD_setInstructionParameter2(method, m->getaddrInst, typeToConvert, 0, IRTYPE, IRTYPE, NULL);

        /* Delete the useless load operation								*/
        IRMETHOD_deleteInstruction(method, m->loadInst);
        //fprintf(stderr, "XAN: %s\n", (char *)IRMETHOD_getSignatureInString(method));

        /* Fetch the next element									*/
        item			= item->next;
    }

    /* Free the memory										*/
    xanList_destroyList(memcpyInsts);
    xanList_destroyListAndData(l);

    return codeChanged ;
}
