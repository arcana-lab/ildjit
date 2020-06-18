/*
 * Copyright (C) 2008 Simone Campanoni, Castiglioni William
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
#include <pre_dominators.h>
#include <config.h>
// End

#define INFORMATIONS    "This step compute pre-dominator tree"
#define AUTHOR          "Simone Campanoni, Castiglioni William, Sozzi Domenico"

static inline JITUINT64 get_ID_job (void);
static inline void do_job (ir_method_t *method);
static inline char * get_version (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline JITUINT64 get_dependences (void);
static inline void pre_dom_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void pre_dom_shutdown (JITFLOAT32 totalTime);
static inline JITBOOLEAN internal_check_tree (ir_method_t *method, XanNode *tree);
static inline JITUINT64 get_invalidations (void);
static inline void pre_dom_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void pre_dom_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    pre_dom_init,
    pre_dom_shutdown,
    do_job,
    get_version,
    get_informations,
    get_author,
    get_invalidations,
    NULL,
    pre_dom_getCompilationFlags,
    pre_dom_getCompilationTime
};

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
ir_method_t     *_method = NULL;

static inline void pre_dom_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    irOptimizer = optimizer;
    irLib = lib;

    PDEBUG("INIT PREDOM\n");
}

static inline void pre_dom_shutdown (JITFLOAT32 totalTime) {
    irOptimizer = NULL;
    irLib = NULL;
    _method = NULL;

    PDEBUG("SHUTDOWN PREDOM\n");
}

static inline JITUINT64 get_ID_job (void) {
    return PRE_DOMINATOR_COMPUTER;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline JITUINT64 get_dependences (void) {
    return BASIC_BLOCK_IDENTIFIER;
}

static inline JITBOOLEAN internal_check_tree (ir_method_t *method, XanNode *tree) {
    JITUINT32 i;
    ir_instruction_t        *inst;

    /* Assertions			*/
    assert(method != NULL);
    assert(tree != NULL);

    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); i++) {
        inst = IRMETHOD_getInstructionAtPosition(method, i);
        assert(inst != NULL);
        if (inst->type == IREXITNODE) {
            continue;
        }
        if (tree->find(tree, inst) == NULL) {
            assert(0==1);
            return JITFALSE;
        }
    }

    return JITTRUE;
}

/**
 * @brief returns the predecessor of the element passed as first parameter
 * that is after the element passed as second parameter, or NULL if elem has no more precessors
 *
 * @note	the predecessor of a Basic Block A is a basic block B whose last instruction is a predecessor
 *              of A's first instrucion
 */
void* getPredecessor (void* elem, void* curr) {
    if (elem == NULL) {
        return NULL;
    }

    IRBasicBlock* bb = (IRBasicBlock*) elem;
    IRBasicBlock* curr_bb = (IRBasicBlock*) curr;
    ir_instruction_t* bb_head = IRMETHOD_getInstructionAtPosition(_method, bb->startInst);

    ir_instruction_t* curr_bb_tail = NULL;
    ir_instruction_t* pred;
    if (curr != NULL) {
        curr_bb_tail = IRMETHOD_getInstructionAtPosition(_method, curr_bb->endInst);
    }

    pred = IRMETHOD_getPredecessorInstruction(_method, bb_head, curr_bb_tail);
    if (pred == NULL) {
        return NULL;
    }
    return IRMETHOD_getBasicBlockContainingInstruction(_method, pred);
}


/**
 * @brief returns the successor of the element passed as first parameter
 * that is after the element passed as second parameter, or NULL if elem has no more successors
 *
 * @note	the successor of a Basic Block A is a basic block B whose first instruction is a successor
 *              of A's last instrucion
 */
void* getSuccessor (void* elem, void* curr) {

    if (elem == NULL) {
        if (curr == NULL) {
            ir_instruction_t* firstInst = IRMETHOD_getInstructionAtPosition(_method, 0);
            IRBasicBlock* firstBB = IRMETHOD_getBasicBlockContainingInstruction(_method, firstInst);
            return firstBB;
        } else {
            return NULL;
        }
    }

    IRBasicBlock* bb = (IRBasicBlock*) elem;
    IRBasicBlock* curr_bb = (IRBasicBlock*) curr;
    assert(bb != NULL);
    ir_instruction_t* bb_tail = IRMETHOD_getInstructionAtPosition(_method, bb->endInst);
    if (bb_tail == NULL) {
        return NULL;
    }
    ir_instruction_t* curr_bb_head = NULL;
    ir_instruction_t* succ;

    if (curr != NULL) {
        assert(curr_bb != NULL);
        curr_bb_head = IRMETHOD_getInstructionAtPosition(_method, curr_bb->startInst);
    }

    succ = IRMETHOD_getSuccessorInstruction(_method, bb_tail, curr_bb_head);

    if (succ == NULL) {
        return NULL;
    }

    IRBasicBlock* temp = IRMETHOD_getBasicBlockContainingInstruction(_method, succ);
    return temp;
}

/**
 * @brief recursive function that translate dominance relation between blocks to dominance
 * between instructions of that block
 */
static inline void explode (XanNode* bb_dominance, XanNode* currNode) {
    IRBasicBlock* block;
    ir_instruction_t* temp;
    XanNode* tempNode;
    XanNode* child = NULL;
    JITINT32 i;

    if (bb_dominance->getData(bb_dominance) != NULL) {
        block = (IRBasicBlock*) bb_dominance->getData(bb_dominance);
    } else {
        block = NULL;
    }

    if (block != NULL) {
        for (i = block->startInst; i <= block->endInst; i++) {
            tempNode = xanNode_new(allocFunction, freeFunction, NULL);
            temp = IRMETHOD_getInstructionAtPosition(_method, i);
            tempNode->setData(tempNode, temp);
            currNode->addChildren(currNode, tempNode);
            currNode = tempNode;
        }
    }

    // Now currNode it's the last instruction of the method node
    // Start recursion
    child = bb_dominance->getNextChildren(bb_dominance, child);
    while (child != NULL) {
        explode(child, currNode);
        child = bb_dominance->getNextChildren(bb_dominance, child);
    }

    // When all childs have been exploded returns;
    return;
}

/**
 * @brief Recursively calculate the number of elements that are under
 * the node passed as parameter in the relative graph
 */
int count (XanNode* r) {
    XanNode* child = NULL;
    int temp = 1;

    child = r->getNextChildren(r, NULL);
    while (child != NULL) {
        temp += count(child);
        child = r->getNextChildren(r, child);
    }
    return temp;
}

/**
 * @brief Prints the first and the last instruction ID of the block passed as parameter
 */
void printBB (void* bb) {
    if (bb == NULL) {
        printf("NULL\n");
        return;
    }
    IRBasicBlock* block = (IRBasicBlock*) bb;
    printf("(%d-%d)\n", block->startInst, block->endInst);
}

/**
 * @brief Recursively call printBB on each node
 */
void printNode (XanNode* r, int depth) {
    if (r == NULL) {
        return;
    }
    XanNode* c;
    int i;
    for (i = 0; i < depth; i++) {
        printf("  ");
    }
    printBB(r->getData(r));
    c = r->getNextChildren(r, NULL);
    while (c != NULL) {
        printNode(c, depth + 1);
        c = r->getNextChildren(r, c);
    }
}

/**
 * @brief Compute preDominance Tree
 *
 * @param method IR method.
 */
static inline void do_job (ir_method_t *method) {
    JITUINT32 	numElems;             // this var indicates number of blocks of IR method
    XanNode         *root = NULL;  // root of idominance tree of the blocks
    XanNode*        res = NULL;     // root of idominance tree of the instructions
    IRBasicBlock    **blocchi;
    JITINT32 	i;

    // Set global reference to method
    _method = method;

    /* Fetch the number of elements			*/
    numElems = IRMETHOD_getNumberOfMethodBasicBlocks(method) + 1;
    PDEBUG("PREDOM: The method %s has %d blocks and %d instructions\n", IRMETHOD_getMethodName(method), numElems, IRMETHOD_getInstructionsNumber(method));

    //if preDominatorTree is not NULL free memory before recomputing preDominanceTree
    if (method->preDominatorTree != NULL) {
        (method->preDominatorTree)->destroyTree(method->preDominatorTree);
        method->preDominatorTree = NULL;
    }

    // Check if there are instructions
    if (numElems == 0) {
        return;
    }

    /* Allocate the necessary memory		*/
    blocchi = allocMemory(numElems * sizeof(IRBasicBlock*));

    // In this case the starting node has a predecessor, so we must build a fake head node
    if (IRMETHOD_getPredecessorInstruction(method, IRMETHOD_getInstructionAtPosition(_method, 0), NULL) != NULL && 0) {
        numElems = numElems + 1;
        blocchi = reallocMemory(blocchi, numElems * sizeof(IRBasicBlock*));
        blocchi[0] = NULL;
        for (i = 1; i < numElems; i++) {
            blocchi[i] = IRMETHOD_getBasicBlockAtPosition(method, i - 1);
        }

    } else {

        // Crate temporary array of pointers to BBs
        for (i = 0; i < numElems; i++) {
            blocchi[i] = IRMETHOD_getBasicBlockAtPosition(method, i);
        }
    }

    // Call libdominance
    root = calculateDominanceTree((void **) blocchi, numElems, 0, getPredecessor, getSuccessor);
    //printNode(root, 0);

    // Explode the tree from Basic blocks to instruction
    // In a block, each instruction idominates its successor
    // From one block to the other, if a BB A idominates a BB B then the last instruction of
    // A idominates the first instruction of B
    res = xanNode_new(allocFunction, freeFunction, NULL);                    // starting node  for recursion
    explode(root, res);

    // remove starting node (that's empty, it's just the starting point for recursion)
    method->preDominatorTree = res->getNextChildren(res, NULL);
    PDEBUG("PREDOM: The calculate trees have %d node for blocks and %d nodes for instructions\n", count(root), count(res));

    /* Check the tree		*/
#ifdef DEBUG
    assert(internal_check_tree(method, method->preDominatorTree));
#endif

    /* Free the memory		*/
    freeMemory(blocchi);
    res->destroyNode(res);
    root->destroyTree(root);

    /* Return			*/
    return;
}

static inline char * get_version (void) {
    return VERSION;
}

static inline char * get_informations (void) {
    return INFORMATIONS;
}

static inline char * get_author (void) {
    return AUTHOR;
}

static inline void pre_dom_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void pre_dom_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
