/*
 * Copyright (C) 2014  Campanoni Simone
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
// End

static inline JITBOOLEAN internal_canHaveOnlyIncomingVarSource (XanGraph *vars, XanGraph *incomingVars, IR_ITEM_VALUE varID, IR_ITEM_VALUE incomingVarID, XanHashTable *alreadyChecked, ir_method_t *m);
static inline void internal_dumpEdges (XanGraph *vars, XanGraph *incomingVars);
static inline JITBOOLEAN internal_areVarsConnected (XanGraph *vars, IR_ITEM_VALUE varID1, IR_ITEM_VALUE varID2, XanHashTable *alreadyChecked);

void remove_cyclic_moves (ir_method_t *m){
    XanHashTable        *alreadyChecked;
    XanGraph            *vars;
    XanGraph            *incomingVars;
    XanList             *toRemove;
    XanList             *toDelete;
    XanList             *moves;
    XanListItem         *item;
    XanHashTableItem    *hashItem;

    /* Assertions.
     */
    assert(m != NULL);
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: Start\n");

    /* Allocate the memory.
     */
    vars            = xanGraph_new(allocFunction, freeFunction, dynamicReallocFunction, NULL);
    incomingVars    = xanGraph_new(allocFunction, freeFunction, dynamicReallocFunction, NULL);
    toRemove        = xanList_new(allocFunction, freeFunction, NULL);
    toDelete        = xanList_new(allocFunction, freeFunction, NULL);
    alreadyChecked  = xanHashTable_new(11, 0, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

    /* Analyze the method.
     */
    IROPTIMIZER_callMethodOptimization(NULL, m, ESCAPES_ANALYZER);

    /* Fetch all move instructions.
     */
    moves   = IRMETHOD_getInstructionsOfType(m, IRMOVE);
    assert(moves != NULL);

    /* Compute the list of variables defined only by move instructions.
     */
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves:  Compute variables defined just by move instructions\n");
    item    = xanList_first(moves);
    while (item != NULL){
        ir_instruction_t    *move;
        IR_ITEM_VALUE       varID;
        XanList             *defs;

        /* Fetch the move.
         */
        move    = item->data;

        /* Fetch the defined variable.
         */
        varID   = IRMETHOD_getInstructionResultValue(move);

        /* Fetch all definitions of the current variable.
         */
        defs    = IRMETHOD_getVariableDefinitions(m, varID);

        /* Check if all definitions are move.
         */
        if (    (xanGraph_getNode(vars, (void *)(JITNUINT)(varID + 1)) == NULL) &&
                (xanList_containsAllElements(defs, moves))                      ){
            XanListItem *item2;

            /* Check that all definitions do not use constants.
             */
            item2   = xanList_first(defs);
            while (item2 != NULL){
                ir_instruction_t    *definedMove;
                ir_item_t           *sourceItem;
                definedMove = item2->data;
                sourceItem  = IRMETHOD_getInstructionParameter1(definedMove);
                if (!IRDATA_isAVariable(sourceItem)){
                    break ;
                }
                item2   = item2->next;
            }
            if (item2 == NULL){
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves:      Var %llu\n", varID);
                xanGraph_addANewNode(vars, (void *)(JITNUINT)(varID + 1));
            }
        }

        /* Free the memory.
         */
        xanList_destroyList(defs);

        /* Fetch the next element.
         */
        item    = item->next;
    }

    /* Add the dependences among variables.
     */
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves:  Compute dependences among variables\n");
    hashItem    = xanHashTable_first(vars->nodes);
    while (hashItem != NULL){
        XanGraphNode    *n;
        XanList         *defs;
        XanListItem     *item;
        IR_ITEM_VALUE   varID;
        void            *pVarID;

        /* Fetch the variable.
         */
        n           = hashItem->element;
        assert(n != NULL);
        pVarID      = n->data;
        varID       = (IR_ITEM_VALUE)(JITNUINT)n->data;
        assert(varID > 0);
        varID--;
        PDEBUG("COPY PROPAGATION: remove_cyclic_moves:      Var %llu\n", varID);

        /* Fetch all definitions of the current variable.
         */
        defs    = IRMETHOD_getVariableDefinitions(m, varID);
        assert(defs != NULL);

        /* Add the edges.
         */
        item    = xanList_first(defs);
        while (item != NULL){
            ir_instruction_t    *i;
            ir_item_t           *sourceItem;
            XanGraphNode        *sourceNode;

            /* Fetch the move.
             */
            i               = item->data;
            assert(i != NULL);
            assert(i->type == IRMOVE);

            /* Check if the source is a variable.
             */
            sourceItem      = IRMETHOD_getInstructionParameter1(i);
            if (IRDATA_isAVariable(sourceItem)){
                IR_ITEM_VALUE       sourceVarID;
                void                *pSourceVarID;
                sourceVarID     = IRMETHOD_getInstructionParameter1Value(i);
                pSourceVarID    = (void *)(JITNUINT)(sourceVarID + 1);

                /* Add the edge.
                 */
                sourceNode      = xanGraph_getNode(vars, pSourceVarID);
                if (sourceNode != NULL){
                    if (xanGraph_getDirectedEdge(sourceNode, n) == NULL){
                        xanGraph_addDirectedEdge(vars, sourceNode, n, NULL);
                        PDEBUG("COPY PROPAGATION: remove_cyclic_moves:          Edge in move graph: %llu -> %llu\n", sourceVarID, varID);
                    }

                } else {
                    XanGraphNode    *incomingVarsSourceNode;
                    XanGraphNode    *incomingVarsDestNode;
                    incomingVarsDestNode    = xanGraph_addANewNodeIfNotExist(incomingVars, pVarID);
                    assert(incomingVarsDestNode != NULL);
                    incomingVarsSourceNode    = xanGraph_addANewNodeIfNotExist(incomingVars, pSourceVarID);
                    assert(incomingVarsSourceNode != NULL);
                    if (xanGraph_getDirectedEdge(incomingVarsSourceNode, incomingVarsDestNode) == NULL){
                        xanGraph_addDirectedEdge(incomingVars, incomingVarsSourceNode, incomingVarsDestNode, NULL);
                        PDEBUG("COPY PROPAGATION: remove_cyclic_moves:          Edge in incoming variables graph: %llu -> %llu\n", sourceVarID, varID);
                        //internal_dumpEdges(vars, incomingVars);
                    }
                    assert(xanGraph_getDirectedEdge(incomingVarsSourceNode, incomingVarsDestNode) != NULL);
                }
            }

            /* Fetch the next element.
             */
            item            = item->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(defs);

        /* Fetch the next element.
         */
        hashItem    = xanHashTable_next(vars->nodes, hashItem);
    }

    /* Identify the variables to substitute.
     */
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves:  Identify variables to substitute\n");
    hashItem    = xanHashTable_first(vars->nodes);
    while (hashItem != NULL){
        XanGraphNode    *n;
        XanGraphNode    *incomingVarNode;
        IR_ITEM_VALUE   varID;

        /* Fetch the variable.
         */
        n           = hashItem->element;
        assert(n != NULL);
        varID       = (IR_ITEM_VALUE)(JITNUINT)n->data;
        assert(varID > 0);
        varID--;

        /* Fetch the incoming variable of varID.
         * We handle only cases where there is only one incoming variable.
         */
        incomingVarNode     = xanGraph_getNode(incomingVars, n->data);
        if (    (incomingVarNode != NULL)                                               &&
                (xanHashTable_elementsInside(incomingVarNode->incomingEdges) == 1)      ){
            IR_ITEM_VALUE       currentIncomingVarID;
            JITBOOLEAN          safe;
            XanList             *defs;
            XanGraphNode        *currentIncomingVar;
            XanHashTableItem    *hashItem2;

            /* Fetch the incoming variable.
             */
            hashItem2               = xanHashTable_first(incomingVarNode->incomingEdges);
            currentIncomingVar      = hashItem2->elementID;
            assert(currentIncomingVar != NULL);
            currentIncomingVarID    = (IR_ITEM_VALUE)(JITNUINT)currentIncomingVar->data;
            assert(currentIncomingVarID > 0);
            currentIncomingVarID--;

            /* Check if the definition of the current incoming variable cannot change after it get used within move instructions of cyclic moves.
             */
            xanList_emptyOutList(toDelete);
            safe                    = JITFALSE;
            defs                    = IRMETHOD_getVariableDefinitions(m, currentIncomingVarID);
            item                    = xanList_first(defs);
            while (item != NULL){
                ir_instruction_t    *def;
                def     = item->data;
                if (def->type == IRMOVE){
                    ir_item_t   *par1;
                    par1    = IRMETHOD_getInstructionParameter1(def);
                    xanHashTable_emptyOutTable(alreadyChecked);
                    if (    IRDATA_isAVariable(par1)                                                &&
                            internal_areVarsConnected(vars, varID, (par1->value).v, alreadyChecked) ){
                        xanList_append(toDelete, def);
                    }
                }
                item    = item->next;
            }
            xanList_removeElements(defs, toDelete, JITTRUE);
            if (xanList_length(defs) <= 1){
                ir_instruction_t    *def;
                item                    = xanList_first(defs);
                if (item == NULL){
                    safe    = JITTRUE;

                } else {
                    def                     = item->data;
                    assert(def != NULL);
                    if (    (!IRMETHOD_isAMemoryInstruction(def))   &&
                            (!IRMETHOD_isACallInstruction(def))     ){
                        XanList *uses;
                        uses    = IRMETHOD_getVariablesUsedByInstruction(def);
                        if (xanList_length(uses) == 0){
                            safe    = JITTRUE;
                        }
                        xanList_destroyList(uses);
                    }
                }
            }

            /* Check if the current variable can be removed.
             */
            xanHashTable_emptyOutTable(alreadyChecked);
            if (    (safe)                                                                                                      &&
                    (!IRMETHOD_isAnEscapedVariable(m, varID))                                                                   &&
                    (!IRMETHOD_isTheVariableAMethodParameter(m, varID))                                                         &&
                    internal_canHaveOnlyIncomingVarSource(vars, incomingVars, varID, currentIncomingVarID, alreadyChecked, m)   ){
                assert(xanList_find(toRemove, (void *)(JITNUINT)(varID + 1)) == NULL);
                xanList_append(toRemove, (void *)(JITNUINT)(varID + 1));
            }

            /* Free the memory.
             */
            xanList_destroyList(defs);
        }

        /* Fetch the next element.
         */
        hashItem    = xanHashTable_next(vars->nodes, hashItem);
    }

    /* Remove useless variables.
     */
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves:  Remove variables to substitute\n");
    item    = xanList_first(toRemove);
    while (item != NULL){
        IR_ITEM_VALUE   varID;
        IR_ITEM_VALUE   currentIncomingVarID;
        XanGraphNode    *currentIncomingVar;
        XanGraphNode    *incomingVarNode;
        XanListItem     *item2;

        /* Fetch the variable to remove.
         */
        varID   = (IR_ITEM_VALUE)(JITNUINT)item->data;
        varID--;
        PDEBUG("COPY PROPAGATION: remove_cyclic_moves:      Var %llu\n", varID);

        /* Fetch the incoming variable to use instead of varID.
         */
        incomingVarNode     = xanGraph_getNode(incomingVars, item->data);
        assert(incomingVarNode != NULL);
        assert(xanHashTable_elementsInside(incomingVarNode->incomingEdges) == 1);
        hashItem    = xanHashTable_first(incomingVarNode->incomingEdges);
        currentIncomingVar      = hashItem->elementID;
        assert(currentIncomingVar != NULL);
        currentIncomingVarID    = (IR_ITEM_VALUE)(JITNUINT)currentIncomingVar->data;
        assert(currentIncomingVarID > 0);
        currentIncomingVarID--;

        /* Delete all instructions that define the variable.
         */
        xanList_emptyOutList(toDelete);
        for (JITUINT32 instID = 0; instID < IRMETHOD_getInstructionsNumber(m); instID++){
            ir_instruction_t    *i;
            i   = IRMETHOD_getInstructionAtPosition(m, instID);
            if (!IRMETHOD_doesInstructionDefineVariable(m, i, varID)){
                continue ;
            }
            xanList_append(toDelete, i);
        }
        item2   = xanList_first(toDelete);
        while (item2 != NULL){
            ir_instruction_t    *i;
            i       = item2->data;
            assert(i != NULL);
            IRMETHOD_deleteInstruction(m, i);
            item2   = item2->next;
        }

        /* Remove the useless variable.
         */
        PDEBUG("COPY PROPAGATION: remove_cyclic_moves:          Substitute var %llu with var %llu\n", varID, currentIncomingVarID);
        IRMETHOD_substituteVariable(m, varID, currentIncomingVarID);

        /* Fetch the next element.
         */
        item    = item->next;
    }

    /* Free the memory.
     */
    xanGraph_destroyGraph(vars);
    xanGraph_destroyGraph(incomingVars);
    xanList_destroyList(toRemove);
    xanList_destroyList(toDelete);
    xanHashTable_destroyTable(alreadyChecked);
    xanList_destroyList(moves);

    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: Exit\n");
    return ;
}

static inline JITBOOLEAN internal_canHaveOnlyIncomingVarSource (XanGraph *vars, XanGraph *incomingVars, IR_ITEM_VALUE varID, IR_ITEM_VALUE incomingVarID, XanHashTable *alreadyChecked, ir_method_t *m){
    XanGraphNode        *incomingVarNode;
    XanGraphNode        *varNode;
    XanHashTableItem    *hashItem;

    /* Assertions.
     */
    assert(vars != NULL);
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource: Start\n");
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource:    Var %llu\n", varID);
    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource:    Incoming var %llu\n", incomingVarID);

    /* Check the incoming variables for varID.
     */
    incomingVarNode     = xanGraph_getNode(incomingVars, (void *)(JITNUINT)(varID + 1));
    if (incomingVarNode != NULL){
        hashItem    = xanHashTable_first(incomingVarNode->incomingEdges);
        while (hashItem != NULL){
            IR_ITEM_VALUE   currentIncomingVarID;
            XanGraphNode    *currentIncomingVar;

            /* Fetch the current incoming variable ID.
             */
            currentIncomingVar      = hashItem->elementID;
            assert(currentIncomingVar != NULL);
            currentIncomingVarID    = (IR_ITEM_VALUE)(JITNUINT)currentIncomingVar->data;
            assert(currentIncomingVarID > 0);
            currentIncomingVarID--;

            /* Check the incoming variable.
             */
            if (currentIncomingVarID != incomingVarID){
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource:    Var %llu has another incoming variable (%llu) other than %llu\n", varID, currentIncomingVarID, incomingVarID);
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource: Exit\n");
                return JITFALSE;
            }

            /* Check if the variable escapes.
             */
            if (IRMETHOD_isAnEscapedVariable(m, currentIncomingVarID)){
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource:    The incoming var %llu escapes\n", currentIncomingVarID);
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource: Exit\n");
                return JITFALSE;
            }

            /* Fetch the next element.
             */
            hashItem    = xanHashTable_next(incomingVarNode->incomingEdges, hashItem);
        }
    }

    /* Check the other variables used to define the current one with only move instructions.
     */
    varNode     = xanGraph_getNode(vars, (void *)(JITNUINT)(varID + 1));
    if (varNode != NULL){
        hashItem    = xanHashTable_first(varNode->incomingEdges);
        while (hashItem != NULL){
            IR_ITEM_VALUE   currentIncomingVarID;
            XanGraphNode    *currentIncomingVar;

            /* Fetch the current incoming variable ID.
             */
            currentIncomingVar      = hashItem->elementID;
            assert(currentIncomingVar != NULL);
            currentIncomingVarID    = (IR_ITEM_VALUE)(JITNUINT)currentIncomingVar->data;
            assert(currentIncomingVarID > 0);
            currentIncomingVarID--;

            /* Check if the variable escapes.
             */
            if (IRMETHOD_isAnEscapedVariable(m, currentIncomingVarID)){
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource:    Var %llu escapes\n", currentIncomingVarID);
                PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource: Exit\n");
                return JITFALSE;
            }

            /* Check the incoming variable.
             */
            if (xanHashTable_lookup(alreadyChecked, currentIncomingVar->data) == NULL){
                xanHashTable_insert(alreadyChecked, currentIncomingVar->data, currentIncomingVar->data);
                if (!internal_canHaveOnlyIncomingVarSource(vars, incomingVars, currentIncomingVarID, incomingVarID, alreadyChecked, m)){
                    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource: Exit\n");
                    return JITFALSE;
                }
            }

            /* Fetch the next element.
             */
            hashItem    = xanHashTable_next(varNode->incomingEdges, hashItem);
        }
    }

    PDEBUG("COPY PROPAGATION: remove_cyclic_moves: canHaveOnlyIncomingVarSource: Exit\n");
    return JITTRUE;
}

static inline JITBOOLEAN internal_areVarsConnected (XanGraph *vars, IR_ITEM_VALUE varID1, IR_ITEM_VALUE varID2, XanHashTable *alreadyChecked){
    XanGraphNode        *varNode;
    XanHashTableItem    *hashItem;

    /* Assertions.
     */
    assert(vars != NULL);

    /* Check the other variables used to define the current one with only move instructions.
     */
    varNode     = xanGraph_getNode(vars, (void *)(JITNUINT)(varID1 + 1));
    if (varNode == NULL){
        return JITFALSE;
    }
    hashItem    = xanHashTable_first(varNode->incomingEdges);
    while (hashItem != NULL){
        IR_ITEM_VALUE   currentIncomingVarID;
        XanGraphNode    *currentIncomingVar;

        /* Fetch the current incoming variable ID.
         */
        currentIncomingVar      = hashItem->elementID;
        assert(currentIncomingVar != NULL);
        currentIncomingVarID    = (IR_ITEM_VALUE)(JITNUINT)currentIncomingVar->data;
        assert(currentIncomingVarID > 0);
        currentIncomingVarID--;

        /* Check if varID2 has been found.
         */
        if (currentIncomingVarID == varID2){
            return JITTRUE;
        }

        /* Check if varID2 is reachable from currentIncomingVarID.
         */
        if (xanHashTable_lookup(alreadyChecked, currentIncomingVar->data) == NULL){
            xanHashTable_insert(alreadyChecked, currentIncomingVar->data, currentIncomingVar->data);
            if (internal_areVarsConnected(vars, currentIncomingVarID, varID2, alreadyChecked)){
                return JITTRUE;
            }
        }

        /* Fetch the next element.
         */
        hashItem    = xanHashTable_next(varNode->incomingEdges, hashItem);
    }

    return JITFALSE;
}

static inline void internal_dumpEdges (XanGraph *vars, XanGraph *incomingVars){
    XanHashTableItem    *hashItem;

    fprintf(stderr, "Graph\n");
    hashItem    = xanHashTable_first(vars->nodes);
    while (hashItem != NULL){
        XanGraphNode    *n;
        XanGraphNode    *incomingVarNode;
        IR_ITEM_VALUE   varID;

        /* Fetch the variable.
         */
        n           = hashItem->element;
        assert(n != NULL);
        varID       = (IR_ITEM_VALUE)(JITNUINT)n->data;
        assert(varID > 0);
        varID--;

        /* Fetch the incoming variables.
         */
        incomingVarNode     = xanGraph_getNode(incomingVars, n->data);
        if (incomingVarNode != NULL){
            XanHashTableItem    *hashItem2;

            /* Fetch the incoming variables.
             */
            hashItem2               = xanHashTable_first(incomingVarNode->incomingEdges);
            while (hashItem2 != NULL){
                XanGraphNode        *currentIncomingVar;
                IR_ITEM_VALUE       currentIncomingVarID;
                currentIncomingVar      = hashItem2->elementID;
                assert(currentIncomingVar != NULL);
                currentIncomingVarID    = (IR_ITEM_VALUE)(JITNUINT)currentIncomingVar->data;
                assert(currentIncomingVarID > 0);
                currentIncomingVarID--;
                fprintf(stderr, "   %llu -> %llu\n", currentIncomingVarID, varID);
                hashItem2               = xanHashTable_next(incomingVarNode->incomingEdges, hashItem2);
            }
        }

        hashItem    = xanHashTable_next(vars->nodes, hashItem);
    }
    fprintf(stderr, "End graph\n");

    return ;
}
