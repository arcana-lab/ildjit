/*
 * Copyright (C) 2007 - 2009  Campanoni Simone
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
#include <platform_API.h>
#include <ir_method.h>

// My headers
#include <optimizer_irchecker.h>
#include <config.h>
// End

#define INFORMATIONS    "This is a irchecker plugin"
#define AUTHOR          "Campanoni Simone"
#define DIM_BUF	1024

static inline void internal_compute_gen_kill (ir_method_t *method, data_flow_t *data);
static inline void internal_openFile (void);
static inline void irchecker_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void irchecker_getCompilationFlags (char *buffer, JITINT32 bufferLength);
static inline JITUINT64 irchecker_get_ID_job (void);
static inline char * irchecker_get_version (void);
static inline void irchecker_do_job (ir_method_t * method);
static inline char * irchecker_get_informations (void);
static inline char * irchecker_get_author (void);
static inline JITUINT64 irchecker_get_dependences (void);
static inline void irchecker_shutdown (JITFLOAT32 totalTime);
static inline void irchecker_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void internal_error_successors_predecessors (ir_method_t *method, ir_instruction_t *inst, XanList **tableS, XanList **tableP);
static inline void irchecker_successors_predecessors (ir_method_t * method);
static inline void irchecker_check_finally_blocks (ir_method_t * method);
static inline void irchecker_check_catcher_blocks (ir_method_t *method);
static inline void internal_print_list (XanList *l);
static inline JITUINT64 irchecker_get_invalidations (void);
static inline void irchecker_instructions (ir_method_t *method);
static inline void irchecker_variables (ir_method_t *method);
static inline void irchecker_variable_uses (ir_method_t *method);
static inline void irchecker_variable_check_intialization (ir_method_t *method);
static inline void irchecker_variable_defines (ir_method_t *method);
static inline void internal_abort (ir_method_t *method);
static inline JITBOOLEAN internal_data_flow_step (data_flow_t *data, XanList **predecessors);
static inline void internal_print_dataflow (data_flow_t *df);

JITINT8	nameBuffer[DIM_BUF];
ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;
FILE		*outFile;

ir_optimization_interface_t plugin_interface = {
    irchecker_get_ID_job,
    irchecker_get_dependences,
    irchecker_init,
    irchecker_shutdown,
    irchecker_do_job,
    irchecker_get_version,
    irchecker_get_informations,
    irchecker_get_author,
    irchecker_get_invalidations,
    NULL,
    irchecker_getCompilationFlags,
    irchecker_getCompilationTime
};

static inline JITUINT64 irchecker_get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void irchecker_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
    assert(outputPrefix != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
    outFile	= NULL;

    return ;
}

static inline void irchecker_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
    irOptimizer = NULL;
    prefix = NULL;

    /* Check if we have opened a file.
     */
    if (	(outFile != NULL)	&&
            (outFile != stdout)	) {
        JITBOOLEAN	empty;

        /* Check whether we have printed something or not.
         */
        fseek(outFile, 0L, SEEK_END);
        empty	= (ftell(outFile) == 0);

        /* Close the file.
         */
        fclose(outFile);

        /* We did not generate any output.
         * Remove the file.
         */
        if (empty) {
            unlink((char *)nameBuffer);
        }
    }

    return ;
}

static inline JITUINT64 irchecker_get_ID_job (void) {
    return IR_CHECKER;
}

static inline JITUINT64 irchecker_get_dependences (void) {
    return BASIC_BLOCK_IDENTIFIER | ESCAPES_ANALYZER | LIVENESS_ANALYZER;
}

static inline void irchecker_do_job (ir_method_t *method) {
    JITINT8	*env;
    JITINT8	fileName[DIM_BUF];

    /* Assertions			*/
    assert(method != NULL);

    /* Open the file.
     */
    internal_openFile();

    /* Fetch the dotprinter file	*
     * name				*/
    env	= (JITINT8 *) getenv("DOTPRINTER_FILENAME");
    SNPRINTF(fileName, DIM_BUF, "irchecker_%s", IRPROGRAM_getProgramName());
    setenv("DOTPRINTER_FILENAME", (char *)fileName, 1);

    /* Check the catcher blocks	*/
    irchecker_check_catcher_blocks(method);

    /* Check the final blocks	*/
    irchecker_check_finally_blocks(method);

    /* Check the successors and     *
     * predecessors			*/
//	irchecker_successors_predecessors(method);

    /* Check the instructions	*/
    irchecker_instructions(method);

    /* Check variables		*/
    irchecker_variables(method);

    /* Restore the variable name	*/
    if (env == NULL) {
        env = (JITINT8 *)"";
    }
    setenv("DOTPRINTER_FILENAME", (char *)env, 1);

    /* Return			*/
    return;
}

static inline void irchecker_variables (ir_method_t *method) {

    irchecker_variable_defines(method);

    //irchecker_variable_uses(method);
    irchecker_variable_check_intialization(method);

    return ;
}

static inline void irchecker_variable_defines (ir_method_t *method) {
    JITUINT32	count;

    if (!IRMETHOD_hasMethodInstructions(method)){
        return ;
    }

    for (count = 0; count < IRMETHOD_getMethodParametersNumber(method); count++) {
        ir_instruction_t        *opt_inst;

        /* Fetch the instruction.
         */
        opt_inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(opt_inst != NULL);
        if (IRMETHOD_getInstructionType(opt_inst) != IRNOP){
            fprintf(outFile, "IRCHECKER: Parameter %u has no IRNOP instruction\n", count);
            internal_abort(method);
        }
    }

    for (count = 0; count < IRMETHOD_getNumberOfVariablesNeededByMethod(method); count++) {
        XanList	*uses;

        if (IRMETHOD_isAnEscapedVariable(method, count)) {
            continue ;
        }
        uses	= IRMETHOD_getVariableUses(method, count);
        if (xanList_length(uses) > 0) {
            XanList	*defs;
            defs	= IRMETHOD_getVariableDefinitions(method, count);
            if (xanList_length(defs) == 0) {
                fprintf(outFile, "IRCHECKER: Variable %u is not defined\n", count);
                internal_abort(method);
            }

            /* Free the memory.
             */
            xanList_destroyList(defs);
        }

        /* Free the memory.
         */
        xanList_destroyList(uses);
    }

    return ;
}

static inline void irchecker_variable_check_intialization (ir_method_t *method) {
    JITUINT32 		count;
    JITUINT32 		instructionsNumber;
    JITUINT32 		varsNumber;
    JITUINT32		catcherInstID;
    JITBOOLEAN		modified;
    ir_instruction_t	*catcherInst;
    XanList			**predecessors;
    data_flow_t		*df;

    /* Assertions.
     */
    assert(method != NULL);

    /* Check if there are variables.
     */
    if (IRMETHOD_getNumberOfVariablesNeededByMethod(method) == 0) {
        return ;
    }

    /* Allocate the data sets.
     */
    instructionsNumber	= IRMETHOD_getInstructionsNumber(method);
    varsNumber		= IRMETHOD_getNumberOfVariablesNeededByMethod(method);
    df			= DATAFLOW_allocateSets(instructionsNumber, varsNumber);
    assert(df != NULL);

    /* Fetch the catcher instruction.
     */
    catcherInstID		= instructionsNumber + 1;
    catcherInst		= IRMETHOD_getCatcherInstruction(method);
    if (catcherInst != NULL) {
        catcherInstID	= catcherInst->ID;
    }

    /* Define GEN and KILL sets.
     */
    internal_compute_gen_kill(method, df);

    /* Set IN and OUT to be full sets.
     */
    DATAFLOW_setAllFullSets(df, JITFALSE, JITFALSE, JITTRUE, JITTRUE);

    /* Set IN set of entry point of the method to be empty.
     */
    DATAFLOW_setEmptySets(df, 0, JITFALSE, JITFALSE, JITTRUE, JITFALSE);

    /* Compute the data flow.
     */
    predecessors	= IRMETHOD_getInstructionsPredecessors(method);
    do {
        modified	= internal_data_flow_step(df, predecessors);
    } while (modified);

    /* Check whether all variables are initialized before their first usage.
     */
    for (count = 0; count < instructionsNumber && count < catcherInstID; count++) {
        ir_instruction_t	*i;
        XanList			*vars;
        XanListItem		*item;

        /* Fetch the instruction.
         */
        i	= IRMETHOD_getInstructionAtPosition(method, count);

        /* Fetch the variables used by the current instruction.
         */
        vars	= IRMETHOD_getVariablesUsedByInstruction(i);

        /* Check all variables used by the current instruction.
         */
        item	= xanList_first(vars);
        while (item != NULL) {
            ir_item_t	*v;
            v	= item->data;
            if (	(!IRMETHOD_isAnEscapedVariable(method, (v->value).v))		&&
                    (!DATAFLOW_doesElementExistInINSet(df, count, (v->value).v))	) {
                fprintf(outFile, "WARNING: Variable %llu is used by instruction %u and it can be not initialized\n", (v->value).v, count);
                internal_print_dataflow(df);
            }
            item	= item->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(vars);
    }

    /* Free the memory.
     */
    DATAFLOW_destroySets(df);
    IRMETHOD_destroyInstructionsPredecessors(predecessors, instructionsNumber);

    return ;
}

static inline void irchecker_variable_uses (ir_method_t *method) {
    JITUINT32 count;
    ir_item_t       **vars;
    XanList         *toDelete;

    /* Assertions			*/
    assert(method != NULL);

    /* Check if there are variables	*/
    if (IRMETHOD_getNumberOfVariablesNeededByMethod(method) == 0) {
        return ;
    }

    /* Allocate the table of        *
     * variables			*/
    vars = allocFunction(sizeof(ir_item_t) * IRMETHOD_getNumberOfVariablesNeededByMethod(method));
    assert(vars != NULL);

    /* Allocate the list of         *
     * structures to delete		*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);
    assert(toDelete != NULL);

    /* Fetch the variables		*
     * information			*/
    for (count = 0; count < IRMETHOD_getNumberOfVariablesNeededByMethod(method); count++) {

        /* Fetch the variable		*/
        vars[count] = IRMETHOD_getIRVariable(method, count);
        if (vars[count] == NULL) {
            ir_item_t       *var;
            TypeDescriptor 	*varType;
            JITUINT16 irType;

            /* The variable is a paramter   */
            varType = NULL;
            if (IRMETHOD_isTheVariableAMethodParameter(method, count)) {
                varType	= IRMETHOD_getParameterILType(method, count);
                irType = IRMETHOD_getParameterType(method, count);
            } else if (IRMETHOD_isTheVariableAMethodDeclaredVariable(method, count)) {
                varType	= IRMETHOD_getDeclaredVariableILType(method, count);
                irType = IRMETHOD_getDeclaredVariableType(method, count);
            }
            if (varType != NULL) {
                var = allocFunction(sizeof(ir_item_t));
                assert(var != NULL);
                (var->value).v = count;
                var->type = IROFFSET;
                var->internal_type = irType;
                var->type_infos = varType;
                vars[count] = var;
                xanList_append(toDelete, var);
            }
        }
    }

    /* Check that every IR variable	*
     * has the same type inside	*
     * every item			*/
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        ir_instruction_t        *inst;
        ir_item_t               *var;
        JITUINT32 count2;

        /* Fetch the instruction	*/
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);

        /* Check if the instruction can	*
         * be checked			*/
        switch (IRMETHOD_getInstructionType(inst)) {
            case IRALLOCA:
                continue;
        }

        /* Check if it uses the variable*
         * and if this is the case, if	*
         * the information matches the	*
         * information we have in the   *
         * table			*/
        switch (IRMETHOD_getInstructionParametersNumber(inst)) {
            case 3:
                var = IRMETHOD_getInstructionParameter3(inst);
                assert(var != NULL);
                if (    (var->type == IROFFSET)                                 &&
                        (vars[(var->value).v] != NULL)			&&
                        (memcmp(var, vars[(var->value).v], sizeof(ir_item_t)) != 0)     ) {
                    fprintf(outFile, "IRCHECKER:  The third parameter of instruction %d uses the variable %llu without keeping its right information in the structure\n", inst->ID, (var->value).v);
                    internal_abort(method);
                }
            case 2:
                var = IRMETHOD_getInstructionParameter2(inst);
                assert(var != NULL);
                if (    (var->type == IROFFSET)                                 &&
                        (vars[(var->value).v] != NULL)			&&
                        (memcmp(var, vars[(var->value).v], sizeof(ir_item_t)) != 0)     ) {
                    fprintf(outFile, "IRCHECKER:  The second parameter of instruction %d uses the variable %llu without keeping its right information in the structure\n", inst->ID, (var->value).v);
                    internal_abort(method);
                }
            case 1:
                var = IRMETHOD_getInstructionParameter1(inst);
                assert(var != NULL);
                if (    (var->type == IROFFSET)                                 &&
                        (vars[(var->value).v] != NULL)			&&
                        (memcmp(var, vars[(var->value).v], sizeof(ir_item_t)) != 0)     ) {
                    fprintf(outFile, "IRCHECKER:  The first parameter of instruction %d uses the variable %llu without keeping its right information in the structure\n", inst->ID, (var->value).v);
                    internal_abort(method);
                }
        }
        var = IRMETHOD_getInstructionResult(inst);
        assert(var != NULL);
        if (    (var->type == IROFFSET)                                 &&
                (vars[(var->value).v] != NULL)				&&
                (memcmp(var, vars[(var->value).v], sizeof(ir_item_t)) != 0)     ) {
            fprintf(outFile, "IRCHECKER:      The result of instruction %d uses the variable %llu without keeping its right information in the structure\n", inst->ID, (var->value).v);
            internal_abort(method);
        }
        for (count2 = 0; count2 < IRMETHOD_getInstructionCallParametersNumber(inst); count2++) {
            var = IRMETHOD_getInstructionCallParameter(inst, count2);
            assert(var != NULL);
            if (    (var->type == IROFFSET)                                 &&
                    (vars[(var->value).v] != NULL)				&&
                    (memcmp(var, vars[(var->value).v], sizeof(ir_item_t)) != 0)     ) {
                fprintf(outFile, "IRCHECKER:      The call parameter %u of instruction %d uses the variable %llu without keeping its right information in the structure\n", count2, count, (var->value).v);
                internal_abort(method);
            }
        }
    }

    /* Free the memory              */
    freeFunction(vars);
    xanList_destroyListAndData(toDelete);

    /* Return			*/
    return;
}

static inline void irchecker_instructions (ir_method_t *method) {
    JITUINT32 	instructionsNumber;
    JITUINT32 	count;
    ir_item_t	*par1;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check the labels of the finally      *
     * blocks				*/
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst == NULL) {
            internal_abort(method);
        }
        switch (inst->type) {
            case IRBRANCHIFPCNOTINRANGE:
            case IRINITMEMORY:
            case IRMEMCPY:
            case IRSTOREREL:
            case IRVCALL:
            case IRNATIVECALL:
            case IRMEMCMP:
                if (IRMETHOD_getInstructionParametersNumber(inst) != 3) {
                    fprintf(outFile, "IRCHECKER:  Instruction %d has not 3 parameters as it should be\n", inst->ID);
                    internal_abort(method);
                }
                break;
            case IRALLOCALIGN:
            case IRLOADREL:
            case IRADD:
            case IRADDOVF:
            case IRSUB:
            case IRSUBOVF:
            case IRMUL:
            case IRMULOVF:
            case IRDIV:
            case IRREM:
            case IRDIV_NOEXCEPTION:
            case IRREM_NOEXCEPTION:
            case IRSTRINGCMP:
            case IRSTRINGCHR:
            case IRREALLOC:
            case IRCALLOC:
            case IRAND:
            case IROR:
            case IRXOR:
            case IRSHL:
            case IRSHR:
            case IRCONV:
            case IRCONVOVF:
            case IREQ:
            case IRLT:
            case IRGT:
            case IRBRANCHIF:
            case IRBRANCHIFNOT:
            case IRENDFILTER:
            case IRNEWARR:
            case IRNEWOBJ:
            case IRPOW:
                if (IRMETHOD_getInstructionParametersNumber(inst) != 2) {
                    fprintf(outFile, "IRCHECKER:  Instruction %d (%s) has not 2 parameters as it should be\n", inst->ID, IRMETHOD_getInstructionTypeName(inst->type));
                    internal_abort(method);
                }
                break;
            case IRALLOCA:
            case IRALLOC:
            case IREXP:
            case IRSTRINGLENGTH:
            case IRFREE:
            case IRBRANCH:
            case IRCALLFILTER:
            case IRCALLFINALLY:
            case IRCHECKNULL:
            case IRENDFINALLY:
            case IRARRAYLENGTH:
            case IRGETADDRESS:
            case IRISINF:
            case IRISNAN:
            case IRSIN:
            case IRCOS:
            case IRCEIL:
            case IRBITCAST:
            case IRACOS:
            case IRSQRT:
            case IRLOG10:
            case IRFLOOR:
            case IRLABEL:
            case IRNEG:
            case IRNOT:
            case IRSTARTFILTER:
            case IRSTARTFINALLY:
            case IREXIT:
            case IRTHROW:
            case IRFREEOBJ:
            case IRSIZEOF:
            case IRMOVE:
                if (IRMETHOD_getInstructionParametersNumber(inst) != 1) {
                    fprintf(outFile, "IRCHECKER:  Instruction %d (%s) has not 1 parameter as it should be\n", inst->ID, IRMETHOD_getInstructionTypeName(inst->type));
                    internal_abort(method);
                }
                break;
            case IRNOP:
            case IRSTARTCATCHER:
                if (IRMETHOD_getInstructionParametersNumber(inst) != 0) {
                    fprintf(outFile, "IRCHECKER:  Instruction %d has not 0 parameter as it should be\n", inst->ID);
                    internal_abort(method);
                }
                break;
            case IRRET:
                if (IRMETHOD_getInstructionParametersNumber(inst) > 1) {
                    fprintf(outFile, "IRCHECKER:  Instruction %d cannot have more than 1 parameter\n", inst->ID);
                    internal_abort(method);
                }
                break;
            case IRCALL:
            case IRICALL:
            case IRLIBRARYCALL:
                /* Instructions not checked.
                 */
                break;
            default:
                fprintf(outFile, "IRCHECKER:  Instruction %d has unknown type. Type %s (%u).\n", inst->ID, IRMETHOD_getInstructionTypeName(inst->type), inst->type);
                internal_abort(method);
        }
        switch (inst->type) {
            case IRBRANCH:
                par1	= IRMETHOD_getInstructionParameter1(inst);
                if (par1->type != par1->internal_type) {
                    fprintf(outFile, "IRCHECKER:  Branch instruction %d has the first parameter with type != internal_type.\n", inst->ID);
                    internal_abort(method);
                }
                if (par1->type != IRLABELITEM) {
                    fprintf(outFile, "IRCHECKER:  Branch instruction %d of %s has the first parameter that is not a label.\n", inst->ID, IRMETHOD_getSignatureInString(method));
                    internal_abort(method);
                }
        }
    }
}

static inline void irchecker_check_catcher_blocks (ir_method_t *method) {
    JITUINT32 instructionsNumber;
    JITUINT32 count;
    JITUINT32 count2;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check the labels of the finally      *
     * blocks				*/
    for (count = 0, count2 = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst == NULL) {
            internal_abort(method);
        }
        if (inst->type == IRSTARTCATCHER) {
            count2++;
        }
    }
    if (count2 > 1) {
        fprintf(outFile, "IRCHECKER:      There are more than one catcher instruction within the method %s\n", IRMETHOD_getSignatureInString(method));
        internal_abort(method);
    }

    /* Return				*/
    return;
}

static inline void irchecker_check_finally_blocks (ir_method_t * method) {
    JITUINT32 instructionsNumber;
    JITUINT32 count;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check the labels of the finally      *
     * blocks				*/
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        ir_instruction_t        *labelInst;
        ir_item_t               *label;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst == NULL) {
            abort();
        }
        if (inst->type != IRSTARTFINALLY) {
            continue;
        }
        label = IRMETHOD_getInstructionParameter1(inst);
        if (    (label->type != IRLABELITEM)            ||
                (label->internal_type != IRLABELITEM)   ) {
            abort();
        }
        labelInst = IRMETHOD_getLabelInstruction(method, (label->value).v);
        if (labelInst != NULL) {
            fprintf(outFile, "IRCHECKER:      Instruction %d defines the label %lld already defined by the final block starting from instruction %d\n", labelInst->ID, (label->value).v, inst->ID);
            internal_abort(method);
        }
    }

    /* Check that the call final block	*
     * instructions have the start final    *
     * block instruction			*/
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        ir_instruction_t        *labelInst;
        ir_item_t               *label;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst == NULL) {
            abort();
        }
        if (inst->type != IRCALLFINALLY) {
            continue;
        }
        label = IRMETHOD_getInstructionParameter1(inst);
        if (    (label->type != IRLABELITEM)            ||
                (label->internal_type != IRLABELITEM)   ) {
            abort();
        }
        labelInst = IRMETHOD_getFinallyInstruction(method, (label->value).v);
        if (labelInst == NULL) {
            fprintf(outFile, "IRCHECKER:      Instruction %d call the final block with label %lld, which it does not exist\n", inst->ID, (label->value).v);
            internal_abort(method);
        }
    }

    /* Check that the end finally           *
     * instruction jumps to a block		*
     * outside its finally block		*/
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        ir_instruction_t        *succ;
        IRBasicBlock           	*bb;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst == NULL) {
            abort();
        }
        if (inst->type != IRENDFINALLY) {
            continue;
        }
        bb = IRMETHOD_getBasicBlockContainingInstruction(method, inst);
        assert(bb != NULL);
        succ = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
        while (succ != NULL) {
            if (IRMETHOD_getBasicBlockContainingInstruction(method, succ) == bb) {
                fprintf(outFile, "IRCHECKER:      Instruction %d cannot have a successor (inst %d) within the same basic block (%d <-> %d) because is a end-finally label\n", inst->ID, succ->ID, bb->startInst, bb->endInst);
                internal_abort(method);
            }
            succ = IRMETHOD_getSuccessorInstruction(method, inst, succ);
        }
    }

    /* Return				*/
    return;
}

static inline void irchecker_successors_predecessors (ir_method_t * method) {
    JITUINT32 count;
    XanList         **tableS;
    XanList         **tableS_start;
    XanList         **tableP;
    JITUINT32 instructionsNumber;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method) + 1;

    /* Allocate the necessary memories	*/
    tableS = allocFunction(sizeof(XanList *) * instructionsNumber);
    tableS_start = allocFunction(sizeof(XanList *) * instructionsNumber);
    tableP = allocFunction(sizeof(XanList *) * instructionsNumber);

    /* Check the successors/predecessors	*/
    for (count = 0; count < instructionsNumber; count++) {
        XanList                 *l;
        ir_instruction_t        *inst;
        ir_instruction_t        *prev;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (inst == NULL) {
            abort();
        }
        l = xanList_new(allocFunction, freeFunction, NULL);
        tableS[count] = l;
        prev = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
        while (prev != NULL) {
            if (xanList_find(l, prev) != NULL) {
                abort();
            }
            xanList_append(l, prev);
            prev = IRMETHOD_getSuccessorInstruction(method, inst, prev);
        }
        tableS_start[count] = xanList_cloneList(l);
        l = xanList_new(allocFunction, freeFunction, NULL);
        tableP[count] = l;
        prev = IRMETHOD_getPredecessorInstruction(method, inst, NULL);
        while (prev != NULL) {
            if (xanList_find(l, prev) != NULL) {
                fprintf(outFile, "IRCHECKER:      Predecessor %d of the instruction %d is considered two times\n", prev->ID, inst->ID);
                internal_abort(method);
            }
            xanList_append(l, prev);
            prev = IRMETHOD_getPredecessorInstruction(method, inst, prev);
        }
    }
    for (count = 0; count < instructionsNumber; count++) {
        XanListItem             *item;
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        item = xanList_first(tableP[count]);
        while (item != NULL) {
            ir_instruction_t        *prev;
            prev = item->data;
            if (xanList_find(tableS[prev->ID], inst) == NULL) {
                internal_error_successors_predecessors(method, prev, tableS_start, tableP);
                fprintf(outFile, "IRCHECKER:      Instruction %d should be a successor of %d\n", inst->ID, prev->ID);
                fprintf(outFile, "IRCHECKER:      Predecessors of %d: ", count);
                internal_print_list(tableP[count]);
                fprintf(outFile, "\n");
                internal_abort(method);
            }
            xanList_removeElement(tableS[prev->ID], inst, JITTRUE);
            if (xanList_find(tableS[prev->ID], inst) != NULL) {
                internal_error_successors_predecessors(method, inst, tableS_start, tableP);
                internal_abort(method);
            }
            item = item->next;
        }
    }
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        if (xanList_length(tableS[count]) > 0) {
            internal_error_successors_predecessors(method, inst, tableS_start, tableP);
            internal_abort(method);
        }
    }

    /* Check the function                           *
     * IRMETHOD_canTheNextInstructionBeASuccessor	*/
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        ir_instruction_t        *instNext;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        instNext = IRMETHOD_getNextInstruction(method, inst);
        if (IRMETHOD_canTheNextInstructionBeASuccessor(method, inst)) {
            ir_instruction_t        *succ;
            JITBOOLEAN found;
            if (instNext == NULL) {
                fprintf(outFile, "IRCHECKER:      Error on the IRMETHOD_canTheNextInstructionBeASuccessor function\n");
                internal_abort(method);
            }
            found = JITFALSE;
            succ = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            while (succ != NULL) {
                if (succ == instNext) {
                    found = JITTRUE;
                    break;
                }
                succ = IRMETHOD_getSuccessorInstruction(method, inst, succ);
            }
            if (!found) {
                fprintf(outFile, "IRCHECKER:      Error on the IRMETHOD_canTheNextInstructionBeASuccessor function\n");
                fprintf(outFile, "IRCHECKER:      Instruction %d should have the instruction %d as successor.\n", inst->ID, instNext->ID);
                internal_abort(method);
            }
        } else {
            ir_instruction_t        *succ;
            JITBOOLEAN found;
            found = JITFALSE;
            if (    (instNext != NULL)              &&
                    (instNext != method->exitNode)  ) {
                succ = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
                while (succ != NULL) {
                    if (succ == instNext) {
                        found = JITTRUE;
                        break;
                    }
                    succ = IRMETHOD_getSuccessorInstruction(method, inst, succ);
                }
            }
            if (found) {
                fprintf(outFile, "IRCHECKER:      Error on the IRMETHOD_canTheNextInstructionBeASuccessor function\n");
                fprintf(outFile, "IRCHECKER:      Instruction %d should not have the instruction %d as successor.\n", inst->ID, instNext->ID);
                internal_abort(method);
            }
        }
    }

    /* Free the memory			*/
    for (count = 0; count < instructionsNumber; count++) {
        xanList_destroyList(tableS[count]);
        xanList_destroyList(tableS_start[count]);
        xanList_destroyList(tableP[count]);
    }
    freeFunction(tableS);
    freeFunction(tableS_start);
    freeFunction(tableP);

    /* Return				*/
    return;
}

static inline char * irchecker_get_version (void) {
    return VERSION;
}

static inline char * irchecker_get_informations (void) {
    return INFORMATIONS;
}

static inline char * irchecker_get_author (void) {
    return AUTHOR;
}

static inline void internal_error_successors_predecessors (ir_method_t *method, ir_instruction_t *inst, XanList **tableS, XanList **tableP) {
    XanListItem     *item;

    /* Assertions			*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(tableS != NULL);
    assert(tableP != NULL);

    IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
    fprintf(outFile, "IRCHECKER: Error on successors/predecessors of the instruction %d of the method %s\n", inst->ID, IRMETHOD_getSignatureInString(method));
    fprintf(outFile, "IRCHECKER:      Successors of instruction %d: ", inst->ID);
    item = xanList_first(tableS[inst->ID]);
    while (item != NULL) {
        ir_instruction_t        *succ;
        succ = item->data;
        fprintf(outFile, "%d ", succ->ID);
        item = item->next;
    }
    fprintf(outFile, "\n");
    item = xanList_first(tableS[inst->ID]);
    while (item != NULL) {
        XanListItem     *item2;
        inst = item->data;
        fprintf(outFile, "IRCHECKER:      Predecessors of instruction %d: ", inst->ID);
        item2 = xanList_first(tableP[inst->ID]);
        while (item2 != NULL) {
            ir_instruction_t        *prev;
            prev = item2->data;
            fprintf(outFile, "%d ", prev->ID);
            item2 = item2->next;
        }
        fprintf(outFile, "\n");
        item = item->next;
    }
    fprintf(outFile, "\n");

    /* Return			*/
    return;
}

static inline void internal_print_list (XanList *l) {
    XanListItem     *item;

    item = xanList_first(l);
    while (item != NULL) {
        ir_instruction_t        *succ;
        succ = item->data;
        fprintf(outFile, "%d ", succ->ID);
        item = item->next;
    }
    return;
}

static inline void internal_abort (ir_method_t *method) {
    if (method != NULL) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, METHOD_PRINTER);
    }
    abort();

    return ;
}

static inline void irchecker_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void irchecker_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}

static inline void internal_openFile (void) {

    /* Open the file.
     */
    if (outFile == NULL) {
        SNPRINTF(nameBuffer, DIM_BUF, "%s_irchecker", IRPROGRAM_getProgramName());
        outFile	= fopen((char *)nameBuffer, "w");
    }
    assert(outFile != NULL);

    return ;
}

static inline void internal_compute_gen_kill (ir_method_t *method, data_flow_t *data) {
    JITUINT32 i;

    /* Assertions.
     */
    assert(method != NULL);
    assert(data != NULL);

    /* Compute the gen sets.
     * Set the parameters to be defined by first instructions, which are nops.
     */
    for (i=0; i < IRMETHOD_getMethodParametersNumber(method); i++) {
        DATAFLOW_addElementToGENSet(data, i, i);
    }

    /* Each iteration of the loop sets the gen set for the i-th instruction.
     */
    for (; i < (data->num); i++) {
        IR_ITEM_VALUE		varID;
        ir_instruction_t	*inst;

        /* Fetch the instruction.
         */
        inst	= IRMETHOD_getInstructionAtPosition(method, i);
        assert(inst != NULL);

        /* Check whether the instruction defines a variable.
         */
        if (!IRMETHOD_doesInstructionDefineAVariable(method, inst)) {
            continue ;
        }

        /* The instruction defines a variable.
         * Fetch this variable.
         */
        varID	= IRMETHOD_getVariableDefinedByInstruction(method, inst);
        assert(varID < IRMETHOD_getNumberOfVariablesNeededByMethod(method));

        /* Add the variable to the gen set of the instruction.
         */
        DATAFLOW_addElementToGENSet(data, i, varID);
    }

    return;
}

static inline JITBOOLEAN internal_data_flow_step (data_flow_t *data, XanList **predecessors) {
    JITBOOLEAN modified;
    JITINT32 i;

    /* Assertions.
     */
    assert(data != NULL);
    assert(predecessors != NULL);

    /* Initialize the variables.
     */
    modified = JITFALSE;

    /* Compute the in and out sets.
     */
    for (i = 0; i < (data->num); i++) {
        JITUINT32 j;
        JITNUINT old;
        JITNUINT new;

        /* Compute the in set.
         */
        for (j = 0; j < (data->elements); j++) {
            XanListItem     *item;
            old	= (data->data[i]).in[j];
            item 	= xanList_first(predecessors[i]);
            while (item != NULL) {
                ir_instruction_t        *pred;

                /* Fetch the predecessor.
                 */
                pred = item->data;
                assert(pred != NULL);
                assert(pred->ID < data->num);

                /* Compute the intersection of predecessor sets.
                 */
                ((data->data[i]).in[j])	&= ((data->data[pred->ID]).out[j]);

                /* Fetch the next element.
                 */
                item = item->next;
            }
            new = (data->data[i]).in[j];
            if (new != old) {
                modified = JITTRUE;
            }
        }

        /* Compute the out set.
         */
        for (j = 0; j < (data->elements); j++) {
            old = (data->data[i]).out[j];
            (data->data[i]).out[j] 	= (data->data[i]).in[j] | (data->data[i]).gen[j];
            new = (data->data[i]).out[j];
            if (new != old) {
                modified = JITTRUE;
            }
        }
    }

    return modified;
}

static inline void internal_print_dataflow (data_flow_t *df) {
    JITUINT32	i;
    JITUINT32	j;

    fprintf(outFile, "Data flow\n");
    for (i=0; i < df->num; i++) {
        for (j = 0; j < (df->elements * sizeof(JITNUINT) * 8); j++) {
            if (DATAFLOW_doesElementExistInINSet(df, i, j)) {
                fprintf(outFile, "	Instruction %u: var %u\n", i, j);
            }
        }
    }

    return ;
}
