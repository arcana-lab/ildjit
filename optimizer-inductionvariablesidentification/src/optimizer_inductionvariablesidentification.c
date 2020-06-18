/*
 * Copyright (C) 2008  Simone Campanoni, Borgio Simone, Bosisio Davide, Licata Caruso Alessandro
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

// My headers
#include <optimizer_inductionvariablesidentification.h>
#include <config.h>
// End

#define INFORMATIONS  "Identifies the induction variables."
#define AUTHOR        "Simone Campanoni, Borgio Simone, Bosisio Davide, Licata Caruso Alessandro"

static inline JITUINT64 induction_vars_get_ID_job (void);
static inline char * induction_vars_get_version (void);
static inline void induction_vars_do_job (ir_method_t * method);
static inline char * induction_vars_get_informations (void);
char * induction_vars_get_author (void);
static inline JITUINT64 induction_vars_get_dependences (void);
static inline void induction_vars_shutdown (JITFLOAT32 totalTime);
void induction_vars_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void free_past_information (ir_method_t *method);
static inline JITUINT64 induction_vars_get_invalidations (void);
static inline JITBOOLEAN internal_identify_induction_variable (ir_method_t *method, circuit_t *loop, ir_instruction_t *inst, JITUINT32 paramNum, JITNUINT* defined_once, JITNUINT* basic_ind, JITNUINT *derived_ind);
static inline void induction_vars_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void induction_vars_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;

ir_optimization_interface_t plugin_interface = {
    induction_vars_get_ID_job,
    induction_vars_get_dependences,
    induction_vars_init,
    induction_vars_shutdown,
    induction_vars_do_job,
    induction_vars_get_version,
    induction_vars_get_informations,
    induction_vars_get_author,
    induction_vars_get_invalidations,
    NULL,
    induction_vars_getCompilationFlags,
    induction_vars_getCompilationTime
};

/**
 * Condition that controls if an IR type is an integer type.
 * The following types are integer types:
 * \arg \c IRINT8
 * \arg \c IRINT16
 * \arg \c IRINT32
 * \arg \c IRINT64
 * \arg \c IRNINT
 * \arg \c IRUINT8
 * \arg \c IRUINT16
 * \arg \c IRUINT32
 * \arg \c IRUINT64
 * \arg \c IRNUINT
 * @param ir_type an IR type.
 * @return \c true if \c ir_type is an integer type, \c false otherwise.
 */
#define IS_INTEGER(ir_type)				  \
	(						  \
		(ir_type) == IRINT32 || (ir_type) == IRUINT32	\
		||					       \
		(ir_type) == IRINT8 || (ir_type) == IRINT16	\
		||					       \
		(ir_type) == IRINT64 || (ir_type) == IRUINT8	\
		||					       \
		(ir_type) == IRUINT16 || (ir_type) == IRUINT64	\
		||					       \
		(ir_type) == IRNINT || (ir_type) == IRNUINT	\
	)

unsigned int var_id_hash (void *element);
int var_id_equals (void *key1, void *key2);
JITNUINT isReachedOutOfLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum);
JITNUINT isReachedInLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum);
JITNUINT isReachedByLoopInvariant (ir_method_t *method, circuit_t *loop,
                                   JITUINT32 instID, JITUINT32 parNum);
JITNUINT isDerivedFromDerived (ir_method_t *method, JITUINT32 base,
                               JITUINT32 derived, JITUINT32 derivedFromDerived);
static inline void identifyInductionVariables (ir_method_t *method, circuit_t *loop);

static inline JITUINT64 induction_vars_get_invalidations (void) {
    return INVALIDATE_NONE;
}

void induction_vars_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

static inline void induction_vars_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;
}

static inline JITUINT64 induction_vars_get_ID_job (void) {
    return INDUCTION_VARIABLES_IDENTIFICATION;
}

static inline JITUINT64 induction_vars_get_dependences (void) {
    return LIVENESS_ANALYZER | REACHING_DEFINITIONS_ANALYZER | LOOP_IDENTIFICATION | LOOP_INVARIANTS_IDENTIFICATION | ESCAPES_ANALYZER;
}

static inline void induction_vars_do_job (ir_method_t * method) {
    XanListItem     *ll_item;
    circuit_t          *loop;

    /* Assertions				*/
    assert(method != NULL);
    PDBG_P("%s Job started for method \"%s\". %s\n", "####", IRMETHOD_getSignatureInString(method), "####");

    /* Free the memory used during past	*
     * invocations of this plugin		*/
    free_past_information(method);

    /* Checks if there are loop in current method */
    if (    (method->loop == NULL)          ||
            (method->loop->len == 0)        ) {

        /*
         * There aren't loop in current method, so
         * no optimization is required.
         */
        PDBG_P("There aren't loop in this method.\n");
        PDBG_P("%s Job finished for method \"%s\". %s\n", "####", IRMETHOD_getSignatureInString(method), "####");
        return;
    }

    /* Identify induction variables			*/
    ll_item = xanList_first(method->loop);
    while (ll_item != NULL) {

        /* Fetch the loop		*/
        loop = (circuit_t *) (ll_item->data);
        assert(loop != NULL);

        if (loop->loop_id != 0) {
            assert(loop->belong_inst != NULL);
            PDBG_P("%s Entering loop %u. %s\n", "----", loop->loop_id, "----");
            PDBG_P("Loop header id = %u\n", loop->header_id);
            identifyInductionVariables(method, loop);
            PDBG_P("%s Leaving loop %u. %s\n", "----", loop->loop_id, "----");
        }

        ll_item = ll_item->next;
    }
    PDBG_P("%s Job finished for method \"%s\". %s\n", "####", IRMETHOD_getSignatureInString(method), "####");

    /* Return				*/
    return;
}

static inline char * induction_vars_get_version (void) {
    return VERSION;
}

static inline char * induction_vars_get_informations (void) {
    return INFORMATIONS;
}

char * induction_vars_get_author (void) {
    return AUTHOR;
}

/**
 * \param method pointer to the ir_method_t structure of the method currently
 *               under analysis
 * \param loop pointer to the circuit_t structure of the loop currently under
 *             analysis
 * Identify the induction variables in loop loop in the following steps
 * 1) Search for variables defined once in the loop
 * 2) Search basic induction variables
 * 3) Search induction variables derived from basic induction variables
 */
static inline void identifyInductionVariables (ir_method_t *method, circuit_t *loop) {
    JITUINT32 	blocksNumber;       //indicates number of blocks for each bit vector
    JITUINT32 	instID;
    JITUINT32 	instructionsNumber;
    XanList         *instructionsOfLoop;
    XanList         *instructionsOfSubLoops;
    XanList         *nestedLoops;
    XanList		*notInvariantInstructionsOfSubLoops;
    XanListItem     *item;
    JITBOOLEAN	modified;
    JITUINT32 	tmp1;
    JITUINT32 	tmp2;

    /* Assertions				*/
    assert(method != NULL);
    assert(loop != NULL);
    assert(loop->invariants != NULL);
    assert(loop->induction_bitmap == NULL);
    assert(loop->induction_table == NULL);
    PDBG_P("Induction vars id: Start\n");
    PDBG_P("Induction vars id: 	%s\n", (char *)IRMETHOD_getSignatureInString(method));

    /* Fetch the number of blocks of memory	*
     * needed to store the information	*/
    blocksNumber = (IRMETHOD_getNumberOfVariablesNeededByMethod(method) / (sizeof(JITNUINT) * 8)) + 1;
    assert(blocksNumber > 0);

    /* Allocate the induction bitmap	*/
    loop->induction_bitmap 	= allocFunction(blocksNumber * sizeof(JITNUINT));
    assert(loop->induction_bitmap != NULL);
    memset(loop->induction_bitmap, 0, blocksNumber * sizeof(JITNUINT));

    JITNUINT* defined 	= (JITNUINT*) allocFunction(blocksNumber * sizeof(JITNUINT));
    JITNUINT* defined_once 	= (JITNUINT*) allocFunction(blocksNumber * sizeof(JITNUINT));

    /* Fetch the number of instructions	*/
    instructionsNumber 	= IRMETHOD_getInstructionsNumber(method);

    /* Fetch the instructions of the loop	*/
    instructionsOfLoop 	= IRMETHOD_getCircuitInstructions(method, loop);
    assert(instructionsOfLoop != NULL);

    /* Get the nested loops			*/
    nestedLoops 		= IRMETHOD_getImmediateNestedCircuits(method, loop);
    assert(nestedLoops != NULL);

    /* Compute the list of instructions	*
     * of nested loops			*/
    instructionsOfSubLoops 			= xanList_new(allocFunction, freeFunction, NULL);
    notInvariantInstructionsOfSubLoops	= xanList_new(allocFunction, freeFunction, NULL);
    item 					= xanList_first(nestedLoops);
    while (item != NULL) {
        circuit_t          *nestedLoop;
        XanList         *tempList;
        XanListItem	*item2;

        /* Fetch the nested loop		*/
        nestedLoop = item->data;
        assert(nestedLoop != NULL);
        assert(nestedLoop->header_id != loop->header_id);

        /* Fetch the instructions of the nested	*
         * loop					*/
        tempList = IRMETHOD_getCircuitInstructions(method, nestedLoop);
        assert(tempList != NULL);

        /* Append these instructions to the list*/
        xanList_appendList(instructionsOfSubLoops, tempList);

        /* Append the not invariant instructions*/
        item2	= xanList_first(tempList);
        while (item2 != NULL) {
            ir_instruction_t	*subloopInst;
            subloopInst	= item2->data;
            assert(subloopInst != NULL);
            if (!IRMETHOD_isCircuitInvariant(method, nestedLoop, subloopInst)) {
                xanList_append(notInvariantInstructionsOfSubLoops, subloopInst);
            }
            item2		= item2->next;
        }

        /* Free the memory			*/
        xanList_destroyList(tempList);

        /* Fetch the next element from the	*
         * list					*/
        item = item->next;
    }
    xanList_destroyList(nestedLoops);

    /* Search for unique definitions that	*
     * belong to the loop			*/
    item = xanList_first(instructionsOfLoop);
    while (item != NULL) {
        ir_instruction_t        *opt_inst;
        IR_ITEM_VALUE		var;
        JITNUINT 		mask1;
        JITBOOLEAN		doesInstructionBelongsToSubloops;

        /* Fetch the instruction		*/
        opt_inst 				= item->data;
        assert(opt_inst != NULL);

        /* Fetch the next element of the list	*/
        item 					= item->next;

        /* Check if the current instruction 	*
         * defines a variable			*/
        if (!IRMETHOD_doesInstructionDefineAVariable(method, opt_inst)) {
            continue;
        }

        /* Fetch the variable defined		*/
        var 					= livenessGetDef(method, opt_inst);
        assert(var != (IR_ITEM_VALUE) -1);

        /* Compute the mask			*/
        mask1 					= 1 << (var % (sizeof(JITNUINT) * 8));
        assert(mask1 != 0);

        /* Check if the instruction belongs to	*
         * a subloop and it is not an invariant	*/
        doesInstructionBelongsToSubloops	= (xanList_find(notInvariantInstructionsOfSubLoops, opt_inst) != NULL);

        /* Check if the variable is already defined	*/
        if (    (defined[var / (sizeof(JITNUINT) * 8)] & mask1)               	||
                (doesInstructionBelongsToSubloops)				) {

            /* The variable was already defined		*/
            defined[var / (sizeof(JITNUINT) * 8)] |= mask1;
            defined_once[var / (sizeof(JITNUINT) * 8)] &= ~mask1;

        } else {

            /* Mark the variable as defined			*/
            defined[var / (sizeof(JITNUINT) * 8)] |= mask1;

            JITUINT32 type = IRMETHOD_getInstructionType(opt_inst);

            /* An induction variable can be defined only by instruction like:
             *   var1 <- var2 + c
             *   var1 <- var2 - c
             *   var1 <- var2 * c
             * with var1 and var2 unescaped integer variable and
             * c integer. */
            if (    (       type == IRMOVE                                  &&
                            IS_INTEGER(opt_inst->result.internal_type)      &&
                            (       (opt_inst->param_1.type == IROFFSET && IS_INTEGER(opt_inst->param_1.internal_type))     ||
                                    (IS_INTEGER(opt_inst->param_1.type))
                            )
                    )  ||
                    (       (type == IRADD || type == IRMUL || type == IRSUB)       &&
                            IS_INTEGER(opt_inst->result.internal_type)              &&
                            (       (opt_inst->param_1.type == IROFFSET && IS_INTEGER(opt_inst->param_1.internal_type))     ||
                                    (IS_INTEGER(opt_inst->param_1.type))
                            )                                                       &&
                            (       (opt_inst->param_2.type == IROFFSET && IS_INTEGER(opt_inst->param_2.internal_type))     ||
                                    (IS_INTEGER(opt_inst->param_2.type))
                            )
                    )
               ) {
                defined_once[var / (sizeof(JITNUINT) * 8)] |= mask1;
            } else {
                defined_once[var / (sizeof(JITNUINT) * 8)] &= ~mask1;
            }
        }
    }

    /* Free the memory			*/
    freeFunction(defined);
    defined	= NULL;

    /* Print the variables defined once	*/
#ifdef PRINTDEBUG
    PDBG_P("Loop variables defined once: ");
    _PRINT_BITMAP(defined_once, blocksNumber * sizeof(JITNUINT) * 8, sizeof(JITNUINT) * 8, tmp1);
#endif

    /* Counts how potential induction variables there are in current loop.
     * The value computed is an upper bound of the induction variables that
     * the algorithm will find in current loop. */
    JITNUINT uniqueDefs = 0;
    JITNUINT block;
    for (tmp1 = 0; tmp1 < blocksNumber; tmp1++) {
        block = defined_once[tmp1];
        for (tmp2 = 0; tmp2 < (sizeof(JITNUINT) * 8); tmp2++) {
            uniqueDefs += (block & 1);
            block >>= 1;
        }
    }

    /* If no induction variables will be found, returns. */
    if (uniqueDefs == 0) {

        /* Free the memory	*/
        freeFunction(defined_once);
        xanList_destroyList(instructionsOfLoop);
        xanList_destroyList(instructionsOfSubLoops);
        xanList_destroyList(notInvariantInstructionsOfSubLoops);
        PDBG_P("Induction vars id: 	No unique definitions\n");
        PDBG_P("Induction vars id: Exit\n");

        return;
    }

    /* Creates a hash table with a suitable fixed length, no realloc function,
     * 'var_id_hash' as hash function and 'var_id_equals' as equals function. */
    loop->induction_table = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, var_id_hash, var_id_equals);
    assert(loop->induction_table != NULL);

    JITNUINT* basic_ind = (JITNUINT*) allocFunction(blocksNumber * sizeof(JITNUINT));
    induction_variable_t *iv_tmp = NULL;
    JITUINT32 *key_tmp = NULL;

    /* Identify basic induction variables		*
     * Notice that once-defined variables have been	*
     * already detected				*/
    JITUINT32 basicIV = 0;
    for (instID = 0; instID < instructionsNumber; instID++) {
        ir_instruction_t *opt_inst;
        JITUINT32 type;
        IR_ITEM_VALUE var;
        JITNUINT mask1;

        /* Fetch the instruction			*/
        opt_inst 	= IRMETHOD_getInstructionAtPosition(method, instID);
        assert(opt_inst != NULL);
        JITNUINT mask 	= (1 << (instID % (sizeof(JITNUINT) * 8)));
        type 		= opt_inst->type;

        /* Check if the instruction belongs to the loop	*/
        if (!(loop->belong_inst[instID / (sizeof(JITNUINT) * 8)] & mask)) {
            continue;
        }

        /* Check if the variable is an invariant	*/
        if ((loop->invariants[instID / (sizeof(JITNUINT) * 8)] & mask)) {
            continue;
        }

        /* Check if the result variable is defined once.
         */
        var 	= (opt_inst->result).value.v;
        mask1 	= 1 << var % (sizeof(JITNUINT) * 8);
        if (!(defined_once[var / (sizeof(JITNUINT) * 8)] & mask1)) {
            continue;
        }
        PDBG_P("Induction vars id: 	Variable %llu is defined once by instruction %u\n", var, opt_inst->ID);

        /* Check if the instruction does not belong to	*
         * any subloop (in this case we will have 	*
         * multiple runtime definitions of the variable)*/
        if (xanList_find(instructionsOfSubLoops, opt_inst) != NULL) {
            PDBG_P("Induction vars id: 	Instruction %u belongs to a subloop\n", opt_inst->ID);
            continue;
        }

        /* Check if the instruction will be always executed.
         */
        assert((loop->backEdge).src != NULL);
        if (!IRMETHOD_isInstructionAPredominator(method, opt_inst, (loop->backEdge).src)) {
            PDBG_P("Induction vars id: 	Instruction %u does not predominate the backedge %u\n", opt_inst->ID, (loop->backEdge).src->ID);
            continue;
        }

        /* A basic induction variable can be defined only by instructions like:
         *  + var1 <- var1 + c
         *  + var1 <- var1 - c
         *  + var1 <- var1 * c
         */
        if (	(type != IRADD) 	&&
                (type != IRSUB)		&&
                (type != IRMUL)		) {
            PDBG_P("Induction vars id: 	Instruction %u is of type %s which is not supported\n", opt_inst->ID, IRMETHOD_getInstructionTypeName(type));
            continue;
        }

        /* Check if the defined variable escapes	*/
        if (IRMETHOD_isAnEscapedVariable(method, var)) {
            PDBG_P("Induction vars id: 	Var %llu is an escaped variable\n", var);
            continue;
        }

        /* The second parameter is loop invariant?	*/
        if (	(	(opt_inst->param_2.type != IROFFSET) 			||    //parameter 2 is a constant
                    (isReachedInLoop(method, loop, instID, 2) == 0)  	||    // parameter 2 is not defined in the loop
                    (
                        (isReachedOutOfLoop(method, loop, instID, 2) == 0) 		&& //parameter 2 is reached only by one loop invariant definition
                        (isReachedInLoop(method, loop, instID, 2) == 1) 		&&
                        (isReachedByLoopInvariant(method, loop, instID, 2) == 1)	)
             )
           ) {

            // The first parameter is the same variable defined by the instruction?
            if (	(IROFFSET == opt_inst->param_1.type) 	&&
                    (var == (opt_inst->param_1).value.v)	) {

                //Add to base induction variables
                JITNUINT mask2 = 1 << var % (sizeof(JITNUINT) * 8);
                basic_ind[var / (sizeof(JITNUINT) * 8)] |= mask2;

                /* Allocate the new inductive variable	*/
                iv_tmp 		= allocFunction(sizeof(induction_variable_t));
                iv_tmp->ID 	= var;
                iv_tmp->i 	= var;          // var = 0 + c*var
                iv_tmp->a.type 	= IRINT64;
                iv_tmp->a.value = 0;
                if ((opt_inst->param_2).type != IROFFSET) {
                    iv_tmp->b.type 	= IRINT64;
                } else {
                    if (type == IRADD) {
                        iv_tmp->b.type 	= IROFFSET;
                    } else {
                        iv_tmp->b.type 	= -IROFFSET;
                    }
                }
                iv_tmp->c.type 	= IRINT64;
                if (	(type == IRADD)	||
                        (type == IRSUB)	) {
                    iv_tmp->b.value = (opt_inst->param_2).value.v;
                    iv_tmp->c.value = 0;
                } else {
                    assert(type == IRMUL);
                    iv_tmp->c.value = (opt_inst->param_2).value.v;
                    iv_tmp->b.value = 0;
                }

                // The key of the induction variable is its ID
                key_tmp 	= allocFunction(sizeof(JITUINT32));
                (*key_tmp) 	= iv_tmp->ID;
                PDBG_P("Var %llu is a basic induction variable\n", var);
                xanHashTable_insert(loop->induction_table, key_tmp, iv_tmp);
                key_tmp 	= 0;
                iv_tmp 		= 0;

                basicIV++;
                continue;
            }
        }

        /* The first parameter is loop invariant?		*/
        if (   ((opt_inst->param_1.type != IROFFSET) ||                //parameter 1 is a constant
                (isReachedInLoop(method, loop, instID, 1) == 0)  ||    // parameter 1 is not defined in the loop
                ((isReachedOutOfLoop(method, loop, instID, 1) == 0) && //parameter 1 is reached only by one loop invariant definition
                 (isReachedInLoop(method, loop, instID, 1) == 1) &&
                 (isReachedByLoopInvariant(method, loop, instID, 1) == 1)))) {

            /* Parameter 2 is the same variable defined by the instruction?
             * In this case the instruction can't be a subtraction. */
            if (    ((type == IRADD) /*|| (type == IRMUL)*/) 	&&
                    (IROFFSET == opt_inst->param_2.type)    &&
                    (var == (opt_inst->param_2).value.v)    ) {

                //Add to base induction variables
                JITNUINT mask2 = 1 << var % (sizeof(JITNUINT) * 8);
                basic_ind[var / (sizeof(JITNUINT) * 8)] |= mask2;

                iv_tmp 		= allocFunction(sizeof(induction_variable_t));
                iv_tmp->ID 	= var;
                iv_tmp->i 	= var;              // var = 0 + 1*var
                iv_tmp->a.type 	= IRINT64;
                iv_tmp->a.value = 0;
                if ((opt_inst->param_1).type != IROFFSET) {
                    iv_tmp->b.type 	= IRINT64;
                } else {
                    iv_tmp->b.type 	= IROFFSET;
                }
                iv_tmp->c.type 	= IRINT64;
                iv_tmp->b.value = (opt_inst->param_1).value.v;
                iv_tmp->c.value = 0;

                // The key of the induction variable is its ID
                key_tmp 	= allocFunction(sizeof(JITUINT32));
                (*key_tmp) 	= iv_tmp->ID;
                PDBG_P("Var %llu is a basic induction variable\n", var);
                xanHashTable_insert(loop->induction_table, key_tmp, iv_tmp);
                key_tmp 	= 0;
                iv_tmp 		= 0;

                basicIV++;
            }
        }
    }

    /* Free the memory			*/
    xanList_destroyList(instructionsOfSubLoops);
    xanList_destroyList(notInvariantInstructionsOfSubLoops);
    instructionsOfSubLoops			= NULL;
    notInvariantInstructionsOfSubLoops	= NULL;

    /* Print the basic induction variables	*/
#ifdef PRINTDEBUG
    PDBG_P("Basic induction variables: ");
    _PRINT_BITMAP(basic_ind, blocksNumber * sizeof(JITNUINT) * 8, sizeof(JITNUINT) * 8, tmp1);
#endif

    /* Check the basic inductive variables	*/
    if (basicIV == 0) {

        /* Free the memory.
         */
        freeFunction(basic_ind);
        freeFunction(defined_once);
        xanList_destroyList(instructionsOfLoop);
        PDBG_P("No basic induction variables.\n");

        return;
    }

    /* There are basic inductive variables.	*
     * Start checking the derivated ones	*/
    JITNUINT* derived_ind = (JITNUINT*) allocFunction(blocksNumber * sizeof(JITNUINT));

    /* Identify derived induction variables	(derived from basic).
     */
    modified	= JITTRUE;
    while (modified) {
        modified	= JITFALSE;
        PDBG_P("Search for derived induction variables\n");
        item 		= xanList_first(instructionsOfLoop);
        while (item != NULL) {
            ir_instruction_t        *opt_inst;
            JITUINT32 type;

            /* Fetch the instruction.
             */
            opt_inst 	= item->data;
            assert(opt_inst != NULL);

            /* Fetch the ID of the instruction.
             */
            instID 		= opt_inst->ID;
            type 		= opt_inst->type;

            /* Check if the instruction is an invariant.
             */
            JITNUINT mask = 1 << instID % (sizeof(JITNUINT) * 8);
            if ((loop->invariants[instID / (sizeof(JITNUINT) * 8)] & mask)) {

                /* The instruction is a loop invariant.
                 */
                item = item->next;
                continue;
            }

            /* Check if the instruction will be always executed.
             */
            assert((loop->backEdge).src != NULL);
            if (!IRMETHOD_isInstructionAPredominator(method, opt_inst, (loop->backEdge).src)) {
                item = item->next;
                continue;
            }
            PDBG_P("Instruction %u is always executed\n", opt_inst->ID);

            /* A derived induction variable can be defined by instructions like:
             *   var1 <- var2 + c
             *   var1 <- var2 - c
             *   var1 <- var2 * c
             */
            if (type == IRADD || type == IRSUB || type == IRMUL) {

                //ID of the result variable
                IR_ITEM_VALUE var = (opt_inst->result).value.v;
                JITNUINT mask1 = 1 << var % (sizeof(JITNUINT) * 8);

                /* Check if var1 has been already considered as induction variable	*/
                if (	(basic_ind[var / (sizeof(JITNUINT) * 8)] & mask1)	||
                        (derived_ind[var / (sizeof(JITNUINT) * 8)] & mask1)	) {
                    item = item->next;
                    continue;
                }

                /* Checks if opt_inst is 'var1 <- var2 (op) c' with:
                 *   var1 defined once
                 *   var2 induction variable
                 *   c loop invariant
                 */
                modified	|= internal_identify_induction_variable(method, loop, opt_inst, 2, defined_once, basic_ind, derived_ind);

                /* Checks if opt_inst is 'var1 <- c (op) var2' with:
                 *   var1 defined once
                 *   var2 basic induction variable
                 *   c loop invariant
                 */
                modified	|= internal_identify_induction_variable(method, loop, opt_inst, 1, defined_once, basic_ind, derived_ind);

            } else if (type == IRMOVE) {
                XanList		*definers;
                XanListItem	*item2;

                /* ID of the variable defined.
                 */
                IR_ITEM_VALUE var = (opt_inst->result).value.v;
                JITNUINT mask1 = 1 << var % (sizeof(JITNUINT) * 8);

                /* Check if the variable defined by the move instruction has been already identified as induction variable.
                 */
                if (	(basic_ind[var / (sizeof(JITNUINT) * 8)] & mask1)	||
                        (derived_ind[var / (sizeof(JITNUINT) * 8)] & mask1)	) {
                    item = item->next;
                    continue;
                }

                /* ID of the source variable.
                 */
                IR_ITEM_VALUE valPar2 = (opt_inst->param_1).value.v;
                JITNUINT maskPar2 = 1 << valPar2 % (sizeof(JITNUINT) * 8);

                /* Check if the source variable is defined before the current instruction.
                 */
                definers	= IRMETHOD_getVariableDefinitions(method, valPar2);
                item2		= xanList_first(definers);
                while (item2 != NULL) {
                    ir_instruction_t	*defInst;
                    defInst	= item2->data;
                    if (!IRMETHOD_isInstructionAPredominator(method, defInst, opt_inst)) {
                        break ;
                    }
                    item2	= item2->next;
                }
                xanList_destroyList(definers);
                if (item2 != NULL) {
                    item = item->next;
                    continue;
                }

                /* Checks if opt_inst is 'var1 <- var2' with:
                 *  + var1 defined once
                 *  + var2 basic induction variable
                 *  + var1 != var2
                 */
                if (    (defined_once[var / (sizeof(JITNUINT) * 8)] & mask1)        	&&
                        (opt_inst->param_1.type == IROFFSET)                    	&&
                        (basic_ind[valPar2 / (sizeof(JITNUINT) * 8)] & maskPar2)    	&&
                        (var != valPar2)                                        	) {

                    /* Add to induction variables (derived by basic)	*/
                    derived_ind[var / (sizeof(JITNUINT) * 8)] |= mask1;

                    /* Allocate the new inductive variable			*/
                    iv_tmp 		= allocFunction(sizeof(induction_variable_t));
                    iv_tmp->ID 	= var;
                    iv_tmp->i 	= valPar2;    // var1 = 0 + 1*var2
                    iv_tmp->a.type 	= IRINT64;
                    iv_tmp->a.value = 0;
                    iv_tmp->b.type 	= IRINT64;
                    iv_tmp->b.value	= 1;
                    iv_tmp->c.type 	= IRINT64;
                    iv_tmp->c.value	= 0;

                    // The key of the induction variable is its ID
                    key_tmp 	= allocFunction(sizeof(JITUINT32));
                    (*key_tmp) 	= iv_tmp->ID;
                    xanHashTable_insert(loop->induction_table, key_tmp, iv_tmp);
                    modified	= JITTRUE;
                    key_tmp 	= 0;
                    iv_tmp 		= 0;
                }
            }

            /* Fetch the next element from the list	*/
            item 	= item->next;
        }
    }

    /* Print the induction variables		*/
#ifdef PRINTDEBUG
    PDBG_P("Derived induction variables: ");
    _PRINT_BITMAP(derived_ind, blocksNumber * sizeof(JITNUINT) * 8, sizeof(JITNUINT) * 8, tmp1);
#endif

    /* Sets the bitmap of the induction variables 	*/
    for (tmp1 = 0; tmp1 < blocksNumber; tmp1++) {
        loop->induction_bitmap[tmp1] |= basic_ind[tmp1];
        loop->induction_bitmap[tmp1] |= derived_ind[tmp1];
    }

    /* Free the memory				*/
    freeFunction(basic_ind);
    freeFunction(derived_ind);
    freeFunction(defined_once);
    xanList_destroyList(instructionsOfLoop);

    /* Return					*/
    return;
}

/**
 * \param       method	: pointer to the ir_method_t structure of the method currently under analysis
 * \param       loop	: pointer to the circuit_t structure of the loop currently under analysis
 * \param       instID	: id of the instruction
 * \param	parNum	: index of the parameter in instruction instID
 * \return	1 if parameter parNum in instruction instID is reached by a loop invariant in in loop loop, otherwise 0
 */
JITNUINT isReachedByLoopInvariant (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum) {
    JITNUINT reachedInLoop = 0;
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, instID);
    JITUINT32 defID;

    for (defID = 0; defID < IRMETHOD_getInstructionsNumber(method); defID++) {
        JITNUINT mask = 1 << defID % (sizeof(JITNUINT) * 8);
        //if defID reaches instID
        //if (in[defID/(sizeof(JITNUINT)*8)] & mask)
        if (ir_instr_reaching_definitions_reached_in_is(method, inst, defID)) {
            //if defID belong to the loop
            if (loop->belong_inst[defID / (sizeof(JITNUINT) * 8)] & mask) {
                //if defID is a loop invariant
                if (loop->invariants[defID / (sizeof(JITNUINT) * 8)] & mask) {
                    if (parNum == 1) {
                        if (IRMETHOD_getInstructionResult(IRMETHOD_getInstructionAtPosition(method, defID))->value.v == IRMETHOD_getInstructionParameter1Value(IRMETHOD_getInstructionAtPosition(method, instID))) {
                            reachedInLoop = 1;
                        }
                    } else if (parNum == 2) {
                        if (IRMETHOD_getInstructionResult(IRMETHOD_getInstructionAtPosition(method, defID))->value.v == IRMETHOD_getInstructionParameter2Value(IRMETHOD_getInstructionAtPosition(method, instID))) {
                            reachedInLoop = 1;
                        }
                    }
                }
            }
        }
    }

    return reachedInLoop;
}

/**
 * \param       method	: pointer to the ir_method_t structure of the method currently under analysis
 * \param       loop	: pointer to the circuit_t structure of the loop currently under analysis
 * \param       instID	: id of the instruction
 * \param	parNum	: index of the parameter in instruction instID
 * \return	the number of definition of parameter parNum in loop loop that reaches instruction instID
 */
JITNUINT isReachedInLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum) {
    JITNUINT reachedInLoop = 0;
    //JITNUINT *in = IRMETHOD_getInstructionAtPosition(method, instID)->reaching_definitions.in;
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, instID);

    JITUINT32 defID;

    for (defID = 0; defID < IRMETHOD_getInstructionsNumber(method); defID++) {
        JITNUINT mask = 1 << defID % (sizeof(JITNUINT) * 8);
        //if defID reaches instID
        //if (in[defID/(sizeof(JITNUINT)*8)] & mask)
        if (ir_instr_reaching_definitions_reached_in_is(method, inst, defID)) {
            //if defID belong to the loop
            if (loop->belong_inst[defID / (sizeof(JITNUINT) * 8)] & mask) {
                if (parNum == 1) {
                    if (IRMETHOD_getInstructionResult(IRMETHOD_getInstructionAtPosition(method, defID))->value.v == IRMETHOD_getInstructionParameter1Value(IRMETHOD_getInstructionAtPosition(method, instID))) {
                        reachedInLoop++;
                    } else {
                    }
                } else if (parNum == 2) {
                    if (IRMETHOD_getInstructionResult(IRMETHOD_getInstructionAtPosition(method, defID))->value.v == IRMETHOD_getInstructionParameter2Value(IRMETHOD_getInstructionAtPosition(method, instID))) {
                        reachedInLoop++;
                    } else {
                    }
                }
            }
        }
    }

    return reachedInLoop;
}

/**
 * \param       method	: pointer to the ir_method_t structure of the method currently under analysis
 * \param       loop	: pointer to the circuit_t structure of the loop currently under analysis
 * \param       instID	: id of the instruction
 * \param	parNum	: index of the parameter in instruction instID
 * \return	the number of definition of parameter parNum NOT in loop loop that reaches instruction instID
 */
JITNUINT isReachedOutOfLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum) {
    JITNUINT reachedOutOfLoop = 0;
    //JITNUINT *in = IRMETHOD_getInstructionAtPosition(method, instID)->reaching_definitions.in;
    ir_instruction_t *inst = IRMETHOD_getInstructionAtPosition(method, instID);

    JITUINT32 defID;

    for (defID = 0; defID < IRMETHOD_getInstructionsNumber(method); defID++) {
        JITNUINT mask = 1 << defID % (sizeof(JITNUINT) * 8);
        //if defID reaches instID
        //if (in[defID/(sizeof(JITNUINT)*8)] & mask)
        if (ir_instr_reaching_definitions_reached_in_is(method, inst, defID)) {
            //if defID does not belong to the loop
            if (!(loop->belong_inst[defID / (sizeof(JITNUINT) * 8)] & mask)) {
                if (parNum == 1) {
                    if (IRMETHOD_getInstructionResult(IRMETHOD_getInstructionAtPosition(method, defID))->value.v == IRMETHOD_getInstructionParameter1Value(IRMETHOD_getInstructionAtPosition(method, instID))) {
                        reachedOutOfLoop++;
                    } else {
                    }
                } else if (parNum == 2) {
                    if (IRMETHOD_getInstructionResult(IRMETHOD_getInstructionAtPosition(method, defID))->value.v == IRMETHOD_getInstructionParameter2Value(IRMETHOD_getInstructionAtPosition(method, instID))) {
                        reachedOutOfLoop++;
                    } else {
                    }
                }
            }
        }
    }
    return reachedOutOfLoop;
}

/**
 * Equals function used by t_loop->induction_table XanHashTable.
 * @param key1 pointer to a variable ID.
 * @param key2 pointer to a variable ID.
 * @return <tt>*((JITUINT32*)key1) == *((JITUINT32*)key2)</tt> or \c 0 if one
 *         between \c key1 and \c key2 is \c NULL.
 */
int var_id_equals (void *key1, void *key2) {
    return key1 != NULL && key2 != NULL ?
           *((JITUINT32*) key1) == *((JITUINT32*) key2) : 0;
}

/**
 * Hash function used by t_loop->induction_table XanHashTable.
 * @param element pointer to a variable ID.
 * @return <tt>*((JITUINT32*)element)</tt> or \c 0 if \c element is \c NULL.
 */
unsigned int var_id_hash (void *element) {
    return element != NULL ? *((JITUINT32*) element) : 0;
}

static inline void free_past_information (ir_method_t *method) {

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if there are loops.
     */
    if (method->loop == NULL) {
        return;
    }

    /* Free memory.
     */
    IRMETHOD_deleteCircuitsInductionVariablesInformation(method);

    return;
}

static inline JITBOOLEAN internal_identify_induction_variable (ir_method_t *method, circuit_t *loop, ir_instruction_t *inst, JITUINT32 paramNum, JITNUINT* defined_once, JITNUINT* basic_ind, JITNUINT *derived_ind) {
    JITUINT32 	reached_in_loop;
    JITUINT32 	instID;
    IR_ITEM_VALUE 	var;
    JITNUINT	mask1;
    ir_item_t	*inputParam;
    ir_item_t	*strideParam;
    IR_ITEM_VALUE	inputValPar;
    IR_ITEM_VALUE 	strideValPar;
    JITNUINT 	maskInputPar;
    JITBOOLEAN	added;
    JITUINT32 	type;
    JITBOOLEAN	isDefinedOnce;
    JITBOOLEAN	isInputInduction;

    /* Assertions				*/
    assert(method != NULL);
    assert(loop != NULL);
    assert(inst != NULL);
    assert(defined_once != NULL);
    assert(basic_ind != NULL);
    assert(derived_ind != NULL);

    /* Initialize the variables		*/
    added		= JITFALSE;

    /* Fetch the ID of the instruction	*/
    instID 		= inst->ID;

    /* Fetch the ID of the defined variable	*/
    var 		= (inst->result).value.v;
    mask1 		= 1 << var % (sizeof(JITNUINT) * 8);

    /* Fetch the type of the instruction	*/
    type 		= inst->type;

    /* Fetch the input variable used to	*
     * define the induction variable	*/
    if (paramNum == 2) {
        inputParam	= IRMETHOD_getInstructionParameter(inst, 1);
        strideParam	= IRMETHOD_getInstructionParameter(inst, 2);
    } else {
        inputParam	= IRMETHOD_getInstructionParameter(inst, 2);
        strideParam	= IRMETHOD_getInstructionParameter(inst, 1);
    }
    assert(inputParam != NULL);
    assert(strideParam != NULL);

    /* Value of the input value		*/
    inputValPar 		= (inputParam->value).v;
    maskInputPar		= 1 << (inputValPar % (sizeof(JITNUINT) * 8));

    // Value of the second parameter
    strideValPar 		= (strideParam->value).v;

    /* Check if the variable defined by	*
     * the current instruction is defined	*
     * once inside the loop			*/
    isDefinedOnce		= ((defined_once[var / (sizeof(JITNUINT) * 8)] & mask1) == 0) ? JITFALSE : JITTRUE;

    /* Check if the input variable is an	*
     * induction variable			*/
    isInputInduction	= JITFALSE;
    if (inputParam->type == IROFFSET) {
        isInputInduction	= ((basic_ind[inputValPar / (sizeof(JITNUINT) * 8)] & maskInputPar) == 0) ? JITFALSE : JITTRUE;
        isInputInduction	|= ((derived_ind[inputValPar / (sizeof(JITNUINT) * 8)] & maskInputPar) == 0) ? JITFALSE : JITTRUE;
    }

    /* Check if the variable defined by the	*
     * current instruction is induction	*/
    reached_in_loop 	= isReachedInLoop(method, loop, instID, paramNum);
    if (    (isDefinedOnce)        						&&
            (inputParam->type == IROFFSET)                    		&&
            (isInputInduction)						&&
            (
                (strideParam->type != IROFFSET) 	                ||              // 'c' is a constant
                (reached_in_loop == 0)                                  ||              //'c' is a variable not defined in the loop
                (
                    (isReachedOutOfLoop(method, loop, instID, paramNum) == 0) 	&&      //'c' is reached only by a loop invariant definition
                    (reached_in_loop)                 	                 	&&
                    (isReachedByLoopInvariant(method, loop, instID, paramNum) == 1)
                )
            )
       ) {
        induction_variable_t 	*iv_tmp;
        JITUINT32 		*key_tmp;

        /* Initialize the variables		*/
        iv_tmp	= NULL;
        key_tmp	= NULL;

        /* Add to induction variables (derived 	*
         * by basic)				*/
        derived_ind[var / (sizeof(JITNUINT) * 8)] |= mask1;

        /* 'c' is a variable			*/
        if (strideParam->type == IROFFSET) {
            iv_tmp 		= allocFunction(sizeof(induction_variable_t));
            iv_tmp->ID 	= var;
            iv_tmp->i 	= inputValPar;              // var1 = (-)c + 1*var2 or var1 = 0 + c*var2
            iv_tmp->c.type 	= IRINT64;
            iv_tmp->c.value	= 0;
            if (type == IRADD) {
                iv_tmp->a.type 		= IROFFSET;
                iv_tmp->a.value 	= strideValPar;
                iv_tmp->b.type 		= IRINT64;
                iv_tmp->b.value 	= 1;
            } else if (type == IRSUB) {
                iv_tmp->a.value 	= strideValPar;
                iv_tmp->b.type 		= IRINT64;
                if (paramNum == 2) {
                    iv_tmp->a.type 		= -IROFFSET;
                    iv_tmp->b.value 	= 1;
                } else {
                    iv_tmp->a.type 		= IROFFSET;
                    iv_tmp->b.value 	= -1;
                }
            } else if (type == IRMUL) {
                iv_tmp->a.type 		= IRINT64;
                iv_tmp->a.value 	= 0;
                iv_tmp->b.type 		= IROFFSET;
                iv_tmp->b.value 	= strideValPar;
            } else {
                abort();
            }

            // The key of the induction variable is its ID
            key_tmp 	= allocFunction(sizeof(JITUINT32));
            (*key_tmp) 	= iv_tmp->ID;
            xanHashTable_insert(loop->induction_table, key_tmp, iv_tmp);
            added		= JITTRUE;
            key_tmp 	= 0;
            iv_tmp 		= 0;

        } else {
            JITUINT32 safe = 0;

            /* 'c' is a constant. For the compiler, 'c' value is in
             * (opt_inst->param_2).value.v, a JITUINT64 variable.
             * 'c' value is stored in induction_variable_t.a or in
             * induction_variable_t.b only if it is safe for a JITINT64 variable.
             */

            /* Check if 'c' could store a unsafe signed 64-bit value	*/
            if (	(strideParam->type == IRINT64) 	||
                    (strideParam->type == IRNINT)	) {

                /* The signed value of 'c' must differ from -2^63. Any other
                 * signed value in ]-2^63, 2^63-1] is ok.
                 * So, the unsigned value of 'c' must differ from
                 * 0x8000000000000000 (2^63).
                 */
                if (strideValPar != 0x8000000000000000LL) {
                    safe = JITTRUE;
                }

            } else if (	(strideParam->type == IRUINT64) 	||
                        (strideParam->type == IRNUINT) 		) {

                /* 'c' could store a unsafe unsigned 64-bit value.
                 * The unsigned value of 'c' must be less than 2^63. Only
                 * unsigned values in [0, 2^63[ are ok.
                 * So, the unsigned value of 'c' must be less than
                 * 0x8000000000000000 (2^63).
                 */
                if (strideValPar < 0x8000000000000000LL) {
                    safe = JITTRUE;
                }

            } else {

                // 'c' stores a safe value
                safe = JITTRUE;
            }

            /* If the value of 'c' is safe, an induction_variable_t element		*
             * is inserted in loop->induction_table.                                */
            if (safe) {

                /* Allocate the new inductive variable					*/
                iv_tmp 		= allocFunction(sizeof(induction_variable_t));
                iv_tmp->ID 	= var;
                iv_tmp->i 	= inputValPar;                  // var1 = (-)c + 1*var2 or var1 = 0 + c*var2
                iv_tmp->c.type 	= IRINT64;
                iv_tmp->c.value	= 0;
                if (type == IRADD) {
                    iv_tmp->a.type 	= IRINT64;
                    iv_tmp->a.value = strideValPar;
                    iv_tmp->b.type 	= IRINT64;
                    iv_tmp->b.value = 1;
                } else if (type == IRSUB) {
                    iv_tmp->a.type 	= IRINT64;
                    iv_tmp->b.type 	= IRINT64;
                    if (paramNum == 2) {
                        iv_tmp->a.value = -strideValPar;
                        iv_tmp->b.value = 1;
                    } else {
                        iv_tmp->a.value = strideValPar;
                        iv_tmp->b.value = -1;
                    }
                } else if (type == IRMUL) {
                    iv_tmp->a.type 	= IRINT64;
                    iv_tmp->a.value = 0;
                    iv_tmp->b.type 	= IRINT64;
                    iv_tmp->b.value = strideValPar;
                } else {
                    abort();
                }

                // The key of the induction variable is its ID
                key_tmp 	= allocFunction(sizeof(JITUINT32));
                (*key_tmp) 	= iv_tmp->ID;
                xanHashTable_insert(loop->induction_table, key_tmp, iv_tmp);
                PDBG_P("Variable %llu is a derived induction variable\n", var);
                added		= JITTRUE;
                key_tmp 	= 0;
                iv_tmp 		= 0;
            }
        }
    }

    /* Return		*/
    return added;
}

static inline void induction_vars_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void induction_vars_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
