/*
 * Copyright (C) 2012 - 2013  Campanoni Simone
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
#include <platform_API.h>

// My headers
#include <chiara.h>
#include <instruction_scanner.h>
// End

XanHashTable * MISC_loadInstructionDictionary (XanHashTable *methodsThatIncludeInstructionsLoaded, char *fileName, XanHashTable **instIDs, XanHashTable **instructionToMethod) {
    XanHashTable	*t;
    XanHashTable	*t2;
    XanHashTable	*t3;
    FILE			*inputFile;

    /* Assertions.
     */
    assert(methodsThatIncludeInstructionsLoaded != NULL);

    /* Allocate the memory.
     */
	t2	= NULL;
	t3	= NULL;
    t	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	if (instIDs != NULL){
		t2	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
		(*instIDs)				= t2;
	}
	if (instructionToMethod != NULL){
		t3	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
		(*instructionToMethod)	= t3;
	}

    /* Open the file to parse.
     */
    inputFile	= fopen(fileName, "r");
    if (inputFile == NULL) {
        print_err("ERROR: cannot open \"instruction_IDs.txt\". ", errno);
        abort();
    }

    /* Load the instructions.
     */
    instructionin	= inputFile;
    instruction_lex(t, methodsThatIncludeInstructionsLoaded, t2, t3);

    /* Close the file.
     */
    fclose(inputFile);

    return t;
}
