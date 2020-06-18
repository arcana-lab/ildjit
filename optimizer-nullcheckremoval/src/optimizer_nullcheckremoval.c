/*
 * Copyright (C) 2008 Simone Campanoni, Licata Caruso Alessandro
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
#include <iljit-utils.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_nullcheckremoval.h>
#include <config.h>
// End

/** @brief Author name. */
#define AUTHOR          "Simone Campanoni, Licata Caruso Alessandro"
/** @brief Info about the optimization step. */
#define INFORMATIONS    "This step removes useless check_null() instructions from IR method."
/** @brief Possible status for a variable checked by a check_null() instruction.
 *
 * A variable is MAY_NULL if is a NULL pointer or its value is not
 * statically computable by the local analysis performed by the plugin.
 */
#define MAY_NULL    0
/** @brief Possible status for a variable checked by a check_null() instruction.
 *
 * A variable is NOT_NULL if it is not a NULL pointer.
 */
#define NOT_NULL    1

/**
   *\mainpage Null-Check Removal Optimization Plugin
 *
 * \author Licata Caruso Alessandro
 * \version 0.0.2.1
 *
 * Project web page: \n
 *      http://ildjit.sourceforge.net
 *
 * The optimizer-nullcheckremoval plugin removes useless check_null() instructions
 * from IR methods.
 *
 */

/* Interface to optimizer module    */

static inline void nullcheckremoval_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void nullcheckremoval_getCompilationFlags (char *buffer, JITINT32 bufferLength);

/** @brief Get the ID of the kind of the work the plugin can do.  */
static inline JITUINT64 nullcheckremoval_get_ID_job (void);

/** @brief Get the ID of each plugin to call before the current.  */
static inline JITUINT64 nullcheckremoval_get_dependences (void);

static inline JITUINT64 nullcheckremoval_get_invalidations (void);


/** @brief This function is called before optimizing a new method, for each new method.
 *
 * Initializes the memory manager of the plugin. See #mem_manager for further info.
 * @param irLib Internal library for memory management. See \c $ILDJIT_HOME/include/iljit/ir_method.h for further info.
 */
void nullcheckremoval_init (ir_lib_t *irLib, ir_optimizer_t *optimizer, char *outputPrefix);

/** @brief This function is called before optimize a new method, for each new method.
 *
 * At the moment, it does nothing.
 */
void nullcheckremoval_shutdown (JITFLOAT32 totalTime);

/** @brief Calls the plugin to perform its optimization step.
 *
 *  The optimization consists of following steps:
 *
 *  1 - find all potentially useless check_null()
 *      instructions in the IR method;
 *
 *  2 - perform a local analysis on all found
 *      checks, in order to determine if a check
 *      is redundant or not;
 *
 *  3 - delete all redundant null-checks;
 *
 * See init_job() and perform_analysis() for further info.
 * @param method IR method to be optimized.
 */
void nullcheckremoval_do_job (ir_method_t *method);


/** @brief Returns the version of the plugin.
 */
char * nullcheckremoval_get_version (void);

/** @brief Returns the information about the optimization step implemented by this plugin.
 */
char * nullcheckremoval_get_informations (void);

/** @brief Returns the author name of the plugin.
 */
char * nullcheckremoval_get_author (void);

/* Internal methods */

/** @brief Internal function.
 *
 * Allocates and initializes a bit vector of b byte from heap memory,
 * in order to store up to b*8 elements.
 * If allocation fails, aborts execution
 * @param b Size, in bytes, of the vector to be allocated.
 * @return A pointer to the first byte of the vector.
 */
JITNUINT *new_bit_vector_set (JITINT32 b);

/** @brief Internal function.
 *
 * Frees heap memory occupied by vector s.
 * @param s vector to be deleted.
 */
void delete_bit_vector_set (JITNUINT *s);

/** @brief Internal function.
 *
 * Examines all the instructions of the method and computes some useful informations
 * for the following local analysis. These info are:
 *
 * 1 - all the potentially useless check-null instructions of the method;
 *
 * 2 - all escaping variables (variables which address is taken) of the method;
 * @param method IR method to be optimized.
 * @param cni_stack Stack of the check-null instructions.
 * @param ev_set Bit vector of all escaping variables of the method.
 */
void init_job (ir_method_t *method, XanStack *cni_stack, JITNUINT *ev_set);

/** @brief Internal function.
 *
 * This function is the core of the plugin. It analyzes all potentially useless checks
 * found by init_job() and marks for removal the ones that are statically useless.
 * The pseudocode of the analysis performed is the following:
 *
 * Variables:
 * \arg \c c : check_null() instruction currently analyzed by the algorithm.
 * \arg \c curr_var : variable checked by instruction <tt>c</tt>.
 * \arg \c v_status : state of \c curr_var (#MAY_NULL or #NOT_NULL).
 * \arg \c curr_node : node of the CFG examined during the analysis of <tt>curr_var</tt>'s state.
 * \arg \c p : a predecessor of \c c in the CFG.
 *
 * Data Structures:
 * \arg \c C : set of the check_null() potentially useless of the current method.
 * \arg \c Visited : set of the nodes visited by the algorithm, while computing the state of <tt>curr_var</tt>.
 * \arg \c Nodes : set of the nodes to be visited by the algorithm, while computing the state of <tt>curr_var</tt>.
 *
 * Pseudocode:
 * \code
 * //Compute C
 * 1. C = {};
 * 2. compute_C(C);
 * //Mark for removal all the check_null() in C that check a NOT_NULL variable
 * 3. FOR ALL c IN C DO
 *     //Simple case: the check_null() checks a constant k. If k != 0 then is useless, else is necessary
 *     4. IF (c == 'check_null(cnst)')
 *         5. IF (cnst != 0) THEN mark(c); ELSE CONTINUE;
 *     //The check_null() doesn't check a constant: the state of the variable checked needs to be determined
 *     6. curr_var = c.getVarID();
 *     7. v_status = NOT_NULL;
 *     8. visited = { c };
 *     9. nodes = {}
 *     10. FOR ALL p IN PREDECESSOR(c) DO
 *         11. nodes.push(p);
 *     //Go up the CFG
 *     12. REPEAT
 *         13. curr_node = nodes.pop();  //Pop a predecessor
 *         14. IF curr_node IN visited THEN CONTINUE;  //Skip an already visited node
 *         15. visited.add(curr_node);
 *         16. IF curr_var IN GEN(curr_node) THEN CONTINUE;  //Skip to another predecessor
 *         17. IF curr_node.isStartNode() THEN v_status = MAYBE_NULL; BREAK;    //curr_var is a method's formal parameter
 *         18. IF curr_var IN KILL(curr_node) THEN v_status = MAYBE_NULL; BREAK;  //curr_var is MAY_NULL, current check_null() is necessary
 *         19. FOR ALL p IN PREDECESSOR(curr_node) DO  //Keep going up the CFG
 *             20. nodes.push(p);
 *     21. UNTIL nodes.isEmpty()
 *     22. IF v_status = NOT_NULL THEN mark(c)
 * 23. DONE
 * \endcode
 *
 * Notes:
 *
 * The \c GEN set is defined as follows:
 * \arg if \c n is an instruction of type \c check_null(var) then \c var is a member of <tt>GEN(n)</tt>.
 * \arg if \c n is an instruction of type <tt>var <- c </tt> with \c c a constant different from zero, then \c var is a member of <tt>GEN(n)</tt>.
 * \arg is \c n in an instruction of type <tt>var <- f(...) </tt> with \c f(...) a function that sets the value of \c var to a value different from NULL,  then \c var is a member of <tt>GEN(n)</tt>.
 * \arg if \c var is not a member of \c KILL(n) and if instruction \c n uses \c var as a pointer to the memory, then \c var is a member of GEN(n), supposed that using a NULL pointer with \c n will cause an error in the program execution.
 *
 * The \c KILL set is defined as follows:
 * \arg if \c n is an instruction that defines the variable \c v (except the cases examined in <tt>GEN()</tt>) then \c v is a member of <tt>KILL(n)</tt>.
 * \arg if \c n is an instruction that writes in memory or calls a function, then every variable "taken" (by means of get_address()) is a member of <tt>KILL(n)</tt>. In fact the variables of IR have an address.
 * \arg if \c n is the first instruction of the method, and if previous cases don't apply (both for \c KILL and for <tt>GEN</tt>), then \c var  is a member of \c KILL(n) since \c var turn out to be a non initialized variable (or a formal parameter of the method).
 *
 */
void perform_analysis (ir_method_t *method, XanStack *check_null_inst_stack, JITNUINT *escaped_var_set, XanStack *ui_stack);

/* Debug functions */

/** @brief Internal debug function.
 *
 * If DEBUG is defined, then prints all the elements of bit vector v,
 * else does nothing.
 * @param v Vector to be printed.
 * @param s Size (in bytes) of v.
 */
void print_bit_vector (JITNUINT* v, JITUINT32 s);

/** @brief Internal debug function.
 *
 * If DEBUG is defined, then prints all the elements of stack s,
 * else does nothig.
 * @param method IR method to be optimized.
 * @param s Stack to be printed.
 */
void print_stack (ir_method_t *method, XanStack *s);

/* Global fileds */

/**
 * @brief Plugin interface.
 *
 * This structure includes each method that each plugin has to implement,
 * in order to be correctly called from the optimizer module.
 */
ir_optimization_interface_t plugin_interface = {
    nullcheckremoval_get_ID_job,
    nullcheckremoval_get_dependences,
    nullcheckremoval_init,
    nullcheckremoval_shutdown,
    nullcheckremoval_do_job,
    nullcheckremoval_get_version,
    nullcheckremoval_get_informations,
    nullcheckremoval_get_author,
    nullcheckremoval_get_invalidations,
    NULL,
    nullcheckremoval_getCompilationFlags,
    nullcheckremoval_getCompilationTime
};

/**
 * @brief Memory management structure.
 *
 * Provides to the plugin a centralized access to memory
 * management functions. See \c $ILDJIT_HOME/include/iljit/ir_method.h for further info.
 */
ir_lib_t *mem_manager = NULL;

/* Methods implementation */

static inline JITUINT64 nullcheckremoval_get_ID_job (void) {
    return NULL_CHECK_REMOVER;
}

static inline JITUINT64 nullcheckremoval_get_dependences (void) {
    return 0;
}


static inline JITUINT64 nullcheckremoval_get_invalidations (void) {
    return INVALIDATE_ALL & ~(ESCAPES_ANALYZER);
}

void nullcheckremoval_init (ir_lib_t *irLib, ir_optimizer_t *optimizer, char *outputPrefix) {
    PDEBUG("NULLCHECKREMOVAL: init\n");
    assert(irLib != NULL);
    assert(optimizer != NULL);
    mem_manager = irLib;
    return;
}

void nullcheckremoval_shutdown (JITFLOAT32 totalTime) {

}

char * nullcheckremoval_get_version (void) {
    return VERSION;
}

char * nullcheckremoval_get_informations (void) {
    return INFORMATIONS;
}

char * nullcheckremoval_get_author (void) {
    return AUTHOR;
}

void nullcheckremoval_do_job (ir_method_t *method) {
    /* Assertions   */
    assert(method != NULL);

    PDEBUG("NULLCHECKREMOVAL: do_job in method %s\n", method->getName(method));

    /* Potentially useless IRCHECKNULL
     * instructions of the method   */
    XanStack *check_null_inst_stack;
    check_null_inst_stack = xanStack_new(allocFunction, freeFunction, NULL);
    assert(check_null_inst_stack != NULL);

    /* Set (bit vector) of variables which
     * address is taken (escaped variables)
     */
    JITNUINT *escaped_var_set;
    JITINT32 blocks = IRMETHOD_getNumberOfVariablesNeededByMethod(method) / (sizeof(JITNUINT) * 8) + 1;
    escaped_var_set = new_bit_vector_set(blocks * sizeof(JITNUINT));

    /* Search for potentially useless IRCHECKNULL
     * instructions and escaped variables
     */
    init_job(method, check_null_inst_stack, escaped_var_set);

    /* If there is no potentially useless IRCHECKNULL
     * instruction, return because no optimization is needed
     */
    if (xanStack_getSize(check_null_inst_stack) == 0) {
        PDEBUG("NULLCHECKREMOVAL: No potentially useless IRCHECKNULL instruction in method\n");
        PDEBUG("NULLCHECKREMOVAL: job done\n");

        /* Free heap memory     */
        xanStack_destroyStack(check_null_inst_stack);
        delete_bit_vector_set(escaped_var_set);
        return;
    }

    /* Useless IRCHECKNULL instructions of the
     * method
     */
    XanStack *useless_inst_stack;
    useless_inst_stack = xanStack_new(allocFunction, freeFunction, NULL);
    assert(useless_inst_stack != NULL);

    /* Apply flow analysis to all IRCHECKNULL
     * instructions in order to found the
     * useless ones
     */
    perform_analysis(method, check_null_inst_stack, escaped_var_set, useless_inst_stack);

    PDEBUG("NULLCHECKREMOVAL: Removing %d useless null checks\n", xanStack_getSize(useless_inst_stack));

    /* Delete useless checks found  */
    while (xanStack_getSize(useless_inst_stack) > 0) {
        IRMETHOD_deleteInstruction(method, xanStack_pop(useless_inst_stack));
    }

    /* Free heap memory     */
    delete_bit_vector_set(escaped_var_set);
    xanStack_destroyStack(check_null_inst_stack);
    xanStack_destroyStack(useless_inst_stack);

    PDEBUG("NULLCHECKREMOVAL: job done\n");
    return;
}

void init_job (ir_method_t *method, XanStack *cni_stack, JITNUINT *ev_set) {
    PDEBUG("NULLCHECKREMOVAL: init_job\n");

    /* First instruction ID */
    JITUINT32 first_instID = IRMETHOD_getInstructionPosition((IRMETHOD_getFirstInstruction(method)));
    JITUINT32 instID = 0;                                   /* Current instruction ID       */
    ir_instruction_t *curr_inst = NULL;                     /* Current instruction          */
    JITUINT32 varID = 0;                                    /* Current variable ID          */
    JITUINT32 block_size = sizeof(JITNUINT) * 8;            /* Block (word) size, in bit    */
    JITUINT32 instructionsNumber;

    /* Fetch the instructions number		*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Examine all method's instructions in order to
     * build cni_stack and ev_set
     */
    for (instID = 0; instID < instructionsNumber; instID++) {
        /* Fetch current instruction    */
        curr_inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(curr_inst != NULL);

        //PDEBUG("NULLCHECKREMOVAL: curr_inst type = %d\n", IRMETHOD_getInstructionType(curr_inst));

        /* If curr_inst type is IRCHECKNULL, push it into the stack.    */
        if (IRMETHOD_getInstructionType(curr_inst) == IRCHECKNULL) {
            /* If current check is the initial node,
             * then it is a necessary check because the
             * variable that it checks may be NULL
             */
            if (instID != first_instID) {
                xanStack_push(cni_stack, curr_inst);
            } else {
                PDEBUG("NULLCHECKREMOVAL: %d is a check_null() and the first instruction of the method\n", instID);
            }
        }
        /* If curr_inst type is IRGETADDRESS, add its argument to the list. */
        else if (IRMETHOD_getInstructionType(curr_inst) == IRGETADDRESS) {
            varID = IRMETHOD_getInstructionParameter1Value(curr_inst);
            ev_set[varID / block_size] = ev_set[varID / block_size] | 0x1 << varID % block_size;
        }
    }

    PDEBUG("NULLCHECKREMOVAL: check_null() found = { ");
#ifdef PRINTDEBUG
    print_stack(method, cni_stack);
#endif
    PDEBUG("}\n");

    PDEBUG("NULLCHECKREMOVAL: escaping variables = { ");
#ifdef DEBUG
    print_bit_vector(ev_set, (IRMETHOD_getNumberOfVariablesNeededByMethod(method) / (sizeof(JITNUINT) * 8) + 1) * sizeof(JITNUINT));
#endif
    PDEBUG("}\n");

    return;
}

JITNUINT *new_bit_vector_set (JITINT32 b) {
    assert(b > 0);
    void *ptr = allocFunction(b);
    assert(ptr != NULL);
    return (JITNUINT *) memset(ptr, 0, (size_t) (b));
}

void delete_bit_vector_set (JITNUINT *s) {
    assert(s != NULL);
    freeFunction(s);
    return;
}

void perform_analysis (ir_method_t *method, XanStack *cni_stack, JITNUINT *ev_set, XanStack *ui_stack) {
    PDEBUG("NULLCHECKREMOVAL: perform_analysis\n");

    ir_instruction_t *curr_check = NULL;            /* Currently examined check_null() instruction                  */
    ir_instruction_t *visiting_node = NULL;         /* Currently examined predecessor (in the CFG) of curr_check    */
    JITUINT32 visiting_nodeID = 0;                  /* ID of visiting_node                                          */
    JITUINT32 varID = 0;                            /* ID of the variable checked by check_null                     */
    JITUINT32 visiting_node_type = 0;               /* Instruction type of visiting_node                            */
    ir_item_t *visiting_node_result = NULL;         /* Result of visiting_node                                      */
    ir_item_t *visiting_node_par1 = NULL;           /* Parameter 1 of visiting_node                                 */
    ir_item_t *visiting_node_par2 = NULL;           /* Parameter 2 of visiting_node                                 */
    JITUINT32 var_status = 0;                       /* Status of varID (NOT_NULL or MAY_NULL)                       */
    JITUINT32 instID = 0;                           /* ID of curr_check                                             */
    ir_instruction_t *pred = NULL;                  /* Used to store predecessors nodes                             */
    /* ID of the first instruction of the method    */
    JITUINT32 first_instID = IRMETHOD_getInstructionPosition((IRMETHOD_getFirstInstruction(method)));

    JITUINT32 block_size = sizeof(JITNUINT) * 8;   /* Block (word) size (in bit) */
    /* Size of sets used in this procedure     */
    JITUINT32 sets_size = (IRMETHOD_getInstructionsNumber(method) / block_size + 1) * sizeof(JITNUINT);

    /* Set (bit vector) of visited nodes    */
    JITNUINT *visited_n_set = new_bit_vector_set(sets_size);

    /* Stack of nodes to be visited     */
    XanStack *nodes_stack = xanStack_new(allocFunction, freeFunction, NULL);
    assert(nodes_stack != NULL);

    /* Used to reinitialize visited_n_set and nodes_stack   */
    JITUINT32 i = 0;
    JITUINT32 bound = 0;

    PDEBUG("NULLCHECKREMOVAL: sets size = %d bit\n", sets_size * 8);

    /* External loop: iterate over all
     * IRCHECKNULL instructions of the method   */
    do {
        /* Fetch next IRCHECKNULL instruction       */
        curr_check = xanStack_pop(cni_stack);

        PDEBUG("NULLCHECKREMOVAL: fetch instruction %d\n", IRMETHOD_getInstructionPosition(curr_check));

        /* If curr_check is: 'check_null(const)'... */
        if (IRMETHOD_getInstructionParameter1Type(curr_check) != IROFFSET) {
            PDEBUG("NULLCHECKREMOVAL: found a check_null(const)\n");
            /* ...and if const != 0, mark this check for removal  */
            if (IRMETHOD_getInstructionParameter1Value(curr_check) != 0) {
                xanStack_push(ui_stack, curr_check);
                PDEBUG("NULLCHECKREMOVAL: const != 0, check marked for removal\n");
                continue;
            }
            /* ...and if const == 0, cannot remove this check    */
            else {
                PDEBUG("NULLCHECKREMOVAL: const == 0, cannot remove it\n");
                continue;
            }
        }

        var_status = NOT_NULL;

        /* Get some info from curr_check  */
        varID = IRMETHOD_getInstructionParameter1Value(curr_check);
        instID = IRMETHOD_getInstructionPosition(curr_check);

        /* Mark current node as visited     */
        visited_n_set[instID / block_size] = visited_n_set[instID / block_size] | 0x1 << instID % block_size;

        PDEBUG("NULLCHECKREMOVAL: visited nodes = { ");
#ifdef DEBUG
        print_bit_vector(visited_n_set, sets_size);
#endif
        PDEBUG("}\n");

        /* Add the predecessors of the current check
         * to the stack of nodes to be visited
         * (depth first exploration of the CFG)
         * NOTE: current check cannot be the first
         *       instruction of the method because
         *       this type of checks aren't added
         *       to the stack of potentially useless
         *       check_null() instructions
         */
        pred = IRMETHOD_getPredecessorInstruction(method, curr_check, NULL);
        while (pred != NULL) {
            PDEBUG("NULLCHECKREMOVAL: %d is a predecessor of %d\n", IRMETHOD_getInstructionPosition(pred), instID);
            xanStack_push(nodes_stack, pred);
            pred = IRMETHOD_getPredecessorInstruction(method, curr_check, pred);
        }

        PDEBUG("NULLCHECKREMOVAL: added predecessors of %d to nodes to be visited\n", instID);
        PDEBUG("NULLCHECKREMOVAL: nodes to be visited = { ");
#ifdef PRINTDEBUG
        print_stack(method, nodes_stack);
#endif
        PDEBUG("}\n");

        /* Inner loop: go up the CFG until when
         * varID status is determined:
         *  1) varID is NOT_NULL
         *  2) varID is MAY_NULL
         */
        while (xanStack_getSize(nodes_stack)) {
            /* Pop from the stack an ancestor (in the
             * CFG) of the current check instruction
             * and get some useful info
             */
            visiting_node = xanStack_pop(nodes_stack);
            visiting_nodeID = IRMETHOD_getInstructionPosition(visiting_node);
            visiting_node_type = IRMETHOD_getInstructionType(visiting_node);
            visiting_node_result = IRMETHOD_getInstructionResult(visiting_node);
            visiting_node_par1 = IRMETHOD_getInstructionParameter1(visiting_node);
            visiting_node_par2 = IRMETHOD_getInstructionParameter2(visiting_node);
            assert(visiting_node_result != NULL);
            assert(visiting_node_par1 != NULL);
            assert(visiting_node_par2 != NULL);

            PDEBUG("NULLCHECKREMOVAL: pop node %d from nodes to be visited\n", visiting_nodeID);

            /* If visiting_node was already visited, skip to the next */
            if (visited_n_set[visiting_nodeID / block_size] & 0x1 << visiting_nodeID % block_size) {
                PDEBUG("NULLCHECKREMOVAL: node %d already visited!\n", visiting_nodeID);
                continue;
            }

            /* Mark current node as visited */
            visited_n_set[visiting_nodeID / block_size] = visited_n_set[visiting_nodeID / block_size] | 0x1 << visiting_nodeID % block_size;

            PDEBUG("NULLCHECKREMOVAL: visited nodes = { ");
#ifdef DEBUG
            print_bit_vector(visited_n_set, sets_size);
#endif
            PDEBUG("}\n");

            /* NOTE: due to efficiency reason, the set
             *       GEN(visiting_node) is partitioned in
             *       GEN'(visiting_node) and GEN''(visiting_node).
             *       Membership of varID to GEN(visiting_node) is
             *       therefore tested earlier for GEN' and later
             *       for GEN''
             */

            /* If varID is in GEN'(visiting_node) break
             * the back research from this node and
             * continue with next node to be visited
             */

            PDEBUG("NULLCHECKREMOVAL: var%d is in GEN'(%d)? ", varID, visiting_nodeID);

            /* visiting_node: 'varID <- f(...)' with f(...)
             * a function that sets varID to a NOT_NULL value */
            if (visiting_node_result->type == IROFFSET &&
                    visiting_node_result->value.v == varID) {
                if (visiting_node_type == IRNEWOBJ ||
                        visiting_node_type == IRNEWARR ||
                        visiting_node_type == IRGETADDRESS) {
                    PDEBUG("Yes: %d: 'var%d <- not_null_returning_function(...)'\n", visiting_nodeID, varID);
                    continue;
                }
            }
            /* visiting_node: 'check_null(varID)'       */
            else if (visiting_node_type == IRCHECKNULL               &&
                     visiting_node_par1->type == IROFFSET  &&
                     visiting_node_par1->value.v == varID) {
                PDEBUG("Yes: %d: 'check_null(var%d)'\n", visiting_nodeID, varID);
                continue;
            }
            /* visiting_node :
             *      'varID <- const' with const != 0
             */
            else if (visiting_node_type == IRMOVE                          &&
                     visiting_node_result->value.v == varID   &&
                     visiting_node_par1->type != IROFFSET &&
                     visiting_node_par1->value.v != 0) {
                PDEBUG("Yes: %d: 'var%d <- %llu'\n", visiting_nodeID, varID, visiting_node_par1->value);
                continue;
            }

            PDEBUG("No\n");

            /* If varID is in KILL(visiting_node) then
             * current check instruction is necessary
             * so, exit from current analysis
             */

            PDEBUG("NULLCHECKREMOVAL: var%d is in KILL(%d)? ", varID, visiting_nodeID);

            /* visiting node is a 'varID <- v' store
             * instruction, with:
             *      'v' variable
             *      'v' constant == 0
             * (the last condition doesn't require
             *  an explicit check)
             */
            if (visiting_node_type == IRMOVE       &&
                    visiting_node_result->value.v == varID) {
                PDEBUG("Yes: %d: 'var%d <- [var]%llu'\n", visiting_nodeID, varID, visiting_node_par1->value);
                var_status = MAY_NULL;
                break;
            }
            /* visiting_node :
             *      'varID <- f(...)'   or
             *      'varID <- M[a]'     or
             *      'varID <- a (op) b'
             */
            else if (visiting_node_result->type == IROFFSET &&
                     visiting_node_result->value.v == varID) {
                PDEBUG("Yes: %d is of type %d and defines var%d\n", visiting_nodeID, visiting_node_type, varID);
                var_status = MAY_NULL;
                break;
            }
            /* visiting_node is the start node          */
            else if (visiting_nodeID == first_instID) {
                PDEBUG("Yes: %d is the start node\n", visiting_nodeID);
                var_status = MAY_NULL;
                break;
            }
            /* varID is an escaped variable...          */
            else if (ev_set[varID / block_size] & 0x1 << varID % block_size) {
                /* and visiting_node :
                 *      'M[a] <- b'      or
                 *      'M[a] <- M[b]'   or
                 *      'a <- f(...)'
                 */
                if (visiting_node_type == IRCALL                ||
                        visiting_node_type == IRICALL               ||
                        visiting_node_type == IRNATIVECALL               ||
                        visiting_node_type == IRLIBRARYCALL               ||
                        visiting_node_type == IRSTOREREL            ||
                        visiting_node_type == IRVCALL               ||
                        visiting_node_type == IRINITMEMORY          ||
                        visiting_node_type == IRMEMCPY) {
                    PDEBUG("Yes: %d is of type %d and var%d escapes\n", visiting_nodeID, visiting_node_type, varID);
                    var_status = MAY_NULL;
                    break;
                }
            }

            PDEBUG("No\n");

            /* If varID is in GEN''(visiting_node) break
             * the back research from this node and
             * continue with next node to be visited
             */

            PDEBUG("NULLCHECKREMOVAL: var%d is in GEN''(%d)? ", varID, visiting_nodeID);

            /* visiting_node :
             *      'a <- M[p]'     or
             *      'M[p] <- b'
             * ...
             */
            if (visiting_node_type == IRLOADREL     ||
                    visiting_node_type == IRSTOREREL    ||
                    visiting_node_type == IRINITMEMORY) {
                /* ...and p is varID */
                if (visiting_node_par1->value.v == varID) {
                    PDEBUG("Yes: %d uses var%d as a pointer\n", visiting_nodeID, varID);
                    continue;
                }
            }
            /* visiting_node :
             *      'M[varID] <- M[b]'  or
             *      'M[a] <- M[varID]'
             */
            else if (visiting_node_type == IRMEMCPY                         &&
                     (visiting_node_par1->value.v == varID ||
                      visiting_node_par2->value.v == varID)) {
                PDEBUG("Yes: %d uses var%d as a pointer\n", visiting_nodeID, varID);
                continue;
            }

            PDEBUG("No\n");

            /* Add the predecessors of the current node
             * to the stack of nodes to be visited
             * (depth first exploration of the CFG)
             */
            pred = IRMETHOD_getPredecessorInstruction(method, visiting_node, NULL);
            while (pred != NULL) {
                PDEBUG("NULLCHECKREMOVAL: %d is a predecessor of %d\n", IRMETHOD_getInstructionPosition(pred), visiting_nodeID);
                xanStack_push(nodes_stack, pred);
                pred = IRMETHOD_getPredecessorInstruction(method, visiting_node, pred);
            }

            PDEBUG("NULLCHECKREMOVAL: added predecessors of %d to nodes to be visited\n", visiting_nodeID);
            PDEBUG("NULLCHECKREMOVAL: nodes to be visited = { ");
#ifdef PRINTDEBUG
            print_stack(method, nodes_stack);
#endif
            PDEBUG("}\n");
        }

        /* Reinitialize the stack of nodes to be visited    */
        bound = xanStack_getSize(nodes_stack);
        for (i = 0; i < bound; i++) {
            xanStack_pop(nodes_stack);
        }

        /* Reinitialize the set of visited node     */
        memset(visited_n_set, 0, (size_t) (sets_size));

        /* If curr_check is redundant, mark it for removal  */
        if (var_status == NOT_NULL) {
            xanStack_push(ui_stack, curr_check);
            PDEBUG("NULLCHECKREMOVAL: %d is a useless check, marked for removal\n", IRMETHOD_getInstructionPosition(curr_check));
        }
    } while (xanStack_getSize(cni_stack) > 0);

    PDEBUG("NULLCHECKREMOVAL: analysis completed\n");

    /* Free heap memory     */
    xanStack_destroyStack(nodes_stack);
    delete_bit_vector_set(visited_n_set);

    return;
}

void print_bit_vector (JITNUINT* v, JITUINT32 s) {
    assert(v != NULL);

    JITUINT32 i = 0;
    JITUINT32 i_rel = 0;
    JITUINT32 block = 0;
    JITUINT32 mask = 0;
    JITUINT32 block_dim = sizeof(JITNUINT) * 8;

    for (i = 0; i < s * 8; i++) {
        i_rel = i % block_dim;
        block = i / block_dim;
        mask = 0x1 << i_rel;
        if (v[block] & mask) {
            PDEBUG("%d ", i);
        }
    }
}

void print_stack (ir_method_t *method, XanStack *s) {
    assert(s != NULL);
    XanList *ilist = s->internalList;
    XanListItem * item = xanList_first(ilist);
    ir_instruction_t *inst = NULL;
    int i = 0;
    int l = ilist->len;
    for (i = 0; i < l; i++) {
        inst = (ir_instruction_t*) item->data;
        assert(inst != NULL);
        fprintf(stderr, "%d ", IRMETHOD_getInstructionPosition(inst));
        item = item->next;
    }

    assert(xanStack_getSize(s) == l);

    return;
}

static inline void nullcheckremoval_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

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

static inline void nullcheckremoval_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
