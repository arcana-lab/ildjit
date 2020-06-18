/*
 * Copyright (C) 2008 - 2012 Simone Campanoni, Castiglioni William, Tartara Michele
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
#include <post_dominators.h>
#include <config.h>
// End

#define INFORMATIONS    "This step compute post-dominator tree"
#define AUTHOR          "Simone Campanoni, Castiglioni William, Sozzi Domenico, Tartara Michele"

static inline JITUINT64 get_ID_job (void);
static inline void do_job (ir_method_t *method);
static inline char * get_version (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline JITUINT64 get_dependences (void);
static inline void post_dom_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void post_dom_shutdown (JITFLOAT32 totalTime);
static inline JITUINT64 get_invalidations (void);
static inline void freePastData (ir_method_t *method);
static inline void post_dom_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void post_dom_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    post_dom_init,
    post_dom_shutdown,
    do_job,
    get_version,
    get_informations,
    get_author,
    get_invalidations,
    NULL,
    post_dom_getCompilationFlags,
    post_dom_getCompilationTime
};

static ir_lib_t        	*irLib = NULL;
static ir_optimizer_t  	*irOptimizer = NULL;
static ir_method_t     	*_method = NULL;
static IRBasicBlock	*fakeHead;   // fake heading node which reaches all exit points of the method
static XanList		*terminators;   // list of exit point of the method

static inline void post_dom_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    irOptimizer = optimizer;
    irLib = lib;
}

static inline void post_dom_shutdown (JITFLOAT32 totalTime) {
    irOptimizer = NULL;
    irLib = NULL;
    _method = NULL;
}

static inline JITUINT64 get_ID_job (void) {
    return POST_DOMINATOR_COMPUTER;
}

static inline JITUINT64 get_dependences (void) {
    return BASIC_BLOCK_IDENTIFIER;
}

IRBasicBlock * getPredecessor (IRBasicBlock *elem, IRBasicBlock *curr) {
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
    if (pred == NULL) {
        return NULL;
    }
    return IRMETHOD_getBasicBlockContainingInstruction(_method, pred);
}

IRBasicBlock * getSuccessor (IRBasicBlock *elem, IRBasicBlock *curr) {
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

int isExitPoint (IRBasicBlock* bb) {
    ir_instruction_t* endI = IRMETHOD_getInstructionAtPosition(_method, bb->endInst);

    if (endI->type == IREXITNODE) {
        return 1;
    }
    ir_instruction_t* succ = IRMETHOD_getSuccessorInstruction(_method, endI, NULL);
    if (succ == NULL || succ->type == IREXITNODE) {
        return 1;
    }
    return 0;
}

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

void printBB (void* bb) {
    if (bb == NULL) {
        printf("NULL\n");
        return;
    }
    if (bb == fakeHead) {
        printf("FAKE\n");
        return;
    }
    IRBasicBlock* block = (IRBasicBlock*) bb;
    printf("(%d-%d)\n", block->startInst, block->endInst);
}

void printInstr (void* in) {
    if (in == NULL) {
        printf("NULL\n");
        return;
    }
    printf("%d\n", ((ir_instruction_t*) in)->ID);
}

void printNode (XanNode* r, int dep) {
    if (r == NULL) {
        return;
    }
    XanNode* c;
    int i;
    for (i = 0; i < dep; i++) {
        printf("  ");
    }
    printBB(r->getData(r));
    c = r->getNextChildren(r, NULL);
    while (c != NULL) {
        printNode(c, dep + 1);
        c = r->getNextChildren(r, c);
    }
}

void printRBB (void* bb) {
    if (bb == NULL) {
        printf("NULL");
        return;
    }
    if (bb == fakeHead) {
        printf("FAKE");
        return;
    }
    IRBasicBlock* block = (IRBasicBlock*) bb;
    printf("(%d-%d)", block->startInst, block->endInst);
}

void printInstNoRet (ir_instruction_t* in) {
    if (in == NULL) {
        printf("NULL");
        return;
    }
    printf("%d", in->ID);
}

void printMethodInfos () {
    printf("Method has %d instructions and %d basic blocks\n", IRMETHOD_getInstructionsNumber(_method), IRMETHOD_getNumberOfMethodBasicBlocks(_method));
    JITUINT32 i = 0;
    for (i = 0; i < IRMETHOD_getInstructionsNumber(_method); i++) {
        ir_instruction_t* c = IRMETHOD_getInstructionAtPosition(_method, i);
        printf("PRED[");
        printInstNoRet(c);
        printf("] = ");
        ir_instruction_t* p = IRMETHOD_getPredecessorInstruction(_method, c, NULL);
        while (p != NULL) {
            printInstNoRet(p);
            printf(",");
            p = IRMETHOD_getPredecessorInstruction(_method, c, p);
        }
        printf("\n");
        printf("SUCC[");
        printInstNoRet(c);
        printf("] = ");
        ir_instruction_t* s = IRMETHOD_getSuccessorInstruction(_method, c, NULL);
        while (s != NULL) {
            printInstNoRet(s);
            printf(",");
            s = IRMETHOD_getSuccessorInstruction(_method, c, s);
        }
        printf("\n");
    }

    for (i = 0; i < IRMETHOD_getNumberOfMethodBasicBlocks(_method); i++) {
        IRBasicBlock* c = IRMETHOD_getBasicBlockAtPosition(_method, i);
        printRBB(c);
        printf("\n");
    }

}

void printDomTree (XanNode* r, int depth) {
    if (r == NULL) {
        return;
    }
    XanNode* c;
    int i;
    for (i = 0; i < depth; i++) {
        printf("  ");
    }
    printInstr(r->getData(r));
    c = r->getNextChildren(r, NULL);
    while (c != NULL) {
        printDomTree(c, depth + 1);
        c = r->getNextChildren(r, c);
    }
}

/**
 * @brief Compute postDominance Tree
 *
 * @param method IR method.
 */

static inline void do_job (ir_method_t *method) {
    JITUINT32 numElems;             //this var indicates number of blocks of IR method
    XanNode* root = NULL;
    XanNode* res = NULL;
    XanNode* r = NULL;
    IRBasicBlock** blocchi;
    JITUINT32 i;
    ir_instruction_t *inst;

    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);

    // Set global reference to method
    _method = method;

    // initialize some data
    numElems = IRMETHOD_getNumberOfMethodBasicBlocks(method);

    /* Free memory used in a previous call.
     */
    freePastData(method);

    // Check if there are instructions
    if (numElems == 0) {
        return;
    }

    /* Allocate the necessary memory (numElems+fakeHead)		*/
    blocchi 	= allocFunction((numElems + 1) * sizeof(IRBasicBlock*));

    // init fake head
    fakeHead 	= (IRBasicBlock*) allocFunction(sizeof(IRBasicBlock));

    terminators 	= xanList_new(allocFunction, freeFunction, NULL);

    // Create temporary array of pointers to BBs
    blocchi[0] = fakeHead;

    for (i = 1; i <= numElems; i++) {
        blocchi[i] = IRMETHOD_getBasicBlockAtPosition(method, i - 1);
        if (isExitPoint(blocchi[i])) {
            xanList_append(terminators, blocchi[i]);
        }
    }
    /* Sanity check */
    if (xanList_length(terminators) == 0) {
        fprintf(stderr, "Method %s has no terminators! This means it will loop forever. Something is wrong with it!", IRMETHOD_getMethodName(method));
        abort();
    }

    // Call libdominance with reversed successor/predecessor functions
    root = calculateDominanceTree((void **) blocchi, numElems + 1, 0, (void * (*)(void *, void*))getSuccessor, (void * (*)(void *, void*))getPredecessor);

    // Explode the tree from Basic blocks to instruction
    // In a block, each instruction immediately postdominates its successor
    // From one block to the other, if a BB A immediately postdominates a BB B then the first instruction of
    // A immediately postdominates the last instruction of B

    /* Prepare the starting node for recursion (the exit node) */
    res = xanNode_new(allocFunction, freeFunction, NULL);
    res->setData(res, method->exitNode);
    explode(root, res);

    /* Prepare the fakeHead node */
    r 		= xanNode_new(allocFunction, freeFunction, NULL);
    inst 		= method->exitNode;
    r->setData(r, inst);
    res->addChildren(res, r);       //The fakehead is a successor of exit in the post-dominance tree

    //save pointer as root of postDominatorTree
    method->postDominatorTree = res;

    /* Free the memory			*/
    freeFunction(blocchi);
    xanList_destroyList(terminators);
    root->destroyTree(root);
    freeFunction(fakeHead);

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

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void freePastData (ir_method_t *method) {
    if (method->postDominatorTree == NULL) {
        return ;
    }

    /* Destroy the tree.
     */
    (method->postDominatorTree)->destroyTree(method->postDominatorTree);
    method->postDominatorTree 	= NULL;

    return ;
}

static inline void post_dom_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void post_dom_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
