/*
 * Copyright (C) 2007 - 2010  Campanoni Simone, Michele Melchiori
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
#include <ir_optimization_interface.h>
#include <ir_language.h>
#include <iljit-utils.h>
#include <compiler_memory_manager.h>

// My headers
#include <optimizer_ssa_back_convert.h>
#include <config.h>
// End

#define INFORMATIONS                   "Plugin for the conversion of the program from SSA form to normal form"
#define AUTHOR                         "Michele Melchiori"
#define JOB                            SSA_BACK_CONVERTER
#define SSA_BACK_CONVERT_DEPENDENCIES  BASIC_BLOCK_IDENTIFIER
#define SSA_BACK_CONVERT_INVALIDATIONS INVALIDATE_ALL & ~ BASIC_BLOCK_IDENTIFIER


static inline JITUINT64 ssa_back_convert_get_ID_job (void);
static inline char *    ssa_back_convert_get_version (void);
static inline void      ssa_back_convert_do_job (ir_method_t * method);
static inline char *    ssa_back_convert_get_informations (void);
static inline char *    ssa_back_convert_get_author (void);
static inline JITUINT64 ssa_back_convert_get_dependences (void);
static inline void      ssa_back_convert_shutdown (void);
static inline void      ssa_back_convert_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 ssa_back_convert_get_invalidations (void);

/* custom functions */
#ifdef DEBUG
static        void      ssa_back_convert_checkUninitializedValues(ir_method_t* method);
static        void      ssa_back_convert_checkBasicBlocks(ir_method_t* method);
#endif

ir_lib_t       *irLib       = NULL;
ir_optimizer_t *irOptimizer = NULL;
char           *prefix      = NULL;

ir_optimization_interface_t plugin_interface = {
	ssa_back_convert_get_ID_job,
	ssa_back_convert_get_dependences,
	ssa_back_convert_init,
	ssa_back_convert_shutdown,
	ssa_back_convert_do_job,
	ssa_back_convert_get_version,
	ssa_back_convert_get_informations,
	ssa_back_convert_get_author,
	ssa_back_convert_get_invalidations
};

static inline void ssa_back_convert_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix) {
	/* Assertions			*/
	assert(lib          != NULL);
	assert(optimizer    != NULL);
	assert(outputPrefix != NULL);
	
	irLib       = lib;
	irOptimizer = optimizer;
	prefix      = outputPrefix;
}

static inline void ssa_back_convert_shutdown (void) {
	irLib       = NULL;
	irOptimizer = NULL;
	prefix      = NULL;
}

static inline JITUINT64 ssa_back_convert_get_ID_job (void) {
	return JOB;
}

static inline JITUINT64 ssa_back_convert_get_dependences (void) {
	return SSA_BACK_CONVERT_DEPENDENCIES;
}

JITINT8* ssa_back_convert_getMethodName(ir_method_t* method) {
	JITINT8* name;
	name = IRMETHOD_getSignatureInString(method);
	if (name == NULL) name = IRMETHOD_getMethodName(method);
	return name;
}

/**
 * @brief it updates the phi functions parameters list respecting new orders
 *
 * When the deletion of one phi function needs the creation of one new basic block
 * between one of the predecessors and the basic block containing the phi, it is
 * possibile that the predecessors order changes for the basic block of the phi.
 * If in that basic block there are more than one phi we have that the order of call
 * parameters of all these phis are not right because the last changes in predecessors
 * order. This function restore the truth of this condition.
 * 
 * @param method pointer to \c ir_method_t structure containing method informations
 * @param phi    pointer to the phi instruction that has generated the creation of one new basic block
 * @param oldPos position of the parameters to move
 * @param newPos new position for the parameters to be moved
 */
static void ssa_back_convert_updatePhiParametersList(ir_method_t*      method,
                                                     t_ir_instruction* phi,
                                                     JITUINT32         oldPos,
                                                     JITUINT32         newPos) {
	/* assertions */
	assert(method != NULL);
	assert(phi != NULL);
	assert(oldPos >= 0);
	assert(newPos >= 0);
	
	BasicBlock*       bb   = IRMETHOD_getTheBasicBlockIncludesInstruction(method, phi);
	t_ir_instruction* inst = IRMETHOD_getInstructionAtPosition(method, bb->startInst);
	while (inst->ID <= bb->endInst) {
		if (IRMETHOD_getInstructionType(inst) == IRPHI)
			if (inst != phi)
				IRMETHOD_moveInstructionCallParameter(method, inst, oldPos, newPos);
		
		inst = IRMETHOD_getNextInstruction(method, inst);
	}
}

#if OPTIMIZER_SSA_BACK_CONVERT_REMOVE_USELESS_CODE == JITTRUE
static inline void ssa_back_convert_removeUselessCode(ir_method_t* method) {
	// NOTE: before terminating this optimization we make one run of the copy propagation plugin, because
	// the back converter may create one great number of almost useless variables and the register allocator
	// in this situation may go crazy.
	
// 	while (1) {
		/* Fetch the number of IR instructions */
// 		JITUINT32 instructionsNumber = IRMETHOD_getInstructionsNumber(method);
// 		JITUINT32 vars               = IRMETHOD_getNumberOfVariablesNeededByMethod(method);
// 		method->modified = JITFALSE;
		
// 		IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, CONSTANT_PROPAGATOR);
		IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, COPY_PROPAGATOR);
		IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, DEADCODE_ELIMINATOR);
// 		IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, NULL_CHECK_REMOVER);
// 		IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, ALGEBRAIC_SIMPLIFICATION);
		
		/* Check the exit condition */
// 		instructionsNumber -= IRMETHOD_getInstructionsNumber(method);
// 		vars               -= IRMETHOD_getNumberOfVariablesNeededByMethod(method);
// 		if ((abs(instructionsNumber) + abs(vars) == 0) && (!method->modified)) return;
// 	}
}
#endif

/**
 * @brief Update the basic block boundaries after the insert of one new store in the method
 *
 * When one \c STORE is inserted in one basic block it is a waste of time recalculate from the beginning
 * all the basic block boundaries: is sufficient to "move forward" one instruction all boundaries that are
 * after the instruction just inserted.
 */
static inline void ssa_back_convert_increase_BasicBlock_boundaries(ir_method_t* method, JITUINT32 fromInstr) {
	JITUINT32 numBB = IRMETHOD_getNumberOfMethodBasicBlocks(method);
	JITUINT32 count = 0;
	for (count = 0; count < numBB; count++) {
		BasicBlock* bb = IRMETHOD_getBasicBlock(method, count);
		// test one particular case
		if (bb->startInst == bb->endInst && bb->endInst == fromInstr) bb->endInst++;
		else if (bb->endInst >= fromInstr) {
			bb->endInst++;
			if (bb->startInst >= fromInstr) {
				bb->startInst++;
			}
		}
	}
}

/**
 * @brief Place one store before the instruction passed as second parameter
 *
 * This method simply duplicates the value of the register described by \c secondParam into the register \c firstParam.
 * This is made by placing one store instruction just before the instruction pointed by \c instr.
 *
 * @param  method      pointer to \c ir_method_t structure that keeps method informations
 * @param  instr       pointer to the instruction that must be the next one of the store we have to create
 * @param  firstParam  the number of the register where we want to put the value moved by the store
 * @param  secondParam pointer to \c ir_item_t structure describing the source value of the store (i.e. its second parameter)
 * @return a pointer to the newly allocated instruction
 */
static inline t_ir_instruction* ssa_back_convert_placeStoreBeforeInstruction(ir_method_t* method, t_ir_instruction* instr, IR_ITEM_VALUE firstParam, ir_item_t* secondParam) {
	t_ir_instruction* newInst = IRMETHOD_newInstructionBefore(method, instr);
	IRMETHOD_setInstructionType(method, newInst, IRSTORE);
	IRMETHOD_cpInstructionParameter1(newInst, secondParam);
	IRMETHOD_cpInstructionParameter2(newInst, secondParam);
	IRMETHOD_setInstructionParameter1Value(method, newInst, firstParam);
	method->setInstrPar1Type(newInst, IROFFSET);
	return newInst;
}

/**
 * @brief Place a store instruction immediately after inst and before the label newLabelID
 *
 * Look if the instruction \c inst is a jump instruction, i.e. one instruction that may
 * move the execution in another place in the code instead the next instruction.
 * If yes it places the store instruction before \c inst, otherwise means that the only
 * successor is the next instruction, so it places the store just after \c inst.
 *
 * @param method     pointer to \c ir_method_t structure that keeps method informations
 * @param newVar     the first parameter of the move we have to create
 * @param callParam  the second parameter of the move we have to create
 * @param inst       the instruction at the end of the basic block we have to place the new \c store instruction
 * 
 * @return the new predecessor that in the future replaces \c inst or \c NULL if the predecessor is not changed
 */
static inline t_ir_instruction* ssa_back_convert_placeMoveInstr(ir_method_t*      method,
                                                                IR_ITEM_VALUE     newVar,
                                                                ir_item_t*        callParam,
                                                                t_ir_instruction* inst) {
	assert(method != NULL);
	assert(callParam != NULL);
	assert(inst != NULL);
	assert(newVar >= 0);
	
	t_ir_instruction* newInst;
	
	if (IRMETHOD_isAJumpInstruction(method, inst)) {
		// we have to place the store instruction before 'inst'
		newInst = ssa_back_convert_placeStoreBeforeInstruction(method, inst, newVar, callParam);
		ssa_back_convert_increase_BasicBlock_boundaries(method, IRMETHOD_getInstructionPosition(newInst));
	} else {
		// the only predecessor that ends with an instruction that is not a jump is the
		// previous instruction. Here we don't have to place the store before 'inst' but
		// exactly after it
		inst = IRMETHOD_getNextInstruction(method, inst);
		newInst = ssa_back_convert_placeStoreBeforeInstruction(method, inst, newVar, callParam);
		ssa_back_convert_increase_BasicBlock_boundaries(method, IRMETHOD_getInstructionPosition(newInst)-1);
	}
	return NULL;
	
#if 0
	/* NOTE: this piece of code may be useful in a future version of the plugin *
	 * in which we can save some variable IDs: with some little modifications   *
	 * it is able to place the store instruction needed for the back-conversion *
	 * exactly on the edge of the CFG, without breaking other parts of the      *
	 * code. Note also that this is able to work only if the target basic block *
	 * is not headed by a start_catcher instruction: in fact in this situation  *
	 * the plugin is not able to place the store instruction between the        *
	 * instruction that may throw the exception and the start of the catching   *
	 * block.                                                                   */
	
	IR_ITEM_VALUE     myLabelID;
	t_ir_instruction* nextInst;
	JITUINT32         instrType = IRMETHOD_getInstructionType(inst);
	
	switch (instrType) {
		case IRCALLFINALLY:
		case IRCALLFILTER:
			// all these instructions have the target label as first parameter, and may be treated (I think...)
			// like as if they were IRBRANCH or IRTHROW
// 			myLabelID = IRMETHOD_newLabelID(method);
// 			method->setInstrPar1Value(inst, myLabelID); // redirect the branch
// 			break;
			
		case IRTHROW:
		case IRBRANCH:
			// this is the case in which the instruction is one unconditional branch
			// so we can put the STORE instruction directly before this branch
			newInst = ssa_back_convert_placeStoreBeforeInstruction(method, inst, newVar, callParam);
			// move after of one step the last instruction of this basic block
			// and all successors
			ssa_back_convert_increase_BasicBlock_boundaries(method, IRMETHOD_getInstructionPosition(newInst));
			return NULL;
			
		case IRBRANCHIFPCNOTINRANGE:
			// this instruction has the target label as third parameter
			// this is a conditional branch, and if the target label is placed at the next instruction
			// we have only one successor instead of two
			nextInst = IRMETHOD_getNextInstruction(method, inst);
			if (IRMETHOD_getInstructionType(nextInst) == IRLABEL) { // the next instruction is a label
				if (method->getInstrPar1Value(nextInst) == method->getInstrPar3Value(inst)) {
					// the target of the branch is the next instruction, this conditional branch
					// has only one successor. In this case we act like this was an unconditional jump
					newInst = ssa_back_convert_placeStoreBeforeInstruction(method, inst, newVar, callParam);
					// move after of one step the last instruction of this basic block
					// and all successors
					ssa_back_convert_increase_BasicBlock_boundaries(method, IRMETHOD_getInstructionPosition(newInst));
					return NULL;
				}
				if (method->getInstrPar1Value(nextInst) == newLabelID) {
					// the label newLabelID is not the target of the branch, but is the next instruction
					// so we can make the new store the next instruction of the branch before the label
					newInst = ssa_back_convert_placeStoreBeforeInstruction(method, nextInst, newVar, callParam);
					
					BasicBlock* bb = IRMETHOD_newBasicBlock(method);
					bb->startInst = IRMETHOD_getInstructionPosition(newInst);
					bb->endInst = bb->startInst;
					// now increase by 1 all the basic block boundaries that are after newInst
					ssa_back_convert_increase_BasicBlock_boundaries(method, newInst->ID +1);
					return NULL;
				}
			}
			if (method->getInstrPar3Value(inst) == newLabelID) {
				// the target of the branch goes to newLabelID
				myLabelID = IRMETHOD_newLabelID(method);
				method->setInstrPar3Value(inst, myLabelID); // redirect the branch
				break;
			}
			fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: one status condition of IRBRANCHIFPCNOTINRANGE is not checked. Aborting.\n");
			abort();
			
		case IRBRANCHIF:
		case IRBRANCHIFNOT:
			// all these instructions are conditional branches and have the target label as second parameter
			// if the target label is placed at the next instruction we have only one successor instead of two
			nextInst = IRMETHOD_getNextInstruction(method, inst);
			if (IRMETHOD_getInstructionType(nextInst) == IRLABEL) { // the next instruction is a label
				if (method->getInstrPar1Value(nextInst) == method->getInstrPar2Value(inst)) {
					// the target of the branch is the next instruction, this conditional branch
					// has only one successor. In this case we act like this was an unconditional jump
					newInst = ssa_back_convert_placeStoreBeforeInstruction(method, inst, newVar, callParam);
					ssa_back_convert_increase_BasicBlock_boundaries(method, IRMETHOD_getInstructionPosition(newInst));
					return NULL;
				}
				if (method->getInstrPar1Value(nextInst) == newLabelID) {
					// the label newLabelID is not the target of the branch, but is the next instruction
					// so we can make the new store the next instruction of the branch before the label
					newInst = ssa_back_convert_placeStoreBeforeInstruction(method, nextInst, newVar, callParam);
					BasicBlock* bb = IRMETHOD_newBasicBlock(method);
					bb->startInst = IRMETHOD_getInstructionPosition(newInst);
					bb->endInst = bb->startInst;
					// now increase by 1 all the basic block boundaries that are after newInst
					ssa_back_convert_increase_BasicBlock_boundaries(method, newInst->ID +1);
					return NULL;
				}
			}
			if (method->getInstrPar2Value(inst) == newLabelID) {
				// the target of the branch goes to newLabelID
				myLabelID = IRMETHOD_newLabelID(method);
				method->setInstrPar2Value(inst, myLabelID); // redirect the branch
				break;
			}
			fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: one status condition of IRBRANCHIF(NOT) is not checked. Aborting.\n");
			abort();
			
		default:
			// this is the case that inst is not a branch instruction
			// so we add the STORE instruction directly here
			newInst = IRMETHOD_newInstructionAfter(method, inst);
			IRMETHOD_setInstructionType(method, newInst, IRSTORE);
			IRMETHOD_cpInstrParameter1(newInst, callParam);
			method->setInstrPar1Value(newInst, newVar);
			method->setInstrPar1Type(newInst, IROFFSET);
			IRMETHOD_cpInstrParameter2(newInst, callParam);
			// we use the position of inst instead of newInst because the last instruction
			// of this basic block is inst, and we must move forward it for including the new
			// instruction into this basic block
			ssa_back_convert_increase_BasicBlock_boundaries(method, IRMETHOD_getInstructionPosition(inst));
			return NULL;
	}
	
	/* here we add the STORE instruction at the end of the code, *
	 * creating one new basic block                              */
	
	// place the new label
	newInst = IRMETHOD_newInstruction(method);
	IRMETHOD_setInstructionType(method, newInst, IRLABEL);
	method->setInstrPar1(newInst, myLabelID, 0, IRLABELITEM, IRLABELITEM, JITFALSE, NULL);
	
	// place the move instruction
	newInst = IRMETHOD_newInstruction(method);
	IRMETHOD_setInstructionType(method, newInst, IRSTORE);
	IRMETHOD_cpInstrParameter1(newInst, callParam);
	method->setInstrPar1Value(newInst, newVar);
	method->setInstrPar1Type(newInst, IROFFSET);
	IRMETHOD_cpInstrParameter2(newInst, callParam);
	
	// place the branch to the label of the successor
	newInst = IRMETHOD_newInstruction(method);
	IRMETHOD_setInstructionType(method, newInst, IRBRANCH);
	method->setInstrPar1(newInst, newLabelID, 0, IRLABELITEM, IRLABELITEM, JITFALSE, NULL);
	
	JITUINT32 numBB = IRMETHOD_getNumberOfMethodBasicBlocks(method);
	method->basicBlocks[numBB-1].endInst = IRMETHOD_getInstructionPosition(newInst);
	BasicBlock* bb = IRMETHOD_newBasicBlock(method);
	bb->startInst  = method->basicBlocks[numBB-1].endInst + 1;
	bb->endInst    = bb->startInst;
	
	return newInst;
	
#endif
}

/**
 * @brief return the list of all predecessors of the second parameter
 *
 * Repeat the call to \c IRMETHOD_getPredecessorInstruction until it returns \c NULL.
 * The predecessors obtained by these calls are stored in one \c XanList and a pointer to it
 * is returned.
 */
static inline XanList* ssa_back_convert_getPredecessorsList(ir_method_t* method, t_ir_instruction* instruction) {
	XanList*          res  = xanListNew(malloc, free, NULL); ///< the resulting list
	t_ir_instruction* pred = IRMETHOD_getPredecessorInstruction(method, instruction, NULL);
	
	while (pred != NULL) {
		res->append(res, pred);
		pred = IRMETHOD_getPredecessorInstruction(method, instruction, pred);
	}
	
	return res;
}

static inline void ssa_back_convert_do_job (ir_method_t * method) {
	/* Assertions */
	assert(method != NULL);
	
	// no phi functions in the method
	if (! method->hasPhiFunctions) return;
	
	t_ir_instruction* inst = IRMETHOD_getFirstInstruction(method);
	while (inst != NULL) {
		if (IRMETHOD_getInstructionType(inst) == IRPHI) {
			// 1. place store instruction in the trailing part of each basic block that
			//    is predecessor of the basic block of the phi instruction
			BasicBlock*       phiBB              = IRMETHOD_getTheBasicBlockIncludesInstruction(method, inst);   ///< the basic block includes the phi instruction pointed by \c inst
			t_ir_instruction* first              = IRMETHOD_getInstructionAtPosition(method, phiBB->startInst); ///< first instr of BB
			IR_ITEM_VALUE     newVar             = IRMETHOD_newVariableID(method);
			XanList*          predecessors       = xanListNew(malloc, free, NULL);   ///< the list of all predecessors of \c first
			JITUINT32         callParamNumber    = 0;
			
			// retrieve the list of all predecessors
			// note that we must retrieve this list by now because when deleting the PHIs we may modify
			// the list of predecessors, and so we risk to leave the optimizer in one unlimited loop
			predecessors = ssa_back_convert_getPredecessorsList(method, first);
			
			// the first instruction must be a label or something reachable from different points of the code
			if (first->type != IRLABEL && first->type != IRSTARTCATCHER && first->type != IRSTARTFINALLY) {
				fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: first->type = %d\n", first->type);
				fprintf(stderr, "                            first->ID   = %d\n", first->ID);
				IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
				fprintf(stderr, "method printer complete\n");
			}
			assert(first->type == IRLABEL || first->type == IRSTARTCATCHER || first->type == IRSTARTFINALLY);
			
			// the number of call parameters of each PHI of one basic block
			// must be equal to the number of predecessors of that basic block
			if (IRMETHOD_getInstructionCallParametersNumber(inst) != predecessors->length(predecessors)) {
				fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: phi @ %d, the number of predecessors (%d) is not the same as the number of call parameters (%d). Predecessors list: ", inst->ID, predecessors->length(predecessors), IRMETHOD_getInstructionCallParametersNumber(inst));
				XanListItem* item = predecessors->first(predecessors);
				while (item != NULL) {
					t_ir_instruction* inst = predecessors->data(predecessors, item);
					fprintf(stderr, "%d ", inst->ID);
					item = predecessors->next(predecessors, item);
				}
				fprintf(stderr, "\n");
				IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
				fprintf(stderr, "method printer complete\n");
			}
			assert(IRMETHOD_getInstructionCallParametersNumber(inst) == predecessors->length(predecessors));
			
			// after initialization now it's time to place the IRSTORE instructions
			XanListItem* predItem = predecessors->first(predecessors);
			while (predItem != NULL) {
				assert(callParamNumber < IRMETHOD_getInstructionCallParametersNumber(inst)); // callParamNumber is zero-based!
				ir_item_t*        callParam      = IRMETHOD_getInstructionCallParameter(inst, callParamNumber);
				t_ir_instruction* predecessor    = predecessors->data(predecessors, predItem);
				t_ir_instruction* newPredecessor = NULL;
				
				assert(callParam != NULL);
				
				if (callParam->value.v != IRMETHOD_getInstructionResult(inst)->value.v) {
					// if the call parameter has the same variable id of the result of phi
					// we have that there is the part of the CFG after phi that at least reads
					// the value of that variable but for sure it not defines it again.
					// in that situation the phi means var x = var x that is of course useless code
					newPredecessor = ssa_back_convert_placeMoveInstr(method, newVar, callParam, predecessor/*, oldLabelID*/);
					if (newPredecessor != NULL) {
						// we need to update the call parameters order of all the other phi
						// in the same basic block of the one we are deleting, because right
						// now one of the predecessors is changed from 'predecessor' to
						// newPredecessor
						JITUINT32 oldPredecessorNumber = callParamNumber;
						JITUINT32 newPredecessorNumber = 0;
						t_ir_instruction* iterator = IRMETHOD_getPredecessorInstruction(method, first, NULL);
						while (iterator != NULL) {
							if (iterator == newPredecessor) {
								iterator = NULL;
							} else {
								newPredecessorNumber++;
								iterator = IRMETHOD_getPredecessorInstruction(method, first, iterator);
							}
						}
						assert(IRMETHOD_getPredecessorsNumber(method, first) > newPredecessorNumber);
						if (oldPredecessorNumber != newPredecessorNumber)
							ssa_back_convert_updatePhiParametersList(method, inst, oldPredecessorNumber, newPredecessorNumber);
					}
				}
				
				predecessors->deleteItem(predecessors, predItem);
				predItem = predecessors->first(predecessors);
				callParamNumber++;
			}
			assert(predecessors->len == 0);
			predecessors->destroyList(predecessors);
			
			// 2. substitute x3 = phi(x1, x2, ...) with STORE(x3, x4) where x4 is the newVar that is defined
			//    in each move instruction just created
			
			// copies the result parameter of the phi into the first parameter of the future IRSTORE
			IRMETHOD_cpInstructionParameter1(inst, IRMETHOD_getInstructionResult(inst));
			IRMETHOD_setInstructionResult(method, inst, 0, 0, NOPARAM, NOPARAM, NULL);
			
			// convert the instruction in IRSTORE
			IRMETHOD_setInstructionType(method, inst, IRSTORE);
			
			// place the new variable as second parameter of the STORE
			IRMETHOD_cpInstructionParameter2(inst, IRMETHOD_getInstructionParameter1(inst));
			IRMETHOD_setInstructionParameter2Value(method, inst, newVar);
			
			// now we have STORE (x3, x4) (... call parameters ...)
			// we have to delete the call parameters
			IRMETHOD_deleteInstructionCallParameters(method, inst);
		}
		inst = IRMETHOD_getNextInstruction(method, inst);
	}
	method->hasPhiFunctions = JITFALSE;
	
#ifdef DEBUG
	ssa_back_convert_checkBasicBlocks(method);
	ssa_back_convert_checkUninitializedValues(method);
#endif
	
#if OPTIMIZER_SSA_BACK_CONVERT_REMOVE_USELESS_CODE == JITTRUE
	ssa_back_convert_removeUselessCode(method);
#endif
	
	PDEBUG("%s end\n", ssa_back_convert_getMethodName(method));

	/* Return				*/
	return;
}

static inline char * ssa_back_convert_get_version (void) {
	return VERSION;
}

static inline char * ssa_back_convert_get_informations (void) {
	return INFORMATIONS;
}

static inline char * ssa_back_convert_get_author (void) {
	return AUTHOR;
}

static inline JITUINT64 ssa_back_convert_get_invalidations (void) {
	return SSA_BACK_CONVERT_INVALIDATIONS;
}

#ifdef DEBUG
/**
 * @brief ensure that all instructions of the method are at least in one basic block
 */
static void ssa_back_convert_checkBasicBlocks(ir_method_t* method) {
	JITUINT32  count1;
	JITBOOLEAN found;
	JITUINT32  numBlocks = IRMETHOD_getNumberOfMethodBasicBlocks(method);
	JITUINT32  minID     = -1; // -1 in unsigned is equal to the maximum unsigned value
	JITUINT32  maxID     = 0;
	// loop through all basic blocks
	for (count1 = 0; count1 < numBlocks; count1++) {
		BasicBlock* bb1  = IRMETHOD_getBasicBlock(method, count1);
		found = (bb1->startInst == IRMETHOD_getInstructionsNumber(method)); // we cannot have instructions after the exit node
		// look for the basic block includes the next instruction of the one at the end of this block
		JITUINT32   count2;
		for (count2 = 0; count2 < numBlocks && !found; count2++) {
			BasicBlock* bb2 = IRMETHOD_getBasicBlock(method, count2);
			found = (bb2->startInst == bb1->endInst + 1);
		}
		if (!found) {
			fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: method %s, unable to find the basic block successor of the one %d -> %d\n", ssa_back_convert_getMethodName(method), bb1->startInst, bb1->endInst);
			fprintf(stderr, "                            inst %d has type %d\n", bb1->endInst, IRMETHOD_getInstructionAtPosition(method, bb1->endInst)->type);
			fprintf(stderr, "                            inst %d has type %d\n", bb1->endInst+1, IRMETHOD_getInstructionAtPosition(method, bb1->endInst+1)->type);
			JITUINT32 count;
			for (count = 0; count < numBlocks; count++) {
				BasicBlock* bb = IRMETHOD_getBasicBlock(method, count);
				fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: basic block %2d, (%3d -> %3d)\n", count, bb->startInst, bb->endInst);
			}
			IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
		}
		assert(found);
		if (bb1->startInst < minID) minID = bb1->startInst;
		if (bb1->endInst   > maxID) maxID = bb1->endInst;
	}
	assert(minID == 0);
	assert(maxID == IRMETHOD_getInstructionsNumber(method));
}

/**
 * @brief look for uninitialized variables
 *
 * This functions calls the \c LIVENESS_ANALYZER plugin, next verify if the \c live_in set of the first instruction
 * of the method is empty or it contains some variables. All the variables in that set are not initialized
 * before being read in some execution paths.
 */
static void ssa_back_convert_checkUninitializedValues(ir_method_t* method) {
	JITUINT32 count;
	IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, LIVENESS_ANALYZER);
	t_ir_instruction* i  = IRMETHOD_getFirstInstruction(method);
	if (i->metadata->liveness.in != NULL) {
		for (count = 0; count < method->livenessBlockNumbers; count++) {
			if (livenessGetIn(method, i, count) != 0) {
				fprintf(stderr, "OPTIMIZER_SSA_BACK_CONVERT: the live_in set of the first instruction of %s is not empty: ", ssa_back_convert_getMethodName(method));
				for (; count < method->livenessBlockNumbers; count++) {
					JITNUINT set = livenessGetIn(method, i, count);
					JITNUINT count1 = 0;
					while (set != 0) {
						while ((set & 0x1) == 0) {
							set = set >> 1;
							count1++;
						}
						fprintf(stderr, "var %lu ", sizeof(JITNUINT)*8*count + count1);
						set &= ~ 0x1;
					}
				}
				fprintf(stderr, "are live_in in instruction %d\n", i->ID);
				IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, METHOD_PRINTER);
				return;
			}
		}
	}
}
#endif
