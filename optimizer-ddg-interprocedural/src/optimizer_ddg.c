/*
 * Copyright (C) 2009 - 2012 Timothy M Jones, Simone Campanoni
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
/**
 * @file optimizer_ddg.c
 * @brief Plugin for Data Dependency Graph. It does not make use of information from the control flow graph
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <xanlib.h>
#include <compiler_memory_manager.h>
#include <chiara.h>

// My headers
#include <optimizer_ddg.h>
#include <memory_aliases.h>
#include <vllpa.h>
#include <config.h>
// End


#define INFORMATIONS    "This plugin builds the Data Dependency Graph without using information from the control flow graph."
#define AUTHOR          "Timothy M Jones, Simone Campanoni"

static inline JITUINT64 get_job_kind (void);
static inline JITUINT64 get_dependences (void);
static inline void ddg_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void ddg_shutdown (JITFLOAT32 totalTime);
static inline char * get_version (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline void ddg_do_job (ir_method_t *method);
static inline JITUINT64 ddg_get_invalidations (void);
static inline void remove_false_data_dependences_due_to_not_escaped_global_variables (void);
static inline void ddg_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void ddg_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline ir_method_t * internal_getEntryPoint (void);

/**
 * The global enabler for debugging output.
 */
#ifdef PRINTDEBUG
JITBOOLEAN enablePDebug = JITTRUE;
#endif


/**
 * Level for CFG dumping.
 */
JITINT32 ddgDumpCfg = 0;


/**
 * Global variables for this optimization.
 */
JITINT32 provideMemoryAllocators = 0;
ir_lib_t *irLib = NULL;
ir_optimizer_t *irOptimizer = NULL;
static char *outPrefix = NULL;


/**
 * A possible variable dependence.  Each bit set represents instructions
 * that could have the given dependence.
 **/
typedef struct {
    XanBitSet *rawDeps;
    XanBitSet *warDeps;
    XanBitSet *wawDeps;
} possible_dep_t;


/**
 * Possible variable dependences, as calculated by the memory alias
 * analyser.  The keys are methods which map to other hash tables.  The
 * keys to these are instructions that map to possible_dep_t structures.
 **/
static XanHashTable *possibleDeps = NULL;


/**
 * Add a new possible dependence for an instruction.  The 'fromInst'
 * corresponds to the 'depending' instruction in the function
 * IRMETHOD_addInstructionDataDependence.
 **/
void
DDG_addPossibleDependence(ir_method_t *method, ir_instruction_t *fromInst, ir_instruction_t *toInst, JITUINT32 depType) {
    XanHashTable *methodDeps;
    possible_dep_t *dep;
    assert(possibleDeps);
    PDEBUG("Possible dependence between %u and %u (%x)\n", fromInst->ID, toInst->ID, depType);
    methodDeps = xanHashTable_lookup(possibleDeps, method);
    if (!methodDeps) {
        methodDeps = xanHashTable_new(11, JITFALSE, allocFunction, reallocFunction, freeFunction, NULL, NULL);
        xanHashTable_insert(possibleDeps, method, methodDeps);
    }
    dep = xanHashTable_lookup(methodDeps, fromInst);
    if (!dep) {
        JITUINT32 numInsts = IRMETHOD_getInstructionsNumber(method) + 1;
        dep = allocFunction(sizeof(possible_dep_t));
        dep->rawDeps = xanBitSet_new(numInsts);
        dep->warDeps = xanBitSet_new(numInsts);
        dep->wawDeps = xanBitSet_new(numInsts);
        xanHashTable_insert(methodDeps, fromInst, dep);
    }
    if (depType & DEP_RAW) {
        xanBitSet_setBit(dep->rawDeps, toInst->ID);
    }
    if (depType & DEP_WAR) {
        xanBitSet_setBit(dep->warDeps, toInst->ID);
    }
    if (depType & DEP_WAW) {
        xanBitSet_setBit(dep->wawDeps, toInst->ID);
    }
}


/**
 * Compare two instructions by ID.
 */
int
compareInstructionIDs(const void *i1, const void *i2) {
    assert(i1 && i2);
    return (*(ir_instruction_t * const *)i1)->ID < (*(ir_instruction_t * const *)i2)->ID;
}


/**
 * Print a possible dependence set.
 **/
#if 0
static void
printPossibleDepSet(ir_method_t *method, ir_instruction_t *inst, XanBitSet *depSet, JITINT16 depType) {
    JITUINT32 i, numInsts;
    JITBOOLEAN printed = JITFALSE;
    assert(inst);
    assert(depSet);
    if (depType & DEP_RAW) {
        fprintf(stderr, "Possible RAW dependences with inst %u: ", inst->ID);
    } else if (depType & DEP_WAR) {
        fprintf(stderr, "Possible WAR dependences with inst %u: ", inst->ID);
    } else if (depType & DEP_WAW) {
        fprintf(stderr, "Possible WAW dependences with inst %u: ", inst->ID);
    }
    numInsts = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < numInsts; ++i) {
        if (xanBitSet_isBitSet(depSet, i)) {
            if (!printed) {
                fprintf(stderr, "%u", i);
                printed = JITTRUE;
            } else {
                fprintf(stderr, ", %u", i);
            }
        }
    }
    fprintf(stderr, "\n");
}
#endif  /* 0 */

/**
 * Print all possible dependences.
 **/
#if 0
static void
printPossibleDeps(ir_method_t *method) {
    JITUINT32 i, j, numInsts;
    XanHashTable *methodDeps;
    fprintf(stderr, "Possible deps for %s\n", IRMETHOD_getMethodName(method));
    assert(possibleDeps);
    methodDeps = possibleDeps->lookup(possibleDeps, method);
    if (!methodDeps) {
        return;
    }
    numInsts = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        possible_dep_t *dep = methodDeps->lookup(methodDeps, inst);
        if (dep) {
            JITBOOLEAN thisPrinted = JITFALSE;
            for (j = 0; j < numInsts; ++j) {
                JITBOOLEAN printed = JITFALSE;
                if (xanBitSet_isBitSet(dep->rawDeps, j)) {
                    if (!thisPrinted) {
                        fprintf(stderr, "Inst %u\n", i);
                        thisPrinted = JITTRUE;
                    }
                    fprintf(stderr, "  %u (RAW", j);
                    printed = JITTRUE;
                }
                if (xanBitSet_isBitSet(dep->warDeps, j)) {
                    if (!printed) {
                        if (!thisPrinted) {
                            fprintf(stderr, "Inst %u\n", i);
                            thisPrinted = JITTRUE;
                        }
                        fprintf(stderr, "  %u (WAR", j);
                        printed = JITTRUE;
                    } else {
                        fprintf(stderr, ", WAR");
                    }
                }
                if (xanBitSet_isBitSet(dep->wawDeps, j)) {
                    if (!printed) {
                        if (!thisPrinted) {
                            fprintf(stderr, "Inst %u\n", i);
                            thisPrinted = JITTRUE;
                        }
                        fprintf(stderr, "  %u (WAW", j);
                        printed = JITTRUE;
                    } else {
                        fprintf(stderr, ", WAW");
                    }
                }
                if (printed) {
                    fprintf(stderr, ")\n");
                }
            }
        }
    }
}
#endif  /* 0 */

/**
 * Initialise structures for possible dependences.
 **/
static void
initialisePossibleDeps(void) {
    assert(!possibleDeps);
    possibleDeps = xanHashTable_new(11, JITFALSE, allocFunction, reallocFunction, freeFunction, NULL, NULL);
    assert(possibleDeps);
}


/**
 * Free memory used by structures tracking possible dependences.
 **/
static void
freePossibleDeps(void) {
    XanHashTableItem *methodItem;
    assert(possibleDeps);
    methodItem = xanHashTable_first(possibleDeps);
    while (methodItem) {
        XanHashTable *methodDeps = methodItem->element;
        XanHashTableItem *depItem = xanHashTable_first(methodDeps);
        while (depItem) {
            possible_dep_t *dep = depItem->element;
            xanBitSet_free(dep->rawDeps);
            xanBitSet_free(dep->warDeps);
            xanBitSet_free(dep->wawDeps);
            freeFunction(dep);
            depItem = xanHashTable_next(methodDeps, depItem);
        }
        xanHashTable_destroyTable(methodDeps);
        methodItem = xanHashTable_next(possibleDeps, methodItem);
    }
    xanHashTable_destroyTable(possibleDeps);
    possibleDeps = NULL;
}


/**
 * Get a bit set of possible dependences for an instruction.
 **/
static XanBitSet *
getPossibleDeps(ir_method_t *method, ir_instruction_t *inst, JITINT16 depType) {
    XanHashTable *methodDeps;
    assert(possibleDeps);
    methodDeps = xanHashTable_lookup(possibleDeps, method);
    if (methodDeps) {
        possible_dep_t *dep = xanHashTable_lookup(methodDeps, inst);
        if (dep) {
            if (depType & DEP_RAW) {
                return dep->rawDeps;
            } else if (depType & DEP_WAR) {
                return dep->warDeps;
            } else if (depType & DEP_WAW) {
                return dep->wawDeps;
            }
        }
    }
    return NULL;
}


/**
 * Print the dependences within the method.
 */
void
printDependences (ir_method_t *method, FILE *file) {
    JITUINT32 i;

    /* Sanity checks. */
    assert(method);

    fprintf(file, "\nPrinting dependences for %s\n", IRMETHOD_getMethodName(method));

    /* Print each instruction with dependence information. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        XanHashTable *dependsOn;

        /* Get the instruction and dependences. */
        /* dependsOn = NULL; //TOFIX inst->metadata->data_dependencies.dependsFrom; */
        dependsOn = inst->metadata->data_dependencies.dependsFrom;

        /* Don't worry if it doesn't depend on anything. */
        if (dependsOn && xanHashTable_elementsInside(dependsOn) > 0) {
            ir_instruction_t **depInsts;
            JITUINT32 numDepInsts;
            JITUINT32 depNum;

            /* Sort the instructions that depend on this. */
            depInsts = (ir_instruction_t **)xanHashTable_toKeyArray(dependsOn);
            numDepInsts = xanHashTable_elementsInside(dependsOn);
            qsort(depInsts, numDepInsts, sizeof(void *), compareInstructionIDs);

            /* Iterate over dependences, printing them too. */
            for (depNum = 0; depNum < numDepInsts; ++depNum) {
                JITBOOLEAN first;
                JITUINT16 depType;
                data_dep_arc_list_t *dep;

                /* Get the dependency. */
                dep = (data_dep_arc_list_t *) xanHashTable_lookup(dependsOn, depInsts[depNum]);

                /* Print the dependency ID. */
                fprintf(file, "%d -> %d ", i, dep->inst->ID);

                /* Print all types of dependency for this instruction. */
                first = JITTRUE;
                for (depType = DEP_RAW; depType <= DEP_MWAW; depType <<= 1) {
                    if (dep->depType & depType) {

                        /* Pretty print the list. */
                        if (!first) {
                            fprintf(file, ", ");
                        } else {
                            first = JITFALSE;
                        }

                        /* Print the type. */
                        switch (depType) {
                            case DEP_RAW:
                                fprintf(file, "RAW");
                                break;
                            case DEP_WAR:
                                fprintf(file, "WAR");
                                break;
                            case DEP_WAW:
                                fprintf(file, "WAW");
                                break;
                            case DEP_MRAW:
                                fprintf(file, "MRAW");
                                break;
                            case DEP_MWAR:
                                fprintf(file, "MWAR");
                                break;
                            case DEP_MWAW:
                                fprintf(file, "MWAW");
                                break;
                        }
                    }
                }

                /* Print the outside instructions. */
                /* if (dep->outsideInstructions) { */
                /*   XanListItem *instItem = dep->outsideInstructions->first(dep->outsideInstructions); */
                /*   first = JITTRUE; */
                /*   while (instItem) { */
                /*     ir_instruction_t *outside = instItem->data; */
                /*     if (first) { */
                /*       fprintf(file, " ("); */
                /*       first = JITFALSE; */
                /*     } else { */
                /*       fprintf(file, ", "); */
                /*     } */
                /*     fprintf(file, "%u", outside->ID); */
                /*     instItem = instItem->next; */
                /*     if (instItem == NULL) { */
                /*       fprintf(file, ")"); */
                /*     } */
                /*   } */
                /* } */

                /* Finish this dependency and move onto the next. */
                fprintf(file, "\n");
            }

            /* Clean up. */
            freeFunction(depInsts);
        }
    }
}


/**
 * Add methods to a worklist of methods to process provided that they
 * haven't been seen already.
 */
static void
addCalledMethodsToWorklist(ir_method_t *method, XanList *methodWorklist, XanHashTable *seenMethods) {
    JITUINT32 i;

    /* Find call instructions. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        XanList *calleeList;
        XanListItem *calleeItem;

        /* Find calls. */
        switch (inst->type) {
            case IRCALL:
            case IRVCALL:
            case IRICALL:

                /* Find the callees. */
                calleeList = IRMETHOD_getCallableMethods(inst);
                calleeItem = xanList_first(calleeList);
                while (calleeItem) {
                    IR_ITEM_VALUE calleeMethodId = (IR_ITEM_VALUE)(JITNUINT)calleeItem->data;
                    ir_method_t *callee = IRMETHOD_getIRMethodFromMethodID(method, calleeMethodId);
                    assert(callee);

                    /* Only add the callee if it's not a library method. */
                    IRMETHOD_translateMethodToIR(callee);
                    if (!IRPROGRAM_doesMethodBelongToALibrary(callee)) {
                        if (!xanHashTable_lookup(seenMethods, callee)) {
                            xanList_append(methodWorklist, callee);
                            xanHashTable_insert(seenMethods, callee, callee);
                        }
                    }

                    /* Next callee. */
                    calleeItem = calleeItem->next;
                }

                /* Free the memory.
                 */
                xanList_destroyList(calleeList);
                break;
        }
    }
}

/**
 * Add all non-seen predecessors to the worklist.
 */
static void
addPredecessors(ir_method_t *method, ir_instruction_t *inst, XanStack *instWorklist, XanBitSet *seenInsts, XanList **predecessors) {
    ir_instruction_t *pred = NULL;
    XanListItem *item;

    item = xanList_first(predecessors[inst->ID]);
    while (item != NULL) {
        pred = item->data;
        if (!xanBitSet_isBitSet(seenInsts, pred->ID)) {
            xanStack_push(instWorklist, pred);
            xanBitSet_setBit(seenInsts, pred->ID);
        }
        item = item->next;
    }
}

/**
 * Add all non-seen successors to the worklist.
 */
static void
addSuccessors(ir_method_t *method, ir_instruction_t *inst, XanStack *instWorklist, XanBitSet *seenInsts, XanList **successors) {
    ir_instruction_t *succ = NULL;
    XanListItem *item;

    item = xanList_first(successors[inst->ID]);
    while (item != NULL) {
        succ = item->data;
        if (!xanBitSet_isBitSet(seenInsts, succ->ID)) {
            xanStack_push(instWorklist, succ);
            xanBitSet_setBit(seenInsts, succ->ID);
        }
        item = item->next;
    }
}

/**
 * Propagate a variable definition down the CFG to mark RAW and WAW
 * dependences.  Continues until it reaches a redefiner or an instruction
 * that has already been seen.
 */
static void
propagateDefDownwards(ir_method_t *method, ir_instruction_t *inst, JITNUINT def, XanList **successors) {
    JITUINT32 instType;
    JITUINT32 block;
    JITUINT8 useOffset;
    XanStack *instWorklist;
    XanBitSet *seenInsts;
    XanBitSet *possibleRAW;
    XanBitSet *possibleWAW;

    /* Compute information about this variable definition. */
    instType = IRMETHOD_getInstructionType(inst);
    block = def / (sizeof(JITNUINT) * 8);
    useOffset = def % (sizeof(JITNUINT) * 8);

    /* Get possible dependences. */
    possibleRAW = getPossibleDeps(method, inst, DEP_RAW);
    possibleWAW = getPossibleDeps(method, inst, DEP_WAW);

    /* Initial stack has all successors on. */
    instWorklist = xanStack_new(allocFunction, freeFunction, NULL);
    seenInsts = xanBitSet_new(IRMETHOD_getInstructionsNumber(method) + 1);
    addSuccessors(method, inst, instWorklist, seenInsts, successors);

    /* Iterate over the worklist until all instructions are visited. */
    while (xanStack_getSize(instWorklist) > 0) {
        ir_instruction_t *succ = xanStack_pop(instWorklist);

        /* Check to see if there is a use of the variable. */
        assert(method && succ && inst);
        if ((ir_instr_liveness_use_get(method, succ, block) >> useOffset) & 0x1) {

            /* Get address instructions don't actually read their variables. */
            if (IRMETHOD_getInstructionType(succ) != IRGETADDRESS) {

                /* Differentiate between memory and register dependences. */
                if (instType == IRSTOREREL) {
                    IRMETHOD_addInstructionDataDependence(method, succ, inst, DEP_MRAW, NULL);
                } else {
                    IRMETHOD_addInstructionDataDependence(method, succ, inst, DEP_RAW, NULL);
                    /* PDEBUG("RAW dependence between %u and %u\n", succ->ID, inst->ID); */
                }
            }
        }

        /* Check to see if there is a definition of this variable. */
        if (succ->metadata->liveness.def == def) {

            /* Differentiate between memory and register dependences. */
            if (instType == IRSTOREREL) {
                IRMETHOD_addInstructionDataDependence(method, succ, inst, DEP_MWAW, NULL);
            } else {
                IRMETHOD_addInstructionDataDependence(method, succ, inst, DEP_WAW, NULL);
                /* PDEBUG("WAW dependence between %u and %u\n", succ->ID, inst->ID); */
            }
        }
        /* No definition, must check successors to this instruction. */
        else {
            addSuccessors(method, succ, instWorklist, seenInsts, successors);
        }

        /* Add possible dependences. */
        if (possibleRAW && xanBitSet_isBitSet(possibleRAW, succ->ID)) {
            IRMETHOD_addInstructionDataDependence(method, succ, inst, DEP_RAW, NULL);
            /* PDEBUG("Definite RAW dependence between %u and %u\n", succ->ID, inst->ID); */
        }
        if (possibleWAW && xanBitSet_isBitSet(possibleWAW, succ->ID)) {
            IRMETHOD_addInstructionDataDependence(method, succ, inst, DEP_WAW, NULL);
            /* PDEBUG("Definite WAW dependence between %u and %u\n", succ->ID, inst->ID); */
        }
    }

    /* Clean up. */
    xanStack_destroyStack(instWorklist);
    xanBitSet_free(seenInsts);
}

/**
 * Propagate a variable definition up the CFG to mark WAR and WAW
 * dependences.  Continues until it reaches a redefiner or an instruction
 * that has already been seen.
 */
static void
propagateDefUpwards(ir_method_t *method, ir_instruction_t *inst, JITNUINT def, XanList **predecessors) {
    JITUINT32 instType;
    JITUINT32 block;
    JITUINT8 useOffset;
    XanStack *instWorklist;
    XanBitSet *seenInsts;
    XanBitSet *possibleWAR;
    XanBitSet *possibleWAW;

    /* Compute information about this variable definition. */
    instType = IRMETHOD_getInstructionType(inst);
    block = def / (sizeof(JITNUINT) * 8);
    useOffset = def % (sizeof(JITNUINT) * 8);

    /* Get possible dependences. */
    possibleWAR = getPossibleDeps(method, inst, DEP_WAR);
    possibleWAW = getPossibleDeps(method, inst, DEP_WAW);

    /* Initial stack has all predecessors on. */
    instWorklist = xanStack_new(allocFunction, freeFunction, NULL);
    seenInsts = xanBitSet_new(IRMETHOD_getInstructionsNumber(method) + 1);
    addPredecessors(method, inst, instWorklist, seenInsts, predecessors);

    /* Iterate over the worklist until all instructions are visited. */
    while (xanStack_getSize(instWorklist) > 0) {
        ir_instruction_t *pred = xanStack_pop(instWorklist);

        /* Check to see if there is a use of the variable. */
        assert(method && pred && inst);
        if ((ir_instr_liveness_use_get(method, pred, block) >> useOffset) & 0x1) {

            /* Get address instructions don't actually read their variables. */
            if (IRMETHOD_getInstructionType(pred) != IRGETADDRESS) {

                /* Differentiate between memory and register dependences. */
                if (instType == IRSTOREREL) {
                    IRMETHOD_addInstructionDataDependence(method, inst, pred, DEP_MWAR, NULL);
                } else {
                    IRMETHOD_addInstructionDataDependence(method, inst, pred, DEP_WAR, NULL);
                    /* PDEBUG("WAR dependence between %u and %u\n", inst->ID, pred->ID); */
                }
            }
        }

        /* Check to see if there is a definition of this variable. */
        if (pred->metadata->liveness.def == def) {

            /* Differentiate between memory and register dependences. */
            if (instType == IRSTOREREL) {
                IRMETHOD_addInstructionDataDependence(method, inst, pred, DEP_MWAW, NULL);
            } else {
                IRMETHOD_addInstructionDataDependence(method, inst, pred, DEP_WAW, NULL);
                /* PDEBUG("WAW dependence between %u and %u\n", inst->ID, pred->ID); */
            }
        }
        /* No definition, must check predecessors to this instruction. */
        else {
            addPredecessors(method, pred, instWorklist, seenInsts, predecessors);
        }

        /* Add possible dependences. */
        if (possibleWAR && xanBitSet_isBitSet(possibleWAR, pred->ID)) {
            IRMETHOD_addInstructionDataDependence(method, inst, pred, DEP_WAR, NULL);
            /* PDEBUG("Definite WAR dependence between %u and %u\n", inst->ID, pred->ID); */
        }
        if (possibleWAW && xanBitSet_isBitSet(possibleWAW, pred->ID)) {
            IRMETHOD_addInstructionDataDependence(method, inst, pred, DEP_WAW, NULL);
            /* PDEBUG("Definite WAW dependence between %u and %u\n", inst->ID, pred->ID); */
        }
    }

    /* Clean up. */
    xanStack_destroyStack(instWorklist);
    xanBitSet_free(seenInsts);
}

/**
 * Calculate data dependences from variable definitions within the given
 * method.
 */
static void calculateVariableDependencesInMethod (ir_method_t *method, XanList **successors, XanList **predecessors) {
    JITUINT32 i;
    PDEBUG("Variable deps for %s\n", IRMETHOD_getMethodName(method));

    /* Consider all instructions. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        JITNUINT def = livenessGetDef(method, inst);

        /* Check whether this instruction defines anything. */
        if (def != -1) {

            /* Propagate this information to all users and definers. */
            propagateDefDownwards(method, inst, def, successors);
            propagateDefUpwards(method, inst, def, predecessors);
        }
    }
}


/**
 * Calculate data dependences between variables within all methods starting
 * at the call graph rooted at the given method.
 */
static void calculateVariableDependences(ir_method_t *method, XanHashTable *methodsInstructionSuccessors, XanHashTable *methodsInstructionPredecessors) {
    XanList *methodWorklist;
    XanHashTable *seenMethods;

    /* Initialise each structure. */
    methodWorklist = xanList_new(allocFunction, freeFunction, NULL);
    seenMethods = xanHashTable_new(11, JITFALSE, allocFunction, reallocFunction, freeFunction, NULL, NULL);
    assert(methodWorklist && seenMethods);

    /* Start with the current method. */
    xanList_insert(methodWorklist, method);
    xanHashTable_insert(seenMethods, method, method);

    /* Visit each method that is reachable. */
    while (xanList_length(methodWorklist) > 0) {
        XanListItem *currItem;
        ir_method_t *currMethod;

        /* Get the first item from the list. */
        currItem = xanList_first(methodWorklist);
        currMethod = currItem->data;
        xanList_deleteItem(methodWorklist, currItem);

        /* Compute necessary method information	*/
        IROPTIMIZER_callMethodOptimization(irOptimizer, currMethod, LIVENESS_ANALYZER);
        IROPTIMIZER_callMethodOptimization(irOptimizer, currMethod, ESCAPES_ANALYZER);

        /* Check if the method is empty	*/
        if (IRMETHOD_hasMethodInstructions(currMethod)) {
            XanList **successors;
            XanList **predecessors;

            /* Add called methods to the worklist. */
            addCalledMethodsToWorklist(currMethod, methodWorklist, seenMethods);

            /* Fetch the successors of the method */
            successors = xanHashTable_lookup(methodsInstructionSuccessors, currMethod);
            if (successors == NULL) {
                JITUINT32 count;
                JITUINT32 instructionsNumber;
                instructionsNumber = IRMETHOD_getInstructionsNumber(currMethod);
                successors = allocFunction(sizeof(XanList *) * (instructionsNumber+1));
                for (count=0; count <= instructionsNumber; count++) {
                    ir_instruction_t *inst;
                    inst = IRMETHOD_getInstructionAtPosition(currMethod, count);
                    successors[count] = IRMETHOD_getInstructionSuccessors(currMethod, inst);
                }
                xanHashTable_insert(methodsInstructionSuccessors, currMethod, successors);
            }

            /* Fetch the predecessors of the method */
            predecessors = xanHashTable_lookup(methodsInstructionPredecessors, currMethod);
            if (predecessors == NULL) {
                JITUINT32 count;
                JITUINT32 instructionsNumber;
                instructionsNumber = IRMETHOD_getInstructionsNumber(currMethod);
                predecessors = allocFunction(sizeof(XanList *) * (instructionsNumber+1));
                for (count=0; count <= instructionsNumber; count++) {
                    ir_instruction_t *inst;
                    inst = IRMETHOD_getInstructionAtPosition(currMethod, count);
                    predecessors[count] = IRMETHOD_getInstructionPredecessors(currMethod, inst);
                }
                xanHashTable_insert(methodsInstructionPredecessors, currMethod, predecessors);
            }

            /* Calculate variable dependences. */
            IROPTIMIZER_callMethodOptimization(irOptimizer, currMethod, LIVENESS_ANALYZER);
            calculateVariableDependencesInMethod(currMethod, successors, predecessors);
        }
    }

    /* Clean up. */
    xanList_destroyList(methodWorklist);
    xanHashTable_destroyTable(seenMethods);
}


/**
 * Print dependences for all non-library methods to the given filename or
 * stdout if the filename is '-'.
 */
static void
printDependencesToFilename(char *filename) {
    XanList *allMethods = IRPROGRAM_getIRMethods();
    XanListItem *methodItem = xanList_first(allMethods);
    FILE *file;
    if (!strcmp(filename, "-")) {
        file = stdout;
    } else {
        file = fopen(filename, "w");
    }
    while (methodItem) {
        ir_method_t *currMethod = methodItem->data;
        if (!IRPROGRAM_doesMethodBelongToALibrary(currMethod)) {
            printDependences(currMethod, file);
        }
        methodItem = xanList_next(methodItem);
    }
    xanList_destroyList(allMethods);
    if (file != stdout) {
        fclose(file);
    }
}


/**
 * Dump dependences between all instructions contained in an instruction IDs
 * file to a new file.
 */
static void
dumpInstructionDependences(char *filename) {
    FILE *file;
    XanList *allMethods;
    XanListItem *methodItem;
    XanHashTable *methodsToConsider;
    XanHashTable *instsToConsider;

    /* Read the list of instructions to consider. */
    methodsToConsider = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    instsToConsider = MISC_loadInstructionDictionary(methodsToConsider, "instruction_IDs.txt", NULL, NULL);

    /* Open the output file. */
    if (strlen(filename) == 0) {
        file = fopen("static_inst_dependences.txt", "w");
    } else if (!strcmp(filename, "-")) {
        file = stdout;
    } else {
        file = fopen(filename, "w");
    }

    /* Iterate over all methods. */
    allMethods = IRPROGRAM_getIRMethods();
    methodItem = xanList_first(allMethods);
    while (methodItem) {
        ir_method_t *currMethod = methodItem->data;
        if (!IRPROGRAM_doesMethodBelongToALibrary(currMethod)) {
            JITUINT32 i, numInsts = IRMETHOD_getInstructionsNumber(currMethod);
            for (i = 0; i < numInsts; ++i) {
                ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(currMethod, i);
                XanHashTableItem *fromIdItem = xanHashTable_lookupItem(instsToConsider, inst);
                if (fromIdItem) {
                    JITUINT32 fromId = (JITUINT32)(JITNUINT)fromIdItem->element;
                    XanHashTable *dependsOn = inst->metadata->data_dependencies.dependsFrom;
                    if (dependsOn && xanHashTable_elementsInside(dependsOn) > 0) {
                        XanHashTableItem *depItem = xanHashTable_first(dependsOn);
                        while (depItem) {
                            ir_instruction_t *toInst = (ir_instruction_t *)depItem->elementID;
                            data_dep_arc_list_t *dep = (data_dep_arc_list_t *)depItem->element;
                            XanHashTableItem *toIdItem = xanHashTable_lookupItem(instsToConsider, toInst);
                            if (toIdItem) {
                                JITUINT32 toId = (JITUINT32)(JITNUINT)toIdItem->element;
                                fprintf(file, "%u %u %u\n", fromId, toId, dep->depType);
                            }
                            depItem = xanHashTable_next(dependsOn, depItem);
                        }
                    }
                }
            }
        }
        methodItem = xanList_next(methodItem);
    }

    /* Close the file. */
    if (file != stdout) {
        fclose(file);
    }

    /* Free the memory. */
    xanList_destroyList(allMethods);
    xanHashTable_destroyTable(instsToConsider);
    xanHashTable_destroyTable(methodsToConsider);
}


/**
 * Initialise environment variables.
 */
static void
initEnvironment(void) {
    char *env;
    env = getenv("DDG_PROVIDE_MEMORY_ALLOCATORS");
    if (env != NULL) {
        provideMemoryAllocators = atoi(env);
    }
    env = getenv("DDG_DUMP_CFG");
    if (env != NULL) {
        ddgDumpCfg = atoi(env);
    }
}


/**
 * Data dependences are calculated by considering the control flow graph and
 * performing interprocedural pointer analysis.
 */
static inline void ddg_do_job (ir_method_t *method) {
    char *env;
    ir_method_t *mainMethod;
    XanHashTable *methodsInstructionSuccessors;
    XanHashTable *methodsInstructionPredecessors;
    XanHashTableItem *item;
    XanList *allMethods;
    XanListItem *methodItem;

    /* Assertions. */
    assert(method != NULL);
    PDEBUG("Start %s\n", IRMETHOD_getMethodName(method));

    /* Can we avoid some work? */
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        PDEBUG("There are no instructions: exiting\n");
        return;
    }
    if (IRMETHOD_getNumberOfVariablesNeededByMethod(method) == 0) {
        PDEBUG("There are no variables: exiting\n");
        return;
    }

    /* Check if the SSA plugin has been enabled	*/
    if (!IROPTIMIZER_canCodeToolBeUsed(irOptimizer, SSA_CONVERTER)) {
        fprintf(stderr, "DDG: Error = SSA transformation cannot be computed.\n");
        if (!IROPTIMIZER_isCodeToolInstalled(irOptimizer, SSA_CONVERTER)) {
            fprintf(stderr, "	SSA plugin has not been installed in the system.\n");
        } else {
            fprintf(stderr, "	SSA plugin has been disabled by command line.\n");
            fprintf(stderr, "	The DDG analysis requires it; please enable it by adding \"%s\" to the enable sets.\n", IROPTIMIZER_jobToShortName(SSA_CONVERTER));
        }
        abort();
    }

    /* Initialise globals from environment variables. */
    initEnvironment();

    /* Keep all methods for use later. */
    allMethods = IRPROGRAM_getIRMethods();

    /* Clear all prior dependence information. */
    methodItem = xanList_first(allMethods);
    while (methodItem) {
        ir_method_t *currMethod = methodItem->data;
        IRMETHOD_destroyDataDependences(currMethod);
        methodItem = xanList_next(methodItem);
    }

    /* Get the verbosity level for this pass. */
    if (ddgDumpCfg > 0) {
        char *prevDotPrinterName = getenv("DOTPRINTER_FILENAME");
        setenv("DOTPRINTER_FILENAME", "For_DDG", 1);
        methodItem = xanList_first(allMethods);
        while (methodItem) {
            ir_method_t *currMethod = methodItem->data;
            if (!IRPROGRAM_doesMethodBelongToALibrary(currMethod)) {
                IROPTIMIZER_callMethodOptimization(irOptimizer, currMethod, METHOD_PRINTER);
            }
            methodItem = xanList_next(methodItem);
        }
        setenv("DOTPRINTER_FILENAME", prevDotPrinterName, 1);
    }

    /* Find the start of the user program. */
    mainMethod = internal_getEntryPoint();

    /* Allocate space for possible dependences. */
    initialisePossibleDeps();

    /* Calculate memory dependences.  It's important to do this first
       because it may expose extra variable dependences which will be
       missed if the variable analysis is performed first. */
    doVllpa(mainMethod);

    /* Print dependences. */
    env = getenv("DDG_PRINT_IP_DEPENDENCES");
    if (env) {
        printDependencesToFilename(env);
    }

    /* Allocate the instruction successors and predecessors caches.  These are for performance only. */
    methodsInstructionSuccessors = xanHashTable_new(11, JITFALSE, allocFunction, reallocFunction, freeFunction, NULL, NULL);
    methodsInstructionPredecessors = xanHashTable_new(11, JITFALSE, allocFunction, reallocFunction, freeFunction, NULL, NULL);

    /* Calculate variable dependences. */
    calculateVariableDependences(mainMethod, methodsInstructionSuccessors, methodsInstructionPredecessors);

    /* Free the memory */
    item = xanHashTable_first(methodsInstructionSuccessors);
    while (item != NULL) {
        XanList 	**successors;
        JITUINT32 	count;
        JITUINT32	instsNumber;
        ir_method_t 	*m;
        successors = item->element;
        m = item->elementID;
        instsNumber	= IRMETHOD_getInstructionsNumber(m);
        for (count=0; count <= instsNumber ; count++) {
            xanList_destroyList(successors[count]);
        }
        freeFunction(successors);
        item = xanHashTable_next(methodsInstructionSuccessors, item);
    }
    item = xanHashTable_first(methodsInstructionPredecessors);
    while (item != NULL) {
        XanList 	**predecessors;
        JITUINT32 	count;
        JITUINT32	instsNumber;
        ir_method_t 	*m;
        predecessors = item->element;
        m = item->elementID;
        instsNumber	= IRMETHOD_getInstructionsNumber(m);
        for (count=0; count <= instsNumber ; count++) {
            xanList_destroyList(predecessors[count]);
        }
        freeFunction(predecessors);
        item = xanHashTable_next(methodsInstructionPredecessors, item);
    }
    xanHashTable_destroyTable(methodsInstructionSuccessors);
    xanHashTable_destroyTable(methodsInstructionPredecessors);

    /* Print dependences. */
    env = getenv("DDG_PRINT_ALL_DEPENDENCES");
    if (env) {
        printDependencesToFilename(env);
    }

    /* Dump dependences with instruction IDs read from a file. */
    env = getenv("DDG_DUMP_INST_DEPENDENCES");
    if (env) {
        dumpInstructionDependences(env);
    }

    /* Remove false data dependences due to not escaped global variables	*/
    remove_false_data_dependences_due_to_not_escaped_global_variables();

    /* Declare memory aliases					*/
    /* declare_memory_aliases(method); */

    /* Notify that the information about DDG is now valid for all methods.
     */
    methodItem = xanList_first(allMethods);
    while (methodItem) {
        ir_method_t *currMethod = methodItem->data;
        IROPTIMIZER_setInformationAsValid(currMethod, DDG_COMPUTER);
        methodItem = xanList_next(methodItem);
    }

    /* Clean up. */
    freePossibleDeps();
    xanList_destroyList(allMethods);

    /* Possibly exit here. */
    env = getenv("DDG_EXIT_AT_FINISH");
    if (env && atoi(env)) {
        PDEBUG("Finished calculating DDG, now exiting\n");
        exit(0);
    }

    PDEBUG("Finished calculating DDG\n");
    return ;
}

/**
 * @brief Plugin interface.
 *
 * Structure storing pointers to the functions used by the plugin as interface.
 */
ir_optimization_interface_t plugin_interface = {
    get_job_kind,
    get_dependences,
    ddg_init,
    ddg_shutdown,
    ddg_do_job,
    get_version,
    get_informations,
    get_author,
    ddg_get_invalidations,
    NULL,
    ddg_getCompilationFlags,
    ddg_getCompilationTime
};

static inline void ddg_shutdown (JITFLOAT32 totalTime) {
}

static inline JITUINT64 ddg_get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline JITUINT64 get_job_kind (void) {
    return DDG_COMPUTER;
}

static inline JITUINT64 get_dependences (void) {
    return LIVENESS_ANALYZER | ESCAPES_ANALYZER;
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

static inline void ddg_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    irLib = lib;
    irOptimizer = optimizer;
    outPrefix = outputPrefix;
}

static inline void remove_false_data_dependences_due_to_not_escaped_global_variables (void) {
    XanList 	*l;
    ir_method_t	*startMethod;
    XanListItem	*item;

    /* Fetch the list of methods	*/
    startMethod	= IRPROGRAM_getEntryPointMethod();
    l		= IRPROGRAM_getReachableMethods(startMethod);

    /* Compute the list of escaped	*
     * global variables		*/
    //TODO

    /* Remove false data dependences*/
    item		= xanList_first(l);
    while (item != NULL) {
        ir_method_t	*m;
        JITUINT32	instructionsNumber;

        /* Fetch the method		*/
        m		= item->data;
        assert(m != NULL);

        /* Look at the instructions	*/
        instructionsNumber	= IRMETHOD_getInstructionsNumber(m);
        for (JITUINT32 instID=0; instID < instructionsNumber; instID++) {
            ir_item_t		*param;
            ir_instruction_t	*inst;

            /* Fetch the instruction	*/
            inst	= IRMETHOD_getInstructionAtPosition(m, instID);
            assert(inst != NULL);
            if (inst->metadata == NULL) {
                continue;
            }
            if (	((inst->metadata->data_dependencies).dependingInsts == NULL)	&&
                    ((inst->metadata->data_dependencies).dependsFrom == NULL)	) {
                continue;
            }

            /* Check if the instruction is	*
             * a memory operation		*/
            if (!IRMETHOD_isAMemoryInstruction(inst)) {
                continue ;
            }
            if (	(inst->type != IRLOADREL)	&&
                    (inst->type != IRSTOREREL)	) {
                continue;
            }

            /* Check if the instruction 	*
             * accesses to a global variable*/
            param	= IRMETHOD_getInstructionParameter1(inst);
            if (!IRDATA_isAGlobalVariable(param)) {
                continue;
            }

            /* Check if the current global	*
             * variable escapes		*/
            //TODO

            /* Check every dependence	*/
            if ((inst->metadata->data_dependencies).dependsFrom != NULL) {
                XanList		*deps;

                /* Fetch the list		*/
                deps	= IRMETHOD_getDataDependencesFromInstruction(m, inst);

                /* Check whether there are dependences.
                 */
                if (deps != NULL) {
                    XanListItem	*item2;
                    JITBOOLEAN	deleted;

                    /* Free the memory.
                     */
                    xanList_destroyList(deps);

                    /* Analyze the list		*/
                    deleted	= JITTRUE;
                    while (deleted) {
                        deleted	= JITFALSE;

                        /* Fetch the list		*/
                        deps	= IRMETHOD_getDataDependencesFromInstruction(m, inst);

                        if (deps != NULL) {
                            item2	= xanList_first(deps);
                            while (item2 != NULL) {
                                data_dep_arc_list_t *dd;
                                dd	= item2->data;
                                assert(dd != NULL);
                                if (	((dd->depType & DEP_MRAW) != 0)	||
                                        ((dd->depType & DEP_MWAR) != 0)	||
                                        ((dd->depType & DEP_MWAW) != 0)	) {
                                    assert(IRMETHOD_doesInstructionBelongToMethod(m, dd->inst));
                                    if (IRMETHOD_isAMemoryInstruction(dd->inst)) {
                                        ir_item_t	*param1;
                                        param1	= IRMETHOD_getInstructionParameter1(dd->inst);
                                        if (!IRDATA_isAGlobalVariable(param1)) {
                                            if (	((dd->depType & DEP_RAW) == 0)	&&
                                                    ((dd->depType & DEP_WAR) == 0)	&&
                                                    ((dd->depType & DEP_WAW) == 0)	) {

                                                /* Delete the dependence		*/
                                                ir_instruction_t *toInst = dd->inst;
                                                IRMETHOD_deleteInstructionsDataDependence(m, inst, toInst);
                                                IRMETHOD_deleteInstructionsDataDependence(m, toInst, inst);
                                                deleted	= JITTRUE;
                                                break;
                                            }
                                        }
                                    }
                                }
                                item2	= item2->next;
                            }

                            /* Free the memory.
                             */
                            xanList_destroyList(deps);
                        }
                    }
                }
            }
        }

        /* Fetch the next element	*/
        item	= item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(l);

    /* Return			*/
    return ;
}

static inline ir_method_t * internal_getEntryPoint (void) {
    XanList		*l;
    ir_method_t	*m;

    /* Initialize variables.
     */
    m	= NULL;

    /* Fetch the main method.
     */
    l 	= IRPROGRAM_getMethodsWithName((JITINT8 *)"main");
    if (l != NULL) {
        if (xanList_length(l) == 1) {
            m	= xanList_first(l)->data;
            assert(m != NULL);
        }
        xanList_destroyList(l);
    }
    if (m == NULL) {
        l 	= IRPROGRAM_getMethodsWithName((JITINT8 *)"Main");
        if (l != NULL) {
            if (xanList_length(l) == 1) {
                m	= xanList_first(l)->data;
                assert(m != NULL);
            }
            xanList_destroyList(l);
        }
    }

    /* Check whether the main method exists.
     */
    if (m == NULL) {
        m	= IRPROGRAM_getEntryPointMethod();
    }
    assert(m != NULL);

    return m;
}

static inline void ddg_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void ddg_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
