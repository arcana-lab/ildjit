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
#include <stdio.h>
#include <assert.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_deadcode.h>
#include <instructions_cleaning.h>
// End

static inline void internal_deleteUnusefulConversion (ir_method_t *method, XanHashTable *uselessConvs);
static inline void print_sets (ir_method_t *method);
static inline void internal_deleteCodeUnreachable (ir_method_t *method);
static inline JITINT32 internal_isInstructionMarked (JITNUINT *instsMarked, JITUINT32 instsMarkedLen, JITUINT32 instID);
static inline void internal_markInstruction (JITNUINT *instsMarked, JITUINT32 instsMarkedLen, JITUINT32 instID);
static inline JITNINT internal_hasSideEffect (ir_method_t *method, ir_instruction_t *inst);
static inline void internal_deleteUnusefulBranches (ir_method_t *method);
static inline void internal_deleteUnusefulLabels (ir_method_t *method);
static inline void internal_mergeLabels (ir_method_t *method);
static inline void internal_set_type (ir_method_t *method, ir_item_t *par);
static inline JITBOOLEAN internal_set_pointer_type (ir_item_t *par);
static inline JITBOOLEAN internal_set_int_type (ir_item_t *par);
static inline JITBOOLEAN internal_set_uint_type (ir_item_t *par);
static inline void internal_set_float_type (ir_item_t *par);
static inline ir_instruction_t * internal_getSingleDefinitionReachedIn (ir_method_t *method, IR_ITEM_VALUE varID, ir_instruction_t *instToCheck, XanHashTable *defsTable);
static inline void internal_deleteUnusefulIntConversion (ir_method_t *method);
static inline void internal_deleteUnusefulPointerConversion (ir_method_t *method);
static inline void internal_deleteUnusefulBoolToIntConversion (ir_method_t *method, XanHashTable *uselessConvs);
static inline void internal_deleteUnusefulFloatConversion (ir_method_t *method);
static inline void internal_convertNativeTypes (ir_method_t *method);
static inline XanList * internal_getDefinitions (ir_method_t *method, IR_ITEM_VALUE varID, XanHashTable *defsTable);
static inline void internal_removeCatchBlocks (ir_method_t *method);
static inline void internal_deleteUnusefulStores (ir_method_t *method);
static inline XanHashTable * internal_fetchUselessBoolToIntConversion (ir_method_t *method);

extern ir_optimizer_t  *irOptimizer;

void eliminate_deadcode_within_method (ir_method_t *method) {
    JITUINT32 instID;
    JITUINT32 limit;
    JITBOOLEAN toDelete;
    ir_instruction_t        *opt_inst;
    XanHashTable		*uselessConvs;

    /* Assertions.
     */
    assert(method != NULL);
    PDEBUG("OPTIMIZER_DEADCODE: Begin to eliminate the dead-codes\n");

    /* Initialize the variables.
     */
    instID = 0;
    opt_inst = NULL;
    toDelete = 0;

    /* Check if there are instructions.
     */
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return ;
    }

    /* Find useless converstion instructions that can be optimized.
     */
    uselessConvs	= internal_fetchUselessBoolToIntConversion(method);

    /* Set the position limit the last instruction of the method.
     */
    limit = IRMETHOD_getInstructionsNumber(method);

    /* Delete dead code			*/
    for (instID = IRMETHOD_getMethodParametersNumber(method); instID < limit; instID++) {
        PDEBUG("OPTIMIZER_DEADCODE:     Inst %u\n", inst->ID);

        /* Fetch the instruction		*/
        opt_inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(opt_inst != NULL);

        /* Initialize the variable		*/
        toDelete = JITFALSE;

        /* Check the type			*/
        if (opt_inst->type == IRNOP) {
            toDelete = JITTRUE;
        } else if (livenessGetDef(method, opt_inst) != -1) {

            /* The instruction define a variable	*
             * so I have to check if this variable	*
             * is live out				*/
            if (IRMETHOD_isVariableLiveOUT(method, opt_inst, livenessGetDef(method, opt_inst))) {
                PDEBUG("OPTIMIZER_DEADCODE:             Variable defined is live out\n");

            } else if (IRMETHOD_isAnEscapedVariable(method, livenessGetDef(method, opt_inst))) {
                PDEBUG("OPTIMIZER_DEADCODE:             Variable defined is an escaped variable\n");

            } else {
                PDEBUG("OPTIMIZER_DEADCODE:             Variable defined is not live out, so I can delete this instruction\n");
                toDelete = JITTRUE;
            }
        } else {
            PDEBUG("OPTIMIZER_DEADCODE:             This instruction not define some variable\n");
        }

        if (    (toDelete)                                      &&
                (!internal_hasSideEffect(method, opt_inst))     ) {
            IRMETHOD_deleteInstruction(method, opt_inst);
            instID--;
            limit--;
        }
    }

    /* Delete the code unreachable							*/
    internal_deleteCodeUnreachable(method);

    /* Delete catch blocks of removed try ones					*/
    internal_removeCatchBlocks(method);

    /* Delete unuseful branches							*/
    internal_deleteUnusefulBranches(method);

    /* Delete unuseful labels							*/
    internal_mergeLabels(method);
    internal_deleteUnusefulLabels(method);

    /* Delete unuseful stores							*/
    internal_deleteUnusefulStores(method);

    /* Delete unuseful conversions          					*/
    internal_deleteUnusefulConversion(method, uselessConvs);

    return ;
}

static inline void internal_deleteUnusefulConversion (ir_method_t *method, XanHashTable *uselessConvs) {

    /* Assertions				*/
    assert(method != NULL);

    /* Convert native types			*/
    internal_convertNativeTypes(method);

    /* Delete the unusefull conversions	*/
    internal_deleteUnusefulFloatConversion(method);
    internal_deleteUnusefulBoolToIntConversion(method, uselessConvs);
    internal_deleteUnusefulIntConversion(method);
    internal_deleteUnusefulPointerConversion(method);

    /* Return				*/
    return;
}

static inline void internal_convertNativeTypes (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;
    ir_item_t               *par1;
    ir_item_t               *par2;
    ir_item_t               *par3;
    ir_item_t               *res;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Set the types                        */
    for (count = 0; count < instructionsNumber; count++) {

        /* Fetch the instruction		*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Fetch the parameters			*/
        par1 = IRMETHOD_getInstructionParameter1(inst);
        assert(par1 != NULL);
        par2 = IRMETHOD_getInstructionParameter2(inst);
        assert(par2 != NULL);
        par3 = IRMETHOD_getInstructionParameter3(inst);
        assert(par3 != NULL);

        /* Convert the parameters		*/
        internal_set_type(method, par1);
        internal_set_type(method, par2);
        internal_set_type(method, par3);

        /* Convert the call parameters		*/
        if (inst->callParameters != NULL) {
            XanListItem     *item;
            item =xanList_first(inst->callParameters);
            while (item != NULL) {
                ir_item_t       *callPar;
                callPar = item->data;
                assert(callPar != NULL);
                internal_set_type(method, callPar);
                item = item->next;
            }
        }

        /* Convert the result			*/
        res = IRMETHOD_getInstructionResult(inst);
        assert(res != NULL);
        internal_set_type(method, res);

        if (inst->type == IRCONV) {
            if ((par2->value).v == IRNUINT) {
                if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
                    (par2->value).v = IRUINT32;
                } else {
                    (par2->value).v = IRUINT64;
                }
            } else if ((par2->value).v == IRNINT) {
                if (IRDATA_getSizeOfIRType(IRNINT) == 4) {
                    (par2->value).v = IRINT32;
                } else {
                    (par2->value).v = IRINT64;
                }
            } else if ((par2->value).v == IRNFLOAT) {
                if (IRDATA_getSizeOfIRType(IRNFLOAT) == 4) {
                    (par2->value).v = IRFLOAT32;
                } else {
                    (par2->value).v = IRFLOAT64;
                }
            }
        }
    }

    /* Return				*/
    return;
}

static inline void internal_deleteUnusefulPointerConversion (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;
    ir_item_t               *par1;
    ir_item_t               *par2;
    JITUINT32 		instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Delete the unuseful conversion       */
    for (count = 0; count < instructionsNumber; count++) {

        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (inst->type == IRCONV) {
            par1 = IRMETHOD_getInstructionParameter1(inst);
            assert(par1 != NULL);
            par2 = IRMETHOD_getInstructionParameter2(inst);
            assert(par2 != NULL);
            if (    (par1->internal_type == IRMPOINTER)     &&
                    ((par2->value).v == IROBJECT)           ) {
                inst->type = IRMOVE;
                IRMETHOD_eraseInstructionParameter(method, inst, 2);
                IRMETHOD_eraseInstructionParameter(method, inst, 3);
            }
        }
    }

    /* Return				*/
    return;
}

static inline void internal_deleteUnusefulIntConversion (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;
    ir_item_t               *par1;
    ir_item_t               *par2;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Delete the unuseful conversion       */
    for (count = 0; count < instructionsNumber; count++) {

        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (inst->type == IRCONV) {
            par1 = IRMETHOD_getInstructionParameter1(inst);
            assert(par1 != NULL);
            par2 = IRMETHOD_getInstructionParameter2(inst);
            assert(par2 != NULL);
            if (par1->internal_type == (par2->value).v) {
                inst->type = IRMOVE;
                IRMETHOD_eraseInstructionParameter(method, inst, 2);
                IRMETHOD_eraseInstructionParameter(method, inst, 3);
            }
        }
    }

    /* Return				*/
    return;
}

static inline void internal_deleteUnusefulBoolToIntConversion (ir_method_t *method, XanHashTable *uselessConvs) {
    XanHashTableItem	*item;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if there are any conversions to be removed.
     */
    if (uselessConvs == NULL) {
        return ;
    }

    /* Optimize the code.
     */
    item	= xanHashTable_first(uselessConvs);
    while (item != NULL) {
        ir_instruction_t	*inst;
        ir_instruction_t	*convInst;

        /* Fetch the instructions.
         */
        inst		= item->elementID;
        convInst	= item->element;
        assert(inst != NULL);
        assert(convInst != NULL);

        /* Apply the optimization.
         */
        IRMETHOD_cpInstructionParameter1(inst, IRMETHOD_getInstructionParameter1(convInst));

        /* Fetch the next element from the list.
         */
        item	= xanHashTable_next(uselessConvs, item);
    }

    /* Free the memory.
     */
    xanHashTable_destroyTable(uselessConvs);

    return ;
}

static inline XanHashTable * internal_fetchUselessBoolToIntConversion (ir_method_t *method) {
    JITINT32 instsNumber;
    ir_instruction_t        *inst;
    ir_instruction_t        *convInst;
    JITUINT32 instType;
    JITINT32 count;
    XanHashTable            *defsTable;
    XanHashTable		*uselessConvs;
    XanHashTableItem        *item;

    /* Allocate the table.
     */
    uselessConvs	= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Allocate the table that will contain the definitions.
     */
    defsTable = xanHashTable_new(11, 0, allocFunction, reallocFunction, freeFunction, NULL, NULL);

    /* Fetch the instructions number	*/
    instsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Search the conditional branches	*/
    for (count = 0; count < instsNumber; count++) {
        ir_item_t	*par1;

        /* Fetch the instruction	*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        instType = inst->type;

        /* Check the instruction	*/
        if (    (instType != IRBRANCHIF)        &&
                (instType != IRBRANCHIFNOT)     ) {
            continue;
        }

        /* Check if the variable is     *
         * lived out			*/
        if (IRMETHOD_isVariableLiveOUT(method, inst, IRMETHOD_getInstructionParameter1Value(inst))) {
            continue;
        }

        /* Fetch the conversion if      *
         * it exists.			*/
        convInst = internal_getSingleDefinitionReachedIn(method, IRMETHOD_getInstructionParameter1Value(inst), inst, defsTable);
        if (convInst == NULL) {
            continue;
        }

        /* Check the conversion		*/
        par1	= IRMETHOD_getInstructionParameter1(convInst);
        if (par1->internal_type != IRINT8) {
            continue;
        }

        /* Register the current conversion as useless.
         */
        xanHashTable_insert(uselessConvs, inst, convInst);
    }

    /* Free the memory			*/
    item = xanHashTable_first(defsTable);
    while (item != NULL) {
        XanList *l;
        l = (XanList *) item->element;
        assert(l != NULL);
        xanList_destroyList(l);
        item = xanHashTable_next(defsTable, item);
    }
    xanHashTable_destroyTable(defsTable);

    return uselessConvs;
}

static inline void internal_deleteUnusefulFloatConversion (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;
    ir_item_t               *par1;
    ir_item_t               *par2;
    ir_item_t               *par3;
    ir_item_t               *res;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Set the float type                   */
    for (count = 0; count < instructionsNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        par1 = IRMETHOD_getInstructionParameter1(inst);
        assert(par1 != NULL);
        par2 = IRMETHOD_getInstructionParameter2(inst);
        assert(par2 != NULL);
        par3 = IRMETHOD_getInstructionParameter3(inst);
        assert(par3 != NULL);
        res = IRMETHOD_getInstructionResult(inst);
        assert(res != NULL);
        internal_set_float_type(par1);
        internal_set_float_type(par2);
        internal_set_float_type(par3);
        internal_set_float_type(res);

        if (inst->type == IRCONV) {
            if ((par2->value).v == IRNFLOAT) {
                if (IRDATA_getSizeOfIRType(IRNFLOAT) == 4) {
                    (par2->value).v = IRFLOAT32;
                } else {
                    (par2->value).v = IRFLOAT64;
                }
            }
        }
    }

    /* Delete the unuseful conversion       */
    for (count = 0; count < instructionsNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (inst->type == IRCONV) {
            par1 = IRMETHOD_getInstructionParameter1(inst);
            assert(par1 != NULL);
            par2 = IRMETHOD_getInstructionParameter2(inst);
            assert(par2 != NULL);
            if (par1->internal_type == (par2->value).v) {
                inst->type = IRMOVE;
                IRMETHOD_eraseInstructionParameter(method, inst, 2);
                IRMETHOD_eraseInstructionParameter(method, inst, 3);
            }
        }
    }
}

static inline void internal_mergeLabels (ir_method_t *method) {
    XanList                 *list;
    XanListItem             *item;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check the instructions number	*/
    if (instructionsNumber == 0) {
        return;
    }

    /* Fetch the list of labels of the      *
     * method				*/
    list = IRMETHOD_getLabelInstructions(method);
    assert(list != NULL);

    /* Merge labels				*/
    item = xanList_first(list);
    while (item != NULL) {
        ir_instruction_t *label;
        ir_instruction_t *nextInst;

        /* Fetch the label			*/
        label = item->data;
        assert(label != NULL);
        assert(label->type == IRLABEL);

        /* Fetch the next instruction		*/
        nextInst = IRMETHOD_getNextInstruction(method, label);
        if (    (nextInst != NULL)              &&
                (nextInst->type == IRLABEL)     ) {

            /* Merge the two labels			*/
            IRMETHOD_substituteLabelInAllBranches(method, (label->param_1).value.v, (nextInst->param_1).value.v);
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(list);

    /* Return				*/
    return;
}

static inline void internal_deleteUnusefulLabels (ir_method_t *method) {
    JITUINT32 instID;
    ir_instruction_t        *inst;
    XanList                 *list;
    XanListItem             *item;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check the instructions number	*/
    if (instructionsNumber == 0) {
        return;
    }

    /* Compute the list of labels of the    *
     * method				*/
    list = IRMETHOD_getLabelInstructions(method);
    assert(list != NULL);

    /* Compute the list of unuseful labels	*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *label;
        JITINT32 totalPar;
        JITINT32 parNumb;
        JITINT32 parValue;
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (inst->type == IRLABEL) {
            continue;
        }
        totalPar = IRMETHOD_getInstructionParametersNumber(inst);
        for (parNumb = 0; parNumb < totalPar; parNumb++) {
            parValue = -1;
            switch (parNumb) {
                case 2:
                    if (IRMETHOD_getInstructionParameter3Type(inst) != IRLABELITEM) {
                        break;
                    }
                    parValue = IRMETHOD_getInstructionParameter3Value(inst);
                    break;
                case 1:
                    if (IRMETHOD_getInstructionParameter2Type(inst) != IRLABELITEM) {
                        break;
                    }
                    parValue = IRMETHOD_getInstructionParameter2Value(inst);
                    break;
                case 0:
                    if (IRMETHOD_getInstructionParameter1Type(inst) != IRLABELITEM) {
                        break;
                    }
                    parValue = IRMETHOD_getInstructionParameter1Value(inst);
                    break;
                default:
                    abort();
            }
            if (parValue != -1) {
                label = IRMETHOD_getLabelInstruction(method, parValue);
                if (label != NULL) {
                    if (xanList_find(list, label) != NULL) {
                        xanList_removeElement(list, label, JITTRUE);
                    }
                }
                assert(xanList_find(list, label) == NULL);
            }
        }
    }

    /* Delete the branches			*/
    item = xanList_first(list);
    while (item != NULL) {
        inst = (ir_instruction_t *) item->data;
        assert(inst != NULL);
        assert(IRMETHOD_getInstructionType(inst) == IRLABEL);
        IRMETHOD_deleteInstruction(method, inst);
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(list);

    /* Return				*/
    return;
}

static inline void internal_deleteUnusefulStores (ir_method_t *method) {
    JITUINT32 		instID;
    ir_instruction_t        *inst;
    XanList                 *list;
    XanListItem             *item;

    /* Assertions				*/
    assert(method != NULL);

    /* Check the instructions number	*/
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return;
    }

    /* Compute the list of branches unuseful*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);
    for (instID = 0; instID < IRMETHOD_getInstructionsNumber(method); instID++) {
        ir_item_t	*param1;
        ir_item_t	*param2;
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (inst->type != IRMOVE) {
            continue;
        }
        param1	= IRMETHOD_getInstructionResult(inst);
        param2	= IRMETHOD_getInstructionParameter1(inst);
        if (	(param1->type == param2->type)			&&
                ((param1->value).v == (param2->value).v)	) {
            xanList_insert(list, inst);
        }
    }

    /* Delete the branches			*/
    item = xanList_first(list);
    while (item != NULL) {
        inst = (ir_instruction_t *) item->data;
        assert(inst != NULL);
        IRMETHOD_deleteInstruction(method, inst);
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(list);

    /* Return				*/
    return;
}

static inline void internal_deleteUnusefulBranches (ir_method_t *method) {
    JITUINT32 instID;
    ir_instruction_t        *inst;
    XanList                 *list;
    XanListItem             *item;

    /* Assertions				*/
    assert(method != NULL);

    /* Check the instructions number	*/
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return;
    }

    /* Compute the list of branches unuseful*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);
    for (instID = 0; instID < IRMETHOD_getInstructionsNumber(method); instID++) {
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (    (inst->type == IRBRANCH)                                &&
                (instID + 1 < IRMETHOD_getInstructionsNumber(method))   ) {
            ir_instruction_t        *inst2;
            inst2 = IRMETHOD_getInstructionAtPosition(method, instID + 1);
            assert(inst2 != NULL);
            if (    (inst2->type == IRLABEL)                                                &&
                    (IRMETHOD_getInstructionParameter1Value(inst2) == IRMETHOD_getInstructionParameter1Value(inst)) ) {
                xanList_insert(list, inst);
            }
        } else if (     (inst->type == IRBRANCHIF)      ||
                        (inst->type == IRBRANCHIFNOT)   ) {
            ir_instruction_t        *succ1;
            ir_instruction_t        *succ2;
            succ1 = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            assert(succ1 != NULL);
            succ2 = IRMETHOD_getSuccessorInstruction(method, inst, succ1);
            if (    (succ2 == NULL)         ||
                    (succ1 == succ2)        ) {
                xanList_insert(list, inst);
            }
        }
    }

    /* Delete the branches			*/
    item = xanList_first(list);
    while (item != NULL) {
        inst = (ir_instruction_t *) item->data;
        assert(inst != NULL);
        IRMETHOD_deleteInstruction(method, inst);
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(list);

    /* Return				*/
    return;
}

static inline void internal_deleteCodeUnreachable (ir_method_t *method) {
    XanList                 *instList;
    XanListItem             *item;
    JITNUINT                *instsMarked;
    JITUINT32 instsMarkedLen;
    JITUINT32 instID;
    JITUINT32 instructionsNumber;
    ir_instruction_t        *opt_inst;
    ir_instruction_t        *opt_inst2;

    /* Assertions				*/
    assert(method != NULL);

    /* Check the instructions number	*/
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return;
    }

    /* Mark the code reachable		*/
    if (IRMETHOD_getInstructionsNumber(method) % (sizeof(JITNUINT) * 8) > 0) {
        instsMarkedLen = (IRMETHOD_getInstructionsNumber(method) / (sizeof(JITNUINT) * 8)) + 1;
    } else {
        instsMarkedLen = (IRMETHOD_getInstructionsNumber(method) / (sizeof(JITNUINT) * 8));
    }
    instsMarked = allocFunction(instsMarkedLen * sizeof(JITNUINT));
    assert(instsMarked != NULL);
    memset(instsMarked, 0, instsMarkedLen * sizeof(JITNUINT));
    instList = xanList_new(allocFunction, freeFunction, NULL);
    assert(instList != NULL);
    opt_inst = IRMETHOD_getInstructionAtPosition(method, 0);
    instID = IRMETHOD_getInstructionPosition(opt_inst);
    internal_markInstruction(instsMarked, instsMarkedLen, instID);
    xanList_insert(instList, opt_inst);
    while (xanList_length(instList) > 0) {
        item = xanList_first(instList);
        assert(item != NULL);
        opt_inst = item->data;
        assert(opt_inst != NULL);
        xanList_deleteItem(instList, item);
        assert(xanList_find(instList, opt_inst) == NULL);
#ifdef DEBUG
        instID = IRMETHOD_getInstructionPosition(opt_inst);
        assert(internal_isInstructionMarked(instsMarked, instsMarkedLen, instID));
#endif
        opt_inst2 = IRMETHOD_getSuccessorInstruction(method, opt_inst, NULL);
        assert(opt_inst2 != opt_inst);
        while (opt_inst2 != method->exitNode && opt_inst2 != NULL) {
            assert(opt_inst2 != opt_inst);
            instID = IRMETHOD_getInstructionPosition(opt_inst2);
            if (!internal_isInstructionMarked(instsMarked, instsMarkedLen, instID)) {
                internal_markInstruction(instsMarked, instsMarkedLen, instID);
                assert(xanList_find(instList, opt_inst2) == NULL);
                xanList_insert(instList, opt_inst2);
            }
            assert(internal_isInstructionMarked(instsMarked, instsMarkedLen, instID));
            opt_inst2 = IRMETHOD_getSuccessorInstruction(method, opt_inst, opt_inst2);
        }
    }
    assert(xanList_length(instList) == 0);

    /* Delete code unreachable		*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);
    for (instID = 0; instID < instructionsNumber; instID++) {
        if (!internal_isInstructionMarked(instsMarked, instsMarkedLen, instID)) {
            opt_inst = IRMETHOD_getInstructionAtPosition(method, instID);
            assert(opt_inst != NULL);
            xanList_insert(instList, opt_inst);
        }
    }
#ifdef DEBUG
    item = xanList_first(instList);
    while (item != NULL) {
        opt_inst = item->data;
        instID = IRMETHOD_getInstructionPosition(opt_inst);
        PDEBUG("OPTIMIZER_DEADCODE: deleteCodeUnreachable:	Delete instruction %d\n", instID);
        item = item->next;
    }
#endif
    item = xanList_first(instList);
    while (item != NULL) {
        opt_inst = item->data;
        if (IRMETHOD_getInstructionsNumber(method) == 1) {
            break;
        }
        if (	(opt_inst->type != IRENDFINALLY)					||
		(IRMETHOD_getSuccessorInstruction(method, opt_inst, NULL) == NULL)	){
            IRMETHOD_deleteInstruction(method, opt_inst);
        }
        item = item->next;
    }

    /* Free the memory			*/
    freeFunction(instsMarked);
    xanList_destroyList(instList);

    /* Return				*/
    return;
}

static inline JITINT32 internal_isInstructionMarked (JITNUINT *instsMarked, JITUINT32 instsMarkedLen, JITUINT32 instID) {
    JITUINT32 trigger;
    JITNUINT flag;

    /* Assertions			*/
    assert(instsMarked != NULL);

    trigger = instID / (sizeof(JITNUINT) * 8);
    assert(trigger < instsMarkedLen);
    flag = 0x1;
    flag = flag << (instID % (sizeof(JITNUINT) * 8));
    if ((instsMarked[trigger] & flag) == 0) {
        return 0;
    }
    return 1;
}

static inline void internal_markInstruction (JITNUINT *instsMarked, JITUINT32 instsMarkedLen, JITUINT32 instID) {
    JITUINT32 trigger;
    JITNUINT flag;

    /* Assertions			*/
    assert(instsMarked != NULL);
    assert(!internal_isInstructionMarked(instsMarked, instsMarkedLen, instID));

    trigger = instID / (sizeof(JITNUINT) * 8);
    assert(trigger < instsMarkedLen);
    flag = 0x1;
    flag = flag << (instID % (sizeof(JITNUINT) * 8));
    instsMarked[trigger] |= flag;

    /* Return			*/
    assert(internal_isInstructionMarked(instsMarked, instsMarkedLen, instID));
    return;
}

static inline JITNINT internal_hasSideEffect (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 instType;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Fetch the instruction type	*/
    instType = inst->type;

    /* Check the type of the        *
     * instruction			*/
    switch (instType) {
        case IRICALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRCALL:
        case IRVCALL:
        case IRRET:
            return JITTRUE;
    }

    /* Check if the instruction can	*
     * rise an exception		*/
    if (IRMETHOD_mayThrowException(inst)) {
        return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

static inline JITBOOLEAN internal_set_pointer_type (ir_item_t *par) {
    JITINT8         *className;
    JITBOOLEAN	modified;

    /* Assertions                   */
#ifdef DEBUG
    assert(par != NULL);
    if (par->internal_type == IRVALUETYPE) {
        assert(par->type_infos != NULL);
    }
#endif

    modified	= JITFALSE;
    if (par->type != IRSYMBOL) {
        switch (par->internal_type) {
            case IRMPOINTER:
            case IROBJECT:
                if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
                    par->internal_type = IRUINT32;
                } else {
                    par->internal_type = IRUINT64;
                }
                modified	= JITTRUE;
                break;
        }

    } else if (par->internal_type == IRNINT) {
        if (IRDATA_getSizeOfIRType(IRNINT) == 4) {
            par->internal_type = IRINT32;
        } else {
            par->internal_type = IRINT64;
        }
        modified	= JITTRUE;

    } else if (    (par->internal_type == IRVALUETYPE)             &&
                   (par->type_infos != NULL)       ) {
        assert(par->type_infos != NULL);
        className = IRMETHOD_getClassName(par->type_infos);
        assert(className != NULL);
        if (STRCMP(className, "System.IntPtr") == 0) {
            if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
                par->internal_type = IRUINT32;
            } else {
                par->internal_type = IRUINT64;
            }
            modified	= JITTRUE;
            if (par->type != IROFFSET) {
                par->type = par->internal_type;
            }
            par->type_infos = NULL;
        }

    } else if (    (par->internal_type == IRTYPE)                  &&
                   ((par->value).v == IRVALUETYPE)                 &&
                   (par->type_infos != NULL)       ) {
        assert(par->type_infos != NULL);
        className = IRMETHOD_getClassName(par->type_infos);
        assert(className != NULL);
        if (STRCMP(className, "System.IntPtr") == 0) {
            if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
                (par->value).v = IRUINT32;
            } else {
                (par->value).v = IRUINT64;
            }
            modified	= JITTRUE;
            par->type_infos = NULL;
        }
    }

    if (IRDATA_isAConstant(par)) {
        par->type	= par->internal_type;
    }

    return modified;
}

static inline JITBOOLEAN internal_set_int_type (ir_item_t *par) {
    JITBOOLEAN	modified;

    /* Assertions                   */
    assert(par != NULL);

    modified	= JITFALSE;
    if (par->internal_type == IRNINT) {
        if (IRDATA_getSizeOfIRType(IRNINT) == 4) {
            par->internal_type = IRINT32;
        } else {
            par->internal_type = IRINT64;
        }
        modified	= JITTRUE;
    }
    if (par->type == IRNINT) {
        par->type = par->internal_type;
        modified	= JITTRUE;
    }

    if (par->type == IRTYPE) {
        if ((par->value).v == IRNINT) {
            if (IRDATA_getSizeOfIRType(IRNINT) == 4) {
                (par->value).v = IRINT32;
            } else {
                (par->value).v = IRINT64;
            }
            modified	= JITTRUE;
        }
    }

    if (IRDATA_isAConstant(par)) {
        par->type	= par->internal_type;
    }

    return modified;
}

static inline void internal_set_type (ir_method_t *method, ir_item_t *par) {
    method->modified	|= internal_set_uint_type(par);
    method->modified	|= internal_set_int_type(par);
    method->modified	|= internal_set_pointer_type(par);

    return ;
}

static inline JITBOOLEAN internal_set_uint_type (ir_item_t *par) {
    JITBOOLEAN	modified;

    /* Assertions                   */
    assert(par != NULL);

    modified	= JITFALSE;
    if (par->internal_type == IRNUINT) {
        if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
            par->internal_type = IRUINT32;
        } else {
            par->internal_type = IRUINT64;
        }
        modified	= JITTRUE;
    }
    if (par->type == IRNUINT) {
        par->type 	= par->internal_type;
        modified	= JITTRUE;
    }

    if (par->type == IRTYPE) {
        if ((par->value).v == IRNUINT) {
            if (IRDATA_getSizeOfIRType(IRNUINT) == 4) {
                (par->value).v = IRUINT32;
            } else {
                (par->value).v = IRUINT64;
            }
            modified	= JITTRUE;
        }
    }

    if (IRDATA_isAConstant(par)) {
        par->type	= par->internal_type;
    }

    return modified;
}

static inline void internal_set_float_type (ir_item_t *par) {

    /* Assertions                   */
    assert(par != NULL);

    if (par->internal_type == IRNFLOAT) {
        if (IRDATA_getSizeOfIRType(IRNFLOAT) == 4) {
            par->internal_type = IRFLOAT32;
        } else {
            par->internal_type = IRFLOAT64;
        }
    }
    if (par->type == IRNFLOAT) {
        par->type = par->internal_type;
    }

    if (par->type == IRTYPE) {
        if ((par->value).v == IRNFLOAT) {
            if (IRDATA_getSizeOfIRType(IRNFLOAT) == 4) {
                (par->value).v = IRFLOAT32;
            } else {
                (par->value).v = IRFLOAT64;
            }
        }
    }

    if (IRDATA_isAConstant(par)) {
        par->type	= par->internal_type;
    }

    return ;
}

static inline ir_instruction_t * internal_getSingleDefinitionReachedIn (ir_method_t *method, IR_ITEM_VALUE varID, ir_instruction_t *instToCheck, XanHashTable *defsTable) {
    XanList                 *list;
    XanListItem             *item;
    JITUINT32 count;
    ir_instruction_t        *champion_inst;
    ir_instruction_t        *inst;

    /* Assertions                   */
    assert(method != NULL);
    assert(instToCheck != NULL);
    assert(defsTable != NULL);

    /* Initialize the variables     */
    champion_inst = NULL;
    count = 0;

    list = internal_getDefinitions(method, varID, defsTable);
    assert(list != NULL);
    item = xanList_first(list);
    while (item != NULL) {
        inst = xanList_getData(item);
        assert(inst != NULL);
        if (ir_instr_reaching_definitions_reached_in_is(method, instToCheck, IRMETHOD_getInstructionPosition(inst))) {
            champion_inst = inst;
            count++;
        }
        item = item->next;
    }

    /* Return                       */
    if (count == 1) {
        return champion_inst;
    }
    return NULL;
}

static inline XanList * internal_getDefinitions (ir_method_t *method, IR_ITEM_VALUE varID, XanHashTable *defsTable) {
    ir_instruction_t        *opt_inst;
    JITUINT32 instID;
    JITNUINT reaching_inst_def;
    XanList                 *list;

    /* Assertions                           */
    assert(method != NULL);
    assert(defsTable != NULL);

    /* Initialize the variables		*/
    opt_inst = NULL;

    list = xanHashTable_lookup(defsTable, (void *) (JITNUINT) varID);
    if (list == NULL) {

        /* Allocate the list                    */
        list = xanList_new(allocFunction, freeFunction, NULL);
        assert(list != NULL);

        /* Find every definitions               */
        for (instID = 0; instID < IRMETHOD_getInstructionsNumber(method); instID++) {

            opt_inst = IRMETHOD_getInstructionAtPosition(method, instID);
            assert(opt_inst != NULL);
            reaching_inst_def = livenessGetDef(method, opt_inst);
            if (reaching_inst_def == varID) {
                xanList_insert(list, opt_inst);
            }
        }

        /* If varID is a parameter, then it has	*
         * to be considered as defined by the	*
         * varID instruction of the method,	*
         * which is, by definition, a IRNOP	*
         * instruction.				*/
        if (    varID < IRMETHOD_getMethodParametersNumber(method)      &&
                (xanList_find(list, opt_inst) == NULL)                    ) {
            opt_inst = IRMETHOD_getInstructionAtPosition(method, varID);
            assert(opt_inst != NULL);
            assert(IRMETHOD_getInstructionType(opt_inst) == IRNOP);
            xanList_insert(list, opt_inst);
        }

        /* Cache the new list			*/
        xanHashTable_insert(defsTable, (void *) (JITNUINT) varID, list);
    }
    assert(list != NULL);

    /* Return                               */
    return list;
}

static inline void internal_removeCatchBlocks (ir_method_t *method) {
    JITUINT32 		num;
    JITUINT32 		count;
    XanList                 *toDelete;
    XanListItem             *item;

    /* Assertions                           */
    assert(method != NULL);

    /* Allocate the necessary memories	*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Remove not useful catch blocks	*/
    num = IRMETHOD_getInstructionsNumber(method);
    for (count = 0; count < num; count++) {
        ir_instruction_t        *opt_inst;
        IR_ITEM_VALUE label1;
        IR_ITEM_VALUE label2;
        IR_ITEM_VALUE label3;

        /* Fetch the instruction		*/
        opt_inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(opt_inst != NULL);

        /* Check the instruction		*/
        if (opt_inst->type != IRBRANCHIFPCNOTINRANGE) {
            continue;
        }
        assert(opt_inst->type == IRBRANCHIFPCNOTINRANGE);

        /* Check if the try block exists	*/
        label1 = IRMETHOD_getInstructionParameter1Value(opt_inst);
        label2 = IRMETHOD_getInstructionParameter2Value(opt_inst);
        label3 = IRMETHOD_getInstructionParameter3Value(opt_inst);
        if (    ((IRMETHOD_getLabelInstruction(method, label1) == NULL) && (IRMETHOD_getFinallyInstruction(method, label1) == NULL))    ||
                ((IRMETHOD_getLabelInstruction(method, label2) == NULL) && (IRMETHOD_getFinallyInstruction(method, label2) == NULL))    ||
                ((IRMETHOD_getLabelInstruction(method, label3) == NULL) && (IRMETHOD_getFinallyInstruction(method, label3) == NULL))    ) {
            xanList_append(toDelete, opt_inst);
        }
    }
    item = xanList_first(toDelete);
    while (item != NULL) {
        IRMETHOD_deleteInstruction(method, item->data);
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(toDelete);

    /* Return				*/
    return;
}

static inline void print_sets (ir_method_t *method) {
    ir_instruction_t        *opt_inst;
    JITINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    /* Init the variables		*/
    count = 0;
    opt_inst = NULL;

    /* Fetch the first instruction	*/
    opt_inst = IRMETHOD_getFirstInstruction(method);
    assert(opt_inst != NULL);

    while (opt_inst != NULL) {
        JITINT32 count2;
        PDEBUG("OPTIMIZER_DEADCODE: <Instruction %d>    Type    = %d\n", count, IRMETHOD_getInstructionType(opt_inst));
        PDEBUG("OPTIMIZER_DEADCODE:                     DEF	= %d\n", (JITINT32)ir_instr_liveness_def_get(method, opt_inst));
        PDEBUG("OPTIMIZER_DEADCODE:                     USE	= 0x");
        for (count2 = 0; count2 < (method->livenessBlockNumbers); count2++) {
            PDEBUG("%X", (JITUINT32)ir_instr_liveness_use_get(method, opt_inst, count2));
        }
        PDEBUG("\n");
        PDEBUG("OPTIMIZER_DEADCODE:                     IN	= 0x");
        for (count2 = 0; count2 < (method->livenessBlockNumbers); count2++) {
            PDEBUG("%X", (JITUINT32)ir_instr_liveness_in_get(method, opt_inst, count2));
        }
        PDEBUG("\n");
        PDEBUG("OPTIMIZER_DEADCODE:                     OUT	= 0x");
        for (count2 = 0; count2 < (method->livenessBlockNumbers); count2++) {
            PDEBUG("%X", (JITUINT32)ir_instr_liveness_out_get(method, opt_inst, count2));
        }
        PDEBUG("\n");
        count++;
        opt_inst = IRMETHOD_getNextInstruction(method, opt_inst);
    }

    return ;
}
