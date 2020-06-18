#include <assert.h>
#include <errno.h>
#include <math.h>
#include <jitsystem.h>
#include <iljit-utils.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <xanlib.h>

//My headers
#include <optimizer_availableexpressions.h>
#include <config.h>
//End

#define INFORMATIONS "This step computes the available expressions at each statement"
#define AUTHOR "Simone Campanoni and Timothy M. Jones"

JITUINT64 get_ID_job (void);
char * get_version (void);
static inline void ae_do_job (ir_method_t *method);
char * get_informations (void);
static inline char * cse_get_author (void);
JITUINT64 get_dependences (void);
void ae_shutdown (JITFLOAT32 totalTime);
void ae_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 ae_get_invalidations (void);
static inline JITBOOLEAN is_a_not_memory_expression (ir_instruction_t *inst);
static inline JITBOOLEAN is_an_expression (ir_instruction_t *inst);
static inline void ae_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void ae_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    ae_init,
    ae_shutdown,
    ae_do_job,
    get_version,
    get_informations,
    cse_get_author,
    ae_get_invalidations,
    NULL,
    ae_getCompilationFlags,
    ae_getCompilationTime
};

static inline JITUINT64 ae_get_invalidations (void) {
    return INVALIDATE_NONE;
}


/**
 * Required for debug printing.
 **/
#if defined(PRINTDEBUG)
JITINT32 debugLevel = -1;
#endif


/**
 * A structure to hold lists of instructions that use each variable and
 * constant.  This is used when initialising the available expression kill
 * and gen sets for each instruction.  Since variables are known initially,
 * there is one slot for each variable in the method.  Constants are added
 * as found, so their actual values need tracking too.
 * numVars - The total number of variables being tracked.
 * numConsts - Tht total number of constants being tracked.
 * allocatedConsts - Tht total number of slots allocated for tracking constants.
 * consts - Each constant that is being tracked.
 * varsArith - Lists of arithmetic instructions that use each variable.
 * varsLoad - Lists of load instructions that use each variable.
 * varsStore - Lists of store (to memory) instructions that use each variable.
 * constsLoad - Lists of load instructions that use each constant.
 * constsStore - Lists of store (to memory) instructions that use each constant.
 */
typedef struct {
    JITUINT32 numVars;
    JITUINT32 numConsts;
    JITUINT32 allocatedConsts;
    JITUINT64* consts;
    XanList **varsArith;
    XanList **varsLoad;
    XanList **varsStore;
    XanList **constsLoad;
    XanList **constsStore;
} var_consts_t;


/**
 * All information that is cached by this pass.
 **/
typedef struct {
    JITUINT32 numInsts;
    JITUINT32 numMethodParams;
    ir_method_t *method;
    ir_instruction_t **insns;
    XanList **predecessors;
    XanBitSet **matchingExprs;
    XanBitSet **genSets;
    XanBitSet **invKillSets;
    XanBitSet **inSets;
    XanBitSet **outSets;
} pass_info_t;


#define CONSTANTS_CHUNK_SIZE 32

#define MAX_INST_TYPE (IREXITNODE + 1)

#define uintToPtr(i) ((void *)(JITNUINT)(i))


/**
 * Add an instruction to a list of instructions, if not already there.
 */
static void
addInstToList (XanList *instList, ir_instruction_t *inst) {
    if (!xanList_find(instList, inst)) {
        xanList_append(instList, inst);
    }
}


/**
 * Determine whether a constant value is being tracked already and, if so,
 * return the index that it is at.  If it is not being tracked, then allocate
 * more memory for it and return its index.
 */
static JITUINT32
findConstIndex (JITUINT64 constVal, var_consts_t *varConsts) {
    JITUINT32 v, index = 0;
    JITBOOLEAN tracked = JITFALSE;

    /* Tim: Check whether this constant is already tracked. */
    for (v = 0; v < varConsts->numConsts && !tracked; ++v) {
        if (varConsts->consts[v] == constVal) {
            index = v;
            tracked = JITTRUE;
        }
    }

    /* Tim: If it's not tracked then add it. */
    if (!tracked) {

        /* Tim: We may need more memory. */
        if ((varConsts->numConsts % CONSTANTS_CHUNK_SIZE) == 0) {
            JITUINT32 c, oldSize, newSize;
            JITUINT64 *newConsts;
            XanList **newConstsLoad;
            XanList **newConstsStore;

            /* Tim: Allocate new memory. */
            oldSize = varConsts->allocatedConsts;
            newSize = oldSize + CONSTANTS_CHUNK_SIZE;
            newConsts = dynamicReallocFunction(varConsts->consts, newSize * sizeof(JITUINT64));
            newConstsLoad = dynamicReallocFunction(varConsts->constsLoad, newSize * sizeof(XanList*));
            newConstsStore = dynamicReallocFunction(varConsts->constsStore, newSize * sizeof(XanList*));
            for (c = oldSize; c < newSize; ++c) {
                newConstsLoad[c] = xanList_new(allocFunction, freeFunction, NULL);
                newConstsStore[c] = xanList_new(allocFunction, freeFunction, NULL);
            }

            /* Tim: Reset pointers. */
            varConsts->allocatedConsts	= newSize;
            varConsts->consts 		= newConsts;
            varConsts->constsLoad 		= newConstsLoad;
            varConsts->constsStore 		= newConstsStore;
        }

        /* Tim: Take the next available index for this constant. */
        index = varConsts->numConsts;
        varConsts->numConsts += 1;
        varConsts->consts[index] = constVal;
    }

    /* Return the index used. */
    return index;
}


/**
 * Add a load from a constant base address to its list in the structure
 * of variable and constant using instructions.  Because we don't know in
 * advance how many constants are needed, we must allocate memory for the
 * lists on-demand and increase it when necessary.
 */
static void
addLoadConst (ir_instruction_t *inst, JITUINT64 constVal,
              var_consts_t *varConsts) {
    JITUINT32 index = 0;

    /* Tim: Add a new instruction to the list. */
    index = findConstIndex(constVal, varConsts);
    addInstToList(varConsts->constsLoad[index], inst);
}


/**
 * Add a store from a constant base address to its list in the structure
 * of variable and constant using instructions.  Because we don't know in
 * advance how many constants are needed, we must allocate memory for the
 * lists on-demand and increase it when necessary.
 */
static void
addStoreConst (ir_instruction_t *inst, JITUINT64 constVal,
               var_consts_t *varConsts) {
    JITUINT32 index = 0;

    /* Tim: Add a new instruction to the list. */
    index = findConstIndex(constVal, varConsts);
    addInstToList(varConsts->constsStore[index], inst);
}


/**
 * Add an arithmetic or logic instruction to the instruction list for each
 * variable it uses.
 */
static void
addArithLogicInst (ir_method_t *method, ir_instruction_t *inst,
                   var_consts_t *varConsts) {

    /* Deal with the first parameter. */
    if (IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter1Value(inst);
        addInstToList(varConsts->varsArith[varId], inst);
    }

    /* Deal with the second parameter, if there. */
    if (IRMETHOD_getInstructionParametersNumber(inst) > 1 &&
            IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter2Value(inst);
        addInstToList(varConsts->varsArith[varId], inst);
    }
}


/**
 * Add a load instruction to the instruction list for each variable and
 * constant that it uses.  We must track constants from the first parameter
 * since stores to memory kill all loads to constants (until we can add in
 * better analysis to determine which ones they should kill).
 */
static void
addLoadInst (ir_method_t *method, ir_instruction_t *inst,
             var_consts_t *varConsts) {

    /* Deal with the first parameter. */
    if (IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter1Value(inst);
        addInstToList(varConsts->varsLoad[varId], inst);
    } else {
        JITUINT64 constVal = IRMETHOD_getInstructionParameter1Value(inst);
        addLoadConst(inst, constVal, varConsts);
    }

    /* Deal with the second parameter. */
    if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter2Value(inst);
        addInstToList(varConsts->varsLoad[varId], inst);
    }
}


/**
 * Add a store instruction to the instruction list for each variable and
 * constant that it uses.  We must track constants from the first parameter
 * since stores to memory kill all stores to constants (until we can add in
 * better analysis to determine which ones they should kill).
 */
static void
addStoreInst (ir_method_t *method, ir_instruction_t *inst,
              var_consts_t *varConsts) {

    /* Deal with the first parameter. */
    if (IRMETHOD_getInstructionParameter1Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter1Value(inst);
        addInstToList(varConsts->varsStore[varId], inst);
    } else {
        JITUINT64 constVal = IRMETHOD_getInstructionParameter1Value(inst);
        addStoreConst(inst, constVal, varConsts);
    }

    /* Deal with the second parameter. */
    if (IRMETHOD_getInstructionParameter2Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter2Value(inst);
        addInstToList(varConsts->varsStore[varId], inst);
    }

    /* Deal with the third parameter. */
    if (IRMETHOD_getInstructionParameter3Type(inst) == IROFFSET) {
        JITUINT64 varId = IRMETHOD_getInstructionParameter3Value(inst);
        addInstToList(varConsts->varsStore[varId], inst);
    }
}


/**
 * Initialise the lists of variable and constant using instructions.
 */
static var_consts_t *
initVarConsts (pass_info_t *info) {
    JITUINT32 instPos;
    JITUINT64 var;
    var_consts_t *varConsts;

    /* Allocate memory. */
    varConsts = allocFunction(sizeof(var_consts_t));
    varConsts->numVars = IRMETHOD_getNumberOfVariablesNeededByMethod(info->method);
    varConsts->varsArith = allocFunction((varConsts->numVars + 1) * sizeof(XanList *));
    varConsts->varsLoad = allocFunction((varConsts->numVars + 1) * sizeof(XanList *));
    varConsts->varsStore = allocFunction((varConsts->numVars + 1) * sizeof(XanList *));

    /* Initialise the lists. */
    for (var = 0; var < varConsts->numVars; ++var) {
        varConsts->varsArith[var] = xanList_new(allocFunction, freeFunction, NULL);
        varConsts->varsLoad[var] = xanList_new(allocFunction, freeFunction, NULL);
        varConsts->varsStore[var] = xanList_new(allocFunction, freeFunction, NULL);
    }

    /* Fill the expressions sets for each variable. */
    for (instPos = info->numMethodParams; instPos < info->numInsts; ++instPos) {
        ir_instruction_t *inst = info->insns[instPos];
        assert(inst);

        /* Action depends on instruction type. */
        if (is_a_not_memory_expression(inst)) {
            switch (inst->type) {
                case IRGETADDRESS:
                    break;
                default :
                    addArithLogicInst(info->method, inst, varConsts);
            }
        } else {
            switch (inst->type) {
                case IRLOADREL:
                    addLoadInst(info->method, inst, varConsts);
                    break;
                case IRSTOREREL:
                    addStoreInst(info->method, inst, varConsts);
                    break;
            }
        }
    }

    /* Return the created varConsts structure. */
    return varConsts;
}


/**
 * Check whether two parameters are different.
 **/
static inline JITBOOLEAN
areParametersDifferent(ir_item_t *par1, ir_item_t *par2) {
    if (par1->type != par2->type) {
        return JITTRUE;
    }
    if ((par1->value).v != (par2->value).v) {
        return JITTRUE;
    }
    if ((par1->value).f != (par2->value).f) {
        return JITTRUE;
    }
    return JITFALSE;
}


/**
 * Check whether two expressions match.  This means that the instructions'
 * types and parameters are exactly the same, although their return parameters
 * can be different.
 */
static JITBOOLEAN
exprsMatch(ir_method_t *method, ir_instruction_t *i1, ir_instruction_t *i2) {
    JITUINT32	parNum1;
    JITUINT32	parNum2;
    ir_item_t	*i1Par;
    ir_item_t	*i2Par;

    /* Assertions.
     */
    assert(i1 != NULL);
    assert(i2 != NULL);

    /* Check trivial cases.
     */
    if (i1 == i2) {

        /* The instructions are the same.
         * Hence, they compute the same expression.
         */
        return JITTRUE;
    }
    assert(i1 != i2);
    if (i1->type != i2->type) {

        /* Types don't match.
         */
        return JITFALSE;
    }
    parNum1	= IRMETHOD_getInstructionParametersNumber(i1);
    if (parNum1 == 0) {

        /* There is no expression.
         */
        return JITFALSE;
    }
    parNum2	= IRMETHOD_getInstructionParametersNumber(i2);
    if (parNum1 != parNum2) {

        /* Different numbers of parameters.
         */
        return JITFALSE;
    }
    assert(i1 != i2);
    assert(i1->type == i2->type);
    assert(parNum1 > 0);
    assert(parNum1 == parNum2);

    /* Each parameter must match.
     */
    switch (parNum1) {
        case 3:
            i1Par	= IRMETHOD_getInstructionParameter3(i1);
            i2Par	= IRMETHOD_getInstructionParameter3(i2);
            if (areParametersDifferent(i1Par, i2Par)) {
                return JITFALSE;
            }

            /* Intentional fall-through. */
        case 2:
            i1Par	= IRMETHOD_getInstructionParameter2(i1);
            i2Par	= IRMETHOD_getInstructionParameter2(i2);
            if (areParametersDifferent(i1Par, i2Par)) {
                return JITFALSE;
            }

            /* Intentional fall-through. */
        case 1:
            i1Par	= IRMETHOD_getInstructionParameter1(i1);
            i2Par	= IRMETHOD_getInstructionParameter1(i2);
            if (areParametersDifferent(i1Par, i2Par)) {
                return JITFALSE;
            }
    }

    /* Type and all parameters match. */
    return JITTRUE;
}

/**
 * Initialise a table of matching expressions.
 **/
static XanBitSet **
initMatchingExprs(pass_info_t *info) {
    JITUINT32 i, j;
    XanBitSet **matchingExprs = allocFunction(info->numInsts * sizeof(XanBitSet *));
    for (i = 0; i < info->numInsts; ++i) {
        matchingExprs[i] = xanBitSet_new(info->numInsts);
    }
    for (i = 0; i < info->numInsts; ++i) {
        ir_instruction_t *currInst = info->insns[i];
        for (j = i; j < info->numInsts; ++j) {
            ir_instruction_t *checkInst = info->insns[j];
            if (exprsMatch(info->method, currInst, checkInst)) {
                xanBitSet_setBit(matchingExprs[i], j);
                if (currInst != checkInst) {
                    xanBitSet_setBit(matchingExprs[j], i);
                }
            }
        }
    }
    return matchingExprs;
}


/**
 * Initialise a pass information structure.
 **/
static pass_info_t *
initPassInfo(ir_method_t *method) {
    JITINT32 i;
    pass_info_t *info = allocFunction(sizeof(pass_info_t));
    info->method = method;
    info->numInsts = IRMETHOD_getInstructionsNumber(method);
    info->numMethodParams = IRMETHOD_getMethodParametersNumber(method);
    info->insns = IRMETHOD_getInstructionsWithPositions(method);
    info->predecessors = IRMETHOD_getInstructionsPredecessors(method);
    info->matchingExprs = initMatchingExprs(info);
    info->genSets = allocFunction(info->numInsts * sizeof(XanBitSet *));
    info->invKillSets = allocFunction(info->numInsts * sizeof(XanBitSet *));
    info->inSets = allocFunction(info->numInsts * sizeof(XanBitSet *));
    info->outSets = allocFunction(info->numInsts * sizeof(XanBitSet *));
    for (i = 0; i < info->numInsts; ++i) {
        info->genSets[i] = xanBitSet_new(info->numInsts);
        info->invKillSets[i] = xanBitSet_new(info->numInsts);
        info->inSets[i] = xanBitSet_new(info->numInsts);
        info->outSets[i] = xanBitSet_new(info->numInsts);
    }
    return info;
}


/**
 * Destroy the lists of instructions using variables and constants.
 */
static void
destroyVarConsts (var_consts_t *varConsts) {
    JITUINT32 i;

    for (i = 0; i < varConsts->numVars; ++i) {
        xanList_destroyList(varConsts->varsArith[i]);
        xanList_destroyList(varConsts->varsLoad[i]);
        xanList_destroyList(varConsts->varsStore[i]);
    }
    for (i = 0; i < varConsts->allocatedConsts; ++i) {
        xanList_destroyList(varConsts->constsLoad[i]);
        xanList_destroyList(varConsts->constsStore[i]);
    }
    freeFunction(varConsts->consts);
    freeFunction(varConsts->varsArith);
    freeFunction(varConsts->varsLoad);
    freeFunction(varConsts->varsStore);
    freeFunction(varConsts->constsLoad);
    freeFunction(varConsts->constsStore);
    freeFunction(varConsts);
}


/**
 * Free a pass information structure.
 **/
static void
freePassInfo(pass_info_t *info) {
    JITINT32 i;
    IRMETHOD_destroyInstructionsPredecessors(info->predecessors, info->numInsts);
    freeFunction(info->insns);
    for (i = 0; i < info->numInsts; ++i) {
        xanBitSet_free(info->matchingExprs[i]);
        xanBitSet_free(info->genSets[i]);
        xanBitSet_free(info->invKillSets[i]);
        xanBitSet_free(info->inSets[i]);
        xanBitSet_free(info->outSets[i]);
    }
    freeFunction(info->matchingExprs);
    freeFunction(info->genSets);
    freeFunction(info->invKillSets);
    freeFunction(info->inSets);
    freeFunction(info->outSets);
    freeFunction(info);
}


/**
 * Kill all expressions in the current instruction that original from
 * instructions within the given list.
 */
static void killAllInstsInList (XanBitSet *killSet, XanList *instList) {
    XanListItem *curItem = xanList_first(instList);

    while (curItem) {
        ir_instruction_t *killInst = curItem->data;
        xanBitSet_setBit(killSet, killInst->ID);
        /* JITUINT32 killInstId = killInst->ID; */
        /* JITUINT32 numBlockBits = sizeof(JITNUINT) * 8; */
        /* JITUINT32 blockNum = killInstId / numBlockBits; */
        /* JITNUINT block = getKillBlock(method, inst, blockNum); */
        /* block |= (JITNUINT)1 << (killInstId % numBlockBits); */
        /* setKillBlock(method, inst, blockNum, block); */
        curItem = curItem->next;
    }

    return ;
}

static void killAllDependentInstsInList (XanBitSet *killSet, XanList *instList, ir_method_t *m, ir_instruction_t *i) {
    XanListItem *curItem = xanList_first(instList);

    while (curItem) {
        ir_instruction_t *killInst = curItem->data;
        assert(killInst != NULL);
        if (    IRMETHOD_isInstructionDataDependentThroughMemory(m, i, killInst)    ||
                IRMETHOD_isInstructionDataDependentThroughMemory(m, killInst, i)    ){
            xanBitSet_setBit(killSet, killInst->ID);
        }
        curItem = curItem->next;
    }

    return ;
}

/**
 * Kill all loads in the given instruction that have the given variable as
 * their first parameter (base address).
 */
static void
killAllLoadsFromVar (XanBitSet *killSet, var_consts_t *varConsts, JITUINT64 varId) {
    killAllInstsInList(killSet, varConsts->varsLoad[varId]);
}


/**
 * Kill all stores in the given instruction that have the given variable as
 * their first parameter (base address).
 */
static void
killAllStoresFromVar (XanBitSet *killSet, var_consts_t *varConsts, JITUINT64 varId) {
    killAllInstsInList(killSet, varConsts->varsStore[varId]);
}

/**
 * Kill all loads and stores in the given instruction that have the given variable as
 * their first parameter (base address) and that are dependent on the input instruction.
 */
static void
killAllDependentLoadsAndStoresWithVariableAddresses (XanBitSet *killSet, var_consts_t *varConsts, ir_method_t *m, ir_instruction_t *i) {
    IR_ITEM_VALUE   var;

    for (var = 0; var < varConsts->numVars; ++var) {
        killAllDependentInstsInList(killSet, varConsts->varsLoad[var], m, i);
        killAllDependentInstsInList(killSet, varConsts->varsStore[var], m, i);
    }
}

/**
 * Kill all memory instructions that depend on a given one.
 */
static void
killAllLoadsAndStoresOfConstantAddressThatDependOn (XanBitSet *killSet, var_consts_t *varConsts, ir_method_t *m, ir_instruction_t *i){
    JITUINT32 c;

    for (c = 0; c < varConsts->numConsts; c++) {
        killAllDependentInstsInList(killSet, varConsts->constsLoad[c], m, i);
        killAllDependentInstsInList(killSet, varConsts->constsStore[c], m, i);
    }
}

/**
 * Kill all loads in the given instruction that have a constant for their
 * first parameter (base address).
 */
static void
killAllLoadsFromConsts (XanBitSet *killSet, var_consts_t *varConsts) {
    JITUINT32 c;

    for (c = 0; c < varConsts->numConsts; c++) {
        killAllInstsInList(killSet, varConsts->constsLoad[c]);
    }
}


/**
 * Kill all stores in the given instruction that have a constant for their
 * first parameter (base address).
 */
static void
killAllStoresFromConsts (XanBitSet *killSet, var_consts_t *varConsts) {
    JITUINT32 c;

    for (c = 0; c < varConsts->numConsts; c++) {
        killAllInstsInList(killSet, varConsts->constsStore[c]);
    }
}


/**
 * Kill all loads in the given instruction that have a given constant for their
 * first parameter (base address).
 */
static void
killAllLoadsFromSpecificConst (XanBitSet *killSet, JITUINT64 constVal, var_consts_t *varConsts) {
    JITUINT32 c;

    for (c = 0; c < varConsts->numConsts; c++) {
        if (varConsts->consts[c] == constVal) {
            killAllInstsInList(killSet, varConsts->constsLoad[c]);
        }
    }
}


/**
 * Kill all stores in the given instruction that have a given constant for their
 * first parameter (base address).
 */
static void
killAllStoresFromSpecificConst (XanBitSet *killSet, JITUINT64 constVal, var_consts_t *varConsts) {
    JITUINT32 c;

    for (c = 0; c < varConsts->numConsts; c++) {
        if (varConsts->consts[c] == constVal) {
            killAllInstsInList(killSet, varConsts->constsStore[c]);
        }
    }
}


/**
 * Kill all arithmetic instructions in the given instruction that use the
 * given variable.
 */
static void
killAllArithFromVar (XanBitSet *killSet, var_consts_t *varConsts, JITUINT64 varId) {
    killAllInstsInList(killSet, varConsts->varsArith[varId]);
}


#ifdef PRINTDEBUG
static void
printSets(ir_method_t *method) {
    JITUINT32 i, numInsts, blockNum, bitNum;
    PDEBUG(0, "AE Sets\n");
    numInsts = IRMETHOD_getInstructionsNumber(method);
    for (i = 0; i < numInsts; ++i) {
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        PDEBUG(0, "Inst %u\n", i);
        PDEBUG(0, "  In:");
        for (blockNum = 0; blockNum < method->availableExpressionsBlockNumbers; ++blockNum) {
            JITNUINT block = ir_instr_available_expressions_in_get(method, inst, blockNum);
            for (bitNum = 0; bitNum < 8 * sizeof(JITNUINT); ++bitNum) {
                if ((block >> bitNum) & 1) {
                    PDEBUGC(0, " %u", (blockNum * 8 * sizeof(JITNUINT)) + bitNum);
                }
            }
        }
        PDEBUGC(0, "\n");
        PDEBUG(0, "  Out:");
        for (blockNum = 0; blockNum < method->availableExpressionsBlockNumbers; ++blockNum) {
            JITNUINT block = ir_instr_available_expressions_out_get(method, inst, blockNum);
            for (bitNum = 0; bitNum < 8 * sizeof(JITNUINT); ++bitNum) {
                if ((block >> bitNum) & 1) {
                    PDEBUGC(0, " %u", (blockNum * 8 * sizeof(JITNUINT)) + bitNum);
                }
            }
        }
        PDEBUGC(0, "\n");
    }
}
#else
#define printSets(method)
#endif  /* ifdef PRINTDEBUG */


#ifdef PRINTDEBUG
static void
printBitSet(XanBitSet *set) {
    JITUINT32 i;
    for (i = 0; i * 32 < set->length; ++i) {
        fprintf(stderr, " 0x%x", set->data[i]);
    }
}
#else
#define printBitSet(set)
#endif  /* ifdef PRINTDEBUG */


/**
 * Initialise the available expression kill and gen sets.
 */
static void
initKillGenSets (pass_info_t *info, var_consts_t *varConsts) {
    JITUINT32 i, instPos;
    ir_instruction_t 	*inst;

    /**
     * Initialise each kill, gen, in and out set.  Kill and gen sets should be
     * empty initially.  In and out sets should be full, apart from the first
     * instruction's set which is empty.  This is because the algorithm uses
     * the intersection operator that makes sets smaller.
     **/

    /* Gen set should already be empty. */
#ifdef DEBUG
    for (i = 0; i < info->numInsts; ++i) {
        assert(xanBitSet_isEmpty(info->genSets[i]));
    }
#endif

    /* In and out sets for all. */
    for (i = info->numMethodParams; i < info->numInsts; ++i) {
        xanBitSet_setAll(info->inSets[i]);
        xanBitSet_setAll(info->outSets[i]);
    }

    /* First instruction's in set is empty. */
    xanBitSet_clearAll(info->inSets[info->numMethodParams]);

    /* Calculate the kill and gen sets for each instruction. */
    for (instPos = info->numMethodParams; instPos < info->numInsts; ++instPos) {
        JITUINT32 	var;
        IR_ITEM_VALUE	definedVar;

        /* Fetch the instruction.
         */
        inst 		= info->insns[instPos];
        assert(inst != NULL);
        definedVar	= IRMETHOD_getVariableDefinedByInstruction(info->method, inst);

        /**
         * Now calculate the expressions killed based on instruction type.  In
         * particular, stores to memory kill all load expressions that use the
         * same variable as the base address.  They also kill loads from a
         * constant base address, since we don't have the analysis (yet) to
         * determine whether the two addresses alias.  Calls again kill all
         * loads because we don't have the interprocedural analysis to work out
         * if the value in memory will change.  Finally, instructions that write
         * to a variable kill off all expressions that use the variable.
         */

        /* Kill expressions due to stores to memory. */
        if (IRMETHOD_getInstructionType(inst) == IRSTOREREL){
            ir_item_t	*ba;
            JITUINT32 	varId;
            ba		= IRMETHOD_getInstructionParameter1(inst);
            varId		= (ba->value).v;

            /* Check for variables that might alias and kill loads and stores using them. */
            for (var = 0; var < varConsts->numVars; ++var) {
                if (	(var == varId)					||
                        (!IRDATA_isAVariable(ba))			||
                        (IRMETHOD_mayAlias(info->method, inst, varId, var)) 	) {
                    killAllLoadsFromVar(info->invKillSets[inst->ID], varConsts, var);
                    killAllStoresFromVar(info->invKillSets[inst->ID], varConsts, var);
                }
            }

            /* Kill loads from a constant too. */
            if (IRDATA_isAVariable(ba)) {

                /* A variable store can change values of every memory location that can be accessed by each dependent instruction.
                 * Hence, kill all dependent load and store instructions.
                 */
                killAllLoadsAndStoresOfConstantAddressThatDependOn(info->invKillSets[inst->ID], varConsts, info->method, inst);
                killAllDependentLoadsAndStoresWithVariableAddresses(info->invKillSets[inst->ID], varConsts, info->method, inst);
               // killAllLoadsFromConsts(info->invKillSets[inst->ID], varConsts);
               // killAllStoresFromConsts(info->invKillSets[inst->ID], varConsts);

            } else {
                killAllLoadsFromSpecificConst(info->invKillSets[inst->ID], varId, varConsts);
                killAllStoresFromSpecificConst(info->invKillSets[inst->ID], varId, varConsts);
            }
        }

        /* Calls kill all loads and stores without interprocedural analysis. */
        else if (IRMETHOD_isACallInstruction(inst)) {
            for (var = 0; var < varConsts->numVars; ++var) {
                killAllLoadsFromVar(info->invKillSets[inst->ID], varConsts, var);
                killAllStoresFromVar(info->invKillSets[inst->ID], varConsts, var);
            }
            killAllLoadsFromConsts(info->invKillSets[inst->ID], varConsts);
            killAllStoresFromConsts(info->invKillSets[inst->ID], varConsts);

            /* Kill escaped variables. */
            for (var = 0; var < varConsts->numVars; ++var) {
                if (IRMETHOD_isAnEscapedVariable(info->method, var)) {
                    killAllArithFromVar(info->invKillSets[inst->ID], varConsts, var);
                    killAllLoadsFromVar(info->invKillSets[inst->ID], varConsts, var);
                    killAllStoresFromVar(info->invKillSets[inst->ID], varConsts, var);
                }
            }
        }

        /* Kill expressions using variables that are written to. */
        if (definedVar != NOPARAM) {

            /* Kill instructions using this variable. */
            killAllArithFromVar(info->invKillSets[inst->ID], varConsts, definedVar);
            killAllLoadsFromVar(info->invKillSets[inst->ID], varConsts, definedVar);
            killAllStoresFromVar(info->invKillSets[inst->ID], varConsts, definedVar);
        }

        /* We want the inverted kill set, it's more efficient storing like this. */
        xanBitSet_invertBits(info->invKillSets[inst->ID]);

        /**
         * We've now killed off all expressions based on instruction type and
         * variables written.  The last step is to calculate the expressions that
         * are generated by the current instruction.
         */
        if (is_an_expression(inst)) {
            xanBitSet_setBit(info->genSets[inst->ID], inst->ID);
            /* JITUINT32 bNum = instPos / info->numBlockBits; */
            /* JITNUINT genBlock = (JITNUINT)1 << (instPos % info->numBlockBits); */
            /* JITNUINT oldGen = getGenBlock(info->method, inst, bNum); */
            /* JITNUINT newGen = oldGen | genBlock; */
            /* setGenBlock(info->method, inst, bNum, newGen); */
        }
    }

#ifdef DEBUG
    /* ir_instruction_t        *firstInst; */
    /* JITUINT32 blockNum; */
    /* firstInst = IRMETHOD_getFirstInstruction(info->method); */
    /* for (blockNum = 0; blockNum < info->method->availableExpressionsBlockNumbers; ++blockNum) { */
    /*     JITNUINT block; */
    /*     block = getInBlock(info->method, firstInst, blockNum); */
    /*     assert(block == 0); */
    /* } */
#endif
}


/**
 * Increase the set of expressions from the join of predecessor sets with
 * expressions that are available in the union of predecessor sets, as long
 * as the expressions are available in all predecessors.
 */
static void
increaseJoinSet(pass_info_t *info, JITUINT32 instPos, XanBitSet *unionSet, XanBitSet *joinSet, XanBitSet **tmpSets) {
    JITNINT exprInstPos = -1;
    PDEBUG(1, "Increase join set for %u\n", instPos);

    /* Find each union bit that is set. */
    while ((exprInstPos = xanBitSet_getFirstBitSetInRange(unionSet, exprInstPos + 1, info->numInsts)) != -1) {

        /* Find expressions in the union set but not in the join set. */
        if (!xanBitSet_isBitSet(joinSet, exprInstPos)) {
            XanListItem   *item;
            JITBOOLEAN inAllPreds;
            PDEBUG(3, "  Expr inst %u\n", exprInstPos);

            /**
             * We've found a bit that's in the union set but not in the join
             * set.  We now get that expression and check to see if the
             * expression is available from each predecessor, but from a
             * different instruction number.  If it is, then we can add this
             * expression (and all others from the predecessors) to the join
             * set and continue.
             */
            inAllPreds = JITTRUE;
            xanBitSet_clearAll(tmpSets[0]);

            /* Check all predecessors. */
            item = xanList_first(info->predecessors[instPos]);
            while (item) {
                ir_instruction_t *pred = item->data;
                assert(pred != NULL);

                /* Check for overlaps. */
                xanBitSet_copy(tmpSets[1], info->matchingExprs[exprInstPos]);
                xanBitSet_intersect(tmpSets[1], info->outSets[pred->ID]);
                if (!xanBitSet_isEmpty(tmpSets[1])) {
                    xanBitSet_union(tmpSets[0], tmpSets[1]);
                    PDEBUG(4, "    In predecessor %u\n", pred->ID);
                } else {
                    inAllPreds = JITFALSE;
                    break;
                }

                /* Next predecessor. */
                item = item->next;
            }

            /* If the expression is in all predecessors, add others. */
            if (inAllPreds) {
                xanBitSet_union(joinSet, tmpSets[0]);
                PDEBUG(3, "    Added\n");
            }
        }
    }
}


/**
 * Perform a data flow step over bit sets.
 */
static void
dataFlowStep(XanBitSet *out, XanBitSet *in, XanBitSet *gen, XanBitSet *invKill) {
    size_t i;
    size_t words;

    assert(out->length == in->length);
    assert(out->length == gen->length);
    assert(out->length == invKill->length);

    words = (out->length + (8 * sizeof(size_t)) - 1) / (8 * sizeof(size_t));
    for (i = 0; i < words; ++i) {
        out->data[i] = (in->data[i] & invKill->data[i]) | gen->data[i];
    }
}


/**
 * Perform one pass over the method to compute the available expressions.  This
 * iterates over all instructions, first building the set of available
 * expressions into the instruction.  This is simple for instructions with only
 * one predecessor.  For those with multiple predecessors, we cannot do a
 * simple set join, since then we may mis-classify expressions from separate
 * predecessors as being unavailable.  Therefore, we must check the cross
 * product of available expressions from all predecessors to ensure that an
 * expression is available down each path into the instruction.  Once this is
 * done, the out set is then calculated.
 */
static JITBOOLEAN
computeAvailableExpressionsPass(pass_info_t *info, XanBitSet *unionSet, XanBitSet *joinSet, XanBitSet **tmpSets) {
    JITUINT32 	instPos;
    JITBOOLEAN 	modified = JITFALSE;

    /* Don't look at first instruction's predecessors, in case it's a branch target. */
    instPos = info->numMethodParams;
    dataFlowStep(info->outSets[instPos], info->inSets[instPos], info->genSets[instPos], info->invKillSets[instPos]);

    /* Iterate over all other instructions. */
    for (instPos = info->numMethodParams + 1; instPos < info->numInsts; ++instPos) {
        ir_instruction_t *pred = NULL;
        JITUINT32 numPreds;

        /* Get the instruction and number of predecessors. */
        numPreds = xanList_length(info->predecessors[instPos]);

        /* Simple case of one predecessor. */
        if (numPreds == 1) {
            XanListItem *item;
            item = xanList_first(info->predecessors[instPos]);
            assert(item != NULL);
            pred = item->data;
            assert(pred);

            /* Each in block is simply the predecessor's out block. */
            if (!xanBitSet_equal(info->inSets[instPos], info->outSets[pred->ID])) {
                xanBitSet_copy(info->inSets[instPos], info->outSets[pred->ID]);
                modified = JITTRUE;
            }
        }

        /* Multiple predecessors. */
        else if (numPreds > 1) {
            XanListItem *item;

            /* Set the join set to the current in set. */
            xanBitSet_clearAll(unionSet);
            xanBitSet_copy(joinSet, info->inSets[instPos]);

            /* Get the join and union sets for all predecessors. */
            item = xanList_first(info->predecessors[instPos]);
            while (item != NULL) {
                pred = item->data;
                assert(pred != NULL);
                xanBitSet_union(unionSet, info->outSets[pred->ID]);
                xanBitSet_intersect(joinSet, info->outSets[pred->ID]);
                item = item->next;
            }

            /* Add expressions to the join set if available in all predecessors. */
            increaseJoinSet(info, instPos, unionSet, joinSet, tmpSets);

            /* The new in set is the join set. */
            if (!xanBitSet_equal(info->inSets[instPos], joinSet)) {
                xanBitSet_copy(info->inSets[instPos], joinSet);
                modified = JITTRUE;
            }
        }

        /* Finally, compute the out set for this instruction. */
        dataFlowStep(tmpSets[0], info->inSets[instPos], info->genSets[instPos], info->invKillSets[instPos]);
        if (!xanBitSet_equal(info->outSets[instPos], tmpSets[0])) {
            xanBitSet_copy(info->outSets[instPos], tmpSets[0]);
            modified = JITTRUE;
        }
    }

    /* Return whether modifications have occurred. */
    return modified;
}


/**
 * Transfer available expression sets to blocks within the method.
 */
static void
recordAvailableExpressions(pass_info_t *info) {
    JITUINT32 i, j, b, blockBits = sizeof(JITNUINT) * 8;
    JITUINT32 numBlocks = ceil(1.0 * info->numInsts / blockBits);
    IRMETHOD_allocMemoryForAvailableExpressionsAnalysis(info->method, numBlocks);
    for (i = 0; i < info->numInsts; ++i) {
        ir_instruction_t *inst = info->insns[i];
        for (b = 0; b < numBlocks; ++b) {
            JITNUINT in = 0, out = 0, kill = 0, gen = 0;
            for (j = 0; j < blockBits; ++j) {
                JITUINT32 bit = b * blockBits + j;
                in |= (xanBitSet_isBitSet(info->inSets[i], bit) ? 1 : 0) << j;
                out |= (xanBitSet_isBitSet(info->outSets[i], bit) ? 1 : 0) << j;
                kill |= (xanBitSet_isBitSet(info->invKillSets[i], bit) ? 0 : 1) << j;
                gen |= (xanBitSet_isBitSet(info->genSets[i], bit) ? 1 : 0) << j;
            }
            ir_instr_available_expressions_in_set(info->method, inst, b, in);
            ir_instr_available_expressions_out_set(info->method, inst, b, out);
            ir_instr_available_expressions_kill_set(info->method, inst, b, kill);
            ir_instr_available_expressions_gen_set(info->method, inst, b, gen);
        }
    }
}


/**
 * Compute the available expressions from the initialised kill and gen sets.
 * This function simply iterates until a fixed point is reached, calling a
 * helper function to do the computation.
 */
static void
computeAvailableExpressions(pass_info_t *info) {
    XanBitSet *unionSet;
    XanBitSet *joinSet;
    XanBitSet **tmpSets;
    JITBOOLEAN 		modified;

    /* Allocate the necessary memory for the join and union sets of expressions. */
    unionSet = xanBitSet_new(info->numInsts);
    joinSet = xanBitSet_new(info->numInsts);

    /* Create two temporary sets for intermediate results. */
    tmpSets = allocFunction(2 * sizeof(XanBitSet *));
    tmpSets[0] = xanBitSet_new(info->numInsts);
    tmpSets[1] = xanBitSet_new(info->numInsts);

    /* Iterate until a fixed point. */
    do {
        modified = computeAvailableExpressionsPass(info, unionSet, joinSet, tmpSets);
    } while (modified);

    /* Record available expression information in the method. */
    recordAvailableExpressions(info);

    /* Free memory that's no longer needed. */
    xanBitSet_free(unionSet);
    xanBitSet_free(joinSet);
    xanBitSet_free(tmpSets[0]);
    xanBitSet_free(tmpSets[1]);
    freeFunction(tmpSets);
}

static void
ae_do_job(ir_method_t *method) {
    pass_info_t *info;
    var_consts_t *varConsts;
    char *env;

    /* Assertions. */
    assert(method);

    /* See if there's anything to do. */
    if (IRMETHOD_getInstructionsNumber(method) == 0) {
        return;
    }

    /* Enable debugging. */
    env = getenv("AE_DEBUG_LEVEL");
    if (env) {
      PDEBUGLEVEL(atoi(env));
    }

    /* Debugging output. */
    PDEBUG(0, "Available expressions for %s\n", IRMETHOD_getCompleteMethodName(method));

    /* Initialise the pass information structure. */
    info = initPassInfo(method);
    varConsts = initVarConsts(info);

    /* Initialise the kill and gen sets. */
    initKillGenSets(info, varConsts);
    printSets(method);

    /* Compute the available expressions. */
    computeAvailableExpressions(info);

    /* Free the memory			*/
    destroyVarConsts(varConsts);
    freePassInfo(info);

    /* Check some invariants		*/
#ifdef DEBUG
    /* ir_instruction_t        *firstInst; */
    /* JITUINT32 blockNum; */
    /* firstInst = IRMETHOD_getFirstInstruction(method); */
    /* for (blockNum = 0; blockNum < method->availableExpressionsBlockNumbers; ++blockNum) { */
    /*     JITNUINT block; */
    /*     block = getInBlock(method, firstInst, blockNum); */
    /*     assert(block == 0); */
    /* } */
#endif

    return;
}

void ae_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

void ae_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

JITUINT64 get_ID_job (void) {
    return AVAILABLE_EXPRESSIONS_ANALYZER;
}

JITUINT64 get_dependences (void) {
    return LIVENESS_ANALYZER | ESCAPES_ANALYZER;
}

char * get_version (void) {
    return VERSION;
}

char * get_informations (void) {
    return INFORMATIONS;
}

static inline char * cse_get_author (void) {
    return AUTHOR;
}

static inline JITBOOLEAN is_an_expression (ir_instruction_t *inst) {
    if (is_a_not_memory_expression(inst)) {
        return JITTRUE;
    }
    switch (IRMETHOD_getInstructionType(inst)) {
        case IRLOADREL:
        case IRSTOREREL:
            return JITTRUE;
    }
    return JITFALSE;
}

static inline JITBOOLEAN is_a_not_memory_expression (ir_instruction_t *inst) {

    /* Assertions			*/
    assert(inst != NULL);

    if (	(IRMETHOD_isAMathInstruction(inst))		||
            (IRMETHOD_isACompareInstruction(inst))		||
            (IRMETHOD_isAConversionInstruction(inst))	||
            (IRMETHOD_isABitwiseInstruction(inst))		) {
        return JITTRUE;
    }

    switch (IRMETHOD_getInstructionType(inst)) {
        case IRMOVE:
        case IRGETADDRESS:
        case IRSIZEOF:
        case IRARRAYLENGTH:
            return JITTRUE;
        default:
            return JITFALSE;
    }
}

static inline void ae_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void ae_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
