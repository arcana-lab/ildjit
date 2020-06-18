/*
 * Copyright (C) 2011 - 2012 Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine_execution.cpp - This file handles functions that belong to the backend API related to running produced code.

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
#include <ir_virtual_machine.h>
#include <compiler_memory_manager.h>
#include <iostream>
#include <fstream>
#include <ir_optimizer.h>
#include <ir_optimization_interface.h>
#include <xanlib.h>

// My headers
#include <utilities.h>
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <config.h>
// End

using namespace std;

struct CallFrame {
  CallFrame * prevFrame;
  void * returnAddr;
};

static Function * internal_get_Function (IRVM_internal_t *llvmRoots) __attribute__((unused));

ir_method_t * IRVM_getRunningMethod (IRVM_t *self){
	IRVM_internal_t 		*llvmRoots __attribute__((unused));
	Function 			*llvmF     __attribute__((unused));
	ir_method_t			*irMethod;

	/* Initialize the variables		*/
	irMethod	= NULL;

	/* Fetch LLVM's top-level handles	*/
	llvmRoots = (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the LLVM function		*/

	irMethod	= IRPROGRAM_getMethodWithSignatureInString((JITINT8 *)const_cast<char *>("System.String System.Object._(System.String tag)"));
	if (irMethod == NULL) abort();

	//llvmF		=  internal_get_Function(llvmRoots);
	//if (llvmF != NULL){

		/* Fetch the IR method			*/
	//	irMethod	= (ir_method_t *)xanHashTable_lookup(llvmRoots->irMethods, llvmF);
	//}

	return irMethod;
}

ir_method_t * IRVM_getCallerOfMethodInExecution (IRVM_t *self, ir_method_t *m){
	abort();
}

void IRVM_getRunningMethodCallerInstruction (IRVM_t *self, ir_method_t **method, ir_instruction_t **instr){
	abort();
}

JITINT32 IRVM_run (IRVM_t *self, t_jit_function *backendFunction, void **args, void *returnArea){
    	vector<GenericValue> 		argValues;
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t 		*llvmRoots;
	FunctionType			*signature;
 	GenericValue			v;

	/* Assertions.
	 */
	assert(backendFunction != NULL);
	assert(args != NULL);
	assert(returnArea != NULL);

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots = (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

    	/* Fetch the LLVM function.
	 */
	llvmFunction 	= (t_llvm_function_internal *) backendFunction->data;
	assert(llvmFunction != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch the LLVM signature.
	 */
	signature	= llvmFunction->functionSignature;

    	/* Generate the arguments.
	 */
	for (JITUINT32 count=0; count < signature->getNumParams(); count++){
		Type	*param;

		/* Fetch a parameter.
		 */
		param	= signature->getParamType(count);

		/* Create the corresponding LLVM argument.
		 */
		if (param->isIntegerTy()){
			IntegerType	*intParam;
			intParam	= (IntegerType *)param;
			v.IntVal	= APInt(intParam->getBitWidth(), *((IR_ITEM_VALUE *)args[count]));

		} else if (param->isPointerTy()){
			v		= PTOGV(*((void **)args[count]));
			
		} else {
			abort();
		}

		/* Push the argument.
		 */
		argValues.push_back(v);
	}

    	/* Run the function and if necessary, generate the machine code.
	 */
    	v	= llvmRoots->executionEngine->runFunction(llvmFunction->function, argValues);
	
	/* Store the returned value.
	 */
	if (returnArea != NULL){
		Type	*retType;
		retType	= signature->getReturnType();
		if (retType->isIntegerTy()){
			*((JITINT32 *)returnArea)	= *((JITINT32 *)const_cast<uint64_t *>(v.IntVal.getRawData()));

		} else if (retType->isPointerTy()){
			*((void **)returnArea)		= ((void *)v.PointerVal);

		} else if (retType->isDoubleTy()){
			*((JITFLOAT64 *)returnArea)	= ((JITFLOAT64)v.DoubleVal);

		} else if (retType->isVoidTy()){

		} else {
			abort();
		}
	}

	return 1;
}

static Function * internal_get_Function (IRVM_internal_t *llvmRoots){
	CallFrame	*framePtr;
	/*#if SIZEOF_VOID_PTR == 8
	__asm__("movq %%rbp, %0" :"=r"(framePtr));
	#else
	__asm__("movl %%ebp, %0" :"=r"(framePtr));
	#endif*/

	while (framePtr != NULL) {
		XanHashTableItem	*item;
		void 		*returnAddr = framePtr->returnAddr;
		if (returnAddr == 0) break ;
		framePtr = framePtr->prevFrame;
		item	= xanHashTable_first(llvmRoots->llvmFunctions);
		while (item != NULL){
			t_jit_function			*backendF;
			t_llvm_function_internal	*llvmF;
			backendF	= (t_jit_function *)item->element;
			llvmF		= (t_llvm_function_internal *)backendF->data;
			if (	(returnAddr >= (llvmF->entryPoint))	&&
				(returnAddr < (llvmF->lastPoint))	){
				return llvmF->function;
			}
			item		= xanHashTable_next(llvmRoots->llvmFunctions, item);
		}
	}
	return NULL;
}
