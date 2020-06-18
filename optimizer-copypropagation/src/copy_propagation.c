/*
 * Copyright (C) 2006 - 2013  Campanoni Simone
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
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <copy_propagation.h>
#include <copy_propagation_front.h>
#include <copy_propagation_patterns.h>
#include <cyclic_moves.h>
#include <config.h>
// End

#define INFORMATIONS    "This optimization step performs the copy propagation algorithm"
#define AUTHOR          "Simone Campanoni and Timothy M. Jones"
#define JOB             COPY_PROPAGATOR

typedef struct {
    XanList 		*usingInsts;
    IR_ITEM_VALUE 		defVar;
    ir_item_t 		varToCopy;
    ir_instruction_t	*instToDelete;
} opt_pass_t;

static inline JITBOOLEAN tryRemoveRedundantMoves (ir_method_t *method, ir_instruction_t *defInst, IR_ITEM_VALUE defVar, XanList *todos);
static inline JITUINT64 copy_propagation_get_ID_job (void);
static inline void cp_do_job (ir_method_t *method);
static inline char * get_version (void);
static inline JITUINT64 copy_propagation_get_dependences (void);
static inline JITUINT64 copy_propagation_get_invalidations (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline void cp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void cp_shutdown (JITFLOAT32 totalTime);
static inline JITBOOLEAN globalCopyPropagation (ir_method_t *method, XanList **successors, XanList **predecessors, JITBOOLEAN doNotSubstituteDefinedVariables);
static inline void copy_propagation_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void copy_propagation_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline void filterNotSafeConversionUsingInsts (ir_method_t *method, XanList *usingInsts, IR_ITEM_VALUE varUsedID);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;

ir_optimization_interface_t plugin_interface = {
    copy_propagation_get_ID_job,
    copy_propagation_get_dependences,
    cp_init,
    cp_shutdown,
    cp_do_job,
    get_version,
    get_informations,
    get_author,
    copy_propagation_get_invalidations,
    NULL,
    copy_propagation_getCompilationFlags,
    copy_propagation_getCompilationTime
};

/**
 * Add all non-seen predecessors of an instruction to a worklist.
 */
static inline void addPredecessors (ir_method_t *method, ir_instruction_t *inst, XanList *instWorklist, XanHashTable *seenInsts, XanList **predecessors) {
    XanListItem *item;

    item =xanList_first(predecessors[inst->ID]);
    while (item != NULL) {
        ir_instruction_t *pred;
        pred = item->data;
        assert(pred != NULL);
        if (!xanHashTable_lookup(seenInsts, pred)) {
            xanList_append(instWorklist, pred);
            xanHashTable_insert(seenInsts, pred, pred);
        }
        item = item->next;
    }

    return ;
}

/**
 * Add all non-seen successors of an instruction to a worklist.
 */
static inline void addSuccessors (ir_method_t *method, ir_instruction_t *inst, XanList *instWorklist, XanHashTable *seenInsts, XanList **successors) {
    addPredecessors(method, inst, instWorklist, seenInsts, successors);

    return ;
}

/**
 * Add an item to a list only if it is not present already.
 */
static inline void
addItemToListIfUnique (XanList *list, void *item) {
    if (!xanList_find(list, item)) {
        xanList_append(list, item);
    }
}

/**
 * Shorter form of some long-named methods.
 */
#define defReachesInstStart(method, checkInst, defInst)	\
	ir_instr_reaching_definitions_reached_in_is(method, checkInst, defInst->ID)
#define defReachesInstEnd(method, checkInst, defInst) \
	ir_instr_reaching_definitions_reached_out_is(method, checkInst, defInst->ID)


/**
 * Check that all definitions of the given variable reach the given copy
 * instruction before they reach the given use instruction.  In other words,
 * the variable cannot be defined without the copy being made before the use.
 * To do this we start at the use and check all paths backwards, stopping at
 * the copy.  If we find a definition then there is a path that doesn't reach
 * the copy first.
 */
static inline JITBOOLEAN allDefinesReachCopyFirst (ir_method_t *method, ir_instruction_t *copyInst, ir_instruction_t *useInst, IR_ITEM_VALUE var, XanList **predecessors) {
    XanHashTable	*seenInsts;
    XanList 	*instWorklist;
    JITBOOLEAN 	reachedDef;

    /* Assertions.
     */
    assert(method != NULL);
    assert(copyInst != NULL);
    assert(useInst != NULL);
    PDEBUG(" Checking all defines of %lld reach inst %u before %u\n", var, copyInst->ID, useInst->ID);

    /* Initialize the variables.
     */
    reachedDef 	= JITFALSE;

    /* Create a list of instructions that have already been seen. */
    seenInsts 	= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    xanHashTable_insert(seenInsts, useInst, useInst);

    /* Create a list of instructions to check. */
    instWorklist = xanList_new(allocFunction, freeFunction, NULL);
    addPredecessors(method, useInst, instWorklist, seenInsts, predecessors);

    /* Iterate over all predecessors back to the definition or copy. */
    while (!reachedDef && xanList_length(instWorklist) >  0) {
        XanListItem *currCheckInst = xanList_first(instWorklist);
        ir_instruction_t *checkInst = (ir_instruction_t *) currCheckInst->data;
        PDEBUG(" Checking inst %u\n", checkInst->ID);

        /* Remove from worklist. */
        xanList_deleteItem(instWorklist, currCheckInst);

        /* Check this instruction and add predecessors if ok. */
        PDEBUG("   Variable defined by %u = %llu\n", checkInst->ID, livenessGetDef(method, checkInst));
        if (livenessGetDef(method, checkInst) == var) {
            reachedDef = JITTRUE;
            PDEBUG(" Nope - inst %u defines\n", checkInst->ID);
        } else if (checkInst != copyInst) {
            addPredecessors(method, checkInst, instWorklist, seenInsts, predecessors);
        }
    }

    /* Clean up. */
    xanHashTable_destroyTable(seenInsts);
    xanList_destroyList(instWorklist);

    /* Success if we didn't reach a definition. */
    return !reachedDef;
}

/**
 * Check that if the given instruction is a user-definer of the first variable
 * then the second variable is not live across this instruction.  In other
 * words, this enables us to substitute the first variable with the second in
 * all parameters of the instruction safely.
 */
static JITBOOLEAN varLiveAcrossUserDefiner (ir_method_t *method, ir_instruction_t *inst, IR_ITEM_VALUE varToRemove, IR_ITEM_VALUE varToCopy) {

    /* If it doesn't redefine the variable then it's ok. */
    if (livenessGetDef(method, inst) != varToRemove) {
        return JITFALSE;
    }

    /* Get the in and out sets to check. */
    assert(inst->metadata != NULL);
    assert((inst->metadata->liveness).in != NULL);
    assert((inst->metadata->liveness).out != NULL);
    if (  IRMETHOD_isVariableLiveIN(method, inst, varToCopy)      &&
            IRMETHOD_isVariableLiveOUT(method, inst, varToCopy)     ) {
        return JITTRUE;
    }

    /* Not live across this instruction. */
    return JITFALSE;
}

/**
 * Find all instructions that use a given variable, starting at the given
 * instruction that is a copy.  This iterates over instructions, adding them
 * to a running list if they use the variable and adding successor
 * instructions to a worklist if the variable is live out of the instruction.
 * Note that because this copy propagation algorithm substitutes the variable
 * in both use and define parameters of the instructions it works on, we must
 * add the successors of instructions that use and define the variable.
 */
static XanList * findUsersForDef (ir_method_t *method, ir_instruction_t *defInst, IR_ITEM_VALUE usedVar, XanList **successors) {
    XanList 	*usingInsts;
    XanList 	*instWorklist;
    XanHashTable	*seenInsts;

    /* Create a list of instructions that use this variable. */
    usingInsts 	= xanList_new(allocFunction, freeFunction, NULL);

    /* Create a list of instructions that have already been seen. */
    seenInsts 	= xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Create a list of instructions to check. */
    instWorklist = xanList_new(allocFunction, freeFunction, NULL);
    addSuccessors(method, defInst, instWorklist, seenInsts, successors);

    /* Iterate over all successors to find all uses. */
    while (xanList_length(instWorklist) >  0) {
        XanListItem *currCheckInst = xanList_first(instWorklist);
        ir_instruction_t *checkInst = (ir_instruction_t *) currCheckInst->data;

        /* Remove from worklist. */
        xanList_deleteItem(instWorklist, currCheckInst);

        /* Check this instruction for a use. */
        if (IRMETHOD_doesInstructionUseVariable(method, checkInst, usedVar)) {
            xanList_append(usingInsts, checkInst);
        }

        /* Add successors if the variable is live out of this instruction. */
        if (IRMETHOD_isVariableLiveOUT(method, checkInst, usedVar)) {
            addSuccessors(method, checkInst, instWorklist, seenInsts, successors);
        }
    }

    /* Clean up. */
    xanHashTable_destroyTable(seenInsts);
    xanList_destroyList(instWorklist);

    /* Return the list of using instructions. */
    return usingInsts;
}


/**
 * Remove an instruction from the list of instructions using a given variable.
 * At the same time as removing this instruction, we must also remove any
 * user-definers of this variable in the same list if their definitions reach
 * the removed instruction.  We do this because when an instruction is altered,
 * the change happens to both the use and define.  Correspondingly, when
 * removing a user-definer instruction, we must also remove any subsequent
 * user instructions that the removed instruction's definition reaches.
 */
static void
removeFromUsingInsts (ir_method_t *method, ir_instruction_t *defInst,
                      IR_ITEM_VALUE defVar, XanList *usingInsts,
                      XanList *toRemove) {
    /* Get the list of all definers of this variable. */
    XanList *allDefInsts = IRMETHOD_getVariableDefinitions(method, defVar);

    /* Remove instructions in the list and add others that now need removing. */
    XanListItem *currUse = xanList_first(toRemove);

    while (currUse) {
        XanListItem *currDef;
        ir_instruction_t *use = currUse->data;

        /* Remove this instruction from the list of using instructions. */
        PDEBUG("  Removing inst %u from using insts\n", use->ID);
        xanList_removeElement(usingInsts, use, JITTRUE);

        /* Add any user-definers to the list whose definitions reach here. */
        currDef = xanList_first(allDefInsts);
        while (currDef) {
            ir_instruction_t *def = currDef->data;
            if (defReachesInstStart(method, use, def) && def != defInst &&
                    xanList_find(usingInsts, def)) {
                addItemToListIfUnique(toRemove, def);
                PDEBUG("  Removing instruction %u causes %u to be removed\n",
                       use->ID, def->ID);
            }
            currDef = currDef->next;
        }

        /* Add any subsequent user instructions if this is a user-definer. */
        if (livenessGetDef(method, use) == defVar) {
            XanListItem *useItem = xanList_first(usingInsts);
            while (useItem) {
                ir_instruction_t *laterUse = useItem->data;
                if (defReachesInstStart(method, laterUse, use)) {
                    PDEBUG("  Removing user-def %u causes %u to be removed\n",
                           use->ID, laterUse->ID);
                    addItemToListIfUnique(toRemove, laterUse);
                }
                useItem = useItem->next;
            }
        }

        /* Next use to remove. */
        currUse = currUse->next;
    }

    /* Clean up. */
    xanList_destroyList(allDefInsts);
}


/**
 * Filter instructions that have multiple definers out of a list of using
 * instructions, provided that the definers are not the original definer
 * nor are in the list of using instructions.  Whenever we remove a using
 * instruction, we must also remove any user-definers in the list of using
 * instructions if their definition reaches the removed instruction.
 * Furthermore, whenever removing any user-definer we must also remove any
 * subsequent using instruction that this definition reaches.
 */
static void filterMultipleDefiners (ir_method_t *method, ir_instruction_t *defInst, IR_ITEM_VALUE defVar, XanList *usingInsts) {
    /*   PDEBUG(" Filtering external definitions of %lld\n", defVar); */

    /* Get the list of all definers of this variable. */
    XanList *allDefInsts = IRMETHOD_getVariableDefinitions(method, defVar);

    /* A list of instructions to remove from the using instructions. */
    XanList *toRemove = xanList_new(allocFunction, freeFunction, NULL);

    /* Build an initial list of instructions to remove. */
    XanListItem *currUse = xanList_first(usingInsts);

    while (currUse) {
        ir_instruction_t *use = currUse->data;

        /* Check each definition to make sure it satisfies all criteria. */
        JITBOOLEAN removeUse = JITFALSE;
        XanListItem *currDef = xanList_first(allDefInsts);
        while (currDef && !removeUse) {
            ir_instruction_t *def = currDef->data;

            /* Definition must reach this use, not be the copy and be a user too. */
            if (	(defReachesInstStart(method, use, def)) 	&&
                    (def != defInst) 				&&
                    (!xanList_find(usingInsts, def))		) {
                xanList_append(toRemove, use);
                removeUse = JITTRUE;
            }
            currDef = currDef->next;
        }
        currUse = currUse->next;
    }

    /* Remove instructions in the list and others that then need removing. */
    removeFromUsingInsts(method, defInst, defVar, usingInsts, toRemove);

    /* Clean up. */
    xanList_destroyList(toRemove);
    xanList_destroyList(allDefInsts);

    return ;
}


/**
 * Check whether a set of instructions that uses a given variable is suitable
 * for replacement of that variable.  This means that for all instructions
 * that use the variable, the only definers are the original copy instruction
 * and instructions that are also in the list of users.
 */
static JITBOOLEAN
usingInstSetSuitable (ir_method_t *method, ir_instruction_t *defInst,
                      IR_ITEM_VALUE defVar, XanList *usingInsts) {
    JITBOOLEAN suitable = JITTRUE;

    /* A list of all definers of this variable. */
    XanList *allDefInsts = IRMETHOD_getVariableDefinitions(method, defVar);

    /* Check that the only definers are the original and those in the list. */
    XanListItem *currUse = xanList_first(usingInsts);

    while (currUse && suitable) {
        ir_instruction_t *use = currUse->data;
        XanListItem *currDef = xanList_first(allDefInsts);
        while (currDef && suitable) {
            ir_instruction_t *def = currDef->data;
            if (defReachesInstStart(method, use, def) && def != defInst &&
                    !xanList_find(usingInsts, def)) {
                suitable = JITFALSE;
            }
            currDef = currDef->next;
        }
        currUse = currUse->next;
    }

    /* Clean up. */
    xanList_destroyList(allDefInsts);

    /* Return the status of the set. */
    return suitable;
}

static inline JITBOOLEAN tryRemoveRedundantMoves (ir_method_t *method, ir_instruction_t *defInst, IR_ITEM_VALUE defVar, XanList *todos) {
    XanList 	*allDefInsts;
    XanList 	*toRemove;
    XanListItem 	*item;
    ir_item_t	*par1;
    JITBOOLEAN	notSafe;
    JITBOOLEAN	modified;

    /* Initialize the variables.
     */
    modified	= JITFALSE;

    /* Check the instruction.
     */
    if (defInst->type != IRMOVE) {
        return JITFALSE;
    }
    return JITFALSE;

    /* The item used by the move instruction must be a variable.
     */
    par1	= IRMETHOD_getInstructionParameter1(defInst);
    if (!IRDATA_isAVariable(par1)) {
        return JITFALSE;
    }

    /* Both variables used by the move instruction cannot be escaped ones.
     */
    if (	(IRMETHOD_isAnEscapedVariable(method, defVar))			||
            (IRMETHOD_isAnEscapedVariable(method, (par1->value).v))		) {
        return JITFALSE;
    }

    /* Fetch the instructions that define the same variable defined by the current move instruction.
     */
    allDefInsts = IRMETHOD_getVariableDefinitions(method, defVar);
    assert(allDefInsts != NULL);
    assert(xanList_find(allDefInsts, defInst) != NULL);

    /* Consider definers of the variable defined by the move instruction, say m, that reach m and that are not m.
     *
     * ...
     * a) defVar = ...
     * ...
     * m) defVar = par1
     * ...
     *
     * For this purpose, we remove definitions that are either m or that do not reaches m.
     */
    toRemove = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(allDefInsts);
    while (item != NULL) {
        ir_instruction_t *def;
        def	= item->data;
        if (	(def == defInst)				||
                (!defReachesInstStart(method, defInst, def))	) {
            xanList_append(toRemove, def);
        }
        item	= item->next;
    }
    xanList_removeElements(allDefInsts, toRemove, JITTRUE);
    xanList_destroyList(toRemove);

    /* Check if every left definer is an exact copy of the current move instruction.
     */
    notSafe	= (xanList_length(allDefInsts) == 0);
    item = xanList_first(allDefInsts);
    while (item != NULL) {
        ir_instruction_t *def;
        def	= item->data;
        if (!IRMETHOD_haveBothInstructionsTheSameExecutionEffects(method, def, defInst)) {
            notSafe	= JITTRUE;
            break;
        }
        item	= item->next;
    }

    /* Check if it is safe to delete the move instruction.
     */
    if (notSafe) {
        xanList_destroyList(allDefInsts);
        return JITFALSE;
    }

    /* Check if there is only one other instruction that is equal to the move instruction that reaches it.
     */
    item	= xanList_first(allDefInsts);
    while (item != NULL) {
        ir_instruction_t	*otherDef;
        XanListItem		*item2;
        JITBOOLEAN		otherDefWillBeDeleted;

        /* Fetch the other move instruction.
         */
        otherDef		= item->data;
        assert(otherDef != NULL);

        /* Check if the other definition is going to be deleted.
         */
        otherDefWillBeDeleted	= JITFALSE;
        item2			= xanList_first(todos);
        while (item2 != NULL) {
            opt_pass_t	*o;
            o	= item2->data;
            assert(o != NULL);
            if (o->instToDelete == otherDef) {
                otherDefWillBeDeleted	= JITTRUE;
                break ;
            }
            item2	= item2->next;
        }
        if (!otherDefWillBeDeleted) {

            /* The other definition (i.e., otherDef) is not going to be deleted.
             * Check if the other definition (i.e., otherDef) makes the current one (i.e., defInst) redundant.
             */
            if (IRMETHOD_isInstructionAPredominator(method, otherDef, defInst)) {
                XanList 		*allDefInsts2;
                XanListItem		*currDef;

                /* otherDef is always executed before defInst.
                 * Moreover, no definition can modify defVar between otherDef and defInst in any possible dynamic trace.
                 * This is how the code looks like:
                 *
                 * (otherDef)	defVar	= par1
                 * ...
                 * (defInst)	defVar 	= par1
                 * ...
                 *
                 * Now we need to check that par1 does not change in between.
                 */
                allDefInsts2	 	= IRMETHOD_getVariableDefinitions(method, (par1->value).v);
                currDef 		= xanList_first(allDefInsts2);
                while (currDef != NULL) {
                    ir_instruction_t 	*def;

                    /* Fetch the definition.
                     */
                    def 	= currDef->data;
                    assert(def != NULL);

                    /* Check if the current definition of par1 reaches defInst.
                     */
                    if (	(defReachesInstStart(method, defInst, def)) 	&&
                            (def	!= otherDef) 				&&
                            (def	!= defInst)				) {

                        /* Check if the current definition can be between otherDef and defInst.
                         * First we check if the current definition does not reach any of the two instructions.
                         */
                        if (	(defReachesInstStart(method, otherDef, def))	||
                                (defReachesInstStart(method, defInst, def))	) {

                            /* The current definition of par1 reaches one of the two instruction.
                             * Check if it can be executed after otherDef.
                             */
                            if (!IRMETHOD_isInstructionAPredominator(method, def, otherDef)) {
                                break;
                            }
                        }
                    }
                    currDef = currDef->next;
                }

                /* Free the memory.
                 */
                xanList_destroyList(allDefInsts2);

                /* Check whether it is safe to remove the current IRMOVE instruction.
                 */
                if (currDef == NULL) {
                    opt_pass_t      *o;
                    o 			= allocFunction(sizeof(opt_pass_t));
                    o->usingInsts		= xanList_new(allocFunction, freeFunction, NULL);
                    o->instToDelete		= defInst;
                    modified		= JITTRUE;
                    xanList_append(todos, o);
                    break ;
                }
            }
        }

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(allDefInsts);

    return modified;
}

/**
 * Try replacing a variable within each instruction a list of instructions
 * using that variable.  The copy (store) instruction that defines this
 * variable is also given.  To do this, ensure that
 * 1) the variable is not defined by any of the instructions that use it, or
 * 2) this second definition does not propagate to any of the other using
 *    instructions, or
 * 3) these other instructions do not have any further definitions of this
 *    variable that reach them.
 * If none of these conditions hold, then replacement cannot continue.
 * Furthermore, in all instructions where replacement can take place, the
 * original defining instruction (and any subsequent user-definers) must be
 * the sole definer(s) of the to-be-replaced variable and that all definitions
 * of the to-be-copied variable reach this definer before the use.
 */
static inline JITBOOLEAN tryVariableReplacement (ir_method_t *method, ir_instruction_t *defInst, IR_ITEM_VALUE defVar, XanList *usingInsts, XanList *todos, XanList **predecessors, ir_item_t *varToPropagate) {
    /* Check the variable to propagate is suitable. */
    ir_item_t varToCopy;

    /* Fetch the variable to forward */
    memcpy(&varToCopy, varToPropagate, sizeof(ir_item_t));
    assert(varToCopy.type == IROFFSET);

    if (IRMETHOD_isAnEscapedVariable(method, (varToCopy.value).v)) {
        return JITFALSE;
    }

    /* Check that the set of using instructions is suitable. */
    if (!usingInstSetSuitable(method, defInst, defVar, usingInsts)) {
        return JITFALSE;
    }

    /* A list of instructions to remove from the using instructions. */
    XanList *toRemove = xanList_new(allocFunction, freeFunction, NULL);

    /* Check each using instruction is suitable for replacement. */
    XanListItem *currUse = xanList_first(usingInsts);
    while (currUse) {
        ir_instruction_t *useInst = currUse->data;

        /* All defs of the new variable must reach the copy before use. */
        if (!allDefinesReachCopyFirst(method, defInst, useInst, (varToCopy.value).v, predecessors)) {
            PDEBUG(" Defines of var %lld don't reach %u first, removing %u\n", varToCopy.value.v, defInst->ID, useInst->ID);
            addItemToListIfUnique(toRemove, useInst);
        }

        /* The new variable cannot be live across any user-definers. */
        if (varLiveAcrossUserDefiner(method, useInst, defVar, (varToCopy.value).v)) {
            PDEBUG(" Var %lld (variable def %lld) is live across user-definer %u\n", (varToCopy.value).v, defVar, useInst->ID);
            addItemToListIfUnique(toRemove, useInst);
        }
        currUse = currUse->next;
    }

    /* Actually perform the removal. */
    removeFromUsingInsts(method, defInst, defVar, usingInsts, toRemove);

    /* Now substitute in the remaining instructions. */
    opt_pass_t      *o;
    o = allocFunction(sizeof(opt_pass_t));
    o->usingInsts = usingInsts;
    o->defVar = defVar;
    o->varToCopy = varToCopy;
    xanList_append(todos, o);

    /* Clean up. */
    xanList_destroyList(toRemove);
    return JITTRUE;
}

static inline JITBOOLEAN doVariableSubstitutionsAndCleanUp (ir_method_t *method, XanList *todos) {
    opt_pass_t      *o;
    XanListItem     *item;
    JITBOOLEAN      modified;

    modified    = JITFALSE;
    item = xanList_first(todos);
    while (item != NULL) {
        XanListItem     *currUse;
        o = item->data;
        assert(o != NULL);
        PDEBUG("CP: Apply transformation for defined variable %llu\n", o->defVar);

        /* Perform the substitutions	*/
        currUse = xanList_first(o->usingInsts);
        while (currUse) {
            ir_instruction_t *useInst = currUse->data;
            PDEBUG("CP:     Substitute var %lld (inst %d) with var %lld in %s\n", o->defVar, useInst->ID, o->varToCopy.value.v, IRMETHOD_getCompleteMethodName(method));
            IRMETHOD_substituteVariableInsideInstructionWithItem(method, useInst, o->defVar, &(o->varToCopy));
            modified    = JITTRUE;
            currUse = currUse->next;
        }

        if (o->instToDelete != NULL) {
            PDEBUG("CP:     Delete instruction %u\n", o->instToDelete->ID);
            IRMETHOD_deleteInstruction(method, o->instToDelete);
            modified    = JITTRUE;
        }

        /* Free the memory		*/
        xanList_destroyList(o->usingInsts);

        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyListAndData(todos);

    return modified;
}

/**
 * Perform global copy propagation (i.e. across basic blocks).  This works by
 * iterating over all instructions, looking for copies (stores).  Then all uses
 * of the copied-to variable that are reached by this definition are found.
 * This set is checked (each instruction individually and the set as a whole)
 * to make sure that the copy can be propagated.  For those instructions where
 * this is possible, the copied-to variable is replaced.
 */
static inline JITBOOLEAN globalCopyPropagation (ir_method_t *method, XanList **successors, XanList **predecessors, JITBOOLEAN doNotSubstituteDefinedVariables) {
    JITUINT32 		i;
    JITUINT32 		instructionsNumber;
    JITBOOLEAN 		codeChanged;
    XanList 		*todos;
    XanHashTable 		*variablesConsidered;

    /* Checks. */
    assert(method);
    PDEBUG("CP: Global copy propagation: Start\n");

    /* Allocate the necessary memories*/
    variablesConsidered = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Fetch the instructions number */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check each instruction. */
    todos = xanList_new(allocFunction, freeFunction, NULL);
    for (i = 0; i < instructionsNumber; ++i) {
        ir_instruction_t 	*defInst;
        IR_ITEM_VALUE 		defVar;
        IR_ITEM_VALUE 		usedVar;
        ir_item_t		*defVarItem;
        ir_item_t		*usedVarItem;

        /* Fetch the instruction				*/
        defInst = IRMETHOD_getInstructionAtPosition(method, i);
        assert(defInst != NULL);

        /* Only store or conv instructions are useful           	*/
        if (	(defInst->type != IRMOVE) 	&&
                (defInst->type != IRCONV)	) {
            continue;
        }
        PDEBUG("CP: Global copy propagation:    Checking the %s instruction %u\n", IRMETHOD_getInstructionTypeName(defInst->type), defInst->ID);

        /* Check if the current conversion could be removed		*/
        if (defInst->type == IRCONV) {
            ir_item_t	*param1;
            ir_item_t	*resultItem;
            param1		= IRMETHOD_getInstructionParameter1(defInst);
            resultItem	= IRMETHOD_getInstructionResult(defInst);
            if (	(IRDATA_getSizeOfType(param1) != IRDATA_getSizeOfType(resultItem))	||
                    (IRMETHOD_hasAnIntType(param1) && IRMETHOD_hasAFloatType(resultItem))	||
                    (IRMETHOD_hasAnIntType(resultItem) && IRMETHOD_hasAFloatType(param1))	) {
                continue;
            }
        }

        /* Fetch both the variable defined and used			*/
        defVarItem 	= IRMETHOD_getInstructionResult(defInst);
        usedVarItem 	= IRMETHOD_getInstructionParameter1(defInst);

        /* Looking for a copy (store) from another variable. 		*/
        if (!IRDATA_isAVariable(usedVarItem)) {
            PDEBUG("CP: Global copy propagation:        The used item is not a variable\n");
            continue;
        }

        /* Fetch the variable IDs					*/
        defVar 	= (defVarItem->value).v;
        usedVar = (usedVarItem->value).v;

        /* We do not want to propagate temporary variables instead of defined ones.
         */
        if (	(doNotSubstituteDefinedVariables)			&&
                (defInst->type == IRMOVE)				&&
                (!IRMETHOD_isTheVariableATemporary(method, defVar))	&&
                (IRMETHOD_isTheVariableATemporary(method, usedVar))	) {
            PDEBUG("CP: Global copy propagation:        The used variable %llu is a temporary and the defined one, var %llu, is not a temporary.\n", usedVar, defVar);
            continue ;
        }

        /* Check if the variable to use in the substitution 	*
         * was already considered				*/
        if (xanHashTable_lookup(variablesConsidered, (void *) (JITNUINT) (usedVar + 1)) != NULL) {
            PDEBUG("CP: Global copy propagation:        The used variable %llu has been already considered\n", usedVar);
            continue;
        }

        /* Check this variable is suitable for replacement. */
        if (    (IRMETHOD_isAnEscapedVariable(method, defVar))  	||
                (IRMETHOD_isAnEscapedVariable(method, usedVar)) 	) {

            /*** TODO: This is ok if there's no call between definer and user. ***/
            PDEBUG("CP: Global copy propagation:        The variable %llu is an escaped variable\n", IRMETHOD_isAnEscapedVariable(method, defVar) ? defVar : usedVar);
            continue;
        }
        PDEBUG("CP: Global copy propagation:            Check usages of variable %lld defined in instruction %d\n", defVar, defInst->ID);

        /* Build the set of using instructions. */
        XanList *usingInsts = findUsersForDef(method, defInst, defVar, successors);
        assert(usingInsts != NULL);

        /* If the current instruction is a conversion, filter out instructions that have uses that are not load operations	*/
        if (defInst->type == IRCONV) {
            filterNotSafeConversionUsingInsts(method, usingInsts, defVar);
        }

        /* Filter out instructions that have external definers. */
        filterMultipleDefiners(method, defInst, defVar, usingInsts);

        /* Attempt to replace the variable in each of them. */
        if (	(xanList_length(usingInsts) == 0)									||
                (!tryVariableReplacement(method, defInst, defVar, usingInsts, todos, predecessors, usedVarItem))	) {
            JITBOOLEAN	toClean;

            toClean	= (xanList_length(usingInsts) == 0);
            if (	(defInst->type == IRMOVE)		&&
                    (xanList_length(usingInsts) == 0)	) {

                /* The current move instruction cannot be removed because the variable defined is used by an instruction where another move instruction reaches it.
                 * Check if the current move instruction is redundant due to another move instruction.
                 * For example:
                 * (i) a = b
                 * (k) L1: print(a)
                 * (j) a = b
                 *     branch L1
                 *
                 * In this case, instruction (j) is redundant but cannot be removed because instruction (k) may use the value generated by both (i) and (j).
                 * Notice that (j) is useless as b does not change from (i) to (j).
                 */
                assert(xanList_length(usingInsts) == 0);
                if (tryRemoveRedundantMoves(method, defInst, defVar, todos)) {
                    xanHashTable_insert(variablesConsidered, (void *) (JITNUINT) (usedVar + 1), (void *)(JITNUINT)(usedVar + 1));
                    xanHashTable_insert(variablesConsidered, (void *) (JITNUINT) (defVar + 1), (void *)(JITNUINT)(defVar + 1));
                }
            }

            if (toClean) {

                /* Clean up. */
                assert(xanList_length(usingInsts) == 0);
                xanList_destroyList(usingInsts);
            }

        } else if (xanList_length(usingInsts) > 0) {
            xanHashTable_insert(variablesConsidered, (void *) (JITNUINT) (usedVar + 1), (void *)(JITNUINT)(usedVar + 1));
            xanHashTable_insert(variablesConsidered, (void *) (JITNUINT) (defVar + 1), (void *)(JITNUINT)(defVar + 1));
        }
    }

    /* Apply the transformations	*/
    codeChanged = doVariableSubstitutionsAndCleanUp(method, todos);

    /* Free the memory		*/
    xanHashTable_destroyTable(variablesConsidered);

    PDEBUG("CP: Global copy propagation: Exit\n");
    return codeChanged;
}

static inline void filterNotSafeConversionUsingInsts (ir_method_t *method, XanList *usingInsts, IR_ITEM_VALUE varUsedID) {
    XanListItem	*item;

    /* Conversions that can be removed are those that define a variable used by a subset of instruction types only.
     * The following loop check this condition.
     */
    item	= xanList_first(usingInsts);
    while (item != NULL) {
        ir_instruction_t	*inst;

        /* Fetch the instruction		*/
        inst	= item->data;

        /* Check the instruction		*/
        if (	(inst->type != IRLOADREL)			&&
                (inst->type != IRMEMCPY)			&&
                (inst->type != IRINITMEMORY)			&&
                (inst->type != IRBRANCHIF)			&&
                (inst->type != IRBRANCHIFNOT)			&&
                (!IRMETHOD_isACallInstruction(inst))		) {

            /* The current instruction does not belong to the allowed-types-no-matter-what-are-the-parameters.
             * Following we are going to check if the instruction has the requirements to be considered as safe by checking its parameters.
             */
            if (	(inst->type != IRSTOREREL)	&&
                    (inst->type != IRCONV)		) {

                /* The instruction has a type that cannot be considered as safe
                 */
                xanList_emptyOutList(usingInsts);
                break;
            }
            assert((inst->type == IRSTOREREL) || (inst->type == IRCONV));

            if (inst->type == IRSTOREREL){
                ir_item_t 	*param3;

                /* The instruction that uses the variable defined by the conversion is a store to memory instruction.
                 * Following we are going to check if the defined variable is used as a pointer; if that is the case, then the instruction can be considered as safe.
                 */
                param3	= IRMETHOD_getInstructionParameter3(inst);
                if (	(param3->type == IROFFSET)		&&
                        ((param3->value).v == varUsedID)	) {
                    xanList_emptyOutList(usingInsts);
                    break;
                }

            } else {
                ir_item_t	*param1;
                ir_item_t	*param2;

                /* The instruction that uses the variable defined by the conversion is a conversion instruction as well.
                 * Following we are going to check if the conversion that uses the defined variable is not extending the bitwidth.
                 * If the last conversion extends the bitwidth of an integer value, the copy propagation is not safe; the reason is the following:
                 *
                 * v1 = conv(v0 (uint32), int32)
                 		 * v2 = conv(v1, int64)
                 *
                 * The second conversion sign extends v1; therefore the following code (the one produced by a copy propagation) has a different semantic:
                 *
                 		 * v2 = conv(v0 (uint32), int64)
                 *
                 */
                assert(inst->type == IRCONV);
                param1	= IRMETHOD_getInstructionParameter1(inst);
                param2	= IRMETHOD_getInstructionParameter2(inst);
                if (	(IRMETHOD_hasAnIntType(param1))						&&
                        (IRMETHOD_hasAnIntType(param2))						&&
                        (IRDATA_getSizeOfType(param1) < IRDATA_getSizeOfType(param2))	) {
                    xanList_emptyOutList(usingInsts);
                    break;
                }
            }
        }

        /* Fetch the next element		*/
        item	= item->next;
    }

    return ;
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

static inline void cp_do_job (ir_method_t *method) {
    XanList         **successors;
    XanList         **predecessors;
    JITUINT32 	instructionsNumber;
    JITBOOLEAN	codeChanged;

    /* Checks. */
    assert(method != NULL);
    PDEBUG("CP: Start\n");
    PDEBUG("CP:	Method %s\n", IRMETHOD_getSignatureInString(method));

    /* Initialize the variables.
     */
    codeChanged	= JITFALSE;

    /* Fetch the number of instructions.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Compute the set of successors	*/
    successors 	= IRMETHOD_getInstructionsSuccessors(method);

    /* Compute the set of predecessors	*/
    predecessors 	= IRMETHOD_getInstructionsPredecessors(method);

    /* Apply the global copy propagation.
     */
    codeChanged	|= globalCopyPropagation(method, successors, predecessors, JITTRUE);

    /* Apply front propagation.
     */
    codeChanged	|= applyFrontPropagation(method);

    /* Apply copy propagation across data structure uses.
     */
    codeChanged	|= apply_copy_propagation_for_value_types(method);

    /* Apply aggressive global copy propagation if no changed has been made.
     */
    if (!codeChanged) {
        codeChanged |= globalCopyPropagation(method, successors, predecessors, JITFALSE);
    }

    /* Remove cyclic move instructions.
     */
    if (!codeChanged){
        remove_cyclic_moves(method);
    }

    /* Free the memory.
     */
    IRMETHOD_destroyInstructionsSuccessors(successors, instructionsNumber);
    IRMETHOD_destroyInstructionsPredecessors(predecessors, instructionsNumber);

    PDEBUG("CP: Exit\n");
    return;
}

static inline void cp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);

    irLib = lib;
    irOptimizer = optimizer;
}

static inline void cp_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
}

static inline JITUINT64 copy_propagation_get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 copy_propagation_get_dependences (void) {
    return REACHING_DEFINITIONS_ANALYZER | LIVENESS_ANALYZER | ESCAPES_ANALYZER | BASIC_BLOCK_IDENTIFIER | PRE_DOMINATOR_COMPUTER;
}

static inline JITUINT64 copy_propagation_get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

static inline void copy_propagation_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void copy_propagation_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
