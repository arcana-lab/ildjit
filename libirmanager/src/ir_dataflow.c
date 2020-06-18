/*
 * Copyright (C) 2006 - 2012 Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABIRITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <jitsystem.h>
#include <compiler_memory_manager.h>
#include <base_symbol.h>

// My headers
#include <ir_method.h>
#include <ir_data.h>
// End

extern ir_lib_t * ir_library;

data_flow_t * DATAFLOW_allocateSets (JITUINT32 instructionsNumber, JITUINT32 elements) {
    data_flow_t     *data;
    JITUINT32 i;
    JITUINT32 blocksNumber;

    /* Allocate the structure.
     */
    data = allocFunction(sizeof(data_flow_t));
    assert(data != NULL);

    /* Compute the number of blocks.
     */
    blocksNumber = (elements / (sizeof(JITNUINT) * 8)) + 1;
    assert(blocksNumber > 0);

    /* Fill up the structure.
     */
    data->num = instructionsNumber + 1;
    data->elements = blocksNumber;
    data->data = allocFunction(sizeof(single_data_flow_t) * (data->num));
    assert(data->data != NULL);
    for (i = 0; i < (data->num); i++) {
        (data->data[i]).kill = allocFunction(blocksNumber * sizeof(JITNUINT));
        (data->data[i]).gen = allocFunction(blocksNumber * sizeof(JITNUINT));
        (data->data[i]).in = allocFunction(blocksNumber * sizeof(JITNUINT));
        (data->data[i]).out = allocFunction(blocksNumber * sizeof(JITNUINT));
    }

    return data;
}

void DATAFLOW_destroySets (data_flow_t *sets) {

    /* Check the data.
     */
    if (sets == NULL) {
        return;
    }

    /* Free the local structures.
     */
    if (sets->data != NULL) {
        JITUINT32 i;
        for (i = 0; i < sets->num; i++) {
            if (sets->data[i].kill != NULL) {
                freeFunction(sets->data[i].kill);
            }
            if (sets->data[i].gen != NULL) {
                freeFunction(sets->data[i].gen);
            }
            if (sets->data[i].in != NULL) {
                freeFunction(sets->data[i].in);
            }
            if (sets->data[i].out != NULL) {
                freeFunction(sets->data[i].out);
            }
        }
        freeFunction(sets->data);
    }

    /* Free the global structure.
     */
    freeFunction(sets);

    return ;
}

void DATAFLOW_setEmptySets (data_flow_t *sets, JITUINT32 setID, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out) {
    JITUINT32	j;

    /* Assertions.
     */
    assert(sets != NULL);

    /* Fill up the structure.
     */
    for (j=0; j < sets->elements; j++) {
        if (kill) {
            (sets->data[setID]).kill[j]	= 0;
        }
        if (gen) {
            (sets->data[setID]).gen[j]	= 0;
        }
        if (in) {
            (sets->data[setID]).in[j]	= 0;
        }
        if (out) {
            (sets->data[setID]).out[j]	= 0;
        }
    }

    return ;
}

void DATAFLOW_setAllEmptySets (data_flow_t *sets, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out) {
    JITUINT32	i;

    /* Assertions.
     */
    assert(sets != NULL);

    /* Fill up the structure.
     */
    for (i = 0; i < (sets->num); i++) {
        DATAFLOW_setEmptySets(sets, i, gen, kill, in, out);
    }

    return ;
}

void DATAFLOW_setFullSets (data_flow_t *sets, JITUINT32 setID, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out) {
    JITUINT32	j;

    /* Assertions.
     */
    assert(sets != NULL);

    /* Fill up the structure.
     */
    for (j=0; j < sets->elements; j++) {
        if (kill) {
            (sets->data[setID]).kill[j]	= (JITNUINT) -1;
        }
        if (gen) {
            (sets->data[setID]).gen[j]	= (JITNUINT) -1;
        }
        if (in) {
            (sets->data[setID]).in[j]	= (JITNUINT) -1;
        }
        if (out) {
            (sets->data[setID]).out[j]	= (JITNUINT) -1;
        }
    }

    return ;
}

void DATAFLOW_setAllFullSets (data_flow_t *sets, JITBOOLEAN gen, JITBOOLEAN kill, JITBOOLEAN in, JITBOOLEAN out) {
    JITUINT32	i;

    /* Assertions.
     */
    assert(sets != NULL);

    /* Fill up the structure.
     */
    for (i = 0; i < (sets->num); i++) {
        DATAFLOW_setFullSets(sets, i, gen, kill, in, out);
    }

    return ;
}

JITBOOLEAN DATAFLOW_doesElementExistInINSet (data_flow_t *sets, JITUINT32 setID, JITUINT32 elementID) {
    JITUINT32	trigger;
    JITNUINT	temp;
    JITBOOLEAN	exist;

    /* Assertions.
     */
    assert(sets != NULL);
    assert(setID < sets->num);
    assert(sets->data[setID].gen != NULL);

    /* Initialize variables.
     */
    temp		= 0x1;

    /* Check the element.
     */
    trigger  	= elementID / (sizeof(JITNUINT) * 8);
    assert(trigger < (sets->elements));
    exist		= (((sets->data[setID]).in[trigger] & (temp << (elementID % (sizeof(JITNUINT) * 8)))) != 0);

    return exist;
}

void DATAFLOW_addElementToGENSet (data_flow_t *sets, JITUINT32 setID, JITUINT32 elementID) {
    JITUINT32	trigger;
    JITNUINT	temp;

    /* Assertions.
     */
    assert(sets != NULL);
    assert(setID < sets->num);
    assert(sets->data[setID].gen != NULL);

    /* Initialize variables.
     */
    temp		= 0x1;

    /* Add the element.
     */
    trigger  	= elementID / (sizeof(JITNUINT) * 8);
    assert(trigger < (sets->elements));
    (sets->data[setID]).gen[trigger]    = (temp << (elementID % (sizeof(JITNUINT) * 8)));

    return ;
}
