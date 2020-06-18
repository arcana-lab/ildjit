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
#include <xanlib.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>

// My headers
#include <misc.h>
// End

static inline XanList * internal_getDefinitions (ir_method_t *method, IR_ITEM_VALUE varID, XanHashTable *defsTable);

ir_instruction_t * internal_getSingleDefinitionReachedIn (ir_method_t *method, IR_ITEM_VALUE varID, ir_instruction_t *instToCheck, XanHashTable *defsTable) {
    XanList                 *list;
    XanListItem             *item;
    JITUINT32 count;
    ir_instruction_t        *champion_inst;
    ir_instruction_t        *inst;

    /* Assertions                   */
    assert(method != NULL);
    assert(instToCheck != NULL);
    assert(defsTable != NULL);

    /* Initialize the variables     */
    champion_inst = NULL;
    count = 0;

    list = internal_getDefinitions(method, varID, defsTable);
    assert(list != NULL);
    item = xanList_first(list);
    while (item != NULL) {
        inst = (ir_instruction_t *) item->data;
        assert(inst != NULL);
        if (ir_instr_reaching_definitions_reached_in_is(method, instToCheck, inst->ID)) {
            champion_inst = inst;
            count++;
        }
        item = item->next;
    }

    /* Return                       */
    if (count == 1) {
        return champion_inst;
    }
    return NULL;
}

void internal_destroyDefsTable (XanHashTable *defsTable) {
    XanHashTableItem        *item;

    /* Assertions			*/
    assert(defsTable != NULL);

    /* Free the memory		*/
    item = xanHashTable_first(defsTable);
    while (item != NULL) {
        XanList *l;
        l = (XanList *) item->element;
        assert(l != NULL);
        xanList_destroyList(l);
        item = xanHashTable_next(defsTable, item);
    }
    xanHashTable_destroyTable(defsTable);

    /* Return			*/
    return;
}

static inline XanList * internal_getDefinitions (ir_method_t *method, IR_ITEM_VALUE varID, XanHashTable *defsTable) {
    XanList                 *list;

    /* Assertions                           */
    assert(method != NULL);
    assert(defsTable != NULL);

    list = xanHashTable_lookup(defsTable, (void *) (JITNUINT) (varID + 1));
    if (list == NULL) {

        /* Fetch the new list			*/
        list = IRMETHOD_getVariableDefinitions(method, varID);

        /* Cache the new list			*/
        xanHashTable_insert(defsTable, (void *) (JITNUINT) (varID + 1), list);
    }
    assert(list != NULL);

    /* Return                               */
    return list;
}
