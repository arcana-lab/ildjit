/*
 * Copyright (C) 2008 - 2012  Simone Campanoni, Michele Tartara
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
#include <jitsystem.h>
#include <ir_method.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>

// My headers
#include <optimizer_basicblockidentifier.h>
#include <config.h>
// End

#define INFORMATIONS    "This step identify the basic block of the method"
#define AUTHOR          "Simone Campanoni, Michele Tartara"

static inline JITUINT64 get_ID_job (void);
static inline void do_job (ir_method_t *);
static inline char * get_version (void);
static inline char * get_informations (void);
static inline char * get_author (void);
static inline JITUINT64 get_dependences (void);
static inline JITUINT64 get_invalidations (void);
static inline void buildBasicBlock (ir_method_t *);
static inline void bb_init (ir_lib_t *irLib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline void bb_shutdown (JITFLOAT32 totalTime);
static inline IRBasicBlock* startBasicBlock (ir_method_t*, JITUINT32);
static inline void endBasicBlock (ir_method_t *method, IRBasicBlock* basicBlock, JITUINT32 endID);
static inline void bb_getCompilationTime (char *buffer, JITINT32 bufferLength);
static inline void bb_getCompilationFlags (char *buffer, JITINT32 bufferLength);

ir_optimization_interface_t plugin_interface = {
    get_ID_job,
    get_dependences,
    bb_init,
    bb_shutdown,
    do_job,
    get_version,
    get_informations,
    get_author,
    get_invalidations,
    NULL,
    bb_getCompilationFlags,
    bb_getCompilationTime
};

static inline void bb_init (ir_lib_t *irLib, ir_optimizer_t *optimizer, char *outputPrefix) {

    /* Assertions			*/
    assert(irLib != NULL);
    assert(optimizer != NULL);

}

static inline void bb_shutdown (JITFLOAT32 totalTime) {

}

static inline JITUINT64 get_ID_job (void) {
    return BASIC_BLOCK_IDENTIFIER;
}

static inline JITUINT64 get_dependences (void) {
    return 0;
}

static inline JITUINT64 get_invalidations (void) {
    return INVALIDATE_NONE;
}

static inline void do_job (ir_method_t *method) {

    /* Assertions				*/
    assert(method != NULL);

    /* Build the basic block		*/
    buildBasicBlock(method);

    /* Return				*/
    return ;
}

static inline void buildBasicBlock (ir_method_t *method) {
    JITUINT32 instID;
    ir_instruction_t        *inst;
    IRBasicBlock              *basicBlock;
    JITUINT32 instructionsNumber;

    /* Assertions                   */
    assert(method != NULL);

    /* Variables Initialization     */
    instID = 0;
    inst = NULL;
    basicBlock = NULL;

    /* Remove old basic blocks */
    IRMETHOD_deleteBasicBlocksInformation(method);

    /* Fetch the instructions number		*/
    instructionsNumber = IRMETHOD_getInstructionsNumber(method);

    /* Check if there are instructions in the method */
    if (instructionsNumber == 0) {
        return;
    }

    /* Start the first basic block.
     */
    basicBlock = startBasicBlock(method, 0);

    /* Scan each instruction to build basic blocks.
     */
    for (instID = 0; instID <= instructionsNumber; instID++) {

        /* Fetch the instructions       */
        PDEBUG("BASIC BLOCK BUILDER: Fetched instruction at %d\n", instID);
        inst = IRMETHOD_getInstructionAtPosition(method, instID);
        assert(inst != NULL);

        /* Check if I have to close     *
         * the current basic block       */
        if (    ((inst->type == IRLABEL) && (basicBlock->startInst != instID))	||
                (inst->type == IRSTARTCATCHER)                                  ||
                (inst->type == IRSTARTFILTER)                                   ||
                (inst->type == IRSTARTFINALLY)                                  ||
                (inst->type == IREXITNODE)                                      ) {

            /* Close the current basic block        */
            assert((instID - 1) >= 0);
            if ((instID - 1) >= basicBlock->startInst) {
                endBasicBlock(method, basicBlock, instID - 1);

                /* Start the new basicBlock             */
                basicBlock = startBasicBlock(method, instID);
            }

            /* Check if the current instruction ends the current basic block	*/
        } else if (	(IRMETHOD_mayHaveNotFallThroughInstructionAsSuccessor(method, inst))	||
                    (inst->type == IREXIT)							) {
            endBasicBlock(method, basicBlock, instID);

            /* Start the new basicBlock             */
            basicBlock = NULL;
            if (    (instID + 1) <= instructionsNumber) {
                basicBlock = startBasicBlock(method, instID + 1);
            }
        }
    }
    if (basicBlock != NULL) {
        endBasicBlock(method, basicBlock, instructionsNumber);
        basicBlock = NULL;
    }
    assert(basicBlock == NULL);

    /* Check the basic blocks	*/
#ifdef DEBUG
    for (instID = 0; instID < IRMETHOD_getNumberOfMethodBasicBlocks(method); instID++) {
        IRBasicBlock      *basicBlock;
        basicBlock = IRMETHOD_getBasicBlockAtPosition(method, instID);
        assert(basicBlock != NULL);
        assert(basicBlock->startInst >= 0);
        assert(basicBlock->startInst <= instructionsNumber);
        assert(basicBlock->endInst >= 0);
        assert(basicBlock->endInst <= instructionsNumber);
    }
#endif

    /* Return			*/
    return;
}

static inline char * get_version (void) {
    return VERSION;
}

static inline char * get_informations (void) {
    return INFORMATIONS;
}

static inline char * get_author (void) {
    return AUTHOR;
}

static inline IRBasicBlock* startBasicBlock (ir_method_t *method, JITUINT32 startID) {
    IRBasicBlock* basicBlock;

    /* Assertions				*/
    assert(method != NULL);

    basicBlock = IRMETHOD_newBasicBlock(method);
    basicBlock->startInst = startID;
    basicBlock->endInst = -1;

    /* Return				*/
    return basicBlock;
}

static inline void endBasicBlock (ir_method_t *method, IRBasicBlock* basicBlock, JITUINT32 endID) {
    JITUINT32 	count;

    /* Assertions.
     */
    assert(basicBlock != NULL);
    assert(method->instructionsBasicBlocksMap != NULL);

    /* Set the end of the basic block.
     */
    basicBlock->endInst = endID;
    assert(basicBlock->startInst <= basicBlock->endInst);

    /* Fill up the mapping instructions -> basic blocks.
     */
    for (count=basicBlock->startInst; count <= endID; count++) {
        method->instructionsBasicBlocksMap[count]	= basicBlock->pos;
    }

    return ;
}

static inline void bb_getCompilationFlags (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, " ");
#ifdef DEBUG
    strncat(buffer, "DEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PRINTDEBUG
    strncat(buffer, "PRINTDEBUG ", sizeof(char) * bufferLength);
#endif
#ifdef PROFILE
    strncat(buffer, "PROFILE ", sizeof(char) * bufferLength);
#endif
}

static inline void bb_getCompilationTime (char *buffer, JITINT32 bufferLength) {

    /* Assertions				*/
    assert(buffer != NULL);

    snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);

    /* Return				*/
    return;
}
