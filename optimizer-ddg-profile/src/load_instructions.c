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
#include <chiara.h>

// My headers
#include <instruction_scanner.h>
#include <optimizer_ddg_profile.h>
// End

XanHashTable * load_instructions (XanHashTable *methods){
	XanHashTable	*t;
	FILE		*inputFile;

	/* Assertions.
	 */
	assert(methods != NULL);

	/* Allocate the memory.
	 */
	t	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

	/* Open the file to parse.
	 */
	inputFile	= fopen("instruction_IDs.txt", "r");
	if (inputFile == NULL){
		print_err("ERROR: cannot open \"instruction_IDs.txt\". ", errno);
		abort();
	}

	/* Load the instructions.
	 */
	instructionin	= inputFile;
	instruction_lex(t, methods);

	/* Close the file.
	 */
	fclose(inputFile);

	return t;
}
