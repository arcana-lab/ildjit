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
#include <sys/resource.h>
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
#include <chiara_profile.h>
#include <chiara_system.h>
// End


/**
 * Macros for debug printing.
 **/
#undef PDEBUG
static JITINT32 debugLevel = 5;
#define PDEBUG(lvl, fmt, args...) if (lvl <= debugLevel) fprintf(stderr, "DDPROF: " fmt, ## args);
#define PDEBUGC(lvl, fmt, args...) if (lvl <= debugLevel) fprintf(stderr, fmt, ## args);
#define PDEBUGNL(lvl, fmt, args...) if (lvl <= debugLevel) fprintf(stderr, "DDPROF:\nDDPROF: " fmt, ##args);
#define PDECL(decl) decl


/**
 * A shadow state structure holds information about the current state of
 * memory for a sequential segment.  Stores are recorded here by placing their
 * iteration number and a pointer the their IR instruction in the hash table at
 * the addresses they touch.  When a dependence is identified, the relevant
 * instructions are placed into the other hash table, along with their
 * iteration numbers.  There is a single shadow state for each sequential
 * segment within each parallelised loop.
 *
 * Dependences are stored a hierarchy of hash tables.  The first has keys of
 * instructions that start the dependence (i.e. the earlier-executed
 * instructions).  The second has keys that are the instructions that end the
 * dependence.  The third has keys that are the iteration numbers of the start
 * of the dependence, mapping to a bit set of all iterations that end the
 * dependence.
 **/
typedef struct shadow_state_t {
    XanHashTable *memReads;
    XanHashTable *memWrites;
    XanHashTable *allocations;
    XanHashTable *dependences;
    ir_instruction_t *libraryCall;
    JITINT32 libraryIter;
    JITINT32 iteration;
} shadow_state_t;


/**
 * All information required by this pass.  It's kept together to avoid having
 * lots of globals everywhere.
 **/
typedef struct profile_info_t {
    JITBOOLEAN identifyDataDepsMWAR;
    JITBOOLEAN identifyDataDepsMWAW;
    XanHashTable *profiledMethods;
    XanHashTable *instToMethod;
    XanHashTable *instToOrigID;
    XanHashTable *shadowStates;
    XanHashTable *allLoops;
    ir_optimizer_t *irOptimizer;
} profile_info_t;


/**
 * Information required for this profiling tool.
 **/
static profile_info_t *profileInfo = NULL;


/**
 * The shadow state of the current sequential segment.
 **/
static shadow_state_t *currShadowState = NULL;


/**
 * Convert from pointer to integer or back.
 **/
#define ptrToInt(ptr) ((JITINT32)(JITNINT)ptr)
#define ptrToUint(ptr) ((JITUINT32)(JITNUINT)ptr)
#define intToPtr(val) ((void *)(JITNINT)val)
#define uintToPtr(val) ((void *)(JITNUINT)val)


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
 * Allocate a parameter with no fields set.
 **/
static inline ir_item_t *
newParamEmpty(void) {
    return allocFunction(sizeof(ir_item_t));
}


/**
 * Allocate a parameter with fields set.
 **/
static inline ir_item_t *
newParam(IR_ITEM_VALUE v, JITINT16 type, JITUINT16 internal_type) {
    ir_item_t *param = newParamEmpty();
    param->value.v = v;
    param->type = type;
    param->internal_type = internal_type;
    return param;
}


/**
 * Free a list of parameters.
 **/
static void
freeParamList(XanList *params) {
    xanList_destroyListAndData(params);
}


/**
 * Initialise global structures for the loop profiler.
 **/
static void
initGlobals(ir_optimizer_t *optimizer) {
    char *env;
    profileInfo = allocFunction(sizeof(profile_info_t));
    profileInfo->irOptimizer = optimizer;
    profileInfo->instToMethod = newHashTable();
    profileInfo->instToOrigID = newHashTable();
    profileInfo->shadowStates = newHashTable();
    profileInfo->allLoops = NULL;
    env = getenv("CHIARA_DEPENDENCE_PROFILE_MWAR");
    if (env != NULL) {
        profileInfo->identifyDataDepsMWAR = atoi(env) != 0;
    } else {
        profileInfo->identifyDataDepsMWAR = JITFALSE;
    }
    env = getenv("CHIARA_DEPENDENCE_PROFILE_MWAW");
    if (env != NULL) {
        profileInfo->identifyDataDepsMWAW = atoi(env) != 0;
    } else {
        profileInfo->identifyDataDepsMWAW = JITFALSE;
    }
}


/**
 * Write the data dependence information from a shadow state to a file.
 **/
static void
writeOutDataDependences(shadow_state_t *state, FILE *outfile) {
    XanHashTableItem *depItem = xanHashTable_first(state->dependences);
    while (depItem) {
        ir_instruction_t *fromInst = depItem->elementID;
        ir_method_t *fromMethod = xanHashTable_lookup(profileInfo->instToMethod, fromInst);
        JITUINT32 fromInstOrigID = ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, fromInst));
        XanHashTable *toInstTable = depItem->element;
        XanHashTableItem *toItem = xanHashTable_first(toInstTable);
        while (toItem) {
            ir_instruction_t *toInst = toItem->elementID;
            ir_method_t *toMethod = xanHashTable_lookup(profileInfo->instToMethod, toInst);
            JITUINT32 toInstOrigID = ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, toInst));
            XanHashTable *fromIterTable = toItem->element;
            XanHashTableItem *fromItem = xanHashTable_first(fromIterTable);
            fprintf(outfile, "%s:%d -> %s:%d\n", IRMETHOD_getMethodName(fromMethod), fromInstOrigID, IRMETHOD_getMethodName(toMethod), toInstOrigID);
            while (fromItem) {
                JITINT32 fromIter = ptrToInt(fromItem->elementID);
                XanBitSet *toIterSet = fromItem->element;
                JITINT32 numIter = xanBitSet_length(toIterSet);
                JITINT32 toIter = 0;
                JITBOOLEAN first = JITTRUE;
                fprintf(outfile, "  %d:", fromIter);
                while ((toIter = xanBitSet_getFirstBitSetInRange(toIterSet, toIter, numIter)) != -1) {
                    if (first) {
                        fprintf(outfile, "%d", toIter);
                        first = JITFALSE;
                    } else {
                        fprintf(outfile, ", %d", toIter);
                    }
                }
                fprintf(outfile, "\n");
                fromItem = xanHashTable_next(fromIterTable, fromItem);
            }
            fprintf(outfile, "\n");
            toItem = xanHashTable_next(toInstTable, toItem);
        }
        depItem = xanHashTable_next(state->dependences, depItem);
    }
}


/**
 * Record a data dependence between two instructions.
 **/
static void
recordDataDependence(ir_instruction_t *fromInst, ir_instruction_t *toInst, JITINT32 fromIter, JITINT32 toIter) {
    XanHashTable *toInstTable;
    XanHashTable *fromIterTable;
    XanBitSet *toIterSet;

    /* Map from the start instruction to all end ones. */
    toInstTable = xanHashTable_lookup(currShadowState->dependences, fromInst);
    if (!toInstTable) {
        toInstTable = newHashTable();
        xanHashTable_insert(currShadowState->dependences, fromInst, toInstTable);
    }

    /* Map from the end instruction to the start iteration. */
    fromIterTable = xanHashTable_lookup(toInstTable, toInst);
    if (!fromIterTable) {
        fromIterTable = newHashTable();
        xanHashTable_insert(toInstTable, toInst, fromIterTable);
    }

    /* Map from the start iteration to a bit set of end iterations. */
    toIterSet = xanHashTable_lookup(fromIterTable, intToPtr(fromIter));
    if (!toIterSet) {
        toIterSet = xanBitSet_new(toIter);
        xanHashTable_insert(fromIterTable, intToPtr(fromIter), toIterSet);
    }

    /* Mark this iteration. */
    xanBitSet_setBit(toIterSet, toIter);
}


/**
 * Determine whether there are any dependences arising from accesses to memory
 * through the given table that records reads or writes.
 **/
static void
identifyMemoryDataDependences(ir_method_t *method, ir_instruction_t *inst, void *addr, JITUINT32 size, XanHashTable *table, JITBOOLEAN add, JITBOOLEAN remove) {
    JITUINT32 s;
    for (s = 0; s < size; ++s) {
        XanHashTableItem *memItem = xanHashTable_lookupItem(table, addr + s);
        if (memItem) {
            if (ptrToInt(memItem->element) < currShadowState->iteration) {
                recordDataDependence(memItem->elementID, inst, ptrToInt(memItem->element), currShadowState->iteration);
            } else {
                assert(ptrToInt(memItem->element) == currShadowState->iteration);
            }
            if (add) {
                memItem->element = intToPtr(currShadowState->iteration);
            } else if (remove) {
                xanHashTable_removeElement(table, memItem->elementID);
            }
        } else {
            if (currShadowState->libraryCall && currShadowState->libraryIter < currShadowState->iteration) {
                recordDataDependence(currShadowState->libraryCall, inst, currShadowState->libraryIter, currShadowState->iteration);
            }
            if (add) {
                xanHashTable_insert(table, addr + s, intToPtr(currShadowState->iteration));
            }
        }
    }
}


/**
 * Add a memory access to a table of them.
 **/
static void
addMemoryAccess(void *addr, JITUINT32 size, XanHashTable *table) {
    JITUINT32 s;
    for (s = 0; s < size; ++s) {
        XanHashTableItem *memItem = xanHashTable_lookupItem(table, addr + s);
        if (memItem) {
            memItem->element = intToPtr(currShadowState->iteration);
        } else {
            xanHashTable_insert(table, addr + s, intToPtr(currShadowState->iteration));
        }
    }
}


/**
 * Native call back from an instruction that reads memory for profiling data
 * dependences within loops.
 **/
static void
nativeMemReadProfile(ir_method_t *method, ir_instruction_t *inst, void *addr, JITUINT32 size) {
    /* Determine whether there are any RAW dependences. */
    PDEBUG(0, "Executing mem read %u to %p for %u bytes\n", ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, inst)), addr, size);
    identifyMemoryDataDependences(method, inst, addr, size, currShadowState->memWrites, JITFALSE, JITFALSE);

    /* Add the read, if required. */
    if (profileInfo->identifyDataDepsMWAR) {
        addMemoryAccess(addr, size, currShadowState->memReads);
    }
}


/**
 * Native call back from an instruction that writes memory for profiling data
 * dependences within loops.
 **/
static void
nativeMemWriteProfile(ir_method_t *method, ir_instruction_t *inst, void *addr, JITUINT32 size) {
    /* Determine whether there are any WAR dependences, if required. */
    PDEBUG(0, "Executing mem write %u to %p for %u bytes\n", ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, inst)), addr, size);
    if (profileInfo->identifyDataDepsMWAR) {
        identifyMemoryDataDependences(method, inst, addr, size, currShadowState->memReads, JITFALSE, JITTRUE);
    }

    /* Determine whether there are any WAW dependences and add the writes. */
    identifyMemoryDataDependences(method, inst, addr, size, currShadowState->memWrites, JITTRUE, JITFALSE);
}


/**
 * Native call back from an instruction that reads a string.
 **/
static void
nativeStringReadProfile(ir_method_t *method, ir_instruction_t *inst, void *addr) {
    nativeMemReadProfile(method, inst, addr, strlen(addr));
}


/**
 * Get the size of a memory allocation.  Returns 0 if no allocation could be
 * found.
 **/
static JITUINT32
getMemoryAllocationSize(void *addr) {
    XanHashTableItem *allocItem = xanHashTable_lookupItem(currShadowState->allocations, addr);
    if (allocItem) {
        return ptrToUint(allocItem->element);
    }
    return 0;
}


/**
 * Native call back from an instruction that frees memory for profiling data
 * dependences within loops.
 **/
static void
nativeMemFreeProfile(ir_method_t *method, ir_instruction_t *inst, void *addr) {
    JITUINT32 size = getMemoryAllocationSize(addr);
    XanHashTableItem *allocItem;

    /* Determine whether there are any WAR dependences, if required. */
    if (profileInfo->identifyDataDepsMWAR) {
        identifyMemoryDataDependences(method, inst, addr, size, currShadowState->memReads, JITFALSE, JITTRUE);
    }

    /* Record the allocation. */
    allocItem = xanHashTable_lookupItem(currShadowState->allocations, addr);
    if (allocItem) {
        allocItem->element = intToPtr(size);
    } else {
        xanHashTable_insert(currShadowState->allocations, addr, intToPtr(size));
    }

    /* Record writes to detect dependences. */
    addMemoryAccess(addr, size, currShadowState->memWrites);
}


/**
 * Native call back from an instruction that allocates memory for profiling
 * data dependences within loops.
 **/
static void
nativeMemAllocProfile(ir_method_t *method, ir_instruction_t *inst, void *addr, JITUINT32 size) {
    XanHashTableItem *allocItem;

    /* Clear out old reads hanging about since they don't affect dependences. */
    if (profileInfo->identifyDataDepsMWAR) {
        JITUINT32 s;
        for (s = 0; s < size; ++s) {
            XanHashTableItem *readItem = xanHashTable_lookupItem(currShadowState->memReads, addr + s);
            if (readItem) {
                xanHashTable_removeElement(currShadowState->memReads, readItem->elementID);
            }
        }
    }

    /* Record the allocation. */
    allocItem = xanHashTable_lookupItem(currShadowState->allocations, addr);
    if (allocItem) {
        allocItem->element = intToPtr(size);
    } else {
        xanHashTable_insert(currShadowState->allocations, addr, intToPtr(size));
    }

    /* Record writes to detect later dependences. */
    addMemoryAccess(addr, size, currShadowState->memWrites);
}


/**
 * Native call back from a direct library call for profiling data dependences
 * within loop iterations.
 **/
static void
nativeLibraryCallProfile(ir_method_t *method, ir_instruction_t *inst, ir_method_t *callee) {
    XanHashTableItem *memItem;

    /* Mark data dependences with all prior reads. */
    if (profileInfo->identifyDataDepsMWAR) {
        memItem = xanHashTable_first(currShadowState->memReads);
        while (memItem) {
            recordDataDependence(memItem->elementID, inst, ptrToInt(memItem->element), currShadowState->iteration);
            memItem = xanHashTable_next(currShadowState->memReads, memItem);
        }
        xanHashTable_emptyOutTable(currShadowState->memReads);
    }

    /* Mark data dependences with all prior writes. */
    memItem = xanHashTable_first(currShadowState->memWrites);
    while (memItem) {
        recordDataDependence(memItem->elementID, inst, ptrToInt(memItem->element), currShadowState->iteration);
        memItem = xanHashTable_next(currShadowState->memWrites, memItem);
    }
    xanHashTable_emptyOutTable(currShadowState->memWrites);

    /* A dependence with the last library call. */
    if (currShadowState->libraryCall && currShadowState->libraryIter < currShadowState->iteration) {
        recordDataDependence(currShadowState->libraryCall, inst, currShadowState->libraryIter, currShadowState->iteration);
    }

    /* Record the library call. */
    currShadowState->libraryCall = inst;
    currShadowState->libraryIter = currShadowState->iteration;
}


/**
 * Native call back from a direct call to a library method for profiling data
 * dependences within loops.
 **/
static void
nativeDirectCallProfile(ir_method_t *method, ir_instruction_t *inst, ir_method_t *callee) {
    /* Could use knowledge of specific library called. */
    nativeLibraryCallProfile(method, inst, callee);
}


/**
 * Create a shadow state structure.
 **/
static shadow_state_t *
createShadowState(void) {
    shadow_state_t *state = allocFunction(sizeof(shadow_state_t));
    if (profileInfo->identifyDataDepsMWAR) {
        state->memReads = newHashTable();
    }
    state->memWrites = newHashTable();
    state->allocations = newHashTable();
    state->dependences = newHashTable();
    state->libraryCall = NULL;
    state->libraryIter = 0;
    state->iteration = 0;
    return state;
}


/**
 * Clear a shadow state structure.
 **/
static void
clearShadowState(shadow_state_t *state, JITBOOLEAN clearDependences) {
    if (profileInfo->identifyDataDepsMWAR) {
        xanHashTable_emptyOutTable(state->memReads);
    }
    xanHashTable_emptyOutTable(state->memWrites);
    xanHashTable_emptyOutTable(state->allocations);
    if (clearDependences) {
        xanHashTable_emptyOutTable(state->dependences);
    }
    state->libraryCall = NULL;
    state->libraryIter = 0;
    state->iteration = 0;
}


/**
 * Native call back from a wait instruction for profiling data dependences
 * within loops.  This needs to select the correct shadow state for the
 * sequential segment from the hash table and update the iteration count, since
 * each sequential segment is entered once per iteration.
 **/
static void
nativeWaitProfile(ir_method_t *method, ir_instruction_t *inst, JITUINT32 id) {
    assert(currShadowState == NULL);
    PDEBUG(0, "Executing wait %u for sequential segment %u\n", ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, inst)), id);
    currShadowState = xanHashTable_lookup(profileInfo->shadowStates, uintToPtr(id));
    if (!currShadowState) {
        currShadowState = createShadowState();
        xanHashTable_insert(profileInfo->shadowStates, uintToPtr(id), currShadowState);
    } else {
        currShadowState->iteration += 1;
    }
}


/**
 * Native call back from a signal instruction for profiling data dependences
 * within loops.  This just resets the current shadow state pointer, to NULL to
 * detect bugs.
 **/
static void
nativeSignalProfile(ir_method_t *method, ir_instruction_t *inst, JITUINT32 id) {
    assert(currShadowState != NULL);
    PDEBUG(0, "Executing signal %u for sequential segment %u\n", ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, inst)), id);
    assert(xanHashTable_lookup(profileInfo->shadowStates, uintToPtr(id)) == currShadowState);
    currShadowState = NULL;
}


/**
 * Native call back from a loop pre-header instruction for profiling data
 * dependences within loops.  At the moment, this doesn't do anything.
 **/
static void
nativeLoopPreHeaderProfile(ir_instruction_t *inst) {
    /* Nothing. */
}


/**
 * Native call back from a loop header instruction for profiling data
 * dependences within loops.  At the moment, this doesn't need to do anything
 * because dealing with the shadow states is done at each loop invocation, so
 * in the pre-header.
 **/
static void
nativeLoopHeaderProfile(ir_instruction_t *inst) {
    PDEBUG(0, "Executing loop header %u\n", ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, inst)));
    /* Nothing. */
}


/**
 * Native call back from a loop exit instruction for profiling data dependences
 * within loops.  This prints each shadow state dependences to an output file
 * then clears everything.
 **/
static void
nativeLoopExitProfile(ir_instruction_t *inst) {
    FILE *dependenceFile = fopen("profiled_loop_dependences", "a");
    XanHashTableItem *stateItem = xanHashTable_first(profileInfo->shadowStates);
    PDEBUG(0, "Executing loop exit %u\n", ptrToUint(xanHashTable_lookup(profileInfo->instToOrigID, inst)));
    while (stateItem) {
        shadow_state_t *state = stateItem->element;
        writeOutDataDependences(state, dependenceFile);
        clearShadowState(state, JITTRUE);
        stateItem = xanHashTable_next(profileInfo->shadowStates, stateItem);
    }
    fclose(dependenceFile);
}


/**
 * Add profiling code to a loop's entry and exit points to determine the inter-
 * iteration data dependences within the loop.
 **/
static void
profileLoopEntryAndExits(ir_method_t *parallelMethod, loop_t *loop, ir_instruction_t *loopEntryPoint) {
    INSTS_addBasicInstructionProfileCall(parallelMethod, loopEntryPoint, "profileLoopHeader", nativeLoopHeaderProfile, NULL);
    LOOPS_profileLoopExits(parallelMethod, loop, "profileLoopExit", nativeLoopExitProfile);
}


/**
 * Set up parameters that are common to all profiling functions.
 **/
static XanList *
getCommonInstProfileParams(ir_method_t *method, ir_instruction_t *inst, ir_item_t *methodParam, ir_item_t *instParam) {
    XanList *params = newList();
    memset(methodParam, 0, sizeof(ir_item_t));
    memset(instParam, 0, sizeof(ir_item_t));
    xanList_append(params, methodParam);
    xanList_append(params, instParam);
    methodParam->value.v = ptrToInt(method);
    methodParam->type = IRNUINT;
    methodParam->internal_type = methodParam->type;
    instParam->value.v = ptrToInt(inst);
    instParam->type = IRNUINT;
    instParam->internal_type = instParam->type;
    return params;
}


/**
 * Add profiling code around a sequential segment to select the correct shadow
 * state structure when profiling data dependences.
 **/
static void
profileSequentialSegmentEntryAndExit(ir_method_t *parallelLoopMethod, sequential_segment_description_t *sequentialSegment) {
    ir_item_t methodParam;
    ir_item_t instParam;
    ir_item_t idParam;
    XanListItem *waitItem;
    XanListItem *signalItem;

    /* Set up the ID of the sequential segment. */
    memset(&idParam, 0, sizeof(ir_item_t));
    idParam.type = IRUINT32;
    idParam.internal_type = idParam.type;
    idParam.value.v = sequentialSegment->ID;
    PDEBUG(2, "Adding profile calls for sequential segment %u\n", sequentialSegment->ID);

    /* Debugging checks. */
    waitItem = xanList_first(sequentialSegment->waitInsns);
    while (waitItem) {
        ir_instruction_t *wait = waitItem->data;
        if (!IRMETHOD_doesInstructionBelongToMethod(parallelLoopMethod, wait)) {
            PDEBUG(2, "Wait instruction %d not part of the parallel loop\n", ptrToInt(xanHashTable_lookup(profileInfo->instToMethod, wait)));;
        }
        waitItem = waitItem->next;
    }
    signalItem = xanList_first(sequentialSegment->signalInsns);
    while (signalItem) {
        ir_instruction_t *signal = signalItem->data;
        if (!IRMETHOD_doesInstructionBelongToMethod(parallelLoopMethod, signal)) {
            PDEBUG(2, "Signal instruction %d not part of the parallel loop\n", ptrToInt(xanHashTable_lookup(profileInfo->instToMethod, signal)));
        }
        signalItem = signalItem->next;
    }

    /* After each wait. */
    waitItem = xanList_first(sequentialSegment->waitInsns);
    while (waitItem) {
        ir_instruction_t *wait = waitItem->data;
        if (IRMETHOD_doesInstructionBelongToMethod(parallelLoopMethod, wait)) {
            XanList *params = getCommonInstProfileParams(parallelLoopMethod, wait, &methodParam, &instParam);
            xanList_append(params, &idParam);
            IRMETHOD_newNativeCallInstructionAfter(parallelLoopMethod, wait, "ProfileWait", nativeWaitProfile, NULL, params);
            freeList(params);
        }
        waitItem = waitItem->next;
    }

    /* Before each signal. */
    signalItem = xanList_first(sequentialSegment->signalInsns);
    while (signalItem) {
        ir_instruction_t *signal = signalItem->data;
        if (IRMETHOD_doesInstructionBelongToMethod(parallelLoopMethod, signal)) {
            XanList *params = getCommonInstProfileParams(parallelLoopMethod, signal, &methodParam, &instParam);
            xanList_append(params, &idParam);
            IRMETHOD_newNativeCallInstructionBefore(parallelLoopMethod, signal, "ProfileSignal", nativeSignalProfile, NULL, params);
            freeList(params);
        }
        signalItem = signalItem->next;
    }
}


/**
 * Set up a list of parameters that are common to all profiling functions.
 **/
static XanList *
setupCommonProfileInstParams(ir_method_t *method, ir_instruction_t *inst) {
    ir_item_t *methodParam = newParam(ptrToInt(method), IRNUINT, IRNUINT);
    ir_item_t *instParam = newParam(ptrToInt(inst), IRNUINT, IRNUINT);
    XanList *params = newList();
    xanList_append(params, methodParam);
    xanList_append(params, instParam);
    return params;
}


/**
 * Add profiling code to a load or store instruction.
 **/
static void
profileLoadOrStore(ir_method_t *method, ir_instruction_t *inst, void *nativeMethod) {
    XanList *params;
    ir_item_t *param1;
    ir_item_t *param2;
    ir_item_t *addrParam;
    ir_item_t *sizeParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require two additional parameters. */
    addrParam = newParamEmpty();
    sizeParam = newParamEmpty();
    xanList_append(params, addrParam);
    xanList_append(params, sizeParam);

    /* Original instruction parameters. */
    param1 = IRMETHOD_getInstructionParameter1(inst);
    param2 = IRMETHOD_getInstructionParameter2(inst);

    /* Number of bytes accessed. */
    sizeParam->type = IRUINT32;
    sizeParam->internal_type = sizeParam->type;
    if (inst->type == IRLOADREL) {
        sizeParam->value.v = IRDATA_getSizeOfType(IRMETHOD_getInstructionResult(inst));
    } else {
        sizeParam->value.v = IRDATA_getSizeOfType(IRMETHOD_getInstructionParameter3(inst));
    }

    /* Work out the address accessed. */
    if (param2->value.v == 0) {
        memcpy(addrParam, param1, sizeof(ir_item_t));
    } else {
        ir_instruction_t *add = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRADD);
        ir_item_t *addRes = IRMETHOD_getInstructionResult(add);
        IRMETHOD_cpInstructionParameter1(add, param1);
        IRMETHOD_cpInstructionParameter2(add, param2);
        IRMETHOD_setInstructionParameterWithANewVariable(method, add, param1->internal_type, param1->type_infos, 0);
        memcpy(addrParam, addRes, sizeof(ir_item_t));
    }

    /* Add the new instruction. */
    if (inst->type == IRLOADREL) {
        IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileLoad", nativeMethod, NULL, params);
    } else {
        IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileStore", nativeMethod, NULL, params);
    }
    freeParamList(params);
}


/**
 * Add profiling code to a regular call instruction.  Since all reachable
 * methods are already profiled, this only inserts a profiling call if the
 * target method is a library call.
 **/
static void
profileDirectCall(ir_method_t *method, ir_instruction_t *inst) {
    ir_method_t *callee;
    assert(inst->type == IRCALL);
    callee = IRMETHOD_getCalledMethod(method, inst);
    if (IRPROGRAM_doesMethodBelongToALibrary(callee)) {
        XanList *params = setupCommonProfileInstParams(method, inst);
        ir_item_t *calleeParam = newParam(ptrToInt(callee), IRNUINT, IRNUINT);
        xanList_append(params, calleeParam);
        IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileDirectCall", nativeDirectCallProfile, NULL, params);
        freeParamList(params);
    }
}


/**
 * Add profiling code to an indirect call instruction.  The profiling code has
 * to extact the actual method called, see whether it has been profiled already
 * and insert profiling code if not.  Or, if it's a library call, deal with it
 * appropriately.
 **/
static void
profileIndirectCall(ir_method_t *method, ir_instruction_t *inst) {
}


/**
 * Add profiling code to a library call.  The profiling code marks all memory
 * as modified and creates dependences with all writes from prior iterations.
 **/
static void
profileLibraryCall(ir_method_t *method, ir_instruction_t *inst) {
    ir_method_t *callee;
    ir_item_t *calleeParam;
    XanList *params = setupCommonProfileInstParams(method, inst);
    callee = IRMETHOD_getCalledMethod(method, inst);
    calleeParam = newParam(ptrToInt(callee), IRNUINT, IRNUINT);
    xanList_append(params, calleeParam);
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileLibraryCall", nativeLibraryCallProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a memory allocation instruction.
 **/
static void
profileAllocInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *param1;
    ir_item_t *addrParam;
    ir_item_t *sizeParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require two additional parameters. */
    addrParam = newParamEmpty();
    sizeParam = newParamEmpty();
    xanList_append(params, addrParam);
    xanList_append(params, sizeParam);

    /* Original instruction parameters. */
    param1 = IRMETHOD_getInstructionParameter1(inst);

    /* Address and number of bytes accessed. */
    memcpy(addrParam, IRMETHOD_getInstructionResult(inst), sizeof(ir_item_t));
    switch (inst->type) {
        case IRNEWOBJ: {
            ir_item_t *param2 = IRMETHOD_getInstructionParameter2(inst);
            sizeParam->type = IRUINT32;
            sizeParam->internal_type = sizeParam->type;
            sizeParam->value.v = IRDATA_getSizeOfType(param1) + param2->value.v;
            break;
        }
        case IRNEWARR:
        case IRCALLOC: {
            ir_instruction_t *mul = IRMETHOD_newInstructionOfTypeBefore(method, inst, IRMUL);
            ir_item_t *mulRes = IRMETHOD_getInstructionResult(mul);
            ir_item_t *mulParam1 = IRMETHOD_getInstructionParameter1(mul);
            ir_item_t *param2 = IRMETHOD_getInstructionParameter2(inst);
            if (inst->type == IRNEWARR) {
                mulParam1->type = IRUINT32;
                mulParam1->internal_type = mulParam1->type;
                mulParam1->value.v = IRDATA_getSizeOfType(param1);
            } else {
                IRMETHOD_cpInstructionParameter1(mul, param1);
            }
            IRMETHOD_cpInstructionParameter2(mul, param2);
            IRMETHOD_setInstructionParameterWithANewVariable(method, mul, param2->internal_type, param2->type_infos, 0);
            memcpy(sizeParam, mulRes, sizeof(ir_item_t));
            break;
        }
        case IRALLOCA:
        case IRALLOC:
        case IRALLOCALIGN:
            memcpy(sizeParam, param1, sizeof(ir_item_t));
            break;
        default:
            PDEBUG(0, "Instruction type is %d\n", inst->type);
            fflush(stderr);
            abort();
    }

    /* Add the new instruction afterwards to get the address allocated. */
    IRMETHOD_newNativeCallInstructionAfter(method, inst, "ProfileAlloc", nativeMemAllocProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a memory reallocation instruction.
 **/
static void
profileReallocInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *addrParam;
    ir_item_t *sizeParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* A call to free this allocation. */
    addrParam = newParamEmpty();
    xanList_append(params, addrParam);
    memcpy(addrParam, IRMETHOD_getInstructionParameter1(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileFree", nativeMemFreeProfile, NULL, params);

    /* A call to create the new allocation. */
    sizeParam = newParamEmpty();
    xanList_append(params, sizeParam);
    memcpy(addrParam, IRMETHOD_getInstructionResult(inst), sizeof(ir_item_t));
    memcpy(sizeParam, IRMETHOD_getInstructionParameter2(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionAfter(method, inst, "ProfileAlloc", nativeMemAllocProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a memory free instruction.
 **/
static void
profileFreeInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *addrParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require one additional parameter. */
    addrParam = newParamEmpty();
    xanList_append(params, addrParam);
    memcpy(addrParam, IRMETHOD_getInstructionParameter1(inst), sizeof(ir_item_t));

    /* Add the new instruction. */
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileFree", nativeMemFreeProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a memory initialisation instruction.
 **/
static void
profileMemInitInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *addrParam;
    ir_item_t *sizeParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require three additional parameters. */
    addrParam = newParamEmpty();
    sizeParam = newParamEmpty();
    xanList_append(params, addrParam);
    xanList_append(params, sizeParam);

    /* Address and number of bytes accessed. */
    memcpy(addrParam, IRMETHOD_getInstructionParameter1(inst), sizeof(ir_item_t));
    memcpy(sizeParam, IRMETHOD_getInstructionParameter3(inst), sizeof(ir_item_t));

    /* Add the new instruction. */
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileMemInit", nativeMemWriteProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a memcpy instruction.
 **/
static void
profileMemCopyInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *addrParam;
    ir_item_t *sizeParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require two additional parameters. */
    addrParam = newParamEmpty();
    sizeParam = newParamEmpty();
    xanList_append(params, addrParam);
    xanList_append(params, sizeParam);

    /* Profile the addresses read. */
    memcpy(addrParam, IRMETHOD_getInstructionParameter2(inst), sizeof(ir_item_t));
    memcpy(sizeParam, IRMETHOD_getInstructionParameter3(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileMemcpy", nativeMemReadProfile, NULL, params);

    /* Profile the addresses written. */
    memcpy(addrParam, IRMETHOD_getInstructionParameter1(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileMemcpy", nativeMemWriteProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a memcpy instruction.
 **/
static void
profileMemCompareInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *addrParam;
    ir_item_t *sizeParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require two additional parameters. */
    addrParam = newParamEmpty();
    sizeParam = newParamEmpty();
    xanList_append(params, addrParam);
    xanList_append(params, sizeParam);

    /* Profile the first addresses read. */
    memcpy(addrParam, IRMETHOD_getInstructionParameter1(inst), sizeof(ir_item_t));
    memcpy(sizeParam, IRMETHOD_getInstructionParameter3(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileMemcpy", nativeMemReadProfile, NULL, params);

    /* Profile the second addresses read. */
    memcpy(addrParam, IRMETHOD_getInstructionParameter2(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileMemcpy", nativeMemReadProfile, NULL, params);
    freeParamList(params);
}


/**
 * Add profile calls to a string read instruction.
 **/
static void
profileStringReadInst(ir_method_t *method, ir_instruction_t *inst) {
    XanList *params;
    ir_item_t *addrParam;

    /* Set up the common parameters. */
    params = setupCommonProfileInstParams(method, inst);

    /* Require one additional parameter. */
    addrParam = newParamEmpty();
    xanList_append(params, addrParam);

    /* Profile the first string read. */
    memcpy(addrParam, IRMETHOD_getInstructionParameter1(inst), sizeof(ir_item_t));
    IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileStringRead", nativeStringReadProfile, NULL, params);

    /* Profile the second string read. */
    if (inst->type == IRSTRINGCMP) {
        memcpy(addrParam, IRMETHOD_getInstructionParameter2(inst), sizeof(ir_item_t));
        IRMETHOD_newNativeCallInstructionBefore(method, inst, "ProfileStringRead", nativeStringReadProfile, NULL, params);
    }
    freeParamList(params);
}


/**
 * Add profiling code to a list of instructions.  Before each instruction of
 * interest (i.e. memory instructions) insert a native call back to record the
 * address of memory being accessed and the number of bytes.
 **/
static void
profileInstructionList(ir_method_t *method, XanList *instructions) {
    XanListItem *instItem = xanList_first(instructions);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        switch (inst->type) {
            case IRLOADREL:
                profileLoadOrStore(method, inst, nativeMemReadProfile);
                break;
            case IRSTOREREL:
                profileLoadOrStore(method, inst, nativeMemWriteProfile);
                break;
            case IRCALL:
                profileDirectCall(method, inst);
                break;
            case IRICALL:
            case IRVCALL:
                fprintf(stderr, "Not profiling indirect calls yet.\n");
                profileIndirectCall(method, inst);
                break;
            case IRLIBRARYCALL:
            case IRNATIVECALL:
                profileLibraryCall(method, inst);
                break;
            case IRNEWOBJ:
            case IRNEWARR:
            case IRALLOCA:
            case IRALLOC:
            case IRALLOCALIGN:
            case IRCALLOC:
                profileAllocInst(method, inst);
                break;
            case IRREALLOC:
                profileReallocInst(method, inst);
                break;
            case IRFREEOBJ:
            case IRFREE:
                profileFreeInst(method, inst);
                break;
            case IRINITMEMORY:
                profileMemInitInst(method, inst);
                break;
            case IRMEMCPY:
                profileMemCopyInst(method, inst);
                break;
            case IRMEMCMP:
                profileMemCompareInst(method, inst);
                break;
            case IRSTRINGLENGTH:
            case IRSTRINGCMP:
            case IRSTRINGCHR:
                profileStringReadInst(method, inst);
                break;
        }
        instItem = instItem->next;
    }
}


/**
 * Add profiling code to all instructions within a method that will determine
 * the inter-iteration data dependences that exist within any loop that calls
 * the method.
 **/
static void
profileMethodForDataDependences(ir_method_t *method) {
    XanList *instructions = IRMETHOD_getInstructions(method);
    profileInstructionList(method, instructions);
    freeList(instructions);
}


/**
 * Push all unseen successor instructions onto a stack.
 **/
static void
pushUnseenSuccessorsOntoStack(ir_method_t *method, ir_instruction_t *inst, XanBitSet *seenInsts, XanStack *stack) {
    ir_instruction_t *succ = NULL;
    while ((succ = IRMETHOD_getSuccessorInstruction(method, inst, succ)) != NULL) {
        if (!xanBitSet_isBitSet(seenInsts, succ->ID) && succ->type != IREXITNODE) {
            /* PDEBUG(2, "Pushing successor %u\n", succ->ID); */
            xanStack_push(stack, succ);
        }
    }
}


/**
 * Get a list of instructions starting from a single instruction and continuing
 * until all unseen successors have been processed.
 **/
static XanList *
getListOfUnseenSuccessors(ir_method_t *method, ir_instruction_t *start, XanBitSet *seenInsts) {
    XanList *unseenSuccs = newList();
    XanStack *toProcess = xanStack_new(allocFunction, freeFunction, NULL);
    xanBitSet_setBit(seenInsts, start->ID);
    pushUnseenSuccessorsOntoStack(method, start, seenInsts, toProcess);
    while (xanStack_getSize(toProcess) > 0) {
        ir_instruction_t *inst = xanStack_pop(toProcess);
        xanList_append(unseenSuccs, inst);
        xanBitSet_setBit(seenInsts, inst->ID);
        pushUnseenSuccessorsOntoStack(method, inst, seenInsts, toProcess);
    }
    xanStack_destroyStack(toProcess);
    return unseenSuccs;
}


/**
 * Get a list of instructions contained within a sequentail segment, all
 * belonging to the same method.
 **/
static XanList *
getListOfSequentialSegmentInstructions(ir_method_t *method, sequential_segment_description_t *seqSeg) {
    JITUINT32 numInsts = IRMETHOD_getInstructionsNumber(method);
    XanBitSet *seenInsts = xanBitSet_new(numInsts);
    XanList *segmentInstructions = newList();
    XanListItem *instItem;

    /* Mark signal instructions as seen. */
    instItem = xanList_first(seqSeg->signalInsns);
    while (instItem) {
        ir_instruction_t *signal = instItem->data;
        if (IRMETHOD_doesInstructionBelongToMethod(method, signal)) {
            PDEBUG(3, "Signal instruction %u\n", signal->ID);
            xanBitSet_setBit(seenInsts, signal->ID);
        }
        instItem = instItem->next;
    }

    /* Get a list of instructions after waits but before signals. */
    instItem = xanList_first(seqSeg->waitInsns);
    while (instItem) {
        ir_instruction_t *wait = instItem->data;
        if (IRMETHOD_doesInstructionBelongToMethod(method, wait)) {
            XanList *fromWait = getListOfUnseenSuccessors(method, wait, seenInsts);
            xanList_appendList(segmentInstructions, fromWait);
            freeList(fromWait);
            PDEBUG(3, "Wait instruction %u\n", wait->ID);
        }
        instItem = instItem->next;
    }

    /* Clean up. */
    xanBitSet_free(seenInsts);
    return segmentInstructions;
}


/**
 * Add profiling code to a single loop that will determine the inter-iteration
 * data dependences that exist within the loop's sequential segments.
 **/
static void
profileLoopForDataDependences(ir_method_t *parallelLoopMethod, loop_t *loop, ir_instruction_t *loopEntryPoint, XanList *sequentialSegments) {
    XanListItem *seqSegItem;

    /* Add code to differentiate between loop invocations. */
    profileLoopEntryAndExits(parallelLoopMethod, loop, loopEntryPoint);

    /* Profile instructions within the sequential segments. */
    seqSegItem = xanList_first(sequentialSegments);
    PDEBUG(1, "Adding profile information for %d sequential segments\n", xanList_length(sequentialSegments));
    while (seqSegItem) {
        sequential_segment_description_t *seqSeg = seqSegItem->data;
        XanList *segmentInsts;

        /* Profile loads and stores in sequential segments. */
        segmentInsts = getListOfSequentialSegmentInstructions(parallelLoopMethod, seqSeg);
        profileInstructionList(parallelLoopMethod, segmentInsts);
        freeList(segmentInsts);
        seqSegItem = seqSegItem->next;
    }

    /* Add code to start and end the seqential segment. */
    seqSegItem = xanList_first(sequentialSegments);
    while (seqSegItem) {
        profileSequentialSegmentEntryAndExit(parallelLoopMethod, seqSegItem->data);
        seqSegItem = seqSegItem->next;
    }
}


/**
 * Add methods called by a list of instructions to another list.
 **/
static void
appendDirectCalledMethodsToList(ir_method_t *method, XanList *instructions, XanList *methodList) {
    XanListItem *instItem = xanList_first(instructions);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        if (inst->type == IRCALL) {
            ir_method_t *callee = IRMETHOD_getCalledMethod(method, inst);
            xanList_append(methodList, callee);
        }
        instItem = instItem->next;
    }
}


/**
 * Add methods called within a sequential segment to a list.
 **/
static void
appendSequentialSegmentCalledMethodsToList(ir_method_t *method, sequential_segment_description_t *seqSeg, XanList *methodList) {
    XanList *instsToCheck = getListOfSequentialSegmentInstructions(method, seqSeg);
    appendDirectCalledMethodsToList(method, instsToCheck, methodList);
    freeList(instsToCheck);
}


/**
 * Create a hash table of methods to profile entirely from the loop that needs
 * profiling.  The method that contains the loop is not placed in this table
 * unless it is reachable call instructions.
 **/
static XanHashTable *
createTableOfMethodsToProfile(ir_method_t *parallelMethod, XanList *sequentialSegments) {
    XanList *methodsToProcess = newList();
    XanHashTable *methodsToProfile = newHashTable();
    XanListItem *seqSegItem = xanList_first(sequentialSegments);
    XanListItem *methodItem;

    /* Create an initial list of methods to process. */
    while (seqSegItem) {
        appendDirectCalledMethodsToList(parallelMethod, seqSegItem->data, methodsToProcess);
        seqSegItem = seqSegItem->next;
    }

    /* Populate the table. */
    methodItem = xanList_first(methodsToProcess);
    while (methodItem) {
        ir_method_t *method = methodItem->data;
        if (!xanHashTable_lookup(methodsToProfile, method)) {
            XanList *instructions = IRMETHOD_getInstructions(method);
            xanHashTable_insert(methodsToProfile, method, method);
            appendDirectCalledMethodsToList(method, instructions, methodsToProcess);
        }
        methodItem = methodItem->next;
    }

    /* Clean up. */
    freeList(methodsToProcess);
    return methodsToProfile;
}


/**
 * Store information about a list of instructions to profile in global
 * structures.  We need to record the method that each instruction belongs to
 * as well as its original instruction ID.
 **/
static void
recordInstructionListInformation(ir_method_t *method, XanList *instructions) {
    XanListItem *instItem = xanList_first(instructions);
    while (instItem) {
        ir_instruction_t *inst = instItem->data;
        xanHashTable_insert(profileInfo->instToMethod, inst, method);
        xanHashTable_insert(profileInfo->instToOrigID, inst, uintToPtr(inst->ID));
        instItem = instItem->next;
    }
}


/**
 * Store information a method to profile in the global structures.  We need to
 * record the method that each instruction belongs to as well as its original
 * instruction ID.
 **/
static void
recordMethodInformation(ir_method_t *method) {
    XanList *instructions = IRMETHOD_getInstructions(method);
    recordInstructionListInformation(method, instructions);
    freeList(instructions);
}


/**
 * Get the parallel version of a loop from the method executed by the parallel
 * threads.
 **/
static loop_t *
getParallelLoop(ir_method_t *parallelLoopMethod) {
    XanList *outerLoops;
    loop_t *parallelLoop;
    if (profileInfo->allLoops != NULL) {
        IRLOOP_destroyLoops(profileInfo->allLoops);
    }
    profileInfo->allLoops = IRLOOP_analyzeMethodLoops(parallelLoopMethod, LOOPS_analyzeCircuits);
    outerLoops = LOOPS_getOutermostProgramLoops(profileInfo->allLoops);
    assert(xanList_length(outerLoops) == 1);
    parallelLoop = xanList_first(outerLoops)->data;
    freeList(outerLoops);
    return parallelLoop;
}


/**
 * Profile a loop so that data dependences across loop iterations are
 * identified.  This means inserting code around each memory operation that
 * records the iteration that the operation executes on.
 **/
void
LOOPS_profileLoopForDataDependences(ir_optimizer_t *irOptimizer, ir_method_t *parallelLoopMethod, ir_instruction_t *loopEntryPoint, JITINT8 *loopName, XanList *sequentialSegments) {
    char *prevName;
    loop_t *parallelLoop;
    XanHashTableItem *methodItem;

    /* Debugging output. */
    PDEBUG(0, "Profiling loop for data dependences for loop %s\n", loopName);

    /* Early exit. */
    if (xanList_length(sequentialSegments) == 0) {
        PDEBUG(0, "  No sequential segments, no profiling required\n");
        return;
    }

    /* Set up output. */
    prevName = getenv("DOTPRINTER_FILENAME");
    if (prevName == NULL) {
        prevName = "";
    }
    setenv("DOTPRINTER_FILENAME", "ddprof", 1);
    IROPTIMIZER_callMethodOptimization(irOptimizer, parallelLoopMethod, METHOD_PRINTER);

    /* Initialise global constants from environment variables. */
    initGlobals(irOptimizer);

    /* Get the parallelised loop. */
    parallelLoop = getParallelLoop(parallelLoopMethod);
    IROPTIMIZER_callMethodOptimization(irOptimizer, parallelLoopMethod, METHOD_PRINTER);
    assert(parallelLoop->method == parallelLoopMethod);

    /* Construct a list of methods to fully profile. */
    profileInfo->profiledMethods = createTableOfMethodsToProfile(parallelLoopMethod, sequentialSegments);

    /* Fully profile all methods. */
    methodItem = xanHashTable_first(profileInfo->profiledMethods);
    while (methodItem) {
        ir_method_t *method = methodItem->elementID;
        assert(method != parallelLoopMethod);
        recordMethodInformation(method);
        profileMethodForDataDependences(method);
        methodItem = xanHashTable_next(profileInfo->profiledMethods, methodItem);
    }

    /* Profile the loop and sequential segments. */
    recordMethodInformation(parallelLoopMethod);
    profileLoopForDataDependences(parallelLoopMethod, parallelLoop, loopEntryPoint, sequentialSegments);
    IROPTIMIZER_callMethodOptimization(irOptimizer, parallelLoopMethod, METHOD_PRINTER);

    /* Reset the old output filename. */
    setenv("DOTPRINTER_FILENAME", prevName, 1);
    PDEBUG(0, "Finished inserting profiling code\n");
}
