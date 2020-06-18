/*
 * Copyright (C) 2010 - 2011  Campanoni Simone
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
#include <stdlib.h>
#include <jitsystem.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <chiara.h>
#include <chiara_system.h>
#include <chiara_load_loop_names.h>
// End

#define NODE_ATTRIBUTES "fontsize=12"

static inline JITBOOLEAN internal_LOOPS_isASubLoop (XanList *allLoops, loop_t *loop, loop_t *subloop, XanList *alreadyConsidered);
static inline JITUINT32 internal_LOOPS_getInstructionLoopNesting (loop_t *loop, ir_instruction_t *inst, XanList *alreadyConsidered);
static inline void internal_getLoopsMayBeNotNested (XanList *loops, JITUINT32 nestingLevel, XanList *notNestedLoops);
static inline void internal_append_subloops (loop_t *loop, JITUINT32 nestingLevel, XanList *subloops, XanList *alreadyConsidered);
static inline void internal_add_node_and_its_subloops_to_the_top_of_the_tree (XanNode *tree, loop_t *loop);
static inline JITUINT32 internal_loops_hash (void *element);
static inline JITINT32 internal_are_loops_equal (void *key1, void *key2);
static inline JITINT8 * internal_get_loop_name (loop_t *loop);

void LOOPS_analyzeCircuits (ir_method_t *m) {
    IROPTIMIZER_callMethodOptimization(NULL, m, LOOP_IDENTIFICATION);
    IROPTIMIZER_callMethodOptimization(NULL, m, LOOP_INVARIANTS_IDENTIFICATION);
    IROPTIMIZER_callMethodOptimization(NULL, m, INDUCTION_VARIABLES_IDENTIFICATION);

    return ;
}

void LOOPS_dumpAllLoops (XanHashTable *loops, FILE *fileToWrite, char *programName) {
    XanList         *l;

    /* Fetch the list of loops	*/
    l = xanHashTable_toList(loops);

    /* Dump all the loops		*/
    LOOPS_dumpLoops(loops, l, fileToWrite, programName);

    /* Free the memory		*/
    xanList_destroyList(l);

    /* Return			*/
    return;
}

XanList * LOOPS_loadLoopNames (JITINT8 *fileName) {
    XanList	*l;

    /* Assertions.
     */
    assert(fileName != NULL);

    /* Fetch the list of names.
     */
    l	= load_list_of_loop_names(fileName);

    return l;
}

void LOOPS_dumpLoops (XanHashTable *allLoops, XanList *loops, FILE *fileToWrite, char *programName) {
    XanListItem     *item;
    XanList         *outermostLoops;
    JITUINT32 count;

    /* Assertions			*/
    assert(loops != NULL);
    assert(fileToWrite != NULL);
    assert(allLoops != NULL);

    /* Fetch the outermost loops	*/
    outermostLoops = LOOPS_getOutermostProgramLoops(allLoops);
    assert(outermostLoops != NULL);

    /* Print the head of the graph	*/
    fprintf(fileToWrite, "digraph \"%s\" {\n", programName);
    fprintf(fileToWrite, "label=\"Loop analysis\"\n");

    /* Print the nodes		*/
    count = 0;
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t          *loop;

        /* Fetch the loop			*/
        loop = (loop_t *) item->data;
        assert(loop != NULL);
        assert(xanList_length(loop->instructions) > 0);

        /* Dump the loop			*/
        fprintf(fileToWrite, "node [ %s color=", NODE_ATTRIBUTES);
        if (xanList_find(outermostLoops, loop) != NULL) {
            fprintf(fileToWrite, "blue ");
        } else {
            fprintf(fileToWrite, "black ");
        }
        fprintf(fileToWrite, "label=\"");
        fprintf(fileToWrite, "%s: %d\\n", IRMETHOD_getSignatureInString(loop->method), loop->header->ID);
        fprintf(fileToWrite, "Body: %d insts\\n", xanList_length(loop->instructions));
        fprintf(fileToWrite, "Exits: %d", xanList_length(loop->exits));
        fprintf(fileToWrite, "\" shape=box ] ;\n");
        fprintf(fileToWrite, "n%u ;\n", count);

        /* Fetch the next element from the list	*/
        count++;
        item = item->next;
    }

    /* Print the edges		*/
    count = 0;
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t          *loop;

        /* Fetch the loop			*/
        loop = (loop_t *) item->data;
        assert(loop != NULL);

        /* Check if the current loop has        *
         * sub-loops				*/
        if (    (loop->subLoops != NULL)                        &&
                (xanList_length(loop->subLoops) > 0)    ) {
            XanListItem     *item2;

            /* Dump the edges from the current loop to every	*
             * sub-loops						*/
            item2 = xanList_first(loop->subLoops);
            while (item2 != NULL) {
                loop_t          *subLoop;
                JITINT32 count2;
                subLoop = (loop_t *) item2->data;
                assert(subLoop != NULL);
                count2 = xanList_getPositionNumberFromElement(loops, subLoop);

                /* Check if the subloop is part of the list of loops to	*
                 * consider (given as input)				*/
                if (count2 != -1) {
                    assert(count2 < xanList_length(loops));
                    assert(count2 != -1);
                    fprintf(fileToWrite, "n%u -> n%d;\n", count2, count);
                }
                item2 = item2->next;
            }
        }

        /* Fetch the next element from the list	*/
        count++;
        item = item->next;
    }

    /* Print the foot of the graph	*/
    fprintf(fileToWrite, "}\n");

    /* Flush the output channel	*/
    fflush(fileToWrite);

    /* Free the memory		*/
    xanList_destroyList(outermostLoops);

    /* Return			*/
    return;
}

XanList * LOOPS_getProgramLoopsMayBeAtNestingLevel (XanHashTable *loops, JITUINT32 nestingLevel) {
    XanList *l;
    XanList *toDelete;
    XanListItem *item;

    /* Assertions			*/
    assert(loops != NULL);

    /* Fetch the loops at the 	*
     * specified nesting level	*/
    l = LOOPS_getLoopsMayBeAtNestingLevel(loops, nestingLevel);
    assert(l != NULL);

    /* Remove every loop that 	*
     * belongs to external libraries*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(l);
    while (item != NULL) {
        loop_t *loop;
        loop = item->data;
        if (IRPROGRAM_doesMethodBelongToALibrary(loop->method)) {
            xanList_append(toDelete, loop);
        }
        item = item->next;
    }
    item = xanList_first(toDelete);
    while (item != NULL) {
        xanList_removeElement(l, item->data, JITTRUE);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    /* Return			*/
    return l;
}

XanList * LOOPS_getLoopsMayBeAtNestingLevel (XanHashTable *loops, JITUINT32 nestingLevel) {
    XanList *l;
    XanList *allLoops;
    XanList *outermostLoops;

    /* Assertions			*/
    assert(loops != NULL);

    /* Fetch all the loops		*/
    allLoops = xanHashTable_toList(loops);

    /* Fetch the outermost loops	*/
    outermostLoops = LOOPS_getMayBeOutermostLoops(loops, allLoops);
    assert(outermostLoops != NULL);

    /* Fetch the loops at nesting	*
     * level specified		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    internal_getLoopsMayBeNotNested(outermostLoops, nestingLevel, l);

    /* Free the memory		*/
    xanList_destroyList(outermostLoops);
    xanList_destroyList(allLoops);

    /* Return			*/
    return l;
}

static inline void internal_getLoopsMayBeNotNested (XanList *loops, JITUINT32 nestingLevel, XanList *notNestedLoops) {
    XanListItem *item;

    /* Assertions			*/
    assert(loops != NULL);
    assert(notNestedLoops != NULL);

    /* Check the nesting level	*/
    if (nestingLevel == 0) {

        /* Add the loops 		*/
        item = xanList_first(loops);
        while (item != NULL) {
            loop_t *loop;
            loop = item->data;
            if (xanList_find(notNestedLoops, loop) == NULL) {
                xanList_append(notNestedLoops, loop);
            }
            item = item->next;
        }

    } else {

        /* Go deeply to the next	*
         * nesting level		*/
        item = xanList_first(loops);
        while (item != NULL) {
            loop_t *loop;
            loop = item->data;
            internal_getLoopsMayBeNotNested(loop->subLoops, nestingLevel - 1, notNestedLoops);
            item = item->next;
        }
    }

    /* Return			*/
    return ;
}

XanList * LOOPS_getProgramLoops (XanHashTable *loops) {
    XanList		*allLoops;

    /* Assertions			*/
    assert(loops != NULL);

    /* Fetch the full list of loops	*/
    allLoops = xanHashTable_toList(loops);
    assert(allLoops != NULL);

    /* Remove every loop that 	*
     * belongs to libraries		*/
    LOOPS_removeLibraryLoops(allLoops);

    /* Return			*/
    return allLoops;
}

XanList * LOOPS_getOutermostProgramLoops (XanHashTable *loops) {
    XanList         *l;
    XanList		*allLoops;

    /* Assertions			*/
    assert(loops != NULL);

    /* Fetch the full list of loops	*/
    allLoops = xanHashTable_toList(loops);
    assert(allLoops != NULL);

    /* Fetch the outermost loops	*/
    l = LOOPS_getOutermostLoops(loops, allLoops);
    assert(l != NULL);

    /* Remove every loop that 	*
     * belongs to libraries		*/
    LOOPS_removeLibraryLoops(l);

    /* Free the memory		*/
    xanList_destroyList(allLoops);

    /* Return			*/
    return l;
}

void LOOPS_destroyLoopsNames (XanHashTable *loopsNames) {
    XanHashTableItem *hashItem;

    /* Check the names		*/
    if (loopsNames == NULL) {
        return ;
    }

    /* Free the memory		*/
    hashItem = xanHashTable_first(loopsNames);
    while (hashItem != NULL) {
        freeFunction(hashItem->element);
        hashItem = xanHashTable_next(loopsNames, hashItem);
    }
    xanHashTable_destroyTable(loopsNames);

    /* Return			*/
    return ;
}

XanList * LOOPS_getLoopsWithinMethod (XanHashTable *loops, ir_method_t *method) {
    XanHashTableItem	*item;
    XanList			*list;

    /* Assertions.
     */
    assert(loops != NULL);
    assert(method != NULL);

    /* Allocate the necessary memory.
     */
    list	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the loops included in the method.
     */
    item	= xanHashTable_first(loops);
    while (item != NULL) {
        loop_t	*loop;
        loop	= item->element;
        assert(loop != NULL);
        assert(loop->method != NULL);
        if (loop->method == method) {
            xanList_append(list, loop);
        }
        item	= xanHashTable_next(loops, item);
    }

    return list;
}

static inline void internal_LOOPS_destroySuccessorsOrPredecessorsOfLoops (XanHashTable *table) {
    XanHashTableItem *item;

    /* Assertions			*/
    assert(table != NULL);

    /* Destroy the successors or 	*
     * predecessors of loops	*/
    item = xanHashTable_first(table);
    while (item != NULL) {
        XanList *l;

        /* Fetch the loop		*/
        l = item->element;
        assert(l != NULL);

        /* Free the memory		*/
        xanList_destroyList(l);

        /* Fetch the next element from	*
         * the list			*/
        item = xanHashTable_next(table, item);
    }
    xanHashTable_destroyTable(table);

    /* Return			*/
    return ;
}

void LOOPS_destroySuccessorsAndPredecessorsOfLoops (XanHashTable *loopsSuccessors, XanHashTable *loopsPredecessors) {

    /* Assertions			*/
    assert(loopsSuccessors != NULL);
    assert(loopsPredecessors != NULL);

    /* Free the memory		*/
    internal_LOOPS_destroySuccessorsOrPredecessorsOfLoops(loopsSuccessors);
    internal_LOOPS_destroySuccessorsOrPredecessorsOfLoops(loopsPredecessors);

    /* Return			*/
    return ;
}

void LOOPS_getSuccessorsAndPredecessorsOfLoops (XanHashTable *loopsSuccessors, XanHashTable *loopsPredecessors, XanList *loops) {
    XanListItem *item;

    /* Assertions			*/
    assert(loopsSuccessors != NULL);
    assert(loopsPredecessors != NULL);
    assert(loops != NULL);

    /* Fetch the successors and 	*
     * predecessors of loops	*/
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t *loop;
        XanList *loopSuccessors;
        XanList *predecessors;

        /* Fetch the loop		*/
        loop = item->data;
        assert(loop != NULL);

        /* Fetch the successors		*/
        loopSuccessors = LOOPS_getSuccessorsOfLoop(loop);
        assert(loopSuccessors != NULL);
        xanHashTable_insert(loopsSuccessors, loop, loopSuccessors);

        /* Fetch the predecessors	*/
        predecessors = IRMETHOD_getInstructionPredecessors(loop->method, loop->header);
        assert(predecessors != NULL);
        xanHashTable_insert(loopsPredecessors, loop, predecessors);

        /* Fetch the next element from	*
         * the list			*/
        item = item->next;
    }

    /* Return			*/
    return ;
}

XanList * LOOPS_getSuccessorsOfLoop (loop_t *loop) {
    XanList         *l;
    XanListItem     *item;

    /* Assertions			*/
    assert(loop != NULL);

    /* Allocate the list of         *
     * successors			*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Check if there are exit      *
     * points of the loop		*/
    if (    (loop->exits == NULL)                      ||
            (xanList_length(loop->exits) == 0)         ) {

        /* Return			*/
        return l;
    }

    /* Fill up the list		*/
    item = xanList_first(loop->exits);
    while (item != NULL) {
        ir_instruction_t        *inst;
        ir_instruction_t        *succ;
        inst = item->data;
        assert(inst != NULL);
        succ = IRMETHOD_getSuccessorInstruction(loop->method, inst, NULL);
        while (succ != NULL) {
            if (	(succ->type != IREXITNODE)					&&
                    (xanList_find(loop->instructions, succ) == NULL) 		&&
                    (xanList_find(l, succ) == NULL)					) {
                xanList_append(l, succ);
                break;
            }
            succ = IRMETHOD_getSuccessorInstruction(loop->method, inst, succ);
        }
        item = item->next;
    }
    assert(xanList_length(loop->exits) >= xanList_length(l));

    /* Return			*/
    return l;
}

loop_t * LOOPS_getLoopFromName (XanHashTable *loops, XanHashTable *loopsNames, JITINT8 *name) {
    XanHashTableItem	*item;

    /* Assertions			*/
    assert(loops != NULL);
    assert(loopsNames != NULL);
    assert(name != NULL);

    item	= xanHashTable_first(loops);
    while (item != NULL) {
        loop_t	*loop;
        JITINT8	*currentLoopName;
        loop		= item->element;
        assert(loop != NULL);
        currentLoopName	= xanHashTable_lookup(loopsNames, loop);
        if (	(currentLoopName != NULL)			&&
                (STRCMP(name, currentLoopName) == 0)		) {
            return loop;
        }
        item	= xanHashTable_next(loops, item);
    }

    /* Return			*/
    return NULL;
}

void LOOPS_removeLibraryLoops (XanList *loops) {
    XanList         *toDelete;
    XanListItem     *item;

    /* Assertions			*/
    assert(loops != NULL);

    /* Allocate the necessary list	*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Remove every loop that       *
     * belongs to a library		*/
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t  * loop;
        loop = (loop_t *) item->data;
        assert(loop != NULL);
        if (IRPROGRAM_doesMethodBelongToALibrary(loop->method)) {
            xanList_insert(toDelete, loop);
        }
        item = item->next;
    }
    item = xanList_first(toDelete);
    while (item != NULL) {
        xanList_removeElement(loops, item->data, JITTRUE);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    /* Return			*/
    return;
}

XanList * LOOPS_getNotLibraryLoops (XanHashTable *loops) {
    XanList         *l;
    XanList         *toDelete;
    XanListItem     *item;

    /* Assertions			*/
    assert(loops != NULL);

    /* Allocate the necessary list	*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the list of loops	*/
    l = xanHashTable_toList(loops);

    /* Remove every loop that       *
     * belongs to a library		*/
    item = xanList_first(l);
    while (item != NULL) {
        loop_t  * loop;
        loop = (loop_t *) item->data;
        assert(loop != NULL);
        if (IRPROGRAM_doesMethodBelongToALibrary(loop->method)) {
            xanList_insert(toDelete, loop);
        }
        item = item->next;
    }
    item = xanList_first(toDelete);
    while (item != NULL) {
        xanList_removeElement(l, item->data, JITTRUE);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    /* Return			*/
    return l;
}

XanNode * LOOPS_getMethodLoopsNestingTree (XanHashTable *loops, ir_method_t *method) {
    XanNode *tree;
    XanList *outermostLoops;
    XanListItem *item;

    /* Assertions				*/
    assert(loops != NULL);
    assert(method != NULL);

    /* Allocate the root of the tree	*/
    tree = xanNode_new(allocFunction, freeFunction, NULL);
    assert(tree != NULL);

    /* Fetch the outermost loops		*/
    outermostLoops = IRLOOP_getOutermostLoopsWithinMethod(method, loops);
    assert(outermostLoops != NULL);

    /* Fill up the tree			*/
    item = xanList_first(outermostLoops);
    while (item != NULL) {
        loop_t *loop;

        /* Fetch the loop			*/
        loop = item->data;
        assert(loop != NULL);

        /* Add the nodes to the tree		*/
        internal_add_node_and_its_subloops_to_the_top_of_the_tree(tree, loop);

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(outermostLoops);

    /* Return				*/
    return tree;
}

static inline void internal_add_node_and_its_subloops_to_the_top_of_the_tree (XanNode *tree, loop_t *loop) {
    XanNode *node;
    XanListItem *child;

    /* Assertions				*/
    assert(tree != NULL);
    assert(loop != NULL);
    assert(tree->find(tree, loop) == NULL);

    /* Add the loop to the tree		*/
    node = xanNode_new(allocFunction, freeFunction, NULL);
    assert(node != NULL);
    node->setData(node, loop);
    tree->addChildren(tree, node);

    /* Check if there are subloops		*/
    if (loop->subLoops == NULL) {
        return ;
    }

    /* Add its subloops			*/
    child = xanList_first(loop->subLoops);
    while (child != NULL) {
        loop_t *childLoop;
        childLoop = child->data;
        assert(childLoop != NULL);
        internal_add_node_and_its_subloops_to_the_top_of_the_tree(node, childLoop);
        child = child->next;
    }

    /* Return				*/
    return ;
}

XanHashTable * LOOPS_getLoopsNames (XanHashTable *loops) {
    XanHashTableItem	*hashItem;
    XanHashTable		*loopsNames;

    /* Assertions			*/
    assert(loops != NULL);

    /* Allocate the loops names 	*
    * table       			*/
    loopsNames = xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, internal_loops_hash, internal_are_loops_equal);
    assert(loopsNames != NULL);

    /* Create the table		*/
    hashItem = xanHashTable_first(loops);
    while (hashItem != NULL) {
        loop_t *loop;
        JITINT8 *loopName;
        loop = hashItem->element;
        assert(loop != NULL);
        loopName = internal_get_loop_name(loop);
        assert(loopName != NULL);
        xanHashTable_insert(loopsNames, loop, loopName);
        assert(xanHashTable_lookup(loopsNames, loop) == loopName);
        hashItem = xanHashTable_next(loops, hashItem);
    }

    /* Return			*/
    return loopsNames;
}

JITBOOLEAN LOOPS_isASubLoop (XanList *allLoops, loop_t *loop, loop_t *subloop) {
    XanList *l;
    JITBOOLEAN is;

    /* Assertions					*/
    assert(allLoops != NULL);
    assert(loop != NULL);
    assert(subloop != NULL);
    assert(xanList_find(allLoops, loop) != NULL);

    /* Allocate the list of loops considered	*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Check if subloop is a subloop of loop	*/
    is = internal_LOOPS_isASubLoop(allLoops, loop, subloop, l);

    /* Free the memory				*/
    xanList_destroyList(l);

    /* Return					*/
    return is;
}

static inline JITBOOLEAN internal_LOOPS_isASubLoop (XanList *allLoops, loop_t *loop, loop_t *subloop, XanList *alreadyConsidered) {
    XanListItem *item;

    /* Assertions					*/
    assert(allLoops != NULL);
    assert(loop != NULL);
    assert(alreadyConsidered != NULL);

    /* Check to see if loop was already considered	*/
    if (xanList_find(alreadyConsidered, loop) != NULL) {
        return JITFALSE;
    }
    xanList_insert(alreadyConsidered, loop);

    /* Check to see if subloop is a direct subloop 	*
     * of loop					*/
    if (xanList_find(loop->subLoops, subloop) != NULL) {
        return JITTRUE;
    }

    /* Iterate over the subloops of loop		*/
    item = xanList_first(loop->subLoops);
    while (item != NULL) {
        loop_t  *loop2;
        loop2 = (loop_t *) item->data;
        assert(loop2 != NULL);
        if (internal_LOOPS_isASubLoop(allLoops, loop2, subloop, alreadyConsidered)) {
            return JITTRUE;
        }
        item = item->next;
    }

    /* Return					*/
    return JITFALSE;
}

JITBOOLEAN LOOPS_isASelfCalledLoop (XanList *allLoops, loop_t *loop) {
    XanList *parents;
    XanListItem *item;
    JITBOOLEAN found;

    /* Assertions					*/
    assert(allLoops != NULL);
    assert(loop != NULL);
    assert(xanList_find(allLoops, loop) != NULL);

    /* Fetch the parents of the loop		*/
    parents = LOOPS_getImmediateParentsOfLoop(allLoops, loop);
    assert(parents != NULL);

    /* Check the loop				*/
    found = JITFALSE;
    item = xanList_first(parents);
    while (item != NULL) {
        loop_t *parent;
        parent = item->data;
        if (parent == loop) {
            found = JITTRUE;
            break;
        }
        item = item->next;
    }

    /* Return					*/
    return found;
}

XanList * LOOPS_getImmediateParentsOfLoop (XanList *loops, loop_t *loop) {
    XanListItem *item;
    XanList *l;

    /* Assertions					*/
    assert(loops != NULL);
    assert(loop != NULL);
    assert(xanList_find(loops, loop) != NULL);

    /* Allocate the list of loops considered	*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Iterate over the subloops of loop		*/
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t  *loop2;
        loop2 = (loop_t *) item->data;
        assert(loop2 != NULL);
        if (xanList_find(loop2->subLoops, loop) != NULL) {
            xanList_append(l, loop2);
        }
        item = item->next;
    }

    /* Return					*/
    return l;
}

XanList * LOOPS_getMayBeOutermostLoops (XanHashTable *loops, XanList *loopsToConsider) {
    XanList         *l;
    XanList         *allLoops;
    XanListItem     *item;
    ir_method_t 	*mainMethod;

    /* Assertions					*/
    assert(loops != NULL);
    assert(loopsToConsider != NULL);
    PDEBUG("CHIARA: GetMayBeOutermostLoops: Start\n");

    /* Allocate the list of outermost loops		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the list of all loops of the program	*/
    allLoops = xanHashTable_toList(loops);
    assert(allLoops != NULL);

    /* Fetch the main method			*/
    mainMethod = METHODS_getMainMethod();
    assert(mainMethod != NULL);

    /* Fill up the list				*/
    item = xanList_first(loopsToConsider);
    while (item != NULL) {
        loop_t          *loop;
        XanList		*parents;
        XanList		*parentsMethods;
        XanListItem 	*item2;
        JITBOOLEAN	mayBeOutermost;

        /* Fetch the loop				*/
        loop = (loop_t *) item->data;
        assert(loop != NULL);
        PDEBUG("CHIARA: GetMayBeOutermostLoops: 	%s: %u\n", IRMETHOD_getSignatureInString(loop->method), loop->header->ID);

        /* Fetch the parents of the loop		*/
        parents = LOOPS_getImmediateParentsOfLoop(allLoops, loop);
        assert(parents != NULL);

        /* Print the parents				*/
#ifdef PRINTDEBUG
        PDEBUG("CHIARA: GetMayBeOutermostLoops: 		Parents\n");
        item2 = xanList_first(parents);
        while (item2 != NULL) {
            loop_t *parent;
            parent = item2->data;
            PDEBUG("CHIARA: GetMayBeOutermostLoops: 			%s: %u\n", IRMETHOD_getSignatureInString(parent->method), parent->header->ID);
            item2 = item2->next;
        }
#endif

        /* Compute the list of methods of parents	*/
        PDEBUG("CHIARA: GetMayBeOutermostLoops: 		Parents methods\n");
        parentsMethods = xanList_new(allocFunction, freeFunction, NULL);
        item2 = xanList_first(parents);
        while (item2 != NULL) {
            loop_t *parent;
            parent = item2->data;
            if (	(loop != parent)						&&
                    (xanList_find(parentsMethods, parent->method) == NULL)	) {
                PDEBUG("CHIARA: GetMayBeOutermostLoops: 			%s\n", IRMETHOD_getSignatureInString(parent->method));
                xanList_append(parentsMethods, parent->method);
            }
            item2 = item2->next;
        }

        /* Check to see if it is included within another*
         * one.						*/
        PDEBUG("CHIARA: GetMayBeOutermostLoops: 		Check if the loop may be an outermost one\n");
        mayBeOutermost = JITTRUE;
        if (xanList_length(parents) == 0) {
            mayBeOutermost = JITTRUE;
            PDEBUG("CHIARA: GetMayBeOutermostLoops: 			No parent => Outermost\n");

        } else if (xanList_find(parentsMethods, loop->method) != NULL) {

            mayBeOutermost = JITFALSE;
            PDEBUG("CHIARA: GetMayBeOutermostLoops: 			There is a parent within the same method of loop => Inner\n");

        } else if (xanList_length(parentsMethods) == 0) {
            mayBeOutermost = JITTRUE;
            PDEBUG("CHIARA: GetMayBeOutermostLoops: 			No parent different than loop => Outermost\n");

        } else {
            assert(xanList_length(parents) > 0);
            assert(xanList_length(parentsMethods) > 0);
            assert(xanList_find(parentsMethods, loop->method) == NULL);

            /* If there is a path starting from the entry	*
             * point of the program and ending to the loop	*
             * without going to any parent loops, then loop	*
             * is an outermost loop				*/
            mayBeOutermost = JITFALSE;
            if (METHODS_isReachableWithoutGoingThroughMethods(mainMethod, loop->method, parentsMethods, NULL)) {
                mayBeOutermost = JITTRUE;
                PDEBUG("CHIARA: GetMayBeOutermostLoops: 			There is a path to loop that avoid every parent\n");
            } else {
#ifdef PRINTDEBUG
                PDEBUG("CHIARA: GetMayBeOutermostLoops: 			There is no path from main to the loop avoiding every parent\n");
#endif
            }
        }
        if (mayBeOutermost) {

            /* Loop is not included in any other	*
             * loop which means it is an outermost	*
             * loop					*/
            PDEBUG("CHIARA: GetMayBeOutermostLoops: 		Outermost\n");
            assert(xanList_find(l, loop) == NULL);
            xanList_insert(l, loop);
        }

        /* Free the memory		*/
        xanList_destroyList(parents);
        xanList_destroyList(parentsMethods);

        /* Fetch the next element from	*
         * the list			*/
        item = item->next;
    }

    /* Print the outermost loops			*/
    PDEBUG("CHIARA: GetMayBeOutermostLoops: 	Outermost loops\n");
#ifdef PRINTDEBUG
    item = xanList_first(l);
    while (item != NULL) {
        loop_t *loop;
        loop = item->data;
        assert(loop != NULL);
        PDEBUG("CHIARA: GetMayBeOutermostLoops: 		%s: %u\n", IRMETHOD_getSignatureInString(loop->method), loop->header->ID);
        item = item->next;
    }
#endif

    /* Free the memory				*/
    xanList_destroyList(allLoops);

    /* Return					*/
    PDEBUG("CHIARA: GetMayBeOutermostLoops: Exit\n");
    return l;
}

XanList * LOOPS_getOutermostLoops (XanHashTable *loops, XanList *loopsToConsider) {
    XanList         *l;
    XanList         *allLoops;
    XanListItem     *item;
    ir_method_t 	*mainMethod;

    /* Assertions					*/
    assert(loops != NULL);
    assert(loopsToConsider != NULL);
    PDEBUG("CHIARA: GetOutermostLoops: Start\n");

    /* Allocate the list of outermost loops		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fetch the list of all loops of the program	*/
    allLoops = xanHashTable_toList(loops);
    assert(allLoops != NULL);

    /* Fetch the main method			*/
    mainMethod = METHODS_getMainMethod();
    assert(mainMethod != NULL);

    /* Fill up the list				*/
    item = xanList_first(loopsToConsider);
    while (item != NULL) {
        loop_t          *loop;
        XanList		*parents;
        XanList		*parentsMethods;
        XanListItem 	*item2;

        /* Fetch the loop				*/
        loop = (loop_t *) item->data;
        assert(loop != NULL);
        PDEBUG("CHIARA: GetOutermostLoops: 	%s: %u\n", IRMETHOD_getSignatureInString(loop->method), loop->header->ID);

        /* Fetch the parents of the loop		*/
        parents = LOOPS_getImmediateParentsOfLoop(allLoops, loop);
        assert(parents != NULL);

        /* Print the parents			*/
#ifdef PRINTDEBUG
        PDEBUG("CHIARA: GetOutermostLoops: 		Parents\n");
        item2 = xanList_first(parents);
        while (item2 != NULL) {
            loop_t *parent;
            parent = item2->data;
            PDEBUG("CHIARA: GetOutermostLoops: 			%s: %u\n", IRMETHOD_getSignatureInString(parent->method), parent->header->ID);
            item2 = item2->next;
        }
#endif

        /* Compute the list of methods of parents	*/
        parentsMethods = xanList_new(allocFunction, freeFunction, NULL);
        item2 = xanList_first(parents);
        while (item2 != NULL) {
            loop_t *parent;
            parent = item2->data;
            if (xanList_find(parentsMethods, parent->method) == NULL) {
                xanList_append(parentsMethods, parent->method);
            }
            item2 = item2->next;
        }

        /* Check to see if it is included within another*
         * one.						*
         * If there is a path starting from the entry	*
         * point of the program and ending to the loop	*
         * without going to any parent loops, then loop	*
         * is an outermost loop				*/
        PDEBUG("CHIARA: GetOutermostLoops: 		Checking if the loop is an outermost one\n");
        item2 = xanList_first(parents);
        while (item2 != NULL) {
            loop_t *parent;

            /* Fetch the parent loop		*/
            parent = item2->data;
            if (loop != parent) {
                XanList *methodsToConsider;
                PDEBUG("CHIARA: GetOutermostLoops: 			%s: %u\n", IRMETHOD_getSignatureInString(parent->method), parent->header->ID);

                /* Check if they are within the same 	*
                 * method				*/
                if (loop->method == parent->method) {

                    /* Loop is a subloop			*/
                    PDEBUG("CHIARA: GetOutermostLoops: 				The loop is self contained to it\n");
                    break;
                }

                /* Check if there is no path that start	*
                 * from the entry point and ends to loop*
                 * without going through parent		*/
                methodsToConsider = xanList_cloneList(parentsMethods);
                assert(methodsToConsider != NULL);
                if (xanList_find(methodsToConsider, loop->method) != NULL) {
                    xanList_removeElement(methodsToConsider, loop->method, JITTRUE);
                }
                if (!METHODS_isReachableWithoutGoingThroughMethods(mainMethod, loop->method, methodsToConsider, NULL)) {

                    /* Loop is not an outermost one		*/
                    PDEBUG("CHIARA: GetOutermostLoops: 				No trace to reach the loop and therefore it is self contained to the current parent\n");
                    xanList_destroyList(methodsToConsider);
                    break;
                }
                xanList_destroyList(methodsToConsider);
            }

            item2 = item2->next;
        }
        if (item2 == NULL) {

            /* Loop is not included in any other	*
             * loop which means it is an outermost	*
             * loop					*/
            PDEBUG("CHIARA: GetOutermostLoops: 		Outermost\n");
            assert(xanList_find(l, loop) == NULL);
            xanList_insert(l, loop);
        }

        /* Free the memory		*/
        xanList_destroyList(parents);
        xanList_destroyList(parentsMethods);

        /* Fetch the next element from	*
         * the list			*/
        item = item->next;
    }

    /* Free the memory				*/
    xanList_destroyList(allLoops);

    /* Return					*/
    PDEBUG("CHIARA: GetOutermostLoops: Exit\n");
    return l;
}

XanList * LOOPS_getCallInstructionsWithinLoop (loop_t *loop) {
    XanList         *l;
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    ir_method_t     *method;

    /* Assertions					*/
    assert(loop->method != NULL);
    assert(loop != NULL);

    /* Fetch the method				*/
    method = loop->method;

    /* Fetch the instructions number		*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Allocate the list				*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fill up the list				*/
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (    (inst->type == IRCALL)                                          &&
                (xanList_find(loop->instructions, inst) != NULL)    ) {
            xanList_insert(l, inst);
        }
    }

    /* Return					*/
    return l;
}

JITUINT32 LOOPS_numberOfBackedges (loop_t *loop) {
    XanList         *l;
    JITUINT32 num;

    l = LOOPS_getBackedges(loop);
    assert(l != NULL);
    num = xanList_length(l);
    xanList_destroyList(l);

    return num;
}

XanList * LOOPS_getExitInstructions (loop_t *loop) {
    if (loop->exits == NULL) {
        return xanList_new(allocFunction, freeFunction, NULL);
    }
    return xanList_cloneList(loop->exits);
}

XanList * LOOPS_getBackedges (loop_t *loop) {
    XanList                 *l;
    ir_instruction_t        *pred;

    /* Assertions				*/
    assert(loop != NULL);
    assert(loop->method != NULL);
    assert(loop->header != NULL);
    assert(loop->instructions != NULL);
    assert(xanList_length(loop->instructions) > 0);
    assert(xanList_find(loop->instructions, loop->header) != NULL);

    /* Allocate the list			*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fill up the list			*/
    pred = IRMETHOD_getPredecessorInstruction(loop->method, loop->header, NULL);
    while (pred != NULL) {
        if (xanList_find(loop->instructions, pred) != NULL) {
            xanList_append(l, pred);
        }
        pred = IRMETHOD_getPredecessorInstruction(loop->method, loop->header, pred);
    }

    /* Return				*/
    return l;
}

induction_variable_t * LOOPS_getInductionVariable (loop_t *loop, IR_ITEM_VALUE varID) {
    XanListItem     *item;

    /* Assertions				*/
    assert(loop != NULL);

    /* Check if there are induction         *
     * variables within the method given as	*
     * input				*/
    if (loop->inductionVariables == NULL) {
        return NULL;
    }

    /* Check if the variable is inductive	*/
    item = xanList_first(loop->inductionVariables);
    while (item != NULL) {
        induction_variable_t    *var;
        var = item->data;
        assert(var != NULL);
        if (var->ID == varID) {
            return var;
        }
        item = item->next;
    }

    /* Return				*/
    return NULL;
}

JITBOOLEAN LOOPS_isAnInvariantInstruction (loop_t *loop, ir_instruction_t *inst) {

    /* Assertions				*/
    assert(loop != NULL);

    return (xanList_find(loop->invariants, inst) != NULL);
}

JITBOOLEAN LOOPS_isAnInvariantVariable (loop_t *loop, IR_ITEM_VALUE varID) {
    XanListItem	*item;

    /* Assertions				*/
    assert(loop != NULL);

    /* Check the invariants			*/
    item = xanList_first(loop->instructions);
    while (item != NULL) {
        ir_instruction_t	*inst;

        /* Fetch the current instruction	*/
        inst = item->data;
        assert(inst != NULL);

        /* Check if the instruction defines the	*
         * variable. In this case, if the 	*
         * instruction is not an invariant, then*
         * the variable is not invariant as well*/
        if (	(IRMETHOD_doesInstructionDefineVariable(loop->method, inst, varID))	&&
                (xanList_find(loop->invariants, inst) == NULL)		) {
            return JITFALSE;
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Return				*/
    return JITTRUE;
}

JITBOOLEAN LOOPS_hasASharedParentInductionVariable (loop_t *loop, IR_ITEM_VALUE varID1, IR_ITEM_VALUE varID2) {
    induction_variable_t    *var1;
    induction_variable_t    *var2;
    induction_variable_t    *tmpVar1;
    induction_variable_t    *tmpVar2;

    /* Assertions				*/
    assert(loop != NULL);

    /* Check the trivial case		*/
    if (varID1 == varID2) {
        return JITTRUE;
    }

    /* Fetch the induction variables	*/
    var1 = LOOPS_getInductionVariable(loop, varID1);
    var2 = LOOPS_getInductionVariable(loop, varID2);
    if (	(var1 == NULL)		||
            (var2 == NULL)		) {
        return JITFALSE;
    }

    /* Check if they have a shared parent	*/
    tmpVar1	= var1;
    do {
        tmpVar2	= var2;
        do {
            assert(tmpVar1 != NULL);
            assert(tmpVar2 != NULL);
            if (tmpVar1->i == tmpVar2->i) {
                return JITTRUE;
            }
            tmpVar2	= LOOPS_getInductionVariable(loop, tmpVar2->i);
            assert(tmpVar2 != NULL);
        } while ((tmpVar2->i) != (tmpVar2->ID));
        tmpVar1	= LOOPS_getInductionVariable(loop, tmpVar1->i);
        assert(tmpVar1 != NULL);
    } while ((tmpVar1->i) != (tmpVar1->ID));

    /* Return				*/
    return JITFALSE;
}

JITBOOLEAN LOOPS_isAnInductionVariable (loop_t *loop, IR_ITEM_VALUE varID) {
    induction_variable_t    *var;

    /* Assertions				*/
    assert(loop != NULL);

    /* Fetch the induction variable		*/
    var = LOOPS_getInductionVariable(loop, varID);

    /* Return				*/
    return var != NULL;
}

JITBOOLEAN LOOPS_isAWhileLoop (ir_optimizer_t *irOptimizer, loop_t *loop) {
    XanList         *backedges;
    XanListItem     *item;
    JITBOOLEAN whileLoop;

    /* Assertions				*/
    assert(irOptimizer != NULL);
    assert(loop != NULL);
    assert(loop->instructions != NULL);

    /* Compute the pre-dominance		*/
    IROPTIMIZER_callMethodOptimization(irOptimizer, loop->method, PRE_DOMINATOR_COMPUTER);

    /* Check if the header of the loop pre	*
     * dominates every instruction of the	*
     * body					*/
    item = xanList_first(loop->instructions);
    while (item != NULL) {
        ir_instruction_t        *inst;

        /* Fetch the instruction		*/
        inst = item->data;
        assert(inst != NULL);

        /* Check if the header pre-dominates	*
         * the current instruction		*/
        if (!IRMETHOD_isInstructionAPredominator(loop->method, loop->header, inst)) {
            return JITFALSE;
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Fetch the backedges			*/
    backedges = LOOPS_getBackedges(loop);
    assert(backedges != NULL);

    /* Check the backedges			*/
    whileLoop = JITTRUE;
    item = xanList_first(backedges);
    while (item != NULL) {
        ir_instruction_t        *back;

        /* Fetch the current backedge		*/
        back = item->data;
        assert(back != NULL);

        /* Check if it is an exit point and it	*
         * is a conditional branch		*/
        if (LOOPS_isAnExitInstruction(loop, back)) {
            assert(back->type != IRBRANCH);

            /* The loop is not a while loop		*/
            whileLoop = JITFALSE;
            break;
        }

        /* Fetch the next element from the list	*/
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(backedges);

    /* Return				*/
    return whileLoop;
}

JITBOOLEAN LOOPS_isAnExitInstruction (loop_t *loop, ir_instruction_t *inst) {

    /* Assertions.
     */
    assert(loop != NULL);
    assert(loop->instructions != NULL);
    assert(inst != NULL);

    /* Check if the instruction belongs to the loop.
     */
    if (xanList_find(loop->instructions, inst) == NULL) {
        return JITFALSE;
    }

    /* Check the successors of the instruction.
     */
    return (xanList_find(loop->exits, inst) != NULL);
}

loop_t * LOOPS_getParentLoopWithinMethod (XanHashTable *loops, loop_t *loop) {
    XanHashTableItem        *item;
    loop_t                  *parent;

    /* Assertions				*/
    assert(loops != NULL);
    assert(loop != NULL);

    /* Search for loops with loop as a      *
     * subloop				*/
    parent = NULL;
    item = xanHashTable_first(loops);
    while (item != NULL) {
        loop_t  *currentLoop;
        currentLoop = (loop_t *) item->element;
        assert(currentLoop != NULL);
        if (    (currentLoop->subLoops != NULL)                                         &&
                (currentLoop->method == loop->method)                                   &&
                (xanList_find(currentLoop->subLoops, loop) != NULL)      ) {
            parent = currentLoop;
            break;
        }
        item = xanHashTable_next(loops, item);
    }

    /* Return				*/
    return parent;
}

void LOOPS_sortInstructions (ir_method_t *method, XanList *instructions, ir_instruction_t *header) {
    JITUINT32       *table;
    JITUINT32       *tableTemp;
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    JITUINT32 delta;
    JITBOOLEAN swapped;
    XanListItem     *item;

    /* Assertions.
     */
    assert(method != NULL);
    assert(instructions != NULL);

    /* Check the length of the list.
     */
    if (xanList_length(instructions) == 0) {
        return ;
    }

    /* Check the list.
     */
#ifdef DEBUG
    item	= xanList_first(instructions);
    while (item != NULL) {
        XanListItem	*item2;
        item2	= item->next;
        while (item2 != NULL) {
            assert(item2->data != item->data);
            item2	= item2->next;
        }
        item	= item->next;
    }
#endif

    /* Compute the table of IDs.
     */
    instructionsNumber = xanList_length(instructions);
    table = allocFunction(sizeof(JITUINT32) * instructionsNumber);
    tableTemp = allocFunction(sizeof(JITUINT32) * instructionsNumber);
    count = 0;
    item = xanList_first(instructions);
    while (item != NULL) {
        ir_instruction_t        *inst;

        /* Fetch the instruction		*/
        inst = item->data;
        assert(inst != NULL);
        assert(IRMETHOD_doesInstructionBelongToMethod(method, inst));
        assert(inst->ID < IRMETHOD_getInstructionsNumber(method));

        /* Add the instruction ID to the table	*/
        table[count] = inst->ID;
        count++;

        /* Fetch the next element from the list	*/
        item = item->next;
    }
#ifdef DEBUG
    for (count = 0; count < instructionsNumber; count++) {
        assert(table[count] < IRMETHOD_getInstructionsNumber(method));
    }
#endif

    /* Order the table			*/
    do {
        swapped = JITFALSE;
        for (count = 0; count < (instructionsNumber - 1); count++) {
            if (table[count] > table[count + 1]) {
                JITUINT32 temp;
                temp = table[count];
                table[count] = table[count + 1];
                table[count + 1] = temp;
                swapped = JITTRUE;
            }
        }
    } while (swapped);

    /* Check the order			*/
#ifdef DEBUG
    for (count = 0; count < (instructionsNumber - 1); count++) {
        assert(table[count] < IRMETHOD_getInstructionsNumber(method));
        assert(table[count] <= table[count + 1]);
    }
#endif

    /* Check if we need to consider the header.
     */
    if (header != NULL) {

        /* Fetch the header element		*/
        for (count = 0; count < instructionsNumber; count++) {
            if (table[count] == header->ID) {
                break;
            }
        }
        assert(table[count] == header->ID);

        /* Move everything there is before the	*
         * header, at the bottom		*/
        delta = instructionsNumber - count;
        assert(delta <= instructionsNumber);
        if (delta > 0) {
            memmove(&(tableTemp[0]), &(table[0]), sizeof(JITUINT32) * count);
            memmove(&(table[0]), &(table[count]), sizeof(JITUINT32) * delta);
            memmove(&(table[delta]), &(tableTemp[0]), sizeof(JITUINT32) * count);
        }
    }

    /* Add the ordered instructions to the	*
     * list					*/
    xanList_emptyOutList(instructions);
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, table[count]);
        assert(inst != NULL);
        assert(xanList_find(instructions, inst) == NULL);
        xanList_append(instructions, inst);
    }

    /* Free the memory			*/
    freeFunction(table);
    freeFunction(tableTemp);

    /* Return				*/
    return;
}

XanList * LOOPS_getMethods (XanList *loops) {
    XanList         *l;
    XanListItem     *item;

    /* Assertions				*/
    assert(loops != NULL);

    /* Allocate the list of methods		*/
    l = xanList_new(allocFunction, freeFunction, NULL);
    assert(l != NULL);

    /* Fill up the list			*/
    item = xanList_first(loops);
    while (item != NULL) {
        loop_t  *loop;
        loop = item->data;
        assert(loop != NULL);
        assert(loop->method != NULL);
        if (xanList_find(l, loop->method) == NULL) {
            xanList_append(l, loop->method);
        }
        assert(xanList_find(l, loop->method) != NULL);
        item = item->next;
    }

    /* Return				*/
    return l;
}

JITUINT32 LOOPS_getInstructionLoopNesting (loop_t *loop, ir_instruction_t *inst) {
    XanList *alreadyConsidered;
    JITUINT32 level;

    /* Allocate the necessary memories		*/
    alreadyConsidered = xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the nesting level			*/
    xanList_append(alreadyConsidered, loop);
    level = internal_LOOPS_getInstructionLoopNesting(loop, inst, alreadyConsidered);

    /* Free the memory				*/
    xanList_destroyList(alreadyConsidered);

    /* Return					*/
    return level;
}

static inline JITUINT32 internal_LOOPS_getInstructionLoopNesting (loop_t *loop, ir_instruction_t *inst, XanList *alreadyConsidered) {
    JITUINT32 level;

    /* Assertions					*/
    assert(loop != NULL);
    assert(inst != NULL);
    assert(alreadyConsidered != NULL);

    /* Check if the loop belongs to the subloops	*/
    level = 0;
    if (loop->subLoops != NULL) {
        XanListItem     *item;
        item = xanList_first(loop->subLoops);
        while ((item != NULL) && (level == 0)) {
            loop_t  *subloop;
            subloop = item->data;
            assert(subloop != NULL);
            if (xanList_find(alreadyConsidered, subloop) == NULL) {
                xanList_append(alreadyConsidered, subloop);
                level = internal_LOOPS_getInstructionLoopNesting(subloop, inst, alreadyConsidered);
            }
            item = item->next;
        }
    }
    if (level == 0) {

        /* The instruction does not belong to	*
         * any sub-loop				*/
        if (    (loop->instructions != NULL)                                    &&
                (xanList_find(loop->instructions, inst) != NULL)    ) {
            level = 1;
        }
    } else {

        /* The instruction belongs to a sub	*
         * loop					*/
        level++;
    }

    /* Return					*/
    return level;
}

void LOOPS_addProgramSubLoopsMayBeAtNestingLevels (XanList *parentLoops, JITUINT32 fromNestingLevel, JITUINT32 toNestingLevel, XanList *subloops) {
    JITUINT32 count;

    /* Assertions			*/
    assert(parentLoops != NULL);
    assert(fromNestingLevel <= toNestingLevel);
    assert(subloops != NULL);

    /* Add the subloops		*/
    for (count=fromNestingLevel; count <= toNestingLevel; count++) {
        XanList *tempList;
        XanListItem *item;
        tempList = LOOPS_getProgramSubLoopsMayBeAtNestingLevelFromParentLoops(parentLoops, count);
        assert(tempList != NULL);
        item = xanList_first(tempList);
        while (item != NULL) {
            loop_t *loop;
            loop = item->data;
            assert(loop != NULL);
            if (xanList_find(subloops, loop) == NULL) {
                xanList_append(subloops, loop);
            }
            item = item->next;
        }
    }

    /* Return			*/
    return ;
}

XanList * LOOPS_getProgramSubLoopsMayBeAtNestingLevel (XanHashTable *loops, JITUINT32 nestingLevel) {
    XanList *outermostLoops;
    XanList *subloops;

    /* Fetch the outermost loops	*/
    outermostLoops = LOOPS_getOutermostProgramLoops(loops);
    assert(outermostLoops != NULL);

    /* Fetch the subloops		*/
    subloops = LOOPS_getProgramSubLoopsMayBeAtNestingLevelFromParentLoops(outermostLoops, nestingLevel);
    assert(subloops != NULL);

    /* Free the memory		*/
    xanList_destroyList(outermostLoops);

    /* Return			*/
    return subloops;
}

XanList * LOOPS_getProgramSubLoopsMayBeAtNestingLevelFromParentLoops (XanList *loops, JITUINT32 nestingLevel) {
    XanList *l;
    XanList *toDelete;
    XanListItem *item;

    /* Assertions			*/
    assert(loops != NULL);

    /* Fetch the subloops		*/
    l = LOOPS_getSubLoopsMayBeAtNestingLevel(loops, nestingLevel);
    assert(l != NULL);

    /* Remove every loop that does 	*
     * not belong to the program	*/
    toDelete = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(l);
    while (item != NULL) {
        loop_t *loop;
        loop = item->data;
        assert(loop != NULL);
        if (IRPROGRAM_doesMethodBelongToALibrary(loop->method)) {
            xanList_append(toDelete, loop);
        }
        item = item->next;
    }
    item = xanList_first(toDelete);
    while (item != NULL) {
        xanList_removeElement(l, item->data, JITTRUE);
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(toDelete);

    /* Return			*/
    return l;
}

XanList * LOOPS_getSubLoopsMayBeAtNestingLevel (XanList *loops, JITUINT32 nestingLevel) {
    XanList *l;
    XanListItem *item;
    XanList *tempList;

    /* Assertions			*/
    assert(loops != NULL);

    /* Check the nesting level	*/
    if (nestingLevel == 0) {
        return xanList_cloneList(loops);
    }

    /* Fetch the subloops		*/
    tempList = xanList_new(allocFunction, freeFunction, NULL);
    l = xanList_new(allocFunction, freeFunction, NULL);
    item = xanList_first(loops);
    while (item != NULL) {
        XanListItem *item2;
        loop_t *parentLoop;
        parentLoop = item->data;
        assert(parentLoop != NULL);
        item2 = xanList_first(parentLoop->subLoops);
        while (item2 != NULL) {
            loop_t *subloop;
            subloop = item2->data;
            assert(subloop != NULL);
            internal_append_subloops(subloop, nestingLevel, l, tempList);
            item2 = item2->next;
        }
        item = item->next;
    }

    /* Free the memory		*/
    xanList_destroyList(tempList);

    /* Return			*/
    return l;
}

static inline void internal_append_subloops (loop_t *loop, JITUINT32 nestingLevel, XanList *subloops, XanList *alreadyConsidered) {
    XanListItem *item;

    /* Assertions			*/
    assert(loop != NULL);
    assert(loop->subLoops != NULL);
    assert(subloops != NULL);
    assert(alreadyConsidered != NULL);

    /* Check the nesting level	*/
    if (nestingLevel == 0) {
        return ;
    }

    /* Append the subloops		*/
    item = xanList_first(loop->subLoops);
    while (item != NULL) {
        loop_t *subloop;
        subloop = item->data;
        assert(subloop != NULL);
        if (xanList_find(alreadyConsidered, subloop) == NULL) {
            xanList_insert(alreadyConsidered, subloop);
            internal_append_subloops(subloop, nestingLevel - 1, subloops, alreadyConsidered);
            xanList_removeElement(alreadyConsidered, subloop, JITTRUE);
        }
        item = item->next;
    }

    /* Check the current loop	*/
    if (	(nestingLevel == 1)				&&
            (xanList_find(subloops, loop) == NULL)	) {
        xanList_append(subloops, loop);
    }

    /* Return			*/
    return ;
}

static inline JITINT32 internal_are_loops_equal (void *key1, void *key2) {
    loop_t *loop1;
    loop_t *loop2;

    /* Assertions				*/
    assert(key1 != NULL);
    assert(key2 != NULL);

    /* Fetch the loops			*/
    loop1 = (loop_t *)key1;
    loop2 = (loop_t *)key2;

    /* Check the trivial case		*/
    if (loop1 == loop2) {

        /* Return				*/
        return JITTRUE;
    }

    /* Check the loops			*/
    if (    (loop1->method == loop2->method)      	&&
            (loop1->header == loop2->header)	) {

        /* Return				*/
        return JITTRUE;
    }

    /* Return				*/
    return JITFALSE;
}

static inline JITUINT32 internal_loops_hash (void *element) {
    loop_t *loop;

    /* Assertions				*/
    assert(element != NULL);

    /* Fetch the loop			*/
    loop = element;

    /* Return				*/
    return (JITUINT32) (JITNUINT) loop->header;
}

static inline JITINT8 * internal_get_loop_name (loop_t *loop) {
    JITINT8                 *methodName;
    JITINT8                 *newName;
    JITUINT32		stringLen;

    /* Assertions				*/
    assert(loop != NULL);
    assert(loop->method != NULL);
    assert(loop->header != NULL);

    /* Fetch the name of the original method*/
    methodName = IRMETHOD_getSignatureInString(loop->method);
    assert(methodName != NULL);

    /* Make the name of the new method	*/
    stringLen	= STRLEN(methodName);
    newName 	= allocFunction(stringLen + 100);
    SNPRINTF(newName, stringLen + 100, "%s_%u", methodName, loop->header->ID);

    return newName;
}

XanGraphNode * LOOPS_getLoopInvocation (XanGraph *g, loop_t *loop, JITUINT32 startInvocation) {
    XanGraphNode			*n;
    XanHashTableItem		*item;

    /* Assertions				*/
    assert(g != NULL);
    assert(loop != NULL);

    /* Search the node			*/
    n	= NULL;
    item	= xanHashTable_first(g->nodes);
    while (item != NULL) {
        XanGraphNode			*tempNode;
        loop_invocations_information_t	*inv;

        /* Fetch the node			*/
        tempNode	= item->element;
        assert(tempNode != NULL);
        inv		= tempNode->data;
        assert(inv != NULL);
        assert(inv->loop != NULL);

        /* Check the node			*/
        if (	(inv->loop->loop == loop)			&&
                (inv->minInvocationsNumber == startInvocation)	) {
            n	= tempNode;
            break;
        }

        /* Fetch the next element		*/
        item	= xanHashTable_next(g->nodes, item);
    }

    /* Return				*/
    return n;
}
