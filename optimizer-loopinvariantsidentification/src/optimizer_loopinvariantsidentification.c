/*
 * Copyright (C) 2008 - 2010  Simone Campanoni, Borgio Simone, Bosisio Davide, Alessandro Licata Caruso
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
#include <optimizer_loopinvariantsidentification.h>
#include <config.h>
// End

#define INFORMATIONS    "This step identifies loop invariants."
#define AUTHOR          "Simone Campanoni, Borgio Simone, Bosisio Davide, Alessandro Licata Caruso"

static inline JITUINT64 loopinvariants_get_ID_job (void);
char * loopinvariants_get_version (void);
static inline void loopinvariants_do_job (ir_method_t * method);
char * loopinvariants_get_informations (void);
char * loopinvariants_get_author (void);
static inline JITUINT64 loopinvariants_get_dependences (void);
static inline JITUINT64 loopinvariants_get_invalidations (void);
static inline JITBOOLEAN internal_is_memory_invariant (ir_method_t *method, circuit_t *loop, ir_instruction_t *inst);
void loopinvariants_shutdown (JITFLOAT32 totalTime);
void loopinvariants_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;

ir_optimization_interface_t plugin_interface = {
    loopinvariants_get_ID_job,
    loopinvariants_get_dependences,
    loopinvariants_init,
    loopinvariants_shutdown,
    loopinvariants_do_job,
    loopinvariants_get_version,
    loopinvariants_get_informations,
    loopinvariants_get_author,
    loopinvariants_get_invalidations
};

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

#define FETCH_INSTR(instr)			      \
	(					      \
		((instr)->type == IRLOADREL		    \
		 &&					   \
		 (instr)->param_1.type == IROFFSET	    \
		 &&					   \
		 (instr)->param_1.internal_type == IROBJECT \
		 &&					   \
		 IS_INTEGER((instr)->param_2.type)	    \
		 &&					   \
		 (instr)->param_2.value.v == -12)	      \
		&&					  \
		(instr)->param_3.type == IRTYPE		   \
		&&					  \
		IS_INTEGER((instr)->param_3.value.v)	     \
	)

JITNUINT isReachedOutOfLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum);
static inline JITBOOLEAN isReachedByLoopInvariant (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum);
static inline JITUINT32 isReachedInLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum, JITUINT32 instructionsNumber);
static inline void identifyLoopInvariants (ir_method_t *method, circuit_t *loop);

void loopinvariants_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
}

void loopinvariants_shutdown (JITFLOAT32 totalTime) {

}

static inline JITUINT64 loopinvariants_get_ID_job (void) {
    return LOOP_INVARIANTS_IDENTIFICATION;
}

static inline JITUINT64 loopinvariants_get_dependences (void) {
    return LIVENESS_ANALYZER | REACHING_DEFINITIONS_ANALYZER | LOOP_IDENTIFICATION;
}

static inline void loopinvariants_do_job (ir_method_t * method) {
    XanListItem     *ll_item;
    circuit_t          *loop;

    /* Assertions				*/
    assert(method != NULL);
    PDBG_P("%s Job started for method \"%s\". %s\n", "####", method->getName(method), "####");

    /* Free past information.
     */
    IRMETHOD_deleteCircuitsInvariantsInformation(method);

    /* Checks if there are loop in current method */
    if (    (method->loop == NULL)          ||
            (method->loop->len == 0)        ) {

        /*
         * There aren't loop in current method, so
         * no optimization is required.
         */
        PDBG_P("There aren't loop in this method.\n");
        PDBG_P("%s Job finished for method \"%s\". %s\n", "####", method->getName(method), "####");
        return;
    }

    ll_item = xanList_first(method->loop);
    while (ll_item != NULL) {
        loop = (circuit_t *) (ll_item->data);
        assert(loop != NULL);

        if (loop->loop_id != 0) {
            assert(loop->belong_inst != NULL);
            PDBG_P("%s Entering loop %d. %s\n", "----", loop->loop_id, "----");
            PDBG_P("Loop header id = %u\n", loop->header_id);
            identifyLoopInvariants(method, loop);
            PDBG_P("%s Leaving loop %d. %s\n", "----", loop->loop_id, "----");
        }
        ll_item = ll_item->next;
    }
    PDBG_P("%s Job finished for method \"%s\". %s\n", "####", method->getName(method), "####");

    /* Return				*/
    return;
}

char * loopinvariants_get_version (void) {
    return VERSION;
}

char * loopinvariants_get_informations (void) {
    return INFORMATIONS;
}

char * loopinvariants_get_author (void) {
    return AUTHOR;
}

/**
 * \param       method	: pointer to the ir_method_t structure of the method currently under analysis
 * \param       loop	: pointer to the circuit_t structure of the loop currently under analysis
 * Identify the loop invariants in loop loop
 */
static inline void identifyLoopInvariants (ir_method_t *method, circuit_t *loop) {
    JITUINT32 blocksNumber;                 //this var indicates number of blocks for each bit vector
    JITNUINT        *may_invariants;        //bit vvector for definitions that may be loop invariants
    JITUINT32 instID;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Fetch the number of instructions	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Allocate the necessary memory	*/
    blocksNumber = (instructionsNumber / (sizeof(JITNINT) * 8)) + 1;
    loop->invariants = (JITNUINT*) allocMemory(blocksNumber * sizeof(JITNUINT));                //bit vector for loop invariants definitions
    may_invariants = (JITNUINT*) allocMemory(blocksNumber * sizeof(JITNUINT));

    /* Bits vector initialization		*/
    memset(loop->invariants, 0, blocksNumber * sizeof(JITNUINT));
    memset(may_invariants, 0, blocksNumber * sizeof(JITNUINT));

    /* Identify basic invariants		*/
    for (instID = 0; instID < instructionsNumber; instID++) {
        JITNUINT 		mask;
        ir_instruction_t 	*opt_inst;
        JITUINT16 		type;
        JITNUINT 		num_reaching_defs_par1 = 0;
        JITNUINT 		num_reaching_defs_par2 = 0;
        JITNUINT 		par1_is_const = 0;
        JITNUINT 		par2_is_const = 0;

        /* Fetch the instruction		*/
        opt_inst 	= IRMETHOD_getInstructionAtPosition(method, instID);
        assert(opt_inst != NULL);

        /* Check if the instruction does not 	*
         * belong to the loop			*/
        mask = 1 << instID % (sizeof(JITNINT) * 8);
        if ((loop->belong_inst[instID / (sizeof(JITNINT) * 8)] & mask) == 0) {
            continue;
        }

        /* Fetch the instruction type		*/
        type 		= IRMETHOD_getInstructionType(opt_inst);

        /* Check if the current instruction is	*
         * an invariant				*/
        switch (type) {
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRAND:
            case IROR:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRLT:
            case IRGT:
            case IREQ:
            case IRPOW:
                if (!IRMETHOD_isInstructionParameterAVariable(opt_inst, 1)) {
                    par1_is_const = 1;
                    num_reaching_defs_par1 = 0;
                } else {
                    num_reaching_defs_par1 = isReachedInLoop(method, loop, instID, 1, instructionsNumber);
                }
                if (!IRMETHOD_isInstructionParameterAVariable(opt_inst, 2)) {
                    par2_is_const = 1;
                    num_reaching_defs_par2 = 0;
                } else {
                    num_reaching_defs_par2 = isReachedInLoop(method, loop, instID, 2, instructionsNumber);
                }

                if (num_reaching_defs_par1 == 0 && num_reaching_defs_par2 == 0) {

                    //Add to loop invariants list
                    loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;

                } else if ( 	(par1_is_const == 1 && num_reaching_defs_par2 == 1) 		||
                                (num_reaching_defs_par1 == 0 && num_reaching_defs_par2 == 1) 	||
                                (num_reaching_defs_par1 == 1 && par2_is_const == 1) 		||
                                (num_reaching_defs_par1 == 1 && num_reaching_defs_par2 == 0) 	||
                                (num_reaching_defs_par1 == 1 && num_reaching_defs_par2 == 1) 	) {

                    //add to may_invariants list
                    may_invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                }
                break;

            case IRCONV:
            case IRCONVOVF:
            case IRNOT:
            case IRNEG:
            case IRISNAN:
            case IRISINF:
            case IRCHECKNULL:
            case IRCOSH:
            case IRCEIL:
            case IRSIN:
            case IRCOS:
            case IRACOS:
            case IRSQRT:
            case IRFLOOR:
            case IRMOVE:
                if (!IRMETHOD_isInstructionParameterAVariable(opt_inst, 1)) {
                    par1_is_const = 1;

                } else {
                    num_reaching_defs_par1 = isReachedInLoop(method, loop, instID, 1, instructionsNumber);
                }
                if ( 	(par1_is_const == 1) 		||
                        (num_reaching_defs_par1 == 0)	) {
                    //Add to loop invariants list
                    loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                } else if (num_reaching_defs_par1 == 1) {
                    //add to may_invariants list
                    may_invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                }
                break;

            case IRGETADDRESS:

                //Add to loop invariants list
                loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                break;

            default:
                if (internal_is_memory_invariant(method, loop, opt_inst)) {

                    //Add to loop invariants list
                    loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                    break;
                }
                if (FETCH_INSTR(opt_inst)) {

                    /* The instruction fetching an array size is a loop invariant. */
                    JITUINT32 instr_num = instructionsNumber;
                    JITUINT32 tmp1_ui32, tmp2_ui32, tmp3_ui32;
                    tmp1_ui32 = tmp2_ui32 = tmp3_ui32 = 0;
                    ir_instruction_t *tmp1_iip, *tmp2_iip, *tmp3_iip;
                    tmp1_iip = tmp2_iip = tmp3_iip = NULL;

                    /*
                     * Controls if the value of the pointer used in the fetch
                     * instruction is the memory address of an array object. The ID
                     * of the pointer (P) is stored in the variable tmp1_ui32 while
                     * the instruction (I) that uses that particular pointer's value
                     * is referenced by tmp1_iip.
                     */
                    tmp1_ui32 = opt_inst->param_1.value.v;
                    tmp1_iip = opt_inst;

                    do {
                        /*
                         * If, at the end of following two 'for' cycles, the value of
                         * tmp3_ui32 is set to:
                         *  + 0: then P isn't an array pointer (or it is statically
                         *       not possible to determine the array which it refers
                         *       to).
                         *  + 1: then the value of P used by I is the memory address
                         *       of an array object.
                         *  + 2: then there is only one definition of P reaching I
                         *       and this definition copies in P the memory address
                         *       stored in another variable.
                         */
                        tmp3_ui32 = 0;
                        /*
                         * Scans all the method's instructions. The scan finishes
                         * when:
                         *  + a definition of P reaching I is found;
                         *  + or all the method's instruction are scanned.
                         */
                        for (tmp2_ui32 = 0; tmp2_ui32 < instr_num; tmp2_ui32++) {
                            assert(IRMETHOD_getInstructionAtPosition(method,
                                    tmp2_ui32) != NULL);
                            if (_GET_BIT_AT(tmp1_iip->metadata->reaching_definitions.in,
                                            tmp2_ui32, sizeof(JITNUINT) * 8)
                                    &&
                                    IRMETHOD_getInstructionAtPosition(method,
                                            tmp2_ui32)->metadata->liveness.def == tmp1_ui32) {
                                /* A definition of P reaching I has been found. */
                                tmp2_iip = IRMETHOD_getInstructionAtPosition(method, tmp2_ui32);
                                if (tmp2_iip->type == IRNEWARR) {
                                    /*
                                     * There is a definition of P reaching I that stores
                                     * in P the address of an array object.
                                     */
                                    tmp3_ui32 = 1;
                                    /* Jumps to the next 'for' cycle. */
                                    tmp2_ui32++;
                                    break;
                                } else if (tmp2_iip->type == IRMOVE
                                           &&
                                           tmp2_iip->param_1.type == IROFFSET) {
                                    /*
                                     * There is a definition of P reaching I that copies
                                     * in P the memory address stored in another variable.
                                     */
                                    tmp3_ui32 = 2;
                                    /* P and I will change in the next do-while cycle, so
                                     * their new values are saved. */
                                    tmp3_iip = tmp2_iip;
                                    /* Jumps to the next 'for' cycle. */
                                    tmp2_ui32++;
                                    break;
                                } else {
                                    /* There is a definition of P reaching I that is
                                     * different from 'store' and 'new_array'. The
                                     * analysis is aborted.
                                     */
                                    tmp3_ui32 = 0;
                                    tmp2_ui32 = instr_num;
                                }
                            }
                        }
                        /*
                         * Scans the remaining method instructions to see if there
                         * are other definitions of P reaching I.
                         */
                        for (; tmp2_ui32 < instr_num; tmp2_ui32++) {
                            assert(IRMETHOD_getInstructionAtPosition(method,
                                    tmp2_ui32) != NULL);
                            if (_GET_BIT_AT(tmp1_iip->metadata->reaching_definitions.in,
                                            instr_num, sizeof(JITNUINT) * 8)
                                    &&
                                    IRMETHOD_getInstructionAtPosition(method,
                                            tmp2_ui32)->metadata->liveness.def == tmp1_ui32) {
                                /*
                                 * There are multiple definitions of P reaching I:
                                 * the analysis is aborted.
                                 */
                                tmp3_ui32 = 0;
                                break;
                            }
                        }
                        if (tmp3_ui32 == 2) {
                            /* Changes P and I. */
                            tmp1_iip = tmp3_iip;
                            tmp1_ui32 = tmp3_iip->param_2.value.v;
                        }
                    } while (tmp3_ui32 == 2);
                    if (tmp3_ui32 == 1) {
                        /*
                         * P is an array pointer.
                         * Add the fetch onstruction to loop invariants list.
                         */
                        loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                    }
                }
        }
    }

    JITNUINT *tmp_invariants = (JITNUINT*) allocMemory(blocksNumber * sizeof(JITNUINT));
    JITUINT32 different = 1;

    while (different) {
        JITUINT32 i;

        /* Initialize the variables			*/
        different = 0;
        for (i = 0; i < blocksNumber; i++) {
            tmp_invariants[i] = loop->invariants[i];
        }

        /* Look for other invariants			*/
        for (instID = 0; instID < instructionsNumber; instID++) {
            ir_instruction_t *opt_inst;
            JITNUINT mask;
            JITUINT32 type;
            JITNUINT num_reaching_defs_par1 = 0;
            JITNUINT num_reaching_defs_par2 = 0;
            JITNUINT invariant_reach_par1 = 0;
            JITNUINT invariant_reach_par2 = 0;
            JITNUINT par1_is_const = 0;
            JITNUINT par2_is_const = 0;

            /* Fetch the instruction			*/
            opt_inst = IRMETHOD_getInstructionAtPosition(method, instID);
            assert(opt_inst != NULL);

            /* Check if the instruction belongs to the loop	*/
            mask = 1 << instID % (sizeof(JITNINT) * 8);
            if ((loop->belong_inst[instID / (sizeof(JITNINT) * 8)] & mask) == 0) {
                continue;
            }

            /* Check if the instruction is already an 	*
             * invariant					*/
            if (loop->invariants[instID / (sizeof(JITNINT) * 8)] & mask) {
                continue;
            }

            /* Check if the instruction may be an invariant	*/
            if ((may_invariants[instID / (sizeof(JITNINT) * 8)] & mask) == 0) {
                continue;
            }

            /* Fetch the instruction type			*/
            type = IRMETHOD_getInstructionType(opt_inst);

            /* Check if the instruction is an invariant	*/
            switch (type) {
                case IRADD:
                case IRADDOVF:
                case IRSUB:
                case IRSUBOVF:
                case IRMUL:
                case IRMULOVF:
                case IRDIV:
                case IRREM:
                case IRAND:
                case IROR:
                case IRXOR:
                case IRSHL:
                case IRSHR:
                case IRLT:
                case IRGT:
                case IREQ:
                case IRPOW:
                    num_reaching_defs_par1 = isReachedInLoop(method, loop, instID, 1, instructionsNumber);
                    num_reaching_defs_par2 = isReachedInLoop(method, loop, instID, 2, instructionsNumber);
                    invariant_reach_par2 = 0;
                    if (!IRMETHOD_isInstructionParameterAVariable(opt_inst, 1)) {
                        par1_is_const = 1;
                    } else if (num_reaching_defs_par1 == 1) {
                        if (isReachedOutOfLoop(method, loop, instID, 1) == 0) {
                            invariant_reach_par1 = isReachedByLoopInvariant(method, loop, instID, 1);
                        }
                    }
                    if (!IRMETHOD_isInstructionParameterAVariable(opt_inst, 2)) {
                        par2_is_const = 1;
                    } else if (num_reaching_defs_par2 == 1) {
                        if (isReachedOutOfLoop(method, loop, instID, 2) == 0) {
                            invariant_reach_par2 = isReachedByLoopInvariant(method, loop, instID, 2);
                        }
                    }
                    if ((par1_is_const == 1 && par2_is_const == 1)                  ||
                            (par1_is_const == 1 && num_reaching_defs_par2 == 0)         ||
                            (par1_is_const == 1 && invariant_reach_par2 == 1)           ||
                            (num_reaching_defs_par1 == 0 && par2_is_const == 1)         ||
                            (num_reaching_defs_par1 == 0 && invariant_reach_par2 == 1)  ||
                            (num_reaching_defs_par1 == 0 && invariant_reach_par2 == 1)  ||
                            (invariant_reach_par1 == 1 && par2_is_const)              ||
                            (invariant_reach_par1 == 1 && num_reaching_defs_par2 == 0)  ||
                            (invariant_reach_par1 == 1 && invariant_reach_par2 == 1)) {

                        //Add to loop invariants list
                        loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                        different	= JITTRUE;
                    }
                    break;

                case IRCONV:
                case IRCONVOVF:
                case IRNOT:
                case IRNEG:
                case IRISNAN:
                case IRISINF:
                case IRCHECKNULL:
                case IRCOSH:
                case IRCEIL:
                case IRSIN:
                case IRCOS:
                case IRACOS:
                case IRSQRT:
                case IRFLOOR:
                    num_reaching_defs_par1 = isReachedInLoop(method, loop, instID, 1, instructionsNumber);
                    if (IRMETHOD_getInstructionParameter1Type(opt_inst) != IROFFSET) {
                        par1_is_const = 1;
                    } else if (num_reaching_defs_par1 == 1) {
                        if (isReachedOutOfLoop(method, loop, instID, 1) == 0) {
                            invariant_reach_par1 = isReachedByLoopInvariant(method, loop, instID, 1);
                        }
                    }

                    if (	(par1_is_const == 1) 		||
                            (num_reaching_defs_par1 == 0) 	||
                            (invariant_reach_par1 == 1) 	) {

                        //Add to loop invariants list
                        loop->invariants[instID / (sizeof(JITNINT) * 8)] |= mask;
                        different	= JITTRUE;
                    }
                    break;
            }
        }
    }

    /* Print the invariants		*/
#ifdef PRINTDEBUG
    JITUINT32 tmp;
    PDBG_P("Loop invariants: ");
    _PRINT_BITMAP(loop->invariants, IRMETHOD_getInstructionsNumber(method), sizeof(JITNUINT) * 8, tmp);
#endif

    /* Free the memory		*/
    freeMemory(may_invariants);
    freeMemory(tmp_invariants);

    /* Return			*/
    return;
}

/**
 * \param       method	: pointer to the ir_method_t structure of the method currently under analysis
 * \param       loop	: pointer to the circuit_t structure of the loop currently under analysis
 * \param       instID	: id of the instruction
 * \param	parNum	: index of the parameter in instruction instID
 * \return	the number of definition of parameter parNum in loop loop that reaches instruction instID
 */
static inline JITUINT32 isReachedInLoop (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum, JITUINT32 instructionsNumber) {
    JITUINT32 		defID;
    JITUINT32 		reachedInLoop;
    ir_instruction_t 	*inst;
    ir_item_t		*parIRItem;
    IR_ITEM_VALUE		parValue;

    /* Fetch the instruction.
     */
    inst 		= IRMETHOD_getInstructionAtPosition(method, instID);
    assert(inst != NULL);

    /* Fetch the parameter value.
     */
    parIRItem	= IRMETHOD_getInstructionParameter(inst, parNum);
    parValue	= (parIRItem->value).v;

    /* Count the number of definitions that reach the loop.
     */
    reachedInLoop = 0;
    for (defID = 0; defID < instructionsNumber; defID++) {
        ir_instruction_t 	*inst2;
        JITNUINT 		mask;

        /* Compute the mask for the instruction	*/
        mask 	= 1 << defID % (sizeof(JITNUINT) * 8);

        /* Check if defID belong to the loop	*/
        if ((loop->belong_inst[defID / (sizeof(JITNUINT) * 8)] & mask) == 0) {
            continue;
        }

        /* Check if defID reaches instID	*/
        if (!ir_instr_reaching_definitions_reached_in_is(method, inst, defID)) {
            continue;
        }

        /* Check the current definition		*/
        inst2 = IRMETHOD_getInstructionAtPosition(method, defID);
        if (IRMETHOD_getInstructionResult(inst2)->value.v == parValue) {
            reachedInLoop++;
        }
    }

    return reachedInLoop;
}

/**
 * \param       method	: pointer to the ir_method_t structure of the method currently under analysis
 * \param       loop	: pointer to the circuit_t structure of the loop currently under analysis
 * \param       instID	: id of the instruction
 * \param	parNum	: index of the parameter in instruction instID
 * \return	1 if parameter parNum in instruction instID is reached by a loop invariant in loop; otherwise 0
 */
static inline JITBOOLEAN isReachedByLoopInvariant (ir_method_t *method, circuit_t *loop, JITUINT32 instID, JITUINT32 parNum) {
    JITUINT32		instructionsNumber;
    JITUINT32 		defID;
    ir_instruction_t	*inst;

    /* Fetch the instruction.
     */
    inst			= IRMETHOD_getInstructionAtPosition(method, instID);
    assert(inst != NULL);

    instructionsNumber	= IRMETHOD_getInstructionsNumber(method);
    for (defID = 0; defID < instructionsNumber; defID++) {
        ir_instruction_t	*defInst;
        JITNUINT mask = 1 << defID % (sizeof(JITNUINT) * 8);

        //if defID reaches instID
        //if (in[defID/(sizeof(JITNUINT)*8)] & mask)
        if (!ir_instr_reaching_definitions_reached_in_is(method, inst, defID)) {
            continue ;
        }

        //if defID does not belong to the loop, do not consider it.
        if (((loop->belong_inst[defID / (sizeof(JITNUINT) * 8)] & mask)) == 0) {
            continue ;
        }

        //if defID is a loop invariant, then it is ok.
        if ((loop->invariants[defID / (sizeof(JITNUINT) * 8)] & mask) != 0) {
            continue ;
        }

        /* The current definition is not a loop invariant.
         * Check what it actually defines.
         */
        defInst	= IRMETHOD_getInstructionAtPosition(method, defID);
        assert(defInst != NULL);
        if (parNum == 1) {
            if (IRMETHOD_getInstructionResultValue(defInst) == IRMETHOD_getInstructionParameter1Value(inst)) {
                return JITFALSE;
            }
        } else if (parNum == 2) {
            if (IRMETHOD_getInstructionResultValue(defInst) == IRMETHOD_getInstructionParameter2Value(inst)) {
                return JITFALSE;
            }
        }
    }

    return JITTRUE;
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
    ir_instruction_t *inst;
    JITUINT32 defID;

    /* Assertions				*/
    assert(method != NULL);
    assert(loop != NULL);

    /* Fetch the instruction		*/
    inst = IRMETHOD_getInstructionAtPosition(method, instID);
    assert(inst != NULL);

    /* Count the number of definitions	*/
    for (defID = 0; defID < IRMETHOD_getInstructionsNumber(method); defID++) {
        JITNUINT mask;
        ir_instruction_t *definer;

        /* Compute the mask			*/
        mask = 1 << defID % (sizeof(JITNUINT) * 8);

        /* Check if defID reaches instID	*/
        if (!ir_instr_reaching_definitions_reached_in_is(method, inst, defID)) {
            continue;
        }

        /* Check if defID does not belong to the loop	*/
        if ((loop->belong_inst[defID / (sizeof(JITNUINT) * 8)] & mask)) {
            continue ;
        }

        /* Fetch the instruction type of the definer	*/
        definer	= IRMETHOD_getInstructionAtPosition(method, defID);

        /* Check the definer				*/
        if (parNum == 1) {
            if (IRMETHOD_getInstructionResult(definer)->value.v == IRMETHOD_getInstructionParameter1Value(inst)) {
                reachedOutOfLoop++;
            } else {
            }
        } else if (parNum == 2) {
            if (IRMETHOD_getInstructionResult(definer)->value.v == IRMETHOD_getInstructionParameter2Value(inst)) {
                reachedOutOfLoop++;
            }
        }
    }

    /* Return				*/
    return reachedOutOfLoop;
}

static inline JITUINT64 loopinvariants_get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline JITBOOLEAN internal_is_memory_invariant (ir_method_t *method, circuit_t *loop, ir_instruction_t *inst) {
    XanList		*l;
    XanListItem	*item;
    JITBOOLEAN	isInvariant;
    ir_item_t	*baseAddress;
    ir_item_t	*offset;
    JITNUINT 	mask;

    /* Assertions				*/
    assert(method != NULL);
    assert(loop != NULL);
    assert(inst != NULL);

    /* Check the instruction		*/
    switch(inst->type) {
        case IRLOADREL:
            break;
        default:
            return JITFALSE;
    }

    /* Fetch the list of instructions	*/
    l 		= IRMETHOD_getCircuitInstructions(method, loop);
    assert(l != NULL);

    /* Fetch the base address of the 	*
     * instruction				*/
    baseAddress	= IRMETHOD_getInstructionParameter1(inst);

    /* Fetch the offset			*/
    offset		= IRMETHOD_getInstructionParameter2(inst);

    /* Check if there is no data dependence	*
     * with the current instruction and the	*
     * address is defined by an invariant.	*
     * In this case, it is loop invariant	*/
    mask 		= 1 << (inst->ID) % (sizeof(JITNINT) * 8);
    isInvariant	= JITTRUE;
    item		= xanList_first(l);
    while (item != NULL) {
        ir_instruction_t	*inst2;
        inst2	= item->data;

        /* Check the dependences.
         */
        if (	(inst != inst2)						&&
                (IRMETHOD_canInstructionPerformAMemoryStore(inst2))	) {
            if (	IRMETHOD_isInstructionDataDependentThroughMemory(method, inst, inst2)	||
                    IRMETHOD_isInstructionDataDependentThroughMemory(method, inst2, inst)	) {
                isInvariant	= JITFALSE;
                break ;
            }
        }

        /* Check the base address	*/
        if (	(baseAddress->type == IROFFSET)								&&
                (IRMETHOD_getVariableDefinedByInstruction(method, inst2) == (baseAddress->value).v)	&&
                ((loop->invariants[(inst2->ID) / (sizeof(JITNINT) * 8)] & mask) == 0)			) {
            isInvariant	= JITFALSE;
            break ;
        }

        /* Check the offset		*/
        if (	(offset->type == IROFFSET)								&&
                (IRMETHOD_getVariableDefinedByInstruction(method, inst2) == (offset->value).v)		&&
                ((loop->invariants[(inst2->ID) / (sizeof(JITNINT) * 8)] & mask) == 0)			) {
            isInvariant	= JITFALSE;
            break ;
        }

        item	= item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(l);

    /* Return				*/
    return isInvariant;
}
