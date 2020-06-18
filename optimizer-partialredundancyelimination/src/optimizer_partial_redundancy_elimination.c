/*
 * Copyright (C) 2008 - 2011  Campanoni Simone
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
#include <xanlib.h>

// My headers
#include <optimizer_partial_redundancy_elimination.h>
#include <config.h>
// End

#define INFORMATIONS 	"This plugin applies the partial redundancy elimination code transformation"
#define	AUTHOR		"Simone Campanoni and Scott Moore and Sophia Shao"

static inline JITUINT64 pre_get_ID_job (void);
static inline char * pre_get_version (void);
static inline void pre_do_job (ir_method_t * method);
static inline char * pre_get_informations (void);
static inline char * pre_get_author (void);
static inline JITUINT64 pre_get_dependences (void);
static inline void pre_shutdown (JITFLOAT32 totalTime);
static inline void pre_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix);
static inline JITUINT64 pre_get_invalidations (void);

ir_lib_t	*irLib		= NULL;
ir_optimizer_t	*irOptimizer	= NULL;
char 	        *prefix		= NULL;

ir_optimization_interface_t plugin_interface = {
	pre_get_ID_job	, 
	pre_get_dependences	,
	pre_init		,
	pre_shutdown		,
	pre_do_job		, 
	pre_get_version	,
	pre_get_informations	,
	pre_get_author,
	pre_get_invalidations
};

static inline void pre_init (ir_lib_t *lib, ir_optimizer_t *optimizer, char *outputPrefix){

	/* Assertions			*/
	assert(lib != NULL);
	assert(optimizer != NULL);
	assert(outputPrefix != NULL);

	irLib		= lib;
	irOptimizer	= optimizer;
	prefix		= outputPrefix;
}

static inline void pre_shutdown (JITFLOAT32 totalTime){
        irLib		= NULL;
	irOptimizer	= NULL;
	prefix		= NULL;
}

static inline JITUINT64 pre_get_ID_job (void){
	return CUSTOM;
}

static inline JITUINT64 pre_get_dependences (void){
	return 0;
}

static inline JITBOOLEAN is_expression(t_ir_instruction *inst){
	assert(inst != NULL);

	switch(IRMETHOD_getInstructionType(inst)){
		case IRADD:
		case IRADDOVF: 
		case IRSUB: 
		case IRSUBOVF: 
		case IRMUL: 
		case IRMULOVF: 
		case IRDIV: 
		case IRREM: 
		case IRAND: 
		case IRNEG: 
		case IROR: 
		case IRNOT: 
		case IRXOR: 
		case IRSHL: 
		case IRSHR: 
		case IRCONV: 
		case IRCONVOVF: 
		case IRLT: 
		case IRGT: 
		case IREQ: 
		//case IRLOADELEM:
		//case IRLOADREL:
		case IRGETADDRESS: 
		case IRISNAN: 
		case IRISINF: 
		case IRCOSH:
		case IRSIN:
		case IRCOS:
		case IRACOS:
		case IRSQRT:
		case IRFLOOR:
		case IRPOW:
			return JITTRUE;
		default:
			return JITFALSE;
	}
}

static JITBOOLEAN same_expression(ir_method_t *method, t_ir_instruction *inst1, t_ir_instruction *inst2){

	assert(inst1 && inst2);
	assert(is_expression(inst1));
	assert(is_expression(inst2));
	if(!(is_expression(inst1)))  return JITFALSE;
	if(!(is_expression(inst2)))  return JITFALSE;
	if(IRMETHOD_getInstructionType(inst1) == IRMETHOD_getInstructionType(inst2)){
		assert(IRMETHOD_getInstructionParametersNumber(inst1) == IRMETHOD_getInstructionParametersNumber(inst2));
		//PDEBUG("same here!!\n");
		/*PDEBUG("inst1 par num: %d!!\n", IRMETHOD_getInstructionParametersNumber(inst1));
		PDEBUG("inst2 par num: %d!!\n", IRMETHOD_getInstructionParametersNumber(inst2));
		PDEBUG("comparing: inst type: %d,otherinst type: %d  \n ", inst1->type,inst2->type); 
		*/
		switch(IRMETHOD_getInstructionParametersNumber(inst1)){
			case 3:

				
				if(IRMETHOD_getInstructionParameter3Type(inst1) != IRMETHOD_getInstructionParameter3Type(inst2) ||
				   IRMETHOD_getInstructionParameter3Value(inst1) != IRMETHOD_getInstructionParameter3Value(inst2) ||
				   IRMETHOD_getInstructionParameter3FValue(inst1) != IRMETHOD_getInstructionParameter3FValue(inst2)){
				   //(JITUINT64)(IRMETHOD_getInstructionParameter3FValue(inst1)) != (JITUINT64)(IRMETHOD_getInstructionParameter3FValue(inst2))){
				 	
					//PDEBUG("return 3 false!!\n");
					return JITFALSE;	
				 }

			case 2:
				if(IRMETHOD_getInstructionParameter2Type(inst1) != IRMETHOD_getInstructionParameter2Type(inst2) ||
				   IRMETHOD_getInstructionParameter2Value(inst1) != IRMETHOD_getInstructionParameter2Value(inst2) ||
				   IRMETHOD_getInstructionParameter2FValue(inst1) != IRMETHOD_getInstructionParameter2FValue(inst2)){
				   //(JITUINT64)(IRMETHOD_getInstructionParameter2FValue(inst1)) != (JITUINT64)(IRMETHOD_getInstructionParameter2FValue(inst2))){
				 	
					//PDEBUG("return 2 false!!\n");
					return JITFALSE;	
				 }


			case 1:
				if((IRMETHOD_getInstructionParameter1Type(inst1) != IRMETHOD_getInstructionParameter1Type(inst2)) ||
				   (IRMETHOD_getInstructionParameter1Value(inst1) != IRMETHOD_getInstructionParameter1Value(inst2)) ||
				   (IRMETHOD_getInstructionParameter1FValue(inst1) != IRMETHOD_getInstructionParameter1FValue(inst2))){
				   //((JITUINT64)(IRMETHOD_getInstructionParameter1FValue(inst1)) != (JITUINT64)(IRMETHOD_getInstructionParameter1FValue(inst2)))){
					/*
					PDEBUG("come in\n");
					PDEBUG("type wrong?? : %d\n", (IRMETHOD_getInstructionParameter1Type(inst1) != IRMETHOD_getInstructionParameter1Type(inst2))); 
					PDEBUG("value wrong?? : %d\n", (IRMETHOD_getInstructionParameter1Value(inst1) != IRMETHOD_getInstructionParameter1Value(inst2))); 
					PDEBUG("fvalue wrong?? : %d\n", (IRMETHOD_getInstructionParameter1FValue(inst1) != IRMETHOD_getInstructionParameter1FValue(inst2))); 
					PDEBUG("fvalue 1: %f - fvalue 2:%f = %f\n", IRMETHOD_getInstructionParameter1FValue(inst1), IRMETHOD_getInstructionParameter1FValue(inst2), IRMETHOD_getInstructionParameter1FValue(inst1) - IRMETHOD_getInstructionParameter1FValue(inst2));
					PDEBUG("fvalue : %lu\n", (JITUINT64)(IRMETHOD_getInstructionParameter1FValue(inst1)));
					PDEBUG("fvalue 2 : %lu\n", (JITUINT64)(IRMETHOD_getInstructionParameter1FValue(inst2)));
					PDEBUG("int1: par1 type: %d, par1 value: %d, par1 fvalue: %f!!\n", IRMETHOD_getInstructionParameter1Type(inst1), IRMETHOD_getInstructionParameter1Value(inst1), IRMETHOD_getInstructionParameter1FValue(inst1));
					PDEBUG("int2: par1 type: %d, par1 value: %d, par1 fvalue: %f!!\n", IRMETHOD_getInstructionParameter1Type(inst2), IRMETHOD_getInstructionParameter1Value(inst2), IRMETHOD_getInstructionParameter1FValue(inst2));
					PDEBUG("return 1 false!!\n");
					*/
					return JITFALSE;	
				 }
			default:
				return JITTRUE;
		}
		return JITTRUE;
	}
	//PDEBUG("return final false!!\n");
	return JITFALSE;

}

static JITBOOLEAN similar_expression(ir_method_t *method, t_ir_instruction *inst1, t_ir_instruction *inst2){

	assert(inst1 && inst2);
	if(!(is_expression(inst1)))  return JITFALSE;
	if(!(is_expression(inst2)))  return JITFALSE;
		/*
		PDEBUG("similar here!!\n");
		PDEBUG("inst1 par num: %d!!\n", IRMETHOD_getInstructionParametersNumber(inst1));
		PDEBUG("inst2 par num: %d!!\n", IRMETHOD_getInstructionParametersNumber(inst2));
		PDEBUG("comparing: inst type: %d,otherinst type: %d  \n ", inst1->type,inst2->type); 
		*/
	if(IRMETHOD_getInstructionType(inst1) == IRMETHOD_getInstructionType(inst2)){
		assert(IRMETHOD_getInstructionParametersNumber(inst1) == IRMETHOD_getInstructionParametersNumber(inst2));
		assert(IRMETHOD_getInstructionParametersNumber(inst1) == 2);
			
		if (( 
			  IRMETHOD_getInstructionParameter1Type(inst1) == IRMETHOD_getInstructionParameter1Type(inst2) && 
			  IRMETHOD_getInstructionParameter1Value(inst1) == IRMETHOD_getInstructionParameter1Value(inst2) &&
			  IRMETHOD_getInstructionParameter1FValue(inst1) == IRMETHOD_getInstructionParameter1FValue(inst2) &&
			  IRMETHOD_getInstructionParameter2Type(inst1) == IRMETHOD_getInstructionParameter2Type(inst2) && 
			  IRMETHOD_getInstructionParameter2Value(inst1) == IRMETHOD_getInstructionParameter2Value(inst2) &&
			  IRMETHOD_getInstructionParameter2FValue(inst1) == IRMETHOD_getInstructionParameter2FValue(inst2)
			  ) ||
			 (
			  IRMETHOD_getInstructionParameter1Type(inst1) == IRMETHOD_getInstructionParameter2Type(inst2) && 
			  IRMETHOD_getInstructionParameter1Value(inst1) == IRMETHOD_getInstructionParameter2Value(inst2) &&
			  IRMETHOD_getInstructionParameter1FValue(inst1) == IRMETHOD_getInstructionParameter2FValue(inst2) &&
			  IRMETHOD_getInstructionParameter2Type(inst1) == IRMETHOD_getInstructionParameter1Type(inst2) && 
			  IRMETHOD_getInstructionParameter2Value(inst1) == IRMETHOD_getInstructionParameter1Value(inst2) &&
			  IRMETHOD_getInstructionParameter2FValue(inst1) == IRMETHOD_getInstructionParameter1FValue(inst2) 
			 )
			  ){
			  return JITTRUE;
			  }
		return JITFALSE;
	}
	return JITFALSE;
}

static JITBOOLEAN equiv_expression(ir_method_t *method, t_ir_instruction *inst1, t_ir_instruction *inst2) {
	if (is_expression(inst1) && is_expression(inst2)) {
		if(inst1 == inst2) return JITTRUE;
		//PDEBUG("equiv here!!\n");
		switch(IRMETHOD_getInstructionType(inst1)){
			case IRADD:
			case IRADDOVF:
			case IRMUL: 
			case IRMULOVF: 	
			case IRAND: 		
			case IROR: 		
			case IRXOR: 
			case IREQ: 
				return similar_expression(method,inst1,inst2);
			default:
				return same_expression(method,inst1,inst2);
		}
	} else {
		return JITFALSE;
	}
}

static JITBOOLEAN kill_expression(ir_method_t *method, t_ir_instruction *inst1, t_ir_instruction *inst2) {
	IR_ITEM_VALUE	result;
	JITUINT32	numParams;

	if (!is_expression(inst2)) return JITFALSE;

	//if (is_expression(inst1) || (inst1->type == IRLOADREL)||(inst1->type == IRLOADELEM)) {
	if ((inst1->type != IRSTORE)&&(IRMETHOD_doesInstructionDefineAVariable(method, inst1))){
		result = IRMETHOD_getInstructionResult(inst1)->value.v;
	} else if (inst1->type == IRSTORE) {
		result = method->getInstrPar1(inst1)->value;
	} else if ((inst1->type == IRSTOREREL)||(inst1->type == IRSTOREELEM)) {
		if( (inst2->type == IRLOADREL)||(inst2->type == IRLOADELEM)) {
			//if (IRMETHOD_hasInstructionEscapes(method,inst2)) {
				return JITTRUE;
			//} else {
			//	return JITFALSE;
			//}
		}else if( IRMETHOD_hasInstructionEscapes(method, inst2)){
			return JITTRUE;
		}else{
			return JITFALSE;
		}
		/*
		if ((inst2->type == IRLOADREL)||(inst2->type == IRLOADELEM)) {
			return JITTRUE;
		} else {
			return JITFALSE;
		}*/
	} else {
		return JITFALSE;
	}

	numParams = IRMETHOD_getInstructionParametersNumber(inst2);
	switch (numParams) {
		case 3:
			if ((IRMETHOD_getInstructionParameter3Type(inst2) == IROFFSET)&&(IRMETHOD_getInstructionParameter3Value(inst2) == result)) return JITTRUE;
		case 2:
			if ((IRMETHOD_getInstructionParameter2Type(inst2) == IROFFSET)&&(IRMETHOD_getInstructionParameter2Value(inst2) == result)) return JITTRUE;
		case 1:
			if ((IRMETHOD_getInstructionParameter1Type(inst2) == IROFFSET)&&(IRMETHOD_getInstructionParameter1Value(inst2) == result)) return JITTRUE;
		default:
			return JITFALSE;
	}
}

#define SETSET(structure,set) \
static inline void ir_instr_ ## structure ## _ ## set ## _set(ir_method_t *method, t_ir_instruction *inst, JITUINT32 block, JITNUINT value) { \
	(inst-> structure ). set [block] = value; \
}

#define GETSET(structure,set) \
static inline JITNUINT ir_instr_ ## structure ## _ ## set ## _get(ir_method_t *method, t_ir_instruction *inst, JITUINT32 block) { \
	return (inst -> structure). set [block]; \
}

SETSET(anticipated_expressions,gen)
GETSET(anticipated_expressions,gen)
SETSET(anticipated_expressions,kill)
GETSET(anticipated_expressions,kill)
SETSET(anticipated_expressions,in)
GETSET(anticipated_expressions,in)
SETSET(anticipated_expressions,out)
GETSET(anticipated_expressions,out)


/*SETSET(available_expressions,gen)
GETSET(available_expressions,gen)
SETSET(available_expressions,kill)
GETSET(available_expressions,kill)
SETSET(available_expressions,in)
GETSET(available_expressions,in)
SETSET(available_expressions,out)
GETSET(available_expressions,out)*/

SETSET(earliest_expressions,earliestSet)
GETSET(earliest_expressions,earliestSet)
SETSET(latest_placements,latestSet)
GETSET(latest_placements,latestSet)

SETSET(postponable_expressions,use)
GETSET(postponable_expressions,use)
SETSET(postponable_expressions,kill)
GETSET(postponable_expressions,kill)
SETSET(postponable_expressions,in)
GETSET(postponable_expressions,in)
SETSET(postponable_expressions,out)
GETSET(postponable_expressions,out)

SETSET(used_expressions,use)
GETSET(used_expressions,use)
SETSET(used_expressions,kill)
GETSET(used_expressions,kill)
SETSET(used_expressions,in)
GETSET(used_expressions,in)
SETSET(used_expressions,out)
GETSET(used_expressions,out)

SETSET(partial_redundancy_elimination,addedInstructions)
GETSET(partial_redundancy_elimination,addedInstructions)

static void printNative(char *string) {
	printf("%s\n",string);
}

static void printNativeExit(char *string) {
	printf("Exiting %s\n",string);
}

static void insertSignaturePrint(ir_method_t* method) {
	JITINT8		*sig;
	t_ir_instruction *printInst;
	ir_item_t	*ir_item, *ir_to_dump;

	sig = IRMETHOD_getSignatureInString(method);
	printInst = IRMETHOD_newInstructionBefore(method,IRMETHOD_getInstructionAtPosition(method,IRMETHOD_getMethodParametersNumber(method)));
	IRMETHOD_setInstructionType(method, printInst, IRNATIVECALL);

	
	ir_item			= method->getInstrPar1(printInst);
	ir_item->value		= IRVOID;
	ir_item->type		= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item			= method->getInstrPar2(printInst);
	ir_item->value		= (IR_ITEM_VALUE)(JITNUINT)"printNative";
	ir_item->type		= IRNINT;
	ir_item->internal_type	= IRNINT;
		
	ir_item			= method->getInstrPar3(printInst);
	ir_item->value		= (IR_ITEM_VALUE)(JITNUINT)printNative;
	ir_item->type		= IRNINT;
	ir_item->internal_type	= IRNINT;
		
	ir_to_dump		= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value  	= (IR_ITEM_VALUE)(JITNUINT)sig;
	ir_to_dump->type	= IRSTRING;
	ir_to_dump->internal_type = IRSTRING;
	printInst->callParameters = xanListNew(allocFunction, freeFunction, NULL);
	printInst->callParameters->append(printInst->callParameters, ir_to_dump);
}

static void insertSignaturePrintAtInstr(ir_method_t* method,t_ir_instruction *inst) {
	JITINT8		*sig;
	t_ir_instruction *printInst;
	ir_item_t	*ir_item, *ir_to_dump;

	sig = IRMETHOD_getSignatureInString(method);
	printInst = IRMETHOD_newInstructionBefore(method,inst);
	IRMETHOD_setInstructionType(method, printInst, IRNATIVECALL);

	
	ir_item			= method->getInstrPar1(printInst);
	ir_item->value		= IRVOID;
	ir_item->type		= IRTYPE;
	ir_item->internal_type	= IRTYPE;

	ir_item			= method->getInstrPar2(printInst);
	ir_item->value		= (IR_ITEM_VALUE)(JITNUINT)"printNativeExit";
	ir_item->type		= IRNINT;
	ir_item->internal_type	= IRNINT;
		
	ir_item			= method->getInstrPar3(printInst);
	ir_item->value		= (IR_ITEM_VALUE)(JITNUINT)printNativeExit;
	ir_item->type		= IRNINT;
	ir_item->internal_type	= IRNINT;
		
	ir_to_dump		= allocFunction(sizeof(ir_item_t));
	ir_to_dump->value  	= (IR_ITEM_VALUE)(JITNUINT)sig;
	ir_to_dump->type	= IRSTRING;
	ir_to_dump->internal_type = IRSTRING;
	printInst->callParameters = xanListNew(allocFunction, freeFunction, NULL);
	printInst->callParameters->append(printInst->callParameters, ir_to_dump);
}



static inline void pre_preprocess(ir_method_t * method) {
	JITUINT32	count;
	JITUINT32	instID;
	JITUINT32	otherInstID;
	JITUINT32	numBlocks;
	JITUINT32	i;
	JITNUINT	block;
	JITBOOLEAN	sameExpr;
	JITBOOLEAN	kill;
	XanList		*instList;
	XanList		*predList;
	XanListItem	*listItem;
	XanListItem	*predListItem;
	JITUINT32	instType;
	t_ir_instruction *inst, *otherInst;
	t_ir_instruction *pred;
	t_ir_instruction *tempInst;
	t_ir_instruction *newLabel;
	t_ir_instruction *newBranch;

	/*
	insertSignaturePrint(method);
	count = IRMETHOD_getInstructionsNumber(method);


	for (instID = IRMETHOD_getMethodParametersNumber(method); instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		if (inst->type==IRRET) {
			insertSignaturePrintAtInstr(method,inst);
		}
	}
	*/


	count = IRMETHOD_getInstructionsNumber(method);
	//PDEBUG("Number of instructions before: %d\n", count);

	
	/* Add "basic blocks" between the source and destination of
	 * an edge if the destination has more than one predecessor */
	instList = xanListNew(allocFunction, freeFunction, NULL);
	
	// Create a list of instructions with more than one precessor
	for (instID = IRMETHOD_getMethodParametersNumber(method); instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		if (IRMETHOD_getPredecessorsNumber(method,inst) > 1) {
			instList->append(instList,inst);
		}
	}

	// Insert instructions between each predessor and instruction
	listItem = instList->first(instList);
	while (listItem != NULL) {
		inst = (t_ir_instruction *)listItem->data;
	
		// Create a list of the predecessors
		predList = xanListNew(allocFunction, freeFunction, NULL);
		pred = NULL;
		while ((pred = IRMETHOD_getPredecessorInstruction(method,inst,pred)) != NULL) {
			predList->append(predList,pred);
		}

		// Add nops along edges
		predListItem = predList->first(predList);
		while (predListItem != NULL) {
			pred = (t_ir_instruction *)predListItem->data;
			instType = IRMETHOD_getInstructionType(pred);
			if (IRMETHOD_getInstructionType(inst) == IRLABEL) {
				switch (instType) {
					case IRBRANCH:
					case IRBRANCHIFPCNOTINRANGE:
						if ((pred->param_1).value == (inst->param_1).value) {
						// Add something that looks like this to the end and change pred to branch to newlabel instead of inst
						// IRLABEL newlabel
						// IRNOP
						// IRBRANCH inst
						
						// Add a new Label at the bottom of the method
						newLabel = IRMETHOD_newLabel(method);
						// Add an IRNOP
						tempInst = IRMETHOD_newInstructionAfter(method,newLabel);
						tempInst->type = IRNOP;
						// Create a branch to inst
						newBranch = IRMETHOD_newBranchToLabel(method,inst);
						// Place the branch after the IRNOP
						IRMETHOD_moveInstructionAfter(method,newBranch,tempInst);
						// Redirect pred to branch to newLabel
						(pred->param_1).value = (newLabel->param_1).value;
						}
						// If the branch could fall through to this instruction, add an IRNOP along this edge
						// XXX not sure if "(pred->ID + 1) == inst->ID" is safe here...
						if (IRMETHOD_canTheNextInstructionBeASuccessor(method,pred)&&((pred->ID + 1) == inst->ID)) {
							tempInst = IRMETHOD_newInstructionAfter(method,pred);
							tempInst->type = IRNOP;
						}
						break;
					case IRBRANCHIF:
					case IRBRANCHIFNOT:
						if ((pred->param_2).value == (inst->param_1).value) {
						// Add something that looks like this to the end and change pred to branch to newlabel instead of inst
						// IRLABEL newlabel
						// IRNOP
						// IRBRANCH inst
						
						// Add a new Label at the bottom of the method
						newLabel = IRMETHOD_newLabel(method);
						// Add an IRNOP
						tempInst = IRMETHOD_newInstructionAfter(method,newLabel);
						tempInst->type = IRNOP;
						// Create a branch to inst
						newBranch = IRMETHOD_newBranchToLabel(method,inst);
						// Place the branch after the IRNOP
						IRMETHOD_moveInstructionAfter(method,newBranch,tempInst);
						// Redirect pred to branch to newLabel
						(pred->param_2).value = (newLabel->param_1).value;
						}
						// If the branch could fall through to this instruction, add an IRNOP along this edge
						// XXX not sure if "(pred->ID + 1) == inst->ID" is safe here...
						if (IRMETHOD_canTheNextInstructionBeASuccessor(method,pred)&&((pred->ID + 1) == inst->ID)) {
							tempInst = IRMETHOD_newInstructionAfter(method,pred);
							tempInst->type = IRNOP;
						}
						break;
					default:
						// Previous instruction is not a branch, just insert nop
						tempInst = IRMETHOD_newInstructionAfter(method,pred);
						tempInst->type = IRNOP;
				}
			}
			predListItem = predListItem->next;
		}
		predList->destroyList(predList);
		listItem = listItem->next;
	}
	
	instList->destroyList(instList);
	
	count = IRMETHOD_getInstructionsNumber(method);
	//PDEBUG("Number of instructions after: %d\n",count);
	//print_method(method);
	//IROPTIMIZER_callMethodOptimization(irOptimizer,method,irLib,METHOD_PRINTER);

	// Compute how many blocks we need
	numBlocks = (count + IRMETHOD_getMethodParametersNumber(method))/(sizeof(JITNINT)*8) + 1;
	// number of blocks is size (maximum size of the sets, as both are equal) / sizeof native int in bits +1 (count from 0)
	// => 1 bit per instruction. 

	// Allocate memory for gen/kill sets
	IRMETHOD_allocMemoryForAnticipatedExpressionsAnalysis(method, numBlocks);
	IRMETHOD_allocMemoryForAvailableExpressionsAnalysis(method, numBlocks);
	IRMETHOD_allocMemoryForPostponableExpressionsAnalysis(method, numBlocks);
	IRMETHOD_allocMemoryForUsedExpressionsAnalysis(method, numBlocks);
	IRMETHOD_allocMemoryForEarliestExpressionsAnalysis(method, numBlocks);
	IRMETHOD_allocMemoryForLatestPlacementsAnalysis(method, numBlocks);
	IRMETHOD_allocMemoryForPartialRedundancyEliminationAnalysis(method, numBlocks);

	//PDEBUG("AnticipatedExpressionsAnalysis: number of blocks = %d\n", method->anticipatedExpressionsBlockNumbers); 
	//PDEBUG("AvailableExpressionsAnalysis: number of blocks = %d\n", method->availableExpressionsBlockNumbers); 
	//PDEBUG("PostponableExpressionsAnalysis: number of blocks = %d\n", method->postponableExpressionsBlockNumbers); 
	//PDEBUG("UsedExpressionsAnalysis: number of blocks = %d\n", method->usedExpressionsBlockNumbers); 
	//PDEBUG("EarliestExpressionsAnalysis: number of blocks = %d\n", method->earliestExpressionsBlockNumbers); 
	//PDEBUG("LatestPlacementsAnalysis: number of blocks = %d\n", method->latestPlacementsBlockNumbers); 

	assert(method->reachingDefinitionsBlockNumbers > 0);
	// Make sure we have reaching definitions analysis so that we can borrow its kill sets
	// XXX Commenting out for now... SEGFAULT issues :(
	//IROPTIMIZER_callMethodOptimization(irOptimizer, method, irLib, REACHING_DEFINITIONS_ANALYZER);
	
	for (instID = IRMETHOD_getMethodParametersNumber(method); instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		// Initialize sets
		for (i = 0; i < method->anticipatedExpressionsBlockNumbers; i++) {
			ir_instr_anticipated_expressions_gen_set(method, inst, i, 0);
			ir_instr_anticipated_expressions_kill_set(method, inst, i, 0);
		}
		// Set gen/kill bits
		for (otherInstID = IRMETHOD_getMethodParametersNumber(method); otherInstID < count; otherInstID++) {
			otherInst = IRMETHOD_getInstructionAtPosition(method,otherInstID);
			sameExpr = equiv_expression(method,inst,otherInst);
			//PDEBUG("comparing instid:  %d,inst type: %d,  otherinstid: %d, otherinst type: %d,  same? %d\n ", instID,inst->type, otherInstID,otherInst->type,  sameExpr); 
			if ((is_expression(inst))&&(is_expression(otherInst))&&(instID == otherInstID)) assert(sameExpr);
			if (sameExpr) {
				block = ir_instr_anticipated_expressions_gen_get(method, inst, otherInstID / (sizeof(JITNINT) * 8));
				block = block | 1 << (otherInstID % ((sizeof(JITNINT) * 8)));
				ir_instr_anticipated_expressions_gen_set(method, inst, otherInstID / (sizeof(JITNINT) * 8), block);
			}
			kill = kill_expression(method,inst,IRMETHOD_getInstructionAtPosition(method,otherInstID));
			if (kill) {
				block = ir_instr_anticipated_expressions_kill_get(method, inst, otherInstID / (sizeof(JITNINT) * 8));
				block = block | 1 << (otherInstID % ((sizeof(JITNINT) * 8)));
				ir_instr_anticipated_expressions_kill_set(method, inst, otherInstID / (sizeof(JITNINT) * 8), block);
			}
		}
		for (i = 0; i < method->anticipatedExpressionsBlockNumbers; i++) {
			block = ir_instr_anticipated_expressions_gen_get(method, inst, i);
			ir_instr_available_expressions_gen_set(method, inst, i, block);
			ir_instr_postponable_expressions_use_set(method, inst, i, block);
			ir_instr_used_expressions_use_set(method, inst, i, block);
		}
		for (i = 0; i < method->anticipatedExpressionsBlockNumbers; i++) {
			block = ir_instr_anticipated_expressions_kill_get(method, inst, i);
			ir_instr_available_expressions_kill_set(method, inst, i, block);
			ir_instr_postponable_expressions_kill_set(method, inst, i, block);
			ir_instr_used_expressions_kill_set(method, inst, i, block);
		}
		/*PDEBUG("Gen %d: ", instID); 
		for (otherInstID = IRMETHOD_getMethodParametersNumber(method); otherInstID < count; otherInstID++) {
			block = ir_instr_anticipated_expressions_gen_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Kill %d: ", instID); 
		for (otherInstID = IRMETHOD_getMethodParametersNumber(method); otherInstID < count; otherInstID++) {
			block = ir_instr_anticipated_expressions_kill_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");*/
	}

	return;
}

static inline void pre_do_job (ir_method_t * method){
	JITUINT32		count,first;
	JITUINT32 		instID, otherInstID, inst3ID;
	JITUINT32		i;
	t_ir_instruction	*inst, *otherInst, *inst3;
	t_ir_instruction	*pred;
	t_ir_instruction	*succ;
	t_ir_instruction	*usingInst;
	JITBOOLEAN		modified;
	JITBOOLEAN		addedNewVar;
	JITBOOLEAN		addedInstToList;
	JITBOOLEAN		seenIt;
	JITNINT			block, tempBlock;
	IR_ITEM_VALUE		newVar;
	ir_item_t		resultItem;
	XanList			*instList, *exprList, *usesNew, *seenExprList;
	XanListItem		*listItem, *exprItem, *seenExprItem;
	IR_ITEM_VALUE		newVarValue;

	/* Assertions				*/
	assert(method != NULL);

	PDEBUG("Optimizing: %s\n",IRMETHOD_getMethodName(method));
	//IROPTIMIZER_callMethodOptimization(irOptimizer,method,irLib,METHOD_PRINTER);
	
	pre_preprocess(method);
	//IROPTIMIZER_callMethodOptimization(irOptimizer,method,irLib,METHOD_PRINTER);


	count = IRMETHOD_getInstructionsNumber(method) + 1;
	first = IRMETHOD_getMethodParametersNumber(method);

	/*********************************************
	 * Compute anticipated expressions
	 ********************************************/

	// Initialize sets
	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method, instID);
		for (i = 0; i < method->anticipatedExpressionsBlockNumbers; i++) {
			ir_instr_anticipated_expressions_in_set(method, inst, i, ~0);
		}	
		if (inst->type == IREXITNODE) {
			for (i=0; i < method->anticipatedExpressionsBlockNumbers; i++) {
				ir_instr_anticipated_expressions_in_set(method, inst, i, 0);
				ir_instr_anticipated_expressions_out_set(method, inst, i, 0);
			}
		}
	}
	
	// Until a fixed point...
	do {
		modified = JITFALSE;
		// for each instruction, uptdate the IN/OUT sets
		for (instID = count-2; instID+1 > first; instID--) {
			inst = IRMETHOD_getInstructionAtPosition(method, instID);
			// block by block update the OUT sets
			for (i=0; i < method->anticipatedExpressionsBlockNumbers; i++) {
				succ = NULL;
				block = ~0;
				// The new OUT set is the intersection of all the successors' IN sets
				while ((succ = IRMETHOD_getSuccessorInstruction(method,inst,succ)) != NULL) {
					if (inst->type == IRRET) assert(succ->type == IREXITNODE);
					if (inst->type == IRRET) assert(!ir_instr_anticipated_expressions_in_get(method, succ, i));
					block = block & ir_instr_anticipated_expressions_in_get(method, succ, i);
				}
				if (inst->type == IRRET) {
					assert(!block);
				}
				// If the block has changed, update it and set modified to true
				if (block != ir_instr_anticipated_expressions_out_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_anticipated_expressions_out_set(method, inst, i, block);
				}
			}
			// Apply the transfer function to compute the new IN set (block by block)
			for (i=0; i < method->anticipatedExpressionsBlockNumbers; i++) {
				// IN = use U (OUT - kill) 
				// <=> use | (OUT & ~kill)
				// ~kill
				block = ~(ir_instr_anticipated_expressions_kill_get(method, inst, i));
				// OUT & ~kill
				block = block & ir_instr_anticipated_expressions_out_get(method, inst, i);
				// use | (OUT & ~kill)
				block = block | ir_instr_anticipated_expressions_gen_get(method, inst, i);
				if (block != ir_instr_anticipated_expressions_in_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_anticipated_expressions_in_set(method, inst, i, block);
				}
			}
		}
	} while (modified);

	/*********************************************
	 * Compute available expressions
	 ********************************************/

	// Initialize sets
	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method, instID);
		for (i = 0; i < method->availableExpressionsBlockNumbers; i++) {
			ir_instr_available_expressions_out_set(method, inst, i, ~0);
		}		
		if (instID == first) { 
			for (i=0; i < method->availableExpressionsBlockNumbers; i++) {
				ir_instr_available_expressions_out_set(method, inst, i, 0);
			}
		}
	}
	
	// Until a fixed point...
	do {
		modified = JITFALSE;
		// for each instruction, uptdate the IN/OUT sets
		for (instID = first; instID < count; instID++) {
			inst = IRMETHOD_getInstructionAtPosition(method, instID);
			// block by block update the IN sets
			for (i=0; i < method->availableExpressionsBlockNumbers; i++) {
				pred = NULL;
				if (instID == first) {
					block = 0;
				} else {
					block = ~0;
					// The new IN set is the intersection of all the predecessors' OUT sets
					while ((pred = IRMETHOD_getPredecessorInstruction(method,inst,pred)) != NULL) {
						block = block & ir_instr_available_expressions_out_get(method, pred, i);
					}
				}
				// If the block has changed, update it and set modified to true
				if (block != ir_instr_available_expressions_in_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_available_expressions_in_set(method, inst, i, block);
				}
			}
			// Apply the transfer function to compute the new OUT set (block by block)
			for (i=0; i < method->availableExpressionsBlockNumbers; i++) {
				// OUT = (anticipated[in] U IN) - kill
				// <=> (anticipated[in] | IN) & ~kill
				// anticipated[in]
				block = ir_instr_anticipated_expressions_in_get(method, inst, i);
				// anticipated[in] | IN
				block = block | ir_instr_available_expressions_in_get(method, inst, i);
				// & ~kill
				block = block & ~(ir_instr_available_expressions_kill_get(method, inst, i));
				if (block != ir_instr_available_expressions_out_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_available_expressions_out_set(method, inst, i, block);
				}
			}
		}
	} while (modified);

	/*********************************************
	 * Compute earliest entry point
	 ********************************************/

	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		for (i=0; i < method->earliestExpressionsBlockNumbers; i++) {
			// anticipated[in] - available[in]
			block = ~ir_instr_available_expressions_in_get(method, inst, i);
			block = block & ir_instr_anticipated_expressions_in_get(method, inst, i);
			ir_instr_earliest_expressions_earliestSet_set(method, inst, i, block);
		}
	}

	/*********************************************
	 * Compute postponable expressions
	 ********************************************/

	// Initialize sets
	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method, instID);
		for (i = 0; i < method->postponableExpressionsBlockNumbers; i++) {
			ir_instr_postponable_expressions_out_set(method, inst, i, ~0);
		}		
		if (instID == first) { 
			for (i=0; i < method->postponableExpressionsBlockNumbers; i++) {
				ir_instr_postponable_expressions_out_set(method, inst, i, 0);
			}
		}
	}
	
	// Until a fixed point...
	do {
		modified = JITFALSE;
		// for each instruction, uptdate the IN/OUT sets
		for (instID = first; instID < count; instID++) {
			inst = IRMETHOD_getInstructionAtPosition(method, instID);
			// block by block update the IN sets
			for (i=0; i < method->postponableExpressionsBlockNumbers; i++) {
				pred = NULL;
				if (instID == first) {
					block = 0;
				} else {
					block = ~0;
					// The new IN set is the intersection of all the predecessors' OUT sets
					while ((pred = IRMETHOD_getPredecessorInstruction(method,inst,pred)) != NULL) {
						block = block & ir_instr_postponable_expressions_out_get(method, pred, i);
					}
				}
				// If the block has changed, update it and set modified to true
				if (block != ir_instr_postponable_expressions_in_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_postponable_expressions_in_set(method, inst, i, block);
				}
			}
			// Apply the transfer function to compute the new OUT set (block by block)
			for (i=0; i < method->postponableExpressionsBlockNumbers; i++) {
				// OUT = (earliest U IN) - gen
				// <=> (earliest | IN) & ~use
				// earliest
				block = ir_instr_earliest_expressions_earliestSet_get(method, inst, i);
				// earliest | IN
				block = block | ir_instr_postponable_expressions_in_get(method, inst, i);
				// & ~use
				block = block & (~(ir_instr_postponable_expressions_use_get(method, inst, i)));
				if (block != ir_instr_postponable_expressions_out_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_postponable_expressions_out_set(method, inst, i, block);
				}
			}
		}
	} while (modified);

	/*********************************************
	 * Compute latest entry point
	 ********************************************/

	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		for (i=0; i < method->latestPlacementsBlockNumbers; i++) {
			succ = NULL;
			block = ~0;
			while ((succ = IRMETHOD_getSuccessorInstruction(method,inst,succ)) != NULL) {
				tempBlock = ir_instr_earliest_expressions_earliestSet_get(method, succ, i);
				tempBlock = tempBlock | ir_instr_postponable_expressions_in_get(method, succ, i);
				block = block & tempBlock;
			}
			block = (~block) | ir_instr_postponable_expressions_use_get(method, inst, i);
			tempBlock = ir_instr_earliest_expressions_earliestSet_get(method, inst, i);
			tempBlock = tempBlock | ir_instr_postponable_expressions_in_get(method, inst, i);
			block = block & tempBlock;
			ir_instr_latest_placements_latestSet_set(method, inst, i, block);
		}
	}

	/*********************************************
	 * Compute used expressions
	 ********************************************/

	// Initialize sets
	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method, instID);
		for (i = 0; i < method->usedExpressionsBlockNumbers; i++) {
			ir_instr_used_expressions_in_set(method, inst, i, 0);
			ir_instr_used_expressions_out_set(method, inst, i, 0);
		}		
	}
	
	// Until a fixed point...
	do {
		modified = JITFALSE;
		// for each instruction, uptdate the IN/OUT sets
		// XXX Why doesn't instID >= IRMETHOD_getMethodParametersNumber(method) work????
		for (instID = count-2; instID+1 > first; instID--) {
		//for (instID = IRMETHOD_getMethodParametersNumber(method); instID < count; instID++) {
			inst = IRMETHOD_getInstructionAtPosition(method, instID);
			// block by block update the OUT sets
			for (i=0; i < method->usedExpressionsBlockNumbers; i++) {
				succ = NULL;
				block = 0;
				// The new OUT set is the intersection of all the successors' IN sets
				while ((succ = IRMETHOD_getSuccessorInstruction(method,inst,succ)) != NULL) {
					block = block | ir_instr_used_expressions_in_get(method, succ, i);
				}
				// If the block has changed, update it and set modified to true
				if (block != ir_instr_used_expressions_out_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_used_expressions_out_set(method, inst, i, block);
				}
			}
			// Apply the transfer function to compute the new IN set (block by block)
			for (i=0; i < method->usedExpressionsBlockNumbers; i++) {
				// (use U OUT) - latest
				// <=> (use U OUT) & ~latest
				// use
				block = ir_instr_used_expressions_use_get(method, inst, i);
				// use U OUT
				block = block | ir_instr_used_expressions_out_get(method, inst, i);
				// - ~latest
				block = block & ~(ir_instr_latest_placements_latestSet_get(method, inst, i));
				if (block != ir_instr_used_expressions_in_get(method, inst, i)) {
					modified = JITTRUE;
					ir_instr_used_expressions_in_set(method, inst, i, block);
				}
			}
		}
	} while (modified);

	/*********************************************
	 * Compute where to add instructions
	 ********************************************/

	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		for (i=0; i < method->partialRedundancyEliminationBlockNumbers; i++) {
			// latest & used[out]
			block = ir_instr_latest_placements_latestSet_get(method, inst, i);
			block = block & ir_instr_used_expressions_out_get(method, inst, i);
			ir_instr_partial_redundancy_elimination_addedInstructions_set(method, inst, i, block);
		}
	}

	/*********************************************
	 * Print data flow sets
	 ********************************************/
	
	/*for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		//if (inst->type != IRRET) continue;
		PDEBUG("Use %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_postponable_expressions_use_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Kill %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_postponable_expressions_kill_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Anticipated %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_anticipated_expressions_in_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Available %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_available_expressions_in_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Earliest %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_earliest_expressions_earliestSet_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Postponable %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_postponable_expressions_in_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Latest %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_latest_placements_latestSet_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Used %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_used_expressions_out_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
		PDEBUG("Inserted %d: ", instID); 
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			block = ir_instr_partial_redundancy_elimination_addedInstructions_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				PDEBUG(" %d ", otherInstID);
			}
		}
		PDEBUG("\n");
	}
	*/

	/*********************************************
	 * Perform PRE
	 ********************************************/

	instList = xanListNew(allocFunction, freeFunction, NULL);
	exprList = xanListNew(allocFunction, freeFunction, NULL);
	usesNew = xanListNew(allocFunction, freeFunction, NULL);


	// For each instructions
	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		//addedNewVar = JITFALSE;
		//addedInstToList = JITFALSE;
		// For each EXPRESSION
		seenExprList = xanListNew(allocFunction, freeFunction, NULL);
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			if (instID != otherInstID) {
				otherInst = IRMETHOD_getInstructionAtPosition(method,otherInstID);
				block = ir_instr_partial_redundancy_elimination_addedInstructions_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
				block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
				// If the instruction is supposed to define the expression
				if (block) {
					seenExprItem = seenExprList->first(seenExprList);
					seenIt = JITFALSE;
					while (seenExprItem != NULL) {
						if (equiv_expression(method,(t_ir_instruction *)(seenExprItem->data),otherInst)) {
							seenIt = JITTRUE;
							otherInst->partial_redundancy_elimination.newVar = ((t_ir_instruction *)(seenExprItem->data))->partial_redundancy_elimination.newVar;
							inst->partial_redundancy_elimination.newVar = ((t_ir_instruction *)(seenExprItem->data))->partial_redundancy_elimination.newVar;
						}
						seenExprItem = seenExprItem->next;
					}
					if (seenIt) continue;
					if(!otherInst->partial_redundancy_elimination.analized){
						newVar = IRMETHOD_newVariableID(method);
						otherInst->partial_redundancy_elimination.analized = JITTRUE;
						otherInst->partial_redundancy_elimination.newVar = newVar;
						inst->partial_redundancy_elimination.newVar = newVar;
					
					}
					//inst->partial_redundancy_elimination.newVar = newVar;
					instList->append(instList, inst);
					exprList->append(exprList, otherInst);
					seenExprList->append(seenExprList,otherInst);
				}
			}
		}
		seenExprList->destroyList(seenExprList);
	}

	// For each instruction
	for (instID = first; instID < count; instID++) {
		inst = IRMETHOD_getInstructionAtPosition(method,instID);
		// For each EXPRESSION
		for (otherInstID = first; otherInstID < count; otherInstID++) {
			//if (!is_expression(inst)) continue;
			if (otherInstID == instID) continue;
			block = ~(ir_instr_latest_placements_latestSet_get(method,inst,otherInstID / (sizeof(JITNINT) * 8)));
			block = block | ir_instr_used_expressions_out_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = block & ir_instr_used_expressions_use_get(method,inst,otherInstID / (sizeof(JITNINT) * 8));
			block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
			if (block) {
				/*// Need to replace instruction with STORE of VAR for x + y
				// Find instruction that sets t and use its var number
				for (inst3ID = first; inst3ID < count; inst3ID++) {
					inst3 = IRMETHOD_getInstructionAtPosition(method,inst3ID);
					block = ir_instr_partial_redundancy_elimination_addedInstructions_get(method,inst3, otherInstID / (sizeof(JITNINT) * 8));
					block = (1 << (otherInstID % (sizeof(JITNINT) * 8))) & block;
					if (block) {
						inst->partial_redundancy_elimination.newVar = inst3->partial_redundancy_elimination.newVar;		
					}
				}*/
				usesNew->append(usesNew,inst);
				break;
			}
		}
	}


	// Inserts the t = x + y instructions
	listItem = instList->first(instList);
	exprItem = exprList->first(exprList);
	while (listItem != NULL) {
		inst = (t_ir_instruction *)listItem->data;
		usingInst = (t_ir_instruction *)exprItem->data;
		//PDEBUG("Inserting at %d with %d\n",inst->ID,usingInst->ID);
		otherInst = IRMETHOD_cloneInstruction(method,usingInst);
		//otherInst->result.value = inst->partial_redundancy_elimination.newVar;
		otherInst->result.value = usingInst->partial_redundancy_elimination.newVar;
		IRMETHOD_moveInstructionBefore(method,otherInst,inst);
		
		listItem = listItem->next;
		exprItem = exprItem->next;
		if (is_expression(inst)) {
			inst->partial_redundancy_elimination.newVar = usingInst->partial_redundancy_elimination.newVar;
			usesNew->append(usesNew,inst);
		}
	}
	instList->destroyList(instList);
	exprList->destroyList(exprList);

	// Replaces x + y with t
	listItem = usesNew->first(usesNew);
	while (listItem != NULL) {
		inst = (t_ir_instruction *)listItem->data;
		assert(inst != NULL);
		if (is_expression(inst)) {
			resultItem = inst->result;
			newVarValue = inst->partial_redundancy_elimination.newVar;
			assert(newVarValue != 0);
			IRMETHOD_eraseInstructionFields(method,inst);
			inst->type = IRSTORE;
			inst->param_2 = resultItem;
			inst->param_1 = resultItem;	
			inst->param_2.value = newVarValue;
		}
		listItem = listItem->next;
	}
	usesNew->destroyList(usesNew);

	//IROPTIMIZER_callMethodOptimization(irOptimizer,method,irLib,METHOD_PRINTER);
	IROPTIMIZER_optimizeMethodAtLevel(irOptimizer,irLib,method,2);
	//IROPTIMIZER_callMethodOptimization(irOptimizer,method,irLib,METHOD_PRINTER);
	
	return;
}

static inline char * pre_get_version (void){
	return VERSION;
}

static inline char * pre_get_informations (void){
	return INFORMATIONS;
}

static inline char * pre_get_author (void){
	return AUTHOR;
}

static inline JITUINT64 pre_get_invalidations (void){
	return INVALIDATE_ALL;
}
