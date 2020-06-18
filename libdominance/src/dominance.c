/*
 * Copyright (C) 2009  Campanoni Simone
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
#include <bitset.h>
#include <dominance.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

/**
 * @brief Print a bitset on the stout
 */
void print_bitset (bitset* b) {
    JITINT32 i;

    for (i = 0; i < b->length; i++) {
        fprintf(stderr, "%d", bitset_test_bit(b, i));
    }
    fprintf(stderr, "\n");
}

/**
 * @brief Returns the numbero of bit that are setted in the bitset passed as parameter
 */
JITINT32 countSet (bitset* b) {
    JITINT32 i, j;

    /* Assertions		*/
    assert(b != NULL);

    j	= 0;

    if (sizeof(size_t) == 4) {
        JITUINT32	value;
        JITUINT32	words;

        words	= (b->length + 31)/32;
        assert(words > 0);
        for (i=0; i < words; i++) {
            value	= b->data[i];
            value 	= value - ((value >> 1) & 0x55555555);
            value 	= (value & 0x33333333) + ((value >> 2) & 0x33333333);
            j	+= ((((value + (value >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24);
        }

    } else {

        for (i = 0; i < (b->length); i++) {
            if (bitset_test_bit(b, i)) {
                j++;
            }
        }
    }

    /* Return		*/
    return j;
}

/**
 * @brief Returns the position of the element passed as argument in the nodes array, -1 if not found
 */
JITINT32 indexOf (void* el, void** nodes, JITINT32 numNodes) {
    JITINT32 i = 0;

    /* Assertions			*/
    assert(nodes != NULL);

    for (i = 0; i < numNodes; i++) {
        if (nodes[i] == NULL) {
            if (el == NULL) {
                return i;
            }
        } else if (nodes[i] == el) {
            return i;
        }
    }

    /* Return			*/
    return -1;
}

/**
 * @brief function to calcutate the immediate dominance relation between given nodes of a graph
 *
 * @param elems                 array of pointers to the elements of the graph
 * @param numElems		number of elements in the array passed as first parameter
 * @param headNodePos		index of the element in the array to be considered as starting node of the graph
 * @param getPredecessors	function that takes two parameters: it must return a predecessor of the first argument
 *				that is after the second passed parameter or NULL if no other predecessor is found
 * @param getSuccessors		function that takes two parameters: it must return a successor of the first argument
 *				that is after the second passed parameter or NULL if no other successor is found
 */
XanNode* calculateDominanceTree (
    void** elems,
    JITINT32 numElems,
    JITINT32 headNodePos,
    void* (*getPredecessors)(void* elem, void* pred),
    void* (*getSuccessors)(void* elem, void* succ)
) {
    XanNode *root;
    bitset** dominators;   // Set of dominators for each element
    bitset* visited;                // to know if an element has already been visited
    bitset* temp;                   // temporary value of dominators for current node
    XanList* worklist;              // list of nodes to visit
    XanListItem* curr_item;         // current item considered of the worklist
    void* curr;                     // current node
    void* prev;                     // node that is a predecessor of curr, used to cycle between them
    void* next;                     // node that is a successor of curr, used to cycle between them
    JITINT32 i, j, curr_pos;        // counters
    JITINT32 max, instrIDmaxDeep;   // counters to build the immediate dominance relation
    JITINT32 changed;               // to know when a fixed point is reached in the dominance sets
    XanList* inst_ordered;          // list of nodes ordered to calculate idom
    XanListItem* it;                // item from inst_ordered
    nodeDeep* nd;                   // struct to build idom
    JITINT32 instrID;               // save element ID for current instruction
    JITINT32 maximum = 0;           // max # dominators founded during computation
    JITUINT32	*bitsSet;

    // Init internal vars
    dominators = allocFunction(sizeof(bitset *) * numElems);
    for (i = 0; i < numElems; i++) {
        // Set of dominators for each element
        dominators[i] = bitset_alloc(numElems);
        bitset_set_all(dominators[i]);
    }

    worklist = xanList_new(allocFunction, freeFunction, NULL);
    visited = bitset_alloc(numElems);
    bitset_clear_all(visited);
    temp = bitset_alloc(numElems);
    changed = 1;

    // Start dominators sets calculus
    while (changed) { //until you don't reach a fixed point
        changed = 0;
        curr = elems[headNodePos];
        xanList_append(worklist, curr); // append first element of the graph to the worklist
        bitset_clear_all(visited);
        do {
            curr_item = xanList_first(worklist);
            curr = curr_item->data;
            curr_pos = indexOf(curr, elems, numElems);
            bitset_set_bit(visited, curr_pos);

            if (curr_pos == headNodePos) {
                bitset_clear_all(dominators[curr_pos]);
                bitset_set_bit(dominators[curr_pos], curr_pos);
            }
            // mark elem as visited
            else {

                // cycle among curr element predecessors
                // dominators[n] = intersect(dominators[p]) for all p in predecessors(n)
                prev = getPredecessors(curr, NULL);
                if (prev != NULL) {
                    bitset_clone(temp, dominators[indexOf(prev, elems, numElems)]);
                    prev = getPredecessors(curr, prev);
                    while (prev != NULL) {
                        bitset_intersect(temp, dominators[indexOf(prev, elems, numElems)]);
                        prev = getPredecessors(curr, prev);
                    }
                }
                // Set dominance riflexivity
                // dominators[n] = dominators[n] U {n}
                bitset_set_bit(temp, curr_pos);
                // Check if something is changed
                if (!bitset_equal(temp, dominators[curr_pos])) {
                    changed = 1;
                    bitset_clone(dominators[curr_pos], temp);
                    /*                                         fprintf(stderr, "Dominators of pos %d: ", curr_pos); */
                    /*                                         print_bitset(temp); */
                }
            }

            // Put in the worklist every successor of the current element if they have not been visited yet
            next = getSuccessors(curr, NULL);
            while (next != NULL) {
                if (xanList_find(worklist, next) == NULL) {
                    if (!bitset_test_bit(visited, indexOf(next, elems, numElems))) {
                        xanList_append(worklist, next);
                    }
                }
                next = getSuccessors(curr, next);
            }

            // Remove current element
            xanList_removeElement(worklist, curr, JITTRUE);

        } while (xanList_length(worklist) > 0); // continue if there are elements in the worklist
    }

    // Now the dominators have been calculated, we can procede calculating idoms

    //1: NODE DEEP CREATION
    nd = (nodeDeep*) allocFunction((numElems) * sizeof(nodeDeep));
    for (i = 0; i < numElems; i++) {
        nd[i].node = NULL;
        nd[i].level = -1;
    }

    //2: ALLOCATION OF  XAN NODES
    //for each element i alloc a new xanNode and save in the data field a pointer to element i
    for (i = 0; i < numElems; i++) {
        nd[i].node = xanNode_new(allocFunction, freeFunction, NULL);
        (nd[i].node)->setData(nd[i].node, elems[i]);
    }

    //3.1: DELETE SELF-DOMINANCE RELATIONSHIP
    for (i = 0; i < numElems; i++) {
        bitset_clear_bit(dominators[i], i);
    }

    //3.2: create a list of elemsID ordered by #dominators in ascendent mode
    bitsSet		= allocFunction(sizeof(JITUINT32) * (numElems + 1));
    inst_ordered 	= xanList_new(allocFunction, freeFunction, NULL);
    for (i = 1; i < numElems; i++) {
        JITINT32	numberOfBitsSet;

        numberOfBitsSet	= countSet(dominators[i]);
        bitsSet[i]	= numberOfBitsSet;

        //if list is empty (insert first element)
        if (i == 1) {
            assert(xanList_length(inst_ordered) == 0);
            xanList_append(inst_ordered, (JITINT32 *) (JITNUINT)i);
            maximum 	= numberOfBitsSet;
            continue ;
        }

        //if #dominators of elem i is greater than maximum append element in the list and update maximum
        if (numberOfBitsSet >= maximum) {
            xanList_append(inst_ordered, (JITINT32*) (JITNUINT)i);
            maximum	= numberOfBitsSet;
            continue ;
        }

        // scan list to insert element
        it = xanList_first(inst_ordered);
        while (it != NULL) {
            instrID = (JITINT32) (JITNUINT)(it->data);
            if (bitsSet[instrID] > numberOfBitsSet) {
                xanList_insertBefore(inst_ordered, it, (JITINT32*) (JITNUINT)i);
                break;
            }
            it = it->next;
        }
    }
    freeMemory(bitsSet);

    //4: Link node in the dominator tree scanning list inst_ordered
    nd[0].level = 0;    //init level of root as 0

    /* For each instruction (instrID) find dominator at     *
    * maximum deep level in the preDominator tree          */
    it = xanList_first(inst_ordered);
    while (it != NULL) {
        max = 0;                                //deepest level in the preDominanceTree founded
        instrIDmaxDeep = 0;                     //position in the nodeDeep array
        instrID = (JITINT32) (JITNUINT)(it->data);
        for (j = 0; j < numElems; j++) {

            if (bitset_test_bit(dominators[instrID], j)) {
                if (nd[j].level >= max) {
                    max = nd[j].level;
                    instrIDmaxDeep = j;
                }
            }
        }
        //add an edge from instrIDmaxDeep to instrID settings level field in the nodeDeep structure
        (nd[instrIDmaxDeep].node)->addChildren(nd[instrIDmaxDeep].node, nd[instrID].node);
        nd[instrID].level = max + 1;
        it = it->next;
    }

    /* Fetch the root		*/
    root = nd[0].node;
    assert(root != NULL);

    /* Check the tree.
     */
#ifdef DEBUG
    {
        XanList *tempList;
        tempList	= root->toPreOrderList(root);
        assert(xanList_length(tempList) == numElems);
        xanList_destroyList(tempList);
    }
#endif

    /* Free the memory		*/
    xanList_destroyList(inst_ordered);
    xanList_destroyList(worklist);
    freeFunction(nd);
    bitset_free(visited);
    bitset_free(temp);
    for (i = 0; i < numElems; i++) {
        bitset_free(dominators[i]);
    }
    freeFunction(dominators);

    /* Return			*/
    return root;
}


/**
 * Determine whether 'elem' dominates 'dominated' within 'tree'.
 *
 * Taken from libiljitir/src/ir_method.c
 **/
JITBOOLEAN
isADominator(void *elem, void *dominated, XanNode *tree) {
    XanNode         *node;
    XanList         *list;

    /* Assertions					*/
    assert(elem != NULL);
    assert(dominated != NULL);
    assert(tree != NULL);

    if (elem == dominated) {

        /* Each element dominates himself		*/
        return JITTRUE;
    }

    /* Search the `elem` inside the dominator tree  */
    node = tree->find(tree, elem);
    assert(node != NULL);

    /* Search in the sub-tree from `elem` for	*
     * `dominated`				*/
    list = node->toPreOrderList(node);
    assert(list != NULL);
    if (xanList_find(list, dominated) != NULL) {

        /* Free the memory			*/
        xanList_destroyList(list);
        return JITTRUE;
    }
    xanList_destroyList(list);

    /* Return					*/
    return JITFALSE;
}


/**
 * Get the elements in 'tree' that dominate 'dominated'.
 **/
XanList *
getElemsDominatingElem(void *dominated, XanNode *tree) {
    XanNode *node;
    XanList *list;

    assert(dominated);
    assert(tree);

    /* Find dominated. */
    node = tree->find(tree, dominated);
    assert(node);

    /* Add all parents to the list. */
    list = xanList_new(allocFunction, freeFunction, NULL);
    while (node) {
        xanList_append(list, node->data);
        node = node->parent;
    }

    /* Return this list. */
    return list;
}
