/*
 * Copyright (C) 2010  Campanoni Simone
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
// End

/**
 * Convert from an integer to pointer and back.
 **/
#define uintToPtr(val) ((void *)(JITNUINT)(val))
#define ptrToUint(ptr) ((JITNUINT)(ptr))


void MISC_sortInstructions (ir_method_t *method, XanList *instructions) {
    JITUINT32       *table;
    JITUINT32 count;
    JITUINT32 instructionsNumber;
    JITBOOLEAN swapped;
    XanListItem     *item;

    /* Assertions				*/
    assert(method != NULL);
    assert(instructions != NULL);

    /* Compute the table of IDs		*/
    instructionsNumber = xanList_length(instructions);
    table = allocFunction(sizeof(JITUINT32) * instructionsNumber);
    count = 0;
    item = xanList_first(instructions);
    while (item != NULL) {
        ir_instruction_t        *inst;
        inst = item->data;
        table[count] = inst->ID;
        count++;
        item = item->next;
    }

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

    /* Add the ordered instructions to the	*
     * list					*/
    xanList_emptyOutList(instructions);
    for (count = 0; count < instructionsNumber; count++) {
        ir_instruction_t        *inst;
        inst = IRMETHOD_getInstructionAtPosition(method, table[count]);
        assert(inst != NULL);
        xanList_append(instructions, inst);
    }

    /* Free the memory			*/
    freeFunction(table);

    /* Return				*/
    return;
}

void MISC_sortProfiledLoops (XanList *profiledLoops) {
    loop_profile_t **set;
    JITUINT32 i;
    JITUINT32 size;
    XanListItem *item;
    JITBOOLEAN swapped;

    /* Assertions				*/
    assert(profiledLoops != NULL);

    /* Check if there are elements to sort	*/
    size = xanList_length(profiledLoops);
    if (size < 2) {
        return ;
    }

    /* Allocate the set 			*/
    set = allocFunction(sizeof(loop_profile_t *) * size);

    /* Fill up the set			*/
    i = 0;
    item = xanList_first(profiledLoops);
    while (item != NULL) {
        set[i] = (item->data);
        assert(set[i] != NULL);
        i++;
        item = item->next;
    }

    /* Order the set			*/
    do {
        swapped = JITFALSE;
        for (i = 0; i < (size - 1); i++) {
            if ((set[i]->totalTime) < (set[i + 1]->totalTime)) {
                loop_profile_t *temp;
                temp = set[i];
                set[i] = set[i + 1];
                set[i + 1] = temp;
                swapped = JITTRUE;
            }
        }
    } while (swapped);

    /* Fill up the list			*/
    xanList_emptyOutList(profiledLoops);
    for (i=0; i < size; i++) {
        xanList_append(profiledLoops, set[i]);
    }
    assert(xanList_length(profiledLoops) == size);

    /* Free the memory			*/
    freeFunction(set);

    /* Return				*/
    return ;
}

void MISC_dumpString (FILE **fileToWrite, FILE *summaryFile, JITINT8 *buf, JITUINT32 *bytesDumped, JITUINT32 *suffixNumber, JITINT8 *prefixName, JITUINT64 maximumBytesPerFile, JITBOOLEAN splitPoint) {
    JITINT8	localBuf[DIM_BUF];

    /* Assertions			*/
    assert(fileToWrite != NULL);
    assert(buf != NULL);
    assert(bytesDumped != NULL);
    assert(suffixNumber != NULL);
    assert(prefixName != NULL);

    /* Open the file		*/
    if ((*fileToWrite) == NULL) {
        SNPRINTF(localBuf, DIM_BUF, "%s_%u", prefixName, (*suffixNumber) - 1);
        (*fileToWrite)	= fopen((char *)localBuf, "w+");
        if ((*fileToWrite) == NULL) {
            abort();
        }
    }
    assert((*fileToWrite) != NULL);

    /* Dump the string		*/
    fprintf(*fileToWrite, "%s", buf);
    (*bytesDumped)	+= STRLEN(buf);

    /* Check the file		*/
    if (	((*bytesDumped) >= maximumBytesPerFile)	&&
            (splitPoint)				) {

        /* We dumped too many bytes, we	*
         * need to create a new one.	*/
        fclose(*fileToWrite);
        SNPRINTF(localBuf, DIM_BUF, "%s_%u", prefixName, *suffixNumber);
        (*suffixNumber)++;
        (*fileToWrite)	= fopen((char *)localBuf, "w+");
        if ((*fileToWrite) == NULL) {
            abort();
        }
        (*bytesDumped)	= 0;

        /* Dump the name of the summary	*
         * file				*/
        if (fprintf(summaryFile, "%s\n", localBuf) <= 0) {
            fprintf(stderr, "ERROR on dumping the string %s\n", localBuf);
            abort();
        }
    }

    /* Return			*/
    return ;
}


/**
 * Hash function for a hash table with a string key.
 **/
static unsigned int
MISC_stringHash(void *str) {
    unsigned i, len = STRLEN(str);
    unsigned int hash = 0;
    for (i = 0; i < len; ++i) {
        hash += ((char *)str)[i];
    }
    return hash;
}


/**
 * Comparison function for a hash table with a string key.
 **/
static int
MISC_stringsMatch(void *str1, void *str2) {
    return STRCMP(str1, str2) == 0;
}


/**
 * Hash table with strings as keys.
 **/
XanHashTable *
MISC_newHashTableStringKey(void) {
    return xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, MISC_stringHash, MISC_stringsMatch);
}



void checkLoops(XanList *loopList, XanHashTable *loopNames, XanHashTable *loopDict) {

    /* Assertions.
     */
    assert(loopList != NULL);
    assert(loopNames != NULL);

    if(xanList_length(loopList) != xanHashTable_elementsInside(loopNames)) {
        fprintf(stderr, "WARNING: looplist length: %i, loopNames length: %i\n", xanList_length(loopList), xanHashTable_elementsInside(loopNames));
    }

    XanHashTableItem *dictItem;
    XanHashTableItem *nameItem;

    if (loopDict == NULL) {
        return ;
    }
    dictItem = xanHashTable_first(loopDict);
    while(dictItem) {
        int found  = 0;
        nameItem = xanHashTable_first(loopNames);
        while(nameItem) {
            //fprintf(stderr, "Compare %s : %s ", (char *)nameItem->element, (char *)dictItem->elementID);
            if(MISC_stringsMatch((char *)nameItem->element,  (char *)dictItem->elementID)) {
                //fprintf(stderr, "Match\n");
                found = 1;
                break;
            }
            //fprintf(stderr, "NO MATCH\n");
            nameItem = xanHashTable_next(loopNames, nameItem);
        }
        if(found == 0) {
            fprintf(stderr, "WARNING: loop: %ld (%s) not found in loopNames\n", ptrToUint(dictItem->element), (char*)dictItem->elementID);
        }
        dictItem = xanHashTable_next(loopDict, dictItem);
    }

    nameItem = xanHashTable_first(loopNames);
    while(nameItem) {
        int found  = 0;
        dictItem = xanHashTable_first(loopDict);
        while(dictItem) {
            if(MISC_stringsMatch((char *)nameItem->element,  (char *)dictItem->elementID)) {
                found = 1;
                break;
            }
            dictItem = xanHashTable_next(loopDict, dictItem);
        }
        if(found == 0) {
            fprintf(stderr, "WARNING: loop: %s not found in loopDict\n", (char*)nameItem->element);
        }
        nameItem = xanHashTable_next(loopNames, nameItem);
    }
}

/**
 * Filter a list of loops.  If there is a *_enabled_loops file for the current
 * program then loops that are not in it are discarded.  By setting
 * CHIARA_FILTER_LOOP_ID a specific loop can be selected by its ID.  Otherwise,
 * loops from the *_enabled_loops file can be selected by using the environment
 * variables PARALLELIZER_LOWEST_LOOPS_TO_PARALLELIZE and
 * PARALLELIZER_MAXIMUM_LOOPS_TO_PARALLELIZE, as in HELIX.
 **/
void
MISC_chooseLoopsToProfile(XanList *loopList, XanHashTable *loopNames, XanHashTable *loopDict) {
    char *env;
    XanList *enabledLoops;
    JITINT8 filename[DIM_BUF];
    XanList *chosenLoops;

    /* Read in the list of loops, if it exists. */
    SNPRINTF(filename, DIM_BUF, "%s_enable_loops", IRPROGRAM_getProgramName());
    enabledLoops = LOOPS_loadLoopNames(filename);

    /* Create a new list of loops to choose. */
    chosenLoops = xanList_new(allocFunction, freeFunction, NULL);

    /* Check loops */
    checkLoops(loopList, loopNames, loopDict);

    /* Filter out those not enabled. */
    if (xanList_length(enabledLoops) > 0) {
        JITNINT loopNum = 0;
        JITNINT lowestLoop = 0;
        JITNINT maximumLoop = xanList_length(enabledLoops);
        XanListItem *enabledItem;

        /* Get the lowest and maximum loop to enable. */
        env = getenv("PARALLELIZER_LOWEST_LOOPS_TO_PARALLELIZE");
        if (env) {
            lowestLoop = atoi(env);
        }
        env = getenv("PARALLELIZER_MAXIMUM_LOOPS_TO_PARALLELIZE");
        if (env) {
            maximumLoop = atoi(env);
        }

        /* Choose loops to enable. */
        enabledItem = xanList_first(enabledLoops);
        while (enabledItem && loopNum < maximumLoop) {
            if (loopNum >= lowestLoop) {
                XanListItem *loopItem = xanList_first(loopList);
                while (loopItem) {
                    loop_t *loop = loopItem->data;
                    JITINT8 *loopName = xanHashTable_lookup(loopNames, loop);
                    if (MISC_stringsMatch(loopName, enabledItem->data)) {
                        xanList_append(chosenLoops, loop);
                        break;
                    }
                    loopItem = loopItem->next;
                }
            }
            loopNum += 1;
            enabledItem = enabledItem->next;
        }
    } else {
        xanList_appendList(chosenLoops, loopList);
    }

    /* Filter based on loop ID. */
    env = getenv("CHIARA_FILTER_LOOP_ID");
    if (env && loopDict) {
        JITNINT chosenLoopID = atoi(env);
        if (chosenLoopID > 0) {
            XanListItem *loopItem = xanList_first(chosenLoops);
            while (loopItem) {
                XanListItem *nextItem = loopItem->next;
                JITINT8 *loopName = xanHashTable_lookup(loopNames, loopItem->data);
                JITNUINT loopID = ptrToUint(xanHashTable_lookup(loopDict, loopName));
                if (loopID != chosenLoopID) {
                    xanList_deleteItem(chosenLoops, loopItem);
                }

                /* Next loop to consider. */
                loopItem = nextItem;
            }
        }
    }

    /* Record the list of chosen loops. */
    xanList_emptyOutList(loopList);
    xanList_appendList(loopList, chosenLoops);

    /* Clean up. */
    xanList_destroyListAndData(enabledLoops);
    xanList_destroyList(chosenLoops);
}
