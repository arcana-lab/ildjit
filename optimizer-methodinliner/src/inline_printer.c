/*
 * Copyright (C) 2010 - 2012  Campanoni Simone
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
#include <assert.h>
#include <errno.h>
#include <ir_method.h>
#include <xanlib.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>
#include <chiara.h>

// My headers
#include <optimizer_methodinliner.h>
#include <inline_heuristics.h>
// End

extern ir_lib_t *irLib;
extern ir_optimizer_t  *irOptimizer;

static inline void internal_dumpCallees (ir_method_t *m, XanHashTable *methodsMap, XanList *callees, JITBOOLEAN isDirect, JITBOOLEAN **alreadyDumped, FILE *callGraphFile);
static inline void internal_dump_call_graph (JITBOOLEAN noLibrary, char *nodesFileName, char *callGraphName, char *directCallGraphName);

void dump_call_graph (char *prefixToUse) {
    char	*prevName;
    char	buf[DIM_BUF];
    char	buf2[DIM_BUF];
    char	buf3[DIM_BUF];

    /* Fetch the name in use for printing.
     */
    prevName = getenv("DOTPRINTER_FILENAME");

    /* Print all methods.
     */
    setenv("DOTPRINTER_FILENAME", "INLINER_allMethods", JITTRUE);
    snprintf(buf, DIM_BUF, "%snodes.txt", prefixToUse != NULL ? prefixToUse : "");
    snprintf(buf2, DIM_BUF, "%scallGraph.txt", prefixToUse != NULL ? prefixToUse : "");
    snprintf(buf3, DIM_BUF, "%sdirectCallGraph.txt", prefixToUse != NULL ? prefixToUse : "");
    //internal_dump_call_graph(JITFALSE, buf, buf2, buf3);

    /* Print program methods.
     */
    setenv("DOTPRINTER_FILENAME", "INLINER_programMethods", JITTRUE);
    snprintf(buf, DIM_BUF, "%sprogram_nodes.txt", prefixToUse != NULL ? prefixToUse : "");
    snprintf(buf2, DIM_BUF, "%sprogram_callGraph.txt", prefixToUse != NULL ? prefixToUse : "");
    snprintf(buf3, DIM_BUF, "%sprogram_directCallGraph.txt", prefixToUse != NULL ? prefixToUse : "");
    internal_dump_call_graph(JITTRUE, buf, buf2, buf3);

    /* Set the file name to use to the previous value.
     */
    setenv("DOTPRINTER_FILENAME", prevName, JITTRUE);

    return ;
}

static inline void internal_dump_call_graph (JITBOOLEAN noLibrary, char *nodesFileName, char *callGraphName, char *directCallGraphName) {
    XanHashTable	*methodsMap;
    XanList		*l;
    XanList		*toDelete;
    XanListItem	*item;
    JITUINT32	count;
    JITBOOLEAN	**alreadyDumped;
    JITBOOLEAN	**directAlreadyDumped;
    JITBOOLEAN	**directAlreadyDumped2;
    FILE		*nodesFile;
    FILE		*callGraphFile;
    FILE		*directCallGraphFile;

    /* Open the files.
     */
    nodesFile	= fopen(nodesFileName, "w");
    if (nodesFile == NULL) {
        print_err("ERROR opening the nodes file. ", errno);
        return ;
    }
    callGraphFile	= fopen(callGraphName, "w");
    if (callGraphFile == NULL) {
        print_err("ERROR opening the call graph file. ", errno);
        fclose(nodesFile);
        return ;
    }
    directCallGraphFile	= fopen(directCallGraphName, "w");
    if (directCallGraphFile == NULL) {
        print_err("ERROR opening the direct call graph file. ", errno);
        fclose(nodesFile);
        fclose(callGraphFile);
        return ;
    }

    /* Dump the header of the call graph
     */
    fprintf(callGraphFile, "digraph \"%s\" {\n", IRPROGRAM_getProgramName());
    fprintf(callGraphFile, "label=\"Call graph. Dotted lines: indirect calls.\"\n");
    fprintf(directCallGraphFile, "digraph \"%s\" {\n", IRPROGRAM_getProgramName());
    fprintf(directCallGraphFile, "label=\"Direct call graph.\"\n");

    /* Allocate the necessary memory.
     */
    methodsMap	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
    toDelete	= xanList_new(allocFunction, freeFunction, NULL);

    /* Fetch the list of methods to dump.
     */
    l 		= IRPROGRAM_getReachableMethodsNeededByProgram();
    assert(l != NULL);

    /* Filter out methods that belong to libraries.
     */
    if (noLibrary) {
        item	= xanList_first(l);
        while (item != NULL) {
            ir_method_t	*m;
            m	= item->data;
            assert(m != NULL);
            if (IRPROGRAM_doesMethodBelongToALibrary(m)) {
                xanList_insert(toDelete, m);
            }
            item	= item->next;
        }
        xanList_removeElements(l, toDelete, JITTRUE);
    }
    xanList_emptyOutList(toDelete);

    /* Allocate the memory used to determine whether an edge has been already dumped.
     */
    alreadyDumped		= allocFunction(sizeof(JITBOOLEAN *) * (xanList_length(l) + 1));
    directAlreadyDumped	= allocFunction(sizeof(JITBOOLEAN *) * (xanList_length(l) + 1));
    directAlreadyDumped2	= allocFunction(sizeof(JITBOOLEAN *) * (xanList_length(l) + 1));
    for (count=0; count <= xanList_length(l); count++) {
        alreadyDumped[count]		= allocFunction(sizeof(JITBOOLEAN) * (xanList_length(l) + 1));
        directAlreadyDumped[count]	= allocFunction(sizeof(JITBOOLEAN) * (xanList_length(l) + 1));
        directAlreadyDumped2[count]	= allocFunction(sizeof(JITBOOLEAN) * (xanList_length(l) + 1));
    }

    /* Dump the nodes.
     */
    count	= 1;
    item	= xanList_first(l);
    while (item != NULL) {
        ir_method_t	*m;
        char		buf[DIM_BUF];

        /* Fetch the method.
         */
        m	= item->data;
        assert(m != NULL);

        /* Dump the CFG.
         */
        IROPTIMIZER_callMethodOptimization(irOptimizer, m, METHOD_PRINTER);

        /* Dump a new node in the call graph.
         */
        snprintf(buf, 2048, "node [ fontsize=12 label=\"%u\" shape=box color=%s] ;\n", count, hasLoop(m) ? "red" : "black");
        fprintf(nodesFile, "%u: %s\n", count, IRMETHOD_getSignatureInString(m));
        fprintf(callGraphFile, "%s", buf);
        fprintf(directCallGraphFile, "%s", buf);
        fprintf(callGraphFile, "n%u ;\n", count);
        fprintf(directCallGraphFile, "n%u ;\n", count);
        xanHashTable_insert(methodsMap, m, (void *)(JITNUINT)count);

        /* Fetch the next element.
         */
        count++;
        item 	= item->next;
    }

    /* Dump the call graph.
     */
    item	= xanList_first(l);
    while (item != NULL) {
        ir_method_t	*m;
        XanList		*callees;
        XanList		*virtual_callees;
        XanList		*indirect_callees;

        /* Fetch the caller.
         */
        m			= item->data;
        assert(m != NULL);

        /* Fetch the direct callees.
         */
        callees			= IRMETHOD_getInstructionsOfType(m, IRCALL);

        /* Dump the edges from the caller to its direct callees.
         */
        internal_dumpCallees(m, methodsMap, callees, JITTRUE, directAlreadyDumped, callGraphFile);
        internal_dumpCallees(m, methodsMap, callees, JITTRUE, directAlreadyDumped2, directCallGraphFile);

        /* Fetch the indirect callees.
         */
        virtual_callees		= IRMETHOD_getInstructionsOfType(m, IRVCALL);
        indirect_callees	= IRMETHOD_getInstructionsOfType(m, IRICALL);

        /* Dump the edges from the caller to its indirect callees.
         */
        internal_dumpCallees(m, methodsMap, virtual_callees, JITFALSE, alreadyDumped, callGraphFile);
        internal_dumpCallees(m, methodsMap, indirect_callees, JITFALSE, alreadyDumped, callGraphFile);

        /* Free the memory.
         */
        xanList_destroyList(callees);
        xanList_destroyList(virtual_callees);
        xanList_destroyList(indirect_callees);

        /* Fetch the next element.
         */
        item 			= item->next;
    }

    /* Dump the footer.
     */
    fprintf(callGraphFile, "\n}\n");
    fprintf(directCallGraphFile, "\n}\n");

    /* Free the memory.
     */
    for (count=0; count <= xanList_length(l); count++) {
        freeFunction(alreadyDumped[count]);
        freeFunction(directAlreadyDumped[count]);
        freeFunction(directAlreadyDumped2[count]);
    }
    xanList_destroyList(toDelete);
    xanHashTable_destroyTable(methodsMap);
    freeFunction(alreadyDumped);
    freeFunction(directAlreadyDumped);
    freeFunction(directAlreadyDumped2);
    xanList_destroyList(l);

    /* Close the files.
     */
    fclose(nodesFile);
    fclose(callGraphFile);
    fclose(directCallGraphFile);

    return ;
}

static inline void internal_dumpCallees (ir_method_t *m, XanHashTable *methodsMap, XanList *callees, JITBOOLEAN isDirect, JITBOOLEAN **alreadyDumped, FILE *callGraphFile) {
    XanListItem	*item;
    JITUINT32	count;
    JITUINT32	count2;

    /* Fetch the caller.
     */
    count	= (JITNUINT)xanHashTable_lookup(methodsMap, m);
    assert(count != 0);

    /* Dump the callees.
     */
    item	= xanList_first(callees);
    while (item != NULL) {
        ir_method_t		*callee;
        ir_instruction_t	*callInst;
        XanList			*methodsToDump;
        XanListItem		*item2;

        /* Fetch all the possible callees of the current call instruction.
         */
        callInst	= item->data;
        assert(callInst != NULL);
        methodsToDump	= IRMETHOD_getCallableIRMethods(callInst);
        assert(methodsToDump != NULL);

        /* Dump the callees.
         */
        item2		= xanList_first(methodsToDump);
        while (item2 != NULL) {
            callee		= item2->data;
            assert(callee != NULL);
            count2		= (JITNUINT)xanHashTable_lookup(methodsMap, callee);
            if (	(count2 != 0)				&&
                    (!alreadyDumped[count][count2])		) {

                /* Dump the callee.
                 */
                alreadyDumped[count][count2]	= JITTRUE;
                fprintf(callGraphFile, "n%u -> n%u %s ;\n", count, count2, isDirect ? " " : "[style=dotted]");
            }
            item2		= item2->next;
        }

        /* Free the memory.
         */
        xanList_destroyList(methodsToDump);

        /* Fetch the next element from the list.
         */
        item	= item->next;
    }

    return ;
}
