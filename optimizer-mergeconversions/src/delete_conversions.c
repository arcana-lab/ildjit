/*
 * Copyright (C) 2014  Campanoni Simone
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
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_mergeconversions.h>
// End

typedef struct {
    ir_instruction_t *firstConvInst;
    ir_instruction_t *mathInst;
    ir_instruction_t *secondConvInst;
} useless_conv_t;

void delete_useless_conversions (ir_method_t *m){
    JITUINT32   instructionsNumber;
    XanList     *uselessConvs;
    XanListItem *item;

    /* Assertions.
     */
    assert(m != NULL);

    /* Compute the basic blocks.
     */
    IROPTIMIZER_callMethodOptimization(NULL, m, BASIC_BLOCK_IDENTIFIER);
    IROPTIMIZER_callMethodOptimization(NULL, m, LIVENESS_ANALYZER);

    /* Allocate the memory.
     */
    uselessConvs     = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the number of instructions.
     */
    instructionsNumber  = IRMETHOD_getInstructionsNumber(m);

    /* Delete useless conversions.
     */
    for (JITUINT32 instID=0; instID < instructionsNumber; instID++){
        ir_instruction_t    *i;
        ir_instruction_t    *iNext;
        ir_item_t           *varDefined;
        ir_item_t           *varUsed;
        IRBasicBlock        *bb;

        /* Fetch the next conversion.
         */
        i           = IRMETHOD_getInstructionAtPosition(m, instID);
        if (i->type != IRCONV){
            continue ;
        }

        /* Fetch the variable defined by the conversion.
         */
        varUsed  = IRMETHOD_getInstructionParameter1(i);
        varDefined  = IRMETHOD_getInstructionResult(i);

        /* Check if the variable defined is an integer value.
         */
        if (!IRMETHOD_isAnIntType(varDefined->internal_type)){
            continue ;
        }

        /* Check if the variable converted is an integer value.
         */
        if (!IRMETHOD_isAnIntType(varUsed->internal_type)){
            continue ;
        }

        /* Check if the two types are of the same size.
         */
        if (IRDATA_getSizeOfType(varUsed) != IRDATA_getSizeOfType(varDefined)){
            continue ;
        }
    
        /* Fetch the basic block that contains the instruction.
         */
        bb          = IRMETHOD_getBasicBlockContainingInstruction(m, i);
        assert(bb != NULL);

        /* Check if there is an instruction within the basic block that uses the variable defined by the conversion.
         */
        iNext       = IRMETHOD_getInstructionAtPosition(m, instID + 1);
        while (iNext != NULL){

            /* Check if the current subsequent instruction uses the variable defined by the conversion.
             */
            if (IRMETHOD_doesInstructionUseVariable(m, iNext, (varDefined->value).v)){

                /* Check if the current instruction is a mathematic instruction.
                 */
                if (IRMETHOD_isAMathInstruction(iNext)){

                    /* Check if the variable defined by the conversion instruction is dead after this one.
                     */
                    if (!IRMETHOD_isVariableLiveOUT(m, iNext, (varDefined->value).v)){
                        ir_item_t           *convertedVarDefined;
                        ir_instruction_t    *iNextConv;

                        /* Fetch the converted variable defined.
                         */
                        convertedVarDefined = IRMETHOD_getInstructionResult(iNext);

                        /* Search for the instruction that convert back the new variable.
                         */
                        iNextConv   = IRMETHOD_getNextInstructionWithinBasicBlock(m, bb, iNext);
                        while (iNextConv != NULL){
                            if (iNextConv->type == IRCONV){
                                if (IRMETHOD_doesInstructionUseVariable(m, iNextConv, (convertedVarDefined->value).v)){

                                    /* Check if the current conversion defines the original variable.
                                     */
                                    if (IRMETHOD_doesInstructionDefineVariable(m, iNextConv, (varUsed->value).v)){

                                        /* Check that the converted variable redefined by the math instruction is dead after the last conversion.
                                         */
                                        if (!IRMETHOD_isVariableLiveOUT(m, iNextConv, (convertedVarDefined->value).v)){
                                            useless_conv_t  *c;

                                            /* Tag the conversion as useless.
                                             */
                                            c   = allocFunction(sizeof(useless_conv_t));
                                            c->firstConvInst    = i;
                                            c->mathInst         = iNext;
                                            c->secondConvInst   = iNextConv;
                                            xanList_append(uselessConvs, c);
                                            break ;
                                        }
                                    }
                                }

                            } else {

                                /* Check whether the instruction uses the value generated by the math.
                                 * In this case, we cannot apply the optimization for the current conversion.
                                 */
                                if (IRMETHOD_doesInstructionUseVariable(m, iNextConv, (convertedVarDefined->value).v)){
                                    break ;
                                }
                            }
                            iNextConv   = IRMETHOD_getNextInstructionWithinBasicBlock(m, bb, iNextConv);
                        }
                    }
                }
            }

            /* Fetch the next instruction within the same basic block of the conversion.
             */
            iNext       = IRMETHOD_getNextInstructionWithinBasicBlock(m, bb, iNext);
        }
    }

    /* Apply all transformations.
     */
    item    = xanList_first(uselessConvs);
    while (item != NULL){
        useless_conv_t  *u;
        ir_item_t       *origVar;
        ir_item_t       *par1;
        ir_item_t       *par2;

        /* Fetch the useless conversions.
         */
        u       = item->data;
        assert(u != NULL);

        /* Fetch the variables.
         */
        origVar = IRMETHOD_getInstructionResult(u->firstConvInst);
        par1    = IRMETHOD_getInstructionParameter1(u->mathInst);
        par2    = IRMETHOD_getInstructionParameter2(u->mathInst);

        /* Delete useless conversions.
         * Notice that we do not delete the first conversion because it could generate value used by some instruction. 
         * If this is not the case, a subsequent dead-code elimination will delete it.
         */
        if ((par1->value).v == (origVar->value).v){
            IRMETHOD_cpInstructionParameter(m, u->firstConvInst, 1, u->mathInst, 1);
        }
        if ((par2->value).v == (origVar->value).v){
            IRMETHOD_cpInstructionParameter(m, u->firstConvInst, 1, u->mathInst, 2);
        }
        IRMETHOD_cpInstructionParameter(m, u->firstConvInst, 1, u->mathInst, 0);
        IRMETHOD_deleteInstruction(m, u->secondConvInst);

        /* Fetch the next element.
         */
        item    = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyListAndData(uselessConvs);

    return ;
}
