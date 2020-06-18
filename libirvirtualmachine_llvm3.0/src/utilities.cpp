/*
 * Copyright (C) 2011  Simone Campanoni, Glenn Holloway
 *
 * ir_virtual_machine.c - This is a translator from the IR language into the assembly language.

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
#include <ir_language.h>
#include <compiler_memory_manager.h>
#include <jit_metadata.h>
#include <jitsystem.h>

// My headers
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <utilities.h>
// End

void allocateBackendFunction (IRVM_t *self, t_jit_function *f, ir_method_t *irMethod){

	/* Assertions.
	 */
	assert(self != NULL);
	assert(f != NULL);

	if (f->data == NULL){
		ir_signature_t	*irSignature;
		void		*meta;
		meta	= IRMETHOD_getMethodMetadata(irMethod, METHOD_METADATA);
		assert(meta != NULL);
		irSignature = IRMETHOD_getSignature(irMethod);
		IRVM_translateIRMethodSignatureToBackendSignature(self, irSignature, f);
		IRVM_newBackendMethod(self, meta, f, JITTRUE, JITTRUE);
	}

	return ;
}

Function * get_LLVM_function_of_binary_method (IRVM_t *self, JITINT8 *functionName, JITUINT16 irReturnType, XanList *paramTypes){
	IRVM_internal_t	*llvmRoots;
	Function	*f;
	AttrListPtr 	attributes;

	/* Assertions.
	 */
	assert(self != NULL);
	assert(functionName != NULL);

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 	= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Create the function.
	 */
	f		= llvmRoots->llvmModule->getFunction((const char *)functionName);
	if (f == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		Type			*returnLLVMType;
		FunctionType		*FuncTy_4;
		XanListItem		*item;

		/* Create the parameters.
	 	 */
		item		= xanList_first(paramTypes);
		while (item != NULL){
			JITUINT16	irType;
			Type		*llvmType;
			irType		= (JITUINT16)(JITNUINT)item->data;
			llvmType	= get_LLVM_type(self, llvmRoots, irType, NULL);
			FuncTy_4_args.push_back(llvmType);
			item	= item->next;
		}

		/* Create the return type.
		 */
		returnLLVMType			= get_LLVM_type(self, llvmRoots, irReturnType, NULL);

		/* Create the function.
		 */
		FuncTy_4 			= FunctionType::get(returnLLVMType, FuncTy_4_args, false);
		f			 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, (const char *)functionName, llvmRoots->llvmModule);
		f->setCallingConv(CallingConv::C);
	}

	/* Set the attributes.
	 */
	SmallVector<AttributeWithIndex, 4> Attrs;
	AttributeWithIndex PAWI;
	PAWI.Index = 4294967295U; 
	PAWI.Attrs = 0  | Attribute::NoReturn | Attribute::NoUnwind;
	Attrs.push_back(PAWI);
	attributes = AttrListPtr::get(Attrs.begin(), Attrs.end());
	f->setAttributes(attributes);

    /* Link the function.
     */
	llvmRoots->executionEngine->getPointerToFunction(f);

	return f;
}

JITINT8 * get_C_function_name (ir_method_t *m){
	JITINT8		*functionName;
	
	functionName		= IRMETHOD_getNativeMethodName(m);
	if (functionName == NULL){
		fprintf(stderr, "ILDJIT: ERROR = Method %s does not have a correspondent C function\n", IRMETHOD_getSignatureInString(m));
		abort();
	}
	return (JITINT8 *)strdup((char *)functionName);
}

Type * get_LLVM_type (IRVM_t *self, IRVM_internal_t *vm, JITUINT32 IRType, TypeDescriptor *ilType){
	Type 		*t;
	ir_item_t	item;
	JITUINT32	bytes;

	if (IRType == IRNINT){
		if (sizeof(JITNINT) == 4){
			IRType	= IRINT32;
		} else {
			IRType	= IRINT64;
		}
	}
	if (IRType == IRNUINT){
		if (sizeof(JITNUINT) == 4){
			IRType	= IRUINT32;
		} else {
			IRType	= IRUINT64;
		}
	}
	if (IRMETHOD_isAPointerType(IRType)){
		t = PointerType::get(IntegerType::get(vm->llvmModule->getContext(), 8), 0);

	} else {
		switch (IRType){
			case IRINT8:
			case IRUINT8:
				t = IntegerType::get(vm->llvmModule->getContext(), 8);
				break;
			case IRINT16:
			case IRUINT16:
				t = IntegerType::get(vm->llvmModule->getContext(), 16);
				break;
			case IRINT32:
			case IRUINT32:
				t = IntegerType::get(vm->llvmModule->getContext(), 32);
				break;
			case IRINT64:
			case IRUINT64:
				t = IntegerType::get(vm->llvmModule->getContext(), 64);
				break;
			case IRFLOAT32:
				t = Type::getFloatTy(vm->llvmModule->getContext());
				break;
			case IRFLOAT64:
				t = Type::getDoubleTy(vm->llvmModule->getContext());
				break;
			case IRVOID:
				t = Type::getVoidTy(vm->llvmModule->getContext());
				break;
			case IRVALUETYPE:

				/* Fetch the size of the data structure	*/
				memset(&item, 0, sizeof(ir_item_t));
				item.type		= IRVALUETYPE;
				item.internal_type	= IRVALUETYPE;
				item.type_infos		= ilType;
				bytes			= IRDATA_getSizeOfType(&item);

				/* Create the type			*/
				t 	= ArrayType::get(IntegerType::get(vm->llvmModule->getContext(), 8), bytes);
				break;
			default:
				fprintf(stderr, "ILDJIT_LLVM: Type %u is not handled\n", IRType);
				abort();
		}
	}

	return t;
}

Value * get_LLVM_value_for_defining (IRVM_t *self, ir_method_t *method, ir_item_t *item, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	IRVM_internal_t *llvmRoots;
	Value		*v;

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 	= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the LLVM value.
	 */
	v		= get_LLVM_value(self, llvmRoots, method, item, llvmFunction);
	assert(v != NULL);

	/* Check whether we need to load the value or not.
	 * If the IR item is a variable or it is a global of a blob, then the value v just taken is a pointer to the real data.
	 */
	if (	(item->type == IRGLOBAL)		&&
		(item->internal_type == IRVALUETYPE)	){
		std::vector<Constant*> 	const_ptr_indices;
		ConstantInt		*const_int32_0;

		/* Fetch the pointer of the global.
		 */
		const_int32_0	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, StringRef("0"), 10));
		const_ptr_indices.push_back(const_int32_0);
		const_ptr_indices.push_back(const_int32_0);
		v		= ConstantExpr::getGetElementPtr((GlobalVariable *)v, const_ptr_indices);
	}

	return v;
}

Value * get_LLVM_value_for_using (IRVM_t *self, ir_method_t *method, ir_item_t *item, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	IRVM_internal_t *llvmRoots;
	Value		*v;

	/* Fetch LLVM's top-level handles.
	 */
	llvmRoots 	= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the LLVM value.
	 */
	v		= get_LLVM_value(self, llvmRoots, method, item, llvmFunction);

	/* Check whether we need to load the value or not.
	 * If the IR item is a variable or it is a global of a blob, then the value v just taken is a pointer to the real data.
	 */
	if (	(IRDATA_isAVariable(item))		||
		(item->type == IRGLOBAL)		){
		v	= new LoadInst(v, "", currentBB);
	}

	return v;
}

Value * get_LLVM_value (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_item_t *item, t_llvm_function_internal *llvmFunction){
	ir_item_t	resolvedSymbol;
	Value		*v;
	JITUINT32	type;
	JITBOOLEAN	is64;
	Type		*intTypeToUseForPointers;

	/* Check if we are in a 64-bit platform			*/
	is64			= sizeof(JITNUINT) == 8;

	/* Fetch the integer type for pointers			*/
	if (is64){
		intTypeToUseForPointers	= get_LLVM_type(self, llvmRoots, IRUINT64, NULL);
	} else {
		intTypeToUseForPointers	= get_LLVM_type(self, llvmRoots, IRUINT32, NULL);
	}

	/* Resolve a possible symbol				*/
	if (IRDATA_isASymbol(item)){
		assert(!(self->behavior).staticCompilation);
		IRSYMBOL_resolveSymbolFromIRItem(item, &resolvedSymbol);
		item	= &resolvedSymbol;
	}
	assert(item != NULL);

	/* Generate the LLVM value				*/
	v	= NULL;
	type	= item->type;
	switch (type){
		case IRNUINT:
			if (sizeof(JITNUINT) == 4){
				type	= IRUINT32;
			} else {
				type	= IRUINT64;
			}
			break;
		case IRNINT:
			if (sizeof(JITNINT) == 4){
				type	= IRINT32;
			} else {
				type	= IRINT64;
			}
			break;
		case IRNFLOAT:
			if (sizeof(JITNFLOAT) == 4){
				type	= IRFLOAT32;
			} else {
				type	= IRFLOAT64;
			}
			break;
	}
	if (IRMETHOD_isAPointerType(type)){
		Constant	*constV;
		constV 	= ConstantInt::get(intTypeToUseForPointers, (item->value).v);
		v	= ConstantExpr::getIntToPtr(constV, Type::getInt8PtrTy(llvmRoots->llvmModule->getContext()));

	} else {
		switch (type){
			case IROFFSET:
				v	= llvmFunction->variables[(item->value).v];
				break ;
			case IRINT64:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(64, (item->value).v, true));
				break;
			case IRINT32:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, (item->value).v, true));
				break;
			case IRINT16:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(16, (item->value).v, true));
				break;
			case IRINT8:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(8, (item->value).v, true));
				break;
			case IRUINT64:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(64, (item->value).v, false));
				break;
			case IRUINT32:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(32, (item->value).v, false));
				break;
			case IRUINT16:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(16, (item->value).v, false));
				break;
			case IRUINT8:
				v 	= ConstantInt::get(llvmRoots->llvmModule->getContext(), APInt(8, (item->value).v, false));
				break;
			case IRFLOAT32:
				v	= ConstantFP::get(llvmRoots->llvmModule->getContext(), APFloat((float)((item->value).f)));
				break;
			case IRFLOAT64:
				v	= ConstantFP::get(llvmRoots->llvmModule->getContext(), APFloat((double)((item->value).f)));
				break;
			default:
				abort();
		}
	}
	assert(v != NULL);

	return v;
}

Value * get_coerced_byte_count (IRVM_t *self, IRVM_internal_t *llvmRoots, ir_method_t *method, ir_item_t *irValue, t_llvm_function_internal *llvmFunction, BasicBlock *currentBB){
	Value		*llvmValue;
	JITBOOLEAN	is64;
	Type		*llvmInt32Type;
        Type            *llvmInt64Type;

	/* Check if we are in a 64-bit platform			*/
	is64			= sizeof(JITNUINT) == 8;

	/* Fetch the LLVM types					*/
	llvmInt32Type		= get_LLVM_type(self, llvmRoots, IRINT32, NULL);
	llvmInt64Type		= get_LLVM_type(self, llvmRoots, IRINT64, NULL);

	/* Fetch the LLVM value					*/
	llvmValue		= get_LLVM_value(self, llvmRoots, method, irValue, llvmFunction);
	if (irValue->type == IROFFSET){
		llvmValue		= new LoadInst(llvmValue, "", currentBB);
	}

	/* Coerce the byte count				*/
	if (is64){
		if (irValue->internal_type != IRINT64){
			JITUINT32	irBytes;
			assert(IRMETHOD_hasAnIntType(irValue));
			irBytes		= IRDATA_getSizeOfType(irValue);
			if (irBytes < 8){
				if (IRMETHOD_hasASignedType(irValue)){
					llvmValue		= new SExtInst(llvmValue, llvmInt64Type, "", currentBB);
				} else {
					llvmValue		= new ZExtInst(llvmValue, llvmInt64Type, "", currentBB);
				}
			} else {
				assert(irBytes == 8);
				llvmValue		= new BitCastInst(llvmValue, llvmInt64Type, "", currentBB);
			}
		}

	} else {
		if (irValue->internal_type != IRINT32){
			JITUINT32	irBytes;
			assert(IRMETHOD_hasAnIntType(irValue));
			irBytes	= IRDATA_getSizeOfType(irValue);
			if (irBytes > 4){
				llvmValue		= new TruncInst(llvmValue, llvmInt32Type, "", currentBB);
			} else if (irBytes < 4){
				if (IRMETHOD_hasASignedType(irValue)){
					llvmValue		= new SExtInst(llvmValue, llvmInt32Type, "", currentBB);
				} else {
					llvmValue		= new ZExtInst(llvmValue, llvmInt32Type, "", currentBB);
				}
			} else {
				llvmValue		= new BitCastInst(llvmValue, llvmInt32Type, "", currentBB);
			}
		}
	}

	return llvmValue;
}

Value * cast_LLVM_value_between_pointers_and_integers (IRVM_t *self, IRVM_internal_t *llvmRoots, JITUINT32 targetIRType, JITUINT32 sourceIRType, Value *llvmValueToConvert, BasicBlock *currentBB){

	/* Check if we have a value type		*/
	if (targetIRType == IRVALUETYPE){

		/* It is not job of the back end to convert these values	*/
		return llvmValueToConvert;
	}

	/* Check if we need to perform any conversion	*/
	if (targetIRType != sourceIRType){
		Type		*targetLlvmType;
		targetLlvmType		= get_LLVM_type(self, llvmRoots, targetIRType, NULL);

		if (	(IRMETHOD_isAnIntType(sourceIRType))		&&
			(IRMETHOD_isAPointerType(targetIRType))		){
			Type		*llvmValueToConvertType;

			/* Check if we really need to convert	*/
			llvmValueToConvertType	= llvmValueToConvert->getType();
			if (!llvmValueToConvertType->isPointerTy()){

				/* Convert the LLVM value		*/
				llvmValueToConvert	= new IntToPtrInst(llvmValueToConvert, targetLlvmType, "", currentBB);
			}
		}

		if (	(IRMETHOD_isAPointerType(sourceIRType))		&&
			(IRMETHOD_isAnIntType(targetIRType))		){

			/* Convert the LLVM value		*/
			llvmValueToConvert	= new PtrToIntInst(llvmValueToConvert, targetLlvmType, "", currentBB);
		}
	}

	return llvmValueToConvert;
}

Type * get_LLVM_integer_type_to_use_as_pointer (IRVM_t *self, IRVM_internal_t *llvmRoots){
	Type			*intTypeToUseForPointers;
	JITBOOLEAN		is64;

	/* Check if we are in a 64-bit platform			*/
	is64			= sizeof(JITNUINT) == 8;

	/* Fetch the integer type for pointers			*/
	if (is64){
		intTypeToUseForPointers	= get_LLVM_type(self, llvmRoots, IRUINT64, NULL);
	} else {
		intTypeToUseForPointers	= get_LLVM_type(self, llvmRoots, IRUINT32, NULL);
	}

	return intTypeToUseForPointers;
}

Value * convert_LLVM_value (IRVM_t *self, IRVM_internal_t *llvmRoots, JITUINT16 currentIRType, JITUINT16 irTypeToConvert, TypeDescriptor *ilTypeToConvert, Value *llvmValueToConvert, BasicBlock *currentBB){
	Value	*convertedValue;
	Type	*resultType;

	/* Fetch the LLVM result type		*/
	resultType			= get_LLVM_type(self, llvmRoots, irTypeToConvert, ilTypeToConvert);

	/* Convert the value			*/
	if (IRMETHOD_isAFloatType(currentIRType)){
		ir_item_t	tmp;
		JITUINT32	srcBytes;
		JITUINT32	dstBytes;
		switch (irTypeToConvert){
			case IRFLOAT32:
			case IRFLOAT64:
			case IRNFLOAT:
				memset(&tmp, 0, sizeof(ir_item_t));
				tmp.type			= irTypeToConvert;
				tmp.internal_type		= tmp.type;
				srcBytes			= IRDATA_getSizeOfIRType(currentIRType);
				dstBytes			= IRDATA_getSizeOfType(&tmp);
				if (dstBytes < srcBytes){
					convertedValue			= new FPTruncInst(llvmValueToConvert, resultType, "", currentBB);
				} else if (dstBytes > srcBytes){
					convertedValue			= new FPExtInst(llvmValueToConvert, resultType, "", currentBB);
				} else {
					convertedValue			= llvmValueToConvert;
				}
				break;
			case IRINT8:
			case IRINT16:
			case IRINT32:
			case IRINT64:
			case IRNINT:
				convertedValue			= new FPToSIInst(llvmValueToConvert, resultType, "", currentBB);
				break;
			case IRUINT8:
			case IRUINT16:
			case IRUINT32:
			case IRUINT64:
			case IRNUINT:
				convertedValue			= new FPToUIInst(llvmValueToConvert, resultType, "", currentBB);
				break;
			default:
				abort();
		}

	} else if (IRMETHOD_isAnIntType(currentIRType)){
		ir_item_t	tmp;
		JITUINT32	srcBytes;
		JITUINT32	dstBytes;
		switch (irTypeToConvert){
			case IRFLOAT32:
			case IRFLOAT64:
			case IRNFLOAT:
				if (IRMETHOD_isASignedType(currentIRType)){
					convertedValue			= new SIToFPInst(llvmValueToConvert, resultType, "", currentBB);
				} else {
					convertedValue			= new UIToFPInst(llvmValueToConvert, resultType, "", currentBB);
				}
				break;
			case IRINT8:
			case IRINT16:
			case IRINT32:
			case IRINT64:
			case IRNINT:
				memset(&tmp, 0, sizeof(ir_item_t));
				tmp.type			= irTypeToConvert;
				tmp.internal_type		= tmp.type;
				srcBytes			= IRDATA_getSizeOfIRType(currentIRType);
				dstBytes			= IRDATA_getSizeOfType(&tmp);
				if (dstBytes < srcBytes){
					convertedValue			= new TruncInst(llvmValueToConvert, resultType, "", currentBB);
				} else if (dstBytes > srcBytes){
					if (IRMETHOD_isASignedType(currentIRType)){
						convertedValue			= new SExtInst(llvmValueToConvert, resultType, "", currentBB);
					} else {
						convertedValue			= new ZExtInst(llvmValueToConvert, resultType, "", currentBB);
					}
				} else {
					convertedValue			= llvmValueToConvert;
				}
				break;
			case IRUINT8:
			case IRUINT16:
			case IRUINT32:
			case IRUINT64:
			case IRNUINT:
				memset(&tmp, 0, sizeof(ir_item_t));
				tmp.type			= irTypeToConvert;
				tmp.internal_type		= tmp.type;
				srcBytes			= IRDATA_getSizeOfIRType(currentIRType);
				dstBytes			= IRDATA_getSizeOfType(&tmp);
				if (dstBytes < srcBytes){
					convertedValue			= new TruncInst(llvmValueToConvert, resultType, "", currentBB);
				} else if (dstBytes > srcBytes){
					convertedValue			= new ZExtInst(llvmValueToConvert, resultType, "", currentBB);
				} else {
					convertedValue			= llvmValueToConvert;
				}
				break;
			case IRCLASSID:
			case IRMETHODID:
			case IRMETHODENTRYPOINT:
			case IRMPOINTER:
			case IRFPOINTER:
			case IROBJECT:
			case IRSTRING:
			case IRSIGNATURE:
				convertedValue			= new IntToPtrInst(llvmValueToConvert, resultType, "", currentBB);
				break;
			default:
				abort();
		}

	} else if (IRMETHOD_isAPointerType(currentIRType)){
		if (IRMETHOD_isAPointerType((JITUINT32)irTypeToConvert)){
			convertedValue	= llvmValueToConvert;

		} else {
			switch (irTypeToConvert){
				case IRINT8:
				case IRINT16:
				case IRINT32:
				case IRINT64:
				case IRNINT:
				case IRUINT8:
				case IRUINT16:
				case IRUINT32:
				case IRUINT64:
				case IRNUINT:
					convertedValue	= new PtrToIntInst(llvmValueToConvert, resultType, "", currentBB);
					break;
				default:
					abort();
			}
		}

	} else if (IRMETHOD_isADataValueType(currentIRType)){
		resultType		= PointerType::get(resultType, 0);
		llvmValueToConvert	= new BitCastInst(llvmValueToConvert, resultType, "", currentBB);
		convertedValue		= new LoadInst(llvmValueToConvert, "", currentBB);

	} else {
		convertedValue		= new BitCastInst(llvmValueToConvert, resultType, "", currentBB);
	}

	return convertedValue;
}

void free_memory_used_for_llvm_function (t_llvm_function_internal *llvmFunction){

	/* Free the LLVM variable array					*/
	if (llvmFunction->variables != NULL) {
		freeMemory(llvmFunction->variables);
		llvmFunction->variables 		= NULL;
	}

	/* Free the LLVM basic block arrays				*/
	if (llvmFunction->basicBlocks != NULL){
		freeMemory(llvmFunction->basicBlocks);
		llvmFunction->basicBlocks		= NULL;
	}
	if (llvmFunction->basicBlockAddresses != NULL){
		freeMemory(llvmFunction->basicBlockAddresses);
		llvmFunction->basicBlockAddresses	= NULL;
	}

	/* Free the finally blocks information				*/
	if (llvmFunction->finallyBlockReturnAddresses != NULL){
		freeMemory(llvmFunction->finallyBlockReturnAddresses);
		llvmFunction->finallyBlockReturnAddresses	= NULL;
	}

	return ;
}

extern "C" {
/// This function is the struct _Unwind_Exception API mandated delete function 
/// used by foreign exception handlers when deleting our exception 
/// (OurException), instances.
/// @param reason @link http://refspecs.freestandards.org/abi-eh-1.21.html 
/// @unlink
/// @param expToDelete exception instance to delete
void deleteFromUnwindOurException(_Unwind_Reason_Code reason, OurUnwindException *expToDelete) {
    if (expToDelete && (expToDelete->exception_class == 42)) {
//    freeFunction(((char*) expToDelete) + ourBaseFromUnwindOffset);
  }
}                

/// Creates (allocates on the heap), an exception (OurException instance),
/// of the supplied type info type.
/// @param type type info type
OurUnwindException * createOurException (void *exceptionObject) {
	size_t size = sizeof(OurException);
	OurException *ret = (OurException*) memset(allocFunction(size), 0, size);
	(ret->type).type = 42;
	(ret->unwindException).exception_class = 42;
	(ret->unwindException).exception_cleanup = deleteFromUnwindOurException;

	ret->thrownObject	= exceptionObject;

	return (&(ret->unwindException));
}
}
