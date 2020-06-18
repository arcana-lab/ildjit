/*
 * Copyright (C) 2009 - 2012  Simone Campanoni, Michele Tartara
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
#include <iljit-utils.h>
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <jitsystem.h>
#include <compiler_memory_manager.h>

// My headers
#include <config.h>
#include <dot_printer.h>
// End

#define FAKEHEAD        IRMETHOD_getInstructionsNumber(method) + 1
#define EXIT            IRMETHOD_getInstructionsNumber(method)
#define DIM_BUF         10000
#define JOB             METHOD_PRINTER
#define NODE_ATTRIBUTES "fontsize=12"
#define EDGE_ATTRIBUTES "fontsize=12"
#define INFORMATIONS    "This step print the control flow graph with or without the basic blocks of the method, using the dot grammar to the stderr"
#define AUTHOR          "Campanoni Simone"

static inline JITUINT64 get_ID_job (void);
static inline void printer_do_job (ir_method_t *method);
char * get_version (void);
char * get_informations (void);
char * get_author (void);
static inline JITUINT64 get_dependences (void);
void print_postdominatortree (ir_method_t *method, FILE *fileToWrite);
void print_postdominatortree_help (ir_method_t *method, FILE *fileToWrite, XanNode *node);
void print_predominatortree (ir_method_t *method, FILE *fileToWrite);
void print_predominatortree_help (ir_method_t *method, FILE *fileToWrite, XanNode *node);
void print_DDG (ir_method_t *method, FILE *fileToWrite);
void print_CDG (ir_method_t *method, FILE *fileToWrite);
void print_DDG_dependences (ir_method_t *method, FILE *fileToWrite, ir_instruction_t *inst, JITUINT16 depType);
void print_ir_method_body (ir_method_t *method, FILE *fileToWrite);
void print_instruction (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void dotprinter_printResult (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void printParameters (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void printCallParameters (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void print_liveness_IN (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void print_liveness_OUT (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void print_available_expressions_IN (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void print_available_expressions_OUT (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void print_reaching_definitions_IN (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
void print_reaching_definitions_OUT (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite);
JITUINT32 findLabel (ir_method_t *method, JITUINT32 labelID);
JITUINT32 findCatcher (ir_method_t *method);
JITUINT32 findFinally (ir_method_t *method);
JITUINT32 findFilter (ir_method_t *method);
JITUINT32 findBasicBlockID (ir_method_t *method, JITUINT32 instID);
void dp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
void dp_shutdown (JITFLOAT32 totalTime);
void internal_print_start_node (FILE *fileToWrite, ir_method_t *method);
static inline void compute_method_information (ir_method_t *method);
void print_loop_help (ir_method_t *method, FILE *fileToWrite, circuit_t *loop);
void print_loops (ir_method_t *method, FILE *fileToWrite);
void print_loops_tree_help (ir_method_t *method, FILE *fileToWrite, XanNode *node);
JITINT8 * internal_getMethodName (ir_method_t *method);
void printParameter (ir_method_t *method, ir_item_t *item, FILE *fileToWrite);
static inline JITUINT64 printer_get_invalidations (void);

ir_lib_t        *irLib = NULL;
ir_optimizer_t  *irOptimizer = NULL;
char            *prefix = NULL;

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    dp_init,
    dp_shutdown,
    printer_do_job,
    get_version,
    get_informations,
    get_author,
    printer_get_invalidations
};

static inline JITUINT64 printer_get_invalidations (void) {
    return INVALIDATE_NONE;
}

void dp_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(optimizer != NULL);

    irLib = lib;
    irOptimizer = optimizer;
    prefix = outputPrefix;
}

void dp_shutdown (JITFLOAT32 totalTime) {

    irLib = NULL;

    /* Return                       */
    return;
}

static inline JITUINT64 get_ID_job (void) {
    return JOB;
}

static inline JITUINT64 get_dependences (void) {
    return BASIC_BLOCK_IDENTIFIER;
}

static inline void printer_do_job (ir_method_t *method) {
    JITUINT32 		count;
    JITUINT32 		startNode;
    JITUINT32 		stopNode;
    JITUINT32 		basicBlocksNumber;
    JITUINT32 		bufLength;
    JITINT8                 *buf;
    JITINT8                 *methodName;
    ir_instruction_t        *inst;
    XanList			*loops;
    FILE                    *fileToWriteAll;
    FILE                    *fileToWrite;
    FILE                    *fileToWriteSymbols;
    char                    *fileName;

    /* Assertions				*/
    assert(method != NULL);
    PDEBUG("METHOD PRINTER: Start\n");

    /* Fetch the signature of the method	*/
    methodName = internal_getMethodName(method);
    assert(methodName != NULL);

    /* Fetch the file name                  */
    fileName = getenv("DOTPRINTER_FILENAME");
    if (	(fileName == NULL)		||
            (strcmp(fileName, "") == 0))	{
        fileName = prefix;
    }
    assert(fileName != NULL);

    /* Open the file			*/
    bufLength = (STRLEN(prefix) + STRLEN(methodName))*2;
    buf = allocFunction(bufLength);
    SNPRINTF(buf, bufLength, "%s.cfg", fileName);
    fileToWrite = fopen((char *) buf, "a");
    if (fileToWrite == NULL) {
        SNPRINTF(buf, sizeof(char) * DIM_BUF, "METHOD_PRINTER: ERROR = Cannot open the file %s. ", buf);
        PRINT_ERR(buf, 0);
        abort();
    }
    SNPRINTF(buf, bufLength, "%s_all.cfg", fileName);
    fileToWriteAll	= fopen((char *)buf, "a");
    if (fileToWriteAll == NULL) {
        SNPRINTF(buf, sizeof(char) * DIM_BUF, "METHOD_PRINTER: ERROR = Cannot open the file %s. ", buf);
        PRINT_ERR(buf, 0);
        abort();
    }
    SNPRINTF(buf, bufLength, "%s_symbols.txt", fileName);
    fileToWriteSymbols	= fopen((char *)buf, "a");
    if (fileToWriteSymbols == NULL) {
        print_err("ERROR: opening the symbols.txt file. ", errno);
        abort();
    }

    /* Print the head of the graph		*/
    fprintf(fileToWriteAll, "digraph \"%s\" {\n", methodName);
    fprintf(fileToWriteAll, "label=\"Control Flow Graph\"\n");
    fprintf(fileToWrite, "digraph \"%s\" {\n", methodName);
    fprintf(fileToWrite, "label=\"Control Flow Graph\"\n");
    fprintf(fileToWriteSymbols, "\n\n====================================\n");
    fprintf(fileToWriteSymbols, "%s\n", methodName);
    fprintf(fileToWriteSymbols, "====================================\n");

    /* Compute method information           */
    compute_method_information(method);

    /* Compute the start and the stop node	*/
    stopNode = IRMETHOD_getInstructionsNumber(method);
    startNode = stopNode + 1;

    /* Print the start node			*/
    internal_print_start_node(fileToWriteAll, method);
    internal_print_start_node(fileToWrite, method);
    fprintf(fileToWriteAll, "n%u ;\n", startNode);
    fprintf(fileToWrite, "n%u ;\n", startNode);

    /* Fetch the loops of the method.
     */
    if (IROPTIMIZER_isInformationValid(method, LOOP_IDENTIFICATION)) {
        loops	= IRMETHOD_getMethodCircuits(method);
    } else {
        loops	= xanList_new(allocFunction, freeFunction, NULL);
    }

    /* Fetch the number of the basic blocks	*/
    if (!PRINT_BASICBLOCK) {

        /* Print the nodes			*/
        print_ir_method_body(method, fileToWrite);

        /* Print the edges			*/
        fprintf(fileToWrite, "n%u -> n%u ;\n", startNode, 0);
        for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
            ir_instruction_t        *successor;
            inst = IRMETHOD_getInstructionAtPosition(method, count);
            assert(inst != NULL);
            successor = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            if (successor == method->exitNode) {
                fprintf(fileToWrite, "n%u -> n%u ;\n", count, stopNode);
            } else {
                while (successor != NULL) {
                    fprintf(fileToWrite, "n%u -> n%u ;\n", count, IRMETHOD_getInstructionPosition(successor));
                    successor = IRMETHOD_getSuccessorInstruction(method, inst, successor);
                }
            }
        }
    } else {

        /* Find the number of basic blocks */
        basicBlocksNumber = IRMETHOD_getNumberOfMethodBasicBlocks(method);
        assert(basicBlocksNumber > 0);

        /* Print the nodes			*/
        for (count = 0; count < basicBlocksNumber; count++) {
            IRBasicBlock      *basicBlock;
            JITUINT32 count2;
            basicBlock = IRMETHOD_getBasicBlockAtPosition(method, count);
            assert(basicBlock != NULL);
            assert(basicBlock->startInst <= basicBlock->endInst);
            fprintf(fileToWriteAll, "node [ %s label=\"", NODE_ATTRIBUTES);
            fprintf(fileToWrite, "node [ %s label=\"%u - %u", NODE_ATTRIBUTES, basicBlock->startInst, basicBlock->endInst);
            for (count2 = basicBlock->startInst; count2 <= basicBlock->endInst; count2++) {
                inst = IRMETHOD_getInstructionAtPosition(method, count2);
                assert(inst != NULL);
                print_instruction(method, inst, fileToWriteAll);
                print_instruction(method, inst, fileToWriteSymbols);
                fprintf(fileToWriteAll, "	\\n");
                fprintf(fileToWriteSymbols, "\n");
            }
            fprintf(fileToWriteAll, "\" shape=box ] ;\n");
            fprintf(fileToWriteAll, "n%u ;\n", count);
            fprintf(fileToWrite, "	\\n");
            fprintf(fileToWrite, "\" shape=box ] ;\n");
            fprintf(fileToWrite, "n%u ;\n", count);
        }

        /* Print the edges			*/
        fprintf(fileToWriteAll, "n%u -> n%u ;\n", startNode, 0);
        fprintf(fileToWrite, "n%u -> n%u ;\n", startNode, 0);
        for (count = 0; count < basicBlocksNumber; count++) {
            IRBasicBlock              *basicBlock;
            JITUINT32 basicBlockID;
            ir_instruction_t        *succ;
            basicBlock = IRMETHOD_getBasicBlockAtPosition(method, count);
            assert(basicBlock != NULL);
            inst = IRMETHOD_getInstructionAtPosition(method, basicBlock->endInst);
            assert(inst != NULL);
            succ = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
            while (succ != NULL) {
                JITBOOLEAN	isBackedge;
                XanListItem	*item;

                basicBlockID = findBasicBlockID(method, IRMETHOD_getInstructionPosition(succ));
                fprintf(fileToWriteAll, "n%u -> n%u [%s ", count, basicBlockID, EDGE_ATTRIBUTES);
                fprintf(fileToWrite, "n%u -> n%u [%s ", count, basicBlockID, EDGE_ATTRIBUTES);

                /* Check if the current edge is a backedge.
                 */
                isBackedge	= JITFALSE;
                item	= xanList_first(loops);
                while (item != NULL) {
                    circuit_t		*loop;
                    ir_instruction_t	*src;
                    ir_instruction_t	*dst;
                    IRBasicBlock		*srcBB;
                    IRBasicBlock		*dstBB;
                    loop	= item->data;
                    src	= IRMETHOD_getTheSourceOfTheCircuitBackedge(loop);
                    dst	= IRMETHOD_getTheDestinationOfTheCircuitBackedge(loop);
                    srcBB	= IRMETHOD_getBasicBlockContainingInstruction(method, src);
                    dstBB	= IRMETHOD_getBasicBlockContainingInstruction(method, dst);
                    if (	(srcBB->pos == count)		&&
                            (dstBB->pos == basicBlockID)	) {
                        isBackedge	= JITTRUE;
                        break;
                    }
                    item	= item->next;
                }

                /* Dump the edge.
                 */
                if (isBackedge) {
                    fprintf(fileToWriteAll, "color=red style=solid");
                    fprintf(fileToWrite, "color=red style=solid");
                }
                fprintf(fileToWriteAll, "];\n");
                fprintf(fileToWrite, "];\n");
                succ = IRMETHOD_getSuccessorInstruction(method, inst, succ);
            }
        }
    }
    fprintf(fileToWriteAll, "}\n");
    fprintf(fileToWrite, "}\n");

    /* Print the DDG			*/
    if (PRINT_DDG) {
        print_DDG(method, fileToWrite);
    }

    /* Print the CDG			*/
    if (PRINT_CDG) {
        print_CDG(method, fileToWrite);
    }

    /* Print the pre dominators tree	*/
    if (PRINT_PREDOMINATOR_TREE) {
        print_predominatortree(method, fileToWrite);
    }

    /* Print the post dominators tree	*/
    if (PRINT_POSTDOMINATOR_TREE) {
        print_postdominatortree(method, fileToWrite);
    }

    if (PRINT_LOOPS) {
        print_loops(method, fileToWrite);
    }

    /* Print the predecessors		*/
    /*for (count=0; count < IRMETHOD_getInstructionsNumber(method); count++){
            ir_instruction_t *inst2;
            inst	= IRMETHOD_getInstructionAtPosition(method, count);
            assert(inst != NULL);
            fprintf(fileToWrite, "Instruction %u\n", count);
            inst2	= IRMETHOD_getPredecessorInstruction(method, inst, NULL);
            while (inst2 != NULL){
                    fprintf(fileToWrite, "	Instruction %u\n", IRMETHOD_getInstructionPosition(inst2));
                    inst2	= IRMETHOD_getPredecessorInstruction(method, inst, inst2);
            }
       }*/

    /* Flush the output channel		*/
    fflush(fileToWrite);

    /* Close the file                       */
    fclose(fileToWriteAll);
    fclose(fileToWrite);
    fclose(fileToWriteSymbols);

    /* Free the memory			*/
    freeFunction(buf);
    xanList_destroyList(loops);

    return;
}

char * get_version (void) {
    return VERSION;
}

void print_ir_method_body (ir_method_t *method, FILE *fileToWrite) {
    JITUINT32 count;
    ir_instruction_t        *inst;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Print the nodes			*/
    for (count = 0; count <= IRMETHOD_getInstructionsNumber(method); count++) {
        fprintf(fileToWrite, "node [ %s label=\"", NODE_ATTRIBUTES);
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        print_instruction(method, inst, fileToWrite);
        fprintf(fileToWrite, "\" shape=box ] ;\n");
        fprintf(fileToWrite, "n%u ;\n", count);
    }

    /* Return				*/
    return;
}

void print_loops (ir_method_t *method, FILE *fileToWrite) {
    XanListItem     *item;
    JITINT32 found;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Print the loop bodies		*/
    item = xanList_first(method->loop);
    while (item != NULL) {
        circuit_t  *loop;
        loop = xanList_getData(item);
        assert(loop != NULL);

        /* Print the head of the graph		*/
        fprintf(fileToWrite, "digraph \"%s Loop %u\" {\n", internal_getMethodName(method), loop->loop_id);
        fprintf(fileToWrite, "label=\"Loop %u\";\n", loop->loop_id);

        /* Print the loop			*/
        print_loop_help(method, fileToWrite, loop);

        /* Print the foot of the graph		*/
        fprintf(fileToWrite, "}\n");

        item = item->next;
    }

    /* Print the loops nesting tree		*/
    fprintf(fileToWrite, "digraph \"%s Loops nesting tree\" {\n", internal_getMethodName(method));
    fprintf(fileToWrite, "label=\"Loops nesting tree\";\n");
    item = xanList_first(method->loop);
    found = JITFALSE;
    while (item != NULL) {
        circuit_t  *loop;
        loop = xanList_getData(item);
        assert(loop != NULL);
        fprintf(fileToWrite, "node [ %s label=\"Loop %u\" ] ;\n", NODE_ATTRIBUTES, loop->loop_id);
        fprintf(fileToWrite, "n%u ;\n", loop->loop_id);
        if (loop->loop_id == 0) {
            found = JITTRUE;
        }
        item = item->next;
    }
    if (!found) {
        fprintf(fileToWrite, "node [ %s label=\"Loop 0\" ] ;\n", NODE_ATTRIBUTES);
        fprintf(fileToWrite, "n0 ;\n");
    }
    print_loops_tree_help(method, fileToWrite, method->loopNestTree);
    fprintf(fileToWrite, "}\n");

    /* Return				*/
    return;
}

void print_loops_tree_help (ir_method_t *method, FILE *fileToWrite, XanNode *node) {
    XanNode         *child;
    circuit_t          *loop;
    circuit_t          *root_loop;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Check if there is the tree		*/
    if (node == NULL) {
        return;
    }

    /* Fetch the root instruction		*/
    root_loop = (circuit_t *) node->getData(node);
    assert(root_loop != NULL);

    child = node->getNextChildren(node, NULL);
    while (child != NULL) {

        /* Print the children edges		*/
        print_loops_tree_help(method, fileToWrite, child);

        /* Fetch the instruction		*/
        loop = (circuit_t *) child->getData(child);
        assert(loop != NULL);

        /* Print the edge			*/
        fprintf(fileToWrite, "n%u -> n%u ;\n", root_loop->loop_id, loop->loop_id);

        /* Get the next children		*/
        child = node->getNextChildren(node, child);
    }

    /* Return			*/
    return;
}

void print_loop_help (ir_method_t *method, FILE *fileToWrite, circuit_t *loop) {
    XanList         *instructions;
    XanListItem     *item;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);
    assert(loop != NULL);

    instructions = IRMETHOD_getCircuitInstructions(method, loop);
    item = xanList_first(instructions);
    while (item != NULL) {
        ir_instruction_t        *inst;

        inst = xanList_getData(item);
        fprintf(fileToWrite, "node [ %s ", NODE_ATTRIBUTES);
        if (xanList_find(loop->loopExits, inst) != NULL) {
            fprintf(fileToWrite, "color=blue ");
        } else {
            fprintf(fileToWrite, "color=black ");
        }
        fprintf(fileToWrite, "label=\"");
        if (loop->header_id == inst->ID) {
            fprintf(fileToWrite, "HEAD\\n");
        }
        print_instruction(method, inst, fileToWrite);
        fprintf(fileToWrite, "\" shape=box ] ;\n");
        fprintf(fileToWrite, "n%u ;\n", inst->ID);
        item = item->next;
    }

    item = xanList_first(instructions);
    while (item != NULL) {
        ir_instruction_t        *inst;
        ir_instruction_t        *succ;
        inst = xanList_getData(item);
        succ = IRMETHOD_getSuccessorInstruction(method, inst, NULL);
        while (succ != NULL) {
            if (xanList_find(instructions, succ) != NULL) {
                fprintf(fileToWrite, "n%u -> n%u ;\n", inst->ID, succ->ID);
            }
            succ = IRMETHOD_getSuccessorInstruction(method, inst, succ);
        }
        item = item->next;
    }

    /* Free the memory			*/
    xanList_destroyList(instructions);

    /* Return				*/
    return;
}

void print_postdominatortree (ir_method_t *method, FILE *fileToWrite) {

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Print the head of the graph		*/
    fprintf(fileToWrite, "digraph \"%s PostDominatorsTree\" {\n", IRMETHOD_getMethodName(method));
    fprintf(fileToWrite, "label=\"Post-Dominators Tree\";\n");

    /* Print the fakeHead node */
    fprintf(fileToWrite, "node [ %s label=\"", NODE_ATTRIBUTES);
    fprintf(fileToWrite, "fakeHead");
    fprintf(fileToWrite, "\" shape=box ] ;\n");
    fprintf(fileToWrite, "n%u ;\n", FAKEHEAD);

    /* Print the exit node */
    fprintf(fileToWrite, "node [ %s label=\"", NODE_ATTRIBUTES);
    fprintf(fileToWrite, "exit");
    fprintf(fileToWrite, "\" shape=box ] ;\n");
    fprintf(fileToWrite, "n%u ;\n", EXIT);

    /* Print the nodes			*/
    print_ir_method_body(method, fileToWrite);

    /* Print the edges			*/
    print_postdominatortree_help(method, fileToWrite, method->postDominatorTree);

    /* Print the foot of the graph		*/
    fprintf(fileToWrite, "}\n");

    /* Return				*/
    return;
}

void print_postdominatortree_help (ir_method_t *method, FILE *fileToWrite, XanNode *node) {
    XanNode                 *child;
    JITUINT32 root_inst_ID;
    JITUINT32 child_inst_ID;
    ir_instruction_t        *inst;
    ir_instruction_t        *root_inst;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Check if there is the tree		*/
    if (node == NULL) {
        return;
    }

    /* Fetch the root instruction		*/
    root_inst = (ir_instruction_t *) node->getData(node);
    if (root_inst != NULL) {
        root_inst_ID = IRMETHOD_getInstructionPosition(root_inst);
    } else {
        root_inst_ID = FAKEHEAD;
    }

    child = node->getNextChildren(node, NULL);
    while (child != NULL) {

        /* Print the children edges		*/
        print_postdominatortree_help(method, fileToWrite, child);

        /* Fetch the instruction		*/
        inst = (ir_instruction_t *) child->getData(child);
        assert(inst != NULL);
        child_inst_ID = IRMETHOD_getInstructionPosition(inst);

        /* Print the edge			*/
        fprintf(fileToWrite, "n%u -> n%u ;\n", root_inst_ID, child_inst_ID);

        /* Get the next children		*/
        child = node->getNextChildren(node, child);
    }

    /* Return			*/
    return;
}

void print_predominatortree (ir_method_t *method, FILE *fileToWrite) {

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Print the head of the graph		*/
    fprintf(fileToWrite, "digraph \"%s PreDominatorsTree\" {\n", internal_getMethodName(method));
    fprintf(fileToWrite, "label=\"Pre-Dominators Tree\"");

    /* Print the nodes			*/
    print_ir_method_body(method, fileToWrite);

    /* Print the edges			*/
    print_predominatortree_help(method, fileToWrite, method->preDominatorTree);

    /* Print the foot of the graph		*/
    fprintf(fileToWrite, "}\n");

    /* Return				*/
    return;
}

void print_predominatortree_help (ir_method_t *method, FILE *fileToWrite, XanNode *node) {
    XanNode                 *child;
    JITUINT32 root_inst_ID;
    JITUINT32 child_inst_ID;
    ir_instruction_t        *inst;
    ir_instruction_t        *root_inst;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Check if there is the tree		*/
    if (node == NULL) {
        return;
    }

    /* Fetch the root instruction		*/
    root_inst = (ir_instruction_t *) node->getData(node);
    assert(root_inst != NULL);
    root_inst_ID = IRMETHOD_getInstructionPosition(root_inst);

    child = node->getNextChildren(node, NULL);
    while (child != NULL) {

        /* Print the children edges		*/
        print_predominatortree_help(method, fileToWrite, child);

        /* Fetch the instruction		*/
        inst = (ir_instruction_t *) child->getData(child);
        assert(inst != NULL);
        child_inst_ID = IRMETHOD_getInstructionPosition(inst);

        /* Print the edge			*/
        fprintf(fileToWrite, "n%u -> n%u ;\n", root_inst_ID, child_inst_ID);

        /* Get the next children		*/
        child = node->getNextChildren(node, child);
    }

    /* Return			*/
    return;
}

void print_CDG (ir_method_t *method, FILE *fileToWrite) {
    JITINT32 count;
    JITUINT32 instNumber;
    ir_instruction_t        *temp;
    ir_instruction_t        *inst;
    XanListItem             *item;
    XanList                 *bb;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Allocate the list of basic block     *
    * printed                              */
    bb = xanList_new(allocFunction, freeFunction, NULL);
    assert(bb != NULL);

    /* Print the head of the graph		*/
    fprintf(fileToWrite, "digraph \"%s CDG\" {\n", internal_getMethodName(method));
    fprintf(fileToWrite, "label=\"Control Dependency Graph\"");

    /* Print the nodes			*/
    print_ir_method_body(method, fileToWrite);

    /* Print the edges	*/
    instNumber = IRMETHOD_getInstructionsNumber(method);
    for (count = 0; count < instNumber; count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (inst->metadata->controlDependencies != NULL) {
            item = xanList_first(inst->metadata->controlDependencies);
            while (item != NULL) {
                temp = xanList_getData(item);
                assert(temp != NULL);
                fprintf(fileToWrite, "n%u -> n%u ;\n", IRMETHOD_getInstructionPosition(temp), IRMETHOD_getInstructionPosition(inst));
                item = item->next;
            }
        }
    }

    fprintf(fileToWrite, "}\n");

    /* Free the memory                      */
    xanList_destroyList(bb);

    /* Return                               */
    return;
}

JITINT8 * internal_getMethodName (ir_method_t *method) {
    JITINT8 *name;

    name = IRMETHOD_getSignatureInString(method);
    if (name == NULL) {
        name = IRMETHOD_getMethodName(method);
        if (name == NULL) {
            name = (JITINT8 *) "UNKNOWN_METHOD";
        }
    }
    assert(name != NULL);

    return name;
}

void print_DDG (ir_method_t *method, FILE *fileToWrite) {
    JITUINT32 count;
    JITUINT32 startNode;
    JITUINT32 stopNode;
    ir_instruction_t        *inst;

    /* Assertions				*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    /* Print the head of the graph		*/
    fprintf(fileToWrite, "digraph \"%s DDG\" {\n", internal_getMethodName(method));
    fprintf(fileToWrite, "label=\"Data Dependency Graph\"");

    /* Compute the start and the stop node	*/
    startNode = IRMETHOD_getInstructionsNumber(method);
    stopNode = startNode + 1;

    /* Print the start node			*/
    fprintf(fileToWrite, "node [ %s shape=box label=\"START\" ] ;\n", NODE_ATTRIBUTES);
    fprintf(fileToWrite, "n%u ;\n", startNode);

    /* Print the stop node			*/
    fprintf(fileToWrite, "node [ %s shape=box label=\"STOP\" ] ;\n", NODE_ATTRIBUTES);
    fprintf(fileToWrite, "n%u ;\n", stopNode);

    /* Print the nodes			*/
    print_ir_method_body(method, fileToWrite);

    /* Print the edges			*/
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if ((inst->metadata->data_dependencies).dependsFrom != NULL) {
            print_DDG_dependences(method, fileToWrite, inst, DEP_RAW);
            print_DDG_dependences(method, fileToWrite, inst, DEP_WAR);
            print_DDG_dependences(method, fileToWrite, inst, DEP_WAW);
            print_DDG_dependences(method, fileToWrite, inst, DEP_MRAW);
            print_DDG_dependences(method, fileToWrite, inst, DEP_MWAR);
            print_DDG_dependences(method, fileToWrite, inst, DEP_MWAW);
        }
    }
    fprintf(fileToWrite, "}\n");

    /* Return				*/
    return;
}

void print_DDG_dependences (ir_method_t *method, FILE *fileToWrite, ir_instruction_t *inst, JITUINT16 depType) {
    XanListItem             *item;
    XanList                 *list;
    data_dep_arc_list_t     *dep;
    char                    *style;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);
    assert(inst != NULL);

    /* Print the edges		*/
    list = IRMETHOD_getDataDependencesFromInstruction(method, inst);
    item = xanList_first(list);
    while (item != NULL) {
        dep = (data_dep_arc_list_t *) xanList_getData(item);
        assert(dep != NULL);
        assert(dep->inst != NULL);
        if ((dep->depType & depType) != 0) { //Is the current type the one we want to print?
            if ((depType >= DEP_FIRSTMEMORY) != 0) {
                style = "dotted";
            } else {
                style = "solid";
            }
            assert(style != NULL);
            switch (depType) {
                case DEP_RAW:
                case DEP_MRAW:
                    fprintf(fileToWrite, "edge [ %s, color=blue, style=\"%s\" label=\"RAW\"]\n", EDGE_ATTRIBUTES, style);
                    break;
                case DEP_WAR:
                case DEP_MWAR:
                    fprintf(fileToWrite, "edge [ %s, color=red, style=\"%s\" label=\"WAR\"]\n", EDGE_ATTRIBUTES, style);
                    break;
                case DEP_WAW:
                case DEP_MWAW:
                    fprintf(fileToWrite, "edge [ %s, color=green, style=\"%s\" label=\"WAW\"]\n", EDGE_ATTRIBUTES, style);
                    break;
            }
            fprintf(fileToWrite, "n%u -> n%u ;\n", IRMETHOD_getInstructionPosition(inst), IRMETHOD_getInstructionPosition(dep->inst));
        }
        item = item->next;
    }

    /* Free the memory.
     */
    xanList_destroyList(list);

    return;
}

void print_liveness_IN (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    fprintf(fileToWrite, "LIVENESS=");
    for (count = 0; count < IRMETHOD_getNumberOfVariablesNeededByMethod(method); count++) {
        if (IRMETHOD_isVariableLiveIN(method, inst, count)) {
            fprintf(fileToWrite, "%u ", count);
        }
    }
    fprintf(fileToWrite, "\\n");
}

void print_liveness_OUT (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    fprintf(fileToWrite, "\\nLIVENESS=");
    for (count = 0; count < IRMETHOD_getNumberOfVariablesNeededByMethod(method); count++) {
        if (IRMETHOD_isVariableLiveOUT(method, inst, count)) {
            fprintf(fileToWrite, "%u ", count);
        }
    }
    fprintf(fileToWrite, "\\n");
}

void print_available_expressions_IN (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    fprintf(fileToWrite, "AE=");
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        if (ir_instr_available_expressions_available_in_is(method, inst, count)) {
            fprintf(fileToWrite, "%u ", count);
        }
    }
    fprintf(fileToWrite, "\\n");
}

void print_available_expressions_OUT (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);

    fprintf(fileToWrite, "\\nAE=");
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        if (ir_instr_available_expressions_available_out_is(method, inst, count)) {
            fprintf(fileToWrite, "%u ", count);
        }
    }
    fprintf(fileToWrite, "\\n");
}

void print_reaching_definitions_OUT (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);
    assert(inst != NULL);

    fprintf(fileToWrite, "\\nRD=");
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        if (ir_instr_reaching_definitions_reached_out_is(method, inst, count)) {
            fprintf(fileToWrite, "%u ", count);
        }
    }
    fprintf(fileToWrite, "\\n");
}

void print_reaching_definitions_IN (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions			*/
    assert(method != NULL);
    assert(fileToWrite != NULL);
    assert(inst != NULL);

    fprintf(fileToWrite, "RD=");
    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        if (ir_instr_reaching_definitions_reached_in_is(method, inst, count)) {
            fprintf(fileToWrite, "%u ", count);
        }
    }
    fprintf(fileToWrite, "\\n");
}

void print_instruction (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 instType;

    /* Assertions		*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(fileToWrite != NULL);

    /* Print the reaching definitions	*/
    if (PRINT_REACHING_DEFINITIONS) {
        print_reaching_definitions_IN(method, inst, fileToWrite);
    }

    /* Print the available expressions	*/
    if (PRINT_AVAILABLE_EXPRESSIONS) {
        print_available_expressions_IN(method, inst, fileToWrite);
    }

    /* Print the liveness			*/
    if (PRINT_LIVENESS) {
        print_liveness_IN(method, inst, fileToWrite);
    }

    /* Fetch the type of the*
     * instruction		*/
    instType = IRMETHOD_getInstructionType(inst);
    fprintf(fileToWrite, "%d) ", IRMETHOD_getInstructionPosition(inst));

    /* Print the volatile flag.
     */
    if (IRMETHOD_isInstructionVolatile(inst)) {
        fprintf(fileToWrite, "VOLATILE ");
    }

    /* Print the result	*/
    dotprinter_printResult(method, inst, fileToWrite);

    /* Print the instruction*/
    fprintf(fileToWrite, "%s(", IRMETHOD_getInstructionTypeName(instType));
    printParameters(method, inst, fileToWrite);
    printCallParameters(method, inst, fileToWrite);
    fprintf(fileToWrite, ")");

    /* Print the reaching definitions	*/
    if (PRINT_REACHING_DEFINITIONS) {
        print_reaching_definitions_OUT(method, inst, fileToWrite);
    }

    /* Print the available expressions	*/
    if (PRINT_AVAILABLE_EXPRESSIONS) {
        print_available_expressions_OUT(method, inst, fileToWrite);
    }

    /* Print the liveness			*/
    if (PRINT_LIVENESS) {
        print_liveness_OUT(method, inst, fileToWrite);
    }

    /* Print the execution probability	*/
    if (PRINT_EXECUTION_PROBABILITY) {
        fprintf(fileToWrite, "\\nExecutionProbability = %.5f ", inst->executionProbability);
    }
}

void dotprinter_printResult (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    ir_item_t       *item;

    /* Assertions		*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(fileToWrite != NULL);

    item = IRMETHOD_getInstructionResult(inst);
    if (    (item->type != NOPARAM)         &&
            (item->type != IRVOID)          ) {
        printParameter(method, item, fileToWrite);
        fprintf(fileToWrite, " = ");
    }
}

void printCallParameters (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {
    JITUINT32 count;

    /* Assertions		*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(fileToWrite != NULL);

    if (IRMETHOD_getInstructionCallParametersNumber(inst) > 0) {
        // call parameters on new line only if the instruction is not a phi
        if (IRMETHOD_getInstructionType(inst) != IRPHI) {
            fprintf(fileToWrite, "\\n");
        }
        fprintf(fileToWrite, " Call parameters: ");
        for (count = 0; count < IRMETHOD_getInstructionCallParametersNumber(inst); count++) {
            printParameter(method, IRMETHOD_getInstructionCallParameter(inst, count), fileToWrite);
            if (count != (IRMETHOD_getInstructionCallParametersNumber(inst) - 1)) {
                fprintf(fileToWrite, ", ");
            }
        }
    }
}

void printParameter (ir_method_t *method, ir_item_t *item, FILE *fileToWrite) {

    /* Dump the IR item		*/
    IRMETHOD_dumpIRItem(method, item, fileToWrite);

    /* Dump the CIL type		*/
    if (item->type_infos != NULL) {
        JITINT8         *className;
        className = IRMETHOD_getClassName(item->type_infos);
        assert(className != NULL);
        fprintf(fileToWrite, "[%s]", className);
    }

    /* Return			*/
    return;
}

void printParameters (ir_method_t *method, ir_instruction_t *inst, FILE *fileToWrite) {

    /* Assertions		*/
    assert(method != NULL);
    assert(inst != NULL);
    assert(fileToWrite != NULL);

    switch (IRMETHOD_getInstructionParametersNumber(inst)) {
        case 3:
            printParameter(method, IRMETHOD_getInstructionParameter1(inst), fileToWrite);
            fprintf(fileToWrite, ", ");
            printParameter(method, IRMETHOD_getInstructionParameter2(inst), fileToWrite);
            fprintf(fileToWrite, ", ");
            printParameter(method, IRMETHOD_getInstructionParameter3(inst), fileToWrite);
            break;
        case 2:
            printParameter(method, IRMETHOD_getInstructionParameter1(inst), fileToWrite);
            fprintf(fileToWrite, ", ");
            printParameter(method, IRMETHOD_getInstructionParameter2(inst), fileToWrite);
            break;
        case 1:
            printParameter(method, IRMETHOD_getInstructionParameter1(inst), fileToWrite);
            break;
        case 0:
            break;
        default:
            abort();
    }
    fprintf(fileToWrite, " ");
}

JITUINT32 findBasicBlockID (ir_method_t *method, JITUINT32 instID) {
    JITUINT32 count;
    JITUINT32 basicBlocksNumber;
    char buf[1024];

    /* Assertions		*/
    assert(method != NULL);
    assert(instID <= IRMETHOD_getInstructionsNumber(method));

    basicBlocksNumber = IRMETHOD_getNumberOfMethodBasicBlocks(method);
    for (count = 0; count < basicBlocksNumber; count++) {
        IRBasicBlock      *basicBlock;
        basicBlock = IRMETHOD_getBasicBlockAtPosition(method, count);
        assert(basicBlock != NULL);
        if (    (instID >= basicBlock->startInst)       &&
                (instID <= basicBlock->endInst)         ) {
            return count;
        }
    }

    SNPRINTF(buf, sizeof(char) * 1024, "METHOD PRINTER: ERROR = There isn't a basic block for the instruction %u\n", instID);
    PRINT_ERR(buf, 0);
    abort();
    return 0;
}

JITUINT32 findLabel (ir_method_t *method, JITUINT32 labelID) {
    JITUINT32 count;
    ir_instruction_t        *inst;
    char buf[1024];

    /* Assertions		*/
    assert(method != NULL);

    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (    (IRMETHOD_getInstructionType(inst) == IRLABEL)          &&
                (IRMETHOD_getInstructionParameter1Value(inst) == labelID)                       ) {
            assert(count < IRMETHOD_getInstructionsNumber(method));
            return count;
        }
    }

    SNPRINTF(buf, sizeof(char) * 1024, "METHOD PRINTER: ERROR = There isn't the label ID %u\n", labelID);
    PRINT_ERR(buf, 0);
    abort();
}

JITUINT32 findCatcher (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;

    /* Assertions		*/
    assert(method != NULL);

    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (IRMETHOD_getInstructionType(inst) == IRSTARTCATCHER) {
            return count;
        }
    }
    return IRMETHOD_getInstructionsNumber(method) + 1;
}

JITUINT32 findFilter (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;

    /* Assertions		*/
    assert(method != NULL);

    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (IRMETHOD_getInstructionType(inst) == IRSTARTFILTER) {
            return count;
        }
    }

    abort();
}

JITUINT32 findFinally (ir_method_t *method) {
    JITUINT32 count;
    ir_instruction_t        *inst;

    /* Assertions		*/
    assert(method != NULL);

    for (count = 0; count < IRMETHOD_getInstructionsNumber(method); count++) {
        inst = IRMETHOD_getInstructionAtPosition(method, count);
        assert(inst != NULL);
        if (IRMETHOD_getInstructionType(inst) == IRSTARTFINALLY) {
            return count;
        }
    }

    abort();
}

char * get_informations (void) {
    return INFORMATIONS;
}

char * get_author (void) {
    return AUTHOR;
}

void internal_print_start_node (FILE *fileToWrite, ir_method_t *method) {
    JITINT8                 *className;
    JITINT8                 *name;

    /* Assertions                   */
    assert(fileToWrite != NULL);
    assert(method != NULL);

    /* Print the header             */
    name = internal_getMethodName(method);
    assert(name != NULL);
    fprintf(fileToWrite, "node [ %s label=\"START\\n%s \\n", NODE_ATTRIBUTES, name);

    /* Print the locals             */
    XanList*     locals = method->locals;
    XanListItem* iterator = xanList_first(locals);
    while (iterator != NULL) {
        ir_local_t* local = xanList_getData(iterator);
        if (local != NULL) {
            fprintf(fileToWrite, "var %d", local->var_number);
            if (PRINT_BYTECODE_CLASS_NAMES) {
                TypeDescriptor* classID = local->type_info;
                if (classID == NULL) {
                    fprintf(fileToWrite, ": ??\\n");
                } else {
                    className = IRMETHOD_getClassName(classID);
                    assert(className != NULL);
                    fprintf(fileToWrite, ": %s\\n", className);
                }
            }
        } else {
            fprintf(fileToWrite, "this local is NULL!\\n");
        }
        iterator = iterator->next;
    }
    /* NOTE: old loop for printing locals of the method, this does not work if method locals are not in the first variables of the method, as in the case the method has some call parameters
       for (count = 0; count < IRMETHOD_getMethodLocalsNumber(method); count++) {
            TypeDescriptor  *classID;
            fprintf(fileToWrite, "var %d: ", count);
            if (PRINT_BYTECODE_CLASS_NAMES) {
                    ir_local_t      *local;
                    classID = NULL;
                    local = IRMETHOD_getLocalVariableOfMethod(method, count);
                    if (local != NULL) {
                            classID = local->type_info;
                    }
                    if (classID == 0) {
                            fprintf(fileToWrite, "??\\n");
                    } else {
                            assert(classID != 0);
                            className = IRMETHOD_getClassName(classID);
                            assert(className != NULL);
                            fprintf(fileToWrite, "%s\\n", className);
                    }
            }
       }
     */
    fprintf(fileToWrite, "\" ] ;\n");
}

static inline void compute_method_information (ir_method_t *method) {

    /* Assertions		*/
    assert(method != NULL);

    /* Compute the reaching definitions		*/
    if (PRINT_REACHING_DEFINITIONS) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, REACHING_DEFINITIONS_ANALYZER);
    }

    /* Compute the liveness of variables		*/
    if (PRINT_LIVENESS) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, LIVENESS_ANALYZER);
    }

    /* Compute the available expressions		*/
    if (PRINT_AVAILABLE_EXPRESSIONS) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, AVAILABLE_EXPRESSIONS_ANALYZER);
    }

    /* Compute the loops				*/
    if (PRINT_LOOPS) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, LOOP_IDENTIFICATION);
    }

    /* Compute the post-domination information	*/
    if (PRINT_POSTDOMINATOR_TREE) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, POST_DOMINATOR_COMPUTER);
    }

    /* Compute the pre-domination information	*/
    if (PRINT_PREDOMINATOR_TREE) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, PRE_DOMINATOR_COMPUTER);
    }

    /* Compute the Control Dependence Graph		*/
    if (PRINT_CDG) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, CDG_COMPUTER);
    }

    /* Compute the Data Dependence Graph		*/
    if (PRINT_DDG) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, DDG_COMPUTER);
    }

    /* Compute the execution probability		*/
    if (PRINT_EXECUTION_PROBABILITY) {
        IROPTIMIZER_callMethodOptimization(irOptimizer, method, BRANCH_PREDICTOR);
    }

    return;
}
