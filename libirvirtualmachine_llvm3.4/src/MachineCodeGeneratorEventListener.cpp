#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// My headers
#include <ir_virtual_machine_backend.h>
#include <MachineCodeGeneratorEventListener.h>
// End

namespace llvm {

void MachineCodeGeneratorEventListener::NotifyFunctionEmitted(const Function &F, void *Code, size_t Size, const EmittedFunctionDetails &Details) {
	ir_method_t			*irMethod;
	IRVM_internal_t	 		*llvmRoots;
	t_jit_function			*backendMethod;
	t_llvm_function_internal	*llvmBackendMethod;
	const Function 			*llvmFunctionInCompilation;

	/* Fetch LLVM's top-level handles		*/
	llvmRoots 			= (IRVM_internal_t *) this->irVM->data;
	assert(llvmRoots != NULL);

	/* Fetch the IR method					*/
	llvmFunctionInCompilation	= &F;
	irMethod			= (ir_method_t *)xanHashTable_lookup(llvmRoots->irMethods, (void *)const_cast<Function *>(llvmFunctionInCompilation));
	if (irMethod == NULL){

		/* Ignore function stubs created by LLVM in the course of translating to machine code.	*
		 * These stubs have no corresponding IR methods.					*/
		return ;
	}
	assert(irMethod != NULL);

	/* Fetch the backend method				*/
	backendMethod			= (t_jit_function *)xanHashTable_lookup(llvmRoots->llvmFunctions, irMethod);
	assert(backendMethod != NULL);
	llvmBackendMethod		= (t_llvm_function_internal *)backendMethod->data;
	assert(llvmBackendMethod != NULL);

	/* Store the information				*/
	llvmBackendMethod->entryPoint	= Code;
	llvmBackendMethod->lastPoint	= (void *)(((JITNUINT)llvmBackendMethod->entryPoint) + Size);
	assert(llvmBackendMethod->entryPoint < llvmBackendMethod->lastPoint);
	
	return ;
}

void MachineCodeGeneratorEventListener::NotifyFreeingMachineCode(void *OldPtr) {
}

}
