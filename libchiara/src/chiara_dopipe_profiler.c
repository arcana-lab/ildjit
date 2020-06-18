/*
 * Copyright (C) 2010-2011  Timothy M. Jones and Simone Campanoni
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

/* My headers. */
#include <chiara.h>
#include <chiara_profile.h>
#include <chiara_system.h>
/* End. */

/**
 * My own debugging system to prevent unnecessary output.  This allows us to
 * enable printing only on certain loops.
 */
#ifdef PRINTDEBUG
static JITBOOLEAN enablePDebug = JITTRUE;
#undef PDEBUG
#define PDEBUG(fmt, args...) if (enablePDebug) fprintf(stderr, "CHIARA_DOPIPE: " fmt, ## args)
#endif


/**
 * Whether to print the loops to dot file or not.
 */
#ifdef PRINTDEBUG
#define PRINTLOOPS
#endif


/**
 * Find the minimum of two values.
 */
#define MIN(a, b) ((a) > (b) ? (b) : (a))



/**
 * The instruction type with the largest identifier.
 */
#define INST_TYPE_MAX IRPHI


/**
 * Information required for computing Tarjan's stronly connected
 * components algorithm.
 */
typedef struct {
    int index;
    int lowlink;
} sccCreate_t;


/**
 * A program dependence graph structure.  Basically, this is a bitset of
 * dependent instructions for each instruction within the loop that the
 * graph refers to.  Also included is a strongly connected components
 * structure for use when running Tarjan's algorithm.
 */
typedef struct {
    JITUINT32 numInsts;
    XanBitSet **dependents;
    sccCreate_t **sccCreate;
    XanList *sccEntries;
} pdg_t;


/**
 * Runtime statistics for each thread that executes part of a loop.  This
 * is essentially an array of counters per region, where each counter
 * represents one instruction type, giving the number of each type of
 * instruction executed in each region.
 */
typedef struct {
    JITUINT32 numRegions;
    JITUINT64 **execCount;
} runtime_t;


/**
 * A structure containing information about each loop.  This is used to
 * hold the program dependence graph whilst the DoPipe splitting
 * algorithm is being run, then the counters for instruction execution
 * counts once the profiling phase is underway.  It also holds a pointer
 * to the loop, the original ID of the header instruction and a list of
 * the original successors to the loop.
 */
typedef struct {
    JITUINT32 origHeaderId;
    loop_t *loop;
    pdg_t *pdg;
    runtime_t *runtime;
    XanList *succList;
} loopInfo_t;


/**
 * A structure that contains a method and a list of instructions from that
 * method, which may or may not be all instructions in it.
 */
typedef struct {
    ir_method_t *method;
    XanList *instList;
} methodInstList_t;


/**
 * A global hash table to hold each loop information structure.  This gets
 * initialised at the beginning of the analysis and freed at the end, once
 * the data has all been collected and used.
 */
static XanHashTable *loopInfoTable;


/**
 * A global hash table of methods that have been profiled.  This prevents
 * methods that are called from two separate loops from being profiled
 * twice.
 */
static XanHashTable *profiledMethodsTable;


/**
 * A global pointer to the current loop runtime structure, to be used when
 * running the application.
 */
static runtime_t *loopRuntime;


/**
 * A global integer representing the region number that an executing loop
 * is currently in, to be used when running the application.
 */
static JITUINT32 loopRegionNum;


/**
 * A global indication of whether we're in a loop or not.  Since profile
 * calls are added to methods that are outside each loop method, these calls
 * can be taken before and after loop iterations.  Therefore, this flag
 * prevents unnecessary work from being done when we're not actually in a
 * loop that we want to get data for.
 */
static JITBOOLEAN LOOPS_inLoop;


/**
 * A global iteration number for the current loop.  This gets incremented when
 * the loop header instruction executes and reset on each loop exit.
 */
static JITUINT32 LOOPS_iterNum;


/**
 * This is the runtime function that is called when the loop header instruction
 * executes to increment the global iteration number.
 */
static void
execLoopHeader(ir_instruction_t *inst) {
    if (LOOPS_iterNum == 0) {
        PDEBUG("Starting loop at instruction (%u)\n", inst->ID);
        if (LOOPS_inLoop) {
            print_err("CHIARA_PROFILE: Error: Loop start but already in one\n", 0);
            abort();
        }
        LOOPS_inLoop = JITTRUE;
    }
    ++LOOPS_iterNum;
    PDEBUG("\nStarting iteration %u\n", LOOPS_iterNum);
}


/**
 * This is the runtime function that is called after a loop has finished
 * to indicate that execution is no longer in a loop and to reset the
 * iteration number.
 */
static void
execLoopFinish(ir_instruction_t *inst) {
    /* Debug information. */
    if (LOOPS_inLoop) {
        PDEBUG("Finishing loop at instruction (%u) on iteration %u\n", inst->ID, LOOPS_iterNum);
    }

    /* Reset loop variables. */
    LOOPS_iterNum = 0;
    LOOPS_inLoop = JITFALSE;
}


/**
 * Initialise global structures and variables.
 */
static void
initialiseAnalysis(void) {
    LOOPS_iterNum = 0;
    loopRuntime = NULL;
    loopRegionNum = 0;
    loopInfoTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    profiledMethodsTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
}


/**
 * Free a runtime structure.
 */
static void
freeRuntime(runtime_t *runtime) {
    JITUINT32 r;

    /* Free the execution counts. */
    for (r = 0; r < runtime->numRegions; ++r) {
        freeFunction(runtime->execCount[r]);
    }

    /* Free structures. */
    freeFunction(runtime->execCount);
    freeFunction(runtime);
}


/**
 * Print the execution counts for the given loop's regions.
 */
static void
printRegionExecCount(loopInfo_t *info) {
    JITUINT32 r;
    FILE *output;
    char filename[DIM_BUF];

    /* Open the file. */
    SNPRINTF(filename, DIM_BUF, "%s_dopipe_execution_counts_%s_thread_loop_%u", IRPROGRAM_getProgramName(), IRMETHOD_getSignatureInString(info->loop->method), info->origHeaderId);
    output = fopen(filename, "w+");
    if (!output) {
        abort();
    }

    /* Print each region's execution counts. */
    for (r = 0; r < info->runtime->numRegions; ++r) {
        JITUINT32 c;
        fprintf(output, "[Region %u]\n", r);
        for (c = 0; c < INST_TYPE_MAX; ++c) {
            if (info->runtime->execCount[r][c] > 0) {
                fprintf(output, "%s: %llu\n", IRMETHOD_getInstructionTypeName(c), info->runtime->execCount[r][c]);
            }
        }
        fprintf(output, "\n");
    }

    /* Finished with the file. */
    fclose(output);
}


/**
 * Free global structures and variables.
 */
static void
finaliseAnalysis(void) {
    XanHashTableItem *item;

    /* Print then free each element in the loop information table. */
    while ((item = xanHashTable_first(loopInfoTable))) {
        loopInfo_t *info = item->element;

        /* Print each region's instruction type execution count. */
        printRegionExecCount(info);

        /* Free the info structure. */
        freeRuntime(info->runtime);
        freeFunction(info);
        xanHashTable_removeElement(loopInfoTable, item->elementID);
    }

    /* Free the whole table. */
    xanHashTable_destroyTable(loopInfoTable);

    /* Free each element in the profiled methods table. */
    while ((item = xanHashTable_first(profiledMethodsTable))) {
        XanList *instList = item->element;
        xanList_destroyList(instList);
        xanHashTable_removeElement(profiledMethodsTable, item->elementID);
    }

    /* Free the whole table. */
    xanHashTable_destroyTable(profiledMethodsTable);
}


/**
 * Create and initialise a new program dependence graph structure.
 */
static pdg_t *
createProgramDependenceGraph(JITUINT32 numInsts) {
    pdg_t *pdg;
    JITUINT32 i;

    /* Allocate memory. */
    pdg = allocFunction(sizeof(pdg_t));
    assert(pdg);

    /* Initialise the bitset and scc arrays. */
    pdg->numInsts = numInsts;
    pdg->dependents = allocFunction(numInsts * sizeof(XanBitSet *));
    pdg->sccCreate = allocFunction(numInsts * sizeof(sccCreate_t *));
    assert(pdg->dependents && pdg->sccCreate);

    /* Initialise each bitset. */
    for (i = 0; i < numInsts; ++i) {
        pdg->dependents[i] = xanBitSet_new(numInsts);
        assert(pdg->dependents[i]);
    }

    /* Return the new graph. */
    return pdg;
}


/**
 * Free a program dependence graph structure.
 */
static void
freeProgramDependenceGraph(pdg_t *pdg) {
    JITUINT32 i;
    XanListItem *currScc;

    /* Free for each instruction. */
    for (i = 0; i < pdg->numInsts; ++i) {
        xanBitSet_free(pdg->dependents[i]);
        freeFunction(pdg->sccCreate[i]);
    }

    /* Free the list of scc entries. */
    currScc = xanList_first(pdg->sccEntries);
    while (currScc) {
        XanList *scc = currScc->data;
        assert(scc);
        xanList_destroyList(scc);
        currScc = currScc->next;
    }

    /* Free main structures. */
    xanList_destroyList(pdg->sccEntries);
    freeFunction(pdg->dependents);
    freeFunction(pdg->sccCreate);
    freeFunction(pdg);
}


/**
 * Create and initialise a new runtime structure with the given regions
 * as the partitions of the loop.
 */
static runtime_t *
createRuntime(XanList *regions) {
    runtime_t *runtime;
    JITUINT32 r;

    /* Allocate memory. */
    runtime = allocFunction(sizeof(runtime_t));
    assert(runtime);

    /* Record the number of regions. */
    runtime->numRegions = xanList_length(regions);

    /* Initialise the instruction execution count arrays. */
    runtime->execCount = allocFunction(runtime->numRegions * sizeof(JITUINT64 *));
    assert(runtime->execCount);
    for (r = 0; r < runtime->numRegions; ++r) {
        runtime->execCount[r] = allocFunction(INST_TYPE_MAX * sizeof(JITUINT64));
        assert(runtime->execCount[r]);
    }

    /* Return the new runtime. */
    return runtime;
}


/**
 * Create and initialise a new loop information structure.
 */
static loopInfo_t *
createLoopInfo(loop_t *loop) {
    loopInfo_t *info = allocFunction(sizeof(loopInfo_t));
    assert(info && loop);
    info->origHeaderId = loop->header->ID;
    info->loop = loop;
    info->pdg = createProgramDependenceGraph(IRMETHOD_getInstructionsNumber(loop->method));
    info->succList = LOOPS_getSuccessorsOfLoop(loop);
    assert(info->pdg && info->succList);
    return info;
}


/**
 * Print out the program dependence graph for debugging.
 */
#ifdef PRINTDEBUG
static void
printProgramDependenceGraph(pdg_t *pdg) {
    JITUINT32 i;

    /* Iterate over all instructions. */
    PDEBUG("Dependence graph:\n");
    for (i = 0; i < pdg->numInsts; ++i) {
        JITBOOLEAN printed = JITFALSE;
        JITUINT32 d;

        /* Iterate over all dependents. */
        for (d = 0; d < pdg->numInsts; ++d) {

            /* Check for dependence. */
            if (xanBitSet_isBitSet(pdg->dependents[i], d)) {
                if (!printed) {
                    PDEBUG("Inst %u\n", i);
                    printed = JITTRUE;
                }
                PDEBUG("  -> Inst %u\n", d);
            }
        }
    }
}
#else
#define printProgramDependenceGraph(pdg)
#endif  /* ifdef PRINTDEBUG */


/**
 * Build the program dependence graph for the given loop using the given
 * list of data dependences.  Creates a bitset of successor and predecessor
 * instructions for each instruction within the loop considering the data
 * and control dependences between them.
 */
static void
buildProgramDependenceGraph(loop_t *loop, XanList *ddg, pdg_t *pdg) {
    XanListItem *item;
    PDEBUG("Building program dependence graph\n");

    /* Add control dependences. */
    item = xanList_first(loop->instructions);
    while (item) {
        ir_instruction_t *inst;
        ir_instruction_t *other;

        /* Get the current instruction. */
        inst = item->data;
        assert(inst);

        /* Control dependents are successor instructions, but not to the header. */
        other = NULL;
        while ((other = IRMETHOD_getSuccessorInstruction(loop->method, inst, other))) {
            if (other != loop->header && xanList_find(loop->instructions, other)) {
                xanBitSet_setBit(pdg->dependents[inst->ID], other->ID);
                /* PDEBUG("Control dependence from inst %u to inst %u\n", inst->ID, other->ID); */
            }
        }

        /* Next instruction. */
        item = item->next;
    }

    /* Add data dependences. */
    item = xanList_first(ddg);
    while (item) {
        loop_inter_interation_data_dependence_t *dd;

        /* Fetch the current dependence. */
        dd = item->data;
        assert(dd && dd->inst1 && dd->inst2);

        /* Ignore false register dependences. */
        if (dd->type != DEP_WAR && dd->type != DEP_WAW) {
            xanBitSet_setBit(pdg->dependents[dd->inst2->ID], dd->inst1->ID);
            /* PDEBUG("Data dependence type %#x from inst %u to inst %u\n", */
            /*        dd->type, dd->inst2->ID, dd->inst1->ID); */
        }

        /* Next data dependence. */
        item = item->next;
    }

    /* Print the dependence graph. */
    printProgramDependenceGraph(pdg);
}


/**
 * Recursive function to compute strongly connected components.  This uses
 * Tarjan's algorithm to traverse the graph in a depth-first manner,
 * checking for a strongly connected component once each subtree has been
 * visited.
 */
static void
createInitialSccs(JITUINT32 instId, pdg_t *pdg, XanStack *nodeStack, XanList *sccs, int *index) {
    JITUINT32 depId;

    /* Assign an index to this instruction. */
    assert(pdg && pdg->sccCreate);
    pdg->sccCreate[instId] = allocFunction(sizeof(sccCreate_t));
    assert(pdg->sccCreate[instId]);
    pdg->sccCreate[instId]->index = *index;
    pdg->sccCreate[instId]->lowlink = *index;
    *index = *index + 1;

    /* Save onto the stack and process dependents. */
    xanStack_push(nodeStack, (void *)(JITNUINT)instId);
    for (depId = 0; depId < pdg->numInsts; ++depId) {
        if (xanBitSet_isBitSet(pdg->dependents[instId], depId)) {

            /* Check whether this instruction has been seen. */
            if (!pdg->sccCreate[depId]) {
                createInitialSccs(depId, pdg, nodeStack, sccs, index);
                pdg->sccCreate[instId]->lowlink = MIN(pdg->sccCreate[instId]->lowlink, pdg->sccCreate[depId]->lowlink);
            } else if (xanStack_contains(nodeStack, (void *)(JITNUINT)depId)) {
                pdg->sccCreate[instId]->lowlink = MIN(pdg->sccCreate[instId]->lowlink, pdg->sccCreate[depId]->index);
            }
        }
    }

    /* Check for a stronly connected component. */
    if (pdg->sccCreate[instId]->lowlink == pdg->sccCreate[instId]->index) {
        XanList *currScc;
        JITUINT32 depId;
        PDEBUG("New SCC starting at inst %u\n", instId);

        /* Initialise the scc list. */
        currScc = xanList_new(allocFunction, freeFunction, NULL);

        /* Add the scc to a new list. */
        depId = JITMAXUINT32;
        while (depId != instId) {
            depId = (JITUINT32)(JITNUINT)xanStack_pop(nodeStack);
            xanList_insert(currScc, (void *)(JITNUINT)depId);
            PDEBUG("  Contains inst %u\n", depId);
        }

        /* Remember this scc at the start of the list. */
        xanList_insert(sccs, currScc);
    }
}


/**
 * Check a strongly connected component to see if it contains a cycle.
 * This function relies on properties of the implementation of finding
 * strongly connected components, which places items in the component
 * list in order.  Therefore, it's enough to check the last item in
 * the list to see if one of its successors is also there.
 */
static JITBOOLEAN
sccHasCycle(XanList *scc, pdg_t *pdg) {
    JITUINT32 instId;
    JITUINT32 depId;

    /* Check all successors of the last instruction. */
    instId = (JITUINT32)(JITNUINT)xanList_last(scc)->data;
    for (depId = 0; depId < pdg->numInsts; ++depId) {
        if (xanBitSet_isBitSet(pdg->dependents[instId], depId) &&
                xanList_find(scc, (void *)(JITNUINT)depId)) {
            return JITTRUE;
        }
    }

    /* No successor found in the scc. */
    return JITFALSE;
}


/**
 * Check for a dependence between two strongly connected components.  This
 * requires checking each instruction in the first to see if a dependent
 * instruction appears in the second.
 */
static JITBOOLEAN
isDependenceBetweenSccs(XanList *first, XanList *second, pdg_t *pdg) {
    XanListItem *currInst;

    /* Check all instructions in the first list. */
    currInst = xanList_first(first);
    while (currInst) {
        JITUINT32 instId;
        JITUINT32 depId;

        /* Check all dependents of this instruction. */
        instId = (JITUINT32)(JITNUINT)currInst->data;
        for (depId = 0; depId < pdg->numInsts; ++depId) {
            if (xanBitSet_isBitSet(pdg->dependents[instId], depId) &&
                    xanList_find(second, (void *)(JITNUINT)depId)) {
                return JITTRUE;
            }
        }

        /* Next instruction. */
        currInst = currInst->next;
    }

    /* No dependents found. */
    return JITFALSE;
}


/**
 * Merge strongly connected components from the tree parts of the initial
 * graph.  This is required because Tarjan's algorithm puts them in their
 * own individual components whereas it is better for the DoPipe algorithm
 * if they are merged.  Any components that cannot be merged are left
 * unchanged.  Furthermore, there must be an ordering to the components so
 * that each arc connecting them flows forwards, but this should be already
 * guaranteed by the implementation of Tarjan's algorithm.  The strongly
 * connected components are processed backwards but merge forwards so that
 * all dependents of the component are processed before the current one.
 */
static void
mergeTreeSccs(ir_method_t *method, XanList *sccs, pdg_t *pdg) {
    XanListItem *currScc;

    /* Find sccs to merge. */
    currScc = xanList_last(sccs);
    while (currScc) {
        XanList *mergeScc;

        /* Check to see if this is a candidate for merging. */
        mergeScc = currScc->data;
        assert(mergeScc);
        if (!sccHasCycle(mergeScc, pdg)) {
            XanListItem *checkItem;

            /* Check subsequent sccs for merge candidate. */
            checkItem = currScc->next;
            while (checkItem) {
                XanList *checkScc;
                XanListItem *nextCheckItem;

                /* Precompute the next scc to check. */
                nextCheckItem = checkItem->next;

                /* Ensure this can be merged in. */
                checkScc = checkItem->data;
                assert(checkScc);
                if (!sccHasCycle(checkScc, pdg)) {

                    /* Check for dependence between the sccs. */
                    if (isDependenceBetweenSccs(mergeScc, checkScc, pdg)) {

                        /* Merge. */
                        PDEBUG("Merging SCC starting inst %u into SCC starting inst %u\n",
                               (JITUINT32)xanList_first(checkScc)->data,
                               (JITUINT32)xanList_first(mergeScc)->data);
                        xanList_appendList(mergeScc, checkScc);
                        xanList_destroyList(checkScc);
                        xanList_deleteItem(sccs, checkItem);
                    }
                }

                /* Next scc to check. */
                checkItem = nextCheckItem;
            }
        }

        /* Next scc. */
        currScc = currScc->prev;
    }
}


/**
 * Find strongly connected components within the loop's program dependence
 * graph using Tarjan's recursive algorithm.  Each strongly connected
 * component in the returned list is a list of instruction IDs for the
 * instructions within that strongly connected component.
 */
static XanList *
findStronglyConnectedComponents(loop_t *loop, loopInfo_t *info) {
    int index;
    XanList *sccs;
    XanStack *nodeStack;

    /* Initialisation. */
    sccs = xanList_new(allocFunction, freeFunction, NULL);
    nodeStack = xanStack_new(allocFunction, freeFunction, NULL);

    /* Perform a depth-first traversal of the graph. */
    index = 0;
    createInitialSccs(loop->header->ID, info->pdg, nodeStack, sccs, &index);

    /* Now merge sccs from the tree parts of the graph. */
    mergeTreeSccs(loop->method, sccs, info->pdg);
    PDEBUG("There are %u SCCs\n", xanList_length(sccs));

    /* Clean up. */
    xanStack_destroyStack(nodeStack);

    /* Return the list of strongly connected components. */
    return sccs;
}


/**
 * Get the entry instructions for each strongly connected component in the
 * given list.  An entry instruction is defined as an instruction that has
 * at least one predecessor instruction outside the current component.
 */
static XanList *
getSccEntryInsts(loop_t *loop, XanList *sccs) {
    XanList *sccEntries;
    XanListItem *currScc;
#ifdef PRINTDEBUG
    JITUINT32 r = 0;
#endif

    /* The list of entry instruction lists. */
    sccEntries = xanList_new(allocFunction, freeFunction, NULL);
    assert(sccEntries);

    /* Find the entry instructions for each scc. */
    PDEBUG("Finding SCC entry instructions\n");
    currScc = xanList_first(sccs);
    while (currScc) {
        XanList *scc;
        XanList *entries;
        XanListItem *currInstId;

        /* The list of entry instructions. */
        entries = xanList_new(allocFunction, freeFunction, NULL);
        assert(entries);

        /* Find each instruction with a non-scc predecessor. */
        PDEBUG("  SCC %u\n", r);
        scc = currScc->data;
        assert(scc);
        currInstId = xanList_first(scc);
        while (currInstId) {
            JITUINT32 instId;
            ir_instruction_t *inst;
            ir_instruction_t *pred;
            JITBOOLEAN addedInst;

            /* Get the instruction. */
            instId = (JITUINT32)(JITNUINT)currInstId->data;
            inst = IRMETHOD_getInstructionAtPosition(loop->method, instId);

            /* Check for a non-scc predecessor. */
            pred = NULL;
            addedInst = JITFALSE;
            while (!addedInst && (pred = IRMETHOD_getPredecessorInstruction(loop->method, inst, pred))) {
                if (!xanList_find(scc, (void *)(JITNUINT)pred->ID)) {
                    xanList_append(entries, inst);
                    addedInst = JITTRUE;
                    PDEBUG("    Instruction %u has non-SCC predecessor %u\n", inst->ID, pred->ID);
                }
            }

            /* Next instruction. */
            currInstId = currInstId->next;
        }

        /* Must be at least one entry to each scc. */
        assert(xanList_length(entries) > 0);
        xanList_append(sccEntries, entries);

        /* Next scc. */
        currScc = currScc->next;
#ifdef PRINTDEBUG
        ++r;
#endif
    }

    /* Return the created list. */
    return sccEntries;
}


/**
 * Split the given program dependence graph for the given loop into regions
 * that would be executed by different threads if exploiting DoPipe
 * parallelism.  This function returns JITTRUE is at least one split has
 * been made (i.e. two or more threads) and JITFALSE if there is only one
 * strongly connected component.
 */
static void
splitProgramDependenceGraph(loop_t *loop, loopInfo_t *info) {
    XanList *sccs;
    XanListItem *currScc;
#ifndef NDEBUG
    XanBitSet *insts;
    XanListItem *currInst;
#endif  /* ifndef NDEBUG */

    /* Debugging output. */
    PDEBUG("Splitting the dependence graph\n");

    /* Find strongly-connected components. */
    sccs = findStronglyConnectedComponents(loop, info);
    assert(sccs && xanList_length(sccs) > 0);

    /* Make sure that each instruction belongs to a single scc. */
#ifndef NDEBUG
    insts = xanBitSet_new(info->pdg->numInsts);

    /* Add each instruction in each scc to the bitset. */
    currScc = xanList_first(sccs);
    while (currScc) {
        XanList *scc = currScc->data;
        currInst = xanList_first(scc);
        while (currInst) {
            JITUINT32 instId = (JITUINT32)(JITNUINT)currInst->data;
            assert(!xanBitSet_isBitSet(insts, instId));
            xanBitSet_setBit(insts, instId);
            currInst = currInst->next;
        }
        currScc = currScc->next;
    }

    /* Check that all loop instructions are included. */
    currInst = xanList_first(loop->instructions);
    while (currInst) {
        ir_instruction_t *inst = currInst->data;
        assert(inst && xanBitSet_isBitSet(insts, inst->ID));
        currInst = currInst->next;
    }

    /* Clean up. */
    xanBitSet_free(insts);
#endif  /* ifndef NDEBUG */

    /* Record the entry instructions for each scc. */
    info->pdg->sccEntries = getSccEntryInsts(loop, sccs);

    /* Create the runtime structure based on this split. */
    info->runtime = createRuntime(sccs);

    /* Clean up. */
    currScc = xanList_first(sccs);
    while (currScc) {
        XanList *scc = currScc->data;
        assert(scc);
        xanList_destroyList(scc);
        currScc = currScc->next;
    }
    xanList_destroyList(sccs);
}


/**
 * Native method that is called to set the pointer to the global loop
 * runtime structure whenever a loop is started.
 */
static void
setLoopRuntime(runtime_t *runtime) {
    loopRuntime = runtime;
}


/**
 * Native method that is called to set the current loop region number at
 * the entry to each region when the application is running.
 */
static void
setRegionNumber(JITUINT32 regionNum) {
    loopRegionNum = regionNum;
}


/**
 * Native method that is called to increment the execution count of the
 * given type of instruction in the given region.
 */
static void
incrementExecCount(JITUINT32 type) {
    if (LOOPS_inLoop) {
        loopRuntime->execCount[loopRegionNum][type]++;
    }
}


/**
 * Add profile calls before the given loop to set the loop runtime for that
 * loop.  To avoid calling this function every time we enter the loop, we
 * place the native call before the loop header and adjust all non-loop
 * predecessors to branch to a new label before the native call.
 */
static void
addLoopEntryProfileCall(loop_t *loop, loopInfo_t *info) {
    XanList *args;
    XanList *loopPreds;
    XanListItem *predItem;
    ir_item_t *param;
    ir_instruction_t *pred;
    ir_instruction_t *newInst;
    IR_ITEM_VALUE headerLabel;
    IR_ITEM_VALUE newLabel;

    /* Create a list of loop predecessor instructions. */
    loopPreds = xanList_new(allocFunction, freeFunction, NULL);
    assert(loopPreds);
    pred = NULL;
    while ((pred = IRMETHOD_getPredecessorInstruction(loop->method, loop->header, pred))) {
        if (!xanList_find(loop->instructions, pred)) {
            xanList_append(loopPreds, pred);
        }
    }

    /* Add a new label before the header for predecessors to branch to. */
    assert(loop->header->type == IRLABEL);
    headerLabel = loop->header->param_1.value.v;
    newLabel = IRMETHOD_newLabelID(loop->method);
    newInst = IRMETHOD_newInstructionBefore(loop->method, loop->header);
    IRMETHOD_setInstructionType(loop->method, newInst, IRLABEL);
    IRMETHOD_setInstructionParameter1(loop->method, newInst, newLabel, 0, IRLABELITEM, IRLABELITEM, NULL);

    /* Create an argument list and a parameter for it. */
    args = xanList_new(allocFunction, freeFunction, NULL);
    param = allocFunction(sizeof(ir_item_t));
    assert(args && param);

    /* Add a profile call before the loop's header instruction. */
    param->value.v = (IR_ITEM_VALUE)(JITNUINT)info->runtime;
    param->type = IRNUINT;
    param->internal_type = IRNUINT;
    xanList_append(args, param);
    IRMETHOD_newNativeCallInstructionBefore(loop->method, loop->header, "DoPipeProfileEnterLoop", setLoopRuntime, NULL, args);
    xanList_destroyListAndData(args);

    /* Update all loop predecessors to branch to new label. */
    predItem = xanList_first(loopPreds);
    while (predItem) {
        IRMETHOD_substituteLabel(loop->method, predItem->data, headerLabel, newLabel);
        predItem = predItem->next;
    }
}


/**
 * Add profile calls along each edge from one strongly connected component
 * to another to set the correct region number during execution.  This uses
 * the precomputed lists of component entry instructions to know where to
 * place the native calls.
 */
static void
addRegionChangeProfileCalls(loop_t *loop, loopInfo_t *info) {
    XanListItem *currEntries;
    XanList *args;
    ir_item_t *param;

    /* Create an argument list and a parameter for it. */
    args = xanList_new(allocFunction, freeFunction, NULL);
    param = allocFunction(sizeof(ir_item_t));
    assert(args && param);

    /* The parameter contains the region number. */
    param->value.v = 0;
    param->type = IRUINT32;
    param->internal_type = IRUINT32;
    xanList_append(args, param);

    /* Add calls to each scc. */
    currEntries = xanList_first(info->pdg->sccEntries);
    while (currEntries) {
        XanList *entries;
        XanListItem *currInst;

        /* Add a call before each entry instruction. */
        entries = currEntries->data;
        assert(entries);
        currInst = xanList_first(entries);
        while (currInst) {
            ir_instruction_t *inst;

            /* Insert after a label, before anything else. */
            inst = currInst->data;
            assert(inst);
            if (inst->type == IRLABEL) {
                IRMETHOD_newNativeCallInstructionAfter(loop->method, inst, "DoPipeProfileChangeRegion", setRegionNumber, NULL, args);
            } else {
                IRMETHOD_newNativeCallInstructionBefore(loop->method, inst, "DoPipeProfileChangeRegion", setRegionNumber, NULL, args);
            }

            /* Next instruction. */
            currInst = currInst->next;
        }

        /* Next scc. */
        ++param->value.v;
        currEntries = currEntries->next;
    }

    /* Clean up. */
    xanList_destroyListAndData(args);
}


/**
 * Add profile calls before the given loop to set the loop runtime for that
 * loop.  Furthermore, insert calls before each strongly connected component
 * to set the correct region number for that component.
 */
static void
addLoopProfileCalls(loop_t *loop, loopInfo_t *info) {
    /* Set the runtime on entry to the loop. */
    addLoopEntryProfileCall(loop, info);

    /* Change the region number at the start of each scc. */
    addRegionChangeProfileCalls(loop, info);
}


/**
 * Add profile calls before each instruction in the given list to count
 * the number of executions of each type of instruction in each loop region.
 * The native method call simply takes the type of instruction that the call
 * refers to.  All instruction types are profiled apart from labels, which
 * are ignored.
 */
static void
addExecCountProfileCallsToInsts(ir_method_t *method, XanList *instList) {
    XanList *args;
    XanListItem *currInst;
    ir_item_t *param;

    /* Create the argument list and a parameter for it. */
    args = xanList_new(allocFunction, freeFunction, NULL);
    param = allocFunction(sizeof(ir_item_t));
    assert(args && param);

    /* Set up the parameter to record the instruction type. */
    param->type = IRUINT32;
    param->internal_type = IRUINT32;
    xanList_append(args, param);

    /* Profile all instructions in the list. */
    currInst = xanList_first(instList);
    while (currInst) {
        ir_instruction_t *inst;

        /* Get the instruction. */
        inst = currInst->data;
        assert(inst);

        /* Ignore labels. */
        if (inst->type != IRLABEL) {

            /* Add the native method call. */
            param->value.v = inst->type;
            IRMETHOD_newNativeCallInstructionBefore(method, inst, "DoPipeProfileInstType", incrementExecCount, NULL, args);
        }

        /* Next instruction. */
        currInst = currInst->next;
    }

    /* Clean up. */
    freeFunction(param);
    xanList_destroyList(args);
}


/**
 * Get a list of methods that are called by instructions from the given
 * instruction list.
 */
static XanList *
getCalledMethodsFromInsts(XanList *instList) {
    XanList *methodList;
    XanListItem *instItem;

    /* Allocate memory. */
    methodList = xanList_new(allocFunction, freeFunction, NULL);

    /* Check for calls. */
    instItem = xanList_first(instList);
    while (instItem) {
        ir_instruction_t *inst;

        /* Check this instruction for a call. */
        inst = instItem->data;
        assert(inst);
        if (IRMETHOD_isACallInstruction(inst)) {
            XanList *calleeList;

            /* Add the list of called methods to the main list. */
            calleeList = IRMETHOD_getCallableIRMethods(inst);
            xanList_appendList(methodList, calleeList);

            /* Clean up. */
            xanList_destroyList(calleeList);
        }

        /* Next instruction. */
        instItem = instItem->next;
    }

    /* Return the list of called methods. */
    return methodList;
}


/**
 * Add lists of instructions from each method within the given method list
 * to the work list, provided that the method has not been seen already.
 * Actually what is added is a pointer to a methodInstList_t structure that
 * contains both the method and its list of instructions.
 */
static void
addMethodInstsToWorklist(XanList *methodList, XanList *worklist, XanHashTable *seenMethods) {
    XanListItem *currMethod;

    /* Check and add instructions from each method in the list. */
    currMethod = xanList_first(methodList);
    while (currMethod) {
        ir_method_t *method;

        /* Check this method hasn't been seen already. */
        method = currMethod->data;
        assert(method);
        if (!xanHashTable_lookup(seenMethods, method)) {
            methodInstList_t *mil;

            /* Add this method and its instructions. */
            mil = allocFunction(sizeof(methodInstList_t));
            assert(mil);
            mil->method = method;
            mil->instList = IRMETHOD_getInstructions(method);
            xanList_append(worklist, mil);
            xanHashTable_insert(seenMethods, method, mil->instList);
        }

        /* Next method. */
        currMethod = currMethod->next;
    }
}


/**
 * Get the program's call graphs that are routed at insructions with each
 * loop in the given list.  Each item in the returned list is a pointer
 * to a methodInstList_t and each method will appear only once within
 * the returned list.
 */
static XanList *
getLoopsCallGraphs(XanList *loops) {
    XanList *worklist;
    XanList *callGraph;
    XanListItem *loopItem;
    XanHashTable *seenMethodsTable;
#ifdef PRINTDEBUG
    XanListItem *currItem;
#endif

    /* Create the call graph list that is returned. */
    callGraph = xanList_new(allocFunction, freeFunction, NULL);
    assert(callGraph);

    /* Create a worklist and table of seen methods. */
    worklist = xanList_new(allocFunction, freeFunction, NULL);
    seenMethodsTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(worklist && seenMethodsTable);

    /* Create the call graph from each loop. */
    loopItem = xanList_first(loops);
    while (loopItem) {
        loop_t *loop;
        methodInstList_t *mil;

        /* Create the initial worklist item and get the loop. */
        mil = allocFunction(sizeof(methodInstList_t));
        loop = loopItem->data;
        assert(mil && loop);

        /* Add to the worklist (no method to avoid doing loop method). */
        mil->instList = loop->instructions;
        xanList_append(worklist, mil);

        /* Process the worklist items. */
        while (xanList_length(worklist) > 0) {
            XanListItem *workListItem;
            XanList *calleeList;

            /* Get the next method and instruction list. */
            workListItem = xanList_first(worklist);
            mil = workListItem->data;
            xanList_deleteItem(worklist, workListItem);
            assert(mil);

            /* Get a list of called methods from this list. */
            calleeList = getCalledMethodsFromInsts(mil->instList);

            /* Add instructions from each method in the list to the worklist. */
            addMethodInstsToWorklist(calleeList, worklist, seenMethodsTable);

            /* Add this work item to the call graph if there's a method. */
            if (mil->method) {
                xanList_append(callGraph, mil);
            } else {
                freeFunction(mil);
            }

            /* Clean up. */
            xanList_destroyList(calleeList);
        }

        /* Next loop. */
        loopItem = loopItem->next;
    }

    /* Debugging output. */
#ifdef PRINTDEBUG
    PDEBUG("Call graph contains:\n");
    currItem = xanList_first(callGraph);
    while (currItem) {
        methodInstList_t *mil;
        mil = currItem->data;
        assert(mil);
        PDEBUG("  %s\n", IRMETHOD_getMethodName(mil->method));
        currItem = currItem->next;
    }
#endif

    /* Clean up. */
    xanList_destroyList(worklist);
    xanHashTable_destroyTable(seenMethodsTable);

    /* Return the call graphs as a list. */
    return callGraph;
}


/**
 * Add profile calls before each instruction within each method in the
 * given list of methods to profile.  Also check each loop in the given
 * list of loops and add profile to the loop's method if not already
 * added.
 */
static void
addExecCountProfileCalls(XanList *loops, XanList *methodsToProfile) {
    XanListItem *currItem;
    XanList *methodsProfiled;

    /* A list of methods that have been profiled. */
    methodsProfiled = xanList_new(allocFunction, freeFunction, NULL);
    assert(methodsProfiled);

    /* Add profile to each method first. */
    currItem = xanList_first(methodsToProfile);
    while (currItem) {
        methodInstList_t *mil;

        /* Add profile to these instructions. */
        mil = currItem->data;
        assert(mil);
        addExecCountProfileCallsToInsts(mil->method, mil->instList);

        /* Remember profiling this method. */
        xanList_append(methodsProfiled, mil->method);

        /* Clean up. */
        xanList_destroyList(mil->instList);
        freeFunction(mil);

        /* Next method. */
        currItem = currItem->next;
    }

    /* Add profile to loop methods if not already seen. */
    currItem = xanList_first(loops);
    while (currItem) {
        loop_t *loop;

        /* Check if this loop's method has been profiled. */
        loop = currItem->data;
        assert(loop);
        if (!xanList_find(methodsProfiled, loop->method)) {
            addExecCountProfileCallsToInsts(loop->method, loop->instructions);
        }

        /* Next loop. */
        currItem = currItem->next;
    }
}


/**
 * Analyse each loop, computing its program dependence graph, then split it
 * into regions.  Add profile calls around the loop and regions to keep
 * track of instruction counts on a per-loop, per-region basis during the
 * application's run.
 */
static void
analyseAndProfileLoops(ir_optimizer_t *irOptimizer, XanList *loops, XanHashTable *ddgs) {
    XanListItem *loopItem;

    /* Create loop information structures for each loop first. */
    loopItem = xanList_first(loops);
    while (loopItem) {
        loop_t *loop;
        loopInfo_t *info;

        /* Get the loop. */
        loop = loopItem->data;
        assert(loop);

        /* Create a new loop information object. */
        info = createLoopInfo(loop);
        xanHashTable_insert(loopInfoTable, loop, info);

        /* Next loop. */
        loopItem = loopItem->next;
    }

    /* Analyse and profile each loop. */
    loopItem = xanList_first(loops);
    while (loopItem) {
        loop_t *loop;
        loopInfo_t *info;
        XanList *ddg;

        /* Get the loop, its DDG and loop information structure. */
        loop = loopItem->data;
        assert(loop);
        ddg = xanHashTable_lookup(ddgs, loop);
        info = xanHashTable_lookup(loopInfoTable, loop);
        assert(ddg && info);

        /* Build the program dependence graph. */
#ifdef PRINTLOOPS
        IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, METHOD_PRINTER);
#endif
        buildProgramDependenceGraph(loop, ddg, info->pdg);

        /* Split the graph for DoPipe parallelism. */
        splitProgramDependenceGraph(loop, info);

        /* Add loop profiling to set the global runtime and region numbers. */
        addLoopProfileCalls(loop, info);

        /* Profile the loop's header and exits. */
        INSTS_addBasicInstructionProfileCall(loop->method, loop->header, NULL, NULL, NULL);
        LOOPS_profileLoopExits(loop->method, loop, NULL, NULL);
#ifdef PRINTLOOPS
        IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, METHOD_PRINTER);
#endif

        /* No longer need the program dependence graph or successor list. */
        freeProgramDependenceGraph(info->pdg);
        xanList_destroyList(info->succList);
#ifdef PRINTLOOPS
        IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, METHOD_PRINTER);
#endif

        /* Next loop. */
        loopItem = loopItem->next;
    }
}


/**
 * Given a list of loops and a hash table of their data dependences,
 * calculate the program dependence graph for each loop, split it as would
 * happen if exploiting DoPipe parallelism, then insert profiling code for
 * each region to count the number of each type of instruction that is
 * executed.  This is then used offline to calculate the ideal speedup
 * achievable with DoPipe.
 */
void
LOOPS_profileDoPipeParallelism(ir_optimizer_t *irOptimizer, XanList *loops, XanHashTable *ddgs) {
    char *prevName;
    XanList *methodsToProfile;
    fprintf(stderr, "This profiler doesn't work at the moment.\n");
    abort();

    /* Debugging output. */
    PDEBUG("Profiling %u loops\n", xanList_length(loops));
    prevName = getenv("DOTPRINTER_FILENAME");
    if (prevName == NULL) {
        prevName = "";
    }
    setenv("DOTPRINTER_FILENAME", "doPipeProfiler", 1);

    /* Assertions. */
    assert(loops != NULL);
    assert(ddgs != NULL);

    /* Initialise global structures. */
    initialiseAnalysis();

    /* A list of methods to profile. */
    methodsToProfile = xanList_new(allocFunction, freeFunction, NULL);
    assert(methodsToProfile);

    /* Get the call graphs routed at instructions within the lists. */
    methodsToProfile = getLoopsCallGraphs(loops);

    /* Profile each loop's entry, exit and regions. */
    analyseAndProfileLoops(irOptimizer, loops, ddgs);

    /* Add instruction profiling to count instruction types. */
    addExecCountProfileCalls(loops, methodsToProfile);

    /* Clean up. */
    xanList_destroyList(methodsToProfile);

    /* Reset debugging output. */
    setenv("DOTPRINTER_FILENAME", prevName, 1);
    PDEBUG("Finished profiling loops\n");
    exit(1);
}


/**
 * Finish profiling DoPipe parallelism by printing the execution counts
 * of each type of instruction for each loop that was analysed.  Then
 * free all memory used by this analysis.
 */
void
LOOPS_profileDoPipeParallelismEnd(void) {
    finaliseAnalysis();
}
