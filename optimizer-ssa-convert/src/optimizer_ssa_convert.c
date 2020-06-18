/*
 * Copyright (C) 2007 - 2012 Michele Melchiori, Michele Tartara, Timothy M. Jones, Simone Campanoni
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

// My headers
#include <optimizer_ssa_convert.h>
#include <definitions.h>
#include <ssa_dominance.h>
#include <config.h>
// End

/* plugin informations */
#define SSA_CONVERT_JOB           SSA_CONVERTER
#define SSA_CONVERT_AUTHOR        "Michele Melchiori, Michele Tartara, Timothy M. Jones, Simone Campanoni"
#define SSA_CONVERT_INFORMATIONS  "This plugin convert the bytecode into Static Single Assignment form"
#define SSA_CONVERT_DEPENDENCIES  BASIC_BLOCK_IDENTIFIER
#define SSA_CONVERT_INVALIDATIONS INVALIDATE_ALL & ~ BASIC_BLOCK_IDENTIFIER

static inline JITUINT64 ssa_convert_get_ID_job (void);
static inline char*     ssa_convert_get_version (void);
static inline void      ssa_convert_do_job (ir_method_t * method);
static inline char*     ssa_convert_get_informations (void);
static inline char*     ssa_convert_get_author (void);
static inline JITUINT64 ssa_convert_get_dependences (void);
static inline JITUINT64 ssa_convert_get_invalidations (void);
static inline void      ssa_convert_shutdown (JITFLOAT32 totalTime);
static inline void      ssa_convert_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);

/* custom methods signatures */
static void ssa_convert_addParameterStores(ir_method_t *method);
static        void            ssa_convert_resolveDoubleDefinitions         (GlobalObjects* glob);
static        IR_ITEM_VALUE   ssa_convert_getFreeVariableID                (ir_method_t* method);
static        void            ssa_convert_renameVarInInstrIfDefined        (ir_method_t* method, ir_instruction_t* instr, IR_ITEM_VALUE oldVariable, IR_ITEM_VALUE newVariable);
static        void            ssa_convert_renameVarInInstrIfUsed           (GlobalObjects* glob, ir_instruction_t* instr, IR_ITEM_VALUE oldVariable, IR_ITEM_VALUE newVariable);
static        void            ssa_convert_initVarLiveAcrossBB              (GlobalObjects* glob);
static        void            ssa_convert_init_varInfoList                 (GlobalObjects* glob);
static inline void            ssa_convert_deleteVarInfo                    (XanList* varInfoList, VarInfo* toDelete);
static        void            ssa_convert_computeDF                        (GlobalObjects* glob, XanNode* root);
static        BasicBlockInfo* ssa_convert_getBasicBlockInfoFromInstruction (BasicBlockInfo** bbInfos, JITUINT32 bbInfosLength, ir_instruction_t* instr);
static        void            ssa_convert_place_phi_functions              (GlobalObjects* glob);
static        void            ssa_convert_renameVarsInDomTree              (GlobalObjects* glob, XanNode* myRoot);
static inline void            ssa_convert_freeUpMemory                     (GlobalObjects* glob);
static inline void            ssa_convert_checkMethod                      (ir_method_t *method);

ir_lib_t       *irLib       = NULL;
ir_optimizer_t *irOptimizer = NULL;
char           *prefix      = NULL;

ir_optimization_interface_t plugin_interface = {
    ssa_convert_get_ID_job,
    ssa_convert_get_dependences,
    ssa_convert_init,
    ssa_convert_shutdown,
    ssa_convert_do_job,
    ssa_convert_get_version,
    ssa_convert_get_informations,
    ssa_convert_get_author,
    ssa_convert_get_invalidations
};

static inline void ssa_convert_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib       = lib;
    irOptimizer = optimizer;
    prefix      = outputPrefix;
}

static inline void ssa_convert_shutdown (JITFLOAT32 totalTime) {
    irLib       = NULL;
    irOptimizer = NULL;
    prefix      = NULL;
}

static inline JITUINT64 ssa_convert_get_ID_job (void) {
    return SSA_CONVERT_JOB;
}

static inline JITUINT64 ssa_convert_get_dependences (void) {
    return SSA_CONVERT_DEPENDENCIES;
}

static inline void ssa_convert_do_job (ir_method_t * method) {
    char *prevName;
    /* Work list:
         * 0. Tim: You can't tell by looking at the first few instructions that they define parameters, so if any of the parameter
         *    variables are redefined elsewhere in the method, add explicit stores to new variables for them and alter all uses
         *    and redefinitions.
     * 1. Calculate the dominance frontier for each basic block in the graph with the following steps
     *    1.1. Build the dominance tree of the basic blocks
     *    1.2. Find the dominance frontier of each basic block starting from the root, using alorithm by chapter 19.1 of Appel
     * 2. This step computes the semi-pruned SSA form. It search for all variables having multiple
     *    definition points and consider only the ones that are live in in some basic blocks.
     *    This sub-kind of liveness is quite easy to compute than the complete liveness information.
     *    For each of this
     * 3. keep track of all definition points
     * 4. use algorithm 19.6 of Appel for inserting the PHI functions
     * 5. algorithm 19.7 of Appel describes how to make the variable renaming
     */

    /* Assertions */
    assert(method != NULL);
    prevName = getenv("DOTPRINTER_FILENAME");
    setenv("DOTPRINTER_FILENAME", "ssaConvert", 1);

    /* Common variable definitions and initializations */
    GlobalObjects* glob = allocMemory(sizeof(GlobalObjects));
    memset(glob, 0, sizeof(GlobalObjects));
    glob->method = method;

    PDEBUG("OPTIMIZER_SSA_CONVERTER: ===== Starting transformation of method %s =====\n", IRMETHOD_getMethodName(method));

    /* Add stores for parameter variables if they are redefined in the method. */
    ssa_convert_addParameterStores(method);

    // compute the dominance tree
    ssa_convert_buildDominanceTree(glob);
    if (glob->root == NULL) {
        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): no dominance information, terminating optimization because useless\n", IRMETHOD_getMethodName(method));
        ssa_convert_freeUpMemory(glob);
        setenv("DOTPRINTER_FILENAME", prevName, 1);
        return;
    }

    // now look for all vars that are live_in in some basic block
    ssa_convert_initVarLiveAcrossBB(glob);

    // resolve trivial cases: one variable define more than one time in the same basic block.
    // We need to have the def and use sets before applying this optimization.
    // Note that these sets are updated when renaming variables, so it is not necessary
    // to rebuild them after this part of the optimization.
    ssa_convert_resolveDoubleDefinitions(glob);

    // check if there is at least one variable live across some basic block
    // that makes useful this optimization
    JITUINT32 count = 0;
    JITBOOLEAN useful = JITFALSE;
    for (count = 0; (!useful) && (count < glob->varLiveAcrossBBLen); count++) {
        if (glob->varLiveAcrossBB[count] != 0) {
            useful = JITTRUE;
        }
    }
    if (!useful) {
        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): there are no variables live_in in at least one basic block. This optimization is useless.\n", IRMETHOD_getMethodName(method));
        freeMemory(glob->varLiveAcrossBB);
        for (count = 0; count < IRMETHOD_getNumberOfMethodBasicBlocks(method); count++) {
            freeFunction(glob->bbInfos[count]);
        }
        freeMemory(glob->bbInfos);
        glob->root->destroyTree(glob->root);
        freeMemory(glob);
        setenv("DOTPRINTER_FILENAME", prevName, 1);
        return;
    }

#ifdef DEBUG
    {
        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): vars live across some basic blocks: ", IRMETHOD_getMethodName(method));
        JITUINT32 count = 0;
        for (count = 0; count < glob->varLiveAcrossBBLen; count++) {
            PDEBUG("0x%lx ", glob->varLiveAcrossBB[count]);
        }
    }
    PDEBUG("\n");
#endif

    // for this vars init the varInfoList structure: keep track of def sites and use sites
    ssa_convert_init_varInfoList(glob);

#ifdef DEBUG
    {
        XanList* varInfoList = glob->varInfoList;
        XanListItem* var = xanList_first(varInfoList);
        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): variable informations:\n", IRMETHOD_getMethodName(method));
        while (var != NULL) {
            VarInfo* info = var->data;
            PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): \tvar %llu definition points: ", IRMETHOD_getMethodName(method), info->var->value.v);
            XanListItem* point = xanList_first(info->defPoints);
            while (point != NULL) {
                PDEBUG("%u\t", ((ir_instruction_t*)info->defPoints->data(info->defPoints, point))->ID);
                point = point->next;
            }
            PDEBUG("\n");
            var = var->next;
        }
    }
#endif

    // compute the dominance frontier
    ssa_convert_computeDF(glob, NULL);

#ifdef DEBUG
    {
        // let's look if the work is done correctly
        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): dominance tree and dominance frontier informations:\n", IRMETHOD_getMethodName(method));
        XanList* listaBB = glob->root->toPreOrderList(glob->root);
        XanListItem* elem = NULL;

        assert(listaBB != NULL);
        for (elem = xanList_first(listaBB); elem != NULL; elem = elem->next) {
            BasicBlockInfo* thisBB = elem->data;
            assert(thisBB != NULL);
            PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): basic block (%4d -> %4d)", IRMETHOD_getMethodName(method), thisBB->startInst->ID, thisBB->endInst->ID);
            XanNode* idom = glob->root->getParent(glob->root->find(glob->root, thisBB));
            if (idom == NULL) {
                PDEBUG("\tidom: none        ");
            } else {
                PDEBUG("\tidom: (%4d -> %4d)", ((BasicBlockInfo*) idom->getData(idom))->startInst->ID, ((BasicBlockInfo*) idom->getData(idom))->endInst->ID);
            }
            PDEBUG("\tdominance frontier:");
            XanList* thisDF = thisBB->domFront;
            if (xanList_length(thisDF) == 0) {
                PDEBUG(" none        ");
            } else {
                XanListItem* tmp = xanList_first(thisDF);
                while (tmp != NULL) {
                    PDEBUG(" (%4d -> %4d)", ((BasicBlockInfo*) tmp->data)->startInst->ID, ((BasicBlockInfo*) tmp->data)->endInst->ID);
                    tmp = tmp->next;
                }
            }
            PDEBUG("\n");
        }
        xanList_destroyList(listaBB);
    }
#endif

    // ok, now we have all the informations needed
    ssa_convert_place_phi_functions(glob);
    ssa_convert_renameVarsInDomTree(glob, NULL);

    // free up previously used memory
    ssa_convert_freeUpMemory(glob);

    // Check the method. */
    ssa_convert_checkMethod(method);

    if (prevName == NULL) {
        prevName	= "";
    }
    setenv("DOTPRINTER_FILENAME", prevName, 1);
    PDEBUG("OPTIMIZER_SSA_CONVERTER: ===== %s complete =====\n", IRMETHOD_getMethodName(method));

    /* Return */
    return;
}

static inline char * ssa_convert_get_version (void) {
    return VERSION;
}

static inline char * ssa_convert_get_informations (void) {
    return SSA_CONVERT_INFORMATIONS;
}

static inline char * ssa_convert_get_author (void) {
    return SSA_CONVERT_AUTHOR;
}

static inline JITUINT64 ssa_convert_get_invalidations (void) {
    return SSA_CONVERT_INVALIDATIONS;
}


/**
 * Check each parameter variable to see if it is redefined within the
 * method.  If it is, create a new variable for it, add a store after the
 * parameter definitions to write to this new variable and update all
 * other uses and definitions of the parameter.
 */
static void
ssa_convert_addParameterStores(ir_method_t *method) {
    JITUINT32 p;
    XanList *instList = IRMETHOD_getInstructions(method);

    /* Consider all parameters. */
    for (p = 0; p < IRMETHOD_getMethodParametersNumber(method); ++p) {

        /* Check for a redefinition of this parameter. */
        if (IRMETHOD_doInstructionsDefineVariable(method, instList, p)) {
            IR_ITEM_VALUE newVar;
            ir_instruction_t *firstRealInst;
            ir_instruction_t *newStore;
            JITUINT32 paramType;

            /* Perform the substitution. */
            newVar = IRMETHOD_newVariableID(method);
            IRMETHOD_substituteVariable(method, p, newVar);

            /* Add a store after the parameter initialisations. */
            firstRealInst = IRMETHOD_getInstructionAtPosition(method, IRMETHOD_getMethodParametersNumber(method));
            newStore = IRMETHOD_newInstructionOfTypeBefore(method, firstRealInst, IRMOVE);
            paramType = IRMETHOD_getParameterType(method, p);
            IRMETHOD_setInstructionParameter(method, newStore, newVar, 0.0, IROFFSET, paramType, NULL, 0);
            IRMETHOD_setInstructionParameter(method, newStore, p, 0.0, IROFFSET, paramType, NULL, 1);

            /* Debugging output. */
            PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Added store for parameter %u\n", IRMETHOD_getMethodName(method), p);
        }
    }

    /* Recompute basic block information. */
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, BASIC_BLOCK_IDENTIFIER);

    /* Clean up. */
    xanList_destroyList(instList);
}


/**
 * @brief convert in SSA form each single basic block, resolving trivial cases
 *
 * Situation: one var is defined twice or more <b>IN THE SAME BASIC BLOCK</b>.
 * In this case we have that one definition kills any previous definition of the same variable
 * in the same basic block, so we can rename all the occurrencies of this var from one definition
 * to the next:
 * \code
 * before (non SSA) | after (is in SSA form)
 * -----------------+-----------------------
 *   ...            |   ...
 *   x = y + x      |   x1 = y  + x
 *   a = x + 1      |   a  = x1 + 1
 *   b = x - 3      |   b  = x1 - 3
 *   x = b          |   x  = b
 *   a = x          |   a  = x
 *   ...            |   ...
 * -----------------+-----------------------
 * \endcode
 * note that this means that variable x1 is NEVER live_out of this basic block, so we
 * don't have to recalculate the previous steps: the continue to remain corrects!
 *
 * This method for each variable scan all its definition points and when it find that
 * two of them are in the same basic block start the renaming.
 *
 * @pre prerequisite for correct execution of this method is that the \c varLiveAcrossBB array
 *      is already compiled with its informations.
 */
static void ssa_convert_resolveDoubleDefinitions(GlobalObjects* glob) {
    JITUINT32         trigger;
    JITNUINT          temp;
    ir_method_t*      method         = glob->method;
    JITNUINT          livenessBlocks = glob->varLiveAcrossBBLen;
    ir_instruction_t* one            = IRMETHOD_getFirstInstruction(method);
    ir_instruction_t* two            = IRMETHOD_getNextInstruction(method, one);
    JITNUINT*         alreadyDefVars = allocMemory(livenessBlocks * sizeof(JITNUINT));

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Starting trivial renaming\n", IRMETHOD_getMethodName(method));

    memset(alreadyDefVars, 0, livenessBlocks * sizeof(JITNUINT));

    while (two != NULL) {
        IR_ITEM_VALUE defByOne = IRMETHOD_getVariableDefinedByInstruction(method, one);
        IRBasicBlock*   bbOne    = IRMETHOD_getBasicBlockContainingInstruction(method, one);
        assert(bbOne != NULL);
        // continue while the pointer one does not point to the last instruction of the method

        // check if we are defining something
        if (defByOne != NOPARAM) {
            while (two != NULL && IRMETHOD_getInstructionPosition(two) <= bbOne->endInst) {
                // loop through all successors of instruction one that resides in its same basic block
                if (defByOne == IRMETHOD_getVariableDefinedByInstruction(method, two)) {
                    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): \tinstr %d and instr %d both defines var %llu\n", IRMETHOD_getMethodName(method), one->ID, two->ID, defByOne);
                    // both instructions defines the same variable, and both instructions are in the same basic
                    // block: we rename the first-defined variable because it is killed by the second definition
                    IR_ITEM_VALUE oldVar = defByOne;
                    IR_ITEM_VALUE newVar = ssa_convert_getFreeVariableID(method);

                    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): \trename var %llu in var %llu\n", IRMETHOD_getMethodName(method), oldVar, newVar);
                    // replace the definition of oldVar with a definition of newVar, the uses of this var
                    // in this instruction must remain untouched
                    ssa_convert_renameVarInInstrIfDefined(method, one, oldVar, newVar);

                    ir_instruction_t* three = IRMETHOD_getNextInstruction(method, one);
                    for (; three != two; three = IRMETHOD_getNextInstruction(method, three)) {
                        // replace any occurrence of oldVar in all the instructions that are between these two definitions
                        // we have just found. Note that in these instructions oldVar is never defined, it is only used
                        ssa_convert_renameVarInInstrIfUsed(glob, three, oldVar, newVar);
                    }

                    assert(three == two);

                    // rename the uses in the instruction that redefines oldVar
                    ssa_convert_renameVarInInstrIfUsed(glob, three, oldVar, newVar);

                    // now 'one' define the variable 'newVar'
                    defByOne = newVar;
                    break;
                }

                two = IRMETHOD_getNextInstruction(method, two);
                assert(two != NULL);
            }

            /* 'one' loops through all the instructions of the method, except the STOP() *
             * here we want to check if there are some definitions that do not reach the *
             * end of its basic block, but still redefines one already defined variable. *
             * In this situation the rename is quite simple: in fact we can stop the     *
             * rename when we reach the end of the current basic block.                  */
            trigger = defByOne / (sizeof(JITNUINT) * 8);
            if (trigger < livenessBlocks) {
                // if this condition does not hold means that the variable we are considering
                // was created by this plugin and so it is for sure not double-defined
                temp    = (JITNUINT)0x1 << (defByOne % (sizeof(JITNUINT) * 8));
                if ((~(glob->varLiveAcrossBB[trigger]) & alreadyDefVars[trigger] & temp) != 0) {
                    // 1. the variable is not live across some basic blocks
                    // 2. the variable is already defined in one previous instruction
                    // 3. the variable is defined in this instruction
                    // we have to rename it and all its uses in this basic block. The first
                    // condition ensures that we do not have to go out of this basic block during
                    // the rename phase
                    IR_ITEM_VALUE newVar = ssa_convert_getFreeVariableID(method);
                    ssa_convert_renameVarInInstrIfDefined(method, one, defByOne, newVar);
                    two = IRMETHOD_getNextInstruction(method, one);
                    while (two != NULL && IRMETHOD_getInstructionPosition(two) <= bbOne->endInst) {
                        ssa_convert_renameVarInInstrIfUsed(glob, two, defByOne, newVar);
                        two = IRMETHOD_getNextInstruction(method, two);
                    }
                } else {
                    alreadyDefVars[trigger] |= temp;
                }
            }
        }

        one = IRMETHOD_getNextInstruction(method, one);
        two = IRMETHOD_getNextInstruction(method, one);
    }
    freeMemory(alreadyDefVars);
    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Trivial renaming complete\n", IRMETHOD_getMethodName(method));
}

/**
 * @brief return the ID of a variable never used, ensuring that USE set is able to keep track also of this new variable
 *
 * After invocation of \c newVariableID this function checks if the number of blocks needed for storing liveness
 * informations is now increased. If yes it scans all method instructions allocating for each instruction one new
 * array large enough for storing liveness informations and copy old data to the new location.
 */
static IR_ITEM_VALUE ssa_convert_getFreeVariableID(ir_method_t* method) {
    IR_ITEM_VALUE newVar = IRMETHOD_newVariableID(method);
    return newVar;
}

/**
 * @brief Rename the variable that is defined by this instruction if it matches with oldVariable
 *
 * If the variable defined by \c instr matches \c oldVariable the method rename that variable with \c newVariable.
 * Also informations in the \c DEF set are updated.
 * @note the method does not update the varInfoList structure.
 *
 * @param instr       pointer to the instruction we are considering
 * @param oldVariable the variable that must be renamed
 * @param newVariable the variable that we want to use to replace \c oldVariable
 */
static void ssa_convert_renameVarInInstrIfDefined(ir_method_t* method, ir_instruction_t* instr, IR_ITEM_VALUE oldVariable, IR_ITEM_VALUE newVariable) {
    ir_item_t  *result = IRMETHOD_getIRItemOfVariableDefinedByInstruction(method, instr);
    assert(result != NULL);

    if (	(result != NULL)			&&
            (result->value.v == oldVariable) 	) {
        result->value.v = newVariable;
        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Instruction %d, defined var %llu become var %llu\n", IRMETHOD_getMethodName(method), instr->ID, oldVariable, newVariable);
    }
}

/**
 * @brief Rename all variables used by this instruction if they matches with oldVariable
 *
 * If any variable defined by \c instr matches \c oldVariable the method rename that variable with \c newVariable.
 * Also informations in the \c USE set are updated.
 * @note If the instruction type is \c IRPHI the method does nothing. PHI instructions are treated separately.
 *
 * @param instr       pointer to the instruction we are considering
 * @param oldVariable the variable that must be renamed
 * @param newVariable the variable that we want to use to replace \c oldVariable
 */
static void ssa_convert_renameVarInInstrIfUsed(GlobalObjects* glob, ir_instruction_t* instr, IR_ITEM_VALUE oldVariable, IR_ITEM_VALUE newVariable) {
    ir_method_t* method      = glob->method;
    ir_item_t  *par1;

    if (IRMETHOD_getInstructionType(instr) == IRLABEL) {
        return;    // labels does not access variables
    }
    if (IRMETHOD_getInstructionType(instr) == IRPHI) {
        return;    // phi functions are not managed
    }

    par1   = IRMETHOD_getInstructionParameter1(instr);
    assert(par1 != NULL);

    if (par1->type == IROFFSET) {
        // here the first parameter is used, not defined
        if (par1->value.v == oldVariable) {
            PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): %d) used var %llu (parameter 1) become var %llu\n", IRMETHOD_getMethodName(method), instr->ID, oldVariable, newVariable);
            par1->value.v = newVariable;
        }
    }

    switch (IRMETHOD_getInstructionParametersNumber(instr)) {
        case 3:
            if (IRMETHOD_getInstructionParameter3Type(instr) == IROFFSET) {
                IR_ITEM_VALUE par3Value = IRMETHOD_getInstructionParameter3Value(instr);
                if (par3Value == oldVariable) {
                    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): %d) used var %llu (parameter 3) become var %llu\n", IRMETHOD_getMethodName(method), instr->ID, oldVariable, newVariable);
                    IRMETHOD_setInstructionParameter3Value(method, instr, newVariable);
                }
            }
        case 2:
            if (IRMETHOD_getInstructionParameter2Type(instr) == IROFFSET) {
                IR_ITEM_VALUE par2Value = IRMETHOD_getInstructionParameter2Value(instr);
                if (par2Value == oldVariable) {
                    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): %d) used var %llu (parameter 2) become var %llu\n", IRMETHOD_getMethodName(method), instr->ID, oldVariable, newVariable);
                    IRMETHOD_setInstructionParameter2Value(method, instr, newVariable);
                }
            }
    }

    switch (instr->type) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL: {
            JITUINT32 count;
            for (count = 0; IRMETHOD_getInstructionCallParameterType(instr, count) != NOPARAM; count++) {
                if (IRMETHOD_getInstructionCallParameterType(instr, count) == IROFFSET) {
                    IR_ITEM_VALUE callParamValue = IRMETHOD_getInstructionCallParameterValue(instr, count);
                    if (callParamValue == oldVariable) {
                        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): %d) used var %llu (call parameter) become var %llu\n", IRMETHOD_getMethodName(method), instr->ID, oldVariable, newVariable);
                        IRMETHOD_setInstructionCallParameterValue(method, instr, count, newVariable);
                        break;
                    }
                }
            }
        }
    }
}

/**
 * @brief initialize a previously allocated bitset with the variables used by one instruction
 *
 * Like the liveness informations: each bit of the array is related to one variable. If the bit
 * is set to \c 1 the variable is used in the instruction, else it is not used.
 *
 * @param method    pointer to method informations
 * @param instr     the instruction we want to know the used variables
 * @param bitset    the bitset used for storing the result of the function
 * @param bitsetLen length of the array containing the bitset
 */
static void ssa_convert_getUsedVariablesByInstruction(ir_method_t* method, ir_instruction_t* instr, JITNUINT* bitset, JITNUINT bitsetLen) {
    JITNUINT trigger;
    JITNUINT temp;

    memset(bitset, 0, sizeof(JITNUINT) * bitsetLen);

    // manage first parameter of non-store instructions
    if (IRMETHOD_getInstructionParameter1Type(instr) == IROFFSET) {
        IR_ITEM_VALUE par1Value = IRMETHOD_getInstructionParameter1Value(instr);
        trigger = par1Value / (sizeof(JITNUINT) * 8);
        temp    = (JITNUINT)0x1 << (par1Value % (sizeof(JITNUINT) * 8));
        if (trigger < bitsetLen) {
            // otherwise we don't need use informations
            bitset[trigger] |= temp;
        }
    }

    // manage second and third instruction parameters
    switch(IRMETHOD_getInstructionParametersNumber(instr)) {
        case 3:
            if (IRMETHOD_getInstructionParameter3Type(instr) == IROFFSET) {
                IR_ITEM_VALUE par3Value = IRMETHOD_getInstructionParameter3Value(instr);
                trigger = (par3Value / (sizeof(JITNUINT) * 8));
                temp    = (JITNUINT)0x1 << (par3Value % (sizeof(JITNUINT) * 8));
                if (trigger < bitsetLen) {
                    // otherwise we don't need use informations
                    bitset[trigger] |= temp;
                }
            }
        case 2:
            if (IRMETHOD_getInstructionParameter2Type(instr) == IROFFSET) {
                IR_ITEM_VALUE par2Value = IRMETHOD_getInstructionParameter2Value(instr);
                trigger = (par2Value / (sizeof(JITNUINT) * 8));
                temp    = (JITNUINT)0x1 << (par2Value % (sizeof(JITNUINT) * 8));
                if (trigger < bitsetLen) {
                    // otherwise we don't need use informations
                    bitset[trigger] |= temp;
                }
            }
    }

    // manage instruction call parameters
    switch(IRMETHOD_getInstructionType(instr)) {
        case IRCALL:
        case IRNATIVECALL:
        case IRLIBRARYCALL:
        case IRVCALL:
        case IRICALL: {
            JITUINT32 count = 0;
            while(IRMETHOD_getInstructionCallParameterType(instr, count) != NOPARAM) {
                if (IRMETHOD_getInstructionCallParameterType(instr, count) == IROFFSET) {
                    IR_ITEM_VALUE callParamValue = IRMETHOD_getInstructionCallParameterValue(instr, count);
                    trigger = (callParamValue / (sizeof(JITNUINT) * 8));
                    temp    = (JITNUINT)0x1 << (callParamValue % (sizeof(JITNUINT) * 8));
                    if (trigger < bitsetLen) {
                        // otherwise we don't need use informations
                        bitset[trigger] |= temp;
                    }
                }
                count++;
            }
            break;
        }
    }
}

/**
 * @brief compute the list of vars that are live_in for some basic block
 *
 * Loop through all basic blocks listed in dominance tree and scans them
 * instructions backward for computing all variables that are live_in for the
 * basic block.
 */
static void ssa_convert_initVarLiveAcrossBB(GlobalObjects* glob) {
    ir_method_t*      method             = glob->method;
    ir_instruction_t* instr              = NULL;
    JITUINT32         blocksNumber       = LIVENESS_BLOCKS_NUMBER(method); // number of JITNUINT in the array varLiveAcrossBB
    JITNUINT*         varLiveAcrossBBLoc = NULL; // bitset that identifies the vars that are live_in in THIS basic block
    JITNUINT*         out                = NULL; // bitset returned by this function
    JITNUINT*         bitsetInstr        = NULL; // bitset related to the current variable
    BasicBlockInfo**  bbInfos            = glob->bbInfos;
    JITNUINT          bbInfosLen         = glob->bbInfosLen;
    JITNUINT          count, temp;

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): start looking for vars live_in in some basic blocks\n", IRMETHOD_getMethodName(method));

    // init the container of the data
    varLiveAcrossBBLoc = allocMemory(blocksNumber * sizeof(JITNUINT));
    out                = allocMemory(blocksNumber * sizeof(JITNUINT));
    bitsetInstr        = allocMemory(blocksNumber * sizeof(JITNUINT));
    memset(varLiveAcrossBBLoc, 0, blocksNumber * sizeof(JITNUINT));
    memset(out,                0, blocksNumber * sizeof(JITNUINT));
    memset(bitsetInstr,        0, blocksNumber * sizeof(JITNUINT));

    for (count = 0; count < bbInfosLen; count++) {
        BasicBlockInfo* bb = bbInfos[count];//bbInfoList->data(bbInfoList, listItem);
        instr = bb->endInst; // get last instruction of the basic block
        do { // scan all the instructions backward in the basic block
            assert(instr != NULL);

            // one definition starts the liveness path of the var, so this var is live before this
            // assignment only if it is used in this instruction or in one of the previous ones.
            IR_ITEM_VALUE definedVar = IRMETHOD_getVariableDefinedByInstruction(method, instr);
            if (definedVar != NOPARAM) {
                // if is defined some var in this instruction...
                JITNUINT trigger = (definedVar / (sizeof(JITNUINT) * 8));
                temp = (JITNUINT)0x1 << (definedVar % (sizeof(JITNUINT) * 8));
                temp = ~temp; // negate all bits: the only bit equal to zero is the one associated with the var defined here
                varLiveAcrossBBLoc[trigger] &= temp;
            }

            /* get the bitset of variables used in this instruction */
            ssa_convert_getUsedVariablesByInstruction(method, instr, bitsetInstr, blocksNumber);

            // now consider the used vars: each use of one var implies that that var is live before this instruction
            // and so we must add it to the set of the vars live_in
            for (temp = 0; temp < blocksNumber; temp++) {
                varLiveAcrossBBLoc[temp] |= bitsetInstr[temp];
            }

            instr = IRMETHOD_getPrevInstruction(method, instr);
        } while (instr != NULL && IRMETHOD_getInstructionPosition(instr) >= IRMETHOD_getInstructionPosition(bb->startInst));

        // completed the basic block it's time to update the global set and reset temporaries
        for (temp = 0; temp < blocksNumber; temp++) {
            out[temp] |= varLiveAcrossBBLoc[temp];
            varLiveAcrossBBLoc[temp] = 0;
        }
    }

    freeMemory(varLiveAcrossBBLoc);
    freeMemory(bitsetInstr);
    glob->varLiveAcrossBB    = out;
    glob->varLiveAcrossBBLen = blocksNumber;
    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): search complete\n", IRMETHOD_getMethodName(method));
}

/**
 * @brief Adds one define instruction for one variable in varInfoList
 *
 * Scan the varInfoList until it find the variable var, so adds the instruction passed ad second parameter
 * to the list of definition points of that variable. If it can't find the XanListItem relative to this variable
 * it creates one brand new item and initializes it with the right values.
 *
 * @param var         the variable that we have found that is defined in this instruction
 * @param varInfoList pointer to \c XanList of all \c BasicBlockInfo of the method
 * @param instr       pointer to the instruction that defines the variable var
 */
static void addDefPoint(ir_method_t* method, XanList* varInfoList, IR_ITEM_VALUE var, ir_instruction_t* instr) {
    assert(var >= 0);
    assert(instr != NULL);
    assert(IRMETHOD_getVariableDefinedByInstruction(method, instr) == var);

    XanListItem* item = xanList_first(varInfoList);
    VarInfo* info = NULL;
    while (item != NULL) {
        info = (VarInfo*) xanList_getData(item);
        if (info->var->value.v == var) {
            xanList_append(info->defPoints, instr);
            if (info->var->type == NOPARAM) {
                // we have not yet copied the variable informations,
                // so let's copy them right now
                if      (CHECK_INSTR_RES_IS_VAR (instr, var)) {
                    memcpy(info->var, IRMETHOD_getInstructionResult(instr),     sizeof(ir_item_t));
                } else if (CHECK_INSTR_PAR1_IS_VAR(instr, var)) {
                    memcpy(info->var, IRMETHOD_getInstructionParameter1(instr), sizeof(ir_item_t));
                } else if (CHECK_INSTR_PAR2_IS_VAR(instr, var)) {
                    memcpy(info->var, IRMETHOD_getInstructionParameter2(instr), sizeof(ir_item_t));
                } else if (CHECK_INSTR_PAR3_IS_VAR(instr, var)) {
                    memcpy(info->var, IRMETHOD_getInstructionParameter3(instr), sizeof(ir_item_t));
                }
            }
            return;
        }
        item = item->next;
    }

    // XanListItem not found: we must create a new one and append it to the list
    info = allocMemory(sizeof(VarInfo));
    memset(info, 0, sizeof(VarInfo));
    xanList_append(varInfoList, info);

    // initialize the ir_item_t container
    ir_item_t* thisVar = NULL;
    if      (CHECK_INSTR_RES_IS_VAR (instr, var)) {
        thisVar = IRMETHOD_getInstructionResult(instr);
    } else if (CHECK_INSTR_PAR1_IS_VAR(instr, var)) {
        thisVar = IRMETHOD_getInstructionParameter1(instr);
    } else if (CHECK_INSTR_PAR2_IS_VAR(instr, var)) {
        thisVar = IRMETHOD_getInstructionParameter2(instr);
    } else if (CHECK_INSTR_PAR3_IS_VAR(instr, var)) {
        thisVar = IRMETHOD_getInstructionParameter3(instr);
    }
    info->var = allocMemory(sizeof(ir_item_t));
    if (thisVar != NULL) {
        memcpy(info->var, thisVar, sizeof(ir_item_t));
    } else {
        // this first definition found is done by a NOP, so we cannot get the variable
        // informations from the instruction but we must wait for the next definition
        info->var->value.v = var; // set var ID
        info->var->type  = NOPARAM; // set temporary type
    }
    info->defPoints = xanList_new(malloc, free, NULL);
    xanList_append(info->defPoints, instr);
    info->renameStack = NULL; // it is allocated when the first definition of this var is reached by the renaming phase
}

/**
 * @brief Scan all code keeping track of def and use points of each var live_in in some basic block
 *
 * This method scan through all basic blocks present in dominance tree, and in
 * each basic block analyze all instructions, lookig if one of them defines or
 * uses one var listed in \c varLiveAcrossBB. If it find that one instruction
 * define one of these variables it adds a pointer to this instruction in the
 * relative field of varInfoList structure.
 */
static void ssa_convert_init_varInfoList(GlobalObjects* glob) {
    XanList*          varInfoList     = xanList_new(malloc, free, NULL);
    BasicBlockInfo**  bbInfos         = glob->bbInfos;
    JITNUINT          bbInfosLen      = glob->bbInfosLen;
    ir_method_t*      method          = glob->method;
    JITNUINT*         varLiveAcrossBB = glob->varLiveAcrossBB;
    JITNUINT          count;

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): start initialization of varInfoList\n", IRMETHOD_getMethodName(method));

    // scan all reachable basic blocks
    for (count = 0; count < bbInfosLen; count++) {
        BasicBlockInfo*   bb    = bbInfos[count];
        ir_instruction_t* instr = bb->startInst;
        // scan all instructions of the basic block
        do {
            // check defined variable
            JITNUINT defVar = IRMETHOD_getVariableDefinedByInstruction(method, instr); ///< var defined in this instruction
            if (defVar != NOPARAM) {
                JITNUINT trigger = (defVar / (sizeof(JITNUINT) * 8));
                if (trigger < glob->varLiveAcrossBBLen) {
                    JITNUINT temp = (JITNUINT)0x1 << (defVar % (sizeof(JITNUINT) * 8));
                    if ((temp & varLiveAcrossBB[trigger]) != 0) {
                        // the var defined is live_in in some basic blocks so we must keep track of it
                        PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): \t%03d) DEF point of var %lu\n", IRMETHOD_getMethodName(method), instr->ID, defVar);
                        addDefPoint(method, varInfoList, defVar, instr);
                    }
                }
            }
            instr = IRMETHOD_getNextInstruction(method, instr);
        } while (instr != NULL && instr->ID <= bb->endInst->ID);
    }

    // reset the array varLiveAcrossBB in order to be reused
    memset(varLiveAcrossBB, 0, glob->varLiveAcrossBBLen * sizeof(JITNUINT));

    // work simplification: if one var is live_in in some basic blocks but the method
    // has only one definition of it we never want to place PHI functions for that
    // variable, because there is only that definition that reach any possibile use point
    XanListItem* listItem = xanList_first(varInfoList);
    while (listItem != NULL) {
        VarInfo* varInfo = xanList_getData(listItem);
        if (xanList_length(varInfo->defPoints) <= 1) {
            ssa_convert_deleteVarInfo(varInfoList, varInfo);
            XanListItem* toDelete = listItem;
            listItem = listItem->next;
            xanList_deleteItem(varInfoList, toDelete);
        } else {
            /* here we want also to reuse the array varLiveAcrossBB in order to keep a   */
            /* fast bitset of all the variables that are present in the list varInfoList */
            JITNUINT trigger = varInfo->var->value.v / (sizeof(JITNUINT) * 8);
            assert(trigger < glob->varLiveAcrossBBLen); // no variables defined by this plugin may stay in varInfoList
            JITNUINT temp    = (JITNUINT)0x1 << (varInfo->var->value.v % (sizeof(JITNUINT) * 8));
            assert((varLiveAcrossBB[trigger] & temp) == 0); // there are no two varInfo related to the same variable
            varLiveAcrossBB[trigger] |= temp;

            listItem = listItem->next;
        }
    }
    glob->varInfoList = varInfoList;
    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): initialization complete\n", IRMETHOD_getMethodName(method));
}

/**
 * @brief deallocate one VarInfo structure with its fields
 */
static inline void ssa_convert_deleteVarInfo (XanList* varInfoList, VarInfo* toDelete) {
    if (toDelete->var != NULL) {
        freeMemory(toDelete->var);
    }
    if (toDelete->defPoints != NULL) {
        xanList_destroyList(toDelete->defPoints);
    }
    if (toDelete->renameStack != NULL) {
        while (xanStack_getSize(toDelete->renameStack) > 0) {
            freeMemory(xanStack_pop(toDelete->renameStack));
        }
        xanStack_destroyStack(toDelete->renameStack);
    }
    freeMemory(toDelete);
}

/**
 * @brief Compute the dominance frontier of the basic block passed as a second parameter
 *
 * This function should be called with the root node of the dominance tree passed as first parameter.
 * It use one XanList of pointer to all basic blocks that are in the dominance frontier of the one passed as parameter.
 *
 * \code
 * definitions:
 * - DFlocal[n] = the successors of n not strictly dominated by n
 * - DFup[n]    = the other nodes in the dominance frontier of n
 * - idom(n)    = the immediate dominator of the node n, i.e. the parent of node n in the dominance tree
 *
 * for each node in CFG {
 * 	S = {}
 * 	// This loop computes DFlocal[n]
 * 	for each node y in succ[n]
 * 		if (idom(y) != n)
 * 			S = S + {y}
 * 	for each child c of n in the dominator tree {
 * 		computeDF(c)
 * 		// This loop computes DFup[c]
 * 		for each element w of DF[c]
 * 			if ((n does not dominate w) or (n == w))
 * 				S = S + {w}
 * 	}
 * 	DF[n] = S
 * }
 * \endcode
 *
 * @param glob   pointer to the structure containing all "global" objects
 * @param myRoot the root for the recursive call of the method, if NULL it consider as root the one in the object \c glob
 */
static void ssa_convert_computeDF(GlobalObjects* glob, XanNode* myRoot) {
    XanNode*         root       = myRoot == NULL ? glob->root : myRoot;
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITFALSE || defined(PRINTDEBUG)
    ir_method_t*     method     = glob->method;
#endif

    assert(root != NULL);
    BasicBlockInfo* myBBInfo = root->getData(root);

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): (%d -> %d) start computing dominance frontier\n", IRMETHOD_getMethodName(method), myBBInfo->startInst->ID, myBBInfo->endInst->ID);

    // extract dominance frontier of root node (if exists)
    if (myBBInfo->domFront == NULL) {
        myBBInfo->domFront = xanList_new(malloc, free, NULL);
    }
    XanList*        myDF     = myBBInfo->domFront;

    /*** compute the DFlocal set of root node ***/
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
    XanListItem*      succ = xanList_first(myBBInfo->succList);
#else
    ir_instruction_t* last = myBBInfo->endInst; // last instruction of root basic block
    assert(last != NULL);
    ir_instruction_t* succ = IRMETHOD_getSuccessorInstruction(method, last, NULL); // the first successor of "last"
#endif
    while (succ != NULL) {
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
        BasicBlockInfo* bbSuccInfo = xanList_getData(succ);
#else
        // get the BasicBlockInfo relative to succ
        BasicBlockInfo* bbSuccInfo = ssa_convert_getBasicBlockInfoFromInstruction(glob->bbInfos, glob->bbInfosLen, succ);
        assert(bbSuccInfo != NULL);
#endif
        // now find the basic block in the dominance tree and verify if its parent is root
        XanNode* bbSuccNode = root->find(root, bbSuccInfo);
        if (bbSuccNode == NULL || (bbSuccNode != NULL && bbSuccNode->getParent(bbSuccNode) != root)) {
            // bbSuccNode is NULL only if bbSuccInfo is not in the subtree rooted by "root"
            // in this case its parent is not root, so the basic block is in the dominance frontier of root
            if (xanList_find(myDF, bbSuccInfo) == NULL) {
                xanList_append(myDF, bbSuccInfo);
            }
        }

#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
        succ = succ->next;
#else
        succ = IRMETHOD_getSuccessorInstruction(method, last, succ); // get the next successor of "last"
#endif
    }

    /*** compute the DFup set of root node ***/
    XanNode *child = root->getNextChildren(root, NULL);
    for (; child != NULL; child = root->getNextChildren(root, child)) {
        ssa_convert_computeDF(glob, child);
        BasicBlockInfo* childInfo = child->getData(child); // get the BasicBlockInfo of child
        XanList*        childDF   = childInfo->domFront;   // get the dominance frontier of that basic block
        assert(childDF != NULL);
        // scan each node of the child's dominance frontier to verify if it's also in the dominance frontier of root
        XanListItem* childDFItm = xanList_first(childDF);
        for (; childDFItm != NULL; childDFItm = childDFItm->next) {
            BasicBlockInfo* childBB = xanList_getData(childDFItm);
            XanNode*        node    = root->find(root, childBB); // now find if that basic block is dominated by root
            if (node == NULL || node == root) {
                // if the root does not dominates this basic block means that child is also in the dominance frontier of root
                if (xanList_find(myDF, childBB) == NULL) {
                    xanList_append(myDF, childBB);
                }
            }
        }
    }
    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): (%d -> %d) dominance frontier complete\n", IRMETHOD_getMethodName(method), myBBInfo->startInst->ID, myBBInfo->endInst->ID);
}

/**
 * @brief Search which basic block contains the instruction passed as fourth parameter
 *
 * Scan the array \c bbInfos until it finds the \c BasicBlockInfo that contains the instruction \c instr.
 *
 * @param  bbInfos       pointer to the array containing all the pointers to \c BasicBlockInfo structures
 * @param  bbInfosLength number of elements in the array \c bbInfos
 * @param  instr         pointer to the instruction which we want to find the containing \c BasicBlockInfo structure
 * @return pointer to \c BasicBlockInfo structure
 */
static BasicBlockInfo* ssa_convert_getBasicBlockInfoFromInstruction (BasicBlockInfo**  bbInfos,
        JITUINT32         bbInfosLength,
        ir_instruction_t* instr) {
    assert(instr != NULL);

    JITUINT32 i;
    for (i = 0; i < bbInfosLength; i++) {
        BasicBlockInfo* bbInfo = bbInfos[i];

        assert(bbInfo->startInst != NULL);
        assert(bbInfo->endInst != NULL);

        if (IRMETHOD_getInstructionPosition(instr) >= IRMETHOD_getInstructionPosition(bbInfo->startInst) &&
                IRMETHOD_getInstructionPosition(instr) <= IRMETHOD_getInstructionPosition(bbInfo->endInst)) {
            // if the position of the instruction instr is between the start and the end instruction
            // of the actual basic block we have found the basic block that contains this instruction
            return bbInfo;
        }
    }
    return NULL;
}

/**
 * @brief Method that scan all the code of the function and place PHI function where needed
 *
 * This function implements one algorithm similar to algorithm 19.6 of Appel's book.
 *
 * \code
 * definitions:
 * 	Aphi[a]     = the set of nodes that must have phi-function for variable a
 * 	Aorig[n]    = the set of variables defined in node n
 * 	defsites[a] = the set of nodes that contains at least one definition of variable a
 *
 * place-phi-functions() {
 * 	for each variable a (that is live_in in at least one basic block) {
 * 		W = defsites[a]
 * 		while W not empty {
 * 			remove some node n from W
 * 			for each y in DF[n] {
 * 				if (y not in Aphi[a]) {
 * 					insert the statement a = phi(a, a,..., a) at the top of block y, where the phi-function has as many arguments as y has predecessors
 * 					Aphi[a] = Aphi[a] + {y}
 * 					if (a not in Aorig[y]) W = W + {y}
 * 				}
 * 			}
 * 		}
 * 	}
 * }
 * \endcode
 *
 */
static void ssa_convert_place_phi_functions(GlobalObjects* glob) {
    ir_method_t*     method      = glob->method;
    XanList*         varInfoList = glob->varInfoList;
    BasicBlockInfo** bbInfos     = glob->bbInfos;
    JITUINT32        bbInfosLen  = glob->bbInfosLen;
    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): Start placing PHI functions\n", IRMETHOD_getMethodName(method));

    // loop through all items in varInfoList
    XanListItem* item = xanList_first(varInfoList);
    for (; item != NULL; item = item->next) {
        VarInfo* var = xanList_getData(item);
        assert(var != NULL);
        assert(var->defPoints != NULL);

        // we have to examine this var because is live_in in at least in one basic block
        // loop through all instructions that defines this var
        XanList*     defList     = var->defPoints;
        XanListItem* defListItem = xanList_first(defList);
        for (; defListItem != NULL; defListItem = defListItem->next) {
            // now let's extract the dominance frontier of the basic block containing this definition point

            // get the BasicBlockInfo relative to succ
            BasicBlockInfo* bbInfo = ssa_convert_getBasicBlockInfoFromInstruction(bbInfos, bbInfosLen, xanList_getData(defListItem));

            // if bbInfo is NULL means that from the root of the dominance tree
            // we cannot reach the basic block containig this definition point
            if (bbInfo == NULL) {
                // remove the definition point because it cannot be executed
                xanList_deleteItem(defList, defListItem);
            } else {
                assert(bbInfo != NULL);
                assert(bbInfo->domFront != NULL);

                // scan its dominance frontier looking if it is necessary to add PHI function for this var
                XanList*     bbDF     = bbInfo->domFront; ///< list of basic blocks in dominance frontier
                XanListItem* bbDFItem = xanList_first(bbDF);
                for (; bbDFItem != NULL; bbDFItem = bbDFItem->next) {
                    BasicBlockInfo* bbInfoDF = xanList_getData(bbDFItem); ///< the basic block in the dominance frontier
                    assert(bbInfoDF != NULL);
                    // it is useless to place phi functions in the exit node of the method
                    if (IRMETHOD_getInstructionType(bbInfoDF->startInst) != IREXITNODE) {
                        // now we can verify if the PHI for this variable is alredy present or not
                        ir_instruction_t* instr     = bbInfoDF->startInst;
                        ir_instruction_t* firstInst = NULL; ///< the first useful instruction after NOPs e labels
                        assert(instr != NULL);
                        // skip all the NOPs and the LABELs, PHI must placed after them
                        // skip also the eventual starter of a catch block or a finally block
                        while (((IRMETHOD_getInstructionType(instr) == IRLABEL       ) ||
                                (IRMETHOD_getInstructionType(instr) == IRSTARTCATCHER) ||
                                (IRMETHOD_getInstructionType(instr) == IRSTARTFINALLY) ||
                                (IRMETHOD_getInstructionType(instr) == IRNOP         )  ) &&
                                (IRMETHOD_getInstructionPosition(instr) <= IRMETHOD_getInstructionPosition(bbInfoDF->endInst))) {
                            assert(instr->type != IRSTARTFILTER);
                            assert(instr->type !=   IRENDFILTER);
                            assert(instr->type !=   IRENDFINALLY);
                            instr = IRMETHOD_getNextInstruction(method, instr);
                        }
                        firstInst = instr;

                        assert(instr != NULL);
                        JITUINT8 found = JITFALSE; ///< JITTRUE if we found the PHI that defines our variable
                        while (!found && IRMETHOD_getInstructionType(instr) == IRPHI) {
                            // test if this PHI defines our variable
                            found = (IRMETHOD_getInstructionParameter1Value(instr) == var->var->value.v);
                            instr = IRMETHOD_getNextInstruction(method, instr);
                        }

                        if (!found) { // we have not found any PHI that defines our variable, so we must create a new one
                            // the new PHI must be added as first useful instruction of the block,
                            ir_instruction_t* newInst = IRMETHOD_newInstructionBefore(method, firstInst);
                            assert(newInst != NULL);

                            // check if the new instruction is placed before the start instruction or after the last instruction of the target basic block
                            if (bbInfoDF->startInst == firstInst) {
                                bbInfoDF->startInst = newInst;
                            }
                            if (IRMETHOD_getInstructionPosition(bbInfoDF->endInst) +1 == IRMETHOD_getInstructionPosition(newInst)) {
                                bbInfoDF->endInst = newInst;
                            }
                            assert(ssa_convert_getBasicBlockInfoFromInstruction(bbInfos, bbInfosLen, newInst) == bbInfoDF);

                            // the phi must have a number of call parameters equal to the number of predecessors of its basic block
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
                            assert(bbInfoDF->predList != NULL);
                            JITUINT32 numPredecessors = xanList_length(bbInfoDF->predList);
#else
                            JITUINT32 numPredecessors = IRMETHOD_getPredecessorsNumber(method, bbInfoDF->startInst);
#endif
                            while (numPredecessors > IRMETHOD_getInstructionCallParametersNumber(newInst)) {
                                ir_item_t	*callPar;
                                JITUINT32 par_num 	= IRMETHOD_newInstructionCallParameter(method, newInst);
                                // initialize the call parameter:
                                IRMETHOD_cpInstructionCallParameter(method, newInst, par_num, var->var);
                                callPar			= IRMETHOD_getInstructionCallParameter(newInst, par_num);
                                callPar->type		= callPar->internal_type;
                                callPar->value.v	= 0;
                                // now the call parameter is the same type of the variable "var", but it is the zero constant instead that a reference to that var
                            }
                            assert(numPredecessors == IRMETHOD_getInstructionCallParametersNumber(newInst));

                            // now this method has at least one phi function so we need to run ssa back covnversion before trying to execute it
                            // method->hasPhiFunctions = JITTRUE;

                            // if this var is alredy defined in this basic block we cannot define
                            // again the same var with the PHI, so we must scan the following instructions
                            // ensuring that this condition keeps
                            ir_instruction_t* instr     = firstInst; // the PHI was placed *before* firstInst
                            JITBOOLEAN        doubleDef = JITFALSE;
                            assert(bbInfoDF->endInst != NULL);
                            while (!doubleDef && instr != NULL && IRMETHOD_getInstructionPosition(instr) <= IRMETHOD_getInstructionPosition(bbInfoDF->endInst)) {
                                doubleDef = (IRMETHOD_getVariableDefinedByInstruction(method, instr) == var->var->value.v);
                                instr = IRMETHOD_getNextInstruction(method, instr);
                            }

                            // initialize the phi
                            IRMETHOD_setInstructionParameter1(method, newInst, var->var->value.v, 0, IRUINT32, IRUINT32, NULL); // we must keep track of the ID of the old variable involved (the one defined before renaming)
                            IRMETHOD_cpInstructionResult(newInst, var->var); // also initialize the result of the phi with the same variable. In case of double definition the variable ID of result will be updated

                            if (doubleDef) {
                                // this variable is now twice-defined in this basic block, so we must act renaming
                                // from the PHI to the next definition of the variable
                                IR_ITEM_VALUE     newVarId = ssa_convert_getFreeVariableID(method);
                                ir_instruction_t* instr    = newInst;
                                do {
                                    instr = IRMETHOD_getNextInstruction(method, instr);
                                    ssa_convert_renameVarInInstrIfUsed(glob, instr, var->var->value.v, newVarId);
                                } while (IRMETHOD_getVariableDefinedByInstruction(method, instr) != var->var->value.v);

                                // now let's define the variable with the PHI function
                                IRMETHOD_setInstructionParameterResultValue(method, newInst, newVarId);
                            } else {
                                // add this new definition to the list var->defPoints
                                xanList_append(var->defPoints, newInst);
                            }
                            // other initializations of the new PHI instruction
                            IRMETHOD_setInstructionType(method, newInst, IRPHI);
                            PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): %d) var %llu=PHI(var %llu)\n", IRMETHOD_getMethodName(method), newInst->ID, IRMETHOD_getVariableDefinedByInstruction(method, newInst), IRMETHOD_getInstructionParameter1Value(newInst));
                        }
                    }
                }
            }
        }
    }

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): PHI functions placed, updating basic blocks boundaries\n", IRMETHOD_getMethodName(method));
    // finally let's update the basic block boundaries (startInst and endInst)
    JITUINT32 i;
    for (i = 0; i < bbInfosLen; i++) {
        BasicBlockInfo* bbInfo = bbInfos[i];
        assert(bbInfo->startInst != NULL);
        assert(bbInfo->endInst != NULL);
        bbInfo->basicBlock->startInst = IRMETHOD_getInstructionPosition(bbInfo->startInst);
        bbInfo->basicBlock->endInst = IRMETHOD_getInstructionPosition(bbInfo->endInst);
    }
    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): boundaries updated\n", IRMETHOD_getMethodName(method));
}

/**
 * @brief rename each definition and each use of all variables in order to follow the SSA one definition rule
 *
 * It follows the recursive algorithm providen by the Appel's book and listed in the following pseudo-code.
 * \code
 * rename(node X) {
 * 	for each assignment A in node X {
 * 		// rename uses of V in non-phi assignment
 * 		if A is “ordinary”, non-phi assignment {
 * 			replace each use of V in A with Vi (where i = Top(S(V))
 * 		}
 *
 * 		// rename definitions of V on LHS of assignments (including phi–functions)
 * 		replace V in LHS(A) by new Vi (where i = C(V))
 * 		push i onto S(V)
 * 		increment counter at C(V)
 * 	}
 *
 * 	// j-th parameter of phi-function must correspond to j-th predecessor in control flow graph
 * 	for each Y in Succ(X) {
 * 		j = Predecessor# of X
 * 		for each phi-function F in Y {
 * 			replace j-th parameter V in F by Vi (where i = Top(S(V))
 * 		}
 * 	}
 *
 * 	// recursively walk down dominator tree
 * 	for each C in Children(X) call rename(C)
 *
 * 	// pop the renamed variables
 * 	for each assignment A in X {
 * 		pop(S(V)) (where V corresponds to V in original LHS(A))
 * 	}
 * }
 * \endcode
 */
static void ssa_convert_renameVarsInDomTree(GlobalObjects* glob, XanNode* myRoot) {
    XanNode*          root        = myRoot == NULL ? glob->root : myRoot;
    XanList*          varInfoList = glob->varInfoList;
    ir_method_t*      method      = glob->method;
    BasicBlockInfo*   bbInfo      = root->getData(root);
    ir_instruction_t* instr       = bbInfo->startInst;
    JITUINT32         pos         = 0;    ///< temporary for accessing the array popNumbers
    JITUINT32*        popNumbers  = NULL; ///< array that defines for each variable how many pops are necessary at the end of the method
    JITNUINT*         usedVars    = allocMemory(sizeof(JITNUINT) * glob->varLiveAcrossBBLen);

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): start rename on node (%d->%d)\n", IRMETHOD_getMethodName(method), bbInfo->startInst->ID, bbInfo->endInst->ID);
    popNumbers = allocMemory(sizeof(JITUINT32) * xanList_length(varInfoList));
    memset(popNumbers, 0, sizeof(JITUINT32) * xanList_length(varInfoList));

    if (instr == IRMETHOD_getFirstInstruction(method)) {
        /* The first instructions of the method defines its call parameters (of the method).     *
         * They are simply NOPs so we don't need to make renaming on them, but we still need     *
         * to update the rename stack if some of them are redefined in other parts of the method */
        pos = 0;
        JITNUINT     numOfMethodCallParameters = IRMETHOD_getMethodParametersNumber(method);
        XanListItem* varInfoItem               = xanList_first(varInfoList);
        while (varInfoItem != NULL) {
            VarInfo*   varInfo = xanList_getData(varInfoItem);
            ir_item_t* var     = varInfo->var;
            assert(var->type == IROFFSET);
            if (var->value.v < numOfMethodCallParameters) {
                IR_ITEM_VALUE* rename = allocMemory(sizeof(IR_ITEM_VALUE));
                assert(varInfo->renameStack == NULL); // because this is the very first definition of this variable
                varInfo->renameStack = xanStack_new(malloc, free, NULL);
                *rename = var->value.v;
                xanStack_push(varInfo->renameStack, rename);
                popNumbers[pos]++;
            }
            varInfoItem = varInfoItem->next;
            pos++;
        }
        // we can safely skip the first NOPs of the method
        instr = IRMETHOD_getInstructionAtPosition(method, numOfMethodCallParameters);
    }

    // skip the label (or similar instructions) that may be at the head of this basic block
    JITUINT16 instrType = IRMETHOD_getInstructionType(instr);
    if (instrType == IRLABEL        ||
            instrType == IRSTARTCATCHER ||
            instrType == IRSTARTFINALLY ||
            instrType == IRSTARTFILTER     ) {
        instr = IRMETHOD_getNextInstruction(method, instr);
    }

    // loop through all instructions of basic block
    while (instr != NULL && IRMETHOD_getInstructionPosition(instr) <= IRMETHOD_getInstructionPosition(bbInfo->endInst)) {
        /* check if the variables used and defined in this instruction are also in varInfoList */
        /* if not we can avoid scan the list. Also if the variable ID overflows the array */
        ssa_convert_getUsedVariablesByInstruction(method, instr, usedVars, glob->varLiveAcrossBBLen);
        IR_ITEM_VALUE varDefined  = IRMETHOD_getVariableDefinedByInstruction(method, instr);
        JITNUINT      trigger;
        JITBOOLEAN    doTheRename = JITFALSE;
        // check the defined var
        if (varDefined != NOPARAM) {
            trigger = varDefined / (sizeof(JITNUINT) * 8);
            // check that the variable is not defined by this plugin
            if (trigger < glob->varLiveAcrossBBLen) {
                // the right shift is used in order to move the bit related to varDefined
                // in the least significative bit, next the bitwise-and with 0x1 turns
                // all other bits to zero, so if we have varDefined in the varInfoList
                // we also have doTheRename equal to 0x1, otherwise it is equal to zero
                doTheRename = 0x1 & (glob->varLiveAcrossBB[trigger] >> (varDefined % (sizeof(JITNUINT) * 8)));
            }
        }
        // check the used vars
        for (trigger = 0; (!doTheRename) && trigger < glob->varLiveAcrossBBLen; trigger++)
            if ((usedVars[trigger] & glob->varLiveAcrossBB[trigger]) != 0) {
                doTheRename = JITTRUE;
            }

        if (doTheRename) {
            XanListItem*  varInfoItem = xanList_first(varInfoList);
            pos = 0; // first item in varInfoList
            while (varInfoItem != NULL) { // loop through all the "interesting" variables
                VarInfo*      varInfo = xanList_getData(varInfoItem);
                IR_ITEM_VALUE varID   = varInfo->var->value.v;

                // rename uses (only no PHI)
                if (IRMETHOD_getInstructionType(instr) != IRPHI) {
                    if (varInfo->renameStack != NULL && xanStack_getSize(varInfo->renameStack) > 0) {
                        XanList*       renameList = varInfo->renameStack->internalList;
                        XanListItem*   firstItem  = xanList_first(renameList);
                        IR_ITEM_VALUE* rename     = xanList_getData(firstItem); // the list item contains a pointer to the variable number

                        ssa_convert_renameVarInInstrIfUsed(glob, instr, varID, *rename);
                    }
                }

                // rename definitions
                if (varDefined == varID) {
                    IR_ITEM_VALUE* rename = allocMemory(sizeof(IR_ITEM_VALUE));
                    if (varInfo->renameStack == NULL) {
                        // first definition of this variable, no rename is needed
                        varInfo->renameStack = xanStack_new(malloc, free, NULL);
                        *rename = varID;
                    } else {
                        // the first definition of the variable var is already reached, this definition needs renaming
                        *rename = ssa_convert_getFreeVariableID(method);
                        ssa_convert_renameVarInInstrIfDefined(method, instr, varID, *rename);
                    }
                    xanStack_push(varInfo->renameStack, rename);
                    popNumbers[pos] += 1;
                }
                // look next variable
                pos++;
                varInfoItem = varInfoItem->next;
            }
        }
        instr = IRMETHOD_getNextInstruction(method, instr);
    }

    freeMemory(usedVars);

    // loop through all successors of the last instruction of the basic block
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
    XanListItem*      succ = xanList_first(bbInfo->succList);
#else
    ir_instruction_t* succ = IRMETHOD_getSuccessorInstruction(method, bbInfo->endInst, NULL);
#endif
    while (succ != NULL) {
        // for each successor we have to scan all the PHI functions and look for the ones that
        // are relative to our "interesting" variables.
        // Note that we DO NOT exit from the basic block we are considering
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
        BasicBlockInfo*   bbI   = xanList_getData(succ);
        ir_instruction_t* i     = bbI->startInst;
#else
        ir_instruction_t* i     = succ;
        BasicBlockInfo*   bbI   = ssa_convert_getBasicBlockInfoFromInstruction(glob->bbInfos, glob->bbInfosLen, i);
#endif

        // Tim: We don't need to worry about the exit node.
        if (i->type == IREXITNODE) {
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
            succ = succ->next;
#else
            succ = IRMETHOD_getSuccessorInstruction(method, bbInfo->endInst, succ);
#endif
            continue;
        }

#ifdef DEBUG
        if (bbI == NULL) {
            fprintf(stderr, "OPTIMIZER_SSA_CONVERTER: instruction %d is not in any basic block\n", i->ID);
            JITUINT32 basicBlock;
            for (basicBlock = 0; basicBlock < glob->bbInfosLen; basicBlock++) {
                BasicBlockInfo* bb = glob->bbInfos[basicBlock];
                fprintf(stderr, "OPTIMIZER_SSA_CONVERTER: basic block %d (%d -> %d)\n", basicBlock, bb->startInst->ID, bb->endInst->ID);
            }
        }
        assert(bbI != NULL);
#endif

        // we have to place the variable we are considering in a well defined position:
        // it must be at nth position, where bbInfo represents the nth predecessor in the CFG
        // note that zero is the first position
        JITUINT32         predecessorNumber = 0;
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
        XanListItem* iterator = xanList_first(bbI->predList);
        while (iterator != NULL) {
            BasicBlockInfo* bb = xanList_getData(iterator);
            if (bb != bbInfo) {
                predecessorNumber++;
                iterator = iterator->next;
            } else {
                break;
            }
        }
        assert(iterator != NULL);
#else
        ir_instruction_t* pred = IRMETHOD_getPredecessorInstruction(method, i, NULL);
        while (pred != NULL && pred != bbInfo->endInst) {
            predecessorNumber++;
            pred = IRMETHOD_getPredecessorInstruction(method, i, pred);
        }
#ifdef DEBUG
        assert(pred != NULL);
#endif
        assert(pred == bbInfo->endInst);
#endif

        while ((i != NULL) && // trivial
                (i == bbI->startInst || IRMETHOD_getInstructionType(i) == IRPHI)) { // this is not "active" code
            //		      && (IRMETHOD_getInstructionPosition(i) <= bbI->endInst)) { // we don't run out the basic block NOTE always true???
            assert(IRMETHOD_getInstructionPosition(i) <= IRMETHOD_getInstructionPosition(bbI->endInst));
            if (IRMETHOD_getInstructionType(i) == IRPHI) {
                XanList*     callParams  = i->callParameters; // get call parameters for this PHI
                XanListItem* varInfoItem = xanList_first(varInfoList);
                for (; varInfoItem != NULL; varInfoItem = varInfoItem->next) { // loop through all VarInfo
                    VarInfo* varInfo = xanList_getData(varInfoItem);
                    if (IRMETHOD_getInstructionParameter1Value(i) == varInfo->var->value.v) { // this phi is about the right variable
                        // get the nth call parameter of the phi in order to copy in it this VarInfo
                        XanListItem* callParam = xanList_getElementFromPositionNumber(callParams, predecessorNumber);
                        ir_item_t*   param     = xanList_getData(callParam);
                        if (param->type == IROFFSET) {
                            fprintf(stderr, "OPTIMIZER_SSA_CONVERTER: phi in position %d is double-defined in call parameter %d/%d\n", i->ID, predecessorNumber, xanList_length(callParams));
                        }
                        assert(param->type != IROFFSET); // assert that this call parameter is not previously written by someone else

                        if (varInfo->renameStack != NULL) {
                            if (xanStack_getSize(varInfo->renameStack) > 0) {
                                // copy the variable informations only if there is one rename stack for this varInfo and there are some
                                // IDs that renames varInfo->var->value.v
                                memcpy(param, varInfo->var, sizeof(ir_item_t));

                                // if the rename stack has more than one element we also have to apply the renaming
                                // (the first element is not a real rename, it's a copy of varInfo->var->value.v)
                                XanList*       renameList = varInfo->renameStack->internalList;
                                XanListItem*   firstItem  = xanList_first(renameList);
                                IR_ITEM_VALUE* rename     = xanList_getData(firstItem); // the list contains a pointer to the register name
                                IRMETHOD_setInstructionCallParameterValue(method, i, predecessorNumber, *rename);
                                PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): var rename: phi at %d, call parameter %d/%d renamed into %llu\n", method->name, i->ID, predecessorNumber, xanList_length(callParams), param->value.v);
                            }
                        }
                    }
                }
            }
            i = IRMETHOD_getNextInstruction(method, i);
        }

#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITTRUE
        succ = succ->next;
#else
        succ = IRMETHOD_getSuccessorInstruction(method, bbInfo->endInst, succ);
#endif
    }

    /* recursive call on all children of dominator tree */
    XanNode* child = root->getNextChildren(root, NULL);
    for (; child != NULL; child = root->getNextChildren(root, child)) {
        ssa_convert_renameVarsInDomTree(glob, child);
    }

    /* finally pop the rename stack as in the algorithm */
    XanListItem* varInfoItem = xanList_first(varInfoList);
    pos = 0;
    while (varInfoItem != NULL) {
        JITUINT32 i;
        VarInfo* varInfo = xanList_getData(varInfoItem);
        for (i = 0; i < popNumbers[pos]; i++) {
            JITUINT32* rename = xanStack_pop(varInfo->renameStack);
            assert(rename != NULL);
            freeMemory(rename);
        }
        varInfoItem = varInfoItem->next;
        pos++;
    }

    freeMemory(popNumbers);

    PDEBUG("OPTIMIZER_SSA_CONVERTER (%s): rename on node (%d->%d) complete\n", IRMETHOD_getMethodName(method), bbInfo->startInst->ID, bbInfo->endInst->ID);
}

/**
 * @brief deallocate temporary memory used by this plugin
 *
 * It free up the memory used for storing:
 * <ol>
 * <li> dominance informations </li>
 * <li> \c BasicBlockInfo structures and array of pointers </li>
 * <li> \c VarInfo structures </li>
 * <li> \c varLiveAcrossBB array </li>
 * </ol>
 */
static inline void ssa_convert_freeUpMemory(GlobalObjects* glob) {
    BasicBlockInfo** bbInfos     = glob->bbInfos;
    JITUINT32        bbInfosLen  = glob->bbInfosLen;
    XanList*         varInfoList = glob->varInfoList;
    XanListItem*     item        = NULL;
    JITUINT32        i;

    if (glob->root != NULL) {
        glob->root->destroyTree(glob->root);    // destroy dominance tree
    }

    // destroy the list of BasicBlockInfo structures
    for (i = 0; i < bbInfosLen; i++) {
        // delete BasicBlockInfo
        BasicBlockInfo* data = bbInfos[i];
        if (data->domFront != NULL) {
            xanList_destroyList(data->domFront);
        }
#if SSA_CONVERT_KEEP_PRED_SUCC_LISTS == JITFALSE
        if (data->predList != NULL) {
            xanList_destroyList(data->predList);
        }
        if (data->succList != NULL) {
            xanList_destroyList(data->succList);
        }
#endif
        freeMemory(data);
        // delete list item
        data = NULL;
    }
    freeMemory(glob->bbInfos);
    glob->bbInfos = NULL;
    glob->bbInfosLen = 0;

    // destroy the list of VarInfo structures
    if (varInfoList != NULL) {
        item = xanList_first(varInfoList);
        while (item != NULL) {
            ssa_convert_deleteVarInfo(varInfoList, xanList_getData(item));
            xanList_deleteItem(varInfoList, item);
            item = xanList_first(varInfoList);
        }

        xanList_destroyList(varInfoList);
    }

    // free up varLiveAcrossBB array
    if (glob->varLiveAcrossBB != NULL) {
        freeMemory(glob->varLiveAcrossBB);
    }
    glob->varLiveAcrossBB = NULL;
    glob->varLiveAcrossBBLen = 0;

    freeMemory(glob);
}


/**
 * Check a method that has been converted to SSA form.  Basically, this
 * checks that each variable is only defined once in the entire method
 * and that every variable is defined or is a parameter variable.
 */
static inline void
ssa_convert_checkMethod(ir_method_t *method) {
    XanHashTable *varDefTable;
    JITUINT32 i;
    char buf[256];

    /* A table of mappings from register to defining instrucion. */
    varDefTable = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    assert(varDefTable);

    /* Must update liveness before doing these checks. */
    IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);

    /* Check each instruction. */
    for (i = 0; i < IRMETHOD_getInstructionsNumber(method); ++i) {
        ir_instruction_t *prevDef;
        ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, i);
        IR_ITEM_VALUE defVar = IRMETHOD_getVariableDefinedByInstruction(method, inst);

        /* Check this variable hasn't been defined. */
        if (defVar != NOPARAM) {
            if ((prevDef = xanHashTable_lookup(varDefTable, (void *)(JITNUINT)defVar))) {
                IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
                snprintf(buf, 256, "Variable %lld defined by inst %u and inst %u\n", defVar, inst->ID, prevDef->ID);
                print_err(buf, 0);
                abort();
            } else {
                xanHashTable_insert(varDefTable, (void *)(JITNUINT)defVar, inst);
            }
        }
    }

    /* Check no parameter variables are defined and all non-parameter variables are defined if used. */
    for (i = 0; i < IRMETHOD_getNumberOfVariablesNeededByMethod(method); ++i) {
        if (IRMETHOD_isTheVariableAMethodParameter(method, i)) {
            ir_instruction_t *prevDef;
            if ((prevDef = xanHashTable_lookup(varDefTable, (void *)(JITNUINT)i))) {
                IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
                snprintf(buf, 256, "Parameter variable %lld defined by inst %u\n", (IR_ITEM_VALUE)i, prevDef->ID);
                print_err(buf, 0);
                abort();
            }
            /* } else { */
            /*   XanList *useList = IRMETHOD_getVariableUses(method, i); */
            /*   if (!varDefTable->lookup(varDefTable, (void *)(JITNUINT)i) && xanList_length(useList) > 0) { */
            /*     snprintf(buf, 256, "Variable %lld not defined by any instruction\n", (IR_ITEM_VALUE)i); */
            /*     print_err(buf, 0); */
            /*     abort(); */
            /*   } */
        }
    }

    /* Clean up. */
    xanHashTable_destroyTable(varDefTable);
}
