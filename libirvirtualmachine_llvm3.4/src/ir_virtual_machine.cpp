/*
 * Copyright (C) 2011 - 2012 Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine.cpp - This is a translator from the IR language into the assembly language.

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
#include <ir_virtual_machine.h>
#include <compiler_memory_manager.h>
#include <iostream>
#include <fstream>
#include <ir_optimizer.h>
#include <ir_optimization_interface.h>
#include <xanlib.h>
#include <jit_metadata.h>
#include <platform_API.h>

// My headers
#include <utilities.h>
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <config.h>
#include <MachineCodeGeneratorEventListener.h>
// End

//#define IR_LLVM_DO_NOT_USE_INTRINSIC

// Why do we open namespace std, but still qualify std names explicitly?
using namespace std;

typedef void (*_Unwind_Exception_Cleanup_Fn) (_Unwind_Reason_Code, struct _Unwind_Exception *);

/* Exception class used to escape from ExecutionEngine::finalizeObject after a relocatable
 * object file has been dumped.
 */
class ObjectFileWritten {
public:
	ObjectFileWritten() { }
};

/* Subclass ObjectCache to obtain a hook that allows dumping a binary object after a
 * module has been translated to relocatable machine code but before it's been prepared
 * for execution.
 */
class ObjectFiler : public ObjectCache {
public:
	ObjectFiler() { }
	virtual ~ObjectFiler() { }

	/* Called before a module is compiled. Return NULL to indicate that the module
	 * needs to be compiled.
	 */
	virtual MemoryBuffer *getObject(const Module *M) {
		return NULL;
	}

	/* Called after module has been compiled to relocatable machine code. Write object
	 * file to disk, then throw and exception to skip the rest of object finalization.
	 */
	virtual void notifyObjectCompiled(const Module *M, const MemoryBuffer *obj) {
		const std::string moduleID = M->getModuleIdentifier();

		std::string objectFileName = moduleID + ".o";
		std::string errStr;
    
		raw_fd_ostream IRObjectStream(objectFileName.c_str(), errStr, sys::fs::F_Binary);
		IRObjectStream << obj->getBuffer();

		throw(ObjectFileWritten());
	}
};

static inline char * internal_linkerCommandLine (IRVM_t *self);
static inline void internal_translateGlobals (IRVM_t *self);
static inline Value * internal_fetchEffectiveAddress (IRVM_t *self, IRVM_internal_t *llvmRoots, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, ir_method_t *method, ir_item_t *irBa, ir_item_t *irOffset, Type *llvmPtrType);
static inline Value * internal_allocateANewVariable (Type *llvmType, JITUINT32 alignment, BasicBlock *entryBB);
static inline void translate_ir_sqrt (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_sin (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_cos (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_cosh (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_ceil (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_acos (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_isnan (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_isinf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_pow (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_exp (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_log10 (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_floor (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_callfinally (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_startfinally (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_alloca (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_alloc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_calloc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_allocalign (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_realloc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_free (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_startcatcher (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_branchifpcnotinrange (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_endfinally (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_throw (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_exit (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_sizeof (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_free_object (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_new_object (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_new_array (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_memset (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_memset_intrinsic (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_memset_libc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_memcpy (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_memcpy_libc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_memcpy_intrinsic (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) ;
static inline void translate_ir_memcmp (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_shr (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_shl (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_or (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_and (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_not (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_xor (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_array_length (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_string_length (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_string_cmp (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_string_chr (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_load_rel (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_store_rel (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_branch (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_branch_if (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_branch_if_not (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_virtual_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_icall (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_library_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_native_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_ret (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_add (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_add_ovf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_mul (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_mul_ovf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_div (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_rem (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_neg (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_sub (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_conv (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_bitcast (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_conv_ovf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_move (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_move (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_item_t *irDst, ir_item_t *irSrc, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_eq (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_lt (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_gt (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);
static inline void translate_ir_get_address (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB);

static inline void translate_ir_generic_math (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, Function *f32, Function *f64, JITUINT16 irSignature32ResultType, JITUINT16 irSignature64ResultType);
static inline void translate_ir_conditional_branch (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, CmpInst::Predicate predicate);
static inline void translate_ir_binary (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, Instruction::BinaryOps floatOp, Instruction::BinaryOps signedIntOp, Instruction::BinaryOps unsignedIntOp);
static inline void translate_ir_binary_comparison (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, CmpInst::Predicate floatOp, CmpInst::Predicate signedIntOp, CmpInst::Predicate unsignedIntOp);
static inline void translate_ir_general_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, JITUINT32 returnIRType, TypeDescriptor *returnIlType, IR_ITEM_VALUE functionPointer, JITBOOLEAN isLLVMFunction, char *functionName, ir_method_t *irCalledMethod, JITBOOLEAN directCall);
static inline void setup_llvm_variables (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, t_llvm_function_internal *llvmFunction, BasicBlock *entryBB);
static inline void internal_free_memory_used_for_previous_translations (t_llvm_function_internal *llvmFunction);
static inline void error_trampoline (void);
static inline void internal_generate_instruction_range_store (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst);

void IRVM_optimizeProgram (IRVM_t *self){
	IRVM_internal_t 		*llvmRoots;

	/* Check if we have been configured as a static compiler.
	 */
	if (	(!(self->behavior).staticCompilation)	&&
		(!(self->behavior).aot)			){
		return ;
	}

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 	= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Apply per module code optimization algorithms available in LLVM.
	 */
	llvmRoots->MPM->run(*llvmRoots->llvmModule);

	return ;
}

void IRVM_generateMachineCode (IRVM_t *self, t_jit_function *jitFunction, ir_method_t *method){
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t 		*llvmRoots;

	/* Postpone machine code generation in the static case.
	 * This is a requirement of MCJIT, which is used in the static case.
	 */
	if ((self->behavior).staticCompilation){
		return ;
	}

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 			        = (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the backend method.
	 */
	llvmFunction			    = (t_llvm_function_internal *)jitFunction->data;

	/* Generate the machine code.
	 */
	if (!llvmFunction->function->empty()){
	    llvmFunction->entryPoint	= llvmRoots->executionEngine->getPointerToFunction(llvmFunction->function);
    } else {
	    llvmFunction->entryPoint	= llvmRoots->executionEngine->getPointerToFunctionOrStub(llvmFunction->function);
    }

	return ;
}

void * IRVM_getFunctionPointer (IRVM_t *self, t_jit_function *backendFunction){
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t 		*llvmRoots;
    void				*functionPointer;

    /* Assertions.
	 */
	assert(backendFunction != NULL);

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 		= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the LLVM function.
	 */
	llvmFunction 		= (t_llvm_function_internal *) backendFunction->data;

	/* Fetch the entry point.
	 */
	if (llvmFunction->entryPoint == NULL){
	
		/* Check whether we are going to dump the machine code.
		 */
		if ((self->behavior).staticCompilation){

			/* We are going to dump the machine code.
			 * In this case, we cannot create any trampoline. 
			 * Hence, a random pointer is returned and this is ok because the code is not going to be actually executed while the machine code is generating.
			 */
			if (llvmFunction->entryPoint == NULL){
				llvmFunction->entryPoint	= &llvmFunction->entryPoint;
			}

		} else {

			/* We are not going to dump the machine code.
			 * Hence, the code generated is going to be executed.
			 * The entry point must be a valid pointer (i.e., either to the actual entry point or to a trampoline).
			 * We are going to handle differently dynamic and static compilations.
			 * The reason is that on static compilation schemes, dead functions are removed. 
			 * Hence, the pointer to llvmFunction->function is invalid for them.
			 */
			if (!(self->behavior).aot){

				/* Dynamic compilation.
				 */
				llvmFunction->entryPoint	= llvmRoots->executionEngine->getPointerToFunctionOrStub(llvmFunction->function);

			} else {
				ir_method_t	*irMethod;

				/* AOT compilation.
				 */
				irMethod			= (ir_method_t *)xanHashTable_lookup(llvmRoots->irMethods, llvmFunction->function);
				assert(irMethod != NULL);
				if (IRMETHOD_hasMethodInstructions(irMethod)){
					llvmFunction->entryPoint	= llvmRoots->executionEngine->getPointerToFunctionOrStub(llvmFunction->function);
				} else {
	      				llvmFunction->entryPoint	= &llvmFunction->entryPoint;
				}
			}
		}
	}
	functionPointer	= llvmFunction->entryPoint;

    return functionPointer;
}

JITUINT32 IRVM_getIRVMTypeSize (IRVM_t *self, IRVM_type *irvmType){
	IRVM_type_internal_t		*t;
	ir_item_t			item;

    /* Assertions.
     */
    assert(irvmType != NULL);

	/* Fetch the LLVM type			*/
	t	= (IRVM_type_internal_t *)irvmType->type;
	assert(t != NULL);

	/* Fetch the bytes			*/
	if (t->typeSize == 0){
		memset(&item, 0, sizeof(ir_item_t));
		item.type		= t->irType;
		item.internal_type	= t->irType;
		item.type_infos		= t->ilType;
		t->typeSize		= IRDATA_getSizeOfType(&item);
	}

	return t->typeSize;
}

void IRVM_startPreCompilation (IRVM_t *self){
	internal_translateGlobals(self);

	return ;
}

void IRVM_finishPreCompilation (IRVM_t *self){
	IRVM_internal_t 		*llvmRoots;
	char				*linkCommand;

	/* Assertions.
	 */
	assert(self != NULL);

	/* Check if we have been configured as an AOT or a static compiler.
	 */
	if (	(!(self->behavior).staticCompilation)		&&
		(!(self->behavior).aot)				){
		return ;
	}

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 	= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Check if we have been configured as an AOT compiler.
	 * Only the AOT compiler uses the legacy LLVM JIT and generate all machine code before starting the execution.
 	 * Hence, only the static compiler needs to generate and dump the machine code.
	 */
	if ((self->behavior).aot){
		return ;
	}
	assert((self->behavior).staticCompilation);

	/* Dump the module bitcode.
	 */
	if ((self->behavior).dumpAssembly.dumpAssembly) {
		llvmRoots->llvmModule->dump();
	}

	/* We have been configured to use the static compiler, so close the LLVM module and generate machine
	 * code.  During the call to finalizeObject, ObjectFiler::notifyObjectCompiled is called to dump an
	 * object file that contains the relocatable machine code. It ends by throwing an exception, so that
	 * the rest of object finalization will be skipped.
	 */
	ObjectFiler objectFiler;

	llvmRoots->executionEngine->setObjectCache(&objectFiler);
	try {
		llvmRoots->executionEngine->finalizeObject();
	} catch(ObjectFileWritten) {
	}
	llvmRoots->executionEngine->setObjectCache(NULL);

	/* Objects have been dumped. Now it is time to link them with ILDJIT libraries.
 	 * 
	 * Fetch the command line to use to invoke the linker.
	 */
	linkCommand	= internal_linkerCommandLine(self);
	assert(linkCommand != NULL);

	/* Check if we are in the verbose mode.
	 */
	if ((self->behavior).verbose){
		printf("ILDJIT: Linker command line = \"%s\"\n", linkCommand);
	}
 
	/* Check whether the linker has been enabled or not.
	 */
	if ((self->behavior).noLinker == JITFALSE){

		/* Link the object file with the libraries.
		 */
		if (system(linkCommand) == 0){

			/* Remove object files.
			 */
			if (system("rm *.o") == -1){
				fprintf(stderr, "ILDJIT: ERROR on removing object files\n");
				abort();
			}

		} else {
			fprintf(stderr, "ILDJIT: ERROR on linking generated object files with ILDJIT libraries.\n");
		}
	}

	/* Free the memory.
	 */
	freeFunction(linkCommand);

	return ;
}

void IRVM_translateIRMethodSignatureToBackendSignature (IRVM_t *self, ir_signature_t *ir_signature, t_jit_function *backendFunction){
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t 		*llvmRoots;
	std::vector<Type*> 		parameterTypes;
	Type				*returnType;

	/* Assertions				*/
	assert(self != NULL);
	assert(ir_signature != NULL);
	assert(backendFunction != NULL);

	/* Fetch LLVM's top-level handles	*/
	llvmRoots 				= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the LLVM function		*/
	if (backendFunction->data == NULL){
		backendFunction->data		= (t_llvm_function_internal *) allocMemory(sizeof(t_llvm_function_internal));
	}
	llvmFunction = (t_llvm_function_internal *) backendFunction->data;

	/* Create the LLVM signature of the LLVM*
	 * function				*/
	for (JITUINT32 count=0; count < ir_signature->parametersNumber; count++){
		Type *llvmType;
		TypeDescriptor	*ilType;
		ilType	= NULL;
		if (ir_signature->ilParamsTypes != NULL){
			ilType	= ir_signature->ilParamsTypes[count];
		}
		llvmType = get_LLVM_type(self, llvmRoots, ir_signature->parameterTypes[count], ilType);
  		parameterTypes.push_back(llvmType);
	}
	returnType			= get_LLVM_type(self, llvmRoots, ir_signature->resultType, ir_signature->ilResultType);
  	llvmFunction->functionSignature	= FunctionType::get(returnType, parameterTypes, false);

	return ;
}

JITINT32 IRVM_isANativeFunction (IRVM_t *self, t_jit_function *backendFunction){
	t_llvm_function_internal	*llvmFunction;

	/* Assertions				*/
	assert(backendFunction != NULL);
	assert(backendFunction->data != NULL);
	
	/* Fetch the LLVM function		*/
	llvmFunction = (t_llvm_function_internal *) backendFunction->data;

	return llvmFunction->isNative;
}	

void * IRVM_getEntryPoint (IRVM_t *self, t_jit_function *backendFunction){
	t_llvm_function_internal	*llvmFunction;

	/* Assertions				*/
	assert(backendFunction != NULL);

	/* Fetch the LLVM function		*/
	if (backendFunction->data == NULL){
		return NULL;
	}
	llvmFunction = (t_llvm_function_internal *) backendFunction->data;

	return llvmFunction->entryPoint;
}

JITINT32 IRVM_hasTheSignatureBeenTranslated (IRVM_t *self, t_jit_function *backendFunction){
	t_llvm_function_internal	*llvmFunction;

	/* Assertions				*/
	assert(backendFunction != NULL);

	/* Fetch the LLVM function		*/
	if (backendFunction->data == NULL){
		return JITFALSE;
	}
	llvmFunction = (t_llvm_function_internal *) backendFunction->data;

	return (llvmFunction->functionSignature != NULL);
}

void IRVM_setNativeEntryPoint (IRVM_t *self, t_jit_function *backendFunction, void *nativeEntryPoint){
	t_llvm_function_internal	*llvmFunction;

	/* Assertions				*/
	assert(backendFunction != NULL);

	/* Fetch the LLVM function		*/
	if (backendFunction->data == NULL){
		backendFunction->data	= (t_llvm_function_internal *) allocMemory(sizeof(t_llvm_function_internal));
	}
	llvmFunction 			= (t_llvm_function_internal *) backendFunction->data;

	/* Set the entry point			*/
	llvmFunction->entryPoint 	= nativeEntryPoint;
	llvmFunction->isNative		= JITTRUE;

	return ;
}

void IRVM_lowerMethod (IRVM_t *self, ir_method_t *method, t_jit_function *jitFunction){
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t 		*llvmRoots;
	JITUINT32			bbNumber;
	BasicBlock			*entryBB;
	ir_instruction_t		**labels;
	IR_ITEM_VALUE			maxLabel;

	/* Assertions.
	 */
	assert(method != NULL);
	assert(jitFunction != NULL);

	/* Translate globals.
	 */
	internal_translateGlobals(self);

	/* Translate the signature.
	 */
	allocateBackendFunction(self, jitFunction, method);
	assert(jitFunction->data != NULL);

	/* Cache some pointers			*/
	llvmFunction = (t_llvm_function_internal *) jitFunction->data;
	assert(llvmFunction != NULL);
	assert(llvmFunction->signature != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch LLVM's top-level handles	*/
	llvmRoots = (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Update the number of variables needed by the method.
	 */
	IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

	/* Compute the IR basic blocks		*/
	IROPTIMIZER_callMethodOptimization(&(self->optimizer), method, BASIC_BLOCK_IDENTIFIER);
	if (!IROPTIMIZER_isInformationValid(method, BASIC_BLOCK_IDENTIFIER)){
		fprintf(stderr, "ILDJIT_LLVM: Error, basic block identifier has not been enabled.\n");
		abort();
	}
   
	/* Free any memory allocated in a	*
	 * previous translation			*/
	internal_free_memory_used_for_previous_translations(llvmFunction);
	
	/* Allocate memory to store variables	*/
	assert(IRMETHOD_getNumberOfVariablesNeededByMethod(method) >= (IRMETHOD_getMethodLocalsNumber(method) + IRMETHOD_getMethodParametersNumber(method)));
	if (IRMETHOD_getNumberOfVariablesNeededByMethod(method) > 0) {
		llvmFunction->variables = (Value **) allocMemory(sizeof(Value *) * (IRMETHOD_getNumberOfVariablesNeededByMethod(method) + 1));
	}

	/* Allocate memory to cache labels	*/
	labels		= NULL;
	maxLabel	= IRMETHOD_getMaxLabelID(method) + 1;
	labels		= (ir_instruction_t **)allocFunction(sizeof(ir_instruction_t *) * maxLabel);

	/* Fetch the number of basic blocks	*/
	bbNumber	= IRMETHOD_getNumberOfMethodBasicBlocks(method);
	assert(bbNumber > 0);

	/* Create the LLVM entry point			
	 * LLVM requires that the entry point of the method has no predecessor, IR language does not.
  	 * For this missmatch we need to create an extra basic block (i.e., entryBB) that allocate space for variable and jumps to the basic block associated to the first IR basic block.	*/
	entryBB		= BasicBlock::Create(llvmRoots->llvmModule->getContext(), "LLVM_entry_point", llvmFunction->function, 0);

	/* Create the LLVM basic blocks		*/
	llvmFunction->basicBlocks			= (BasicBlock **)allocMemory(sizeof(BasicBlock *) * (bbNumber + 1));
	llvmFunction->basicBlockAddresses		= (BlockAddress **)allocMemory(sizeof(BlockAddress *) * bbNumber);
	llvmFunction->finallyBlockReturnAddresses	= (Value **)allocMemory(sizeof(Value *) * bbNumber);
	for (JITUINT32 bbPos=0; bbPos < (bbNumber-1); bbPos++){
		IRBasicBlock		*bb;
		ir_instruction_t	*firstInst;
		char 			bbPosString[DIM_BUF];

		/* Fetch the basic block.
	 	 */
		bb 		= IRMETHOD_getBasicBlockAtPosition(method, bbPos);
		assert(bb != NULL);
		snprintf(bbPosString, DIM_BUF, "BB_%u_%u", bb->startInst, bb->endInst);

		/* If the last basic block does not contain instruction, skip it.
		 */
		if (	(bbPos == (bbNumber - 2))							&&
			(bb->startInst == bb->endInst)							&&
			(IRMETHOD_getFirstInstructionWithinBasicBlock(method, bb)->type == IRNOP)	){
			continue ;
		}

		/* Create the LLVM basic block.
		 */
		llvmFunction->basicBlocks[bbPos]	= BasicBlock::Create(llvmRoots->llvmModule->getContext(), bbPosString, llvmFunction->function, 0);

		/* Cache the label.
		 */
		firstInst	= IRMETHOD_getFirstInstructionWithinBasicBlock(method, bb);
		if (firstInst->type == IRLABEL){
			labels[(firstInst->param_1).value.v]	= firstInst;
		}
	}

	/* Create the landing pad basic block	*/
	if (IRMETHOD_getCatcherInstruction(method) != NULL){
		char 		bbPosString[DIM_BUF];
		snprintf(bbPosString, DIM_BUF, "BB_landingpad");
		llvmFunction->basicBlocks[bbNumber]	= BasicBlock::Create(llvmRoots->llvmModule->getContext(), bbPosString, llvmFunction->function, 0);
	}

	/* Set up low-level variables		*/
	setup_llvm_variables(self, llvmRoots, method, llvmFunction, entryBB);

	/* Fill up the basic blocks		*/
	for (JITUINT32 bbPos=0; bbPos < (bbNumber-1); bbPos++){
		IRBasicBlock	*bb;
		BasicBlock	*llvmBB;

		/* Fetch the current basic block	*/
		bb 	= IRMETHOD_getBasicBlockAtPosition(method, bbPos);
		llvmBB	= llvmFunction->basicBlocks[bbPos];
		if (llvmBB == NULL){
			continue ;
		}

		/* Fill up the basic block		*/
		for (JITUINT32 instPos=bb->startInst; instPos <= bb->endInst; instPos++){
			ir_instruction_t	        *currentIRInstruction;

			/* Fetch the instruction		*/
			currentIRInstruction	= IRMETHOD_getInstructionAtPosition(method, instPos);
			assert(currentIRInstruction != NULL);

			/* Convert the current IR instruction	*/
			switch (currentIRInstruction->type) {
				case IRCHECKNULL:
					//TODO
					break;
				case IRBRANCHIFPCNOTINRANGE:
					translate_ir_branchifpcnotinrange(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSTARTCATCHER:
					translate_ir_startcatcher(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRISNAN:
					translate_ir_isnan(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRISINF:
					translate_ir_isinf(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSQRT:
					translate_ir_sqrt(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSIN:
					translate_ir_sin(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCOS:
					translate_ir_cos(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCOSH:
					translate_ir_cosh(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCEIL:
					translate_ir_ceil(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRACOS:
					translate_ir_acos(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRFLOOR:
					translate_ir_floor(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRPOW:
					translate_ir_pow(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IREXP:
					translate_ir_exp(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRLOG10:
					translate_ir_log10(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRALLOCALIGN:
					translate_ir_allocalign(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRALLOC:
					translate_ir_alloc(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCALLOC:
					translate_ir_calloc(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRREALLOC:
					translate_ir_realloc(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRALLOCA:
					translate_ir_alloca(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRFREE:
					translate_ir_free(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSTARTFINALLY:
					translate_ir_startfinally(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRENDFINALLY:
					translate_ir_endfinally(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCALLFINALLY:
					translate_ir_callfinally(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRTHROW:
					translate_ir_throw(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IREXIT:
					translate_ir_exit(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSIZEOF:
					translate_ir_sizeof(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRFREEOBJ:
					translate_ir_free_object(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRNEWOBJ:
					translate_ir_new_object(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRNEWARR:
					translate_ir_new_array(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRMEMCPY:
					translate_ir_memcpy(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRMEMCMP:
					translate_ir_memcmp(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRINITMEMORY:
					translate_ir_memset(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IROR:
					translate_ir_or(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRAND:
					translate_ir_and(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRNOT:
					translate_ir_not(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRXOR:
					translate_ir_xor(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSHL:
					translate_ir_shl(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSHR:
					translate_ir_shr(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRGT:
					translate_ir_gt(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRLT:
					translate_ir_lt(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IREQ:
					translate_ir_eq(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRGETADDRESS:
					translate_ir_get_address(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRARRAYLENGTH:
					translate_ir_array_length(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSTRINGLENGTH:
					translate_ir_string_length(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSTRINGCMP:
					translate_ir_string_cmp(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSTRINGCHR:
					translate_ir_string_chr(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRLOADREL:
					translate_ir_load_rel(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSTOREREL:
					translate_ir_store_rel(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRBRANCH:
					translate_ir_branch(self, llvmRoots, labels, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRBRANCHIF:
					translate_ir_branch_if(self, llvmRoots, labels, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRBRANCHIFNOT:
					translate_ir_branch_if_not(self, llvmRoots, labels, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCONV:
					translate_ir_conv(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCONVOVF:
					translate_ir_conv_ovf(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRBITCAST:
					translate_ir_bitcast(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRLIBRARYCALL:
					translate_ir_library_call(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRNATIVECALL:
					translate_ir_native_call(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRCALL:
					translate_ir_call(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRVCALL:
					translate_ir_virtual_call(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRICALL:
					translate_ir_icall(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRSUB:
					translate_ir_sub(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRADD:
					translate_ir_add(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRADDOVF:
					translate_ir_add_ovf(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRMULOVF:
					translate_ir_mul_ovf(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRMUL:
					translate_ir_mul(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRDIV:
				case IRDIV_NOEXCEPTION:
					translate_ir_div(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRREM:
				case IRREM_NOEXCEPTION:
					translate_ir_rem(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRNEG:
					translate_ir_neg(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRMOVE:
					translate_ir_move(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRRET:
					translate_ir_ret(self, llvmRoots, method, currentIRInstruction, llvmFunction, llvmBB);
					break;
				case IRNOP:
				case IRLABEL:
                		case IREXITNODE:
					break;
				default:
					fprintf(stderr, "ILDJIT_LLVM: Instruction kind \"%s\" is not handled yet\n", IRMETHOD_getInstructionTypeName(currentIRInstruction->type));
					abort();
			}
		}

		/* Unless current block ends with a
		   control instruction, append a branch	*/
		if (llvmBB->getTerminator() == NULL) {
			ir_instruction_t        *finalIRInstruction;
			ir_instruction_t        *destIRInstruction;
			IRBasicBlock		*destIRBB;
			BasicBlock		*destLlvmBB;

			/* Successor of the last IR instruction
			   is branch destination instruction	*/
			finalIRInstruction	= IRMETHOD_getInstructionAtPosition(method, bb->endInst);
			destIRInstruction	= IRMETHOD_getSuccessorInstruction(method, finalIRInstruction, NULL);
			assert(destIRInstruction != NULL);

			/* From destination instruction comes
			   destination block			*/
			destIRBB		= IRMETHOD_getBasicBlockContainingInstruction(method, destIRInstruction);
			destLlvmBB		= llvmFunction->basicBlocks[destIRBB->pos];

			/* If successor is not exit, append 
			   unconditional branch			*/
			if (destIRInstruction->type != IREXITNODE){
				BranchInst::Create(destLlvmBB, llvmBB);
			}
		}
	}

	/* Verify the code				*/
	if ((self->behavior).dumpAssembly.dumpAssembly) {
		if (IROPTIMIZER_isCodeToolInstalled(&(self->optimizer), METHOD_PRINTER)){
			IROPTIMIZER_callMethodOptimization(&(self->optimizer), method, METHOD_PRINTER);
		}
		llvmFunction->function->dump();
	}
	#ifdef DEBUG
	verifyFunction(*llvmFunction->function);
	#endif

	/* Optimize the generated bitcode		*/
	llvmRoots->FPM->doInitialization();
	llvmRoots->FPM->run(*llvmFunction->function);
	llvmRoots->FPM->doFinalization();

	/* Dump the code				*/
	if ((self->behavior).dumpAssembly.dumpAssembly) {
		llvmFunction->function->dump();
	}
	assert(!llvmFunction->function->empty());

	/* Verify the code				*/
	#ifdef DEBUG
	verifyFunction(*llvmFunction->function);
	#endif

	/* Free the memory.
	 */
	internal_free_memory_used_for_previous_translations(llvmFunction);
	freeFunction(labels);

	/* Translation succeeded		       	*/
	return ;
}

// Create local storage for the parameters, variables, and temporaries of a method.  Fill an array,
// indexed by IR identifier, with the resulting LLVM "values" (i.e., temporaries).
static inline void setup_llvm_variables (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, t_llvm_function_internal *llvmFunction, BasicBlock *entryBB){
	JITUINT32		startTemp;
	JITUINT32		endTemp;
	XanListItem     	*item;
	ir_item_t		**vars;
	Function::arg_iterator 	args;

	/* Fetch the IR variables.
	 */
	IRMETHOD_updateNumberOfVariablesNeededByMethod(method);
	vars	= IRMETHOD_getIRVariables(method);

	/* Fetch the parameters			*/
	args 	= llvmFunction->function->arg_begin();
	for (JITUINT32 count=0; count < IRMETHOD_getMethodParametersNumber(method); count++){
  		Value		*currentArg;
		Type	  	*type;
		JITUINT32	irType;
		JITUINT32	irTypeToUse;
		TypeDescriptor	*ilType;
		ParamDescriptor	*param;
		ir_item_t	*item;

		/* Fetch the parameter type		*/
		currentArg			= args++;
		irType				= IRMETHOD_getParameterType(method, count);
		ilType				= IRMETHOD_getParameterILType(method, count);
		param				= IRMETHOD_getParameterILDescriptor(method, count);
		item				= vars[count];
		assert((irType != IRVALUETYPE) || (item->type_infos != NULL));

		/* Due to IR optimizations, we could have uses of the parameter as an equivalent type (pointers, IRNUINT).			*
		 * Detect if this is the case													*/
		irTypeToUse			= irType;
		if (	(item != NULL)				&&
			(item->internal_type != irType)		){
			irTypeToUse			= item->internal_type;
		}
		type				= get_LLVM_type(self, llvmRoots, irTypeToUse, ilType);

		/* Set the parameter name		*/
		if (param != NULL){
  			currentArg->setName((const char *)param->getName(param));
		}

		/* Allocate the parameter		*/
		llvmFunction->variables[count] 	= internal_allocateANewVariable(type, 0, entryBB);

		/* Perform the necessary conversions	*/
		currentArg		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irTypeToUse, irType, currentArg, entryBB);
		new StoreInst(currentArg, llvmFunction->variables[count], entryBB);
	}

	/* Fetch the locally defined variables	*/
	for (item = xanList_first(method->locals); item != NULL; item = item->next) {
		Type	   	*type;
		ir_local_t 	*local;
		ir_item_t	*irItem;
		JITUINT32  	number;
		JITUINT32	irTypeToUse;

		/* Fetch the local variable		*/
		local				= (ir_local_t *) item->data;
		assert(local != NULL);

		/* Fetch the local number		*/
		number				= local->var_number;
		assert(number >= IRMETHOD_getMethodParametersNumber(method));

		/* Fetch the IR type of the local variabled declared	*/
		irTypeToUse			= local->internal_type;

		/* Due to IR optimizations, we could have uses of the parameter as an equivalent type (pointers, IRNUINT).			*
		 * Detect if this is the case													*/
		irItem				= vars[number];
		if (	(irItem != NULL)					&&
			(irItem->internal_type != irTypeToUse)			){
			irTypeToUse			= irItem->internal_type;
		}
		assert((irTypeToUse != IRVALUETYPE) || (irItem->type_infos != NULL));

		/* Fetch the LLVM type					*/
		type				= get_LLVM_type(self, llvmRoots, irTypeToUse, local->type_info);

		/* Allocate the variable 				*/
		llvmFunction->variables[number]	= internal_allocateANewVariable(type, 0, entryBB);
	}

	/* Fetch the temporaries.		*
	 * Fetch the start and the end ID of	*
	 * temporaries				*/
	endTemp		= IRMETHOD_getNumberOfVariablesNeededByMethod(method);
	startTemp	= IRMETHOD_getMethodLocalsNumber(method) + IRMETHOD_getMethodParametersNumber(method);

	/* Fetch the temporaries		*/
	for (JITUINT32 count=startTemp; count < endTemp; count++){
		ir_item_t	*item;
		Type	   	*type;
		item				= vars[count];
		if (item != NULL){
			type				= get_LLVM_type(self, llvmRoots, item->internal_type, item->type_infos);
			llvmFunction->variables[count]	= internal_allocateANewVariable(type, 0, entryBB);
		}
	}

	/* Create the finally block return address variables	*/
	for (JITUINT32 count=0; count < IRMETHOD_getNumberOfMethodBasicBlocks(method); count++){
		Type		*ptrType;
		ptrType		= get_LLVM_type(self, llvmRoots, IRINT32, NULL);
		llvmFunction->finallyBlockReturnAddresses[count]	= internal_allocateANewVariable(ptrType, 0, entryBB);
	}

	/* Create the variable to store thrown exceptions	*/
	if (IRMETHOD_getCatcherInstruction(method) != NULL){
		Type		*ptrType;
		ptrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
		llvmFunction->exceptionThrown	= internal_allocateANewVariable(ptrType, 0, entryBB);
	}

	/* Add a branch to the first IR basic block.		*/
	BranchInst::Create(llvmFunction->basicBlocks[0], entryBB);	

	/* Free the memory.
	 */
	freeFunction(vars);

	return ;
}

static inline void translate_ir_move (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irSrc;
	ir_item_t	*irDst;

	/* Fetch the IR values			*/
	irSrc		= IRMETHOD_getInstructionParameter1(inst);
	irDst		= IRMETHOD_getInstructionResult(inst);

	/* Generate code			*/
	translate_move(self, llvmRoots, method, irDst, irSrc, llvmFunction, currentBB);

	return ;
}

static inline void translate_move (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_item_t *irDst, ir_item_t *irSrc, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	Value		*llvmSrc;
	Value		*llvmDst;

	/* Assertions				*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch the LLVM values		*/
	llvmDst		= get_LLVM_value_for_defining(self, method, irDst, llvmFunction, currentBB);
	llvmSrc		= get_LLVM_value_for_using(self, method, irSrc, llvmFunction, currentBB);

	/* Perform the necessary operation	*/
	llvmSrc		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irDst->internal_type, irSrc->internal_type, llvmSrc, currentBB);

	/* Create a LLVM instruction		*/
	new StoreInst(llvmSrc, llvmDst, currentBB);

	return ;
}

static inline void translate_ir_general_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, JITUINT32 returnIRType, TypeDescriptor *returnIlType, IR_ITEM_VALUE functionPointer, JITBOOLEAN isLLVMFunction, char *functionName, ir_method_t *irCalledMethod, JITBOOLEAN directCall){
	Value				*llvmDefinedValue;
	Type				*intTypeToUseForPointers;
	std::vector<Type*>		functionParamTypes;
	ir_item_t			*irReturnItem;
	Value				*actualFunctionPtrValue;
	std::vector<Value*> 		args;
	FunctionType			*functionType;
	PointerType			*functionPtrType;
	CallInst			*llvmCallInst;
	Type				*functionResultType;
	BasicBlock			*bbToStoreResult;
	ir_instruction_t		*catchInst;
	ir_signature_t			*calledMethodSignature;
	bool				isVararg;

	/* Check whether we are going to dump the binary and the callee is empty. 
	 * In this case we do not need to generate the call because the callee will always be empty as there is no IR for it.
	 */
	if (	((self->behavior).staticCompilation)			&&
		(irCalledMethod != NULL)				&&
		(directCall)						&&
		(!IRMETHOD_hasMethodInstructions(irCalledMethod))	){
		return ;
	}
		
	/* Check if the call is to a vararg method.
	 */
	isVararg		= (IRMETHOD_isACallInstructionToAVarArgMethod(inst) == JITTRUE) ? true : false;

	/* Set the basic block to use to store the return value	*/
	bbToStoreResult		= currentBB;

	/* Fetch the signature of the method we are going to call	*/
	calledMethodSignature	= NULL;
	if (irCalledMethod != NULL){
		calledMethodSignature	= IRMETHOD_getSignature(irCalledMethod);
	}

	/* Fetch the catcher					*/
	catchInst		= IRMETHOD_getCatcherInstruction(method);

	/* Create the arguments					*/
	if (inst->callParameters != NULL){
		XanListItem	*item;
		JITUINT32	count;

		/* Create the arguments			*/
		count	= 0;
		item 	= xanList_first(inst->callParameters);
		while (item != NULL) {
			ir_item_t	*arg;
			Value		*llvmArg;
			Type		*llvmArgType;

			/* Fetch the IR argument		*/
			arg 	= (ir_item_t *) item->data;
			assert(arg != NULL);

			/* Create the LLVM argument		*/
			llvmArg	= get_LLVM_value_for_using(self, method, arg, llvmFunction, currentBB);

			/* Convert the LLVM argument		*/
			if (calledMethodSignature != NULL){
				assert(count < calledMethodSignature->parametersNumber);
				llvmArgType	= get_LLVM_type(self, llvmRoots, calledMethodSignature->parameterTypes[count], calledMethodSignature->ilParamsTypes[count]);
				llvmArg		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, calledMethodSignature->parameterTypes[count], arg->internal_type, llvmArg, currentBB);

			} else {
				llvmArgType	= get_LLVM_type(self, llvmRoots, arg->internal_type, arg->type_infos);
			}

			/* Add the LLVM argument		*/
			args.push_back(llvmArg);
			functionParamTypes.push_back(llvmArgType);

			/* Fetch the next argument		*/
			item = item->next;
			count++;
		}
	}

	/* Create the function pointer types	*/
	functionResultType		= get_LLVM_type(self, llvmRoots, returnIRType, returnIlType);
	functionType			= FunctionType::get(functionResultType, functionParamTypes, isVararg);
	functionPtrType			= PointerType::get(functionType, 0);

	/* Create the function pointer		*/
	if (isLLVMFunction){
		Type		*actualFunctionPtrType;
		actualFunctionPtrValue		= (Value *)functionPointer;
		actualFunctionPtrType		= actualFunctionPtrValue->getType();
		if (!actualFunctionPtrType->isPointerTy()){
			actualFunctionPtrValue		= new IntToPtrInst(actualFunctionPtrValue, functionPtrType, "", currentBB);
		} else {
			actualFunctionPtrValue		= new BitCastInst(actualFunctionPtrValue, functionPtrType, "", currentBB);
		}

	} else {
		Constant			*actualFunctionPtrConstant;

		/* The call is to a binary method.
		 * To perform this call, we need to know whether we are going to dump the machine code or not.
		 */
		if (!((self->behavior).staticCompilation)){

			/* We are not going to dump the machine code.
			 * The code that is going to be generated will be executed in memory.
			 * Hence, we can use the actual address of the entry point of the callee.
			 * Fetch the LLVM pointer type.
			 */
			intTypeToUseForPointers		= get_LLVM_integer_type_to_use_as_pointer(self, llvmRoots);

			/* Create the function pointer.
			 */
			actualFunctionPtrConstant	= ConstantInt::get(intTypeToUseForPointers, functionPointer);
			actualFunctionPtrValue		= ConstantExpr::getIntToPtr(actualFunctionPtrConstant, functionPtrType);

			/* Append the function name.
			 */
			if (functionName){
				actualFunctionPtrValue		= new BitCastInst(actualFunctionPtrValue, functionPtrType, functionName, currentBB);
			}

		} else {
			Function 	*nativeFunction;
			Type		*actualFunctionPtrType;
			XanList		*irTypes;
			XanListItem	*item;

			/* We are going to dump the machine code.
			 * Hence, we must use a symbol instead of the actual address of the entry point of the callee.
			 * Fetch the LLVM function that represents the native method.
			 */
			if (	(functionName == NULL) 		|| 
				(strcmp(functionName, "") == 0)	){
				fprintf(stderr, "ILDJIT: Error while generating the machine code: calls to native methods must have names\n");
				fprintf(stderr, "Caller method: %s\n", IRMETHOD_getSignatureInString(method));
				fprintf(stderr, "IR instruction: %u\n", inst->ID);
				IROPTIMIZER_callMethodOptimization(NULL, method, METHOD_PRINTER);
				abort();
			}
			irTypes		= xanList_new(allocFunction, freeFunction, NULL);
			if (inst->callParameters != NULL){
				item		= xanList_first(inst->callParameters);
				while (item != NULL){
					ir_item_t	*irPar;
					irPar	= (ir_item_t *)item->data;
					xanList_append(irTypes, (void *)(JITNUINT)irPar->internal_type);
					item	= item->next;
				}
			}
			nativeFunction	= get_LLVM_function_of_binary_method(self, (JITINT8 *)functionName, returnIRType, irTypes);
			assert(nativeFunction != NULL);

			/* Adjust the function type.
			 */
			actualFunctionPtrValue		= (Value *)nativeFunction;
			actualFunctionPtrType		= actualFunctionPtrValue->getType();
			if (!actualFunctionPtrType->isPointerTy()){
				actualFunctionPtrValue		= new IntToPtrInst(actualFunctionPtrValue, functionPtrType, "", currentBB);
			} else {
				actualFunctionPtrValue		= new BitCastInst(actualFunctionPtrValue, functionPtrType, "", currentBB);
			}

			/* Free the memory.
			 */
			xanList_destroyList(irTypes);
		}
	}

	/* Generate the store of the instruction range that the current IR call belongs to	*/
	internal_generate_instruction_range_store(self, llvmRoots, method, inst);

	/* Fetch the IR return value								*/
	irReturnItem			= IRMETHOD_getInstructionResult(inst);

	/* Generate the call instruction							*/
	if ((catchInst == NULL) || (inst->ID > catchInst->ID)){

		/* Create the call				*/
		llvmDefinedValue		= CallInst::Create(actualFunctionPtrValue, args, "", currentBB);

		/* Set attributes of the call instruction	*/
		llvmCallInst			= (CallInst *)llvmDefinedValue;
		llvmCallInst->setCallingConv(CallingConv::C);
		llvmCallInst->setTailCall(false);

	} else {
		BasicBlock		*landingPadBB;
		BasicBlock		*normalSuccessorBB;
		ir_instruction_t	*nextInst;
		IRBasicBlock		*nextIrBB;

		/* Fetch the successor in case an exception will be thrown by the callee		*/
		landingPadBB		= llvmFunction->basicBlocks[IRMETHOD_getNumberOfMethodBasicBlocks(method)];

		/* Allocate a basic block to use as a normal successor of the invoke instruction	*/
		nextInst		= IRMETHOD_getNextInstruction(method, inst);
		nextIrBB		= IRMETHOD_getBasicBlockContainingInstruction(method, nextInst);
		normalSuccessorBB	= llvmFunction->basicBlocks[nextIrBB->pos];
		if (	(irReturnItem->type != NOPARAM)	&&
			(irReturnItem->type != IRVOID)	){
			BasicBlock		*extraBB;
			char 			bbString[DIM_BUF];
			snprintf(bbString, DIM_BUF, "BB_extra_%u", inst->ID);
			extraBB			= BasicBlock::Create(llvmRoots->llvmModule->getContext(), bbString, llvmFunction->function, 0);
			bbToStoreResult		= extraBB;
			normalSuccessorBB	= extraBB;
		}

		/* Create the invoke									*/
		llvmDefinedValue	= InvokeInst::Create(actualFunctionPtrValue, normalSuccessorBB, landingPadBB, args, "", currentBB);
	}

	/* Store the value defined by the call		*/
	if (	(irReturnItem->type != NOPARAM)	&&
		(irReturnItem->type != IRVOID)	){
		Value	*llvmResultLocation;

		/* Fetch the LLVM result location		*/
		llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irReturnItem, llvmFunction);

		/* Convert to the correct type			*/
		llvmDefinedValue	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irReturnItem->internal_type, returnIRType, llvmDefinedValue, bbToStoreResult);

		/* Perform the store				*/
		new StoreInst(llvmDefinedValue, llvmResultLocation, bbToStoreResult);

		if (bbToStoreResult != currentBB){
			ir_instruction_t	*nextInst;
			IRBasicBlock		*irBB;
			BasicBlock		*normalSuccessorBB;

			/* Fetch the successor in case no exception will be thrown				*/
			nextInst		= IRMETHOD_getNextInstruction(method, inst);
			irBB			= IRMETHOD_getBasicBlockContainingInstruction(method, nextInst);
			normalSuccessorBB	= llvmFunction->basicBlocks[irBB->pos];

			/* Jump to the successor								*/
			BranchInst::Create(normalSuccessorBB, bbToStoreResult);
		}
	}

	return ;
}

static inline void translate_ir_library_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t			*irValue1;
	IR_ITEM_VALUE 			methodID;
	t_jit_function			*calledMethodWrapper;
	t_llvm_function_internal	*llvmCalledMethod;
	ir_method_t			*irCalledMethod;
	char				*internalFunctionName;

	/* Fetch the method to call			*/
	irValue1			= IRMETHOD_getInstructionParameter1(inst);
	if (irValue1->type == IRSYMBOL) {
		methodID 	= IRSYMBOL_resolveSymbol((ir_symbol_t *) (JITNUINT) (irValue1->value.v)).v;
	} else {
		methodID	= (irValue1->value).v;
	}
	calledMethodWrapper 	= self->getJITMethod(methodID);
	assert(calledMethodWrapper != NULL);
	llvmCalledMethod 	= (t_llvm_function_internal *) calledMethodWrapper->data;
	assert(llvmCalledMethod != NULL);
	assert(llvmCalledMethod->isNative);
	assert(llvmCalledMethod->entryPoint != NULL);
	irCalledMethod		= self->getIRMethod(methodID);
	assert(irCalledMethod != NULL);

	/* Fetch the function name			*/
	internalFunctionName	= (char *)get_C_function_name(irCalledMethod);
	assert(internalFunctionName != NULL);

	/* Generate the call				*/
	translate_ir_general_call(self, llvmRoots, method, inst, llvmFunction, currentBB, (inst->result).internal_type, (inst->result).type_infos, (IR_ITEM_VALUE)(JITNUINT)llvmCalledMethod->entryPoint, JITFALSE, internalFunctionName, NULL, JITTRUE);

	/* Free the memory.
	 */
	freeFunction(internalFunctionName);

	return ;
}

static inline void translate_ir_virtual_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t			*irValue1;
	ir_item_t			*irValue3;
	IR_ITEM_VALUE 			methodID;
	ir_method_t			*irCalledMethod;
	Value				*llvmPointerToFunction;

	/* Fetch the method to call			*/
	irValue1			= IRMETHOD_getInstructionParameter1(inst);
	irValue3			= IRMETHOD_getInstructionParameter3(inst);
	if (irValue1->type == IRSYMBOL) {
		methodID 	= IRSYMBOL_resolveSymbol((ir_symbol_t *) (JITNUINT) (irValue1->value.v)).v;
	} else {
		methodID	= (irValue1->value).v;
	}
	irCalledMethod		= self->getIRMethod(methodID);
	assert(irCalledMethod != NULL);	
	llvmPointerToFunction	= get_LLVM_value_for_using(self, method, irValue3, llvmFunction, currentBB);
	
	/* Generate the call				*/
	translate_ir_general_call(self, llvmRoots, method, inst, llvmFunction, currentBB, IRMETHOD_getResultType(irCalledMethod), IRMETHOD_getResultILType(irCalledMethod), (IR_ITEM_VALUE)(JITNUINT)llvmPointerToFunction, JITTRUE, (char *)IRMETHOD_getSignatureInString(irCalledMethod), irCalledMethod, JITFALSE);

	return ;
}

static inline void translate_ir_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t			*irValue1;
	IR_ITEM_VALUE 			methodID;
	t_jit_function			*calledMethodWrapper;
	t_llvm_function_internal	*llvmCalledMethod;
	ir_method_t			*irCalledMethod;

	/* Fetch the method to call			*/
	irValue1			= IRMETHOD_getInstructionParameter1(inst);
	if (IRDATA_isASymbol(irValue1)){
		methodID 	= IRSYMBOL_resolveSymbol((ir_symbol_t *) (JITNUINT) (irValue1->value.v)).v;
	} else {
		methodID	= (irValue1->value).v;
	}
	assert(methodID != NULL);
	calledMethodWrapper 	= self->getJITMethod(methodID);
	assert(calledMethodWrapper != NULL);

	/* Fetch the IR method				*/
	irCalledMethod		= self->getIRMethod(methodID);
	assert(irCalledMethod != NULL);

	/* Check that we have a backend method		*/
	allocateBackendFunction(self, calledMethodWrapper, irCalledMethod);
	llvmCalledMethod 	= (t_llvm_function_internal *) calledMethodWrapper->data;
	assert(llvmCalledMethod != NULL);

	/* Generate the call				*/
	translate_ir_general_call(self, llvmRoots, method, inst, llvmFunction, currentBB, IRMETHOD_getResultType(irCalledMethod), IRMETHOD_getResultILType(irCalledMethod), (IR_ITEM_VALUE)(JITNUINT)llvmCalledMethod->function, JITTRUE, (char *)IRMETHOD_getSignatureInString(irCalledMethod), irCalledMethod, JITTRUE);

	return ;
}

static inline void translate_ir_string_chr (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	std::vector<Value*> 	args;

	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);

	/* Convert the LLVM values								*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);
	llvmValue2		= convert_LLVM_value(self, llvmRoots, irValue2->internal_type, IRINT32, NULL, llvmValue2, currentBB);
	assert(llvmValue1 != NULL);
	assert(llvmValue2 != NULL);

	/* Fetch the LLVM result location							*/
	llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Create the arguments					*/
	args.push_back(llvmValue1);
	args.push_back(llvmValue2);

	/* Call the LLVM intrinsic								*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->strchrFunction, args, "", currentBB);

	/* Convert the return value								*/
	llvmMemoryAllocated	= convert_LLVM_value(self, llvmRoots, IRMPOINTER, irResult->internal_type, NULL, llvmMemoryAllocated, currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_string_cmp (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	std::vector<Value*> 	args;

	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);

	/* Convert the LLVM values								*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);
	llvmValue2		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue2->internal_type, llvmValue2, currentBB);
	assert(llvmValue1 != NULL);
	assert(llvmValue2 != NULL);

	/* Fetch the LLVM result location							*/
	llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Create the arguments					*/
	args.push_back(llvmValue1);
	args.push_back(llvmValue2);

	/* Call the LLVM intrinsic								*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->strcmpFunction, args, "", currentBB);

	/* Convert the return value								*/
	llvmMemoryAllocated	= convert_LLVM_value(self, llvmRoots, irResult->internal_type, IRINT32, NULL, llvmMemoryAllocated, currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_string_length (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;

	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);

	/* Convert the LLVM values								*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);
	assert(llvmValue1 != NULL);

	/* Fetch the LLVM result location							*/
	llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Call the LLVM intrinsic								*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->strlenFunction, llvmValue1, "", currentBB);

	/* Convert the return value								*/
	llvmMemoryAllocated	= convert_LLVM_value(self, llvmRoots, irResult->internal_type, IRINT32, NULL, llvmMemoryAllocated, currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_array_length (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_instruction_t	localInst;

	/* Create the equivalent IR load instruction		*/
	memcpy(&localInst, inst, sizeof(ir_instruction_t));
	localInst.type			= IRLOADREL;
	localInst.param_2.value.v	= self->gc->getArrayLengthOffset();
	localInst.param_2.type 		= IRINT32;
	localInst.param_2.internal_type = localInst.param_2.type;

	/* Produce the LLVM code				*/
	translate_ir_load_rel(self, llvmRoots, method, &localInst, llvmFunction, currentBB);

	return ;
}

static inline void translate_ir_store_rel (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irBa;
	ir_item_t	*irOffset;
	ir_item_t	*irValueToStore;
	Value		*llvmEa;
	Value		*llvmValueToStore;
	Type		*valueToStoreType;
	Type		*ptrValueToStoreType;

	/* Fetch the IR values.
     */
	irBa		    	= IRMETHOD_getInstructionParameter1(inst);
	irOffset	    	= IRMETHOD_getInstructionParameter2(inst);
	irValueToStore		= IRMETHOD_getInstructionParameter3(inst);

    /* Fetch the LLVM values.
     */
	llvmValueToStore	= get_LLVM_value_for_using(self, method, irValueToStore, llvmFunction, currentBB);

	/* Convert the effective address to the appropriate LLVM type.
	 */
	if (	(irValueToStore->type == IRGLOBAL)		&&
		(irValueToStore->internal_type == IRVALUETYPE)	){
		valueToStoreType	= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
		llvmValueToStore	= new BitCastInst(llvmValueToStore, valueToStoreType, "", currentBB);
	} else {
		valueToStoreType	= get_LLVM_type(self, llvmRoots, irValueToStore->internal_type, irValueToStore->type_infos);
		if (irValueToStore->internal_type != IRVALUETYPE){
			llvmValueToStore	= new BitCastInst(llvmValueToStore, valueToStoreType, "", currentBB);
		}
	}
	ptrValueToStoreType	= PointerType::get(valueToStoreType, 0);

    /* Fetch the effective address.
     */
    llvmEa              = internal_fetchEffectiveAddress(self, llvmRoots, llvmFunction, currentBB, method, irBa, irOffset, ptrValueToStoreType);

	/* Store the value to memory.
     */
	new StoreInst(llvmValueToStore, llvmEa, IRMETHOD_isInstructionVolatile(inst), currentBB);

	return;
}

static inline void translate_ir_load_rel (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irBa;
	ir_item_t	*irOffset;
	ir_item_t	*irResultLocation;
	Value		*llvmReferent;
	Value		*llvmEa;
	Value		*llvmResultLocation;
	Type		*referentType;
	Type		*ptrReferentType;
	Type		*llvmReferentType;

	/* Fetch the IR values.
     */
	irBa			    = IRMETHOD_getInstructionParameter1(inst);
	irOffset		    = IRMETHOD_getInstructionParameter2(inst);
	irResultLocation	= IRMETHOD_getInstructionResult(inst);

    /* Fetch the effective address.
     */
	referentType		= get_LLVM_type(self, llvmRoots, irResultLocation->internal_type, irResultLocation->type_infos);
	ptrReferentType		= PointerType::get(referentType, 0);
    llvmEa              = internal_fetchEffectiveAddress(self, llvmRoots, llvmFunction, currentBB, method, irBa, irOffset, ptrReferentType);

	/* Load the memory value.
     */
	llvmReferent		= new LoadInst(llvmEa, "", IRMETHOD_isInstructionVolatile(inst), currentBB);

	/* Perform the necessary cast.
     */
	llvmReferent		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irResultLocation->internal_type, irResultLocation->internal_type, llvmReferent, currentBB);

	/* Fetch the result location.
     */
	llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irResultLocation, llvmFunction);

	/* Perform the necessary conversions.
     */
	llvmReferentType	= llvmReferent->getType();
	llvmResultLocation	= new BitCastInst(llvmResultLocation, PointerType::get(llvmReferentType, 0), "", currentBB);

	/* Store the result to the destination.
     */
	new StoreInst(llvmReferent, llvmResultLocation, currentBB);

	return;
}

static inline void translate_ir_branch (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_instruction_t	*takenInst;
	IRBasicBlock		*takenIRBB;
	BasicBlock		*takenBranch;

	/* Fetch the target label.
	 */
	takenInst		= labels[(inst->param_1).value.v];
	assert(takenInst != NULL);

	/* Fetch the taken branch		*/
	takenIRBB		= IRMETHOD_getBasicBlockContainingInstruction(method, takenInst);
	takenBranch		= llvmFunction->basicBlocks[takenIRBB->pos];

	/* Generate the branch			*/
	BranchInst::Create(takenBranch, currentBB);

	return ;
}

static inline void translate_ir_conditional_branch (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, CmpInst::Predicate predicate){
	ir_item_t		*irCondition;
	ir_instruction_t	*nextInst;
	ir_instruction_t	*takenInst;
	IRBasicBlock		*untakenIRBB;
	IRBasicBlock		*takenIRBB;
	BasicBlock		*untakenBranch;
	BasicBlock		*takenBranch;
	Value			*llvmCondition;
	Value			*const_0;

	/* Fetch the condition to take the branch			*/
	irCondition		= IRMETHOD_getInstructionParameter1(inst);

	/* Fetch the LLVM condition					*/
	llvmCondition		= get_LLVM_value_for_using(self, method, irCondition, llvmFunction, currentBB);

	/* Create the constant zero to compare with			*/
	if (IRMETHOD_hasAnIntType(irCondition)){
		const_0			= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(IRDATA_getSizeOfType(irCondition)*8, StringRef("0"), 10));

	} else if (IRMETHOD_hasAFloatType(irCondition)){
		const_0			= ConstantFP::get(llvmRoots->llvmModule->getContext(), APFloat(0.0));

	} else {
		Type			*llvmConditionType;
		assert(IRMETHOD_hasAPointerType(irCondition));
		llvmConditionType	= get_LLVM_integer_type_to_use_as_pointer(self, llvmRoots);
		llvmCondition		= new PtrToIntInst(llvmCondition, llvmConditionType, "", currentBB);
		const_0			= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(IRDATA_getSizeOfType(irCondition)*8, StringRef("0"), 10));
	}

	/* Compare the value with the constant zero			*/
	if (IRMETHOD_hasAFloatType(irCondition)){
		llvmCondition		= new FCmpInst(*currentBB, predicate, llvmCondition, const_0, "");
	} else {
		llvmCondition		= new ICmpInst(*currentBB, predicate, llvmCondition, const_0, "");
	}

	/* Fetch the target label.
	 */
	assert((inst->param_2).type == IRLABEL);
	takenInst		= labels[(inst->param_2).value.v];
	assert(takenInst != NULL);

	/* Fetch the taken branch					*/
	takenIRBB		= IRMETHOD_getBasicBlockContainingInstruction(method, takenInst);
	takenBranch		= llvmFunction->basicBlocks[takenIRBB->pos];

	/* Fetch the untaken branch					*/
	nextInst		= IRMETHOD_getNextInstruction(method, inst);
	untakenIRBB		= IRMETHOD_getBasicBlockContainingInstruction(method, nextInst);
	untakenBranch		= llvmFunction->basicBlocks[untakenIRBB->pos];

	/* Generate the branch						*/
	BranchInst::Create(takenBranch, untakenBranch, llvmCondition, currentBB);

	return ;
}

static inline void translate_ir_conv_ovf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_conv(self, llvmRoots, method, inst, llvmFunction, currentBB);//FIXME

	return ;
}

static inline void translate_ir_bitcast (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irValueToConvert;
	ir_item_t	*irResultLocation;
	Value		*llvmValueToConvert;
	Value		*llvmResultLocation;
	Value		*bitcastedValue;
	Type		*resultType;

    /* Initialize the variables.
     */
    bitcastedValue          = NULL;

	/* Fetch the IR value to convert			*/
	irValueToConvert		= IRMETHOD_getInstructionParameter1(inst);

	/* Fetch the IR result 					*/
	irResultLocation		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM result type				*/
	resultType			= get_LLVM_type(self, llvmRoots, irResultLocation->internal_type, irResultLocation->type_infos);

	/* Fetch the LLVM result location			*/
	llvmResultLocation		= get_LLVM_value(self, llvmRoots, method, irResultLocation, llvmFunction);

	/* Check if the value to convert is a variable		*/
	if (!IRDATA_isAVariable(irValueToConvert)){

        if (    (irValueToConvert->internal_type == IRMETHODID)     &&
                (irResultLocation->internal_type == IRFPOINTER)     ){
		    IR_ITEM_VALUE			methodID;
			t_jit_function			*calledMethodWrapper;
			ir_method_t			*irCalledMethod;
			t_llvm_function_internal	*llvmCalledMethod;
			Constant			*const_ptr;
			PointerType			*PointerTy;

			/* Fetch the backend method.
			 */
			methodID		        = (irValueToConvert->value).v;
			calledMethodWrapper 	= self->getJITMethod(methodID);
			assert(calledMethodWrapper != NULL);

			/* Fetch the IR method.
			 */
			irCalledMethod		= self->getIRMethod(methodID);
			assert(irCalledMethod != NULL);

			if (IRMETHOD_hasMethodInstructions(irCalledMethod)){

				/* Fetch the backend-dependent method,
				 */
				allocateBackendFunction(self, calledMethodWrapper, irCalledMethod);
				llvmCalledMethod 	= (t_llvm_function_internal *) calledMethodWrapper->data;
				assert(llvmCalledMethod != NULL);

				/* Create the constant.
				 */
				if (llvmCalledMethod->function != NULL){
					PointerTy 		= PointerType::get(IntegerType::get(llvmRoots->llvmModule->getContext(), 8), 0);
					const_ptr 		= ConstantExpr::getCast(Instruction::BitCast, llvmCalledMethod->function, PointerTy);
					bitcastedValue	= (Value *) const_ptr;
				}

			} else {
				JITINT8		*fName;
				ir_signature_t 	*sign;
				XanList		*paramTypes;

				/* Fetch the signature.
				 */
				sign		= IRMETHOD_getSignature(irCalledMethod);
				assert(sign != NULL);

				/* Create the list of IR types of the parameters of the callee.
				 */
				paramTypes	= xanList_new(allocFunction, freeFunction, NULL);
				for (JITUINT32 count=0; count < sign->parametersNumber; count++){
					void		*parType;
					JITUINT32	irType;
					irType	= sign->parameterTypes[count];
					if (irType == IRVALUETYPE){
						irType	= IRNUINT;
					}
					parType	= (void *)(JITNUINT)irType;
					xanList_append(paramTypes, parType);
				}

				/* Fetch the name of the C function.
				 */
				fName		= get_C_function_name(irCalledMethod);
				assert(fName != NULL);

				/* Fetch the LLVM function.
				 */
				bitcastedValue		= (Value *) get_LLVM_function_of_binary_method(self, fName, sign->resultType, paramTypes);
				assert(bitcastedValue != NULL);

				/* Free the memory.
				 */
				freeFunction(fName);
				xanList_destroyList(paramTypes);
		    }
        } else {
            ir_item_t	tempValue;
            IR_ITEM_VALUE	valueToCast;

            /* Fetch the constant to convert			*/
            memcpy(&tempValue, irValueToConvert, sizeof(ir_item_t));

            /* Adjust the type					*/
            tempValue.type_infos	= NULL;
            tempValue.internal_type	= irResultLocation->internal_type;
            tempValue.type		= tempValue.internal_type;

            /* Adjust the value					*/
            valueToCast		= 0;
            switch (IRDATA_getSizeOfType(&tempValue)){
                case 8:
                    valueToCast		= 0xFFFFFFFFFFFFFFFFLL;
                    break;
                case 4:
                    valueToCast		= 0xFFFFFFFF;
                    break;
                case 3:
                    valueToCast		= 0xFFFFFF;
                    break;
                case 2:
                    valueToCast		= 0xFFFF;
                    break;
                case 1:
                    valueToCast		= 0xFF;
                    break;
            }
            (tempValue.value.v)	&= valueToCast;

            /* Fetch the LLVM constant 				*/
            bitcastedValue	= get_LLVM_value(self, llvmRoots, method, &tempValue, llvmFunction);
        }

	} else {

		/* Fetch the pointer to the LLVM value to bitcast 	*/
		llvmValueToConvert		= get_LLVM_value(self, llvmRoots, method, irValueToConvert, llvmFunction);

		/* Bitcast the pointer					*/
		bitcastedValue			= new BitCastInst(llvmValueToConvert, PointerType::get(resultType, 0), "", currentBB);

		/* Load the value bitcasted				*/
		bitcastedValue			= new LoadInst(bitcastedValue, "", currentBB);
	}

    if (bitcastedValue == NULL){
		bitcastedValue	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, StringRef("0"), 10));
    }

	/* Store the result			*/
	new StoreInst(bitcastedValue, llvmResultLocation, currentBB);

	return ;
}

static inline void translate_ir_conv (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irValueToConvert;
	ir_item_t	*irResultLocation;
	ir_item_t	*irResultType;
	Value		*llvmValueToConvert;
	Value		*llvmResultLocation;
	Value		*convertedValue;

	/* Fetch the IR value to convert	*/
	irValueToConvert		= IRMETHOD_getInstructionParameter1(inst);

	/* Fetch the IR result type		*/
	irResultType			= IRMETHOD_getInstructionParameter2(inst);

	/* Fetch the LLVM value to convert	*/
	llvmValueToConvert		= get_LLVM_value(self, llvmRoots, method, irValueToConvert, llvmFunction);
	if (	(irValueToConvert->type == IROFFSET)			&&
		(irValueToConvert->internal_type != IRVALUETYPE)	){
		llvmValueToConvert		= new LoadInst(llvmValueToConvert, "", currentBB);
	}

	/* Convert the value			*/
	convertedValue			= convert_LLVM_value(self, llvmRoots, irValueToConvert->internal_type, (irResultType->value).v, irResultType->type_infos, llvmValueToConvert, currentBB);

	/* Fetch the IR result location		*/
	irResultLocation		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM result location	*/
	llvmResultLocation		= get_LLVM_value(self, llvmRoots, method, irResultLocation, llvmFunction);
		
	/* Perform the necessary conversions between pointers and native integers	*/
	convertedValue			= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irResultLocation->internal_type, (irResultType->value).v, convertedValue, currentBB);

	/* Store the result			*/
	new StoreInst(convertedValue, llvmResultLocation, currentBB);
}

static inline void translate_ir_rem (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::FRem, Instruction::SRem, Instruction::URem);

	return ;
}

static inline void translate_ir_get_address (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	ir_item_t	*irValue1;
	ir_item_t	*irReturnValue;
	Value		*llvmValue1;
	Value		*llvmResultLocation;
	Type		*llvmValue1Type;
	Type		*pointerToLLVM1Type;

	/* Assertions.
	 */
	assert(method != NULL);
	assert(inst != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch the IR values.
	 */
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irReturnValue		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM destination.
	 */
	llvmResultLocation	= get_LLVM_value_for_defining(self, method, irReturnValue, llvmFunction, currentBB);

	/* Take the address of the variable.
	 */
	llvmValue1		= get_LLVM_value_for_defining(self, method, irValue1, llvmFunction, currentBB);
	assert(llvmValue1 != NULL);
	if (	(irValue1->type == IRGLOBAL)			&&
		(irValue1->internal_type == IRVALUETYPE)	){
		Type			*llvmPtrType;

		/* Cast the pointer to a generic (void *).
		 */
		llvmPtrType	= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
		llvmValue1	= new BitCastInst(llvmValue1, llvmPtrType, "", currentBB);
	}

	/* Perform the necessary casts.
	 */
	llvmValue1Type		= llvmValue1->getType();
	assert(llvmValue1Type != NULL);
	pointerToLLVM1Type	= PointerType::get(llvmValue1Type, 0);
	assert(pointerToLLVM1Type != NULL);
	llvmResultLocation	= new BitCastInst(llvmResultLocation, pointerToLLVM1Type, "", currentBB);

	/* Store the address to the variable.
	 */
	new StoreInst(llvmValue1, llvmResultLocation, currentBB);

	return ;
}

static inline void translate_ir_binary_comparison (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, CmpInst::Predicate floatOp, CmpInst::Predicate signedIntOp, CmpInst::Predicate unsignedIntOp){
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irReturnValue;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmResultLocation;
	Value	 		*llvmResultValue;
	Type			*llvmResultType;
	CmpInst::Predicate 	opToUse;

	/* Assertions				*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch the IR values			*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irReturnValue		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the predicate to use		*/
	if (IRMETHOD_hasAFloatType(irValue1)){
		opToUse		= floatOp;
	} else {
		if (IRMETHOD_hasASignedType(irValue1)){
			opToUse		= signedIntOp;
		} else {
			opToUse		= unsignedIntOp;
		}
	}

	/* Fetch the LLVM values		*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irReturnValue, llvmFunction);

	/* Convert the values			*/
	if (	(IRMETHOD_hasAPointerType(irValue1)) 		&&
		(IRMETHOD_hasAnIntType(irValue2))		){
		Type		*llvmPtrType;
		llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
		llvmValue2		= new IntToPtrInst(llvmValue2, llvmPtrType, "", currentBB);
	}
	if (	(IRMETHOD_hasAPointerType(irValue2)) 		&&
		(IRMETHOD_hasAnIntType(irValue1))		){
		Type		*llvmPtrType;
		llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
		llvmValue1		= new IntToPtrInst(llvmValue1, llvmPtrType, "", currentBB);
	}

	/* Create a LLVM instruction		*/
	if (IRMETHOD_hasAFloatType(irValue1)){
		llvmResultValue		= new FCmpInst(*currentBB, opToUse, llvmValue1, llvmValue2, "");
	} else {
		llvmResultValue		= new ICmpInst(*currentBB, opToUse, llvmValue1, llvmValue2, "");
	}

	/* Store the result			*/
	llvmResultType		= get_LLVM_type(self, llvmRoots, irReturnValue->internal_type, NULL);
	llvmResultValue		= new ZExtInst(llvmResultValue, llvmResultType, "", currentBB);
	new StoreInst(llvmResultValue, llvmResultLocation, currentBB);

	/* Return				*/
	return ;
}

static inline void translate_ir_binary (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, Instruction::BinaryOps floatOp, Instruction::BinaryOps signedIntOp, Instruction::BinaryOps unsignedIntOp){
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irReturnValue;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmResultLocation;
	Value	 		*llvmResultValue;
	Instruction::BinaryOps	opToUse;

	/* Assertions				*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch the IR values			*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irReturnValue		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM opcode to use		*/
	if (IRMETHOD_hasAFloatType(irValue1)){
		opToUse	= floatOp;
	} else if (IRMETHOD_hasASignedType(irValue1)){
		opToUse	= signedIntOp;
	} else {
		opToUse	= unsignedIntOp;
	}

	/* Fetch the LLVM values		*/
	llvmValue1		= get_LLVM_value(self, llvmRoots, method, irValue1, llvmFunction);
	llvmValue2		= get_LLVM_value(self, llvmRoots, method, irValue2, llvmFunction);
	llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irReturnValue, llvmFunction);

	if (	((irValue1->type != IRGLOBAL) || (irValue1->internal_type != IRVALUETYPE))	&&
		((irValue2->type != IRGLOBAL) || (irValue2->internal_type != IRVALUETYPE))	){

		/* Load the values to use		*/
		if (	(IRDATA_isAVariable(irValue1))		||
			(irValue1->type == IRGLOBAL)		){
			llvmValue1		= new LoadInst(llvmValue1, "", currentBB);
		}
		if (	(IRDATA_isAVariable(irValue2))		||
			(irValue2->type == IRGLOBAL)		){
			llvmValue2		= new LoadInst(llvmValue2, "", currentBB);
		}

		/* Convert the values to use to the	*
		 * appropriate types			*/
		if (	(IRMETHOD_hasAPointerType(irValue1))	||
			(IRMETHOD_hasAPointerType(irValue2))	){
			Type		*intTypeToUseForPointers;

			/* Fetch the LLVM type to use		*/
			intTypeToUseForPointers	= get_LLVM_integer_type_to_use_as_pointer(self, llvmRoots);

			/* Convert the first parameter		*/
			if (IRMETHOD_hasAPointerType(irValue1)){
				llvmValue1		= new PtrToIntInst(llvmValue1, intTypeToUseForPointers, "", currentBB);
			}

			/* Convert the second parameter		*/
			if (IRMETHOD_hasAPointerType(irValue2)){
				llvmValue2		= new PtrToIntInst(llvmValue2, intTypeToUseForPointers, "", currentBB);
			} 
		}

		/* Create a LLVM instruction to perform	*
		 * the operation			*/
		llvmResultValue		= BinaryOperator::Create(opToUse, llvmValue1, llvmValue2,  "", currentBB);

		/* Convert the produced value		*/
		if (IRMETHOD_hasAPointerType(irReturnValue)){
			Type		*llvmPtrType;
			llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
			llvmResultValue		= new IntToPtrInst(llvmResultValue, llvmPtrType, "", currentBB);
		}

	} else {
		Value			*llvmGlobalValue;
		Value			*llvmOffset;
		std::vector<Constant*> 	const_ptr_indices;
		ConstantInt		*const_int32_0;
		Constant		*pointerToGlobal;

		/* Fetch the global and the other value.
		 */
		if (irValue1->type == IRGLOBAL){
			llvmGlobalValue		= llvmValue1;
			llvmOffset		= llvmValue2;
		} else {
			llvmGlobalValue		= llvmValue2;
			llvmOffset		= llvmValue1;
		}
		
		/* The elements are blobs.
		 * IR global blobs are translated into LLVM char arrays.
		 * LLVM char arrays need to be handled in a special way to get the pointers.
		 * Set the indices.
		 */
		const_int32_0	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, StringRef("0"), 10));
		const_ptr_indices.push_back(const_int32_0);
		const_ptr_indices.push_back(const_int32_0);

		/* Fetch the pointer of the global.
		 */
		pointerToGlobal	= ConstantExpr::getGetElementPtr((Constant *)llvmGlobalValue, const_ptr_indices);

		/* Create space to store the offset.
		 */
		llvmResultValue	= GetElementPtrInst::Create(pointerToGlobal, llvmOffset, "", currentBB);
		llvmResultValue	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irReturnValue->internal_type, IRMPOINTER, llvmResultValue, currentBB);
	}

	/* Store the result			*/
	new StoreInst(llvmResultValue, llvmResultLocation, currentBB);

	return ;
}

static inline void translate_ir_ret (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irValue;
	Value		*llvmValue;

	/* Assertions				*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(llvmFunction->function != NULL);

	/* Fetch the LLVM variable or constant	*
	 * to return				*/
	llvmValue	= NULL;
	irValue		= IRMETHOD_getInstructionParameter1(inst);
	if (	(irValue->type != NOPARAM)	&&
		(irValue->type != IRVOID)	){

		/* Load the value to return		*/
		llvmValue	= get_LLVM_value_for_using(self, method, irValue, llvmFunction, currentBB);

		/* Perform the necessary cast		*/
		llvmValue	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMETHOD_getResultType(method), irValue->internal_type, llvmValue, currentBB);
	}

	/* Create a LLVM instruction		*/
	if (llvmValue == NULL){
		ReturnInst::Create(llvmRoots->llvmModule->getContext(), currentBB);
	} else {
		ReturnInst::Create(llvmRoots->llvmModule->getContext(), llvmValue, currentBB);
	}

	return ;
}

static inline void translate_ir_neg (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	ir_instruction_t	tempInst;

	/* Generate the equivalent IRSUB instruction	*/
	memcpy(&tempInst, inst, sizeof(ir_instruction_t));
	tempInst.type			= IRSUB;
	tempInst.param_1.value.v	= 0;
	tempInst.param_1.type		= tempInst.param_1.internal_type;
	memcpy(&tempInst.param_2, &(inst->param_1), sizeof(ir_item_t));

	/* Generate the LLVM code			*/
	translate_ir_sub(self, llvmRoots, method, &tempInst, llvmFunction, currentBB);

	return ;
}

static inline void translate_ir_new_array (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	std::vector<Type*>		functionParamTypes;
	std::vector<Value*> 		args;
	Type				*int32LlvmType;
	Type				*llvmPtrType;
	Value				*actualFunctionPtrValue;
	Value				*llvmArg;
	Value				*llvmDefinedValue;
	ir_item_t			*irValue1;
	ir_item_t			*irValue2;
	ir_item_t			*irReturnItem;
	ir_item_t			irArrayType;
	TypeDescriptor  		*ilType;
	TypeDescriptor			*arrayType;
	ArrayDescriptor 		*arrayDescriptor;

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irReturnItem		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM types					*/
	llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);

	/* Create the function pointer				*/
	actualFunctionPtrValue	= llvmRoots->allocArrayFunction;

	/* Fetch the input language element type		*/
	memcpy(&irArrayType, irValue1, sizeof(ir_item_t));
	if (irValue1->type != IRGLOBAL){

		/* Check the type of the IR item.
		 */
		if (irValue1->type == IRSYMBOL) {
			ilType = (TypeDescriptor *) (JITNUINT) IRSYMBOL_resolveSymbol((ir_symbol_t *) (JITNUINT) ((irValue1->value).v)).v;
		} else {
			ilType = (TypeDescriptor *) (JITNUINT) (irValue1->value).v;
		}
		assert(ilType != NULL);

		/* Fetch the input language array descriptor.
		 */
		arrayType 		= ilType->makeArrayDescriptor(ilType, 1);
		assert(arrayType != NULL);
		arrayDescriptor 	= GET_ARRAY(arrayType);

		/* Store the IL type into the IR item.
		 */
		irArrayType.value.v		= (IR_ITEM_VALUE)(JITNUINT)(arrayDescriptor);
		irArrayType.type		= irArrayType.internal_type;
	}

	/* Create the arguments for the call.
	 * The first parameter is the pointer to the metadata array descriptor.
	 */
	llvmArg	= get_LLVM_value_for_using(self, method, &irArrayType, llvmFunction, currentBB);
	if (!IRMETHOD_hasAPointerType(irValue1)){
		assert(IRMETHOD_hasAnIntType(irValue1));
		llvmArg	= new IntToPtrInst(llvmArg, llvmPtrType, "", currentBB);
	}
	args.push_back(llvmArg);
	llvmArg	= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	int32LlvmType			= get_LLVM_type(self, llvmRoots, IRINT32, NULL);
	if (irValue2->internal_type != IRINT32){
		assert(IRMETHOD_hasAnIntType(irValue2));
		if (IRMETHOD_hasASignedType(irValue2)){
			switch (IRDATA_getSizeOfType(irValue2)){
				case 1:
				case 2:
					llvmArg		= new SExtInst(llvmArg, int32LlvmType, "", currentBB);
					break;
				case 4:
					assert(irValue2->internal_type == IRNINT);
					break;
				case 8:
					llvmArg		= new TruncInst(llvmArg, int32LlvmType, "", currentBB);
					break;
				default:
					abort();
			}

		} else {
			switch (IRDATA_getSizeOfType(irValue2)){
				case 1:
				case 2:
					llvmArg		= new ZExtInst(llvmArg, int32LlvmType, "", currentBB);
					break;
				case 4:
					assert(irValue2->internal_type == IRNUINT);
					break;
				case 8:
					llvmArg		= new TruncInst(llvmArg, int32LlvmType, "", currentBB);
					break;
				default:
					abort();
			}
		}
	}
	args.push_back(llvmArg);

	/* Call the object allocator			*/
	llvmDefinedValue	= CallInst::Create(actualFunctionPtrValue, args, "", currentBB);

	/* Store the reference to the destination	*/
	if (	(irReturnItem->type != NOPARAM)	&&
		(irReturnItem->type != IRVOID)	){
		Value	*llvmResultLocation;
		Type	*llvmDefinedValueType;

		/* Fetch the result location			*/
		llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irReturnItem, llvmFunction);

		/* Perform the necessary cast			*/
		llvmResultLocation	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irReturnItem->internal_type, llvmResultLocation, currentBB);
		llvmDefinedValueType	= llvmDefinedValue->getType();
		llvmResultLocation	= new BitCastInst(llvmResultLocation, PointerType::get(llvmDefinedValueType, 0), "", currentBB);

		/* Perform the store				*/
		new StoreInst(llvmDefinedValue, llvmResultLocation, currentBB);
	}

	return;
}

static inline void translate_ir_endfinally (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	IRBasicBlock		*irBB;
	ir_item_t		*finallyLabel;
	ir_instruction_t	*startFinallyInst;
	Value			*llvmBlockAddress;
	SwitchInst		*indirectBranchInst;
	XanList			*l;
	XanListItem		*item;

	/* Fetch the basic block of the beginning of the finally block				*/
	finallyLabel			= IRMETHOD_getInstructionParameter1(inst);
	startFinallyInst		= IRMETHOD_getFinallyInstruction(method, (finallyLabel->value).v);
	assert(startFinallyInst != NULL);
	irBB				= IRMETHOD_getBasicBlockContainingInstruction(method, startFinallyInst);

	/* Fetch the possible successors							*/
	l				= IRMETHOD_getCallersOfFinallyBlock(method, (finallyLabel->value).v);
	assert(l != NULL);
	assert(xanList_length(l) > 0);

	/* Generate the indirect branch								*/
	llvmBlockAddress		= new LoadInst(llvmFunction->finallyBlockReturnAddresses[irBB->pos], "", currentBB);
	indirectBranchInst		= SwitchInst::Create(llvmBlockAddress, currentBB, xanList_length(l), currentBB);

	/* Add the possible destinations							*/
	item				= xanList_first(l);
	while (item != NULL){
		ir_instruction_t	*finallyInst;
		ir_instruction_t	*nextFinallyInst;
		ir_item_t		returnID;
		Value			*llvmReturnID;

		/* Fetch the instruction								*/
		finallyInst		= (ir_instruction_t *)item->data;
		assert((IRMETHOD_getInstrucionType(finallyInst) == IRCALLFINALLY) || (finallyInst == IRMETHOD_getPrevInstruction(method, startFinallyInst)));
		
		/* Fetch the return instruction								*/
		nextFinallyInst	= IRMETHOD_getNextInstruction(method, finallyInst);
		irBB		= IRMETHOD_getBasicBlockContainingInstruction(method, nextFinallyInst);

		/* Create the LLVM value of the return ID						*/
		memset(&returnID, 0, sizeof(ir_item_t));
		returnID.value.v		= nextFinallyInst->ID;
		returnID.type			= IRINT32;
		returnID.internal_type		= returnID.type;
		llvmReturnID			= get_LLVM_value(self, llvmRoots, method, &returnID, llvmFunction);

		/* Add the branch									*/
		indirectBranchInst->addCase((ConstantInt *)llvmReturnID, llvmFunction->basicBlocks[irBB->pos]);

		/* Fetch the next element								*/
		item	= item->next;
	}

	/* Free the memory									*/
	xanList_destroyList(l);

	return ;
}

static inline void translate_ir_callfinally (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*finallyLabel;
	ir_instruction_t	*irDestination;
	ir_instruction_t	*nextInst;
	IRBasicBlock		*irBB;
	IRBasicBlock		*irSrcBB;
	BasicBlock		*bb;
	ir_item_t		returnID;
	Value			*llvmReturnID;

	/* Fetch the destination of the jump							*/
	finallyLabel			= IRMETHOD_getInstructionParameter1(inst);
	irDestination			= IRMETHOD_getFinallyInstruction(method, (finallyLabel->value).v);
	assert(irDestination != NULL);

	/* Fetch the LLVM destination								*/
	irBB				= IRMETHOD_getBasicBlockContainingInstruction(method, irDestination);
	bb				= llvmFunction->basicBlocks[irBB->pos];

	/* Fetch the address of the LLVM source of the jump					*/
	nextInst			= IRMETHOD_getNextInstruction(method, inst);
	irSrcBB				= IRMETHOD_getBasicBlockContainingInstruction(method, nextInst);
	if (llvmFunction->basicBlockAddresses[irSrcBB->pos] == NULL){
		llvmFunction->basicBlockAddresses[irSrcBB->pos]	= BlockAddress::get(llvmFunction->function, llvmFunction->basicBlocks[irSrcBB->pos]);
	}
	assert(llvmFunction->basicBlockAddresses[irSrcBB->pos] != NULL);

	/* Create the LLVM value of the return ID						*/
	memset(&returnID, 0, sizeof(ir_item_t));
	returnID.value.v		= nextInst->ID;
	returnID.type			= IRINT32;
	returnID.internal_type		= returnID.type;
	llvmReturnID			= get_LLVM_value(self, llvmRoots, method, &returnID, llvmFunction);

	/* Store the position where we currently are inside the finally-specific variable	*/
	assert(llvmFunction->finallyBlockReturnAddresses[irSrcBB->pos] != NULL);
	new StoreInst(llvmReturnID, llvmFunction->finallyBlockReturnAddresses[irBB->pos], currentBB);
	
	/* Jump to the destination								*/
	BranchInst::Create(bb, currentBB);
	
	return ;
}

static inline void translate_ir_throw (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_instruction_t	*catchInst;
	ir_item_t		*irValue1;
	IRBasicBlock		*destIRBB;
	Value			*llvmValue1;
	BasicBlock		*destLlvmBB;

	/* Fetch the catcher								*/
	catchInst	= IRMETHOD_getCatcherInstruction(method);

	/* Fetch the result location to store the pointer of the thrown exception 	*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);

	/* Check if we need to unwind the call stack					*/
	if (    (catchInst != NULL)				&&
                (catchInst->ID < inst->ID)      		){
		Value		*exceptionDescriptor;
		Value		*llvmExceptionTypeID;

		/* Prepare the exception data structure						*/
		exceptionDescriptor	= UndefValue::get(llvmRoots->llvmExceptionType);
		llvmExceptionTypeID	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, 42, true));
		InsertValueInst::Create(exceptionDescriptor, llvmValue1, 0, "", currentBB);
		InsertValueInst::Create(exceptionDescriptor, llvmExceptionTypeID, 1, "", currentBB);

		/* Unwind the call stack and return to the callee				*/
		ResumeInst::Create(exceptionDescriptor, currentBB);

		return ;
	}
	if (catchInst == NULL){
		std::vector<llvm::Type*>	argTypes;
		Value				*actualFunctionPtrValue;
		Value				*exception;

		/* Create the exception to throw	*/
		actualFunctionPtrValue		= llvmRoots->createExceptionFunction;
		exception			= CallInst::Create(actualFunctionPtrValue, llvmValue1, "", currentBB);

		/* Throw the exception.
		 */
		actualFunctionPtrValue		= llvmRoots->throwExceptionFunction;
		CallInst::Create(actualFunctionPtrValue, exception, "", currentBB);

		new UnreachableInst(llvmRoots->llvmModule->getContext(), currentBB);

		return ;
	}

	/* Store the exception thrown							*/
	assert(llvmFunction->exceptionThrown != NULL);
	new StoreInst(llvmValue1, llvmFunction->exceptionThrown, currentBB);

	/* Jump to the catcher								*/
	destIRBB		= IRMETHOD_getBasicBlockContainingInstruction(method, catchInst);
	destLlvmBB		= llvmFunction->basicBlocks[destIRBB->pos];
	BranchInst::Create(destLlvmBB, currentBB);

	return ;
}

static inline void translate_ir_sizeof (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_instruction_t	localInst;
	ir_item_t		*param1;

	/* Fetch the parameter			*/
	param1		= IRMETHOD_getInstructionParameter1(inst);

	/* Create the move instruction		*/
	memset(&localInst, 0, sizeof(ir_instruction_t));
	localInst.type	= IRMOVE;
	memcpy(&(localInst.result), &(inst->result), sizeof(ir_item_t));
	localInst.param_1.type		= localInst.result.internal_type;
	localInst.param_1.internal_type	= localInst.param_1.type;
	switch ((param1->value).v){
		case IROBJECT:
		case IRVALUETYPE:
			localInst.param_1.value.v	= IRDATA_getSizeOfObject(param1);
			break;
		default :
			localInst.param_1.value.v	= IRDATA_getSizeOfType(param1);
	}

	return translate_ir_move(self, llvmRoots, method, &localInst, llvmFunction, currentBB);
}

static inline void translate_ir_free_object (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	std::vector<Type*>		functionParamTypes;
	std::vector<Value*> 		args;
	Value				*actualFunctionPtrValue;
	Value				*llvmArg;
	ir_item_t			*irValue1;

	/* Fetch the IR value					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);

	/* Create the function pointer				*/
	actualFunctionPtrValue		= llvmRoots->freeObjectFunction;

	/* Create the function pointer				*/
	std::vector<llvm::Type*>	argTypes;
	argTypes.clear();
	argTypes.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
	Type				*retType;
	retType					= get_LLVM_type(self, llvmRoots, IRVOID, NULL);
	FunctionType			*functionType;
	functionType				= FunctionType::get(retType, argTypes, false);
    PointerType				*freeObjectFunctionType;
	freeObjectFunctionType	= PointerType::get(functionType, 0);
	Constant			*actualFunctionPtrConstant;
	actualFunctionPtrConstant	= ConstantInt::get(get_LLVM_type(self, llvmRoots, IRUINT64, NULL), (JITUINT64)(JITNUINT)self->gc->freeObject);
	actualFunctionPtrValue		= ConstantExpr::getIntToPtr(actualFunctionPtrConstant, freeObjectFunctionType);

	actualFunctionPtrValue		= llvmRoots->freeObjectFunction;

	/* Create the arguments					*/
	llvmArg	= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmArg	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmArg, currentBB);
	args.push_back(llvmArg);

	/* Call the object allocator				*/
	CallInst::Create(actualFunctionPtrValue, args, "", currentBB);

	return ;
}

static inline void translate_ir_new_object (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	std::vector<Type*>		functionParamTypes;
	std::vector<Value*> 		args;
	Value				*actualFunctionPtrValue;
	Value				*llvmArg;
	Value				*llvmDefinedValue;
	Type				*llvmPtrType;
	ir_item_t			*irValue1;
	ir_item_t			*irValue2;
	ir_item_t			*irReturnItem;

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irReturnItem		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM types					*/
	llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);

	/* Create the function pointer				*/
	actualFunctionPtrValue		= llvmRoots->allocObjectFunction;

	/* Create the arguments				*/
	llvmArg	= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	if (!IRMETHOD_hasAPointerType(irValue1)){
		assert(IRMETHOD_hasAnIntType(irValue1));
		llvmArg	= new IntToPtrInst(llvmArg, llvmPtrType, "", currentBB);
	}
	args.push_back(llvmArg);
	llvmArg	= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	args.push_back(llvmArg);

	/* Call the object allocator			*/
	llvmDefinedValue	= CallInst::Create(actualFunctionPtrValue, args, "", currentBB);

	/* Store the reference to the destination	*/
	if (	(irReturnItem->type != NOPARAM)	&&
		(irReturnItem->type != IRVOID)	){
		Value	*llvmResultLocation;
		Type	*llvmDefinedValueType;

		/* Fetch the result location			*/
		llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irReturnItem, llvmFunction);

		/* Perform the necessary cast			*/
		llvmResultLocation	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irReturnItem->internal_type, llvmResultLocation, currentBB);
		llvmDefinedValueType	= llvmDefinedValue->getType();
		llvmResultLocation	= new BitCastInst(llvmResultLocation, PointerType::get(llvmDefinedValueType, 0), "", currentBB);

		/* Perform the store				*/
		new StoreInst(llvmDefinedValue, llvmResultLocation, currentBB);
	}

	return ;
}

static inline void translate_ir_exit (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	Value			*llvmValue1;
	ir_item_t		*irValue1;

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	assert(IRMETHOD_hasAnIntType(irValue1));

	/* Fetch the LLVM value					*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	if (irValue1->internal_type != IRINT32){
		assert(IRMETHOD_hasAnIntType(irValue1));
		llvmValue1		= convert_LLVM_value(self, llvmRoots, irValue1->internal_type, IRINT32, NULL, llvmValue1, currentBB);
	}

	/* Call the LLVM intrinsic				*/
	CallInst::Create(llvmRoots->exitFunction, llvmValue1, "", currentBB);

	/* Declare the unreachability 				*/
	new UnreachableInst(llvmRoots->llvmModule->getContext(), currentBB);

	return ;
}

static inline void translate_ir_memcpy (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	#ifdef IR_LLVM_DO_NOT_USE_INTRINSIC
	translate_ir_memcpy_libc(self, llvmRoots, method, inst, llvmFunction, currentBB);
	#else
	translate_ir_memcpy_intrinsic(self, llvmRoots, method, inst, llvmFunction, currentBB);
	#endif

	return ;
}

static inline void translate_ir_memcpy_libc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irValue3;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmValue3;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	std::vector<Value*> 	args;
	
	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irValue3		= IRMETHOD_getInstructionParameter3(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	llvmValue3		= get_LLVM_value_for_using(self, method, irValue3, llvmFunction, currentBB);

	/* Convert the LLVM values								*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);
	llvmValue2		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue2->internal_type, llvmValue2, currentBB);
	llvmValue3		= convert_LLVM_value(self, llvmRoots, irValue3->internal_type, IRINT32, NULL, llvmValue3, currentBB);
	assert(llvmValue1 != NULL);
	assert(llvmValue2 != NULL);
	assert(llvmValue3 != NULL);

	/* Fetch the LLVM result location							*/
	llvmResult 		= NULL;
	if (irResult->type != NOPARAM){
		llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);
	}

	/* Create the arguments					*/
	args.push_back(llvmValue1);
	args.push_back(llvmValue2);
	args.push_back(llvmValue3);

	/* Call the function					*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->memcpyFunction, args, "", currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	if (llvmResult != NULL){
		new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);
	}

	return ;
}

static inline void translate_ir_memcmp (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irValue3;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmValue3;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	std::vector<Value*> 	args;
	
	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irValue3		= IRMETHOD_getInstructionParameter3(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	llvmValue3		= get_LLVM_value_for_using(self, method, irValue3, llvmFunction, currentBB);

	/* Convert the LLVM values								*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);
	llvmValue2		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue2->internal_type, llvmValue2, currentBB);
	llvmValue3		= convert_LLVM_value(self, llvmRoots, irValue3->internal_type, IRINT32, NULL, llvmValue3, currentBB);
	assert(llvmValue1 != NULL);
	assert(llvmValue2 != NULL);
	assert(llvmValue3 != NULL);

	/* Fetch the LLVM result location							*/
	llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Create the arguments					*/
	args.push_back(llvmValue1);
	args.push_back(llvmValue2);
	args.push_back(llvmValue3);

	/* Call the LLVM intrinsic								*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->memcmpFunction, args, "", currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_memcpy_intrinsic (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	Function	*llvmMemcpyFunction;
	Value		*llvmValue1;
	Value		*llvmValue2;
	Value		*llvmValue3;
	Type		*llvmPtrType;
	Type		*llvmInt32Type;
        Type            *llvmInt64Type;
	CallInst	*llvmCallInst;
	ir_item_t	*irValue1;
	ir_item_t	*irValue2;
	ir_item_t	*irValue3;
	string		functionName;
	JITBOOLEAN	is64;

	/* Check if we are in a 64-bit platform			*/
	is64			= sizeof(JITNUINT) == 8;

	/* Set the function name				*/
	if (is64){
		functionName		= "llvm.memcpy.p0i8.p0i8.i64"; 
	} else {
		functionName		= "llvm.memcpy.p0i8.p0i8.i32"; 
	}

	/* Fetch the LLVM types					*/
	llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
	llvmInt32Type		= get_LLVM_type(self, llvmRoots, IRINT32, NULL);
	llvmInt64Type		= get_LLVM_type(self, llvmRoots, IRINT64, NULL);

	/* Fetch the LLVM intrinsic				*/
	llvmMemcpyFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmMemcpyFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(llvmPtrType);
		FuncTy_4_args.push_back(llvmPtrType);
		if (is64){
			FuncTy_4_args.push_back(llvmInt64Type);
		} else {
			FuncTy_4_args.push_back(llvmInt32Type);
		}
		FuncTy_4_args.push_back(llvmInt32Type);
		FuncTy_4_args.push_back(IntegerType::get(llvmRoots->llvmModule->getContext(), 1));
		FuncTy_4 		= FunctionType::get(Type::getVoidTy(llvmRoots->llvmModule->getContext()), FuncTy_4_args, false);
		llvmMemcpyFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmMemcpyFunction->setCallingConv(CallingConv::C);
	}
	assert(llvmMemcpyFunction != NULL);

	/* Set the attributes of the LLVM intrinsic		*/
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes = Attributes.addAttribute(Context,          1U, Attribute::NoCapture);
		Attributes = Attributes.addAttribute(Context,          2U, Attribute::NoCapture);
		Attributes = Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);

		llvmMemcpyFunction->setAttributes(Attributes);
	}

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irValue3		= IRMETHOD_getInstructionParameter3(inst);
	assert(IRMETHOD_hasAnIntType(irValue3));

	/* Fetch the destination LLVM value			*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	if (!IRMETHOD_hasAPointerType(irValue1)){
		assert(IRMETHOD_hasAnIntType(irValue1));
		llvmValue1 		= new IntToPtrInst(llvmValue1, llvmPtrType, "", currentBB);
	}

	/* Fetch the source LLVM value				*/
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	if (!IRMETHOD_hasAPointerType(irValue2)){
		assert(IRMETHOD_hasAnIntType(irValue2));
		llvmValue2 		= new IntToPtrInst(llvmValue2, llvmPtrType, "", currentBB);
	}

	/* Fetch the byte count LLVM value			*/
	llvmValue3		= get_coerced_byte_count(self, llvmRoots, method, irValue3, llvmFunction, currentBB);

	/* Call the LLVM intrinsic				*/
	ConstantInt* const_int32_5 = ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, StringRef("1"), 10));
	ConstantInt* const_int1_8 = ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(1, StringRef("0"), 10));
	std::vector<Value*> llvmCallInst_params;
	llvmCallInst_params.push_back(llvmValue1);
	llvmCallInst_params.push_back(llvmValue2);
	llvmCallInst_params.push_back(llvmValue3);
	llvmCallInst_params.push_back(const_int32_5);
	llvmCallInst_params.push_back(const_int1_8);
	llvmCallInst 	= CallInst::Create(llvmMemcpyFunction, llvmCallInst_params, "", currentBB);
	llvmCallInst->setCallingConv(CallingConv::C);
	llvmCallInst->setTailCall(false);
	AttributeSet Attributes;
	llvmCallInst->setAttributes(Attributes);

	return ;
}

static inline void translate_ir_memset_libc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	Value			*llvmEa;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmValue3;
	Type			*llvmPtrType;
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irValue3;
	std::vector<Value*>	args;

	/* Fetch the LLVM types					*/
	llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irValue3		= IRMETHOD_getInstructionParameter3(inst);

	/* Fetch the destination LLVM value			*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	if (!IRMETHOD_hasAPointerType(irValue1)){
		assert(IRMETHOD_hasAnIntType(irValue1));
		llvmValue1 		= new IntToPtrInst(llvmValue1, llvmPtrType, "", currentBB);
	}

	/* Fetch the value to store				*/
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);

	/* Fetch the size					*/
	llvmValue3		= get_coerced_byte_count(self, llvmRoots, method, irValue3, llvmFunction, currentBB);

	/* Cast the pointer of the effective memory address	*/
	llvmEa 			= new BitCastInst(llvmValue1, llvmPtrType, "", currentBB);

	/* Call the function					*/
	args.push_back(llvmEa);
	args.push_back(llvmValue2);
	args.push_back(llvmValue3);
	CallInst::Create(llvmRoots->memsetFunction, args, "", currentBB);

	return ;
}

static inline void translate_ir_memset (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	#ifdef IR_LLVM_DO_NOT_USE_INTRINSIC
	translate_ir_memset_libc(self, llvmRoots, method, inst, llvmFunction, currentBB);
	#else
	translate_ir_memset_intrinsic(self, llvmRoots, method, inst, llvmFunction, currentBB);
	#endif

	return ;
}

static inline void translate_ir_memset_intrinsic (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	Function	*llvmMemsetFunction;
	Value		*llvmEa;
	Value		*llvmValue1;
	Value		*llvmValue2;
	Value		*llvmValue3;
	Type		*llvmPtrType;
	Type		*llvmInt8Type;
	Type		*llvmInt32Type;
        Type            *llvmInt64Type;
	CallInst	*llvmCallInst;
	ir_item_t	*irValue1;
	ir_item_t	*irValue2;
	ir_item_t	*irValue3;
	string		functionName;
	JITBOOLEAN	is64;

	/* Check if we are in a 64-bit platform			*/
	is64			= sizeof(JITNUINT) == 8;

	/* Set the function name				*/
	if (is64){
		functionName	= "llvm.memset.p0i8.i64"; 
	} else {
		functionName	= "llvm.memset.p0i8.i32"; 
	}

	/* Fetch the LLVM types					*/
	llvmPtrType		= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
	llvmInt8Type		= get_LLVM_type(self, llvmRoots, IRINT8, NULL);
	llvmInt32Type		= get_LLVM_type(self, llvmRoots, IRINT32, NULL);
	llvmInt64Type		= get_LLVM_type(self, llvmRoots, IRINT64, NULL);

	/* Fetch the LLVM intrinsic				*/
	llvmMemsetFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmMemsetFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(llvmPtrType);
		FuncTy_4_args.push_back(llvmInt8Type);
		if (is64){
			FuncTy_4_args.push_back(llvmInt64Type);
		} else {
			FuncTy_4_args.push_back(llvmInt32Type);
		}
		FuncTy_4_args.push_back(llvmInt32Type);
		FuncTy_4_args.push_back(IntegerType::get(llvmRoots->llvmModule->getContext(), 1));
		FuncTy_4 		= FunctionType::get(Type::getVoidTy(llvmRoots->llvmModule->getContext()), FuncTy_4_args, false);
		llvmMemsetFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmMemsetFunction->setCallingConv(CallingConv::C);
	}
	assert(llvmMemsetFunction != NULL);

	/* Set the attributes of the LLVM intrinsic		*/
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes = Attributes.addAttribute(Context,          1U, Attribute::NoCapture);
		Attributes = Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);

		llvmMemsetFunction->setAttributes(Attributes);
	}

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irValue3		= IRMETHOD_getInstructionParameter3(inst);

	/* Fetch the destination LLVM value			*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	if (!IRMETHOD_hasAPointerType(irValue1)){
		assert(IRMETHOD_hasAnIntType(irValue1));
		llvmValue1 		= new IntToPtrInst(llvmValue1, llvmPtrType, "", currentBB);
	}

	/* Fetch the value to store				*/
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	if (irValue2->internal_type != IRINT8){
		assert(IRMETHOD_hasAnIntType(irValue2));
		llvmValue2		= new TruncInst(llvmValue2, llvmInt8Type, "", currentBB);
	}

	/* Fetch the size					*/
	llvmValue3		= get_coerced_byte_count(self, llvmRoots, method, irValue3, llvmFunction, currentBB);

	/* Cast the pointer of the effective memory address	*/
	llvmEa 			= new BitCastInst(llvmValue1, llvmPtrType, "", currentBB);

	/* Call the LLVM intrinsic				*/
	ConstantInt* const_int32_5 = ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, StringRef("1"), 10));
	ConstantInt* const_int1_8 = ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(1, StringRef("0"), 10));
	std::vector<Value*> llvmCallInst_params;
	llvmCallInst_params.push_back(llvmEa);
	llvmCallInst_params.push_back(llvmValue2);
	llvmCallInst_params.push_back(llvmValue3);
	llvmCallInst_params.push_back(const_int32_5);
	llvmCallInst_params.push_back(const_int1_8);
	llvmCallInst 	= CallInst::Create(llvmMemsetFunction, llvmCallInst_params, "", currentBB);
	llvmCallInst->setCallingConv(CallingConv::C);
	llvmCallInst->setTailCall(false);
	AttributeSet Attributes;
	llvmCallInst->setAttributes(Attributes);

	return ;
}

static inline void translate_ir_native_call (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t			*irReturnItem;
	ir_item_t			*irValue3;
	ir_item_t			irFPointer;
	char				*functionName;

	/* Generate the call				*/
	irReturnItem			= IRMETHOD_getInstructionParameter1(inst);

	/* Fetch the function pointer			*/
	irValue3			= IRMETHOD_getInstructionParameter3(inst);
	if (IRDATA_isASymbol(irValue3)){
		functionName	= (char *) IRSYMBOL_unresolveSymbolFromIRItem(irValue3);
		IRSYMBOL_resolveSymbolFromIRItem(irValue3, &irFPointer);
	} else {
		functionName	= (char *)(JITNUINT)(inst->param_2).value.v;
		memcpy(&irFPointer, irValue3, sizeof(ir_item_t));
	}

	/* Generate the call instruction		*/
	translate_ir_general_call(self, llvmRoots, method, inst, llvmFunction, currentBB, (irReturnItem->value).v, irReturnItem->type_infos, irFPointer.value.v, JITFALSE, functionName, NULL, JITTRUE);

	return ;
}

static inline void translate_ir_icall (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t			*irReturnItem;
	ir_item_t			*irValue2;
	Value				*llvmPointerToFunction;
	Value				*llvmEntryPoint;

	/* Fetch the pointer to the method to call	*/
	irValue2			= IRMETHOD_getInstructionParameter2(inst);
	llvmPointerToFunction		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	llvmEntryPoint			= llvmPointerToFunction;

	/* Generate the call				*/
	irReturnItem			= IRMETHOD_getInstructionParameter1(inst);
	translate_ir_general_call(self, llvmRoots, method, inst, llvmFunction, currentBB, (irReturnItem->value).v, irReturnItem->type_infos, (IR_ITEM_VALUE)(JITNUINT)llvmEntryPoint, JITTRUE, NULL, NULL, JITFALSE);

	return ;
}

static inline void translate_ir_not (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	ir_instruction_t	tempInst;

	/* Generate the equivalent IRXOR instruction		*/
	memcpy(&tempInst, inst, sizeof(ir_instruction_t));
	tempInst.param_2.value.v	= -1;
	tempInst.param_2.type		= tempInst.param_1.internal_type;
	tempInst.param_2.internal_type	= tempInst.param_2.type;

	/* Generate LLVM code					*/
	translate_ir_binary(self, llvmRoots, method, &tempInst, llvmFunction, currentBB, Instruction::Xor, Instruction::Xor, Instruction::Xor);

	return ;
}

static inline void translate_ir_branchifpcnotinrange (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	//TODO this is required for exception handling

	return ;
}

static inline void translate_ir_startcatcher (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t	*irResult;
	Value		*llvmResult;
	Value		*exceptionThrown;
	Value		*PersonalityFn;
	Value		*exceptionThrownDescriptor;
	Value		*llvmOffsetValue;
	Value		*irExceptionThrown;
	LandingPadInst	*landingPadInst;
	BasicBlock	*landingPadBB;
	Type		*intTypeToUseForPointers;
	Type		*ptrType;

	/* Fetch the LLVM pointer type										*/
	intTypeToUseForPointers	= get_LLVM_integer_type_to_use_as_pointer(self, llvmRoots);

	/* Fetch the IR result location										*/
	irResult	= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM result location									*/
	llvmResult	= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Fetch the landing pad basic block									*/
	landingPadBB	= llvmFunction->basicBlocks[IRMETHOD_getNumberOfMethodBasicBlocks(method)];

	/* Create the landing pad instruction									*/
	PersonalityFn			= llvmRoots->personalityFunction;
	landingPadInst			= LandingPadInst::Create(llvmRoots->llvmExceptionType, PersonalityFn, 0, "", landingPadBB);
	exceptionThrownDescriptor	= (Value *) landingPadInst;
	landingPadInst->addClause(Constant::getNullValue(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL)));

	/* Load the exception thrown										*/
	exceptionThrown			= ExtractValueInst::Create(exceptionThrownDescriptor, 0, "", landingPadBB);
	new StoreInst(exceptionThrown, llvmFunction->exceptionThrown, landingPadBB);
	BranchInst::Create(currentBB, landingPadBB);

	/* Load the pointer of the exception object thrown 							*/
	exceptionThrown			= new LoadInst(llvmFunction->exceptionThrown, "", currentBB);

	/* Take our custom exception from the pointer returned by createOurException function			*/
	exceptionThrown			= new PtrToIntInst(exceptionThrown, intTypeToUseForPointers, "", currentBB);
	llvmOffsetValue			= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(sizeof(JITNUINT)*8, sizeof(struct _Unwind_Exception), true));
	irExceptionThrown		= BinaryOperator::Create(Instruction::Add, exceptionThrown, llvmOffsetValue,  "", currentBB);
	ptrType				= get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL);
	irExceptionThrown		= new IntToPtrInst(irExceptionThrown, PointerType::get(ptrType, 0), "", currentBB);
	irExceptionThrown		= new LoadInst(irExceptionThrown, "", currentBB);

	/* Convert the memory location that we are using to store the exception					*/
	llvmResult			= new BitCastInst(llvmResult, PointerType::get(ptrType, 0), "", currentBB);

	/* Store the pointer of the exception object thrown to the variable defined by the current instruction	*/
	assert(llvmFunction->exceptionThrown != NULL);
	new StoreInst(irExceptionThrown, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_free (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	Value			*llvmValue1;
	
	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);

	/* Fetch the LLVM value					*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);

	/* Convert the value					*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);

	/* Call the LLVM intrinsic				*/
	CallInst::Create(llvmRoots->freeFunction, llvmValue1, "", currentBB);

	return ;
}

static inline void translate_ir_generic_math (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, Function *f32, Function *f64, JITUINT16 irSignature32ResultType, JITUINT16 irSignature64ResultType){
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmResultLocation;
	Value			*llvmResult;

	/* Initialize the variables				*/
	llvmValue1		= NULL;
	llvmValue2		= NULL;
	llvmResult		= NULL;
	llvmResultLocation	= NULL;

	/* Fetch the IR values					*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value					*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	if (irValue2->type != NOPARAM){
		llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);
	}

	/* Perform the necessary conversions			*/
	if (!IRMETHOD_hasAFloatType(irValue1)){
		Type	*floatTypeToUse;
		if (IRDATA_getSizeOfType(irValue1) == 4){
			floatTypeToUse		= get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL);
		} else {
			floatTypeToUse		= get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL);
		}
		if (IRMETHOD_hasASignedType(irValue1)){
			llvmValue1		= new SIToFPInst(llvmValue1, floatTypeToUse, "", currentBB);
		} else {
			llvmValue1		= new UIToFPInst(llvmValue1, floatTypeToUse, "", currentBB);
		}
	}
	if (	(!IRMETHOD_hasAFloatType(irValue2))	&&
		(irValue2->type != NOPARAM)		){
		Type	*floatTypeToUse;
		if (IRDATA_getSizeOfType(irValue2) == 4){
			floatTypeToUse		= get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL);
		} else {
			floatTypeToUse		= get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL);
		}
		if (IRMETHOD_hasASignedType(irValue2)){
			llvmValue2		= new SIToFPInst(llvmValue2, floatTypeToUse, "", currentBB);
		} else {
			llvmValue2		= new UIToFPInst(llvmValue2, floatTypeToUse, "", currentBB);
		}
	}

	/* Fetch the LLVM result location			*/
	if (irResult->type != NOPARAM){
		llvmResultLocation	= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);
	}

	/* Prepare the arguments				*/
	std::vector<Value*> llvmCallInst_params;
	llvmCallInst_params.push_back(llvmValue1);
	if (llvmValue2 != NULL){
		llvmCallInst_params.push_back(llvmValue2);
	}

	/* Call the LLVM intrinsic				*/
	if (IRDATA_getSizeOfType(irValue1) == 4){
		llvmResult 		= CallInst::Create(f32, llvmCallInst_params, "", currentBB);
	} else {
		llvmResult	 	= CallInst::Create(f64, llvmCallInst_params, "", currentBB);
	}

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	if (llvmResultLocation != NULL){
		JITUINT16 irSignatureResultType;
		if (IRDATA_getSizeOfType(irValue1) == 4){
			irSignatureResultType	= irSignature32ResultType;
		} else {
			irSignatureResultType	= irSignature64ResultType;
		}

		/* Perform the necessary conversions due to the choice of functions used internally to implement math operations	*/
		if (irSignatureResultType != irResult->internal_type){
			llvmResult	= convert_LLVM_value(self, llvmRoots, irSignatureResultType, irResult->internal_type, irResult->type_infos, llvmResult, currentBB);
		}

		/* Store the value													*/
		new StoreInst(llvmResult, llvmResultLocation, currentBB);
	}

	return ;
}

static inline void translate_ir_realloc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	std::vector<Value*> 	args;
	
	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM values								*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);

	/* Fetch the LLVM result location							*/
	llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Perform the necessary casts								*/
	llvmValue1		= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irValue1->internal_type, llvmValue1, currentBB);

	/* Call the LLVM intrinsic								*/
	args.push_back(llvmValue1);
	args.push_back(llvmValue2);
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->reallocFunction, args, "", currentBB);

	/* Perform the necessary conversions							*/
	llvmMemoryAllocated	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irResult->internal_type, IRMPOINTER, llvmMemoryAllocated, currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_allocalign (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_alloc(self, llvmRoots, method, inst, llvmFunction, currentBB);//FIXME

	return ;
}

static inline void translate_ir_calloc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irValue2;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmValue2;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	std::vector<Value*> 	args;
	
	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irValue2		= IRMETHOD_getInstructionParameter2(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);
	llvmValue2		= get_LLVM_value_for_using(self, method, irValue2, llvmFunction, currentBB);

	/* Fetch the LLVM result location							*/
	llvmResult		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Create the arguments					*/
	args.push_back(llvmValue1);
	args.push_back(llvmValue2);

	/* Call the LLVM intrinsic								*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->callocFunction, args, "", currentBB);

	/* Perform the necessary conversions							*/
	llvmMemoryAllocated	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irResult->internal_type, IRMPOINTER, llvmMemoryAllocated, currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_alloc (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	ir_item_t		*irValue1;
	ir_item_t		*irResult;
	Value			*llvmValue1;
	Value			*llvmMemoryAllocated;
	Value			*llvmResult;
	
	/* Fetch the IR values									*/
	irValue1		= IRMETHOD_getInstructionParameter1(inst);
	irResult		= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value									*/
	llvmValue1		= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);

	/* Fetch the LLVM result location							*/
	llvmResult	= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Call the LLVM intrinsic								*/
	llvmMemoryAllocated 	= CallInst::Create(llvmRoots->mallocFunction, llvmValue1, "", currentBB);

	/* Perform the necessary conversions							*/
	llvmMemoryAllocated	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, irResult->internal_type, IRMPOINTER, llvmMemoryAllocated, currentBB);

	/* Store the pointer to the new allocated piece of memory to the LLVM result location	*/
	new StoreInst(llvmMemoryAllocated, llvmResult, currentBB);

	return ;
}

static inline void translate_ir_alloca (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	Value		*newVariable;
	Value		*llvmDst;
	Value		*llvmValue1;
	ir_item_t	*irValue1;
	ir_item_t	*irResult;
	Type		*llvmType;

	/* Fetch the IR values									*/
	irValue1	= IRMETHOD_getInstructionParameter1(inst);
	irResult	= IRMETHOD_getInstructionResult(inst);

	/* Fetch the LLVM value that specify how many bytes we need to allocate on the stack	*/
	llvmValue1	= get_LLVM_value_for_using(self, method, irValue1, llvmFunction, currentBB);

	/* Fetch the LLVM type that has the same amount of bytes requested			*/
	llvmType 	= IntegerType::get(llvmRoots->llvmModule->getContext(), 8);

	/* Allocate the parameter		*/
	newVariable	= new AllocaInst(llvmType, llvmValue1, "", currentBB);

	/* Fetch the LLVM value			*/
	llvmDst		= get_LLVM_value(self, llvmRoots, method, irResult, llvmFunction);

	/* Perform the conversions		*/
	if (IRMETHOD_hasAnIntType(irResult)){
		llvmDst		= new BitCastInst(llvmDst, PointerType::get(PointerType::get(llvmType, 0), 0), "", currentBB);
	}

	/* Store the value			*/
	new StoreInst(newVariable, llvmDst, currentBB);

	return ;
}

static inline void translate_ir_shr (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::AShr, Instruction::AShr, Instruction::LShr);

	return ;
}

static inline void translate_ir_div (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::FDiv, Instruction::SDiv, Instruction::UDiv);

	return ;
}

static inline void translate_ir_mul_ovf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_mul(self, llvmRoots, method, inst, llvmFunction, currentBB);

	return ;
}

static inline void translate_ir_mul (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::FMul, Instruction::Mul, Instruction::Mul);

	return ;
}

static inline void translate_ir_add_ovf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_add(self, llvmRoots, method, inst, llvmFunction, currentBB); //FIXME

	return ;
}

static inline void translate_ir_add (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::FAdd, Instruction::Add, Instruction::Add);

	return ;
}

static inline void translate_ir_sub (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::FSub, Instruction::Sub, Instruction::Sub);

	return ;
}

static inline void translate_ir_or (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::Or, Instruction::Or, Instruction::Or);

	return ;
}

static inline void translate_ir_and (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::And, Instruction::And, Instruction::And);

	return ;
}

static inline void translate_ir_xor (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::Xor, Instruction::Xor, Instruction::Xor);

	return ;
}

static inline void translate_ir_shl (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary(self, llvmRoots, method, inst, llvmFunction, currentBB, Instruction::Shl, Instruction::Shl, Instruction::Shl);

	return ;
}

static inline void translate_ir_eq (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary_comparison(self, llvmRoots, method, inst, llvmFunction, currentBB, FCmpInst::FCMP_OEQ, ICmpInst::ICMP_EQ, ICmpInst::ICMP_EQ);

	return ;
}

static inline void translate_ir_lt (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary_comparison(self, llvmRoots, method, inst, llvmFunction, currentBB, ICmpInst::FCMP_OLT, ICmpInst::ICMP_SLT, ICmpInst::ICMP_ULT);

	return ;
}

static inline void translate_ir_gt (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	translate_ir_binary_comparison(self, llvmRoots, method, inst, llvmFunction, currentBB, FCmpInst::FCMP_OGT, ICmpInst::ICMP_SGT, ICmpInst::ICMP_UGT);

	return ;
}

static inline void translate_ir_branch_if (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_conditional_branch(self, llvmRoots, labels, method, inst, llvmFunction, currentBB, ICmpInst::ICMP_NE);

	return ;
}

static inline void translate_ir_branch_if_not (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_instruction_t **labels, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_conditional_branch(self, llvmRoots, labels, method, inst, llvmFunction, currentBB, ICmpInst::ICMP_EQ);

	return ;
}

static inline void translate_ir_sin (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->sin32Function, llvmRoots->sin64Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_cos (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->cos32Function, llvmRoots->cos64Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_ceil (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->ceil32Function, llvmRoots->ceil64Function, IRFLOAT32, IRFLOAT64);

	return;
}

static inline void translate_ir_cosh (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->cosh32Function, llvmRoots->cosh64Function, IRFLOAT32, IRFLOAT64);

	return;
}

static inline void translate_ir_acos (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->acos32Function, llvmRoots->acos64Function, IRFLOAT32, IRFLOAT64);

	return;
}

static inline void translate_ir_isnan (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->isnan32Function, llvmRoots->isnan64Function, IRINT32, IRINT32);

	return ;
}

static inline void translate_ir_isinf (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->isinf32Function, llvmRoots->isinf64Function, IRINT32, IRINT32);

	return ;
}

static inline void translate_ir_sqrt (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->sqrt32Function, llvmRoots->sqrt64Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_log10 (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->log1032Function, llvmRoots->log1064Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_exp (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->exp32Function, llvmRoots->exp64Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_pow (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->pow32Function, llvmRoots->pow64Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_floor (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	translate_ir_generic_math(self, llvmRoots, method, inst, llvmFunction, currentBB, llvmRoots->floor32Function, llvmRoots->floor64Function, IRFLOAT32, IRFLOAT64);

	return ;
}

static inline void translate_ir_startfinally (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB) {
	return ;
}

static inline void error_trampoline (void){
	fprintf(stderr, "ERROR: Execution in a trampoline. \n");
	abort();
}

static inline void internal_free_memory_used_for_previous_translations (t_llvm_function_internal *llvmFunction){
	free_memory_used_for_llvm_function(llvmFunction);

	return ;
}

static inline void internal_generate_instruction_range_store (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_instruction_t *inst){
	//TODO this is required for exception handling

	return ;
}

static inline Value * internal_allocateANewVariable (Type *llvmType, JITUINT32 alignment, BasicBlock *entryBB){
	AllocaInst	*allocaInst;

	allocaInst			= new AllocaInst(llvmType, "", entryBB);
	allocaInst->setAlignment(alignment);

	return allocaInst;
}

static inline void internal_translateGlobals (IRVM_t *self){
	XanList			*irGlobals;
	XanListItem		*item;
	IRVM_internal_t		*llvmRoots;

	/* Assertions.
	 */
	assert(self != NULL);

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 	= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Generate the DATA section.
	 * Translate IR globals to LLVM globals.
	 */
	irGlobals	= IRDATA_getGlobals();
	item		= xanList_first(irGlobals);
	while (item != NULL){
		ir_global_t	*g;
		GlobalVariable	*llvmG;

		/* Fetch the global.
		 */
		g	= (ir_global_t *)item->data;
		assert(g != NULL);

		/* Create the LLVM global.
		 */
		if (xanHashTable_lookup(llvmRoots->globals, g) == NULL){
			llvmG	= get_globalVariable_for_data(self, g->IRType, (char *)g->name, (const char *)g->initialValue, g->size, g->isConstant);
			xanHashTable_insert(llvmRoots->globals, g, llvmG);
		}

		/* Fetch the next element from the list.
	 	 */
		item	= item->next;
	}

	/* Free the memory.
	 */
	xanList_destroyList(irGlobals);

	return ;
}

static inline char * internal_linkerCommandLine (IRVM_t *self){
	JITUINT32			programNameLength;
	JITUINT32			linkCommandLength;
	JITUINT32			prefixLength;
	JITUINT32			librariesToStaticallyLinkLength;
	JITUINT32			numberOfLibs;
	JITUINT32			libraryDirsLength;
	char				*programName;
	char				*librariesToStaticallyLink;
	char				*linkCommand;
	char 				*libraryDirs;
    const char          *extraLinkerOptions;

	/* Fetch the name of the program.
	 */
	programName 		= strdup((char *)IRPROGRAM_getProgramName());
	programNameLength	= strlen(programName);
	for (JITUINT32 count=0; count < programNameLength; count++){
		if (programName[count] == '.'){
			programName[count]	= '\0';
			programNameLength	= count;
			break ;
		}
	}

	/* Fetch the length of the prefix string.
	 */
	prefixLength			= strlen(PREFIX);
	librariesToStaticallyLinkLength	= prefixLength + 1;

	/* Set the directories where to search for libraries.
	 */
	const char libraryPartialDirs[]		= "-L/lib/iljit -L/lib -L/lib/iljit/clrs";

	/* Set up directories of libraries to link.
	 */
	libraryDirsLength		= strlen(libraryPartialDirs) + ((prefixLength + 1) * 3);
	libraryDirs			= (char *)allocFunction(libraryDirsLength);
	snprintf(libraryDirs, libraryDirsLength, "-L%s/lib/iljit -L%s/lib -L%s/lib/iljit/clrs", PREFIX, PREFIX, PREFIX);

	/* Select which libraries to link statically.
	 */
	const char* libs[] 	= {"lib/iljit/clrs/clrbase.a", "lib/iljit/loaders/iljit_ecmasoft_decoder.a"};
	numberOfLibs	= 2;
	for (JITUINT32 libsID=0; libsID < numberOfLibs; libsID++){
		librariesToStaticallyLinkLength	+= prefixLength + 1 + strlen(libs[libsID]) + 1;
	}
	librariesToStaticallyLink	= (char *)allocFunction(librariesToStaticallyLinkLength);
	for (JITUINT32 libsID=0; libsID < numberOfLibs; libsID++){
		strncat(librariesToStaticallyLink, PREFIX, librariesToStaticallyLinkLength);
		strncat(librariesToStaticallyLink, "/", librariesToStaticallyLinkLength);
		strncat(librariesToStaticallyLink, libs[libsID], librariesToStaticallyLinkLength);
		strncat(librariesToStaticallyLink, " ", librariesToStaticallyLinkLength);
	}

	/* Select which libraries to link dynamically.
	 */
	char librariesToDynamicallyLink[]	= "-lclimanager -lcompilermemorymanager -ldla -ldominance -lhwmodel -lilbinary -liljitgc -liljitil -liljitiroptimizer -liljitmm -liljitsm -liljit -liljitu -lirmanager -lirvirtualmachine_llvm -lirvirtualmachine -lmanfred -lplatform -lpluginmanager -lxan -lm -lpthread -ldl";

    /* Select the extra linker options.
     */
    if (self->linkerOptions != NULL){
        extraLinkerOptions  = (const char *)self->linkerOptions;
    } else {
        extraLinkerOptions  = "";
    }

	/* Create the link command line.
	 */
	linkCommandLength		= 1024;
	linkCommand			= (char *)allocFunction(linkCommandLength);
	snprintf(linkCommand, linkCommandLength, "g++ -o %s.ildjit *.o %s %s %s %s -m32", programName, libraryDirs, librariesToStaticallyLink, librariesToDynamicallyLink, extraLinkerOptions);

	/* Free the memory.
	 */
	freeFunction(programName);
	freeFunction(librariesToStaticallyLink);
	freeFunction(libraryDirs);

	return linkCommand;
}

static inline Value * internal_fetchEffectiveAddress (IRVM_t *self, IRVM_internal_t *llvmRoots, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB, ir_method_t *method, ir_item_t *irBa, ir_item_t *irOffset, Type *llvmPtrType){
	Value		*llvmEa;
	Value		*llvmBa;
	Value		*intLlvmBa;
	Value		*llvmOffset;
	Value		*intLlvmEa;
	Type		*intTypeToUseAsPointer;

    /* Assertions.
     */
    assert(irBa != NULL);
	
    /* Fetch the base address.
     */
	llvmBa			        = get_LLVM_value_for_using(self, method, irBa, llvmFunction, currentBB);

	/* Convert the base address to the integer type to use for pointers.
	 */
	intTypeToUseAsPointer	= get_LLVM_integer_type_to_use_as_pointer(self, llvmRoots);
	llvmBa		        	= cast_LLVM_value_between_pointers_and_integers(self, llvmRoots, IRMPOINTER, irBa->internal_type, llvmBa, currentBB);
    intLlvmBa		        = new PtrToIntInst(llvmBa, intTypeToUseAsPointer, "", currentBB);

    /* Check if there is an offset to add.
     */
    if (    (!IRDATA_isAConstant(irOffset))     ||
            ((irOffset->value).v != 0)          ){

        /* Fetch the offset.
         */
        llvmOffset		= get_LLVM_value_for_using(self, method, irOffset, llvmFunction, currentBB);
        if (((size_t)IRDATA_getSizeOfType(irOffset)) < sizeof(JITNUINT)){
            llvmOffset		= new ZExtInst(llvmOffset, intTypeToUseAsPointer, "", currentBB);
        }

	    /* Add the offset to the base address to create the effective address.
	     */
	    intLlvmEa		= BinaryOperator::Create(Instruction::Add, intLlvmBa, llvmOffset,  "", currentBB);

    } else {
        intLlvmEa       = intLlvmBa;
    }

    /* Cast the effective address to be a pointer.
     */
	llvmEa			= new IntToPtrInst(intLlvmEa, llvmPtrType, "", currentBB);

    return llvmEa;
}
