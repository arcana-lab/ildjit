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

void IRVM_newBackendMethod (IRVM_t *self, void *method, t_jit_function *backendFunction, JITBOOLEAN isRecompilable, JITBOOLEAN isCallableExternally){
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t 		*llvmRoots;
	ir_method_t			*irMethod;
	GlobalValue::LinkageTypes	linkTypeToUse;
	char				*functionName;

	/* Assertions				*/
	assert(self != NULL);
	assert(backendFunction != NULL);

	/* Fetch LLVM's top-level handles	*/
	llvmRoots = (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Fetch the IR method			*/
	irMethod = self->getIRMethod((IR_ITEM_VALUE) (JITNUINT) method);

	/* Fetch the LLVM function		*/
	llvmFunction = (t_llvm_function_internal *) backendFunction->data;
	assert(llvmFunction != NULL);
	assert(llvmFunction->functionSignature != NULL);
	assert(llvmFunction->function == NULL);

	/* Choose the linking type		*/
	if (isCallableExternally){
		linkTypeToUse	= GlobalValue::ExternalLinkage;
	} else {
		linkTypeToUse	= GlobalValue::InternalLinkage;
	}

	/* Create the LLVM function		*/
	functionName	= (char *)IRMETHOD_getSignatureInString(irMethod);
	if (functionName == NULL){
		functionName	= (char *)IRMETHOD_getMethodName(irMethod);
	}
	llvmFunction->function = Function::Create(llvmFunction->functionSignature, linkTypeToUse, functionName, llvmRoots->llvmModule);

	/* Check if we have already created the function	*/
	if (xanHashTable_lookup(llvmRoots->irMethods, llvmFunction->function) == irMethod){
		assert(xanHashTable_lookup(llvmRoots->ilMethods, irMethod) == method);
		return ;
	}

	/* Set the attributes of the call			*/
	llvmFunction->function->setCallingConv(CallingConv::C);
	AttrListPtr func_PAL;
	{
		SmallVector<AttributeWithIndex, 4> Attrs;
		AttributeWithIndex PAWI;
		PAWI.Index = 4294967295U; 
		PAWI.Attrs = 0 | Attribute::UWTable;
		Attrs.push_back(PAWI);
		func_PAL = AttrListPtr::get(Attrs.begin(), Attrs.end());
	}
	llvmFunction->function->setAttributes(func_PAL);

	/* Store the mapping from the IR method	*
	 * to the LLVM function			*/
	assert(xanHashTable_lookup(llvmRoots->irMethods, llvmFunction->function) == NULL);
	assert(xanHashTable_lookup(llvmRoots->ilMethods, irMethod) == NULL);
	xanHashTable_insert(llvmRoots->llvmFunctions, irMethod, backendFunction);
	xanHashTable_insert(llvmRoots->irMethods, llvmFunction->function, irMethod);
	xanHashTable_insert(llvmRoots->ilMethods, irMethod, method);

	return ;
}

void IRVM_destroyFunction (IRVM_t *self, t_jit_function *backendFunction){
	t_llvm_function_internal	*llvmFunction;
	IRVM_internal_t			*vm;
	XanHashTableItem		*item;

	/* Assertions				*/
	assert(backendFunction != NULL);

	/* Fetch the VM internal structure.
	 */
	vm	= (IRVM_internal_t *)self->data;
	assert(vm != NULL);

	/* Fetch the LLVM function		*/
	llvmFunction = (t_llvm_function_internal *) backendFunction->data;

	/* Destroy the LLVM function		*/
	item	= xanHashTable_first(vm->llvmFunctions);
	while (item != NULL){

		/* Fetch the function.
		 */
		if (item->element == backendFunction){
			t_llvm_function_internal	*llvmFunction;

			/* Fetch the LLVM function.
			 */
			llvmFunction 	= (t_llvm_function_internal *) backendFunction->data;

			/* Destroy the LLVM function.
			 */
			if (llvmFunction != NULL){
				free_memory_used_for_llvm_function(llvmFunction);
			}

			/* Remove the element from the table.
			 */
			xanHashTable_removeElement(vm->llvmFunctions, item->elementID);
			break;
		}

		item	= xanHashTable_next(vm->llvmFunctions, item);
	}

	if (llvmFunction != NULL){
		freeMemory(llvmFunction);
	}

	/* Return				*/
	return ;
}

void IRVM_translateIRTypeToIRVMType (IRVM_t *self, JITUINT16 irType, IRVM_type *irvmType, TypeDescriptor *ilClass){
	IRVM_internal_t 		*llvmRoots;
	IRVM_type_internal_t		*t;

	/* Fetch LLVM's top-level handles	*/
	llvmRoots 				= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	/* Define the type			*/
	t		= (IRVM_type_internal_t *)allocFunction(sizeof(IRVM_type_internal_t));
	t->irType	= irType;
	t->ilType	= ilClass;
	t->llvmType	= get_LLVM_type(self, llvmRoots, irType, ilClass);

	return ;
}

void IRVM_setIRVMStructure (IRVM_t *self, IRVM_type *irvmType, JITNINT structureSize, JITNINT alignment){
	IRVM_internal_t 		*llvmRoots;
	IRVM_type_internal_t		*t;

	/* Fetch LLVM's top-level handles	*/
	llvmRoots 				= (IRVM_internal_t *) self->data;
	assert(llvmRoots != NULL);

	t					= (IRVM_type_internal_t *)allocFunction(sizeof(IRVM_type_internal_t));
	irvmType->type				= t;
	t->typeSize				= structureSize;
	t->llvmType 				= ArrayType::get(IntegerType::get(llvmRoots->llvmModule->getContext(), 8), structureSize); //FIXME we need to use the alignment information

	return ;
}
