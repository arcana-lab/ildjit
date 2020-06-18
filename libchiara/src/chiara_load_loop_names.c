/*
 * Copyright (C) 2013  Campanoni Simone
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
#include <loop_names_scanner.h>
// End

XanList * load_list_of_loop_names (JITINT8 *fileName) {
    XanList         *inhibitLoops;
    FILE            *inputFile;

    inhibitLoops = xanList_new(allocFunction, freeFunction, NULL);
    inputFile = fopen((char *) fileName, "r");
    if (inputFile != NULL) {

        /* Decode the input file			*/
        loop_namesin = inputFile;
        loop_names_lex(inhibitLoops);

        /* Close the file				*/
        fclose(inputFile);
    }

    return inhibitLoops;
}
