/*
 * Copyright (C) 2012 Simone Campanoni
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
#include <jitsystem.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <xanlib.h>
#include <dominance.h>
#include <string.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
// End

static inline JITBOOLEAN isExitPoint (IRBasicBlock* bb);
static inline void explode (XanNode* bb_dominance, XanNode* currNode);
static inline IRBasicBlock * getSuccessor (IRBasicBlock *elem, IRBasicBlock *curr);
static inline IRBasicBlock * getPredecessor (IRBasicBlock *elem, IRBasicBlock *curr);

extern ir_optimizer_t  	*irOptimizer;
static ir_method_t     	*_method = NULL;
static IRBasicBlock	*fakeHead = NULL;   // fake heading node which reaches all exit points of the method
static XanList		*terminators = NULL;   // list of exit point of the method

void DOMINANCE_computePostdominance (ir_optimizer_t *irOptimizer, ir_method_t *method) {
    JITUINT32 numElems;
    XanNode* root = NULL;
    XanNode* res = NULL;
    XanNode* r = NULL;
    IRBasicBlock** blocchi;
    JITUINT32 i;
    ir_instruction_t *inst;

    /* Compute the basic blocks.
     */
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);

    // Set global reference to method
    _method = method;

    /* Fetch the number of basic blocks included in the method.
     */
    numElems = IRMETHOD_getNumberOfMethodBasicBlocks(method);

    /* if postDominatorTree is not NULL free memory before recomputing postDominanceTree.
     */
    if (method->postDominatorTree != NULL) {
        (method->postDominatorTree)->destroyTree(method->postDominatorTree);
        method->postDominatorTree = NULL;
    }

    /* Check if there are instructions.
     */
    if (numElems == 0) {
        return;
    }

    /* Allocate the necessary memory (numElems+fakeHead)		*/
    blocchi = allocFunction((numElems + 1) * sizeof(IRBasicBlock*));

    // init fake head
    fakeHead = (IRBasicBlock*) allocFunction(sizeof(IRBasicBlock));

    /* Create temporary array of pointers to BBs.
     */
    terminators = xanList_new(allocFunction, freeFunction, NULL);
    blocchi[0] = fakeHead;
    for (i = 1; i <= numElems; i++) {
        blocchi[i] = IRMETHOD_getBasicBlockAtPosition(method, i - 1);
        if (isExitPoint(blocchi[i])) {
            xanList_append(terminators, blocchi[i]);
        }
    }

    /* Compute the dominance relation.
     */
    root = calculateDominanceTree((void **) blocchi, numElems + 1, 0, (void * (*)(void *, void*))getSuccessor, (void * (*)(void *, void*))getPredecessor);

    /* Prepare the starting node for recursion (the exit node).
     */
    res = xanNode_new(allocFunction, freeFunction, NULL);
    res->setData(res, method->exitNode);
    explode(root, res);

    /* Prepare the fakeHead node.
     */
    r = xanNode_new(allocFunction, freeFunction, NULL);
    inst = 	method->exitNode;
    r->setData(r, inst);
    res->addChildren(res, r);       //The fakehead is a successor of exit in the post-dominance tree

    /* Save the analysis performed in the method.
     */
    method->postDominatorTree = res;

    /* Free the memory.
     */
    freeFunction(fakeHead);
    freeFunction(blocchi);
    xanList_destroyList(terminators);
    root->destroyTree(root);

    return;
}

static inline IRBasicBlock * getPredecessor (IRBasicBlock *elem, IRBasicBlock *curr) {
    if (elem == fakeHead) { // the fake head has as predecessors all originals exit points
        if (curr == NULL) {
            return xanList_getData(xanList_first(terminators));
        }
        XanListItem* item = xanList_find(terminators, curr);
        if (item == NULL) {
            return NULL;
        }
        item = item->next;
        if (item == NULL) {
            return NULL;
        }
        return item->data;
    }
    ir_instruction_t* bb_head = IRMETHOD_getInstructionAtPosition(_method, elem->startInst);

    ir_instruction_t* curr_bb_tail = NULL;
    ir_instruction_t* pred;
    if (curr != NULL) {
        curr_bb_tail = IRMETHOD_getInstructionAtPosition(_method, curr->endInst);
    }

    pred = IRMETHOD_getPredecessorInstruction(_method, bb_head, curr_bb_tail);
    while(	(bb_head->type == IREXITNODE)		&&
            (pred != NULL)				&&
            (IRMETHOD_isACallInstruction(pred))	) {
        pred = IRMETHOD_getPredecessorInstruction(_method, bb_head, pred);
    }
    if (pred == NULL) {
        return NULL;
    }
    return IRMETHOD_getBasicBlockContainingInstruction(_method, pred);
}

static inline IRBasicBlock * getSuccessor (IRBasicBlock *elem, IRBasicBlock *curr) {
    if (elem == fakeHead) {                                   // fake head has no successors
        return NULL;
    }
    if (xanList_find(terminators, elem) != NULL) { // originals exit points has as successor only the fake head
        if (curr == NULL) {
            return fakeHead;
        }
        return NULL;
    }
    ir_instruction_t* bb_tail = IRMETHOD_getInstructionAtPosition(_method, elem->endInst);
    ir_instruction_t* curr_bb_head = NULL;
    ir_instruction_t* succ;

    if (curr != NULL) {
        curr_bb_head = IRMETHOD_getInstructionAtPosition(_method, curr->startInst);
    }

    succ = IRMETHOD_getSuccessorInstruction(_method, bb_tail, curr_bb_head);
    if (	(IRMETHOD_isACallInstruction(bb_tail))	&&
            (succ != NULL)				&&
            (succ->type == IREXITNODE)		) {
        succ = IRMETHOD_getSuccessorInstruction(_method, bb_tail, succ);
    }
    if (succ == NULL) {
        return NULL;
    }
    IRBasicBlock* temp = IRMETHOD_getBasicBlockContainingInstruction(_method, succ);

    return temp;
}

static inline void explode (XanNode* bb_dominance, XanNode* currNode) {
    IRBasicBlock* block = (IRBasicBlock*) bb_dominance->getData(bb_dominance);

    XanNode* child;
    XanNode* tempNode;

    if (block != fakeHead) {
        JITINT32 i;
        for (i = block->endInst; i >= (JITINT32) block->startInst; i--) {
            tempNode = xanNode_new(allocFunction, freeFunction, NULL);
            tempNode->setData(tempNode, IRMETHOD_getInstructionAtPosition(_method, i));
            currNode->addChildren(currNode, tempNode);
            currNode = tempNode;
        }
    }

    child = bb_dominance->getNextChildren(bb_dominance, NULL);
    while (child != NULL) {
        explode(child, currNode);
        child = bb_dominance->getNextChildren(bb_dominance, child);
    }
}

static inline JITBOOLEAN isExitPoint (IRBasicBlock* bb) {
    ir_instruction_t* endI = IRMETHOD_getInstructionAtPosition(_method, bb->endInst);

    if (endI->type == IREXITNODE) {
        return JITTRUE;
    }
    if (IRMETHOD_isACallInstruction(endI)) {
        return JITFALSE;
    }
    ir_instruction_t* succ = IRMETHOD_getSuccessorInstruction(_method, endI, NULL);
    if (succ == NULL || succ->type == IREXITNODE) {
        return JITTRUE;
    }
    return JITFALSE;
}
