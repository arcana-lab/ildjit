/*
 * Copyright (C) 2011  Simone Campanoni, Glenn Holloway
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
#ifndef IR_VIRTUAL_MACHINE_BACKEND_H
#define IR_VIRTUAL_MACHINE_BACKEND_H

#include <xanlib.h>
#include <jitsystem.h>
#include <ir_optimizer.h>
#include <garbage_collector.h>
#include <framework_garbage_collector.h>
#include <ir_virtual_machine.h>
#include <algorithm>
#include <unwind.h>

// LLVM headers
#include <llvm/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Constants.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Function.h>
#include <llvm/CallingConv.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>
#include <llvm/InlineAsm.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Intrinsics.h>

using namespace llvm;
// End

typedef struct {
    void            *entryPoint;			/**< First memory address where the machine code of the function has been stored.				*/
    void		*lastPoint;			/**< Last memory address used to store the machine code of the LLVM function					*/
    Value		**variables;			/**< Set of parameters, locals and temporaries created by instructions						*/
    Function	*function;			/**< LLVM function representation 										*/
    FunctionType	*functionSignature;		/**< LLVM signature of the function										*/
    BasicBlock	**basicBlocks;			/**< LLVM basic blocks included inside the LLVM function							*/
    BlockAddress	**basicBlockAddresses;		/**< LLVM basic block addresses											*/
    Value		**finallyBlockReturnAddresses;	/**< Finally block specific variables										*/
    Value		*exceptionThrown;		/**< Pointer to the exception object thrown. The object always resides inside the memory heap			*/
    JITBOOLEAN	isNative;			/**< JITTRUE if the function is native (i.e., C function)							*/
} t_llvm_function_internal;

typedef struct {
    JITUINT16	irType;
    TypeDescriptor	*ilType;
    Type		*llvmType;
    JITUINT64	typeSize;
} IRVM_type_internal_t;

typedef struct {
    Type					*llvmExceptionType;
    Function				*allocObjectFunction;
    Function				*allocArrayFunction;
    Function				*freeObjectFunction;
    Function				*createExceptionFunction;
    Function				*throwExceptionFunction;
    PointerType				*createExceptionFunctionType;
    PointerType				*throwExceptionFunctionType;
    PointerType				*newArrayFunctionType;
    PointerType				*freeObjectFunctionType;
    PointerType				*allocObjectFunctionType;
    Function				*exitFunction;
    Function				*mallocFunction;
    Function				*strlenFunction;
    Function				*strcmpFunction;
    Function				*strchrFunction;
    Function				*memcpyFunction;
    Function				*memsetFunction;
    Function				*memcmpFunction;
    Function				*callocFunction;
    Function				*reallocFunction;
    Function				*freeFunction;
    Function				*sqrt32Function;
    Function				*sqrt64Function;
    Function				*sin32Function;
    Function				*sin64Function;
    Function				*ceil32Function;
    Function				*ceil64Function;
    Function				*cos32Function;
    Function				*cos64Function;
    Function				*cosh32Function;
    Function				*cosh64Function;
    Function				*acos32Function;
    Function				*acos64Function;
    Function				*floor32Function;
    Function				*floor64Function;
    Function				*isnan32Function;
    Function				*isnan64Function;
    Function				*isinf32Function;
    Function				*isinf64Function;
    Function				*pow32Function;
    Function				*pow64Function;
    Function				*exp32Function;
    Function				*exp64Function;
    Function				*log1032Function;
    Function				*log1064Function;
    Function				*personalityFunction;
    Module 					*llvmModule;
    ExecutionEngine 			*executionEngine;
    XanHashTable				*llvmFunctions;			/**< Mapping from (ir_method_t *) to (t_jit_function *)					*/
    XanHashTable				*irMethods;			/**< Mapping from (Function *) to (ir_method_t *) 					*/
    XanHashTable				*ilMethods;			/**< Mapping from (ir_method_t *) to input language methods (i.e., IR_ITEM_VALUE) 	*/
    FunctionPassManager 			*FPM;
    PassManager				*MPM;
} IRVM_internal_t;

#endif
