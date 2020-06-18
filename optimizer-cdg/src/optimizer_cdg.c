/*
 * Copyright (C) 2006 - 2012  Campanoni Simone
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

/*! \file optimizer_cdg.c
 * \brief Control-dependence graph computer plugin
 *
 * This plugin computes the Control-dependence graph of a method and fill
 * the controlDependencies field of each instruction with the result data.
 * For each instruction the plugin generates a XanList which contains
 * the list of all the instructions from which that instruction
 * is control-dependent.
 * Some instructions may be dependent from a fiction entry point r.
 * The plugin use the method described in Appel - Modern Compiler Implementation (2nd ed.)
 * in Java at page n° 464.
 *
 * A sample code for parsing the control dependence information is given:
 *
 * \verbatim
        XanList*	temp_list=NULL;
        XanListItem* temp_el=NULL;
        ir_instruction_t *inst_temp=NULL;
        ir_instruction_t *inst_data=NULL;
        JITUINT32 instr_count=IRMETHOD_getInstructionsNumber(method);
        for(i=0; i<instr_count+2; i++){
                inst_temp = IRMETHOD_getInstructionAtPosition(_method, i);
                if(inst_temp!=NULL){
                        temp_list = inst_temp->metadata->controlDependencies;
                        if(temp_list!=NULL){
                                temp_el = temp_list->first(temp_list);
                                fprintf(stdout, "%d depends from: ", i);
                                while(temp_el!=NULL){
                                        inst_data = temp_el->data;
                                        if(inst_data!=NULL){
                                                fprintf(stdout, "%d - ", IRMETHOD_getInstructionPosition(inst_data));
                                        }
                                        temp_el = temp_list->next(temp_list, temp_el);
                                }
                                fprintf(stdout, "\n");
                                fflush(stdout);
                        }
                }
        }
        \endverbatim
 *
 */
#include <assert.h>
#include <ir_optimization_interface.h>
#include <iljit-utils.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_cdg.h>
#include <config.h>

// End

#define INFORMATIONS    "Control-Dependence Graph"
#define AUTHOR          "Simone Campanoni, Emanuele Parrinello"
#define JOB             CDG_COMPUTER

/*         Global Data			*/

/*! \def R_NODE
 * \brief r-node index
 *
 * The fakeNode is the first element after the last instruction
 */
#define R_NODE          instr_count + 1

/*! \def EXIT_NODE
 * \brief exit-node index
 *
 * The node 'exit' is placed into the array after the r-node
 */
#define EXIT_NODE       instr_count

/*! \typedef typedef struct t_node
 *  \brief Structure for graph data
 *
 * This struct holds graph data for a single node.
 * It contains both cdg data and both intermediate value like
 * post-dominator tree, immediate dominator and post-dominance
 * frontier.
 */
typedef struct node {
    JITINT32 pidom;         /**< immediate post-dominator of the node */
    XanBitSet*      cdg;    /**< nodes from which this node is control-dependent */
    XanNode*        pDT;    /**< childs of this node in the post-dominators tree */
} t_node;

/*! \var t_node* nodes
 *  \brief method computed data
 *
 * Holds data for all instructions of the working method
 */
t_node* nodes;

/*! \var ir_method_t* _method
 *  \brief working method
 *
 * Global pointer to the working method
 */
ir_method_t* _method;

/*! \var JITINT32 instr_count
 *  \brief instructions count
 *
 * Global counting of the number of instructions of the working method.
 */
JITINT32 instr_count;

/*! \var XanList* entry_dependent;
 *  \brief entry-dependent instruction
 *
 * Global list of the instructions that are control dependent by a fictional entry-node
 */
XanList* entry_dependent;

ir_instruction_t *r_inst;


/* Interface methods	*/
static inline JITUINT64 get_ID_job (void);
static inline void do_job (ir_method_t *method);
static inline char * get_version (void);
static inline JITUINT64 get_dependences (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline void cp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void cp_shutdown (JITFLOAT32 totalTime);
static inline JITUINT64 cdg_get_invalidations (void);

/* Internal methods	*/

/*! \fn depth_first_search(XanNode* dfs_node, int level)
 * \brief perform recursive depth first search of a tree
 *
 * In order to use this function set dfs_node to the root of the tree and level= 0
 */
static inline void depth_first_search (ir_method_t *method, XanNode* dfs_node, int level);

/*! \fn cdg_dominate(int start, int target)
 * \brief perform recursive depth first search of a tree
 *
 * In order to use this function set dfs_node to the root of the tree and level = 0.
 */
static inline int cdg_dominate (int start, int target);

/*! \fn compute_post_dominance_frontier()
 * \brief check if start dominates target
 *
 * start and target must be the instructions' IDs you want to test.
 */
static inline void compute_post_dominance_frontier ();

/*! \fn post_dominance_frontier(JITINT32 n)
 * \brief compute the post dominance frontier of a instruction
 *
 * This function implements the algorithm described in the book
 * "Appel - Modern Compiler Implementation" at page n° 406.
 *
 * \verbatim
        computeDF[n] =
                S ← {}
                for each node y in succ[n]                   This loop computes DFlocal[n]
                        if idom(y) ≠ n
                                S ← S ∪ {y}
                for each child c of n in the dominator tree
                        computeDF[c]
                        for each element w of DF[c]              This loop computes DFup[c]
                                if n does not dominate w, or if n = w
                                        S ← S ∪ {w}
                DF[n] ← S
   \endverbatim
 */
static inline void post_dominance_frontier (JITNINT n);

static inline void free_memory ();

/* Debug methods	*/
static inline void create_graphviz_cdg ();

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    cp_init,
    cp_shutdown,
    do_job,
    get_version,
    get_informations,
    get_author,
    cdg_get_invalidations
};

static inline JITUINT64 cdg_get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void cp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(lib != NULL);
    assert(optimizer != NULL);
}

static inline void cp_shutdown (JITFLOAT32 totalTime) {

}

static inline JITUINT64 get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 get_dependences (void) {
    return POST_DOMINATOR_COMPUTER;
}

static inline void depth_first_search (ir_method_t *method, XanNode* dfs_node, int level) {
    ir_instruction_t *inst;

    /* Assertions.
     */
    assert(method != NULL);
    assert(dfs_node != NULL);

    inst = (ir_instruction_t *) dfs_node->data;
    nodes[IRMETHOD_getInstructionPosition(inst)].pDT = dfs_node;
    XanNode* cursore = dfs_node->getNextChildren(dfs_node, NULL);
    while (cursore != NULL) {
        depth_first_search(method, cursore, level + 1);
        cursore = dfs_node->getNextChildren(dfs_node, cursore);
    }
}

static inline int cdg_dominate (int start, int target) {
    ir_instruction_t *inst;

    if (start == target) {
        return 1;
    } else {
        XanNode* temp = (nodes[target].pDT)->parent;
        if (temp != NULL && temp->data == _method->exitNode) {
            return 0;
        } else {
            inst = temp->data;
            return cdg_dominate(start, IRMETHOD_getInstructionPosition(inst));
        }
    }
}

static inline void do_job (ir_method_t *method) {
    JITUINT32 i;
    ir_instruction_t                *inst_temp;

    /* Assertions				*/
    assert(method != NULL);

    /* Fetch the instructions number	*/
    instr_count = IRMETHOD_getInstructionsNumber(method);

    /* Initialize global data	*/
    _method = method;

    /* if control dependence data is not NULL clean memory before recomputing	*/
    for (i = 0; i < instr_count; i++) {
        inst_temp = IRMETHOD_getInstructionAtPosition(method, i);
        assert(inst_temp != NULL);
        if (inst_temp->metadata->controlDependencies != NULL) {
            xanList_destroyList(inst_temp->metadata->controlDependencies);
        }
        inst_temp->metadata->controlDependencies = NULL;
    }

    /* Allocate space for graphs data (there are two extra nodes
     * for keeping the fakeHead and exit node)
     */
    nodes = (t_node*) allocFunction(sizeof(t_node) * (instr_count + 2));
    assert(nodes != NULL);

    /* get post dominance data into nodes struct	*/
    //nodes[R_NODE].pDT = method->postDominatorTree; //TEST
    //method->postDominatorTree->parent->data=IRMETHOD_getInstructionAtPosition(method, 0); //TEST
    /* get all other post-dominance data into the struct	*/
    depth_first_search(method, method->postDominatorTree, 0);

    /* Initialize graphs data	*/
    for (i = 0; i < instr_count + 2; i++) {
        nodes[i].cdg = xanBitSet_new(instr_count + 2);
        xanBitSet_clearAll(nodes[i].cdg);
    }

    /* compute the post dominance frontier and the cdg	*/
    compute_post_dominance_frontier();

    /* Generate Graphviz graphs	*/
    //create_graphviz_cdg(); //Only for testing

    /* free dynamic allocated memory */
    free_memory();

    /* Return				*/
    return;
}

static inline void compute_post_dominance_frontier () {
    JITINT32 i;

    for (i = 0; i < (instr_count + 2); i++) {
        post_dominance_frontier(i);
    }
}

static inline void post_dominance_frontier (JITNINT n) {
    XanBitSet*      S;
    ir_instruction_t *inst_n, *inst_next, *inst_temp1, *inst_temp2, *inst;
    XanNode*        pDT_item = NULL;
    JITINT32 w, y;
    JITUINT32 instNum;

    /* DF[n] <- S;	*/
    S = xanBitSet_new(instr_count + 2);

    /* S <- {}	*/
    xanBitSet_clearAll(S);

    /* for each node y in succ[n]	*/

    /* special case for r node	*/
    if (n == R_NODE) {
        /* dominance frontier of r is empty	*/
        /* do nothing	*/
        ;

        /* special case for exit node	*/
    } else if (n == EXIT_NODE ) {
        /* do nothing	*/
        ;

        /* special case for entry node	*/
    } else {
        inst_n = IRMETHOD_getInstructionAtPosition(_method, n);
        inst_next = NULL;
        /* in reverse cfg the successors are the predecessors	*/
        while (  (inst_next = IRMETHOD_getPredecessorInstruction(_method, inst_n, inst_next)) != NULL) {
            XanNode *node;

            y = IRMETHOD_getInstructionPosition(inst_next);

            /* if idom(y) != n  (check if y's immediate dominator is not n)	*/
            node = (nodes[y].pDT)->getParent(nodes[y].pDT);
            assert(node != NULL);
            inst = (ir_instruction_t *) node->data;
            assert(inst != NULL);
            if (IRMETHOD_getInstructionPosition(inst) != n) {
                /* S <- S U {y}	*/
                xanBitSet_setBit(S, y);

                /* if y is in n's DF, then n is control-dependent from y	*/

                /* check if this element is not yet in the list	*/
                if (!xanBitSet_isBitSet(nodes[n].cdg, y)) {
                    inst_temp1 = IRMETHOD_getInstructionAtPosition(_method, n);
                    if (y == R_NODE) {
                        inst_temp2 = r_inst;
                    } else {
                        inst_temp2 = IRMETHOD_getInstructionAtPosition(_method, y);
                    }

                    IRMETHOD_addInstructionControlDependence(_method, inst_temp1, inst_temp2);
                    xanBitSet_setBit(nodes[n].cdg, y);
                }
            }
        }
    }

    /* for each child c of n in the dominator tree	*/
    assert(nodes != NULL);
    assert(nodes[n].pDT != NULL);
    pDT_item = (nodes[n].pDT)->getNextChildren(nodes[n].pDT, NULL);
    while (pDT_item != NULL) {
        /* computeDF[c]	*/
        inst = (ir_instruction_t *) pDT_item->data;
        instNum = IRMETHOD_getInstructionPosition(inst);
        post_dominance_frontier(instNum);

        /* for each element w of DF[c]	*/
        for (w = 0; w < (instr_count + 2); w++) {
            if (xanBitSet_isBitSet(nodes[instNum].cdg, w)) {
                /* if n does not dominate w, or if n = w	*/
                if ( !cdg_dominate(n, w) || (n == w) ) {
                    /* S <- S U {w}	*/

                    xanBitSet_setBit(S, w);
                    /* if w is in n's DF, then n is control-dependente from w	*/

                    /* check if this element is not yet in the list	*/
                    if (!xanBitSet_isBitSet(nodes[n].cdg, w)) {
                        inst_temp1 = IRMETHOD_getInstructionAtPosition(_method, n);
                        if (w == R_NODE) {
                            inst_temp2 = r_inst;
                        } else {
                            inst_temp2 = IRMETHOD_getInstructionAtPosition(_method, w);
                        }
                        IRMETHOD_addInstructionControlDependence(_method, inst_temp1, inst_temp2);
                        xanBitSet_setBit(nodes[n].cdg, w);
                    }

                }
            }
        }
        pDT_item = (nodes[n].pDT)->getNextChildren(nodes[n].pDT, pDT_item);
    }

    /* clean temp bitset	*/
    xanBitSet_free(S);
}

static inline void free_memory () {
    JITUINT32 i;

    for (i = 0; i < (instr_count + 2); i++) {
        xanBitSet_free(nodes[i].cdg);
        //(nodes[i].pDT)->free(nodes[i].pDT);
    }
    freeFunction(nodes);
}

static inline void create_graphviz_cdg () {
    FILE *cdg;
    JITINT32 i, j;

    if ( (cdg = fopen("cdg.dot", "w")) == NULL) {
        fprintf(stdout, "Unable to create cdg.dot\n");
    } else {
        fprintf(cdg, "digraph cdg {\n");
        for (i = 0; i < (instr_count + 2); i++) {
            fprintf(cdg, "%d [label=\"%d\"]\n", i, i);
        }
        for (i = 0; i < (instr_count + 2); i++) {
            for (j = 0; j < (instr_count + 2); j++) {
                if (xanBitSet_isBitSet(nodes[i].cdg, j)) {
                    /* node i is control-dependent on node j	*/
                    fprintf(cdg, "%d->%d\n", j, i);
                }
            }
        }
        fprintf(cdg, "}\n");
        fclose(cdg);
    }
}

static inline char * get_version (void) {
    return VERSION;
}

static inline char * get_informations (void) {
    return "Build the Control-Dependence Graph";
}

static inline char * get_author (void) {
    return AUTHOR;
}
