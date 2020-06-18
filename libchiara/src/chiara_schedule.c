/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#include <stdlib.h>
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>
#include <dominance.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
#include <chiara_tools.h>
// End

/**
 * Macros for debug printing.
 **/
#undef PDEBUG
/* #define TIMDEBUG */
#if defined(PRINTDEBUG) || defined(TIMDEBUG)
static JITINT32 debugLevel = -1;
#define PDEBUGGING
#define PDEBUG(lvl, fmt, args...) do { if (lvl <= debugLevel) fprintf(stderr, "CHIARA_SCHED: " fmt, ## args); } while (0)
#define PDEBUGC(lvl, fmt, args...) do { if (lvl <= debugLevel) fprintf(stderr, fmt, ## args); } while (0)
#define PDEBUGNL(lvl, fmt, args...) do { if (lvl <= debugLevel) fprintf(stderr, "CHIARA_SCHED:\nCHIARA_SCHED: " fmt, ##args); } while (0)
#define PDEBUGSTMT(lvl, stmt) do { if (lvl <= debugLevel) stmt; } while (0)
#define PDEBUGLEVEL(lvl) do { debugLevel = lvl; } while (0)
#define PDECL(decl) decl
#else
#define PDEBUG(lvl, fmt, args...)
#define PDEBUGC(lvl, fmt, args...)
#define PDEBUGNL(lvl, fmt, args...)
#define PDEBUGSTMT(lvl, stmt)
#define PDEBUGLEVEL(lvl)
#define PDECL(decl)
/* #define PDEBUG(fmt, args...) */
/* #define PDEBUGC(fmt, args...) */
/* #define PDEBUGNL(fmt, args...) */
/* #define PDECL(decl) */
#endif

/**
 * Mark a variable as possibly unused to gcc.
 **/
#define JITUNUSED __attribute__ ((unused))

/**
 * The direction of moving instructions in the CFG.
 **/
typedef enum { Upwards, Downwards } move_dir_t;


/**
 * The position to place instructions relative to another.
 **/
typedef enum { Before, After } place_pos_t;


/**
 * An edge in the CFG.  This is duplicated in chiara.h and we should therefore
 * remove this definition.
 **/
typedef struct {
    ir_instruction_t *pred;
    ir_instruction_t *succ;
} edge_t;


/**
 * Successors of an instruction.
 **/
typedef struct {
    /* The target of a branch, NULL if not a branch. */
    ir_instruction_t *target;
    /* The fall-through block, NULL if an unconditional branch. */
    ir_instruction_t *fallThrough;
} succ_relation_t;


/**
 * Information required to patch up branches.
 **/
typedef struct {
    /* The branch that should be patched. */
    ir_instruction_t *branch;
    /* The branch's new target label. */
    ir_instruction_t *target;
    /* The branch's new fall-through instruction. */
    ir_instruction_t *fallThrough;
} branch_patch_t;


/**
 * A structure to hold information as we determine instructions reached up
 * and down the CFG.
 **/
typedef struct reached_t {
    ir_instruction_t *inst;
    JITBOOLEAN onStack;
    JITBOOLEAN split;
    XanBitSet *loopInsts;
} reached_t;


/**
 * Capture all information that scheduling needs to move instructions above
 * and below each other.
 **/
typedef struct {
    /* The scheduling structure passed from the calling function. */
    schedule_t *schedule;
    /* Direction of moving and position of instructions. */
    move_dir_t direction;
    place_pos_t position;
    /* The instruction to move as much as possible. */
    ir_instruction_t *maxMoveInst;
    /* The instructions to move. */
    XanBitSet *instsToMove;
    /* The instructions to move next to. */
    XanBitSet *nextToInsts;
    /* Whether the scheduling should fail. */
    JITBOOLEAN shouldFail;
    /* Instructions reached from each region instruction. */
    XanBitSet **movingReachedInsts;
    XanBitSet **branchingReachedInsts;
} schedule_info_t;

static inline XanHashTable * internal_fetchReachableInstructions (ir_method_t *method, XanHashTable **reachableInstructions, ir_instruction_t *inst);
static inline ir_instruction_t * internal_fetchInstruction (schedule_t *schedule, JITUINT32 instID);

/**
 * Set the level of debugging.
 **/
static inline void
setDebugLevel(void) {
    char *env = getenv("CHIARA_SCHEDULER_DEBUG_LEVEL");
    if (env != NULL) {
      PDEBUGLEVEL(atoi(env));
    }
}


/**
 * Allocate a new list.
 **/
static inline XanList *
newList(void) {
    return xanList_new(allocFunction, freeFunction, NULL);
}


/**
 * Free a list.
 **/
static inline void
freeList(XanList *list) {
    xanList_destroyList(list);
}


/**
 * Free a list and all data.
 **/
static inline void
freeListAndData(XanList *list) {
    xanList_destroyListAndData(list);
}


/**
 * Allocate a new hash table.
 **/
static inline XanHashTable *
newHashTable(void) {
    return xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
}


/**
 * Free a hash table.
 **/
static inline void
freeHashTable(XanHashTable *table) {
    xanHashTable_destroyTable(table);
}


/**
 * Print a bit set of instructions.
 **/
static inline void
printInstBitSet(JITINT32 level, JITUINT32 numInsts, XanBitSet *instSet) {
    JITUINT32 i;
    for (i = 0; i < numInsts; ++i) {
        if (xanBitSet_isBitSet(instSet, i)) {
            PDEBUGC(level, " %u", i);
        }
    }
    PDEBUGC(level, "\n");
}


/**
 * Print a list of instructions.
 **/
static inline void
printInstList(JITINT32 level, XanList *instList) {
    XanListItem *instItem = xanList_first(instList);
    while (instItem) {
        PDECL(ir_instruction_t *inst = instItem->data);
        PDEBUGC(level, " %u", inst->ID);
        instItem = instItem->next;
    }
    PDEBUGC(level, "\n");
}


/**
 * Print a list of edges.
 **/
static void
printEdgeList(JITINT32 level, XanList *edgeList) {
    XanListItem *edgeItem = xanList_first(edgeList);
    while (edgeItem) {
        PDECL(edge_t *edge = edgeItem->data);
        PDEBUGC(level, " (%u-%u)", edge->pred->ID, edge->succ->ID);
        edgeItem = edgeItem->next;
    }
    PDEBUGC(level, "\n");
}


/**
 * Print a set of basic blocks.
 **/
static inline void
printBlockBitSet(JITINT32 level, ir_method_t *method, JITUINT32 numBlocks, XanBitSet *blockSet) {
    JITUINT32 b;
    for (b = 0; b < numBlocks; ++b) {
        if (xanBitSet_isBitSet(blockSet, b)) {
            PDECL(IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(method, b));
            PDEBUGC(level, " (%u-%u)", block->startInst, block->endInst);
        }
    }
    PDEBUGC(level, "\n");
}


/**
 * Create an edge between 'pred' and 'succ'.
 *
 * Returns the created edge.
 **/
static inline edge_t *
createEdge(ir_instruction_t *pred, ir_instruction_t *succ) {
    edge_t *edge = allocFunction(sizeof(edge_t));
    edge->pred = pred;
    edge->succ = succ;
    return edge;
}


/**
 * Clone an edge.
 *
 * Returns the cloned edge.
 **/
static inline edge_t *
cloneEdge(edge_t *edge) {
    edge_t *clone = allocFunction(sizeof(edge_t));
    memcpy(clone, edge, sizeof(edge_t));
    return clone;
}


/**
 * Check whether two edges match.
 *
 * Returns JITTRUE if the edges match.
 **/
static inline JITBOOLEAN
edgesMatch(edge_t *first, edge_t *second) {
    return first->pred == second->pred && first->succ == second->succ;
}


/**
 * Check whether all instructions in 'instList' are marked in the bit set
 * 'markedInsts'.
 *
 * Returns JITTRUE if all instructions are marked, JITFALSE otherwise.
 **/
static JITBOOLEAN
areAllInstsMarked(XanBitSet *markedInsts, XanList *instList) {
    XanListItem *instItem = xanList_first(instList);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        if (!xanBitSet_isBitSet(markedInsts, inst->ID)) {
            return JITFALSE;
        }
        instItem = instItem->next;
    }
    return JITTRUE;
}


/**
 * Mark bits in 'instSet' corresponding to instructions in 'reachedList'.
 **/
static void
markInstsInBitSetFromReachedList(XanBitSet *instSet, XanList *reachedList) {
    XanListItem *reachItem = xanList_first(reachedList);
    while (reachItem) {
        reached_t *reach = reachItem->data;
        xanBitSet_setBit(instSet, reach->inst->ID);
        if (reach->loopInsts) {
            xanBitSet_union(instSet, reach->loopInsts);
        }
        reachItem = reachItem->next;
    }
}


/* /\** */
/*  * Build a data dependence graph for 'method' using 'isDataDependent' to */
/*  * determine dependences between instructions.  The final graph is an array */
/*  * of bit sets indicating dependences from one instruction to another. */
/*  * */
/*  * Returns an array of bit sets representing the data dependence graph. */
/*  **\/ */
/* static XanBitSet ** */
/* buildDataDependenceGraph(ir_method_t *method, JITUINT32 numInsts, JITBOOLEAN (*isDataDependent)(ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst)) */
/* { */
/*   JITUINT32 i, j; */
/*   XanBitSet **dataDepGraph; */

/*   /\* The data dependence graph we'll create. *\/ */
/*   dataDepGraph = allocFunction(numInsts * sizeof(XanBitSet *)); */
/*   for (i = 0; i < numInsts; ++i) { */
/*     dataDepGraph[i] = xanBitSet_new(numInsts); */
/*   } */

/*   /\* Build the graph, an instruction at a time. *\/ */
/*   for (i = 0; i < numInsts; ++i) { */
/*     ir_instruction_t *fromInst = IRMETHOD_getInstructionAtPosition(method, i); */
/*     for (j = 0; j < numInsts; ++j) { */
/*       ir_instruction_t *toInst = IRMETHOD_getInstructionAtPosition(method, j); */
/*       if (isDataDependent(method, fromInst, toInst)) { */
/*         xanBitSet_setBit(dataDepGraph[i], j); */
/*         xanBitSet_setBit(dataDepGraph[j], i); */
/*       } */
/*     } */
/*   } */

/*   /\* Return the dependence graph. *\/ */
/*   return dataDepGraph; */
/* } */


/**
 * Find loops within 'method'.  To do this, first find all back edges in
 * the CFG and create loops for each one.  Then repeatedly add instructions
 * from basic blocks that are predecessors of existing loop blocks (not the
 * header block) as long as they are pre-dominated by the header.
 *
 * Returns a list of schedule_loop_t structures representing loops found.
 **/
static XanList *
findLoopsInMethod(ir_method_t *method, XanList **predecessors, XanList **successors) {
    JITUINT32 b, c, i;
    JITUINT32 numInsts, numBlocks;
    XanList *loopsList;
    schedule_loop_t **innerLoops;
    JITINT32 **postdoms;
    JITINT32 **predoms;

    /* Assertions.
     */
    assert(predecessors != NULL);
    assert(method != NULL);

    /* Set up. */
    numInsts 		= IRMETHOD_getInstructionsNumber(method);
    numBlocks 		= IRMETHOD_getNumberOfMethodBasicBlocks(method);

    /* Allocate the memory	*/
    predoms = allocFunction(sizeof(JITINT32 *) * (numInsts+1));
    postdoms = allocFunction(sizeof(JITINT32 *) * (numInsts+1));

    /* Find back edges. */
    /* PDEBUG(0, "Finding loops in method %s\n", IRMETHOD_getMethodName(method)); */
    /* PDEBUG(1, "Finding back edges\n"); */
    innerLoops = allocFunction(numBlocks * sizeof(schedule_loop_t *));
    for (b = 0; b < numBlocks; ++b) {
        XanListItem	*predItem;
        IRBasicBlock *head = IRMETHOD_getBasicBlockAtPosition(method, b);
        ir_instruction_t *headInst = IRMETHOD_getInstructionAtPosition(method, head->startInst);
        predItem	= xanList_first(predecessors[headInst->ID]);
        while (predItem != NULL) {
            ir_instruction_t *pred;
            pred	= predItem->data;
            assert(pred != NULL);

            /* A back edge is pre-dominated and post-dominated by the header. */
            if (TOOLS_isInstructionADominator(method, numInsts, headInst, pred, predoms, IRMETHOD_isInstructionAPredominator) && TOOLS_isInstructionADominator(method, numInsts, headInst, pred, postdoms, IRMETHOD_isInstructionAPostdominator)) {
                IRBasicBlock *backEdgeBlock = IRMETHOD_getBasicBlockContainingInstruction(method, pred);
                JITUINT32 blockID = IRMETHOD_getBasicBlockPosition(method, backEdgeBlock);
                assert(!innerLoops[blockID]);
                innerLoops[blockID] = allocFunction(sizeof(schedule_loop_t));
                innerLoops[blockID]->headerID = head->startInst;
                innerLoops[blockID]->exits = xanBitSet_new(numInsts);
                innerLoops[blockID]->loop = xanBitSet_new(numInsts);
                /* PDEBUG(2, "  Back edge (%u-%u)\n", pred->ID, head->startInst); */
                for (i = head->startInst; i <= head->endInst; ++i) {
                    xanBitSet_setBit(innerLoops[blockID]->loop, i);
                }
                for (i = backEdgeBlock->startInst; i <= backEdgeBlock->endInst; ++i) {
                    xanBitSet_setBit(innerLoops[blockID]->loop, i);
                }
            }
            predItem	= predItem->next;
        }
    }

    /* Add predecessors to exising blocks. */
    JITBOOLEAN changes = JITTRUE;
    while (changes) {
        changes = JITFALSE;
        for (b = 0; b < numBlocks; ++b) {

            /* Find exising loops. */
            if (innerLoops[b]) {
                ir_instruction_t *headInst = IRMETHOD_getInstructionAtPosition(method, innerLoops[b]->headerID);

                /* Search for blocks that aren't included yet. */
                for (c = 0; c < numBlocks; ++c) {
                    IRBasicBlock *insideBlock = IRMETHOD_getBasicBlockAtPosition(method, c);
                    ir_instruction_t *insideStartInst = IRMETHOD_getInstructionAtPosition(method, insideBlock->startInst);

                    /* Filter the header block and existing blocks. */
                    if (insideStartInst != headInst && insideBlock->startInst < numInsts && xanBitSet_isBitSet(innerLoops[b]->loop, insideBlock->startInst)) {
                        XanListItem	*predItem;
                        predItem	= xanList_first(predecessors[insideStartInst->ID]);
                        while (predItem != NULL) {
                            ir_instruction_t *pred;
                            pred	= predItem->data;
                            assert(pred != NULL);

                            /* This is only needed if there could be side-entries. */
                            if (TOOLS_isInstructionADominator(method, numInsts, headInst, pred, predoms, IRMETHOD_isInstructionAPredominator)) {
                                IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(method, pred);

                                /* Avoid existing predecessors. */
                                if (!xanBitSet_isBitSet(innerLoops[b]->loop, predBlock->startInst)) {
                                    for (i = predBlock->startInst; i <= predBlock->endInst; ++i) {
                                        xanBitSet_setBit(innerLoops[b]->loop, i);
                                    }
                                    changes = JITTRUE;
                                }
                            }
                            predItem	= predItem->next;
                        }
                    }
                }
            }
        }
    }

    /* Identify exit instructions. */
    for (b = 0; b < numBlocks; ++b) {
        if (innerLoops[b]) {
            for (i = 0; i < numInsts; ++i) {
                if (xanBitSet_isBitSet(innerLoops[b]->loop, i)) {
                    XanListItem *succItem = xanList_first(successors[i]);
                    while (succItem) {
                        ir_instruction_t *succ = succItem->data;
                        if (succ->type != IREXITNODE && !xanBitSet_isBitSet(innerLoops[b]->loop, succ->ID)) {
                            xanBitSet_setBit(innerLoops[b]->exits, i);
                            break;
                        }
                        succItem = succItem->next;
                    }
                }
            }
        }
    }

    /* Create a list of loops. */
    loopsList = newList();
    for (b = 0; b < numBlocks; ++b) {
        if (innerLoops[b]) {
            xanList_append(loopsList, innerLoops[b]);
        }
    }

    /* Clean up. */
    freeFunction(innerLoops);
    for (b=0; b <= numInsts; b++) {
        freeFunction(predoms[b]);
        freeFunction(postdoms[b]);
    }
    freeFunction(predoms);
    freeFunction(postdoms);

    return loopsList;
}


/**
 * Merge loops with the same header together in a new list of schedule_loop_t *
 * structures.  Each loop contains all instructions from the combined loops
 * with exit instructions as the exits from the new loop, not the individual
 * loops.
 **/
static XanList *
mergeLoopsWithSameHeader(JITUINT32 numInsts, XanList *loops) {
    XanList *mergedList;
    XanHashTable *mergedLoops = newHashTable();
    XanListItem *loopItem = xanList_first(loops);
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        schedule_loop_t *mloop = xanHashTable_lookup(mergedLoops, (void *)(JITNUINT)loop->headerID);
        XanBitSet *mergedExits = xanBitSet_clone(loop->exits);
        JITINT32 i = -1;

        /* Create the initial merged loop. */
        if (mloop == NULL) {
            mloop = allocFunction(sizeof(schedule_loop_t));
            mloop->headerID = loop->headerID;
            mloop->exits = xanBitSet_new(numInsts);
            mloop->loop = xanBitSet_new(numInsts);
            xanHashTable_insert(mergedLoops, (void *)(JITNUINT)loop->headerID, mloop);
        }

        /* Find exits for the merged loop.  Is there a simple way of doing this? */
        i = -1;
        while ((i = xanBitSet_getFirstBitSetInRange(loop->exits, i + 1, numInsts)) != -1) {
            if (xanBitSet_isBitSet(mloop->exits, i) || !xanBitSet_isBitSet(mloop->loop, i)) {
                xanBitSet_setBit(mergedExits, i);
            }
        }
        i = -1;
        while ((i = xanBitSet_getFirstBitSetInRange(mloop->exits, i + 1, numInsts)) != -1) {
            if (xanBitSet_isBitSet(loop->exits, i) || !xanBitSet_isBitSet(loop->loop, i)) {
                xanBitSet_setBit(mergedExits, i);
            }
        }

        /* Update the merged loop. */
        xanBitSet_union(mloop->loop, loop->loop);
        xanBitSet_free(mloop->exits);
        mloop->exits = mergedExits;
        loopItem = loopItem->next;
    }
    mergedList = xanHashTable_toList(mergedLoops);
    freeHashTable(mergedLoops);
    return mergedList;
}


/**
 * Determine whether an instruction is a loop header.
 **/
static JITBOOLEAN
isInstALoopHeader(XanList *loops, ir_instruction_t *inst) {
    XanListItem *loopItem = xanList_first(loops);
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        if (loop->headerID == inst->ID) {
            return JITTRUE;
        }
        loopItem = loopItem->next;
    }
    return JITFALSE;
}


/**
 * Free a list of loops.
 **/
static void
freeLoopList(XanList *loopList) {
    XanListItem *loopItem = xanList_first(loopList);
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        xanBitSet_free(loop->loop);
        xanBitSet_free(loop->exits);
        freeFunction(loop);
        loopItem = loopItem->next;
    }
    xanList_destroyList(loopList);
}

/**
 * Find loops within 'method', merging together loops that have the same header
 * instruction.
 *
 * Returns a list of schedule_loop_t structures representing loops found.
 **/
static XanList *
findMergedLoopsInMethod(ir_method_t *method, JITUINT32 numInsts, XanList **predecessors, XanList **successors) {
    XanList *methodLoops = findLoopsInMethod(method, predecessors, successors);
    XanList *mergedLoops = mergeLoopsWithSameHeader(numInsts, methodLoops);
    freeLoopList(methodLoops);
    return mergedLoops;
}


/**
 * Find back edges in 'method' and place them in a new list.
 *
 * Returns a list of back edges.
 */
static XanList *
findBackEdgesInMethod(ir_method_t *method, XanList **predecessors) {
    JITUINT32 b;
    JITUINT32 numBlocks;
    XanList *backEdges;

    /* Set up. */
    setDebugLevel();
    /* PDEBUG(0, "Finding back edges in %s\n", IRMETHOD_getMethodName(method)); */
    numBlocks = IRMETHOD_getNumberOfMethodBasicBlocks(method);
    backEdges = newList();

    /* Find back edges. */
    for (b = 0; b < numBlocks; ++b) {
        XanListItem	*predItem;
        IRBasicBlock *head = IRMETHOD_getBasicBlockAtPosition(method, b);
        ir_instruction_t *headInst = IRMETHOD_getInstructionAtPosition(method, head->startInst);
        predItem	= xanList_first(predecessors[headInst->ID]);
        while (predItem != NULL) {
            ir_instruction_t *pred;
            pred	= predItem->data;
            assert(pred != NULL);

            /* A back edge is pre-dominated and post-dominated by the header. */
            if (IRMETHOD_isInstructionAPredominator(method, headInst, pred) && IRMETHOD_isInstructionAPostdominator(method, headInst, pred)) {
                edge_t *edge = createEdge(pred, headInst);
                xanList_append(backEdges, edge);
                /* PDEBUG(1, "  Back edge (%u-%u)\n", pred->ID, headInst->ID); */
            }
            predItem	= predItem->next;
        }
    }

    /* Return a list of edges. */
    return backEdges;
}


/**
 * Initialise a scheduling structure to enable scheduling within a method with
 * most information required.  The data dependence graph and region of
 * instructions are the notable absences.
 **/
schedule_t *
SCHEDULE_initialiseScheduleForMethod(ir_method_t *method) {
    schedule_t *schedule = allocFunction(sizeof(schedule_t));
    schedule->method = method;
    schedule->numInsts = IRMETHOD_getInstructionsNumber(schedule->method);
    schedule->numBlocks = IRMETHOD_getNumberOfMethodBasicBlocks(schedule->method);
    schedule->predecessors = IRMETHOD_getInstructionsPredecessors(schedule->method);
    schedule->successors = IRMETHOD_getInstructionsSuccessors(schedule->method);
    schedule->loops = findMergedLoopsInMethod(schedule->method, schedule->numInsts, schedule->predecessors, schedule->successors);
    schedule->backEdges = findBackEdgesInMethod(schedule->method, schedule->predecessors);
    return schedule;
}


/**
 * Free memory used by a scheduling structure.
 **/
void
SCHEDULE_freeSchedule(schedule_t *schedule) {
    IRMETHOD_destroyInstructionsPredecessors(schedule->predecessors, schedule->numInsts);
    IRMETHOD_destroyInstructionsSuccessors(schedule->successors, schedule->numInsts);
    freeLoopList(schedule->loops);
    freeListAndData(schedule->backEdges);
    if (schedule->dataDepGraph) {
        JITUINT32 i;
        for (i = 0; i < schedule->numInsts; ++i) {
            if (schedule->dataDepGraph[i]) {
                xanBitSet_free(schedule->dataDepGraph[i]);
            }
        }
        freeFunction(schedule->dataDepGraph);
    }
    if (schedule->regionInsts) {
        xanBitSet_free(schedule->regionInsts);
    }
    if (schedule->regionBlocks) {
        xanBitSet_free(schedule->regionBlocks);
    }
    freeFunction(schedule);
}


/**
 * Determine whether an edge exists within a list of edges.
 *
 * Returns JITTRUE if the edge exists within the list, JITFALSE otherwise.
 **/
static JITBOOLEAN
edgeExistsInList(ir_instruction_t *pred, ir_instruction_t *succ, XanList *edgeList) {
    XanListItem *edgeItem = xanList_first(edgeList);
    while (edgeItem) {
        edge_t *edge = edgeItem->data;
        if (edge->pred == pred && edge->succ == succ) {
            return JITTRUE;
        }
        edgeItem = edgeItem->next;
    }
    return JITFALSE;
}


/**
 * Determine whether an instruction is a predecessor in any edges within a list
 * of edges.
 **/
static JITBOOLEAN
instructionIsEdgePredecessorInList(ir_instruction_t *inst, XanList *edgeList) {
    XanListItem *edgeItem = xanList_first(edgeList);
    while (edgeItem) {
        edge_t *edge = edgeItem->data;
        if (edge->pred == inst) {
            return JITTRUE;
        }
        edgeItem = edgeItem->next;
    }
    return JITFALSE;
}


/* /\** */
/*  * Find instructions that can be reached by 'startInst' along all paths in */
/*  * the CFG after it, without going along a back edge. */
/*  * */
/*  * Returns a bit set indicating the instructions that are reached. */
/*  **\/ */
/* XanBitSet * SCHEDULE_getInstsReachedDownwards (ir_method_t *method, ir_instruction_t *startInst, XanList **predecessors, XanList **successors) { */
/*   XanList *backEdges; */
/*   XanBitSet *reachedInsts; */
/*   setDebugLevel(); */
/*   backEdges = SCHEDULE_findBackEdgesInMethod(method, sched->schedule->predecessors); */
/*   reachedInsts = SCHEDULE_getInstsReachedDownwardsNoBackEdge(method, startInst, backEdges, successors); */
/*   SCHEDULE_freeBackEdgeList(backEdges); */
/*   return reachedInsts; */
/* } */

/* /\** */
/*  * Find instructions that can be reached by 'startInst' along all paths in */
/*  * the CFG before it, without going along a back edge. */
/*  * */
/*  * Returns a bit set indicating the instructions that are reached. */
/*  **\/ */
/* XanBitSet * */
/* SCHEDULE_getInstsReachedUpwards(ir_method_t *method, ir_instruction_t *startInst, XanList **predecessors) */
/* { */
/*   XanList *backEdges; */
/*   XanBitSet *reachedInsts; */
/*   setDebugLevel(); */
/*   backEdges = SCHEDULE_findBackEdgesInMethod(method, sched->schedule->predecessors); */
/*   reachedInsts = SCHEDULE_getInstsReachedUpwardsNoBackEdge(method, startInst, backEdges, sched->schedule->predecessors); */
/*   SCHEDULE_freeBackEdgeList(backEdges); */
/*   return reachedInsts; */
/* } */

/* XanBitSet * */
/* SCHEDULE_getInstsBetweenInstsNoPredominator(ir_method_t *method, JITUINT32 numInsts, ir_instruction_t *startInst, ir_instruction_t *endInst, XanBitSet *instsToConsider) */
/* { */
/*   JITUINT32 i; */
/*   JITBOOLEAN finished; */
/*   XanStack *path = xanStack_new(allocFunction, freeFunction, NULL); */
/*   XanBitSet *reachedInsts = xanBitSet_new(numInsts); */
/*   XanBitSet *seenInsts = xanBitSet_new(numInsts); */
/*   reached_t *rinfo = allocFunction(numInsts * sizeof(reached_t)); */

/*   /\* Initialise the reached information. *\/ */
/*   setDebugLevel(); */
/*   for (i = 0; i < numInsts; ++i) { */
/*     rinfo[i].inst = IRMETHOD_getInstructionAtPosition(method, i); */
/*   } */

/*   /\* Start with startInst. *\/ */
/*   finished = JITFALSE; */
/*   xanStack_push(path, &rinfo[startInst->ID]); */
/*   rinfo[startInst->ID].onStack = JITTRUE; */
/*   xanBitSet_setBit(seenInsts, startInst->ID); */
/*   while (!finished) { */
/*     reached_t *topReached = xanStack_top(path); */
/*     ir_instruction_t *inst = topReached->inst; */
/*     ir_instruction_t *succ = NULL; */
/*     JITBOOLEAN pushed = JITFALSE; */
/*     JITINT32 loopHead = -1; */

/*     /\* Consider each successor. *\/ */
/*     while (!pushed && (succ = IRMETHOD_getSuccessorInstruction(method, inst, succ))) { */
/*       if (succ->ID < numInsts && (!instsToConsider || xanBitSet_isBitSet(instsToConsider, succ->ID)) && !(IRMETHOD_isInstructionAPredominator(method, succ, startInst) && IRMETHOD_isInstructionAPredominator(method, succ, endInst))) { */

/*         /\* Reached the end instruction or a path to it. *\/ */
/*         if (succ == endInst || xanBitSet_isBitSet(reachedInsts, succ->ID)) { */
/*           XanList *currPath = xanStack_toList(path); */
/*           markInstsInBitSetFromReachedList(reachedInsts, currPath); */
/*           xanBitSet_setBit(reachedInsts, succ->ID); */
/*           freeList(currPath); */
/*         } */

/*         /\* Not at the end, push onto the stack if not seen. *\/ */
/*         else if (!xanBitSet_isBitSet(seenInsts, succ->ID)) { */
/*           xanStack_push(path, &rinfo[succ->ID]); */
/*           rinfo[succ->ID].onStack = JITTRUE; */
/*           xanBitSet_setBit(seenInsts, succ->ID); */
/*           pushed = JITTRUE; */
/*           if (IRMETHOD_getSuccessorInstruction(method, inst, succ)) { */
/*             rinfo[succ->ID].split = JITTRUE; */
/*           } */
/*         } */

/*         /\* Instruction currently on stack, so a loop. *\/ */
/*         else if (rinfo[succ->ID].onStack) { */
/*           loopHead = succ->ID; */
/*           if (!rinfo[loopHead].loopInsts) { */
/*             rinfo[loopHead].loopInsts = xanBitSet_new(numInsts); */
/*           } */
/*         } */
/*       } */
/*     } */

/*     /\* Unwind the stack until the last split point. *\/ */
/*     if (!pushed) { */
/*       JITBOOLEAN foundSplit = JITFALSE; */
/*       while (!foundSplit && xanStack_getSize(path) > 0) { */
/*         reached_t *reach = xanStack_pop(path); */
/*         reach->onStack = JITFALSE; */
/*         foundSplit = reach->split; */
/*         if (loopHead != -1) { */
/*           xanBitSet_setBit(rinfo[loopHead].loopInsts, reach->inst->ID); */
/*           if (reach->inst->ID == loopHead) { */
/*             loopHead = -1; */
/*           } */
/*         } */
/*       } */
/*       finished = !foundSplit; */
/*       if (foundSplit) { */
/*         assert(xanStack_getSize(path) > 0); */
/*       } */
/*     } */
/*   } */

/*   /\* Clean up. *\/ */
/*   freeFunction(rinfo); */
/*   xanBitSet_free(seenInsts); */
/*   xanStack_destroyStack(path); */
/*   return reachedInsts; */
/* } */


/* XanBitSet * */
/* SCHEDULE_getInstsBetweenInstsNoPostdominator(ir_method_t *method, JITUINT32 numInsts, ir_instruction_t *startInst, ir_instruction_t *endInst, XanBitSet *instsToConsider) */
/* { */
/*   JITUINT32 i; */
/*   JITBOOLEAN finished; */
/*   XanStack *path = xanStack_new(allocFunction, freeFunction, NULL); */
/*   XanBitSet *reachedInsts = xanBitSet_new(numInsts); */
/*   XanBitSet *seenInsts = xanBitSet_new(numInsts); */
/*   reached_t *rinfo = allocFunction(numInsts * sizeof(reached_t)); */

/*   /\* Initialise the reached information. *\/ */
/*   setDebugLevel(); */
/*   for (i = 0; i < numInsts; ++i) { */
/*     rinfo[i].inst = IRMETHOD_getInstructionAtPosition(method, i); */
/*   } */

/*   /\* Start with startInst. *\/ */
/*   finished = JITFALSE; */
/*   xanStack_push(path, &rinfo[startInst->ID]); */
/*   rinfo[startInst->ID].onStack = JITTRUE; */
/*   xanBitSet_setBit(seenInsts, startInst->ID); */
/*   while (!finished) { */
/*     reached_t *topReached = xanStack_top(path); */
/*     ir_instruction_t *inst = topReached->inst; */
/*     ir_instruction_t *pred = NULL; */
/*     JITBOOLEAN pushed = JITFALSE; */
/*     JITINT32 loopHead = -1; */

/*     /\* Consider each predecessor. *\/ */
/*     while (!pushed && (pred = IRMETHOD_getPredecessorInstruction(method, inst, pred))) { */
/*       if (pred->ID < numInsts && (!instsToConsider || xanBitSet_isBitSet(instsToConsider, pred->ID)) && !(IRMETHOD_isInstructionAPostdominator(method, pred, startInst) && IRMETHOD_isInstructionAPostdominator(method, pred, endInst))) { */

/*         /\* Reached the end instruction or a path to it. *\/ */
/*         if (pred == endInst || xanBitSet_isBitSet(reachedInsts, pred->ID)) { */
/*           XanList *currPath = xanStack_toList(path); */
/*           PDEBUG(2, "*** Reached current endInst or reachedInst %u ***\n", pred->ID); */
/*           markInstsInBitSetFromReachedList(reachedInsts, currPath); */
/*           xanBitSet_setBit(reachedInsts, pred->ID); */
/*           freeList(currPath); */
/*         } */

/*         /\* Not at the end, push onto the stack if not seen. *\/ */
/*         else if (!xanBitSet_isBitSet(seenInsts, pred->ID)) { */
/*           xanStack_push(path, &rinfo[pred->ID]); */
/*           rinfo[pred->ID].onStack = JITTRUE; */
/*           xanBitSet_setBit(seenInsts, pred->ID); */
/*           PDEBUG(2, "Pushed %u\n", pred->ID); */
/*           pushed = JITTRUE; */
/*           if (IRMETHOD_getPredecessorInstruction(method, inst, pred)) { */
/*             rinfo[pred->ID].split = JITTRUE; */
/*             PDEBUG(2, "  Split point\n"); */
/*           } */
/*         } */

/*         /\* Instruction currently on stack, so a loop. *\/ */
/*         else if (rinfo[pred->ID].onStack) { */
/*           loopHead = pred->ID; */
/*           PDEBUG(2, "Currently on stack %u\n", pred->ID); */
/*           if (!rinfo[loopHead].loopInsts) { */
/*             rinfo[loopHead].loopInsts = xanBitSet_new(numInsts); */
/*           } */
/*         } */
/*       } */
/*     } */

/*     /\* Unwind the stack until the last split point. *\/ */
/*     if (!pushed) { */
/*       JITBOOLEAN foundSplit = JITFALSE; */
/*       while (!foundSplit && xanStack_getSize(path) > 0) { */
/*         reached_t *reach = xanStack_pop(path); */
/*         reach->onStack = JITFALSE; */
/*         foundSplit = reach->split; */
/*         if (loopHead != -1) { */
/*           xanBitSet_setBit(rinfo[loopHead].loopInsts, reach->inst->ID); */
/*           PDEBUG(2, "  Loop inst %u\n", reach->inst->ID); */
/*           if (reach->inst->ID == loopHead) { */
/*             loopHead = -1; */
/*           } */
/*         } */
/*       } */
/*       finished = !foundSplit; */
/*       if (foundSplit) { */
/*         PDEBUG(2, "Unwound stack to %u\n", ((reached_t *)xanStack_top(path))->inst->ID); */
/*       } */
/*     } */
/*   } */

/*   /\* Debugging output. *\/ */
/*   PDEBUG(3, "Reached upwards:"); */
/*   printInstBitSet(3, numInsts, reachedInsts); */

/*   /\* Clean up. *\/ */
/*   freeFunction(rinfo); */
/*   xanBitSet_free(seenInsts); */
/*   xanStack_destroyStack(path); */
/*   return reachedInsts; */
/* } */


/* XanBitSet * */
/* SCHEDULE_getInstsBetweenInstsDownwardsNoBackEdge(ir_method_t *method, ir_instruction_t *startInst, ir_instruction_t *endInst, XanBitSet *instsToConsider, XanList *backEdges) */
/* { */
/*   JITUINT32 i; */
/*   JITBOOLEAN finished; */
/*   JITUINT32 numInsts = IRMETHOD_getInstructionsNumber(method); */
/*   XanStack *path = xanStack_new(allocFunction, freeFunction, NULL); */
/*   XanBitSet *reachedInsts = xanBitSet_new(numInsts); */
/*   XanBitSet *seenInsts = xanBitSet_new(numInsts); */
/*   reached_t *rinfo = allocFunction(numInsts * sizeof(reached_t)); */
/*   XanBitSet *backEdgeInsts = xanBitSet_new(numInsts); */
/*   XanListItem *edgeItem = xanList_first(backEdges); */

/*   /\* Create a bit set for back edges. *\/ */
/*   setDebugLevel(); */
/*   while (edgeItem) { */
/*     edge_t *edge = edgeItem->data; */
/*     xanBitSet_setBit(backEdgeInsts, edge->pred->ID); */
/*     edgeItem = edgeItem->next; */
/*   } */

/*   /\* Initialise the reached information. *\/ */
/*   for (i = 0; i < numInsts; ++i) { */
/*     rinfo[i].inst = IRMETHOD_getInstructionAtPosition(method, i); */
/*   } */

/*   /\* Start with startInst. *\/ */
/*   finished = JITFALSE; */
/*   xanStack_push(path, &rinfo[startInst->ID]); */
/*   rinfo[startInst->ID].onStack = JITTRUE; */
/*   xanBitSet_setBit(seenInsts, startInst->ID); */
/*   while (!finished) { */
/*     reached_t *topReached = xanStack_top(path); */
/*     ir_instruction_t *inst = topReached->inst; */
/*     ir_instruction_t *succ = NULL; */
/*     JITBOOLEAN pushed = JITFALSE; */
/*     JITINT32 loopHead = -1; */

/*     /\* Consider each successor. *\/ */
/*     if (!xanBitSet_isBitSet(backEdgeInsts, inst->ID)) { */
/*       while (!pushed && (succ = IRMETHOD_getSuccessorInstruction(method, inst, succ))) { */
/*         if (succ->ID < numInsts && (!instsToConsider || xanBitSet_isBitSet(instsToConsider, succ->ID))) { */

/*           /\* Reached the end instruction or a path to it. *\/ */
/*           if (succ == endInst || xanBitSet_isBitSet(reachedInsts, succ->ID)) { */
/*             XanList *currPath = xanStack_toList(path); */
/*             markInstsInBitSetFromReachedList(reachedInsts, currPath); */
/*             xanBitSet_setBit(reachedInsts, succ->ID); */
/*             freeList(currPath); */
/*           } */

/*           /\* Not at the end, push onto the stack if not seen. *\/ */
/*           else if (!xanBitSet_isBitSet(seenInsts, succ->ID)) { */
/*             xanStack_push(path, &rinfo[succ->ID]); */
/*             rinfo[succ->ID].onStack = JITTRUE; */
/*             xanBitSet_setBit(seenInsts, succ->ID); */
/*             pushed = JITTRUE; */
/*             if (IRMETHOD_getSuccessorInstruction(method, inst, succ)) { */
/*               rinfo[succ->ID].split = JITTRUE; */
/*             } */
/*           } */

/*           /\* Instruction currently on stack, so a loop. *\/ */
/*           else if (rinfo[succ->ID].onStack) { */
/*             loopHead = succ->ID; */
/*             if (!rinfo[loopHead].loopInsts) { */
/*               rinfo[loopHead].loopInsts = xanBitSet_new(numInsts); */
/*             } */
/*           } */
/*         } */
/*       } */

/*       /\* Unwind the stack until the last split point. *\/ */
/*       if (!pushed) { */
/*         JITBOOLEAN foundSplit = JITFALSE; */
/*         while (!foundSplit && xanStack_getSize(path) > 0) { */
/*           reached_t *reach = xanStack_pop(path); */
/*           reach->onStack = JITFALSE; */
/*           foundSplit = reach->split; */
/*           if (loopHead != -1) { */
/*             xanBitSet_setBit(rinfo[loopHead].loopInsts, reach->inst->ID); */
/*             if (reach->inst->ID == loopHead) { */
/*               loopHead = -1; */
/*             } */
/*           } */
/*         } */
/*         finished = !foundSplit; */
/*         if (foundSplit) { */
/*           assert(xanStack_getSize(path) > 0); */
/*         } */
/*       } */
/*     } */
/*   } */

/*   /\* Clean up. *\/ */
/*   freeFunction(rinfo); */
/*   xanBitSet_free(seenInsts); */
/*   xanBitSet_free(backEdgeInsts); */
/*   xanStack_destroyStack(path); */
/*   return reachedInsts; */
/* } */


/* /\** */
/*  * Find the set of instructions that lie between 'from' and 'to'.  This is */
/*  * the intersection between the instructions reached one way by one */
/*  * instruction and the instructions reached the other way by the other. */
/*  * Loops are allowed as long as they don't encompass 'from' and 'to.  The */
/*  * only exception is the loop header and another is the back edge branch. */
/*  * It is assumed that 'from' occurs before 'to' in the CFG. */
/*  * */
/*  * Returns a bit set of instructions between instructions. */
/*  **\/ */
/* static XanBitSet * */
/* findInstSetBetweenInstAndInstWithLoops(schedule_info_t *sched, ir_instruction_t *from, ir_instruction_t *to, XanList **predecessors, XanList **successors) */
/* { */
/*   XanBitSet *reachedUp; */
/*   XanBitSet *reachedDown; */

/*   /\* Instructions reached by the instruction to move next to. *\/ */
/*   PDEBUG(2, "Finding insts between %u and %u\n", from->ID, to->ID); */
/*   reachedUp = SCHEDULE_getInstsReachedUpwardsNoBackEdge(sched->schedule->method, to, sched->backEdges, predecessors); */
/*   reachedDown = SCHEDULE_getInstsReachedDownwardsNoBackEdge(sched->schedule->method, from, sched->backEdges, successors); */

/*   /\* Add loops to reachedUp since it can't traverse back edges. *\/ */
/*   addLoopsToRegion(sched->schedule->method, sched->loops, reachedUp, predecessors, successors); */
/*   #ifdef PRINTDEBUG */
/*   PDEBUG(3, "Reached up:"); */
/*   printInstBitSet(3, sched->schedule->numInsts, reachedUp); */
/*   PDEBUG(3, "Reached down:"); */
/*   printInstBitSet(3, sched->schedule->numInsts, reachedDown);   */
/*   #endif */

/*   /\* Create the intersection. *\/ */
/*   xanBitSet_intersect(reachedUp, reachedDown); */
/*   assert(xanBitSet_isBitSet(reachedUp, from->ID)); */
/*   assert(xanBitSet_isBitSet(reachedUp, to->ID)); */

/*   /\* Clean up. *\/ */
/*   xanBitSet_free(reachedDown); */
/*   return reachedUp; */
/* } */


/**
 * Find instructions that can be reached by 'startInst' along all paths in
 * the CFG before it, without going along a back edge.
 *
 * Returns a bit set indicating the instructions that are reached.
 **/
XanBitSet *
SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule_t *schedule, ir_instruction_t *startInst) {
    XanList *worklist = newList();
    XanBitSet *reachedInsts = xanBitSet_new(schedule->numInsts);

    /* Consider each predecessor in turn. */
    xanList_append(worklist, startInst);
    while (xanList_length(worklist) > 0) {
        XanListItem	*predItem;
        XanListItem *instItem = xanList_first(worklist);
        ir_instruction_t *inst = instItem->data;
        xanList_deleteItem(worklist, instItem);

        /* Mark this instruction as reached. */
        if (!xanBitSet_isBitSet(reachedInsts, inst->ID)) {
            xanBitSet_setBit(reachedInsts, inst->ID);
        }

        /* Add unseen predecessors that don't follow back edges. */
        predItem = xanList_first(schedule->predecessors[inst->ID]);
        while (predItem != NULL) {
            ir_instruction_t *pred = predItem->data;
            if (pred->ID < schedule->numInsts && !xanBitSet_isBitSet(reachedInsts, pred->ID) && !edgeExistsInList(pred, inst, schedule->backEdges)) {
                xanList_append(worklist, pred);
            }
            predItem	= predItem->next;
        }
    }

    /* Clean up. */
    freeList(worklist);

    /* Return the reached instructions. */
    return reachedInsts;
}


/**
 * Find instructions that can be reached by 'startInst' along all paths in
 * the CFG after it, without going along a back edge.
 *
 * Returns a bit set indicating the instructions that are reached.
 **/
XanBitSet *
SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule_t *schedule, ir_instruction_t *startInst) {
    XanList *worklist = newList();
    XanBitSet *reachedInsts = xanBitSet_new(schedule->numInsts);

    /* Consider each successor in turn. */
    xanList_append(worklist, startInst);
    while (xanList_length(worklist) > 0) {
        XanListItem *succItem;
        XanListItem *instItem = xanList_first(worklist);
        ir_instruction_t *inst = instItem->data;
        xanList_deleteItem(worklist, instItem);

        /* Mark this instruction as reached. */
        if (!xanBitSet_isBitSet(reachedInsts, inst->ID)) {
            xanBitSet_setBit(reachedInsts, inst->ID);
        }

        /* Add unseen successors that don't follow back edges. */
        succItem = xanList_first(schedule->successors[inst->ID]);
        while (succItem != NULL) {
            ir_instruction_t *succ = succItem->data;
            if (succ->ID < schedule->numInsts && !xanBitSet_isBitSet(reachedInsts, succ->ID) && !edgeExistsInList(inst, succ, schedule->backEdges)) {
                xanList_append(worklist, succ);
            }
            succItem	= succItem->next;
        }
    }

    /* Clean up. */
    freeList(worklist);

    /* Return the reached instructions. */
    return reachedInsts;
}


/**
 * Find the set of instructions that lie between 'inst' and those in
 * 'instList'.  This is the intersection between the instructions reached
 * one way by one instruction and the instructions reached the other way
 * by the others.
 *
 * Returns a bit set of instructions between instructions.
 **/
static XanBitSet *
findInstSetBetweenInstAndListNoLoops(schedule_t *schedule, ir_instruction_t *inst, XanList *instList) {
    XanBitSet *reachedUp;
    XanBitSet *reachedDown;
    XanBitSet *regionInsts;
    XanListItem *instItem;

    /* Instructions reached by the instruction to move next to. */
    reachedUp = SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule, inst);
    reachedDown = SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, inst);

    /* See if all instructions to move are in one set. */
    if (areAllInstsMarked(reachedUp, instList)) {
        xanBitSet_clearAll(reachedDown);
        instItem = xanList_first(instList);
        while (instItem) {
            XanBitSet *reached = SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, instItem->data);
            xanBitSet_union(reachedDown, reached);
            xanBitSet_free(reached);
            instItem = instItem->next;
        }
        xanBitSet_intersect(reachedUp, reachedDown);
        regionInsts = reachedUp;
        xanBitSet_free(reachedDown);
        reachedDown	= NULL;
    }

    /* Check whether they're in the other set. */
    else if (areAllInstsMarked(reachedDown, instList)) {
        xanBitSet_clearAll(reachedUp);
        instItem = xanList_first(instList);
        while (instItem) {
            XanBitSet *reached = SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule, instItem->data);
            xanBitSet_union(reachedUp, reached);
            xanBitSet_free(reached);
            instItem = instItem->next;
        }
        xanBitSet_intersect(reachedDown, reachedUp);
        regionInsts = reachedDown;
        xanBitSet_free(reachedUp);
        reachedUp	= NULL;
    }

    /* Not all are in either set. */
    else {
        PDEBUG(4, "Can't find instructions between instructions\n");
        xanBitSet_clearAll(reachedDown);
        regionInsts = reachedDown;
        xanBitSet_free(reachedUp);
        reachedUp	= NULL;
    }

    /* Return the region instructions. */
    return regionInsts;
}


/**
 * Find the set of instructions that lie between 'first' and 'second'
 * without traversing loops in the CFG.
 *
 * Returns a bit set of instructions between instructions.
 **/
static XanBitSet *
findInstSetBetweenTwoInstsNoLoops(schedule_t *schedule, ir_instruction_t *first, ir_instruction_t *second) {
    XanBitSet *regionInsts;
    XanList *instList = newList();
    xanList_append(instList, second);
    regionInsts = findInstSetBetweenInstAndListNoLoops(schedule, first, instList);
    freeList(instList);
    return regionInsts;
}


/**
 * Add loops to 'regionInsts' if the loop header is already in the region
 * and there is at least one non-loop predecessor and one non-loop
 * successor that is also in the region.
 **/
static void
addLoopsToRegion(schedule_t *schedule, XanBitSet *regionInsts) {
    XanListItem *loopItem = xanList_first(schedule->loops);
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        if (xanBitSet_isBitSet(regionInsts, loop->headerID)) {
            XanListItem *predItem;
            ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, loop->headerID);
            IRBasicBlock *headerBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, startInst);
            JITBOOLEAN nonLoopPred = JITFALSE;
            predItem = xanList_first(schedule->predecessors[startInst->ID]);
            while (predItem != NULL && !nonLoopPred) {
                ir_instruction_t *nextTo = predItem->data;
                if (!xanBitSet_isBitSet(loop->loop, nextTo->ID) && xanBitSet_isBitSet(regionInsts, nextTo->ID)) {
                    nonLoopPred = JITTRUE;
                }
                predItem = predItem->next;
            }
            if (nonLoopPred) {
                XanListItem *succItem;
                JITBOOLEAN nonLoopSucc = JITFALSE;
                ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(schedule->method, headerBlock->endInst);
                succItem = xanList_first(schedule->successors[endInst->ID]);
                while (succItem != NULL && !nonLoopSucc) {
                    ir_instruction_t *nextTo = succItem->data;
                    if (nextTo->ID < schedule->numInsts && !xanBitSet_isBitSet(loop->loop, nextTo->ID) && xanBitSet_isBitSet(regionInsts, nextTo->ID)) {
                        nonLoopSucc = JITTRUE;
                    }
                    succItem = succItem->next;
                }
                if (nonLoopPred && nonLoopSucc) {
                    xanBitSet_union(regionInsts, loop->loop);
                }
            }
        }
        loopItem = loopItem->next;
    }
}


/**
 * Add loops to 'regionInsts' if the loop header is already in the region,
 * at least one non-loop predecessor can be reached from instructions in the
 * region and all non-loop successors can reach instructions in the region.
 **/
void
SCHEDULE_addContainedLoopsToInstRegion(schedule_t *schedule, XanBitSet *regionInsts) {
    XanListItem *loopItem = xanList_first(schedule->loops);
    PDEBUG(4, "Adding contained loops to the region\n");
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        if (xanBitSet_isBitSet(regionInsts, loop->headerID)) {
            JITBOOLEAN nonLoopPred = JITFALSE;
            ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, loop->headerID);
            XanListItem *predItem = xanList_first(schedule->predecessors[startInst->ID]);
            PDEBUG(4, "Looking at loop header %u\n", loop->headerID);
            while (predItem && !nonLoopPred) {
                ir_instruction_t *pred = predItem->data;
                if (!xanBitSet_isBitSet(loop->loop, pred->ID) && xanBitSet_isBitSet(regionInsts, pred->ID)) {
                    nonLoopPred = JITTRUE;
                }
                predItem = predItem->next;
            }
            if (nonLoopPred) {
                JITUINT32 s;
                JITBOOLEAN allSuccsReach = JITTRUE;
                XanBitSet *instsToAdd = xanBitSet_new(schedule->numInsts);
                for (s = 0; s < schedule->numInsts && allSuccsReach; ++s) {
                    if (xanBitSet_isBitSet(loop->loop, s)) {
                        XanListItem *succItem = xanList_first(schedule->successors[s]);
                        while (succItem && allSuccsReach) {
                            ir_instruction_t *succ = succItem->data;
                            if (!xanBitSet_isBitSet(loop->loop, succ->ID) && succ->type != IREXITNODE) {
                                XanBitSet *reachedDown = SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, succ);
                                xanBitSet_intersect(reachedDown, regionInsts);
                                if (xanBitSet_getCountOfBitsSet(reachedDown) == 0) {
                                    /* PDEBUG(4, "Non-loop successor %u doesn't reach down to the region\n", succ->ID); */
                                    allSuccsReach = JITFALSE;
                                } else {
                                    JITUINT32 anyRegionInstID = xanBitSet_getFirstBitSetInRange(reachedDown, 0, schedule->numInsts);
                                    ir_instruction_t *anyRegionInst = IRMETHOD_getInstructionAtPosition(schedule->method, anyRegionInstID);
                                    XanBitSet *betweenInsts = findInstSetBetweenTwoInstsNoLoops(schedule, succ, anyRegionInst);
                                    assert(xanBitSet_getCountOfBitsSet(betweenInsts) > 0);
                                    xanBitSet_union(instsToAdd, betweenInsts);
                                    xanBitSet_free(betweenInsts);
                                }
                                xanBitSet_free(reachedDown);
                            }
                            succItem = succItem->next;
                        }
                    }
                }
                if (allSuccsReach) {
                    xanBitSet_union(regionInsts, loop->loop);
                    xanBitSet_union(regionInsts, instsToAdd);
                    /* PDEBUG(3, "  Added loop header %u to region\n", loop->headerID); */
                }
                xanBitSet_free(instsToAdd);
            } else {
                /* PDEBUG(4, "  Doesn't have a non-loop predecessor in the region\n"); */
            }
        }
        loopItem = loopItem->next;
    }
}


/**
 * Find the set of instructions that lie between 'inst' and those in
 * 'instList'.  This is the intersection between the instructions reached
 * one way by one instruction and the instructions reached the other way
 * by the others.  Loops are allowed as long as they don't encompass all
 * instructions in 'instList' as well as 'inst'.  The only exception is
 * one instruction is the loop header and another is the back edge branch.
 *
 * Returns a bit set of instructions between instructions.
 **/
static XanBitSet *
findInstSetBetweenInstAndListWithLoops(schedule_t *schedule, ir_instruction_t *inst, XanList *instList, JITBOOLEAN *shouldFail) {
    XanBitSet *reachedUp;
    XanBitSet *reachedDown;
    XanBitSet *reachedUpLoops;
    XanBitSet *regionInsts;
    XanListItem *instItem;

    /* Instructions reached by the instruction to move next to. */
    reachedUp = SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule, inst);
    reachedDown = SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, inst);

    /* Add loops to reachedUp since it can't traverse back edges. */
    reachedUpLoops = xanBitSet_clone(reachedUp);
    addLoopsToRegion(schedule, reachedUpLoops);

    /* See if all instructions to move are reached upwards. */
    if (areAllInstsMarked(reachedUp, instList)) {
        xanBitSet_clearAll(reachedDown);
        instItem = xanList_first(instList);
        while (instItem) {
            XanBitSet *reached = SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, instItem->data);
            xanBitSet_union(reachedDown, reached);
            xanBitSet_free(reached);
            instItem = instItem->next;
        }
        xanBitSet_intersect(reachedUp, reachedDown);
        regionInsts = reachedUp;
        xanBitSet_free(reachedDown);
        SCHEDULE_addContainedLoopsToInstRegion(schedule, regionInsts);
        assert(areAllInstsMarked(regionInsts, instList));
    }

    /* Check whether they're reached downwards. */
    else if (areAllInstsMarked(reachedDown, instList)) {
        xanBitSet_clearAll(reachedUp);
        instItem = xanList_first(instList);
        while (instItem) {
            XanBitSet *reached = SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule, instItem->data);
            xanBitSet_union(reachedUp, reached);
            xanBitSet_free(reached);
            instItem = instItem->next;
        }
        xanBitSet_intersect(reachedDown, reachedUp);
        regionInsts = reachedDown;
        xanBitSet_free(reachedUp);
        SCHEDULE_addContainedLoopsToInstRegion(schedule, regionInsts);
        assert(areAllInstsMarked(regionInsts, instList));
    }

    /* They may be in a loop. */
    else if (areAllInstsMarked(reachedUpLoops, instList)) {
        PDEBUG(0, "Fail: Not dealing with loops correctly yet.\n");
        xanBitSet_clearAll(reachedDown);
        regionInsts = reachedDown;
        xanBitSet_free(reachedUp);
        *shouldFail = JITTRUE;
    }

    /* Not all are in any set. */
    else {
        PDEBUG(4, "Can't find instructions between instructions\n");
        xanBitSet_clearAll(reachedDown);
        regionInsts = reachedDown;
        xanBitSet_free(reachedUp);
    }

    /* Clean up. */
    xanBitSet_free(reachedUpLoops);
    return regionInsts;
}


/**
 * Create a bit set of instructions from a hash table.
 **/
static XanBitSet *
createInstBitSetFromHashTable(XanHashTable *table, JITUINT32 numInsts) {
    XanBitSet *bitset = xanBitSet_new(numInsts);
    XanHashTableItem *item = xanHashTable_first(table);
    while (item) {
        ir_instruction_t *inst = item->elementID;
        xanBitSet_setBit(bitset, inst->ID);
        item = xanHashTable_next(table, item);
    }
    return bitset;
}


/**
 * Find the set of instructions that lie between two instructions.
 **/
XanBitSet *
SCHEDULE_findInstSetBetweenTwoInstsWithLoops(schedule_t *schedule, ir_instruction_t *first, ir_instruction_t *second) {
    XanBitSet *betweenInsts;
    XanHashTable *reachedUpTable = IRMETHOD_getInstructionsThatCanHaveBeenExecutedBeforeInstruction(schedule->method, first);
    XanHashTable *reachedDownTable = IRMETHOD_getInstructionsThatCanBeReachedFromPosition(schedule->method, first);

    /* Check for reaching downwards. */
    if (xanHashTable_lookup(reachedDownTable, second)) {
        XanBitSet *reachedUp;
        assert(!xanHashTable_lookup(reachedUpTable, second));
        betweenInsts = createInstBitSetFromHashTable(reachedDownTable, schedule->numInsts);
        freeHashTable(reachedUpTable);
        reachedUpTable = IRMETHOD_getInstructionsThatCanHaveBeenExecutedBeforeInstruction(schedule->method, second);
        reachedUp = createInstBitSetFromHashTable(reachedUpTable, schedule->numInsts);
        xanBitSet_intersect(betweenInsts, reachedUp);
        xanBitSet_free(reachedUp);
        SCHEDULE_addContainedLoopsToInstRegion(schedule, betweenInsts);
    }

    /* Check for reaching upwards. */
    else if (xanHashTable_lookup(reachedUpTable, second)) {
        XanBitSet *reachedDown;
        assert(!xanHashTable_lookup(reachedDownTable, second));
        betweenInsts = createInstBitSetFromHashTable(reachedUpTable, schedule->numInsts);
        freeHashTable(reachedDownTable);
        reachedDownTable = IRMETHOD_getInstructionsThatCanBeReachedFromPosition(schedule->method, second);
        reachedDown = createInstBitSetFromHashTable(reachedDownTable, schedule->numInsts);
        xanBitSet_intersect(betweenInsts, reachedDown);
        xanBitSet_free(reachedDown);
        SCHEDULE_addContainedLoopsToInstRegion(schedule, betweenInsts);
    }

    /* Not found, probably in a loop or on different paths. */
    else {
        betweenInsts = createInstBitSetFromHashTable(reachedUpTable, schedule->numInsts);
        /* SCHEDULE_addContainedLoopsToRegion(schedule, betweenInsts); */
        /* if (xanBitSet_isBitSet(betweenInsts, second->ID)) { */
        /*   XanBitSet *reachedDown; */
        /*   freeHashTable(reachedDownTable); */
        /*   reachedDownTable = IRMETHOD_getInstructionsThatCanBeReachedFromPosition(schedule->method, second); */
        /*   reachedDown = createInstBitSetFromHashTable(reachedDownTable, schedule->numInsts); */
        /*   xanBitSet_intersect(betweenInsts, reachedDown); */
        /*   xanBitSet_free(reachedDown); */
        /*   SCHEDULE_addContainedLoopsToInstRegion(schedule, betweenInsts); */
        /* } */

        /* /\* Must be on different paths, I guess. *\/ */
        /* else { */
        PDEBUG(4, "Can't find instructions between instructions\n");
        xanBitSet_clearAll(betweenInsts);
        /* } */
    }

    /* Clean up. */
    freeHashTable(reachedUpTable);
    freeHashTable(reachedDownTable);
    return betweenInsts;
}


/**
 * Check whether 'inst' pre-dominates all instructions in 'region'.
 *
 * Returns JITTRUE if 'inst' pre-dominates, JITFALSE otherwise.
 **/
static JITBOOLEAN
doesInstPreDominateRegion(schedule_t *schedule, ir_instruction_t *inst, XanBitSet *region) {
    JITINT32 i = -1;
    while ((i = xanBitSet_getFirstBitSetInRange(region, i + 1, schedule->numInsts)) != -1) {
        ir_instruction_t *check = IRMETHOD_getInstructionAtPosition(schedule->method, i);
        if (!IRMETHOD_isInstructionAPredominator(schedule->method, inst, check)) {
            return JITFALSE;
        }
    }
    return JITTRUE;
}


/**
 * Check whether there is an instruction in 'region' that pre-dominates the
 * whole region.
 *
 * Returns the pre-dominator, if there is one, NULL otherwise.
 **/
static ir_instruction_t *
getRegionPreDominator(schedule_t *schedule, XanBitSet *region) {
    JITINT32 i = -1;
    while ((i = xanBitSet_getFirstBitSetInRange(region, i + 1, schedule->numInsts)) != -1) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
        XanNode *domSubTree = schedule->method->preDominatorTree->find(schedule->method->preDominatorTree, inst);
        XanList *domList = domSubTree->toPreOrderList(domSubTree);
        XanBitSet *domSet = xanBitSet_new(schedule->numInsts);
        XanListItem *domItem = xanList_first(domList);
        while (domItem) {
            ir_instruction_t *domInst = (ir_instruction_t *)domItem->data;
            if (domInst->ID < schedule->numInsts) {
                xanBitSet_setBit(domSet, domInst->ID);
            }
            domItem = domItem->next;
        }
        freeList(domList);
        if (xanBitSet_isSubSetOf(region, domSet)) {
            xanBitSet_free(domSet);
            return inst;
        }
        xanBitSet_free(domSet);
    }
    return NULL;
}


/**
 * Add instructions to 'region' until there is at least one instruction in
 * 'region' that pre-dominates the whole region.
 **/
void
SCHEDULE_addRegionPreDominator(schedule_t *schedule, XanBitSet *region) {
    JITUINT32 anyInstID;
    ir_instruction_t *anyInst = NULL;
#if 0
    JITUINT32 i;
    ir_instruction_t *preDomInst = NULL;
    XanList *preDomList;
    XanListItem *domItem;
#endif

    /* Easy check first. */
    if (getRegionPreDominator(schedule, region)) {
        return;
    }

    /* Choose any instruction in the region. */
    anyInstID = xanBitSet_getFirstBitSetInRange(region, 0, schedule->numInsts);
    anyInst = IRMETHOD_getInstructionAtPosition(schedule->method, anyInstID);

    /* Check whether this is a pre-dominator. */
    if (doesInstPreDominateRegion(schedule, anyInst, region)) {
        return;
    }

    /* Constraint: Sometimes the call to findInstSetBetweenInstAndInstWithLoops()
       can't find a set of instructions and so an assertion fails in it.  Until
       this is fixed we won't move instructions downwards if they get to this
       point. */
    xanBitSet_clearAll(region);
    return;

#if 0
    /* Find a pre-dominator that does dominate the region. */
    preDomList = getElemsDominatingElem(anyInst, sched->schedule->method->preDominatorTree);
    domItem = xanList_first(preDomList);
    while (domItem && !preDomInst) {
        ir_instruction_t *domInst = domItem->data;
        if (doesInstPreDominateRegion(sched, domInst, region)) {
            xanBitSet_setBit(region, domInst->ID);
            preDomInst = domInst;
        }
        domItem = domItem->next;
    }

    /* Must have found one. */
    assert(preDomInst);

    /* Add in all instructions between. */
    for (i = 0; i < sched->schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(region, i) && i != preDomInst->ID) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(sched->schedule->method, i);
            XanBitSet *betweenInsts = findInstSetBetweenInstAndInstWithLoops(sched, preDomInst, inst);
            xanBitSet_union(region, betweenInsts);
            xanBitSet_free(betweenInsts);
        }
    }

    /* There must be a pre-dominator now. */
    assert(getRegionPreDominator(sched, region) == preDomInst);

    /* Clean up. */
    freeList(preDomList);
    return JITTRUE;
#endif
}


/* /\** */
/*  * Determine whether at least one predecessor instruction to 'inst' is marked */
/*  * in 'instSet'. */
/*  * */
/*  * Returns JITTRUE if at least one predecessor is marked, JITFALSE otherwise. */
/*  **\/ */
/* static JITBOOLEAN */
/* isOnePredecessorInstInBitSet(ir_method_t *method, ir_instruction_t *inst, XanBitSet *instSet, XanList **predecessors) */
/* { */
/*   XanListItem		*predItem; */
/*   predItem	= xanList_first(predecessors[inst->ID]); */
/*   while (predItem != NULL){ */
/*     ir_instruction_t 	*pred; */
/*     pred	= predItem->data; */
/*     assert(pred != NULL); */
/*     if (xanBitSet_isBitSet(instSet, pred->ID)) { */
/*       return JITTRUE; */
/*     } */
/*     predItem	= predItem->next; */
/*   } */
/*   return JITFALSE; */
/* } */


/* /\** */
/*  * Determine whether at least one successor instruction to 'inst' is marked */
/*  * in 'instSet'. */
/*  * */
/*  * Returns JITTRUE if at least one predecessor is marked, JITFALSE otherwise. */
/*  **\/ */
/* static JITBOOLEAN */
/* isOneSuccessorInstInBitSet(ir_method_t *method, JITUINT32 numInsts, ir_instruction_t *inst, XanBitSet *instSet) */
/* { */
/*   ir_instruction_t *succ = NULL; */
/*   while ((succ = IRMETHOD_getSuccessorInstruction(method, inst, succ))) { */
/*     if (succ->ID < numInsts && xanBitSet_isBitSet(instSet, succ->ID)) { */
/*       return JITTRUE; */
/*     } */
/*   } */
/*   return JITFALSE; */
/* } */


/* /\** */
/*  * Build a bit set of instructions that lie within the region defined by */
/*  * instructions between 'instsToMove' and 'nextToInst' in 'sched'.  These */
/*  * are instructions to consider for moving.  As a side effect, determine */
/*  * the direction they should be moved in the CFG too. */
/*  * */
/*  * The basic algorithm is to first determine the instructions reached by */
/*  * 'nextToInst' both up and down in the CFG.  All instructions in */
/*  * 'instsToMove' should be reachable in one and only one of these sets. */
/*  * Then find the union of all instructions reached the opposite way by */
/*  * 'instsToMove'.  The set of instructions in the region is the */
/*  * intersection of the two sets.  Loops are explicitly included if a loop */
/*  * header is in the region with a non-loop predecessor and non-loop */
/*  * successor also in the region. */
/*  * */
/*  * If moving downwards the region must include at least one instruction */
/*  * that pre-dominates all region instructions, so that branches can be */
/*  * correctly cloned. */
/*  * */
/*  * Blocked instructions need special attention.  They can extend the region */
/*  * both up and down.  If 'nextToInst' is a blocked instruction, and the */
/*  * blocked region extends beyond a branch, then there may be several places */
/*  * where the instructions need to be moved to. */
/*  *  */
/*  * Returns a bit set of instructions within the region. */
/*  **\/ */
/* static XanBitSet * */
/* buildRegionInsts(schedule_info_t *sched, XanList **predecessors, XanList **successors) */
/* { */
/*   JITUINT32 i; */
/*   JITBOOLEAN added; */
/*   XanBitSet *regionInsts; */

/*   /\* Instructions reached. *\/ */
/*   regionInsts = findInstSetBetweenInstAndListWithLoops(sched->schedule->method, sched->nextToInst, sched->instsToMove, sched->loops, sched->backEdges, predecessors, successors, &(sched->shouldFail)); */

/*   /\* Determine direction. *\/ */
/*   if (isOnePredecessorInstInBitSet(sched->schedule->method, sched->nextToInst, regionInsts, predecessors)) { */
/*     PDEBUG(1, "Moving downwards\n"); */
/*     sched->direction = Downwards; */
/*   } else { */
/*     if (isOneSuccessorInstInBitSet(sched->schedule->method, sched->schedule->numInsts, sched->nextToInst, regionInsts)) { */
/*       PDEBUG(1, "Moving upwards\n"); */
/*       sched->direction = Upwards; */
/*     } else { */
/*       PDEBUG(1, "Can't find direction to move instructions\n"); */
/*       xanBitSet_clearAll(regionInsts); */
/*     } */
/*   } */

/*   /\* Add blocked instructions and pre-dominators if necessary. *\/ */
/*   added = JITTRUE; */
/*   while (added) { */
/*     added = JITFALSE; */

/*     /\* Add in blocked instructions.  Adding them in here makes things a lot */
/*        simpler later since we don't have to explicitly add them on every */
/*        edge where clones are placed.  Since they can't be moved, they can */
/*        be viewed as part of the region anyway. *\/ */
/*     for (i = 0; i < sched->schedule->numInsts; ++i) { */
/*       if (xanBitSet_isBitSet(regionInsts, i)) { */
/*         ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(sched->schedule->method, i); */
/*         blocked_region_t *blockedRegion = xanHashTable_lookup(sched->blockedRegions, inst); */

/*         /\* Adding in blocked regions to expand the region is difficult to get */
/*            right.  It may mean that there are multiple region start */
/*            instructions.  When moving upwards, this is causing issues with */
/*            finding the right instructions to clone and positions to do the */
/*            cloning, so for now this is just not allowed. *\/ */
/*         if (blockedRegion && !xanBitSet_isSubSetOf(blockedRegion->insts, regionInsts)) { */
/*           /\* xanBitSet_union(regionInsts, blockedRegion->insts); *\/ */
/*           /\* added = JITTRUE; *\/ */
/*           xanBitSet_clearAll(regionInsts); */
/*         } */
/*       } */
/*     } */

/*     /\* Ensure there's a region pre-dominator. *\/ */
/*     if (sched->direction == Downwards) { */
/*       JITBOOLEAN shouldFail = JITFALSE; */
/*       added |= addRegionPreDominator(sched, regionInsts, &shouldFail); */
/*       if (shouldFail) { */
/*         xanBitSet_clearAll(regionInsts); */
/*         added = JITFALSE; */
/*       } */
/*     } */
/*   } */

/*   /\* Debugging. *\/ */
/*   /\* #ifdef PRINTDEBUG *\/ */
/*   PDEBUG(3, "Region insts:"); */
/*   printInstBitSet(3, sched->schedule->numInsts, regionInsts); */
/*   /\* #endif *\/ */

/*   /\* Return this bit set. *\/ */
/*   return regionInsts; */
/* } */


/**
 * Create a bit set representing blocks containing instructions from
 * 'insts'.
 *
 * Returns the newly-created bit set.
 **/
static XanBitSet *
getBlocksContainingInsts(schedule_t *schedule, XanBitSet *insts) {
    JITUINT32 i;
    XanBitSet *blocks = xanBitSet_new(schedule->numBlocks);
    for (i = 0; i < schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(insts, i)) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
            IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, inst);
            JITUINT32 blockId = IRMETHOD_getBasicBlockPosition(schedule->method, block);
            assert(blockId < schedule->numBlocks);
            xanBitSet_setBit(blocks, blockId);
        }
    }
    return blocks;
}


/**
 * Find instructions that can be reached by 'startInst' along all paths in
 * the CFG, without going through a pre-dominator or post-dominator
 * instruction.  The direction that 'startInst' will be moved determines
 * the direction to look for instructions.
 *
 * Returns a bit set indicating the instructions that are reached.
 **/
static XanBitSet *
getInstsReachedWhenMoving(schedule_t *schedule, ir_instruction_t *startInst, move_dir_t direction) {
    if (direction == Downwards) {
        return SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, startInst);
    } else {
        XanBitSet *reached = SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule, startInst);
        addLoopsToRegion(schedule, reached);
        return reached;
    }
}


/**
 * Find instructions that can be reached by 'startInst' along all paths in
 * the CFG, without going through a pre-dominator or post-dominator
 * instruction.  The direction that 'startInst' will be moved determines
 * the direction to look for instructions.
 *
 * Returns a bit set indicating the instructions that are reached.
 **/
static XanBitSet *
getInstsReachedWhenBranching(schedule_t *schedule, ir_instruction_t *startInst, move_dir_t direction) {
    if (direction == Upwards) {
        return SCHEDULE_getInstsReachedDownwardsNoBackEdge(schedule, startInst);
    } else {
        XanBitSet *reached = SCHEDULE_getInstsReachedUpwardsNoBackEdge(schedule, startInst);
        addLoopsToRegion(schedule, reached);
        return reached;
    }
}


/**
 * Build an array of bit sets that represent instructions reached by
 * instructions that can be moved.
 *
 * Returns an array of bit sets indicating instructions reached.
 **/
static XanBitSet **
buildInstsReachedWhenMoving(schedule_t *schedule, move_dir_t direction) {
    JITUINT32 i;
    XanBitSet **reachedInsts = allocFunction(schedule->numInsts * sizeof(XanBitSet *));
    for (i = 0; i < schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(schedule->regionInsts, i)) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
            reachedInsts[i] = getInstsReachedWhenMoving(schedule, inst, direction);
        } else {
            reachedInsts[i] = NULL;
        }
    }
    return reachedInsts;
}


/**
 * Build an array of bit sets that represent instructions reached by
 * instructions when branching.
 *
 * Returns an array of bit sets indicating instructions reached.
 **/
static XanBitSet **
buildInstsReachedWhenBranching(schedule_t *schedule, move_dir_t direction) {
    JITUINT32 i;
    XanBitSet **reachedInsts = allocFunction(schedule->numInsts * sizeof(XanBitSet *));
    for (i = 0; i < schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(schedule->regionInsts, i)) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
            reachedInsts[i] = getInstsReachedWhenBranching(schedule, inst, direction);
        } else {
            reachedInsts[i] = NULL;
        }
    }
    return reachedInsts;
}


/* /\** */
/*  * Build a bit set representing instructions from 'instList'. */
/*  * */
/*  * Returns a bit set of instructions. */
/*  **\/ */
/* static XanBitSet * */
/* buildBitSetFromInstList(JITUINT32 numInsts, XanList *instList) */
/* { */
/*   XanBitSet *instSet = xanBitSet_new(numInsts); */
/*   XanListItem *instItem = xanList_first(instList); */
/*   while (instItem) { */
/*     ir_instruction_t *inst = instItem->data; */
/*     assert(inst->ID < numInsts); */
/*     xanBitSet_setBit(instSet, inst->ID); */
/*     instItem = instItem->next; */
/*   } */
/*   return instSet; */
/* } */


/**
 * Build a list of instructions marked in 'instSet'.
 *
 * Returns a list of instructions.
 **/
static XanList *
buildListFromInstBitSet(ir_method_t *method, JITUINT32 numInsts, XanBitSet *instSet) {
    JITUINT32 i;
    XanList *instList = newList();
    for (i = 0; i < numInsts; ++i) {
        if (xanBitSet_isBitSet(instSet, i)) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
            xanList_append(instList, inst);
        }
    }
    return instList;
}


/* /\** */
/*  * Build a bit set of instructions in 'blockedInsts' that are adjacent to */
/*  * 'blocked'. */
/*  * */
/*  * Returns a bit set of adjacent blocked instructions. */
/*  **\/ */
/* static XanBitSet * */
/* getAdjacentBlockedInsts(ir_method_t *method, JITUINT32 numInsts, ir_instruction_t *blocked, XanBitSet *blockedInsts, XanList **predecessors, XanList **successors) */
/* { */
/*   XanList *instsToCheck = newList(); */
/*   XanBitSet *adjacentInsts = xanBitSet_new(numInsts); */
/*   XanListItem *checkItem; */
/*   xanList_append(instsToCheck, blocked); */
/*   while ((checkItem = xanList_first(instsToCheck))) { */
/*     XanListItem	*predItem; */
/*     XanListItem	*succItem; */
/*     ir_instruction_t *nextTo; */
/*     ir_instruction_t *checkInst = checkItem->data; */
/*     predItem	= xanList_first(predecessors[checkInst->ID]); */
/*     while (predItem != NULL){ */
/*       nextTo	= predItem->data; */
/*       assert(nextTo != NULL); */
/*       if (xanBitSet_isBitSet(blockedInsts, nextTo->ID) && !xanBitSet_isBitSet(adjacentInsts, nextTo->ID)) { */
/*         xanBitSet_setBit(adjacentInsts, nextTo->ID); */
/*         xanList_append(instsToCheck, nextTo); */
/*       } */
/*       predItem	= predItem->next; */
/*     } */
/*     succItem	= xanList_first(successors[checkInst->ID]); */
/*     while (succItem != NULL){ */
/*       nextTo	= succItem->data; */
/*       assert(nextTo != NULL); */
/*       if (nextTo->ID < numInsts && xanBitSet_isBitSet(blockedInsts, nextTo->ID) && !xanBitSet_isBitSet(adjacentInsts, nextTo->ID)) { */
/*         xanBitSet_setBit(adjacentInsts, nextTo->ID); */
/*         xanList_append(instsToCheck, nextTo); */
/*       } */
/*       succItem	= succItem->next; */
/*     } */
/*     xanList_deleteItem(instsToCheck, checkItem); */
/*   } */
/*   freeList(instsToCheck); */
/*   return adjacentInsts; */
/* } */


/* /\** */
/*  * Build a region of blocked instructions for those in 'blockedInsts'.  The */
/*  * region will contain edges leading into and out of the region. */
/*  * */
/*  * Returns a blocked region. */
/*  **\/ */
/* static blocked_region_t * */
/* buildSingleBlockedRegion(ir_method_t *method, JITUINT32 numInsts, XanBitSet *blockedInsts, XanList **predecessors, XanList **successors) */
/* { */
/*   JITUINT32 i; */
/*   blocked_region_t *region = allocFunction(sizeof(blocked_region_t)); */
/*   region->insts = blockedInsts; */
/*   region->edgesIn = newList(); */
/*   region->edgesOut = newList(); */
/*   for (i = 0; i < numInsts; ++i) { */
/*     if (xanBitSet_isBitSet(blockedInsts, i)) { */
/*       XanListItem	*predItem; */
/*       XanListItem	*succItem; */
/*       ir_instruction_t *blocked = IRMETHOD_getInstructionAtPosition(method, i); */
/*       ir_instruction_t *nextTo = NULL; */

/*       /\* Edges leading into the region. *\/ */
/*       predItem	= xanList_first(predecessors[blocked->ID]); */
/*       while (predItem != NULL){ */
/* 	nextTo	= predItem->data; */
/*         assert(nextTo != NULL); */
/*         if (!xanBitSet_isBitSet(blockedInsts, nextTo->ID)) { */
/*           edge_t *edge = createEdge(nextTo, blocked); */
/*           xanList_append(region->edgesIn, edge); */
/*         } */
/*         predItem	= predItem->next; */
/*       } */

/*       /\* Edges leading out of the region. *\/ */
/*       nextTo	= NULL; */
/*       succItem	= xanList_first(successors[blocked->ID]); */
/*       while (succItem != NULL){ */
/*         nextTo	= succItem->data; */
/*         assert(nextTo != NULL); */
/*         if (nextTo->ID < numInsts && !xanBitSet_isBitSet(blockedInsts, nextTo->ID)) { */
/*           edge_t *edge = createEdge(blocked, nextTo); */
/*           xanList_append(region->edgesOut, edge); */
/*         } */
/*         succItem	= succItem->next; */
/*       } */
/*     } */
/*   } */
/*   return region; */
/* } */


/* /\** */
/*  * Build a hash table that maps instructions that are blocked to a region */
/*  * containing edges into and out of that region. */
/*  * */
/*  * Returns a hash table mapping blocked instructions to regions. */
/*  **\/ */
/* static XanHashTable * */
/* buildBlockedRegions(ir_method_t *method, JITUINT32 numInsts, XanBitSet *blockedInsts, XanList **predecessors, XanList **successors) */
/* { */
/*   JITUINT32 i, j; */
/*   XanHashTable *blockedRegions = newHashTable(); */
/*   PDEBUG(1, "Building blocked regions\n"); */
/*   for (i = 0; i < numInsts; ++i) { */
/*     if (xanBitSet_isBitSet(blockedInsts, i)) { */
/*       blocked_region_t *region = NULL; */
/*       ir_instruction_t *blocked = IRMETHOD_getInstructionAtPosition(method, i); */
/*       XanBitSet *adjacentBlockedInsts = getAdjacentBlockedInsts(method, numInsts, blocked, blockedInsts, predecessors, successors); */
/*       if (!xanBitSet_isEmpty(adjacentBlockedInsts)) { */
/*         assert(xanBitSet_isBitSet(adjacentBlockedInsts, blocked->ID)); */

/*         /\* Find the region for this instruction. *\/ */
/*         for (j = 0; j < numInsts && !region; ++j) { */
/*           if (xanBitSet_isBitSet(adjacentBlockedInsts, j)) { */
/*             ir_instruction_t *adjacentInst = IRMETHOD_getInstructionAtPosition(method, j); */
/*             region = xanHashTable_lookup(blockedRegions, adjacentInst); */
/*           } */
/*         } */

/*         /\* Create a new region if necessary. *\/ */
/*         if (!region) { */
/*           region = buildSingleBlockedRegion(method, numInsts, adjacentBlockedInsts, predecessors, successors); */
/*           region->owner = blocked; */
/*           PDEBUG(3, "  Blocked region:"); */
/*           printInstBitSet(3, numInsts, adjacentBlockedInsts); */
/*         } */

/*         /\* Can clean up otherwise. *\/ */
/*         else { */
/*           assert(xanBitSet_equal(adjacentBlockedInsts, region->insts)); */
/*           xanBitSet_free(adjacentBlockedInsts); */
/*         } */
/*       } */

/*       /\** This instruction is all by itself, but later analysis expects all */
/*        * blocked instructions to be part of a region, so create one here so */
/*        * that there are no problems later on. *\/ */
/*       else { */
/*         xanBitSet_setBit(adjacentBlockedInsts, i); */
/*         region = buildSingleBlockedRegion(method, numInsts, adjacentBlockedInsts, predecessors, successors); */
/*         region->owner = blocked; */
/*         PDEBUG(3, "  Blocked region (single):"); */
/*         printInstBitSet(3, numInsts, adjacentBlockedInsts); */
/*       } */

/*       /\* Add to the hash table. *\/ */
/*       assert(region != NULL); */
/*       assert(region->owner != NULL); */
/*       assert(xanBitSet_isBitSet(region->insts, blocked->ID)); */
/*       assert(xanHashTable_lookup(blockedRegions, blocked) == NULL); */
/*       xanHashTable_insert(blockedRegions, blocked, region); */
/*       assert(xanHashTable_lookup(blockedRegions, blocked) == region); */
/*     } */
/*   } */

/*   /\* Check the blocked regions. *\/ */
/* #ifdef DEBUG */
/*   XanBitSet *checkBlockedInsts = xanBitSet_new(numInsts); */
/*   XanHashTableItem *blockedItem = xanHashTable_first(blockedRegions); */
/*   while (blockedItem) { */
/*     ir_instruction_t *inst = blockedItem->elementID; */
/*     blocked_region_t *region = blockedItem->element; */
/*     assert(region); */
/*     if (xanBitSet_isBitSet(checkBlockedInsts, inst->ID)) { */
/*       assert(xanBitSet_isSubSetOf(region->insts, checkBlockedInsts)); */
/*     } else { */
/*       assert(xanBitSet_isIntersectionEmpty(region->insts, checkBlockedInsts)); */
/*       xanBitSet_union(checkBlockedInsts, region->insts); */
/*     } */
/*     blockedItem = xanHashTable_next(blockedRegions, blockedItem); */
/*   } */
/*   assert(xanBitSet_isSubSetOf(checkBlockedInsts, blockedInsts)); */
/*   xanBitSet_free(checkBlockedInsts); */
/* #endif */

/*   /\* Return blocked regions. *\/ */
/*   return blockedRegions; */
/* } */


/* /\** */
/*  * Determine whether all instructions in 'instList' lie within 'block'. */
/*  * */
/*  * Returns JITTRUE if all instructions lie within 'block', JITFALSE otherwise. */
/*  **\/ */
/* static JITBOOLEAN */
/* allInstsInListLieWithinBlock(XanList *instList, IRBasicBlock *block) */
/* { */
/*   XanListItem *instItem = xanList_first(instList); */
/*   while (instItem) { */
/*     ir_instruction_t *inst = instItem->data; */
/*     if (inst->ID < block->startInst || block->endInst < inst->ID) { */
/*       return JITFALSE; */
/*     } */
/*     instItem = instItem->next; */
/*   } */
/*   return JITTRUE; */
/* } */


/* /\** */
/*  * Determine whether all instructions in 'instSet' lie within 'block'. */
/*  * */
/*  * Returns JITTRUE if all instructions lie within 'block', JITFALSE otherwise. */
/*  **\/ */
/* static JITBOOLEAN */
/* allInstsInBitSetLieWithinBlock(JITUINT32 numInsts, XanBitSet *instSet, IRBasicBlock *block) */
/* { */
/*   JITUINT32 i; */
/*   for (i = 0; i < numInsts; ++i) { */
/*     if (i < block->startInst || block->endInst < i) { */
/*       return JITFALSE; */
/*     } */
/*   } */
/*   return JITTRUE; */
/* } */


/**
 * Free a scheduling information structure.
 **/
static void
freeScheduleInfo(schedule_info_t *sched) {
    JITUINT32 i;
    schedule_t *schedule = sched->schedule;
    if (sched->movingReachedInsts) {
        for (i = 0; i < schedule->numInsts; ++i) {
            if (xanBitSet_isBitSet(schedule->regionInsts, i)) {
                assert(sched->movingReachedInsts[i]);
                xanBitSet_free(sched->movingReachedInsts[i]);
            } else {
                assert(!sched->movingReachedInsts[i]);
            }
        }
        freeFunction(sched->movingReachedInsts);
    }
    if (sched->branchingReachedInsts) {
        for (i = 0; i < schedule->numInsts; ++i) {
            if (xanBitSet_isBitSet(schedule->regionInsts, i)) {
                assert(sched->branchingReachedInsts[i]);
                xanBitSet_free(sched->branchingReachedInsts[i]);
            } else {
                assert(!sched->branchingReachedInsts[i]);
            }
        }
        freeFunction(sched->branchingReachedInsts);
    }
    /* for (i = 0; i < sched->schedule->numInsts; ++i) { */
    /*   xanBitSet_free(sched->dataDepGraph[i]); */
    /* } */
    /* freeFunction(sched->dataDepGraph); */
    freeFunction(sched);
}


/**
 * Initialise a scheduling information structure.
 *
 * Returns the newly-created information structure.
 **/
static schedule_info_t *
initialiseScheduleInfoForManyMove(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *nextToInsts, move_dir_t direction, place_pos_t position) {
    schedule_info_t *sched;

    /* Construct the information structure. */
    sched = allocFunction(sizeof(schedule_info_t));
    sched->schedule = schedule;
    sched->direction = direction;
    sched->instsToMove = instsToMove;
    sched->nextToInsts = nextToInsts;
    sched->position = position;
    sched->shouldFail = JITFALSE;

    /* Cache instructions reached. */
    sched->movingReachedInsts = buildInstsReachedWhenMoving(schedule, direction);
    sched->branchingReachedInsts = buildInstsReachedWhenBranching(schedule, direction);

    /* This schedule is ok. */
    return sched;
}


/**
 * Initialise a scheduling information structure.
 *
 * Returns the newly-created information structure.
 **/
static schedule_info_t *
initialiseScheduleInfoForMaxMove(schedule_t *schedule, ir_instruction_t *instToMove, move_dir_t direction) {
    schedule_info_t *sched;

    /* Construct the information structure. */
    sched = allocFunction(sizeof(schedule_info_t));
    sched->schedule = schedule;
    sched->direction = direction;
    sched->maxMoveInst = instToMove;
    sched->shouldFail = JITFALSE;

    /* This schedule is ok. */
    return sched;
}


/**
 * Determine whether 'block' is a fake head basic block.
 *
 * Returns JITTRUE if it is a fake head, JITFALSE otherwise.
 **/
static inline JITBOOLEAN
isFakeHeadBlock(IRBasicBlock *block) {
    if (block->startInst == -1 && block->endInst == -1) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * @brief Prints the first and the last instruction ID of the block passed as parameter
 */
void printBB(void* bb) {
    if (bb == NULL) {
        printf("NULL\n");
        return;
    }
    IRBasicBlock* block = (IRBasicBlock*) bb;
    if (isFakeHeadBlock(block)) {
        printf("(FAKE)\n");
    } else {
        printf("(%d-%d)\n", block->startInst, block->endInst);
    }
}

void
printInst(void *inst) {
    if (inst == NULL) {
        printf("NULL\n");
    } else {
        ir_instruction_t *i = (ir_instruction_t *)inst;
        printf("%u\n", i->ID);
    }
}

/**
 * @brief Recursively call 'printFunc' on each node
 */
void printNode (XanNode* r, int depth, XanBitSet *hasSiblings, void (printFunc)(void *)) {
    XanNode* c = NULL;
    int i;
    if (r == NULL) {
        return;
    }
    for (i = 0; i < depth; i++) {
        if (xanBitSet_isBitSet(hasSiblings, i)) {
            printf("| ");
        } else {
            printf("  ");
        }
    }
    printFunc(r->getData(r));
    while ((c = r->getNextChildren(r, c))) {
        if (r->getNextChildren(r, c)) {
            xanBitSet_setBit(hasSiblings, depth + 1);
            printNode(c, depth + 1, hasSiblings, printFunc);
            xanBitSet_clearBit(hasSiblings, depth + 1);
        } else {
            printNode(c, depth + 1, hasSiblings, printFunc);
        }
    }
}

static void
printTree(XanNode *tree, void (printFunc)(void *)) {
    XanBitSet *hasSiblings = xanBitSet_new(tree->getDepth(tree));
    printNode(tree, 0, hasSiblings, printFunc);
    xanBitSet_free(hasSiblings);
}

static ir_optimizer_t *irOptimizer = NULL;

void
setIrOptimizerInChiara(ir_optimizer_t *opt) {
    irOptimizer = opt;
}


/**
 * Insert 'inst' into 'instList' so that the list is kept sorted, in order
 * of increasing IDs.
 **/
static void
insertInstIntoSortedList(ir_instruction_t *inst, XanList *instList) {
    XanListItem *instItem = xanList_first(instList);
    while (instItem) {
        ir_instruction_t *currInst = instItem->data;
        if (currInst == inst) {
            return;
        } else if (currInst->ID > inst->ID) {
            xanList_insertBefore(instList, instItem, inst);
            return;
        }
        instItem = instItem->next;
    }
    xanList_append(instList, inst);
}


/**
 * Information needed when computing pre-dominators and post-dominators for
 * blocks in a region.
 **/
typedef struct dom_info_t {
    ir_method_t *method;
    IRBasicBlock *fakeHead;
    XanBitSet *startBlocks;
    XanBitSet *blocksToConsider;
} dom_info_t;


/**
 * Used to hold information when computing dominators.
 **/
static dom_info_t *domInfo;


/**
 * Get a predecessor basic block of 'elem' that is the next predecessor to
 * 'prev'.  Only consider blocks in the global domInfo structure.  This is
 * for use when calculating post-dominators.
 *
 * Modified from optimizer-predominators/src/pre_dominators.c
 *
 * Returns the next predecessor basic block.
 */
static void *
getPostDomPredBlock(void *elem, void *prev) {
    IRBasicBlock* block = elem;
    IRBasicBlock* prevPred = prev;
    if (elem == NULL) {
        return NULL;
    }

    /* Checks. */
    assert(domInfo);
    assert(domInfo->method);
    assert(domInfo->fakeHead);
    assert(domInfo->blocksToConsider);
    assert(domInfo->startBlocks);

    /* Fake head has exit blocks as predecessors. */
    if (block == domInfo->fakeHead) {
        JITUINT32 predBlockID, prevPredID = 0;
        if (prevPred) {
            prevPredID = IRMETHOD_getBasicBlockPosition(domInfo->method, prevPred);
        }
        predBlockID = xanBitSet_getFirstBitSetInRange(domInfo->startBlocks, prevPredID + 1, IRMETHOD_getNumberOfMethodBasicBlocks(domInfo->method) - 1);
        if (predBlockID == -1) {
            return NULL;
        } else {
            return IRMETHOD_getBasicBlockAtPosition(domInfo->method, predBlockID);
        }
    }

    /* Not the fake head. */
    else {
        ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(domInfo->method, block->startInst);
        ir_instruction_t* pred = NULL;

        /* Get previous predecessor. */
        if (prevPred) {
            pred = IRMETHOD_getInstructionAtPosition(domInfo->method, prevPred->endInst);
        }

        /* Find the next predecessor. */
        while ((pred = IRMETHOD_getPredecessorInstruction(domInfo->method, startInst, pred))) {
            IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(domInfo->method, pred);
            assert(IRMETHOD_getBasicBlockPosition(domInfo->method, predBlock) < IRMETHOD_getNumberOfMethodBasicBlocks(domInfo->method));
            if (xanBitSet_isBitSet(domInfo->blocksToConsider, IRMETHOD_getBasicBlockPosition(domInfo->method, predBlock))) {
                return predBlock;
            }
        }
    }

    /* No predecessor found. */
    return NULL;
}


/**
 * Get a successor basic block of 'elem' that is the next successor to
 * 'prev'.  Only consider blocks in the global domInfo structure.  This is
 * for use when calculating post-dominators.
 *
 * Modified from optimizer-predominators/src/pre_dominators.c
 *
 * Returns the next successor basic block.
 */
static void *
getPostDomSuccBlock(void *elem, void *prev) {
    IRBasicBlock* block = elem;
    IRBasicBlock* prevSucc = prev;
    if (elem == NULL) {
        return NULL;
    }

    /* Checks. */
    assert(domInfo);
    assert(domInfo->method);
    assert(domInfo->fakeHead);
    assert(domInfo->blocksToConsider);
    assert(domInfo->startBlocks);

    /* Fake head has no successors. */
    if (block == domInfo->fakeHead) {
        return NULL;
    } else {
        JITUINT32 blockID = IRMETHOD_getBasicBlockPosition(domInfo->method, block);

        /* Successors of exit blocks. */
        if (xanBitSet_isBitSet(domInfo->startBlocks, blockID)) {
            if (!prevSucc) {
                return domInfo->fakeHead;
            } else {
                return NULL;
            }
        }

        /* Other blocks. */
        else {
            ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(domInfo->method, block->endInst);
            ir_instruction_t *succ = NULL;

            /* Get previous successor. */
            if (prevSucc != NULL) {
                succ = IRMETHOD_getInstructionAtPosition(domInfo->method, prevSucc->startInst);
            }

            /* Find the next successor. */
            while ((succ = IRMETHOD_getSuccessorInstruction(domInfo->method, endInst, succ))) {
                IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(domInfo->method, succ);
                assert(IRMETHOD_getBasicBlockPosition(domInfo->method, succBlock) < IRMETHOD_getNumberOfMethodBasicBlocks(domInfo->method));
                if (xanBitSet_isBitSet(domInfo->blocksToConsider, IRMETHOD_getBasicBlockPosition(domInfo->method, succBlock))) {
                    return succBlock;
                }
            }
        }
    }

    /* No successor found. */
    return NULL;
}


/**
 * Get a predecessor basic block of 'elem' that is the next predecessor to
 * 'prev'.  Only consider blocks in the global domInfo structure.  This is
 * for use when calculating pre-dominators.
 *
 * Modified from optimizer-predominators/src/pre_dominators.c
 *
 * Returns the next predecessor basic block.
 */
static void *
getPreDomPredBlock(void *elem, void *prev) {
    IRBasicBlock* block = elem;
    IRBasicBlock* prevPred = prev;
    if (elem == NULL) {
        return NULL;
    }

    /* Checks. */
    assert(domInfo);
    assert(domInfo->method);
    assert(domInfo->fakeHead);
    assert(domInfo->blocksToConsider);
    assert(domInfo->startBlocks);

    /* Fake head has no predecessors. */
    if (block == domInfo->fakeHead) {
        return NULL;
    } else {
        JITUINT32 blockID = IRMETHOD_getBasicBlockPosition(domInfo->method, block);

        /* Predecessors of entry blocks. */
        if (xanBitSet_isBitSet(domInfo->startBlocks, blockID)) {
            if (!prevPred) {
                return domInfo->fakeHead;
            } else {
                return NULL;
            }
        }

        /* Other blocks. */
        else {
            ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(domInfo->method, block->startInst);
            ir_instruction_t *pred = NULL;

            /* Get previous predecessor. */
            if (prevPred) {
                pred = IRMETHOD_getInstructionAtPosition(domInfo->method, prevPred->endInst);
            }

            /* Find the next predecessor. */
            while ((pred = IRMETHOD_getPredecessorInstruction(domInfo->method, startInst, pred))) {
                IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(domInfo->method, pred);
                assert(IRMETHOD_getBasicBlockPosition(domInfo->method, predBlock) < IRMETHOD_getNumberOfMethodBasicBlocks(domInfo->method));
                if (xanBitSet_isBitSet(domInfo->blocksToConsider, IRMETHOD_getBasicBlockPosition(domInfo->method, predBlock))) {
                    return predBlock;
                }
            }
        }
    }

    /* No predecessor found. */
    return NULL;
}


/**
 * Get a successor basic block of 'elem' that is the next successor to
 * 'prev'.  Only consider blocks in the global domInfo structure.  This is
 * for use when calculating pre-dominators.
 *
 * Modified from optimizer-predominators/src/pre_dominators.c
 *
 * Returns the next successor basic block.
 */
static void *
getPreDomSuccBlock(void *elem, void *prev) {
    IRBasicBlock* block = elem;
    IRBasicBlock* prevSucc = prev;
    if (elem == NULL) {
        return NULL;
    }

    /* Checks. */
    assert(domInfo);
    assert(domInfo->method);
    assert(domInfo->fakeHead);
    assert(domInfo->blocksToConsider);
    assert(domInfo->startBlocks);

    /* Fake head has entry blocks as successors. */
    if (block == domInfo->fakeHead) {
        JITUINT32 succBlockID, prevSuccID = 0;
        if (prevSucc) {
            prevSuccID = IRMETHOD_getBasicBlockPosition(domInfo->method, prevSucc);
        }
        succBlockID = xanBitSet_getFirstBitSetInRange(domInfo->startBlocks, prevSuccID + 1, IRMETHOD_getNumberOfMethodBasicBlocks(domInfo->method) - 1);
        if (succBlockID == -1) {
            return NULL;
        } else {
            return IRMETHOD_getBasicBlockAtPosition(domInfo->method, succBlockID);
        }
    }

    /* Not the fake head. */
    else {
        ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(domInfo->method, block->endInst);
        ir_instruction_t *succ = NULL;

        /* Get previous successor. */
        if (prevSucc != NULL) {
            succ = IRMETHOD_getInstructionAtPosition(domInfo->method, prevSucc->startInst);
        }

        /* Find the next successor. */
        while ((succ = IRMETHOD_getSuccessorInstruction(domInfo->method, endInst, succ))) {
            IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(domInfo->method, succ);
            assert(IRMETHOD_getBasicBlockPosition(domInfo->method, succBlock) < IRMETHOD_getNumberOfMethodBasicBlocks(domInfo->method));
            if (xanBitSet_isBitSet(domInfo->blocksToConsider, IRMETHOD_getBasicBlockPosition(domInfo->method, succBlock))) {
                return succBlock;
            }
        }
    }

    /* No successor found. */
    return NULL;
}


/**
 * Append an edge to a list of edges, only if it is unique in the list.
 **/
static void
appendEdgeToListUnique(XanList *edgeList, edge_t *edge) {
    XanListItem *checkItem = xanList_first(edgeList);
    while (checkItem) {
        edge_t *check = checkItem->data;
        if (check->pred == edge->pred && check->succ == edge->succ) {
            freeFunction(edge);
            return;
        }
        checkItem = checkItem->next;
    }
    xanList_append(edgeList, edge);
}


/**
 * Build a list of edges where clones of basic block number 'blockNum'
 * should be placed when moving them upwards to 'nextToInst' within
 * 'sched'.  These are edges where the successor instruction is marked
 * within 'regionInsts' in 'sched' but the predecessor is not.
 *
 * Returns the list of edges where clones should be placed.
 **/
static XanList *
buildUpwardsCloneLocationListForBlock(schedule_info_t *sched, JITUINT32 blockNum) {
    JITINT32 n = -1;
    XanBitSet *seenBlocks;
    XanList *blocksToProcess;
    XanList *cloneEdges;
    IRBasicBlock *blockToClone;
    schedule_t *schedule = sched->schedule;

    /* Early exit. */
    blockToClone = IRMETHOD_getBasicBlockAtPosition(schedule->method, blockNum);
    while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
        if (blockToClone->startInst <= n && n <= blockToClone->endInst) {
            return NULL;
        }
    }
    PDEBUG(1, "  Marking clone locations for block (%u-%u)\n", blockToClone->startInst, blockToClone->endInst);

    /* A list of edges that need clones, for this basic block. */
    cloneEdges = newList();

    /* A record of basic blocks seen. */
    seenBlocks = xanBitSet_new(schedule->numBlocks);
    xanBitSet_setBit(seenBlocks, blockNum);

    /* Add blocks containing entry instructions. */
    while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
        IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstructionPosition(schedule->method, n);
        xanBitSet_setBit(seenBlocks, IRMETHOD_getBasicBlockPosition(schedule->method, block));
    }

    /* Blocks that need to be processed. */
    blocksToProcess = newList();
    xanList_append(blocksToProcess, blockToClone);

    /* Check for leaving the instructions to consider. */
    while (xanList_length(blocksToProcess) > 0) {
        XanListItem *blockItem;
        IRBasicBlock *block;

        /* Get this basic block. */
        blockItem = xanList_first(blocksToProcess);
        block = blockItem->data;
        xanList_deleteItem(blocksToProcess, blockItem);
        PDEBUG(2, "    Checking block (%u-%u)\n", block->startInst, block->endInst);

        /* See if the edge is within this block. */
        if (!xanBitSet_isBitSet(schedule->regionInsts, block->startInst)) {
            JITUINT32 i;
            assert(xanBitSet_isBitSet(schedule->regionInsts, block->endInst));
            for (i = block->startInst + 1; i <= block->endInst; ++i) {
                if (xanBitSet_isBitSet(schedule->regionInsts, i)) {
                    edge_t *crossing;
                    ir_instruction_t *pred = IRMETHOD_getInstructionAtPosition(schedule->method, i - 1);
                    ir_instruction_t *succ = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                    crossing = createEdge(pred, succ);
                    appendEdgeToListUnique(cloneEdges, crossing);
                    PDEBUG(2, "    Need clones on mid-block edge %u -> %u\n", pred->ID, succ->ID);
                }
            }
        }

        /* Check the edges before this block. */
        else {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, block->startInst);
            XanListItem *predItem = xanList_first(schedule->predecessors[inst->ID]);
            while (predItem) {
                ir_instruction_t *pred = predItem->data;
                if (xanBitSet_isBitSet(schedule->regionInsts, pred->ID)) {
                    IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);
                    JITINT32 predBlockNum = IRMETHOD_getBasicBlockPosition(schedule->method, predBlock);
                    assert(predBlockNum < schedule->numBlocks);
                    if (!xanBitSet_isBitSet(seenBlocks, predBlockNum)) {
                        xanList_append(blocksToProcess, predBlock);
                        xanBitSet_setBit(seenBlocks, predBlockNum);
                    }
                }

                /* Leaves instructions to consider. */
                else {
                    edge_t *crossing = createEdge(pred, inst);
                    appendEdgeToListUnique(cloneEdges, crossing);
                    PDEBUG(2, "    Need clones on edge %u -> %u\n", pred->ID, inst->ID);
                }
                predItem = predItem->next;
            }
        }
    }

    /* Clean up. */
    freeList(blocksToProcess);
    xanBitSet_free(seenBlocks);

    /* Return the list of edges needing clones. */
    return cloneEdges;
}


/**
 * Build a list of edges where clones of basic block number 'blockNum'
 * should be placed when moving them downwards to 'nextToInst' within
 * 'sched'.  These are edges where the predecessor instruction is marked
 * within 'regionInsts' in 'sched' but the successor is not.
 *
 * Returns the list of edges where clones should be placed.
 **/
static XanList *
buildDownwardsCloneLocationListForBlock(schedule_info_t *sched, JITUINT32 blockNum) {
    JITINT32 n = -1;
    XanBitSet *seenBlocks;
    XanList *blocksToProcess;
    XanList *cloneEdges;
    IRBasicBlock *blockToClone;
    schedule_t *schedule = sched->schedule;

    /* Early exit. */
    blockToClone = IRMETHOD_getBasicBlockAtPosition(schedule->method, blockNum);
    while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
        if (blockToClone->startInst <= n && n <= blockToClone->endInst) {
            return NULL;
        }
    }
    PDEBUG(1, "  Marking clone locations for block (%u-%u)\n", blockToClone->startInst, blockToClone->endInst);

    /* A list of edges that need clones, for this basic block. */
    cloneEdges = newList();

    /* A record of basic blocks seen. */
    seenBlocks = xanBitSet_new(schedule->numBlocks);
    xanBitSet_setBit(seenBlocks, blockNum);

    /* Add blocks containing instructions to go next to. */
    while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
        IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstructionPosition(schedule->method, n);
        xanBitSet_setBit(seenBlocks, IRMETHOD_getBasicBlockPosition(schedule->method, block));
    }

    /* Blocks that need to be processed. */
    blocksToProcess = newList();
    xanList_append(blocksToProcess, blockToClone);

    /* Check for leaving the instructions to consider. */
    while (xanList_length(blocksToProcess) > 0) {
        XanListItem *blockItem;
        IRBasicBlock *block;

        /* Get this basic block. */
        blockItem = xanList_first(blocksToProcess);
        block = blockItem->data;
        xanList_deleteItem(blocksToProcess, blockItem);
        PDEBUG(2, "    Checking block (%u-%u)\n", block->startInst, block->endInst);

        /* See if the edge is within this block. */
        if (!xanBitSet_isBitSet(schedule->regionInsts, block->endInst)) {
            JITUINT32 i;
            assert(xanBitSet_isBitSet(schedule->regionInsts, block->startInst));
            for (i = block->startInst + 1; i <= block->endInst; ++i) {
                if (!xanBitSet_isBitSet(schedule->regionInsts, i)) {
                    edge_t *crossing;
                    ir_instruction_t *pred = IRMETHOD_getInstructionAtPosition(schedule->method, i - 1);
                    ir_instruction_t *succ = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                    crossing = createEdge(pred, succ);
                    appendEdgeToListUnique(cloneEdges, crossing);
                    PDEBUG(2, "    Need clones on mid-block edge %u -> %u\n", pred->ID, succ->ID);
                }
            }
        }

        /* Check the edges after this block. */
        else {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
            XanListItem *succItem = xanList_first(schedule->successors[inst->ID]);
            while (succItem) {
                ir_instruction_t *succ = succItem->data;
                if (succ->ID >= schedule->numInsts) {
                    succItem = succItem->next;
                    continue;
                } else if (xanBitSet_isBitSet(schedule->regionInsts, succ->ID)) {
                    IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                    JITINT32 succBlockNum = IRMETHOD_getBasicBlockPosition(schedule->method, succBlock);
                    assert(succBlockNum < schedule->numBlocks);
                    if (!xanBitSet_isBitSet(seenBlocks, succBlockNum)) {
                        xanList_append(blocksToProcess, succBlock);
                        xanBitSet_setBit(seenBlocks, succBlockNum);
                    }
                }

                /* Leaves instructions to consider. */
                else {
                    edge_t *crossing = createEdge(inst, succ);
                    appendEdgeToListUnique(cloneEdges, crossing);
                    PDEBUG(2, "    Need clones on edge %u -> %u\n", inst->ID, succ->ID);
                }
                succItem = succItem->next;
            }
        }
    }

    /* Clean up. */
    freeList(blocksToProcess);
    xanBitSet_free(seenBlocks);

    /* Return the list of edges needing clones. */
    return cloneEdges;
}


static void
addInstToInstListMapping(XanHashTable *table, ir_instruction_t *inst, ir_instruction_t *mappedTo) {
    XanList *instList = xanHashTable_lookup(table, inst);
    if (!instList) {
        instList = newList();
        xanHashTable_insert(table, inst, instList);
    }
    xanList_append(instList, mappedTo);
}


/**
 * Get a label's value.
 **/
static inline IR_ITEM_VALUE
getLabelValue(ir_instruction_t *label) {
    return label->param_1.value.v;
}


static XanHashTable *
cloneInstsOnSingleEdge(schedule_info_t *sched, XanList *instsToClone, ir_instruction_t *entryInst) {
    ir_instruction_t *prevClone;
    XanHashTable *clonedInstMap;
    XanListItem *instItem;

    /* Keep a track of clones. */
    clonedInstMap = newHashTable();

    /* Clone each instruction and place after previous. */
    prevClone = entryInst;
    instItem = xanList_first(instsToClone);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        ir_instruction_t *clone;
        if (inst->type == IRLABEL) {
            clone = IRMETHOD_newLabelAfter(sched->schedule->method, prevClone);
            PDEBUG(2, "    Cloned label %u (%s), as %u (%llu)\n", inst->ID, IRMETHOD_getInstructionTypeName(inst->type), clone->ID, getLabelValue(clone));
        } else {
            clone = IRMETHOD_cloneInstruction(sched->schedule->method, inst);
            IRMETHOD_moveInstructionAfter(sched->schedule->method, clone, prevClone);
            PDEBUG(2, "    Cloned inst %u (%s), as %u\n", inst->ID, IRMETHOD_getInstructionTypeName(inst->type), clone->ID);
        }
        xanHashTable_insert(clonedInstMap, inst, clone);
        prevClone = clone;
        instItem = instItem->next;
    }
    /*   IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER); */

    /* Return the mapping to clones. */
    return clonedInstMap;
}


/**
 * Return a label for use as a target for 'inst'.  If 'inst' is already a
 * label then this is returned.  If the immediate predecessor of 'inst' is
 * a label then that is used.  Otherwise a new label is created before
 * 'inst' and that is returned.
 *
 * Returns a label to use as a target.
 **/
static ir_instruction_t *
getTargetLabel(ir_method_t *method, ir_instruction_t *inst) {
    ir_instruction_t *label = inst;
    if (inst->type != IRLABEL) {
        ir_instruction_t *pred = IRMETHOD_getPredecessorInstruction(method, inst, NULL);
        if (!pred || pred->type != IRLABEL) {
            label = IRMETHOD_newLabelBefore(method, inst);
            PDEBUG(3, "  Created new label %u (label %llu)\n", label->ID, getLabelValue(label));
        } else {
            label = pred;
        }
    }
    return label;
}


static void
fixClonedTargets(schedule_info_t *sched, XanList *instsToClone, XanHashTable *clonedInstMap, ir_instruction_t *exitLabel, XanHashTable *succMap) {
    XanListItem *instItem;
    schedule_t *schedule = sched->schedule;

    /* Fix up successor relations. */
    instItem = xanList_first(instsToClone);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        ir_instruction_t *clone = xanHashTable_lookup(clonedInstMap, inst);
        succ_relation_t *succRelation = xanHashTable_lookup(succMap, inst);
        PDEBUG(1, "  Checking clone %u\n", clone->ID);

        /* The end instruction in a basic block. */
        if (succRelation) {

            /* Fix the branch target. */
            if (succRelation->target) {
                ir_instruction_t *clonedTarget = xanHashTable_lookup(clonedInstMap, succRelation->target);
                assert(IRMETHOD_isABranchInstruction(clone));

                /* Branch to cloned target, could be a label. */
                if (clonedTarget) {
                    ir_instruction_t *targetLabel = getTargetLabel(schedule->method, clonedTarget);
                    PDEBUG(1, "    Should target cloned %u\n", clonedTarget->ID);
                    if (IRMETHOD_getBranchDestination(schedule->method, clone) != targetLabel) {
                        IRMETHOD_setBranchDestination(schedule->method, clone, getLabelValue(targetLabel));
                        PDEBUG(2, "  Fixed up branch %u to cloned inst %u (label %llu)\n", clone->ID, targetLabel->ID, getLabelValue(targetLabel));
                    }
                }

                /* Branch to non-cloned instruction. */
                else {
                    IRMETHOD_setBranchDestination(schedule->method, clone, exitLabel->param_1.value.v);
                    PDEBUG(2, "  Fixed up branch %u to exit %u (label %llu)\n", clone->ID, exitLabel->ID, exitLabel->param_1.value.v);
                }
            }

            /* Fix fall-through. */
            if (succRelation->fallThrough) {
                ir_instruction_t *clonedFallThrough = xanHashTable_lookup(clonedInstMap, succRelation->fallThrough);

                /* Fall-through to cloned instruction. */
                if (clonedFallThrough) {
                    ir_instruction_t *succ = NULL;
                    PDEBUG(3, "    Should fall through to %u\n", clonedFallThrough->ID);

                    /* Get the correct successor. */
                    if (IRMETHOD_isABranchInstruction(clone)) {
                        assert(clone->type != IRBRANCH);
                        succ = IRMETHOD_getBranchFallThrough(schedule->method, clone);
                    } else {
                        if (IRMETHOD_isACallInstruction(clone)) {
                            assert(IRMETHOD_getSuccessorsNumber(schedule->method, clone) == 2);
                            while ((succ = IRMETHOD_getSuccessorInstruction(schedule->method, clone, succ))) {
                                if (succ->type != IREXITNODE) {
                                    break;
                                }
                            }
                        } else {
                            assert(IRMETHOD_getSuccessorsNumber(schedule->method, clone) == 1);
                            succ = IRMETHOD_getSuccessorInstruction(schedule->method, clone, NULL);
                        }
                    }

                    /* See if we need to patch up. */
                    if (succ != clonedFallThrough) {
                        ir_instruction_t *targetLabel = getTargetLabel(schedule->method, clonedFallThrough);
                        IRMETHOD_newBranchToLabelAfter(schedule->method, targetLabel, clone);
                        PDEBUG(2, "  Added new branch after %u to %u (label %llu)\n", clone->ID, targetLabel->ID, targetLabel->param_1.value.v);
                    }
                }

                /* Fall-through to non-cloned instruction. */
                else if (!IRMETHOD_isASuccessorInstruction(schedule->method, clone, exitLabel)) {
                    IRMETHOD_newBranchToLabelAfter(schedule->method, exitLabel, clone);
                    PDEBUG(2, "  Fixed up fall-through %u to exit %u (label %llu)\n", clone->ID, exitLabel->ID, exitLabel->param_1.value.v);
                }
            }
        }

        /* Next instruction. */
        instItem = instItem->next;
    }

    /* If the exit label is not the target of a branch, remove it. */
    /* if (IRMETHOD_getPredecessorsNumber(schedule->method, exitLabel) == 1) { */
    /*   ir_instruction_t *pred = IRMETHOD_getPredecessorInstruction(schedule->method, exitLabel, NULL); */
    /*   if (!IRMETHOD_isABranchInstruction(pred) || IRMETHOD_getBranchDestination(schedule->method, pred) != exitLabel) { */
    /*     PDEBUG(3, "  Removing redundant exit %u (label %lld)\n", exitLabel->ID, getLabelValue(exitLabel)); */
    /*     IRMETHOD_deleteInstruction(schedule->method, exitLabel); */
    /*     if (xanHashTable_lookup(clonedInstMap, exitLabel) == NULL) { */
    /*       xanHashTable_insert(clonedInstMap, exitLabel, newList()); */
    /*     } */
    /*   } */
    /* } */
}


/**
 * Patch up a branch to point to a label.  Creates a new branch, if necessary.
 *
 * Returns any newly-created branch.
 **/
static ir_instruction_t *
createBranchToLabel(ir_method_t *method, ir_instruction_t *inst, ir_instruction_t *dest, ir_instruction_t *label) {
    ir_instruction_t *branch = NULL;
    if (!IRMETHOD_isABranchInstruction(inst)) {
        branch = IRMETHOD_newBranchToLabelAfter(method, label, inst);
        PDEBUG(3, "    Created new branch %u to %u (label %llu)\n", branch->ID, label->ID, getLabelValue(label));
    } else if (IRMETHOD_getBranchDestination(method, inst) == dest) {
        IRMETHOD_setBranchDestination(method, inst, getLabelValue(label));
        PDEBUG(3, "    Set branch %u to %u (label %llu)\n", inst->ID, label->ID, getLabelValue(label));
    } else {
        branch = IRMETHOD_newBranchToLabelAfter(method, label, inst);
        PDEBUG(3, "    Created new branch %u to %u (label %llu)\n", branch->ID, label->ID, getLabelValue(label));
    }
    return branch;
}


/**
 * Fix up the entry to the instructions that have just been cloned, ensuring
 * that the edge predecessor executed the cloned entry instruction first.
 **/
static void
fixEntryToClones(schedule_info_t *sched, edge_t *edge, ir_instruction_t *firstToExec, XanHashTable *clonedInstMap) {
    schedule_t *schedule = sched->schedule;
    PDEBUG(3, "  Fixing entry to clones from edge predecessor %u to entry inst %u\n", edge->pred->ID, firstToExec->ID);

    /* Ensure no clones branch to the edge predecessor (if it's a label). */
    if (edge->pred->type == IRLABEL) {
        XanHashTableItem *clonedItem = xanHashTable_first(clonedInstMap);
        while (clonedItem) {
            ir_instruction_t *clone = clonedItem->element;
            if (IRMETHOD_isASuccessorInstruction(schedule->method, clone, edge->pred)) {
                ir_instruction_t *succ = IRMETHOD_getSuccessorInstruction(schedule->method, edge->pred, NULL);
                ir_instruction_t *label = IRMETHOD_newLabelBefore(schedule->method, succ);
                createBranchToLabel(schedule->method, clone, edge->pred, label);
            }
            clonedItem = xanHashTable_next(clonedInstMap, clonedItem);
        }
    }

    /* Ensure the edge predecessor executes the first instruction. */
    if (!IRMETHOD_isASuccessorInstruction(schedule->method, edge->pred, firstToExec)) {
        ir_instruction_t *label = firstToExec;

        /* Always insert a new label if the first instruction isn't one. */
        if (firstToExec->type != IRLABEL) {
            label = IRMETHOD_newLabelBefore(schedule->method, firstToExec);
            PDEBUG(3, "    Created new label %u (label %llu) before first cloned %u\n", label->ID, getLabelValue(label), firstToExec->ID);
        }

        /* Branch to this label. */
        createBranchToLabel(schedule->method, edge->pred, edge->succ, label);
    }
}


/**
 * Clone a single instruction on a single edge.  If the edge follows a branch
 * then add in any labels and unconditional branch instructions necessary to
 * keep the program semantics.
 *
 * Returns the cloned instruction.
 **/
static ir_instruction_t *
cloneSingleInstOnEdge(schedule_info_t *sched, ir_instruction_t *inst, schedule_edge_t *edge) {
    schedule_t *schedule = sched->schedule;
    ir_instruction_t *exitInst JITUNUSED;
    ir_instruction_t *entryInst;
    ir_instruction_t *clone;

    assert(IRMETHOD_isAPredecessorInstruction(schedule->method, edge->succ, edge->pred));
    PDEBUG(2, "  Cloning instruction on edge %u -> %u\n", edge->pred->ID, edge->succ->ID);
    if (IRMETHOD_isABranchInstruction(edge->pred)) {
        PDECL(ir_instruction_t *label = IRMETHOD_getBranchDestination(schedule->method, edge->pred));
        PDEBUG(2, "    Branch inst %u currently taken to inst %u (label %llu)\n", edge->pred->ID, label->ID, label->param_1.value.v);
    }

    /* Branch edge, fall-throughs are treated differently. */
    if (IRMETHOD_isABranchInstruction(edge->pred) && IRMETHOD_getBranchDestination(schedule->method, edge->pred) == edge->succ) {
        assert(edge->succ->type == IRLABEL);
        if (edge->pred->type == IRBRANCH) {
            assert(IRMETHOD_getPredecessorsNumber(schedule->method, edge->pred) == 1);
            entryInst = IRMETHOD_getPredecessorInstruction(schedule->method, edge->pred, NULL);
            exitInst = edge->pred;
        } else {
            entryInst = IRMETHOD_newLabel(schedule->method);
            IRMETHOD_substituteLabel(schedule->method, edge->pred, edge->succ->param_1.value.v, entryInst->param_1.value.v);
            PDEBUG(2, "    New label %u (val %llu) substituted into edge entry\n", entryInst->ID, entryInst->param_1.value.v);
            exitInst = IRMETHOD_newBranchToLabelAfter(schedule->method, edge->succ, entryInst);
        }
    }

    /* Fall-through or non-branch edge. */
    else {
        entryInst = edge->pred;
        exitInst = edge->succ;
    }

    /* Perform the clone. */
    clone = IRMETHOD_cloneInstruction(schedule->method, inst);
    IRMETHOD_moveInstructionAfter(schedule->method, clone, entryInst);

    /* Check and return the clone. */
    assert(IRMETHOD_isAPredecessorInstruction(schedule->method, clone, entryInst));
    assert(IRMETHOD_isASuccessorInstruction(schedule->method, clone, exitInst));
    return clone;
}


/**
 * Set up an edge so instructions can be cloned on it.
 **/
static void
setupEdgeForCloning(schedule_info_t *sched, edge_t *edge, ir_instruction_t **entryInst, ir_instruction_t **exitLabel) {
    schedule_t *schedule = sched->schedule;
    assert(IRMETHOD_isAPredecessorInstruction(schedule->method, edge->succ, edge->pred));
    PDEBUG(2, "  Will clone instructions on edge %u -> %u\n", edge->pred->ID, edge->succ->ID);
    if (IRMETHOD_isABranchInstruction(edge->pred)) {
        PDECL(ir_instruction_t *label = IRMETHOD_getBranchDestination(schedule->method, edge->pred));
        PDEBUG(2, "    Branch inst %u currently taken to inst %u (label %llu)\n", edge->pred->ID, label->ID, label->param_1.value.v);
    }

    /* Branch edge, fall-throughs are treated differently. */
    if (IRMETHOD_isABranchInstruction(edge->pred) && IRMETHOD_getBranchDestination(schedule->method, edge->pred) == edge->succ) {

        /* A new label on the edge. */
        *entryInst = IRMETHOD_newLabelBefore(schedule->method, edge->succ);

        /* Substitute label in old branch. */
        IRMETHOD_substituteLabel(schedule->method, edge->pred, edge->succ->param_1.value.v, (*entryInst)->param_1.value.v);
        PDEBUG(2, "      New label %u (val %llu) substituted into edge entry\n", (*entryInst)->ID, (*entryInst)->param_1.value.v);
        PDEBUG(3, "      Branch inst %u now taken to inst %u (label %llu)\n", edge->pred->ID, IRMETHOD_getBranchDestination(schedule->method, edge->pred)->ID, IRMETHOD_getBranchDestination(schedule->method, edge->pred)->param_1.value.v);

        /* Remember the new edge predecessor. */
        edge->pred = *entryInst;

        /* The label to exit to. */
        *exitLabel = edge->succ;
    }

    /* Fall-through edge. */
    else {

        /* The entry instruction (clones placed after this). */
        *entryInst = edge->pred;

        /* A label to exit to. */
        if (edge->succ->type == IRLABEL) {
            *exitLabel = edge->succ;
        } else {
            *exitLabel = IRMETHOD_newLabelBefore(schedule->method, edge->succ);
            PDEBUG(2, "      New label %u (val %llu) inserted as edge exit\n", (*exitLabel)->ID, (*exitLabel)->param_1.value.v);
        }
    }
}


/**
 * Place clones of instructions from 'instsToClone' in new basic blocks
 * created along the edges within 'edgeTable'.  This maps basic blocks to
 * a list of edges that will contain the cloned instructions.
 **/
static void
cloneInstsOnEdges(schedule_info_t *sched, XanHashTable *edgeTable, XanHashTable *edgeEntryMap, XanHashTable *succMap, XanHashTable *clonedInstMap) {
    XanHashTable *edgeCloneMap;
    XanHashTableItem *tableItem;
    XanHashTableItem *cloneItem;
    PDEBUG(1, "Cloning instructions\n");

    /* Create new basic blocks within the edges. */
    tableItem = xanHashTable_first(edgeTable);
    while (tableItem) {
        edge_t *edge = tableItem->elementID;
        XanList *instList = tableItem->element;
        ir_instruction_t *firstToClone;
        ir_instruction_t *entryInst;
        ir_instruction_t *exitLabel;

        /* Set everything up. */
        setupEdgeForCloning(sched, edge, &entryInst, &exitLabel);
        firstToClone = xanHashTable_lookup(edgeEntryMap, edge);

        /* Now clone the whole subgraph. */
        edgeCloneMap = cloneInstsOnSingleEdge(sched, instList, entryInst);
        fixClonedTargets(sched, instList, edgeCloneMap, exitLabel, succMap);
        fixEntryToClones(sched, edge, xanHashTable_lookup(edgeCloneMap, firstToClone), edgeCloneMap);
        if (0) {
            char *prevDotPrinterName = getenv("DOTPRINTER_FILENAME");
            setenv("DOTPRINTER_FILENAME", "Chiara_clone", 1);
            IROPTIMIZER_callMethodOptimization(irOptimizer, sched->schedule->method, METHOD_PRINTER);
            setenv("DOTPRINTER_FILENAME", prevDotPrinterName, 1);
        }

        /* Add clones to the global table. */
        cloneItem = xanHashTable_first(edgeCloneMap);
        while (cloneItem) {
            ir_instruction_t *inst = cloneItem->elementID;
            ir_instruction_t *clone = cloneItem->element;
            addInstToInstListMapping(clonedInstMap, inst, clone);
            cloneItem = xanHashTable_next(edgeCloneMap, cloneItem);
        }
        freeHashTable(edgeCloneMap);

        /* Next edge. */
        tableItem = xanHashTable_next(edgeTable, tableItem);
    }
}


/**
 * Find the first instruction to clone from 'instsToClone' along 'edge'.  This
 * is basically the only instruction that can reach all other instructions to
 * clone.
 **/
static ir_instruction_t *
findClonesEntryInst(schedule_t *schedule, edge_t *edge, XanList *instsToClone) {
    XanListItem *instItem = xanList_first(instsToClone);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        XanListItem *checkItem = xanList_first(instsToClone);
        JITBOOLEAN reachesAll = JITTRUE;
        while (checkItem && reachesAll) {
            ir_instruction_t *check = checkItem->data;
            if (!IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, check, inst)) {
                reachesAll = JITFALSE;
            }
            checkItem = checkItem->next;
        }
        if (reachesAll) {
            return inst;
        }
        instItem = instItem->next;
    }

    /* We should never get here. */
    abort();
    return NULL;
}


/**
 * Find the entry instruction into the instructions to clone for each edge that
 * instructions are cloned along.
 **/
static XanHashTable *
createEdgeCloneEntryMap(schedule_info_t *sched, XanHashTable *edgeTable) {
    XanHashTable *edgeEntryMap = newHashTable();
    XanHashTableItem *tableItem = xanHashTable_first(edgeTable);
    while (tableItem) {
        edge_t *edge = tableItem->elementID;
        XanList *instList = tableItem->element;
        ir_instruction_t *firstToClone = findClonesEntryInst(sched->schedule, edge, instList);
        assert(!xanHashTable_lookup(edgeEntryMap, edge));
        xanHashTable_insert(edgeEntryMap, edge, firstToClone);
        PDEBUG(3, "Edge (%u-%u) entry inst %u\n", edge->pred->ID, edge->succ->ID, firstToClone->ID);
        tableItem = xanHashTable_next(edgeTable, tableItem);
    }
    return edgeEntryMap;
}


/* /\** */
/*  * Mark the bits in 'instSet' and 'blockSet' for each instruction in */
/*  * 'instList', corresponding to its instruction ID and block ID. */
/*  **\/ */
/* static void */
/* markInstsAndBlocks(ir_method_t *method, XanBitSet *instSet, XanBitSet *blockSet, XanList *instList) */
/* { */
/*   XanListItem *instItem = xanList_first(instList); */
/*   while (instItem) { */
/*     ir_instruction_t *inst = instItem->data; */
/*     IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(method, inst); */
/*     JITUINT32 blockNum = IRMETHOD_getBasicBlockPosition(method, block); */
/*     assert(inst->ID < IRMETHOD_getInstructionsNumber(method)); */
/*     assert(blockNum < IRMETHOD_getNumberOfMethodBasicBlocks(method)); */
/*     xanBitSet_setBit(instSet, inst->ID); */
/*     xanBitSet_setBit(blockSet, blockNum); */
/*     instItem = instItem->next; */
/*   } */
/* } */


/* /\** */
/*  * Mark bits in 'instSet' and 'blockSet' for instructions and blocks that */
/*  * are control dependent on conditional branches in 'instList'.  This is */
/*  * all blocks that do not post-dominate those branches. */
/*  **\/ */
/* static void */
/* markInstsDependentOnConditionalBranch(schedule_info_t *sched, XanBitSet *instSet, XanBitSet *blockSet, XanList *instList, XanList **successors) */
/* { */
/*   XanBitSet *seenInsts = xanBitSet_new(sched->schedule->numInsts); */
/*   XanListItem *branchItem = xanList_first(instList); */
/*   while (branchItem) { */
/*     ir_instruction_t *branch = branchItem->data; */
/*     if (IRMETHOD_isABranchInstruction(branch) && branch->type != IRBRANCH) { */
/*       XanList *instsToCheck = newList(); */
/*       XanListItem *checkItem; */
/*       xanList_append(instsToCheck, branch); */
/*       checkItem = xanList_first(instsToCheck); */
/*       while (checkItem) { */
/*         XanListItem	*succItem; */
/*         ir_instruction_t *check = checkItem->data; */
/*     	succItem	= xanList_first(successors[check->ID]); */
/* 	while (succItem != NULL){ */
/*  	  ir_instruction_t 	*nextTo; */
/*           nextTo	= succItem->data; */
/*           assert(nextTo != NULL); */
/*           if (nextTo->ID < sched->schedule->numInsts && !IRMETHOD_isInstructionAPostdominator(sched->schedule->method, nextTo, branch) && !xanBitSet_isBitSet(seenInsts, nextTo->ID)) { */
/*             IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(sched->schedule->method, nextTo); */
/*             JITUINT32 blockNum = IRMETHOD_getBasicBlockPosition(sched->schedule->method, block); */
/*             xanBitSet_setBit(instSet, nextTo->ID); */
/*             xanBitSet_setBit(seenInsts, nextTo->ID); */
/*             xanBitSet_setBit(blockSet, blockNum); */
/*             xanList_append(instsToCheck, nextTo); */
/*           } */
/*       	  succItem	= succItem->next; */
/*         } */
/*         checkItem = checkItem->next; */
/*       } */
/*       freeList(instsToCheck); */
/*     } */
/*     branchItem = branchItem->next; */
/*   } */
/*   xanBitSet_free(seenInsts); */
/* } */


/**
 * If a branch is moved out of a region, it must take with it all of the
 * instructions in its basic block, due to control dependences.  Otherwise
 * a new branch instruction would be needed in its place.  Predecessor
 * blocks can later be patched up to target the successor and the branch
 * itself can be patched to jump to a new target.  Therefore, for each
 * branch in 'instSet', ensure that all instructions in its basic block
 * are also marked.
 **/
static void
markInstsInBranchBasicBlocks(schedule_t *schedule, XanBitSet *instSet) {
    JITUINT32 i, j;
    for (i = 0; i < schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(instSet, i)) {
            ir_instruction_t *branch = internal_fetchInstruction(schedule, i);
            if (IRMETHOD_isABranchInstruction(branch)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, branch);
                for (j = block->startInst; j <= block->endInst; ++j) {
                    if (xanBitSet_isBitSet(schedule->regionInsts, j)) {
                        xanBitSet_setBit(instSet, j);
                    }
                }
            }
        }
    }
}


/**
 * Mark the bits in 'instsToClone' and 'blocksToCloneFrom' for each branch in
 * 'regionInsts' in 'sched' that targets an instruction outside the region,
 * as defined by 'regionInsts'.  This is only required when there are
 * instructions to clone that come after this branch in the CFG.
 *
 * The rationale behind this is that some instructions may only be executed if
 * these branches do not leave the region.
 **/
static void
markBranchesLeavingRegion(schedule_t *schedule, XanBitSet *instsToClone, move_dir_t direction, XanHashTable **reachableInstructions) {
    JITINT32 i = -1;
    while ((i = xanBitSet_getFirstBitSetInRange(schedule->regionInsts, i + 1, schedule->numInsts)) != -1) {
        IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstructionPosition(schedule->method, i);

        /* Some blocks are only half in the region.
         */
        if (xanBitSet_isBitSet(schedule->regionInsts, block->endInst) && !xanBitSet_isBitSet(instsToClone, block->endInst)) {
            ir_instruction_t *branch = internal_fetchInstruction(schedule, block->endInst);
            XanHashTable    *reachableInstructionsAtBranch;
            reachableInstructionsAtBranch   = internal_fetchReachableInstructions(schedule->method, reachableInstructions, branch);

            /* Unconditional branches should only leave the region when they are
               the final instruction from a group that we're trying to move. */
            if (IRMETHOD_isABranchInstruction(branch) && branch->type != IRBRANCH) {
                JITBOOLEAN candidate = JITFALSE;
                XanListItem *succItem = xanList_first(schedule->successors[branch->ID]);
                while (succItem != NULL) {
                    ir_instruction_t *succ = succItem->data;
                    if (succ->ID < schedule->numInsts && !xanBitSet_isBitSet(schedule->regionInsts, succ->ID)) {
                        candidate = JITTRUE;
                        break;
                    }
                    succItem = succItem->next;
                }

                /* Check this branch should be included. */
                if (candidate) {
                    JITINT32 j = -1;
                    while ((j = xanBitSet_getFirstBitSetInRange(instsToClone, j + 1, schedule->numInsts)) != -1) {
                        XanHashTable    *reachableInstructionsAtClone;
                        ir_instruction_t *clone = internal_fetchInstruction(schedule, j);
                        reachableInstructionsAtClone    = internal_fetchReachableInstructions(schedule->method, reachableInstructions, clone);
                        if (    (direction == Upwards && (xanHashTable_lookup(reachableInstructionsAtBranch, clone) != NULL))       ||
                                (direction == Downwards && (xanHashTable_lookup(reachableInstructionsAtClone, branch) != NULL))     ) {
                            xanBitSet_setBit(instsToClone, branch->ID);
                            PDEBUG(2, "  Branch out of region inst %u\n", branch->ID);
                            break;
                        }
                    }
                }
            }
        }
    }
}


/**
 * Build an array of pointers to basic blocks that are marked in 'blocks'.
 *
 * Returns an array of pointers to basic blocks.
 **/
static IRBasicBlock **
buildBlockArrayFromBitSet(schedule_t *schedule, XanBitSet *blocks, IRBasicBlock *fakeHead) {
    JITUINT32 a, i;
    JITUINT32 numBlocks = xanBitSet_getCountOfBitsSet(blocks) + 1;
    IRBasicBlock **blockArray = allocFunction(numBlocks * sizeof(IRBasicBlock *));
    blockArray[0] = fakeHead;
    for (i = 0, a = 1; i < schedule->numBlocks; ++i) {
        if (xanBitSet_isBitSet(blocks, i)) {
            assert(a < numBlocks);
            blockArray[a] = IRMETHOD_getBasicBlockAtPosition(schedule->method, i);
            a += 1;
        }
    }
    return blockArray;
}


/* /\** */
/*  * Build a bit set of all entry basic blocks within 'regionBlocks' in */
/*  * 'schedule'.  These are blocks that are in the region and have no */
/*  * predecessors in the region.  Loop headers are the exception, where */
/*  * predecessors in the loop are ignored. */
/*  * */
/*  * Returns a bit set of entry basic blocks. */
/*  **\/ */
/* static XanBitSet * */
/* buildEntryBlockBitSet(schedule_t *schedule) */
/* { */
/*   JITUINT32 b; */
/*   XanBitSet *entryBlocks; */
/*   XanBitSet *loopHeaders; */
/*   XanListItem *loopItem; */

/*   /\* Identify loop headers. *\/ */
/*   loopHeaders = xanBitSet_new(schedule->numInsts); */
/*   loopItem = xanList_first(schedule->loops); */
/*   while (loopItem) { */
/*     schedule_loop_t *loop = loopItem->data; */
/*     xanBitSet_setBit(loopHeaders, loop->headerID); */
/*     loopItem = loopItem->next; */
/*   } */

/*   /\* Identify entry blocks. *\/ */
/*   entryBlocks = xanBitSet_new(schedule->numBlocks); */
/*   for (b = 0; b < schedule->numBlocks; ++b) { */
/*     if (xanBitSet_isBitSet(schedule->regionBlocks, b)) { */
/*       XanListItem *predItem; */
/*       IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b); */
/*       ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->startInst); */
/*       JITBOOLEAN predInRegion = JITFALSE; */

/*       /\* Check all predecessors. *\/ */
/*       predItem = xanList_first(schedule->predecessors[startInst->ID]); */
/*       while (predItem && !predInRegion) { */
/*         ir_instruction_t *pred = predItem->data; */
/*         IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred); */
/*         JITUINT32 predBlockID = IRMETHOD_getBasicBlockPosition(schedule->method, predBlock); */
/*         if (xanBitSet_isBitSet(schedule->regionBlocks, predBlockID)) { */

/*           /\* Loop headers are allowed loop blocks as predecessors. *\/ */
/*           if (xanBitSet_isBitSet(loopHeaders, block->startInst)) { */

/*             /\* This should say whether pred is in this loop. *\/ */
/*             if (!IRMETHOD_isInstructionAPredominator(schedule->method, startInst, pred)) { */
/*               predInRegion = JITTRUE; */
/*             } */
/*           } */

/*           /\* Not a loop header. *\/ */
/*           else { */
/*             predInRegion = JITTRUE; */
/*           } */
/*         } */
/*         predItem = predItem->next; */
/*       } */

/*       /\* Add exit blocks. *\/ */
/*       if (!predInRegion) { */
/*         xanBitSet_setBit(entryBlocks, b); */
/*       } */
/*     } */
/*   } */

/*   /\* Clean up. *\/ */
/*   xanBitSet_free(loopHeaders); */
/*   assert(xanBitSet_getCountOfBitsSet(entryBlocks) > 0); */
/*   PDEBUG(3, "Entry blocks are:"); */
/*   printBlockBitSet(3, schedule->method, schedule->numBlocks, entryBlocks); */
/*   return entryBlocks; */
/* } */


/* /\** */
/*  * Build a bit set of all exit basic blocks within 'regionBlocks' in */
/*  * 'schedule'.  These are blocks that are in the region and have no successors */
/*  * in the region.  Loop headers are the exception, where successors in the */
/*  * loop are ignored. */
/*  * */
/*  * Returns a bit set of exit basic blocks. */
/*  **\/ */
/* static XanBitSet * */
/* buildExitBlockBitSet(schedule_t *schedule) */
/* { */
/*   JITUINT32 b; */
/*   XanBitSet *exitBlocks; */
/*   XanBitSet *loopHeaders; */
/*   XanListItem *loopItem; */

/*   /\* Identify loop headers. *\/ */
/*   loopHeaders = xanBitSet_new(schedule->numInsts); */
/*   loopItem = xanList_first(schedule->loops); */
/*   while (loopItem) { */
/*     schedule_loop_t *loop = loopItem->data; */
/*     xanBitSet_setBit(loopHeaders, loop->headerID); */
/*     loopItem = loopItem->next; */
/*   } */

/*   /\* Identify exit blocks. *\/ */
/*   exitBlocks = xanBitSet_new(schedule->numBlocks); */
/*   for (b = 0; b < schedule->numBlocks; ++b) { */
/*     if (xanBitSet_isBitSet(schedule->regionBlocks, b)) { */
/*       XanListItem *succItem; */
/*       IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b); */
/*       ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst); */
/*       JITBOOLEAN succInRegion = JITFALSE; */

/*       /\* Check all successors. *\/ */
/*       succItem = xanList_first(successors[endInst->ID]); */
/*       while (succItem && !succInRegion) { */
/*         ir_instruction_t *succ = succItem->data; */
/*         IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ); */
/*         JITUINT32 succBlockID = IRMETHOD_getBasicBlockPosition(schedule->method, succBlock); */
/*         if (xanBitSet_isBitSet(sched->regionBlocks, succBlockID)) { */

/*           /\* Loop headers are allowed loop blocks as successors. *\/ */
/*           if (xanBitSet_isBitSet(loopHeaders, block->startInst)) { */

/*             /\* This should say whether succ is in this loop. *\/ */
/*             if (!IRMETHOD_isInstructionAPostdominator(schedule->method, endInst, succ)) { */
/*               succInRegion = JITTRUE; */
/*             } */
/*           } */

/*           /\* Not a loop header. *\/ */
/*           else { */
/*             succInRegion = JITTRUE; */
/*           } */
/*         } */
/*         succItem = succItem->next; */
/*       } */

/*       /\* Add exit blocks. *\/ */
/*       if (!succInRegion) { */
/*         xanBitSet_setBit(exitBlocks, b); */
/*       } */
/*     } */
/*   } */

/*   /\* Clean up. *\/ */
/*   xanBitSet_free(loopHeaders); */
/*   PDEBUG(3, "Exit blocks are:"); */
/*   printBlockBitSet(3, schedule->method, schedule->numBlocks, exitBlocks); */
/*   return exitBlocks; */
/* } */


/**
 * Build a pre-dominance or post-dominance tree for basic blocks in
 * 'regionBlocks' in 'schedule'.  The entry basic block for this tree contains
 * the instructions in 'outerInsts'.
 *
 * Returns the dominance tree.
 **/
static XanNode *
buildDominanceTree(schedule_t *schedule, XanBitSet *outerInsts, JITBOOLEAN preDom) {
    JITUINT32 numRegionBlocks;
    IRBasicBlock **blockArray;
    XanNode *domTree;

    /* The information object that is used. */
    assert(!domInfo);
    domInfo = allocFunction(sizeof(dom_info_t));
    domInfo->method = schedule->method;
    domInfo->blocksToConsider = schedule->regionBlocks;
    domInfo->fakeHead = allocFunction(sizeof(IRBasicBlock));
    domInfo->fakeHead->startInst = -1;
    domInfo->fakeHead->endInst = -1;
    domInfo->startBlocks = getBlocksContainingInsts(schedule, outerInsts);

    /* Build an array of pointers to the basic blocks. */
    numRegionBlocks = xanBitSet_getCountOfBitsSet(schedule->regionBlocks) + 1;
    blockArray = buildBlockArrayFromBitSet(schedule, schedule->regionBlocks, domInfo->fakeHead);

    /* Build the dominance tree for basic blocks to consider. */
    if (preDom) {
        /* domInfo->startBlocks = buildEntryBlockBitSet(schedule); */
        domTree = calculateDominanceTree((void **)blockArray, numRegionBlocks, 0, getPreDomPredBlock, getPreDomSuccBlock);
    } else {
        /* domInfo->startBlocks = buildExitBlockBitSet(schedule); */
        domTree = calculateDominanceTree((void **)blockArray, numRegionBlocks, 0, getPostDomSuccBlock, getPostDomPredBlock);
    }

    /* Clean up (fake head freed when tree is). */
    freeFunction(blockArray);
    xanBitSet_free(domInfo->startBlocks);
    freeFunction(domInfo);
    domInfo = NULL;

    /* Print the tree (for checking). */
    /*   printTree(domTree, printBB); */

    /* Return this tree. */
    return domTree;
}

/* /\** */
/*  * Check 'instsToClone' to see if a blocked instruction is included.  If so, */
/*  * ensure that all adjacent blocked instructions are also included. */
/*  * */
/*  * Returns JITTRUE if any further instructions were added. */
/*  **\/ */
/* static JITBOOLEAN */
/* includeBlockedInsts(schedule_info_t *sched, XanBitSet *instsToClone, XanBitSet *blocksToCloneFrom, XanList **predecessors, XanList **successors) */
/* { */
/*   JITUINT32 i; */
/*   JITBOOLEAN additions = JITFALSE; */
/*   XanBitSet *instsToCheck = xanBitSet_clone(instsToClone); */
/*   xanBitSet_intersect(instsToCheck, sched->blockedInstSet); */
/*   while (xanBitSet_getCountOfBitsSet(instsToCheck) > 0) { */
/*     for (i = 0; i < sched->schedule->numInsts; ++i) { */
/*       if (xanBitSet_isBitSet(instsToCheck, i)) { */
/*     	XanListItem	*predItem; */
/*     	XanListItem	*succItem; */
/*         ir_instruction_t *nextTo; */
/*         ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(sched->schedule->method, i); */
/*     	predItem	= xanList_first(predecessors[inst->ID]); */
/* 	while (predItem != NULL){ */
/*           nextTo	= predItem->data; */
/*           assert(nextTo != NULL); */
/*           if (!xanBitSet_isBitSet(instsToClone, nextTo->ID) && xanBitSet_isBitSet(sched->blockedInstSet, nextTo->ID)) { */
/*             IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(sched->schedule->method, nextTo); */
/*             JITUINT32 blockNum = IRMETHOD_getBasicBlockPosition(sched->schedule->method, block); */
/*             xanBitSet_setBit(instsToClone, nextTo->ID); */
/*             xanBitSet_setBit(instsToCheck, nextTo->ID); */
/*             xanBitSet_setBit(blocksToCloneFrom, blockNum); */
/*             PDEBUG(2, "  Blocked inst %u\n", nextTo->ID); */
/*             additions = JITTRUE; */
/*           } */
/* 	  predItem	= predItem->next; */
/*         } */
/*     	succItem	= xanList_first(successors[inst->ID]); */
/*         while (succItem != NULL){ */
/*           nextTo	= succItem->data; */
/*           assert(nextTo != NULL); */
/*           if (nextTo->ID < sched->schedule->numInsts && !xanBitSet_isBitSet(instsToClone, nextTo->ID) && xanBitSet_isBitSet(sched->blockedInstSet, nextTo->ID)) { */
/*             IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(sched->schedule->method, nextTo); */
/*             JITUINT32 blockNum = IRMETHOD_getBasicBlockPosition(sched->schedule->method, block); */
/*             xanBitSet_setBit(instsToClone, nextTo->ID); */
/*             xanBitSet_setBit(instsToCheck, nextTo->ID); */
/*             xanBitSet_setBit(blocksToCloneFrom, blockNum); */
/*             PDEBUG(2, "  Blocked inst %u\n", nextTo->ID); */
/*             additions = JITTRUE; */
/*           } */
/*           succItem	= succItem->next; */
/*         } */
/*         xanBitSet_clearBit(instsToCheck, i); */
/*       } */
/*     } */
/*   } */
/*   xanBitSet_free(instsToCheck); */

/*   return additions; */
/* } */


/**
 * Determine whether there's a data dependence between two instructions.
 **/
static JITBOOLEAN
hasDataDependence(XanBitSet **dataDepGraph, JITUINT32 i, JITUINT32 j) {
    return (dataDepGraph[i] && xanBitSet_isBitSet(dataDepGraph[i], j)) || (dataDepGraph[j] && xanBitSet_isBitSet(dataDepGraph[j], i));
}


/**
 * Determine whether two instructions define the same variable.
 **/
static JITBOOLEAN
instructionsDefineSameVariable(ir_instruction_t *first, ir_instruction_t *second) {
    if (first->result.type != IROFFSET || second->result.type != IROFFSET) {
        return JITFALSE;
    }
    return first->result.value.v == second->result.value.v;
}


/**
 * Determine whether one instruction can be reached from another, including
 * paths that traverse back edges.
 **/
static JITBOOLEAN canInstructionBeReachedFromPositionWithBackEdges(schedule_t *schedule, ir_instruction_t *inst, ir_instruction_t *position, XanHashTable **reachableInstructions, XanStack *todo, XanBitSet *seenInsts) {
    XanHashTable    *reachedTable;

    /* An easy test.
     */
    reachedTable    = internal_fetchReachableInstructions(schedule->method, reachableInstructions, position);
    if (xanHashTable_lookup(reachedTable, inst) != NULL) {
        return JITTRUE;
    }

    /* Erase the bitset.
     */
    xanBitSet_clearAll(seenInsts);

    /* Empty the stack.
     */
    while (xanStack_pop(todo));

    /* Consider back edges.
     */
    xanStack_push(todo, reachedTable);
    do {
        XanHashTableItem    *reachedItem;
        XanHashTable        *currentReachedTable;

        /* Fetch the table to consider.
         */
        currentReachedTable = (XanHashTable *) xanStack_pop(todo);
        assert(currentReachedTable != NULL);

        /* Analyze the instructions within the current table.
         */
        reachedItem = xanHashTable_first(currentReachedTable);
        while (reachedItem != NULL) {
            ir_instruction_t *reached = reachedItem->element;
            if (!xanBitSet_isBitSet(seenInsts, reached->ID)) {
                xanBitSet_setBit(seenInsts, reached->ID);
                if (instructionIsEdgePredecessorInList(reached, schedule->backEdges)) {
                    XanListItem *succItem = xanList_first(schedule->successors[reached->ID]);
                    while (succItem) {
                        XanHashTable *secondTable;
                        ir_instruction_t *succ = succItem->data;
                        secondTable = internal_fetchReachableInstructions(schedule->method, reachableInstructions, succ);
                        if (xanHashTable_lookup(secondTable, inst) != NULL) {
                            return JITTRUE;
                        }
                        xanStack_push(todo, secondTable);
                        succItem = succItem->next;
                    }
                }
            }

            reachedItem = xanHashTable_next(currentReachedTable, reachedItem);
        }

    } while (xanStack_getSize(todo) > 0);

    return JITFALSE;
}


/**
 * Mark instructions and move in 'instsToClone' for all instructions that are
 * data dependent on those already being moved.
 **/
static void
markDataDependentInstsToMove(schedule_t *schedule, XanBitSet *instsToClone, move_dir_t direction, XanHashTable **reachableInstructions, XanStack *todoStack, XanBitSet *seenInsts) {
    JITBOOLEAN additions = JITTRUE;
    PDECL(XanBitSet *addedInsts = xanBitSet_new(schedule->numInsts));
    while (additions) {
        JITINT32 j, i = -1;
        additions = JITFALSE;
        while ((i = xanBitSet_getFirstBitSetInRange(instsToClone, i + 1, schedule->numInsts)) != -1) {
            ir_instruction_t *clone = internal_fetchInstruction(schedule, i);
            for (j = 0; j < schedule->numInsts; ++j) {
                if (xanBitSet_isBitSet(schedule->regionInsts, j) && !xanBitSet_isBitSet(instsToClone, j)) {
                    ir_instruction_t *check = internal_fetchInstruction(schedule, j);
                    if (    (direction == Upwards && hasDataDependence(schedule->dataDepGraph, j, i) && canInstructionBeReachedFromPositionWithBackEdges(schedule, clone, check, reachableInstructions, todoStack, seenInsts)) ||
                            (direction == Downwards && hasDataDependence(schedule->dataDepGraph, i, j) && canInstructionBeReachedFromPositionWithBackEdges(schedule, check, clone, reachableInstructions, todoStack, seenInsts))) {
                        xanBitSet_setBit(instsToClone, j);
                        PDECL(xanBitSet_setBit(addedInsts, j));
                        additions = JITTRUE;
                    }
                }
            }
        }
    }
    PDEBUG(2, "  Data dependences:");
    PDEBUGSTMT(0, printInstBitSet(2, schedule->numInsts, addedInsts));
    PDEBUGSTMT(0, xanBitSet_free(addedInsts));
}


/**
 * Add additional branches that are required to keep the program semantics
 * when moving upwards.  We want branches within the region that are above
 * existing blocks and that can choose not to go to an existing block.
 * That means blocks ending in a conditional branch that are not
 * post-dominated by the existing block.  However, we need to make sure
 * that we can reach existing blocks too so that we don't include branches
 * that are on a totally different path.
 *
 * Returns JITTRUE if any additions were made, JITFALSE otherwise.
 **/
/* static JITBOOLEAN */
/* markUpwardsControlBranchesToMove(schedule_t *schedule, XanBitSet *instsToClone, XanNode *preDomTree, XanNode *postDomTree) */
/* { */
/*   JITUINT32 i, j; */
/*   JITBOOLEAN additions = JITFALSE; */
/*   XanBitSet *blocksToCloneFrom = getBlocksContainingInsts(schedule, instsToClone); */
/*   PDEBUG(2, "  Control branches\n"); */
/*   for (i = 0; i < schedule->numBlocks; ++i) { */
/*     if (xanBitSet_isBitSet(blocksToCloneFrom, i) && xanBitSet_isBitSet(schedule->regionBlocks, i)) { */
/*       IRBasicBlock *existingBlock = IRMETHOD_getBasicBlockAtPosition(schedule->method, i); */
/*       for (j = 0; j < schedule->numBlocks; ++j) { */
/*         if (xanBitSet_isBitSet(schedule->regionBlocks, j)) { */
/*           IRBasicBlock *otherBlock = IRMETHOD_getBasicBlockAtPosition(schedule->method, j); */

/*           /\* Check post-dominance relation. *\/ */
/*           if (!isADominator(existingBlock, otherBlock, postDomTree)) { */
/*             ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(schedule->method, otherBlock->endInst); */
/*             JITBOOLEAN preDomPostDoms = JITFALSE; */
/*             XanList *preDomBlocks = getElemsDominatingElem(existingBlock, preDomTree); */
/*             /\* XanListItem *preDomItem = xanList_first(preDomBlocks); *\/ */

/*             /\* /\\* Check whether a pre-dominator is a post-dominator. *\\/ *\/ */
/*             /\* while (preDomItem && !preDomPostDoms) { *\/ */
/*             /\*   IRBasicBlock *preDomBlock = preDomItem->data; *\/ */
/*             /\*   if (preDomBlock != otherBlock && !isFakeHeadBlock(preDomBlock) && isADominator(preDomBlock, otherBlock, postDomTree)) { *\/ */
/*             /\*     preDomPostDoms = JITTRUE; *\/ */
/*             /\*     PDEBUG(3, "      But this is a pre-dominator as well as a post-dominator\n"); *\/ */
/*             /\*   } *\/ */
/*             /\*   preDomItem = preDomItem->next; *\/ */
/*             /\* } *\/ */

/*             /\* Conditional branches only. *\/ */
/*             if (!preDomPostDoms && (endInst->type == IRBRANCHIF || endInst->type == IRBRANCHIFNOT)) { */
/*               ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, existingBlock->startInst); */
/*               if (IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, startInst, endInst)) { */
/*                 if (!xanBitSet_isBitSet(blocksToCloneFrom, j) || !xanBitSet_isBitSet(instsToClone, otherBlock->endInst)) { */
/*                   xanBitSet_setBit(blocksToCloneFrom, j); */
/*                   PDEBUG(2, "    Undominated block (%u-%u)\n", otherBlock->startInst, otherBlock->endInst); */
/*                   xanBitSet_setBit(instsToClone, otherBlock->endInst); */
/*                   PDEBUG(2, "    Undominated branch %u\n", otherBlock->endInst); */
/*                   additions = JITTRUE; */
/*                 } */
/*               } */
/*             } */

/*             /\* Clean up. *\/ */
/*             freeList(preDomBlocks); */
/*           } */
/*         } */
/*       } */
/*     } */
/*   } */
/*   xanBitSet_free(blocksToCloneFrom); */
/*   return additions; */
/* } */
static JITBOOLEAN
markUpwardsControlBranchesToMove(schedule_t *schedule, XanBitSet *instsToClone, XanNode *preDomTree, XanNode *postDomTree, XanHashTable **reachableInstructions) {
    JITINT32 i, j = -1;
    JITBOOLEAN additions = JITFALSE;
    XanBitSet *blocksToCloneFrom = getBlocksContainingInsts(schedule, instsToClone);
    PDEBUG(2, "  Control branches\n");
    for (i = 0; i < schedule->numBlocks; ++i) {
        if (xanBitSet_isBitSet(blocksToCloneFrom, i) && xanBitSet_isBitSet(schedule->regionBlocks, i)) {
            IRBasicBlock *existingBlock = IRMETHOD_getBasicBlockAtPosition(schedule->method, i);
            while ((j = xanBitSet_getFirstBitSetInRange(schedule->regionBlocks, j + 1, schedule->numBlocks)) != -1) {
                IRBasicBlock *otherBlock = IRMETHOD_getBasicBlockAtPosition(schedule->method, j);
                ir_instruction_t *endInst = internal_fetchInstruction(schedule, otherBlock->endInst);
                XanHashTable    *reachableInstructionsAtEndInst;
                reachableInstructionsAtEndInst  = internal_fetchReachableInstructions(schedule->method, reachableInstructions, endInst);

                /* Conditional branches only. */
                if (endInst->type == IRBRANCHIF || endInst->type == IRBRANCHIFNOT) {
                    if (!xanBitSet_isBitSet(blocksToCloneFrom, j) || !xanBitSet_isBitSet(instsToClone, otherBlock->endInst)) {
                        ir_instruction_t *startInst = internal_fetchInstruction(schedule, existingBlock->startInst);
                        if (xanHashTable_lookup(reachableInstructionsAtEndInst, startInst) != NULL) {

                            /* Check post-dominance relation. */
                            if (!isADominator(existingBlock, otherBlock, postDomTree)) {
                                xanBitSet_setBit(blocksToCloneFrom, j);
                                PDEBUG(2, "    Undominated block (%u-%u)\n", otherBlock->startInst, otherBlock->endInst);
                                xanBitSet_setBit(instsToClone, otherBlock->endInst);
                                PDEBUG(2, "    Undominated branch %u\n", otherBlock->endInst);
                                additions = JITTRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    xanBitSet_free(blocksToCloneFrom);
    return additions;
}


/* /\** */
/*  * Mark branches that end blocks in 'blocksToCloneFrom' if those branches */
/*  * are in 'regionInsts' in 'sched.  This is necessary to avoid missing */
/*  * branches in the initial set of basic blocks that are marked. */
/*  **\/ */
/* static void */
/* markBranchesEndingExistingBlocks(schedule_info_t *sched, XanBitSet *instsToClone, XanBitSet *blocksToCloneFrom) */
/* { */
/*   JITUINT32 b; */
/*   for (b = 0; b < sched->schedule->numBlocks; ++b) { */
/*     if (xanBitSet_isBitSet(blocksToCloneFrom, b)) { */
/*       IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(sched->schedule->method, b); */
/*       if (xanBitSet_isBitSet(sched->regionInsts, block->endInst) && !xanBitSet_isBitSet(instsToClone, block->endInst)) { */
/*         ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(sched->schedule->method, block->endInst); */
/*         if (IRMETHOD_isABranchInstruction(branch)) { */
/*           xanBitSet_setBit(instsToClone, branch->ID); */
/*         } */
/*       } */
/*     } */
/*   } */
/* } */


/* /\** */
/*  * Build bit sets corresponding to instructions and blocks that */
/*  * instructions in 'endInsts' depend on.  The bit sets to update are */
/*  * 'instsToClone' and 'blocksToCloneFrom'.  Only instructions in the region */
/*  * defined by 'regionInsts' in 'sched' are used.  Data dependences are */
/*  * derived from 'dataDepGraph' whereas control dependences are worked out */
/*  * as instructions are added to the bit sets. */
/*  **\/ */
/* static void */
/* findInstsAndBlocksToClone(schedule_info_t *sched, XanBitSet *instsToClone, XanBitSet *blocksToCloneFrom, XanList **predecessors, XanList **successors) */
/* { */
/*   JITBOOLEAN additions; */
/*   XanNode *preDomTree; */
/*   XanNode *postDomTree; */
/*   PDEBUG(1, "Building bit set of instructions to clone\n"); */

/*   /\* Build the dominator tree for all blocks to consider. *\/ */
/*   preDomTree = buildDominanceTree(sched, sched->nextToInst, JITTRUE, predecessors, successors); */
/*   postDomTree = buildDominanceTree(sched, sched->nextToInst, JITFALSE, predecessors, successors); */

/*   /\* Instructions to clone. *\/ */
/*   markInstsAndBlocks(sched->schedule->method, instsToClone, blocksToCloneFrom, sched->instsToMove); */
/*   markInstsInBranchBasicBlocks(sched, instsToClone); */
/*   /\* The problem with this function is that branches get included even if there */
/*      are no later instructions to clone.  This only happens when moving */
/*      downwards.  Moving upwards, by definition, there will be later */
/*      instructions to move. *\/ */
/* /\*   markBranchesEndingExistingBlocks(sched, instsToClone, blocksToCloneFrom); *\/ */
/*   markBranchesLeavingRegion(sched, instsToClone, blocksToCloneFrom, successors); */

/*   /\* Since we only set bits, this will reach a fixed point. *\/ */
/*   additions = JITTRUE; */
/*   while (additions) { */

/*     /\* Make sure all data dependences are included. *\/ */
/*     markDataDependentInstsAndBlocksToMove(sched, instsToClone, blocksToCloneFrom); */

/*     /\* Mark branches to keep control flow. *\/ */
/*     additions = markUpwardsControlBranchesToMove(sched, instsToClone, blocksToCloneFrom, preDomTree, postDomTree, successors); */

/*     /\* Ensure blocked instructions are treated correctly. *\/ */
/*     additions |= includeBlockedInsts(sched, instsToClone, blocksToCloneFrom, predecessors, successors); */
/*   } */

/*   /\* Don't necessarily include certain instructions. *\/ */
/*   if (sched->direction == Upwards && sched->position == After) { */
/*     xanBitSet_clearBit(instsToClone, sched->nextToInst->ID); */
/*   } */

/*   /\* Debugging output. *\/ */
/*   PDEBUG(3, "Insts to clone:"); */
/*   printInstBitSet(3, sched->schedule->numInsts, instsToClone); */
/*   PDEBUG(3, "Blocks to clone from:"); */
/*   printBlockBitSet(3, sched->schedule->method, sched->schedule->numBlocks, blocksToCloneFrom); */

/*   /\* Clean up (the first data element is the fake head). *\/ */
/*   freeFunction(preDomTree->data); */
/*   freeFunction(postDomTree->data); */
/*   preDomTree->destroyTree(preDomTree); */
/*   postDomTree->destroyTree(postDomTree); */
/* } */


/**
 * COMMENT NEEDS UPDATING
 *
 * Build bit sets corresponding to instructions and blocks that
 * instructions in 'endInsts' depend on.  The bit sets to update are
 * 'instsToClone' and 'blocksToCloneFrom'.  Only instructions in the region
 * defined by 'regionInsts' in 'sched' are used.  Data dependences are
 * derived from 'dataDepGraph' whereas control dependences are worked out
 * as instructions are added to the bit sets.
 **/
XanBitSet * SCHEDULE_findInstsToCloneWhenMoving(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *outerInsts, JITBOOLEAN upwards, XanHashTable **reachableInstructions, XanNode **cachedPreDomTree, XanNode **cachedPostDomTree) {
    JITBOOLEAN additions;
    XanStack    *todoStack;
    XanBitSet *instsToClone;
    XanBitSet *seenInsts;
    XanNode     *preDomTree;
    XanNode     *postDomTree;
    JITBOOLEAN  reachableInstructionsToFree;
    move_dir_t direction = upwards ? Upwards : Downwards;

    /* Assertions.
     */
    assert(schedule != NULL);

    /* Debugging output.
     */
    setDebugLevel();
    PDEBUG(1, "Building bit set of instructions to clone\n");
    PDEBUG(2, "  Moving:");
    PDEBUGSTMT(2, printInstBitSet(2, schedule->numInsts, instsToMove));
    PDEBUG(2, "  Beyond:");
    PDEBUGSTMT(2, printInstBitSet(2, schedule->numInsts, outerInsts));

    /* Allocate the memory.
     */
    reachableInstructionsToFree = JITFALSE;
    if (reachableInstructions == NULL) {
        reachableInstructions       = allocFunction(sizeof(XanHashTable *) * (schedule->numInsts + 1));
        reachableInstructionsToFree = JITTRUE;
    }
    todoStack               = xanStack_new(allocFunction, freeFunction, NULL);
    seenInsts               = xanBitSet_new(schedule->numInsts);

    /* The instructions to clone.
     */
    instsToClone            = xanBitSet_clone(instsToMove);

    /* Build the dominator tree for all blocks to consider.
     */
    if (cachedPreDomTree != NULL) {
        preDomTree  = *cachedPreDomTree;
        if (preDomTree == NULL) {
            preDomTree   = buildDominanceTree(schedule, outerInsts, JITTRUE);
            (*cachedPreDomTree) = preDomTree;
        }
        assert((*cachedPreDomTree) != NULL);
    } else {
        preDomTree   = buildDominanceTree(schedule, outerInsts, JITTRUE);
    }
    assert(preDomTree != NULL);
    if (cachedPostDomTree != NULL) {
        postDomTree  = *cachedPostDomTree;
        if (postDomTree == NULL) {
            postDomTree   = buildDominanceTree(schedule, outerInsts, JITTRUE);
            (*cachedPostDomTree) = postDomTree;
        }
        assert((*cachedPostDomTree) != NULL);
    } else {
        postDomTree   = buildDominanceTree(schedule, outerInsts, JITTRUE);
    }
    assert(postDomTree != NULL);

    /* Mark initial instructions to clone.
     */
    markInstsInBranchBasicBlocks(schedule, instsToClone);
    if (direction == Upwards) {
        markBranchesLeavingRegion(schedule, instsToClone, direction, reachableInstructions);
    }

    /* Since we only set bits, this will reach a fixed point.
     */
    additions = JITTRUE;
    while (additions) {

        /* Make sure all data dependences are included. */
        markDataDependentInstsToMove(schedule, instsToClone, direction, reachableInstructions, todoStack, seenInsts);

        /* Mark branches to keep control flow. */
        additions = markUpwardsControlBranchesToMove(schedule, instsToClone, preDomTree, postDomTree, reachableInstructions);
    }

    /* Debugging output.
     */
    PDEBUG(3, "Insts to clone:");
    PDEBUGSTMT(3, printInstBitSet(3, schedule->numInsts, instsToClone));

    /* Clean up (the first data element is the fake head).
     */
    if (reachableInstructionsToFree) {
        for (JITUINT32 count=0; count <= schedule->numInsts; count++) {
            if (reachableInstructions[count] != NULL) {
                freeHashTable(reachableInstructions[count]);
            }
        }
        freeFunction(reachableInstructions);
    }
    xanStack_destroyStack(todoStack);
    xanBitSet_free(seenInsts);
    if (cachedPreDomTree == NULL) {
        freeFunction(preDomTree->data);
        preDomTree->destroyTree(preDomTree);
    }
    if (cachedPostDomTree == NULL) {
        freeFunction(postDomTree->data);
        postDomTree->destroyTree(postDomTree);
    }

    return instsToClone;
}

/**
 * Check whether there is a dependence between branches that aren't in
 * 'branchesToSkip' and instructions in 'instsToCheckWith' that would
 * prevent instructions moving past this branch.
 *
 * Returns JITTRUE if there are no dependences, JITFALSE otherwise.
 **/
static JITBOOLEAN
checkBranchesCanBePassed(schedule_info_t *sched, XanBitSet *branchesToSkip, XanBitSet *instsToCheckWith) {
    JITUINT32 b, i;
    schedule_t *schedule = sched->schedule;
    for (b = 0; b < schedule->numBlocks; ++b) {
        IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
        if (block->endInst < schedule->numInsts && !xanBitSet_isBitSet(branchesToSkip, block->endInst)) {
            ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
            if (IRMETHOD_isABranchInstruction(endInst) && schedule->dataDepGraph[endInst->ID]) {
                XanBitSet *reachedInsts = getInstsReachedWhenBranching(schedule, endInst, sched->direction);

                /* We check against the instructions reached when branching, which
                   is the set of instructions that will actually pass this branch.
                   Other instructions don't matter. */
                for (i = 0; i < schedule->numInsts; ++i) {
                    if (i != endInst->ID && xanBitSet_isBitSet(instsToCheckWith, i) && xanBitSet_isBitSet(reachedInsts, i) && hasDataDependence(schedule->dataDepGraph, endInst->ID, i)) {
                        PDEBUG(0, "Fail: Data dependence between branch %u and passing inst %u prevents movement\n", endInst->ID, i);
                        xanBitSet_free(reachedInsts);
                        sched->shouldFail = JITTRUE;
                        return JITFALSE;
                    }
                }
                xanBitSet_free(reachedInsts);
            }
        }
    }
    return JITTRUE;
}


/**
 * Check whether instructions from a loop are being cloned along one of that
 * loop's back edges.  We don't want to allow this for now, because it causes
 * problems patching up the branches.  It also seems like a bad idea because
 * it just clones instructions in the region, without removing them.
 **/
static JITBOOLEAN
cloningLoopInstsOnItsBackEdge(schedule_info_t *sched, XanBitSet *instsToClone, XanHashTable *edgeCloneMap) {
    schedule_t *schedule = sched->schedule;
    XanHashTableItem *cloneEdgeItem = xanHashTable_first(edgeCloneMap);
    while (cloneEdgeItem) {
        edge_t *cloneEdge = cloneEdgeItem->elementID;
        if (IRMETHOD_isABranchInstruction(cloneEdge->pred) && IRMETHOD_getBranchDestination(schedule->method, cloneEdge->pred) == cloneEdge->succ) {
            XanListItem *backEdgeItem = xanList_first(schedule->backEdges);
            while (backEdgeItem) {
                edge_t *backEdge = backEdgeItem->data;
                if (edgesMatch(cloneEdge, backEdge)) {
                    XanListItem *loopItem = xanList_first(schedule->loops);
                    while (loopItem) {
                        schedule_loop_t *loop = loopItem->data;
                        if (!xanBitSet_isIntersectionEmpty(loop->loop, instsToClone)) {
                            return JITTRUE;
                        }
                        loopItem = loopItem->next;
                    }
                }
                backEdgeItem = backEdgeItem->next;
            }
        }
        cloneEdgeItem = xanHashTable_next(edgeCloneMap, cloneEdgeItem);
    }
    return JITFALSE;
}


/**
 * Check cloned instructions wouldn't violate other dependences along paths
 * leading into the region from outside.  This means that clones cannot have
 * a dependence with any instruction on a path from outside the region if
 * that instruction did not reach the clone already.
 **/
static JITBOOLEAN
checkClonesDontViolateOutsideDependences(schedule_info_t *sched, XanBitSet *instsToClone) {
    JITINT32 i = -1;
    XanList *priorInsts = newList();
    schedule_t *schedule = sched->schedule;

    /* I believe this is only an issue moving downwards, since branches are
       cloned if necessary to keep semantics when moving upwards. */
    if (sched->direction == Upwards) {
        return JITTRUE;
    }

    /* Paths into the region. */
    PDEBUG(2, "Checking clones don't violate dependences into the region\n");
    while ((i = xanBitSet_getFirstBitSetInRange(schedule->regionInsts, i + 1, schedule->numInsts)) != -1) {
        XanListItem *priorItem = xanList_first(schedule->predecessors[i]);
        while (priorItem) {
            ir_instruction_t *prior = priorItem->data;
            if (!xanBitSet_isBitSet(schedule->regionInsts, prior->ID) && !xanList_find(priorInsts, prior)) {
                xanList_append(priorInsts, prior);
            }
            priorItem = priorItem->next;
        }
    }
    PDEBUG(3, "Prior instructions:");
    PDEBUGSTMT(3, printInstList(3, priorInsts));

    /* Check clones that pass each path. */
    while ((i = xanBitSet_getFirstBitSetInRange(instsToClone, i + 1, schedule->numInsts)) != -1) {
        ir_instruction_t *clone = IRMETHOD_getInstructionAtPosition(schedule->method, i);
        XanListItem *priorItem = xanList_first(priorInsts);
        while (priorItem) {
            ir_instruction_t *prior = priorItem->data;
            XanHashTable *reachingInsts = IRMETHOD_getInstructionsThatCanHaveBeenExecutedBeforeInstruction(schedule->method, prior);
            XanHashTableItem *reachingItem;
            PDEBUG(3, "Checking clone %d and prior instruction %u\n", i, prior->ID);

            /* Check live-in variables aren't redefined. */
            if (IRMETHOD_doesInstructionDefineAVariable(schedule->method, clone) && IRMETHOD_isVariableLiveOUT(schedule->method, prior, clone->result.value.v)) {
                freeList(priorInsts);
                freeHashTable(reachingInsts);
                /* Not strictly true, we need to check the edge, rather than out of the predecessor. */
                PDEBUG(0, "Fail: Cloned instruction redefines a variable live into region\n");
                return JITFALSE;
            }

            /* Check memory dependences aren't violated.  Actually we can't
               distinguish between variable and memory dependences so this will
               catch all, including some that aren't an issue.  Sigh. */
            xanHashTable_insert(reachingInsts, prior, prior);
            reachingItem = xanHashTable_first(reachingInsts);
            while (reachingItem) {
                ir_instruction_t *reaching = reachingItem->element;
                if (hasDataDependence(schedule->dataDepGraph, clone->ID, reaching->ID) && !IRMETHOD_isInstructionAPredominator(schedule->method, reaching, clone)) {
                    freeList(priorInsts);
                    freeHashTable(reachingInsts);
                    PDEBUG(0, "Fail: Cloned instruction violates a data dependence along path to region\n");
                    return JITFALSE;
                }
                reachingItem = xanHashTable_next(reachingInsts, reachingItem);
            }
            freeHashTable(reachingInsts);
            priorItem = priorItem->next;
        }
    }
    freeList(priorInsts);
    return JITTRUE;
}


/**
 * Perform checks on instructions within 'instsToClone' to ensure they can
 * be safely cloned before 'nextToInsts' in 'sched'.
 * As a side-effect, if 'nextToInst'
 * is an unconditional branch then it is taken out of 'instsToClone', since
 * the only dependence on it was a control dependence.
 *
 * Returns JITTRUE if the instructions can be cloned, JITFALSE otherwise.
 **/
static JITBOOLEAN
canMoveInstsEarly(schedule_info_t *sched, XanBitSet *instsToClone) {
    JITINT32 i = -1;
    schedule_t *schedule = sched->schedule;

    /* Initial check for failure. */
    if (sched->shouldFail) {
        return JITFALSE;
    }

    /* Check the instruction to move next to. */
    if (!xanBitSet_isIntersectionEmpty(instsToClone, sched->nextToInsts)) {
        while ((i = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, i + 1, schedule->numInsts)) != -1) {
            if (xanBitSet_isBitSet(instsToClone, i)) {
                ir_instruction_t *nextToInst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                if (nextToInst->type == IRBRANCH && sched->position == Before && sched->direction == Upwards) {
                    xanBitSet_clearBit(instsToClone, nextToInst->ID);
                } else {
                    PDEBUG(1, "Trying to move inst %d\n", i);
                    PDEBUG(0, "Fail: Can't move instruction to move next to\n");
                    return JITFALSE;
                }
            }
        }
    }

    /* Check all instructions can be moved. */
    for (i = 0; i < schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(instsToClone, i)) {
            JITINT32 j = -1;

            /* Must be within the region. */
            if (!xanBitSet_isBitSet(schedule->regionInsts, i)) {
                PDEBUG(0, "Fail: Can't move non-region inst %u\n", i);
                return JITFALSE;
            }

            /* No dependence with the instructions to move next to. Actually, we
               should check the instructions that will go above and below it and
               make our decision on that basis, but that will have to go later in
               the pass so we'll disallow all for now. */
            while ((j = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, j + 1, schedule->numInsts)) != -1) {
                if ((sched->direction == Upwards && sched->position == Before &&
                        hasDataDependence(schedule->dataDepGraph, i, j)) ||
                        (sched->direction == Downwards && sched->position == After &&
                         hasDataDependence(schedule->dataDepGraph, j, i))) {
                    PDEBUG(0, "Fail: Can't have dependence between %d and %d\n", i, j);
                    return JITFALSE;
                }
            }
        }

        /* Can't move memcpy instructions yet since their variable dependences
           are not handled correctly by the DDG pass.  This is because it can't
           specify to iljit the variable that a dependence relates to, which is
           needed for the inductive variable analysis. Until this happens,
           memcpy can't be moved or have instructions move around it, so we
           simply prevent it being in the region.  Loads and stores to memory
           should also be prevented too, but it's rare that this will be an
           issue so we allow them though. */
        if (xanBitSet_isBitSet(schedule->regionInsts, i)) {
            ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
            if (inst->type == IRMEMCPY) {
                PDEBUG(0, "Fail: Can't move memcpy instructions currently\n");
                return JITFALSE;
            }
        }
    }

    /* Check uncloned branches are safe to pass. */
    if (!checkBranchesCanBePassed(sched, instsToClone, instsToClone)) {
        return JITFALSE;
    }

    /* Check dependences across the region are respected. */
    if (!checkClonesDontViolateOutsideDependences(sched, instsToClone)) {
        return JITFALSE;
    }

    /* All checks passed. */
    return JITTRUE;
}


/**
 * Perform final checks on instructions within 'instsToClone' and
 * 'instsToMove' to ensure they can be safely moved before 'nextToInst' in
 * 'sched'.
 *
 * Returns JITTRUE if the instructions can be moved, JITFALSE otherwise.
 **/
static JITBOOLEAN
canMoveInstsFinally(schedule_info_t *sched, XanBitSet *instsToClone, XanBitSet *instsToMove, XanHashTable *edgeCloneMap) {
    JITINT32 i = -1;
    schedule_t *schedule = sched->schedule;

    /* Initial check for failure. */
    if (sched->shouldFail) {
        return JITFALSE;
    }

    /* Must move instructions that were given. */
    while ((i = xanBitSet_getFirstBitSetInRange(sched->instsToMove, i + 1, schedule->numInsts)) != -1) {
        if (!xanBitSet_isBitSet(instsToMove, i)) {
            PDEBUG(1, "Instruction that isn't moved is %d\n", i);
            PDEBUG(0, "Fail: Not moving an instruction that should be moved\n");
            return JITFALSE;
        }
    }

    /* Must clone all instructions that are being moved. */
    if (!xanBitSet_isSubSetOf(instsToMove, instsToClone)) {
        PDEBUG(0, "Fail: Some insts to be moved will not be deleted\n");
        return JITFALSE;
    }

    /* Must be within the region. */
    while ((i = xanBitSet_getFirstBitSetInRange(instsToClone, i + 1, schedule->numInsts)) != -1) {
        if (!xanBitSet_isBitSet(schedule->regionInsts, i)) {
            PDEBUG(1, "Non-region instruction %u\n", i);
            PDEBUG(0, "Fail: Can't clone a non-region instruction\n");
            return JITFALSE;
        }
    }

    /* Check unmoving branches are safe to pass. */
    if (!checkBranchesCanBePassed(sched, instsToMove, instsToClone)) {
        return JITFALSE;
    }

    /* Don't move instructions from a loop along its back edge. */
    if (cloningLoopInstsOnItsBackEdge(sched, instsToClone, edgeCloneMap)) {
        PDEBUG(0, "Fail: Cloning instructions on their loop back edge\n");
        return JITFALSE;
    }

    /* Tests passed. */
    return JITTRUE;
}


/**
 * Check whether 'edge' has been used as a key into the hash table
 * 'edgeMap' and, if so, return the key.
 *
 * Returns the key edge, if there is one, or NULL otherwise.
 **/
static edge_t *
findKeyEdgeInMap(edge_t *edge, XanHashTable *edgeMap) {
    XanHashTableItem *keyItem = xanHashTable_first(edgeMap);
    while (keyItem) {
        edge_t *key = keyItem->elementID;
        if (edge->pred == key->pred && edge->succ == key->succ) {
            return key;
        }
        keyItem = xanHashTable_next(edgeMap, keyItem);
    }
    return NULL;
}


/**
 * Build a hash table that maps edges within the CFG to lists of
 * instructions that should be cloned on them.
 *
 * Returns a hash table mapping edges to instruction lists.
 **/
static XanHashTable *
buildEdgeCloneListMapping(schedule_info_t *sched, XanBitSet *instsToClone, XanBitSet *blocksToCloneFrom) {
    XanHashTable *edgeCloneMap;
    schedule_t *schedule = sched->schedule;

    /* Build the mapping. */
    edgeCloneMap = newHashTable();
    if (xanBitSet_getCountOfBitsSet(blocksToCloneFrom) > 0) {
        JITUINT32 b, i;
        PDEBUG(1, "Finding edges where cloned blocks should be placed\n");
        for (b = 0; b < schedule->numBlocks; ++b) {
            if (xanBitSet_isBitSet(blocksToCloneFrom, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                XanList *edgesForBlock;

                /* Get a list of edges for this block's clones. */
                if (sched->direction == Upwards) {
                    edgesForBlock = buildUpwardsCloneLocationListForBlock(sched, b);
                } else {
                    edgesForBlock = buildDownwardsCloneLocationListForBlock(sched, b);
                }

                /* Convert to a hash table mapping edges to instructions. */
                if (edgesForBlock) {
                    XanListItem *edgeItem = xanList_first(edgesForBlock);
                    while (edgeItem) {
                        edge_t *edgeForBlock = edgeItem->data;
                        edge_t *keyEdge = findKeyEdgeInMap(edgeForBlock, edgeCloneMap);
                        XanList *cloneList;
                        PDEBUG(2, "    Considering edge %u -> %u\n", edgeForBlock->pred->ID, edgeForBlock->succ->ID);

                        /* See if this edge has been seen or not. */
                        if (!keyEdge) {
                            cloneList = newList();
                            xanHashTable_insert(edgeCloneMap, edgeForBlock, cloneList);
                        } else {
                            freeFunction(edgeForBlock);
                            cloneList = xanHashTable_lookup(edgeCloneMap, keyEdge);
                        }

                        /* Add to sorted list of instructions to clone on this edge. */
                        PDEBUG(2, "      Appending insts:");
                        for (i = block->startInst; i <= block->endInst; ++i) {
                            if (xanBitSet_isBitSet(instsToClone, i)) {
                                ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                                insertInstIntoSortedList(inst, cloneList);
                                PDEBUGC(2, " %u", inst->ID);
                            }
                        }
                        PDEBUGC(2, "\n");
                        PDEBUG(2, "      Appended insts from block (%u-%u)\n", block->startInst, block->endInst);
                        edgeItem = edgeItem->next;
                    }

                    /* Finished with this list (edges have been freed or reused). */
                    freeList(edgesForBlock);
                }
            }
        }
    }

    /* Return the mapping. */
    return edgeCloneMap;
}


/**
 **/
static JITBOOLEAN
addBlockSkipMapping(IRBasicBlock *block, IRBasicBlock *succBlock, XanHashTable *skipMap) {
    IRBasicBlock *skipToBlock = xanHashTable_lookup(skipMap, block);
    assert(!skipToBlock || skipToBlock == succBlock);
    if (!skipToBlock) {
        xanHashTable_insert(skipMap, block, succBlock);
        PDEBUG(2, "    New skip mapping, block (%u-%u) skips to block (%u-%u)\n", block->startInst, block->endInst, succBlock->startInst, succBlock->endInst);
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Build a hash table mapping blocks that should be skipped to the blocks
 * that should be skipped to.  Uses 'skipBlocks' to determine those that
 * should be skipped.  Blocks to skip could be those that are not moved
 * or their inverse.
 *
 * Returns a hash table mapping blocks that should be skipped to targets.
 */
static XanHashTable *
buildBlockSkipMapping(schedule_info_t *sched, XanBitSet *skipBlocks) {
    XanHashTable *skipMap;
    JITBOOLEAN additions;
    schedule_t *schedule = sched->schedule;
    PDEBUG(2, "  Building block skip mappings\n");

    /* Since we only add blocks, this will reach a fixed point. */
    skipMap = newHashTable();
    additions = JITTRUE;
    while (additions) {
        JITINT32 b = -1;
        additions = JITFALSE;

        /* Find blocks to skip, backwards since we're computing successors. */
        while ((b = xanBitSet_getFirstBitSetInRange(skipBlocks, b + 1, schedule->numBlocks)) != -1) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            XanListItem *succItem = xanList_first(schedule->successors[block->endInst]);
            JITBOOLEAN foundSuccessor = JITFALSE;
            PDEBUG(2, "  Block (%u-%u) is skipped\n", block->startInst, block->endInst);

            /* Check each successor. */
            while (succItem) {
                ir_instruction_t *succ = succItem->data;
                IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                JITUINT32 succBlockNum = IRMETHOD_getBasicBlockPosition(schedule->method, succBlock);

                /* Ignore exit node successors and those not in the region. */
                if (succ->type == IREXITNODE || !xanBitSet_isBitSet(schedule->regionBlocks, succBlockNum)) {
                    succItem = succItem->next;
                    continue;
                }

                /* This successor skipped too. */
                foundSuccessor = JITTRUE;
                if (xanBitSet_isBitSet(skipBlocks, succBlockNum)) {
                    IRBasicBlock *succSkipToBlock = xanHashTable_lookup(skipMap, succBlock);
                    if (!succSkipToBlock) {
                        PDEBUG(2, "    Successor (%u-%u) skips to undefined block\n", succBlock->startInst, succBlock->endInst);
                    } else {
                        PDEBUG(2, "    Successor (%u-%u) skips to (%u-%u)\n", succBlock->startInst, succBlock->endInst, succSkipToBlock->startInst, succSkipToBlock->endInst);
                        additions |= addBlockSkipMapping(block, succSkipToBlock, skipMap);
                    }
                }

                /* This successor not skipped. */
                else {
                    assert(!xanHashTable_lookup(skipMap, block) || xanHashTable_lookup(skipMap, block) == succBlock);
                    additions |= addBlockSkipMapping(block, succBlock, skipMap);
                }
                succItem = succItem->next;
            }

            /* Ensure there's a successor to skip to. */
            if (!foundSuccessor) {
                IRBasicBlock *succBlock;
                ir_instruction_t *endInst JITUNUSED = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
                assert(!IRMETHOD_isABranchInstruction(endInst) || endInst->type == IRBRANCH);
                assert(IRMETHOD_getSuccessorsNumber(schedule->method, endInst) == 1 || IRMETHOD_isACallInstruction(endInst));
                succItem = xanList_first(schedule->successors[block->endInst]);
                while (succItem && ((ir_instruction_t *)succItem->data)->type == IREXITNODE) {
                    succItem = succItem->next;
                }
                succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succItem->data);
                assert(!xanBitSet_isBitSet(skipBlocks, IRMETHOD_getBasicBlockPosition(schedule->method, succBlock)));
                additions |= addBlockSkipMapping(block, succBlock, skipMap);
            }
        }
    }

    /* Return this hash table. */
    return skipMap;
}


/**
 * Get the block that should be skipped to given 'inst' and 'skipMap',
 * where 'inst' may or may not be in a block that is skipped.
 *
 * Returns the target basic block that should be skipped to.
 **/
static IRBasicBlock *
getTargetBasicBlock(schedule_info_t *sched, ir_instruction_t *inst, XanHashTable *skipMap) {
    IRBasicBlock *currBlock = IRMETHOD_getBasicBlockContainingInstruction(sched->schedule->method, inst);
    IRBasicBlock *target = xanHashTable_lookup(skipMap, currBlock);
    if (target) {
        return target;
    } else {
        return currBlock;
    }
}


/**
 * Check that the first non-skipped block is the same for each branch successor
 * for 'branch'.
 *
 * Returns JITTRUE if all successors skip to the same block.
 **/
static JITBOOLEAN
allBranchSuccsSkipToSameBlock(schedule_info_t *sched, ir_instruction_t *branch, XanHashTable *skipMap) {
    ir_instruction_t *succ = NULL;
    IRBasicBlock *targetBlock = NULL;
    assert(IRMETHOD_isABranchInstruction(branch));
    while ((succ = IRMETHOD_getSuccessorInstruction(sched->schedule->method, branch, succ))) {
        IRBasicBlock *succBlock = getTargetBasicBlock(sched, succ, skipMap);
        if (!targetBlock) {
            targetBlock = succBlock;
        } else if (targetBlock != succBlock) {
            return JITFALSE;
        }
    }
    return JITTRUE;
}


/**
 * Check whether all successor blocks to the given block are within the region,
 * whether this block ends with a branch or not.
 *
 * Returns JITTRUE if all successor blocks are within the region.
 **/
static JITBOOLEAN
allSuccBlocksInRegion(schedule_info_t *sched, IRBasicBlock *block) {
    schedule_t *schedule = sched->schedule;
    XanListItem *succItem = xanList_first(schedule->successors[block->endInst]);
    while (succItem) {
        ir_instruction_t *succ = succItem->data;
        if (!xanBitSet_isBitSet(schedule->regionInsts, succ->ID) && succ->type != IREXITNODE) {
            return JITFALSE;
        }
        succItem = succItem->next;
    }
    return JITTRUE;
}


/**
 * Check whether any successor blocks to the given block are within the region.
 *
 * Returns JITTRUE if any successor block is within the region.
 **/
static JITBOOLEAN
anySuccBlockInRegion(schedule_t *schedule, IRBasicBlock *block) {
    XanListItem *succItem = xanList_first(schedule->successors[block->endInst]);
    while (succItem) {
        ir_instruction_t *succ = succItem->data;
        if (xanBitSet_isBitSet(schedule->regionInsts, succ->ID) && succ->type != IREXITNODE) {
            return JITTRUE;
        }
        succItem = succItem->next;
    }
    return JITFALSE;
}


/**
 * Build a hash table mapping instructions to a successor relation which
 * points to their target and fall-through instructions.  The instructions
 * that are included are those that are moved.
 *
 * Returns the hash table mapping instructions to successor relations.
 **/
static XanHashTable *
buildInstSuccessorMapping(schedule_info_t *sched, XanBitSet *instsToClone, XanBitSet *blocksToCloneFrom) {
    JITINT32 b = -1;
    JITINT32 i = -1;
    XanHashTable *succMap;
    XanHashTable *skipMap;
    XanBitSet *skipBlocks;
    schedule_t *schedule = sched->schedule;

    /* Debug output. */
    PDEBUG(3, "Blocks to clone from:");
    PDEBUGSTMT(3, printBlockBitSet(3, schedule->method, schedule->numBlocks, blocksToCloneFrom));

    /* Blocks that should be skipped. */
    PDEBUG(3, "Blocks to skip:");
    skipBlocks = xanBitSet_new(schedule->numBlocks);
    while ((b = xanBitSet_getFirstBitSetInRange(schedule->regionBlocks, b + 1, schedule->numBlocks)) != -1) {
        if (!xanBitSet_isBitSet(blocksToCloneFrom, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            if (anySuccBlockInRegion(schedule, block)) {
                JITINT32 n = -1;
                while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
                    if (n < block->startInst || block->endInst < n) {
                        xanBitSet_setBit(skipBlocks, b);
                        PDEBUGC(3, " (%u-%u)", block->startInst, block->endInst);
                    }
                }
            }
        }
    }
    PDEBUGC(3, "\n");

    /* A mapping between blocks to skip and their successor to skip to. */
    skipMap = buildBlockSkipMapping(sched, skipBlocks);
    if (!skipMap) {
        xanBitSet_free(skipBlocks);
        return NULL;
    }

    /* A mapping between instructions and their successors. */
    PDEBUG(1, "Build successor relations\n");
    succMap = newHashTable();
    while ((i = xanBitSet_getFirstBitSetInRange(instsToClone, i + 1, schedule->numInsts)) != -1) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
        IRBasicBlock *block = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, inst);
        succ_relation_t *succRelation = allocFunction(sizeof(succ_relation_t));

        /* Branch targets and fall-throughs. */
        if (IRMETHOD_isABranchInstruction(inst)) {
            XanListItem *succItem = xanList_first(schedule->successors[i]);
            while (succItem) {
                ir_instruction_t *succ = succItem->data;
                IRBasicBlock *succBlock = getTargetBasicBlock(sched, succ, skipMap);
                JITINT32 firstCloned = xanBitSet_getFirstBitSetInRange(instsToClone, succBlock->startInst, succBlock->endInst);

                /* Find cloned target of branch. */
                if (succ == IRMETHOD_getBranchDestination(schedule->method, inst)) {
                    if (firstCloned != -1) {
                        succRelation->target = IRMETHOD_getInstructionAtPosition(schedule->method, firstCloned);
                        PDEBUG(2, "  Target for inst %u inst %u (cloned)\n", inst->ID, firstCloned);
                    } else {
                        succRelation->target = IRMETHOD_getInstructionAtPosition(schedule->method, succBlock->startInst);
                        PDEBUG(2, "  Target for inst %u inst %u (not cloned)\n", inst->ID, succBlock->startInst);
                    }
                }

                /* Find cloned target of fall-through. */
                else {
                    if (firstCloned != -1) {
                        succRelation->fallThrough = IRMETHOD_getInstructionAtPosition(schedule->method, firstCloned);
                        PDEBUG(2, "  Fall-through for inst %u inst %u (cloned)\n", inst->ID, firstCloned);
                    } else {
                        succRelation->fallThrough = IRMETHOD_getInstructionAtPosition(schedule->method, succBlock->startInst);
                        PDEBUG(2, "  Fall-through for inst %u inst %u (not cloned)\n", inst->ID, succBlock->startInst);
                    }
                }
                succItem = succItem->next;
            }
        }

        /* Not a branch instruction. */
        else {
            JITINT32 nextCloned;

            /* This is the last instruction in the region on this path. */
            if (!xanBitSet_isBitSet(schedule->regionInsts, i + 1)) {
                succRelation->fallThrough = IRMETHOD_getInstructionAtPosition(schedule->method, i + 1);
                PDEBUG(2, "    Fall-through for non-branch final inst %u inst %u (not cloned)\n", inst->ID, i + 1);
            }

            /* Another clone in this basic block. */
            else if ((nextCloned = xanBitSet_getFirstBitSetInRange(instsToClone, i + 1, block->endInst)) != -1) {
                succRelation->fallThrough = IRMETHOD_getInstructionAtPosition(schedule->method, nextCloned);
                PDEBUG(2, "    Fall-through for non-branch inst %u inst %u (cloned)\n", inst->ID, nextCloned);
            }

            /* No more clones in this basic block, must fall through. */
            else {
                ir_instruction_t *endInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
                ir_instruction_t *succ = IRMETHOD_getSuccessorInstruction(schedule->method, endInst, NULL);
                IRBasicBlock *succBlock = getTargetBasicBlock(sched, succ, skipMap);
                JITINT32 firstCloned = xanBitSet_getFirstBitSetInRange(instsToClone, succBlock->startInst, succBlock->endInst);
                assert(!IRMETHOD_isABranchInstruction(endInst) || firstCloned == -1 || !allSuccBlocksInRegion(sched, block) || allBranchSuccsSkipToSameBlock(sched, endInst, skipMap));
                if (firstCloned != -1) {
                    succRelation->fallThrough = IRMETHOD_getInstructionAtPosition(schedule->method, firstCloned);
                    PDEBUG(2, "    Fall-through for non-branch inst %u inst %u (cloned)\n", inst->ID, firstCloned);
                } else {
                    succRelation->fallThrough = IRMETHOD_getInstructionAtPosition(schedule->method, succBlock->startInst);
                    PDEBUG(2, "    Fall-through for non-branch inst %u inst %u (not cloned)\n", inst->ID, succBlock->startInst);
                }
            }
        }

        /* Insert this relation and move on. */
        xanHashTable_insert(succMap, inst, succRelation);
    }

    /* Clean up. */
    freeHashTable(skipMap);
    xanBitSet_free(skipBlocks);

    /* Return the mapping of blocks to successors. */
    return succMap;
}


/**
 * Build a list of instructions to delete after cloning has been performed.
 *
 * Returns a list of instructions to delete after cloning.
 **/
static XanList *
buildInstsToDeleteList(schedule_info_t *sched, XanBitSet *instsToMove, XanBitSet *blocksToMove) {
    JITUINT32 b, i;
    schedule_t *schedule = sched->schedule;

    /* Check that all instructions in the blocks to move are included. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            for (i = block->startInst; i <= block->endInst; ++i) {
                if (!xanBitSet_isBitSet(instsToMove, i)) {
                    ir_instruction_t *inst JITUNUSED = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                    assert(inst->type == IRLABEL);
                }
            }
        }
    }

    /* Convert to a list. */
    return buildListFromInstBitSet(schedule->method, schedule->numInsts, instsToMove);
}


/**
 * Create fake edges between 'nextToInst' in 'sched' and all predecessors
 * where clones of 'instList' should be placed.  Put those edges in
 * 'edgeCloneMap'.  Also checks any blocked instructions that were skipped
 * to ensure that there are no dependences with them.
 *
 * Returns JITFALSE if there were dependences with blocked instructions,
 * JITFALSE otherwise.
 **/
static void
createFakeEdgesAfterPreds(schedule_info_t *sched, XanList *instList, XanHashTable *edgeCloneMap) {
    JITINT32 n = -1;
    schedule_t *schedule = sched->schedule;
    while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
        ir_instruction_t *nextToInst = IRMETHOD_getInstructionAtPosition(schedule->method, n);
        XanList *reachedInsts = newList();
        XanListItem *instItem;

        /* Select instructions to clone this time. */
        instItem = xanList_first(instList);
        while (instItem) {
            ir_instruction_t *toClone = instItem->data;
            if (IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, toClone, nextToInst)) {
                xanList_append(reachedInsts, toClone);
            }
            instItem = instItem->next;
        }

        /* Clone instructions on this edge. */
        instItem = xanList_first(schedule->predecessors[n]);
        while (instItem) {
            ir_instruction_t *pred = instItem->data;
            edge_t *fakeEdge = createEdge(pred, nextToInst);
            XanList *clonedList = xanList_cloneList(reachedInsts);
            PDEBUG(2, "  Created fake edge %u -> %d\n", pred->ID, n);
            xanHashTable_insert(edgeCloneMap, fakeEdge, clonedList);
            instItem = instItem->next;
        }

        /* Clean up. */
        freeList(reachedInsts);
    }
}


/**
 * Create fake edges between 'nextToInst' in 'sched' and all successors
 * where clones of 'instList' should be placed.  Put those edges in
 * 'edgeCloneMap'.
 **/
static void
createFakeEdgesBeforeSuccs(schedule_info_t *sched, XanList *instList, XanHashTable *edgeCloneMap) {
    JITINT32 n = -1;
    schedule_t *schedule = sched->schedule;
    while ((n = xanBitSet_getFirstBitSetInRange(sched->nextToInsts, n + 1, schedule->numInsts)) != -1) {
        ir_instruction_t *nextToInst = IRMETHOD_getInstructionAtPosition(schedule->method, n);
        XanList *reachedInsts = newList();
        XanListItem *instItem;

        /* Select instructions to clone this time. */
        instItem = xanList_first(instList);
        while (instItem) {
            ir_instruction_t *toClone = instItem->data;
            if (IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, nextToInst, toClone)) {
                xanList_append(reachedInsts, toClone);
            } else {

                /* This needs thinking about.  What if toClone is in a loop - it needs
                   to follow a back edge to reach nextToInst - but it shouldn't follow
                   all back edges otherwise it might stray out of the region.  It's
                   only region back edges we should follow. */
                abort();
            }
            instItem = instItem->next;
        }

        /* Clone instructions on this edge. */
        instItem = xanList_first(schedule->successors[n]);
        while (instItem) {
            ir_instruction_t *succ = instItem->data;
            if (succ->ID < schedule->numInsts) {
                edge_t *fakeEdge = createEdge(nextToInst, succ);
                XanList *clonedList = xanList_cloneList(instList);
                PDEBUG(2, "  Created fake edge %d -> %u\n", n, succ->ID);
                xanHashTable_insert(edgeCloneMap, fakeEdge, clonedList);
            }
            instItem = instItem->next;
        }
        freeList(reachedInsts);
    }
}


/**
 * Create fake edges and place them in 'edgeCloneMap' for the locations
 * where instructions should be moved to.  Moving instructions upwards and
 * before an instruction will require only one fake edge, as long as that
 * instruction isn't a label.  Moving downwards and after an instruction
 * likewise, as long as it's not a conditional branch.  Otherwise, two
 * edges will be required to split the instructions to be moved into before
 * and after sets, or to place instructions on all edges into and out of
 * the block.
 **/
static void
createFakeEdgesForCloningInsts(schedule_info_t *sched, XanBitSet *instsToClone, XanHashTable *edgeCloneMap) {
    JITUINT32 i;
    XanList *cloneInstsList;
    schedule_t *schedule = sched->schedule;
    PDEBUG(1, "Creating fake edges for placing instructions\n");

    /* Instructions kept together on all edges into the instruction. */
    cloneInstsList = newList();
    if ((sched->position == Before && sched->direction == Upwards) ||
            (sched->position == After && sched->direction == Downwards) ||
            xanBitSet_getCountOfBitsSet(instsToClone) == 1) {

        /* A list of instructions to be cloned. */
        for (i = 0; i < schedule->numInsts; ++i) {
            if (xanBitSet_isBitSet(instsToClone, i)) {
                ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                insertInstIntoSortedList(inst, cloneInstsList);
            }
        }

        /* Clone on all edges into the region.  This may be a single edge
           around nextToInst, or it may be multiple edges if nextToInst is a
           blocked instruction that is blocked with a branch. */
        if (sched->position == Before) {
            createFakeEdgesAfterPreds(sched, cloneInstsList, edgeCloneMap);
        } else {
            createFakeEdgesBeforeSuccs(sched, cloneInstsList, edgeCloneMap);
        }
    }

    /* Instructions to move are split. */
    else {
        /* JITINT32 cutInstID = -1; */
        /* XanBitSet *moveInsts = xanBitSet_new(schedule->numInsts); */
        /* XanListItem *instItem = xanList_first(sched->instsToMove); */

        /* Can't deal with this yet. */
        PDEBUG(0, "Fail: Can't deal with splitting instructions yet\n");
        sched->shouldFail = JITTRUE;
        return;

        /* /\* Initial list contains all required instructions to move. *\/ */
        /* while (instItem) { */
        /*   ir_instruction_t *inst = instItem->data; */
        /*   xanBitSet_setBit(moveInsts, inst->ID); */
        /*   instItem = instItem->next; */
        /* } */

        /* /\* We need to find the instruction that reaches all moveInsts, with the minimum number of other instructions. *\/ */
        /* for (i = 0; i < schedule->numInsts; ++i) { */
        /*   if (xanBitSet_isBitSet(instsToClone, i) && xanBitSet_isSubSetOf(moveInsts, sched->branchingReachedInsts[i])) { */
        /*     if (cutInstID == -1 || xanBitSet_getCountOfBitsSet(sched->branchingReachedInsts[i]) < xanBitSet_getCountOfBitsSet(sched->branchingReachedInsts[cutInstID])) { */
        /*       cutInstID = i; */
        /*     } */
        /*   } */
        /* } */

        /* /\* Populate the first list. *\/ */
        /* assert(cutInstID != -1); */
        /* cloneInstsList = newList(); */
        /* for (i = 0; i < schedule->numInsts; ++i) { */
        /*   if (xanBitSet_isBitSet(instsToClone, i) && xanBitSet_isBitSet(sched->branchingReachedInsts[cutInstID], i)) { */
        /*     ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i); */
        /*     xanBitSet_setBit(moveInsts, i); */
        /*     insertInstIntoSortedList(inst, cloneInstsList); */
        /*   } */
        /* } */

        /* /\* Create clone edges for this. *\/ */
        /* if (sched->position == Before) { */
        /*   createFakeEdgesAfterPreds(sched, cloneInstsList, edgeCloneMap); */
        /* } else { */
        /*   createFakeEdgesBeforeSuccs(sched, cloneInstsList, edgeCloneMap); */
        /* } */

        /* /\* Invert the list, if required. *\/ */
        /* if (!xanBitSet_equal(moveInsts, instsToClone)) { */
        /*   xanList_emptyOutList(cloneInstsList); */
        /*   for (i = 0; i < schedule->numInsts; ++i) { */
        /*     if (xanBitSet_isBitSet(instsToClone, i) && !xanBitSet_isBitSet(moveInsts, i)) { */
        /*       ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i); */
        /*       insertInstIntoSortedList(inst, cloneInstsList); */
        /*     } */
        /*   } */
        /*   if (sched->position == Before) { */
        /*     createFakeEdgesBeforeSuccs(sched, cloneInstsList, edgeCloneMap); */
        /*   } else { */
        /*     createFakeEdgesAfterPreds(sched, cloneInstsList, edgeCloneMap); */
        /*   } */
        /* } */

        /* /\* Clean up. *\/ */
        /* xanBitSet_free(moveInsts); */
    }

    /* Clean up. */
    freeList(cloneInstsList);
}


/**
 * Unmark blocks in 'blocksToMove' for blocks that don't have all
 * instructions marked in 'instsToMove'.  If there are labels or nops that
 * are not marked in this bit set they are ignored.
 *
 * Returns JITTRUE if any blocks were unmarked, JITFALSE otherwise.
 **/
static JITBOOLEAN
unmarkBlocksWithoutAllInstsMarked(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *blocksToMove) {
    JITUINT32 b, i;
    JITBOOLEAN changes = JITFALSE;

    /* Identify blocks with all instructions marked. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            for (i = block->startInst; i <= block->endInst; ++i) {
                ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                if (!xanBitSet_isBitSet(instsToMove, i) && inst->type != IRLABEL && inst->type != IRNOP) {
                    xanBitSet_clearBit(blocksToMove, b);
                    PDEBUG(2, "  Not all insts marked in block (%u-%u)\n", block->startInst, block->endInst);
                    changes = JITTRUE;
                    break;
                }
            }
        }
    }

    /* Return whether there were changes. */
    return changes;
}


/**
 * Remove blocks from 'blocksToMove' if they are a fall-through target of a
 * block that isn't being moved.
 *
 * This is safe if there is a post-dominator block that is unmoved and all
 * blocks between the unmoved block and post-dominator are moved.  However,
 * this hasn't been implemented yet.
 **/
static void
removeFallThroughBlocks(schedule_t *schedule, XanBitSet *blocksToMove) {
    JITUINT32 b;
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->startInst);
            XanListItem *predItem = xanList_first(schedule->predecessors[startInst->ID]);
            while (predItem) {
                ir_instruction_t *pred = predItem->data;
                if (!IRMETHOD_isABranchInstruction(pred) || startInst != IRMETHOD_getBranchDestination(schedule->method, pred)) {
                    IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);
                    JITUINT32 predBlockID = IRMETHOD_getBasicBlockPosition(schedule->method, predBlock);
                    if (!xanBitSet_isBitSet(blocksToMove, predBlockID)) {
                        xanBitSet_clearBit(blocksToMove, b);
                        PDEBUG(2, "  Not moving fall-through block (%u-%u)\n", block->startInst, block->endInst);
                        break;
                    }
                }
                predItem = predItem->next;
            }
        }
    }
}


/* /\** */
/*  * Remove blocks from 'blocksToMove' if they end in a conditional branch. */
/*  **\/ */
/* static void */
/* removeConditionalBranchBlocks(schedule_info_t *sched, XanBitSet *blocksToMove) */
/* { */
/*   JITUINT32 b; */
/*   schedule_t *schedule = sched->schedule; */
/*   for (b = 0; b < schedule->numBlocks; ++b) { */
/*     if (xanBitSet_isBitSet(blocksToMove, b)) { */
/*       IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b); */
/*       ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst); */
/*       if (IRMETHOD_isABranchInstruction(branch) && branch->type != IRBRANCH) { */
/*         xanBitSet_clearBit(blocksToMove, b); */
/*         PDEBUG(2, "  Not moving conditional branch in block (%u-%u)\n", block->startInst, block->endInst); */
/*       } */
/*     } */
/*   } */
/* } */


/**
 * Remove blocks from 'blocksToMove' if they end in a conditional branch and
 * both successors are not moved.  This is because the conditional branch must
 * stay to choose between the two unmoving successors.
 *
 * Returns JITTRUE if any blocks were removed, JITFALSE otherwise.
 **/
static JITBOOLEAN
removeConditionalBranchBlocksWithBothSuccessorsUnmoved(schedule_t *schedule, XanBitSet *blocksToMove) {
    JITUINT32 b;
    JITBOOLEAN removals = JITFALSE;

    /* Find blocks ending in conditional branches. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
            if (IRMETHOD_isABranchInstruction(branch) && branch->type != IRBRANCH) {
                JITUINT32 unmovedSuccs = 0;
                XanListItem *succItem = xanList_first(schedule->successors[branch->ID]);
                assert(xanList_length(schedule->successors[branch->ID]) == 2);

                /* Check whether successors are also moved. */
                while (succItem) {
                    ir_instruction_t *succ = succItem->data;
                    IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                    JITINT32 sbPos = IRMETHOD_getBasicBlockPosition(schedule->method, succBlock);
                    if (succ->ID < schedule->numInsts && !xanBitSet_isBitSet(blocksToMove, sbPos)) {
                        unmovedSuccs += 1;
                    }
                    succItem = succItem->next;
                }

                /* Remove if too many successors unmoved. */
                if (unmovedSuccs > 1) {
                    if (xanBitSet_isBitSet(blocksToMove, b)) {
                        xanBitSet_clearBit(blocksToMove, b);
                        removals = JITTRUE;
                        PDEBUG(2, "  Block (%u-%u) cannot be moved because successors not moved.\n", block->startInst, block->endInst);
                    }
                }
            }
        }
    }

    /* Return whether there were changes. */
    PDEBUG(4, "  Removed cond branch blocks with both succs unmoved with%s changes\n", removals ? "" : "out");
    return removals;
}


/**
 * Remove blocks from 'blocksToMove' if they end in a conditional branch and
 * there is no single post-dominator block with all intervening blocks being
 * moved.  Put another way, a conditional branch can only be moved if there is
 * a single post-dominator block (that can be moved or unmoved) and all blocks
 * between this block and the post-dominator are also moved.
 *
 * Returns JITTRUE if any blocks were removed, JITFALSE otherwise.
 **/
static JITBOOLEAN
removeConditionalBranchBlocksWithNoPostDominator(schedule_t *schedule, XanBitSet *blocksToMove) {
    JITUINT32 b;
    JITINT32 **postdoms;
    JITBOOLEAN removals = JITFALSE;

    /* Set up analysis. */
    postdoms = allocFunction(schedule->numInsts * sizeof(JITINT32 *));

    /* Find blocks ending in conditional branches. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
            if (IRMETHOD_isABranchInstruction(branch) && branch->type != IRBRANCH) {
                XanList *worklist = newList();
                XanBitSet *blocksSeen = xanBitSet_new(schedule->numBlocks);
                JITBOOLEAN finished = JITFALSE;
                JITBOOLEAN foundPostDominator = JITFALSE;
                xanList_append(worklist, block);
                xanBitSet_setBit(blocksSeen, b);
                /* PDEBUG("Checking conditional branch %u (%u-%u)\n", branch->ID, block->startInst, block->endInst); */

                /* Find a post-dominator and check all blocks are moved. */
                while (!finished && xanList_length(worklist) > 0) {
                    XanListItem *succItem;
                    XanListItem *blkItem = xanList_first(worklist);
                    IRBasicBlock *blk = blkItem->data;
                    ir_instruction_t *end = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
                    JITINT32 blkPos = IRMETHOD_getBasicBlockPosition(schedule->method, blk);
                    xanList_deleteItem(worklist, blkItem);
                    assert(xanBitSet_isBitSet(blocksSeen, blkPos));
                    /* PDEBUG("  Block (%u-%u)\n", blk->startInst, blk->endInst); */

                    /* This block must be moved. */
                    if (!xanBitSet_isBitSet(blocksToMove, blkPos)) {
                        xanBitSet_clearBit(blocksToMove, b);
                        removals = JITTRUE;
                        finished = JITTRUE;
                    }

                    /* Put successors on the worklist if all region predecessors are there. */
                    assert(xanList_length(schedule->successors[end->ID]) > 0);
                    assert(xanList_length(schedule->successors[end->ID]) <= 2);
                    succItem = xanList_first(schedule->successors[end->ID]);
                    while (succItem) {
                        ir_instruction_t *succ = succItem->data;
                        IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                        JITINT32 sbPos = IRMETHOD_getBasicBlockPosition(schedule->method, succBlock);
                        JITBOOLEAN allSeen = JITTRUE;
                        XanListItem *predItem;

                        /* Skip if seen, as happens in loops. */
                        if (succ->ID >= schedule->numInsts && xanBitSet_isBitSet(blocksSeen, sbPos)) {
                            succItem = succItem->next;
                            continue;
                        }

                        /* Check all region predecessors. */
                        assert(xanList_length(schedule->predecessors[succ->ID]) > 0);
                        predItem = xanList_first(schedule->predecessors[succ->ID]);
                        while (allSeen && predItem) {
                            ir_instruction_t *pred = predItem->data;
                            IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);
                            JITINT32 pbPos = IRMETHOD_getBasicBlockPosition(schedule->method, predBlock);
                            if (xanBitSet_isBitSet(schedule->regionBlocks, pbPos) && !xanBitSet_isBitSet(blocksSeen, pbPos)) {
                                allSeen = JITFALSE;
                            }
                            predItem	= predItem->next;
                        }

                        /* Check for a post-dominator and add this successor. */
                        if (allSeen) {
                            if (TOOLS_isInstructionADominator(schedule->method, schedule->numInsts - 1, succ, branch, postdoms, IRMETHOD_isInstructionAPostdominator)) {
                                foundPostDominator = JITTRUE;
                            } else if (xanBitSet_isBitSet(schedule->regionInsts, succ->ID) && !xanBitSet_isBitSet(blocksSeen, sbPos)) {
                                xanList_append(worklist, succBlock);
                                xanBitSet_setBit(blocksSeen, sbPos);
                            }
                        }
                        succItem = succItem->next;
                    }
                }

                /* Must have found a post-dominator. */
                if (!foundPostDominator) {
                    xanBitSet_clearBit(blocksToMove, b);
                    removals = JITTRUE;
                }

                /* Clean up. */
                xanBitSet_free(blocksSeen);
                xanList_destroyList(worklist);
            }
        }
    }

    /* Free the memory. */
    for (b = 0; b < schedule->numInsts; ++b) {
        freeFunction(postdoms[b]);
    }
    freeFunction(postdoms);

    /* Return whether there were changes. */
    PDEBUG(4, "  Removed cond branch blocks without post-dominator with%s changes\n", removals ? "" : "out");
    return removals;
}


/**
 * Ensure that 'instsToMove' reflects 'blocksToMove' and vice-versa.  This
 * means that 'blocksToMove' should not contain blocks where some
 * instructions are not moved and then branches at the end of blocks that
 * are not in 'blocksToMove' should not be marked.
 *
 * Returns JITTRUE if any changes were made, JITFALSE otherwise.
 **/
static JITBOOLEAN
syncInstsAndBlocksToMove(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *blocksToMove) {
    JITUINT32 b;
    JITBOOLEAN changes;

    /* Unmark blocks if not all instructions are moved. */
    changes = unmarkBlocksWithoutAllInstsMarked(schedule, instsToMove, blocksToMove);

    /* Clear branch instructions from non-moving blocks. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (!xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
            if (IRMETHOD_isABranchInstruction(branch)) {
                if (xanBitSet_isBitSet(instsToMove, branch->ID)) {
                    xanBitSet_clearBit(instsToMove, branch->ID);
                    PDEBUG(2, "  Not moving branch %u in unmoving block\n", branch->ID);
                    changes = JITTRUE;
                }
            }
        }
    }

    /* Return whether there were changes. */
    PDEBUG(4, "  Synced insts and blocks to move with%s changes\n", changes ? "" : "out");
    return changes;
}


/**
 * COMMENT NEEDS UPDATING
 *
 * Check instructions that are not moved to ensure that their source
 * variables are not redefined by instructions that are moved past them.
 * If they are, then these instructions that would pass them are marked
 * as unmoved too, which prevents them being moved.  However, they will be
 * cloned at least once (before or after nextToInst, since the cloned
 * instructions are the minimum set needed), so dependences that don't pass
 * must also be prevented from moving.  If these dependences are outside
 * the region of instructions to consider then there's going to be a
 * problem, so clear all instructions to move.  This will then cause later
 * checks to fail.
 *
 * At the moment this is far from perfect.  It doesn't consider the type
 * of dependence (source variable).
 *
 * Returns JITTRUE if any changes were made, JITFALSE otherwise.
 **/
static JITBOOLEAN
preventDependencesPassingUnmovedInsts(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *nextToInsts, move_dir_t direction, place_pos_t position) {
    JITINT32 i, j = -1;
    JITBOOLEAN changes = JITFALSE;
    for (i = 0; i < schedule->numInsts; ++i) {
        if (xanBitSet_isBitSet(schedule->regionInsts, i) && !xanBitSet_isBitSet(instsToMove, i)) {
            ir_instruction_t *unmoved = IRMETHOD_getInstructionAtPosition(schedule->method, i);

            /* Check the instruction itself. */
            while ((j = xanBitSet_getFirstBitSetInRange(instsToMove, j + 1, schedule->numInsts)) != -1) {
                ir_instruction_t *passing = IRMETHOD_getInstructionAtPosition(schedule->method, j);

                /* Actually, for now, don't move these instructions. */
                if ((direction == Upwards && IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, passing, unmoved) && hasDataDependence(schedule->dataDepGraph, i, j)) ||
                        (direction == Downwards && IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, unmoved, passing) && hasDataDependence(schedule->dataDepGraph, j, i))) {
                    if (!xanBitSet_isBitSet(nextToInsts, i) ||
                            (position == Before && direction == Upwards) ||
                            (position == After && direction == Downwards)) {
                        PDEBUG(2, "  Inst %d can't pass unmoved dependence %d\n", j, i);
                        PDEBUG(0, "Fail: Instructions trying to pass an unmoved dependence\n");
                        xanBitSet_clearBit(instsToMove, j);
                        /* PDEBUG(2, "  Clearing all insts to move from unmoved inst %u and dependence %u\n", i, j); */
                        /* xanBitSet_clearAll(instsToMove); */
                        changes = JITTRUE;
                    }
                }
            }
        }
    }

    /* Return whether there were changes. */
    PDEBUG(4, "  Prevented dependences passing unmoved insts with%s changes\n", changes ? "" : "out");
    return changes;
}


/**
 * Find the set of instructions that shouldn't be cloned because they will pass
 * an instruction that they have a dependence with that itself won't be moved
 * or cloned.  The initial set of instructions that can't be cloned are those
 * that have a dependence with an unmoved instruction?  Or is it more subtle
 * than this?
 **/
static XanBitSet *
findInstsCantBeCloned(schedule_t *schedule, XanBitSet *instsToClone, XanBitSet *instsToMove, move_dir_t direction, place_pos_t position) {
    JITINT32 i = -1;
    JITINT32 j = -1;
    JITBOOLEAN changes = JITTRUE;
    XanBitSet *instsCantClone = xanBitSet_new(schedule->numInsts);

    /* Initial set. */
    while ((i = xanBitSet_getFirstBitSetInRange(schedule->regionInsts, i + 1, schedule->numInsts)) != -1) {
        if (!xanBitSet_isBitSet(instsToMove, i)) {
            ir_instruction_t *unmoved = IRMETHOD_getInstructionAtPosition(schedule->method, i);
            while ((j = xanBitSet_getFirstBitSetInRange(instsToClone, j + 1, schedule->numInsts)) != -1) {
                ir_instruction_t *passing = IRMETHOD_getInstructionAtPosition(schedule->method, j);
                if ((direction == Upwards && IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, passing, unmoved) && hasDataDependence(schedule->dataDepGraph, i, j)) ||
                        (direction == Downwards && IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, unmoved, passing) && hasDataDependence(schedule->dataDepGraph, j, i))) {
                    PDEBUG(2, "  Inst %d can't pass unmoved dependence %d\n", j, i);
                    xanBitSet_setBit(instsCantClone, j);
                }
            }
        }
    }

    /* Add others. */
    while (changes) {
        changes = JITFALSE;
        while ((i = xanBitSet_getFirstBitSetInRange(instsCantClone, i + 1, schedule->numInsts)) != -1) {
            ir_instruction_t *unmoved = IRMETHOD_getInstructionAtPosition(schedule->method, i);
            while ((j = xanBitSet_getFirstBitSetInRange(instsToClone, j + 1, schedule->numInsts)) != -1) {
                if (!xanBitSet_isBitSet(instsCantClone, j)) {
                    ir_instruction_t *passing = IRMETHOD_getInstructionAtPosition(schedule->method, j);
                    if ((direction == Upwards && IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, passing, unmoved) && hasDataDependence(schedule->dataDepGraph, i, j)) ||
                            (direction == Downwards && IRMETHOD_canInstructionBeReachedFromPosition(schedule->method, unmoved, passing) && hasDataDependence(schedule->dataDepGraph, j, i))) {
                        PDEBUG(2, "  Inst %d can't pass unmoved dependence %d\n", j, i);
                        xanBitSet_setBit(instsCantClone, j);
                        changes = JITTRUE;
                    }
                }
            }
        }
    }
    return instsCantClone;
}


/**
 * Create an initial bit set of blocks to move by marking all blocks with
 * at least one instruction that will be moved.
 *
 * Returns a bit set of blocks to move.
 **/
static XanBitSet *
createInitialBlocksToMove(schedule_t *schedule, XanBitSet *instsToMove) {
    JITUINT32 b;
    XanBitSet *blocksToMove = xanBitSet_new(schedule->numBlocks);
    for (b = 0; b < schedule->numBlocks - 1; ++b) {
        IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
        JITINT32 firstInst = xanBitSet_getFirstBitSetInRange(instsToMove, block->startInst, block->endInst);
        if (firstInst != -1) {
            xanBitSet_setBit(blocksToMove, b);
        }
    }
    return blocksToMove;
}


/**
 * For each block in 'blocksToMove', ensure that labels and nops are marked in
 * 'instsToMove' so that the whole block will be moved.
 **/
static void
addLabelsAndNopsInMovingBlocks(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *blocksToMove) {
    JITUINT32 b, i;
    for (b = 0; b < schedule->numBlocks; ++b) {
        IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
        if (xanBitSet_isBitSet(blocksToMove, b)) {
            for (i = block->startInst; i <= block->endInst; ++i) {
                if (!xanBitSet_isBitSet(instsToMove, i)) {
                    ir_instruction_t *inst JITUNUSED = IRMETHOD_getInstructionAtPosition(schedule->method, i);
                    assert(inst->type == IRLABEL || inst->type == IRNOP);
                    xanBitSet_setBit(instsToMove, i);
                }
            }
        }
    }
}


/**
 * Identify blocks that can be moved.  For a branch to be moved, all
 * instructions in its basic block must also be moved, and it can't be the
 * fall-through successor of one of its predecessors.  In addition, for
 * conditional branches, there must be a single post-dominator block that
 * is not moved and is the first block reached along each successor edge.
 * To allow moving without having all instructions in the block being
 * moved too, it must be possible to fall-through to the single successor
 * block, but determining that is hard and not currently performed.
 *
 * Returns a bit set identifying blocks that can be moved.
 **/
static XanBitSet *
identifyInstsAndBlocksToMove(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *nextToInsts, move_dir_t direction, place_pos_t position) {
    JITBOOLEAN changes;
    XanBitSet *blocksToMove;

    /* Find the initial set of blocks that can move. */
    PDEBUG(1, "Finding insts and blocks to move\n");
    blocksToMove = createInitialBlocksToMove(schedule, instsToMove);
    unmarkBlocksWithoutAllInstsMarked(schedule, instsToMove, blocksToMove);

    /* Remove blocks that are a fall-through. */
    removeFallThroughBlocks(schedule, blocksToMove);

    /* Only removing instructions, so will reach fixed point. */
    changes = JITTRUE;
    while (changes) {

        /* Sync-up the instructions to move. */
        changes = syncInstsAndBlocksToMove(schedule, instsToMove, blocksToMove);

        /* Dependences can't pass unmoved instructions. */
        changes |= preventDependencesPassingUnmovedInsts(schedule, instsToMove, nextToInsts, direction, position);

        /* Conditional branches not moved if both successors unmoved. */
        changes |= removeConditionalBranchBlocksWithBothSuccessorsUnmoved(schedule, blocksToMove);

        /* Conditional branches are only moved in certain situations. */
        changes |= removeConditionalBranchBlocksWithNoPostDominator(schedule, blocksToMove);
    }

    /* Now there's a fixed point, add in labels and nops from moving blocks. */
    addLabelsAndNopsInMovingBlocks(schedule, instsToMove, blocksToMove);

    /* Debugging. */
    PDEBUG(3, "Blocks that can be moved:");
    PDEBUGSTMT(3, printBlockBitSet(3, schedule->method, schedule->numBlocks, blocksToMove));
    PDEBUG(3, "Insts that can be moved:");
    PDEBUGSTMT(3, printInstBitSet(3, schedule->numInsts, instsToMove));

    /* Return blocks that can be moved entirely. */
    return blocksToMove;
}


/**
 * Filter instructions that can't be moved.  For a branch to be moved, all
 * instructions in its basic block must also be moved, and it can't be the
 * fall-through successor of one of its predecessors.  In addition, for
 * conditional branches, there must be a single post-dominator block that
 * is not moved and is the first block reached along each successor edge.
 * To allow moving without having all instructions in the block being
 * moved too, it must be possible to fall-through to the single successor
 * block, but determining that is hard and not currently performed.
 **/
void
SCHEDULE_filterInstsCantBeMoved(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *outerInsts, JITBOOLEAN upwards, JITBOOLEAN before) {
    XanBitSet *blocksToMove;
    setDebugLevel();
    PDEBUG(1, "Filtering instructions that can't be moved\n");
    PDEBUG(2, "  Moving:");
    PDEBUGSTMT(2, printInstBitSet(2, schedule->numInsts, instsToMove));
    PDEBUG(2, "  Beyond:");
    PDEBUGSTMT(2, printInstBitSet(2, schedule->numInsts, outerInsts));
    blocksToMove = identifyInstsAndBlocksToMove(schedule, instsToMove, outerInsts, upwards ? Upwards : Downwards, before ? Before : After);
    xanBitSet_free(blocksToMove);
}


/**
 * Find the set of instructions that can't be moved or cloned.  This is
 * basically instructions that are dependent on others that don't move, since
 * passing these instructions would not preserve the dependence.
 **/
XanBitSet *
SCHEDULE_findInstsCantBeCloned(schedule_t *schedule, XanBitSet *instsToClone, XanBitSet *instsToMove, JITBOOLEAN upwards, JITBOOLEAN before) {
    setDebugLevel();
    PDEBUG(1, "Finding instructions that can't be cloned\n");
    PDEBUG(2, "  Cloning:");
    PDEBUGSTMT(2, printInstBitSet(2, schedule->numInsts, instsToClone));
    PDEBUG(2, "  Moving:");
    PDEBUGSTMT(2, printInstBitSet(2, schedule->numInsts, instsToMove));
    return findInstsCantBeCloned(schedule, instsToClone, instsToMove, upwards ? Upwards : Downwards, before ? Before : After);
}


/**
 * Update an existing branch patch or create a new one if 'patch' doesn't
 * already exist.
 *
 * Returns a new or updated branch patch.
 **/
static branch_patch_t *
updateBranchPatch(branch_patch_t *patch, ir_instruction_t *branch, ir_instruction_t *target, ir_instruction_t *fallThrough) {
    if (!patch) {
        patch = allocFunction(sizeof(branch_patch_t));
        patch->branch = branch;
    }
    assert(patch->branch == branch);
    if (target) {
        patch->target = target;
    }
    if (fallThrough) {
        patch->fallThrough = fallThrough;
    }
    return patch;
}


/**
 * Update a branch patch with an instruction to target.
 **/
static branch_patch_t *
buildSingleBranchPatch(schedule_info_t *sched, ir_instruction_t *branch, ir_instruction_t *succ, branch_patch_t *patch, XanHashTable *skipMap) {
    schedule_t *schedule = sched->schedule;
    IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
    IRBasicBlock *targetBlock = getTargetBasicBlock(sched, succ, skipMap);

    /* The block is skipped. */
    if (targetBlock != succBlock) {
        ir_instruction_t *firstInst = IRMETHOD_getInstructionAtPosition(schedule->method, targetBlock->startInst);

        /* Ideally we'd get a label for the target, creating one if one doesn't
           exist for some reason.  However, adding a label here means that the
           basic block analysis becomes stale, so we'd have to recompute if
           required later.  To avoid this, we create the label when we actually
           use the branch patch. */
        if (succ == IRMETHOD_getBranchDestination(schedule->method, branch)) {
            patch = updateBranchPatch(patch, branch, firstInst, NULL);
        } else {
            patch = updateBranchPatch(patch, branch, NULL, firstInst);
        }
    }

    /* The block is not skipped, but all instructions in it could still
       be moved if the last instruction in the block is the last
       instruction to be moved.  Then there will be no skip mapping.  We
       still need to take care of this branch then, because the successor
       basic block will be empty afterwards. */
    else {
        if (succ == IRMETHOD_getBranchDestination(schedule->method, branch)) {
            ir_instruction_t *label = getTargetLabel(schedule->method, succ);
            patch = updateBranchPatch(patch, branch, label, NULL);
        } else {
            patch = updateBranchPatch(patch, branch, NULL, succ);
        }
    }
    return patch;
}


/**
 * Build a list of branch patches that need to be applied to blocks that
 * aren't in 'blocksToMove'.
 *
 * Returns a list of branch patches.
 **/
static XanList *
buildBranchPatchList(schedule_info_t *sched, XanBitSet *blocksToMove, XanHashTable *skipMap) {
    JITUINT32 b;
    XanList *patchList;
    schedule_t *schedule = sched->schedule;

    /* Create the list, starting with blocks that are in the region. */
    PDEBUG(2, "  Building branch patch list\n");
    patchList = newList();
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(schedule->regionBlocks, b) && !xanBitSet_isBitSet(blocksToMove, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);

            /* Obviously only patching branches. */
            if (xanBitSet_isBitSet(schedule->regionInsts, branch->ID) && IRMETHOD_isABranchInstruction(branch)) {
                branch_patch_t *patch = NULL;
                XanListItem *succItem = xanList_first(schedule->successors[branch->ID]);
                PDEBUG(2, "  Creating patch for region branch %u\n", branch->ID);

                /* Check all successors. */
                while (succItem) {
                    ir_instruction_t *succ = succItem->data;
                    patch = buildSingleBranchPatch(sched, branch, succ, patch, skipMap);
                    succItem = succItem->next;
                }

                /* Add the patch, if created. */
                if (patch) {
                    xanList_append(patchList, patch);
                }
            }
        }
    }

    /* Consider branches that are outside the region but branch to a block that
       will be moved that is inside the region. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (!xanBitSet_isBitSet(schedule->regionBlocks, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            ir_instruction_t *branch = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
            XanListItem *succItem = xanList_first(schedule->successors[branch->ID]);
            branch_patch_t *patch = NULL;

            /* Check all successors. */
            while (succItem) {
                ir_instruction_t *succ = succItem->data;
                IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                if (succ->ID < schedule->numInsts && xanBitSet_isBitSet(blocksToMove, IRMETHOD_getBasicBlockPosition(schedule->method, succBlock))) {
                    if (!patch) {
                        PDEBUG(2, "  Creating patch for non-region branch %u\n", branch->ID);
                    }

                    /* For now, ensure that this is a branch. */
                    if (!IRMETHOD_isABranchInstruction(branch)) {
                        abort();
                    }

                    /* Create or update the patch. */
                    patch = buildSingleBranchPatch(sched, branch, succ, patch, skipMap);
                }
                succItem = succItem->next;
            }

            /* Add the patch, if created. */
            if (patch) {
                xanList_append(patchList, patch);
            }
        }
    }

    /* Debugging output. */
    if (xanList_length(patchList) == 0) {
        PDEBUG(2, "  No branches to patch\n");
    }
    return patchList;
}


/**
 * Patch edges in 'edgeList' to reflect changes in the predecessor and
 * successor instructions along the edge.  If the instructions 'newPred' or
 * 'newSucc' are NULL, then the corresponding instruction in the edge will not
 * be altered.
 **/
static void
patchEdgesAfterBranchPatch(XanList *edgeList, ir_instruction_t *oldPred, ir_instruction_t *oldSucc, ir_instruction_t *newPred, ir_instruction_t *newSucc) {
    XanListItem *edgeItem = xanList_first(edgeList);
    while (edgeItem) {
        edge_t *edge = edgeItem->data;
        if (edge->pred == oldPred && edge->succ == oldSucc) {
            if (newPred) {
                edge->pred = newPred;
            }
            if (newSucc) {
                edge->succ = newSucc;
            }
            PDEBUG(2, "    Fixed edge (%u-%u) to (%u-%u)\n", oldPred->ID, oldSucc->ID, edge->pred->ID, edge->succ->ID);
        }
        edgeItem = xanList_next(edgeItem);
    }
}


/**
 * Patch branches that aren't moved so that they target the correct successor
 * basic blocks, adding new labels if necessary.  Using a skip map, a list of
 * patches to apply is generated.  These are then applied to the targets and
 * fall-throughs of each branch.  At the same time, check that no edges in
 * 'edgeList' have been altered.  If they have then fix them up.
 **/
static void
patchUnmovedBranches(schedule_info_t *sched, XanBitSet *blocksToMove, XanList *edgeList) {
    XanHashTable *skipMap;
    XanList *patchList;
    XanListItem *patchItem;
    schedule_t *schedule = sched->schedule;

    /* A map for blocks to skip to. */
    PDEBUG(1, "Patching unmoved branches\n");
    skipMap = buildBlockSkipMapping(sched, blocksToMove);
    if (!skipMap) {
        return;
    }

    /* A list of patches to apply. */
    patchList = buildBranchPatchList(sched, blocksToMove, skipMap);

    /* Patch branches. */
    PDEBUG(2, "  Patching branches\n");
    patchItem = xanList_first(patchList);
    while (patchItem) {
        branch_patch_t *patch = patchItem->data;
        assert(patch->branch);

        /* Patch the target.  We must create a new label here, if necessary, since
           we didn't ensure a label was the target when creating the patch. */
        if (patch->target) {
            ir_instruction_t *oldTarget = IRMETHOD_getBranchDestination(schedule->method, patch->branch);
            ir_instruction_t *label = getTargetLabel(schedule->method, patch->target);
            IRMETHOD_setBranchDestination(schedule->method, patch->branch, label->param_1.value.v);
            PDEBUG(2, "  Set target of branch %u to be inst %u (val %llu)\n", patch->branch->ID, label->ID, label->param_1.value.v);
            patchEdgesAfterBranchPatch(edgeList, patch->branch, oldTarget, NULL, label);
        }

        /* Patch the fall-through. */
        if (patch->fallThrough) {

            /* Really this should simply fall through, but we can't ensure this
               now, so add a new label and branch to that. Hopefully dead code
               elimination will remove this for us later. */
            ir_instruction_t *oldFallThrough = IRMETHOD_getBranchFallThrough(schedule->method, patch->branch);
            ir_instruction_t *label = getTargetLabel(schedule->method, patch->fallThrough);
            ir_instruction_t *br = IRMETHOD_newBranchToLabelAfter(schedule->method, label, patch->branch);
            PDEBUG(2, "  Set fall-through of branch %u as new branch %u target inst %u (val %llu)\n", patch->branch->ID, br->ID, label->ID, label->param_1.value.v);
            patchEdgesAfterBranchPatch(edgeList, patch->branch, oldFallThrough, br, label);
        }

        /* Next branch. */
        patchItem = patchItem->next;
    }

    /* Clean up. */
    patchItem = xanList_first(patchList);
    while (patchItem) {
        freeFunction(patchItem->data);
        patchItem = patchItem->next;
    }
    freeList(patchList);
    freeHashTable(skipMap);
}


/**
 * Free a successor relation map.
 **/
static void
freeSuccRelationMap(XanHashTable *succMap) {
    XanHashTableItem *tableItem = xanHashTable_first(succMap);
    while (tableItem) {
        succ_relation_t *succRelation = tableItem->element;
        freeFunction(succRelation);
        tableItem = xanHashTable_next(succMap, tableItem);
    }
    freeHashTable(succMap);
}


/**
 * Free an edge clone map.
 **/
static void
freeEdgeCloneMap(XanHashTable *edgeCloneMap) {
    XanHashTableItem *tableItem = xanHashTable_first(edgeCloneMap);
    while (tableItem) {
        edge_t *edge = tableItem->elementID;
        XanList *instList = tableItem->element;
        freeFunction(edge);
        freeList(instList);
        tableItem = xanHashTable_next(edgeCloneMap, tableItem);
    }
    freeHashTable(edgeCloneMap);
}


/**
 * Determine whether an instruction is within a basic block.
 **/
static JITBOOLEAN
basicBlockContainsInst(IRBasicBlock *block, ir_instruction_t *inst) {
    return inst->ID >= block->startInst && inst->ID <= block->endInst;
}


/**
 * Find the latest instruction in a basic block that has a dependence with
 * 'inst'.  If 'inst' is in 'block' it find the latest instruction that occurs
 * before 'inst'.
 *
 * Returns the ID of the latest instruction with a dependence, or -1 if none
 * exists.
 **/
static JITINT32
findLatestInstructionWithDependenceInBlock(IRBasicBlock *block, ir_instruction_t *inst, XanBitSet **dataDepGraph) {
    JITINT32 i, endInst = block->endInst;
    JITINT32 startInst = block->startInst;
    if (basicBlockContainsInst(block, inst)) {
        endInst = inst->ID - 1;
    }

    /* Use local startInst to make comparison signed. */
    for (i = endInst; i >= startInst; --i) {
        if (hasDataDependence(dataDepGraph, i, inst->ID)) {
            return i;
        }
    }
    return -1;
}


/**
 * Find the earliest instruction in a basic block that has a dependence with
 * 'inst'.  If 'inst' is in 'block' it find the earliest instruction that
 * occurs after 'inst'.
 *
 * Returns the ID of the earliest instruction with a dependence, or -1 if none
 * exists.
 **/
static JITINT32
findEarliestInstructionWithDependenceInBlock(IRBasicBlock *block, ir_instruction_t *inst, XanBitSet **dataDepGraph) {
    JITINT32 i, endInst = block->endInst;
    JITINT32 startInst = block->startInst;
    if (basicBlockContainsInst(block, inst)) {
        startInst = inst->ID + 1;
    }

    /* Use local endInst to make comparison signed. */
    for (i = startInst; i <= endInst; ++i) {
        if (hasDataDependence(dataDepGraph, inst->ID, i)) {
            return i;
        }
    }
    return -1;
}


/**
 * Get a bit set of loop header instructions from a list of loop structures.
 **/
static XanBitSet *
getLoopHeaders(JITUINT32 numInsts, XanList *loops) {
    XanBitSet *loopHeaders = xanBitSet_new(numInsts);
    XanListItem *loopItem = xanList_first(loops);
    /* PDEBUG(2, "Getting loop headers\n"); */
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        xanBitSet_setBit(loopHeaders, loop->headerID);
        /* PDEBUG(2, "  Loop at %u\n", loop->headerID); */
        loopItem = loopItem->next;
    }
    return loopHeaders;
}


/**
 * Get a bit set of loop exit instructions from a list of loop structures.  The
 * issue here is that a header instruction can be the header for multiple loops
 * and if one loop is subsumed by another, an exit from the inner loop may not
 * be an exit from the outer loop.  Therefore, we make sure that all exit
 * instructions do have a non-loop successor before marking them as true exits.
 * We should probably rename this function, or deal with header-of-two-loops
 * problems later on, at the point the assumption of single loops fails.
 **/
static XanBitSet *
getLoopExits(ir_method_t *method, JITUINT32 numInsts, XanList *loops, XanHashTable *loopInstMap, XanList **successors) {
    XanBitSet *loopExits = xanBitSet_new(numInsts);
    XanListItem *loopItem = xanList_first(loops);
    /* PDEBUG(2, "Getting loop exits\n"); */
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        ir_instruction_t *header = IRMETHOD_getInstructionAtPosition(method, loop->headerID);
        XanBitSet *loopInsts = xanHashTable_lookup(loopInstMap, header);
        JITINT32 id = -1;
        /* PDEBUG(2, "  Exits for header %u:\n", loop->headerID); */
        while ((id = xanBitSet_getFirstBitSetInRange(loop->exits, id + 1, numInsts)) != -1) {
            XanListItem *succItem = xanList_first(successors[id]);
            while (succItem) {
                ir_instruction_t *succ = succItem->data;
                if (!xanBitSet_isBitSet(loopInsts, succ->ID)) {
                    xanBitSet_setBit(loopExits, id);
                    /* PDEBUG(2, "    Inst %d\n", id); */
                    break;
                }
                succItem = succItem->next;
            }
        }
        loopItem = loopItem->next;
    }
    return loopExits;
}


/**
 * Build a mapping between loop instructions and bit sets of all instructions
 * within their loops.  As a side-effect, also populate a reverse map that
 * maps instructions to their nearest header.
 **/
static XanHashTable *
buildLoopInstructionMap(ir_method_t *method, JITUINT32 numInsts, XanList *loops, XanHashTable *headerMap) {
    XanHashTable *instMap = newHashTable();
    XanHashTable *headers = newHashTable();
    XanListItem *loopItem = xanList_first(loops);

    /* Create the loop header sets, unions of all loops instructions are headers for. */
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        ir_instruction_t *header = IRMETHOD_getInstructionAtPosition(method, loop->headerID);
        XanBitSet *loopInsts = xanHashTable_lookup(instMap, header);
        if (!loopInsts) {
            loopInsts = xanBitSet_clone(loop->loop);
            xanHashTable_insert(instMap, header, loopInsts);
            xanHashTable_insert(headerMap, header, header);
        } else {
            xanBitSet_union(loopInsts, loop->loop);
        }
        xanHashTable_insert(headers, header, header);
        loopItem = loopItem->next;
    }

    /** All other instructions in the loops select the smallest loop they're
     * part of to avoid selecting outer loops.  Ignore any loop headers. */
    loopItem = xanList_first(loops);
    while (loopItem) {
        JITUINT32 i;
        schedule_loop_t *loop = loopItem->data;
        for (i = 0; i < numInsts; ++i) {
            if (xanBitSet_isBitSet(loop->loop, i)) {
                ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
                if (!xanHashTable_lookup(headers, inst)) {
                    JITINT32 j = -1;
                    ir_instruction_t *header = IRMETHOD_getInstructionAtPosition(method, loop->headerID);
                    XanBitSet *loopInsts = xanHashTable_lookup(instMap, inst);
                    if (!loopInsts) {
                        loopInsts = xanBitSet_clone(loop->loop);
                        xanHashTable_insert(instMap, inst, loopInsts);
                    } else if (xanBitSet_getCountOfBitsSet(loopInsts) > xanBitSet_getCountOfBitsSet(loop->loop)) {
                        xanBitSet_copy(loopInsts, loop->loop);
                    }
                    while ((j = xanBitSet_getFirstBitSetInRange(loopInsts, j + 1, numInsts)) != -1) {
                        ir_instruction_t *contained = IRMETHOD_getInstructionAtPosition(method, j);
                        XanHashTableItem *mapItem = xanHashTable_lookupItem(headerMap, contained);
                        if (!mapItem) {
                            xanHashTable_insert(headerMap, contained, header);
                        } else {
                            mapItem->element = header;
                        }
                    }
                }
            }
        }
        loopItem = loopItem->next;
    }
    freeHashTable(headers);
    return instMap;
}


/**
 * Free a hash table that maps to a bit set.
 **/
static void
freeMapToBitSet(XanHashTable *map) {
    XanHashTableItem *mapItem = xanHashTable_first(map);
    while (mapItem) {
        xanBitSet_free(mapItem->element);
        mapItem = xanHashTable_next(map, mapItem);
    }
    freeHashTable(map);
}


/**
 * Create a edge between two instructions.  However, if they are a branch and
 * label, then create the edge after the label, if the label has no other
 * predecessors.
 **/
static edge_t *
createEdgeAfterLabelIfPossible(ir_instruction_t *pred, ir_instruction_t *succ, XanList **predecessors, XanList **successors) {
    ir_instruction_t *predToUse = pred;
    ir_instruction_t *succToUse = succ;
    if (succ->type == IRLABEL && xanList_length(predecessors[succ->ID]) == 1) {
        assert(xanList_first(predecessors[succ->ID])->data == pred);
        assert(xanList_length(successors[succ->ID]) == 1);
        predToUse = succ;
        succToUse = xanList_first(successors[succ->ID])->data;
    }
    return createEdge(predToUse, succToUse);
}


/**
 * Find the highest position that an instruction can move without moving any
 * other instruction too.  Therefore, this is the point that is immediately
 * after the first data dependence along any path in the CFG.  Where the CFG
 * splits, all successors must be able to have the instruction pass through
 * them in order to propagate up.
 *
 * Returns a list of edges where the instruction should be placed, or cloned.
 **/
static XanList *
findEdgesToPlaceMaxMoveInstructionUpwards(schedule_info_t *sched) {
    JITINT32 b, pos;
    JITBOOLEAN changes;
    XanBitSet *loopContainsDep;
    XanBitSet *blockContainsDep;
    XanBitSet *instReachesBlock;
    XanBitSet *loopExits;
    XanHashTable *loopInstructionMap;
    XanHashTable *loopHeaderMap;
    XanList *edgeList;
    XanListItem *loopItem;
    IRBasicBlock *initialBlock;
    schedule_t *schedule = sched->schedule;

    /* Check whether to move outside this basic block. */
    initialBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, sched->maxMoveInst);
    pos = findLatestInstructionWithDependenceInBlock(initialBlock, sched->maxMoveInst, schedule->dataDepGraph);
    if (pos != -1 && pos < sched->maxMoveInst->ID) {
        ir_instruction_t *pred = IRMETHOD_getInstructionAtPosition(schedule->method, pos);
        ir_instruction_t *succ = IRMETHOD_getInstructionAtPosition(schedule->method, pos + 1);
        edge_t *edge = createEdge(pred, succ);
        PDEBUG(1, "Inst %u in same block has dependence\n", pos);
        edgeList = newList();
        xanList_append(edgeList, edge);
        return edgeList;
    }

    /* Get loop information. */
    loopHeaderMap = newHashTable();
    loopInstructionMap = buildLoopInstructionMap(schedule->method, schedule->numInsts, schedule->loops, loopHeaderMap);
    loopExits = getLoopExits(schedule->method, schedule->numInsts, schedule->loops, loopInstructionMap, schedule->successors);

    /* Calculate the initial basic block sets. */
    PDEBUG(1, "Finding edges to place instruction\n");
    blockContainsDep = xanBitSet_new(schedule->numBlocks);
    instReachesBlock = xanBitSet_new(schedule->numBlocks);
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(schedule->regionBlocks, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            if (basicBlockContainsInst(block, sched->maxMoveInst)) {
                xanBitSet_setBit(instReachesBlock, b);
                PDEBUG(3, "  Reaches block (%u-%u)\n", block->startInst, block->endInst);
            }
            if (findLatestInstructionWithDependenceInBlock(block, sched->maxMoveInst, schedule->dataDepGraph) != -1) {
                xanBitSet_setBit(blockContainsDep, b);
                PDEBUG(3, "  Dependence in block (%u-%u)\n", block->startInst, block->endInst);
            }
        }
    }

    /* Determine whether there is a dependence in any loop. */
    loopContainsDep = xanBitSet_new(schedule->numInsts);
    loopItem = xanList_first(schedule->loops);
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        ir_instruction_t *header = IRMETHOD_getInstructionAtPosition(schedule->method, loop->headerID);
        XanBitSet *loopInsts = xanHashTable_lookup(loopInstructionMap, header);
        JITBOOLEAN foundDep = JITFALSE;
        for (b = 0; b < schedule->numBlocks && !foundDep; ++b) {
            if (xanBitSet_isBitSet(schedule->regionBlocks, b) && xanBitSet_isBitSet(blockContainsDep, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                if (xanBitSet_isBitSet(loopInsts, block->startInst)) {
                    xanBitSet_setBit(loopContainsDep, header->ID);
                    foundDep = JITTRUE;
                    PDEBUG(4, "  Loop header %u has dependence\n", header->ID);
                }
            }
        }
        loopItem = loopItem->next;
    }

    /* Propagate the information. */
    changes = JITTRUE;
    while (changes) {
        changes = JITFALSE;
        PDEBUG(5, "  Propagating information\n");
        for (b = schedule->numBlocks - 1; b >= 0; --b) {
            if (xanBitSet_isBitSet(schedule->regionBlocks, b) && !xanBitSet_isBitSet(instReachesBlock, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->startInst);
                XanBitSet *loopInsts = xanHashTable_lookup(loopInstructionMap, startInst);

                /* Treat loops the instruction isn't a part of separately. */
                if (loopInsts && !xanBitSet_isBitSet(loopInsts, sched->maxMoveInst->ID)) {
                    ir_instruction_t *header = xanHashTable_lookup(loopHeaderMap, startInst);
                    PDEBUG(7, "    Looking at successors for loop block (%u-%u), header %u\n", block->startInst, block->endInst, header->ID);

                    /* Can't move into the loop if there's a dependence inside. */
                    if (!xanBitSet_isBitSet(loopContainsDep, header->ID)) {
                        JITBOOLEAN reachesAllLoopSuccs = JITTRUE;
                        JITBOOLEAN depInLoopSucc = JITFALSE;
                        JITINT32 exitID = -1;

                        /* The instruction must reach all loop successors. */
                        PDEBUG(7, "      Loop exits are:");
                        PDEBUGSTMT(7, printInstBitSet(7, schedule->numInsts, loopExits));
                        while ((exitID = xanBitSet_getFirstBitSetInRange(loopExits, exitID + 1, schedule->numInsts)) != -1 && !depInLoopSucc && reachesAllLoopSuccs) {
                            if (xanBitSet_isBitSet(loopInsts, exitID)) {
                                XanListItem *succItem = xanList_first(schedule->successors[exitID]);
                                while (succItem && !depInLoopSucc && reachesAllLoopSuccs) {
                                    ir_instruction_t *succ = succItem->data;
                                    IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                                    if (!xanBitSet_isBitSet(loopInsts, succBlock->startInst)) {
                                        if (!xanBitSet_isBitSet(schedule->regionBlocks, succBlock->pos) || !xanBitSet_isBitSet(instReachesBlock, succBlock->pos)) {
                                            reachesAllLoopSuccs = JITFALSE;
                                            PDEBUG(7, "      Doesn't reach loop successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                                        } else {
                                            PDEBUG(7, "      Reaches loop successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                                        }
                                        if (xanBitSet_isBitSet(blockContainsDep, succBlock->pos)) {
                                            depInLoopSucc = JITTRUE;
                                            PDEBUG(7, "      Dependence in loop successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                                        }
                                    }
                                    succItem = succItem->next;
                                }
                            }
                        }

                        /* Mark all loop blocks as being reached, if so. */
                        if (!depInLoopSucc && reachesAllLoopSuccs) {
                            JITINT32 loopInstID = -1;
                            while ((loopInstID = xanBitSet_getFirstBitSetInRange(loopInsts, loopInstID + 1, schedule->numInsts)) != -1) {
                                IRBasicBlock *loopBlock = IRMETHOD_getBasicBlockContainingInstructionPosition(schedule->method, loopInstID);
                                PDEBUG(6, "      Now reaches loop block (%u-%u)\n", loopBlock->startInst, loopBlock->endInst);
                                xanBitSet_setBit(instReachesBlock, loopBlock->pos);
                                changes = JITTRUE;
                                loopInstID = loopBlock->endInst;
                            }
                        }
                    } else {
                        PDEBUG(7, "    Loop with header %u contains dependence\n", header->ID);
                    }
                }

                /* Non-loops. */
                else {
                    XanListItem *succItem = xanList_first(schedule->successors[block->endInst]);
                    JITBOOLEAN depInSuccs = JITFALSE;
                    JITBOOLEAN reachesAllSuccs = JITTRUE;
                    PDEBUG(7, "    Looking at successors for block (%u-%u)\n", block->startInst, block->endInst);
                    while (succItem && !depInSuccs && reachesAllSuccs) {
                        ir_instruction_t *succ = succItem->data;
                        IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);

                        /* All successors must be in the region. */
                        if (xanBitSet_isBitSet(schedule->regionBlocks, succBlock->pos)) {
                            if (xanBitSet_isBitSet(blockContainsDep, succBlock->pos)) {
                                depInSuccs = JITTRUE;
                                PDEBUG(7, "      Dependence in successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                            }
                            if (!xanBitSet_isBitSet(instReachesBlock, succBlock->pos)) {
                                reachesAllSuccs = JITFALSE;
                                PDEBUG(7, "      Doesn't reach successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                            }
                        } else {
                            reachesAllSuccs = JITFALSE;
                            PDEBUG(7, "      Doesn't reach non-region successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                        }
                        succItem = succItem->next;
                    }

                    /* Mark this block as being reached, if so. */
                    if (!depInSuccs && reachesAllSuccs) {
                        PDEBUG(6, "      Now reaches block (%u-%u)\n", block->startInst, block->endInst);
                        xanBitSet_setBit(instReachesBlock, b);
                        changes = JITTRUE;
                    }
                }
            }
        }
    }

    /* Debug printing. */
    for (b = schedule->numBlocks - 1; b >= 0; --b) {
        if (!xanBitSet_isBitSet(instReachesBlock, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            XanListItem *succItem = xanList_first(schedule->successors[block->endInst]);
            PDEBUG(7, "  Doesn't reach block (%u-%u)\n", block->startInst, block->endInst);
            while (succItem) {
                ir_instruction_t *succ = succItem->data;
                IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                if (xanBitSet_isBitSet(blockContainsDep, succBlock->pos)) {
                    PDEBUG(7, "    Due to dep in successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                } else if (!xanBitSet_isBitSet(instReachesBlock, succBlock->pos)) {
                    PDEBUG(7, "    Due to not reaching successor (%u-%u)\n", succBlock->startInst, succBlock->endInst);
                }
                succItem = succItem->next;
            }
        }
    }

    /* Create edges where clones must be placed. */
    edgeList = newList();
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(schedule->regionBlocks, b)) {
            if (xanBitSet_isBitSet(instReachesBlock, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                PDEBUG(3, "  Looking for edges in block (%u-%u)\n", block->startInst, block->endInst);
                if (xanBitSet_isBitSet(blockContainsDep, b)) {
                    JITINT32 depID = findLatestInstructionWithDependenceInBlock(block, sched->maxMoveInst, schedule->dataDepGraph);
                    ir_instruction_t *pred;
                    XanListItem *succItem;
                    assert(depID != -1);
                    pred = IRMETHOD_getInstructionAtPosition(schedule->method, depID);
                    /* PDEBUG(3, "    Dependence in block, considering edge predecessor %u\n", pred->ID); */
                    succItem = xanList_first(schedule->successors[pred->ID]);
                    while (succItem) {
                        ir_instruction_t *succ = succItem->data;
                        if (xanBitSet_isBitSet(schedule->regionInsts, succ->ID)) {
                            edge_t *edge = createEdgeAfterLabelIfPossible(pred, succ, schedule->predecessors, schedule->successors);
                            if (edge->succ != sched->maxMoveInst) {
                                xanList_append(edgeList, edge);
                                PDEBUG(2, "  New edge (%u-%u)\n", edge->pred->ID, edge->succ->ID);
                            } else {
                                freeFunction(edge);
                            }
                        }
                        succItem = succItem->next;
                    }
                } else {
                    ir_instruction_t *succ = IRMETHOD_getInstructionAtPosition(schedule->method, block->startInst);
                    XanListItem *predItem = xanList_first(schedule->predecessors[succ->ID]);
                    /* PDEBUG(3, "    No dependence in block, considering predecessors\n"); */
                    while (predItem) {
                        ir_instruction_t *pred = predItem->data;
                        IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);
                        /* PDEBUG(3, "      Predecessor %u, block (%u-%u)\n", pred->ID, predBlock->startInst, predBlock->endInst); */
                        if (!xanBitSet_isBitSet(schedule->regionInsts, pred->ID) || !xanBitSet_isBitSet(instReachesBlock, predBlock->pos)) {
                            edge_t *edge = createEdgeAfterLabelIfPossible(pred, succ, schedule->predecessors, schedule->successors);
                            /* PDEBUG(3, "      Is %sin region, does %sreach this block\n", xanBitSet_isBitSet(sched->regionInsts, pred->ID) ? "" : "not ", xanBitSet_isBitSet(instReachesBlock, predBlock->pos) ? "" : "not "); */
                            if (edge->succ != sched->maxMoveInst) {
                                xanList_append(edgeList, edge);
                                PDEBUG(2, "  New edge (%u-%u)\n", edge->pred->ID, edge->succ->ID);
                            } else {
                                freeFunction(edge);
                            }
                        }
                        predItem = predItem->next;
                    }
                }
            }
        }
    }

    /* Clean up. */
    xanBitSet_free(instReachesBlock);
    xanBitSet_free(blockContainsDep);
    xanBitSet_free(loopContainsDep);
    xanBitSet_free(loopExits);
    freeHashTable(loopHeaderMap);
    freeMapToBitSet(loopInstructionMap);
    return edgeList;
}


/**
 * Find the lowest position that an instruction can move without moving any
 * other instruction too.  Therefore, this is the point that is immediately
 * before the first data dependence along any path in the CFG.  Where the CFG
 * splits, all successors must be able to have the instruction pass through
 * them in order to propagate down.
 *
 * Returns a list of edges where the instruction should be placed, or cloned.
 **/
static XanList *
findEdgesToPlaceMaxMoveInstructionDownwards(schedule_info_t *sched) {
    JITINT32 b, pos;
    JITBOOLEAN changes;
    XanBitSet *loopContainsDep;
    XanBitSet *blockContainsDep;
    XanBitSet *instReachesBlock;
    XanBitSet *loopExits;
    XanHashTable *loopInstructionMap;
    XanHashTable *loopHeaderMap;
    XanList *edgeList;
    XanListItem *loopItem;
    IRBasicBlock *initialBlock;
    schedule_t *schedule = sched->schedule;

    /* Check whether to move outside this basic block. */
    PDEBUG(1, "Finding edges to place instruction\n");
    initialBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, sched->maxMoveInst);
    pos = findEarliestInstructionWithDependenceInBlock(initialBlock, sched->maxMoveInst, schedule->dataDepGraph);
    if (pos != -1 && pos > sched->maxMoveInst->ID) {
        ir_instruction_t *pred = IRMETHOD_getInstructionAtPosition(schedule->method, pos - 1);
        ir_instruction_t *succ = IRMETHOD_getInstructionAtPosition(schedule->method, pos);
        edge_t *edge = createEdge(pred, succ);
        edgeList = newList();
        xanList_append(edgeList, edge);
        return edgeList;
    }

    /* Get loop information. */
    loopHeaderMap = newHashTable();
    loopInstructionMap = buildLoopInstructionMap(schedule->method, schedule->numInsts, schedule->loops, loopHeaderMap);
    loopExits = getLoopExits(schedule->method, schedule->numInsts, schedule->loops, loopInstructionMap, schedule->successors);

    /* Calculate the initial basic block sets. */
    blockContainsDep = xanBitSet_new(schedule->numBlocks);
    instReachesBlock = xanBitSet_new(schedule->numBlocks);
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(schedule->regionBlocks, b)) {
            JITINT32 depID;
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            if (basicBlockContainsInst(block, sched->maxMoveInst)) {
                xanBitSet_setBit(instReachesBlock, b);
                PDEBUG(3, "  Reaches block (%u-%u)\n", block->startInst, block->endInst);
            }
            if ((depID = findEarliestInstructionWithDependenceInBlock(block, sched->maxMoveInst, schedule->dataDepGraph)) != -1) {
                xanBitSet_setBit(blockContainsDep, b);
                PDEBUG(3, "  Dependence at inst %d in block (%u-%u)\n", depID, block->startInst, block->endInst);
            }
        }
    }

    /* Determine whether there is a dependence in any loop. */
    loopContainsDep = xanBitSet_new(schedule->numInsts);
    loopItem = xanList_first(schedule->loops);
    while (loopItem) {
        schedule_loop_t *loop = loopItem->data;
        ir_instruction_t *header = IRMETHOD_getInstructionAtPosition(schedule->method, loop->headerID);
        XanBitSet *loopInsts = xanHashTable_lookup(loopInstructionMap, header);
        JITBOOLEAN foundDep = JITFALSE;
        for (b = 0; b < schedule->numBlocks && !foundDep; ++b) {
            if (xanBitSet_isBitSet(schedule->regionBlocks, b) && xanBitSet_isBitSet(blockContainsDep, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                if (xanBitSet_isBitSet(loopInsts, block->startInst)) {
                    xanBitSet_setBit(loopContainsDep, header->ID);
                    foundDep = JITTRUE;
                    PDEBUG(4, "  Loop header %u has dependence\n", header->ID);
                }
            }
        }
        loopItem = loopItem->next;
    }

    /* Propagate the information. */
    changes = JITTRUE;
    while (changes) {
        changes = JITFALSE;
        PDEBUG(5, "  Propagating information\n");
        for (b = 0; b < schedule->numBlocks; ++b) {
            if (xanBitSet_isBitSet(schedule->regionBlocks, b) && !xanBitSet_isBitSet(instReachesBlock, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                ir_instruction_t *startInst = IRMETHOD_getInstructionAtPosition(schedule->method, block->startInst);
                XanBitSet *loopInsts = xanHashTable_lookup(loopInstructionMap, startInst);

                /* Treat loops the instruction isn't a part of separately. */
                if (loopInsts && !xanBitSet_isBitSet(loopInsts, sched->maxMoveInst->ID)) {
                    ir_instruction_t *header = xanHashTable_lookup(loopHeaderMap, startInst);
                    PDEBUG(7, "    Looking at predecessors for loop block (%u-%u), header %u\n", block->startInst, block->endInst, header->ID);

                    /* Can't move into the loop if there's a dependence inside. */
                    if (!xanBitSet_isBitSet(loopContainsDep, header->ID)) {
                        JITBOOLEAN reachesAllLoopPreds = JITTRUE;
                        JITBOOLEAN depInLoopPred = JITFALSE;
                        XanListItem *predItem = xanList_first(schedule->predecessors[header->ID]);

                        /* The instruction must reach all loop predecessors. */
                        while (predItem && !depInLoopPred && reachesAllLoopPreds) {
                            ir_instruction_t *pred = predItem->data;
                            IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);
                            if (!xanBitSet_isBitSet(schedule->regionBlocks, predBlock->pos) || !xanBitSet_isBitSet(instReachesBlock, predBlock->pos)) {
                                reachesAllLoopPreds = JITFALSE;
                                PDEBUG(7, "      Doesn't reach loop predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                            } else {
                                PDEBUG(7, "      Reaches loop predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                            }
                            if (xanBitSet_isBitSet(blockContainsDep, predBlock->pos)) {
                                depInLoopPred = JITTRUE;
                                PDEBUG(7, "      Dependence in loop predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                            }
                            predItem = predItem->next;
                        }

                        /* Mark all loop blocks as being reached, if so. */
                        if (!depInLoopPred && reachesAllLoopPreds) {
                            JITINT32 loopInstID = -1;
                            while ((loopInstID = xanBitSet_getFirstBitSetInRange(loopInsts, loopInstID + 1, schedule->numInsts)) != -1) {
                                IRBasicBlock *loopBlock = IRMETHOD_getBasicBlockContainingInstructionPosition(schedule->method, loopInstID);
                                PDEBUG(6, "      Now reaches loop block (%u-%u)\n", loopBlock->startInst, loopBlock->endInst);
                                xanBitSet_setBit(instReachesBlock, loopBlock->pos);
                                changes = JITTRUE;
                                loopInstID = loopBlock->endInst;
                            }
                        }
                    } else {
                        PDEBUG(7, "    Loop with header %u contains dependence\n", header->ID);
                    }
                }

                /* Non-loops. */
                else {
                    XanListItem *predItem = xanList_first(schedule->predecessors[block->startInst]);
                    JITBOOLEAN depInPreds = JITFALSE;
                    JITBOOLEAN reachesAllPreds = JITTRUE;
                    PDEBUG(7, "    Looking at predecessors for block (%u-%u)\n", block->startInst, block->endInst);
                    while (predItem && !depInPreds && reachesAllPreds) {
                        ir_instruction_t *pred = predItem->data;
                        IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);

                        /* All predecessors must be in the region. */
                        if (xanBitSet_isBitSet(schedule->regionBlocks, predBlock->pos)) {
                            if (xanBitSet_isBitSet(blockContainsDep, predBlock->pos)) {
                                depInPreds = JITTRUE;
                                PDEBUG(7, "      Dependence in predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                            }
                            if (!xanBitSet_isBitSet(instReachesBlock, predBlock->pos)) {
                                reachesAllPreds = JITFALSE;
                                PDEBUG(7, "      Doesn't reach predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                            }
                        } else {
                            reachesAllPreds = JITFALSE;
                            PDEBUG(7, "      Doesn't reach non-region predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                        }
                        predItem = predItem->next;
                    }

                    /* Mark this block as being reached, if so. */
                    if (!depInPreds && reachesAllPreds) {
                        PDEBUG(6, "      Now reaches block (%u-%u)\n", block->startInst, block->endInst);
                        xanBitSet_setBit(instReachesBlock, b);
                        changes = JITTRUE;
                    }
                }
            }
        }
    }

    /* Debug printing. */
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (!xanBitSet_isBitSet(instReachesBlock, b)) {
            IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
            XanListItem *predItem = xanList_first(schedule->predecessors[block->endInst]);
            PDEBUG(7, "  Doesn't reach block (%u-%u)\n", block->startInst, block->endInst);
            while (predItem) {
                ir_instruction_t *pred = predItem->data;
                IRBasicBlock *predBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, pred);
                if (xanBitSet_isBitSet(blockContainsDep, predBlock->pos)) {
                    PDEBUG(7, "    Due to dep in predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                } else if (!xanBitSet_isBitSet(instReachesBlock, predBlock->pos)) {
                    PDEBUG(7, "    Due to not reaching predecessor (%u-%u)\n", predBlock->startInst, predBlock->endInst);
                }
                predItem = predItem->next;
            }
        }
    }

    /* Create edges where clones must be placed. */
    edgeList = newList();
    for (b = 0; b < schedule->numBlocks; ++b) {
        if (xanBitSet_isBitSet(schedule->regionBlocks, b)) {
            if (xanBitSet_isBitSet(instReachesBlock, b)) {
                IRBasicBlock *block = IRMETHOD_getBasicBlockAtPosition(schedule->method, b);
                PDEBUG(3, "  Looking for edges in block (%u-%u)\n", block->startInst, block->endInst);
                if (xanBitSet_isBitSet(blockContainsDep, b)) {
                    JITINT32 depID = findEarliestInstructionWithDependenceInBlock(block, sched->maxMoveInst, schedule->dataDepGraph);
                    ir_instruction_t *succ;
                    XanListItem *predItem;
                    assert(depID != -1);
                    succ = IRMETHOD_getInstructionAtPosition(schedule->method, depID);
                    /* PDEBUG(3, "    Dependence in block, considering edge predecessor %u\n", pred->ID); */
                    predItem = xanList_first(schedule->predecessors[succ->ID]);
                    while (predItem) {
                        ir_instruction_t *pred = predItem->data;
                        if (xanBitSet_isBitSet(schedule->regionInsts, pred->ID)) {
                            edge_t *edge = createEdgeAfterLabelIfPossible(pred, succ, schedule->predecessors, schedule->successors);
                            if (edge->succ != sched->maxMoveInst) {
                                xanList_append(edgeList, edge);
                                PDEBUG(2, "  New edge (%u-%u)\n", edge->pred->ID, edge->succ->ID);
                            } else {
                                freeFunction(edge);
                            }
                        }
                        predItem = predItem->next;
                    }
                } else {
                    ir_instruction_t *pred = IRMETHOD_getInstructionAtPosition(schedule->method, block->endInst);
                    XanListItem *succItem = xanList_first(schedule->successors[pred->ID]);
                    /* PDEBUG(3, "    No dependence in block, considering successors\n"); */
                    while (succItem) {
                        ir_instruction_t *succ = succItem->data;
                        if (succ->type != IREXITNODE) {
                            IRBasicBlock *succBlock = IRMETHOD_getBasicBlockContainingInstruction(schedule->method, succ);
                            /* PDEBUG(3, "      Successor %u, block (%u-%u)\n", succ->ID, succBlock->startInst, succBlock->endInst); */
                            if (!xanBitSet_isBitSet(schedule->regionInsts, succ->ID) || !xanBitSet_isBitSet(instReachesBlock, succBlock->pos)) {
                                edge_t *edge = createEdgeAfterLabelIfPossible(pred, succ, schedule->predecessors, schedule->successors);
                                /* PDEBUG(3, "      Is %sin region, does %sreach this block\n", xanBitSet_isBitSet(sched->regionInsts, succ->ID) ? "" : "not ", xanBitSet_isBitSet(instReachesBlock, succBlock->pos) ? "" : "not "); */
                                if (edge->succ != sched->maxMoveInst) {
                                    xanList_append(edgeList, edge);
                                    PDEBUG(2, "  New edge (%u-%u)\n", edge->pred->ID, edge->succ->ID);
                                } else {
                                    freeFunction(edge);
                                }
                            }
                        }
                        succItem = succItem->next;
                    }
                }
            }
        }
    }

    /* Clean up. */
    xanBitSet_free(instReachesBlock);
    xanBitSet_free(blockContainsDep);
    xanBitSet_free(loopContainsDep);
    xanBitSet_free(loopExits);
    freeHashTable(loopHeaderMap);
    freeMapToBitSet(loopInstructionMap);
    return edgeList;
}


/**
 * Move an instruction as high as possible in the CFG without moving or cloning
 * any other instruction.  Place clones of the instruction on CFG edges where
 * necessary to preserve semantics and record them in 'clones'.
 *
 * Returns JITTRUE if the instruction was moved, JITFALSE otherwise.
 **/
static JITBOOLEAN
moveInstructionAsMuchAsPossible(schedule_info_t *sched, XanHashTable *clones) {
    XanList *edgeList;
    XanListItem *edgeItem;
    schedule_t *schedule = sched->schedule;

    /* Early exit. */
    if (xanBitSet_getCountOfBitsSet(schedule->regionInsts) == 0 || xanBitSet_getCountOfBitsSet(schedule->regionBlocks) == 0) {
        PDEBUG(0, "No instructions or blocks in the region\n");
        return JITFALSE;
    }

    /* Really don't want to mess with these at the moment. */
    if (IRMETHOD_isABranchInstruction(sched->maxMoveInst)) {
        PDEBUG(0, "Not moving branch instructions\n");
        return JITFALSE;
    }

    /* Is it worth doing. */
    if (sched->direction == Upwards) {
        JITBOOLEAN depAllPreds = JITTRUE;
        XanListItem *predItem = xanList_first(schedule->predecessors[sched->maxMoveInst->ID]);
        while (predItem && depAllPreds) {
            ir_instruction_t *pred = predItem->data;
            if (!hasDataDependence(schedule->dataDepGraph, pred->ID, sched->maxMoveInst->ID)) {
                depAllPreds = JITFALSE;
            }
            predItem = predItem->next;
        }
        if (depAllPreds) {
            PDEBUG(0, "Dependence with all predecessors\n");
            return JITFALSE;
        }
    } else {
        JITBOOLEAN depAllSuccs = JITTRUE;
        XanListItem *succItem = xanList_first(schedule->successors[sched->maxMoveInst->ID]);
        assert(schedule->dataDepGraph[sched->maxMoveInst->ID]);
        while (succItem && depAllSuccs) {
            ir_instruction_t *succ = succItem->data;
            if (succ->type != IREXITNODE && !hasDataDependence(schedule->dataDepGraph, sched->maxMoveInst->ID, succ->ID)) {
                depAllSuccs = JITFALSE;
            }
            succItem = succItem->next;
        }
        if (depAllSuccs) {
            PDEBUG(0, "Dependence with all successors\n");
            return JITFALSE;
        }
    }

    /* Find the maximal points that the instruction can move to. */
    if (sched->direction == Upwards) {
        edgeList = findEdgesToPlaceMaxMoveInstructionUpwards(sched);
    } else {
        edgeList = findEdgesToPlaceMaxMoveInstructionDownwards(sched);
    }

    /* Clone the instruction. */
    if (xanList_length(edgeList) > 0) {
        edgeItem = xanList_first(edgeList);
        while (edgeItem) {
            schedule_edge_t *edge = edgeItem->data;
            ir_instruction_t *clone = cloneSingleInstOnEdge(sched, sched->maxMoveInst, edge);
            addInstToInstListMapping(clones, sched->maxMoveInst, clone);
            edgeItem = edgeItem->next;
        }

        /* Delete the orginal instruction. */
        IRMETHOD_deleteInstruction(schedule->method, sched->maxMoveInst);

        /* Clean up. */
        freeListAndData(edgeList);
        return JITTRUE;
    }
    freeList(edgeList);
    return JITFALSE;
}


/**
 * Print instructions cloned on each edge.
 **/
static void
printInstsClonedOnEachEdge(XanHashTable *edgeCloneMap)
{
    XanHashTableItem *tableItem = xanHashTable_first(edgeCloneMap);
    while (tableItem) {
        PDECL(edge_t *edge = tableItem->elementID);
        PDECL(XanList *instList = tableItem->element);
        PDEBUG(3, "Insts cloned on edge %u -> %u:", edge->pred->ID, edge->succ->ID);
        PDEBUGSTMT(3, printInstList(2, instList));
        tableItem = xanHashTable_next(edgeCloneMap, tableItem);
    }
}


/**
 * Print original instruction to clones mappings.
 **/
static void
printOrigInstToClonesMappings(XanHashTable *origIDMap, XanHashTable *clones)
{
    XanHashTableItem *tableItem;
    PDEBUG(3, "Original instruction to clone mapping:\n");
    tableItem = xanHashTable_first(origIDMap);
    while (tableItem) {
        PDECL(JITINT32 id = (JITINT32)(JITNINT)tableItem->element);
        ir_instruction_t *inst = tableItem->elementID;
        XanList *cloneList = xanHashTable_lookup(clones, inst);
        if (cloneList) {
            XanListItem *instItem;
            PDEBUG(3, "  Inst %d -> Clones", id);
            instItem = xanList_first(cloneList);
            while (instItem) {
                PDECL(ir_instruction_t *clone = instItem->data);
                PDEBUGC(3, " %u", clone->ID);
                instItem = instItem->next;
            }
            PDEBUGC(3, "\n");
        }
        tableItem = xanHashTable_next(origIDMap, tableItem);
    }
}


/**
 * Move instructions above or below another instruction in the CFG.  Place
 * clones on CFG edges where necessary to preserve semantics and record
 * each cloned instruction in 'clones'.  Do not place new instructions
 * between adjacent instructions in 'instsBlocked' in sched, and if an
 * instruction is moved from this list, move all adjacent instructions in
 * the list too.  Ensure all instructions in 'instsToMove' in sched are
 * moved, or none.  Move the minimum number of instructions possible.
 *
 * Returns JITTRUE if the instructions were moved, JITFALSE otherwise.
 **/
static JITBOOLEAN
moveInstructionsAroundOthers(schedule_info_t *sched, XanHashTable *clones) {
    char *env;
    JITINT32 i = -1;
    XanList *instsToDelete;
    XanList *edgeList;
    XanBitSet *instsToMove;
    XanBitSet *instsToClone;
    XanBitSet *blocksToCloneFrom;
    XanBitSet *blocksToMove;
    XanListItem *instItem;
    XanHashTable *succMap;
    XanHashTable *edgeCloneMap;
    XanHashTable *edgeEntryMap;
    XanHashTable *origIDMap;
    JITBOOLEAN weShouldContinue;
    schedule_t *schedule = sched->schedule;

    /* Early exit. */
    if (xanBitSet_isEmpty(schedule->regionInsts) || xanBitSet_isEmpty(schedule->regionBlocks)) {
        PDEBUG(0, "No instructions or blocks in the region\n");
        return JITFALSE;
    }

    /* Debugging info. */
    PDEBUG(1, "Region insts:");
    PDEBUGSTMT(1, printInstBitSet(1, schedule->numInsts, schedule->regionInsts));
    PDEBUG(1, "Region blocks:");
    PDEBUGSTMT(1, printBlockBitSet(1, schedule->method, schedule->numBlocks, schedule->regionBlocks));
    env = getenv("PARALLELIZER_DUMP_CFG");
    if (env && atoi(env) >= 2) {
        char *prevDotPrinterName = getenv("DOTPRINTER_FILENAME");
        setenv("DOTPRINTER_FILENAME", "Schedule_movement", 1);
        IROPTIMIZER_callMethodOptimization(irOptimizer, schedule->method, METHOD_PRINTER);
        setenv("DOTPRINTER_FILENAME", prevDotPrinterName, 1);
    }

    /* Bit sets for instructions to clone and blocks to clone from.  This is
       the minimum set of instructions to clone to preserve semantics.  Not
       all instructions will be cloned on all edges but all will be cloned
       before or after nextToInst.  The actual instructions to move (i.e.
       delete) are calculated later. */
    instsToClone = SCHEDULE_findInstsToCloneWhenMoving(schedule, sched->instsToMove, sched->nextToInsts, sched->direction == Upwards, NULL, NULL, NULL);

    /* Check whether these instructions can safely be moved. */
    if (!canMoveInstsEarly(sched, instsToClone)) {
        xanBitSet_free(instsToClone);
        return JITFALSE;
    }

    /* Identify instructions and blocks that can be totally moved. */
    instsToMove = xanBitSet_clone(instsToClone);
    blocksToMove = identifyInstsAndBlocksToMove(sched->schedule, instsToMove, sched->nextToInsts, sched->direction, sched->position);

    /* When identifying instructions and blocks to move, only labels, nops
       and branches were added to instsToMove, in addition to those marked in
       instsToClone.  However, some instructions may have been removed if
       they had a dependence with another instruction that was not moving and
       were going to pass that instruction.  These instructions will still be
       cloned and their clones could cause redefinitions of the unmoved
       instructions' source variables.  This must also be checked for, either
       here or in identifyInstsAndBlocksToMove. */

    /* Remove instructions from instsToClone if they pass a dependence.  If any
       instructions are removed then we must fail, since the instructions to
       clone is the minimum set of instructions to be cloned. */
    if (preventDependencesPassingUnmovedInsts(sched->schedule, instsToClone, sched->nextToInsts, sched->direction, sched->position)) {
        xanBitSet_free(instsToMove);
        xanBitSet_free(instsToClone);
        xanBitSet_free(blocksToMove);
        return JITFALSE;
    }

    /* Build a list of instructions that should be removed. */
    instsToDelete = buildInstsToDeleteList(sched, instsToMove, blocksToMove);

    /* Check where clones need to be put. */
    blocksToCloneFrom = getBlocksContainingInsts(sched->schedule, instsToClone);
    edgeCloneMap = buildEdgeCloneListMapping(sched, instsToClone, blocksToCloneFrom);

    /* Don't move but clone instructions on fake edges. */
    createFakeEdgesForCloningInsts(sched, instsToClone, edgeCloneMap);

    /* Final checks on instructions that are moved. */
    weShouldContinue = canMoveInstsFinally(sched, instsToClone, instsToMove, edgeCloneMap);
    if (!weShouldContinue) {
        freeEdgeCloneMap(edgeCloneMap);
        xanBitSet_free(instsToMove);
        xanBitSet_free(instsToClone);
        xanBitSet_free(blocksToMove);
        xanBitSet_free(blocksToCloneFrom);
        freeList(instsToDelete);
        return JITFALSE;
    }

    /* Printing the list of instructions per edge. */
    PDEBUGSTMT(3, printInstsClonedOnEachEdge(edgeCloneMap));

    /* Compute successor instructions. */
    succMap = buildInstSuccessorMapping(sched, instsToClone, blocksToCloneFrom);

    /* Build a map to the entry instructions for clones. */
    edgeEntryMap = createEdgeCloneEntryMap(sched, edgeCloneMap);

    /* Set up a map to original instruction IDs, for later debug printing. */
    origIDMap = newHashTable();
    while ((i = xanBitSet_getFirstBitSetInRange(instsToClone, i + 1, sched->schedule->numInsts)) != -1) {
        xanHashTable_insert(origIDMap, IRMETHOD_getInstructionAtPosition(sched->schedule->method, i), (void *)(JITNINT)i);
    }

    /**
     * After calling patchUnmovedBranches(), cached predecessors and successors
     * are not valid and must be recomputed, or fall rely on IRMETHOD calls.
     **/

    /* Patch up branches that aren't moved. */
    if (!sched->shouldFail) {
        edgeList = xanHashTable_toKeyList(edgeCloneMap);
        patchUnmovedBranches(sched, blocksToMove, edgeList);
        freeList(edgeList);
    }

    /* We may still fail here, without having done anything. */
    if (sched->shouldFail) {
        if (succMap) {
            freeSuccRelationMap(succMap);
        }
        freeEdgeCloneMap(edgeCloneMap);
        xanBitSet_free(instsToMove);
        xanBitSet_free(instsToClone);
        xanBitSet_free(blocksToMove);
        xanBitSet_free(blocksToCloneFrom);
        freeList(instsToDelete);
        freeHashTable(edgeEntryMap);
        freeHashTable(origIDMap);
        return JITFALSE;
    }

    /* Do the cloning. */
    cloneInstsOnEdges(sched, edgeCloneMap, edgeEntryMap, succMap, clones);

    /* Delete the instructions we're meant to move. */
    instItem = xanList_first(instsToDelete);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        IRMETHOD_deleteInstruction(sched->schedule->method, inst);
        instItem = instItem->next;
    }

    /* Print the old instruction to clones mappings. */
    PDEBUGSTMT(3, printOrigInstToClonesMappings(origIDMap, clones));

    /* Clean up. */
    freeSuccRelationMap(succMap);
    freeEdgeCloneMap(edgeCloneMap);
    xanBitSet_free(instsToMove);
    xanBitSet_free(instsToClone);
    xanBitSet_free(blocksToMove);
    xanBitSet_free(blocksToCloneFrom);
    freeList(instsToDelete);
    freeHashTable(edgeEntryMap);
    freeHashTable(origIDMap);

    /* Success. */
    return JITTRUE;
}


/**
 * Check the inputs to the instruction moving functions to ensure that
 * assumptions hold.
 *
 * Returns JITTRUE if assumptions hold, JITFALSE otherwise.
 **/
static JITBOOLEAN
checkMovingAssumptions(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *nextToInsts) {
    if (!xanBitSet_isIntersectionEmpty(instsToMove, nextToInsts)) {
        PDEBUG(0, "Fail: Overlap between instructions to move and those to move next to\n");
        return JITFALSE;
    }
    return JITTRUE;
}


JITBOOLEAN
SCHEDULE_moveInstructionsBefore(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *beforeInsts, XanHashTable *clones) {
    schedule_info_t *sched;
    JITBOOLEAN success;

    /* Ensure assumptions hold. */
    setDebugLevel();
    PDEBUGNL(0, "********** New Schedule **********\n");
    PDEBUG(0, "Scheduling for method %s\n", IRMETHOD_getMethodName(schedule->method));
    PDEBUG(0, "Moving:");
    PDEBUGSTMT(0, printInstBitSet(0, schedule->numInsts, instsToMove));
    PDEBUG(0, "  Before:");
    PDEBUGSTMT(0, printInstBitSet(0, schedule->numInsts, beforeInsts));
    if (!checkMovingAssumptions(schedule, instsToMove, beforeInsts)) {
        return JITFALSE;
    }

    /* Create an information structure. */
    sched = initialiseScheduleInfoForManyMove(schedule, instsToMove, beforeInsts, Upwards, Before);
    if (!sched) {
        return JITFALSE;
    }

    /* Move the instructions. */
    success = moveInstructionsAroundOthers(sched, clones);

    /* Clean up. */
    freeScheduleInfo(sched);
    PDEBUG(0, "Finished schedule\n");
    return success;
}


/**
 * Move the instruction 'instToMove' so that it occurs after another,
 * 'afterInst', along every path in the control flow graph, whilst
 * preserving the semantics of the method.  This means moving additional
 * instructions too, if required.  Instructions in the list 'instsBlocked'
 * will not be moved.  For each instruction that is moved, an entry is
 * created in the hash table 'clones' that maps the instruction to a list
 * of clones that were created (or an empty list if no clones were actually
 * required).  Side effects only occur on success.
 *
 * Returns: JITTRUE if the move succeeded, JITFALSE otherwise.
 *
 * Requires: Pre-dominator, post-dominator and basic blocks analysis to
 * have previously been performed.
 **/
JITBOOLEAN SCHEDULE_moveInstructionsAfter(schedule_t *schedule, XanBitSet *instsToMove, XanBitSet *afterInsts, XanHashTable *clones) {
    schedule_info_t *sched;
    JITBOOLEAN success;

    /* Ensure assumptions hold. */
    setDebugLevel();
    PDEBUGNL(0, "********** New Schedule **********\n");
    PDEBUG(0, "Scheduling for method %s\n", IRMETHOD_getMethodName(schedule->method));
    PDEBUG(0, "Moving:");
    PDEBUGSTMT(0, printInstBitSet(0, schedule->numInsts, instsToMove));
    PDEBUG(0, "  After:");
    PDEBUGSTMT(0, printInstBitSet(0, schedule->numInsts, afterInsts));
    if (!checkMovingAssumptions(schedule, instsToMove, afterInsts)) {
        return JITFALSE;
    }

    /* Create an information structure. */
    sched = initialiseScheduleInfoForManyMove(schedule, instsToMove, afterInsts, Downwards, After);
    if (!sched) {
        return JITFALSE;
    }

    /* Move the instructions. */
    success = moveInstructionsAroundOthers(sched, clones);

    /* Clean up. */
    freeScheduleInfo(sched);
    PDEBUG(0, "Finished schedule\n");
    return success;
}


/**
 * Move the instruction 'instToMove' as high up in the control flow graph as
 * possible without moving or cloning any other instructions.  The instruction
 * can be duplicated if necessary to preserve the semantics of the method.
 * The list 'clones' contains the duplicates of 'instToMove' that were created
 * by the movement.  Side effects only occur on success.
 *
 * Returns: JITTRUE if the move succeeded, JITFALSE otherwise.
 *
 * Requires: Pre-dominator, post-dominator and basic blocks analysis to
 * have previously been performed.
 **/
JITBOOLEAN
SCHEDULE_moveInstructionOnlyAsHighAsPossible(schedule_t *schedule, ir_instruction_t *instToMove, XanHashTable *clones) {
    schedule_info_t *sched;
    JITBOOLEAN success;

    /* Create an information structure. */
    setDebugLevel();
    PDEBUGNL(0, "********** New Schedule **********\n");
    PDEBUG(0, "Scheduling for method %s\n", IRMETHOD_getMethodName(schedule->method));
    PDEBUG(0, "Moving %u only as high as possible\n", instToMove->ID);
    sched = initialiseScheduleInfoForMaxMove(schedule, instToMove, Upwards);
    if (!sched) {
        return JITFALSE;
    }

    /* Move the instructions. */
    success = moveInstructionAsMuchAsPossible(sched, clones);

    /* Clean up. */
    freeScheduleInfo(sched);
    PDEBUG(0, "Finished schedule\n");
    return success;
}


/**
 * Move the instruction 'instToMove' as far down the control flow graph as
 * possible without moving or cloning any other instructions.  The instruction
 * can be duplicated if necessary to preserve the semantics of the method.
 * The list 'clones' contains the duplicates of 'instToMove' that were created
 * by the movement.  Side effects only occur on success.
 *
 * Returns: JITTRUE if the move succeeded, JITFALSE otherwise.
 *
 * Requires: Pre-dominator, post-dominator and basic blocks analysis to
 * have previously been performed.
 **/
JITBOOLEAN
SCHEDULE_moveInstructionOnlyAsLowAsPossible(schedule_t *schedule, ir_instruction_t *instToMove, XanHashTable *clones) {
    schedule_info_t *sched;
    JITBOOLEAN success;

    /* Create an information structure. */
    setDebugLevel();
    PDEBUGNL(0, "********** New Schedule **********\n");
    PDEBUG(0, "Scheduling for method %s\n", IRMETHOD_getMethodName(schedule->method));
    PDEBUG(0, "Moving %u only as low as possible\n", instToMove->ID);
    sched = initialiseScheduleInfoForMaxMove(schedule, instToMove, Downwards);
    if (!sched) {
        return JITFALSE;
    }

    /* Move the instructions. */
    success = moveInstructionAsMuchAsPossible(sched, clones);

    /* Clean up. */
    freeScheduleInfo(sched);
    PDEBUG(0, "Finished schedule\n");
    return success;
}

static inline ir_instruction_t * internal_fetchInstruction (schedule_t *schedule, JITUINT32 instID) {
    if (schedule->insts != NULL) {
        return schedule->insts[instID];
    }
    return IRMETHOD_getInstructionAtPosition(schedule->method, instID);
}

static inline XanHashTable * internal_fetchReachableInstructions (ir_method_t *method, XanHashTable **reachableInstructions, ir_instruction_t *inst) {
    XanHashTable    *reachableInstructionsAtInst;

    reachableInstructionsAtInst    = reachableInstructions[inst->ID];
    if (reachableInstructionsAtInst == NULL) {
        reachableInstructionsAtInst        = IRMETHOD_getInstructionsThatCanBeReachedFromPosition(method, inst);
        reachableInstructions[inst->ID]    = reachableInstructionsAtInst;
    }
    assert(reachableInstructionsAtInst != NULL);

    return reachableInstructionsAtInst;
}
