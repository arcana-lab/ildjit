/*
 * Copyright (C) 2006 - 2014  Campanoni Simone
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
#include <errno.h>
#include <jitsystem.h>
#include <iljit-utils.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <reaching_definitions.h>
#include <config.h>
// End

#define INFORMATIONS    "This step compute the reaching definitions of each temporary to each statement"
#define AUTHOR          "Simone Campanoni and Ezequiel Lara Gómez"

static inline JITUINT64 rd_get_ID_job (void);
static inline char * rd_get_version (void);
static inline char * rd_get_information (void);
static inline char * rd_get_author (void);
static inline JITUINT64 get_dependences (void);
static inline JITUINT64 get_invalidations (void);
static inline void reaching_definitions_do_job (ir_method_t *method);
static inline void init_gen_kill (ir_method_t *method, ir_instruction_t **insns);
static inline void print_sets (ir_method_t *method);
static inline void compute_reaching_definitions (ir_method_t *method, ir_instruction_t **insns);
static inline JITINT32 compute_reaching_definitions_step (ir_method_t *method, XanList **predecessors, JITUINT32 instructionsNumber, JITUINT32 reachingDefinitionsBlockNumbers, ir_instruction_t **insns);
static inline void rd_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void rd_shutdown (JITFLOAT32 totalTime);
static inline void rd_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void rd_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_lib_t        *irLib;

ir_optimization_interface_t plugin_interface = {
    rd_get_ID_job,
    get_dependences,
    rd_init,
    rd_shutdown,
    reaching_definitions_do_job,
    rd_get_version,
    rd_get_information,
    rd_get_author,
    get_invalidations,
    NULL,
    rd_getCompilationFlags,
    rd_getCompilationTime
};

static inline void rd_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
    irLib = lib;
}

static inline void rd_shutdown (JITFLOAT32 totalTime) {
    irLib = NULL;
}

static inline JITUINT64 rd_get_ID_job (void) {
    return REACHING_DEFINITIONS_ANALYZER;
}

static inline JITUINT64 get_dependences (void) {
    return 0;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void reaching_definitions_do_job (ir_method_t *method) {
    ir_instruction_t    **insns;

    /* Assertions.
     */
    assert(method != NULL);
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Start with the method %s\n", method->getSignatureInString(method, irLib->typeChecker));

    /* Fetch the instructions.
     */
    insns   = IRMETHOD_getInstructionsWithPositions(method);

    /* Init the GEN/KILL sets.
     */
    init_gen_kill(method, insns);

    /* Print the sets.
     */
#ifdef PRINTDEBUG
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Sets after the definition for the GEN, KILL\n");
    print_sets(method);
#endif

    /* Compute the reaching definitions.
     */
    compute_reaching_definitions(method, insns);

    /* Free the memory.
     */
    freeFunction(insns);

    return;
}

static inline void initDefs (definitions *defs, JITUINT32 numberOfVariables) {

    /* Assertions			*/
    assert(defs != NULL);

    defs->numberOfVariables = numberOfVariables;
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: defs->numberOfVariables %d\n", defs->numberOfVariables);

    //is the conversion between JIT types and the native of size_t safe?
    defs->variables = NULL;
    if (numberOfVariables > 0) {
        defs->variables = (instructionList**) allocFunction(numberOfVariables * sizeof(instructionList *));
        memset(defs->variables, 0, numberOfVariables * sizeof(instructionList *));
    }
}

static inline void destroyDefs (definitions *defs) {
    instructionList         *pointer;
    instructionList         *old;
    JITNUINT i;

    /* Assertions			*/
    assert(defs != NULL);

    /* Check if there is something	*
     * to free			*/
    if (defs->variables == NULL) {
        return;
    }

    /* Free the memory              */
    for (i = 0; i < defs->numberOfVariables; i++) {
        pointer = (instructionList *) defs->variables[i];
        while (pointer != NULL) {
            old = pointer->next;
            freeFunction(pointer);
            pointer = old;
        }

    }
    freeFunction(defs->variables);

    /* Return                       */
    return;
}

static inline void addToDefs (definitions* defs, JITUINT64 variableSetted, JITUINT32 instructionID) {
    instructionList* instruction;

    //add to list the instructionID given as defining the variable variableSetted
    instruction = (instructionList*) allocFunction(sizeof(instructionList));
    instruction->next = NULL;
    instruction->instId = instructionID;

    instructionList* pointer = defs->variables[variableSetted];
    if (pointer == NULL) {

        //uninitialized variable
        defs->variables[variableSetted] = instruction;
    } else {

        //find where to insert: at the end, old points to the last instruction filled
        assert(pointer != NULL);
        while (pointer->next != NULL) {
            //instruction cannot be inserted twice if instructionID is unique, as we travel them sequentially
            pointer = pointer->next;
            assert(pointer != NULL);
        }
        assert(pointer != NULL);
        pointer->next = instruction;
    }
}

void printDefs (definitions* defs) {
    JITNINT i = 0;

    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Defs set, ordered by variable, contains: \n");
    for (; i < defs->numberOfVariables; i++) {
        if (defs->variables[i] != NULL) {
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:         variable %d\n", i);
            instructionList* list = defs->variables[i];

            while (list != NULL) {
                PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 instruction %d\n", list->instId);
                list = list->next;
            }
        }
    }
}

inline instructionList* getDefs (definitions* defs, JITUINT64 variableGetted) {
    //TODO checks for validity of variableGetted...

    return defs->variables[variableGetted];
}

static inline void init_gen_kill (ir_method_t *method, ir_instruction_t **insns) {
    ir_instruction_t        *opt_inst;
    JITUINT32 genkillset_size;
    JITINT32 count;
    JITUINT32 instID;
    JITUINT32 instructionsNumber;
    JITUINT32 blocksNumber;
    JITNUINT temp;
    JITNUINT trigger;
    definitions defs;
    JITUINT32 variablesNumber;

    /* Assertions			*/
    assert(method != NULL);

    /* Init the variables		*/
    opt_inst = NULL;
    genkillset_size = 0;
    count = 0;
    instID = 0;
    blocksNumber = 0;

    //init the defs set (conceptually, should be in optimizer and belonging to each instruction, shouldn't it?).
    //it stores one empty pointer for each VARIABLE, which in turn will store a list of intructions id's where it gets defined
    variablesNumber = IRMETHOD_getNumberOfVariablesNeededByMethod(method);
    initDefs(&defs, variablesNumber);

    // In theory, a maximum of one variable can be defined per instruction, and defs(t) contain definitions for each variable
    // so the getMaxVariables number shouldn't be greater than the number of instructions plus the number of input parameters.
    // Other possible way could be to create a hashtable (search.h, hsearch) of getMaxInstructions number, indexed by
    // variable, which because of the former should be safe.
    // Nevertheless getMaxVAriables should* be smaller than that, so a simple array suffices.

    /* Fetch the instructions number			*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Fetch the GEN/KILL size, as both have the same size	*/
    genkillset_size = instructionsNumber + IRMETHOD_getMethodParametersNumber(method);
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: genkillset_size (instructionsnumber): %d\n", genkillset_size);
    if (genkillset_size == 0) {

        /* Set to zero every reaching definitions	*/
        IRMETHOD_allocMemoryForReachingDefinitionsAnalysis(method, 0);

        /* Free the memory                              */
        destroyDefs(&defs);

        /* Return					*/
        return;
    }

    /* Compute how many block we	*
     * have to allocate for the	*
     * GEN set			*/

    //number of blocks is size (maximum size of the sets, as both are equal) / sizeof native int in bits +1 (count from 0)
    // => 1 bit per instruction.
    blocksNumber = (genkillset_size / (sizeof(JITNINT) * 8)) + 1;

    IRMETHOD_allocMemoryForReachingDefinitionsAnalysis(method, blocksNumber);
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: USE block number = %d\n", method->reachingDefinitionsBlockNumbers);
    assert(method->reachingDefinitionsBlockNumbers > 0);

    /* fill the DEFS(t) set
     * for the kill set we'll need defs(t), that is, the set of statements where variable t gets defined.
     * Therefore it is useful to have it local to this reaching definitions method as a list of instruction IDs,
     * indexed by variable. For each instruction that returns some variable, we will access the array indexed by that variable,
     * and add the instruction id's found.
     * The first instructions (IRNOP) are used as dummy nodes for the definitions of the input parameters of the method.
     * e.g. inst 0 = definition of the parameter 0 ecc...
     */
    for (count = 0; count < IRMETHOD_getMethodParametersNumber(method); count++) {
#ifdef DEBUG
        opt_inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(opt_inst != NULL);
        assert(IRMETHOD_getInstructionType(opt_inst) == IRNOP);
        assert(count < variablesNumber);
#endif
        addToDefs(&defs, count, count);
    }
    for (instID = count; instID < instructionsNumber; instID++) {
        IR_ITEM_VALUE	varDefined;

        /* Fetch the instruction.
         */
        opt_inst 	= insns[instID];
        assert(opt_inst != NULL);
        assert((opt_inst->metadata->reaching_definitions).kill != NULL);
        assert((opt_inst->metadata->reaching_definitions).gen != NULL);
        assert((opt_inst->metadata->reaching_definitions).in != NULL);
        assert((opt_inst->metadata->reaching_definitions).out != NULL);

        /* Fetch the variable defined by the instruction.
         */
        varDefined	= IRMETHOD_getVariableDefinedByInstruction(method, opt_inst);

        /* Add the definition of this instruction to the defs sets.
         */
        if (varDefined != NOPARAM) {
            addToDefs(&defs, varDefined, instID);
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 Instruction %d defines the variable %lld\n", method->getInstructionID(method, opt_inst), IRMETHOD_getInstructionResult(opt_inst)->value.v);
        }
    }

#ifdef DEBUG
    printDefs(&defs);
#endif

    /* Create the GEN, KILL set for	*
     * each definition == statement == instruction	*/

    /* each statement has two sets, and each set has as many positions as instructions */
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Init the GEN KILL sets\n");

    /* Set the GEN sets for the first dummy instruction	*/
    for (count = 0; count < IRMETHOD_getMethodParametersNumber(method); count++) {

        /* Fetch the instruction		*/
        opt_inst = insns[count];
        assert(opt_inst != NULL);
        assert(IRMETHOD_getInstructionType(opt_inst) == IRNOP);

        /* Set the GEN set			*/
        trigger = count / (sizeof(JITNINT) * 8);
        temp = 0x1;
        temp = temp << (count % (sizeof(JITNINT) * 8));
        reachingDefinitionsSetGen(method, opt_inst, trigger, reachingDefinitionsGetGen(method, opt_inst, trigger) | temp);
    }

    //for each instruction, sequentially...
    for (instID = count; instID < instructionsNumber; instID++) {
        IR_ITEM_VALUE	varDefined;
        PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:         Inst %d\n", instID);

        /* Fetch the instruction.
         */
        opt_inst = insns[instID];
        assert(opt_inst != NULL);

        /* Fetch the variable defined by the instruction.
         */
        varDefined	= IRMETHOD_getVariableDefinedByInstruction(method, opt_inst);

        // Init the sets

        //the Kill set will be initialized to 0 instead of doing it the way liveness does it.
        //This is because a single bit will be set per instruction, instead of storing the instruction number as with the Def set.
        //that forces some changes in the compute_step method.

        //idea is "inside of the instruction, for each position in the gen set, which is as large as rdBlockNumbers
        // header is void (*setGen or *setKill) (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);
        // also void (*setIn or *setOut) (void *method, struct ir_instruction_t *inst, JITUINT32 blockNumber, JITNUINT value);
        //we used to do a setdef "all-in-one", now as setKill has one more parameter (blockNumber) we do both at the same time.
        for (count = 0; count < method->reachingDefinitionsBlockNumbers; count++) {
            reachingDefinitionsSetGen(method, opt_inst, count, 0);
            reachingDefinitionsSetKill(method, opt_inst, count, 0);
            reachingDefinitionsSetIn(method, opt_inst, count, 0);
            reachingDefinitionsSetOut(method, opt_inst, count, 0);
        }


        /* Init the GEN set, procedurally equal to the DEF set of liveness
         * conceptually, the GEN set can only contain its own number, or not, as an instruction only can add itself to the GEN set.
         * In order to mantain compatibility with the headers given and with the book example, page 356, it will store its
         * instruction number, or instructionID, although it is VERY inefficient (it could store 3 words per instruction instead of
         * a single boolean!!! or, more efficient still, the GEN set could be stored in the method instead of the instruction as an array...
         * Nevertheless the given code is respected, just in case.
         *
         * Therefore, we do as in the liveness code (though that code deals with VARIABLES for each statement, and not statements):
         * we find the bit position of the instruction with /, and set to 1 its bit with %.

         */

        //For the instruction being analized...
        trigger = instID / (sizeof(JITNINT) * 8);
        temp = 0x1;
        temp = temp << (instID % (sizeof(JITNINT) * 8));

        //if instruction returns a variable (as in the liveness code, expressed as IROFFSET)
        //set to 1 the bit corresponding to the instruction ID as done in liveness analysis.
        if (varDefined != NOPARAM) {
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 variable being written is %d\n", varDefined);

            /* init the GEN sets */
            assert( IRMETHOD_getInstructionResult(opt_inst)->type == IROFFSET || IRMETHOD_getInstructionParameter1Type(opt_inst) == IROFFSET);
            reachingDefinitionsSetGen(method, opt_inst, trigger, reachingDefinitionsGetGen(method, opt_inst, trigger) | temp);
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 Add to GEN the instruction %d\n", method->getInstructionID(method, opt_inst));

            /* now init the KILL set */

            //for it, add the defs(t) - {d}: for each element in defs(t), add it to kills, except if it is the current instruction
            instructionList* list = defs.variables[varDefined];
            while (list != NULL) {
                if (list->instId != (instID)) { //remove from defs(t) the current instruction
                    trigger = list->instId / (sizeof(JITNINT) * 8);
                    temp = 0x1;
                    temp = temp << (list->instId % (sizeof(JITNINT) * 8));
                    reachingDefinitionsSetKill(method, opt_inst, trigger, reachingDefinitionsGetKill(method, opt_inst, trigger) | temp);
                    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                         Add to KILL the instruction %d\n", list->instId);
                }
                list = list->next;
            }
        }
    }

    /* Free the memory.
     */
    destroyDefs(&defs);

    return;
}

static inline void print_sets (ir_method_t *method) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);

    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        ir_instruction_t        *opt_inst;
        JITINT32 		count2;

        /* Fetch the instruction	*/
        opt_inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(opt_inst != NULL);

        fprintf(stderr, "OPTIMIZER_REACHING_DEFINITIONS: <Instruction %d>        Type    = %d\n", count, IRMETHOD_getInstructionType(opt_inst));
        fprintf(stderr, "OPTIMIZER_REACHING_DEFINITIONS:                         KILL	= 0x");
        for (count2 = 0; count2 < (method->reachingDefinitionsBlockNumbers); count2++) {
            fprintf(stderr, "%lX", reachingDefinitionsGetKill(method, opt_inst, count2));
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "OPTIMIZER_REACHING_DEFINITIONS:                         GEN	= 0x");
        for (count2 = 0; count2 < (method->reachingDefinitionsBlockNumbers); count2++) {
            fprintf(stderr, "%lX", reachingDefinitionsGetGen(method, opt_inst, count2));
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "OPTIMIZER_REACHING_DEFINITIONS:                         IN	= 0x");
        for (count2 = 0; count2 < (method->reachingDefinitionsBlockNumbers); count2++) {
            fprintf(stderr, "%lX", reachingDefinitionsGetIn(method, opt_inst, count2));
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "OPTIMIZER_REACHING_DEFINITIONS:                         OUT	= 0x");
        for (count2 = 0; count2 < (method->reachingDefinitionsBlockNumbers); count2++) {
            fprintf(stderr, "%lX", reachingDefinitionsGetOut(method, opt_inst, count2));
        }
        fprintf(stderr, "\n");
    }
}

static inline void compute_reaching_definitions (ir_method_t *method, ir_instruction_t **insns) {
    JITINT32 		modified;
    XanList                 **predecessors;
    JITUINT32 		instructionsNumber;
#ifdef PRINTDEBUG
    JITUINT32 		step_number;
#endif

    /* Assertions.
     */
    assert(method != NULL);

    /* Init the variables.
     */
    modified = JITFALSE;
#ifdef PRINTDEBUG
    step_number = 0;
#endif

    /* Fetch the number of instructions.
     */
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);
    if (instructionsNumber == 0) {
        return ;
    }

    /* Compute the set of predecessors.
     */
    predecessors = IRMETHOD_getInstructionsPredecessors(method);

    /* Compute the reaching definitions.
     */
    PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Begin to compute the reaching definitions\n");
    do {
        modified = compute_reaching_definitions_step(method, predecessors, instructionsNumber, method->reachingDefinitionsBlockNumbers, insns);

#ifdef PRINTDEBUG
        PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Step number %d\n", step_number);
        step_number++;
        PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Modified: %d\n", modified);
        PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Sets\n");
        print_sets(method);
#endif
    } while (modified);

    /* Free the memory.
     */
    IRMETHOD_destroyInstructionsPredecessors(predecessors, instructionsNumber);

    return;
}

static inline JITINT32 compute_reaching_definitions_step (ir_method_t *method, XanList **predecessors, JITUINT32 instructionsNumber, JITUINT32 reachingDefinitionsBlockNumbers, ir_instruction_t **insns) {
    JITNUINT temp;
    JITUINT32 count;
    JITUINT32 instID;
    ir_instruction_t        *inst;
    ir_instruction_t        *inst2;
    JITINT32 modified;

    /* Assertions.
     */
    assert(method != NULL);
    assert(instructionsNumber > 0);

    /* Init the variables.
     */
    temp = 0;
    count = 0;
    modified = JITFALSE;
    inst = NULL;
    inst2 = NULL;

    /* Compute the reaching definitions.
     * Basically done the same way as the liveness analysis, but forward instead of backwards.
     */
    for (instID = 0; instID < instructionsNumber; instID++) {
        JITNUINT 	in_kill;
        JITNUINT 	old;
        XanListItem     *item;

        /* Fetch the instruction.
         */
        PDEBUG("OPTIMIZER_REACHING_DEFINITIONS: Reaching_definitions_step: Inst %d\n", instID);
        inst = insns[instID];
        assert(inst != NULL);

        //steps of fetching succesor and computing in[n] are now merged:

        /* Compute the in[n] (as out[n] is done in the liveness code)		*/

        //Before, in the liveness code, an instruction was guaranteed to have a maximum of two successors (then / else, for instance)
        //therefore only two sucessors were tried to be taken.

        //now, instead, we can potentially have a label to which all of the jumps in the code go, so we cannot take that assertion.
        //therefore, a do while is needed until all predecessors are cleared for the calculation.

        /* Fetch the first PREdecessor (assuming that it works...)		*/
        //note: getPred (or getSucc) should work as follows: called with method, inst, and the previous pred (succ),
        //it iterates through the (linked) list of preds (succs). Cost of iterating them each time is n^2
        //This is because of storing them as a linked list and having to go through the whole list each time...
        //perhaps for a big enough list of predecessors, or travelling through the CFG several times,
        // a hashtable should be used (which, nevertheless, is transparent to the user as it is a function of the optimizer!).
        item = xanList_first(predecessors[instID]);
        while (item != NULL) {
            inst2 = item->data;
            assert(inst2 != NULL);
            for (count = 0; count < reachingDefinitionsBlockNumbers; count++) {
                old = reachingDefinitionsGetIn(method, inst, count);
                PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:         Before In[%d][%d]                       = 0x%X\n", \ instID, count, (inst->metadata->reaching_definitions).getIn(method, inst, count));
                PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 Out[predecessor1][%d]           = 0x%X\n", \ count, (inst2->metadata->reaching_definitions).getOut(method, inst2, count));
                reachingDefinitionsSetIn(method, inst, count, reachingDefinitionsGetIn(method, inst, count) | reachingDefinitionsGetOut(method, inst2, count));
                PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:         After In[%d][%d]                                = 0x%X\n", instID, count, reachingDefinitionsGetIn(method, inst, count));
                if (old != reachingDefinitionsGetIn(method, inst, count)) {
                    modified |= 1;
                }
            }
            item = item->next;
        }

        /* Compute the out[n] first (opposite to liveness in[n])		*/
        // Beware of the allmighty 64 bit! in 64 bit archs should work, but it will be inefficient
        //(it will use 64 bit words to store 32 bits, only way for this to not fail).
        for (count = 0; count < reachingDefinitionsBlockNumbers; count++) {
            temp = JITMAXNUINT - (reachingDefinitionsGetKill(method, inst, count));   //temp = Complement to 1
            in_kill = reachingDefinitionsGetIn(method, inst, count) & temp;
            old = reachingDefinitionsGetOut(method, inst, count);
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:         Before Out[%d][%d]                      = 0x%X\n", instID, count, (inst->metadata->reaching_definitions).getOut(method, inst, count));
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 Gen[%d][%d]                     = 0x%X\n", instID, count, (inst->metadata->reaching_definitions).getGen(method, inst, count));
            reachingDefinitionsSetOut(method, inst, count, reachingDefinitionsGetGen(method, inst, count) | in_kill);
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:                 In - Kill[%d][%d]			= 0x%X\n", instID, count, in_kill);
            PDEBUG("OPTIMIZER_REACHING_DEFINITIONS:         After Out[%d][%d]                       = 0x%X\n", instID, count, (inst->metadata->reaching_definitions).getOut(method, inst, count));

            /* instead we return the number of modifications  */
            if (old != reachingDefinitionsGetOut(method, inst, count)) {
                modified    |= 1;
            }
        }
    }

    return modified;
}

static inline char * rd_get_version (void) {
    return VERSION;
}

static inline char * rd_get_information (void) {
    return INFORMATIONS;
}

static inline char * rd_get_author (void) {
    return AUTHOR;
}

static inline void rd_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void rd_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
