/*
 * Copyright (C) 2006 - 2012 Campanoni Simone, Ciancone AndreA
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ir_language.h>
#include <metadata_manager.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <base_symbol.h>

// My headers
#include <ir_method.h>
#include <ir_data.h>
#include <ir_internal_functions.h>
#include <codetool_types.h>
#include <config.h>
// End

#define START_IR_INSTRUCTIONS_ID        0
#define GROW_BUFFERED_INSTRUCTIONS        50
#define SHRINK_BUFFERED_INSTRUCTIONS    10
#define BEGIN_INSTRUCTIONS_NUMBER       2
#define INSN_VOLATILE_FLAG		0x1
#define INSN_VARARG_FLAG		0x2

extern ir_lib_t * ir_library;

static inline void internal_addInstructionsThatCanBeReachedFromPosition (ir_method_t *method, XanHashTable *t, JITUINT32 positionID, data_flow_t *reachableInstructions, JITUINT32 instructionsNumber, ir_instruction_t **insns);
static inline XanList * internal_getInstructionSuccessors (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized);
static inline XanList * internal_getInstructionPredecessors (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized);
static inline JITBOOLEAN internal_is_instruction_reachable_in (data_flow_t *reachableInstructions, ir_instruction_t *inst, ir_instruction_t *position);
static inline void ir_instr_par_delete (ir_item_t *p);
static inline void internal_destroyDataDependence (data_dep_arc_list_t *d);
static inline void internal_destroyDataDependences (XanHashTable *l);
static inline void ir_method_update_max_variable (ir_method_t *method);
static inline ir_instruction_t * ir_instr_get_first (ir_method_t *method);
static inline JITBOOLEAN internal_areItemsEqual (ir_item_t *item1, ir_item_t *item2);
static inline JITBOOLEAN _IRMETHOD_isACallInstruction (ir_instruction_t *inst);
static inline void _IRMETHOD_substituteVariableInsideInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID);
static inline JITBOOLEAN _IRMETHOD_mayAccessMemory (ir_method_t *method, ir_instruction_t *inst);
static inline ir_instruction_t * ir_instr_get_by_pos (ir_method_t * method, JITUINT32 position);
static inline JITBOOLEAN _IRMETHOD_mayThrowException (ir_instruction_t *inst);
static inline JITBOOLEAN _IRMETHOD_isAFloatType (JITUINT32 type);
static inline void ir_instr_data_delete (ir_method_t *method, ir_instruction_t *instr, JITBOOLEAN deleteMetadata);
static inline ir_instruction_t * ir_instr_get_predecessor (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevPredecessor, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized);
static inline ir_instruction_t * ir_instr_get_successor (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevSuccessor, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized);
static inline ir_instruction_t * ir_instr_get_successor_debug (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevSuccessor);
static inline JITINT8 * ir_method_name_get (ir_method_t * method);
static inline JITINT8 * ir_method_completename_get (ir_method_t * method);
static inline JITUINT32 ir_instr_count (ir_method_t * method);
static inline void ir_method_lock (ir_method_t *method);
static inline void ir_method_unlock (ir_method_t *method);
static inline JITUINT32 ir_instr_pars_count_get (ir_instruction_t *inst);
static inline void ir_alias_destroy (XanHashTable *alias);
static inline JITBOOLEAN internal_has_next_instruction_as_successor (ir_method_t *method, ir_instruction_t *inst);
static inline JITBOOLEAN internal_has_next_instruction_as_successor_without_jumps (ir_method_t *method, ir_instruction_t *inst);
static inline JITINT32 ir_instr_move_after (ir_method_t * method, ir_instruction_t * instr, ir_instruction_t * instr_succ);
static inline JITUINT32 ir_method_var_max_get (ir_method_t * method);
static inline void ir_method_var_max_set (ir_method_t * method, JITUINT32 var_max);
static inline XanList * internal_getDefinitions (ir_method_t *method, IR_ITEM_VALUE varID);
static inline XanList * internal_getUses (ir_method_t *method, IR_ITEM_VALUE varID);
static inline JITINT32 ir_instr_has_escapes (ir_method_t *method, ir_instruction_t *inst);
static inline JITINT32 ir_instr_may_instruction_access_existing_heap_memory (ir_method_t *method, ir_instruction_t *inst);
static inline JITBOOLEAN internal_is_a_dominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated, XanNode *tree);
static inline ir_instruction_t * ir_instr_get_last (ir_method_t *method);
static inline void update_exit_node_ID (ir_method_t *method);
static inline void ir_method_delete_extra_memory (ir_method_t *method);
static inline void internal_ir_instr_reaching_definitions_alloc (ir_instruction_t * inst, JITUINT32 blocksNumber);
static inline ir_instruction_t * ir_instr_label_like_get (ir_method_t * method, JITUINT32 labelID, JITUINT32 instType);
static inline ir_instruction_t * ir_instr_finally_get (ir_method_t * method, JITUINT32 labelID);
static inline ir_instruction_t * ir_instr_call_finally_get (ir_method_t * method, JITUINT32 labelID);
static inline ir_instruction_t * ir_instr_filter_get (ir_method_t * method, JITUINT32 labelID);
static inline JITBOOLEAN _IRMETHOD_isTheVariableATemporary (ir_method_t *method, JITUINT32 varID);
static inline JITUINT32 ir_method_pars_count_get (ir_method_t * method);
static inline JITUINT32 ir_method_var_local_count_get (ir_method_t * method);
static inline void _IRMETHOD_setNativeCallInstruction (ir_method_t *method, ir_instruction_t *inst, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters);
static inline XanList* ir_instr_get_loop_instructions (ir_method_t *method, circuit_t *loop);
static inline JITBOOLEAN _IRMETHOD_mayInstructionOfTypeThrowException (JITUINT32 type);
static inline JITBOOLEAN internal_is_valid_information (ir_method_t *method, JITUINT64 codetool_type);

static inline XanList * _IRMETHOD_getCircuitInductionVariables (ir_method_t *method, circuit_t *loop) {
    XanList                 *list;

    /* Assertions			*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Fetch the list		*/
    if (loop->induction_table != NULL) {
        list = xanHashTable_toList(loop->induction_table);
    } else {
        list = xanList_new(allocFunction, freeFunction, NULL);
    }
    assert(list != NULL);

    /* Return			*/
    return list;
}

static inline JITBOOLEAN _IRMETHOD_isTheVariableATemporary (ir_method_t *method, JITUINT32 varID) {
    JITUINT32 pars;
    JITUINT32 locals;

    pars = ir_method_pars_count_get(method);
    locals = ir_method_var_local_count_get(method);
    return (varID >= (pars + locals)) ? JITTRUE : JITFALSE;
}

ir_instruction_t * IRMETHOD_getTheSourceOfTheCircuitBackedge (circuit_t *loop) {
    return (loop->backEdge).src;
}

ir_instruction_t * IRMETHOD_getTheDestinationOfTheCircuitBackedge (circuit_t *loop) {
    return (loop->backEdge).dst;
}

JITBOOLEAN IRMETHOD_isCircuitInvariant (ir_method_t *method, circuit_t *loop, ir_instruction_t *inst) {
    JITUINT32 	trigger;
    JITNUINT 	mask;

    /* Assertions						*/
    assert(method != NULL);
    assert(loop != NULL);
    assert(inst != NULL);

    /* Check if there are invariants			*/
    if (loop->invariants == NULL) {

        /* Return						*/
        return JITFALSE;
    }

    /* Compute the bitmap to use				*/
    mask 		= (1 << ((inst->ID) % (sizeof(JITNUINT) * 8)));
    assert(mask != 0);
    trigger 	= ((inst->ID) / (sizeof(JITNUINT) * 8));

    /* Check if the instruction belongs to the loop		*/
    if (!(loop->belong_inst[trigger] & mask)) {

        /* Return						*/
        return JITFALSE;
    }

    /* Check if the instruction is invariant		*/
    if (!(loop->invariants[trigger] & mask)) {

        /* Return						*/
        return JITFALSE;
    }

    /* Return						*/
    return JITTRUE;
}

XanList * IRMETHOD_getCircuitInvariants (ir_method_t *method, circuit_t *loop) {
    XanList                 *list;
    XanList                 *nestedCircuits;
    JITNUINT 		mask;
    ir_instruction_t        *inst;
    JITUINT32 		instID;
    JITUINT32 		instructionsNumber;

    /* Assertions						*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Fetch the number of instructions of the method	*/
    instructionsNumber = ir_instr_count(method);

    /* Allocate the list					*/
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Check if there are invariants			*/
    if (loop->invariants == NULL) {

        /* There is no invariants in this loop.
         */
        return list;
    }

    /* Fetch the nested loops				*/
    nestedCircuits = IRMETHOD_getNestedCircuits(method, loop);

    /* Fill up the list					*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        JITUINT32 	trigger;
        JITBOOLEAN 	isInvariant;
        XanListItem 	*item;

        /* Create the bitmask for the current instruction	*/
        mask 		= (1 << (instID % (sizeof(JITNUINT) * 8)));
        assert(mask != 0);
        trigger 	= (instID / (sizeof(JITNUINT) * 8));

        /* Fetch the instruction				*/
        inst 		= ir_instr_get_by_pos(method, instID);
        assert(inst != NULL);

        /* Check if the instruction belongs to the loop and     *
         * it is invariant					*/
        if (    ((loop->belong_inst[trigger] & mask) == 0)  	||
                ((loop->invariants[trigger] & mask) == 0)     	) {
            continue ;
        }

        /* Check if the instruction is invariant to every       *
         * subloop as well					*/
        isInvariant = JITTRUE;
        if (IRMETHOD_doesInstructionDefineAVariable(method, inst)) {

            /* Check that every instruction that belongs to the     *
             * subloop does not redefine the variable		*/
            item = xanList_first(nestedCircuits);
            while ((item != NULL) && (isInvariant)) {
                circuit_t 		*subloop;
                XanList 	*subloopInsts;
                XanListItem 	*item2;

                /* Fetch the subloop					*/
                subloop = item->data;
                assert(subloop != NULL);

                /* Fetch the list of instructions that belong to the	*
                 * subloop						*/
                subloopInsts = ir_instr_get_loop_instructions(method, subloop);

                /* Check the subloop					*/
                item2 = xanList_first(subloopInsts);
                while (item2 != NULL) {
                    ir_instruction_t 	*subloopInst;
                    JITUINT32 		triggerSubloopInst;
                    JITNUINT 		maskSubloopInst;
                    JITUINT32 		inst2ID;

                    /* Fetch the instruction that belongs to the current	*
                     * subloop						*/
                    subloopInst 	= item2->data;
                    assert(subloopInst != NULL);
                    inst2ID		= subloopInst->ID;

                    /* Check if it is invariant				*/
                    maskSubloopInst 		= (1 << (inst2ID % (sizeof(JITNUINT) * 8)));
                    triggerSubloopInst 		= ((inst2ID) / (sizeof(JITNUINT) * 8));
                    if ((subloop->invariants[triggerSubloopInst] & maskSubloopInst) == 0) {
                        isInvariant = JITFALSE;
                        break;
                    }

                    item2 = item2->next;
                }

                /* Free the memory					*/
                xanList_destroyList(subloopInsts);

                item = item->next;
            }
        }

        /* Add the instruction if it is invariant		*/
        if (isInvariant) {
            switch (inst->type) {
                case IRLABEL:
                    break;
                default:
                    xanList_append(list, inst);
            }
        }
    }

    /* Free the memory		*/
    xanList_destroyList(nestedCircuits);

    /* Return			*/
    return list;
}

static inline XanList * internal_getDominators (ir_instruction_t *inst, XanNode *tree) {
    XanList         *l;
    XanNode         *node;

    /* Assertions				*/
    assert(tree != NULL);
    assert(inst != NULL);

    /* Find the instruction within the tree	*/
    node = tree->find(tree, inst);
    assert(node != NULL);

    /* Fetch the list			*/
    l = node->toPreOrderList(node);
    assert(l != NULL);

    /* Return				*/
    return l;
}

static inline void ir_method_lock (ir_method_t *method) {
    PLATFORM_lockMutex(&(method->mutex));
}

static inline void ir_method_unlock (ir_method_t *method) {

    /* Assertions			*/
    assert(method != NULL);
    assert(PLATFORM_trylockMutex(&(method->mutex)) != 0);

    PLATFORM_unlockMutex(&(method->mutex));
}


static inline JITUINT32 ir_instr_pars_count_get (ir_instruction_t *inst) {

    /* Assertions			*/
    assert(inst != NULL);

    if ((inst->param_1).type == ((JITUINT32) NOPARAM)) {
        return 0;
    }
    if ((inst->param_2).type == ((JITUINT32) NOPARAM)) {
        return 1;
    }
    if ((inst->param_3).type == ((JITUINT32) NOPARAM)) {
        return 2;
    }
    return 3;
}

static inline JITUINT32 ir_instrcall_pars_count_get (ir_instruction_t *inst) {
    assert(inst != NULL);

    if (inst->callParameters == NULL) {
        return 0;
    }
    return inst->callParameters->len;
}

static inline ir_item_t * ir_instrcall_par_new (ir_method_t * method, ir_instruction_t * inst, JITUINT32 *position) {
    ir_item_t       *data;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(position != NULL);

    data = sharedAllocFunction(sizeof(ir_item_t));
    if (inst->callParameters == NULL) {
        inst->callParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
    }
    assert(inst->callParameters != NULL);
    xanList_append(inst->callParameters, data);

    (*position) = inst->callParameters->len - 1;

    return data;
}

static inline void ir_instrcall_pars_delete (ir_method_t * method, ir_instruction_t *inst) {

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Check if there are parameters*/
    if (inst->callParameters == NULL) {
        return;
    }

    /* Delete the call parameter    */
    xanList_destroyListAndData(inst->callParameters);
    inst->callParameters = NULL;

    /* Return			*/
    return;
}

static inline void ir_instrcall_par_move (ir_method_t * method, ir_instruction_t *inst, JITUINT32 oldPos, JITUINT32 newPos) {
    XanListItem * item;
    XanListItem * prec;
    ir_item_t * data;
    XanList *xan;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(oldPos != newPos);
    assert(inst->callParameters);


    xan = inst->callParameters;
    item = xanList_getElementFromPositionNumber(xan, oldPos);
    assert(item != NULL);
    data = item->data;
    assert(data != NULL);
    xanList_deleteItem(xan, item);

    if (newPos == 0) {
        xanList_insert(xan, data);
        return;
    }
    prec = xanList_getElementFromPositionNumber(xan, newPos - 1);
    assert(prec != NULL);
    xanList_insertAfter(xan, prec, data);

    /* Return			*/
    return;
}

static inline void ir_instrcall_par_delete (ir_method_t * method, ir_instruction_t *inst, JITUINT32 nth) {
    assert(method != NULL);
    assert(inst != NULL);

    XanListItem * item;

    /* Delete the call parameter */
    item = xanList_getElementFromPositionNumber(inst->callParameters, nth);
    xanList_deleteItem(inst->callParameters, item);
}

static inline ir_item_t * ir_instrcall_par_get_by_pos (ir_instruction_t *inst, JITUINT32 nth) {
    assert(inst != NULL);
    XanListItem * item;

    if (inst->callParameters == NULL) {
        return NULL;
    }
    assert(ir_instrcall_pars_count_get(inst) >= nth);
    item = xanList_getElementFromPositionNumber(inst->callParameters, nth);
    if (item == NULL) {
        return NULL;
    }
    return item->data;
}

static inline ir_item_t * ir_instrcall_par_get (ir_instruction_t *inst, JITUINT32 nth) {

    /* Assertions			*/
    assert(inst != NULL);

    return ir_instrcall_par_get_by_pos(inst, nth);
}

static inline JITUINT64 ir_instrcall_par_value_get (ir_instruction_t *inst, JITUINT32 nth) {
    ir_item_t * param;

    assert(inst != NULL);

    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return NOPARAM;
    }
    return (param->value).v;
}

static inline JITUINT32 ir_instrcall_par_type_get (ir_instruction_t *inst, JITUINT32 nth) {
    ir_item_t * param;

    assert(inst != NULL);

    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return NOPARAM;
    }
    return param->type;
}

static inline XanList* ir_instr_get_loop_instructions (ir_method_t *method, circuit_t *loop) {
    XanList                 *list;
    ir_instruction_t        *inst;
    JITUINT32 count;
    JITNUINT temp;
    JITUINT32 block;

    /* Assertions                   */
    assert(method != NULL);
    assert(loop != NULL);

    /* Allocate the list            */
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Search the instructions      */
    for (count = 0; count < ir_instr_count(method); count++) {
        temp = 1;
        temp = temp << (count % (sizeof(JITNUINT) * 8));
        block = count / (sizeof(JITNUINT) * 8);
        if ((loop->belong_inst[block] & temp) != 0) {
            assert((loop->belong_inst[block] & temp) == temp);
            inst = ir_instr_get_by_pos(method, count);
            assert(inst != NULL);
            xanList_insert(list, inst);
        }
    }

    /* Return                       */
    return list;
}

static inline circuit_t * ir_instr_get_parent_loop (ir_method_t *method, circuit_t *loop) {
    XanNode         *node;
    circuit_t          *parentCircuit;

    /* Assertions                           */
    assert(method != NULL);
    assert(loop != NULL);

    /* Initialize the variables             */
    node = NULL;
    parentCircuit = NULL;

    /* Check if there is the loops tree     */
    if (method->loopNestTree == NULL) {
        return NULL;
    }

    /* Find the node in the tree            */
    node = method->loopNestTree->find(method->loopNestTree, loop);
    assert(node != NULL);

    /* Fetch the parent                     */
    node = node->getParent(node);
    if (node == NULL) {
        return NULL;
    }

    /* Fetch the parent loop                */
    parentCircuit = (circuit_t *) node->getData(node);
    assert(parentCircuit != NULL);

    /* Return                               */
    return parentCircuit;
}

/**
 * @brief Seek the most nested loop where the instruction belongs
 *
 * @param method IR Method.
 * @param inst the instruction tested
 * @return the loop resulting from the search,if the instruction doesn't belongs to any loop return NULL
 */
static inline circuit_t * ir_instr_get_loop (ir_method_t* method, ir_instruction_t* inst) {
    JITNUINT temp, trigger;
    XanNode         *Node;
    XanListItem     *it;
    circuit_t          *Circuit;
    circuit_t          *nestedCircuit;

    /* Assertions                                   */
    assert(method != NULL);
    assert(inst->ID < IRMETHOD_getInstructionsNumber(method));
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Initialize the variables                     */
    Circuit = NULL;
    nestedCircuit = NULL;

    /* Check if there are loops                     */
    if (    (method->loop == NULL)                          ||
            (xanList_length(method->loop) == 0)       ) {
        return NULL;
    }

    /* This code segment navigates the loop list and        *
     * appends to CandidateCircuitList all loops which the     *
     * input instruction belongs to				*/
    Node = NULL;
    it = xanList_first((method->loop));
    while (it != NULL) {
        Circuit = (circuit_t *) it->data;
        assert(Circuit != NULL);
        assert(Circuit->belong_inst != NULL);
        temp = 1;
        temp = temp << ((inst->ID) % (sizeof(JITNUINT) * 8));
        trigger = (inst->ID) / (sizeof(JITNUINT) * 8);

        /* If instruction inst belongs to loop Circuit, add this   *
        * loop to CandidateCircuitList.                           */
        if (((Circuit->belong_inst[trigger]) | temp) == (Circuit->belong_inst[trigger])) {
            if (    (Node == NULL)                          ||
                    (Circuit->header_id == inst->ID)           ||
                    (Node->find(Node, Circuit) != NULL)        ) {
                nestedCircuit = Circuit;
                if (method->loopNestTree != NULL) {
                    Node = method->loopNestTree->find(method->loopNestTree, Circuit);
                    assert(Node != NULL);
                    assert(Node->find(Node, Circuit) != NULL);
                }
                assert(nestedCircuit != NULL);
            }
        }

        /* Fetch the next element from the list			*/
        it = it->next;
    }

    /* Return                                       */
    return nestedCircuit;
}

JITNUINT ir_instr_liveness_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->livenessBlockNumbers > blockNumber);
    return (inst->metadata->liveness).out[blockNumber];
}

void ir_instr_liveness_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->livenessBlockNumbers > blockNumber);
    (inst->metadata->liveness).out[blockNumber] = value;
}

JITNUINT ir_instr_liveness_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->livenessBlockNumbers > blockNumber);
    return (inst->metadata->liveness).in[blockNumber];
}

void ir_instr_liveness_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->livenessBlockNumbers > blockNumber);
    (inst->metadata->liveness).in[blockNumber] = value;
}

JITNUINT ir_instr_liveness_use_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->livenessBlockNumbers > blockNumber);
    return (inst->metadata->liveness).use[blockNumber];
}

void ir_instr_liveness_use_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->livenessBlockNumbers > blockNumber);
    assert((inst->metadata->liveness).use != NULL);

    (inst->metadata->liveness).use[blockNumber] = value;
}

JITNUINT ir_instr_liveness_def_get (void *method, ir_instruction_t *inst) {
    JITNUINT ret;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    ret = (inst->metadata->liveness).def;

    /* Return			*/
    return ret;
}

void ir_instr_liveness_def_set (void *method, ir_instruction_t *inst, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    (inst->metadata->liveness).def = value;
}

JITBOOLEAN IRMETHOD_isVariableLiveIN (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (varID / (sizeof(JITNINT) * 8));
    temp = temp << (varID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->liveness).in == NULL) {
        return JITFALSE;
    }
    if (((inst->metadata->liveness).in[trigger] & temp) != temp) {
        return JITFALSE;
    }
    return JITTRUE;
}

JITBOOLEAN IRMETHOD_isVariableLiveOUT (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (varID / (sizeof(JITNINT) * 8));
    temp = temp << (varID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->liveness).out == NULL) {
        return JITFALSE;
    }
    if (((inst->metadata->liveness).out[trigger] & temp) != temp) {
        return JITFALSE;
    }
    return JITTRUE;
}

JITNUINT ir_instr_reaching_definitions_gen_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_definitions).gen[blockNumber];
}

void ir_instr_reaching_definitions_gen_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_definitions).gen[blockNumber] = value;
}

JITNUINT ir_instr_reaching_definitions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_definitions).out[blockNumber];
}

void ir_instr_reaching_definitions_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_definitions).out[blockNumber] = value;
}

JITNUINT ir_instr_reaching_definitions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_definitions).in[blockNumber];
}

void ir_instr_reaching_definitions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_definitions).in[blockNumber] = value;
}

JITNUINT ir_instr_reaching_definitions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_definitions).kill[blockNumber];
}

void ir_instr_reaching_definitions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingDefinitionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_definitions).kill[blockNumber] = value;
}

JITBOOLEAN ir_instr_reaching_definitions_reached_out_is (void *method, ir_instruction_t *inst, JITUINT32 definitionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (definitionID / (sizeof(JITNINT) * 8));
    temp = temp << (definitionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->reaching_definitions).out == NULL) {
        return 0;
    }
    if (((inst->metadata->reaching_definitions).out[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

JITBOOLEAN ir_instr_reaching_definitions_reached_in_is (void *method, ir_instruction_t *inst, JITUINT32 definitionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (definitionID / (sizeof(JITNUINT) * 8));
    temp = temp << (definitionID % (sizeof(JITNUINT) * 8));

    if ((inst->metadata->reaching_definitions).in == NULL) {
        return JITFALSE;
    }
    if (((inst->metadata->reaching_definitions).in[trigger] & temp) != temp) {
        return JITFALSE;
    }
    return JITTRUE;
}

static inline JITNUINT ir_instr_reaching_expressions_gen_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_expressions).gen[blockNumber];
}

static inline void ir_instr_reaching_expressions_gen_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_expressions).gen[blockNumber] = value;
}

static inline JITNUINT ir_instr_reaching_expressions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_expressions).out[blockNumber];
}

static inline void ir_instr_reaching_expressions_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_expressions).out[blockNumber] = value;
}

static inline JITNUINT ir_instr_reaching_expressions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_expressions).in[blockNumber];
}

static inline void ir_instr_reaching_expressions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_expressions).in[blockNumber] = value;
}

static inline JITNUINT ir_instr_reaching_expressions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->reaching_expressions).kill[blockNumber];
}

static inline void ir_instr_reaching_expressions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->reachingExpressionsBlockNumbers > blockNumber);
    (inst->metadata->reaching_expressions).kill[blockNumber] = value;
}

static inline JITBOOLEAN ir_instr_reaching_expressions_reached_out_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->reaching_expressions).out == NULL) {
        return 0;
    }
    if (((inst->metadata->reaching_expressions).out[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

static inline JITBOOLEAN ir_instr_reaching_expressions_reached_in_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->reaching_expressions).in == NULL) {
        return 0;
    }
    if (((inst->metadata->reaching_expressions).in[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

JITNUINT ir_instr_available_expressions_gen_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->available_expressions).gen[blockNumber];
}

void ir_instr_available_expressions_gen_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->available_expressions).gen[blockNumber] = value;
}

JITNUINT ir_instr_available_expressions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->available_expressions).out[blockNumber];
}

void ir_instr_available_expressions_out_set (ir_method_t *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);

    (inst->metadata->available_expressions).out[blockNumber] = value;
}

JITNUINT ir_instr_available_expressions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->available_expressions).in[blockNumber];
}

void ir_instr_available_expressions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->available_expressions).in[blockNumber] = value;
}

JITNUINT ir_instr_available_expressions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->available_expressions).kill[blockNumber];
}

void ir_instr_available_expressions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->availableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->available_expressions).kill[blockNumber] = value;
}

JITBOOLEAN ir_instr_available_expressions_available_out_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->available_expressions).out == NULL) {
        return 0;
    }
    if (((inst->metadata->available_expressions).out[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

JITBOOLEAN ir_instr_available_expressions_available_in_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->available_expressions).in == NULL) {
        return 0;
    }
    if (((inst->metadata->available_expressions).in[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

static inline JITNUINT ir_instr_anticipated_expressions_gen_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->anticipated_expressions).gen[blockNumber];
}

static inline void ir_instr_anticipated_expressions_gen_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->anticipated_expressions).gen[blockNumber] = value;
}

static inline JITNUINT ir_instr_anticipated_expressions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->anticipated_expressions).out[blockNumber];
}

static inline void ir_instr_anticipated_expressions_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->anticipated_expressions).out[blockNumber] = value;
}

static inline JITNUINT ir_instr_anticipated_expressions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->anticipated_expressions).in[blockNumber];
}

static inline void ir_instr_anticipated_expressions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->anticipated_expressions).in[blockNumber] = value;
}

static inline JITNUINT ir_instr_anticipated_expressions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->anticipated_expressions).kill[blockNumber];
}

static inline void ir_instr_anticipated_expressions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->anticipatedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->anticipated_expressions).kill[blockNumber] = value;
}

static inline JITBOOLEAN ir_instr_anticipated_expressions_anticipated_out_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->anticipated_expressions).out == NULL) {
        return 0;
    }
    if (((inst->metadata->anticipated_expressions).out[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

static inline JITBOOLEAN ir_instr_anticipated_expressions_anticipated_in_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->anticipated_expressions).in == NULL) {
        return 0;
    }
    if (((inst->metadata->anticipated_expressions).in[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}


static inline JITNUINT ir_instr_postponable_expressions_use_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->postponable_expressions).use[blockNumber];
}

static inline void ir_instr_postponable_expressions_use_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->postponable_expressions).use[blockNumber] = value;
}

static inline JITNUINT ir_instr_postponable_expressions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->postponable_expressions).out[blockNumber];
}

static inline void ir_instr_postponable_expressions_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->postponable_expressions).out[blockNumber] = value;
}

static inline JITNUINT ir_instr_postponable_expressions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->postponable_expressions).in[blockNumber];
}

static inline void ir_instr_postponable_expressions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->postponable_expressions).in[blockNumber] = value;
}

static inline JITNUINT ir_instr_postponable_expressions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->postponable_expressions).kill[blockNumber];
}

static inline void ir_instr_postponable_expressions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->postponableExpressionsBlockNumbers > blockNumber);
    (inst->metadata->postponable_expressions).kill[blockNumber] = value;
}

static inline JITBOOLEAN ir_instr_postponable_expressions_postponable_out_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->postponable_expressions).out == NULL) {
        return 0;
    }
    if (((inst->metadata->postponable_expressions).out[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

static inline JITBOOLEAN ir_instr_postponable_expressions_postponable_in_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->postponable_expressions).in == NULL) {
        return 0;
    }
    if (((inst->metadata->postponable_expressions).in[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}



static inline JITNUINT ir_instr_used_expressions_use_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->used_expressions).use[blockNumber];
}

static inline void ir_instr_used_expressions_use_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->used_expressions).use[blockNumber] = value;
}

static inline JITNUINT ir_instr_used_expressions_out_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->used_expressions).out[blockNumber];
}

static inline void ir_instr_used_expressions_out_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->used_expressions).out[blockNumber] = value;
}

static inline JITNUINT ir_instr_used_expressions_in_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->used_expressions).in[blockNumber];
}

static inline void ir_instr_used_expressions_in_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->used_expressions).in[blockNumber] = value;
}

static inline JITNUINT ir_instr_used_expressions_kill_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->used_expressions).kill[blockNumber];
}

static inline void ir_instr_used_expressions_kill_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->usedExpressionsBlockNumbers > blockNumber);
    (inst->metadata->used_expressions).kill[blockNumber] = value;
}

static inline JITBOOLEAN ir_instr_used_expressions_used_out_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->used_expressions).out == NULL) {
        return 0;
    }
    if (((inst->metadata->used_expressions).out[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}

static inline JITBOOLEAN ir_instr_used_expressions_used_in_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->used_expressions).in == NULL) {
        return 0;
    }
    if (((inst->metadata->used_expressions).in[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}



static inline JITNUINT ir_instr_earliest_expressions_earliest_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->earliestExpressionsBlockNumbers > blockNumber);
    return (inst->metadata->earliest_expressions).earliestSet[blockNumber];
}

static inline void ir_instr_earliest_expressions_earliest_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->earliestExpressionsBlockNumbers > blockNumber);
    (inst->metadata->earliest_expressions).earliestSet[blockNumber] = value;
}

static inline JITBOOLEAN ir_instr_earliest_expressions_earliest_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->earliest_expressions).earliestSet == NULL) {
        return 0;
    }
    if (((inst->metadata->earliest_expressions).earliestSet[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}



static inline JITNUINT ir_instr_latest_placements_latest_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->latestPlacementsBlockNumbers > blockNumber);
    return (inst->metadata->latest_placements).latestSet[blockNumber];
}

static inline void ir_instr_latest_placements_latest_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->latestPlacementsBlockNumbers > blockNumber);
    (inst->metadata->latest_placements).latestSet[blockNumber] = value;
}

static inline JITBOOLEAN ir_instr_latest_placements_latest_is (void *method, ir_instruction_t *inst, JITUINT32 expressionID) {
    JITNUINT temp;
    JITUINT32 trigger;

    assert(method != NULL);
    assert(inst != NULL);

    temp = 0x1;
    trigger = (expressionID / (sizeof(JITNINT) * 8));
    temp = temp << (expressionID % (sizeof(JITNINT) * 8));

    if ((inst->metadata->latest_placements).latestSet == NULL) {
        return 0;
    }
    if (((inst->metadata->latest_placements).latestSet[trigger] & temp) != temp) {
        return 0;
    }
    return 1;
}



static inline JITNUINT ir_instr_partial_redundancy_elimination_addedInstructions_get (void *method, ir_instruction_t *inst, JITUINT32 blockNumber) {
    assert(inst != NULL);
    assert(method != NULL);
    assert(((ir_method_t *) method)->partialRedundancyEliminationBlockNumbers > blockNumber);
    return (inst->metadata->partial_redundancy_elimination).addedInstructions[blockNumber];

}

static inline void ir_instr_partial_redundancy_elimination_addedInstructions_set (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value) {
    assert(method != NULL);
    assert(inst != NULL);
    assert(((ir_method_t *) method)->partialRedundancyEliminationBlockNumbers > blockNumber);
    (inst->metadata->partial_redundancy_elimination).addedInstructions[blockNumber] = value;
}

static inline IR_ITEM_VALUE ir_instr_partial_redundancy_elimination_newVar_get (void *method, ir_instruction_t *inst) {
    assert(inst != NULL);
    assert(method != NULL);
    return (inst->metadata->partial_redundancy_elimination).newVar;

}

static inline void ir_instr_partial_redundancy_elimination_newVar_set (void *method, ir_instruction_t *inst, IR_ITEM_VALUE variable) {
    assert(method != NULL);
    assert(inst != NULL);
    (inst->metadata->partial_redundancy_elimination).newVar = variable;
}

static inline JITBOOLEAN ir_instr_partial_redundancy_elimination_analized_get (void *method, ir_instruction_t *inst) {
    assert(method != NULL);
    assert(inst != NULL);
    return (inst->metadata->partial_redundancy_elimination).analized;

}

static inline void ir_instr_partial_redundancy_elimination_analized_set (void *method, ir_instruction_t *inst, JITBOOLEAN analizedBoolean) {
    assert(method != NULL);
    assert(inst != NULL);
    (inst->metadata->partial_redundancy_elimination).analized = analizedBoolean;
}

static inline void ir_instr_liveness_alloc (ir_method_t * method, ir_instruction_t * inst) {

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->livenessBlockNumbers;

    if ((inst->metadata->liveness).use == NULL) {
        (inst->metadata->liveness).use = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->liveness).use = (JITNUINT *) dynamicReallocFunction( (inst->metadata->liveness).use, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->liveness).use, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->liveness).in == NULL) {
        (inst->metadata->liveness).in = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->liveness).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->liveness).in, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->liveness).in, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->liveness).out == NULL) {
        (inst->metadata->liveness).out = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->liveness).out = (JITNUINT *) dynamicReallocFunction( (inst->metadata->liveness).out, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->liveness).out, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_liveness_free (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->liveness).use != NULL) {
        freeFunction( (inst->metadata->liveness).use);
        (inst->metadata->liveness).use = NULL;
    }
    if ((inst->metadata->liveness).in != NULL) {
        freeFunction( (inst->metadata->liveness).in);
        (inst->metadata->liveness).in = NULL;
    }
    if ((inst->metadata->liveness).out != NULL) {
        freeFunction( (inst->metadata->liveness).out);
        (inst->metadata->liveness).out = NULL;
    }
    (inst->metadata->liveness).def = NOPARAM;
}

static inline void internal_ir_instr_reaching_definitions_alloc (ir_instruction_t * inst, JITUINT32 blocksNumber) {
    size_t sizeToUse;

    /* Assertions			*/
    assert(inst != NULL);

    /* Compute the size to use	*/
    sizeToUse = sizeof(JITNUINT) * blocksNumber;

    if ((inst->metadata->reaching_definitions).kill == NULL) {
        (inst->metadata->reaching_definitions).kill = (JITNUINT *) sharedAllocFunction(sizeToUse);
    } else {
        (inst->metadata->reaching_definitions).kill = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_definitions).kill, sizeToUse);
        memset((inst->metadata->reaching_definitions).kill, 0, sizeToUse);
    }
    if ((inst->metadata->reaching_definitions).gen == NULL) {
        (inst->metadata->reaching_definitions).gen = (JITNUINT *) sharedAllocFunction(sizeToUse);
    } else {
        (inst->metadata->reaching_definitions).gen = (JITNUINT *) dynamicReallocFunction((inst->metadata->reaching_definitions).gen, sizeToUse);
        memset((inst->metadata->reaching_definitions).gen, 0, sizeToUse);
    }
    if ((inst->metadata->reaching_definitions).in == NULL) {
        (inst->metadata->reaching_definitions).in = (JITNUINT *) sharedAllocFunction(sizeToUse);
    } else {
        (inst->metadata->reaching_definitions).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_definitions).in, sizeToUse);
        memset((inst->metadata->reaching_definitions).in, 0, sizeToUse);
    }
    if ((inst->metadata->reaching_definitions).out == NULL) {
        (inst->metadata->reaching_definitions).out = (JITNUINT *) sharedAllocFunction(sizeToUse);
    } else {
        (inst->metadata->reaching_definitions).out = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_definitions).out, sizeToUse);
        memset((inst->metadata->reaching_definitions).out, 0, sizeToUse);
    }

    /* Return			*/
    return;
}

static inline void ir_instr_reaching_definitions_free (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->reaching_definitions).kill != NULL) {
        freeFunction( (inst->metadata->reaching_definitions).kill);
        (inst->metadata->reaching_definitions).kill = NULL;
    }
    if ((inst->metadata->reaching_definitions).gen != NULL) {
        freeFunction( (inst->metadata->reaching_definitions).gen);
        (inst->metadata->reaching_definitions).gen = NULL;
    }
    if ((inst->metadata->reaching_definitions).in != NULL) {
        freeFunction( (inst->metadata->reaching_definitions).in);
        (inst->metadata->reaching_definitions).in = NULL;
    }
    if ((inst->metadata->reaching_definitions).out != NULL) {
        freeFunction( (inst->metadata->reaching_definitions).out);
        (inst->metadata->reaching_definitions).out = NULL;
    }
}

static inline void ir_instr_reaching_expressions_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->reachingExpressionsBlockNumbers;

    if ((inst->metadata->reaching_expressions).kill == NULL) {
        (inst->metadata->reaching_expressions).kill = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->reaching_expressions).kill = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_expressions).kill, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->reaching_expressions).kill, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->reaching_expressions).gen == NULL) {
        (inst->metadata->reaching_expressions).gen = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->reaching_expressions).gen = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_expressions).gen, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->reaching_expressions).gen, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->reaching_expressions).in == NULL) {
        (inst->metadata->reaching_expressions).in = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->reaching_expressions).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_expressions).in, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->reaching_expressions).in, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->reaching_expressions).out == NULL) {
        (inst->metadata->reaching_expressions).out = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->reaching_expressions).out = (JITNUINT *) dynamicReallocFunction( (inst->metadata->reaching_expressions).out, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->reaching_expressions).out, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_reaching_expressions_free (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->reaching_expressions).kill != NULL) {
        freeFunction( (inst->metadata->reaching_expressions).kill);
        (inst->metadata->reaching_expressions).kill = NULL;
    }
    if ((inst->metadata->reaching_expressions).gen != NULL) {
        freeFunction( (inst->metadata->reaching_expressions).gen);
        (inst->metadata->reaching_expressions).gen = NULL;
    }
    if ((inst->metadata->reaching_expressions).in != NULL) {
        freeFunction( (inst->metadata->reaching_expressions).in);
        (inst->metadata->reaching_expressions).in = NULL;
    }
    if ((inst->metadata->reaching_expressions).out != NULL) {
        freeFunction( (inst->metadata->reaching_expressions).out);
        (inst->metadata->reaching_expressions).out = NULL;
    }
}

void ir_instr_available_expressions_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->availableExpressionsBlockNumbers;

    if ((inst->metadata->available_expressions).kill == NULL) {
        (inst->metadata->available_expressions).kill = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->available_expressions).kill = (JITNUINT *) dynamicReallocFunction( (inst->metadata->available_expressions).kill, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->available_expressions).kill, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->available_expressions).gen == NULL) {
        (inst->metadata->available_expressions).gen = (JITNUINT *) sharedAllocFunction(sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->available_expressions).gen = (JITNUINT *) dynamicReallocFunction( (inst->metadata->available_expressions).gen, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->available_expressions).gen, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->available_expressions).in == NULL) {
        (inst->metadata->available_expressions).in = (JITNUINT *) sharedAllocFunction(sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->available_expressions).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->available_expressions).in, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->available_expressions).in, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->available_expressions).out == NULL) {
        (inst->metadata->available_expressions).out = (JITNUINT *) sharedAllocFunction(sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->available_expressions).out = (JITNUINT *) dynamicReallocFunction((inst->metadata->available_expressions).out, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->available_expressions).out, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

void ir_instr_available_expressions_free (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->available_expressions).kill != NULL) {
        freeFunction( (inst->metadata->available_expressions).kill);
        (inst->metadata->available_expressions).kill = NULL;
    }
    if ((inst->metadata->available_expressions).gen != NULL) {
        freeFunction( (inst->metadata->available_expressions).gen);
        (inst->metadata->available_expressions).gen = NULL;
    }
    if ((inst->metadata->available_expressions).in != NULL) {
        freeFunction( (inst->metadata->available_expressions).in);
        (inst->metadata->available_expressions).in = NULL;
    }
    if ((inst->metadata->available_expressions).out != NULL) {
        freeFunction( (inst->metadata->available_expressions).out);
        (inst->metadata->available_expressions).out = NULL;
    }
}

static inline void ir_instr_anticipated_expressions_alloc (ir_method_t * method, ir_instruction_t * inst) {

    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->anticipatedExpressionsBlockNumbers;

    if ((inst->metadata->anticipated_expressions).kill == NULL) {
        (inst->metadata->anticipated_expressions).kill = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->anticipated_expressions).kill = (JITNUINT *) dynamicReallocFunction( (inst->metadata->anticipated_expressions).kill, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->anticipated_expressions).kill, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->anticipated_expressions).gen == NULL) {
        (inst->metadata->anticipated_expressions).gen = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->anticipated_expressions).gen = (JITNUINT *) dynamicReallocFunction( (inst->metadata->anticipated_expressions).gen, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->anticipated_expressions).gen, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->anticipated_expressions).in == NULL) {
        (inst->metadata->anticipated_expressions).in = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->anticipated_expressions).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->anticipated_expressions).in, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->anticipated_expressions).in, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->anticipated_expressions).out == NULL) {
        (inst->metadata->anticipated_expressions).out = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->anticipated_expressions).out = (JITNUINT *) dynamicReallocFunction( (inst->metadata->anticipated_expressions).out, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->anticipated_expressions).out, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_anticipated_expressions_free (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->anticipated_expressions).kill != NULL) {
        freeFunction( (inst->metadata->anticipated_expressions).kill);
        (inst->metadata->anticipated_expressions).kill = NULL;
    }
    if ((inst->metadata->anticipated_expressions).gen != NULL) {
        freeFunction( (inst->metadata->anticipated_expressions).gen);
        (inst->metadata->anticipated_expressions).gen = NULL;
    }
    if ((inst->metadata->anticipated_expressions).in != NULL) {
        freeFunction( (inst->metadata->anticipated_expressions).in);
        (inst->metadata->anticipated_expressions).in = NULL;
    }
    if ((inst->metadata->anticipated_expressions).out != NULL) {
        freeFunction( (inst->metadata->anticipated_expressions).out);
        (inst->metadata->anticipated_expressions).out = NULL;
    }
}


static inline void ir_instr_postponable_expressions_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->postponableExpressionsBlockNumbers;

    if ((inst->metadata->postponable_expressions).kill == NULL) {
        (inst->metadata->postponable_expressions).kill = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->postponable_expressions).kill = (JITNUINT *) dynamicReallocFunction( (inst->metadata->postponable_expressions).kill, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->postponable_expressions).kill, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->postponable_expressions).use == NULL) {
        (inst->metadata->postponable_expressions).use = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->postponable_expressions).use = (JITNUINT *) dynamicReallocFunction( (inst->metadata->postponable_expressions).use, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->postponable_expressions).use, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->postponable_expressions).in == NULL) {
        (inst->metadata->postponable_expressions).in = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->postponable_expressions).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->postponable_expressions).in, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->postponable_expressions).in, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->postponable_expressions).out == NULL) {
        (inst->metadata->postponable_expressions).out = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->postponable_expressions).out = (JITNUINT *) dynamicReallocFunction( (inst->metadata->postponable_expressions).out, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->postponable_expressions).out, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_postponable_expressions_free (ir_method_t * method, ir_instruction_t * inst) {

    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->postponable_expressions).kill != NULL) {
        freeFunction( (inst->metadata->postponable_expressions).kill);
        (inst->metadata->postponable_expressions).kill = NULL;
    }
    if ((inst->metadata->postponable_expressions).use != NULL) {
        freeFunction( (inst->metadata->postponable_expressions).use);
        (inst->metadata->postponable_expressions).use = NULL;
    }
    if ((inst->metadata->postponable_expressions).in != NULL) {
        freeFunction( (inst->metadata->postponable_expressions).in);
        (inst->metadata->postponable_expressions).in = NULL;
    }
    if ((inst->metadata->postponable_expressions).out != NULL) {
        freeFunction( (inst->metadata->postponable_expressions).out);
        (inst->metadata->postponable_expressions).out = NULL;
    }
}

static inline void ir_instr_used_expressions_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->usedExpressionsBlockNumbers;

    if ((inst->metadata->used_expressions).kill == NULL) {
        (inst->metadata->used_expressions).kill = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->used_expressions).kill = (JITNUINT *) dynamicReallocFunction( (inst->metadata->used_expressions).kill, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->used_expressions).kill, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->used_expressions).use == NULL) {
        (inst->metadata->used_expressions).use = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->used_expressions).use = (JITNUINT *) dynamicReallocFunction( (inst->metadata->used_expressions).use, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->used_expressions).use, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->used_expressions).in == NULL) {
        (inst->metadata->used_expressions).in = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->used_expressions).in = (JITNUINT *) dynamicReallocFunction( (inst->metadata->used_expressions).in, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->used_expressions).in, 0, sizeof(JITNUINT) * blocksNumber);
    }
    if ((inst->metadata->used_expressions).out == NULL) {
        (inst->metadata->used_expressions).out = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->used_expressions).out = (JITNUINT *) dynamicReallocFunction( (inst->metadata->used_expressions).out, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->used_expressions).out, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_used_expressions_free (ir_method_t * method, ir_instruction_t * inst) {

    assert(method != NULL);
    assert(inst != NULL);

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->used_expressions).kill != NULL) {
        freeFunction( (inst->metadata->used_expressions).kill);
        (inst->metadata->used_expressions).kill = NULL;
    }
    if ((inst->metadata->used_expressions).use != NULL) {
        freeFunction( (inst->metadata->used_expressions).use);
        (inst->metadata->used_expressions).use = NULL;
    }
    if ((inst->metadata->used_expressions).in != NULL) {
        freeFunction( (inst->metadata->used_expressions).in);
        (inst->metadata->used_expressions).in = NULL;
    }
    if ((inst->metadata->used_expressions).out != NULL) {
        freeFunction( (inst->metadata->used_expressions).out);
        (inst->metadata->used_expressions).out = NULL;
    }
}

static inline void ir_instr_earliest_expressions_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->earliestExpressionsBlockNumbers;

    if ((inst->metadata->earliest_expressions).earliestSet == NULL) {
        (inst->metadata->earliest_expressions).earliestSet = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->earliest_expressions).earliestSet = (JITNUINT *) dynamicReallocFunction( (inst->metadata->earliest_expressions).earliestSet, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->earliest_expressions).earliestSet, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_earliest_expressions_free (ir_method_t * method, ir_instruction_t * inst) {
    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->earliest_expressions).earliestSet != NULL) {
        freeFunction( (inst->metadata->earliest_expressions).earliestSet);
        (inst->metadata->earliest_expressions).earliestSet = NULL;
    }
}

static inline void ir_instr_latest_placements_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->latestPlacementsBlockNumbers;

    if ((inst->metadata->latest_placements).latestSet == NULL) {
        (inst->metadata->latest_placements).latestSet = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->latest_placements).latestSet = (JITNUINT *) dynamicReallocFunction( (inst->metadata->latest_placements).latestSet, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->latest_placements).latestSet, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_latest_placements_free (ir_method_t * method, ir_instruction_t * inst) {

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->latest_placements).latestSet != NULL) {
        freeFunction( (inst->metadata->latest_placements).latestSet);
        (inst->metadata->latest_placements).latestSet = NULL;
    }
}

static inline void ir_instr_partial_redundancy_elimination_alloc (ir_method_t * method, ir_instruction_t * inst) {
    assert(method != NULL);
    assert(inst != NULL);

    JITUINT32 blocksNumber = method->partialRedundancyEliminationBlockNumbers;

    if ((inst->metadata->partial_redundancy_elimination).addedInstructions == NULL) {
        (inst->metadata->partial_redundancy_elimination).addedInstructions = (JITNUINT *) sharedAllocFunction( sizeof(JITNUINT) * blocksNumber);
    } else {
        (inst->metadata->partial_redundancy_elimination).addedInstructions = (JITNUINT *) dynamicReallocFunction( (inst->metadata->partial_redundancy_elimination).addedInstructions, sizeof(JITNUINT) * blocksNumber);
        memset((inst->metadata->partial_redundancy_elimination).addedInstructions, 0, sizeof(JITNUINT) * blocksNumber);
    }
}

static inline void ir_instr_partial_redundancy_elimination_free (ir_method_t * method, ir_instruction_t * inst) {

    if (inst->metadata == NULL){
        return ;
    }

    if ((inst->metadata->partial_redundancy_elimination).addedInstructions != NULL) {
        freeFunction( (inst->metadata->partial_redundancy_elimination).addedInstructions);
        (inst->metadata->partial_redundancy_elimination).addedInstructions = NULL;
    }
}

static inline void ir_instr_remove_data_dependency (ir_method_t *method, ir_instruction_t *depending, ir_instruction_t *toNode, JITUINT16 depType) {
    XanHashTable 	*from;
    XanHashTable 	*to;

    /* Assertions */
    assert(method != NULL);
    assert(depending != NULL);
    assert(toNode != NULL);
    assert(depending->ID < IRMETHOD_getInstructionsNumber(method));
    assert(toNode->ID < IRMETHOD_getInstructionsNumber(method));
    assert(IRMETHOD_doesInstructionBelongToMethod(method, depending));
    assert(IRMETHOD_doesInstructionBelongToMethod(method, toNode));

    /* Fetch the lists */
    from = (depending->metadata->data_dependencies).dependsFrom;
    to = (toNode->metadata->data_dependencies).dependingInsts;

    /* Remove from the from list. */
    if (from) {
        data_dep_arc_list_t *data_dep = xanHashTable_lookup(from, toNode);
        if (data_dep != NULL) {
            assert(data_dep->inst->ID < IRMETHOD_getInstructionsNumber(method));
            assert(data_dep->inst == toNode);
            data_dep->depType &= ~depType;
        }
    }

    /* Remove from the to list. */
    if (to) {
        data_dep_arc_list_t *data_dep_to = xanHashTable_lookup(to, depending);
        if (data_dep_to != NULL) {
            assert(data_dep_to->inst->ID < IRMETHOD_getInstructionsNumber(method));
            assert(data_dep_to->inst == depending);
            data_dep_to->depType &= ~depType;
        }
    }

    return ;
}

static inline JITBOOLEAN ir_instr_add_data_dependency (ir_method_t *method, ir_instruction_t *depending, ir_instruction_t *toNode, JITUINT16 depType, XanList *outsideInstructions) {
    JITUINT32	 	added;
    data_dep_arc_list_t     *data_dep;
    data_dep_arc_list_t     *data_dep_to;
    XanHashTable            *from;
    XanHashTable            *to;

    /* Assertions				*/
    assert(method != NULL);
    assert(depending != NULL);
    assert(toNode != NULL);
    assert(depending->ID < IRMETHOD_getInstructionsNumber(method));
    assert(toNode->ID < IRMETHOD_getInstructionsNumber(method));
    assert(IRMETHOD_doesInstructionBelongToMethod(method, depending));
    assert(IRMETHOD_doesInstructionBelongToMethod(method, toNode));

    /* Initialize the variables		*/
    added = JITFALSE;
    data_dep = NULL;
    data_dep_to = NULL;

    /* Fetch the lists			*/
    from = (depending->metadata->data_dependencies).dependsFrom;                ///< List of nodes from which depending node depends
    if (from == NULL) {
        from 							= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        (depending->metadata->data_dependencies).dependsFrom	= from;
    }
    assert(from != NULL);
    to = (toNode->metadata->data_dependencies).dependingInsts;                  ///< List of nodes depending from which
    if (to == NULL) {
        to 							= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        (toNode->metadata->data_dependencies).dependingInsts 	= to;
    }
    assert(to != NULL);

    /* Search the dependences.
     */
    data_dep 	= (data_dep_arc_list_t *) xanHashTable_lookup(from, toNode);
    data_dep_to 	= (data_dep_arc_list_t *) xanHashTable_lookup(to, depending);

    /* Update both the lists		*/
    added = JITFALSE;
    if (data_dep != NULL ) {
        assert(data_dep_to != NULL);
        assert(data_dep->inst == toNode);
        assert(data_dep_to->inst == depending);

        /* Update dependency    */
        if (    ((data_dep->depType & depType) != depType)      &&
                ((data_dep_to->depType & depType) != depType)   ) {
            added = JITTRUE;
        }
        data_dep->depType |= depType;
        data_dep_to->depType |= depType;
        if (outsideInstructions != NULL) {
            if (data_dep->outsideInstructions != NULL) {
                XanListItem *item3;
                item3 = xanList_first(outsideInstructions);
                while (item3 != NULL) {
                    ir_instruction_t *currentInst;
                    currentInst = item3->data;
                    assert(currentInst != NULL);
                    if (xanList_find(data_dep->outsideInstructions, currentInst) == NULL) {
                        xanList_append(data_dep->outsideInstructions, currentInst);
                    }
                    item3 = item3->next;
                }
            } else {
                data_dep->outsideInstructions = xanList_cloneList(outsideInstructions);
            }
            if (data_dep_to->outsideInstructions != NULL) {
                XanListItem *item3;
                item3 = xanList_first(outsideInstructions);
                while (item3 != NULL) {
                    ir_instruction_t *currentInst;
                    currentInst = item3->data;
                    assert(currentInst != NULL);
                    if (xanList_find(data_dep->outsideInstructions, currentInst) == NULL) {
                        xanList_append(data_dep->outsideInstructions, currentInst);
                    }
                    item3 = item3->next;
                }
            } else {
                data_dep_to->outsideInstructions = xanList_cloneList(outsideInstructions);
            }
        }

    } else {

        /* Insert the dependency	*/
        assert(data_dep_to == NULL);
        data_dep = (data_dep_arc_list_t *) allocMemory( sizeof(data_dep_arc_list_t));
        data_dep->inst = toNode;
        data_dep->depType = depType;
        if (outsideInstructions != NULL) {
            data_dep->outsideInstructions = xanList_cloneList(outsideInstructions);
        }
        assert(data_dep->inst->ID < IRMETHOD_getInstructionsNumber(method));
        xanHashTable_insert(from, data_dep->inst, data_dep);

        data_dep = (data_dep_arc_list_t *) allocMemory( sizeof(data_dep_arc_list_t));
        data_dep->inst = depending;
        data_dep->depType = depType;
        if (outsideInstructions != NULL) {
            data_dep->outsideInstructions = xanList_cloneList(outsideInstructions);
        }
        assert(data_dep->inst->ID < IRMETHOD_getInstructionsNumber(method));
        xanHashTable_insert(to, data_dep->inst, data_dep);

        added = JITTRUE;
    }
    assert(IRMETHOD_isInstructionDataDependent(method, depending, toNode));

    return added;
}

XanList * IRMETHOD_getDataDependencesToInstruction (ir_method_t *method, ir_instruction_t *toInst) {
    XanList			*l;

    if (toInst->metadata == NULL) {
        return NULL;
    }
    if (toInst->metadata->data_dependencies.dependingInsts == NULL) {
        return NULL;
    }
    if (xanHashTable_elementsInside(toInst->metadata->data_dependencies.dependingInsts) == 0) {
        return NULL;
    }

    /* Transform the table to a list.
     */
    l	= xanHashTable_toList((toInst->metadata->data_dependencies).dependingInsts);

    return l;
}

XanList * IRMETHOD_getDataDependencesFromInstruction (ir_method_t *method, ir_instruction_t *fromInst) {
    XanList			*l;

    if (fromInst->metadata == NULL) {
        return NULL;
    }
    if (fromInst->metadata->data_dependencies.dependsFrom == NULL) {
        return NULL;
    }
    if (xanHashTable_elementsInside(fromInst->metadata->data_dependencies.dependsFrom) == 0) {
        return NULL;
    }

    /* Transform the table to a list.
     */
    l	= xanHashTable_toList((fromInst->metadata->data_dependencies).dependsFrom);

    return l;
}

static inline void ir_instr_init (ir_method_t * method, ir_instruction_t * instr, JITBOOLEAN allocateMetadata) {

    /* Assertions.
     */
    assert(method != NULL);
    assert(instr != NULL);

    /* Initialize the instruction.
     */
    memset(instr, 0, sizeof(ir_instruction_t));
    instr->ID = -1;
    (instr->result).internal_type = NOPARAM;
    (instr->result).type = NOPARAM;
    (instr->param_1).type = NOPARAM;
    (instr->param_1).internal_type = NOPARAM;
    (instr->param_2).type = NOPARAM;
    (instr->param_2).internal_type = NOPARAM;
    (instr->param_3).type = NOPARAM;
    (instr->param_3).internal_type = NOPARAM;

    /* Initialize the metadata.
     */
    if (allocateMetadata) {

        /* Allocate the metadata		*/
        instr->metadata = sharedAllocFunction(sizeof(ir_instruction_metadata_t));

        /* Fill up the metadata			*/
        if (method->livenessBlockNumbers > 0 ) {
            ir_instr_liveness_alloc(method, instr);
        }
        if (method->reachingDefinitionsBlockNumbers > 0 ) {
            internal_ir_instr_reaching_definitions_alloc(instr, method->reachingDefinitionsBlockNumbers);
        }
        if (method->reachingExpressionsBlockNumbers > 0 ) {
            ir_instr_reaching_expressions_alloc(method, instr);
        }
        if (method->availableExpressionsBlockNumbers > 0 ) {
            ir_instr_available_expressions_alloc(method, instr);
        }
        if (method->anticipatedExpressionsBlockNumbers > 0 ) {
            ir_instr_anticipated_expressions_alloc(method, instr);
        }
        if (method->postponableExpressionsBlockNumbers > 0 ) {
            ir_instr_postponable_expressions_alloc(method, instr);
        }
        if (method->usedExpressionsBlockNumbers > 0 ) {
            ir_instr_used_expressions_alloc(method, instr);
        }
        if (method->earliestExpressionsBlockNumbers > 0 ) {
            ir_instr_earliest_expressions_alloc(method, instr);
        }
        if (method->latestPlacementsBlockNumbers > 0 ) {
            ir_instr_latest_placements_alloc(method, instr);
        }
        if (method->partialRedundancyEliminationBlockNumbers > 0 ) {
            ir_instr_partial_redundancy_elimination_alloc(method, instr);
        }
    }

    instr->executionProbability = -1;

    return ;
}

static inline ir_instruction_t * ir_instr_make (ir_method_t * method) {
    ir_instruction_t        *instr;
    JITBOOLEAN allocateMetadata;

    /* Assertions.
     */
    assert(method != NULL);

    /* Initialize the variables.
     */
    allocateMetadata = JITFALSE;

    /* Allocate a new instruction.
     */
    method->valid_optimization = 0;
    instr = (ir_instruction_t *) sharedAllocFunction( sizeof(ir_instruction_t));
    assert(instr != NULL);

    /* Check whether we need to allocate metadata or not.
     */
    if (    ((method->instructions[0] != NULL) && (method->instructions[0]->metadata != NULL))      	||
            ((method->instructions[1] != NULL) && (method->instructions[1]->metadata != NULL))      	) {
        allocateMetadata = JITTRUE;
    } else {
        allocateMetadata = JITFALSE;
    }

    ir_instr_init(method, instr, allocateMetadata);

    return instr;
}

static inline void ir_instr_check_new_space (ir_method_t *method) {
    JITUINT32 newSize;

    /* Assertions.
     */
    assert(method != NULL);
    assert(method->instructions != NULL);
    assert(method->instructionsTop <= method->instructionsAllocated);

    /* Check if we need to either grow or shrink the memory.
     */
    if (method->instructionsTop == method->instructionsAllocated) {

        /* Grow the memory.
         */
        newSize = method->instructionsAllocated + GROW_BUFFERED_INSTRUCTIONS;
        method->instructions = dynamicReallocFunction(method->instructions, sizeof(ir_instruction_t *) * newSize);
        method->instructionsAllocated = newSize;

    } else if (method->instructionsTop < (method->instructionsAllocated - GROW_BUFFERED_INSTRUCTIONS)){

        /* Increase the memory.
         */
        newSize = method->instructionsTop + SHRINK_BUFFERED_INSTRUCTIONS;
        method->instructions = dynamicReallocFunction(method->instructions, sizeof(ir_instruction_t *) * newSize);
        method->instructionsAllocated = newSize;
    }
    assert(method->instructionsTop < method->instructionsAllocated);

    /* Check the memory.
     */
#ifdef DEBUG
#ifdef HEAVY_DEBUG
    assert(method->instructions != NULL);
    assert(method->instructionsTop < method->instructionsAllocated);
    JITUINT32 count;
    JITUINT32 count2;
    for (count = START_IR_INSTRUCTIONS_ID; count < method->instructionsTop; count++) {
        ir_instruction_t        *inst;
        ir_instruction_t        *inst2;
        inst = method->instructions[count];
        assert(inst != NULL);
        assert(inst->ID == count);
        for (count2 = count + 1; count2 < method->instructionsTop; count2++) {
            inst2 = method->instructions[count2];
            assert(inst2 != NULL);
            assert(inst2->ID == count2);
            assert(inst != inst2);
        }
    }
#endif
#endif
}

static inline ir_instruction_t * ir_instr_insert (ir_method_t * method, ir_instruction_t * instr) {

    /* Assertions			*/
    assert(method != NULL);
    assert(method->instructions != NULL);
    assert(instr != NULL);
    assert(method->instructionsTop <= method->instructionsAllocated);
#ifdef DEBUG
    JITUINT32 count;
    for (count = 0; count < method->instructionsTop; count++) {
        assert(method->instructions[count] != instr);
    }

#endif
    PDEBUG("LIBILJITIR: ir_instr_insert: Start\n");

    ir_instr_check_new_space(method);
    method->instructions[method->instructionsTop] = instr;
    instr->ID = method->instructionsTop;
    (method->instructionsTop)++;

#ifdef DEBUG
    ir_instr_check_new_space(method);
#endif

    update_exit_node_ID(method);

    PDEBUG("LIBILJITIR: ir_instr_insert: Exit\n");
    return instr;
}

static inline ir_instruction_t * ir_instr_insert_before (ir_method_t * method, ir_instruction_t * instr, ir_instruction_t * instr_prev) {
    JITUINT32 count;
    JITUINT32 freeSpace;
    JITUINT32 delta;

    /* Assertions			*/
    assert(method != NULL);
    assert(method->instructions != NULL);
    assert(instr != NULL);
#ifdef DEBUG
    if (    (instr_prev != NULL)                    &&
            (instr_prev != method->exitNode)        ) {
        assert(method->instructions[instr_prev->ID] == instr_prev);
    }
    for (count = 0; count < method->instructionsTop; count++) {
        assert(method->instructions[count] != instr);
    }

#endif
    PDEBUG("LIBILJITIR: ir_instr_insert_before: Start\n");

    if (instr_prev == NULL) {
        PDEBUG("LIBILJITIR: ir_instr_insert_before: Exit\n");
        return ir_instr_insert(method, instr);
    }
    assert(instr_prev != NULL);
    assert(instr_prev->ID <= method->instructionsTop);

    /* Fetch the previous item      */
    ir_instr_check_new_space(method);
    freeSpace = instr_prev->ID;
    delta = method->instructionsTop - instr_prev->ID;

    /* Move the instructions after	*
     * the previous			*/
    if (delta > 0) {

        memmove(&(method->instructions[freeSpace + 1]), &(method->instructions[freeSpace]), sizeof(ir_instruction_t *) * delta);
        assert(method->instructions[freeSpace + 1] == instr_prev);
        for (count = freeSpace + 1; count < method->instructionsTop + 1; count++) {
            ((method->instructions[count])->ID)++;
        }
    }

    /* Insert the new IR instruction */
    method->instructions[freeSpace] = instr;
    instr->ID = freeSpace;
    (method->instructionsTop)++;
#ifdef DEBUG
    ir_instr_check_new_space(method);
#endif

    update_exit_node_ID(method);

    PDEBUG("LIBILJITIR: ir_instr_insert_before: Exit\n");
    return instr;
}

static inline IR_ITEM_VALUE ir_instr_new_variable_ID (ir_method_t *method) {
    IR_ITEM_VALUE current;

    /* Assertions			*/
    assert(method != NULL);

    /* Initialize the variables     */
    current = 0;

    current = ir_method_var_max_get(method);
    ir_method_var_max_set(method, current + 1);
    assert(ir_method_var_max_get(method) == (current + 1));

    /* Return                       */
    return current;
}

static inline IR_ITEM_VALUE ir_method_label_max_get (ir_method_t *method) {
    IR_ITEM_VALUE max;
    ir_instruction_t        *inst;
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    /* Initialize the variables     */
    max = 0;

    /* Search the maximum label     */
    for (count = 0; count < ir_instr_count(method); count++) {
        inst = ir_instr_get_by_pos(method, count);
        assert(inst != NULL);
        if (((inst->param_1).type == IRLABELITEM) && ((inst->param_1).value.v > max)) {
            max = (inst->param_1).value.v;
        }
        if (((inst->param_2).type == IRLABELITEM) && ((inst->param_2).value.v > max)) {
            max = (inst->param_2).value.v;
        }
        if (((inst->param_3).type == IRLABELITEM) && ((inst->param_3).value.v > max)) {
            max = (inst->param_3).value.v;
        }
    }

    /* Return                       */
    return max;
}

static inline IR_ITEM_VALUE ir_instr_new_label_ID (ir_method_t *method) {
    IR_ITEM_VALUE win;

    /* Assertions			*/
    assert(method != NULL);

    /* Search the maximum label     */
    win = ir_method_label_max_get(method);

    /* Return                       */
    return win + 1;
}

static inline void ir_instr_erase (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 ID;
    ir_instruction_metadata_t       *metadata;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Get the metadata		*/
    metadata = inst->metadata;

    /* Erase the instruction        */
    ID = inst->ID;
    inst->metadata = NULL;

    /* Delete the parameters of the call	*
     * instruction				*/
    ir_instrcall_pars_delete(method, inst);

    /* Initialize the instruction	*/
    ir_instr_init(method, inst, JITFALSE);
    inst->ID = ID;
    inst->metadata = metadata;

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    /* Return                       */
    return;
}

static inline void update_exit_node_ID (ir_method_t *method) {
    assert(method != NULL);

    if (method->exitNode != NULL) {
        method->exitNode->ID = method->instructionsTop;
    }
}

static inline ir_instruction_t * ir_instr_new (ir_method_t * method) {
    ir_instruction_t * instr;

    /* Assertions			*/
    assert(method != NULL);
    PDEBUG("LIBILJITIR: ir_instr_new: Start\n");

    method->modified = JITTRUE;
    instr = ir_instr_make(method);
    ir_instr_insert(method, instr);

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    PDEBUG("LIBILJITIR: ir_instr_new: Exit\n");
    return instr;
}

static inline ir_instruction_t * ir_instr_new_after (ir_method_t * method, ir_instruction_t * instr_succ) {
    ir_instruction_t * instr;

    /* Assertions			*/
    assert(method != NULL);

    instr = ir_instr_make(method);
    ir_instr_insert(method, instr);
    if (instr_succ != NULL) {
        ir_instr_move_after(method, instr, instr_succ);
    }

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    /* Return			*/
    return instr;
}

static inline ir_instruction_t * ir_instr_new_before (ir_method_t * method, ir_instruction_t * instr_prev) {
    ir_instruction_t * instr;

    /* Assertions			*/
    assert(method != NULL);
    PDEBUG("LIBILJITIR: ir_instr_new_before: Start\n");

    instr = ir_instr_make(method);
    ir_instr_insert_before(method, instr, instr_prev);

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    /* Return			*/
    PDEBUG("LIBILJITIR: ir_instr_new_before: Exit\n");
    return instr;
}

static inline ir_instruction_t * ir_instr_clone (ir_method_t *method, ir_instruction_t *instToClone) {
    ir_instruction_t        *instr;
    XanListItem             *item;

    /* Assertions			*/
    assert(method != NULL);
    assert(instToClone != NULL);

    /* Allocate memory for the new	*
     * instruction			*/
    instr = ir_instr_make(method);

    /* Move the instruction at the	*
     * end				*/
    ir_instr_insert(method, instr);

    /* Copy the instruction		*/
    instr->type = instToClone->type;
    memcpy(&(instr->param_1), &(instToClone->param_1), sizeof(ir_item_t));
    memcpy(&(instr->param_2), &(instToClone->param_2), sizeof(ir_item_t));
    memcpy(&(instr->param_3), &(instToClone->param_3), sizeof(ir_item_t));
    memcpy(&(instr->result), &(instToClone->result), sizeof(ir_item_t));
    instr->byte_offset = instToClone->byte_offset;
    if (instToClone->callParameters != NULL) {
        instr->callParameters = xanList_new(sharedAllocFunction, freeFunction, NULL);
        item = xanList_first(instToClone->callParameters);
        while (item != NULL) {
            ir_item_t       *elem;
            ir_item_t       *newElem;
            elem = (ir_item_t *) item->data;
            newElem = sharedAllocFunction(sizeof(ir_item_t));
            memcpy(newElem, elem, sizeof(ir_item_t));
            xanList_append(instr->callParameters, newElem);
            item = item->next;
        }
    }

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    /* Sanity checks		*/
    assert(IRMETHOD_doesInstructionBelongToMethod(method, instr));

    /* Return			*/
    return instr;
}

static inline void ir_instr_data_delete (ir_method_t *method, ir_instruction_t *instr, JITBOOLEAN deleteMetadata) {

    /* Assertions				*/
    assert(method != NULL);
    assert(instr != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, instr));

    /* Check if there are metadata		*/
    if (instr->metadata == NULL) {
        return;
    }

    /* Delete the data flow data		*/
    freeFunction((instr->metadata->liveness).use);
    freeFunction((instr->metadata->liveness).in);
    freeFunction((instr->metadata->liveness).out);

    freeFunction((instr->metadata->reaching_definitions).kill);
    freeFunction((instr->metadata->reaching_definitions).gen);
    freeFunction((instr->metadata->reaching_definitions).in);
    freeFunction((instr->metadata->reaching_definitions).out);

    freeFunction((instr->metadata->reaching_expressions).kill);
    freeFunction((instr->metadata->reaching_expressions).gen);
    freeFunction((instr->metadata->reaching_expressions).in);
    freeFunction((instr->metadata->reaching_expressions).out);

    freeFunction((instr->metadata->available_expressions).kill);
    freeFunction((instr->metadata->available_expressions).gen);
    freeFunction((instr->metadata->available_expressions).in);
    freeFunction((instr->metadata->available_expressions).out);

    freeFunction((instr->metadata->anticipated_expressions).kill);
    freeFunction((instr->metadata->anticipated_expressions).gen);
    freeFunction((instr->metadata->anticipated_expressions).in);
    freeFunction((instr->metadata->anticipated_expressions).out);

    freeFunction((instr->metadata->postponable_expressions).kill);
    freeFunction((instr->metadata->postponable_expressions).use);
    freeFunction((instr->metadata->postponable_expressions).in);
    freeFunction((instr->metadata->postponable_expressions).out);

    freeFunction((instr->metadata->used_expressions).kill);
    freeFunction((instr->metadata->used_expressions).use);
    freeFunction((instr->metadata->used_expressions).in);
    freeFunction((instr->metadata->used_expressions).out);

    freeFunction((instr->metadata->earliest_expressions).earliestSet);

    freeFunction((instr->metadata->latest_placements).latestSet);

    freeFunction((instr->metadata->partial_redundancy_elimination).addedInstructions);

    /* Destroy control dependences.
     */
    if (instr->metadata->controlDependencies != NULL) {
        xanList_destroyList(instr->metadata->controlDependencies);
        instr->metadata->controlDependencies = NULL;
    }

    /* Destroy data dependences.
     */
    IRMETHOD_destroyInstructionDataDependences(method, instr);

    /* Free the metadata			*/
    if (deleteMetadata) {
        freeFunction(instr->metadata);
        instr->metadata = NULL;
    }

    /* Return				*/
    return;
}

static inline void ir_instr_delete (ir_method_t *method, ir_instruction_t *instr) {
    JITUINT32 	delta;
    JITUINT32 	count;

    /* Assertions.
     */
    assert(method != NULL);
    assert(method->instructions != NULL);
    assert(instr != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, instr));

    /* Delete metadata.
     */
    ir_instr_data_delete(method, instr, JITTRUE);
 
    /* Remove the pointer to the instruction from the array.
     */
    method->instructions[instr->ID] = NULL;

    /* Fill up the hole.
     */
    delta = method->instructionsTop - instr->ID - 1;
    if (delta > 0) {
        memmove(&(method->instructions[instr->ID]), &(method->instructions[instr->ID + 1]), sizeof(ir_instruction_t *) * delta);
        for (count = instr->ID; count < method->instructionsTop - 1; count++) {
            ((method->instructions[count])->ID)--;
        }
    }
    (method->instructionsTop)--;
#ifdef DEBUG
    ir_instr_check_new_space(method);
#endif

    /* Delete parameters.
     */
    ir_instr_par_delete(&(instr->param_1));
    ir_instr_par_delete(&(instr->param_2));
    ir_instr_par_delete(&(instr->param_3));

    /* Delete the parameters of the call instruction.
     */
    ir_instrcall_pars_delete(method, instr);

    /* Delete the instruction.
     */
    freeFunction(instr);

    /* Update the exit node ID.
     */
    update_exit_node_ID(method);

    /* Shrink the allocated memory.
     */
    ir_instr_check_new_space(method);

    /* Update the valid optimization information.
     */
    method->valid_optimization = 0;

    return;
}

static inline void ir_instr_delete_all (ir_method_t *method) {

    /* Assertions.
     */
    assert(method != NULL);
#ifdef DEBUG
    for (JITUINT32 count=0; count < ir_instr_count(method); count++) {
        ir_instruction_t	*inst;
        inst	= ir_instr_get_by_pos(method, count);
        assert(inst != NULL);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));
    }
#endif

    /* Check if there are instructions.
     */
    if (method->instructionsTop == START_IR_INSTRUCTIONS_ID) {
        return;
    }

    /* Delete IR instructions.
     */
    while (method->instructionsTop > START_IR_INSTRUCTIONS_ID) {
        ir_instruction_t	*inst;
        inst	= ir_instr_get_last(method);
        assert(inst != method->exitNode);
        ir_instr_delete(method, inst);
    }
    assert(method->instructionsTop == START_IR_INSTRUCTIONS_ID);

    /* Reset the buffer of instructions.
     */
    memset(method->instructions, 0, sizeof(ir_instruction_t *) * method->instructionsAllocated);

    return;
}

static inline JITINT32 ir_instr_move_before (ir_method_t * method, ir_instruction_t * instr, ir_instruction_t * instr_prev) {
    JITUINT32 delta;
    JITUINT32 count;
    JITUINT32 freeSpace;

    /* Assertions			*/
    assert(instr != NULL);
    assert(method != NULL);
    assert(instr_prev != NULL);
    assert(instr != instr_prev);
    assert(instr->ID < method->instructionsTop);
    assert(instr_prev->ID <= method->instructionsTop);
    assert(method->instructions[instr->ID] != NULL);
    assert(method->instructions[instr->ID] == instr);
    PDEBUG("LIBILJITIR: ir_instr_move_before: Start\n");

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    if (instr_prev->ID == method->instructionsTop) {
        ir_instruction_t *last;
        //Moving at the end
        last = ir_instr_get_last(method);
        if (instr != last) {
            ir_instr_move_after(method, instr, last);
        }
        return 0;
    }
    assert(method->instructions[instr_prev->ID] != NULL);
    assert(method->instructions[instr_prev->ID] == instr_prev || instr_prev->type == IREXITNODE);

    /* Delete the IR instructions	*/
    method->instructions[instr->ID] = NULL;
    delta = method->instructionsTop - instr->ID - 1;
    memmove(&(method->instructions[instr->ID]), &(method->instructions[instr->ID + 1]), sizeof(ir_instruction_t *) * delta);
    for (count = instr->ID; count < method->instructionsTop - 1; count++) {
        ((method->instructions[count])->ID)--;
    }
    (method->instructionsTop)--;

    /* Insert the new IR instruction */
    freeSpace = instr_prev->ID;
    delta = method->instructionsTop - freeSpace;
    memmove(&(method->instructions[freeSpace + 1]), &(method->instructions[freeSpace]), sizeof(ir_instruction_t *) * delta);
    assert(method->instructions[freeSpace] == instr_prev);
    assert(method->instructions[freeSpace + 1] == instr_prev);
    for (count = freeSpace + 1; count < method->instructionsTop + 1; count++) {
        ((method->instructions[count])->ID)++;
    }
    method->instructions[freeSpace] = instr;
    instr->ID = freeSpace;
    (method->instructionsTop)++;
#ifdef DEBUG
    ir_instr_check_new_space(method);
#endif

    /* Return			*/
    PDEBUG("LIBILJITIR: ir_instr_move_before: Exit\n");
    return 0;
}

static inline JITINT32 ir_instr_move_after (ir_method_t * method, ir_instruction_t * instr, ir_instruction_t * instr_succ) {
    ir_instruction_t        *temp;

    /* Assertions			*/
    assert(instr != NULL);
    assert(method != NULL);
    assert(instr_succ != NULL);
    assert(instr != instr_succ);
    assert(instr->ID < method->instructionsTop);
    assert(instr_succ->ID < method->instructionsTop);
    assert(method->instructions[instr->ID] != NULL);
    assert(method->instructions[instr->ID] == instr);
    assert(method->instructions[instr_succ->ID] != NULL);
    assert(method->instructions[instr_succ->ID] == instr_succ);

    /* Check if we need to make some*
    * movement                     */
    if (instr->ID == (instr_succ->ID + 1)) {
        return 0;
    }

    /* Update the valid optimization*
     * information			*/
    method->valid_optimization = 0;

    /* Move the IR instructions	*/
    if (instr_succ->ID == method->instructionsTop - 1) {
        temp = ir_instr_new(method);
        assert(temp != NULL);
        ir_instr_move_before(method, instr, temp);
        ir_instr_delete(method, temp);
    } else {
        temp = method->instructions[instr_succ->ID + 1];
        assert(temp != NULL);
        assert(temp->ID < method->instructionsTop);
        ir_instr_move_before(method, instr, temp);
    }
#ifdef DEBUG
    assert(instr->ID == (instr_succ->ID + 1));
    ir_instr_check_new_space(method);
#endif

    /* Return			*/
    return 0;
}

static inline JITUINT32 ir_instr_count (ir_method_t * method) {
    JITINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    count = method->instructionsTop;

    return count;
}

static inline ir_instruction_t * ir_instr_get_first (ir_method_t *method) {
    ir_instruction_t        *instr;

    /* Assertions			*/
    assert(method != NULL);
    assert(method->instructions != NULL);
    PDEBUG("LIBILJITIR: ir_instr_get_first: Start\n");

    instr = NULL;
    if (ir_instr_count(method) > 0) {
        instr = method->instructions[START_IR_INSTRUCTIONS_ID];
    }

    PDEBUG("LIBILJITIR: ir_instr_get_first: Exit\n");
    return instr;
}

static inline ir_instruction_t * ir_instr_get_next (ir_method_t *method, ir_instruction_t *instr) {
    ir_instruction_t        *irInstr;

    /* Assertions			*/
    assert(method != NULL);
    PDEBUG("LIBILJITIR: ir_instr_get_next: Start\n");

    irInstr = NULL;
    if (instr == NULL) {
        irInstr = ir_instr_get_first(method);
        PDEBUG("LIBILJITIR: ir_instr_get_next: Exit\n");
        return irInstr;
    }
    assert(instr != NULL);
    if (instr == ir_instr_get_last(method)) {
        irInstr = method->exitNode;
    } else if (instr == method->exitNode) {
        return NULL;
    } else {
        assert(instr->ID + 1 < method->instructionsTop);
        irInstr = method->instructions[instr->ID + 1];
        assert(irInstr != NULL);
        assert(irInstr->ID < method->instructionsTop);
    }

    PDEBUG("LIBILJITIR: ir_instr_get_next: Exit\n");
    return irInstr;
}

static inline ir_instruction_t * ir_instr_get_prev (ir_method_t *method, ir_instruction_t *instr) {
    ir_instruction_t        *irInstr;

    assert(method != NULL);
    PDEBUG("LIBILJITIR: ir_instr_get_prev: Start\n");

    if (instr == NULL) {
        PDEBUG("LIBILJITIR: ir_instr_get_prev: Exit\n");
        return NULL;
    }
    assert(instr->ID <= method->instructionsTop);
    if (instr->ID == 0) {
        PDEBUG("LIBILJITIR: ir_instr_get_prev: Exit\n");
        return NULL;
    }
    irInstr = method->instructions[instr->ID - 1];
    assert(irInstr != NULL);
    assert(irInstr->ID < method->instructionsTop);

    PDEBUG("LIBILJITIR: ir_instr_get_prev: Exit\n");
    return irInstr;
}

static inline ir_instruction_t * ir_instr_get_last (ir_method_t *method) {
    ir_instruction_t        *instr;

    /* Assertions				*/
    assert(method != NULL);
    assert(method->exitNode != NULL);
    PDEBUG("LIBILJITIR: ir_instr_get_last: Start\n");

    if (method->instructionsTop == 0) {
        PDEBUG("LIBILJITIR: ir_instr_get_last: Exit\n");
        return method->exitNode;
    }
    instr = method->instructions[method->instructionsTop - 1];
    assert(instr != NULL);
    assert(instr->ID < method->instructionsTop);

    PDEBUG("LIBILJITIR: ir_instr_get_last: Exit\n");
    return instr;
}

static inline ir_instruction_t * ir_instr_get_by_pos (ir_method_t * method, JITUINT32 position) {
    ir_instruction_t        *irInstr;

    /* Assertions			*/
    assert(method != NULL);
    assert(position <= method->instructionsTop);
    PDEBUG("LIBILJITIR: ir_instr_get_by_pos: Start\n");

    if (position == method->instructionsTop) {
        return method->exitNode;
    } else if (position > method->instructionsTop) {
        PDEBUG("LIBILJITIR: ir_instr_get_by_pos: Exit\n");
        return NULL;
    }
    irInstr = method->instructions[position];
    assert(irInstr != NULL);
    assert(irInstr->ID < method->instructionsTop);

    PDEBUG("LIBILJITIR: ir_instr_get_by_pos: Exit\n");
    return irInstr;
}

static inline void ir_instr_type_set (ir_instruction_t *instr, JITUINT32 type) {

    /* Assertions                   */
    assert(instr != NULL);

    instr->type = type;
    return;
}

static inline JITUINT16 ir_instr_type_get (ir_instruction_t *instr) {

    /* Assertions                   */
    assert(instr != NULL);

    return instr->type;
}

JITBOOLEAN IRMETHOD_canInstructionPerformAMemoryLoad (ir_instruction_t *inst) {
    if (_IRMETHOD_isACallInstruction(inst)) {
        return JITTRUE;
    }

    switch (inst->type) {
        case IRLOADREL:
        case IRMEMCPY:
        case IRMEMCMP:
            return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRMETHOD_canInstructionPerformAMemoryStore (ir_instruction_t *inst) {
    if (_IRMETHOD_isACallInstruction(inst)) {
        return JITTRUE;
    }
    switch (inst->type) {
        case IRNEWOBJ:
        case IRNEWARR:
        case IRFREEOBJ:
        case IRINITMEMORY:
        case IRMEMCPY:
        case IRALLOC:
        case IRALLOCALIGN:
        case IRCALLOC:
        case IRALLOCA:
        case IRREALLOC:
        case IRSTOREREL:
            return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isACallInstruction (ir_instruction_t *inst) {
    return _IRMETHOD_isACallInstruction(inst);
}

static inline JITBOOLEAN _IRMETHOD_isACallInstruction (ir_instruction_t *inst) {
    /* Assertions					*/
    assert(inst != NULL);

    switch (inst->type) {
        case IRCALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
        case IRNATIVECALL:
            return JITTRUE;
    }

    return JITFALSE;
}

static inline JITINT32 ir_instr_is_a_jump_instruction (ir_method_t * method, ir_instruction_t *inst) {

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);

    /* Check if the instruction is a call.
     */
    if (_IRMETHOD_isACallInstruction(inst)) {
        return JITTRUE;
    }

    /* Check if the instruction is a branch.
     */
    if (IRMETHOD_isAJumpInstructionType(inst->type)) {
        return JITTRUE;
    }

    /* Check if the instruction jumps for exceptions.
     */
    if (_IRMETHOD_mayInstructionOfTypeThrowException(inst->type)) {
        return JITTRUE;
    }
    switch (inst->type) {
        case IRTHROW:
        case IRCALLFILTER:
        case IRENDFILTER:
            return JITTRUE;
    }

    /* Check if the instruction is the last one.
     */
    if (inst->type == IREXITNODE) {
        return JITTRUE;
    }

    /* Check consistency.
     */
#ifdef DEBUG
    assert(ir_instr_get_successor(method, inst, NULL, NULL, NULL) != NULL);
    assert(IRMETHOD_getSuccessorsNumber(method, inst) == 1);
    assert(ir_instr_get_next(method, inst) == ir_instr_get_successor(method, inst, NULL, NULL, NULL));
#endif

    return JITFALSE;
}

static inline JITUINT32 ir_instr_position_get (ir_instruction_t * inst) {

    /* Assertions					*/
    assert(inst != NULL);

    return inst->ID;
}

static inline JITFLOAT64 ir_instr_par1_fvalue_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_1).value.f;
}

static inline JITFLOAT64 ir_instr_par2_fvalue_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_2).value.f;
}

static inline JITFLOAT64 ir_instr_par3_fvalue_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_3).value.f;
}

static inline JITFLOAT64 ir_instr_result_fvalue_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->result).value.f;
}

static inline JITUINT64 ir_instr_par1_value_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_1).value.v;
}

static inline JITUINT64 ir_instr_par2_value_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_2).value.v;
}

static inline JITUINT64 ir_instr_par3_value_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_3).value.v;
}

static inline JITUINT32 ir_instr_par1_type_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_1).type;
}

static inline JITUINT32 ir_instr_par2_type_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_2).type;
}

static inline JITUINT32 ir_instr_par3_type_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return (instr->param_3).type;
}

static inline ir_item_t * ir_instr_par1_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return &(instr->param_1);
}

static inline ir_item_t * ir_instr_par2_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return &(instr->param_2);
}

static inline ir_item_t * ir_instr_par3_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return &(instr->param_3);
}

static inline ir_item_t * ir_instr_result_get (ir_instruction_t *instr) {
    assert(instr != NULL);
    return &(instr->result);
}

static inline ir_instruction_t * ir_instr_par1_cp (ir_instruction_t *to, ir_item_t *from) {

    /* Assertions                   */
    assert(from != NULL);
    assert(to != NULL);

    if (from != &(to->param_1)) {
        memcpy(&(to->param_1), from, sizeof(ir_item_t));
    }

    return to;
}

static inline ir_instruction_t * ir_instr_result_cp (ir_instruction_t *to, ir_item_t *from) {

    /* Assertions                   */
    assert(from != NULL);
    assert(to != NULL);

    if (from != &(to->result)) {
        memcpy(&(to->result), from, sizeof(ir_item_t));
    }

    return to;
}

static inline ir_instruction_t * ir_instr_par2_cp (ir_instruction_t *to, ir_item_t *from) {

    /* Assertions                   */
    assert(from != NULL);
    assert(to != NULL);

    if (from != &(to->param_2)) {
        memcpy(&(to->param_2), from, sizeof(ir_item_t));
    }

    return to;
}

static inline ir_instruction_t * ir_instr_par3_cp (ir_instruction_t *to, ir_item_t *from) {

    /* Assertions                   */
    assert(from != NULL);
    assert(to != NULL);

    if (from != &(to->param_3)) {
        memcpy(&(to->param_3), from, sizeof(ir_item_t));
    }

    return to;
}

static inline ir_instruction_t * ir_instr_par1_fvalue_set (ir_instruction_t *instr, JITFLOAT64 value) {
    assert(instr != NULL);
    (instr->param_1).value.f = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_par2_fvalue_set (ir_instruction_t *instr, JITFLOAT64 value) {
    assert(instr != NULL);
    (instr->param_2).value.f = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_par3_fvalue_set (ir_instruction_t *instr, JITFLOAT64 value) {
    assert(instr != NULL);
    (instr->param_3).value.f = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_result_fvalue_set (ir_instruction_t *instr, JITFLOAT64 value) {
    assert(instr != NULL);
    (instr->result).value.f = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_par1_value_set (ir_instruction_t *instr, JITUINT64 value) {
    assert(instr != NULL);
    (instr->param_1).value.v = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_par2_value_set (ir_instruction_t *instr, JITUINT64 value) {
    assert(instr != NULL);
    (instr->param_2).value.v = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_par3_value_set (ir_instruction_t *instr, JITUINT64 value) {
    assert(instr != NULL);
    (instr->param_3).value.v = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_result_value_set (ir_instruction_t *instr, JITUINT64 value) {
    assert(instr != NULL);
    (instr->result).value.v = value;
    return instr;
}

static inline ir_instruction_t * ir_instr_par1_type_set (ir_instruction_t *instr, JITINT16 type) {
    assert(instr != NULL);
    (instr->param_1).type = type;
    return instr;
}

static inline ir_instruction_t * ir_instr_par2_type_set (ir_instruction_t *instr, JITINT16 type) {
    assert(instr != NULL);
    (instr->param_2).type = type;
    return instr;
}

static inline ir_instruction_t * ir_instr_par3_type_set (ir_instruction_t *instr, JITINT16 type) {
    assert(instr != NULL);
    (instr->param_3).type = type;
    return instr;
}

static inline ir_instruction_t * ir_instr_result_type_set (ir_instruction_t *instr, JITINT16 type) {
    assert(instr != NULL);
    (instr->result).type = type;
    return instr;
}

//*ir_instruction_t * ir_instr_par1_infos_set(ir_instruction_t *instr, ir_item_infos_t * infos) {
//*	assert(instr != NULL);
//*	(instr->param_1).infos=infos;
//*	return instr;
//*}
//*
//*ir_instruction_t * ir_instr_par2_infos_set(ir_instruction_t *instr, ir_item_infos_t * infos) {
//*	assert(instr != NULL);
//*	(instr->param_2).infos=infos;
//*	return instr;
//*}
//*
//*ir_instruction_t * ir_instr_par3_infos_set(ir_instruction_t *instr, ir_item_infos_t * infos) {
//*	assert(instr != NULL);
//*	(instr->param_3).infos=infos;
//*	return instr;
//*}
//*
//*ir_instruction_t * ir_instr_par4_infos_set(ir_instruction_t *instr, ir_item_infos_t * infos) {
//*	assert(instr != NULL);
//*	(instr->param_4).infos=infos;
//*	return instr;
//*}
//*
static inline void ir_instr_par1_set (ir_instruction_t *instr, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *class) {

    /* Assertions		*/
    assert(instr != NULL);

    /* Check if the item is	*
     * a floating point	*/
    if (    (_IRMETHOD_isAFloatType(internal_type))         &&
            (type != IROFFSET)                              ) {
        (instr->param_1).value.f = fvalue;
    } else {
        (instr->param_1).value.v = value;
    }
    (instr->param_1).type = type;
    (instr->param_1).internal_type = internal_type;
    (instr->param_1).type_infos = class;

    return;
}

static inline void ir_instr_par2_set (ir_instruction_t *instr, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *class) {

    /* Assertions		*/
    assert(instr != NULL);

    if (    (_IRMETHOD_isAFloatType(internal_type))         &&
            (type != IROFFSET)                              ) {
        (instr->param_2).value.f = fvalue;
    } else {
        (instr->param_2).value.v = value;
    }
    (instr->param_2).type = type;
    (instr->param_2).internal_type = internal_type;
    (instr->param_2).type_infos = class;

    return;
}

static inline void ir_instr_par3_set (ir_instruction_t *instr, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *class) {
    assert(instr != NULL);

    if (    (_IRMETHOD_isAFloatType(internal_type))         &&
            (type != IROFFSET)                              ) {
        (instr->param_3).value.f = fvalue;
    } else {
        (instr->param_3).value.v = value;
    }
    (instr->param_3).type = type;
    (instr->param_3).internal_type = internal_type;
    (instr->param_3).type_infos = class;

    return;
}

static inline void ir_instr_result_set (ir_instruction_t *instr, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *class) {
    assert(instr != NULL);

    if (    (_IRMETHOD_isAFloatType(internal_type))         &&
            (type != IROFFSET)                              ) {
        (instr->result).value.f = fvalue;
    } else {
        (instr->result).value.v = value;
    }
    (instr->result).type = type;
    (instr->result).internal_type = internal_type;
    (instr->result).type_infos = class;

    return;
}

static inline JITUINT32 ir_method_var_local_count_get (ir_method_t * method) {
    JITUINT32 num;

    assert(method != NULL);

    num = xanList_length(method->locals);

    return num;
}

static inline ir_local_t *ir_method_var_local_insert (ir_method_t * method, JITUINT32 internal_type, TypeDescriptor *type) {

    assert(method != NULL);

    ir_local_t *local = sharedAllocFunction(sizeof(ir_local_t));

    local->var_number = method->var_max++;
    local->internal_type = internal_type;
    local->type_info = type;

    xanList_append(method->locals, local);

    return local;
}

static inline void ir_method_var_max_set (ir_method_t * method, JITUINT32 var_max) {

    assert(method != NULL);

    method->var_max = var_max;
}

static inline JITUINT32 ir_method_var_max_get (ir_method_t * method) {
    JITUINT32 num;

    assert(method != NULL);

    num = method->var_max;

    return num;
}

/* set the parameters number */
static inline void ir_method_insert_dummy_instruction (ir_method_t * method) {
    ir_instruction_t	*dummyInstr;
    JITUINT32		parametersNumber;
    JITUINT32		count;

    /* Assertions					*/
    assert(method != NULL);

    /* Set max variable number */
    ir_method_update_max_variable(method);

    /* Allocate the first instructions of the method*
    * as dummy instructions.			*
    * Each one is the definition of a parameter of *
    * the method.					*
    * e.g. instruction 0 is the definition of var0	*/
    parametersNumber = (method->signature).parametersNumber;
    for (count = 0; count < parametersNumber; count++) {
        dummyInstr	= ir_instr_get_by_pos(method, count);
        if (	(dummyInstr != NULL)		&&
                (dummyInstr->type == IRNOP)	) {
            continue;
        }

        if (dummyInstr == NULL) {
            dummyInstr = ir_instr_new(method);
        } else {
            dummyInstr = ir_instr_new_before(method, dummyInstr);
        }
        assert(dummyInstr != NULL);
        assert(dummyInstr->ID == count);

        ir_instr_type_set(dummyInstr, IRNOP);
    }

    return;
}

static inline JITUINT32 ir_method_pars_count_get (ir_method_t * method) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    count = (method->signature).parametersNumber;

    return count;
}

static inline JITINT32 ir_method_is_a_parameter (ir_method_t * method, JITUINT32 varID) {

    /* Assertions			*/
    assert(method != NULL);

    return varID < ir_method_pars_count_get(method);
}

static inline JITUINT32 * ir_method_pars_type_get (ir_method_t * method, JITUINT32 position) {
    JITUINT32       *type;

    assert((method->signature).parametersNumber > position);

    type = (&((method->signature).parameterTypes[position]));

    return type;
}

static inline void ir_method_pars_type_set (ir_method_t * method, JITUINT32 position, JITUINT32 value) {

    assert((method->signature).parametersNumber > position);

    (method->signature).parameterTypes[position] = value;
}

static inline JITUINT32 ir_method_result_type_get (ir_method_t * method) {
    JITUINT32 type;

    assert(method != NULL);

    type = ((method->signature).resultType);

    return type;
}

static inline void ir_method_result_type_set (ir_method_t * method, JITUINT32 value) {

    assert(method != NULL);

    (method->signature).resultType = value;
}

static inline ir_signature_t * ir_method_signature_get (ir_method_t * method) {

    assert(method != NULL);

    return &(method->signature);
}

static inline JITINT8 * ir_method_signature_in_string_get (ir_method_t *method) {
    JITINT8 *signatureInString;

    /* Assertions                   */
    assert(method != NULL);

    /* Initialize the variable	*/
    signatureInString = NULL;

    /* Check if we have metadata	*
     * information			*/
    if (method->ID != NULL) {
        signatureInString = method->ID->getSignatureInString(method->ID);
    }

    return signatureInString;
}

static inline JITINT8 * ir_method_completename_get (ir_method_t * method) {
    JITINT8 *completeName;

    /* Assertions                   */
    assert(method != NULL);

    /* Initialize the variables	*/
    completeName = NULL;

    if (method->ID != NULL) {
        completeName = method->ID->getCompleteName(method->ID);
    }

    /* Return                       */
    return completeName;
}

static inline JITINT8 * ir_method_name_get (ir_method_t * method) {
    JITINT8 *name;

    /* Assertions                   */
    assert(method != NULL);

    name = method->name;

    return name;
}

static inline void ir_method_id_set (ir_method_t * method, MethodDescriptor *ID) {

    assert(method != NULL);
    assert(ID != NULL);

    method->ID = ID;
}

static inline MethodDescriptor *ir_method_id_get (ir_method_t * method) {
    MethodDescriptor *id;

    assert(method != NULL);

    id = method->ID;

    return id;
}

static inline void ir_basicblock_delete_all (ir_method_t * method) {

    /* Assertions				*/
    assert(method != NULL);

    /* Check if there are basic blocks	*/
    if (method->basicBlocks == NULL) {
        method->basicBlocksTop = 0;
        return;
    }

    /* Free the basic blocks		*/
    assert(method->basicBlocksTop > 0);
    freeFunction(method->basicBlocks);
    freeFunction(method->instructionsBasicBlocksMap);
    method->basicBlocks 			= NULL;
    method->instructionsBasicBlocksMap	= NULL;
    method->basicBlocksTop 			= 0;

    return;
}


static inline JITUINT32 ir_basicblock_count (ir_method_t * method) {
    assert(method != NULL);
    return method->basicBlocksTop;
}

static inline IRBasicBlock * ir_basicblock_get (ir_method_t *method, JITUINT32 basicBlockNumber) {
    assert(method != NULL);

    if (basicBlockNumber >= method->basicBlocksTop) {
        return NULL;
    }
    return &(method->basicBlocks[basicBlockNumber]);
}

JITUINT32 IRMETHOD_getNumberOfInstructionsOfTypeWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb, JITUINT16 type) {
    JITUINT32 count;
    JITUINT32 count2;
    for (count = bb->startInst, count2=0; count <= bb->endInst; count++) {
        ir_instruction_t *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst->type == type) {
            count2++;
        }
    }
    return count2;
}

XanList * IRMETHOD_getInstructionsOfBasicBlock (ir_method_t *method, IRBasicBlock *bb) {
    XanList *l;
    JITUINT32 count;

    l = xanList_new(allocFunction, freeFunction, NULL);
    for (count = bb->startInst; count <= bb->endInst; count++) {
        ir_instruction_t *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        xanList_append(l, inst);
    }
    return l;
}

static inline IRBasicBlock * ir_basicblock_get_basic_block_from_instruction_id (ir_method_t *method, JITUINT32 instID) {
    IRBasicBlock    *bb;
    JITUINT32 	count;

    /* Assertions.
     */
    assert(method != NULL);
    assert(IRMETHOD_getInstructionAtPosition(method, instID)->ID == instID);

    /* Fetch the basic block.
     */
    count	= method->instructionsBasicBlocksMap[instID];
    bb 	= ir_basicblock_get(method, count);
    assert(bb != NULL);
    assert(instID >= bb->startInst);
    assert(instID <= bb->endInst);

    return bb;
}

JITINT32 IRMETHOD_getBasicBlockPosition (ir_method_t *method, IRBasicBlock *bb) {

    /* Assertions			*/
    assert(method != NULL);
    assert(bb != NULL);

    /* Return			*/
    return bb->pos;
}

static inline JITBOOLEAN _IRMETHOD_mayThrowException (ir_instruction_t *inst) {
    return _IRMETHOD_mayInstructionOfTypeThrowException(inst->type);
}

static inline JITBOOLEAN _IRMETHOD_mayInstructionOfTypeThrowException (JITUINT32 type) {
    switch (type) {
        case IRTHROW:
        case IRSUBOVF:
        case IRADDOVF:
        case IRMULOVF:
        case IRCONVOVF:
        case IRCHECKNULL:
        case IRDIV:
        case IRREM:
        case IRCALL:
        case IRVCALL:
        case IRICALL:
        case IRLIBRARYCALL:
        case IRNATIVECALL:
            return JITTRUE;
    }
    return JITFALSE;
}

static inline IRBasicBlock * ir_basicblock_new (ir_method_t *method) {

    /* Assertions			*/
    assert(method != NULL);

    if (method->basicBlocks == NULL) {
        assert(method->basicBlocksTop == 0);

        /* Allocate the instructions -> basic block mapping.
         */
        method->instructionsBasicBlocksMap	= allocFunction(sizeof(JITUINT32) * (ir_instr_count(method) + 1));

        method->basicBlocks = (IRBasicBlock *) sharedAllocFunction( sizeof(IRBasicBlock));
        method->basicBlocksTop = 1;
        method->basicBlocks[0].pos	= 0;
        return &(method->basicBlocks[0]);
    }

    (method->basicBlocksTop)++;
    method->basicBlocks = (IRBasicBlock *) dynamicReallocFunction( method->basicBlocks, sizeof(IRBasicBlock) * method->basicBlocksTop);
    method->basicBlocks[method->basicBlocksTop - 1].pos	= method->basicBlocksTop - 1;
    return &(method->basicBlocks[method->basicBlocksTop - 1]);
}

static inline ir_instruction_t * ir_instr_filter_get (ir_method_t * method, JITUINT32 labelID) {
    return ir_instr_label_like_get(method, labelID, IRSTARTFILTER);
}

static inline ir_instruction_t * ir_instr_finally_get (ir_method_t * method, JITUINT32 labelID) {
    return ir_instr_label_like_get(method, labelID, IRSTARTFINALLY);
}

static inline ir_instruction_t * ir_instr_call_finally_get (ir_method_t * method, JITUINT32 labelID) {
    return ir_instr_label_like_get(method, labelID, IRCALLFINALLY);
}

static inline ir_instruction_t * ir_instr_label_get (ir_method_t * method, JITUINT32 labelID) {
    return ir_instr_label_like_get(method, labelID, IRLABEL);
}

static inline ir_instruction_t * ir_instr_label_like_get (ir_method_t * method, JITUINT32 labelID, JITUINT32 instType) {
    ir_instruction_t        *inst;
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    for (count = 0; count < method->instructionsTop; count++) {
        inst = method->instructions[count];
        assert(inst != NULL);
        assert(inst->ID == count);
        if (    (ir_instr_type_get(inst) == instType)   &&
                (ir_instr_par1_value_get(inst) == labelID)      ) {
            return inst;
        }
    }
    return NULL;
}

static inline ir_instruction_t * ir_instrcatcher_get (ir_method_t *method) {
    ir_instruction_t        *inst;
    JITINT32 count;

    /* Assertions                   */
    assert(method != NULL);

    /* Find the catcher             */
    for (count = (method->instructionsTop - 1); count >= 0; count--) {
        inst = ir_instr_get_by_pos(method, count);
        assert(inst != NULL);
        assert(inst->ID == count);
        if (ir_instr_type_get(inst) == IRSTARTCATCHER) {
            return inst;
        }
    }

    /* Return                       */
    return NULL;
}

static inline JITBOOLEAN ir_method_escapes_isVariableEscaped (ir_method_t *method, JITUINT32 varID) {
    JITNUINT temp;
    JITUINT32 trigger;

    /* Assertions				*/
    assert(method != NULL);
    assert(varID < ir_method_var_max_get(method));

    if ((method->escapesVariables).bitmap == NULL) {
        return JITTRUE;
    }
    trigger = varID / (sizeof(JITNINT) * 8);
    assert(trigger < (method->escapesVariables).blocksNumber);
    temp = 0x1 << (varID % (sizeof(JITNINT) * 8));

    if (((method->escapesVariables).bitmap[trigger] & temp) != temp) {
        return JITFALSE;
    }
    return JITTRUE;
}

static inline void ir_method_escapes_setVariableAsEscaped (ir_method_t *method, JITUINT32 varID) {
    JITNUINT temp;
    JITUINT32 trigger;

    /* Assertions				*/
    assert(method != NULL);
    assert(varID < ir_method_var_max_get(method));
    assert((method->escapesVariables).bitmap != NULL);

    trigger = varID / (sizeof(JITNINT) * 8);
    assert(trigger < (method->escapesVariables).blocksNumber);
    temp = 0x1;
    temp = temp << varID % (sizeof(JITNINT) * 8);
    (method->escapesVariables).bitmap[trigger] |= temp;
}

static inline void ir_method_escapes_alloc (ir_method_t * method, JITUINT32 blocksNumber) {

    /* Assertions				*/
    assert(method != NULL);

    if (blocksNumber == 0) {
        if ((method->escapesVariables).bitmap != NULL) {
            freeMemory((method->escapesVariables).bitmap);
        }
        (method->escapesVariables).bitmap = NULL;
        (method->escapesVariables).blocksNumber = 0;
        return;
    }

    if ((method->escapesVariables).blocksNumber == blocksNumber) {
        return;
    }
    if ((method->escapesVariables).bitmap == NULL) {
        (method->escapesVariables).bitmap = (JITNUINT *) allocFunction(sizeof(JITNUINT) * blocksNumber);
    } else {
        (method->escapesVariables).bitmap = (JITNUINT *) dynamicReallocFunction((method->escapesVariables).bitmap, sizeof(JITNUINT) * blocksNumber);
    }
    (method->escapesVariables).blocksNumber = blocksNumber;

    return;
}

static inline void ir_method_liveness_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t        *inst;

    /* Assertions				*/
    assert(method != NULL);

    /* Initialize the variables		*/
    inst = NULL;

    if (blocksNumber < 1) {
        method->livenessBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_liveness_free(method, inst);
        }
        method->livenessBlockNumbers = 0;
    } else {
        if (method->livenessBlockNumbers == blocksNumber) {
            while ((inst = ir_instr_get_next(method, inst))) {
                if (    ((inst->metadata->liveness).use == NULL)        ||
                        ((inst->metadata->liveness).in == NULL)         ||
                        ((inst->metadata->liveness).out == NULL)        ) {
                    ir_instr_liveness_alloc(method, inst);
                }
                assert((inst->metadata->liveness).use != NULL);
                assert((inst->metadata->liveness).in != NULL);
                assert((inst->metadata->liveness).out != NULL);
                memset((inst->metadata->liveness).use, 0, blocksNumber * sizeof(JITNUINT));
                memset((inst->metadata->liveness).in, 0, blocksNumber * sizeof(JITNUINT));
                memset((inst->metadata->liveness).out, 0, blocksNumber * sizeof(JITNUINT));
                (inst->metadata->liveness).def = -1;
            }
            return;
        }
        method->livenessBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_liveness_alloc(method, inst);
            (inst->metadata->liveness).def = -1;
            assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));
            assert((inst->metadata->liveness).use != NULL);
            assert((inst->metadata->liveness).in != NULL);
            assert((inst->metadata->liveness).out != NULL);
        }
    }
}

static inline void ir_method_reaching_definitions_alloc (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_instruction_t        *inst;

    /* Assertions				*/
    assert(method != NULL);

    /* Initialize the variables		*/
    inst = NULL;

    if (blocksNumber < 1) {
        method->reachingDefinitionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_reaching_definitions_free(method, inst);
        }
        method->reachingDefinitionsBlockNumbers = 0;
    } else {
        JITUINT32 count;
        method->reachingDefinitionsBlockNumbers = blocksNumber;
        for (count = 0; count < ir_instr_count(method); count++) {
            inst = ir_instr_get_by_pos(method, count);
            assert(inst != NULL);
            internal_ir_instr_reaching_definitions_alloc(inst, method->reachingDefinitionsBlockNumbers);
        }
    }
}

static inline void ir_method_reaching_expressions_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    /* Assertions			*/
    assert(method != NULL);

    if (blocksNumber < 1) {
        method->reachingExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_reaching_expressions_free(method, inst);
        }
        method->reachingExpressionsBlockNumbers = 0;
    } else {
        if (method->reachingExpressionsBlockNumbers == blocksNumber) {
            return;
        }
        method->reachingExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_reaching_expressions_alloc(method, inst);
        }
    }
}

static inline void ir_method_available_expressions_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    /* Assertions			*/
    assert(method != NULL);

    if (blocksNumber < 1) {
        method->availableExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_available_expressions_free(method, inst);
        }
        method->availableExpressionsBlockNumbers = 0;
    } else {
        method->availableExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_available_expressions_alloc(method, inst);
        }
    }
}

static inline void ir_method_anticipated_expressions_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    assert(method != NULL);

    if (blocksNumber < 1) {
        method->anticipatedExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_anticipated_expressions_free(method, inst);
        }
        method->anticipatedExpressionsBlockNumbers = 0;
    } else {
        if (method->anticipatedExpressionsBlockNumbers == blocksNumber) {
            return;
        }
        method->anticipatedExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_anticipated_expressions_alloc(method, inst);
        }
    }
}

static inline void ir_method_postponable_expressions_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    assert(method != NULL);

    ir_instruction_t * inst = NULL;

    if (blocksNumber < 1) {
        method->postponableExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_postponable_expressions_free(method, inst);
        }
        method->postponableExpressionsBlockNumbers = 0;
    } else {
        if (method->postponableExpressionsBlockNumbers == blocksNumber) {
            return;
        }
        method->postponableExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_postponable_expressions_alloc(method, inst);
        }
    }
}

static inline void ir_method_used_expressions_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    assert(method != NULL);


    if (blocksNumber < 1) {
        method->usedExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_used_expressions_free(method, inst);
        }
        method->usedExpressionsBlockNumbers = 0;
    } else {
        if (method->usedExpressionsBlockNumbers == blocksNumber) {
            return;
        }
        method->usedExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_used_expressions_alloc(method, inst);
        }
    }
}


static inline void ir_method_earliest_expressions_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    assert(method != NULL);

    if (blocksNumber < 1) {
        method->earliestExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_earliest_expressions_free(method, inst);
        }
        method->earliestExpressionsBlockNumbers = 0;
    } else {
        if (method->earliestExpressionsBlockNumbers == blocksNumber) {
            return;
        }
        method->earliestExpressionsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_earliest_expressions_alloc(method, inst);
        }
    }
}

static inline void ir_method_latest_placements_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    assert(method != NULL);

    if (blocksNumber < 1) {
        method->latestPlacementsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_latest_placements_free(method, inst);
        }
        method->latestPlacementsBlockNumbers = 0;
    } else {
        if (method->latestPlacementsBlockNumbers == blocksNumber) {
            return;
        }
        method->latestPlacementsBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_latest_placements_alloc(method, inst);
        }
    }
}

static inline void ir_method_partial_redundancy_elimination_alloc (ir_method_t * method, JITUINT32 blocksNumber) {
    ir_instruction_t * inst = NULL;

    assert(method != NULL);

    if (blocksNumber < 1) {
        method->partialRedundancyEliminationBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_partial_redundancy_elimination_free(method, inst);
        }
        method->partialRedundancyEliminationBlockNumbers = 0;
    } else {
        if (method->partialRedundancyEliminationBlockNumbers == blocksNumber) {
            return;
        }
        method->partialRedundancyEliminationBlockNumbers = blocksNumber;
        while ((inst = ir_instr_get_next(method, inst))) {
            ir_instr_partial_redundancy_elimination_alloc(method, inst);
        }
    }
}

static inline ir_instruction_t * ir_instr_get_successor_debug (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevSuccessor) {
    ir_instruction_t        *succ;
    ir_instruction_t        *pred;

    succ = ir_instr_get_successor(method, inst, prevSuccessor, NULL, NULL);
    if (succ != NULL) {
        pred = ir_instr_get_predecessor(method, succ, NULL, NULL, NULL);
        while (pred != NULL) {
            if (pred == inst) {
                break;
            }
            pred = ir_instr_get_predecessor(method, succ, pred, NULL, NULL);
        }
        if (pred == NULL) {
            assert(0 == 1);
        }
    }

    return succ;
}

static inline ir_instruction_t * ir_instr_get_successor (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevSuccessor, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized) {
    ir_instruction_t        *successor;
    ir_instruction_t        *catcher;
    JITUINT16		instType;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize the variables.
     */
    successor = NULL;
    if (prevSuccessor == method->exitNode) {
        return NULL;
    }
    instType	= ir_instr_type_get(inst);
    switch (instType) {
        case IRRET:
            return method->exitNode;
        case IRBRANCH:
            if (prevSuccessor != NULL) {
                assert(prevSuccessor == ir_instr_label_get(method, ir_instr_par1_value_get(inst)));
                return NULL;
            }
            return ir_instr_label_get(method, ir_instr_par1_value_get(inst));
        case IRCALLFINALLY:
            if (prevSuccessor != NULL) {
                return NULL;
            }
            return ir_instr_finally_get(method, ir_instr_par1_value_get(inst));
        case IRCALLFILTER:
            if (prevSuccessor != NULL) {
                return NULL;
            }
            return ir_instr_filter_get(method, ir_instr_par1_value_get(inst));
        case IRBRANCHIF:
        case IRBRANCHIFNOT:
            if (prevSuccessor == NULL) {
                return ir_instr_get_next(method, inst);
            }
            successor = ir_instr_label_get(method, ir_instr_par2_value_get(inst));
            if (successor == prevSuccessor) {
                return NULL;
            }
            return successor;
        case IRBRANCHIFPCNOTINRANGE:
            if (prevSuccessor == NULL) {
                return ir_instr_get_next(method, inst);
            }
            successor = ir_instr_label_get(method, ir_instr_par3_value_get(inst));
            if (successor == prevSuccessor) {
                return NULL;
            }
            return successor;
        case IRTHROW:
            if (prevSuccessor != NULL) {
                if (prevSuccessor->type == IRSTARTCATCHER) {
                    return method->exitNode;
                }
                assert(prevSuccessor->type == IREXITNODE);
                return NULL;
            }
            assert(prevSuccessor == NULL);
            if (catcherInst != NULL) {
                if (*catcherInstInitialized) {
                    catcher	= *catcherInst;
                } else {
                    catcher 			= ir_instrcatcher_get(method);
                    (*catcherInst)			= catcher;
                    (*catcherInstInitialized)	= JITTRUE;
                }
            } else {
                catcher = ir_instrcatcher_get(method);
            }
            if (	(catcher != NULL)		&&
                    (inst->ID < catcher->ID)	) {
                return catcher;
            }
            return method->exitNode;
        case IRENDFINALLY:
            if (prevSuccessor != NULL) {
                IR_ITEM_VALUE label;
                label = ir_instr_par1_value_get(inst);
                successor = prevSuccessor;
                while ((successor = ir_instr_get_next(method, successor))) {
                    if (     (ir_instr_type_get(successor) == IRCALLFINALLY) &&
                             (ir_instr_par1_value_get(successor) == label)           ) {
                        successor = ir_instr_get_next(method, successor);
                        return successor;
                    }
                }
                return NULL;
            }
            if (ir_instr_par1_type_get(inst) != NOPARAM) {
                successor = ir_instr_call_finally_get(method, ir_instr_par1_value_get(inst));
		if (successor == NULL){
			return NULL;
		}
                assert(successor != NULL);
                assert(successor->type == IRCALLFINALLY);
                successor = ir_instr_get_next(method, successor);
                if (successor != inst) { //FIXME to remove, this condition should be always true
                    return successor;
                }
            }
            return NULL;
        case IRENDFILTER:
            if (prevSuccessor != NULL) {
                assert(successor == ir_instr_label_get(method, ir_instr_par2_value_get(inst)));
                return NULL;
            }
            if (ir_instr_par2_type_get(inst) == NOPARAM) {
                return NULL;
            }
            return ir_instr_label_get(method, ir_instr_par2_value_get(inst));
        default:
            if (_IRMETHOD_mayInstructionOfTypeThrowException(instType)) {
                if (prevSuccessor != NULL) {
                    if (prevSuccessor == ir_instr_get_next(method, inst)) {
                        ir_instruction_t	*catcher;

                        if (catcherInst != NULL) {
                            if (*catcherInstInitialized) {
                                catcher		= *catcherInst;
                            } else {
                                catcher 			= ir_instrcatcher_get(method);
                                (*catcherInst)			= catcher;
                                (*catcherInstInitialized)	= JITTRUE;
                            }
                        } else {
                            catcher		= ir_instrcatcher_get(method);
                        }
                        if (	(catcher != NULL)		&&
                                (catcher != prevSuccessor)	&&
                                (inst->ID < catcher->ID)	) {
                            return catcher;
                        }
                        if (prevSuccessor != method->exitNode) {
                            return method->exitNode;
                        }
                    }
                    return NULL;
                }
                return ir_instr_get_next(method, inst);
            }
            assert(!_IRMETHOD_mayInstructionOfTypeThrowException(instType));
            if (prevSuccessor != NULL) {
                assert(prevSuccessor == ir_instr_get_next(method, inst));
                return NULL;
            }
            return ir_instr_get_next(method, inst);
    }
    abort();
}

static inline ir_instruction_t* ir_get_first_instr_by_type (ir_method_t *method, JITNUINT instrType, JITUINT32 startPosition) {
    JITNUINT i, lastInst;
    ir_instruction_t *inst;

    assert(method != NULL);

    lastInst = method->instructionsTop;

    for (i = startPosition; i < lastInst; i++) {
        inst = ir_instr_get_by_pos(method, i);

        if (inst->type == instrType) {
            return inst;
        }
    }

    return NULL;
}

static inline ir_instruction_t* ir_get_next_instr_of_same_type (ir_method_t *method, ir_instruction_t *instr) {
    JITNUINT i, lastInstr, instrType;
    ir_instruction_t *foundInstr;

    assert(method != NULL);
    assert(instr != NULL);

    lastInstr = method->instructionsTop;
    instrType = instr->type;

    for (i = instr->ID + 1; i < lastInstr; i++) {
        foundInstr = ir_instr_get_by_pos(method, i);

        if (foundInstr->type == instrType) {
            return foundInstr;
        }
    }

    return NULL;
}

static inline ir_instruction_t* ir_get_first_return_instr (ir_method_t *method) {
    return ir_get_first_instr_by_type(method, IRRET, START_IR_INSTRUCTIONS_ID);
}

static inline ir_instruction_t *ir_get_exit_node_first_predecessor (ir_method_t *method) {
    ir_instruction_t 	*inst;
    ir_instruction_t 	*lastInst;
    ir_instruction_t	*catchInst;
    JITUINT32		startPosition;

    inst = ir_get_first_return_instr(method);
    if (inst != NULL) {
        return inst;
    }

    catchInst	= ir_instrcatcher_get(method);
    if (catchInst == NULL) {
        startPosition	= START_IR_INSTRUCTIONS_ID;
    } else {
        startPosition	= catchInst->ID + 1;
    }
    inst	= ir_get_first_instr_by_type(method, IRTHROW, startPosition);
    if (inst != NULL) {
        return inst;
    }

    lastInst = ir_instr_get_last(method);
    if (!ir_instr_is_a_jump_instruction(method, lastInst)) {
        return lastInst;
    }

    return NULL;

}

static inline JITNINT next_type (JITNINT type) {
    switch (type) {
        case IRRET:
            return IRTHROW;
        case IRTHROW:
            return IRSUBOVF;
        case IRSUBOVF:
            return IRADDOVF;
        case IRADDOVF:
            return IRMULOVF;
        case IRMULOVF:
            return IRCONVOVF;
        case IRCONVOVF:
            return IRCHECKNULL;
        case IRCHECKNULL:
            return IRDIV;
        case IRDIV:
            return IRREM;
        case IRREM:
            return IRCALL;
        case IRCALL:
            return IRVCALL;
        case IRVCALL:
            return IRICALL;
        case IRICALL:
            return IRLIBRARYCALL;
        case IRLIBRARYCALL:
            return -1;
    }
    return -1;
}

static inline JITBOOLEAN is_already_checked_type_for_exit_node_predecessor (ir_instruction_t *inst) {
    if (	(inst->type == IRRET) 	||
            (inst->type == IRTHROW) ) {
        return JITTRUE;
    }

    return JITFALSE;
}

/**
 * @brief Return a predecessor of the exit node
 *
 * Predecessors of the exit node are, in turn, all IRRET, then all IRTHROW, then the last instruction of the method, then NULL
 */
static inline ir_instruction_t *ir_get_exit_node_predecessor (ir_method_t *method, ir_instruction_t *prevPredecessor) {
    ir_instruction_t 	*inst;
    ir_instruction_t	*lastInst;
    ir_instruction_t	*catchInst;
    JITNINT 		type;

    /* Assertions			*/
    assert(method != NULL);

    /* Check if we need to return the first predecessor			*/
    if (prevPredecessor == NULL) {
        inst = ir_get_exit_node_first_predecessor(method);
        return inst;
    }

    /* Fetch the next predecessor of the same type				*/
    inst 		= ir_get_next_instr_of_same_type(method, prevPredecessor);
    if (inst != NULL) {
        return inst;
    }

    /* Switch the type since there is no more predecessor of the same type	*/
    type 		= next_type(prevPredecessor->type);

    /* Fetch the catcher							*/
    catchInst	= ir_instrcatcher_get(method);

    /* Fetch the next predecessor						*/
    while (inst == NULL && type != -1) {
        JITUINT32		startPosition;

        /* Fetch the start position for the search				*/
        startPosition	= START_IR_INSTRUCTIONS_ID;
        if (	(_IRMETHOD_mayInstructionOfTypeThrowException(type))	&&
                (catchInst != NULL)					) {
            startPosition	= catchInst->ID + 1;
        }

        /* Fetch the first instruction of the new type				*/
        inst 		= ir_get_first_instr_by_type(method, type, startPosition);

        /* Fetch the next type to consider					*/
        type 		= next_type(type);
    }

    if (inst == NULL) {
        lastInst = ir_instr_get_last(method);
        if (     (!ir_instr_is_a_jump_instruction(method, lastInst))             &&
                 (prevPredecessor != lastInst)                                   &&
                 (!is_already_checked_type_for_exit_node_predecessor(lastInst))  ) {
            inst = lastInst;
        }
    }

    return inst;
}

static inline ir_instruction_t * ir_instr_get_predecessor (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevPredecessor, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized) {
    ir_instruction_t        *prevInst;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize the variables		*/
    prevInst = NULL;

    /* Check if the instruction is the successor of an IRCALLFINALLY.	*
     * In this case, its predecessor can be a IRENDFINALLY instruction.	*/
    if (inst->ID > 0) {
        ir_instruction_t        *call_finally_inst;
        call_finally_inst = method->instructions[inst->ID - 1];
        if (    (call_finally_inst->type == IRCALLFINALLY)                              &&
                ((prevPredecessor == NULL) || (prevPredecessor->type == IRENDFINALLY))  ) {
            prevInst = prevPredecessor;
            while ((prevInst = ir_instr_get_next(method, prevInst))) {
                if (     (ir_instr_type_get(prevInst) == IRENDFINALLY)                                   &&
                         (ir_instr_par1_value_get(prevInst) == ir_instr_par1_value_get(call_finally_inst))       ) {
                    return prevInst;
                }
            }
        }
    }

    switch (ir_instr_type_get(inst)) {
            // IRLABEL instruction must has at least one predecessor. All the predecessors are either
            // IRBRANCH IRBRANCHIF IRBRANCHIFNOT IRBRANCHIFPCNOTINRANGE, which target the IRLABEL,
            // or precedend istruction if it is not a IRTHROW and IRRET.
            // The predecessors are returned in parsing order excepting of the last predecessor, if it
            // is precedent, it is not a IRBRANCH or IRTHROW or IRRET.
        case IRLABEL:

            /* Check the final condition			*/
            prevInst = ir_instr_get_prev(method, inst);
            if (    (prevPredecessor != NULL)                                                           &&
                    (prevPredecessor == prevInst)                                                       &&
                    (internal_has_next_instruction_as_successor_without_jumps(method, prevInst))        ) {
                return NULL;
            }

            /* Seek						*/
            if (    (prevPredecessor == NULL)               ||
                    (prevPredecessor->type == IRENDFINALLY) ) {
                prevInst = ir_instr_get_first(method);
            } else if ((prevPredecessor->ID + 1) < (method->instructionsTop)) {
                prevInst = method->instructions[prevPredecessor->ID + 1];
            } else {
                prevInst = NULL;
            }

            /* Search the branches predecessor		*/
            while (prevInst != NULL) {
                JITUINT32 prevInstType;
                prevInstType = ir_instr_type_get(prevInst);
                if (prevInstType == IRBRANCH) {
                    if (ir_instr_par1_value_get(prevInst) == ir_instr_par1_value_get(inst)) {
                        assert(prevInst->type != IRENDFINALLY);
                        return prevInst;
                    }
                } else if ( (prevInstType == IRBRANCHIF)    	||
                            (prevInstType == IRBRANCHIFNOT) 	) {
                    if (	(ir_instr_par2_value_get(prevInst) == ir_instr_par1_value_get(inst))	&&
                            (prevInst->ID != (inst->ID - 1))					) {
                        assert(prevInst->type != IRENDFINALLY);
                        return prevInst;
                    }
                    if (prevInst->ID + 1 < method->instructionsTop) {
                        prevInst = method->instructions[prevInst->ID + 1];
                        continue;
                    } else {
                        break;
                    }
                } else if (prevInstType == IRBRANCHIFPCNOTINRANGE) {
                    if (ir_instr_par3_value_get(prevInst) == ir_instr_par1_value_get(inst)) {
                        assert(prevInst->type != IRENDFINALLY);
                        return prevInst;
                    }
                    if (prevInst->ID + 1 < method->instructionsTop) {
                        prevInst = method->instructions[prevInst->ID + 1];
                        continue;
                    } else {
                        break;
                    }
                }
                if (prevInst->ID + 1 < method->instructionsTop) {
                    prevInst = method->instructions[prevInst->ID + 1];
                } else {
                    break;
                }
            }

            /* Search the last predecessor			*/
            prevInst = ir_instr_get_prev(method, inst);
            if (prevInst != NULL) {
                if (internal_has_next_instruction_as_successor_without_jumps(method, prevInst)) {
                    assert(prevInst->type != IRENDFINALLY);
                    return prevInst;
                }
            }
            return NULL;
        case IRSTARTFILTER:
            abort();
            break;
        case IRSTARTFINALLY:
            if (prevPredecessor == NULL) {
                prevInst = ir_instr_get_prev(method, inst);
                if (    (internal_has_next_instruction_as_successor(method, prevInst)   &&
                         (ir_instr_type_get(prevInst) != IRCALLFINALLY))         ) {
                    return prevInst;
                }
            }
            if (     (prevPredecessor != NULL)                                               &&
                     (internal_has_next_instruction_as_successor(method, prevPredecessor)    &&
                      (ir_instr_type_get(prevPredecessor) != IRCALLFINALLY))          ) {
                prevInst = NULL;
            } else {
                prevInst = prevPredecessor;
            }
            while ((prevInst = ir_instr_get_next(method, prevInst))) {
                if (     (ir_instr_type_get(prevInst) == IRCALLFINALLY)                  &&
                         (ir_instr_par1_value_get(prevInst) == ir_instr_par1_value_get(inst))    ) {
                    return prevInst;
                }
            }
            return NULL;
        case IRSTARTCATCHER:
            if (    (prevPredecessor != NULL)                                     	&&
                    (ir_instr_type_get(prevPredecessor) == IRENDFINALLY)            	) {
                prevInst = NULL;
            } else {
                prevInst = prevPredecessor;
            }
            while ((prevInst = ir_instr_get_next(method, prevInst))) {
                if (    (_IRMETHOD_mayThrowException(prevInst) && (prevInst->ID < inst->ID))       		||
                        (ir_instr_type_get(prevInst) == IRLABEL && (prevInst->ID == inst->ID - 1))        	) {
                    return prevInst;
                }
            }
            return NULL;
        case IREXITNODE:
            return ir_get_exit_node_predecessor(method, prevPredecessor);
        default:
            if (prevPredecessor != NULL) {
                return NULL;
            }
            prevInst = ir_instr_get_prev(method, inst);
            if (prevInst == NULL) {
                return NULL;
            }
            assert(prevInst != NULL);
            if (ir_instr_get_successor(method, prevInst, NULL, catcherInst, catcherInstInitialized) == inst) {
                return prevInst;
            }
            return NULL;
    }
    abort();
    return NULL;
}

static inline void ir_method_delete_extra_memory (ir_method_t *method) {
    JITUINT32		instructionsNumber;
    ir_instruction_t        *inst;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if there are instructions.
     */
    if (method->instructionsTop == START_IR_INSTRUCTIONS_ID) {
        return;
    }

    /* Free data-flow information.
     */
    inst = method->instructions[0];
    if (    (inst != NULL)                  &&
            (inst->metadata != NULL)        ) {
        IRMETHOD_deleteDataFlowInformation(method);
    }

    /* Free the DDG.
     */
    instructionsNumber	= ir_instr_count(method);
    for (JITUINT32 count = 0; count <= instructionsNumber; count++) {
        inst = ir_instr_get_by_pos(method, count);
        if (inst->metadata == NULL) {
            continue ;
        }
        if ((inst->metadata->data_dependencies).dependingInsts != NULL) {
            internal_destroyDataDependences((inst->metadata->data_dependencies).dependingInsts);
            (inst->metadata->data_dependencies).dependingInsts	= NULL;
        }
        if ((inst->metadata->data_dependencies).dependsFrom != NULL) {
            internal_destroyDataDependences((inst->metadata->data_dependencies).dependsFrom);
            (inst->metadata->data_dependencies).dependsFrom		= NULL;
        }
    }

    /* Free basic block information	*/
    ir_basicblock_delete_all(method);

    /* Free the escapes information	*/
    ir_method_escapes_alloc(method, 0);

    /* Free the loops		*/
    IRMETHOD_deleteCircuitsInformation(method);

    /* Free the dominators		*/
    if (method->preDominatorTree != NULL) {
        method->preDominatorTree->destroyTree(method->preDominatorTree);
        method->preDominatorTree = NULL;
    }
    if (method->postDominatorTree != NULL) {
        method->postDominatorTree->destroyTree(method->postDominatorTree);
        method->postDominatorTree = NULL;
    }

    /* Set the flags		*/
    method->valid_optimization = 0;

    return;
}

static inline void ir_method_delete (ir_method_t *method) {
    ir_signature_t *signature;

    /* Assertions			*/
    assert(method != NULL);

    /* Check if the instructions    *
     * were already freed		*/
    if (method->instructions != NULL) {

        /* Delete extra memory		*/
        ir_method_delete_extra_memory(method);

        /* Delete instructions		*/
        ir_instr_delete_all(method);
        freeFunction(method->instructions);
        method->instructions = NULL;
        method->instructionsTop = 0;
        method->instructionsAllocated = 0;

        /* Delete the mutex		*/
        PLATFORM_destroyMutex(&(method->mutex));
    }

    /* Delete the exit node		*/
    if (method->exitNode != NULL) {
        ir_instr_data_delete(method, method->exitNode, JITTRUE);
        freeMemory(method->exitNode);
        method->exitNode = NULL;
    }

    /* Delete the local variables   *
     * bytecode information		*/
    if (method->locals != NULL) {
        xanList_destroyListAndData(method->locals);
        method->locals = NULL;
    }

    /* Delete the signature		*/
    signature = &(method->signature);
    if (signature->parameterTypes != NULL) {
        freeMemory(signature->parameterTypes);
    }
    if (signature->ilParamsTypes != NULL) {
        freeMemory(signature->ilParamsTypes);
    }
    if (signature->ilParamsDescriptor != NULL) {
        freeMemory(signature->ilParamsDescriptor);
    }
    memset(signature, 0, sizeof(ir_signature_t));

    /* Delete the metadata          */
    if (method->metadata != NULL) {
        IrMetadata *nextMetadata;
        IrMetadata *metadata;
        metadata = method->metadata;
        while (metadata != NULL) {
            nextMetadata = metadata->next;
            freeMemory(metadata);
            metadata = nextMetadata;
        }
    }

    /* Delete the method name	*/
    if (method->name != NULL) {
        freeMemory(method->name);
        method->name	= NULL;
    }

    return;
}

static inline void ir_method_update_max_variable (ir_method_t *method) {
    ir_instruction_t        *instr;
    ir_item_t               *item;
    JITINT32 max;
    JITUINT32 count;
    JITUINT32 count2;

    /* Assertions				*/
    assert(method != NULL);

    /* Compute the max variables		*/
    max = ir_method_pars_count_get(method) + ir_method_var_local_count_get(method);
    assert(max >= 0);
    for (count = START_IR_INSTRUCTIONS_ID; count < method->instructionsTop; count++) {

        /* Get the next instruction		*/
        instr = method->instructions[count];

        /* Check the parameters			*/
        assert(instr != NULL);
        assert(instr->ID == count);
        switch (ir_instr_pars_count_get(instr)) {
            case 3:
                item = ir_instr_par3_get(instr);
                assert(item != NULL);
                if (item->type == IROFFSET) {
                    if ((item->value).v > max) {
                        max = (item->value).v;
                    }
                }
            case 2:
                item = ir_instr_par2_get(instr);
                assert(item != NULL);
                if (item->type == IROFFSET) {
                    if ((item->value).v > max) {
                        max = (item->value).v;
                    }
                }
            case 1:
                item = ir_instr_par1_get(instr);
                if (item->type == IROFFSET) {
                    if ((item->value).v > max) {
                        max = (item->value).v;
                    }
                }
                break;
        }
        assert(max >= 0);

        /* Check the result			*/
        item = ir_instr_result_get(instr);
        assert(item != NULL);
        if (item->type == IROFFSET) {
            if ((item->value).v > max) {
                max = (item->value).v;
            }
        }
        assert(max >= 0);

        /* Check the call parameters		*/
        for (count2 = 0; count2 < ir_instrcall_pars_count_get(instr); count2++) {
            JITUINT32 varID;
            if (ir_instrcall_par_type_get(instr, count2) == IROFFSET) {
                varID = ir_instrcall_par_value_get(instr, count2);
                if (varID > max) {
                    max = varID;
                }
            }
        }
        assert(max >= 0);
    }
    assert(max >= 0);
    assert(max >= (ir_method_pars_count_get(method) + ir_method_var_local_count_get(method)));

    /* Update the max variables ID		*/
    if (IRMETHOD_hasMethodInstructions(method)) {
        ir_method_var_max_set(method, max + 1);
    } else {
        ir_method_var_max_set(method, max);
    }

    /* Check				*/
#ifdef DEBUG
    for (count = 0; count < method->instructionsTop; count++) {
        instr = IRMETHOD_getInstructionAtPosition(method, count);
        if (instr->param_1.type == IROFFSET) {
            assert(instr->param_1.value.v < ir_method_var_max_get(method));
        }
        if (instr->callParameters != NULL) {
            XanListItem     *item;
            item = xanList_first(instr->callParameters);
            while (item != NULL) {
                ir_item_t       *param;
                param = item->data;
                if (param->type == IROFFSET) {
                    assert((param->value).v < ir_method_var_max_get(method));
                }
                item = item->next;
            }
        }
    }
#endif

    /* Return				*/
    return;
}

JITBOOLEAN IRMETHOD_deleteInstructionsDataDependence (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to) {
    JITBOOLEAN	found;

    /* Assertions.
     */
    assert(method != NULL);
    assert(from != NULL);
    assert(to != NULL);

    /* Initialize the variables.
     */
    found		= JITFALSE;

    /* Remove the dependence.
     */
    if (	(from->metadata != NULL)					&&
            ((from->metadata->data_dependencies).dependsFrom != NULL)	) {
        data_dep_arc_list_t	*d;
        d	= xanHashTable_lookup((from->metadata->data_dependencies).dependsFrom, to);
        if (d != NULL) {
            assert(d->inst == to);
            xanHashTable_removeElement((from->metadata->data_dependencies).dependsFrom, to);
            internal_destroyDataDependence(d);
            found	= JITTRUE;
        }
    }
    if (	(to->metadata != NULL)						&&
            ((to->metadata->data_dependencies).dependingInsts != NULL)	) {
        data_dep_arc_list_t	*d;
        d	= xanHashTable_lookup((to->metadata->data_dependencies).dependingInsts, from);
        if (d != NULL) {
            assert(d->inst == from);
            xanHashTable_removeElement((to->metadata->data_dependencies).dependingInsts, from);
            internal_destroyDataDependence(d);
            found	= JITTRUE;
        }
    }
    assert(!IRMETHOD_isInstructionDataDependent(method, from, to));

    return found;
}

static inline JITUINT16 ir_instr_get_data_dependence (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to) {
    XanHashTable            *deps;
    data_dep_arc_list_t     *dep;

    /* Assertions			*/
    assert(method != NULL);
    assert(from != NULL);
    assert(to != NULL);

    deps = (from->metadata->data_dependencies).dependsFrom;
    if (deps == NULL) {
        return JITFALSE;
    }
    dep	= xanHashTable_lookup(deps, to);
    if (dep != NULL) {
        assert(dep->inst == to);
        return JITTRUE;
    }
    deps = (to->metadata->data_dependencies).dependingInsts;
    if (deps == NULL) {
        return JITFALSE;
    }
    dep	= xanHashTable_lookup(deps, from);
    if (dep != NULL) {
        assert(dep->inst == from);
        return JITTRUE;
    }

    return JITFALSE;
}

static inline JITBOOLEAN ir_instr_is_a_postdominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated) {
    JITBOOLEAN result;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(dominated != NULL);
    assert(method->postDominatorTree != NULL);

    /* Ensure reflexive property of the dominance	*
     * relation					*/
    if (inst == dominated) {
        return JITTRUE;
    }

    /* Check if we have the predominator tree       */
    if (method->postDominatorTree == NULL) {
        return JITFALSE;
    }

    /* Check the pre-dominance relation		*/
    result = internal_is_a_dominator(method, inst, dominated, method->postDominatorTree);

    /* Return					*/
    return result;
}

static inline JITNINT ir_instr_is_a_predominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated) {
    JITNINT result;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(dominated != NULL);
    assert(method->preDominatorTree != NULL);

    /* Ensure reflexive property of the dominance	*
     * relation					*/
    if (inst == dominated) {
        return JITTRUE;
    }

    /* Check if we have the predominator tree       */
    if (method->preDominatorTree == NULL) {
        return JITFALSE;
    }

    /* Check if the instruction is the exit node	*/
    if (inst->type == IREXITNODE) {
        return JITFALSE;
    }

    /* Check the pre-dominance relation		*/
    result = internal_is_a_dominator(method, inst, dominated, method->preDominatorTree);

    /* Return					*/
    return result;
}

static inline JITINT8 * ir_get_class_name (TypeDescriptor * classID) {
    JITINT8                 *name;

    /* Assertions                   */
    assert(classID != 0);

    /* Initialize the variables	*/
    name = NULL;

    /* Fetch the type               */
    if (classID != NULL) {
        name = classID->getCompleteName(classID);
    }

    /* Return                       */
    return name;
}

static inline TypeDescriptor *ir_get_result_il_type (ir_method_t *method) {
    /* Assertions                   */
    assert(method != NULL);

    /* Return                       */
    return (method->signature).ilResultType;
}

static inline TypeDescriptor *ir_get_parameter_il_type (ir_method_t *method, JITUINT32 parameterNumber) {
    /* Assertions                   */
    assert(method != NULL);
    assert(parameterNumber < (method->signature).parametersNumber);

    if ((method->signature).ilParamsTypes == NULL) {
        return NULL;
    }
    return (method->signature).ilParamsTypes[parameterNumber];
}

static inline ir_local_t *ir_get_local (ir_method_t * method, JITUINT32 var_number) {
    ir_local_t *local;
    XanListItem *item;

    /* Assertions                   */
    assert(method != NULL);

    /* Initialization */
    local = NULL;

    /* Fetch the local              */
    item = xanList_first(method->locals);
    while (item != NULL) {
        ir_local_t *current_local = (ir_local_t *) item->data;
        if (current_local->var_number == var_number) {
            local = current_local;
            break;
        }
        item = item->next;
    }

    /* Return                       */
    return local;
}

void createExitNode (ir_method_t *method) {
    ir_instruction_t *exitNode;
    JITBOOLEAN allocateMetadata;

    exitNode = sharedAllocFunction(sizeof(ir_instruction_t));

    if (    ((method->instructions[0] != NULL) && (method->instructions[0]->metadata != NULL))      	||
            ((method->instructions[1] != NULL) && (method->instructions[1]->metadata != NULL))      	) {
        allocateMetadata = JITTRUE;
    } else {
        allocateMetadata = JITFALSE;
    }

    ir_instr_init(method, exitNode, allocateMetadata);

    exitNode->type = IREXITNODE;
    exitNode->ID = 0;

    method->exitNode = exitNode;

    return;
}

void IRMETHOD_initSignature (ir_signature_t *signature, JITUINT32 parametersNumber) {

    /* Free the memory.
     */
    if (signature->parameterTypes != NULL) {
        freeFunction(signature->parameterTypes);
    }
    if (signature->ilParamsTypes != NULL) {
        freeFunction(signature->ilParamsTypes);
    }
    if (signature->ilParamsDescriptor != NULL) {
        freeFunction(signature->ilParamsDescriptor);
    }

    /* Allocate array for parameters and signatures.
     */
    if (parametersNumber > 0) {
        signature->parameterTypes     = sharedAllocFunction((sizeof(JITUINT32) * parametersNumber));
        signature->ilParamsTypes                   = sharedAllocFunction((sizeof(TypeDescriptor *) * parametersNumber));
        signature->ilParamsDescriptor             = sharedAllocFunction((sizeof(ParamDescriptor *) * parametersNumber));
    } else {
        signature->parameterTypes = NULL;
        signature->ilParamsTypes = NULL;
        signature->ilParamsDescriptor = NULL;
    }
    signature->parametersNumber = parametersNumber;

    return ;
}

JITUINT32 IRMETHOD_hashIRSignature (ir_signature_t *signature) {
    if (signature == NULL) {
        return 0;
    }
    JITUINT32 seed = signature->resultType;
    seed = combineHash(seed, (JITUINT32) (JITNUINT)(signature->ilResultType));
    JITUINT32 count;
    for (count = 0; count < signature->parametersNumber; count++) {
        seed = combineHash(seed, signature->parameterTypes[count]);
    }
    return seed;
}

JITINT32 IRMETHOD_equalsIRSignature (ir_signature_t *key1, ir_signature_t *key2) {
    if (key1 == key2) {
        return JITTRUE;
    }
    if (key1 == NULL || key2 == NULL) {
        return 0;
    }
    if (key1->resultType != key2->resultType) {
        return 0;
    }
    if (key1->parametersNumber != key2->parametersNumber) {
        return 0;
    }
    if (key1->parametersNumber > 0) {
        if (memcmp(key1->parameterTypes, key2->parameterTypes, sizeof(JITUINT32) * key1->parametersNumber) != 0) {
            return 0;
        }
    }
    return 1;
}

JITINT8 * IRMETHOD_IRSignatureInString (ir_signature_t *signature) {
    JITINT8 *completeName = (JITINT8*) "";
    JITINT8 *resultInString = signature->ilResultType->getCompleteName(signature->ilResultType);
    JITUINT32 param_count = signature->parametersNumber;
    JITINT8 **paramInString = NULL;

    if (param_count > 0) {
        paramInString = alloca(sizeof(JITINT8 *) * param_count);
    }
    JITUINT32 count = 0;
    JITUINT32 char_count = STRLEN(resultInString) + 1 + STRLEN(completeName) + 1 + 1 + 1;
    for (count = 0; count < param_count; count++) {
        paramInString[count] = (signature->ilParamsTypes)[count]->getCompleteName((signature->ilParamsTypes)[count]);
        char_count += STRLEN(paramInString[count]) + 1;
    }
    size_t length = sizeof(JITINT8) * (char_count);
    JITINT8 *result = allocFunction(length);
    SNPRINTF(result, length, "%s %s(", resultInString, completeName);
    if (param_count > 0) {
        for (count = 0; count < param_count - 1; count++) {
            result = STRCAT(result, paramInString[count]);
            result = STRCAT(result, (JITINT8 *) ",");
        }
        result = STRCAT(result, paramInString[count]);
    }
    result = STRCAT(result, (JITINT8 *) ")");
    return result;
}

void IRMETHOD_init (ir_method_t * method, JITINT8 *name) {
    pthread_mutexattr_t mutex_attr;

    memset(method, 0, sizeof(ir_method_t));
    method->name = name;
    method->valid_optimization = 0;
    method->var_max = 0;
    method->signature.resultType = NOPARAM;
    method->locals = xanList_new(sharedAllocFunction, freeFunction, NULL);
    method->instructions = sharedAllocFunction(sizeof(ir_instruction_t *) * BEGIN_INSTRUCTIONS_NUMBER);
    method->instructionsTop = START_IR_INSTRUCTIONS_ID;
    method->instructionsAllocated = BEGIN_INSTRUCTIONS_NUMBER;

    method->metadata = NULL;

    PLATFORM_initMutexAttr(&mutex_attr);
    PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    MEMORY_shareMutex(&mutex_attr);
    PLATFORM_initMutex(&(method->mutex), &mutex_attr);

    createExitNode(method);

    IRMETHOD_newInstructionOfType(method, IRNOP);

    return;
}

static inline void ir_alias_destroy (XanHashTable *alias) {
    if (alias == NULL) {
        return ;
    }
    XanHashTableItem *current_element = xanHashTable_first(alias);
    while (current_element != NULL) {
        XanList *cur_var_alias = (XanList *) (current_element->element);
        xanList_destroyList(cur_var_alias);
        current_element = xanHashTable_next(alias, current_element);
    }
    xanHashTable_destroyTable(alias);

    return ;
}

void IRMETHOD_destroyMemoryAliasInformation (ir_method_t *method) {
    JITUINT32	instructionsNumber;

    instructionsNumber	= ir_instr_count(method);
    for (JITUINT32 count=0; count < instructionsNumber; count++) {
        ir_instruction_t	*inst;
        inst	= ir_instr_get_by_pos(method, count);
        if (inst->metadata != NULL) {
            ir_alias_destroy((inst->metadata->alias).alias);
            (inst->metadata->alias).alias	= NULL;
        }
    }

    return ;
}

void IRMETHOD_setAlias (ir_method_t *method, ir_instruction_t *inst, JITUINT32 i, JITUINT32 j) {
    XanHashTable    *alias;
    XanList	 	*var1_alias_set;
    XanList	 	*var2_alias_set;
    void		*pI;
    void		*pJ;

    /* Assertions				*/
    assert(inst != NULL);
    assert(inst->metadata != NULL);

    /* Allocate the table			*/
    alias = (inst->metadata->alias).alias;
    if (alias == NULL) {
        alias	= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        (inst->metadata->alias).alias	= alias;
    }

    /* Allocate the lists			*/
    pI	= (void *) (JITNUINT)(i + 1);
    pJ	= (void *) (JITNUINT)(j + 1);
    var1_alias_set = (XanList *) xanHashTable_lookup(alias, pI);
    if (var1_alias_set == NULL) {
        var1_alias_set	= xanList_new(allocFunction, freeFunction, NULL);
        xanHashTable_insert(alias, pI, var1_alias_set);
    }
    var2_alias_set = (XanList *) xanHashTable_lookup(alias, pJ);
    if (var2_alias_set == NULL) {
        var2_alias_set	= xanList_new(allocFunction, freeFunction, NULL);
        xanHashTable_insert(alias, pJ, var2_alias_set);
    }

    /* Add the relation			*/
    if (xanList_find(var1_alias_set, pJ) == NULL) {
        xanList_append(var1_alias_set, pJ);
    }
    if (xanList_find(var2_alias_set, pI) == NULL) {
        xanList_append(var2_alias_set, pI);
    }

    return ;
}

JITBOOLEAN IRMETHOD_mayAlias (ir_method_t *method, ir_instruction_t *inst, JITUINT32 i, JITUINT32 j) {
    JITBOOLEAN not_empty_intesection;
    XanHashTable    *alias;

    /* Assertions					*/
    assert(inst != NULL);

    /* Check if the DDG analysis is available	*/
    if (!internal_is_valid_information(method, DDG_COMPUTER)) {
        return JITTRUE;
    }

    /* Check the reflexive property		*/
    if (i == j) {
        return JITTRUE;
    }

    not_empty_intesection = JITFALSE;
    alias = (inst->metadata->alias).alias;
    if (alias == NULL) {
        return JITFALSE;
    }

    XanList *var1_alias_set = (XanList *) xanHashTable_lookup(alias, (void *) (JITNUINT)(i + 1));
    XanList *var2_alias_set = (XanList *) xanHashTable_lookup(alias, (void *) (JITNUINT)(j + 1));
    if (    (var1_alias_set == NULL)        ||
            (var2_alias_set == NULL)        ) {
        return JITFALSE;
    }
    XanListItem *alias_class_item = xanList_first(var1_alias_set);
    while (alias_class_item != NULL) {
        void *current_class = alias_class_item->data;
        if (xanList_find(var2_alias_set, current_class) != NULL) {
            not_empty_intesection = JITTRUE;
            break;
        }
        alias_class_item = alias_class_item->next;
    }

    return not_empty_intesection;
}

static inline XanList * internal_getUses (ir_method_t *method, IR_ITEM_VALUE varID) {
    XanList                 *l;
    ir_instruction_t        *inst;
    JITINT32 blockBits;
    JITNUINT inst_use;
    JITNUINT temp;
    JITINT32 count;
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions                           */
    assert(method != NULL);
    assert(varID < IRMETHOD_getNumberOfVariablesNeededByMethod(method));

    /* Compute the size of the bitmaps	*/
    if (sizeof(JITNINT) == 4) {
        blockBits = 32;
    } else {
        blockBits = 64;
    }

    l = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = ir_instr_count(method);

    for (instID = 0; instID < instructionsNumber; instID++) {

        /* Fetch the current instruction	*/
        inst = ir_instr_get_by_pos(method, instID);
        assert(inst != NULL);

        count = varID / blockBits;
        inst_use = livenessGetUse(method, inst, count);
        temp = 0x1;
        temp = temp << (varID % blockBits);

        /* Check if the current variable is     *
         * used by the current instruction.	*/
        if ((inst_use & temp) != temp) {
            continue;
        }

        xanList_append(l, inst);
    }

    return l;
}

static inline XanHashTable * internal_getAllVariableDefinitions (ir_method_t *method) {
    JITUINT32 		instID;
    JITUINT32 		instructionsNumber;
    XanHashTable		*defs;

    /* Assertions.
     */
    assert(method != NULL);

    /* Allocate the table of definitions.
     */
    defs	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the number of instructions.
     */
    instructionsNumber = ir_instr_count(method);

    /* Find every definitions.
     */
    for (instID = ir_method_pars_count_get(method); instID < instructionsNumber; instID++) {
        ir_instruction_t        *opt_inst;
        IR_ITEM_VALUE 		reaching_inst_def;
        XanList                 *list;

        /* Fetch the instruction.
         */
        opt_inst = ir_instr_get_by_pos(method, instID);
        assert(opt_inst != NULL);

        /* Fetch the variable defined by the instruction.
         */
        reaching_inst_def	= IRMETHOD_getVariableDefinedByInstruction(method, opt_inst);
        if (reaching_inst_def == NOPARAM) {
            continue ;
        }

        /* Fetch the list of definitions of the current variable.
         */
        list		= xanHashTable_lookup(defs, (void *)(JITNUINT)(reaching_inst_def + 1));
        if (list == NULL) {
            list	= xanList_new(allocFunction, freeFunction, NULL);
            xanHashTable_insert(defs, (void *)(JITNUINT)(reaching_inst_def + 1), list);
        }
        assert(list != NULL);

        /* Append the definition.
         */
        if (xanList_find(list, opt_inst) == NULL) {
            xanList_insert(list, opt_inst);
        }
    }

    /* If varID is a parameter, then it has	to be considered as defined by the varID instruction of the method, which is, by definition, a IRNOP instruction.
     */
    for (instID = 0; instID < ir_method_pars_count_get(method); instID++) {
        ir_instruction_t        *opt_inst;
        XanList                 *list;

        /* Fetch the instruction.
         */
        opt_inst = ir_instr_get_by_pos(method, instID);
        assert(opt_inst != NULL);
        assert(IRMETHOD_getInstructionType(opt_inst) == IRNOP);

        /* Fetch the list of definitions of the current variable.
         */
        list		= xanHashTable_lookup(defs, (void *)(JITNUINT)(instID + 1));
        if (list == NULL) {
            list	= xanList_new(allocFunction, freeFunction, NULL);
            xanHashTable_insert(defs, (void *)(JITNUINT)(instID + 1), list);
        }
        assert(list != NULL);

        /* Append the definition.
         */
        if (xanList_find(list, opt_inst) == NULL) {
            xanList_insert(list, opt_inst);
        }
    }

    return defs;
}

static inline XanList * internal_getDefinitions (ir_method_t *method, IR_ITEM_VALUE varID) {
    ir_instruction_t        *opt_inst;
    JITUINT32 		instID;
    JITNUINT 		reaching_inst_def;
    XanList                 *list;
    JITUINT32 		instructionsNumber;

    /* Assertions.
     */
    assert(method != NULL);

    /* Initialize the variables.
     */
    opt_inst = NULL;

    /* Allocate the list.
     */
    list = xanList_new(allocFunction, freeFunction, NULL);
    assert(list != NULL);

    /* Fetch the number of instructions.
     */
    instructionsNumber = ir_instr_count(method);

    /* Find every definitions.
     */
    for (instID = ir_method_pars_count_get(method); instID < instructionsNumber; instID++) {
        opt_inst = ir_instr_get_by_pos(method, instID);
        assert(opt_inst != NULL);
        reaching_inst_def	= IRMETHOD_getVariableDefinedByInstruction(method, opt_inst);
        if (reaching_inst_def == varID) {
            assert((opt_inst->type == IRNOP) || IRMETHOD_getVariableDefinedByInstruction(method, opt_inst) == varID);
            xanList_insert(list, opt_inst);
        }
    }

    /* If varID is a parameter, then it has	to be considered as defined by the varID instruction of the method, which is, by definition, a IRNOP instruction.
     */
    if (varID < ir_method_pars_count_get(method)) {
        opt_inst = ir_instr_get_by_pos(method, varID);
        assert(opt_inst != NULL);
        assert(IRMETHOD_getInstructionType(opt_inst) == IRNOP);
        if (xanList_find(list, opt_inst) == NULL) {
            xanList_insert(list, opt_inst);
        }
    }
    assert(list != NULL);

    return list;
}

static inline JITINT32 ir_instr_may_instruction_access_existing_heap_memory (ir_method_t *method, ir_instruction_t *inst) {

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Check if the instruction is a	*
     * call					*/
    if (_IRMETHOD_isACallInstruction(inst)) {
        return JITTRUE;
    }

    /* Check if the instruction may read or	*
     * write to memory			*/
    switch (ir_instr_type_get(inst)) {
        case IRLOADREL:
        case IRSTOREREL:
        case IRINITMEMORY:
        case IRMEMCPY:
        case IRMEMCMP:
            return JITTRUE;
        case IRGETADDRESS:
            return JITFALSE;
    }

    /* Return				*/
    return ir_instr_has_escapes(method, inst);
}

static inline JITINT32 ir_instr_has_escapes (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 	count;
    JITUINT32	countParameters;
    ir_item_t	*par;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);

    /* Check the used variables.
     */
    countParameters	= ir_instr_pars_count_get(inst);
    for (count=0; count <= countParameters; count++) {
        par	= IRMETHOD_getInstructionParameter(inst, count);
        assert(par != NULL);
        if (!IRDATA_isAVariable(par)) {
            continue ;
        }

        /* Check if the current variable used escapes.
         */
        if (ir_method_escapes_isVariableEscaped(method, (par->value).v)) {
            return JITTRUE;
        }
    }
    countParameters	= ir_instrcall_pars_count_get(inst);
    for (count=0; count < countParameters ; count++) {
        par	= ir_instrcall_par_get(inst, count);
        assert(par != NULL);
        if (!IRDATA_isAVariable(par)) {
            continue ;
        }

        /* Check if the current variable used escapes.
         */
        if (ir_method_escapes_isVariableEscaped(method, (par->value).v)) {
            return JITTRUE;
        }
    }

    return JITFALSE;
}

static inline JITBOOLEAN internal_has_next_instruction_as_successor_without_jumps (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 type;

    type = ir_instr_type_get(inst);
    switch (type) {
        case IRTHROW:
        case IRCALLFINALLY:
        case IRENDFINALLY:
        case IRBRANCH:
        case IRENDFILTER:
        case IRRET:
        case IREXITNODE:
            return JITFALSE;
    }
    return JITTRUE;
}

static inline JITBOOLEAN internal_has_next_instruction_as_successor (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 type;
    ir_instruction_t        *next;

    type = ir_instr_type_get(inst);
    switch (type) {
        case IRBRANCH:
            next = ir_instr_get_next(method, inst);
            if (    (next->type == IRLABEL)                                 &&
                    ((next->param_1).value.v == (inst->param_1).value.v)        ) {
                return JITTRUE;
            }
            return JITFALSE;
        case IRTHROW:
            next = ir_instr_get_next(method, inst);
            if (next->type == IRSTARTCATCHER) {
                return JITTRUE;
            }
            return JITFALSE;
        case IRCALLFINALLY:
            next = ir_instr_get_next(method, inst);
            if (    (next->type == IRSTARTFINALLY)                          &&
                    ((next->param_1).value.v == (inst->param_1).value.v)        ) {
                return JITTRUE;
            }
            return JITFALSE;
        case IRENDFINALLY:
        case IRENDFILTER:
        case IRRET:
        case IREXITNODE:
            return JITFALSE;
    }
    return JITTRUE;
}

static inline JITBOOLEAN internal_is_a_dominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated, XanNode *tree) {
    XanNode         *node;

    /* Assertions					*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(dominated != NULL);
    assert(tree != NULL);

    if (inst == dominated) {

        /* Each instruction dominates itsef.
         */
        return JITTRUE;
    }

    /* Search the `inst` inside the dominator tree  */
    node = tree->find(tree, inst);
    assert(node != NULL);

    /* Search in the sub-tree from instruction `inst` the	*
     * instruction `dominated`				*/
    return (node->find(node, dominated) != NULL);
}

JITBOOLEAN IRMETHOD_doesInstructionDefineVariable (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID) {
    ir_item_t       *res;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    res = &(inst->result);

    if (    (res->type == IROFFSET) &&
            ((res->value).v == varID)   ) {
        return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_doInstructionsDefineVariable (ir_method_t *method, XanList *instructions, IR_ITEM_VALUE varID) {
    XanListItem             *item;
    ir_instruction_t        *inst;

    /* Check if the list is empty		*/
    if (    (instructions == NULL)          ||
            (instructions->len == 0)        ) {
        return JITFALSE;
    }

    item = xanList_first(instructions);
    while (item != NULL) {
        inst = item->data;
        assert(inst != NULL);
        if (IRMETHOD_doesInstructionDefineVariable(method, inst, varID)) {
            return JITTRUE;
        }
        item = item->next;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_doInstructionsUseVariable (ir_method_t *method, XanList *instructions, IR_ITEM_VALUE varID) {
    XanListItem             *item;
    ir_instruction_t        *inst;

    /* Check if the list is empty		*/
    if (    (instructions == NULL)          ||
            (instructions->len == 0)        ) {
        return JITFALSE;
    }

    item = xanList_first(instructions);
    while (item != NULL) {
        inst = item->data;
        assert(inst != NULL);
        if (IRMETHOD_doesInstructionUseVariable(method, inst, varID)) {
            return JITTRUE;
        }
        item = item->next;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_doesInstructionUseLabel (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE labelID) {
    ir_item_t       *item;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            assert(item != NULL);
            if (item->type == IRLABELITEM) {
                if ((item->value).v == labelID) {
                    return JITTRUE;
                }
            }
        case 2:
            item = ir_instr_par2_get(inst);
            assert(item != NULL);
            if (item->type == IRLABELITEM) {
                if ((item->value).v == labelID) {
                    return JITTRUE;
                }
            }
        case 1:
            item = ir_instr_par1_get(inst);
            if (item->type == IRLABELITEM) {
                if ((item->value).v == labelID) {
                    return JITTRUE;
                }
            }
            break;
    }

    return JITFALSE;
}

JITBOOLEAN IRMETHOD_doesInstructionUseVariable (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID) {
    ir_item_t       *item;
    JITUINT32 count2;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == varID) {
                    return JITTRUE;
                }
            }
        case 2:
            item = ir_instr_par2_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == varID) {
                    return JITTRUE;
                }
            }
        case 1:
            item = ir_instr_par1_get(inst);
            if (item->type == IROFFSET) {
                if ((item->value).v == varID) {
                    return JITTRUE;
                }
            }
            break;
    }

    /* Check the call parameters		*/
    for (count2 = 0; count2 < ir_instrcall_pars_count_get(inst); count2++) {
        item = ir_instrcall_par_get(inst, count2);
        if (    (item->type == IROFFSET)                &&
                ((item->value).v == varID)                  ) {
            return JITTRUE;
        }
    }

    /* Return				*/
    return JITFALSE;
}

void IRMETHOD_shiftVariableIDsHigherThanThreshold (ir_method_t *method, JITINT32 offset_value, JITINT32 threshold) {
    JITUINT32 count;

    /* Assertions				*/
    assert(method != NULL);

    /* Check if we need to do something	*/
    if (offset_value == 0) {
        return;
    }
    method->modified = JITTRUE;

    /* Change the variable IDs		*/
    for (count = START_IR_INSTRUCTIONS_ID; count < method->instructionsTop; count++) {
        ir_instruction_t        *instr;
        ir_item_t               *item;
        JITUINT32 count2;

        /* Get the next instruction		*/
        instr = method->instructions[count];

        /* Check the parameters			*/
        assert(instr != NULL);
        assert(instr->ID == count);
        switch (ir_instr_pars_count_get(instr)) {
            case 3:
                item = ir_instr_par3_get(instr);
                assert(item != NULL);
                if (    (item->type == IROFFSET)        &&
                        ((item->value).v > threshold)       ) {
                    ((item->value).v) += offset_value;
                }
            case 2:
                item = ir_instr_par2_get(instr);
                assert(item != NULL);
                if (    (item->type == IROFFSET)        &&
                        ((item->value).v > threshold)       ) {
                    ((item->value).v) += offset_value;
                }
            case 1:
                item = ir_instr_par1_get(instr);
                if (    (item->type == IROFFSET)        &&
                        ((item->value).v > threshold)       ) {
                    ((item->value).v) += offset_value;
                }
                break;
        }

        /* Check the result			*/
        item = ir_instr_result_get(instr);
        assert(item != NULL);
        if (    (item->type == IROFFSET)        &&
                ((item->value).v > threshold)       ) {
            ((item->value).v) += offset_value;
        }

        /* Check the call parameters		*/
        for (count2 = 0; count2 < ir_instrcall_pars_count_get(instr); count2++) {
            item = ir_instrcall_par_get(instr, count2);
            if (    (item->type == IROFFSET)        &&
                    ((item->value).v > threshold)       ) {
                ((item->value).v) += offset_value;
            }
        }
    }

    /* Update the number of variables needed*
     * by this method			*/
    ir_method_update_max_variable(method);

    /* Return				*/
    return;

}

void IRMETHOD_shiftVariableIDs (ir_method_t *method, JITINT32 offset_value) {
    JITUINT32 count;

    /* Assertions				*/
    assert(method != NULL);

    /* Check if we need to do something	*/
    if (offset_value == 0) {
        return;
    }
    method->modified = JITTRUE;

    /* Change the variable IDs		*/
    for (count = START_IR_INSTRUCTIONS_ID; count < method->instructionsTop; count++) {
        ir_instruction_t        *instr;
        ir_item_t               *item;

        /* Get the next instruction		*/
        instr = method->instructions[count];

        /* Check the parameters			*/
        assert(instr != NULL);
        assert(instr->ID == count);
        item = ir_instr_par3_get(instr);
        assert(item != NULL);
        if (item->type == IROFFSET) {
            ((item->value).v) += offset_value;
        }
        item = ir_instr_par2_get(instr);
        assert(item != NULL);
        if (item->type == IROFFSET) {
            ((item->value).v) += offset_value;
        }
        item = ir_instr_par1_get(instr);
        if (item->type == IROFFSET) {
            ((item->value).v) += offset_value;
        }

        /* Check the result			*/
        item = ir_instr_result_get(instr);
        assert(item != NULL);
        if (item->type == IROFFSET) {
            ((item->value).v) += offset_value;
        }

        /* Check the call parameters		*/
        if (instr->callParameters != NULL) {
            XanListItem *itemList;
            itemList = xanList_first(instr->callParameters);
            while (itemList != NULL) {
                item = (ir_item_t *) itemList->data;
                if (item->type == IROFFSET) {
                    ((item->value).v) += offset_value;
                }
                itemList = itemList->next;
            }
        }
    }

    /* Update the number of variables needed*
     * by this method			*/
    ir_method_update_max_variable(method);

    /* Return				*/
    return;
}

void libirmanagerCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

void libirmanagerCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libirmanagerVersion () {
    return VERSION;
}

/**
 * \brief Returns a bitset representing the set of instructions the given one depends on
 *
 * The bitset will have to be explicitly allocated after use
 */
XanBitSet * IRMETHOD_getControlDependencesBitSet (ir_method_t *method, ir_instruction_t *inst) {
    XanBitSet *result;
    XanListItem *depItem;
    ir_instruction_t *dep;
    XanList *controlDependencies;

    result = xanBitSet_new(IRMETHOD_getInstructionsNumber(method));

    controlDependencies = inst->metadata->controlDependencies;
    if (controlDependencies != NULL) {
        depItem = xanList_first(controlDependencies);
        while (depItem != NULL) {
            dep = depItem->data;

            xanBitSet_setBit(result, dep->ID);

            depItem = depItem->next;
        }
    }

    return result;
}

void IRMETHOD_dumpIRItem ( ir_method_t *method, ir_item_t *item, FILE *fileToWrite) {
    TypeDescriptor  *classID;
    ir_method_t     *otherMethod;
    ir_global_t	*g;

    JITINT8 buf[1024];

    switch (item->type) {
        case NOPARAM:
            break;
        case IRGLOBAL:
            g = (ir_global_t *)(JITNUINT)item->value.v;
            fprintf(fileToWrite, "Global[");
            if (g == NULL) {
                fprintf(fileToWrite, "Unset");
            } else {
                fprintf(fileToWrite, "%s", g->name);
            }
            fprintf(fileToWrite, "] ");
            IRMETHOD_dumpIRType(item->internal_type, fileToWrite);
            break ;
        case IRSYMBOL:
            IRMETHOD_dumpSymbol((ir_symbol_t *) (JITNUINT) (item->value).v, fileToWrite);
            fprintf(fileToWrite, " [%s]", IRMETHOD_getIRTypeName(item->internal_type));
            fprintf(fileToWrite, " ");
            break;
        case IRMETHODID:
            otherMethod = IRMETHOD_getIRMethodFromMethodID(method, (item->value).v);
            if (otherMethod != NULL) {
                fprintf(fileToWrite, "%s ", IRMETHOD_getMethodName(otherMethod));
            } else {
                fprintf(fileToWrite, "%p ", otherMethod);
            }
            break;
        case IRCLASSID:
            classID = (TypeDescriptor *) (JITNUINT) (item->value).v;
            fprintf(fileToWrite, "Type %s ", IRMETHOD_getClassName(classID));
            break;
        case IROFFSET:

            fprintf(fileToWrite, "Variable[ ");
            IRMETHOD_dumpIRType(item->internal_type, fileToWrite);
            fprintf(fileToWrite, "] %lld ", (item->value).v);
            break;
        case IRNINT:
            fprintf(fileToWrite, "Unmanaged Pointer %p ", (void *) (JITNUINT) (item->value).v);
            break;
        case IRMPOINTER:
            fprintf(fileToWrite, "Managed Pointer %p ", (void *) (JITNUINT) (item->value).v);
            break;
        case IRTPOINTER:
            fprintf(fileToWrite, "Transient Pointer %p ", (void *) (JITNUINT) (item->value).v);
            break;
        case IRFPOINTER:
        case IRMETHODENTRYPOINT:
            //fprintf(fileToWrite, "Function Pointer %p ", (void *) (JITNUINT) (item->value).v);
            fprintf(fileToWrite, "Function Pointer ");
            break;
        case IROBJECT:
            fprintf(fileToWrite, "Object Pointer %p ", (void *) (JITNUINT) (item->value).v);
            break;
        case IRVALUETYPE:
            fprintf(fileToWrite, "Value type ");
            break;
        case IRSIGNATURE:
            fprintf(fileToWrite, "Signature ");
            break;
        case IRINT8:
            fprintf(fileToWrite, "INT8 %lld ", (item->value).v);
            break;
        case IRINT16:
            fprintf(fileToWrite, "INT16 %lld ", (item->value).v);
            break;
        case IRINT32:
            fprintf(fileToWrite, "INT32 %lld ", (item->value).v);
            break;
        case IRINT64:
            fprintf(fileToWrite, "INT64 %lld ", (item->value).v);
            break;
        case IRUINT8:
            fprintf(fileToWrite, "UINT8 %lld ", (item->value).v);
            break;
        case IRUINT16:
            fprintf(fileToWrite, "UINT16 %lld ", (item->value).v);
            break;
        case IRUINT32:
            fprintf(fileToWrite, "UINT32 %lld ", (item->value).v);
            break;
        case IRUINT64:
            fprintf(fileToWrite, "UINT64 %lld ", (item->value).v);
            break;
        case IRNUINT:
            fprintf(fileToWrite, "NUINT %lld ", (item->value).v);
            break;
        case IRFLOAT32:
            fprintf(fileToWrite, "FLOAT32 %f ", (item->value).f);
            break;
        case IRNFLOAT:
            fprintf(fileToWrite, "NFLOAT %f ", (item->value).f);
            break;
        case IRFLOAT64:
            fprintf(fileToWrite, "FLOAT64 %f ", (item->value).f);
            break;
        case IRSTRING:
            fprintf(fileToWrite, "\\\"%s\\\"", (char *) (JITNUINT) (item->value).v);
            break;
        case IRTYPE:
            IRMETHOD_dumpIRType((item->value).v, fileToWrite);
            break;
        case IRLABELITEM:
            fprintf(fileToWrite, "%llu ", (item->value).v);
            break;
        case IRVOID:
            fprintf(fileToWrite, "VOID ");
            break;
        default:
            SNPRINTF(buf, sizeof(char) * 1024, "ERROR = IR type %u is not known. ", item->type);
            PRINT_ERR(buf, 0);
            abort();
    }
}

JITINT8 * IRMETHOD_getIRTypeName (JITUINT32 type) {
    switch ((JITINT16)type) {
        case IRINT8:
            return (JITINT8 *) "INT8";
        case IRINT16:
            return (JITINT8 *) "INT16";
        case IRINT32:
            return (JITINT8 *) "INT32";
        case IRINT64:
            return (JITINT8 *) "INT64";
        case IRNINT:
            return (JITINT8 *) "NINT";
        case IRUINT8:
            return (JITINT8 *) "UINT8";
        case IRUINT16:
            return (JITINT8 *) "UINT16";
        case IRUINT32:
            return (JITINT8 *) "UINT32";
        case IRUINT64:
            return (JITINT8 *) "UINT64";
        case IRNUINT:
            return (JITINT8 *) "NUINT";
        case IRFLOAT32:
            return (JITINT8 *) "FLOAT32";
        case IRFLOAT64:
            return (JITINT8 *) "FLOAT64";
        case IRNFLOAT:
            return (JITINT8 *) "NFLOAT";
        case IRMPOINTER:
            return (JITINT8 *) "MPOINTER";
        case IRTPOINTER:
            return (JITINT8 *) "TPOINTER";
        case IRFPOINTER:
            return (JITINT8 *) "FPOINTER";
        case IROBJECT:
            return (JITINT8 *) "OBJECT";
        case IRVALUETYPE:
            return (JITINT8 *) "VALUETYPE";
        case IRTYPE:
            return (JITINT8 *) "TYPE";
        case IRCLASSID:
            return (JITINT8 *) "CLASSID";
        case IRMETHODID:
            return (JITINT8 *) "METHODID";
        case IRMETHODENTRYPOINT:
            return (JITINT8 *) "METHODENTRYPOINT";
        case IRLABELITEM:
            return (JITINT8 *) "LABELITEM";
        case IRSIGNATURE:
            return (JITINT8 *) "SIGNATURE";
        case IRVOID:
            return (JITINT8 *) "VOID";
        case IRSTRING:
            return (JITINT8 *) "STRING";
        case IROFFSET:
            return (JITINT8 *) "VAR";
        case IRSYMBOL:
            return (JITINT8 *) "SYMBOL";
        case IRGLOBAL:
            return (JITINT8 *) "GLOBAL";
        case NOPARAM:
            break;
        default:
            fprintf(stderr, "ERROR = IR type %u is not known.\n", type);
            abort();
    }

    return NULL;
}

void IRMETHOD_dumpIRType (JITUINT32 type, FILE *fileToWrite) {
    char	*n;
    n	= (char *)IRMETHOD_getIRTypeName(type);
    if (n != NULL) {
        fprintf(fileToWrite, "%s ", n);
    }
    return ;
}

ir_instruction_t * IRMETHOD_getImmediatePredominator (ir_method_t *method, ir_instruction_t *inst) {
    XanNode         *node;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Fetch the predominator tree		*/
    node = method->preDominatorTree;
    if (node == NULL) {
        return NULL;
    }

    /* Find the instruction given as input	*/
    node = node->find(node, inst);
    assert(node != NULL);

    /* Get the immediate predominator	*/
    node = node->getParent(node);
    if (node == NULL) {
        return NULL;
    }

    /* Return				*/
    return node->getData(node);
}

JITBOOLEAN IRMETHOD_doesInstructionDefineAVariable (ir_method_t *method, ir_instruction_t *inst) {
    ir_item_t       *res;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    res = &(inst->result);

    if (res->type == IROFFSET) {
        return JITTRUE;
    }
    return JITFALSE;
}

ir_item_t * IRMETHOD_getIRItemOfVariableDefinedByInstruction (ir_method_t *method, ir_instruction_t *inst) {
    ir_item_t       *res;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    res = &(inst->result);

    if (res->type == IROFFSET) {
        return res;
    }
    return NULL;
}

XanList * IRMETHOD_getVariablesUsedByInstruction (ir_instruction_t *inst) {
    XanList		*l;
    ir_item_t	*par;
    JITUINT32	count;

    /* Allocate the memory.
     */
    l	= xanList_new(allocFunction, freeFunction, NULL);

    for (count=1; count <= ir_instr_pars_count_get(inst); count++) {
        par	= IRMETHOD_getInstructionParameter(inst, count);
        assert(par != NULL);
        if (IRDATA_isAVariable(par)) {
            xanList_append(l, par);
        }
    }
    for (count=0; count < ir_instrcall_pars_count_get(inst); count++) {
        par	= ir_instrcall_par_get(inst, count);
        assert(par != NULL);
        if (IRDATA_isAVariable(par)) {
            xanList_append(l, par);
        }
    }

    return l;
}

XanHashTable * IRMETHOD_getInstructionsThatCanHaveBeenExecutedBeforeInstruction (ir_method_t *method, ir_instruction_t *inst) {
    data_flow_t	*reachableInstructions;
    XanHashTable	*t;
    JITUINT32	instructionsNumber;
    JITUINT32	instID;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Allocate the table.
     */
    t			= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the data flow.
     */
    reachableInstructions	= IRMETHOD_getMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    if (reachableInstructions == NULL) {
        fprintf(stderr, "ILDJIT: ERROR = Reachable instruction data flow is missing. Probably the plugin has not been called.\n");
        abort();
    }

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber	= ir_instr_count(method);

    /* Collect the instructions.
     */
    for (instID=0; instID < instructionsNumber; instID++) {
        ir_instruction_t	*currentInst;

        /* Fetch the instruction.
         */
        currentInst	= ir_instr_get_by_pos(method, instID);

        /* If inst can be reached from the current instruction, then add it to the set.
         */
        if (internal_is_instruction_reachable_in(reachableInstructions, inst, currentInst)) {
            xanHashTable_insert(t, currentInst, currentInst);
        }
    }

    return t;
}

XanHashTable * IRMETHOD_getInstructionsThatCanBeReachedFromPosition (ir_method_t *method, ir_instruction_t *position) {
    data_flow_t	*reachableInstructions;
    XanHashTable	*t;
    JITUINT32	instructionsNumber;

    /* Assertions.
     */
    assert(method != NULL);
    assert(position != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, position));

    /* Allocate the table.
     */
    t			= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the data flow.
     */
    reachableInstructions	= IRMETHOD_getMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    if (reachableInstructions == NULL) {
        fprintf(stderr, "ILDJIT: ERROR = Reachable instruction data flow is missing. Probably the plugin has not been called.\n");
        abort();
    }

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber	= ir_instr_count(method);

    /* Fetch all instructions that can be reached from position.
     */
    internal_addInstructionsThatCanBeReachedFromPosition(method, t, position->ID, reachableInstructions, instructionsNumber, NULL);

    return t;
}

JITBOOLEAN IRMETHOD_canInstructionBeReachedFromPosition (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *position) {
    data_flow_t	*reachableInstructions;

    /* Assertions.
     */
    assert(method != NULL);
    assert(position != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, position));
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    /* Fetch the data flow.
     */
    reachableInstructions	= IRMETHOD_getMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    if (reachableInstructions == NULL) {
        fprintf(stderr, "ILDJIT: ERROR = Reachable instruction data flow is missing. Probably the plugin has not been called.\n");
        abort();
    }
    return internal_is_instruction_reachable_in(reachableInstructions, inst, position);
}

IR_ITEM_VALUE IRMETHOD_getVariableDefinedByInstruction (ir_method_t *method, ir_instruction_t *inst) {
    ir_item_t       *res;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    res = &(inst->result);

    if (res->type == IROFFSET) {
        return (res->value).v;
    }
    return NOPARAM;
}

JITUINT32 IRMETHOD_getSuccessorsNumber (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 		count;
    JITBOOLEAN		catcherInstInitialized;
    ir_instruction_t        *pred;
    ir_instruction_t	*catcherInst;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize the variables		*/
    count = 0;
    pred = NULL;

    /* Compute the number of predecessor	*/
    catcherInst		= NULL;
    catcherInstInitialized	= JITFALSE;
    while ((pred = ir_instr_get_successor(method, inst, pred, &catcherInst, &catcherInstInitialized))) {
        count++;
    }

    return count;
}

JITUINT32 IRMETHOD_getPredecessorsNumber (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 		count;
    JITBOOLEAN		catcherInstInitialized;
    ir_instruction_t        *pred;
    ir_instruction_t	*catcherInst;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize the variables		*/
    count = 0;
    pred = NULL;
    catcherInst	= NULL;
    catcherInstInitialized	= JITFALSE;

    /* Compute the number of predecessor	*/
    while ((pred = ir_instr_get_predecessor(method, inst, pred, &catcherInst, &catcherInstInitialized))) {
        count++;
    }

    return count;
}

JITINT32 IRMETHOD_tryLock (ir_method_t *method) {

    /* Assertions				*/
    assert(method != NULL);

    return PLATFORM_trylockMutex(&(method->mutex));
}

JITBOOLEAN IRMETHOD_doInstructionsBelongToMethod (ir_method_t *method, XanList *insts) {
    XanListItem	*item;

    if (insts == NULL) {
        return JITTRUE;
    }

    item	= xanList_first(insts);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (!IRMETHOD_doesInstructionBelongToMethod(method, inst)) {
            return JITFALSE;
        }
        item	= item->next;
    }
    return JITTRUE;
}

JITBOOLEAN IRMETHOD_doesInstructionBelongToMethod (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 count;
    JITUINT32 instructionsNumber;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Check if the instruction is	*
     * stored inside the array.	*
     * Notice that we cannot read	*
     * anything from the memory     *
     * address inst since it could	*
     * be not a correct address	*/
    instructionsNumber	= ir_instr_count(method);
    for (count = 0; count <= instructionsNumber; count++) {
        if (ir_instr_get_by_pos(method, count) == inst) {
            assert(inst->ID == count);
            return JITTRUE;
        }
    }
    return JITFALSE;
}

ir_method_t * IRMETHOD_getCalledMethod (ir_method_t *method, ir_instruction_t *callInstruction) {
    ir_method_t     *calledMethod;

    /* Assertions			*/
    assert(method != NULL);
    assert(callInstruction != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, callInstruction));

    /* Initialize the variables	*/
    calledMethod = NULL;

    switch (callInstruction->type) {
        case IRCALL:
        case IRLIBRARYCALL:
            calledMethod = ir_library->getIRMethod(method, (callInstruction->param_1).value.v, JITFALSE);
            break;
    }

    /* Return			*/
    return calledMethod;
}

ir_method_t * IRMETHOD_getIRMethodFromEntryPoint (void *entryPointAddress) {
    ir_method_t     *method;

    /* Check the address		*/
    if (entryPointAddress == NULL) {
        return NULL;
    }

    /* Fetch the method		*/
    method = ir_library->getIRMethodFromEntryPoint(entryPointAddress);

    /* Return			*/
    return method;
}

IR_ITEM_VALUE IRMETHOD_getMethodID (ir_method_t *method) {

    /* Assertions			*/
    assert(method != NULL);

    /* Return			*/
    return ir_library->getIRMethodID(method);
}

XanList *IRMETHOD_getCallableIRMethods (ir_instruction_t *inst) {
    XanList         *result;
    XanList         *ids;
    XanListItem     *item;

    /* Assertions			*/
    assert(inst != NULL);

    /* Allocate the list of IR	*
     * methods			*/
    result = xanList_new(allocFunction, freeFunction, NULL);
    assert(result != NULL);

    /* Fill up the list		*/
    ids = IRMETHOD_getCallableMethods(inst);
    item = xanList_first(ids);
    while (item != NULL) {
        IR_ITEM_VALUE id;
        ir_method_t     *m;
        id = (IR_ITEM_VALUE) (JITNUINT) item->data;
        if (id != 0) {
            m = IRMETHOD_getIRMethodFromMethodID(NULL, id);
            assert(m != NULL);
            xanList_append(result, m);
        }
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(ids);

    /* Return			*/
    return result;
}

XanList *IRMETHOD_getCallableMethods (ir_instruction_t *inst) {
    XanList *result;

    /* Assertions			*/
    assert(inst != NULL);

    switch (inst->type) {
        case IRCALL:
            result = xanList_new(allocFunction, freeFunction, NULL);
            if (((inst->param_1).value.v) != 0) {
                xanList_append(result, (void *) (JITNUINT) ((inst->param_1).value.v));
            }
            break;
        case IRVCALL:
            result = IRPROGRAM_getImplementationsOfMethod((inst->param_1).value.v);
            break;
        case IRICALL:
            result = IRPROGRAM_getCompatibleMethods((ir_signature_t *) (JITNUINT) (inst->param_3).value.v);
            break;
        default:
            result = xanList_new(allocFunction, freeFunction, NULL);
    }

    return result;
}

JITBOOLEAN IRMETHOD_mayAccessMemory (ir_method_t *method, ir_instruction_t *inst) {
    return _IRMETHOD_mayAccessMemory(method, inst);
}

static inline JITBOOLEAN _IRMETHOD_mayAccessMemory (ir_method_t *method, ir_instruction_t *inst) {

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Check if the instruction may	*
     * access the heap		*/
    if (ir_instr_may_instruction_access_existing_heap_memory(method, inst)) {
        return JITTRUE;
    }

    /* Check if the instruction may	*
     * access to new memory or to	*
     * the stack			*/
    switch (inst->type) {
        case IRNEWOBJ:
        case IRNEWARR:
        case IRALLOCA:
        case IRCALLOC:
        case IRREALLOC:
            return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

static inline IR_ITEM_VALUE _IRMETHOD_getInstructionResultValue (ir_instruction_t *inst) {
    ir_item_t       *item;

    /* Assertions			*/
    assert(inst != NULL);

    /* Fetch the result		*/
    item = ir_instr_result_get(inst);
    assert(item != NULL);

    /* Return			*/
    return (item->value).v;
}

static inline JITUINT32 _IRMETHOD_getInstructionResultType (ir_instruction_t *inst) {
    ir_item_t       *item;

    /* Assertions			*/
    assert(inst != NULL);

    /* Fetch the result		*/
    item = ir_instr_result_get(inst);
    assert(item != NULL);

    /* Return			*/
    return item->type;
}

static inline JITBOOLEAN _IRMETHOD_isAFloatType (JITUINT32 type) {
    switch (type) {
        case IRFLOAT32:
        case IRFLOAT64:
        case IRNFLOAT:

            /* Return			*/
            return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

static inline JITUINT32 _IRMETHOD_getInstructionResultInternalType (ir_instruction_t *inst) {
    ir_item_t       *item;

    /* Assertions			*/
    assert(inst != NULL);

    /* Fetch the result		*/
    item = ir_instr_result_get(inst);
    assert(item != NULL);

    /* Return			*/
    return item->internal_type;
}

void _IRMETHOD_cpInstructionCallParameter (ir_instruction_t *inst, JITUINT32 nth, ir_item_t *from) {
    ir_item_t       *param;

    /* Assertions			*/
    assert(inst != NULL);
    assert(from != NULL);

    /* Fetch the parameter		*/
    param = ir_instrcall_par_get(inst, nth);
    if (param == NULL) {
        JITINT8 buf[DIM_BUF];
        SNPRINTF(buf, DIM_BUF, "IRMETHOD_cpInstructionCallParameter: ERROR = the instruction does not have the %u parameter. ", nth);
        print_err((char *) buf, 0);
        abort();
    }
    memcpy(param, from, sizeof(ir_item_t));

    /* Return			*/
    return;
}

JITUINT32 IRMETHOD_getInstructionResultInternalType (ir_instruction_t *inst) {
    return _IRMETHOD_getInstructionResultInternalType(inst);
}

JITUINT32 IRMETHOD_getInstructionResultType (ir_instruction_t *inst) {
    return _IRMETHOD_getInstructionResultType(inst);
}

IR_ITEM_VALUE IRMETHOD_getInstructionResultValue (ir_instruction_t *inst) {
    return _IRMETHOD_getInstructionResultValue(inst);
}

void IRMETHOD_cpInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth, ir_item_t *from) {
    _IRMETHOD_cpInstructionCallParameter(inst, nth, from);
    method->modified = JITTRUE;
}

XanList * IRMETHOD_getOutermostCircuits (ir_method_t *method) {
    XanList         *l;
    XanNode         *child;
    circuit_t          *loop;

    /* Assertions				*/
    assert(method != NULL);

    /* Check the loops nesting tree		*/
    if (method->loopNestTree == NULL) {
        if (method->loop != NULL) {
            l = xanList_cloneList(method->loop);
        } else {
            l = xanList_new(allocFunction, freeFunction, NULL);
        }
        return l;
    }
    assert(method->loopNestTree != NULL);

    /* Allocate the list			*/
    l = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the outermost loop		*/
    loop = method->loopNestTree->getData(method->loopNestTree);
    assert(loop != NULL);

    /* Check the outermost loop		*/
    if (loop->loop_id != 0) {

        /* The outermost loop contains every others	*/
        xanList_append(l, loop);

        /* Return					*/
        return l;
    }

    /* The outermost loop is a fake node	*
     * that represent the CFG		*/
    child = method->loopNestTree->getNextChildren(method->loopNestTree, NULL);
    assert(child != NULL);
    while (child != NULL) {

        /* Fetch the loop			*/
        loop = (circuit_t *) child->getData(child);
        assert(loop != NULL);

        /* Add the current loop to the list	*/
        xanList_append(l, loop);

        /* Get the next children of the fake	*
         * outermost loop			*/
        child = method->loopNestTree->getNextChildren(method->loopNestTree, child);
    }

    /* Return				*/
    return l;
}

XanList * IRMETHOD_getPostDominators (ir_method_t *method, ir_instruction_t *inst) {
    XanList         *l;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Fetch the list			*/
    l = internal_getDominators(inst, method->postDominatorTree);
    assert(l != NULL);

    /* Return				*/
    return l;
}

XanList * IRMETHOD_getPreDominators (ir_method_t *method, ir_instruction_t *inst) {
    XanList         *l;

    /* Assertions				*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Fetch the list			*/
    l = internal_getDominators(inst, method->preDominatorTree);
    assert(l != NULL);

    /* Return				*/
    return l;
}

void IRMETHOD_lock (ir_method_t *method) {
    ir_method_lock(method);
}

void IRMETHOD_unlock (ir_method_t *method) {
    ir_method_unlock(method);
}

JITBOOLEAN IRMETHOD_addInstructionControlDependence (ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst) {
    JITBOOLEAN	added;

    added	= JITFALSE;
    if (fromInst->metadata == NULL) {
        IRMETHOD_allocateMethodExtraMemory(method);
    }
    assert(fromInst->metadata != NULL);

    if (fromInst->metadata->controlDependencies == NULL) {
        fromInst->metadata->controlDependencies	= xanList_new(allocFunction, freeFunction, NULL);
    }
    added	= (xanList_find(fromInst->metadata->controlDependencies, toInst) == NULL);
    if (added) {
        xanList_insert(fromInst->metadata->controlDependencies, toInst);
    }

    return added;
}

JITBOOLEAN IRMETHOD_addInstructionDataDependence (ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst, JITUINT16 depType, XanList *outsideInstructions) {
    return ir_instr_add_data_dependency(method, fromInst, toInst, depType, outsideInstructions);
}

void IRMETHOD_destroyInstructionDataDependences (ir_method_t *method, ir_instruction_t *inst) {

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    if ((inst->metadata->data_dependencies).dependingInsts != NULL) {
        XanHashTableItem *item;
        while ((item = xanHashTable_first((inst->metadata->data_dependencies).dependingInsts)) != NULL) {
            data_dep_arc_list_t *dd = item->element;
            ir_instruction_t *otherInst;
            assert(dd != NULL);
            assert(dd->inst != NULL);
            otherInst = dd->inst;
            IRMETHOD_deleteInstructionsDataDependence(method, inst, otherInst);
            IRMETHOD_deleteInstructionsDataDependence(method, otherInst, inst);
        }
        xanHashTable_destroyTable((inst->metadata->data_dependencies).dependingInsts);
        (inst->metadata->data_dependencies).dependingInsts = NULL;
    }
    if ((inst->metadata->data_dependencies).dependsFrom != NULL) {
        XanHashTableItem *item;
        while ((item = xanHashTable_first((inst->metadata->data_dependencies).dependsFrom)) != NULL) {
            data_dep_arc_list_t *dd = item->element;
            ir_instruction_t *otherInst;
            assert(dd != NULL);
            assert(dd->inst != NULL);
            assert(inst == dd->inst || IRMETHOD_doesInstructionBelongToMethod(method, dd->inst));
            otherInst = dd->inst;
            IRMETHOD_deleteInstructionsDataDependence(method, inst, otherInst);
            IRMETHOD_deleteInstructionsDataDependence(method, otherInst, inst);
        }
        xanHashTable_destroyTable((inst->metadata->data_dependencies).dependsFrom);
        (inst->metadata->data_dependencies).dependsFrom = NULL;
    }

    /* Destroy the memory aliases information.
     */
    ir_alias_destroy((inst->metadata->alias).alias);
    (inst->metadata->alias).alias	= NULL;

    return ;
}

void IRMETHOD_destroyDataDependences (ir_method_t *method) {
    JITUINT32	instructionsNumber;

    /* Assertions.
     */
    assert(method != NULL);

    /* Destroy data dependences.
     */
    instructionsNumber	= ir_instr_count(method);
    for (JITUINT32 count=0; count < instructionsNumber; count++) {
        ir_instruction_t	*inst;

        /* Fetch an instruction.
         */
        inst	= ir_instr_get_by_pos(method, count);
        if (inst->metadata == NULL) {
            continue ;
        }

        /* Destroy dependences from.
         */
        IRMETHOD_destroyInstructionDataDependences(method, inst);
    }

    /* Destroy memory aliases.
     */
    IRMETHOD_destroyMemoryAliasInformation(method);

    return ;
}

void IRMETHOD_removeInstructionDataDependence (ir_method_t *method, ir_instruction_t *depending, ir_instruction_t *toNode, JITUINT16 depType) {
    ir_instr_remove_data_dependency(method, depending, toNode, depType);
}

ir_instruction_t * IRMETHOD_getCatcherInstruction (ir_method_t *method) {
    return ir_instrcatcher_get(method);
}

ir_instruction_t * IRMETHOD_getSuccessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevSuccessor) {
    ir_instruction_t        *pred;

    pred = ir_instr_get_successor(method, inst, prevSuccessor, NULL, NULL);
    assert(inst != pred);
#ifdef DEBUG
    if (pred != NULL) {
        assert(pred != prevSuccessor);
    }

#endif
    return pred;
}

JITBOOLEAN IRMETHOD_isASuccessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *succ) {
    ir_instruction_t 	*successor;
    ir_instruction_t	*catcherInst;
    JITBOOLEAN		catcherInstInitialized;

    /* Initialize the variables.
     */
    successor		= NULL;
    catcherInst		= NULL;
    catcherInstInitialized	= JITFALSE;

    /* Check whether the instruction is a successor or not.
     */
    while ((successor = ir_instr_get_successor(method, inst, successor, &catcherInst, &catcherInstInitialized))) {
        if (succ == successor) {
            return JITTRUE;
        }
    }

    return JITFALSE;
}

ir_instruction_t * IRMETHOD_getPredecessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prevPredecessor) {
    ir_instruction_t        *pred;

    pred = ir_instr_get_predecessor(method, inst, prevPredecessor, NULL, NULL);
#ifdef DEBUG
    if (pred != NULL) {
        assert(pred != prevPredecessor);
    }

#endif
    return pred;
}

JITBOOLEAN IRMETHOD_isAPredecessorInstruction (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *pred) {
    ir_instruction_t *predecessor = NULL;
    ir_instruction_t	*catcherInst;
    JITBOOLEAN		catcherInstInitialized;

    catcherInst	= NULL;
    catcherInstInitialized	= JITFALSE;
    while ((predecessor = ir_instr_get_predecessor(method, inst, predecessor, &catcherInst, &catcherInstInitialized))) {
        if (pred == predecessor) {
            return JITTRUE;
        }
    }

    return JITFALSE;
}

JITINT32 IRMETHOD_isAMemoryDataDependenceType (JITUINT16 depType) {
    return (depType >= DEP_MRAW);
}

JITUINT16 IRMETHOD_getInstructionDataDependence (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to) {
    return ir_instr_get_data_dependence(method, from, to);
}

JITINT32 IRMETHOD_isInstructionDataDependent (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to) {
    return (ir_instr_get_data_dependence(method, from, to) != 0);
}

JITINT32 IRMETHOD_isInstructionDataDependentThroughMemory (ir_method_t *method, ir_instruction_t *from, ir_instruction_t *to) {
    JITUINT16	dep;

    /* Check if the DDG analysis is available	*/
    if (!internal_is_valid_information(method, DDG_COMPUTER)) {
        return JITTRUE;
    }

    dep	= ir_instr_get_data_dependence(method, from, to);
    if (	((dep & DEP_MRAW) == 0)	&&
            ((dep & DEP_MWAW) == 0)	&&
            ((dep & DEP_MWAR) == 0)	) {
        return JITFALSE;
    }
    return JITTRUE;
}

void IRMETHOD_moveInstructionAfter (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *succ) {
    method->modified = JITTRUE;
    ir_instr_move_after(method, inst, succ);
    return ;
}

void IRMETHOD_moveInstructionBefore (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prev) {
    method->modified = JITTRUE;
    ir_instr_move_before(method, inst, prev);
    return ;
}

void IRMETHOD_scheduleInstructionBefore (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *prev) {
    method->modified = JITTRUE;
    if (prev == NULL) {
        ir_instr_move_before(method, inst, prev);
        return;
    }
    if (prev->type == IRLABEL) {
        ir_instr_move_after(method, inst, prev);
        return;
    }
    while (prev->type == IRBRANCH) {
        prev = ir_instr_get_prev(method, prev);
    }
    assert(prev != NULL);
    ir_instr_move_before(method, inst, prev);
    return;
}

void IRMETHOD_newVariable (ir_method_t *method, ir_item_t *newVariable, JITUINT16 internal_type, TypeDescriptor *typeInfos) {
    method->modified 		= JITTRUE;

    /* Allocate a new variable.
     */
    memset(newVariable, 0, sizeof(ir_item_t));
    (newVariable->value).v		= ir_instr_new_variable_ID(method);
    newVariable->type		= IROFFSET;
    newVariable->internal_type	= internal_type;
    newVariable->type_infos		= typeInfos;

    return ;
}

IR_ITEM_VALUE IRMETHOD_newVariableID (ir_method_t *method) {
    method->modified = JITTRUE;
    return ir_instr_new_variable_ID(method);
}

JITBOOLEAN IRMETHOD_isInstructionAPredominatorOfAllInstructions (ir_method_t *method, ir_instruction_t *inst, XanList *dominatedInstructions) {
    XanListItem	*item;
    item	= xanList_first(dominatedInstructions);
    while (item != NULL) {
        ir_instruction_t	*dom;
        dom	= item->data;
        if (!IRMETHOD_isInstructionAPredominator(method, inst, dom)) {
            return JITFALSE;
        }
        item	= item->next;
    }
    return JITTRUE;
}

JITBOOLEAN IRMETHOD_isInstructionAPredominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated) {
    return (JITBOOLEAN) ir_instr_is_a_predominator(method, inst, dominated);
}

JITBOOLEAN IRMETHOD_isInstructionAPostdominator (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dominated) {
    JITBOOLEAN result;

    result = ir_instr_is_a_postdominator(method, inst, dominated);
    return result;
}

XanList * IRMETHOD_getVariableUses (ir_method_t *method, IR_ITEM_VALUE varID) {
    return internal_getUses(method, varID);
}

XanList * IRMETHOD_getVariableUsesWithinInstructions (ir_method_t *method, IR_ITEM_VALUE varID, XanList *instructions) {
    XanList		*l;
    XanList		*toRemove;
    XanListItem	*item;

    l	= internal_getUses(method, varID);
    if (instructions == NULL) {
        return l;
    }

    toRemove	= xanList_new(allocFunction, freeFunction, NULL);
    item		= xanList_first(l);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (xanList_find(instructions, inst) == NULL) {
            xanList_append(toRemove, inst);
        }
        item	= item->next;
    }
    item	= xanList_first(toRemove);
    while (item != NULL) {
        xanList_removeElement(l, item->data, JITTRUE);
        item	= item->next;
    }

    xanList_destroyList(toRemove);

    return l;
}

XanList * IRMETHOD_getVariableDefinitionsWithinInstructions (ir_method_t *method, IR_ITEM_VALUE varID, XanList *instructions) {
    XanList		*l;
    XanList		*toRemove;
    XanListItem	*item;

    l	= internal_getDefinitions(method, varID);
    if (instructions == NULL) {
        return l;
    }

    toRemove	= xanList_new(allocFunction, freeFunction, NULL);
    item		= xanList_first(l);
    while (item != NULL) {
        ir_instruction_t	*inst;
        inst	= item->data;
        if (xanList_find(instructions, inst) == NULL) {
            xanList_append(toRemove, inst);
        }
        item	= item->next;
    }
    item	= xanList_first(toRemove);
    while (item != NULL) {
        xanList_removeElement(l, item->data, JITTRUE);
        item	= item->next;
    }

    xanList_destroyList(toRemove);

    return l;
}

XanList * IRMETHOD_getVariableDefinitions (ir_method_t *method, IR_ITEM_VALUE varID) {
    return internal_getDefinitions(method, varID);
}

XanHashTable * IRMETHOD_getAllVariableDefinitions (ir_method_t *method) {
    return internal_getAllVariableDefinitions(method);
}

IR_ITEM_VALUE IRMETHOD_newLabelID (ir_method_t *method) {
    return ir_instr_new_label_ID(method);
}

JITINT8 * IRMETHOD_getNameOfCalledMethod (ir_method_t *method, ir_instruction_t *callInstruction) {
    ir_method_t	*callee;
    JITINT8		*name;

    if (callInstruction == NULL) {
        return NULL;
    }

    /* Check whether this instruction calles a native method.
     */
    if (callInstruction->type == IRNATIVECALL) {
        ir_item_t	*par;
        par	= ir_instr_par2_get(callInstruction);
        return (JITINT8 *)(JITNUINT)((par->value).v);
    }

    /* Fetch the callee.
     */
    callee	= IRMETHOD_getCalledMethod(method, callInstruction);
    if (callee == NULL) {
        return NULL;
    }

    /* Fetch the name.
     */
    name	= IRMETHOD_getSignatureInString(callee);
    if (name == NULL) {
        name	= IRMETHOD_getCompleteMethodName(callee);
    }

    return name;
}

void * IRMETHOD_getCalledNativeMethod (ir_instruction_t *callInstruction) {
    ir_item_t	*par;
    ir_item_t	resolvedSymbol;

    if (callInstruction->type != IRNATIVECALL) {
        return NULL;
    }
    par	= ir_instr_par3_get(callInstruction);
    if (!IRDATA_isASymbol(par)) {
        return (void *)(JITNUINT)((par->value).v);
    }
    IRSYMBOL_resolveSymbolFromIRItem(par, &resolvedSymbol);
    return (void *)(JITNUINT)(resolvedSymbol.value.v);
}

ir_instruction_t * IRMETHOD_newCallInstruction (ir_method_t *caller, ir_method_t *callee, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t 	*inst;

    caller->modified 		= JITTRUE;

    inst 				= IRMETHOD_newInstructionOfType(caller, IRCALL);
    (inst->param_1).value.v		= IRMETHOD_getMethodID(callee);
    (inst->param_1).type		= IRMETHODID;
    (inst->param_1).internal_type	= IRMETHODID;

    if (returnItem != NULL) {
        ir_instr_result_cp(inst, returnItem);
    }
    if (callParameters != NULL) {
        XanListItem *item;
        item = xanList_first(callParameters);
        while (item != NULL) {
            IRMETHOD_addInstructionCallParameter(caller, inst, (ir_item_t *) item->data);
            item = item->next;
        }
    }

    return inst;
}

ir_instruction_t * IRMETHOD_newCallInstructionBefore (ir_method_t *caller, ir_method_t *callee, ir_instruction_t *prev, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t *inst;
    inst	= IRMETHOD_newCallInstruction(caller, callee, returnItem, callParameters);
    ir_instr_move_before(caller, inst, prev);
    return inst;
}

ir_instruction_t * IRMETHOD_newCallInstructionAfter (ir_method_t *caller, ir_method_t *callee, ir_instruction_t *prev, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t *inst;
    inst	= IRMETHOD_newCallInstruction(caller, callee, returnItem, callParameters);
    ir_instr_move_after(caller, inst, prev);
    return inst;
}

void IRMETHOD_setInstructionAsNativeCall(ir_method_t *method, ir_instruction_t *i, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {

    /* Set the method as modified.
     */
    method->modified = JITTRUE;

    /* Erase the instruction.
     */
    ir_instr_erase(method, i);

    /* Set the instruction to be a native call.
     */
    _IRMETHOD_setNativeCallInstruction(method, i, functionToCallName, functionToCallPointer, returnItem, callParameters);

    return ;
}

ir_instruction_t * IRMETHOD_newNativeCallInstruction (ir_method_t *method, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t *inst;

    /* Set the method as modified.
     */
    method->modified = JITTRUE;

    /* Create a new instruction.
     */
    inst = ir_instr_new(method);

    /* Set the instruction to be a native call.
     */
    _IRMETHOD_setNativeCallInstruction(method, inst, functionToCallName, functionToCallPointer, returnItem, callParameters);

    return inst;
}

void IRMETHOD_addNativeCallInstructionsBeforeEveryInstructionWithThemAsParameter (ir_method_t *method, XanList *instructionsToConsider, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem) {
    XanListItem *item;
    XanList *callParameters;
    ir_item_t par1;
    ir_item_t par2;

    /* Check if there are		*
     * instruction to consider	*/
    if (instructionsToConsider == NULL) {
        return ;
    }

    /* Profile			*/
    memset(&par1, 0, sizeof(ir_item_t));
    memset(&par2, 0, sizeof(ir_item_t));
    callParameters = xanList_new(allocFunction, freeFunction, NULL);
    xanList_append(callParameters, &par1);
    xanList_append(callParameters, &par2);
    par1.value.v = (IR_ITEM_VALUE)(JITNUINT) method;
    par1.type = IRNUINT;
    par1.internal_type = IRNUINT;
    par2.type = IRNUINT;
    par2.internal_type = IRNUINT;
    item = xanList_first(instructionsToConsider);
    while (item != NULL) {
        ir_instruction_t *inst;
        inst = item->data;
        par2.value.v = (IR_ITEM_VALUE)(JITNUINT) inst;
        IRMETHOD_newNativeCallInstructionBefore(method, inst, functionToCallName, functionToCallPointer, returnItem, callParameters);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(callParameters);

    /* Return			*/
    return ;
}

void IRMETHOD_addNativeCallInstructionsBeforeEveryInstruction (ir_method_t *method, XanList *instructionsToConsider, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    XanListItem *item;

    if (instructionsToConsider == NULL) {
        return ;
    }
    item = xanList_first(instructionsToConsider);
    while (item != NULL) {
        ir_instruction_t *inst;
        inst = item->data;
        IRMETHOD_newNativeCallInstructionBefore(method, inst, functionToCallName, functionToCallPointer, returnItem, callParameters);
        item = item->next;
    }
}

ir_instruction_t * IRMETHOD_newNativeCallInstructionByUsingSymbolsAfter (ir_method_t *method, ir_instruction_t *prev, ir_symbol_t *functionToCallName, ir_symbol_t *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t 	*inst;
    ir_item_t		*par3;

    /* Add the native call.
     */
    method->modified = JITTRUE;
    inst = ir_instr_new_after(method, prev);
    _IRMETHOD_setNativeCallInstruction(method, inst, NULL, NULL, returnItem, callParameters);

    /* Replace actual values with symbols.
     */
    par3		= IRMETHOD_getInstructionParameter3(inst);
    IRSYMBOL_storeSymbolToIRItem(par3, functionToCallPointer, IRMETHODENTRYPOINT, NULL);
    if (functionToCallName != NULL) {
        ir_item_t	*par2;
        par2		= IRMETHOD_getInstructionParameter2(inst);
        IRSYMBOL_storeSymbolToIRItem(par2, functionToCallName, IRSTRING, NULL);
    }

    return inst;

}

ir_instruction_t * IRMETHOD_newNativeCallInstructionByUsingSymbolsBefore (ir_method_t *method, ir_instruction_t *prev, ir_symbol_t *functionToCallName, ir_symbol_t *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t 	*inst;
    ir_item_t		*par3;

    /* Add the native call.
     */
    method->modified = JITTRUE;
    inst = ir_instr_new_before(method, prev);
    _IRMETHOD_setNativeCallInstruction(method, inst, NULL, NULL, returnItem, callParameters);

    /* Replace actual values with symbols.
     */
    par3		= IRMETHOD_getInstructionParameter3(inst);
    IRSYMBOL_storeSymbolToIRItem(par3, functionToCallPointer, IRMETHODENTRYPOINT, NULL);
    if (functionToCallName != NULL) {
        ir_item_t	*par2;
        par2		= IRMETHOD_getInstructionParameter2(inst);
        IRSYMBOL_storeSymbolToIRItem(par2, functionToCallName, IRSTRING, NULL);
    }

    return inst;
}

ir_instruction_t * IRMETHOD_newNativeCallInstructionBefore (ir_method_t *method, ir_instruction_t *prev, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t *inst;

    method->modified = JITTRUE;
    inst = ir_instr_new_before(method, prev);
    _IRMETHOD_setNativeCallInstruction(method, inst, functionToCallName, functionToCallPointer, returnItem, callParameters);
    return inst;
}

ir_instruction_t * IRMETHOD_newNativeCallInstructionAfter (ir_method_t *method, ir_instruction_t *prev, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    ir_instruction_t *inst;

    method->modified = JITTRUE;
    inst = ir_instr_new_after(method, prev);
    _IRMETHOD_setNativeCallInstruction(method, inst, functionToCallName, functionToCallPointer, returnItem, callParameters);
    return inst;
}

ir_instruction_t * IRMETHOD_newInstructionOfTypeAfter (ir_method_t *method, ir_instruction_t *prev, JITUINT16 instructionType) {
    ir_instruction_t *inst;

    method->modified = JITTRUE;
    inst = ir_instr_new_after(method, prev);
    inst->type = instructionType;
    return inst;
}

ir_instruction_t * IRMETHOD_newInstructionOfTypeBefore (ir_method_t *method, ir_instruction_t *prev, JITUINT16 instructionType) {
    ir_instruction_t *inst;

    method->modified = JITTRUE;
    inst = ir_instr_new_before(method, prev);
    inst->type = instructionType;
    return inst;
}

ir_instruction_t * IRMETHOD_newInstructionBefore (ir_method_t *method, ir_instruction_t *prev) {
    method->modified = JITTRUE;
    return ir_instr_new_before(method, prev);
}

ir_instruction_t * IRMETHOD_newInstructionAfter (ir_method_t *method, ir_instruction_t *prev) {
    method->modified = JITTRUE;
    return ir_instr_new_after(method, prev);
}

ir_instruction_t * IRMETHOD_cloneInstruction (ir_method_t *method, ir_instruction_t *instToClone) {
    method->modified = JITTRUE;
    return ir_instr_clone(method, instToClone);
}

ir_instruction_t * IRMETHOD_cloneInstructionAndInsertAfter (ir_method_t *method, ir_instruction_t *instToClone, ir_instruction_t *afterInst) {
    ir_instruction_t	*i;

    method->modified 	= JITTRUE;
    i			= ir_instr_clone(method, instToClone);
    ir_instr_move_after(method, i, afterInst);

    return i;
}

ir_instruction_t * IRMETHOD_cloneInstructionAndInsertBefore (ir_method_t *method, ir_instruction_t *instToClone, ir_instruction_t *beforeInst) {
    ir_instruction_t	*i;

    method->modified 	= JITTRUE;
    i			= ir_instr_clone(method, instToClone);
    ir_instr_move_before(method, i, beforeInst);

    return i;
}

ir_instruction_t * IRMETHOD_newInstructionOfType (ir_method_t *method, JITUINT16 instructionType) {
    ir_instruction_t *inst;

    method->modified = JITTRUE;
    inst = ir_instr_new(method);
    assert(inst != NULL);
    inst->type = instructionType;

    return inst;
}

ir_instruction_t * IRMETHOD_newInstruction (ir_method_t *method) {
    method->modified = JITTRUE;
    return ir_instr_new(method);
}

ir_signature_t * IRMETHOD_getSignature (ir_method_t * method) {
    return ir_method_signature_get(method);
}

JITBOOLEAN IRMETHOD_isAMemoryInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isAMemoryInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isAMemoryAllocationInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isAMemoryAllocationInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isAnExceptionInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isAnExceptionInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isAConversionInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isAConversionInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isABitwiseInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isABitwiseInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isAnExceptionInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRSTARTFILTER:
        case IRENDFILTER:
        case IRCALLFILTER:
        case IRTHROW:
        case IRSTARTCATCHER:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAMemoryAllocationInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRNEWOBJ:
        case IRNEWARR:
        case IRFREEOBJ:
        case IRALLOCA:
        case IRALLOC:
        case IRALLOCALIGN:
        case IRCALLOC:
        case IRREALLOC:
        case IRFREE:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAMemoryInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRLOADREL:
        case IRSTOREREL:
        case IRINITMEMORY:
        case IRMEMCPY:
        case IRMEMCMP:
        case IRARRAYLENGTH:
        case IRSTRINGLENGTH:
        case IRSTRINGCMP:
        case IRSTRINGCHR:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAConversionInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRCONV:
        case IRCONVOVF:
        case IRBITCAST:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isABitwiseInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRAND:
        case IROR:
        case IRNOT:
        case IRXOR:
        case IRSHL:
        case IRSHR:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isACompareInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRLT:
        case IRGT:
        case IREQ:
        case IRISNAN:
        case IRISINF:
        case IRCHECKNULL:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAJumpInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRBRANCH:
        case IRBRANCHIF:
        case IRBRANCHIFNOT:
        case IRRET:
        case IRBRANCHIFPCNOTINRANGE:
        case IREXIT:
        case IRCALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL:
        case IRNATIVECALL:
        case IRCALLFINALLY:
        case IRSTARTFINALLY:
        case IRENDFINALLY:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isABranchInstructionType (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRBRANCH:
        case IRBRANCHIF:
        case IRBRANCHIFNOT:
        case IRBRANCHIFPCNOTINRANGE:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAMathInstructionType (JITUINT16 instructionType) {
    if (IRMETHOD_isABitwiseInstructionType(instructionType)) {
        return JITTRUE;
    }
    switch (instructionType) {
        case IRADD:
        case IRADDOVF:
        case IRSUB:
        case IRSUBOVF:
        case IRMUL:
        case IRMULOVF:
        case IRDIV:
        case IRDIV_NOEXCEPTION:
        case IRREM:
        case IRREM_NOEXCEPTION:
        case IRNEG:
        case IRCOSH:
        case IRCEIL:
        case IRSIN:
        case IRCOS:
        case IRACOS:
        case IRSQRT:
        case IRFLOOR:
        case IRPOW:
        case IREXP:
        case IRLOG10:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAMathInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isAMathInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isACompareInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isACompareInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_mayHaveNotFallThroughInstructionAsSuccessor (ir_method_t *method, ir_instruction_t *inst) {
    return (JITBOOLEAN) ir_instr_is_a_jump_instruction(method, inst);
}

JITBOOLEAN IRMETHOD_isAJumpInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isAJumpInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_isABranchInstruction (ir_instruction_t *inst) {
    return IRMETHOD_isABranchInstructionType(inst->type);
}

JITBOOLEAN IRMETHOD_hasInstructionEscapes (ir_method_t *method, ir_instruction_t *inst) {
    return ir_instr_has_escapes(method, inst);
}

JITBOOLEAN IRMETHOD_mayInstructionAccessHeapMemory (ir_method_t *method, ir_instruction_t *inst) {
    return ir_instr_may_instruction_access_existing_heap_memory(method, inst);
}

JITINT8 * IRMETHOD_getSignatureInString (ir_method_t *method) {
    return ir_method_signature_in_string_get(method);
}

ir_instruction_t * IRMETHOD_getFinallyInstruction (ir_method_t *method, IR_ITEM_VALUE labelID) {
    return ir_instr_finally_get(method, labelID);
}

ir_instruction_t * IRMETHOD_getFilterInstruction (ir_method_t *method, IR_ITEM_VALUE labelID) {
    return ir_instr_filter_get(method, labelID);
}

ir_instruction_t * IRMETHOD_getLabelInstruction (ir_method_t *method, IR_ITEM_VALUE labelID) {
    return ir_instr_label_get(method, labelID);
}

void IRMETHOD_eraseInstructionFields (ir_method_t *method, ir_instruction_t *inst) {
    method->modified = JITTRUE;
    ir_instr_erase(method, inst);
}

JITUINT32 IRMETHOD_getInstructionPosition (ir_instruction_t *inst) {
    return ir_instr_position_get(inst);
}

MethodDescriptor * IRMETHOD_getMethodDescription (ir_method_t * method) {
    return ir_method_id_get(method);
}

JITINT8 * IRMETHOD_getMethodName (ir_method_t *method) {
    return ir_method_name_get(method);
}

JITINT8 * IRMETHOD_getNativeMethodName (ir_method_t *method) {
    JITINT8		*internalName;
    JITINT8		*nativeMethodName;

    if (!method->ID->attributes->is_internal_call) {
        return NULL;
    }

    internalName		= method->ID->getInternalName(method->ID);
    assert(internalName != NULL);

    nativeMethodName	= ir_library->getNativeMethodName(internalName);

    return nativeMethodName;
}

JITINT8 * IRMETHOD_getCompleteMethodName (ir_method_t *method) {
    return ir_method_completename_get(method);
}

JITINT8 * IRMETHOD_getClassName (TypeDescriptor *class) {
    return ir_get_class_name(class);
}

void IRMETHOD_deleteInstruction (ir_method_t *method, ir_instruction_t *inst) {
    method->modified = JITTRUE;
    ir_instr_delete(method, inst);
}

void IRMETHOD_deleteInstructions (ir_method_t *method) {
    method->modified = JITTRUE;
    ir_instr_delete_all(method);

    return ;
}

IR_ITEM_VALUE IRMETHOD_getMaxLabelID (ir_method_t *method) {
    return ir_method_label_max_get(method);
}

ir_instruction_t * IRMETHOD_getInstructionAtPosition (ir_method_t *method, JITUINT32 position) {
    return ir_instr_get_by_pos(method, position);
}

JITUINT32 IRMETHOD_getInstructionsNumber (ir_method_t *method) {
    return ir_instr_count(method);
}

JITBOOLEAN IRMETHOD_isAnEscapedVariable (ir_method_t *method, JITUINT32 varID) {
    return ir_method_escapes_isVariableEscaped(method, varID);
}

void IRMETHOD_setVariableAsEscaped (ir_method_t *method, JITUINT32 varID) {
    ir_method_escapes_setVariableAsEscaped(method, varID);
}

void IRMETHOD_allocEscapedVariablesMemory (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_escapes_alloc(method, blocksNumber);
}

void IRMETHOD_destroyMethod (ir_method_t *method) {
    ir_method_t clone;
    memcpy(&clone, method, sizeof(ir_method_t));
    if (ir_library->unregisterMethod(method)) {
        ir_method_delete(&clone);
    }
}

ir_method_t * IRMETHOD_newMethod (JITINT8 *name) {
    ir_method_t *method;

    method = ir_library->newIRMethod(name);

    return method;
}

void IRMETHOD_createMethodTrampoline (ir_method_t *method) {
    ir_library->createTrampoline(method);
}

IRBasicBlock * IRMETHOD_newBasicBlock (ir_method_t *method) {
    return ir_basicblock_new(method);
}

static inline void _IRMETHOD_deleteCircuitInvariantsInformation (circuit_t *loop) {
    if (loop->invariants == NULL) {
        return ;
    }

    freeMemory(loop->invariants);
    loop->invariants = NULL;

    return ;
}

static inline void _IRMETHOD_deleteCircuitInductionInformation (circuit_t *loop) {
    XanHashTableItem *item;

    /* Free the bitmap.
     */
    if (loop->induction_bitmap != NULL) {
        freeMemory(loop->induction_bitmap);
        loop->induction_bitmap = NULL;
    }

    /* Free the table.
     */
    if (loop->induction_table == NULL) {
        return ;
    }
    item = xanHashTable_first(loop->induction_table);
    while (item != NULL) {
        induction_variable_t *v;
        v = item->element;
        if (v->coordinated != NULL) {
            freeMemory(v->coordinated);
            v->coordinated = NULL;
        }
        freeMemory(v);
        freeMemory(item->elementID);
        item = xanHashTable_next(loop->induction_table, item);
    }
    xanHashTable_destroyTable(loop->induction_table);
    loop->induction_table = NULL;

    return ;
}

static inline void _IRMETHOD_deleteCircuitInformation (circuit_t *loop) {

    /* Destroy the loop.
     */
    if (loop->belong_inst != NULL) {
        freeMemory(loop->belong_inst);
        loop->belong_inst = NULL;
    }
    if (loop->loopExits != NULL) {
        xanList_destroyList(loop->loopExits);
        loop->loopExits = NULL;
    }
    _IRMETHOD_deleteCircuitInvariantsInformation(loop);
    _IRMETHOD_deleteCircuitInductionInformation(loop);
    freeMemory(loop);

    return;
}

void IRMETHOD_deleteCircuitsInvariantsInformation (ir_method_t *method) {
    XanListItem     *item;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if there is something to free.
     */
    if (method->loop == NULL) {
        return;
    }

    /* Free the loop induction variable structures.
     */
    item = xanList_first(method->loop);
    while (item != NULL) {
        circuit_t          *loop;

        /* Fetch the loop.
         */
        loop = item->data;
        assert(loop != NULL);

        /* Free the induction variable information.
         */
        _IRMETHOD_deleteCircuitInvariantsInformation(loop);

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    return;
}

void IRMETHOD_deleteCircuitsInductionVariablesInformation (ir_method_t *method) {
    XanListItem     *item;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if there is something to free.
     */
    if (method->loop == NULL) {
        return;
    }

    /* Free the loop induction variable structures.
     */
    item = xanList_first(method->loop);
    while (item != NULL) {
        circuit_t          *loop;

        /* Fetch the loop.
         */
        loop = item->data;
        assert(loop != NULL);

        /* Free the induction variable information.
         */
        _IRMETHOD_deleteCircuitInductionInformation(loop);

        /* Fetch the next element from the list.
         */
        item = item->next;
    }

    return;
}

void IRMETHOD_deleteDataFlowInformation (ir_method_t *method) {
    data_flow_t		*dataflow;

    /* Assertions.
     */
    assert(method != NULL);

    /* Free the memory.
     */
    ir_method_liveness_alloc(method, 0);
    ir_method_reaching_definitions_alloc(method, 0);
    ir_method_reaching_expressions_alloc(method, 0);
    ir_method_available_expressions_alloc(method, 0);
    ir_method_anticipated_expressions_alloc(method, 0);
    ir_method_postponable_expressions_alloc(method, 0);
    ir_method_used_expressions_alloc(method, 0);
    ir_method_earliest_expressions_alloc(method, 0);
    ir_method_latest_placements_alloc(method, 0);
    ir_method_partial_redundancy_elimination_alloc(method, 0);

    /* Free the method data flows.
     */
    dataflow	= IRMETHOD_getMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    if (dataflow != NULL) {
        DATAFLOW_destroySets(dataflow);
        IRMETHOD_removeMethodMetadata(method, REACHING_INSTRUCTIONS_ANALYZER);
    }
    method->valid_optimization 	= 0;

    return ;
}

void IRMETHOD_deleteCircuitsInformation (ir_method_t *method) {
    XanListItem     *item;
    circuit_t          *loop;

    /* Assertions                   */
    assert(method != NULL);

    /* Check if there is something to free.
     */
    if (	(method->loop == NULL) 		&&
            (method->loopNestTree == NULL)	) {
        return;
    }

    /* Free the loop nesting tree	  */
    if (method->loopNestTree != NULL) {
        circuit_t *l;
        l = method->loopNestTree->getData(method->loopNestTree);
        if (l->loop_id == 0) {
            _IRMETHOD_deleteCircuitInformation(l);
        }
        method->loopNestTree->destroyTree(method->loopNestTree);
        method->loopNestTree = NULL;
    }

    /* Free the loop structures     */
    if (method->loop != NULL) {
        item = xanList_first(method->loop);
        while (item != NULL) {

            /* Fetch the loop		*/
            loop = item->data;
            assert(loop != NULL);

            /* Destroy the loop		*/
            _IRMETHOD_deleteCircuitInformation(loop);

            /* Fetch the next element from	*
             * the list			*/
            item = item->next;
        }

        /* Destroy the list             */
        xanList_destroyList(method->loop);
        method->loop = NULL;
    }

    /* Set the flags		*/
    method->valid_optimization &= ~(LOOP_INVARIANTS_IDENTIFICATION | LOOP_IDENTIFICATION | INDUCTION_VARIABLES_IDENTIFICATION);

    return;
}

void IRMETHOD_deleteBasicBlocksInformation (ir_method_t *method) {

    /* Free the memory		*/
    ir_basicblock_delete_all(method);

    /* Set the flags		*/
    method->valid_optimization &= ~(BASIC_BLOCK_IDENTIFIER);

    return;
}

XanList * IRMETHOD_getCircuitInstructions (ir_method_t* method, circuit_t *loop) {
    return ir_instr_get_loop_instructions(method, loop);
}

circuit_t * IRMETHOD_getParentCircuit (ir_method_t *method, circuit_t *loop) {
    return ir_instr_get_parent_loop(method, loop);
}

XanList * IRMETHOD_getMethodCircuits (ir_method_t *method) {
    if (method->loop == NULL) {
        return xanList_new(allocFunction, freeFunction, NULL);
    }
    return xanList_cloneList(method->loop);
}

circuit_t * IRMETHOD_getTheMoreNestedCircuitOfInstruction (ir_method_t *method, ir_instruction_t *inst) {
    return ir_instr_get_loop(method, inst);
}

XanList * IRMETHOD_getNestedCircuits (ir_method_t *method, circuit_t *loop) {
    XanList         *l;
    XanNode         *node;

    /* Assertions			*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Check if there is a nesting	*
     * loops			*/
    if (method->loopNestTree == NULL) {

        /* Allocate the list		*/
        l = xanList_new(allocFunction, freeFunction, NULL);
        assert(l != NULL);

        /* Return			*/
        return l;
    }

    /* Check the node of the tree	*/
    node = method->loopNestTree->find(method->loopNestTree, loop);
    assert(node != NULL);

    /* Compute the subloops		*/
    l = node->toPreOrderList(node);
    assert(xanList_find(l, loop) != NULL);
    xanList_removeElement(l, loop, JITTRUE);

    /* Return			*/
    return l;
}

XanList * IRMETHOD_getImmediateNestedCircuits (ir_method_t *method, circuit_t *loop) {
    XanList         *l;
    XanNode         *node;
    XanNode         *child;

    /* Assertions			*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Allocate the list		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Check if there is a nesting	*
     * loops			*/
    if (method->loopNestTree == NULL) {

        /* Return			*/
        return l;
    }

    /* Check the node of the tree	*/
    node = method->loopNestTree->find(method->loopNestTree, loop);
    assert(node != NULL);
    child = node->getNextChildren(node, NULL);
    while (child != NULL) {
        circuit_t  *subloop;
        subloop = child->data;
        assert(subloop != NULL);
        xanList_append(l, subloop);
        child = node->getNextChildren(node, child);
    }

    /* Return			*/
    return l;
}

void IRMETHOD_allocateMethodExtraMemory (ir_method_t *method) {
    JITUINT32 count;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Erase the valid optimizations	*/
    method->valid_optimization = 0;

    /* Fetch the number of instructions	*/
    instructionsNumber = ir_instr_count(method);

    for (count = 0; count <= instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = ir_instr_get_by_pos(method, count);
        assert(inst != NULL);
        if (inst->metadata == NULL) {
            inst->metadata = sharedAllocFunction(sizeof(ir_instruction_metadata_t));
        }
    }
}

void IRMETHOD_destroyMethodExtraMemory (ir_method_t *method) {
    ir_method_delete_extra_memory(method);
#ifdef DEBUG
    JITUINT32 count;
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        ir_instruction_t *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));
        if (inst->metadata == NULL) {
            continue;
        }
        assert((inst->metadata->liveness).use == NULL);
        assert((inst->metadata->liveness).in == NULL);
        assert((inst->metadata->liveness).out == NULL);
    }
#endif
}

JITUINT32 IRMETHOD_getMethodParametersNumber (ir_method_t *method) {
    return ir_method_pars_count_get(method);
}

void IRMETHOD_addMethodDummyInstructions (ir_method_t *method) {
    ir_method_insert_dummy_instruction(method);
}

JITINT32 IRMETHOD_isTheVariableAMethodDeclaredVariable (ir_method_t *method, JITUINT32 varID) {
    if (ir_method_is_a_parameter(method, varID)) {
        return JITFALSE;
    }
    if (_IRMETHOD_isTheVariableATemporary(method, varID)) {
        return JITFALSE;
    }
    return JITTRUE;
}

JITINT32 IRMETHOD_isTheVariableAMethodParameter (ir_method_t *method, JITUINT32 varID) {
    return ir_method_is_a_parameter(method, varID);
}

void IRMETHOD_allocMemoryForLivenessAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_liveness_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForReachingDefinitionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_reaching_definitions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForReachingExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_reaching_expressions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForAvailableExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_available_expressions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForAnticipatedExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_anticipated_expressions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForPostponableExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_postponable_expressions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForUsedExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_used_expressions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForEarliestExpressionsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_earliest_expressions_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForLatestPlacementsAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_latest_placements_alloc(method, blocksNumber);
}

void IRMETHOD_allocMemoryForPartialRedundancyEliminationAnalysis (ir_method_t *method, JITUINT32 blocksNumber) {
    ir_method_partial_redundancy_elimination_alloc(method, blocksNumber);
}

void IRMETHOD_setMethodDescriptor (ir_method_t * method, MethodDescriptor *methodDescriptor) {
    ir_method_id_set(method, methodDescriptor);
}

JITUINT32 IRMETHOD_getNumberOfMethodBasicBlocks (ir_method_t *method) {
    return ir_basicblock_count(method);
}

IRBasicBlock * IRMETHOD_getBasicBlockContainingInstruction (ir_method_t *method, ir_instruction_t *inst) {

    /* Assertions.
     */
    assert(inst != NULL);

    return ir_basicblock_get_basic_block_from_instruction_id(method, inst->ID);
}

IRBasicBlock * IRMETHOD_getBasicBlockContainingInstructionPosition (ir_method_t *method, JITUINT32 instID) {
    return ir_basicblock_get_basic_block_from_instruction_id(method, instID);
}

void IRMETHOD_deleteInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth) {
    method->modified = JITTRUE;
    ir_instrcall_par_delete(method, inst, nth);
}

void IRMETHOD_deleteInstructionCallParameters (ir_method_t * method, ir_instruction_t *inst) {
    method->modified = JITTRUE;
    ir_instrcall_pars_delete(method, inst);
}

void IRMETHOD_setInstructionCallParameterFValue (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth, IR_ITEM_FVALUE valueToSet) {
    ir_item_t * param;

    assert(inst != NULL);

    method->modified = JITTRUE;
    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return ;
    }
    (param->value).v	= valueToSet;

    return ;
}

void IRMETHOD_setInstructionCallParameterValue (ir_method_t *method, ir_instruction_t *inst, JITUINT32 nth, IR_ITEM_VALUE valueToSet) {
    ir_item_t * param;

    assert(inst != NULL);

    method->modified = JITTRUE;
    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return ;
    }
    (param->value).v	= valueToSet;

    return ;
}

IR_ITEM_FVALUE IRMETHOD_getInstructionCallParameterFValue (ir_instruction_t *inst, JITUINT32 nth) {
    ir_item_t * param;

    assert(inst != NULL);

    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return NOPARAM;
    }
    return (param->value).f;
}

IR_ITEM_VALUE IRMETHOD_getInstructionCallParameterValue (ir_instruction_t *inst, JITUINT32 nth) {
    ir_item_t * param;

    assert(inst != NULL);

    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return NOPARAM;
    }
    return (param->value).v;
}

JITUINT32 IRMETHOD_getInstructionCallParameterType (ir_instruction_t *inst, JITUINT32 nth) {
    ir_item_t * param;

    assert(inst != NULL);

    param = ir_instrcall_par_get_by_pos(inst, nth);
    if (param == NULL) {
        return NOPARAM;
    }
    return param->type;
}

JITUINT32 IRMETHOD_getInstructionCallParametersNumber (ir_instruction_t *inst) {
    return ir_instrcall_pars_count_get(inst);
}

void IRMETHOD_moveInstructionCallParameter (ir_method_t * method, ir_instruction_t *inst, JITUINT32 oldPos, JITUINT32 newPos) {
    method->modified = JITTRUE;
    ir_instrcall_par_move(method, inst, oldPos, newPos);
}

ir_item_t * IRMETHOD_getInstructionCallParameter (ir_instruction_t *inst, JITUINT32 nth) {
    return ir_instrcall_par_get(inst, nth);
}

ir_method_t * IRMETHOD_getIRMethodFromMethodID (ir_method_t *method, IR_ITEM_VALUE irMethodID) {
    return ir_library->getIRMethod(method, irMethodID, JITFALSE);
}

void IRMETHOD_translateMethodToIR (ir_method_t *method) {
    IR_ITEM_VALUE ID;

    ID = (IR_ITEM_VALUE) (JITNUINT) ir_library->getIRMethodID(method);
    assert(ID != 0);
    ir_library->getIRMethod(NULL, ID, JITTRUE);
    method->valid_optimization = 0;

    return;
}

void IRMETHOD_updateNumberOfVariablesNeededByMethod (ir_method_t *method) {
    ir_method_update_max_variable(method);
    assert(IRMETHOD_getMethodParametersNumber(method) <= IRMETHOD_getNumberOfVariablesNeededByMethod(method));
}

IRBasicBlock * IRMETHOD_getBasicBlockAtPosition (ir_method_t *method, JITUINT32 position) {
    return ir_basicblock_get(method, position);
}

JITUINT32 IRMETHOD_getNumberOfVariablesNeededByMethod (ir_method_t *method) {
    return ir_method_var_max_get(method);
}

void IRMETHOD_setNumberOfVariablesNeededByMethod (ir_method_t *method, JITUINT32 var_max) {
    ir_method_var_max_set(method, var_max);
}

JITBOOLEAN IRMETHOD_mayThrowException (ir_instruction_t *inst) {
    return _IRMETHOD_mayThrowException(inst);
}

JITBOOLEAN IRMETHOD_canTheNextInstructionBeASuccessor (ir_method_t *method, ir_instruction_t *inst) {
    return internal_has_next_instruction_as_successor(method, inst);
}

void IRMETHOD_translateMethodToMachineCode (ir_method_t *method) {
    ir_library->translateToMachineCode(method);

    return ;
}

JITUINT16 IRMETHOD_getInstructionType (ir_instruction_t *inst) {
    return ir_instr_type_get(inst);
}

void IRMETHOD_setInstructionType (ir_method_t *method, ir_instruction_t *inst, JITUINT32 type) {
    method->modified = JITTRUE;
    ir_instr_type_set(inst, type);
}

JITUINT32 IRMETHOD_getInstructionParametersNumber (ir_instruction_t *inst) {
    return ir_instr_pars_count_get(inst);
}

void IRMETHOD_substituteVariable (ir_method_t *method, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID) {
    ir_instruction_t        *opt_inst;
    JITUINT32 instID;
    JITUINT32 instructionsNumber;

    /* Assertions                           */
    assert(method != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = ir_instr_count(method);

    /* Substitute                           */
    for (instID = 0; instID < instructionsNumber; instID++) {
        opt_inst = ir_instr_get_by_pos(method, instID);
        assert(opt_inst != NULL);
        _IRMETHOD_substituteVariableInsideInstruction(method, opt_inst, fromVarID, toVarID);
    }

    /* Return                               */
    return;
}

void IRMETHOD_substituteUsesOfVariableInsideInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID) {
    ir_item_t               *item;
    JITUINT32 count2;

    /* Assertions                           */
    assert(method != NULL);
    assert(inst != NULL);

    /* Check the parameters			*/
    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    (item->value).v = toVarID;
                }
            }
        case 2:
            item = ir_instr_par2_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    (item->value).v = toVarID;
                }
            }
        case 1:
            item = ir_instr_par1_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    (item->value).v = toVarID;
                }
            }
            break;
    }

    /* Check the call parameters		*/
    for (count2 = 0; count2 < ir_instrcall_pars_count_get(inst); count2++) {
        if (IRMETHOD_getInstructionCallParameterType(inst, count2) == IROFFSET) {
            if (IRMETHOD_getInstructionCallParameterValue(inst, count2) == fromVarID) {
                IRMETHOD_setInstructionCallParameterValue(method, inst, count2, toVarID);
            }
        }
    }
    method->modified = JITTRUE;

    /* Return                               */
    return;
}

void IRMETHOD_substituteVariableInsideInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID) {
    _IRMETHOD_substituteVariableInsideInstruction(method, inst, fromVarID, toVarID);
}

void IRMETHOD_substituteVariableInsideInstructionWithItem (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, ir_item_t *itemToUse) {
    JITUINT32 count2;
    ir_item_t *item;

    /* Assertions                           */
    assert(method != NULL);
    assert(inst != NULL);

    /* Check the parameters			*/
    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    memcpy(item, itemToUse, sizeof(ir_item_t));
                }
            }
        case 2:
            item = ir_instr_par2_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    memcpy(item, itemToUse, sizeof(ir_item_t));
                }
            }
        case 1:
            item = ir_instr_par1_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    memcpy(item, itemToUse, sizeof(ir_item_t));
                }
            }
            break;
    }

    /* Check the result			*/
    item = ir_instr_result_get(inst);
    assert(item != NULL);
    if (item->type == IROFFSET) {
        if ((item->value).v == fromVarID) {
            memcpy(item, itemToUse, sizeof(ir_item_t));
        }
    }

    /* Check the call parameters		*/
    for (count2 = 0; count2 < ir_instrcall_pars_count_get(inst); count2++) {
        item = ir_instrcall_par_get_by_pos(inst, count2);
        if (item->type != IROFFSET) {
            continue;
        }
        if ((item->value).v != fromVarID) {
            continue;
        }
        memcpy(item, itemToUse, sizeof(ir_item_t));
    }
    method->modified = JITTRUE;

    /* Return                               */
    return;
}

static inline void _IRMETHOD_substituteVariableInsideInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE fromVarID, IR_ITEM_VALUE toVarID) {
    ir_item_t               *item;
    JITUINT32 count2;

    /* Assertions                           */
    assert(method != NULL);
    assert(inst != NULL);

    /* Check the parameters			*/
    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    (item->value).v = toVarID;
                }
            }
        case 2:
            item = ir_instr_par2_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    (item->value).v = toVarID;
                }
            }
        case 1:
            item = ir_instr_par1_get(inst);
            assert(item != NULL);
            if (item->type == IROFFSET) {
                if ((item->value).v == fromVarID) {
                    (item->value).v = toVarID;
                }
            }
            break;
    }

    /* Check the result			*/
    item = ir_instr_result_get(inst);
    assert(item != NULL);
    if (item->type == IROFFSET) {
        if ((item->value).v == fromVarID) {
            (item->value).v = toVarID;
        }
    }

    /* Check the call parameters		*/
    for (count2 = 0; count2 < ir_instrcall_pars_count_get(inst); count2++) {
        if (IRMETHOD_getInstructionCallParameterType(inst, count2) == IROFFSET) {
            if (IRMETHOD_getInstructionCallParameterValue(inst, count2) == fromVarID) {
                IRMETHOD_setInstructionCallParameterValue(method, inst, count2, toVarID);
            }
        }
    }
    method->modified = JITTRUE;

    /* Return                               */
    return;
}

void IRMETHOD_substituteLabelInAllBranches (ir_method_t *method, IR_ITEM_VALUE old_label, IR_ITEM_VALUE new_label) {
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions						*/
    assert(method != NULL);

    /* Substitute the parameters				*/
    method->modified = JITTRUE;

    /* Fetch the number of instructions of the method	*/
    instructionsNumber = ir_instr_count(method);

    /* Substitute the label					*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (inst->type == IRLABEL) {
            continue;
        }
        IRMETHOD_substituteLabel(method, inst, old_label, new_label);
    }

    /* Return			*/
    return;
}

void IRMETHOD_substituteLabel (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE old_label, IR_ITEM_VALUE new_label) {
    ir_item_t       *item;
    JITUINT32 count;

    /* Assertions			*/
    assert(inst != NULL);

    /* Substitute the parameters	*/
    method->modified = JITTRUE;
    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            if (    (item->type == IRLABELITEM)     &&
                    ((item->value).v == old_label)      ) {
                (item->value).v = new_label;
            }
        case 2:
            item = ir_instr_par2_get(inst);
            if (    (item->type == IRLABELITEM)     &&
                    ((item->value).v == old_label)      ) {
                (item->value).v = new_label;
            }
        case 1:
            item = ir_instr_par1_get(inst);
            if (    (item->type == IRLABELITEM)     &&
                    ((item->value).v == old_label)      ) {
                (item->value).v = new_label;
            }
            break;
    }

    /* Substitute the call          *
     * parameters			*/
    for (count = 0; count < ir_instrcall_pars_count_get(inst); count++) {
        item = ir_instrcall_par_get(inst, count);
        assert(item != NULL);
        if (    (item->type == IRLABELITEM)     &&
                ((item->value).v == old_label)      ) {
            (item->value).v = new_label;
        }
    }

    /* Return			*/
    return;
}

JITBOOLEAN IRMETHOD_hasAPointerType (ir_item_t *item) {
    JITUINT32	type;
    if (item->type == IRTYPE) {
        type	= item->value.v;
    } else {
        type	= item->internal_type;
    }
    return IRMETHOD_isAPointerType(type);
}

JITBOOLEAN IRMETHOD_isAPointerType (JITUINT32 type) {
    switch (type) {
        case IRCLASSID:
        case IRMETHODID:
        case IRMETHODENTRYPOINT:
        case IRMPOINTER:
        case IRFPOINTER:
        case IROBJECT:
        case IRSTRING:
        case IRSIGNATURE:
            return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAPlatformDependentType (JITUINT32 type) {
    switch (type) {
        case IRNINT:
        case IRNUINT:
        case IRNFLOAT:
            return JITTRUE;
    }
    if (IRMETHOD_isAPointerType(type)) {
        return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isADataValueType (JITUINT32 type) {

    switch (type) {
        case IRVALUETYPE:
            return JITTRUE;
    }
    return JITFALSE;
}

JITBOOLEAN IRMETHOD_isAnIntType (JITUINT32 type) {
    switch (type) {
        case IRINT8:
        case IRINT16:
        case IRINT32:
        case IRINT64:
        case IRNINT:
        case IRUINT8:
        case IRUINT16:
        case IRUINT32:
        case IRUINT64:
        case IRNUINT:
            return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRMETHOD_hasADataValueType (ir_item_t *item) {
    JITUINT32	type;
    if (item->type == IRTYPE) {
        type	= item->value.v;
    } else {
        type	= item->internal_type;
    }
    return IRMETHOD_isADataValueType(type);
}

JITBOOLEAN IRMETHOD_hasAnIntType (ir_item_t *item) {
    JITUINT32	type;
    if (item->type == IRTYPE) {
        type	= item->value.v;
    } else {
        type	= item->internal_type;
    }
    return IRMETHOD_isAnIntType(type);
}

JITBOOLEAN IRMETHOD_isAFloatType (JITUINT32 type) {
    return _IRMETHOD_isAFloatType(type);
}

JITBOOLEAN IRMETHOD_hasAFloatType (ir_item_t *item) {
    JITUINT32	type;
    if (item->type == IRTYPE) {
        type	= item->value.v;
    } else {
        type	= item->internal_type;
    }
    return _IRMETHOD_isAFloatType(type);
}

JITBOOLEAN IRMETHOD_isAnInductionVariable (ir_method_t *method, IR_ITEM_VALUE varID, circuit_t *loop) {
    XanList                 *inductionVariables;
    XanListItem             *item;
    JITINT32 found;
    induction_variable_t    *inductionVariable;

    /* Assertions				*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Fetch the inductive list		*/
    inductionVariables = _IRMETHOD_getCircuitInductionVariables(method, loop);
    assert(inductionVariables != NULL);

    /* Look for the variable		*/
    found = JITFALSE;
    item = xanList_first(inductionVariables);
    while (item != NULL) {
        inductionVariable = (induction_variable_t *) item->data;
        if (inductionVariable->ID == varID) {
            found = JITTRUE;
            break;
        }
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(inductionVariables);

    /* Return				*/
    return found;
}

XanList * IRMETHOD_getCircuitInductionVariables (ir_method_t *method, circuit_t *loop) {
    return _IRMETHOD_getCircuitInductionVariables(method, loop);
}

void IRMETHOD_setInstructionToANewLabel (ir_method_t *method, ir_instruction_t *inst) {

    /* Erase the instruction		*/
    ir_instr_erase(method, inst);

    /* Set up the new instruction		*/
    method->modified = JITTRUE;
    inst->type = IRLABEL;
    (inst->param_1).type = IRLABELITEM;
    (inst->param_1).internal_type = IRLABELITEM;
    (inst->param_1).value.v = ir_instr_new_label_ID(method);
}

ir_instruction_t * IRMETHOD_newLabelBefore (ir_method_t *method, ir_instruction_t *beforeInstruction) {
    ir_instruction_t *inst;

    inst = IRMETHOD_newLabel(method);
    ir_instr_move_before(method, inst, beforeInstruction);
    return inst;
}

ir_instruction_t * IRMETHOD_newLabelAfter (ir_method_t *method, ir_instruction_t *afterInstruction) {
    ir_instruction_t *inst;

    inst = IRMETHOD_newLabel(method);
    ir_instr_move_after(method, inst, afterInstruction);
    return inst;
}

ir_instruction_t * IRMETHOD_newLabel (ir_method_t *method) {
    ir_instruction_t        *inst;

    /* Assertions				*/
    assert(method != NULL);

    /* Allocate a new instruction		*/
    method->modified = JITTRUE;
    inst = ir_instr_new(method);
    assert(inst != NULL);

    /* Set up the new instruction		*/
    inst->type = IRLABEL;
    (inst->param_1).type = IRLABELITEM;
    (inst->param_1).internal_type = IRLABELITEM;
    (inst->param_1).value.v = ir_instr_new_label_ID(method);

    /* Return				*/
    return inst;
}

ir_instruction_t * IRMETHOD_newBranchToLabelBefore (ir_method_t *method, ir_instruction_t *labelInstruction, ir_instruction_t *beforeInstruction) {
    ir_instruction_t *inst;

    inst = IRMETHOD_newBranchToLabel(method, labelInstruction);
    ir_instr_move_before(method, inst, beforeInstruction);
    return inst;
}

ir_instruction_t * IRMETHOD_newBranchToLabelAfter (ir_method_t *method, ir_instruction_t *labelInstruction, ir_instruction_t *afterInstruction) {
    ir_instruction_t *inst;

    inst = IRMETHOD_newBranchToLabel(method, labelInstruction);
    ir_instr_move_after(method, inst, afterInstruction);
    return inst;
}

ir_instruction_t * IRMETHOD_newBranchToLabel (ir_method_t *method, ir_instruction_t *labelInstruction) {
    ir_instruction_t        *inst;

    /* Assertions				*/
    assert(method != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, labelInstruction));

    /* Check the label			*/
    if (labelInstruction->type != IRLABEL) {
        fprintf(stderr, "ILDJIT: IRMETHOD_newBranchToLabel: The instruction %u given as input is not a label; it is a %s instruction.\n", labelInstruction->ID, IRMETHOD_getInstructionTypeName(labelInstruction->type));
        abort();
    }

    /* Allocate a new instruction		*/
    inst = ir_instr_new(method);
    assert(inst != NULL);

    /* Set up the new instruction		*/
    inst->type = IRBRANCH;
    memcpy(&(inst->param_1), &(labelInstruction->param_1), sizeof(ir_item_t));

    /* Return				*/
    return inst;
}

JITBOOLEAN IRMETHOD_hasIRMethodBody (ir_method_t *method) {
    JITBOOLEAN hasBody;

    /* Assertions				*/
    assert(method != NULL);

    /* Check if the method can have a body	*/
    hasBody = !(method->ID->attributes->is_abstract || method->ID->attributes->is_internal_call);

    /* Return				*/
    return hasBody;
}

ir_local_t * IRMETHOD_getLocalVariableOfMethod (ir_method_t * method, JITUINT32 var_number) {
    return ir_get_local(method, var_number);
}

JITUINT32 IRMETHOD_getMethodLocalsNumber (ir_method_t * method) {
    return ir_method_var_local_count_get(method);
}

ir_local_t * IRMETHOD_insertLocalVariableToMethod (ir_method_t * method, JITUINT32 internal_type, TypeDescriptor * bytecodeType) {
    return ir_method_var_local_insert(method, internal_type, bytecodeType);
}

JITBOOLEAN IRMETHOD_isTheVariableATemporary (ir_method_t *method, JITUINT32 varID) {
    return _IRMETHOD_isTheVariableATemporary(method, varID);
}


XanHashTable * IRMETHOD_getInstructionPositions (ir_method_t *method) {
    XanHashTable	*t;
    JITUINT32 	instructionsNumber;
    JITUINT32 	instID;

    /* Allocate the necessary memory.
     */
    t		= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(t != NULL);

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber = ir_instr_count(method);

    /* Perform the mapping.
     */
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *inst;
        inst = ir_instr_get_by_pos(method, instID);
        xanHashTable_insert(t, inst, (void *)(JITNUINT)(instID + 1));
    }

    return t;
}

ir_instruction_t * IRMETHOD_getFirstInstructionOfType (ir_method_t *method, JITUINT16 type) {
    ir_instruction_t *i;

    i = ir_instr_get_first(method);
    while ((i != NULL) && (i->type != type)) {
        i = ir_instr_get_next(method, i);
    }
    return i;
}

ir_instruction_t * IRMETHOD_getFirstInstructionNotOfType (ir_method_t *method, JITUINT16 type) {
    ir_instruction_t *i;

    i = ir_instr_get_first(method);
    while ((i != NULL) && (i->type == type)) {
        i = ir_instr_get_next(method, i);
    }
    return i;
}

ir_instruction_t * IRMETHOD_getFirstInstruction (ir_method_t *method) {
    return ir_instr_get_first(method);
}

ir_instruction_t * IRMETHOD_getLastInstruction (ir_method_t *method) {
    return ir_instr_get_last(method);
}

void * IRMETHOD_getFunctionPointer (ir_method_t * method) {
    return ir_library->getFunctionPointer(method);
}

JITBOOLEAN IRMETHOD_doesMethodHaveItsEntryPointDirectlyReferenced (ir_method_t * method) {

    /* Assertions.
     */
    assert(method != NULL);

    return method->isDirectlyReferenced;
}

void IRMETHOD_setMethodToHaveItsEntryPointDirectlyReferenced (ir_method_t * method, JITBOOLEAN isReferenced) {

    /* Assertions.
     */
    assert(method != NULL);

    method->isDirectlyReferenced	= isReferenced;

    return ;
}

JITBOOLEAN IRMETHOD_canInstructionBeTheLastOneExecutedInTheMethod (ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t	*s;

    switch (inst->type) {
        case IRRET:
        case IREXIT:
            return JITTRUE;
    }

    s	= IRMETHOD_getSuccessorInstruction(method, inst, NULL);
    while (s != NULL) {
        if (s == method->exitNode) {
            return JITTRUE;
        }
        s	= IRMETHOD_getSuccessorInstruction(method, inst, s);
    }

    return JITFALSE;
}

XanList * IRMETHOD_getInstructionSuccessors (ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t	*catcherInst;
    JITBOOLEAN		catcherInstInitialized;

    catcherInst		= NULL;
    catcherInstInitialized	= JITFALSE;

    return internal_getInstructionSuccessors(method, inst, &catcherInst, &catcherInstInitialized);
}

XanList * IRMETHOD_getCallersOfFinallyBlock (ir_method_t *method, IR_ITEM_VALUE finallyLabelID) {
    ir_instruction_t	*irFinallyInst;

    irFinallyInst			= IRMETHOD_getFinallyInstruction(method, finallyLabelID);

    return IRMETHOD_getInstructionPredecessors(method, irFinallyInst);
}

XanList * IRMETHOD_getInstructionPredecessors (ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t	*catcherInst;
    JITBOOLEAN		catcherInstInitialized;

    catcherInst	= NULL;
    catcherInstInitialized	=	JITFALSE;

    return internal_getInstructionPredecessors(method, inst, &catcherInst, &catcherInstInitialized);
}

void IRMETHOD_substituteLabels (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE new_label) {
    ir_item_t       *item;
    JITUINT32 count;

    /* Assertions			*/
    assert(inst != NULL);

    /* Substitute the parameters	*/
    method->modified = JITTRUE;
    switch (ir_instr_pars_count_get(inst)) {
        case 3:
            item = ir_instr_par3_get(inst);
            if (item->type == IRLABELITEM) {
                (item->value).v = new_label;
            }
        case 2:
            item = ir_instr_par2_get(inst);
            if (item->type == IRLABELITEM) {
                (item->value).v = new_label;
            }
        case 1:
            item = ir_instr_par1_get(inst);
            if (item->type == IRLABELITEM) {
                (item->value).v = new_label;
            }
            break;
    }

    /* Substitute the call          *
     * parameters			*/
    for (count = 0; count < ir_instrcall_pars_count_get(inst); count++) {
        item = ir_instrcall_par_get(inst, count);
        assert(item != NULL);
        if (item->type == IRLABELITEM) {
            (item->value).v = new_label;
        }
    }

    /* Return			*/
    return;
}

ir_instruction_t * IRMETHOD_getBranchDestination (ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t        *label;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize the variables	*/
    label = NULL;

    /* Get the label		*/
    switch (inst->type) {
        case IRBRANCH:
            label = ir_instr_label_get(method, (inst->param_1).value.v);
            break;
        case IRBRANCHIF:
        case IRBRANCHIFNOT:
            label = ir_instr_label_get(method, (inst->param_2).value.v);
            break;
        case IRBRANCHIFPCNOTINRANGE:
            label = ir_instr_label_get(method, (inst->param_3).value.v);
            break;
    }

    /* Return			*/
    return label;
}

ir_instruction_t * IRMETHOD_getBranchFallThrough (ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t        *fallThrough;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Initialize the variables	*/
    fallThrough = NULL;

    /* Get the label		*/
    switch (inst->type) {
        case IRBRANCHIF:
        case IRBRANCHIFNOT:
        case IRBRANCHIFPCNOTINRANGE:
            fallThrough = ir_instr_get_next(method, inst);
            break;
    }

    /* Return			*/
    return fallThrough;
}

void IRMETHOD_setBranchDestination (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE labelID) {

    switch (inst->type) {
        case IRBRANCH:
            (inst->param_1).value.v = labelID;
            break;
        case IRBRANCHIF:
        case IRBRANCHIFNOT:
            (inst->param_2).value.v = labelID;
            break;
    }
}

static inline JITBOOLEAN internal_addVar (ir_item_t *par, ir_item_t **vars) {
    if (par->type != IROFFSET) {
        return JITFALSE;
    }
    if (	(vars[(par->value).v] != NULL)			&&
            (vars[(par->value).v]->type_infos != NULL)	) {
        return JITFALSE;
    }
    vars[(par->value).v]	= par;
    return (par->type_infos != NULL);
}

ir_item_t ** IRMETHOD_getIRVariables (ir_method_t *method) {
    ir_item_t	**vars;
    IR_ITEM_VALUE	highestVar;
    JITUINT32	instID;
    JITUINT32	instructionsNumber;
    JITUINT32	nums;

    /* Initialize the variables.
     */
    nums			= 0;
    highestVar		= method->var_max;
    if (highestVar == 0) {
        highestVar	= 1;
    }

    /* Allocate the necessary memory.
     */
    vars			= allocFunction(sizeof(ir_item_t *) * highestVar);

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber	= ir_instr_count(method);

    /* Find the variables.
     */
    for (instID = 0; instID < instructionsNumber && nums < highestVar; instID++) {
        ir_instruction_t	*inst;
        inst 	= IRMETHOD_getInstructionAtPosition(method, instID);
        nums	+= internal_addVar(&(inst->param_1), vars);
        nums	+= internal_addVar(&(inst->param_2), vars);
        nums	+= internal_addVar(&(inst->param_3), vars);
        nums	+= internal_addVar(&(inst->result), vars);
        if (inst->callParameters != NULL) {
            XanListItem	*item;
            item	= xanList_first(inst->callParameters);
            while (item != NULL) {
                ir_item_t       *par;
                par	= item->data;
                nums	+= internal_addVar(par, vars);
                item	= item->next;
            }
        }
    }

    return vars;
}

ir_item_t * IRMETHOD_getIRVariable (ir_method_t *method, JITUINT32 varID) {
    ir_instruction_t        *inst;
    JITUINT32 instID;
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    ir_item_t *var;

    /* Assertions						*/
    assert(method != NULL);

    /* Initialize the variables				*/
    var = NULL;

    /* Fetch the number of instructions of the method	*/
    instructionsNumber = ir_instr_count(method);

    /* Search the IR variable				*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);
        if (    ((inst->param_1).type == IROFFSET)      &&
                ((inst->param_1).value.v == varID)        ) {
            var = IRMETHOD_getInstructionParameter1(inst);
            assert((var->internal_type != IRVALUETYPE) || (var->type_infos != NULL));
            if (	((inst->type) != IRALLOCA)					&&
                    (var->internal_type != IRVALUETYPE || var->type_infos != NULL)	) {
                return var;
            }
        }
        if (    ((inst->param_2).type == IROFFSET)      &&
                ((inst->param_2).value.v == varID)        ) {
            var = IRMETHOD_getInstructionParameter2(inst);
            assert((var->internal_type != IRVALUETYPE) || (var->type_infos != NULL));
            if (	((inst->type) != IRALLOCA)					&&
                    (var->internal_type != IRVALUETYPE || var->type_infos != NULL)	) {
                return var;
            }
        }
        if (    ((inst->param_3).type == IROFFSET)      &&
                ((inst->param_3).value.v == varID)        ) {
            var = IRMETHOD_getInstructionParameter3(inst);
            assert((var->internal_type != IRVALUETYPE) || (var->type_infos != NULL));
            if (	((inst->type) != IRALLOCA)					&&
                    (var->internal_type != IRVALUETYPE || var->type_infos != NULL)	) {
                return var;
            }
        }
        if (    ((inst->result).type == IROFFSET)       &&
                ((inst->result).value.v == varID)         ) {
            var = IRMETHOD_getInstructionResult(inst);
            assert((var->internal_type != IRVALUETYPE) || (var->type_infos != NULL));
            if (	((inst->type) != IRALLOCA)					&&
                    (var->internal_type != IRVALUETYPE || var->type_infos != NULL)	) {
                return var;
            }
        }
        for (count = 0; count < ir_instrcall_pars_count_get(inst); count++) {
            ir_item_t       *par;
            par = ir_instrcall_par_get(inst, count);
            assert(par != NULL);
            assert((par->internal_type != IRVALUETYPE) || (par->type_infos != NULL));
            if (    (par->type == IROFFSET) &&
                    ((par->value).v == varID)   ) {
                var = par;
                if (	((inst->type) != IRALLOCA)					&&
                        (var->internal_type != IRVALUETYPE || var->type_infos != NULL)	) {
                    return var;
                }
            }
        }
    }

    /* Return			*/
    return var;
}

JITUINT16 IRMETHOD_getDeclaredVariableType (ir_method_t * method, IR_ITEM_VALUE varID) {
    ir_local_t *local;

    local = ir_get_local(method, varID - ir_method_pars_count_get(method));
    if (local == NULL) {
        return NOPARAM;
    }
    return local->internal_type;
}

TypeDescriptor * IRMETHOD_getDeclaredVariableILType (ir_method_t * method, IR_ITEM_VALUE varID) {
    ir_local_t *local;

    local = ir_get_local(method, varID - ir_method_pars_count_get(method));
    if (local == NULL) {
        return NULL;
    }
    return local->type_info;
}

ParamDescriptor * IRMETHOD_getParameterILDescriptor (ir_method_t * method, JITUINT32 parameterNumber) {
    if ((method->signature).ilParamsDescriptor == NULL) {
        return NULL;
    }
    return (method->signature).ilParamsDescriptor[parameterNumber];
}

TypeDescriptor * IRMETHOD_getParameterILType (ir_method_t * method, JITUINT32 parameterNumber) {
    return ir_get_parameter_il_type(method, parameterNumber);
}

JITUINT32 IRMETHOD_getParameterType (ir_method_t * method, JITUINT32 position) {
    return *ir_method_pars_type_get(method, position);
}

void IRMETHOD_setResultType (ir_method_t *method, JITUINT32 value) {
    ir_method_result_type_set(method, value);
}

void IRMETHOD_setParameterType (ir_method_t * method, JITUINT32 position, JITUINT32 value) {
    ir_method_pars_type_set(method, position, value);
}

void * get_metadata (IrMetadata *metadata, JITUINT64 type) {
    void *result = NULL;

    while (metadata != NULL) {
        if (metadata->type == type) {
            result = metadata->data;
            break;
        }
        metadata = metadata->next;
    }

    return result;
}

void * add_metadata (IrMetadata *metadata, JITUINT64 type, void *data) {
    IrMetadata *element = metadata;

    while (element != NULL) {
        if (element->type == type) {
            element->data = data;
            return metadata;
        }
        element = element->next;
    }

    element = sharedAllocFunction(sizeof(IrMetadata));
    element->data = data;
    element->type = type;
    element->next = metadata;

    return element;
}

void * IRMETHOD_getInstructionMetadata (ir_instruction_t *inst, JITUINT64 type) {

    /* Assertions.
     */
    assert(inst != NULL);

    if (inst->metadata == NULL) {
        return NULL;
    }
    return get_metadata(inst->metadata->metadata, type);
}

void * IRMETHOD_getMethodMetadata (ir_method_t *method, JITUINT64 type) {
    return get_metadata(method->metadata, type);
}

void IRMETHOD_setInstructionMetadata (ir_instruction_t *inst, JITUINT64 type, void *data) {
    inst->metadata->metadata = add_metadata(inst->metadata->metadata, type, data);
}

void IRMETHOD_removeMethodMetadata (ir_method_t *method, JITUINT64 type) {
    IrMetadata 	*element;
    IrMetadata 	*prevElement;

    /* Fetch the first element.
     */
    prevElement	= NULL;
    element		= method->metadata;

    /* Remove the metadata.
     */
    while (element != NULL) {
        IrMetadata 	*nextElement;
        nextElement	= element->next;

        if (element->type == type) {
            if (prevElement == NULL) {
                method->metadata	= nextElement;
                freeFunction(element);
                element			= NULL;
            } else {
                prevElement->next	= nextElement;
                freeFunction(element);
                element			= prevElement;
            }
        }

        prevElement	= element;
        element 	= nextElement;
    }

    return ;
}

void IRMETHOD_setMethodMetadata (ir_method_t *method, JITUINT64 type, void *data) {
    method->metadata = add_metadata(method->metadata, type, data);
}

JITBOOLEAN IRMETHOD_hasCatchBlocks (ir_method_t *method) {
    return ir_instrcatcher_get(method) != NULL;
}

ir_instruction_t * IRMETHOD_getNextInstruction (ir_method_t *method, ir_instruction_t *prev) {
    return ir_instr_get_next(method, prev);
}

ir_instruction_t * IRMETHOD_getPrevInstruction (ir_method_t *method, ir_instruction_t *inst) {
    return ir_instr_get_prev(method, inst);
}

ir_instruction_t * IRMETHOD_getNextInstructionNotOfType (ir_method_t *method, ir_instruction_t *prev, JITUINT16 type) {
    ir_instruction_t	*i;
    i	= ir_instr_get_next(method, prev);
    while ((i != NULL) && (i->type == type)) {
        i	= ir_instr_get_next(method, i);
    }
    return i;
}

void IRMETHOD_cpInstructionParameter1 (ir_instruction_t *inst, ir_item_t * from) {
    ir_instr_par1_cp(inst, from);
}

void IRMETHOD_cpInstructionParameter2 (ir_instruction_t *inst, ir_item_t * from) {
    ir_instr_par2_cp(inst, from);
}

void IRMETHOD_cpInstructionParameter3 (ir_instruction_t *inst, ir_item_t * from) {
    ir_instr_par3_cp(inst, from);
}

void IRMETHOD_cpInstructionResult (ir_instruction_t *inst, ir_item_t * from) {
    ir_instr_result_cp(inst, from);
}

ir_item_t * IRMETHOD_getInstructionParameter (ir_instruction_t * inst, JITUINT32 parNum) {
    switch (parNum) {
        case 1:
            return IRMETHOD_getInstructionParameter1(inst);
        case 2:
            return IRMETHOD_getInstructionParameter2(inst);
        case 3:
            return IRMETHOD_getInstructionParameter3(inst);
        case 0:
            return IRMETHOD_getInstructionResult(inst);
    }
    return NULL;
}

ir_item_t * IRMETHOD_getInstructionParameter1 (ir_instruction_t * inst) {
    return ir_instr_par1_get(inst);
}

ir_item_t * IRMETHOD_getInstructionParameter2 (ir_instruction_t * inst) {
    return ir_instr_par2_get(inst);
}

ir_item_t * IRMETHOD_getInstructionParameter3 (ir_instruction_t * inst) {
    return ir_instr_par3_get(inst);
}

ir_item_t * IRMETHOD_getInstructionResult (ir_instruction_t * inst) {
    return ir_instr_result_get(inst);
}

IRBasicBlock * IRMETHOD_getPrevBasicBlock (ir_method_t *method, IRBasicBlock *bb) {
    if (bb->pos == 0) {
        return NULL;
    }
    return ir_basicblock_get(method, bb->pos - 1);
}

IRBasicBlock * IRMETHOD_getNextBasicBlock (ir_method_t *method, IRBasicBlock *bb) {
    return ir_basicblock_get(method, bb->pos + 1);
}

IRBasicBlock * IRMETHOD_getFirstBasicBlock (ir_method_t *method) {
    return ir_basicblock_get(method, 0);
}

JITUINT32 IRMETHOD_getBasicBlockLength (IRBasicBlock *bb) {
    return (bb->endInst - bb->startInst) + 1;
}

ir_instruction_t * IRMETHOD_getLastInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb) {
    return ir_instr_get_by_pos(method, bb->endInst);
}

ir_instruction_t * IRMETHOD_getFirstInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb) {
    return ir_instr_get_by_pos(method, bb->startInst);
}

ir_instruction_t * IRMETHOD_getPrevInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb, ir_instruction_t *inst) {
    if (inst == NULL) {
        return ir_instr_get_by_pos(method, bb->endInst);
    }
    if (inst->ID > bb->startInst) {
        return ir_instr_get_by_pos(method, inst->ID - 1);
    }
    return NULL;
}

ir_instruction_t * IRMETHOD_getNextInstructionWithinBasicBlock (ir_method_t *method, IRBasicBlock *bb, ir_instruction_t *inst) {
    if (inst == NULL) {
        return ir_instr_get_by_pos(method, bb->startInst);
    }
    if (inst->ID < bb->endInst) {
        return ir_instr_get_by_pos(method, inst->ID + 1);
    }
    return NULL;
}

IR_ITEM_VALUE IRMETHOD_getInstructionParameter1Value (ir_instruction_t *inst) {
    return ir_instr_par1_value_get(inst);
}

IR_ITEM_VALUE IRMETHOD_getInstructionParameter2Value (ir_instruction_t *inst) {
    return ir_instr_par2_value_get(inst);
}

IR_ITEM_VALUE IRMETHOD_getInstructionParameter3Value (ir_instruction_t *inst) {
    return ir_instr_par3_value_get(inst);
}

IR_ITEM_FVALUE IRMETHOD_getInstructionParameter1FValue (ir_instruction_t *inst) {
    return ir_instr_par1_fvalue_get(inst);
}

IR_ITEM_FVALUE IRMETHOD_getInstructionParameter2FValue (ir_instruction_t *inst) {
    return ir_instr_par2_fvalue_get(inst);
}

IR_ITEM_FVALUE IRMETHOD_getInstructionParameter3FValue (ir_instruction_t *inst) {
    return ir_instr_par3_fvalue_get(inst);
}

JITUINT32 IRMETHOD_getInstructionParameter1Type (ir_instruction_t *inst) {
    return ir_instr_par1_type_get(inst);
}

JITUINT32 IRMETHOD_getInstructionParameter2Type (ir_instruction_t *inst) {
    return ir_instr_par2_type_get(inst);
}

JITUINT32 IRMETHOD_getInstructionParameter3Type (ir_instruction_t *inst) {
    return ir_instr_par3_type_get(inst);
}

void IRMETHOD_executeMethod (ir_method_t * method, void **args, void *returnArea) {
    ir_library->run(method, args, returnArea);
}

TypeDescriptor * IRMETHOD_getResultILType (ir_method_t * method) {
    return ir_get_result_il_type(method);
}

JITUINT32 IRMETHOD_getResultType (ir_method_t *method) {
    return ir_method_result_type_get(method);
}

ir_method_t * IRMETHOD_cloneMethod (ir_method_t *method, JITINT8 *name) {
    ir_method_t *newMethod;
    XanListItem *item;
    JITUINT32 instructionsCount;
    JITUINT32 parametersNumber;
    JITUINT32 count;
    JITBOOLEAN allocateMetadata;
    ir_instruction_t *firstInst;

    /* Assertions				*/
    assert(method != NULL);
    assert(name != NULL);
    assert(method->name != name);

    /* Check if we need to allocate	the	*
     * method extra memory			*/
    firstInst = ir_instr_get_first(method);
    assert(firstInst != NULL);
    if (firstInst->metadata != NULL) {
        allocateMetadata = JITTRUE;
    } else {
        allocateMetadata = JITFALSE;
    }

    /* Create new Method                    */
    newMethod = IRMETHOD_newMethod(name);
    assert(newMethod != method);
    assert(newMethod->name == name);
    assert(newMethod->name != method->name);

    /* Remove all IRNOP instructions created by default.
     */
    ir_instr_delete_all(newMethod);

    /* Fetch the number of parameters of the method.
     */
    parametersNumber = (method->signature).parametersNumber;

    /* Clone Method Signature               */
    IRMETHOD_initSignature(&(newMethod->signature), parametersNumber);
    assert((newMethod->signature).parametersNumber == (method->signature).parametersNumber);
    for (count = 0; count < parametersNumber; count++) {
        (newMethod->signature).parameterTypes[count] = (method->signature).parameterTypes[count];
        (newMethod->signature).ilParamsTypes[count] = (method->signature).ilParamsTypes[count];
        (newMethod->signature).ilParamsDescriptor[count] = (method->signature).ilParamsDescriptor[count];
    }
    (newMethod->signature).resultType = (method->signature).resultType;
    (newMethod->signature).ilResultType = (method->signature).ilResultType;

    /* Clone Method Variables               */
    newMethod->var_max = method->var_max;

    /* Clone method locals			*/
    item = xanList_first(method->locals);
    while (item != NULL) {
        ir_local_t *local = (ir_local_t *) item->data;
        ir_local_t *newLocal = sharedAllocFunction(sizeof(ir_local_t));
        memcpy(newLocal, local, sizeof(ir_local_t));
        xanList_append(newMethod->locals, newLocal);
        item = item->next;
    }

    /* Fetch the number of instructions	*
     * of the method			*/
    instructionsCount = ir_instr_count(method);

    /* Check the method to clone		*/
#ifdef DEBUG
    for (count = 0; count < instructionsCount; count++) {
        ir_instruction_t *instruction;
        instruction = ir_instr_get_by_pos(method, count);
        assert(instruction->ID == count);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, instruction));
    }
#endif

    /* Clone Method Instructions            */
    for (count = 0; count < instructionsCount; count++) {
        ir_instruction_t *instruction;

        /* Fetch the instruction to clone	*/
        instruction = ir_instr_get_by_pos(method, count);
        assert(instruction->ID == count);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, instruction));

        /* Create a new instruction of the      *
         * cloned method			*/
        ir_instr_clone(newMethod, instruction);
    }

    /* Check the metadata			*/
    if (allocateMetadata) {
        IRMETHOD_allocateMethodExtraMemory(newMethod);
    }
    newMethod->modified = JITTRUE;
    newMethod->valid_optimization = 0;

    /* Update the number of variables need by the cloned method.
     */
    ir_method_update_max_variable(newMethod);

    /* Return				*/
    return newMethod;
}

void IRMETHOD_swapInstructionParameters (ir_method_t *method, ir_instruction_t *inst, JITUINT8 firstParam, JITUINT8 secondParam) {
    ir_instruction_t	temp;

    memset(&temp, 0, sizeof(ir_instruction_t));
    IRMETHOD_cpInstructionParameter(method, inst, firstParam, &temp, 1);
    IRMETHOD_cpInstructionParameter(method, inst, secondParam, inst, firstParam);
    IRMETHOD_cpInstructionParameter(method, &temp, 1, inst, secondParam);

    return ;
}

void IRMETHOD_cpInstructionParameterToItem (ir_method_t *method, ir_instruction_t *fromInst, JITUINT8 fromParam, ir_item_t *toItem) {
    ir_item_t *param;

    /* Get the parameter from the first instruction.
     */
    switch (fromParam) {
        case 0:
            param = ir_instr_result_get(fromInst);
            break;
        case 1:
            param = ir_instr_par1_get(fromInst);
            break;
        case 2:
            param = ir_instr_par2_get(fromInst);
            break;
        case 3:
            param = ir_instr_par3_get(fromInst);
            break;
        default:
            abort();
    }

    /* Copy the parameter.
     */
    if (param != toItem) {
        memcpy(toItem, param, sizeof(ir_item_t));
    }

    return ;
}

void IRMETHOD_cpInstructionParameter (ir_method_t *method, ir_instruction_t *fromInst, JITUINT8 fromParam, ir_instruction_t *toInst, JITUINT8 toParam) {
    ir_item_t *param;

    /* Set the flag				*/
    method->modified = JITTRUE;

    /* Get the parameter from the first instruction. */
    switch (fromParam) {
        case 0:
            param = ir_instr_result_get(fromInst);
            break;
        case 1:
            param = ir_instr_par1_get(fromInst);
            break;
        case 2:
            param = ir_instr_par2_get(fromInst);
            break;
        case 3:
            param = ir_instr_par3_get(fromInst);
            break;
        default:
            abort();
    }

    /* Copy into second instruction. */
    switch (toParam) {
        case 0:
            ir_instr_result_cp(toInst, param);
            break;
        case 1:
            ir_instr_par1_cp(toInst, param);
            break;
        case 2:
            ir_instr_par2_cp(toInst, param);
            break;
        case 3:
            ir_instr_par3_cp(toInst, param);
            break;
        default:
            abort();
    }
}

void IRMETHOD_setInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos, JITUINT32 parameterNumber) {
    ir_item_t * param;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Set the modified flag	*/
    method->modified = JITTRUE;

    /* Fetch the call parameter	*/
    param = ir_instrcall_par_get_by_pos(inst, parameterNumber);
    assert(param != NULL);

    /* Set the call parameter	*/
    if (_IRMETHOD_isAFloatType(internal_type)) {
        (param->value).f = fvalue;
    } else {
        (param->value).v = value;
    }
    param->type = type;
    param->internal_type = internal_type;
    param->type_infos = typeInfos;

    /* Return			*/
    return;
}

void IRMETHOD_setInstructionParameter (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos, JITUINT16 parameterNumber) {
    method->modified = JITTRUE;
    switch (parameterNumber) {
        case 0:
            ir_instr_result_set(inst, value, fvalue, type, internal_type, typeInfos);
            break;
        case 1:
            ir_instr_par1_set(inst, value, fvalue, type, internal_type, typeInfos);
            break;
        case 2:
            ir_instr_par2_set(inst, value, fvalue, type, internal_type, typeInfos);
            break;
        case 3:
            ir_instr_par3_set(inst, value, fvalue, type, internal_type, typeInfos);
            break;
        default:
            abort();
    }
}

void IRMETHOD_setInstructionParameter1 (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos) {
    method->modified = JITTRUE;
    ir_instr_par1_set(inst, value, fvalue, type, internal_type, typeInfos);
}

void IRMETHOD_setInstructionParameter2 (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos) {
    method->modified = JITTRUE;
    ir_instr_par2_set(inst, value, fvalue, type, internal_type, typeInfos);
}

void IRMETHOD_setInstructionParameter3 (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos) {
    method->modified = JITTRUE;
    ir_instr_par3_set(inst, value, fvalue, type, internal_type, typeInfos);
}

void IRMETHOD_setInstructionResult (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value, IR_ITEM_FVALUE fvalue, JITINT16 type, JITUINT16 internal_type, TypeDescriptor *typeInfos) {
    method->modified = JITTRUE;
    ir_instr_result_set(inst, value, fvalue, type, internal_type, typeInfos);
}

JITUINT32 IRMETHOD_newInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT32 position;

    ir_instrcall_par_new(method, inst, &position);
    return position;
}

XanList * IRMETHOD_getLabelInstructions (ir_method_t *method) {
    return IRMETHOD_getInstructionsOfType(method, IRLABEL);
}

XanList * IRMETHOD_getCallInstructions (ir_method_t *method) {
    XanList *l;
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions				*/
    assert(method != NULL);

    /* Allocate the list of instructions	*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the number of instructions of	*
     * the method				*/
    instructionsNumber = ir_instr_count(method);

    /* Fill up the list			*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *inst;
        inst = ir_instr_get_by_pos(method, instID);
        assert(inst != NULL);
        if (IRMETHOD_isACallInstruction(inst)) {
            xanList_append(l, inst);
        }
    }

    return l;
}

XanList * IRMETHOD_getMemoryInstructions (ir_method_t *method) {
    XanList *l;
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions				*/
    assert(method != NULL);

    /* Allocate the list of instructions	*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the number of instructions of	*
     * the method				*/
    instructionsNumber = ir_instr_count(method);

    /* Fill up the list			*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *inst;
        inst = ir_instr_get_by_pos(method, instID);
        assert(inst != NULL);
        if (IRMETHOD_isAMemoryInstruction(inst)) {
            xanList_append(l, inst);
        }
    }

    return l;
}

XanList * IRMETHOD_getInstructionsOfType (ir_method_t *method, JITUINT16 instructionsType) {
    XanList *l;
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions				*/
    assert(method != NULL);

    /* Allocate the list of instructions	*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the number of instructions of	*
     * the method				*/
    instructionsNumber = ir_instr_count(method);

    /* Fill up the list			*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *inst;
        inst = ir_instr_get_by_pos(method, instID);
        assert(inst != NULL);
        if (inst->type == instructionsType) {
            xanList_append(l, inst);
        }
    }

    return l;
}

ir_instruction_t ** IRMETHOD_getInstructionsWithPositions (ir_method_t *method) {
    ir_instruction_t    **insns;
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions.
     */
    assert(method != NULL);

    /* Fetch the number of instructions of the method.
     */
    instructionsNumber = ir_instr_count(method);

    /* Allocate the memory.
     */
    insns   = allocFunction(sizeof(ir_instruction_t *) * (instructionsNumber + 1));

    /* Fill up the array.
     */
    for (instID = 0; instID <= instructionsNumber; instID++) {
        insns[instID] = ir_instr_get_by_pos(method, instID);
    }

    return insns;
}

XanList * IRMETHOD_getInstructions (ir_method_t *method) {
    XanList         *l;
    JITUINT32 instructionsNumber;
    JITUINT32 instID;

    /* Assertions				*/
    assert(method != NULL);

    /* Allocate the list of instructions	*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the number of instructions of the method	*/
    instructionsNumber = ir_instr_count(method);

    /* Fill up the list					*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t        *inst;
        inst = ir_instr_get_by_pos(method, instID);
        assert(inst != NULL);
        xanList_append(l, inst);
    }

    /* Return				*/
    return l;
}

void IRMETHOD_setInstructionParameter1Value (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value) {
    method->modified = JITTRUE;
    ir_instr_par1_value_set(inst, value);
}

void IRMETHOD_setInstructionParameter2Value (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value) {
    method->modified = JITTRUE;
    ir_instr_par2_value_set(inst, value);
}

void IRMETHOD_setInstructionParameter3Value (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value) {
    method->modified = JITTRUE;
    ir_instr_par3_value_set(inst, value);
}

void IRMETHOD_setInstructionParameterResultValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE value) {
    method->modified = JITTRUE;
    ir_instr_result_value_set(inst, value);
}

void IRMETHOD_setInstructionParameter1FValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_FVALUE fvalue) {
    method->modified = JITTRUE;
    ir_instr_par1_fvalue_set(inst, fvalue);
}

void IRMETHOD_setInstructionParameter2FValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_FVALUE fvalue) {
    method->modified = JITTRUE;
    ir_instr_par2_fvalue_set(inst, fvalue);
}

void IRMETHOD_setInstructionParameter3FValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_FVALUE fvalue) {
    method->modified = JITTRUE;
    ir_instr_par3_fvalue_set(inst, fvalue);
}

void IRMETHOD_setInstructionParameterResultFValue (ir_method_t *method, ir_instruction_t * inst, IR_ITEM_VALUE fvalue) {
    method->modified = JITTRUE;
    ir_instr_result_fvalue_set(inst, fvalue);
}

JITBOOLEAN IRMETHOD_isASignedType (JITUINT32 type) {
    switch (type) {
        case IRNINT:
        case IRINT8:
        case IRINT16:
        case IRINT32:
        case IRINT64:
        case IRNFLOAT:
        case IRFLOAT32:
        case IRFLOAT64:
            return JITTRUE;
    }

    return JITFALSE;
}

JITBOOLEAN IRMETHOD_hasASignedType (ir_item_t *item) {
    IR_ITEM_VALUE type;

    /* Find the type to check. */
    switch (item->type) {
        case IROFFSET:
        case IRGLOBAL:
        case IRSYMBOL:
            type = item->internal_type;
            break;
        case IRTYPE:
            type = item->value.v;
            break;
        case IRNINT:
        case IRINT8:
        case IRINT16:
        case IRINT32:
        case IRINT64:
        case IRNFLOAT:
        case IRFLOAT32:
        case IRFLOAT64:
        case IRNUINT:
        case IRUINT8:
        case IRUINT16:
        case IRUINT32:
        case IRUINT64:
        case IROBJECT:
        case IRMPOINTER:
        case IRTPOINTER:
        case IRFPOINTER:
        case IRMETHODENTRYPOINT:
            type	= item->type;
            break;
        default:
            abort();
    }

    return IRMETHOD_isASignedType(type);
}

JITBOOLEAN IRMETHOD_isInstructionParameterAConstant (ir_instruction_t *inst, JITUINT8 parameterNumber) {
    ir_item_t       *param;
    param	= IRMETHOD_getInstructionParameter(inst, parameterNumber);
    return IRDATA_isAConstant(param);
}

JITBOOLEAN IRMETHOD_isInstructionParameterAVariable (ir_instruction_t *inst, JITUINT8 parameterNumber) {
    ir_item_t       *param;
    param	= IRMETHOD_getInstructionParameter(inst, parameterNumber);
    return IRDATA_isAVariable(param);
}

JITBOOLEAN IRMETHOD_isInstructionCallParameterAVariable (ir_instruction_t *inst, JITUINT32 parameterNumber) {
    ir_item_t       *param;

    /* Assertions			*/
    assert(inst != NULL);

    /* Fetch the call parameter	*/
    param = ir_instrcall_par_get_by_pos(inst, parameterNumber);

    /* Check the call parameter	*/
    if (    (param != NULL)                 &&
            (param->type == IROFFSET)       ) {

        /* Return			*/
        return JITTRUE;
    }

    /* Return			*/
    return JITFALSE;
}

void IRMETHOD_eraseInstructionParameter (ir_method_t *method, ir_instruction_t *inst, JITUINT16 parameterNumber) {
    ir_item_t *par;

    method->modified = JITTRUE;
    switch (parameterNumber) {
        case 0:
            par = ir_instr_result_get(inst);
            break;
        case 1:
            par = ir_instr_par1_get(inst);
            break;
        case 2:
            par = ir_instr_par2_get(inst);
            break;
        case 3:
            par = ir_instr_par3_get(inst);
            break;
        default:
            abort();
    }
    memset(par, 0, sizeof(ir_item_t));
    par->type = NOPARAM;
    par->internal_type = NOPARAM;
}

void IRMETHOD_setInstructionParameterWithANewVariable (ir_method_t *method, ir_instruction_t * inst, JITUINT16 internal_type, TypeDescriptor *typeInfos, JITUINT16 parameterNumber) {
    ir_item_t *item;

    /* Assertions.
     */
    assert(method != NULL);
    assert(inst != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));

    method->modified = JITTRUE;
    switch (parameterNumber) {
        case 0:
            item = ir_instr_result_get(inst);
            break;
        case 1:
            item = ir_instr_par1_get(inst);
            break;
        case 2:
            item = ir_instr_par2_get(inst);
            break;
        case 3:
            item = ir_instr_par3_get(inst);
            break;
        default:
            abort();
    }
    assert(item != NULL);
    memset(item, 0, sizeof(ir_item_t));
    item->type = IROFFSET;
    item->internal_type = internal_type;
    item->type_infos = typeInfos;
    (item->value).v = ir_instr_new_variable_ID(method);

    return;
}

void IRMETHOD_setInstructionParameter1Type (ir_method_t *method, ir_instruction_t * inst, JITINT16 type) {
    method->modified = JITTRUE;
    ir_instr_par1_type_set(inst, type);
}

void IRMETHOD_setInstructionParameter2Type (ir_method_t *method, ir_instruction_t * inst, JITINT16 type) {
    method->modified = JITTRUE;
    ir_instr_par2_type_set(inst, type);
}

void IRMETHOD_setInstructionParameter3Type (ir_method_t *method, ir_instruction_t * inst, JITINT16 type) {
    method->modified = JITTRUE;
    ir_instr_par3_type_set(inst, type);
}

void IRMETHOD_setInstructionResultValue (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE value) {
    method->modified = JITTRUE;
    ir_instr_result_value_set(inst, value);
}

void IRMETHOD_setInstructionResultType (ir_method_t *method, ir_instruction_t * inst, JITINT16 type) {
    method->modified = JITTRUE;
    ir_instr_result_type_set(inst, type);
}

void IRMETHOD_addInstructionCallParameter (ir_method_t *method, ir_instruction_t *inst, ir_item_t *newCallParameter) {
    JITUINT32 position;
    ir_item_t *item;

    item = ir_instrcall_par_new(method, inst, &position);
    memcpy(item, newCallParameter, sizeof(ir_item_t));
}

ir_method_t * IRMETHOD_getRunningMethod () {
    return ir_library->getCallerOfMethodInExecution(NULL);
}

ir_method_t * IRMETHOD_getCallerOfMethodInExecution (ir_method_t *m) {
    return ir_library->getCallerOfMethodInExecution(m);
}

JITBOOLEAN IRMETHOD_doesMethodInitializeGlobalMemory (ir_method_t *method) {
    MethodDescriptor	*d;
    d	= method->ID;
    if (d == NULL) {
        return JITFALSE;
    }
    return d->isCctor(d);
}

JITBOOLEAN IRMETHOD_canHaveIRBody (ir_method_t *method) {
    return ir_library->hasIRBody(method);
}

static inline void _IRMETHOD_setNativeCallInstruction (ir_method_t *method, ir_instruction_t *inst, char *functionToCallName, void *functionToCallPointer, ir_item_t *returnItem, XanList *callParameters) {
    inst->type = IRNATIVECALL;
    if (returnItem != NULL) {
        ir_instr_result_cp(inst, returnItem);
        ir_instr_par1_set(inst, returnItem->internal_type, 0, IRTYPE, IRTYPE, NULL);
    } else {
        ir_instr_par1_set(inst, IRVOID, 0, IRTYPE, IRTYPE, NULL);
    }
    ir_instr_par2_set(inst, (IR_ITEM_VALUE) (JITNUINT) functionToCallName, 0, IRSTRING, IRSTRING, NULL);
    ir_instr_par3_set(inst, (IR_ITEM_VALUE) (JITNUINT) functionToCallPointer, 0, IRMETHODENTRYPOINT, IRMETHODENTRYPOINT, NULL);
    if (callParameters != NULL) {
        XanListItem *item;
        item = xanList_first(callParameters);
        while (item != NULL) {
            IRMETHOD_addInstructionCallParameter(method, inst, (ir_item_t *) item->data);
            item = item->next;
        }
    }
    return;
}

JITBOOLEAN IRMETHOD_hasMethodCircuits (ir_method_t *method) {
    if (method->loop == NULL) {
        return JITFALSE;
    }
    if (xanList_length(method->loop) == 0) {
        return JITFALSE;
    }
    if (xanList_length(method->loop) == 1) {
        XanListItem *item;
        circuit_t *loop;
        item = xanList_first(method->loop);
        loop = item->data;
        if (loop->loop_id == 0) {
            return JITFALSE;
        }
    }
    return JITTRUE;
}

JITBOOLEAN IRMETHOD_hasMethodInstructions (ir_method_t *method) {
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    if (method->instructions != NULL) {
        instructionsNumber = ir_instr_count(method);
        for (count = 0; count < instructionsNumber; count++) {
            ir_instruction_t *inst;
            inst = ir_instr_get_by_pos(method, count);
            if (inst->type != IRNOP) {
                return JITTRUE;
            }
        }
    }
    return JITFALSE;
}

void IRMETHOD_tagMethodAsProfiled (ir_method_t *method) {

    /* Assertions			*/
    assert(method != NULL);

    /* Tag the method		*/
    ir_library->tagMethodAsProfiled(method);

    /* Return			*/
    return;
}

JITBOOLEAN IRDATA_isAFunctionPointer (ir_item_t *item) {
    return internal_isATaggedSymbol(item, FUNCTION_POINTER_SYMBOL);
}

JITINT8 * IRMETHOD_getInstructionTypeName (JITUINT16 instructionType) {
    switch (instructionType) {
        case IRPHI:
            return (JITINT8 *) "phi";
        case IRMOVE:
            return (JITINT8 *) "move";
        case IRADD:
            return (JITINT8 *) "add";
        case IRADDOVF:
            return (JITINT8 *) "add_ovf";
        case IRSUB:
            return (JITINT8 *) "sub";
        case IRSUBOVF:
            return (JITINT8 *) "sub_ovf";
        case IREXIT:
            return (JITINT8 *) "exit";
        case IRMUL:
            return (JITINT8 *) "mul";
        case IRMULOVF:
            return (JITINT8 *) "mul_ovf";
        case IRDIV:
            return (JITINT8 *) "div";
        case IRDIV_NOEXCEPTION:
            return (JITINT8 *) "div_noexception";
        case IRREM:
            return (JITINT8 *) "rem";
        case IRREM_NOEXCEPTION:
            return (JITINT8 *) "rem_noexception";
        case IRAND:
            return (JITINT8 *) "and";
        case IRNEG:
            return (JITINT8 *) "neg";
        case IROR:
            return (JITINT8 *) "or";
        case IRNOT:
            return (JITINT8 *) "not";
        case IRXOR:
            return (JITINT8 *) "xor";
        case IRSHL:
            return (JITINT8 *) "shl";
        case IRSHR:
            return (JITINT8 *) "shr";
        case IRCONV:
            return (JITINT8 *) "conv";
        case IRBITCAST:
            return (JITINT8 *) "bitcast";
        case IRCONVOVF:
            return (JITINT8 *) "conv_ovf";
        case IRBRANCH:
            return (JITINT8 *) "branch";
        case IRBRANCHIF:
            return (JITINT8 *) "branch_if";
        case IRBRANCHIFNOT:
            return (JITINT8 *) "branch_if_not";
        case IRLABEL:
            return (JITINT8 *) "label";
        case IRLT:
            return (JITINT8 *) "less_than";
        case IRGT:
            return (JITINT8 *) "greater_than";
        case IREQ:
            return (JITINT8 *) "equal";
        case IRCALL:
            return (JITINT8 *) "call";
        case IRLIBRARYCALL:
            return (JITINT8 *) "library_call";
        case IRVCALL:
            return (JITINT8 *) "virtual_call";
        case IRICALL:
            return (JITINT8 *) "indirect_call";
        case IRNATIVECALL:
            return (JITINT8 *) "native_call";
        case IRNEWOBJ:
            return (JITINT8 *) "new_object";
        case IRNEWARR:
            return (JITINT8 *) "new_array";
        case IRFREEOBJ:
            return (JITINT8 *) "free_object";
        case IRLOADREL:
            return (JITINT8 *) "load_relative";
        case IRSTOREREL:
            return (JITINT8 *) "store_relative";
        case IRFENCE:
            return (JITINT8 *) "fence";
        case IRGETADDRESS:
            return (JITINT8 *) "get_address";
        case IRRET:
            return (JITINT8 *) "return";
        case IRCALLFINALLY:
            return (JITINT8 *) "call_finally";
        case IRTHROW:
            return (JITINT8 *) "throw";
        case IRSTARTFILTER:
            return (JITINT8 *) "start_filter";
        case IRENDFILTER:
            return (JITINT8 *) "end_filter";
        case IRSTARTFINALLY:
            return (JITINT8 *) "start_finally";
        case IRENDFINALLY:
            return (JITINT8 *) "end_finally";
        case IRSTARTCATCHER:
            return (JITINT8 *) "start_catcher";
        case IRBRANCHIFPCNOTINRANGE:
            return (JITINT8 *) "branch_if_pc_not_in_range";
        case IRCALLFILTER:
            return (JITINT8 *) "call_filter";
        case IRISNAN:
            return (JITINT8 *) "is_nan";
        case IRISINF:
            return (JITINT8 *) "is_inf";
        case IRINITMEMORY:
            return (JITINT8 *) "init_memory";
        case IRALLOCA:
            return (JITINT8 *) "alloca";
        case IRMEMCPY:
            return (JITINT8 *) "memcpy";
        case IRMEMCMP:
            return (JITINT8 *) "memcmp";
        case IRCHECKNULL:
            return (JITINT8 *) "check_null";
        case IRNOP:
            return (JITINT8 *) "nop";
        case IRCOSH:
            return (JITINT8 *) "cosh";
        case IRCEIL:
            return (JITINT8 *) "ceil";
        case IRSIN:
            return (JITINT8 *) "sin";
        case IRCOS:
            return (JITINT8 *) "cos";
        case IRACOS:
            return (JITINT8 *) "acos";
        case IRSQRT:
            return (JITINT8 *) "sqrt";
        case IRFLOOR:
            return (JITINT8 *) "floor";
        case IRPOW:
            return (JITINT8 *) "pow";
        case IREXP:
            return (JITINT8 *) "exp";
        case IRLOG10:
            return (JITINT8 *) "log10";
        case IRALLOC:
            return (JITINT8 *) "alloc";
        case IRCALLOC:
            return (JITINT8 *) "calloc";
        case IRREALLOC:
            return (JITINT8 *) "realloc";
        case IRALLOCALIGN:
            return (JITINT8 *) "alloc_align";
        case IRFREE:
            return (JITINT8 *) "free";
        case IRARRAYLENGTH:
            return (JITINT8 *) "array_length";
        case IRSTRINGLENGTH:
            return (JITINT8 *) "string_length";
        case IRSTRINGCMP:
            return (JITINT8 *) "string_cmp";
        case IRSTRINGCHR:
            return (JITINT8 *) "string_chr";
        case IRSIZEOF:
            return (JITINT8 *) "sizeof";
        case IREXITNODE:
            return (JITINT8 *) "exitnode";
        default:
            abort();
    }
}

XanList * IRMETHOD_getVariableDefinitionsThatReachInstruction (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varID) {
    JITNUINT	b;
    JITUINT32	posID;
    JITUINT32	instructionsNumber;
    XanList		*l;

    instructionsNumber	= IRMETHOD_getInstructionsNumber(method);
    l			= xanList_new(allocFunction, freeFunction, NULL);
    posID			= 0;
    for (JITUINT32 blockNumber = 0; blockNumber < method->reachingDefinitionsBlockNumbers; blockNumber++) {
        JITNUINT	temp;
        b	= reachingDefinitionsGetIn(method, inst, blockNumber);
        temp 	= 1;
        for (JITUINT32 bitNumber=0; (bitNumber < (sizeof(JITNUINT) * 8)) && (posID < instructionsNumber); bitNumber++) {
            if ((temp & b) == temp) {
                ir_instruction_t	*defInst;
                IR_ITEM_VALUE		defVar;

                /* The instruction correspondent to the current bit reaches the instruction.
                 * Check if this instruction defines the variable					*/
                defInst	= ir_instr_get_by_pos(method, posID);
                defVar	= IRMETHOD_getVariableDefinedByInstruction(method, defInst);
                if (defVar == varID) {
                    xanList_append(l, defInst);
                }
            }
            temp 	= temp << 1;
            posID++;
        }
    }

    return l;
}

static inline JITBOOLEAN internal_is_valid_information (ir_method_t *method, JITUINT64 codetool_type) {
    return ((method->valid_optimization & codetool_type) == codetool_type);
}

void IRMETHOD_setInstructionDistance (ir_method_t *method, ir_instruction_t *inst, JITINT16 distance) {
    JITUINT32	expanded;
    expanded	= (JITUINT32)distance;
    expanded	= (expanded << 16) & 0xFFFF0000;

    inst->flags	&= 0x0000FFFF;
    inst->flags	|= expanded;

    return ;
}

JITINT16 IRMETHOD_getInstructionDistance (ir_method_t *method, ir_instruction_t *inst) {
    JITUINT16	distance;
    distance	= (inst->flags & 0xFFFF0000);
    distance	= distance >> 16;
    return (JITINT16) distance;
}

JITBOOLEAN IRMETHOD_isInstructionVolatile (ir_instruction_t *inst) {
    JITBOOLEAN	isVolatile;
    isVolatile	= ((inst->flags & INSN_VOLATILE_FLAG) == INSN_VOLATILE_FLAG);
    return isVolatile;
}

JITBOOLEAN IRMETHOD_isACallInstructionToAVarArgMethod (ir_instruction_t *inst) {
    JITBOOLEAN	isVararg;
    isVararg	= ((inst->flags & INSN_VARARG_FLAG) == INSN_VARARG_FLAG);
    return isVararg;
}

void IRMETHOD_setInstructionAsVarArg  (ir_method_t *method, ir_instruction_t *inst, JITBOOLEAN isVararg) {
    if (isVararg) {
        inst->flags	|= INSN_VARARG_FLAG;
    } else {
        inst->flags	&= ~INSN_VARARG_FLAG;
    }

    return ;
}

void IRMETHOD_setInstructionAsVolatile (ir_method_t *method, ir_instruction_t *inst, JITBOOLEAN isVolatile) {
    if (isVolatile) {
        inst->flags	|= INSN_VOLATILE_FLAG;
    } else {
        inst->flags	&= ~INSN_VOLATILE_FLAG;
    }

    return ;
}

JITBOOLEAN IRMETHOD_haveInstructionsTheSameExecutionEffects (ir_method_t *method, XanList *instructions) {
    ir_instruction_t	*inst1;
    XanListItem		*item;

    item	= xanList_first(instructions);
    inst1	= item->data;
    item	= item->next;
    while (item != NULL) {
        ir_instruction_t	*inst2;
        inst2	= item->data;
        if (!IRMETHOD_haveBothInstructionsTheSameExecutionEffects(method, inst1, inst2)) {
            return JITFALSE;
        }
        item	= item->next;
    }
    return JITTRUE;
}

JITBOOLEAN IRMETHOD_haveBothInstructionsTheSameExecutionEffects (ir_method_t *method, ir_instruction_t *inst1, ir_instruction_t *inst2) {
    JITUINT32	pars;
    JITUINT32	count;
    ir_item_t	*res1;
    ir_item_t	*res2;

    /* Check the trivial case where instructions are alias.
     */
    if (inst1 == inst2) {
        return JITTRUE;
    }

    /* Check the instruction type.
     */
    if (inst1->type != inst2->type) {
        return JITFALSE;
    }

    /* Check the number of parameters.
     */
    pars	= ir_instr_pars_count_get(inst1);
    if (pars != ir_instr_pars_count_get(inst2)) {
        return JITFALSE;
    }

    /* Check the parameters.
     */
    for (count=1; count <= pars; count++) {
        ir_item_t *inst1Par;
        ir_item_t *inst2Par;
        inst1Par	= IRMETHOD_getInstructionParameter(inst1, count);
        inst2Par	= IRMETHOD_getInstructionParameter(inst2, count);
        if (!internal_areItemsEqual(inst1Par, inst2Par)) {
            return JITFALSE;
        }
    }

    /* Check the result.
     */
    res1	= IRMETHOD_getInstructionResult(inst1);
    res2	= IRMETHOD_getInstructionResult(inst2);
    if (!internal_areItemsEqual(res1, res2)) {
        return JITFALSE;
    }

    /* Check the number of call parameters.
     */
    pars	= IRMETHOD_getInstructionCallParametersNumber(inst1);
    if (pars != IRMETHOD_getInstructionCallParametersNumber(inst2)) {
        return JITFALSE;
    }

    /* Check the call parameters.
     */
    for (count=0; count < pars; count++) {
        ir_item_t *inst1Par;
        ir_item_t *inst2Par;
        inst1Par	= IRMETHOD_getInstructionCallParameter(inst1, count);
        inst2Par	= IRMETHOD_getInstructionCallParameter(inst2, count);
        if (!internal_areItemsEqual(inst1Par, inst2Par)) {
            return JITFALSE;
        }
    }

    return JITTRUE;
}

static inline JITBOOLEAN internal_areItemsEqual (ir_item_t *item1, ir_item_t *item2) {
    if (	(item1->internal_type != item2->internal_type)	||
            (item1->type != item2->type)			||
            ((item1->value).v != (item2->value).v)		||
            ((item1->value).f != (item2->value).f)		) {
        return JITFALSE;
    }
    return JITTRUE;
}

void IRMETHOD_refreshInstructionsSuccessors (ir_method_t *method, XanList ***successors, JITUINT32 *instructionsNumber) {

    IRMETHOD_destroyInstructionsSuccessors(*successors, *instructionsNumber);
    (*successors)		= IRMETHOD_getInstructionsSuccessors(method);
    (*instructionsNumber)	= IRMETHOD_getInstructionsNumber(method);

    return ;
}

void IRMETHOD_refreshInstructionsPredecessors (ir_method_t *method, XanList ***predecessors, JITUINT32 *instructionsNumber) {

    IRMETHOD_destroyInstructionsPredecessors(*predecessors, *instructionsNumber);
    (*predecessors)		= IRMETHOD_getInstructionsPredecessors(method);
    (*instructionsNumber)	= IRMETHOD_getInstructionsNumber(method);

    return ;
}

void IRMETHOD_destroyInstructionsPredecessors (XanList **predecessors, JITUINT32 instructionsNumber) {

    /* Free the memory.
     */
    for (JITUINT32 count=0; count <= instructionsNumber; count++) {
        xanList_destroyList(predecessors[count]);
    }
    freeFunction(predecessors);

    return ;
}

void IRMETHOD_destroyInstructionsSuccessors (XanList **successors, JITUINT32 instructionsNumber) {
    IRMETHOD_destroyInstructionsPredecessors(successors, instructionsNumber);
    return ;
}

XanList ** IRMETHOD_getInstructionsSuccessors (ir_method_t *method) {
    XanList			**successors;
    JITUINT32		instructionsNumber;
    JITBOOLEAN		catcherInstInitialized;
    ir_instruction_t	*catcherInst;

    /* Assertions.
     */
    assert(method != NULL);

    catcherInst		= NULL;
    catcherInstInitialized	= JITFALSE;
    instructionsNumber    	= IRMETHOD_getInstructionsNumber(method);
    successors          	= allocFunction(sizeof(XanList *) * (instructionsNumber + 1));
    for (JITUINT32 count=0; count <= instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst                    	= IRMETHOD_getInstructionAtPosition(method, count);
        successors[count]     	= internal_getInstructionSuccessors(method, inst, &catcherInst, &catcherInstInitialized);
    }

    return successors;
}

XanList ** IRMETHOD_getInstructionsPredecessors (ir_method_t *method) {
    XanList			**predecessors;
    JITUINT32		instructionsNumber;
    JITBOOLEAN		catcherInstInitialized;
    ir_instruction_t	*catcherInst;

    /* Initialize the variables.
     */
    catcherInst		= NULL;
    catcherInstInitialized	= JITFALSE;

    /* Allocate the memory used to store predecessors.
     */
    instructionsNumber    = IRMETHOD_getInstructionsNumber(method);
    predecessors          = allocFunction(sizeof(XanList *) * (instructionsNumber + 1));

    /* Store predecessors.
     */
    for (JITUINT32 count=0; count <= instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst                    = IRMETHOD_getInstructionAtPosition(method, count);
        predecessors[count]     = internal_getInstructionPredecessors(method, inst, &catcherInst, &catcherInstInitialized);
    }

    return predecessors;
}

static inline void internal_destroyDataDependences (XanHashTable *l) {
    XanHashTableItem *item;

    /* Free dependences.
     */
    item 	= xanHashTable_first(l);
    while (item != NULL) {
        data_dep_arc_list_t *dd;

        /* Fetch the dependence.
         */
        dd = item->element;
        assert(dd != NULL);
        assert(dd->inst != NULL);

        /* Free the dependence.
         */
        internal_destroyDataDependence(dd);

        /* Fetch the next element from the list.
         */
        item	= xanHashTable_next(l, item);
    }

    /* Free the list.
     */
    xanHashTable_destroyTable(l);

    return ;
}

static inline void internal_destroyDataDependence (data_dep_arc_list_t *d) {
    if (d->outsideInstructions != NULL) {
        xanList_destroyList(d->outsideInstructions);
        d->outsideInstructions	= NULL;
    }
    freeFunction(d);

    return ;
}

void IRMETHOD_setMethodAsCallableExternally (ir_method_t *method, JITBOOLEAN callableExternally) {
    method->isExternalLinked	= callableExternally;
    return ;
}

JITBOOLEAN IRMETHOD_isCallableExternally (ir_method_t *method) {
    return method->isExternalLinked;
}

static inline void ir_instr_par_delete (ir_item_t *p) {

    if (p == NULL) {
        return ;
    }

    switch (p->type) {
        case IRSIGNATURE:
            assert(p->internal_type == IRSIGNATURE);
            /*if ((p->value).v != 0){
            	void	*pointer;
            	pointer	= (void *)(JITNUINT)(p->value).v;
            	freeFunction(pointer);
            	(p->value).v	= 0;
            }*/
            break;
    }

    return ;
}

void IRMETHOD_baseAddressAndOffsetOfAccessedMemoryLocation (ir_instruction_t *inst, ir_item_t *ba, ir_item_t *offset) {

    /* Assertions.
     */
    assert(inst != NULL);
    assert(ba != NULL);
    assert(offset != NULL);

    /* Initialize variables.
     */
    memset(ba, 0, sizeof(ir_item_t));
    memset(offset, 0, sizeof(ir_item_t));
    ba->type		= NOPARAM;
    ba->internal_type	= ba->type;
    offset->type		= ba->type;
    offset->internal_type	= ba->type;

    /* Fetch the address.
     */
    if (	(inst->type == IRSTOREREL)	||
            (inst->type == IRLOADREL)	) {
        if (!IRMETHOD_isInstructionParameterAVariable(inst, 1)) {
            ir_item_t	*par2;
            par2	= IRMETHOD_getInstructionParameter2(inst);
            if (IRDATA_isAConstant(par2)) {
                ir_item_t	*par1;

                /* Store the base address.
                 */
                par1	= IRMETHOD_getInstructionParameter1(inst);
                memcpy(ba, par1, sizeof(ir_item_t));

                /* Store the offset.
                 */
                memcpy(offset, par2, sizeof(ir_item_t));

                return ;
            }
        }
    }

    return ;
}

void IRMETHOD_addressOfAccessedMemoryLocation (ir_instruction_t *inst, ir_item_t *addr) {

    /* Assertions.
     */
    assert(inst != NULL);
    assert(addr != NULL);

    /* Initialize variables.
     */
    memset(addr, 0, sizeof(ir_item_t));
    addr->type		= NOPARAM;
    addr->internal_type	= addr->type;

    /* Fetch the address.
     */
    if (	(inst->type == IRSTOREREL)	||
            (inst->type == IRLOADREL)	) {
        if (!IRMETHOD_isInstructionParameterAVariable(inst, 1)) {
            ir_item_t	*par2;
            par2	= IRMETHOD_getInstructionParameter2(inst);
            if (IRDATA_isAConstant(par2)) {
                ir_item_t	*par1;

                /* Fetch the base address.
                 */
                par1	= IRMETHOD_getInstructionParameter1(inst);
                if (IRDATA_isASymbol(par1)) {
                    IRSYMBOL_resolveSymbolFromIRItem(par1, addr);
                } else {
                    memcpy(addr, par1, sizeof(ir_item_t));
                }
                assert(IRDATA_isAConstant(addr));

                /* Add the offset.
                 */
                ((addr->value).v)	+= (par2->value).v;

                return ;
            }
        }
    }

    return ;
}

static inline JITBOOLEAN internal_is_instruction_reachable_in (data_flow_t *reachableInstructions, ir_instruction_t *inst, ir_instruction_t *position) {
    JITUINT32 	trigger;
    JITNUINT 	temp;
    JITUINT32	instID;

    /* Assertions.
     */
    assert(reachableInstructions != NULL);
    assert(inst != NULL);
    assert(position != NULL);

    /* Fetch the ID of the instruction to check.
     */
    instID	= inst->ID;

    /* Check whether the instruction reaches the position specified.
     */
    trigger = instID / (sizeof(JITNUINT) * 8);
    assert(trigger < (reachableInstructions->elements));
    temp = 0x1;
    temp = temp << ((instID) % (sizeof(JITNUINT) * 8));
    return (((reachableInstructions->data[position->ID]).in[trigger]) & temp) != 0;
}

XanList * IRMETHOD_getInstructionsWithinAnyCircuitsOfSameHeader (ir_method_t *method, ir_instruction_t *header) {
    XanList         *l;
    XanListItem     *item;
    JITBOOLEAN	*instsAdded;

    /* Assertions					*/
    assert(method != NULL);
    assert(header != NULL);
    assert(IRMETHOD_doesInstructionBelongToMethod(method, header));

    /* Allocate the list of instructions		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Check if there are loops			*/
    if (method->loop == NULL) {

        /* Return					*/
        return l;
    }

    /* Looking for loops with the header		*/
    instsAdded	= allocFunction(sizeof(JITBOOLEAN) * (IRMETHOD_getInstructionsNumber(method) + 1));
    item 		= xanList_first(method->loop);
    while (item != NULL) {
        circuit_t  *loop;
        loop = item->data;
        assert(loop != NULL);
        if (loop->header_id == header->ID) {
            XanList         *tmp;
            XanListItem     *item2;

            /* Fetch the list of instructions of	*
             * current loop				*/
            tmp = IRMETHOD_getCircuitInstructions(method, loop);
            assert(tmp != NULL);

            /* Add the instructions not already     *
             * present within the list		*/
            item2 = xanList_first(tmp);
            while (item2 != NULL) {
                ir_instruction_t	*i;
                i	= item2->data;
                assert(i != NULL);
                if (instsAdded[i->ID] == JITFALSE) {
                    instsAdded[i->ID]	= JITTRUE;
                    xanList_append(l, i);
                }
                item2 	= item2->next;
            }

            /* Free the memory.
             */
            xanList_destroyList(tmp);
        }
        item = item->next;
    }
    freeFunction(instsAdded);

    return l;
}

static inline XanList * internal_getInstructionSuccessors (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized) {
    XanList                 *l;
    ir_instruction_t        *pred;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Allocate the list		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fill up the list		*/
    pred = ir_instr_get_successor(method, inst, NULL, catcherInst, catcherInstInitialized);
    while (pred != NULL) {
        xanList_append(l, pred);
        pred = ir_instr_get_successor(method, inst, pred, catcherInst, catcherInstInitialized);
    }

    return l;
}

static inline XanList * internal_getInstructionPredecessors (ir_method_t *method, ir_instruction_t *inst, ir_instruction_t **catcherInst, JITBOOLEAN *catcherInstInitialized) {
    XanList                 *l;
    ir_instruction_t        *pred;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);

    /* Allocate the list		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fill up the list		*/
    pred = ir_instr_get_predecessor(method, inst, NULL, catcherInst, catcherInstInitialized);
    while (pred != NULL) {
        xanList_append(l, pred);
        pred = ir_instr_get_predecessor(method, inst, pred, catcherInst, catcherInstInitialized);
    }

    return l;
}

static inline void internal_addInstructionsThatCanBeReachedFromPosition (ir_method_t *method, XanHashTable *t, JITUINT32 positionID, data_flow_t *reachableInstructions, JITUINT32 instructionsNumber, ir_instruction_t **insns) {
    JITUINT32	trigger;
    JITUINT32	instID;
    JITNUINT        *in;

    /* Fetch all instructions that can be reached from position.
     */
    in			= (reachableInstructions->data[positionID]).in;
    assert(in != NULL);
    instID			= 0;
    for (trigger=0; trigger < reachableInstructions->elements; trigger++) {
        JITUINT32	elemID;
        for (elemID=0; (elemID < (sizeof(JITNUINT) * 8)) && (instID < instructionsNumber); elemID++,instID++) {
            ir_instruction_t	*inst;
            JITNUINT		temp;
            temp	= 0x1 << elemID;
            if ((in[trigger] & temp) == 0) {
                continue ;
            }
            if (insns != NULL) {
                inst    = insns[instID];
            } else {
                inst	= ir_instr_get_by_pos(method, instID);
            }
            assert(inst != NULL);
            xanHashTable_insert(t, inst, inst);
        }
    }

    return ;
}
