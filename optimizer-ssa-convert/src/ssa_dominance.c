/*
 * Copyright (C) 2007 - 2010  Campanoni Simone, Michele Melchiori
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
#include <dominance.h>

#include <ssa_dominance.h>
#include <definitions.h>

extern ir_lib_t *irLib;
extern ir_optimizer_t *irOptimizer;
extern char *prefix;

/**
 * @brief returns the predecessor of the element passed as first parameter
 * that is after the element passed as second parameter, or NULL if elem has no more precessors
 */
void* getPredecessor(void* elem, void* curr) {
    if (elem == NULL) {
        return NULL;
    }

    BasicBlockInfo* bb      = (BasicBlockInfo*) elem;
    BasicBlockInfo* curr_bb = (BasicBlockInfo*) curr;
    assert(bb != NULL);
    XanListItem*    item    = xanList_first(bb->predList);
    while (item != NULL && curr_bb != NULL) {
        BasicBlockInfo* bb_iterator = xanList_getData(item);
        if (bb_iterator == curr_bb) {
            curr_bb = NULL;
        }
        item = item->next;
    }
    assert(curr_bb == NULL);
    if (item == NULL) {
        return NULL;
    }
    return xanList_getData(item);
}

/**
 * @brief returns the successor of the element passed as first parameter
 * that is after the element passed as second parameter, or NULL if elem has no more successors
 */
void* getSuccessor(void* elem, void* curr) {
    if (elem == NULL) {
// 		if (curr == NULL) {
        /* TODO verify if the next piece of code is really necessary or if it can be replaced with a return NULL */
#if 0
        ir_instruction_t* firstInst = IRMETHOD_getFirstInstruction(glob->method);
        BasicBlock*       firstBB   = getBasicBlockContainingInstruction(glob->method, firstInst);
        return firstBB;
#endif
// 			return NULL;
// 		} else {
        return NULL;
// 		}
    }

    BasicBlockInfo* bb      = (BasicBlockInfo*) elem;
    BasicBlockInfo* curr_bb = (BasicBlockInfo*) curr;
    assert(bb != NULL);
    XanListItem*    item    = xanList_first(bb->succList);
    while (item != NULL && curr_bb != NULL) {
        BasicBlockInfo* bb_iterator = xanList_getData(item);
        if (curr_bb == bb_iterator) {
            curr_bb = NULL;
        }
        item = item->next;
    }
    assert(curr_bb == NULL); // otherwise we have not found the next successor
    if (item == NULL) {
        return NULL;
    }
    return xanList_getData(item);
}

/**
 * @brief use the method providen by libdominance for building the dominance tree of basic blocks in the root global variable
 *
 * The algorithm is taken from the one in optimizer-predominators, with some modifications,
 * in particular some controls (ueseless here) have been removed.
 *
 * The trailing part of the method creates one clone of the tree and converts it in a tree
 * of BasicBlockInfo structure, so we can keep also informations on the dominance frontier
 * of each basic block.
 */
void ssa_convert_buildDominanceTree(GlobalObjects* glob) {
    ir_method_t*     method             = glob->method;
    JITUINT32        numOfBB            = IRMETHOD_getNumberOfMethodBasicBlocks(method); ///< number of basic blocks in the method
    XanList*         noPredecessorNodes;
    BasicBlockInfo** basicBlocks;
    JITUINT32        count;
    JITUINT32        rootBlockIdx       = -1; ///< index in the array basicBlocks of the root block

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Start building dominance tree\n", IRMETHOD_getMethodName(method));

    // Check trivial case (only one basic block in the method, or no instructions)
    // note that one basic block is the one with only STOP() instruction
    if (numOfBB <= 2) {
        glob->root = NULL;
        return;
    }

    /* Allocate the list of basic blocks without predecessors.
     */
    noPredecessorNodes = xanList_new(allocFunction, freeFunction, NULL);

    basicBlocks = allocMemory(numOfBB * sizeof(BasicBlockInfo*));

    // Crate array of pointers to basic blocks
    ir_instruction_t* first = IRMETHOD_getFirstInstruction(method);
    assert(first != NULL);
    for (count = 0; count < numOfBB; count++) {
        BasicBlockInfo* out = allocMemory(sizeof(BasicBlockInfo));
        out->basicBlock     = IRMETHOD_getBasicBlockAtPosition(method, count);
        out->startInst      = IRMETHOD_getInstructionAtPosition(method, out->basicBlock->startInst);
        out->endInst        = IRMETHOD_getInstructionAtPosition(method, out->basicBlock->endInst);
        out->predList       = IRMETHOD_getInstructionPredecessors(method, out->startInst);
        out->succList       = IRMETHOD_getInstructionSuccessors(method, out->endInst);
        out->domFront       = NULL;
        basicBlocks[count] = out;
        /* fprintf(stderr, "Block %u, startInst %u, endInst %u\n", count, out->startInst->ID, out->endInst->ID); */

        // look for the root basic block (the entry point of the method)
        if (basicBlocks[count]->startInst == first) {
            rootBlockIdx = count;
        }
    }
    assert(rootBlockIdx != -1lu); // we must have found the root block!
    /* update pointers of predecessors and successors in order to make them */
    /* point to BasicBlockInfo structures instead of ir_instruction_t       */
    for (count = 0; count < numOfBB; count++) {
        BasicBlockInfo* bb   = basicBlocks[count];
        /*** loop throug all predecessors ***/
        XanListItem*    item = xanList_first(bb->predList);
        // keep track if this basic block has no predecessors and it is not the node
        // containig the first instruction of the method
        if (item == NULL && count != rootBlockIdx) {
            xanList_append(noPredecessorNodes, bb);
        }
        while (item != NULL) {
            ir_instruction_t* inst = xanList_getData(item); // the predecessor
            JITNUINT count2;
            for (count2 = 0; count2 < numOfBB; count2++) {
                BasicBlockInfo* bb2 = basicBlocks[count2];
                if (bb2->endInst == inst) {
                    *(xanList_getSlotData(item)) = bb2;
                    count2 = numOfBB;
                }
            }
            assert(count2 == numOfBB + 1); // if count2 == numOfBB means that we have not found the basic block
            item = item->next;
        }
        /*** loop through all successors ***/
        item = xanList_first(bb->succList);
        while (item != NULL) {
            ir_instruction_t* inst = xanList_getData(item);
            JITNUINT count2;
            for (count2 = 0; count2 < numOfBB; count2++) {
                BasicBlockInfo* bb2 = basicBlocks[count2];
                if (bb2->startInst == inst) {
                    *(xanList_getSlotData(item)) = bb2;
                    count2 = numOfBB;
                }
            }
            assert(count2 == numOfBB + 1); // if count2 == numOfBB means that we have not found the basic block
            item = item->next;
        }
    }

    // call libdominance
    glob->root = calculateDominanceTree((void **) basicBlocks, numOfBB, rootBlockIdx, getPredecessor, getSuccessor);

    // and free some memory
    freeFunction(basicBlocks);

    // recreate the array purging the unreachable nodes
    JITNUINT     dst = 0; // destination array index
    XanList*     l   = glob->root->toPreOrderList(glob->root);
    glob->bbInfosLen = xanList_length(l);
    glob->bbInfos    = allocMemory(glob->bbInfosLen * sizeof(BasicBlockInfo*));
    while (xanList_length(l) > 0) {
        XanListItem* itm = xanList_first(l);
        glob->bbInfos[dst++] = xanList_getData(itm);
        xanList_deleteItem(l, itm);
    }
    assert(dst == glob->bbInfosLen);
    xanList_destroyList(l);

    // free up unused memory
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITFALSE
    for (count = 0; count < glob->bbInfosLen; count++) {
        xanList_destroyList(glob->bbInfos[count]->predList);
        glob->bbInfos[count]->predList	= NULL;
        xanList_destroyList(glob->bbInfos[count]->succList);
        glob->bbInfos[count]->succList	= NULL;
    }
#endif

    /* Free the memory.
     */
    xanList_destroyList(noPredecessorNodes);

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Dominance tree complete\n", IRMETHOD_getMethodName(glob->method));
    return ;
}
