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
#include <loop_scanner.h>
// End

XanHashTable * MISC_loadLoopDictionary (char *fileName, XanHashTable *loopDict) {
    FILE		*inputFile;

    /* Allocate the memory.
     */
    if (loopDict == NULL) {
        loopDict	= MISC_newHashTableStringKey();
    }

    /* Default file name.
     */
    if (fileName == NULL) {
        fileName = "loop_IDs.txt";
    }

    /* Open the file to parse.
     */
    inputFile	= fopen(fileName, "r");
    if (inputFile == NULL) {
        print_err("ERROR: cannot open loop dictionary file. ", errno);
        abort();
    }

    /* Load the loops.
     */
    loop_idin	= inputFile;
    loop_id_lex(loopDict);

    /* Close the file.
     */
    fclose(inputFile);

    return loopDict;
}
