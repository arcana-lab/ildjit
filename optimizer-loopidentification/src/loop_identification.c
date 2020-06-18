/*
 * Copyright (C) 2008 - 2012  Campanoni Simone, Brambilla Marco, Massari Giuseppe
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
#include <ir_method.h>
#include <ir_optimization_interface.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>

// My headers
#include <loop_identification.h>
#include <config.h>
// End

#define INFORMATIONS    "This plugin computes the loop identification algorithm"
#define AUTHOR          "Simone Campanoni, Brambilla Marco, Massari Giuseppe"

static inline void loop_identification_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void loop_identification_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void loop_identification_start_execution (void);
static inline JITUINT64 get_ID_job (void);
static inline void loop_identification_do_job (ir_method_t * method);
static inline char *get_version (void);
static inline char *get_informations (void);
static inline char *get_author (void);
static inline JITUINT64 get_dependences (void);
static inline void loop_identification_init (ir_lib_t * lib, ir_optimizer_t * optimizer, char *outputPrefix);
static inline void loop_identification_shutdown (JITFLOAT32 totalTime);
static inline XanList * createBackEdgeList (ir_method_t * method, XanList **successors);
static inline XanList * createLoopList (ir_method_t * method, XanList * backEdges, XanList **successors);
static inline XanNode * createLoopNestTree (ir_method_t * method);
static inline circuit_t * t_loop_zero (void);
static inline JITNUINT isConnectedNoThroughHeader (ir_method_t *method, JITNUINT header, JITNUINT source, JITNUINT dest, XanList **successors, JITUINT8 *Nodi, JITUINT8 *bbsVisited, XanStack *waitingNodes, JITUINT32 NumberStatements, JITUINT32 basicBlocksNumber, XanHashTable *alreadyChecked);
static inline JITBOOLEAN TreeIsConnected (XanNode ** T_loop_nodes, JITUINT32 length, JITUINT32 startingPosition);
static inline XanList *dominateNodes (ir_method_t * method, JITNUINT instID);
static inline void freePastLoopStructure (ir_method_t * method);
static inline void print_bit_vector (JITNUINT * new_set, int n);
static inline JITUINT64 get_invalidations (void);
static inline JITBOOLEAN internal_add_children (XanList *loops, XanNode **T_loop_nodes, circuit_t *parent, circuit_t *child, JITNUINT *Levels);
static inline void internal_increase_level (XanNode *node, JITNUINT *Levels, JITNUINT level);
static inline void internal_print_tree (ir_method_t *method);
static inline void internal_write_edges (FILE *fileOut, XanNode *n, ir_method_t *method);

static ir_lib_t *irLib = NULL;

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    loop_identification_init,
    loop_identification_shutdown,
    loop_identification_do_job,
    get_version,
    get_informations,
    get_author,
    get_invalidations,
    loop_identification_start_execution,
    loop_identification_getCompilationFlags,
    loop_identification_getCompilationTime
};

static inline void loop_identification_start_execution (void) {
    return ;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void loop_identification_init (ir_lib_t * lib, ir_optimizer_t * optimizer, char *outputPrefix) {
    irLib = lib;
}

static inline void loop_identification_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
}

static inline JITUINT64 get_ID_job (void) {
    return LOOP_IDENTIFICATION;
}

static inline JITUINT64 get_dependences (void) {
    return PRE_DOMINATOR_COMPUTER;
}

/**
 * @brief Compute preDominance Tree
 *
 * @param method IR method.
 */
static inline void loop_identification_do_job (ir_method_t * method) {
    XanList         *backEdges;
    XanList         **successors;
    JITUINT32 	instructionsNumber;

    /* Assertions.
     */
    assert(method != NULL);
    PDEBUG("LOOP IDENTIFICATION: Start\n");

    /* Fetch the instructions number.
     */
    instructionsNumber 	= IRMETHOD_getInstructionsNumber(method);

    /* Compute the set of successors.
     */
    successors 		= IRMETHOD_getInstructionsSuccessors(method);
    assert(successors != NULL);

    /* Free the memory used in the previous analysis.
     */
    freePastLoopStructure(method);

    /* Compute the back edges.
     */
    PDEBUG("LOOP IDENTIFICATION:    Identify loops for %s\n", IRMETHOD_getSignatureInString(method));
    backEdges = createBackEdgeList(method, successors);
    assert(backEdges != NULL);

    /* Identify the loops.
     */
    method->loop = createLoopList(method, backEdges, successors);
    assert(method->loop != NULL);

    /* Print the loops.
     */
#ifdef PRINTDEBUG
    if (method->loop != NULL) {
        XanListItem *it;
        it = xanList_first(method->loop);
        while (it != NULL) {
            circuit_t *T;
            T = (circuit_t *) it->data;
            PDEBUG("ID = %u\n", T->loop_id);
            PDEBUG("HEADER_ID = %u\n", T->header_id);
            print_bit_vector(T->belong_inst, IRMETHOD_getInstructionsNumber(method));
            it = it->next;
        }
    }
#endif

    /* Compute the loop nesting tree.
     */
    PDEBUG("LOOP IDENTIFICATION:    Create the loop nesting tree\n");
    method->loopNestTree = createLoopNestTree(method);

    /* Print the loop nesting tree.
     */
#ifdef PRINTDEBUG
    internal_print_tree(method);
    XanList *WorkList = xanList_new(allocFunction, freeFunction, NULL);
    if (method->loopNestTree != NULL) {
        XanListItem     *it;
        XanNode *Node = NULL;
        XanNode *ChildNode = NULL;
        PDEBUG("LoopNestTree: \n");
        xanList_append(WorkList, method->loopNestTree);
        it = xanList_first(WorkList);
        while (it != NULL) {
            Node = (XanNode *) it->data;
            ChildNode = NULL;
            do {
                ChildNode = Node->getNextChildren(Node, ChildNode);
                if (ChildNode != NULL) {
                    PDEBUG("Parent loop %u <- Nested loop ID %u \n", ((circuit_t *) Node->data)->loop_id, ((circuit_t *) ChildNode->data)->loop_id);
                    xanList_append(WorkList, ChildNode);
                }
            } while (ChildNode != NULL);
            it = it->next;
        }
    }
    xanList_destroyList(WorkList);
#endif

    /* Free the memory.
     */
    xanList_destroyListAndData(backEdges);
    IRMETHOD_destroyInstructionsSuccessors(successors, instructionsNumber);

    PDEBUG("LOOP IDENTIFICATION: Exit\n");
    return;
}

static inline JITBOOLEAN TreeIsConnected (XanNode ** T_loop_nodes, JITUINT32 length, JITUINT32 startingPosition) {
    JITINT32 counter;
    XanList                 *WorkList;
    XanListItem             *it;
    XanNode                 *Node;
    XanNode                 *ChildNode;

    /* Initialize the variables	*/
    it = NULL;
    Node = NULL;
    ChildNode = NULL;
    counter = 0;

    /* Allocate the working list.
     */
    WorkList = xanList_new(allocFunction, freeFunction, NULL);

    /* Add the first element to the working list.
     */
    xanList_append(WorkList, T_loop_nodes[startingPosition]);

    /* Analyze the tree.
     */
    it = xanList_first(WorkList);
    while (it != NULL) {
        Node = (XanNode *) it->data;
        counter++;
        ChildNode = NULL;
        do {
            ChildNode = Node->getNextChildren(Node, ChildNode);
            if (ChildNode != NULL) {
                xanList_append(WorkList, ChildNode);
                assert(ChildNode->getParent(ChildNode) == Node);
            }
        } while (ChildNode != NULL);
        it = it->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(WorkList);

    /* Check if the tree is connected.
     * The counter can be bigger than length if the fake node has been inserted to have tree when multiple outermost loops exist.
     */
    return (counter >= length);
}

static inline char *get_version (void) {
    return VERSION;
}

static inline char *get_informations (void) {
    return INFORMATIONS;
}

static inline char *get_author (void) {
    return AUTHOR;
}

static inline circuit_t * t_loop_zero (void) {
    return (circuit_t *) allocFunction(sizeof(circuit_t));
}

static inline XanList * createBackEdgeList (ir_method_t * method, XanList **successors) {
    JITUINT8              *Nodi;
    XanList               *BackEdgeList;
    XanList               *WaitingNodes;
    XanListItem           *it;
    BackEdge              *Edge;
    ir_instruction_t      *inst;
    ir_instruction_t      *succ;
    JITUINT32 NumberStatements;
    JITUINT8              *instsVisited;

    /* Assertions                       */
    assert(method != NULL);
    assert(successors != NULL);

    /* Initialize the variables         */
    Nodi = NULL;
    BackEdgeList = NULL;
    WaitingNodes = NULL;
    it = NULL;
    Edge = NULL;
    inst = NULL;
    succ = NULL;

    //calcolo dei blocchi necessari per il bit-vector
    NumberStatements = IRMETHOD_getInstructionsNumber(method);

    /* Allocate the necessary memories		*/
    Nodi = (JITUINT8 *) allocFunction((NumberStatements + 1) * sizeof(JITUINT8));
    instsVisited = (JITUINT8 *) allocFunction((NumberStatements + 1) * sizeof(JITUINT8));

    /* Allocate the necessary lists                             */
    BackEdgeList = xanList_new(allocFunction, freeFunction, NULL);
    WaitingNodes = xanList_new(allocFunction, freeFunction, NULL);

    /* Insert the first instruction into the working list       */
    inst = IRMETHOD_getFirstInstruction(method);
    assert(inst != NULL);
    xanList_append(WaitingNodes, inst);
    instsVisited[inst->ID] = JITTRUE;

    /* Compute the back edges                                   */
    it = xanList_first(WaitingNodes);
    while (it != NULL) {
        XanListItem     *item2;

        /* Fetch the instruction				*/
        inst = (ir_instruction_t *) it->data;
        assert(inst != NULL);

        //ciclo che scandisce tutti i successori di un nodo
        item2 = xanList_first(successors[inst->ID]);
        while (item2 != NULL) {
            succ = item2->data;
            assert(succ != NULL);
            if (succ->type == IREXITNODE) {
                break;
            }
            if (    (succ->type == IRLABEL)                                         &&
                    (IRMETHOD_isInstructionAPredominator(method, succ, inst))       ){
                XanListItem     *bi;
                JITNUINT alreadyExist;
                alreadyExist = JITFALSE;
                Edge = (BackEdge *) allocMemory(sizeof(BackEdge));
                Edge->Source = inst->ID;
                Edge->Dest = succ->ID;

                //Scorri la lista dei backedge alla ricerca di un edge tra source e dest
                bi = xanList_first(BackEdgeList);
                while (bi != NULL) {
                    BackEdge *e;
                    e = (BackEdge *) bi->data;
                    assert(e != NULL);
                    if (    e->Source == Edge->Source
                            && e->Dest == Edge->Dest) {
                        alreadyExist = JITTRUE;
                        break;
                    }
                    bi = bi->next;
                }
                if (!alreadyExist) {
#ifdef DEBUG
                    ir_instruction_t        *header;
                    assert(Edge != NULL);
                    header = IRMETHOD_getInstructionAtPosition(method, Edge->Dest);
                    assert(header != NULL);
                    assert(IRMETHOD_getInstructionType(header) == IRLABEL);
#endif
                    xanList_append(BackEdgeList, Edge);
                } else {
                    freeMemory(Edge);
                }
            }

            /* Check if the current successor has to be processed   */
            if (    (Nodi[succ->ID] == 0)                           &&
                    (instsVisited[succ->ID] == JITFALSE)            ) {
                xanList_append(WaitingNodes, succ);
                instsVisited[succ->ID] = JITTRUE;
            }

            item2 = item2->next;
        }

        //metto a 1 la pos del vettore corrisp. al nodo che ho finito di esaminare
        Nodi[inst->ID] = 1;

        /* Delete the current instruction already processed     */
        assert(it->data == inst);
        assert(xanList_find(WaitingNodes, inst) != NULL);
        assert(xanList_equalsInstancesNumber(WaitingNodes, inst) == 1);
        xanList_deleteItem(WaitingNodes, it);

        /* Take the next instruction to process			*/
        it = xanList_first(WaitingNodes);
    }

    /* Free the memory          */
    assert(WaitingNodes != NULL);
    assert(xanList_length(WaitingNodes) == 0);
    xanList_destroyList(WaitingNodes);
    freeMemory(Nodi);
    freeMemory(instsVisited);

    /* the back edge list       */
    return BackEdgeList;
}

static inline XanList * createLoopList (ir_method_t * method, XanList * backEdges, XanList **successors) {
    JITNUINT            trigger;
    JITNUINT            temp;
    JITINT32            count;
    JITUINT32           NumberStatements;
    JITUINT32           BlocksNumber;
    JITUINT32		    basicBlocksNumber;
    JITNUINT            loopCounter;
    JITNUINT            *Nodi;
    JITUINT8            *statements;
    JITUINT8            *bbsVisited;
    XanStack            *waitingNodes;
    XanList             *loopList;
    XanList             *dominated;
    XanListItem         *d;
    XanHashTable        *alreadyChecked;
    ir_instruction_t    *d_instr;

    /* Assertions                   */
    assert(method != NULL);
    assert(backEdges != NULL);
    assert(successors != NULL);

    /* Initialize the variables     */
    Nodi = NULL;
    dominated = NULL;
    d = NULL;
    d_instr = NULL;
    loopCounter = 1;

    /* Allocate the list of loops   */
    loopList = xanList_new(allocFunction, freeFunction, NULL);
    assert(loopList != NULL);

    /* Compute the number of blocks         */
    NumberStatements = IRMETHOD_getInstructionsNumber(method);
    BlocksNumber = (NumberStatements / (sizeof(JITNINT) * 8)) + 1;
    assert(BlocksNumber > 0);

    /* Fetch the number of basic blocks.
     */
    basicBlocksNumber	= IRMETHOD_getNumberOfMethodBasicBlocks(method);

    /* Allocate the memory.
     */
    statements  	= (JITUINT8 *) allocFunction((NumberStatements + 1) * sizeof(JITNUINT));
    bbsVisited 		= (JITUINT8 *) allocFunction((basicBlocksNumber + 1) * sizeof(JITUINT8));
    waitingNodes    = xanStack_new(allocFunction, freeFunction, NULL);
    alreadyChecked  = xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the first back-edge		*/
    XanListItem *it = xanList_first(backEdges);
    while (it != NULL) {
        circuit_t          *loopStruct;
        BackEdge        *backEdge;
        XanHashTable	*visitedBBs;

        /* Fetch the back edge			*/
        backEdge = (BackEdge *) it->data;
        assert(backEdge != NULL);

        /* Allocate the loop structure          */
        loopStruct = (circuit_t *) allocMemory(sizeof(circuit_t));
        assert(loopStruct != NULL);

        /* Fill up the loop structure           */
        loopStruct->loop_id = loopCounter;
        loopCounter++;
        loopStruct->header_id = backEdge->Dest;
        (loopStruct->backEdge).src = IRMETHOD_getInstructionAtPosition(method, backEdge->Source);
        (loopStruct->backEdge).dst = IRMETHOD_getInstructionAtPosition(method, backEdge->Dest);
        assert(loopStruct->header_id < IRMETHOD_getInstructionsNumber(method));
        assert(IRMETHOD_getInstructionType((loopStruct->backEdge).dst) == IRLABEL);

        /* Allocate the blocks                  */
        Nodi = (JITNUINT *) allocFunction(BlocksNumber * sizeof(JITNUINT));

        /* Fetch the set of instructions        *
         * dominated by the loop		*/
        dominated = dominateNodes(method, loopStruct->header_id);
        assert(dominated != NULL);

        /* Fetch every nodes dominated by the header of the loop.
         */
        visitedBBs	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
        d 		= xanList_first(dominated);
        while (d != NULL) {
            IRBasicBlock	*bb;

            /* Fetch the instruction.
             */
            d_instr = (ir_instruction_t *) (d->data);
            assert(d_instr != NULL);
            bb	= IRMETHOD_getBasicBlockContainingInstruction(method, d_instr);
            assert(bb != NULL);
            if (xanHashTable_lookup(visitedBBs, bb) == NULL) {
                ir_instruction_t	*instInBB;

                /* Fetch the first instruction of the basic block.
                 */
                instInBB	= IRMETHOD_getFirstInstructionWithinBasicBlock(method, bb);
                assert(instInBB != NULL);

                /* Check if the basic block belongs to the current loop.
                 */
                if (    (instInBB->ID == loopStruct->header_id)                                                                	            ||
                        (instInBB->ID == backEdge->Source)                                                                    	            ||
                        (isConnectedNoThroughHeader(method, loopStruct->header_id, instInBB->ID, backEdge->Source, successors, statements, bbsVisited, waitingNodes, NumberStatements, basicBlocksNumber, alreadyChecked)) ){

                    /* Add the instructions that belong to the basic block as part of the current loop.
                     */
                    while (instInBB != NULL) {

                        /* Add the instruction.
                         */
                        trigger = (instInBB->ID) / (sizeof(JITNUINT) * 8);
                        assert(trigger < BlocksNumber);
                        temp = 1;
                        temp = temp << ((instInBB->ID) % (sizeof(JITNINT) * 8));
                        Nodi[trigger] = Nodi[trigger] | temp;

                        /* Fetch the next instruction of the basic block.
                         */
                        instInBB	= IRMETHOD_getNextInstructionWithinBasicBlock(method, bb, instInBB);
                    }
                }

                /* Tag the basic block as visited.
                 */
                xanHashTable_insert(visitedBBs, bb, bb);
            }

            /* Fetch the next instruction.
             */
            d = d->next;
        }
        xanHashTable_destroyTable(visitedBBs);
        assert(loopStruct != NULL);
        assert(Nodi != NULL);

        /* Set the nodes belongs to the *
         * current loop			*/
        loopStruct->belong_inst = Nodi;
        assert(loopStruct->belong_inst != NULL);

        /* Set the exit points		*/
        loopStruct->loopExits = xanList_new(allocFunction, freeFunction, NULL);
        for (count = 0; count < NumberStatements; count++) {
            ir_instruction_t        *inst;
            ir_instruction_t        *succ;
            temp = 1;
            temp = temp << (count % (sizeof(JITNUINT) * 8));
            trigger = count / (sizeof(JITNUINT) * 8);
            if ((loopStruct->belong_inst[trigger] & temp) != 0) {
                XanListItem     *item;
                inst = IRMETHOD_getInstructionAtPosition(method, count);
                assert(inst != NULL);
                item = xanList_first(successors[inst->ID]);
                while (item != NULL) {
                    succ = item->data;
                    assert(succ != NULL);
                    temp = 1;
                    temp = temp << (succ->ID % (sizeof(JITNUINT) * 8));
                    trigger = succ->ID / (sizeof(JITNUINT) * 8);
                    if ((loopStruct->belong_inst[trigger] & temp) == 0) {
                        xanList_insert(loopStruct->loopExits, inst);
                        break;
                    }
                    item = item->next;
                }
            }
        }

        /* Free the memory              */
        xanList_destroyList(dominated);

        /* Append the current loop	*/
        xanList_append(loopList, loopStruct);

        /* Fetch the next element       */
        it = it->next;
    }
    assert(loopList != NULL);

    /* Free the memory.
     */
    freeFunction(statements);
    freeFunction(bbsVisited);
    xanStack_destroyStack(waitingNodes);
    xanHashTable_destroyTableAndData(alreadyChecked);

    return loopList;
}

static inline XanNode * createLoopNestTree (ir_method_t * method) {
    JITNUINT        *BitVector;
    JITNUINT        *ActualBitVector;
    JITUINT32 	NumberStatements;
    JITUINT32 	BlocksNumber;
    JITINT32 	flag;
    JITINT32 	i;
    JITINT32 	length;
    JITNUINT        *Levels;
    circuit_t          *loopStruct;
    circuit_t          *loopStruct2;
    XanNode         **T_loop_nodes;
    XanNode         *root;
    JITBOOLEAN 	changed;
    JITNUINT 	Nesting;

    /* Assertions				*/
    assert(method != NULL);
    assert(method->loop != NULL);

    /* Initialize the variables		*/
    loopStruct = NULL;
    loopStruct2 = NULL;
    T_loop_nodes = NULL;
    ActualBitVector = NULL;
    BitVector = NULL;
    i = 1;

    /* Check if we have loops		*/
    if (xanList_length(method->loop) == 0) {

        /* There is no loop			*/
        return NULL;
    }

    /* Fetch the number of loops		*/
    length = xanList_length(method->loop);
    assert(length > 0);

    /* Allocate the nesting levels		*/
    Levels = (JITNUINT *) allocMemory((length + 1) * (sizeof(JITNUINT)));

    /* Allocate the set of trees		*/
    T_loop_nodes = (XanNode **) allocMemory((length + 1) * sizeof(XanNode *));

    /* Fetch the number of statements of    *
     * the method				*/
    NumberStatements = IRMETHOD_getInstructionsNumber(method);
    BlocksNumber = (NumberStatements / (sizeof(JITNINT) * 8)) + 1;

    /* Allocate every necessary trees	*/
    for (i = 0; i < (length + 1); i++) {
        T_loop_nodes[i] = xanNode_new(allocFunction, freeFunction, NULL);
    }

    /* Fill up the necessary trees		*/
    T_loop_nodes[0]->data = t_loop_zero();
    XanListItem *it = xanList_first((method->loop));
    while (it != NULL) {
        circuit_t  *currentLoop;

        /* Fetch the loop			*/
        currentLoop = (circuit_t *) it->data;
        assert(currentLoop != NULL);
        assert(currentLoop->loop_id > 0);
        assert(currentLoop->loop_id < (length + 1));
        assert(T_loop_nodes[currentLoop->loop_id]->data == NULL);

        /* Append the loop to the tree		*/
        T_loop_nodes[currentLoop->loop_id]->data = currentLoop;

        /* Fetch the next element from the list */
        it = it->next;
    }

    /* Compute the loop nest tree		*/
    Nesting = JITFALSE;
    changed = JITTRUE;
    while (changed) {
        changed = JITFALSE;
        it = xanList_first(method->loop);
        while (it != NULL) {
            XanListItem     *it2;

            /* Fetch the loop.
             */
            loopStruct = (circuit_t *) (it->data);
            assert(loopStruct != NULL);
            BitVector = loopStruct->belong_inst;

            /* Looking for nested loops.
             */
            it2 = it->next;
            while (it2 != NULL) {

                /* Fetch the next loop.
                 */
                assert(it2 != it);
                loopStruct2 = (circuit_t *) (it2->data);
                assert(loopStruct2 != NULL);
                ActualBitVector = loopStruct2->belong_inst;
                assert(ActualBitVector != NULL);

                /* Check whether the current two loops share any instruction.
                 */
                flag = JITFALSE;
                if (loopStruct->header_id != loopStruct2->header_id) {
                    for (i = 0; i < BlocksNumber; i++) {
                        if ((BitVector[i] & ActualBitVector[i]) != 0) {
                            flag = JITTRUE;
                            break;
                        }
                    }
                }
                if (flag) {
                    JITBOOLEAN loopStructIsBigger;
                    JITBOOLEAN loopStructIsSmaller;

                    /* There is one instruction in common at least.
                     * Check the direction of the nesting relation.
                     * We consider subloops to be fully included within their parents.
                     */
                    assert(BlocksNumber > 0);
                    assert(T_loop_nodes[loopStruct->loop_id]->find(T_loop_nodes[loopStruct->loop_id], T_loop_nodes[loopStruct2->loop_id]) == NULL);
                    assert(T_loop_nodes[loopStruct2->loop_id]->find(T_loop_nodes[loopStruct2->loop_id], T_loop_nodes[loopStruct->loop_id]) == NULL);

                    /* Check if loopStruct fully contains loopStruct2.
                     */
                    loopStructIsSmaller 	= JITFALSE;
                    loopStructIsBigger 	= JITTRUE;
                    for (i = 0; i < BlocksNumber; i++) {
                        if ((loopStruct->belong_inst[i] & loopStruct2->belong_inst[i]) != loopStruct2->belong_inst[i]) {

                            /* There is at least one instruction that both it belongs to loopStruct2 and it does not belong to loopStruct.
                             * Therefore, the loop loopStruct does not fully contain loopStruct2.
                             */
                            loopStructIsBigger = JITFALSE;
                            break;
                        }
                    }
                    if (!loopStructIsBigger) {

                        /* Check if loopStruct2 fully contains loopStruct.
                         */
                        loopStructIsSmaller 	= JITTRUE;
                        for (i = 0; i < BlocksNumber; i++) {
                            if ((loopStruct->belong_inst[i] & loopStruct2->belong_inst[i]) != loopStruct->belong_inst[i]) {

                                /* There is at least one instruction that both it belongs to loopStruct and it does not belong to loopStruct2.
                                 * Therefore, the loop loopStruct does not fully contain loopStruct2.
                                 */
                                loopStructIsSmaller 	= JITFALSE;
                                break;
                            }
                        }
                    }

                    /* Add the nesting relation.
                     */
                    if (loopStructIsBigger) {
                        assert(!loopStructIsSmaller);
                        changed |= internal_add_children(method->loop, T_loop_nodes, loopStruct, loopStruct2, Levels);
                        Nesting = JITTRUE;

                    } else if (loopStructIsSmaller) {
                        assert(!loopStructIsBigger);
                        changed |= internal_add_children(method->loop, T_loop_nodes, loopStruct2, loopStruct, Levels);
                        Nesting = JITTRUE;
                    }
                }
                it2 = it2->next;
            }
            it = it->next;
        }
    }
    if (!Nesting) {

        /* Free the memory.
         */
        freeMemory(T_loop_nodes[0]->data);
        for (i = 0; i < length + 1; i++) {
            T_loop_nodes[i]->destroyNode(T_loop_nodes[i]);
        }
        freeMemory(Levels);
        freeMemory(T_loop_nodes);

        return NULL;
    }

    /* Check if from loop 1 we can  *
     * reach every other nodes	*/
    if (TreeIsConnected(T_loop_nodes, length, 1)) {

        /* Fetch the root		*/
        root = T_loop_nodes[1];
        assert(root != NULL);

        /* Free the memory		*/
        freeMemory(T_loop_nodes[0]->data);
        T_loop_nodes[0]->destroyNode(T_loop_nodes[0]);
        freeMemory(Levels);
        freeMemory(T_loop_nodes);

        /* Return			*/
        return root;
    }
    for (i = 1; i < (length + 1); i++) {
        if (Levels[i] == 0) {
            assert(T_loop_nodes[0]->find(T_loop_nodes[0], T_loop_nodes[i]) == NULL);
            assert(T_loop_nodes[i]->find(T_loop_nodes[i], T_loop_nodes[0]) == NULL);
            T_loop_nodes[0]->addChildren(T_loop_nodes[0], T_loop_nodes[i]);
        }
    }
    assert(TreeIsConnected(T_loop_nodes, length, 0));

    /* Fetch the root		*/
    root = T_loop_nodes[0];
    assert(root != NULL);

    /* Free the memory		*/
    freeMemory(Levels);
    freeMemory(T_loop_nodes);

    /* Return			*/
    return root;
}

static inline void print_bit_vector (JITNUINT * new_set, int n) {
    int i;
    JITNUINT temp, block;

    for (i = n - 1; i >= 0; i--) {
        temp = 1;
        temp = temp << (i % (sizeof(JITNUINT) * 8));
        block = i / (sizeof(JITNUINT) * 8);
        if ((new_set[block] | temp) == new_set[block]) {
            PDEBUG("Instruction %d\n", i);
        }
    }
    PDEBUG("\n");
}

//controlla se due nodi sono connessi senza back-edges cioè
// SE ESISTE UN PERCORSO TRA SOURCE E DEST SENZA PASSARE PER HEADER
static inline JITNUINT isConnectedNoThroughHeader (ir_method_t *method, JITNUINT header, JITNUINT source, JITNUINT dest, XanList **successors, JITUINT8 *Nodi, JITUINT8 *bbsVisited, XanStack *waitingNodes, JITUINT32 NumberStatements, JITUINT32 basicBlocksNumber, XanHashTable *alreadyChecked){
    IRBasicBlock		*sourceBB;
    JITBOOLEAN          *sourcesAlreadyChecked;

    /* Assertions.
     */
    assert(method != NULL);
    assert(successors != NULL);
    assert(Nodi != NULL);
    assert(bbsVisited != NULL);
    assert(alreadyChecked != NULL);

    /* Check trivial cases.
     */
    if (source == header) {
        return JITFALSE;
    }
    if (source == dest) {
        return JITTRUE;
    }

    /* Fetch the source instruction.
     */
    sourceBB	= IRMETHOD_getBasicBlockContainingInstructionPosition(method, source);

    /* Check whether the destination belongs to the current basic block.
     * Notice that the header cannot belong to the current basic block as it must start at the beginning of a basic block and this condition has been already checked before.
     */
    if (	(dest >= sourceBB->startInst) 	&&
            (dest <= sourceBB->endInst)	) {
        return JITTRUE;
    }

    /* Check whether the source has been already checked.
     */
    sourcesAlreadyChecked   = xanHashTable_lookup(alreadyChecked, (void *)(dest + 1));
    if (    (sourcesAlreadyChecked != NULL)         &&
            (sourcesAlreadyChecked[sourceBB->pos])  ){
        return JITFALSE;
    }

    /* Reset the memory.
     */
    memset(Nodi, 0, (NumberStatements + 1) * sizeof(JITNUINT));
    memset(bbsVisited, 0, (basicBlocksNumber + 1) * sizeof(JITUINT8));
    while (xanStack_getSize(waitingNodes) > 0){
        xanStack_pop(waitingNodes);
    }

    /* Walking through the CFG.
     */
    bbsVisited[sourceBB->pos] = JITTRUE;
    xanStack_push(waitingNodes, sourceBB);
    while (xanStack_getSize(waitingNodes) > 0) {
        XanListItem    	 	*item;
        IRBasicBlock		*bb;
        ir_instruction_t        *inst;
        ir_instruction_t        *succ;

        /* Fetch the basic block to analyze.
         */
        bb 	= (IRBasicBlock *) xanStack_pop(waitingNodes);
        assert(bb != NULL);

        /* Fetch the last instruction of the basic block.
         */
        inst	= IRMETHOD_getLastInstructionWithinBasicBlock(method, bb);
        assert(inst != NULL);

        /* Check every successor of the current basic block.
         */
        item = xanList_first(successors[inst->ID]);
        while (item != NULL) {

            /* Fetch the successor.
             */
            succ 	= item->data;
            assert(succ != NULL);

            /* Check the successor.
             */
            if (succ->ID != header) {
                IRBasicBlock	*succBB;

                /* Fetch the successor basic block.
                 */
                succBB	= IRMETHOD_getBasicBlockContainingInstruction(method, succ);
                assert(succBB != NULL);
                assert(succ->ID == succBB->startInst);

                /* The header cannot belong to the current successor basic block as it must start at the beginning of a basic block.
                 */
                assert((header < succBB->startInst) || (header > succBB->endInst));

                /* Check the successor basic block.
                 */
                if (	(dest >= succBB->startInst) 	&&
                        (dest <= succBB->endInst)	) {
                    return JITTRUE;
                }

                /* Add the successor to the working list if necessary.
                 */
                if (    (Nodi[succBB->pos] == 0)                                &&
                        (bbsVisited[succBB->pos] == JITFALSE)            	) {

                    /* Add the successor.
                     */
                    xanStack_push(waitingNodes, succBB);
                    bbsVisited[succBB->pos] 	= JITTRUE;
                }
            }

            /* Fetch the next element from the list.
             */
            item = item->next;
        }

        /* Set the current basic block as analyzed.
         */
        Nodi[bb->pos] = 1;
    }

    /* Cache the information.
     */
    if (sourcesAlreadyChecked == NULL){
        sourcesAlreadyChecked   = allocFunction(sizeof(JITBOOLEAN) * (basicBlocksNumber + 1));
        xanHashTable_insert(alreadyChecked, (void *)(dest + 1), sourcesAlreadyChecked);
    }
    for (JITUINT32 count=0; count < basicBlocksNumber; count++){
        sourcesAlreadyChecked[count]    |= bbsVisited[count];
    }

    return JITFALSE;
}

/**
 * @brief return a list of ir_instruction_t belongs to the subtree in the preDominatorTree from instID instruction,
 * @param method IR method
 * @param instID dominator instruction
 */
static inline XanList *dominateNodes (ir_method_t * method, JITNUINT instID) {
    XanNode         *child;
    XanListItem     *it;
    ir_instruction_t *inst;
    ir_instruction_t *instruction2;
    XanNode *n;
    XanNode *node2;
    XanNode *root;

    XanList *list = xanList_new(allocFunction, freeFunction, NULL);          //lista per deep search fino a inst
    XanList *list2 = xanList_new(allocFunction, freeFunction, NULL);         //lista per deep search nel sottoalbero
    XanList *ret_list = xanList_new(allocFunction, freeFunction, NULL);

    root = method->preDominatorTree;

    xanList_append(list, root);
    it = xanList_first(list);   //search for node of instruction instID
    while (it != NULL) {

        /* Fetch the node.
         */
        n = (XanNode *) it->data;
        inst = (ir_instruction_t *) n->data;

        if (inst->ID == instID) { //se lo trovi svuota list2
            xanList_append(list2, n);
            do {
                it = xanList_first(list2);
                if (it != NULL) {
                    node2 = (XanNode *) it->data;
                    instruction2 = (ir_instruction_t *) node2->data;
                    child = NULL;
                    do {
                        child = node2->getNextChildren(node2, child);   //for each child of node node2
                        if (child != NULL) {
                            xanList_append(list2, child);            //add this node to list2
                        }
                    } while (child != NULL);
                    xanList_append(ret_list, instruction2); //save instruction visited into ret_list
                    xanList_deleteItem(list2, it);
                }
            } while (it != NULL);

            /* Free the memory.
             */
            xanList_destroyList(list);
            xanList_destroyList(list2);

            return ret_list;
        }

        //naviga i successori
        child = NULL;
        do {
            child = n->getNextChildren(n, child);   //for each child of node node2
            if (child != NULL) {
                xanList_append(list, child);      //add this node to list2
            }
        } while (child != NULL);

        /* Fetch the next element.
         */
        it = it->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(list);
    xanList_destroyList(list2);

    return ret_list;
}

static inline void freePastLoopStructure (ir_method_t * method) {
    IRMETHOD_deleteCircuitsInformation(method);
}

static inline JITBOOLEAN internal_add_children (XanList *loops, XanNode **T_loop_nodes, circuit_t *parent, circuit_t *child, JITNUINT *Levels) {
    XanList         *l;
    XanListItem     *item;
    JITBOOLEAN alreadyInserted;

    /* Assertions			*/
    assert(loops != NULL);
    assert(T_loop_nodes != NULL);
    assert(parent != NULL);
    assert(child != NULL);
    assert(Levels != NULL);
    PDEBUG("LOOP IDENTIFICATION:            Check if we need to add the nesting relation: Parent loop L%d (H=%u) <- Nested loop L%d (H=%u)\n", parent->loop_id, parent->header_id, child->loop_id, child->header_id);

    /* Initialize the variables	*/
    alreadyInserted = JITFALSE;

    /* Fetch the list of loops with	*
     * the same header of the parent*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(loops);
    while (item != NULL) {
        circuit_t  *loop;
        loop = item->data;
        assert(loop != NULL);
        if (loop->header_id == parent->header_id) {
            xanList_insert(l, loop);
        }
        item = item->next;
    }
    assert(xanList_length(l) > 0);

    /* Check if there is on loop at *
     * least with the same header of*
     * the parent that contains     *
     * already the child inside its	*
     * tree				*/
    item = xanList_first(l);
    while (item != NULL) {
        circuit_t  *loop;
        loop = item->data;
        assert(loop != NULL);
        if (T_loop_nodes[loop->loop_id]->find(T_loop_nodes[loop->loop_id], child) != NULL) {
            alreadyInserted = JITTRUE;
            break;
        }
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(l);

    if (alreadyInserted) {

        /* The child is already part of	*
         * the tree starting from the	*
         * parent			*/
        return JITFALSE;
    }

    /* Add the children		*/
    PDEBUG("LOOP IDENTIFICATION:                    Relation added\n");
    T_loop_nodes[parent->loop_id]->addChildren(T_loop_nodes[parent->loop_id], T_loop_nodes[child->loop_id]);

    /* Update the level of the new	*
     * child			*/
    internal_increase_level(T_loop_nodes[child->loop_id], Levels, Levels[parent->loop_id] + 1);
    assert(T_loop_nodes[parent->loop_id]->find(T_loop_nodes[parent->loop_id], child) != NULL);
    assert(T_loop_nodes[parent->loop_id]->find(T_loop_nodes[parent->loop_id], child) != NULL);

    /* Return			*/
    return JITTRUE;
}

static inline void internal_increase_level (XanNode *node, JITNUINT *Levels, JITNUINT level) {
    XanNode         *child;
    circuit_t          *loop;

    /* Assertions			*/
    assert(node != NULL);
    assert(Levels != NULL);

    /* Increase the root		*/
    loop = node->data;
    assert(loop != NULL);
    Levels[loop->loop_id] = level;

    /* Increase the childs		*/
    child = node->getNextChildren(node, NULL);
    while (child != NULL) {
        internal_increase_level(child, Levels, level + 1);
        child = node->getNextChildren(node, child);
    }

    /* Return			*/
    return;
}

static inline void loop_identification_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void loop_identification_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}

static inline void internal_print_tree (ir_method_t *method) {
    XanListItem	*item;
    JITUINT32	count;
    FILE		*fileOut;
    char		buf[DIM_BUF];

    /* Check if there is any tree.
     */
    if (method->loopNestTree == NULL) {
        return ;
    }

    /* Open the file.
     */
    fileOut	= fopen("loopNestingTree.dot", "a");
    if (fileOut == NULL) {
        return ;
    }

    /* Write the header.
     */
    fprintf(fileOut, "digraph \"%s\" {\n", IRMETHOD_getSignatureInString(method));
    fprintf(fileOut, "label=\"Loop nesting tree\"\n");

    /* Write the nodes of the tree.
     */
    count	= 0;
    item	= xanList_first(method->loop);
    while (item != NULL) {
        circuit_t	*l;
        l	= item->data;
        assert(l != NULL);
        snprintf(buf, DIM_BUF, "node [ fontsize=12 label=\"%u\" shape=box] ;\n", l->header_id);
        fprintf(fileOut, "%s\n", buf);
        fprintf(fileOut, "n%u ;\n", count);
        count++;
        item	= item->next;
    }

    /* Write the edges of the tree.
     */
    internal_write_edges(fileOut, method->loopNestTree, method);

    /* Dump the footer.
     */
    fprintf(fileOut, "\n}\n");

    /* Close the file.
     */
    fclose(fileOut);

    return ;
}

static inline void internal_write_edges (FILE *fileOut, XanNode *n, ir_method_t *method) {
    XanNode		*c;
    circuit_t		*parent;
    JITUINT32	parentID;
    JITBOOLEAN	parentExist;

    /* Assertions.
     */
    assert(fileOut != NULL);
    assert(method != NULL);

    /* Check the node.
     */
    if (n == NULL) {
        return ;
    }

    /* Fetch the parent.
     */
    parent		= n->getData(n);
    parentExist	= (xanList_find(method->loop, parent) != NULL);
    parentID	= xanList_getPositionNumberFromElement(method->loop, parent);

    /* Write the edges.
     */
    c		= n->getNextChildren(n, NULL);
    while (c != NULL) {
        circuit_t		*child;
        JITUINT32	childID;

        /* Fetch the child.
         */
        child		= c->getData(c);
        if (	(parentExist)					&&
                (xanList_find(method->loop, child) != NULL)	) {
            childID		= xanList_getPositionNumberFromElement(method->loop, child);
            assert(childID < xanList_length(method->loop));

            /* Write the edge.
             */
            fprintf(fileOut, "n%u -> n%u ; \n", childID, parentID);
        }

        /* Go down to the tree.
         */
        internal_write_edges(fileOut, c, method);

        c	= n->getNextChildren(n, c);
    }

    return ;
}
