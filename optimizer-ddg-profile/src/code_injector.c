/*
 * Copyright (C) 2012  Campanoni Simone
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
#include <optimizer_ddg_profile.h>
#include <code_injector.h>
#include <runtime.h>
#include <load_instructions.h>
#include <load_loops.h>
// End

static inline XanHashTable * internal_cache_instruction_positions (void);
static inline void internal_injectCodeToDumpDependenceTrace(BZFILE *compressedOutputFile, XanHashTable *methods, XanHashTable *insts, XanHashTable *methodsToConsider);
static inline void internal_inject_code_to_dump_trace_in_method (ir_method_t *m, XanHashTable *instsToConsider, XanHashTable *methodsToConsider);
static inline void internal_codeBlockExecuted (JITUINT32 cpuCycles, JITUINT32 memoryCycles);
static inline void internal_massage_code (void);

extern ir_optimizer_t                   *irOptimizer;
static BZFILE				*compressedOutputFile;
static FILE				*outputFile 		= NULL;
XanHashTable				*loopNamesTable		= NULL;
XanHashTable				*methodNamesTable	= NULL;
XanHashTable				*loopProfileTable	= NULL;

BZFILE * inject_code_to_program (void){
	XanHashTable		*table;
	XanHashTable		*insts;
	XanHashTableItem	*item;
	XanHashTable				*methodsToConsider	= NULL;

	/* Open the output file.
	 */
	outputFile              = fopen("ddg_trace.txt.bz2", "w");
	if (outputFile == NULL){
		print_err("ERROR: Opening ddg_trace.txt.bz2 file. ", errno);
		abort();
	}
	compressedOutputFile	= BZ2_bzWriteOpen(NULL, outputFile, 9, 0, 250);
	if (compressedOutputFile == NULL){
		abort();
	}
	RUNTIME_init(compressedOutputFile);

	/* Fetch the instruction positions of each method.
	 */
	table	= internal_cache_instruction_positions();

	/* Fetch the set of instructions to consider.
	 */
	methodsToConsider	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	insts			= load_instructions(methodsToConsider);

	/* Inject the code.
	 */
	internal_injectCodeToDumpDependenceTrace(compressedOutputFile, table, insts, methodsToConsider);

	/* Free the memory.
	 */
	item	= xanHashTable_first(table);
	while (item != NULL){
		xanHashTable_destroyTable(item->element);
		item	= xanHashTable_next(table, item);
	}
	xanHashTable_destroyTable(table);
	xanHashTable_destroyTable(insts);

	return compressedOutputFile ;
}

static inline void internal_injectCodeToDumpDependenceTrace(BZFILE *compressedOutputFile, XanHashTable *methods, XanHashTable *insts, XanHashTable *methodsToConsider){
	XanList			*loopsList;
	XanList			*loopNamesToProfile;
	XanListItem		*listItem;
	XanHashTableItem	*item;
	XanHashTable		*loops;
	JITUINT32		count;
	JITINT8			filename[DIM_BUF];
	FILE			*methodHashFile;

	/* Assertions.
	 */
	assert(compressedOutputFile != NULL);
	assert(methods != NULL);
	assert(insts != NULL);

	/* Analyze the loops.
	 */
	loops			= IRLOOP_analyzeLoops(LOOPS_analyzeCircuits);
	assert(loops != NULL);
	loopsList		= xanHashTable_toList(loops);
	assert(loopsList != NULL);
	loopsNames		= LOOPS_getLoopsNames(loops);
	assert(loopsNames != NULL);

	/* Create the hash of loop names.
	 */
	loopNamesTable		= load_loops();

	/* Profile the loops.
	 */
	LOOPS_profileLoopsWithNames(irOptimizer, loopsList, JITTRUE, loopsNames, dump_start_loop, dump_end_loop, dump_start_loop_iteration, NULL);

	/* Create the set of loops to analyze.
	 */
	loopProfileTable	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	SNPRINTF(filename, DIM_BUF, "%s_enable_loops", IRPROGRAM_getProgramName());
	loopNamesToProfile	= LOOPS_loadLoopNames(filename);
	assert(loopNamesToProfile != NULL);
	if (xanList_length(loopNamesToProfile) == 0){
		fprintf(stderr, "ERROR: File %s_enable_loops does not exist or it is empty.\n", IRPROGRAM_getProgramName());
		abort();
	}
	listItem		= xanList_first(loopNamesToProfile);
	while (listItem != NULL){
		char	*loopName;
		loopName	= listItem->data;
		assert(loopName != NULL);
		item		= xanHashTable_first(loopsNames);
		while (item != NULL){
			char *cachedName;
			cachedName	= (char *)item->element;
			if (STRCMP(cachedName, loopName) == 0){
				xanHashTable_insert(loopProfileTable, cachedName, cachedName);
				break ;
			}
			item		= xanHashTable_next(loopsNames, item);
		}
		assert(item != NULL);
		listItem		= listItem->next;
	}

	/* Create the hash of method names.
	 */
	methodHashFile	= fopen("method_IDs.txt", "w");
	if (methodHashFile == NULL){
		print_err("ERROR: Opening file \"method_hash.txt\". ", errno);
		abort();
	}

	/* Inject code to both the parallel method and everything is reachable from it.
	 */
	count			= 1;
	methodNamesTable	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	item			= xanHashTable_first(methods);
	while (item != NULL){
		ir_method_t		*m;
		JITINT8			*methodName;

		/* Fetch the method.
	 	 */
		m		= item->elementID;
		assert(m != NULL);

		/* Add the method name to the mapping.
		 */
		methodName 	= IRMETHOD_getSignatureInString(m);
		assert(methodName != NULL);
		fprintf(methodHashFile, "%u	%s\n", count, methodName);
		xanHashTable_insert(methodNamesTable, methodName, (void *)(JITNUINT)count);
		count++;

		/* Inject code for memory instructions.
		 */
		internal_inject_code_to_dump_trace_in_method(m, insts, methodsToConsider);
		
		/* Fetch the next element.
		 */
		item		= xanHashTable_next(methods, item);
	}

	/* Close the files.
	 */
	fclose(methodHashFile);

	/* Free the memory.
	 */
	IRLOOP_destroyLoops(loops);
	xanList_destroyList(loopsList);
	xanList_destroyList(loopNamesToProfile);

	return ;
}

static inline void internal_inject_code_to_dump_trace_in_method (ir_method_t *m, XanHashTable *instsToConsider, XanHashTable *methodsToConsider){
	XanList			*insts;
	XanList			*callInsts;
	XanList			*params;
	XanList			*nativeCalls;
	XanListItem		*item;
	XanList			*l;
	ir_item_t		methodNameParam;
	ir_instruction_t	*firstInst;
	ir_item_t		instPosParam;
	ir_item_t		input1Param;
	ir_item_t		input1SizeParam;
	ir_item_t		input2Param;
	ir_item_t		input2SizeParam;
	ir_item_t		outputParam;
	ir_item_t		outputSizeParam;
	JITINT8			*methodName;
	JITBOOLEAN		isAMethodThatContainsAConsideredLoop;

	/* Assertions.
	 */
	assert(m != NULL);
	assert(instsToConsider != NULL);

	/* Fetch the name of the method.
	 */
	methodName	= IRMETHOD_getSignatureInString(m);
	assert(methodName != NULL);

	/* Check if the current method has been already profiled.
	 */
	nativeCalls	= IRMETHOD_getInstructionsOfType(m, IRNATIVECALL);
	item		= xanList_first(nativeCalls);
	while (item != NULL){
		ir_instruction_t	*nativeCall;
		void			*methodCalled;

		/* Fetch the native call.
		 */
		nativeCall	= item->data;
		assert(nativeCall != NULL);

		/* Check whether the call is to a function private to this source file.
		 */
		methodCalled	= IRMETHOD_getCalledNativeMethod(nativeCall);
		if (	(methodCalled == dump_memory_instruction)		||
			(methodCalled == dump_start_method)			||
			(methodCalled == dump_end_method)			){

			/* Free the memory.
			 */
			xanList_destroyList(nativeCalls);

			return ;
		}

		/* Fetch the next element from the list.
		 */
		item		= item->next;
	}
	xanList_destroyList(nativeCalls);
	
	/* Fetch the first instruction.
	 */
	firstInst	= IRMETHOD_getFirstInstructionNotOfType(m, IRNOP);
	if (firstInst->type == IRLABEL){
		firstInst	= IRMETHOD_newInstructionOfTypeBefore(m, firstInst, IRNOP);
	}
	assert(firstInst != NULL);
	assert(firstInst->type != IRLABEL);

	/* Fetch the memory instructions that belong to the method.
	 */
	insts		= IRMETHOD_getMemoryInstructions(m);
	assert(insts != NULL);

	/* Fetch the call instructions that belong to the method.
	 */
	callInsts	= IRMETHOD_getCallInstructions(m);
	assert(callInsts != NULL);

	/* Allocate the necessary memory.
	 */
	params		= xanList_new(allocFunction, freeFunction, NULL);

	/* Inject the call for the begin of the method.
	 */
	memset(&methodNameParam, 0, sizeof(ir_item_t));
	methodNameParam.value.v		= (IR_ITEM_VALUE)(JITNUINT)methodName;
	methodNameParam.internal_type	= IRNUINT;
	methodNameParam.type		= methodNameParam.internal_type;
	xanList_insert(params, &methodNameParam);
	IRMETHOD_newNativeCallInstructionBefore(m, firstInst, "MemoryInstructionDumperBeginMethod", dump_start_method, NULL, params);
	xanList_emptyOutList(params);
	
	/* Inject the call for the end of the method.
	 */
	l	= IRMETHOD_getInstructionsOfType(m, IRRET);
	item	= xanList_first(l);
	while (item != NULL){
		ir_instruction_t	*retInst;

		/* Fetch the return instruction.
		 */
		retInst	= item->data;
		assert(retInst != NULL);

		/* Inject the code.
		 */
		IRMETHOD_newNativeCallInstructionBefore(m, retInst, "MemoryInstructionDumperEndMethod", dump_end_method, NULL, NULL);

		/* Fetch the next element from the list.
		 */
		item	= item->next;
	}

	/* Free the memory.
	 */
	xanList_destroyList(l);

	/* Fill up the call parameters.
	 */
	memset(&instPosParam, 0, sizeof(ir_item_t));
	memset(&input1Param, 0, sizeof(ir_item_t));
	memset(&input1SizeParam, 0, sizeof(ir_item_t));
	memset(&input2Param, 0, sizeof(ir_item_t));
	memset(&input2SizeParam, 0, sizeof(ir_item_t));
	memset(&outputParam, 0, sizeof(ir_item_t));
	memset(&outputSizeParam, 0, sizeof(ir_item_t));
	xanList_append(params, &instPosParam);
	xanList_append(params, &input1Param);
	xanList_append(params, &input1SizeParam);
	xanList_append(params, &input2Param);
	xanList_append(params, &input2SizeParam);
	xanList_append(params, &outputParam);
	xanList_append(params, &outputSizeParam);

	/* Inject the code.
	 */
	isAMethodThatContainsAConsideredLoop	= (xanHashTable_lookup(methodsToConsider, m) != NULL);
	item					= xanList_first(insts);
	while (item != NULL){
		ir_instruction_t	*inst;
		ir_instruction_t	*instPrev;

		/* Fetch the memory instruction.
	 	 */
		inst		= item->data;
		assert(inst != NULL);

		/* Fetch the previous instruction.
	 	 */
		instPrev	= IRMETHOD_getPrevInstruction(m, inst);
		assert(instPrev != NULL);

		/* Check if the instruction is used to forward variables shared among loop iterations.
		 */
		if (	(inst->type != IRARRAYLENGTH)	&& 
			(inst->type != IRLOADELEM) 	&& 
			(inst->type != IRSTOREELEM)	){
			ir_item_t		*par1;
			ir_item_t		*par2;
			ir_item_t		*par3;
			ir_item_t		*res;
			ir_instruction_t	*strLen1;
			ir_instruction_t	*strLen2;
			ir_instruction_t	*add1;
			ir_instruction_t	*add2;

			/* Fetch the parameters of the instruction.
			 */
			par1	= IRMETHOD_getInstructionParameter1(inst);
			par2	= IRMETHOD_getInstructionParameter2(inst);
			par3	= IRMETHOD_getInstructionParameter3(inst);
			res	= IRMETHOD_getInstructionResult(inst);

			/* Fetch the position of the memory instruction.
			 */
			instPosParam.value.v		= ((JITUINT32)(JITNUINT)xanHashTable_lookup(instsToConsider, inst));

			/* Check if the current instruction needs to be considered.
		 	 */
			if (	(instPosParam.value.v != 0)				||
				(!isAMethodThatContainsAConsideredLoop)			){

				/* Set the types of the parameters of the call.
				 */
				instPosParam.internal_type	= IRUINT32;
				instPosParam.type		= instPosParam.internal_type;
				input1Param.internal_type	= IRNUINT;
				input1Param.type		= input1Param.internal_type;
				input1SizeParam.internal_type	= IRUINT32;
				input1SizeParam.type		= input1SizeParam.internal_type;
				input2Param.internal_type	= input1Param.internal_type;
				input2Param.type		= input2Param.internal_type;
				input2SizeParam.internal_type	= input1SizeParam.internal_type;
				input2SizeParam.type		= input2SizeParam.internal_type;
				outputParam.internal_type	= IRNUINT;
				outputParam.type		= outputParam.internal_type;
				outputSizeParam.internal_type	= IRUINT32;
				outputSizeParam.type		= outputSizeParam.internal_type;

				/* Inject the code to dump the instruction at run time.
				 */
				input1Param.value.v		= 0;
				input1SizeParam.value.v		= 0;
				input2Param.value.v		= 0;
				input2SizeParam.value.v		= 0;
				outputParam.value.v		= 0;
				outputSizeParam.value.v		= 0;
				switch (inst->type){
					case IRLOADREL:
						if (par2->value.v == 0){
							memcpy(&input1Param, par1, sizeof(ir_item_t));

						} else {
							ir_instruction_t	*addInst;
							ir_item_t		*addInstResult;
							addInst		= IRMETHOD_newInstructionOfTypeBefore(m, inst, IRADD);
							IRMETHOD_cpInstructionParameter1(addInst, par1);
							IRMETHOD_cpInstructionParameter2(addInst, par2);
							IRMETHOD_setInstructionParameterWithANewVariable(m, addInst, par1->internal_type, par1->type_infos, 0);
							addInstResult	= IRMETHOD_getInstructionResult(addInst);
							memcpy(&input1Param, addInstResult, sizeof(ir_item_t));
						}
						input1SizeParam.value.v	= IRDATA_getSizeOfType(res);
						break;
					case IRSTOREREL:
						if (par2->value.v == 0){
							memcpy(&outputParam, par1, sizeof(ir_item_t));

						} else {
							ir_instruction_t	*addInst;
							ir_item_t		*addInstResult;
							addInst		= IRMETHOD_newInstructionOfTypeBefore(m, inst, IRADD);
							IRMETHOD_cpInstructionParameter1(addInst, par1);
							IRMETHOD_cpInstructionParameter2(addInst, par2);
							IRMETHOD_setInstructionParameterWithANewVariable(m, addInst, par1->internal_type, par1->type_infos, 0);
							addInstResult	= IRMETHOD_getInstructionResult(addInst);
							memcpy(&outputParam, addInstResult, sizeof(ir_item_t));
						}
						outputSizeParam.value.v	= IRDATA_getSizeOfType(par3);
						break;
					case IRMEMCPY:
						memcpy(&input1Param, par2, sizeof(ir_item_t));
						memcpy(&outputParam, par1, sizeof(ir_item_t));
						outputSizeParam.value.v	= IRDATA_getSizeOfType(par3);
						input1SizeParam.value.v	= outputSizeParam.value.v;
						break;
					case IRINITMEMORY:
						memcpy(&outputParam, par1, sizeof(ir_item_t));
						outputSizeParam.value.v	= IRDATA_getSizeOfType(par3);
						break;
					case IRSTRINGCMP:
						memcpy(&input1Param, par1, sizeof(ir_item_t));
						strLen1	= IRMETHOD_newInstructionOfTypeAfter(m, instPrev, IRSTRINGLENGTH);
						IRMETHOD_cpInstructionParameter(m, inst, 1, strLen1, 1);
						IRMETHOD_setInstructionParameterWithANewVariable(m, strLen1, IRUINT32, NULL, 0);
						
						add1	= IRMETHOD_newInstructionOfTypeAfter(m, strLen1, IRADD);
						IRMETHOD_cpInstructionParameter(m, strLen1, 0, add1, 1);
						IRMETHOD_setInstructionParameter2(m, add1, 1, 0, IRUINT32, IRUINT32, NULL);
						IRMETHOD_cpInstructionParameter(m, add1, 1, add1, 0);

						IRMETHOD_cpInstructionParameterToItem(m, add1, 0, &input1SizeParam);

						memcpy(&input2Param, par2, sizeof(ir_item_t));
						strLen2	= IRMETHOD_newInstructionOfTypeAfter(m, instPrev, IRSTRINGLENGTH);
						IRMETHOD_cpInstructionParameter(m, inst, 2, strLen2, 1);
						IRMETHOD_setInstructionParameterWithANewVariable(m, strLen2, IRUINT32, NULL, 0);

						add2	= IRMETHOD_newInstructionOfTypeAfter(m, strLen2, IRADD);
						IRMETHOD_cpInstructionParameter(m, strLen2, 0, add2, 1);
						IRMETHOD_setInstructionParameter2(m, add2, 1, 0, IRUINT32, IRUINT32, NULL);
						IRMETHOD_cpInstructionParameter(m, add2, 1, add2, 0);

						IRMETHOD_cpInstructionParameterToItem(m, add2, 0, &input2SizeParam);
						break ;
					default:
						fprintf(stderr, "Instruction %s is not handled.\n", IRMETHOD_getInstructionTypeName(inst->type));
						abort();
				}
				IRMETHOD_newNativeCallInstructionBefore(m, inst, "MemoryInstructionDumper", dump_memory_instruction, NULL, params);
			}
		}

		/* Fetch the next element from the list.
	 	 */
		item	= item->next;
	}

	/* Inject code to dump the call instructions.
	 */
	item		= xanList_first(callInsts);
	while (item != NULL){
		ir_instruction_t	*inst;

		/* Fetch the memory instruction.
	 	 */
		inst	= item->data;
		assert(inst != NULL);

		/* If the call instruction is a call to OS, then inject the code.
		 */
		if (inst->type == IRLIBRARYCALL){
	
			/* Inject the code.
			 */
			IRMETHOD_newNativeCallInstructionBefore(m, inst, "OSDumper", dump_os_instruction, NULL, NULL);

		} else if (inst->type != IRNATIVECALL){

			/* Check if this instruction needs to be considered.
			 */
			if (xanHashTable_lookup(instsToConsider, inst) != NULL){
				ir_instruction_t	*nativeCall;

				/* Fetch the position of the call instruction.
			 	*/
				instPosParam.value.v		= ((JITUINT32)(JITNUINT)xanHashTable_lookup(instsToConsider, inst));

				/* Inject the code.
				 */
				nativeCall	= IRMETHOD_newNativeCallInstructionBefore(m, inst, "CallDumper", dump_call_instruction, NULL, NULL);	
	 			IRMETHOD_addInstructionCallParameter(m, nativeCall, &instPosParam);
			}
		}

		/* Fetch the next element from the list.
	 	 */
		item	= item->next;
	}
	IROPTIMIZER_callMethodOptimization(irOptimizer, m, METHOD_PRINTER);

	/* Inject code to compute cycles spent in between instructions that could be dependent.
	 */
//	IRPROFILER_profileMethod(m, loop, internal_isInstructionIndependent, internal_codeBlockExecuted);

	/* Free the memory.
	 */
	xanList_destroyList(params);
	xanList_destroyList(insts);
	xanList_destroyList(callInsts);

	return ;
}

static inline void internal_codeBlockExecuted (JITUINT32 cpuCycles, JITUINT32 memoryCycles){
	executedCycles	+= cpuCycles;
	executedCycles	+= memoryCycles;

	return ;
}

static inline XanHashTable * internal_cache_instruction_positions (void){
	ir_method_t	*entryPoint;
	XanList		*l;
	XanListItem	*item;
	XanHashTable	*table;

	/* Allocate the necessary memory.
	 */
	table		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

	/* Fetch the entry point of the program.
	 */
	entryPoint	= IRPROGRAM_getEntryPointMethod();
	assert(entryPoint != NULL);

	/* Fetch the list of methods reachable from the entry point.
	 */
	l		= IRPROGRAM_getReachableMethods(entryPoint);
	assert(l != NULL);

	/* Cache the positions of instructions of each method.
	 */
	item		= xanList_first(l);
	while (item != NULL){
		ir_method_t		*m;

		/* Fetch the method to consider.
		 */
		m		= item->data;
		assert(m != NULL);

		/* Check whether the method should be considered.
		 */
		if (	(!IRPROGRAM_doesMethodBelongToTheBaseClassLibrary(m))	&&
			(!IRPROGRAM_doesMethodBelongToALibrary(m))		&&
			(IRMETHOD_getSignatureInString(m) != NULL)		&&
			(IRMETHOD_hasMethodInstructions(m))			){
			XanHashTable		*instsPos;

			/* Fetch the positions of instructions.
			 */
			instsPos	= IRMETHOD_getInstructionPositions(m);
			assert(instsPos != NULL);

			/* Cache the positions.
			 */
			xanHashTable_insert(table, m, instsPos);
		}

		/* Fetch the next element from the list.
		 */
		item		= item->next;
	}

	/* Free the memory.
	 */
	xanList_destroyList(l);

	return table;
}

static inline void internal_massage_code (void){
	ir_method_t	*entryPoint;
	XanList		*l;
	XanListItem	*item;

	/* Fetch the entry point of the program.
	 */
	entryPoint	= IRPROGRAM_getEntryPointMethod();
	assert(entryPoint != NULL);

	/* Fetch the list of methods reachable from the entry point.
	 */
	l		= IRPROGRAM_getReachableMethods(entryPoint);
	assert(l != NULL);

	/* Inject the code.
	 */
	item		= xanList_first(l);
	while (item != NULL){
		ir_method_t	*m;
		m	= item->data;
		assert(m != NULL);
		METHODS_addEmptyBasicBlocksJustBeforeInstructionsWithMultiplePredecessors(m);
		item	= item->next;
	}

	/* Free the memory.
	 */
	xanList_destroyList(l);

	return ;
}
