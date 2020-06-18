/*
 * Copyright (C) 2006 - 2010  Campanoni Simone
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
#include <optimizer_liveness.h>
#include <config.h>
// End

#define INFORMATIONS    "This step compute the liveness analysis for each instruction without using the basic block"
#define AUTHOR          "Campanoni Simone"

static inline JITUINT64 get_ID_job (void);
static inline char * get_version (void);
static inline void liveness_do_job (ir_method_t *method);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline JITUINT64 get_dependences (void);
static inline JITUINT64 get_invalidations (void);
static inline void init_def_use (ir_method_t *method, JITUINT32 instructionsNumber, ir_instruction_t **insns);
static inline void compute_the_liveness (ir_method_t *method, XanList **successors, JITUINT32 instructionsNumber, ir_instruction_t **insns);
static inline JITINT32 compute_liveness_step (ir_method_t *method, XanList **successors, JITUINT32 instructionsNumber, ir_instruction_t **insns);
static inline void l_shutdown (JITFLOAT32 totalTime);
static inline void l_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void print_sets (ir_method_t *method);
static inline void liveness_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void liveness_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    l_init,
    l_shutdown,
    liveness_do_job,
    get_version,
    get_informations,
    get_author,
    get_invalidations,
    NULL,
    liveness_getCompilationFlags,
    liveness_getCompilationTime
};

static inline void l_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);

    /* Return			*/
    return;
}

static inline void l_shutdown (JITFLOAT32 totalTime) {

}

static inline JITUINT64 get_ID_job (void) {
    return LIVENESS_ANALYZER;
}

static inline JITUINT64 get_dependences (void) {
    return 0;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void liveness_do_job (ir_method_t *method) {
    XanList         **successors;
    ir_instruction_t **insns;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Check if there is variables		*/
    if (method->var_max == 0) {
        PDEBUG("OPTIMIZER_LIVENESS:     There is no variables\n");
        return;
    }

    /* Fetch the instructions number*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Fetch the instructions.
     */
    insns   = IRMETHOD_getInstructionsWithPositions(method);

    /* Compute the set of successors	*/
    successors 	= IRMETHOD_getInstructionsSuccessors(method);

    /* Init the def, use sets		*/
    init_def_use(method, instructionsNumber, insns);

    /* Print the DEF, USE sets		*/
#ifdef DEBUG
    PDEBUG("OPTIMIZER_LIVENESS: Sets after the definition of DEF and USE sets\n");
    print_sets(method);
#endif

    /* Compute the liveness.
     */
    compute_the_liveness(method, successors, instructionsNumber, insns);

    /* Free the memory.
     */
    freeFunction(insns);
    IRMETHOD_destroyInstructionsSuccessors(successors, instructionsNumber);

    return;
}

static inline void init_def_use (ir_method_t *method, JITUINT32 instructionsNumber, ir_instruction_t **insns) {
    ir_instruction_t        *opt_inst;
    JITINT32 use_size;
    JITINT32 count;
    JITUINT32 instID;
    JITUINT32 blocksNumber;
    JITNUINT temp;
    JITUINT32 trigger;
    JITUINT32 parametersNumber;

    /* Assertions			*/
    assert(method != NULL);

    /* Init the variables		*/
    opt_inst = NULL;
    use_size = 0;
    count = 0;
    instID = 0;
    blocksNumber = 0;

    /* Fetch the parameters number	*/
    parametersNumber = IRMETHOD_getMethodParametersNumber(method);

    /* Fetch the USE size		*/
    PDEBUG("OPTIMIZER_LIVENESS: Make use set with %d size\n", method->var_max);
    use_size = IRMETHOD_getNumberOfVariablesNeededByMethod(method);

    /* Compute how many block we	*
     * have to allocate for the	*
     * USE set			*/
    blocksNumber = (use_size / (sizeof(JITNINT) * 8)) + 1;
    PDEBUG("OPTIMIZER_LIVENESS: USE block number = %d\n", blocksNumber);
    IRMETHOD_allocMemoryForLivenessAnalysis(method, blocksNumber);
    assert(method->livenessBlockNumbers > 0);
    assert(method->livenessBlockNumbers == blocksNumber);

    /* Create the DEF, USE set for	*
     * the first dummy instructions	*
     * which are about the input	*
     * parameters of the method.	*/
    for (instID = 0; instID < parametersNumber; instID++) {

        /* Fetch the instruction	*/
        opt_inst = insns[instID];
        assert(opt_inst != NULL);
        assert(IRMETHOD_getInstructionType(opt_inst) == IRNOP);
        assert(opt_inst->metadata != NULL);
        assert((opt_inst->metadata->liveness).in != NULL);
        assert((opt_inst->metadata->liveness).use != NULL);
        assert((opt_inst->metadata->liveness).out != NULL);

        /* Init the sets		*/
        livenessSetDef(method, opt_inst, -1);
        for (count = 0; count < blocksNumber; count++) {
            livenessSetUse(method, opt_inst, count, 0);
            livenessSetIn(method, opt_inst, count, 0);
            livenessSetOut(method, opt_inst, count, 0);
        }

        /* Init the DEF set		*/
        livenessSetDef(method, opt_inst, instID);
    }

    /* Create the DEF, USE set for	*
     * each instruction		*/
    PDEBUG("OPTIMIZER_LIVENESS: Init the USE DEF sets\n");
    for (; instID < instructionsNumber; instID++) {
        JITUINT32 	count;
        ir_item_t       *result;
        ir_item_t       *par1;

        PDEBUG("OPTIMIZER_LIVENESS:     Inst %d\n", instID);

        /* Fetch the instruction	*/
        opt_inst = insns[instID];
        assert(opt_inst != NULL);
        assert(opt_inst->metadata != NULL);
        assert((opt_inst->metadata->liveness).in != NULL);
        assert((opt_inst->metadata->liveness).use != NULL);
        assert((opt_inst->metadata->liveness).out != NULL);

        /* Init the sets		*/
        livenessSetDef(method, opt_inst, -1);
        for (count = 0; count < method->livenessBlockNumbers; count++) {
            livenessSetUse(method, opt_inst, count, 0);
            livenessSetIn(method, opt_inst, count, 0);
            livenessSetOut(method, opt_inst, count, 0);
        }

        /* Cache some pointers		*/
        result = IRMETHOD_getIRItemOfVariableDefinedByInstruction(method, opt_inst);
        par1 = IRMETHOD_getInstructionParameter1(opt_inst);
        assert(par1 != NULL);

        /* Init the DEF set		*/
        if (result != NULL) {
            assert(result->type == IROFFSET);
            livenessSetDef(method, opt_inst, (result->value).v);
            PDEBUG("OPTIMIZER_LIVENESS:             Define the variable %lu\n", (opt_inst->liveness).getDef(method, opt_inst));
        }

        /* Init the USE set		*/
        if (	(par1->type == IROFFSET)		&&
                (opt_inst->type != IRGETADDRESS)	) {
            PDEBUG("OPTIMIZER_LIVENESS:             Use the variable %lld\n", method->getInstrPar1Value(opt_inst));
            trigger = (par1->value).v / (sizeof(JITNINT) * 8);
            temp = 0x1;
            temp = temp << ((par1->value).v % (sizeof(JITNINT) * 8));
            livenessSetUse(method, opt_inst, trigger, livenessGetUse(method, opt_inst, trigger) | temp);
        }
        switch (IRMETHOD_getInstructionParametersNumber(opt_inst)) {
            case 3:
                if (IRMETHOD_getInstructionParameter3Type(opt_inst) == IROFFSET) {
                    PDEBUG("OPTIMIZER_LIVENESS:                 Use the variable %lld\n", IRMETHOD_getInstructionParameter3Value(opt_inst));
                    trigger = (IRMETHOD_getInstructionParameter3Value(opt_inst) / (sizeof(JITNINT) * 8));
                    temp = 0x1;
                    temp = temp << (IRMETHOD_getInstructionParameter3Value(opt_inst) % (sizeof(JITNINT) * 8));
                    livenessSetUse(method, opt_inst, trigger, livenessGetUse(method, opt_inst, trigger) | temp);
                }
            case 2:
                if (IRMETHOD_getInstructionParameter2Type(opt_inst) == IROFFSET) {
                    PDEBUG("OPTIMIZER_LIVENESS:                 Use the variable %lld\n", IRMETHOD_getInstructionParameter2Value(opt_inst));
                    trigger = (IRMETHOD_getInstructionParameter2Value(opt_inst) / (sizeof(JITNINT) * 8));
                    temp = 0x1;
                    temp = temp << (IRMETHOD_getInstructionParameter2Value(opt_inst) % (sizeof(JITNINT) * 8));
                    livenessSetUse(method, opt_inst, trigger, livenessGetUse(method, opt_inst, trigger) | temp);
                }
        }
        if (IRMETHOD_isACallInstruction(opt_inst)) {
            JITUINT32   callParametersNumber;
            callParametersNumber    = IRMETHOD_getInstructionCallParametersNumber(opt_inst);
            for (count=0; count < callParametersNumber; count++) {
                ir_item_t   *irCallParameterItem;
                irCallParameterItem = IRMETHOD_getInstructionCallParameter(opt_inst, count);
                if (irCallParameterItem->type == IROFFSET) {
                    PDEBUG("OPTIMIZER_LIVENESS:                 Use the variable %lld\n", IRMETHOD_getInstructionCallParameterValue(opt_inst, count));
                    trigger = (IRMETHOD_getInstructionCallParameterValue(opt_inst, count) / (sizeof(JITNINT) * 8));
                    temp = 0x1;
                    temp = temp << (IRMETHOD_getInstructionCallParameterValue(opt_inst, count) % (sizeof(JITNINT) * 8));
                    livenessSetUse(method, opt_inst, trigger, livenessGetUse(method, opt_inst, trigger) | temp);
                }
            }
        }
    }
}

static inline void print_sets (ir_method_t *method) {
    ir_instruction_t        *opt_inst;
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    /* Init the variables		*/
    count = 0;
    opt_inst = NULL;

    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        JITINT32 count2;

        /* Fetch the instruction	*/
        opt_inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(opt_inst != NULL);

        PDEBUG("OPTIMIZER_LIVENESS: <Instruction %d>    Type    = %u\n", count, IRMETHOD_getInstructionType(opt_inst));
        PDEBUG("OPTIMIZER_LIVENESS:                     DEF	= %lu\n", (opt_inst->liveness).getDef(method, opt_inst));
        PDEBUG("OPTIMIZER_LIVENESS:                     USE	= 0x");
        for (count2 = 0; count2 < (method->livenessBlockNumbers); count2++) {
            PDEBUG("%X", ir_instr_liveness_use_get(method, opt_inst, count2));
        }
        PDEBUG("\n");
        PDEBUG("OPTIMIZER_LIVENESS:                     IN	= ");
        for (count2 = 0; count2 < IRMETHOD_getNumberOfVariablesNeededByMethod(method); count2++) {
            if (IRMETHOD_isVariableLiveIN(method, opt_inst, count2)) {
                PDEBUG("%d ", count2);
            }
        }
        PDEBUG("\n");
        PDEBUG("OPTIMIZER_LIVENESS:                     OUT	= ");
        for (count2 = 0; count2 < IRMETHOD_getNumberOfVariablesNeededByMethod(method); count2++) {
            if (IRMETHOD_isVariableLiveOUT(method, opt_inst, count2)) {
                PDEBUG("%d ", count2);
            }
        }
        PDEBUG("\n");
    }
}

static inline void compute_the_liveness (ir_method_t *method, XanList **successors, JITUINT32 instructionsNumber, ir_instruction_t **insns) {
    JITBOOLEAN modified;

    /* Assertions.
     */
    assert(method != NULL);
    assert(successors != NULL);

    /* Compute the liveness	of variables.
     */
    PDEBUG("OPTIMIZER_LIVENESS: Begin to compute the liveness\n");
    do {
        PDEBUG("OPTIMIZER_LIVENESS:     Computation step\n");
        modified = compute_liveness_step(method, successors, instructionsNumber, insns);

#ifdef DEBUG
        PDEBUG("OPTIMIZER_LIVENESS:     Sets computed\n");
        print_sets(method);
#endif
    } while (modified);

    return;
}

static inline JITINT32 compute_liveness_step (ir_method_t *method, XanList **successors, JITUINT32 instructionsNumber, ir_instruction_t **insns) {
    JITNUINT temp;
    JITNUINT temp2;
    JITUINT32 trigger;
    JITUINT32 count;
    JITUINT32 inst_count;
    ir_instruction_t        *inst;
    ir_instruction_t        *inst2;
    ir_instruction_t        *inst3;
    JITINT32 modified;
    JITUINT32 livenessBlockNumbers;

    /* Assertions				*/
    assert(method != NULL);

    /* Init the variables			*/
    modified = JITFALSE;
    inst = NULL;
    inst2 = NULL;
    inst3 = NULL;

    /* Compute the liveness			*/
    livenessBlockNumbers = method->livenessBlockNumbers;
    for (inst_count = instructionsNumber; inst_count > 0; inst_count--) {
        JITNUINT out_def;
        JITNUINT old;
        JITUINT32 instID;
        XanListItem     *item;

        /* Compute the instruction ID	*/
        instID = inst_count - 1;

        /* Fetch the instruction	*/
        PDEBUG("OPTIMIZER_LIVENESS: Liveness_step: Inst %d\n", instID);
        inst = insns[instID];
        assert(inst != NULL);

        /* Init the variables		*/
        inst2 = NULL;
        inst3 = NULL;

        /* Fetch the successor		*/
        item = xanList_first(successors[inst->ID]);
        if (item != NULL) {
            inst2 = item->data;
            assert(inst2 != NULL);
            item = item->next;
            if (item != NULL) {
                inst3 = item->data;
                assert(inst3 != NULL);
            }
        }

        /* Compute the out[n]		*/
        if (inst2 != NULL) {
            for (count = 0; count < livenessBlockNumbers; count++) {
                JITNUINT new;
                old = livenessGetOut(method, inst, count);
                PDEBUG("OPTIMIZER_LIVENESS:     Before Out[%d][%d]                      = 0x%X\n", instID, count, (inst->liveness).getOut(method, inst, count));
                PDEBUG("OPTIMIZER_LIVENESS:             In[successore][%d]              = 0x%X\n", count, (inst2->liveness).getIn(method, inst2, count));
                //livenessSetOut(method, inst, count, livenessGetOut(method, inst, count) | livenessGetIn(method, inst2, count));
                new = old | livenessGetIn(method, inst2, count);
                PDEBUG("OPTIMIZER_LIVENESS:     After Out[%d][%d]                       = 0x%X\n", instID, count, (inst->liveness).getOut(method, inst, count));
                if (old != new) {
                    modified = JITTRUE;
                    livenessSetOut(method, inst, count, new);
                }
            }
        }
        if (inst3 != NULL) {
            for (count = 0; count < livenessBlockNumbers; count++) {
                JITNUINT new;
                old = livenessGetOut(method, inst, count);
                PDEBUG("OPTIMIZER_LIVENESS:     Before Out[%d][%d]                      = 0x%X\n", instID, count, (inst->liveness).getOut(method, inst, count));
                PDEBUG("OPTIMIZER_LIVENESS:             In[successore2][%d]             = 0x%X\n", count, livenessGetIn(method, inst3, count));
                //livenessSetOut(method, inst, count, livenessGetOut(method, inst, count) | livenessGetIn(method, inst3, count));
                new = old | livenessGetIn(method, inst3, count);
                PDEBUG("OPTIMIZER_LIVENESS:     After Out[%d][%d]                       = 0x%X\n", instID, count, (inst->liveness).getOut(method, inst, count));
                if (old != new) {
                    modified = JITTRUE;
                    livenessSetOut(method, inst, count, new);
                }
            }
        }

        /* Compute the in[n]		*/
        if (livenessGetDef(method, inst) != -1) {
            trigger = (livenessGetDef(method, inst) / (sizeof(JITNINT) * 8));
            temp2 = JITMAXNUINT;
            temp = 0x1;
            temp = temp << (livenessGetDef(method, inst) % (sizeof(JITNINT) * 8));
            temp = temp2 - temp;
            out_def = livenessGetOut(method, inst, trigger) & temp;
            PDEBUG("OPTIMIZER_LIVENESS:     Out[trigger] - DEF	= 0x%X\n", out_def);
            for (count = 0; count < livenessBlockNumbers; count++) {
                JITNUINT new;
                old = livenessGetIn(method, inst, count);
                PDEBUG("OPTIMIZER_LIVENESS:     Before In[%d][%d]                       = 0x%X\n", instID, count, livenessGetIn(method, inst, count));
                PDEBUG("OPTIMIZER_LIVENESS:             Use[%d][%d]                     = 0x%X\n", instID, count, (inst->liveness).getUse(method, inst, count));
                if (count != trigger) {
                    //livenessSetIn(method, inst, count, livenessGetUse(method, inst, count) | livenessGetOut(method, inst, count));
                    new = livenessGetUse(method, inst, count) | livenessGetOut(method, inst, count);
                    PDEBUG("OPTIMIZER_LIVENESS:             Out[%d][%d]                     = 0x%X\n", instID, count, (inst->liveness).getOut(method, inst, count));
                } else {
                    //livenessSetIn(method, inst, count, livenessGetUse(method, inst, count) | out_def);
                    new = livenessGetUse(method, inst, count) | out_def;
                    PDEBUG("OPTIMIZER_LIVENESS:             Out - Def[%d][%d]		= 0x%X\n", instID, count, out_def);
                }
                PDEBUG("OPTIMIZER_LIVENESS:     After In[%d][%d]                        = 0x%X\n", instID, count, livenessGetIn(method, inst, count));
                if (old != new) {
                    modified = JITTRUE;
                    livenessSetIn(method, inst, count, new);
                }
            }
        } else {
            for (count = 0; count < livenessBlockNumbers; count++) {
                JITNUINT new;
                old = livenessGetIn(method, inst, count);
                PDEBUG("OPTIMIZER_LIVENESS:     Before In[%d][%d]                       = 0x%X\n", instID, count, (inst->liveness).getIn(method, inst, count));
                PDEBUG("OPTIMIZER_LIVENESS:             Use[%d][%d]                     = 0x%X\n", instID, count, (inst->liveness).getUse(method, inst, count));
                PDEBUG("OPTIMIZER_LIVENESS:             Out[%d][%d]                     = 0x%X\n", instID, count, (inst->liveness).getOut(method, inst, count));
                //livenessSetIn(method, inst, count, livenessGetUse(method, inst, count) | livenessGetOut(method, inst, count));
                new = livenessGetUse(method, inst, count) | livenessGetOut(method, inst, count);
                PDEBUG("OPTIMIZER_LIVENESS:     After In[%d][%d]                        = 0x%X\n", instID, count, (inst->liveness).getIn(method, inst, count));
                if (old != new) {
                    modified = JITTRUE;
                    livenessSetIn(method, inst, count, new);
                }
            }
        }
#ifdef PRINTDEBUG
        print_sets(method);
#endif
    }

    return modified;
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

static inline void liveness_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void liveness_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
