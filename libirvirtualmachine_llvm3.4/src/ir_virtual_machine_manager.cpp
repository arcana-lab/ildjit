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
#include <llvm/Support/ManagedStatic.h>
#include <ir_virtual_machine_runtime.h>
#include <ir_virtual_machine_pass_manager.h>
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <config.h>
#include <IRtoBCMaterializer.h>
#include <MachineCodeGeneratorEventListener.h>
// End

#define INTEL_x86_64bits	"x86_64-pc-linux-gnu"
#define INTEL_x86_32bits	"i386-pc-linux-gnu"

using namespace std;

static inline void internal_createFunctionUtilities (IRVM_t *self, IRVM_internal_t *llvmRoots, JITINT8 *leaveExecutionFunctionName);

IRVM_t *localVM;

void IRVM_init(IRVM_t *self, t_running_garbage_collector *gc, IRVM_type * (*lookupValueType)(TypeDescriptor *type), ir_method_t * (*getIRMethod)(IR_ITEM_VALUE method), void (*leaveExecution)(JITINT32 exitCode), JITINT8 *leaveExecutionFunctionName, t_jit_function * (*getJITMethod)(IR_ITEM_VALUE method), JITINT32 (*recompilerFunction)(void *ilmethod, void *caller, ir_instruction_t *inst)){
	IRVM_internal_t				*vm;
	const JITINT8				*programName;
	char 					*moduleID;
	JITUINT32				moduleIDLen;
	CodeGenOpt::Level 			optLevel;
	IRtoBCMaterializer			*ourMaterializer;
	Type					*llvmPtrType;
	Type					*llvmIntType;

	/* Cache pointers					*/
	self->gc = gc;
	self->lookupValueType = lookupValueType;
	self->getIRMethod = getIRMethod;
	self->leaveExecution = leaveExecution;
	self->getJITMethod = getJITMethod;
	self->recompilerFunction = recompilerFunction;

	/* Create the LLVM data structure			*/
	vm 			= (IRVM_internal_t *)allocMemory(sizeof(IRVM_internal_t));
	self->data		= vm;
	localVM			= self;

	/* Create our materializer, which translates IR methods to LLVM bitcode	*/
	ourMaterializer		= new IRtoBCMaterializer(self);

	/* Create our listener to LLVM events					*/
	vm->llvmListener	= new MachineCodeGeneratorEventListener(self);

	/* Create the module ID			*/
	programName 		= IRPROGRAM_getProgramName();
	if (programName == NULL){
		programName = (const JITINT8 *)"Unknown";
	}
	moduleIDLen 		= STRLEN(programName) + strlen(".bc") + 1;
	assert(moduleIDLen > 0);
	moduleID 		= (char *)allocFunction(moduleIDLen);
	snprintf(moduleID, moduleIDLen, "%s", programName);
	StringRef	stringModuleID(moduleID);

	/* Create the LLVM module		*/
	vm->llvmModule	= new Module(stringModuleID, getGlobalContext());
	//vm->llvmModule->setTargetTriple("i386-pc-linux-gnu");

	/* Set the materializer			*/
	vm->llvmModule->setMaterializer(ourMaterializer);

	/* Initialize LLVM			*/
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();

	/* Set the optimization level		*/
	optLevel	= CodeGenOpt::None;
	if ((self->optimizer).enableMachineDependentOptimizations) {
		optLevel	= CodeGenOpt::Aggressive;
	}

	/* Setup the LLVM pass manager 		*/
	setupCodeOptimizations(self);

	/* Create the LLVM JIT			*/
    	std::string ErrStr;
	TargetOptions Opts;

	Opts.PrintMachineCode		= false;

	vm->executionEngine = EngineBuilder(vm->llvmModule)
		.setErrorStr(&ErrStr)
		.setUseMCJIT((self->behavior).staticCompilation == JITTRUE)
		.setOptLevel(optLevel)
		.setAllocateGVsWithCode(false)
		.setTargetOptions(Opts)
		.create();
  	if (!vm->executionEngine) {
		fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
		abort();
	}

	/* Set the listener			*/
	vm->executionEngine->RegisterJITEventListener(vm->llvmListener);

	/* Set the compilation scheme		*/
	vm->executionEngine->DisableLazyCompilation(false);
	assert(!vm->executionEngine->isLazyCompilationDisabled());

	/* Allocate the mapping table from IR	*
	 * methods to LLVM functions		*/
	vm->llvmFunctions	= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	vm->irMethods		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	vm->ilMethods		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);
	vm->globals		= xanHashTable_new(11, JITFALSE, allocFunction, dynamicReallocFunction, freeFunction, NULL, NULL);

	/* Define the important LLVM types	*/
	llvmPtrType		= get_LLVM_type(self, vm, IRMPOINTER, NULL);
	llvmIntType		= get_LLVM_type(self, vm, IRINT32, NULL);
	Type *llvmStructType[] 	= {llvmPtrType, llvmIntType};
	vm->llvmExceptionType	= StructType::get(vm->llvmModule->getContext(), ArrayRef<Type*>(llvmStructType));

	/* Create function utilities		*/
	internal_createFunctionUtilities(self, vm, leaveExecutionFunctionName);

	/* Free the memory			*/
	freeMemory(moduleID);

	/* Leave the initialization		*/
	return ;
}

void IRVM_shutdown (IRVM_t *self){
	IRVM_internal_t				*vm;
	XanHashTableItem			*item;

	/* Fetch the VM internal structure.
	 */
	vm	= (IRVM_internal_t *)self->data;
	assert(vm != NULL);

	/* Destroy the IRVM.
	 */
	item	= xanHashTable_first(vm->llvmFunctions);
	while (item != NULL){
		t_jit_function 			*backendFunction;
		t_llvm_function_internal	*llvmFunction;

		/* Fetch the function.
		 */
		backendFunction	= (t_jit_function *)item->element;
		assert(backendFunction != NULL);
		llvmFunction = (t_llvm_function_internal *) backendFunction->data;
		assert(llvmFunction != NULL);

		/* Free the function.
		 */
		free_memory_used_for_llvm_function(llvmFunction);
		
		item	= xanHashTable_next(vm->llvmFunctions, item);
	}
	xanHashTable_destroyTable(vm->ilMethods);
	xanHashTable_destroyTable(vm->irMethods);
	xanHashTable_destroyTable(vm->llvmFunctions);
	xanHashTable_destroyTable(vm->globals);

	/* Shutdown LLVM.
	 */
	delete vm->FPM;
	delete vm->MPM;
	delete vm->executionEngine;
	llvm_shutdown();
	delete vm->llvmListener;

	/* Free LLVM data structure.
	 */
	freeMemory(vm);

	return ;
}

JITINT32 IRVM_isCompilationAvailable (IRVM_t *self){

	/* Assertions				*/
	assert(self != NULL);

	return JITTRUE;
}

void libirvirtualmachineCompilationFlags (char *buffer, int bufferLength){

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

void libirvirtualmachineCompilationTime (char *buffer, int bufferLength){

	/* Assertions				*/
	assert(buffer != NULL);

	snprintf(buffer, sizeof(char) * bufferLength, "%s %s", __DATE__, __TIME__);
}

char * libirvirtualmachineVersion (){
	return const_cast<char *>(VERSION);
}

static inline void internal_createFunctionUtilities (IRVM_t *self, IRVM_internal_t *llvmRoots, JITINT8 *leaveExecutionFunctionName){
	std::vector<llvm::Type*>	argTypes;
	XanList				*irTypes;
	Type				*retType;
	FunctionType			*functionType;
	string				functionName;

	/* Allocate the necessary memory.
	 */
	irTypes		= xanList_new(allocFunction, freeFunction, NULL);

	// We need a general solution for function names TODO
	/* Create the function to allocate exceptions at runtime	*/
	xanList_emptyOutList(irTypes);
	xanList_append(irTypes, (void *)(JITNUINT)IRMPOINTER);
	llvmRoots->createExceptionFunction	= get_LLVM_function_of_binary_method(self, const_cast<JITINT8 *>((const signed char *)"createOurException"), IRMPOINTER, irTypes);

	/* Create the function to throw exceptions at runtime		*/
	xanList_emptyOutList(irTypes);
 	xanList_append(irTypes, (void *)(JITNUINT)IRMPOINTER);
	llvmRoots->throwExceptionFunction	= get_LLVM_function_of_binary_method(self, const_cast<JITINT8 *>((const signed char *)"_Unwind_RaiseException"), IRINT32, irTypes);

	/* Create the function to allocate arrays			*/
	xanList_emptyOutList(irTypes);
	xanList_append(irTypes, (void *)(JITNUINT)IRMPOINTER);
	xanList_append(irTypes, (void *)(JITNUINT)IRINT32);
	llvmRoots->allocArrayFunction		= get_LLVM_function_of_binary_method(self, const_cast<JITINT8 *>((const signed char *)"GC_allocArray"), IROBJECT, irTypes);

	/* Create the function to free objects				*/
	xanList_emptyOutList(irTypes);
	xanList_append(irTypes, (void *)(JITNUINT)IRMPOINTER);
	llvmRoots->freeObjectFunction		= get_LLVM_function_of_binary_method(self, const_cast<JITINT8 *>((const signed char *)"GC_freeObject"), IRVOID, irTypes);

	/* Create the function to allocate objects			*/
	xanList_emptyOutList(irTypes);
	xanList_append(irTypes, (void *)(JITNUINT)IRMPOINTER);
	xanList_append(irTypes, (void *)(JITNUINT)IRUINT32);
	llvmRoots->allocObjectFunction		= get_LLVM_function_of_binary_method(self, const_cast<JITINT8 *>((const signed char *)"GC_allocObject"), IROBJECT, irTypes);

	/* Create the function to exit					*/
	xanList_emptyOutList(irTypes);
	xanList_append(irTypes, (void *)(JITNUINT)IRINT32);
	llvmRoots->exitFunction			= get_LLVM_function_of_binary_method(self, leaveExecutionFunctionName, IRVOID, irTypes);

	/* Create the function to memcpy				*/
	functionName			= "ir_memcpy"; 
	llvmRoots->memcpyFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->memcpyFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL), FuncTy_4_args, false);
		llvmRoots->memcpyFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->memcpyFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->memcpyFunction->setAttributes(Attributes);
	}

	/* Create the function to memcmp				*/
	functionName			= "memcmp"; 
	llvmRoots->memcmpFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->memcmpFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->memcmpFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->memcmpFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->memcmpFunction->setAttributes(Attributes);
	}

	/* Create the function to memcmp				*/
	functionName			= "ir_memset"; 
	llvmRoots->memsetFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->memsetFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRVOID, NULL), FuncTy_4_args, false);
		llvmRoots->memsetFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->memsetFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->memsetFunction->setAttributes(Attributes);
	}

	/* Create the function to strchr				*/
	functionName			= "strchr"; 
	llvmRoots->strchrFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->strchrFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL), FuncTy_4_args, false);
		llvmRoots->strchrFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->strchrFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->strchrFunction->setAttributes(Attributes);
	}

	/* Create the function to strcmp				*/
	functionName			= "strcmp"; 
	llvmRoots->strcmpFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->strcmpFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->strcmpFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->strcmpFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->strcmpFunction->setAttributes(Attributes);
	}

	/* Create the function to strlen				*/
	functionName			= "strlen"; 
	llvmRoots->strlenFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->strlenFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->strlenFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->strlenFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->strlenFunction->setAttributes(Attributes);
	}

	/* Create the function to malloc				*/
	functionName			= "ir_malloc"; 
	llvmRoots->mallocFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->mallocFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL), FuncTy_4_args, false);
		llvmRoots->mallocFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->mallocFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->mallocFunction->setAttributes(Attributes);
	}

	/* Create the function to calloc				*/
	functionName			= "calloc"; 
	llvmRoots->callocFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->callocFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL), FuncTy_4_args, false);
		llvmRoots->callocFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->callocFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->callocFunction->setAttributes(Attributes);
	}

	/* Create the function to realloc				*/
	functionName			= "realloc"; 
	llvmRoots->reallocFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->reallocFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRINT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL), FuncTy_4_args, false);
		llvmRoots->reallocFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->reallocFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->reallocFunction->setAttributes(Attributes);
	}

	/* Create the function to free					*/
	functionName			= "free"; 
	llvmRoots->freeFunction	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->freeFunction == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRMPOINTER, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRVOID, NULL), FuncTy_4_args, false);
		llvmRoots->freeFunction 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->freeFunction->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->freeFunction->setAttributes(Attributes);
	}

	/* Create the function for sqrt(float)				*/
	functionName			= "llvm.sqrt.f32"; 
	llvmRoots->sqrt32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->sqrt32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->sqrt32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->sqrt32Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for sqrt(double)				*/
	functionName			= "llvm.sqrt.f64"; 
	llvmRoots->sqrt64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->sqrt64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->sqrt64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->sqrt64Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for sin(float)				*/
	functionName			= "llvm.sin.f32"; 
	llvmRoots->sin32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->sin32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->sin32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->sin32Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for sin(double)				*/
	functionName			= "llvm.sin.f64"; 
	llvmRoots->sin64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->sin64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->sin64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->sin64Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for ceil(float)				*/
	functionName			= "ceilf"; 
	llvmRoots->ceil32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->ceil32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->ceil32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->ceil32Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for ceil(double)				*/
	functionName			= "ceil"; 
	llvmRoots->ceil64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->ceil64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->ceil64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->ceil64Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for cos(float)				*/
	functionName			= "llvm.cos.f32"; 
	llvmRoots->cos32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->cos32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->cos32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->cos32Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for cos(double)				*/
	functionName			= "llvm.cos.f64"; 
	llvmRoots->cos64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->cos64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->cos64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->cos64Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for cosh(float)				*/
	functionName			= "coshf"; 
	llvmRoots->cosh32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->cosh32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->cosh32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->cosh32Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->cosh32Function->setAttributes(Attributes);
	}

	/* Create the function for cosh(double)				*/
	functionName			= "cosh"; 
	llvmRoots->cosh64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->cosh64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->cosh64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->cosh64Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->cosh64Function->setAttributes(Attributes);
	}

	/* Create the function for acos(float)				*/
	functionName			= "acosf"; 
	llvmRoots->acos32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->acos32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->acos32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->acos32Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->acos32Function->setAttributes(Attributes);
	}

	/* Create the function for acos(double)				*/
	functionName			= "acos"; 
	llvmRoots->acos64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->acos64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->acos64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->acos64Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->acos64Function->setAttributes(Attributes);
	}

	/* Create the function for floor(float)				*/
	functionName			= "llvm.floor.f32" ;
	llvmRoots->floor32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->floor32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->floor32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->floor32Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->floor32Function->setAttributes(Attributes);
	}

	/* Create the function for floor(double)				*/
	functionName			= "llvm.floor.f64"; 
	llvmRoots->floor64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->floor64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->floor64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->floor64Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->floor64Function->setAttributes(Attributes);
	}

	/* Create the function for isnan(float)				*/
	functionName			= "ir_isnanf"; 
	llvmRoots->isnan32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->isnan32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->isnan32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->isnan32Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->isnan32Function->setAttributes(Attributes);
	}

	/* Create the function for isnan(double)				*/
	functionName			= "ir_isnan"; 
	llvmRoots->isnan64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->isnan64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->isnan64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->isnan64Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->isnan64Function->setAttributes(Attributes);
	}

	/* Create the function for isinf(float)				*/
	functionName			= "ir_isinff"; 
	llvmRoots->isinf32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->isinf32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->isinf32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->isinf32Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->isinf32Function->setAttributes(Attributes);
	}

	/* Create the function for isinf(double)				*/
	functionName			= "ir_isinf"; 
	llvmRoots->isinf64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->isinf64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRINT32, NULL), FuncTy_4_args, false);
		llvmRoots->isinf64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->isinf64Function->setCallingConv(CallingConv::C);
	}
	{
		LLVMContext& Context = llvmRoots->llvmModule->getContext();
		AttributeSet Attributes;
		Attributes.addAttribute(Context, 4294967295U, Attribute::NoUnwind);
		llvmRoots->isinf64Function->setAttributes(Attributes);
	}

	/* Create the function for pow(float, float)			*/
	functionName			= "llvm.pow.f32"; 
	llvmRoots->pow32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->pow32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->pow32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->pow32Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for pow(double, double)			*/
	functionName			= "llvm.pow.f64"; 
	llvmRoots->pow64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->pow64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->pow64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->pow64Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for log10(float)				*/
	functionName			= "llvm.log10.f32"; 
	llvmRoots->log1032Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->log1032Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->log1032Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->log1032Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for log10(double)			*/
	functionName			= "llvm.log10.f64"; 
	llvmRoots->log1064Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->log1064Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->log1064Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->log1064Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for exp(float)				*/
	functionName			= "llvm.exp.f32"; 
	llvmRoots->exp32Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->exp32Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT32, NULL), FuncTy_4_args, false);
		llvmRoots->exp32Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->exp32Function->setCallingConv(CallingConv::C);
	}

	/* Create the function for exp(double)				*/
	functionName			= "llvm.exp.f64"; 
	llvmRoots->exp64Function	= llvmRoots->llvmModule->getFunction(functionName);
	if (llvmRoots->exp64Function == NULL) {
		std::vector<Type*>	FuncTy_4_args;
		FunctionType		*FuncTy_4;
		FuncTy_4_args.push_back(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL));
		FuncTy_4 			= FunctionType::get(get_LLVM_type(self, llvmRoots, IRFLOAT64, NULL), FuncTy_4_args, false);
		llvmRoots->exp64Function 	= Function::Create(FuncTy_4, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);
		llvmRoots->exp64Function->setCallingConv(CallingConv::C);
	}

	/* Create our personality					*/
	argTypes.clear();
	retType					= get_LLVM_type(self, llvmRoots, IRINT32, NULL);
	functionType				= FunctionType::get(retType, argTypes, false);
	functionName				= "__gxx_personality_v0";
	llvmRoots->personalityFunction		= Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, llvmRoots->llvmModule);

	/* Free the memory.
	 */
	xanList_destroyList(irTypes);

	return ;
}
