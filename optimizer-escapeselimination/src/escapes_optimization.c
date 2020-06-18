/*
 * Copyright (C) 2009 - 2010  Campanoni Simone
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
#include <ir_method.h>
#include <compiler_memory_manager.h>
#include <ir_optimization_interface.h>

// My headers
#include <optimizer_escapeselimination.h>
#include <misc.h>
// End

static inline void internal_reduce_getaddress_instructions (ir_method_t *method);
static inline void optimize_escape (ir_method_t *method, ir_instruction_t *getAddress);
static inline JITINT32 optimization_is_safe (XanList *uses);
static inline JITINT32 have_the_same_reched_definition (ir_method_t *method, XanList *uses, ir_instruction_t *def, IR_ITEM_VALUE def_var);
static inline void internal_optimize_init_memory_instructions (ir_method_t *method);
static inline void internal_optimize_memcpy_instructions (ir_method_t *method);
static inline JITBOOLEAN internal_escaped_type_safe_to_optimize (JITUINT32 type, TypeDescriptor *ilType);

extern ir_lib_t *irLib;
extern ir_optimizer_t *irOptimizer;

void optimize_escapes (ir_method_t *method) {
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    ir_instruction_t        *inst;

    /* Assertions.
     */
    assert(method != NULL);
    PDEBUG("OPTIMIZE ESCAPES: Start\n");
    PDEBUG("OPTIMIZE ESCAPES: 	%s\n", (char *)IRMETHOD_getSignatureInString(method));

    /* Fetch the instructions number.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Optimize the escaped         *
     * variables			*/
    for (count = 0; count < instructionsNumber; count++) {
        JITBOOLEAN safe;

        /* Fetch the instruction			*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check the instruction			*/
        if (inst->type != IRGETADDRESS) {
            continue;
        }
        assert((inst->param_1).type == IROFFSET);
        PDEBUG("OPTIMIZE ESCAPES:       Check the instruction %d\n", inst->ID);

        /* Check that the variable is not a value type	*/
        safe = internal_escaped_type_safe_to_optimize((inst->param_1).internal_type, (inst->param_1).type_infos);

        /* Check if the instruction produces an		*
         * escaped variable				*/
        if (safe) {

            /* Apply the code optimization			*/
            PDEBUG("OPTIMIZE ESCAPES:       Try to apply the code transformation\n");
            optimize_escape(method, inst);
        }
    }

    /* Check if the required analyses are available in the system.
     */
    if (IROPTIMIZER_canCodeToolBeUsed(irOptimizer, PRE_DOMINATOR_COMPUTER)) {

        /* Optimize the IRINITMEMORY instructions.
         */
        internal_optimize_init_memory_instructions(method);

        /* Optimize the IRMEMCPY instructions.
         */
        internal_optimize_memcpy_instructions(method);
    }

    /* Reduce the number of instructions used to take the address of the escaped variables.
     */
    internal_reduce_getaddress_instructions(method);

    PDEBUG("OPTIMIZE ESCAPES: Exit\n");
    return;
}

static inline void internal_reduce_getaddress_instructions (ir_method_t *method){
    XanList             *getAddresses;
    XanList             *toDelete;
    XanListItem         *item;
    XanHashTable        *gts;
    ir_instruction_t    *firstInst;
    IRBasicBlock        *firstBB;

    /* Assertions.
     */
    assert(method != NULL);

    /* Identify the basic blocks.
     */
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);

    /* Allocate the necessary memories.
     */
    gts             = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    toDelete        = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch where to bring IRGETADDRESS instructions.
     */
    firstInst       = IRMETHOD_getFirstInstructionNotOfType(method, IRNOP);
    firstBB         = IRMETHOD_getBasicBlockContainingInstruction(method, firstInst);

    /* Fetch the set of instructions of type IRGETADDRESS.
     */
    getAddresses    = IRMETHOD_getInstructionsOfType(method, IRGETADDRESS);
    assert(getAddresses != NULL);

    /* Remove instructions already at the top of the method.
     */
    item            = xanList_first(getAddresses);
    while (item != NULL){
        ir_instruction_t    *gt;
        IRBasicBlock        *gtBB;

        /* Fetch an IRGETADDRESS instruction.
         */
        gt              = item->data;
        assert(gt != NULL);
        gtBB            = IRMETHOD_getBasicBlockContainingInstruction(method, gt);

        /* Check if the current instruction is already at the beginning of the method.
         */
        if (firstBB == gtBB){
            xanList_insert(toDelete, gt);
        }

        /* Fetch the next element.
         */
        item            = item->next;
    }
    xanList_removeElements(getAddresses, toDelete, JITTRUE);

    /* Bring instructions at the beginning of the method.
     */
    item            = xanList_first(getAddresses);
    while (item != NULL){
        ir_instruction_t    *gt;
        ir_instruction_t    *cloneGt;
        ir_item_t           *par1;
        ir_item_t           *res;

        /* Fetch an IRGETADDRESS instruction.
         */
        gt              = item->data;
        assert(gt != NULL);
        par1            = IRMETHOD_getInstructionParameter1(gt);
        res             = IRMETHOD_getInstructionResult(gt);

        /* Check if it is the first instruction that takes the address of a variable that has not been considered yet.
         */
        cloneGt         = xanHashTable_lookup(gts, (void *)(JITNUINT)((par1->value).v + 1));
        if (cloneGt == NULL){

            /* Clone the current IRGETADDRESS instruction and bring it at the beginning of the method.
             */
            cloneGt     = IRMETHOD_cloneInstructionAndInsertBefore(method, gt, firstInst);
            IRMETHOD_setInstructionParameterWithANewVariable(method, cloneGt, res->internal_type, res->type_infos, 0);

            /* Register the mapping between variables.
             */
            xanHashTable_insert(gts, (void *)(JITNUINT)((par1->value).v + 1), cloneGt);
        }

        /* Fetch the next element.
         */
        item            = item->next;
    }

    /* Adjust all the old IRGETADDRESS instructions.
     */
    item            = xanList_first(getAddresses);
    while (item != NULL){
        ir_instruction_t    *gt;
        ir_instruction_t    *cloneGt;
        ir_item_t           par1;

        /* Fetch an IRGETADDRESS instruction.
         */
        gt              = item->data;
        assert(gt != NULL);
        memcpy(&par1, IRMETHOD_getInstructionParameter1(gt), sizeof(ir_item_t));

        /* Fetch the variable that holds the address.
         */
        cloneGt         = xanHashTable_lookup(gts, (void *)(JITNUINT)(par1.value.v + 1));
        if (cloneGt != NULL){

            /* Adjust the instruction.
             */
            gt->type        = IRMOVE;
            IRMETHOD_cpInstructionParameter(method, cloneGt, 0, gt, 1);
        }

         /* Fetch the next element.
         */
        item            = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(getAddresses);
    xanList_destroyList(toDelete);
    xanHashTable_destroyTable(gts);

    return ;
}

static inline JITBOOLEAN internal_escaped_type_safe_to_optimize (JITUINT32 type, TypeDescriptor *ilType) {
    if (	(IRMETHOD_isAnIntType(type))	||
            (IRMETHOD_isAFloatType(type))	) {
        if (ilType == NULL) {
            return JITTRUE;
        }
        return ilType->getIRType(ilType) != IRVALUETYPE;
    }

    return JITFALSE;
}

static inline void internal_optimize_memcpy_instructions (ir_method_t *method) {
    JITUINT32 		count;
    JITUINT32 		instructionsNumber;
    ir_instruction_t        *inst;

    /* Assertions.
     */
    assert(method != NULL);

    /* Compute method information	*
     * needed by the following	*
     * optimization			*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, REACHING_DEFINITIONS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, PRE_DOMINATOR_COMPUTER);

    /* Fetch the instructions number.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Optimize the escaped variables.
     */
    for (count = 0; count < instructionsNumber; count++) {
        XanList         *defs;
        IR_ITEM_VALUE 	varID;
        JITUINT32	defsNum;
        ir_item_t	varToUse;
        ir_item_t	*par2;
        XanListItem	*item;

        /* Fetch the instruction.
         */
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check the instruction type.
         */
        if (IRMETHOD_getInstructionType(inst) != IRMEMCPY) {
            continue;
        }

        /* Check if the memory addresses are stored in variables.
         */
        if (	(!IRMETHOD_isInstructionParameterAVariable(inst, 1))	||
                (!IRMETHOD_isInstructionParameterAVariable(inst, 2))	) {
            continue;
        }

        /* Check if the amount of memory pointed by the pointer cannot be a builtin type.
         */
        if (	IRMETHOD_isInstructionParameterAConstant(inst, 3)			&&
                IRMETHOD_getInstructionParameter3Value(inst) > sizeof(JITUINT64)	) {
            continue;
        }

        /* Fetch the variable, which contains the memory where to read the value to store in memory.
         */
        varID	= IRMETHOD_getInstructionParameter2Value(inst);

        /* Fetch the definitions of the variable that contain the address.
         */
        defs 		= IRMETHOD_getVariableDefinitions(method, varID);
        assert(defs != NULL);
        defsNum 	= xanList_length(defs);

        /* Check if the address points to an escaped variable that is not a structure.
         */
        item	= xanList_first(defs);
        while (item != NULL) {
            ir_instruction_t	*def;
            ir_item_t		*par1;
            def	= item->data;
            if (def->type != IRGETADDRESS) {
                break ;
            }
            par1	= IRMETHOD_getInstructionParameter1(def);
            if (par1->internal_type == IRVALUETYPE) {
                break;
            }
            memcpy(&varToUse, par1, sizeof(ir_item_t));
            item		= item->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(defs);

        /* Check if it is safe to apply the transformation.
         */
        if (	(item != NULL)		||
                (defsNum != 1)		) {
            continue ;
        }

        /* Apply the transformation.
         * The instruction
         *       ...
         *       src = &var;
         *       ...
         * inst) memcpy(dest, src, bytesNumber)
         *
         * is transformed to
         *       ...
         * inst) store_rel(dest, 0, var)
         */
        IRMETHOD_setInstructionType(method, inst, IRSTOREREL);
        par2			= IRMETHOD_getInstructionParameter2(inst);
        memset(par2, 0, sizeof(ir_item_t));
        par2->internal_type	= IRINT32;
        par2->type		= par2->internal_type;
        IRMETHOD_cpInstructionParameter3(inst, &varToUse);
    }

    return ;
}

static inline void internal_optimize_init_memory_instructions (ir_method_t *method) {
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    ir_instruction_t        *inst;

    /* Assertions			*/
    assert(method != NULL);

    /* Compute method information	*
     * needed by the following	*
     * optimization			*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, REACHING_DEFINITIONS_ANALYZER);
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, PRE_DOMINATOR_COMPUTER);

    /* Fetch the instructions number*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Optimize the escaped         *
     * variables			*/
    for (count = 0; count < instructionsNumber; count++) {
        XanList         *uses;
        XanList         *defs;
        IR_ITEM_VALUE varID;
        JITUINT32 defsNum;
        JITBOOLEAN onlyUse;

        /* Fetch the instruction			*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check the instruction			*/
        if (	(IRMETHOD_getInstructionType(inst) != IRINITMEMORY) 	&&
                (IRMETHOD_getInstructionType(inst) != IRMEMCPY) 	) {
            continue;
        }

        /* Check if the memory address is stored in a	*
         * variable					*/
        if (!IRMETHOD_isInstructionParameterAVariable(inst, 1)) {

            /* The memory address is a constant		*/
            continue;
        }

        /* Check if the amount of memory pointed by the pointer cannot be a builtin type.
         */
        if (	IRMETHOD_isInstructionParameterAConstant(inst, 3)			&&
                IRMETHOD_getInstructionParameter3Value(inst) > sizeof(JITUINT64)	) {
            continue;
        }

        /* Fetch the variable, which contains the memory*
         * address					*/
        varID = IRMETHOD_getInstructionParameter1Value(inst);

        /* Check that the memory address is only used by*
         * the current instruction or every other uses	*
         * can be executed only after the current one	*/
        onlyUse = JITFALSE;
        uses = IRMETHOD_getVariableUses(method, varID);
        assert(uses != NULL);
        assert(xanList_length(uses) >= 1);
        if (xanList_length(uses) > 1) {
            XanListItem *item;
            onlyUse = JITTRUE;
            item = xanList_first(uses);
            while (item != NULL) {
                ir_instruction_t *currentUse;
                currentUse = item->data;
                if (!IRMETHOD_isInstructionAPredominator(method, inst, currentUse)) {
                    onlyUse = JITFALSE;
                    break;
                }
                item = item->next;
            }
        } else {
            onlyUse = JITTRUE;
        }
        xanList_destroyList(uses);
        if (!onlyUse) {
            continue;
        }
        assert(IRMETHOD_isInstructionParameterAVariable(inst, 1));

        /* Check that there is only one definition of	*
         * the memory address, which is from the        *
         * IRGETADDRESS we are considering		*/
        defs = IRMETHOD_getVariableDefinitions(method, varID);
        assert(defs != NULL);
        defsNum = xanList_length(defs);

        /* Check how many definitions there are for the	variable*
        * that keeps the memory address                        */
        if (defsNum == 1) {
            ir_instruction_t        *def;
            ir_item_t		*firstPar;

            /* Fetch the single definition			*/
            def = xanList_first(defs)->data;
            assert(def != NULL);

            /* Fetch the first parameter			*/
            firstPar = IRMETHOD_getInstructionParameter1(def);

            /* Check the type of the instruction		*/
            if (    (def->type == IRGETADDRESS)                                                     		&&
                    (internal_escaped_type_safe_to_optimize(firstPar->internal_type, firstPar->type_infos))      	) {
                ir_item_t 	startValue;
                ir_item_t       *par1;

                /* We can remove the IRINITMEMORY instruction	*
                 * by transforming it to a IRMOVE		*/
                par1 = IRMETHOD_getInstructionParameter1(def);
                memcpy(&startValue, IRMETHOD_getInstructionParameter2(inst), sizeof(ir_item_t));
                IRMETHOD_eraseInstructionFields(method, inst);
                IRMETHOD_setInstructionType(method, inst, IRMOVE);
                IRMETHOD_cpInstructionResult(inst, par1);
                IRMETHOD_cpInstructionParameter1(inst, &startValue);
            }
        }

        /* Free the memory				*/
        xanList_destroyList(defs);
    }

    /* Return			*/
    return;
}

static inline void optimize_escape (ir_method_t *method, ir_instruction_t *getAddress) {
    IR_ITEM_VALUE variablePointer;
    XanList                 *uses;
    XanList                 *variablesPointer;
    XanListItem             *item;
    ir_instruction_t        *inst;
    JITBOOLEAN 		modified;

    /* Assertions.
     */
    assert(method != NULL);
    assert(getAddress != NULL);
    assert(getAddress->type == IRGETADDRESS);

    /* Allocate the list of variables that store the address of the escaped	variable.
     */
    variablesPointer = xanList_new(allocFunction, freeFunction, NULL);
    assert(variablesPointer != NULL);

    /* Fetch the variable defined by the getaddree instruction.
     */
    variablePointer = IRMETHOD_getInstructionResult(getAddress)->value.v;
    assert(IRDATA_isAVariable(IRMETHOD_getInstructionResult(getAddress)));

    /* Fetch the set of instructions that uses the pointer of the variable escaped.
     */
    xanList_append(variablesPointer, (void *) (JITNUINT) (variablePointer + 1));
    uses = IRMETHOD_getVariableUses(method, variablePointer);
    assert(uses != NULL);

    /* Add to the uses the instruction that	use the renamed variables of the pointer we are considering.
     */
    do {
        modified = JITFALSE;
        item = xanList_first(uses);
        while (item != NULL) {
            ir_instruction_t        *inst;

            /* Fetch the instruction.
             */
            inst = item->data;
            assert(inst != NULL);

            /* Check to see if the instruction renames the variable that store the pointer we are considering.
             */
            if (inst->type == IRMOVE) {
                XanList         *moveVarDefs;
                XanList         *moveUses;
                XanListItem     *item2;
                JITUINT32       moveVarDefsLen;
                IR_ITEM_VALUE 	moveVar;

                /* Fetch the new name of the variable.
                 */
                moveVar = IRMETHOD_getVariableDefinedByInstruction(method, inst);
                assert(moveVar != NOPARAM);

                /* Check if the new variable escapes.
                 * Currently, we do not handle the case the moved variable escapes as well as the original one.
                 * In the future, this can be improved.
                 */
                if (IRMETHOD_isAnEscapedVariable(method, moveVar)) {
                    break ;
                }

                /* Make sure that this is the unique definition of the renamed variable.
                 * In the future, this can be improved as this check is conservative.
                 */
                moveVarDefs = IRMETHOD_getVariableDefinitions(method, moveVar);
                moveVarDefsLen  = xanList_length(moveVarDefs);
                xanList_destroyList(moveVarDefs);
                if (moveVarDefsLen > 1) {
                    break ;
                }

                /* Check to see if the current variable	has been already considered.
                 */
                if (xanList_find(variablesPointer, (void *) (JITNUINT) (moveVar + 1)) == NULL) {

                    /* Add the new variable.
                     */
                    modified = JITTRUE;
                    xanList_append(variablesPointer, (void *) (JITNUINT) (moveVar + 1));

                    /* Get the uses of the new variable	discovered.
                     */
                    moveUses = IRMETHOD_getVariableUses(method, moveVar);
                    assert(moveUses != NULL);
                    item2 = xanList_first(moveUses);
                    while (item2 != NULL) {
                        ir_instruction_t        *inst2;
                        inst2 = item2->data;
                        assert(inst2 != NULL);
                        if (xanList_find(uses, inst2) == NULL) {
                            xanList_append(uses, inst2);
                        }
                        item2 = item2->next;
                    }
                    xanList_destroyList(moveUses);
                }
            }

            /* Fetch the next element from the list.
             */
            item = item->next;
        }
        if (item != NULL) {
            break ;
        }
    } while (modified);

    /* Check if it is safe to optimize the current escaped variable.
     */
    if (item == NULL) {

        /* Optimize.
         */
        if (have_the_same_reched_definition(method, uses, getAddress, variablePointer)) {
            ir_item_t varToStore;
            ir_item_t varToDefine;
            ir_item_t escapedVariable;
            ir_item_t ptrEscapedVariable;
            PDEBUG("OPTIMIZE ESCAPES:               Uses and definition satisfy the condition\n");
            memcpy(&escapedVariable, IRMETHOD_getInstructionParameter1(getAddress), sizeof(ir_item_t));
            memcpy(&ptrEscapedVariable, IRMETHOD_getInstructionResult(getAddress), sizeof(ir_item_t));
            item = xanList_first(uses);
            while (item != NULL) {
                inst = item->data;
                assert(inst != NULL);
                PDEBUG("OPTIMIZE ESCAPES:                       Instruction that uses the escaped variable = %d\n", inst->ID);
                if (    ((inst->param_2).type != IROFFSET)                                                              &&
                        ((inst->param_2).value.v == 0)                                                                  &&
                        ((inst->param_1).type == IROFFSET)                                                              &&
                        (xanList_find(variablesPointer, (void *) (JITNUINT) ((inst->param_1).value.v + 1)) != NULL)     ) {
                    PDEBUG("OPTIMIZE ESCAPES:                       Apply the optimization\n");
                    switch (inst->type) {
                        case IRSTOREREL:
                            memcpy(&varToStore, IRMETHOD_getInstructionParameter3(inst), sizeof(ir_item_t));
                            IRMETHOD_eraseInstructionFields(method, inst);
                            IRMETHOD_setInstructionType(method, inst, IRMOVE);
                            IRMETHOD_cpInstructionParameter1(inst, &varToStore);
                            IRMETHOD_cpInstructionResult(inst, &escapedVariable);
                            break;
                        case IRLOADREL:
                            memcpy(&varToDefine, IRMETHOD_getInstructionResult(inst), sizeof(ir_item_t));
                            IRMETHOD_eraseInstructionFields(method, inst);
                            if (escapedVariable.internal_type != varToDefine.internal_type) {
                                IRMETHOD_setInstructionType(method, inst, IRBITCAST);
                                IRMETHOD_cpInstructionParameter1(inst, &escapedVariable);
                                IRMETHOD_cpInstructionResult(inst, &varToDefine);

                            } else {
                                IRMETHOD_setInstructionType(method, inst, IRMOVE);
                                IRMETHOD_cpInstructionParameter1(inst, &escapedVariable);
                                IRMETHOD_cpInstructionResult(inst, &varToDefine);
                            }
                            break;
                    }
                }
                item = item->next;
            }
        }
    }

    /* Free the memory.
     */
    xanList_destroyList(uses);
    xanList_destroyList(variablesPointer);

    return ;
}

static inline JITINT32 have_the_same_reched_definition (ir_method_t *method, XanList *uses, ir_instruction_t *def, IR_ITEM_VALUE def_var) {
    XanListItem             *item;
    ir_instruction_t        *inst;
    JITINT32 found;
    XanHashTable            *defsTable;

    /* Allocate the table that will contains the definitions.
     */
    defsTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(defsTable != NULL);

    /* Check each use.
     */
    found = JITFALSE;
    item = xanList_first(uses);
    while (item != NULL) {
        inst = item->data;
        if (internal_getSingleDefinitionReachedIn(method, def_var, inst, defsTable) != def) {
            found = JITTRUE;
            break;
        }
        item = item->next;
    }

    /* Free the memory.
     */
    internal_destroyDefsTable(defsTable);

    return !found;
}

static inline JITINT32 optimization_is_safe (XanList *uses) {
    XanListItem             *item;
    ir_instruction_t        *inst;

    item = xanList_first(uses);
    while (item != NULL) {
        inst = item->data;
        switch (inst->type) {
            case IRLOADREL:
            case IRSTOREREL:
                if (    ((inst->param_2).type == IROFFSET)      ||
                        ((inst->param_2).value.v != 0)          ) {
                    return JITFALSE;
                }
                break;
            case IRCALL:
            case IRLIBRARYCALL:
            case IRNATIVECALL:
            case IRICALL:
            case IRVCALL:
                return JITFALSE;
            default:
                return JITFALSE;
        }
        item = item->next;
    }

    return JITTRUE;
}
