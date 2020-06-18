/*
 * Copyright (C) 2006 - 2012 Campanoni Simone, Di Biagio Andrea, Tartara Michele
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <jit/jit.h>
#include <jit/jit-dump.h>
#include <string.h>
#include <time.h>
#include <jitsystem.h>
#include <ir_language.h>
#include <ir_optimization_interface.h>
#include <compiler_memory_manager.h>
#include <jit_metadata.h>
#include <error_codes.h>
#include <platform_API.h>
#include <ir_virtual_machine.h>

// My headers
#include <ir_virtual_machine_backend.h>
#include <ir_virtual_machine_system.h>
#include <config.h>
// End

#define DIM_BUF 1024

typedef struct {
	jit_label_t label;
	JITUINT32 ID;
	JITBOOLEAN fixed;
} t_jit_label;

static inline JITUINT32 fromJITTypeToIRType (jit_type_t jitType);
static inline t_jit_label * insert_label (XanList *labels, JITUINT32 label_ID);
static inline JITINT32 check_labels (XanList *labels);
static inline jit_value_t make_variable (IRVM_t *_this, ir_method_t *method, ir_item_t *item, t_jit_function_internal *jitFunction);
static inline JITINT16 bind_instruction_offset (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline void set_volatile_variables (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline void fetch_temporaries (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction);
static inline JITINT16 fetch_parameters (ir_method_t *method, t_jit_function_internal *jitFunction);
static inline JITINT32 internal_isRootSetEmpty (ir_method_t *method, t_jit_function_internal *jitFunction);
static inline void internal_lockVM (IRVM_t *_this);
static inline void internal_unlockVM (IRVM_t *_this);
static inline void internal_lockLibjit (IRVM_t_internal *_this);
static inline void internal_unlockLibjit (IRVM_t_internal *_this);
static inline void variablesToRootSets (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction);
static inline JITINT32 internal_addRootSetProfile (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction);
static inline void internal_set_variables_addressable (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction);


/* IR -> machine code translation functions			*/
/**
 * @brief Translates the IR instruction IRLOADREL in its JIT counterpart
 *
 * If a static field is being loaded, the first parameter of the IR instruction (inst->param_1) must be an IRMPOINTER containing the address of the field. Otherwise it must be an IROFFSET indicating the number of the local of the function containing the address.
 */
static inline JITINT16 translate_ir_load_rel (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_ret (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_newobj (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_freeobj (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_exit (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_newarr (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_load_elem (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_store_elem (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_add (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_sizeof (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_sub (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_mul (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_div (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_rem (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_and (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_array_length (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_shl (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_shr (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_not (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_or (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_xor (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_store_rel (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_add_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_sub_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_mul_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_conv (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_conv_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_label (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_branch (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_branchif (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_branchifnot (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_call_finally (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_call_filter (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_start_filter (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_start_finally (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_branch_if_pc_not_in_range (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_eq (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_checknull (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_lt (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_gt (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_store (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_neg (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_call (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_vcall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_nativecall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_librarycall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_icall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_initmemory (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_memory_copy (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_alloca (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_alloc (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_realloc (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_free (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_alloc_align (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_sqrt (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_floor (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_cos (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_cosh (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_acos (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_sin (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_pow (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_throw (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_get_address (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_end_filter (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_end_finally (ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_start_catcher (ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_isNaN (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 translate_ir_isInf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction);
static inline JITINT16 internal_translate_ir_conv (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction, JITBOOLEAN checking_overflow);

static inline JITINT32 internal_run (t_jit_function_internal *jitFunction, void **args, void *returnArea);
static inline jit_type_t internal_fromIRTypeToJITType (IRVM_t *_this, JITUINT32 IRType, TypeDescriptor *class);
static inline JITINT32 internal_recompilerFunctionWrapper (jit_function_t function);
static inline JITINT32 internal_dummyRecompiler (jit_function_t function, void *pc);

void IRVM_destroyStackTrace (IRVM_stackTrace *stack){
	jit_stack_trace_free(stack->stack_trace);
	freeFunction(stack);
	return;
}

JITUINT32 IRVM_getStackTraceOffset (IRVM_t *IRVM, IRVM_stackTrace *stack, JITUINT32 position){
	return jit_stack_trace_get_offset(((IRVM_t_internal *) IRVM->data)->context, stack->stack_trace, position);
}

void * IRVM_getStackTraceFunctionAt (IRVM_t *IRVM, IRVM_stackTrace *stack, JITUINT32 position){
	jit_function_t current_jit_function;
	IRVM_t_internal *internalVM;
	void *current_method;

	/* Assertions				*/
	assert(IRVM != NULL);
	assert(stack != NULL);

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) IRVM->data;
	assert(internalVM != NULL);

	/* Fetch the libjit function		*/
	current_jit_function = jit_stack_trace_get_function(internalVM->context, stack->stack_trace, position);
	if (current_jit_function == NULL) {
		return NULL;
	}

	/* Get the Method (if present)          *
	* associated with the current          *
	* jit-function                         */
	current_method = jit_function_get_meta(current_jit_function, METHOD_METADATA);

	/* Return				*/
	return current_method;
}

JITUINT32 IRVM_getStackTraceSize (IRVM_stackTrace *stack){
	return jit_stack_trace_get_size(stack->stack_trace);
}

IRVM_stackTrace * IRVM_getStackTrace (void){
	IRVM_stackTrace *s;

	s = allocFunction(sizeof(IRVM_stackTrace));
	s->stack_trace = jit_exception_get_stack_trace();
	return s;
}

void IRVM_throwBuiltinException (IRVM_t *self, JITUINT32 exceptionType){
	switch (exceptionType) {
	    case OUT_OF_MEMORY_EXCEPTION:
		    jit_exception_builtin(JIT_RESULT_OUT_OF_MEMORY);
		    break;
	    default:
		    abort();
	}
}

void IRVM_throwException (IRVM_t *self, void *object){
	jit_exception_throw(object);
}

void IRVM_init (IRVM_t *_this, t_running_garbage_collector *gc, IRVM_type * (*lookupValueType)(TypeDescriptor *type), ir_method_t * (*getIRMethod)(IR_ITEM_VALUE method), void (*leaveExecution)(JITINT32 exitCode), t_jit_function * (*getJITMethod)(IR_ITEM_VALUE method), JITINT32 (*recompilerFunction)(void *ilmethod, void *caller, ir_instruction_t *inst)){
	jit_type_t params_ctor[1];
	jit_type_t params[3];
	IRVM_t_internal *data;
	pthread_mutexattr_t mutex_attr;

	/* Assertions			*/
	assert(_this != NULL);
	assert(gc != NULL);
	assert(lookupValueType != NULL);

	_this->gc = gc;
	_this->lookupValueType = lookupValueType;
	_this->getIRMethod = getIRMethod;
	_this->leaveExecution = leaveExecution;
	_this->getJITMethod = getJITMethod;
	_this->recompilerFunction = recompilerFunction;

	/* Allocate the necessary memory		*/
	data = allocFunction(sizeof(IRVM_t_internal)); \
	_this->data = data;

	/* Initialize the mutex				*/
	PLATFORM_initMutexAttr(&mutex_attr);
	PLATFORM_setMutexAttr_type(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	PLATFORM_initMutex(&(_this->vmMutex), &mutex_attr);

	/* Create the JIT context					*/
	data->context = jit_context_create();
	assert(data->context != NULL);
	if (!jit_context_supports_threads(data->context)) {
		print_err("IRVIRTUALMACHINE: ERROR: Libjit does not support threads. ", 0);
		abort();
	}

	/* Set the Libjit optimization level	*/
	if ((_this->optimizer).enableMachineDependentOptimizations) {
		(_this->behavior).libjitOptimizations = jit_function_get_max_optimization_level();
	}

	/* Make the signature of the basic      *
	 * constructor of the exceptions	*/
	params_ctor[0] = jit_type_void_ptr;
	data->basicExceptionConstructorJITSignature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params_ctor, 1, 1);

	/* Make the signature of the leave	*
	 * execution function			*/
	params_ctor[0] = jit_type_int;
	data->leaveExecutionSignature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params_ctor, 1, 1);

	/* Create allocObject signature */
	params[0] = jit_type_void_ptr;
	params[1] = jit_type_uint;
	data->signAllocObject = jit_type_create_signature(jit_abi_cdecl, jit_type_void_ptr, params, 2, 0);

	/* Create createArray signature */
	params[0] = jit_type_void_ptr;
	params[1] = jit_type_int;
	data->signCreateArray = jit_type_create_signature(jit_abi_cdecl, jit_type_void_ptr, params, 2, 0);

	/* Create the freeObject signature	*/
	params[0] = jit_type_void_ptr;
	data->signFreeObject = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 1, 0);

	/* Return				*/
	return;
}

void IRVM_setExceptionHandler (IRVM_t *_this, void * (*exceptionHandler)(int exception_type)){

	/* Assertions				*/
	assert(_this != NULL);
	assert(exceptionHandler != NULL);

	/* Lock Libjit				*/
	internal_lockLibjit(_this->data);

	/* Set a LIBJIT EXCEPTION HANDLER: USED *
	 * TO PROPERLY INITIALIZE EXCEPTION     *
	 * OBJECTS.				*/
	jit_exception_set_handler(exceptionHandler);

	/* Unlock Libjit			*/
	internal_unlockLibjit(_this->data);

	/* Return				*/
	return;
}

void IRVM_makeTheMethodCallable (IRVM_t *_this, t_jit_function *jitFunction, ir_method_t *method){
	t_jit_function_internal *internalFunction;

	/* Assertions				*/
	assert(_this != NULL);
	assert(jitFunction != NULL);

	/* Cache some pointers			*/
	internalFunction = jitFunction->data;
	assert(internalFunction != NULL);

	/* Check if there is an entry point	*/
	if (internalFunction->entryPoint == NULL) {
		return;
	}

	/* Setup the trampolines		*/
	internal_lockLibjit(_this->data);
	jit_function_setup_entry(internalFunction->function, internalFunction->entryPoint);
	internal_unlockLibjit(_this->data);

	/* Dump the methods in the target       *
	 * machine code				*/
	if (    ((_this->behavior).dumpAssembly.dumpAssembly)   &&
		(!jit_uses_interpreter())                       ) {
		assert((_this->behavior).dumpAssembly.dumpFileAssembly != NULL);
		IRMETHOD_lock(method);
		jit_dump_function((_this->behavior).dumpAssembly.dumpFileAssembly, internalFunction->function, (char *) IRMETHOD_getCompleteMethodName(method));
		IRMETHOD_unlock(method);
	}

	/* Return				*/
	return;
}

void IRVM_translateMethodToMachineCode (IRVM_t *_this, ir_method_t *method, t_jit_function *jitFunction){
	ir_instruction_t        *currentIRInstruction;
	XanList                 *labels;
	XanListItem             *item;
	char buf[1024];
	JITUINT32 count;
	JITINT32 libjitSuccess;
	t_jit_function_internal *internalFunction;

	/* Assertions			*/
	assert(method != NULL);
	assert(_this != NULL);
	assert(jitFunction != NULL);

	/* Init the variables		*/
	labels = NULL;
	count = 0;

	/* Cache some pointers			*/
	internalFunction = jitFunction->data;
	assert(internalFunction != NULL);

	/* Lock the method		*/
	IRMETHOD_lock(method);

	/* Lock the VM			*/
	internal_lockVM(_this);

	/* Check the locals             */
	if (internalFunction->locals != NULL) {
		freeMemory(internalFunction->locals);
		internalFunction->locals = NULL;
	}
	assert(internalFunction->locals == NULL);

	/* Allocate the list of labels	*/
	labels = xanList_new(allocFunction, freeFunction, NULL);
	assert(labels != NULL);

	/* Lock the Libjit library	*/
	internal_lockLibjit(_this->data);

	/* Translate the feedback code	*/
	//translate_enter_method_feedback_code(system, method);

	/* Update the number of         *
	 * variables needed by the	*
	 * method			*/
	IRMETHOD_updateNumberOfVariablesNeededByMethod(method);

	/* Fetch the first instruction	*/
	PDEBUG("IRVIRTUALMACHINE: Translate the method %s into the machine language\n", IRMETHOD_getCompleteMethodName(method));
	currentIRInstruction = IRMETHOD_getFirstInstruction(method);

	/* Create the locals		*/
	assert(IRMETHOD_getNumberOfVariablesNeededByMethod(method) > (IRMETHOD_getMethodLocalsNumber(method) + IRMETHOD_getMethodParametersNumber(method)));
	PDEBUG("IRVIRTUALMACHINE:               Make %d variables\n", IRMETHOD_getNumberOfVariablesNeededByMethod(method));
	PDEBUG("IRVIRTUALMACHINE:                       %d locals\n", method->getLocalsNumber(method));
	PDEBUG("IRVIRTUALMACHINE:                       %d parameters\n", IRMETHOD_getMethodParametersNumber(method));
	if (IRMETHOD_getNumberOfVariablesNeededByMethod(method) > 0) {
		internalFunction->locals = (jit_value_t *) allocMemory(sizeof(jit_value_t) * (IRMETHOD_getNumberOfVariablesNeededByMethod(method) + 1));
		assert(internalFunction->locals != NULL);
		memset(internalFunction->locals, 0, sizeof(jit_value_t) * (IRMETHOD_getNumberOfVariablesNeededByMethod(method)));
	}

	/* Create the variables		*/
	PDEBUG("IRVIRTUALMACHINE:               Create %d locals\n", method->getLocalsNumber(method));
	item = xanList_first(method->locals);
	while (item != NULL) {
		jit_type_t JITvalueType;
		#ifdef DEBUG
		jit_type_t type;
		#endif
		ir_local_t *param = (ir_local_t *) item->data;
		assert(param != NULL);
		count = param->var_number;
		switch (param->internal_type) {
		    case IRINT64:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as INT64\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_long);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_long);
				#endif
			    break;
		    case IRINT32:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as INT32\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_int);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_int);
				#endif
			    break;
		    case IRINT16:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as INT16\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_short);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_short);
				#endif
			    break;
		    case IRINT8:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as INT8\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_sbyte);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_sbyte);
				#endif
			    break;
		    case IRNINT:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as NINT\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_nint);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_nint);
				#endif
			    break;
		    case IRUINT64:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as UINT64\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_ulong);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_ulong);
				#endif
			    break;
		    case IRUINT32:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as UINT32\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_uint);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_uint);
				#endif
			    break;
		    case IRUINT16:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as UINT16\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_ushort);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_ushort);
				#endif
			    break;
		    case IRUINT8:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as UINT8\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_ubyte);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_ubyte);
				#endif
			    break;
		    case IRNUINT:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as NUINT\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_nuint);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_nuint);
				#endif
			    break;
		    case IRFLOAT32:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as FLOAT32\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_float32);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_float32);
				#endif
			    break;
		    case IRFLOAT64:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as FLOAT64\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_float64);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_float64);
				#endif
			    break;
		    case IRNFLOAT:
			    if (sizeof(JITNUINT) == 4) {
				    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as FLOAT32\n", count);
				    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_float32);
						#ifdef DEBUG
				    type = jit_value_get_type(internalFunction->locals[count]);
				    assert(type == jit_type_float32);
						#endif
			    } else {
				    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as FLOAT64\n", count);
				    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_float64);
						#ifdef DEBUG
				    type = jit_value_get_type(internalFunction->locals[count]);
				    assert(type == jit_type_float64);
						#endif
			    }
			    break;
		    case IROBJECT:
		    case IRMPOINTER:
		    case IRTPOINTER:
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as POINTER\n", count);
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, jit_type_void_ptr);
				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == jit_type_void_ptr);
				#endif
			    break;
		    case IRVALUETYPE:
			    assert(method->locals != NULL);
			    PDEBUG("IRVIRTUALMACHINE:                       Make local %d as VALUETYPE\n", count);

			    /* retrieve the JIT type associated with the valuetype */
			    JITvalueType = (jit_type_t) (_this->lookupValueType(param->type_info))->type;
			    assert(JITvalueType != NULL);

			    /* create the JIT variable for the current valuetype */
			    internalFunction->locals[count] = jit_value_create(internalFunction->function, JITvalueType);
			    assert(internalFunction->locals[count] != NULL);

				#ifdef DEBUG
			    type = jit_value_get_type(internalFunction->locals[count]);
			    assert(type == JITvalueType);
				#endif
			    break;
		    case NOPARAM:
			    PDEBUG("%d \n", (*param));
			    snprintf(buf, sizeof(buf), "IRVIRTUALMACHINE: ERROR = Local variable %u has no type information. ", count - IRMETHOD_getMethodParametersNumber(method));
			    print_err(buf, 0);
			    abort();
		    default:
			    PDEBUG("%d \n", (*param));
			    snprintf(buf, sizeof(buf), "IRVIRTUALMACHINE: ERROR = Type of local variable %u is not known. ", count - IRMETHOD_getMethodParametersNumber(method));
			    print_err(buf, 0);
			    abort();
		}
		assert((internalFunction->locals[count]) != NULL);
		item = item->next;
	}

	/* Fetch the parameters		*/
	fetch_parameters(method, internalFunction);

	/* Fetch temporaries		*/
	fetch_temporaries(_this, method, internalFunction);

	/* Set the addressable variables*/
	internal_set_variables_addressable(_this, method, internalFunction);

	/* Insert the native start call */
	#ifdef MORPHEUS
	internal_insert_native_start_function(method);
	#endif

	/* Translate the IR method to	*
	 * the libjit method.		*
	 * Declare whether we have the catcher block		*/
	if (IRMETHOD_getCatcherInstruction(method) != NULL){
		jit_insn_uses_catcher(internalFunction->function);
	}
	count = 0;
	assert(method->exitNode != NULL);
	if (currentIRInstruction != NULL) {
		while (currentIRInstruction != method->exitNode) {
			assert(currentIRInstruction != NULL);
			PDEBUG("IRVIRTUALMACHINE: Instruction %d\n", count);

			/* Bind the offset information to the jit       *
			* instruction                                  */
			count++;
			bind_instruction_offset(_this, method, currentIRInstruction, internalFunction);

			/* Set the variables as volatile if it is needed	*/
			set_volatile_variables(_this, method, currentIRInstruction, internalFunction);

			/* Convert the current IR instruction	*/
			switch (currentIRInstruction->type) {
			    case IRSIZEOF:
				    translate_ir_sizeof(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRADD:
				    translate_ir_add(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRADDOVF:
				    translate_ir_add_ovf(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRAND:
				    translate_ir_and(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRARRAYLENGTH:
				    translate_ir_array_length(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRBRANCH:
				    translate_ir_branch(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRBRANCHIF:
				    translate_ir_branchif(_this, method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRBRANCHIFNOT:
				    translate_ir_branchifnot(_this, method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRCALL:
				    translate_ir_call(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRCHECKNULL:
				    translate_ir_checknull(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRCONV:
				    translate_ir_conv(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRCONVOVF:
				    translate_ir_conv_ovf(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRDIV:
				    translate_ir_div(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IREQ:
				    translate_ir_eq(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRGETADDRESS:
				    translate_ir_get_address(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRGT:
				    translate_ir_gt(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRLABEL:
				    translate_ir_label(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRLOADELEM:
				    translate_ir_load_elem(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRLOADREL:
				    translate_ir_load_rel(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRLT:
				    translate_ir_lt(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRNOP:
				    count--;
				    break;
			    case IRNOT:
				    translate_ir_not(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRLIBRARYCALL:
				    translate_ir_librarycall(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRNATIVECALL:
				    translate_ir_nativecall(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRNEG:
				    translate_ir_neg(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRNEWARR:
				    translate_ir_newarr(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRNEWOBJ:
				    translate_ir_newobj(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IREXIT:
				    translate_ir_exit(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRFREEOBJ:
				    translate_ir_freeobj(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRMUL:
				    translate_ir_mul(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRMULOVF:
				    translate_ir_mul_ovf(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IROR:
				    translate_ir_or(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRREM:
				    translate_ir_rem(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRRET:
				    translate_ir_ret(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSHL:
				    translate_ir_shl(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSHR:
				    translate_ir_shr(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSUB:
				    translate_ir_sub(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSUBOVF:
				    translate_ir_sub_ovf(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSTORE:
				    translate_ir_store(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSTOREELEM:
				    translate_ir_store_elem(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSTOREREL:
				    translate_ir_store_rel(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRVCALL:
				    translate_ir_vcall(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRXOR:
				    translate_ir_xor(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRCALLFINALLY:
				    translate_ir_call_finally(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRTHROW:
				    translate_ir_throw(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSTARTFILTER:
				    translate_ir_start_filter(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRENDFILTER:
				    translate_ir_end_filter(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSTARTFINALLY:
				    translate_ir_start_finally(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRENDFINALLY:
				    translate_ir_end_finally(method, currentIRInstruction, internalFunction);
				    break;
			    case IRSTARTCATCHER:
				    translate_ir_start_catcher(method, currentIRInstruction, internalFunction);
				    break;
			    case IRBRANCHIFPCNOTINRANGE:
				    translate_ir_branch_if_pc_not_in_range(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRCALLFILTER:
				    translate_ir_call_filter(method, currentIRInstruction, labels, internalFunction);
				    break;
			    case IRISNAN:
				    translate_ir_isNaN(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRISINF:
				    translate_ir_isInf(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRICALL:
				    translate_ir_icall(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRINITMEMORY:
				    translate_ir_initmemory(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRMEMCPY:
				    translate_ir_memory_copy(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRALLOCA:
				    translate_ir_alloca(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRALLOC:
				    translate_ir_alloc(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRREALLOC:
				    translate_ir_realloc(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRFREE:
				    translate_ir_free(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRALLOCALIGN:
				    translate_ir_alloc_align(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRFLOOR:
				    translate_ir_floor(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSQRT:
				    translate_ir_sqrt(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRSIN:
				    translate_ir_sin(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRPOW:
				    translate_ir_pow(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRCOS:
				    translate_ir_cos(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRCOSH:
				    translate_ir_cosh(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IRACOS:
				    translate_ir_acos(_this, method, currentIRInstruction, internalFunction);
				    break;
			    case IREXITNODE:
				    break;
			    default:
				    snprintf(buf, sizeof(buf), "IRVIRTUALMACHINE: ERROR= IR instruction %d is not knwon\n", currentIRInstruction->type);
				    print_err(buf, 0);
				    abort();
			}
			PDEBUG("IRVIRTUALMACHINE:                       Translated\n");

			/* Fetch the next item		*/
			assert(currentIRInstruction != NULL);
			currentIRInstruction = IRMETHOD_getNextInstruction(method, currentIRInstruction);
		}
	}

	/* Check the labels		*/
	#ifdef PRINTDEBUG
	check_labels(labels);
	#endif

	/* Refresh the root set		*/
	variablesToRootSets(_this, method, internalFunction);

	/* Dump the assembly		*/
	if ((_this->behavior).dumpAssembly.dumpAssembly) {
		assert((_this->behavior).dumpAssembly.dumpFileJIT != NULL);
		jit_dump_function((_this->behavior).dumpAssembly.dumpFileJIT, internalFunction->function, (char *) IRMETHOD_getMethodName(method));
		fflush((_this->behavior).dumpAssembly.dumpFileJIT);
	}

	/* Compile the function		*/
	libjitSuccess = JITFALSE;
	PDEBUG("IRVIRTUALMACHINE:		Compile the method\n");
	if (count > 0) {
		libjitSuccess = jit_function_compile_entry(internalFunction->function, &(internalFunction->entryPoint));
		if (!libjitSuccess) {
			IROPTIMIZER_callMethodOptimization(&(_this->optimizer), method, METHOD_PRINTER);
			print_err("IRVIRTUALMACHINE: ERROR during the compilation of the following function: ", 0);
			snprintf(buf, DIM_BUF, "IRVIRTUALMACHINE:       %s ", IRMETHOD_getSignatureInString(method));
			print_err(buf, 0);
			abort();
		}
		assert(internalFunction->entryPoint != NULL);
	}

	/* Unlock the Libjit library			*/
	internal_unlockLibjit(_this->data);

	/* Free the memory		*/
	xanList_destroyListAndData(labels);

	/* Unlock the VM				*/
	internal_unlockVM(_this);

	/* Unlock the method				*/
	IRMETHOD_unlock(method);

	/* Return					*/
	PDEBUG("IRVIRTUALMACHINE:		Exit from the build phase\n");
	return;
}

static inline jit_type_t internal_fromIRTypeToJITType (IRVM_t *_this, JITUINT32 IRType, TypeDescriptor *class){
	jit_type_t type;
	char buf[1024];

	/* Assertions			*/
	assert(_this != NULL);

	/* initialize the local variable */
	type = NULL;

	/* Make the type conversion	*/
	switch (IRType) {
	    case IRINT8:
		    type = jit_type_sbyte;
		    break;
	    case IRINT16:
		    type = jit_type_short;
		    break;
	    case IRINT32:
		    type = jit_type_int;
		    break;
	    case IRINT64:
		    type = jit_type_long;
		    break;
	    case IRNINT:
	    case IRSTRING:
	    case IRFPOINTER:
		    type = jit_type_nint;
		    break;
	    case IRUINT8:
		    type = jit_type_ubyte;
		    break;
	    case IRUINT16:
		    type = jit_type_ushort;
		    break;
	    case IRUINT32:
		    type = jit_type_uint;
		    break;
	    case IRUINT64:
		    type = jit_type_ulong;
		    break;
	    case IRNUINT:
		    type = jit_type_nuint;
		    break;
	    case IRFLOAT32:
		    type = jit_type_float32;
		    break;
	    case IRFLOAT64:
		    type = jit_type_float64;
		    break;
	    case IRNFLOAT:
		    type = jit_type_nfloat;
		    break;
	    case IRMPOINTER:
	    case IRTPOINTER:
	    case IRCLASSID:
	    case IRMETHODID:
	    case IRMETHODENTRYPOINT:
	    case IROBJECT:
		    type = jit_type_void_ptr;
		    break;
	    case IRVALUETYPE:
		    assert(class != NULL);
		    type = (jit_type_t) (_this->lookupValueType(class))->type;
		    break;
	    case IRVOID:
		    type = jit_type_void;
		    break;
	    default:
		    snprintf(buf, sizeof(char) * 1024, "IRVIRTUALMACHINE: fromIRTypeToJitType: ERROR = IR type %u is not known. ", IRType);
		    print_err(buf, 0);
		    abort();
	}

	/* Return		*/
	return type;
}

static inline jit_value_t internal_create_value (IRVM_t *_this, JITUINT32 irType, TypeDescriptor *type_infos, ir_value_t value, t_jit_function_internal *jitFunction){
	jit_value_t param;
	char buf[1024];
	jit_type_t JITvalueType;

	/* Init the variables	*/
	param = NULL;

	switch (irType) {
	    case IRINT8:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_sbyte, (JITINT32) value.v);
		    break;
	    case IRINT16:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_short, (JITINT32) value.v);
		    break;
	    case IRINT32:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_int, (JITINT32) value.v);
		    break;
	    case IRINT64:
		    param = jit_value_create_long_constant(jitFunction->function, jit_type_long, (JITINT64) value.v);
		    break;
	    case IRFPOINTER:
	    case IRSTRING:
	    case IRNINT:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_nint, (JITINT32) value.v);
		    break;
	    case IRUINT8:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_ubyte, (JITINT32) value.v);
		    break;
	    case IRUINT16:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_ushort, (JITINT32) value.v);
		    break;
	    case IRUINT32:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_uint, (JITUINT32) value.v);
		    break;
	    case IRUINT64:
		    param = jit_value_create_long_constant(jitFunction->function, jit_type_ulong, (JITUINT64) value.v);
		    break;
	    case IRNUINT:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_nint, (JITINT32) value.v);
		    break;
	    case IRFLOAT32:
		    param = jit_value_create_float32_constant(jitFunction->function, jit_type_float32, (JITFLOAT32) value.f);
		    break;
	    case IRFLOAT64:
		    param = jit_value_create_float64_constant(jitFunction->function, jit_type_float64, (JITFLOAT64) value.f);
		    break;
	    case IRNFLOAT:
		    if (sizeof(JITNUINT) == 4) {
			    param = jit_value_create_float32_constant(jitFunction->function, jit_type_float32, (JITFLOAT32) value.f);
		    } else {
			    param = jit_value_create_float64_constant(jitFunction->function, jit_type_float64, (JITFLOAT64) value.f);
		    }
		    break;
	    case IRMPOINTER:
	    case IRTPOINTER:
	    case IROBJECT:
	    case IRCLASSID:
	    case IRMETHODID:
	    case IRMETHODENTRYPOINT:
		    param = jit_value_create_nint_constant(jitFunction->function, jit_type_void_ptr, (JITNUINT) value.v);
		    break;
	    case IRVALUETYPE:

		    /* retrieve the JIT type associated with the valuetype */
		    assert(type_infos != NULL);
		    JITvalueType = (jit_type_t) (_this->lookupValueType(type_infos))->type;
		    assert(JITvalueType != NULL);

		    /* create the JIT variable for the current valuetype */
		    param = jit_value_create(jitFunction->function, JITvalueType);
		    break;
	    default:
		    snprintf(buf, sizeof(char) * 1024, "IRVIRTUALMACHINE: internal_create_variable: ERROR= Type %d is not known. ", irType);
		    print_err(buf, 0);
		    abort();
	}

	return param;
}

static inline jit_value_t make_variable (IRVM_t *_this, ir_method_t *method, ir_item_t *item, t_jit_function_internal *jitFunction){
	jit_value_t param;
	jit_value_t var;
	char buf[1024];
	TypeDescriptor  *iltype;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(item != NULL);
	assert(jitFunction != NULL);

	/* Init the variables	*/
	param = NULL;
	iltype = NULL;

	switch (item->type) {
	    case IROFFSET:
		    assert((item->value).v < IRMETHOD_getNumberOfVariablesNeededByMethod(method));
		    if (jitFunction->locals[(item->value).v] == NULL) {
			    jitFunction->locals[(item->value).v] = jit_value_create(jitFunction->function, internal_fromIRTypeToJITType(_this, item->internal_type, item->type_infos));
		    }
		    assert(jitFunction->locals[(item->value).v] != NULL);
		    param = jitFunction->locals[(item->value).v];
		    return param;
	    case IRSYMBOL:
		    iltype = item->type_infos;
		    internal_unlockLibjit(_this->data);
		    internal_unlockVM(_this);
		    param = internal_create_value(_this, item->internal_type, item->type_infos, IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) ((item->value).v)), jitFunction);
		    internal_lockVM(_this);
		    internal_lockLibjit(_this->data);
		    break;
	    case IRINT8:
	    case IRINT16:
	    case IRINT32:
	    case IRINT64:
	    case IRFPOINTER:
	    case IRSTRING:
	    case IRNINT:
	    case IRUINT8:
	    case IRUINT16:
	    case IRUINT32:
	    case IRUINT64:
	    case IRNUINT:
	    case IRFLOAT32:
	    case IRFLOAT64:
	    case IRNFLOAT:
	    case IRMPOINTER:
	    case IRTPOINTER:
	    case IROBJECT:
	    case IRCLASSID:
	    case IRMETHODID:
	    case IRMETHODENTRYPOINT:
	    case IRVALUETYPE:
		    iltype = item->type_infos;
		    param = internal_create_value(_this, item->internal_type, item->type_infos, item->value, jitFunction);
		    break;
	    default:
		    snprintf(buf, sizeof(char) * 1024, "IRVIRTUALMACHINE: make_variable: ERROR= Type %d is not known. ", item->type);
		    print_err(buf, 0);
		    abort();
	}
	assert(param != NULL);

	/* Make the variable		*/
	var = jit_value_create(jitFunction->function, internal_fromIRTypeToJITType(_this, item->internal_type, iltype));
	assert(var != NULL);
	jit_insn_store(jitFunction->function, var, param);

	return var;
}

static inline JITINT32 check_labels (XanList *labels){
	XanListItem     *item;
	t_jit_label     *label;

	PDEBUG("IRVIRTUALMACHINE:               Check the %d labels\n", xanList_length(labels));
	if (labels == NULL) {
		PDEBUG("IRVIRTUALMACHINE:                       There isn't label\n");
		return 0;
	}

	/* Check if the label is in the list	*/
	item = xanList_first(labels);
	while (item != NULL) {
		label = (t_jit_label *) item->data;
		assert(label != NULL);
		assert(label->fixed);
		assert(label->label != jit_label_undefined);
		item = item->next;
	}

	/* Return				*/
	PDEBUG("IRVIRTUALMACHINE:                       Labels OK\n");
	return 0;
}

static inline t_jit_label * insert_label (XanList *labels, JITUINT32 label_ID){
	XanListItem     *item;
	t_jit_label     *label;

	/* Init the variables			*/
	PDEBUG("IRVIRTUALMACHINE: INSERT_LABEL: %d Labels present\n", xanList_length(labels));
	item = NULL;
	label = NULL;

	/* Check if the label is in the list	*/
	item = xanList_first(labels);
	while (item != NULL) {
		label = (t_jit_label *) item->data;
		assert(label != NULL);
		if (label->ID == label_ID) {
			PDEBUG("IRVIRTUALMACHINE: INSERT_LABEL:                 Found the target label\n");
			return label;
		}
		item = item->next;
	}

	/* The label is not in the list		*/
	PDEBUG("IRVIRTUALMACHINE: INSERT_LABEL:         The target label is not found\n");
	PDEBUG("IRVIRTUALMACHINE: INSERT_LABEL:         Insert the new label\n");
	label = (t_jit_label *) allocMemory(sizeof(t_jit_label));
	label->ID = label_ID;
	label->label = jit_label_undefined;
	label->fixed = JITFALSE;
	xanList_append(labels, label);

	return label;
}

static inline JITINT16 translate_ir_neg (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param;

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(jitFunction != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = neg(", (inst->result).value.v);

	/* Create the first parameter	*/
	param = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param != NULL);
	PDEBUG(")\n");

	/* Create the neg instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_neg(jitFunction->function, param);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_neg(jitFunction->function, param);
	}

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_store_elem (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t field_number;
	jit_value_t value;
	jit_value_t array;
	jit_type_t type;

	#ifdef PRINTDEBUG
	jit_value_t args[2];
	#endif

	/* Assertions					*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(jitFunction != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IROFFSET);
	assert((inst->result).type == NOPARAM);

	/* Init the variables				*/
	value = NULL;
	type = NULL;
	field_number = NULL;

	/* Print the instruction			*/
	PDEBUG("IRVIRTUALMACHINE:               store_elem(Array in variable %lld, Element number in ", (inst->param_1).value.v);

	/* Fetch the array                              */
	array = make_variable(_this, method, &(inst->param_1), jitFunction);

	/* Create the field number			*/
	field_number = make_variable(_this, method, &(inst->param_2), jitFunction);
	assert(field_number != NULL);
	PDEBUG(", Value in ");

	/* Create the value to store			*/
	value = make_variable(_this, method, &(inst->param_3), jitFunction);
	assert(value != NULL);
	PDEBUG(", Type array element ");

	/* Fetch the type of the value to store		*/
	type = jit_value_get_type(value);
	assert(type != NULL);

	/* Create the Store_elem instruction	*/
	if (jit_insn_store_elem(jitFunction->function, array, field_number, value) == 0) {
		print_err("IRVIRTUALMACHINE: store_elem: ERROR = The jit_insn_store_elem return an error. ", 0);
		abort();
	}

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_load_elem (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	JITUINT32 typeSlot;
	char buffer[1024];
	jit_value_t field_number;
	jit_value_t array;
	jit_type_t arraySlotType;

	/* Assertions					*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(jitFunction != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	assert((inst->param_1).type == IROFFSET);
	assert((inst->param_3).type == IRTYPE);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = load_elem(Array = Variable %lld, Field = ", (inst->result).value.v, (inst->param_1).value.v);

	/* Initialize the variables			*/
	arraySlotType = NULL;
	field_number = NULL;
	array = NULL;
	typeSlot = 0;

	/* Fetch the array                              */
	array = make_variable(_this, method, &(inst->param_1), jitFunction);

	/* Create the field number			*/
	field_number = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(", IRTYPE = ");

	/* Compute the type of the single slot		*/
	typeSlot = (inst->param_3).value.v;

	/* Make the JIT type				*/
	switch (typeSlot) {
	    case IRINT8:
		    PDEBUG("IRINT8\n");
		    arraySlotType = jit_type_sbyte;
		    break;
	    case IRINT16:
		    arraySlotType = jit_type_short;
		    PDEBUG("IRINT16\n");
		    break;
	    case IRINT32:
		    PDEBUG("IRINT32\n");
		    arraySlotType = jit_type_int;
		    break;
	    case IRINT64:
		    PDEBUG("IRINT64\n");
		    arraySlotType = jit_type_long;
		    break;
	    case IRUINT8:
		    PDEBUG("IRUINT8\n");
		    arraySlotType = jit_type_ubyte;
		    break;
	    case IRUINT16:
		    PDEBUG("IRUINT16\n");
		    arraySlotType = jit_type_ushort;
		    break;
	    case IRUINT32:
		    PDEBUG("IRUINT32\n");
		    arraySlotType = jit_type_uint;
		    break;
	    case IRUINT64:
		    PDEBUG("IRUINT64\n");
		    arraySlotType = jit_type_ulong;
		    break;
	    case IRFLOAT32:
		    PDEBUG("IRFLOAT32\n");
		    arraySlotType = jit_type_float32;
		    break;
	    case IRFLOAT64:
		    PDEBUG("IRFLOAT64\n");
		    arraySlotType = jit_type_float64;
		    break;
	    case IRNINT:
		    PDEBUG("IRNINT\n");
		    arraySlotType = jit_type_nint;
		    break;
	    case IRNUINT:
		    PDEBUG("IRNUINT\n");
		    arraySlotType = jit_type_nuint;
		    break;
	    case IRNFLOAT:
		    if (sizeof(JITNUINT) == 4) {
			    PDEBUG("IRFLOAT32\n");
			    arraySlotType = jit_type_float32;
		    } else {
			    PDEBUG("IRFLOAT64\n");
			    arraySlotType = jit_type_float64;
		    }
		    break;
	    case IRMPOINTER:
		    PDEBUG("IRMPOINTER\n");
		    arraySlotType = jit_type_void_ptr;
		    break;
	    case IRTPOINTER:
		    PDEBUG("IRTPOINTER\n");
		    arraySlotType = jit_type_void_ptr;
		    break;
	    case IROBJECT:
		    PDEBUG("IROBJECT\n");
		    arraySlotType = jit_type_void_ptr;
		    break;
	    default:
		    snprintf(buffer, sizeof(buffer), "IRVIRTUALMACHINE: load_elem: ERROR = Type %d is not known. ", typeSlot);
		    print_err(buffer, 0);
		    abort();
	}

	/* Make the JIT instruction	*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_load_elem(jitFunction->function, array, field_number, arraySlotType);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_load_elem(jitFunction->function, array, field_number, arraySlotType);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_exit (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t args[1];
	IRVM_t_internal *internalVM;

	/* Assertions					*/
	assert(_this != NULL);
	assert(inst != NULL);

	/* Cache some pointers				*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Create the arguments				*/
	args[0] = make_variable(_this, method, &(inst->param_1), jitFunction);

	/* Call the GC					*/
	jit_insn_call_native(jitFunction->function, "exit", _this->leaveExecution, internalVM->leaveExecutionSignature, args, 1, JIT_CALL_NOTHROW);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_freeobj (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t args[1];
	IRVM_t_internal *internalVM;

	/* Assertions		*/
	assert(_this != NULL);
	assert(inst != NULL);

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Create the arguments				*/
	args[0] = make_variable(_this, method, &(inst->param_1), jitFunction);

	/* Call the GC					*/
	jit_insn_call_native(jitFunction->function, "freeObject", _this->gc->freeObject, internalVM->signFreeObject, args, 1, JIT_CALL_NOTHROW);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_newobj (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t args[2];
	TypeDescriptor  *class;
	IRVM_t_internal *internalVM;

	/* Assertions		*/
	assert(_this != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRCLASSID || ((inst->param_1).type == IRSYMBOL && (inst->param_1).internal_type == IRCLASSID));
	assert((inst->param_2).type == IRUINT32);
	assert((inst->param_2).internal_type == IRUINT32);

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Fetch the class	*/
	if ((inst->param_1).type == IRSYMBOL) {
		internal_unlockLibjit(_this->data);
		internal_unlockVM(_this);
		class = (TypeDescriptor *) (JITNUINT) IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) ((inst->param_1).value.v)).v;
		internal_lockVM(_this);
		internal_lockLibjit(_this->data);
	}else{
		class = (TypeDescriptor *) (JITNUINT) (inst->param_1).value.v;
	}
	assert(class != NULL);

	#ifdef PRINTDEBUG
	JITINT8 *name = class->getCompleteName(class);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = newobj(%s)\n" , (inst->result).value.v, name);
	PDEBUG("IRVIRTUALMACHINE:                       Call the allocObject function of the garbage Collector\n");
	#endif

	/* Create the arguments				*/
	args[0] = jit_value_create_nint_constant(jitFunction->function, jit_type_void_ptr, (JITNUINT) class);
	args[1] = make_variable(_this, method, &(inst->param_2), jitFunction);

	/* Call the Allocator				*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_call_native(jitFunction->function, "allocObject", _this->gc->allocObject, internalVM->signAllocObject, args, 2, JIT_CALL_NOTHROW);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_call_native(jitFunction->function, "allocObject", _this->gc->allocObject, internalVM->signAllocObject, args, 2, JIT_CALL_NOTHROW);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_label (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction) {
	t_jit_label             *label;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRLABELITEM);

	/* Fetch the label	*/
	PDEBUG("IRVIRTUALMACHINE:               Label \"L%lld\"\n", (inst->param_1).value.v);
	PDEBUG("IRVIRTUALMACHINE:			%d Labels present\n", xanList_length(labels));
	PDEBUG("IRVIRTUALMACHINE:			Insert a label\n");
	label = insert_label(labels, (inst->param_1).value.v);
	assert(label != NULL);
	assert(!(label->fixed));

	/* Insert the label	*/
	jit_insn_label(jitFunction->function, &(label->label));
	label->fixed = JITTRUE;
	PDEBUG("IRVIRTUALMACHINE:			%d Labels present\n", xanList_length(labels));

	/* Return		*/
	return 0;
}

static inline JITINT16 translate_ir_xor (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = xor(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the xor instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_xor(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_xor(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_array_length (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_type_t type;
	JITINT32 offset;
	jit_value_t object;
	jit_value_t temp;

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(jitFunction != NULL);
	assert((inst->result).type == IROFFSET);

	/* Init the variables		*/
	type = NULL;

	/* Make the parameters of the instruction	*/
	object = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(object != NULL);

	offset = _this->gc->getArrayLengthOffset();
	type = internal_fromIRTypeToJITType(_this, IRINT32, NULL);

	/* Translate the instruction			*/
	temp = jit_insn_load_relative(jitFunction->function, object, offset, type);
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	return 0;
}

static inline JITINT16 translate_ir_and (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = and(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the and instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_and(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_and(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_or (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = or(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the aor instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_or(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_or(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_mul (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = mul(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the lt instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_mul(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_mul(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_div (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = div(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the div instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_div(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_div(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_sub (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = sub(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the sub instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_sub(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_sub(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_sizeof (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	ir_instruction_t	localInst;
	ir_item_t		*param1;

	/* Fetch the parameter			*/
	param1		= IRMETHOD_getInstructionParameter1(inst);

	/* Create the move instruction		*/
	memset(&localInst, 0, sizeof(ir_instruction_t));
	localInst.type	= IRSTORE;
	memcpy(&(localInst.param_1), &(inst->result), sizeof(ir_item_t));
	localInst.param_2.type		= localInst.param_1.internal_type;
	localInst.param_2.internal_type	= localInst.param_2.type;
	switch ((param1->value).v){
		case IROBJECT:
		case IRVALUETYPE:
			localInst.param_2.value.v	= IRMETHOD_getSizeOfObject(param1);
			break;
		default :
			localInst.param_2.value.v	= IRMETHOD_getSizeOfType(param1);
	}

	return translate_ir_store(_this, method, &localInst, jitFunction);
}

static inline JITINT16 translate_ir_add (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = add(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the add instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_add(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_add(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_branch (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction){
	t_jit_label     *label;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(labels != NULL);

	PDEBUG("IRVIRTUALMACHINE:               branch(Label \"L%lld\")\n", (inst->param_1).value.v);
	assert((inst->param_1).type == IRLABELITEM);
	label = insert_label(labels, (inst->param_1).value.v);
	assert(label != NULL);
	jit_insn_branch(jitFunction->function, &(label->label));

	return 0;
}

static inline JITINT16 translate_ir_branch_if_pc_not_in_range (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction){
	t_jit_label     *label_from;
	t_jit_label     *label_to;
	t_jit_label     *label_target;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRLABELITEM);
	assert((inst->param_2).type == IRLABELITEM);
	assert((inst->param_3).type == IRLABELITEM);
	PDEBUG("IRVIRTUALMACHINE:               branch_if_pc_not_in_range(Label_from \"L%lld\" , Label_to \"L%lld\" , Label_target \"L%lld\")\n", (inst->param_1).value.v, (inst->param_2).value.v,     (inst->param_3).value.v);

	label_from = insert_label(labels, (inst->param_1).value.v);
	assert(label_from != NULL);
	label_to = insert_label(labels, (inst->param_2).value.v);
	assert(label_to != NULL);
	label_target = insert_label(labels, (inst->param_3).value.v);
	assert(label_target != NULL);

	jit_insn_branch_if_pc_not_in_range(jitFunction->function, (label_from->label), (label_to->label), &(label_target->label));

	return 0;
}

static inline JITINT16 translate_ir_start_filter (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction) {
	t_jit_label     *label;

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	assert((inst->param_1).type == IRLABELITEM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);
	PDEBUG("IRVIRTUALMACHINE:               start_filter (Label \"L%lld\")\n", (inst->param_1).value.v);

	/* Fetch the label		*/
	label = insert_label(labels, (inst->param_1).value.v);
	assert(label != NULL);
	assert(!(label->fixed));

	/* Translate the instruction	*/
	jitFunction->locals[(inst->result).value.v] = jit_insn_start_filter(jitFunction->function, &(label->label), jit_type_void_ptr);
	label->fixed = JITTRUE;

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_start_finally (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction) {
	t_jit_label     *label;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRLABELITEM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);
	assert((inst->result).type == NOPARAM);
	PDEBUG("IRVIRTUALMACHINE:               start_finally (Label \"L%lld\")\n", (inst->param_1).value.v);

	/* Fetch the label	*/
	label = insert_label(labels, (inst->param_1).value.v);
	assert(label != NULL);
	assert(!(label->fixed));

	/* Translate the start	*
	* finally instruction	*/
	jit_insn_start_finally(jitFunction->function, &(label->label));
	label->fixed = JITTRUE;

	/* Return		*/
	return 0;
}

static inline JITINT16 translate_ir_end_finally (ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(((inst->param_1).type == NOPARAM) || ((inst->param_1).type == IRLABELITEM));
	assert((inst->param_3).type == NOPARAM);
	assert((inst->result).type == NOPARAM);
	PDEBUG("IRVIRTUALMACHINE:               end_finally \n");

	/* Translate the instruction	*/
	jit_insn_return_from_finally(jitFunction->function);

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_start_catcher (ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == NOPARAM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);

	/* create the jit-instruction that marks the start of the catcher */
	PDEBUG("IRVIRTUALMACHINE:               start_catcher : jit_insn_start_catcher \n");
	jit_insn_start_catcher(jitFunction->function);

	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_thrown_exception(jitFunction->function);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_thrown_exception(jitFunction->function);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	return 0;
}

static inline JITINT16 translate_ir_end_filter (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type != NOPARAM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->result).type == NOPARAM);
	PDEBUG("IRVIRTUALMACHINE:               end_filter \n");

	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);

	jit_insn_return_from_filter(jitFunction->function, param_1);

	return 0;
}

static inline JITINT16 translate_ir_call_filter (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction) {
	t_jit_label     *label;
	jit_value_t filter_parameter;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	assert((inst->param_1).type == IRLABELITEM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);
	assert((inst->result).type == NOPARAM);

	PDEBUG("IRVIRTUALMACHINE:               call_filter(Label \"L%lld\")\n", (inst->param_1).value.v);
	label = insert_label(labels, (inst->param_1).value.v);
	assert(label != NULL);
	filter_parameter = jit_value_create_nint_constant(jitFunction->function, jit_type_void_ptr, (JITNUINT) 0);

	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_call_filter(jitFunction->function, &(label->label), filter_parameter, jit_type_int);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_call_filter(jitFunction->function, &(label->label), filter_parameter, jit_type_int);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_call_finally (ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction) {
	t_jit_label     *label;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRLABELITEM);
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);
	assert((inst->result).type == NOPARAM);

	PDEBUG("IRVIRTUALMACHINE:               call_finally(Label \"L%lld\")\n", (inst->param_1).value.v);
	label = insert_label(labels, (inst->param_1).value.v);
	assert(label != NULL);
	jit_insn_call_finally(jitFunction->function, &(label->label));

	return 0;
}

static inline JITINT16 translate_ir_throw (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;
	ir_instruction_t	*catchInst;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               rethrow \n");
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);

	catchInst	= IRMETHOD_getCatcherInstruction(method);
	if (	(catchInst != NULL)		&&
		(catchInst->ID < inst->ID)	){
		jit_insn_rethrow_unhandled(jitFunction->function);

	} else {
		jit_insn_throw(jitFunction->function, param_1);
	}

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_conv_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	return internal_translate_ir_conv(_this, method, inst, jitFunction, JITTRUE);
}

static inline JITINT16 translate_ir_conv (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	return internal_translate_ir_conv(_this, method, inst, jitFunction, JITFALSE);
}

static inline JITINT16 translate_ir_nativecall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_type_t signature;
	jit_type_t returnType;
	jit_type_t      *jit_param_types;
	jit_value_t     *jit_param_values;
	JITUINT32 param_size;
	JITUINT32 count;
	XanListItem     *current_parameter;
	void *function_pointer;

	/* Assertions			*/
	#ifdef DEBUG
	assert(method != NULL);
	assert(_this != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRTYPE);
	#endif

	if ((inst->param_3).type == IRSYMBOL) {
		internal_unlockLibjit(_this->data);
		internal_unlockVM(_this);
		function_pointer = (void *) (JITNUINT) IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) (inst->param_3).value.v).v;
		internal_lockVM(_this);
		internal_lockLibjit(_this->data);
	}else{
		function_pointer = (void *) (JITNUINT) (inst->param_3).value.v;
	}


	/* initialize the local variables */
	signature = NULL;
	returnType = NULL;
	jit_param_types = NULL;
	jit_param_values = NULL;
	current_parameter = NULL;
	param_size = 0;
	count = 0;

	/* retrieve the return_type */
	returnType = internal_fromIRTypeToJITType(_this, (inst->param_1).value.v, (inst->param_1).type_infos);
	assert(returnType != NULL);

	/* there are some parameters */
	if (    (inst->callParameters != NULL)                                  &&
		(xanList_length(inst->callParameters) > 0)        ) {
		param_size = xanList_length(inst->callParameters);
		assert(param_size != 0);
		current_parameter = xanList_first(inst->callParameters);
		assert(current_parameter != NULL);
		jit_param_types = (jit_type_t *) allocMemory(sizeof(jit_type_t) * param_size);
		jit_param_values = (jit_value_t *) allocMemory(sizeof(jit_value_t) * param_size);
	}

	/* create the type for each function parameter */
	for (count = 0; count < param_size; count++) {
		jit_type_t current_parameter_type;
		jit_value_t current_parameter_value;
		ir_item_t       *current_stack_item;

		/* retrieve the t_stack_item of the current parameter */
		assert(current_parameter != NULL);
		current_stack_item = (ir_item_t *) xanList_getData(current_parameter);
		assert(current_stack_item != NULL);

		/* retrieve the type & the value of the current parameter */
		current_parameter_type = internal_fromIRTypeToJITType(_this, current_stack_item->internal_type, current_stack_item->type_infos);
		current_parameter_value = make_variable(_this, method, current_stack_item, jitFunction);

		/* Update the values            */
		jit_param_types[count] = current_parameter_type;
		jit_param_values[count] = current_parameter_value;

		/* Fetch the next parameter     */
		current_parameter = current_parameter->next;
	}

	/* Create the signature                 */
	signature = jit_type_create_signature( jit_abi_cdecl, returnType, jit_param_types, param_size, 0);
	assert(signature != NULL);

	/* Translate the call instruction	*/
	if (    (returnType != jit_type_void)           &&
		((inst->result).type != NOPARAM)        ) {
		assert( ((JITINT8 *) ((JITNUINT) ((inst->param_2).value.v))) != NULL );
		if (jitFunction->locals[(inst->result).value.v] != NULL) {
			jit_value_t temp;
			temp = jit_insn_call_native(jitFunction->function, (char *) ((JITNUINT) ((inst->param_2).value.v)), function_pointer, signature, jit_param_values, param_size, 0);
			jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
		} else{
			jitFunction->locals[(inst->result).value.v] = jit_insn_call_native(jitFunction->function, (char *) ((JITNUINT) ((inst->param_2).value.v)), function_pointer, signature, jit_param_values, param_size, 0);
		}
		assert(jitFunction->locals[(inst->result).value.v] != NULL);
	} else{
		jit_insn_call_native(jitFunction->function, (char *) ((JITNUINT) ((inst->param_2).value.v)), function_pointer, signature, jit_param_values, param_size, 0);
	}

	/* Free the memory		*/
	jit_type_free(signature);
	if (jit_param_types != NULL) {
		freeMemory(jit_param_types);
	}
	if (jit_param_values != NULL) {
		freeMemory(jit_param_values);
	}

	/* Return                       */
	return 0;
}

static inline JITINT16 translate_ir_librarycall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	ir_method_t             *jumpMethod;
	t_jit_function          *jumpMethodJIT;
	XanListItem             *item;
	ir_item_t               *param;
	jit_value_t             *args;
	IR_ITEM_VALUE method_value;
	JITUINT32 args_number;
	JITUINT32 count;
	t_jit_function_internal *jumpMethodJITInternal;

	/* Assertions			*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRMETHODID || ((inst->param_1).type == IRSYMBOL && (inst->param_1).internal_type == IRMETHODID));
	assert(jitFunction->function != NULL);

	/* Init the variables		*/
	jumpMethod = NULL;
	item = NULL;
	param = NULL;
	args = NULL;

	if ((inst->param_1).type == IRSYMBOL) {
		internal_unlockLibjit(_this->data);
		internal_unlockVM(_this);
		method_value = IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) (inst->param_1).value.v).v;
		internal_lockVM(_this);
		internal_lockLibjit(_this->data);
	}else{
		method_value = (inst->param_1).value.v;
	}

	/* Search the jump method	*/
	jumpMethod = _this->getIRMethod(method_value);
	assert(jumpMethod != NULL);
	jumpMethodJIT = _this->getJITMethod(method_value);
	assert(jumpMethodJIT != NULL);
	jumpMethodJITInternal = (t_jit_function_internal *) jumpMethodJIT->data;
	assert(jumpMethodJITInternal != NULL);
	assert(jumpMethodJITInternal->signature != NULL);
	assert(jumpMethodJITInternal->nativeFunction != NULL);

#ifdef PRINTDEBUG
	switch ((inst->result).type) {
	    case IROFFSET:
		    PDEBUG("IRVIRTUALMACHINE: IR_NCALL:     Variable %lld = ncall(Method = %s Binary = %s: ", (inst->result).value.v, IRMETHOD_getMethodName(jumpMethod), inst->binary->name);
		    break;
	    case IRVOID:
		    PDEBUG("IRVIRTUALMACHINE: IR_NCALL: ncall(Method = %s Binary = %s: ", IRMETHOD_getMethodName(jumpMethod), inst->binary->name);
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: IR_NCALL: ERROR = Result type not known. ", 0);
		    abort();
	}
#endif

	/* Create the arguments			*/
	args_number = IRMETHOD_getMethodParametersNumber(jumpMethod);

	/* Alloc the JIT arguments.		*
	 * We allocate an array with an extra	*
	 * element for the calls that has a	*
	 * value type as return			*/
	args = (jit_value_t *) allocMemory(sizeof(jit_value_t) * (args_number + 1));

	/* Fetch the first formal parameter	*/
	item = xanList_first(inst->callParameters);

	/* Create all the formal arguments	*/
	count = 0;
	while (item != NULL) {
		param = (ir_item_t *) item->data;
		assert(param != NULL);

		/* Create the argument		*/
		args[count] = make_variable(_this, method, param, jitFunction);
		PDEBUG(", ");
		assert(args[count] != NULL);

		/* Fetch the next parameter	*/
		count++;
		item = item->next;
	}
	args_number = count;
	PDEBUG(")\n");

	/* Create the JIT call instruction	*/
	PDEBUG("IRVIRTUALMACHINE: IR_NCALL:	Insert the JIT call instruction\n");
	assert(jit_type_num_params(jumpMethodJITInternal->signature) == args_number);
	switch ((inst->result).type) {
	    case IROFFSET:
		    if (jitFunction->locals[(inst->result).value.v] != NULL) {
			    jit_value_t temp;
			    temp = jit_insn_call_native(jitFunction->function, (char *) IRMETHOD_getMethodName(jumpMethod), jumpMethodJITInternal->nativeFunction, jumpMethodJITInternal->signature, args, args_number, 0);
			    jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
		    } else{
			    jitFunction->locals[(inst->result).value.v] = jit_insn_call_native(jitFunction->function, (char *) IRMETHOD_getMethodName(jumpMethod), jumpMethodJITInternal->nativeFunction, jumpMethodJITInternal->signature, args, args_number, 0);
		    }
		    assert(jitFunction->locals[(inst->result).value.v] != NULL);
		    break;
	    case IRVOID:
		    jit_insn_call_native(jitFunction->function, (char *) IRMETHOD_getMethodName(jumpMethod), jumpMethodJITInternal->nativeFunction, jumpMethodJITInternal->signature, args, args_number, 0);
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: IR_NCALL: ERROR = The result type is not known. ", 0);
		    abort();
	}

	/* Free the memory			*/
	freeMemory(args);

	/* Return				*/
	PDEBUG("IRVIRTUALMACHINE: IR_NCALL: End\n");
	return 0;
}

static inline JITINT16 translate_ir_icall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_type_t signature;
	jit_type_t returnType;
	jit_type_t      *jit_param_types;
	jit_value_t     *jit_param_values;
	JITUINT32 param_size;
	JITUINT32 count;
	XanListItem     *current_parameter;
	jit_value_t function_pointer;

	/* Assertions			*/
	#ifdef DEBUG
	assert(method != NULL);
	assert(_this != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRTYPE);
	if ((inst->param_1).value.v == IRVOID) {
		assert((inst->result).type == NOPARAM);
	} else{
		assert((inst->result).type == IROFFSET);
	}
	#endif

	/* initialize the local variables */
	signature = NULL;
	returnType = NULL;
	jit_param_types = NULL;
	jit_param_values = NULL;
	current_parameter = NULL;
	param_size = 0;
	count = 0;

	/* retrieve the return_type */
	PDEBUG("IRVIRTUALMACHINE: translate_ir_icall:   Make the JIT signature\n");
	PDEBUG("IRVIRTUALMACHINE: translate_ir_icall:           Make the return type\n");
	returnType = internal_fromIRTypeToJITType(_this, (inst->param_1).value.v, (inst->param_1).type_infos);
	assert(returnType != NULL);

	/* there are some parameters */
	PDEBUG("IRVIRTUALMACHINE: translate_ir_icall:           Make the call parameters\n");
	if (    (inst->callParameters != NULL)                                  &&
		(xanList_length(inst->callParameters) > 0)        ) {
		param_size = xanList_length(inst->callParameters);
		assert(param_size != 0);
		current_parameter = xanList_first(inst->callParameters);
		assert(current_parameter != NULL);
		jit_param_types = (jit_type_t *) allocMemory(sizeof(jit_type_t) * param_size);
		jit_param_values = (jit_value_t *) allocMemory(sizeof(jit_value_t) * param_size);
	}

	/* create the type for each function parameter */
	for (count = 0; count < param_size; count++) {
		jit_type_t current_parameter_type;
		jit_value_t current_parameter_value;
		ir_item_t       *current_stack_item;
		PDEBUG("IRVIRTUALMACHINE: translate_ir_icall:                   Parameter %u: ", count);

		/* retrieve the t_stack_item of the current parameter */
		assert(current_parameter != NULL);
		current_stack_item = (ir_item_t *) xanList_getData(current_parameter);
		assert(current_stack_item != NULL);

		/* retrieve the type & the value of the current parameter */
		current_parameter_type = internal_fromIRTypeToJITType(_this, current_stack_item->internal_type, current_stack_item->type_infos);
		current_parameter_value = make_variable(_this, method, current_stack_item, jitFunction);
		PDEBUG("\n");

		/* Assertions						*/
#ifdef PRINTDEBUG
		jit_type_t type;
		type = jit_value_get_type(current_parameter_value);
		PDEBUG("IRVIRTUALMACHINE: translate_ir_icall:                           IR Type: %u\n", current_stack_item->internal_type);
		PDEBUG("IRVIRTUALMACHINE: translate_ir_icall:                           JIT Type: %u\n", fromJITTypeToIRType(type));
		assert(current_parameter_type != NULL);
		assert(current_parameter_value != NULL);
#endif

		/* Update the values            */
		jit_param_types[count] = current_parameter_type;
		jit_param_values[count] = current_parameter_value;

		/* Fetch the next parameter     */
		current_parameter = current_parameter->next;
	}

	/* create the signature */
	signature = jit_type_create_signature( jit_abi_cdecl, returnType, jit_param_types, param_size, 0);
	assert(signature != NULL);

	/* Fetch the function pointer	*/
	function_pointer = make_variable(_this, method, &(inst->param_2), jitFunction);
	assert(function_pointer != NULL);

	if ((_this->behavior).dla) {
		jit_insn_mark_offset(jitFunction->function, inst->ID);
	}

	/* Translate the call		*/
	if (returnType != jit_type_void) {
		if (jitFunction->locals[(inst->result).value.v] != NULL) {
			jit_value_t temp;
			temp = jit_insn_call_indirect(jitFunction->function, function_pointer, signature, jit_param_values, param_size, 0);
			jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
		} else{
			jitFunction->locals[(inst->result).value.v] = jit_insn_call_indirect(jitFunction->function, function_pointer, signature, jit_param_values, param_size, 0);
		}
		assert(jitFunction->locals[(inst->result).value.v] != NULL);
	} else{
		jit_insn_call_indirect(jitFunction->function, function_pointer, signature, jit_param_values, param_size, 0);
	}

	/* Free the signature		*/
	jit_type_free(signature);
	if (jit_param_types != NULL) {
		freeMemory(jit_param_types);
	}
	if (jit_param_values != NULL) {
		freeMemory(jit_param_values);
	}

	/* Return                       */
	return 0;
}

static inline JITINT16 translate_ir_call (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	ir_method_t             *ir_jump_method;
	t_jit_function          *jumpMethodJIT;
	XanListItem             *item;
	ir_item_t               *param;
	jit_value_t             *args;
	IR_ITEM_VALUE method_value;
	JITINT8 buf[DIM_BUF];
	JITUINT32 args_number;
	JITUINT32 count;
	JITINT32 call_flags;
	jit_type_t jit_signature;
	t_jit_function_internal *jumpMethodJITInternal;

	/* Assertions			*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRMETHODID || ((inst->param_1).type == IRSYMBOL && (inst->param_1).internal_type == IRMETHODID));
	assert((inst->param_2).type == IRINT32);
	assert(jitFunction->function != NULL);

	/* Init the variables		*/
	args = NULL;
	ir_jump_method = NULL;
	item = NULL;
	param = NULL;
	args_number = 0;
	count = 0;
	call_flags = 0;

	if ((inst->param_1).type == IRSYMBOL) {
		internal_unlockLibjit(_this->data);
		internal_unlockVM(_this);
		method_value = IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) (inst->param_1).value.v).v;
		internal_lockVM(_this);
		internal_lockLibjit(_this->data);
	}else{
		method_value = (inst->param_1).value.v;
	}

	/* Search the jump method	*/
	jumpMethodJIT = _this->getJITMethod(method_value);
	assert(jumpMethodJIT != NULL);
	jumpMethodJITInternal = (t_jit_function_internal *) jumpMethodJIT->data;
	assert(jumpMethodJITInternal != NULL);
	assert(jumpMethodJITInternal->signature != NULL);
	assert(jumpMethodJITInternal->function != NULL);

	/* Fetch the IR jump method	*/
	ir_jump_method = _this->getIRMethod(method_value);
	assert(ir_jump_method != NULL);
	assert(IRMETHOD_getSignature(ir_jump_method) != NULL);

	/* Fetch the jit signature	*/
	jit_signature = jumpMethodJITInternal->signature;
	assert(jit_signature != NULL);

	/* Print the instruction	*/
	#ifdef PRINTDEBUG
	switch ((inst->result).type) {
	    case IROFFSET:
		    PDEBUG("IRVIRTUALMACHINE: IR_CALL:		Variable %lld = call(Method = %s)\n", (inst->result).value.v, ir_jump_IRMETHOD_getMethodName(ir_jump_method));
		    break;
	    case IRVOID:
		    PDEBUG("IRVIRTUALMACHINE: IR_CALL:      call(Method = %s)\n", ir_jump_IRMETHOD_getMethodName(ir_jump_method));
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: IR_CALL: ERROR = Result type not known. ", 0);
		    abort();
	}
	#endif

	/* Create the arguments			*/
	PDEBUG("IRVIRTUALMACHINE: IR_CALL:			Set the arguments\n");
	if (    (inst->callParameters == NULL)                                  ||
		(xanList_length(inst->callParameters) == 0)       ) {

		/* The method hasn't some parameters	*/
		PDEBUG("IRVIRTUALMACHINE: IR_CALL:				No arguments\n");
		args = NULL;
		args_number = 0;
	} else {

		/* The method has some parameters	*/
		args_number = xanList_length(inst->callParameters);
		PDEBUG("IRVIRTUALMACHINE: IR_CALL:				%d arguments\n", args_number);
		args = (jit_value_t *) allocMemory(sizeof(jit_value_t) * args_number);
		item = xanList_first(inst->callParameters);
		assert(item != NULL);
		count = 0;

		/* Create the arguments			*/
		while (item != NULL) {
			param = (ir_item_t *) xanList_getData(item);
			assert(param != NULL);

			/* Create the argument		*/
			PDEBUG("IRVIRTUALMACHINE: IR_CALL:					Argument %d : ", count);
			args[count] = make_variable(_this, method, param, jitFunction);
			PDEBUG("\n");
			assert(args[count] != NULL);

			/* Fetch the next parameter	*/
			count++;
			item = item->next;
		}
	}

	/* Fetch the call flags			*/
	if ((inst->param_2).value.v == 1) {
		call_flags = JIT_CALL_TAIL;
	}

	if ((_this->behavior).dla) {
		jit_insn_mark_offset(jitFunction->function, inst->ID);
	}

	/* Create the JIT call instruction	*/
	PDEBUG("IRVIRTUALMACHINE: IR_CALL:			Insert the JIT call instruction\n");
	switch ((inst->result).type) {
	    case IROFFSET:
		    PDEBUG("IRVIRTUALMACHINE: IR_CALL:	Variable %lld = call(Token = 0x%llX)\n", (inst->result).value.v, method_value);
		    if (jitFunction->locals[(inst->result).value.v] != NULL) {
			    jit_value_t temp;
			    temp = jit_insn_call(jitFunction->function, (char *) IRMETHOD_getMethodName(ir_jump_method), jumpMethodJITInternal->function, jit_signature, args, args_number, call_flags);
			    jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
		    } else{
			    jitFunction->locals[(inst->result).value.v] = jit_insn_call(jitFunction->function, (char *) IRMETHOD_getMethodName(ir_jump_method), jumpMethodJITInternal->function, jit_signature, args, args_number, call_flags);
		    }
		    assert(jitFunction->locals[(inst->result).value.v] != NULL);
		    break;
	    case IRVOID:
	    case NOPARAM:
		    PDEBUG("IRVIRTUALMACHINE: IR_CALL:				call(Token = 0x%llX)\n", method_value);
		    jit_insn_call(jitFunction->function, (char *) IRMETHOD_getMethodName(ir_jump_method), jumpMethodJITInternal->function, jit_signature, args, args_number, call_flags);
		    break;
	    default:
		    SNPRINTF(buf, DIM_BUF, "IRVIRTUALMACHINE: IRCALL: ERROR = The result type %u is not known. ", (inst->result).type);
		    print_err((char *) buf, 0);
		    abort();
	}

	/* Free the memory			*/
	if (args != NULL) {
		freeMemory(args);
	}

	/* Return				*/
	PDEBUG("IRVIRTUALMACHINE: IR_CALL:	End\n");
	return 0;
}

static inline JITINT16 translate_ir_ret (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t result;
	IRVM_t_internal *internalVM;

	/* Assertions				*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(jitFunction->function != NULL);
	PDEBUG("IRVIRTUALMACHINE: translate_ir_ret:	Start\n");

	/* Init the variable			*/
	result = NULL;

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Check if we have to profile the root *
	* set                                  */
	if (internal_addRootSetProfile(_this, method, jitFunction)) {
		jit_insn_call_native(jitFunction->function, "popLastRootSet", _this->gc->popLastRootSet, internalVM->signPopLastRootSet, NULL, 0, 0);
	}

	/* Translate the feedback code	*/
	//translate_exit_method_feedback_code(system, method);

	switch (IRMETHOD_getResultType(method)) {
	    case IRVOID:
		    assert(((inst->param_1).type == NOPARAM) || ((inst->param_1).type == IRVOID));
		    PDEBUG("IRVIRTUALMACHINE:		Return(VOID)\n");
		    break;
	    default:
		    assert((inst->param_1).type != NOPARAM);

		    /* Check where the value is	*/
		    result = make_variable(_this, method, &(inst->param_1), jitFunction);
		    assert(result != NULL);
	}

	/* Insert the native stop call      */
	#ifdef MORPHEUS
	internal_insert_native_stop_function(method);
	#endif

	/* Create the return instruction	*/
	jit_insn_return(jitFunction->function, result);

	PDEBUG("IRVIRTUALMACHINE: translate_ir_ret:	End\n");
	return 0;
}

static inline JITINT16 translate_ir_eq (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;
	jit_value_t temp;
	jit_value_t temp2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);

	/* Create the first parameter	*/
	PDEBUG("IRVIRTUALMACHINE: IREQ:		Variable %lld = equal(", (inst->result).value.v);
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the equal instruction		*/
	temp = jit_insn_eq(jitFunction->function, param_1, param_2);
	temp2 = jit_insn_to_bool(jitFunction->function, temp);
	assert(temp2 != NULL);

	/* Store the result                     */
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp2);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp2;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_branchifnot (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction){
	t_jit_label             *label;
	jit_value_t param_1;

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(_this != NULL);
	assert((inst->param_2).type == IRLABELITEM);
	PDEBUG("IRVIRTUALMACHINE:               branch_if_not(Variable %lld, Label \"L%lld\")\n", (inst->param_1).value.v, (inst->param_2).value.v);

	/* Insert the label		*/
	label = insert_label(labels, (inst->param_2).value.v);
	assert(label != NULL);

	/* Create the condition		*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);

	/* Create the jump		*/
	jit_insn_branch_if_not(jitFunction->function, param_1, &(label->label));

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_branchif (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, XanList *labels, t_jit_function_internal *jitFunction){
	t_jit_label     *label;
	jit_value_t param_1;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_2).type == IRLABELITEM);

	PDEBUG("IRVIRTUALMACHINE:               branch_if(Variable %lld, Label \"L%lld\")\n", (inst->param_1).value.v, (inst->param_2).value.v);
	label = insert_label(labels, (inst->param_2).value.v);
	assert(label != NULL);

	/* Make the condition of the branch			*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);

	/* Generate the jump					*/
	jit_insn_branch_if(jitFunction->function, param_1, &(label->label));

	/* Return						*/
	return 0;
}

static inline JITINT16 translate_ir_gt (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;
	jit_value_t temp;
	jit_value_t temp2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);

	/* Create the first parameter	*/
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = gt(", (inst->result).value.v);
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the gt instruction		*/
	temp = jit_insn_gt(jitFunction->function, param_1, param_2);
	temp2 = jit_insn_to_bool(jitFunction->function, temp);
	assert(temp2 != NULL);

	/* Store the result                     */
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp2);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp2;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_not (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);

	/* Create the first parameter	*/
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = not (", (inst->result).value.v);
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the lt instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_not(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_not(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_lt (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;
	jit_value_t temp;
	jit_value_t temp2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);

	/* Create the first parameter	*/
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = lt(", (inst->result).value.v);
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the lt instruction		*/
	temp = jit_insn_lt(jitFunction->function, param_1, param_2);
	temp2 = jit_insn_to_bool(jitFunction->function, temp);
	assert(temp2 != NULL);

	/* Store the result                     */
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp2);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp2;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_store (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t temp;

	/* Assertions						*/
	assert(method != NULL);
	assert(_this != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IROFFSET);
	#ifdef DEBUG
	if ((inst->param_1).internal_type == IRVALUETYPE) {
		assert( (inst->param_1).type_infos != NULL);
	}
	#endif
	PDEBUG("IRVIRTUALMACHINE: translate_ir_store: Start\n");

	/* Init the variables					*/
	temp = NULL;

	/* Check the parameters					*/
	jitFunction->locals[(inst->param_1).value.v] = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(jitFunction->locals[(inst->param_1).value.v] != NULL);

	/* Create the second parameter				*/
	PDEBUG("IRVIRTUALMACHINE:               store(Variable %lld, ", (inst->param_1).value.v);
	temp = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(temp != NULL);

	/* Create the store instruction				*/
	jit_insn_store(jitFunction->function, jitFunction->locals[(inst->param_1).value.v], temp);

	PDEBUG("IRVIRTUALMACHINE: translate_ir_store: End\n");

	/* Return						*/
	return 0;
}

static inline JITINT16 translate_ir_rem (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = rem(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_rem(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_rem(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline void fetch_temporaries (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction){
	JITUINT32	count;
	JITUINT32	startVar;
	JITUINT32	endVar;

	/* Fetch the start and the end ID of	*
	 * temporaries				*/
	endVar		= IRMETHOD_getNumberOfVariablesNeededByMethod(method);
	startVar	= IRMETHOD_getMethodLocalsNumber(method) + IRMETHOD_getMethodParametersNumber(method);

	/* Fetch the temporaries		*/
	for (count=startVar; count < endVar; count++){
		ir_item_t	*item;
		item		= IRMETHOD_getIRVariable(method, count);
		if (	(item != NULL)								&&
			((item->internal_type != IRVALUETYPE) || (item->type_infos != NULL))	){
			make_variable(_this, method, item, jitFunction);
		}
	}

	/* Return				*/
	return ;
}

static inline JITINT16 fetch_parameters (ir_method_t *method, t_jit_function_internal *jitFunction){
	JITUINT32 count;
	JITUINT32 paramsNum;

	/* Assertions                           */
	assert(method != NULL);
	assert(jitFunction != NULL);

	/* Fetch the number of parameters	*/
	paramsNum = IRMETHOD_getMethodParametersNumber(method);

	/* Check if there are parameters	*/
	if (paramsNum == 0) {
		return 0;
	}
	assert(jitFunction->locals != NULL);

	PDEBUG("IRVIRTUALMACHINE:		Fetch the parameters\n");
	for (count = 0; count < paramsNum; count++) {
		PDEBUG("IRVIRTUALMACHINE:			Parameter %d\n", count);
		jitFunction->locals[count] = jit_value_get_param(jitFunction->function, count);
	}

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_add_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = add_ovf(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the add_ovf instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_add_ovf(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_add_ovf(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_sub_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = sub_ovf(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_sub_ovf(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_sub_ovf(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_mul_ovf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = mul_ovf(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_mul_ovf(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_mul_ovf(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_shl (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = shl(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the shl instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_shl(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_shl(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_shr (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = shr(", (inst->result).value.v);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(")\n");
	assert(param_2 != NULL);

	/* Create the shr instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_shr(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_shr(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_get_address (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* Assertions						*/
	assert(method != NULL);
	assert(inst != NULL);
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = get_address(Variable/constant %lld)\n", (inst->result).value.v, (inst->param_1).value.v);

	/* Make the first parameter				*/
	assert((inst->param_1).type == IROFFSET);
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);

	/* Perform the insn_address_of */
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_address_of(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_address_of(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return			*/
	return 0;
}

static inline JITINT16 translate_ir_isNaN (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t value;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	assert( ((inst->param_1).internal_type == IRNFLOAT)
		|| ((inst->param_1).internal_type == IRFLOAT32)
		|| ((inst->param_1).internal_type == IRFLOAT64));
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);

	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = isNaN(Variable %lld)\n", (inst->result).value.v, (inst->param_1).value );

	value = make_variable(_this, method, &(inst->param_1), jitFunction);

	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_is_nan(jitFunction->function, value);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_is_nan(jitFunction->function, value);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_isInf (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t value;

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->result).type == IROFFSET);
	assert( ((inst->param_1).internal_type == IRNFLOAT)
		|| ((inst->param_1).internal_type == IRFLOAT32)
		|| ((inst->param_1).internal_type == IRFLOAT64));
	assert((inst->param_2).type == NOPARAM);
	assert((inst->param_3).type == NOPARAM);

	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = isInfinite(Variable %lld)\n", (inst->result).value.v, (inst->param_1).value );

	value = make_variable(_this, method, &(inst->param_1), jitFunction);

	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_is_inf(jitFunction->function, value);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_is_inf(jitFunction->function, value);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_newarr (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	TypeDescriptor  *class;
	TypeDescriptor  *resolvedType;
	ArrayDescriptor *arrayType;
	jit_value_t valueLength;
	jit_value_t args[2];
	IRVM_t_internal *internalVM;

	/* Assertions			*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(jitFunction != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRCLASSID || ((inst->param_1).type == IRSYMBOL && (inst->param_1).internal_type == IRCLASSID));
	assert((inst->param_2).type != NOPARAM);
	assert((inst->param_2).type != IRCLASSID);
	assert((inst->param_2).type != IRMETHODID);
	assert((inst->result).type == IROFFSET);

	/* Initialize the variables	*/
	class = NULL;

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Fetch the TypeDescriptor		*/
	if ((inst->param_1).type == IRSYMBOL) {
		internal_unlockLibjit(_this->data);
		internal_unlockVM(_this);
		class = (TypeDescriptor *) (JITNUINT) IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) ((inst->param_1).value.v)).v;
		internal_lockVM(_this);
		internal_lockLibjit(_this->data);
	}else{
		class = (TypeDescriptor *) (JITNUINT) (inst->param_1).value.v;
	}
	assert(class != NULL);
	resolvedType = class->makeArrayDescriptor(class, 1);
	assert(resolvedType != NULL);
	arrayType = GET_ARRAY(resolvedType);

	/* Print the instruction	*/
#ifdef PRINTDEBUG
	PDEBUG("IRVIRTUALMACHINE: newarr:		Variable %lld = newarr(%s., ", (inst->result).value.v, class->getCompleteName(class));
	switch ((inst->param_2).type) {
	    case IRINT8:
	    case IRINT16:
	    case IRINT32:
	    case IRINT64:
	    case IRUINT8:
	    case IRUINT16:
	    case IRUINT32:
	    case IRUINT64:
		    PDEBUG("Length = %lld)\n", (inst->param_2).value.v);
		    break;
	    case IROFFSET:
		    PDEBUG("Length stored inside the Variable = %lld)\n", (inst->param_2).value.v);
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: newarr: ERROR = Type of the length variable of the array is not known. ", 0);
		    abort();
	}
#endif

	/* Create the arguments				*/
	PDEBUG("IRVIRTUALMACHINE: newarr:		Create the argument for the GC call\n");
	valueLength = make_variable(_this, method, &(inst->param_2), jitFunction);
	args[0] = jit_value_create_nint_constant(jitFunction->function, jit_type_void_ptr, (JITNUINT) arrayType);
	args[1] = valueLength;

	/* Call the Allocator				*/
	PDEBUG("IRVIRTUALMACHINE: newarr:		Call the GC\n");
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_call_native(jitFunction->function, "createArray", _this->gc->createArray, internalVM->signCreateArray, args, 2, JIT_CALL_NOTHROW);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_call_native(jitFunction->function, "createArray", _this->gc->createArray, internalVM->signCreateArray, args, 2, JIT_CALL_NOTHROW);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return			*/
	PDEBUG("IRVIRTUALMACHINE: newarr: Exit\n");
	return 0;
}

static inline JITINT16 translate_ir_checknull (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t var;

	/* Assertions				*/
	assert(method != NULL);
	assert(inst != NULL);

	/* Fetch the variable			*/
	var = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(var != NULL);

	/* Make the JIT instruction		*/
	jit_insn_check_null(jitFunction->function, var);

	/* Return				*/
	return 0;
}

static inline JITINT16 translate_ir_load_rel (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_type_t type;
	JITINT32 offset;
	jit_value_t object;
	jit_value_t temp;

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert(jitFunction != NULL);
	assert((inst->result).type == IROFFSET);
	assert( ((inst->param_2).type == IRINT32)       ||
		((inst->param_2).type == IRUINT32)      );
	assert((inst->param_3).type == NOPARAM);

	/* Init the variables		*/
	type = NULL;

	/* Make the parameters of the instruction	*/
	object = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(object != NULL);

	offset = (inst->param_2).value.v;
	if ((inst->result).internal_type == IRVALUETYPE) {

		/* retrieve the jit type associated with the valuetype */
		assert((inst->result).type_infos != NULL);
		type = (_this->lookupValueType((inst->result).type_infos))->type;
	} else{
		type = internal_fromIRTypeToJITType(_this, (inst->result).internal_type, (inst->result).type_infos);
	}
	assert(type != NULL);

	/* Translate the instruction			*/
	temp = jit_insn_load_relative(jitFunction->function, object, offset, type);
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	return 0;
}

/**
 * Translates the IR instruction IRSTOREREL in its JIT counterpart
 *
 * * If a static field is being stored, the first parameter of the IR instruction (inst->param_1) must be an IRMPOINTER containing the address of the field. Otherwise it must be an IROFFSET indicating the number of the local of the function containing the address.
 */
static inline JITINT16 translate_ir_store_rel (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t value;
	JITINT16 success;
	jit_value_t object;

	/* Assertions			*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_2).type != IROFFSET);

	/* Prepare the parameters	*/
	object = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(object != NULL);

	/* Make the value to store	*/
	PDEBUG("IRVIRTUALMACHINE:               store_relative(Object Variable %lld, Offset %lld, Value ", (inst->param_1).value.v, (inst->param_2).value.v);

	value = make_variable(_this, method, &(inst->param_3), jitFunction);
	assert(value != NULL);
	PDEBUG(")\n");

	success = jit_insn_store_relative(jitFunction->function, object, (inst->param_2).value.v, value);
	if (success == 0) {
		print_err("IRVIRTUALMACHINE: translate_ir_store_rel : ERROR: Libjit return an error. ", 0);
		abort();
	}

	return 0;
}

static inline JITINT16 translate_ir_vcall (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	XanListItem     *item;
	t_jit_function  *methodToJumpJIT;
	ir_item_t       *param;
	jit_type_t type;
	jit_value_t v_method;
	jit_value_t     *v_args;
	JITUINT32 v_args_number;
	JITUINT32 count;
	JITINT32 call_flags;
	IR_ITEM_VALUE method_value;
	t_jit_function_internal *methodToJumpJITInternal;

	#ifdef PRINTDEBUG
	ir_method_t     *methodToJump;
	#endif

	/* Assertions		*/
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).type == IRMETHODID || ((inst->param_1).type == IRSYMBOL && (inst->param_1).internal_type == IRMETHODID));
	assert((inst->param_2).type == IRINT32);
	assert((inst->param_3).type == IROFFSET);
	assert(inst->callParameters != NULL);
	assert(xanList_length(inst->callParameters) > 0);
	assert(method != NULL);
	assert(jitFunction->function != NULL);
	assert(jitFunction->signature != NULL);
	PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall: Start\n");

	/* Initialize the variables	*/
	v_args = NULL;
	param = NULL;
	methodToJumpJIT = NULL;
	item = NULL;
	call_flags = 0;
	count = 0;
	v_args_number = 0;

	if ((inst->param_1).type == IRSYMBOL) {
		internal_unlockLibjit(_this->data);
		internal_unlockVM(_this);
		method_value = IRMETHOD_resolveSymbol((ir_symbol_t *) (JITNUINT) (inst->param_1).value.v).v;
		internal_lockVM(_this);
		internal_lockLibjit(_this->data);
	}else{
		method_value = (inst->param_1).value.v;
	}

	/* Fetch the method to	*
	 * jump			*/
	#ifdef PRINTDEBUG
	methodToJump = _this->getIRMethod(method_value);
	assert(methodToJump != NULL);
	PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall:   Method to jump	= %s\n", methodToJump->getName(methodToJump));
	#endif
	methodToJumpJIT = _this->getJITMethod(method_value);
	assert(methodToJumpJIT != NULL);
	methodToJumpJITInternal = (t_jit_function_internal *) methodToJumpJIT->data;
	assert(methodToJumpJITInternal != NULL);

	/* Print the instruction	*/
#ifdef PRINTDEBUG
	switch ((inst->result).type) {
	    case IROFFSET:
		    PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall:	Variable %lld = virtual call(Method = %s, VTable = Var %lld)\n", (inst->result).value.v, methodToJump->getName(methodToJump), (inst->param_3).value.v);
		    break;
	    case IRVOID:
		    PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall:   virtual call(Method = %s, VTable = Var %lld)\n", methodToJump->getName(methodToJump), (inst->param_3).value.v);
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: translate_ir_vcall: ERROR = Result type not known. ", 0);
		    abort();
	}
#endif

	/* Fetch the object				*/
	PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall:   Fetch the object\n");
	item = xanList_first(inst->callParameters);
	assert(item != NULL);
	param = (ir_item_t *) xanList_getData(item);
	assert(param != NULL);

	/* Fetch the pointer of the indirect function   *
	 * to call					*/
	v_method = make_variable(_this, method, &(inst->param_3), jitFunction);

	/* Compute the number of the arguments		*/
	v_args_number = xanList_length(inst->callParameters);
	assert(v_args_number > 0);
	PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall:   Parameters number       = %d\n", v_args_number);

	/* Call the virtual method			*/
	v_args = (jit_value_t *) allocMemory(sizeof(jit_value_t) * v_args_number);
	item = xanList_first(inst->callParameters);
	assert(item != NULL);
	count = 0;
	while (item != NULL) {
		param = (ir_item_t *) xanList_getData(item);
		assert(param != NULL);

		/* Create the argument		*/
		v_args[count] = make_variable(_this, method, param, jitFunction);
		assert(v_args[count] != NULL);

		/* Fetch the next parameter	*/
		count++;
		item = item->next;
	}
	assert(count == v_args_number);

	/* Fetch the call flags			*/
	if ((inst->param_2).value.v == 1) {
		call_flags = JIT_CALL_TAIL;
	}

	if ((_this->behavior).dla) {
		jit_insn_mark_offset(jitFunction->function, inst->ID);
	}

	/* Create the call instruction		*/
	PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall:	Create the call to the correct method returned by the virtual call handler\n");
	switch ((inst->result).type) {
	    case IROFFSET:
		    assert(v_args != NULL);

		    /* call indirect */
		    if (jitFunction->locals[(inst->result).value.v] != NULL) {
			    jit_value_t temp;
			    temp = jit_insn_call_indirect(jitFunction->function, v_method, methodToJumpJITInternal->signature, v_args, v_args_number, call_flags);
			    jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
		    } else{
			    jitFunction->locals[(inst->result).value.v] = jit_insn_call_indirect(jitFunction->function, v_method, methodToJumpJITInternal->signature, v_args, v_args_number, call_flags);
		    }
		    assert(jitFunction->locals[(inst->result).value.v] != NULL);

		    /* Update the locals type	*/
		    type = jit_value_get_type(jitFunction->locals[(inst->result).value.v]);
		    assert(type != NULL);
		    break;
	    case IRVOID:

		    /* call indirect */
		    jit_insn_call_indirect(jitFunction->function, v_method, methodToJumpJITInternal->signature, v_args, v_args_number, call_flags );
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: translate_ir_vcall: ERROR = Result type not known. ", 0);
		    abort();
	}

	/* Free the memory		*/
	freeMemory(v_args);

	/* Return			*/
	PDEBUG("IRVIRTUALMACHINE: translate_ir_vcall: Exit\n");
	return 0;
}

static inline void variablesToRootSets (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction){
	jit_type_t rootsets;
	jit_label_t start;
	jit_label_t end;
	jit_value_t addPointerArgs[2];
	jit_value_t sets;
	jit_value_t sets_pointer;
	JITUINT32 count;
	JITUINT32 count2;
	IRVM_t_internal *internalVM;

	/* Assertions				*/
	assert(_this != NULL);
	assert(method != NULL);
	assert(jitFunction != NULL);
	PDEBUG("IRVIRTUALMACHINE: variablesToRootSets: Start\n");

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Check if we have to profile the root *
	* set                                  */
	if (!internal_addRootSetProfile(_this, method, jitFunction)) {
		PDEBUG("IRVIRTUALMACHINE: variablesToRootSets: Exit\n");
		return;
	}

	/* Init the variables		*/
	start = jit_label_undefined;
	end = jit_label_undefined;
	rootsets = jit_type_void;

	/* Set the start label		*/
	PDEBUG("IRVIRTUALMACHINE: variablesToRootSets:  Insert the start label\n");
	jit_insn_label(jitFunction->function, &start);

	/* Make the array type		*/
	rootsets = jit_type_create_struct(0, 0, 0);
	jit_type_set_size_and_alignment(rootsets, sizeof(JITUINT32) * method->getRootSetTop(method), 0);
	sets = jit_value_create(jitFunction->function, rootsets);
	assert(sets != NULL);
	jit_type_free(rootsets);
	sets_pointer = jit_insn_address_of(jitFunction->function, sets);
	assert(sets_pointer != NULL);

	/* Make the set			*/
	PDEBUG("IRVIRTUALMACHINE: variablesToRootSets:  Make the root set\n");
	for (count = 0, count2 = 0; count < method->getRootSetTop(method); count++) {
		JITUINT32 rootSetSlot;
		jit_value_t temp;

		/* Fetch the ID of the next pointer variable	*/
		rootSetSlot = method->getRootSetSlot(method, count);
		PDEBUG("IRVIRTUALMACHINE: variablesToRootSets:          Add the variable %u\n", rootSetSlot);

		if (jitFunction->locals[rootSetSlot] != NULL) {
			temp = jit_insn_address_of(jitFunction->function, jitFunction->locals[rootSetSlot]);
			assert(temp != NULL);
			jit_insn_store_relative(jitFunction->function, sets_pointer, sizeof(void *) * count2, temp);
			count2++;
		}
	}
	assert(count2 > 0);

	/* Add the set to the root set	*
	 * of the GC			*/
	PDEBUG("IRVIRTUALMACHINE: variablesToRootSets:  Call the addRootSet function of the GC\n");
	addPointerArgs[0] = sets_pointer;
	addPointerArgs[1] = jit_value_create_nint_constant(jitFunction->function, jit_type_uint, (JITUINT32) count2);

	jit_insn_call_native(jitFunction->function, "addRootSet", _this->gc->addRootSet, internalVM->signAddRootSet, addPointerArgs, 2, 0);

	/* Set the stop label		*/
	PDEBUG("IRVIRTUALMACHINE: variablesToRootSets:  Insert the end label\n");
	jit_insn_label(jitFunction->function, &end);

	/* Move the previous code to the start of the function	*/
	jit_insn_move_blocks_to_start(jitFunction->function, start, end);

	/* Return			*/
	PDEBUG("IRVIRTUALMACHINE: variablesToRootSets: Exit\n");
	return;
}

static inline JITUINT32 fromJITTypeToIRType (jit_type_t jitType){

	if (jitType == jit_type_sbyte) {
		return IRINT8;
	}
	if (jitType == jit_type_short) {
		return IRINT16;
	}
	if (jitType == jit_type_int) {
		return IRINT32;
	}
	if (jitType == jit_type_long) {
		return IRINT64;
	}
	if (jitType == jit_type_nint) {
		if (sizeof(JITNINT) == 4) {
			return IRINT32;
		} else{
			assert(sizeof(JITNINT) == 8);
			return IRINT64;
		}
	}
	if (jitType == jit_type_ubyte) {
		return IRUINT8;
	}
	if (jitType == jit_type_ushort) {
		return IRUINT16;
	}
	if (jitType == jit_type_uint) {
		return IRUINT32;
	}
	if (jitType == jit_type_ulong) {
		return IRUINT64;
	}
	if (jitType == jit_type_float32) {
		return IRFLOAT32;
	}
	if (jitType == jit_type_float64) {
		return IRFLOAT64;
	}
	if (jitType == jit_type_nfloat) {
		return IRNFLOAT;
	}
	if (jitType == jit_type_void_ptr) {
		return IRNINT;
	}
	print_err("IRVIRTUALMACHINE: fromJITTypeToIRType: ERROR = JIT Type is not known. ", 0);
//	abort();
	return 0;
}

static inline JITINT16 translate_ir_alloc_align (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_type_t sign;
	jit_type_t params[2];
	jit_value_t args[2];
	jit_value_t temp;

	/* Assertions                   */
	assert(method != NULL);
	assert(inst != NULL);

	/* Create the signature				*/
	params[0] = jit_type_int;
	params[1] = jit_type_int;
	sign = jit_type_create_signature(jit_abi_cdecl, jit_type_void_ptr, params, 2, 0);
	assert(sign != NULL);

	/* Create the arguments				*/
	args[0] = make_variable(_this, method, &(inst->param_2), jitFunction);
	args[1] = make_variable(_this, method, &(inst->param_1), jitFunction);

	/* Call the memory allocator			*/
	temp = jit_insn_call_native(jitFunction->function, "allocAlignedMemoryFunction", allocAlignedMemoryFunction, sign, args, 2, JIT_CALL_NOTHROW);

	/* Store the result				*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Free the memory				*/
	jit_type_free(sign);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_free (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_type_t sign;
	jit_type_t params[1];
	jit_value_t args[1];

	/* Assertions                   */
	assert(method != NULL);
	assert(inst != NULL);

	/* Create the signature				*/
	params[0] = jit_type_void_ptr;
	sign = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 1, 0);
	assert(sign != NULL);

	/* Create the arguments				*/
	args[0] = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(args[0] != NULL);

	/* Call the memory allocator			*/
	jit_insn_call_native(jitFunction->function, "freeFunction", freeFunction, sign, args, 1, JIT_CALL_NOTHROW);

	/* Free the memory				*/
	jit_type_free(sign);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_realloc (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t temp;
	jit_type_t sign;
	jit_type_t params[2];
	jit_value_t args[2];

	/* Assertions                   */
	assert(method != NULL);
	assert(inst != NULL);

	/* Create the signature				*/
	params[0] = jit_type_void_ptr;
	params[1] = jit_type_int;
	sign = jit_type_create_signature(jit_abi_cdecl, jit_type_void_ptr, params, 2, 0);
	assert(sign != NULL);

	/* Create the arguments				*/
	args[0] = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(args[0] != NULL);
	args[1] = make_variable(_this, method, &(inst->param_2), jitFunction);
	assert(args[1] != NULL);

	/* Call the memory allocator			*/
	temp = jit_insn_call_native(jitFunction->function, "rellocFunction", dynamicReallocFunction, sign, args, 2, JIT_CALL_NOTHROW);

	/* Store the result				*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Free the memory				*/
	jit_type_free(sign);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_alloc (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t temp;
	jit_type_t sign;
	jit_type_t params[1];
	jit_value_t args[1];

	/* Assertions                   */
	assert(method != NULL);
	assert(inst != NULL);

	/* Create the signature				*/
	params[0] = jit_type_int;
	sign = jit_type_create_signature(jit_abi_cdecl, jit_type_void_ptr, params, 1, 0);
	assert(sign != NULL);

	/* Create the arguments				*/
	args[0] = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(args[0] != NULL);

	/* Call the memory allocator			*/
	temp = jit_insn_call_native(jitFunction->function, "allocFunction", allocFunction, sign, args, 1, JIT_CALL_NOTHROW);

	/* Store the result				*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = temp;
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Free the memory				*/
	jit_type_free(sign);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_alloca (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);
	assert((inst->param_1).internal_type == IRINT32);

	PDEBUG("IRVIRTUALMACHINE:               alloca(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_alloca(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_alloca(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_floor (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               floor(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_floor(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_floor(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_sqrt (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               sqrt(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_sqrt(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_sqrt(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_pow (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;
	jit_value_t param_2;

	/* Assertions                   */
	assert(method != NULL);
	assert(inst != NULL);

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_pow(jitFunction->function, param_1, param_2);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_pow(jitFunction->function, param_1, param_2);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_sin (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               sin(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_sin(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_sin(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_acos (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               acos(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_acos(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_acos(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_cosh (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               cosh(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_cosh(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_cosh(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_cos (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);

	PDEBUG("IRVIRTUALMACHINE:               cos(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param_1 != NULL);
	PDEBUG(")\n");

	/* Create the memset instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_cos(jitFunction->function, param_1);
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_cos(jitFunction->function, param_1);
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return					*/
	return 0;
}

static inline JITINT16 translate_ir_memory_copy (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction) {
	jit_value_t param_1;
	jit_value_t param_2;
	jit_value_t param_3;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);
	PDEBUG("IRVIRTUALMACHINE:               memcopy(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(", ");
	assert(param_2 != NULL);

	param_3 = make_variable(_this, method, &(inst->param_3), jitFunction);
	PDEBUG(")\n");
	assert(param_3 != NULL);

	/* Create the memcopy instruction		*/
	jit_insn_memcpy(jitFunction->function, param_1, param_2, param_3);

	return 0;

}

static inline JITINT16 translate_ir_initmemory (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t param_1;
	jit_value_t param_2;
	jit_value_t param_3;

	/* assertions */
	assert(method != NULL);
	assert(inst != NULL);
	PDEBUG("IRVIRTUALMACHINE:               initmemory(");

	/* Create the first parameter	*/
	param_1 = make_variable(_this, method, &(inst->param_1), jitFunction);
	PDEBUG(", ");
	assert(param_1 != NULL);

	/* Create the second parameter	*/
	param_2 = make_variable(_this, method, &(inst->param_2), jitFunction);
	PDEBUG(", ");
	assert(param_2 != NULL);

	param_3 = make_variable(_this, method, &(inst->param_3), jitFunction);
	PDEBUG(")\n");
	assert(param_3 != NULL);

	/* Create the memset instruction		*/
	jit_insn_memset(jitFunction->function, param_1, param_2, param_3);

	return 0;
}

static inline void set_volatile_variables (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){
	jit_value_t	value;

	if (!IRMETHOD_isInstructionVolatile(inst)){
		return ;
	}
	switch (IRMETHOD_getInstructionParametersNumber(inst)){
		case 3:
			value = make_variable(_this, method, &(inst->param_3), jitFunction);
			jit_value_set_volatile(value);
		case 2:
			value = make_variable(_this, method, &(inst->param_2), jitFunction);
			jit_value_set_volatile(value);
		case 1:
			value = make_variable(_this, method, &(inst->param_1), jitFunction);
			jit_value_set_volatile(value);
			break;
	}

	return;
}

static inline JITINT16 bind_instruction_offset (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction){

	/* Assertions           */
	assert(method != NULL);
	assert(jitFunction != NULL);
	assert(inst != NULL);
	assert(inst->byte_offset >= 0);

	if ((_this->behavior).debugExecution) {
		#ifdef MORPHEUS
		jit_insn_mark_offset(jitFunction->function, method->getInstructionID(method, inst));
		#else
		jit_insn_mark_offset(jitFunction->function, (jit_int) inst->byte_offset);
		#endif
	}

	/* Return		*/
	return 0;
}

void dump_string (char *prefix){
	fprintf(stderr, "%s", prefix);
}

void dump_int (int value){
	fprintf(stderr, "%d\n", value);
}

void dump_float (double value){
	fprintf(stderr, "%f\n", value);
}

void dump_prefix (int instID){
	fprintf(stderr, "Begin instruction %d\n", instID);
}

void dump_suffix (int instID){
	fprintf(stderr, "End instruction %d\n", instID);
}

void dump_varprefix (int varID){
	fprintf(stderr, "	Var %d: ", varID);
}

static inline JITINT32 internal_isRootSetEmpty (ir_method_t *method, t_jit_function_internal *jitFunction){
	JITUINT32 count;
	JITUINT32 rootSetSlot;

	/* Assertions			*/
	assert(method != NULL);
	assert(jitFunction != NULL);
	assert(jitFunction->locals != NULL);

	for (count = 0; count < method->getRootSetTop(method); count++) {
		rootSetSlot = method->getRootSetSlot(method, count);
		if (jitFunction->locals[rootSetSlot] != NULL) {
			return JITFALSE;
		}
	}

	return JITTRUE;
}

static inline JITINT32 internal_addRootSetProfile (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction){

	/* Assertions                           */
	assert(method != NULL);
	assert(jitFunction != NULL);

	/* Check if the current GC needs	*
	 * this kind of support			*/
	if (((_this->gc->gc_plugin)->getSupport() & ILDJIT_GCSUPPORT_ROOTSET) == 0) {
		return JITFALSE;
	}

	/* Check if the root set contains at    *
	 * at most one element			*/
	if (    (method->getRootSetTop(method) == 0)            ||
		(internal_isRootSetEmpty(method, jitFunction))  ) {
		return JITFALSE;
	}
	assert(method->getRootSetTop(method) > 0);
	assert(!internal_isRootSetEmpty(method, jitFunction));

	return JITTRUE;
}

static inline JITINT32 internal_dummyRecompiler (jit_function_t function, void *pc){
	return JIT_RESULT_OK;
}

static inline void internal_lockVM (IRVM_t *_this){

	/* Assertions			*/
	assert(_this != NULL);

	PLATFORM_lockMutex(&(_this->vmMutex));
}

static inline void internal_unlockVM (IRVM_t *_this){

	/* Assertions			*/
	assert(_this != NULL);

	PLATFORM_unlockMutex(&(_this->vmMutex));
}

static inline void internal_lockLibjit (IRVM_t_internal *_this){

	/* Assertions			*/
	assert(_this != NULL);

	jit_context_build_start(_this->context);
}

static inline void internal_unlockLibjit (IRVM_t_internal *_this){

	/* Assertions			*/
	assert(_this != NULL);

	jit_context_build_end(_this->context);
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
	return VERSION;
}

JITINT32 IRVM_run (IRVM_t *self, t_jit_function *jitFunction, void **args, void *returnArea){
	return internal_run(jitFunction->data, args, returnArea);
}

static inline JITINT32 internal_run (t_jit_function_internal *jitFunction, void **args, void *returnArea){
	JITINT32 error;

	/* Assertions				*/
	assert(jitFunction != NULL);
	assert(jitFunction->function != NULL);

	/* Run					*/
	error = jit_function_apply(jitFunction->function, args, returnArea);

	/* Return				*/
	return error;
}

void IRVM_newBackendMethod (IRVM_t *_this, void *method, t_jit_function *backendFunction, JITBOOLEAN isRecompilable, JITBOOLEAN isCallableExternally){
	jit_function_t function;
	IRVM_t_internal *internalVM;
	t_jit_function_internal *internalFunction;

	/* Assertions				*/
	assert(_this != NULL);
	assert(backendFunction != NULL);

	/* Check if the function was previously	*
	 * allocated				*/
	if (backendFunction->data == NULL) {

		/* Allocate the internal data		*/
		backendFunction->data = allocFunction(sizeof(t_jit_function_internal));
	}

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);
	internalFunction = backendFunction->data;
	assert(internalFunction != NULL);

	/* Check if we need to allocate the     *
	 * function				*/
	if (internalFunction->function != NULL) {

		/* Return				*/
		return;
	}

	/* Lock					*/
	internal_lockVM(_this);
	internal_lockLibjit(_this->data);

	/* Allocate a new libjit function	*/
	function = jit_function_create(internalVM->context, internalFunction->signature);
	assert(function != NULL);
	if (isRecompilable) {
		jit_function_set_recompilable(function);
	} else {
		jit_function_clear_recompilable(function);
	}
	jit_function_set_on_demand_compiler(function, internal_recompilerFunctionWrapper);
	jit_function_set_optimization_level(function, (_this->behavior).libjitOptimizations);
	if (method != NULL) {
		jit_function_set_meta(function, METHOD_METADATA, method, 0, 0);
	}
	jit_function_set_meta(function, IRVM_METADATA, _this, 0, 0);

	/* Store the function			*/
	internalFunction->function = function;

	/* Unlock				*/
	internal_unlockLibjit(_this->data);
	internal_unlockVM(_this);

	/* Return				*/
	return;
}

static inline JITINT32 internal_recompilerFunctionWrapper (jit_function_t function){
	IRVM_t *_this;
	void *ilmethod;
	void *caller;
	ir_instruction_t *inst;
	JITINT32 result;

	/* Assertions					*/
	assert(function != NULL);

	/* Initialize the variables			*/
	caller = NULL;
	inst = NULL;

	/* Fetch the IRVM				*/
	_this = (IRVM_t *) jit_function_get_meta(function, IRVM_METADATA);
	assert(_this != NULL);

	/* Call the user compiler function		*/
	ilmethod = jit_function_get_meta(function, METHOD_METADATA);

	/* Unlock the libjit				*/
	internal_unlockLibjit(_this->data);

	result = _this->recompilerFunction(ilmethod, caller, inst);

	/* Lock the Libjit library			*/
	internal_lockLibjit(_this->data);

	/* Return					*/
	return result;
}

static inline JITINT16 internal_translate_ir_conv (IRVM_t *_this, ir_method_t *method, ir_instruction_t *inst, t_jit_function_internal *jitFunction, JITBOOLEAN checking_overflow) {
	jit_value_t param;
	jit_type_t type;

	#ifdef DEBUG
	jit_type_t _type;
	#endif

	/* Assertions			*/
	assert(_this != NULL);
	assert(inst != NULL);
	assert(method != NULL);
	assert((inst->result).type == IROFFSET);
	assert((inst->param_1).type != NOPARAM);
	assert((inst->param_2).type == IRTYPE);

	/* Init the variables		*/
	PDEBUG("IRVIRTUALMACHINE:               Variable %lld = conv(", (inst->result).value.v);
	type = NULL;

	/* Create the first parameter	*/
	param = make_variable(_this, method, &(inst->param_1), jitFunction);
	assert(param != NULL);
	PDEBUG(", ");

	/* Fetch the type of the value	*
	 * to store			*/
	#ifdef DEBUG
	_type = jit_value_get_type(param);
	assert(_type != NULL);
	#endif

	/* Create the type		*/
	switch ((inst->param_2).value.v) {
	    case IRINT8:
		    PDEBUG("INT8)\n");
		    type = jit_type_sbyte;
		    break;
	    case IRINT16:
		    PDEBUG("INT16)\n");
		    type = jit_type_short;
		    break;
	    case IRINT32:
		    PDEBUG("INT32)\n");
		    type = jit_type_int;
		    break;
	    case IRINT64:
		    PDEBUG("INT64)\n");
		    type = jit_type_long;
		    break;
	    case IRNINT:
		    PDEBUG("Native INT)\n");
		    type = jit_type_nint;
		    break;
	    case IRUINT8:
		    PDEBUG("UINT8)\n");
		    type = jit_type_ubyte;
		    break;
	    case IRUINT16:
		    PDEBUG("UINT16)\n");
		    type = jit_type_ushort;
		    break;
	    case IRUINT32:
		    PDEBUG("UINT32)\n");
		    type = jit_type_uint;
		    break;
	    case IRUINT64:
		    PDEBUG("UINT64)\n");
		    type = jit_type_ulong;
		    break;
	    case IRNUINT:
		    PDEBUG("Native UINT)\n");
		    type = jit_type_nuint;
		    break;
	    case IRFLOAT32:
		    PDEBUG("FLOAT32)\n");
		    type = jit_type_float32;
		    break;
	    case IRFLOAT64:
		    PDEBUG("FLOAT64)\n");
		    type = jit_type_float64;
		    break;
	    case IRNFLOAT:
		    if (sizeof(JITNFLOAT) == 4) {
			    PDEBUG("FLOAT32)\n");
			    type = jit_type_float32;
		    } else {
			    PDEBUG("FLOAT64)\n");
			    type = jit_type_float64;
		    }
		    break;
	    case IROBJECT:
	    case IRMPOINTER:
		    PDEBUG("UPOINTER)\n");
		    type = jit_type_void_ptr;
		    break;
	    default:
		    print_err("IRVIRTUALMACHINE: ERROR = Conversion type is not known. ", 0);
		    abort();
	}
	#ifdef DEBUG
	_type = jit_value_get_type(param);
	assert(_type != NULL);
	#endif

	/* Create the instruction		*/
	if (jitFunction->locals[(inst->result).value.v] != NULL) {
		jit_value_t temp;
		temp = jit_insn_convert(jitFunction->function, param, type, (checking_overflow != 0));
		jit_insn_store(jitFunction->function, jitFunction->locals[(inst->result).value.v], temp);
	} else{
		jitFunction->locals[(inst->result).value.v] = jit_insn_convert(jitFunction->function, param, type, (checking_overflow != 0));
	}
	assert(jitFunction->locals[(inst->result).value.v] != NULL);

	/* Return				*/
	return 0;
}

void * IRVM_getFunctionPointer (IRVM_t *self, t_jit_function *jitFunction){
	void    *func;
	t_jit_function_internal *internalFunction;

	/* Assertions			*/
	assert(self != NULL);
	assert(jitFunction != NULL);

	/* Cache some pointers			*/
	internalFunction = jitFunction->data;
	assert(internalFunction != NULL);

	/* Fetch the function pointer
	 * libjit jit_function_to_closure is already thread safe.
	 * NEVER USE LIBJIT BIG LOCK
	 * */
	func = jit_function_to_closure(internalFunction->function);

	/* Return			*/
	return func;
}

ir_method_t * IRVM_getRunningMethod (IRVM_t *self){
	ir_method_t *m;
	void *call_frame;
	void *ilmethod;
	jit_function_t jit_caller_method;
	void *exceptionHandler;
	IRVM_t_internal *internalVM;

	/* Assertions				*/
	assert(self != NULL);

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) self->data;
	assert(internalVM != NULL);

	/* Fetch the current call frame		*/
	call_frame = jit_get_current_frame();
	assert(call_frame != NULL);

	/* Find the first callframe that has a  *
	* libjit function assigned.            */
	do {
		void *return_address;
		return_address = jit_get_return_address(call_frame);
		if (!(call_frame = _jit_get_next_frame_address(call_frame))) {
			return NULL;
		}
		if ((jit_caller_method = jit_function_from_pc(internalVM->context, return_address, &exceptionHandler))) {
			break;
		}
	} while (1);
	assert(jit_caller_method != NULL);

	/* Fetch the IL method		*/
	ilmethod = jit_function_get_meta(jit_caller_method, METHOD_METADATA);
	assert(ilmethod != NULL);

	/* Fetch the IR method		*/
	m = self->getIRMethod((IR_ITEM_VALUE) (JITNUINT) ilmethod);

	/* Return			*/
	return m;
}

ir_method_t * IRVM_getCallerOfMethodInExecution (IRVM_t *self, ir_method_t *m){
	void *call_frame;
	void *ilmethod;
	jit_function_t jit_caller_method;
	void *exceptionHandler;
	ir_method_t *caller;
	JITBOOLEAN returnNext;
	IRVM_t_internal *internalVM;

	/* Assertions				*/
	assert(self != NULL);

	/* Initialize the variables		*/
	returnNext = JITFALSE;
	caller = NULL;

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) self->data;
	assert(internalVM != NULL);

	/* Fetch the current call frame		*/
	call_frame = jit_get_current_frame();
	assert(call_frame != NULL);

	/* Find the first callframe that has a  *
	* libjit function assigned.            */
	do {
		void *return_address;
		return_address = jit_get_return_address(call_frame);
		if (!(call_frame = _jit_get_next_frame_address(call_frame))) {
			return NULL;
		}
		if ((jit_caller_method = jit_function_from_pc(internalVM->context, return_address, &exceptionHandler))) {
			ir_method_t *currentM;
			ilmethod = jit_function_get_meta(jit_caller_method, METHOD_METADATA);
			assert(ilmethod != NULL);
			currentM = self->getIRMethod((IR_ITEM_VALUE) (JITNUINT) ilmethod);
			if (returnNext) {
				caller = currentM;
				break;
			}
			if (currentM == m) {
				returnNext = JITTRUE;
			}
		}
	} while (1);

	/* Return			*/
	return caller;
}

void IRVM_getRunningMethodCallerInstruction (IRVM_t *self, ir_method_t **method, ir_instruction_t **instr){
	JITINT32 instrID;
	void *call_frame;
	void *ilmethod;
	jit_function_t jit_caller_method;
	void *exceptionHandler;
	void *return_address;
	IRVM_t_internal *internalVM;

	/* Assertions				*/
	assert(self != NULL);

	/* Initialize the variables		*/
	instrID	= 0;
	(*method) = NULL;
	(*instr) = NULL;
	return_address = NULL;

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) self->data;
	assert(internalVM != NULL);

	/* Fetch the current call frame		*/
	call_frame = jit_get_current_frame();
	assert(call_frame != NULL);

	/* Find the first callframe that has a  *
	* libjit function assigned.            */
	do {
		return_address = jit_get_return_address(call_frame);
		if (!(call_frame = _jit_get_next_frame_address(call_frame))) {
			return;
		}
		if ((jit_caller_method = jit_function_from_pc(internalVM->context, return_address, &exceptionHandler))) {
			break;
		}
	} while (1);
	assert(jit_caller_method != NULL);

	/* Fetch the IR method & instruction		*/
	ilmethod = jit_function_get_meta(jit_caller_method, METHOD_METADATA);
	assert(ilmethod != NULL);
	(*method) = self->getIRMethod((IR_ITEM_VALUE) (JITNUINT) ilmethod);
	/*instrID = jit_function_get_offset_from_pc(self->context, jit_caller_method,return_address);
	   (*instr) = IRMETHOD_getInstructionAtPosition((*method), instrID);*/

}

void IRVM_dumpMachineCode (IRVM_t *self, FILE *outputFile, ir_method_t *method){
	IR_ITEM_VALUE ilmethod;
	t_jit_function *jit_method;
	t_jit_function_internal *internalFunction;

	/* Fetch the libjit method		*/
	ilmethod = IRMETHOD_getMethodID(method);
	jit_method = self->getJITMethod(ilmethod);
	assert(jit_method != NULL);

	/* Cache some pointers			*/
	internalFunction = jit_method->data;
	assert(internalFunction != NULL);

	/* Dump the machine code		*/
	IRMETHOD_lock(method);
	jit_dump_function(outputFile, internalFunction->function, (char *) IRMETHOD_getSignatureInString(method));
	IRMETHOD_unlock(method);

	/* Return				*/
	return;
}

static inline void internal_set_variables_addressable (IRVM_t *_this, ir_method_t *method, t_jit_function_internal *jitFunction){
	JITUINT32 	i;
	JITUINT32	instructionsNumber;

	/* Assertions				*/
	assert(method != NULL);
	assert(jitFunction != NULL);

	/* Fetch the number of instructions	*/
	instructionsNumber	= IRMETHOD_getInstructionsNumber(method);

	/* Set the addressable variables        */
	for (i = 0; i < instructionsNumber; i++) {
		ir_instruction_t	*inst;
		ir_item_t		*item;
		jit_value_t 		value;

		/* Fetch the instruction		*/
		inst	= IRMETHOD_getInstructionAtPosition(method, i);

		/* Check the instruction		*/
		if (inst->type != IRGETADDRESS){
			continue ;
		}

		/* Fetch the escaped parameter		*/
		item		= IRMETHOD_getInstructionParameter1(inst);
		assert(item != NULL);
		assert(item->type == IROFFSET);
		item		= IRMETHOD_getIRVariable(method, (item->value).v);

		/* Mark as addressable			*/
		if (	(item != NULL)								&&
			((item->internal_type != IRVALUETYPE) || (item->type_infos != NULL))	){

			/* Fetch the libjit variable		*/
			value		= make_variable(_this, method, item, jitFunction);
			assert(value != NULL);

			/* Mark the libjit variable		*/
			jit_value_set_addressable(value);
		}
	}

	/* Return				*/
	return;
}

void IRVM_destroyFunction (IRVM_t *self, t_jit_function *function){
	t_jit_function_internal *internalFunction;

	/* Assertions				*/
	assert(function != NULL);

	/* Cache some pointers			*/
	internalFunction = function->data;

	/* Check if we need to free some memory	*/
	if (internalFunction == NULL) {
		return;
	}
	assert(internalFunction != NULL);

	/* Destroy the JIT information          */
	if (internalFunction->locals != NULL) {
		freeMemory(internalFunction->locals);
		internalFunction->locals = NULL;
	}

	/* Destroy the signature	*/
	jit_type_free(internalFunction->signature);

	/* Destroy the internal data	*/
	freeFunction(internalFunction);
	function->data = NULL;

	/* Return			*/
	return;
}

void IRVM_translateIRTypeToIRVMType (IRVM_t *IRVM, JITUINT16 irType, IRVM_type *irvmType, TypeDescriptor *ilClass){
	irvmType->type = internal_fromIRTypeToJITType(IRVM, irType, ilClass);
}

void IRVM_setIRVMStructure (IRVM_t *self, IRVM_type *irvmType, JITNINT structureSize, JITNINT alignment){
	irvmType->type = jit_type_create_struct(0, 0, 0);
	jit_type_set_size_and_alignment(irvmType->type, structureSize, alignment);
}

JITUINT32 IRVM_getIRVMTypeSize (IRVM_t *self, IRVM_type *irvmType){
	return jit_type_get_size(irvmType->type);
}

void IRVM_shutdown (IRVM_t *_this){
	IRVM_t_internal *internalVM;

	/* Assertions			*/
	assert(_this != NULL);

	/* Cache some pointers			*/
	internalVM = (IRVM_t_internal *) _this->data;
	assert(internalVM != NULL);

	/* Destroy the mutex		*/
	PLATFORM_destroyMutex(&(_this->vmMutex));

	/* Destroy the Libjit type	*/
	jit_type_free(internalVM->basicExceptionConstructorJITSignature);
	/*TODO: jit_context_destroy has to be performed,
	 * but there is a bug in libjit that prevents it to work
	 * correctly: there are wrong accesses to the memory, that
	 * are visible with valgrind.
	 *
	 * RE-ACTIVATE it when this bug is corrected */
	//jit_context_destroy(_this->context);

	/* Destroy the internal data	*/
	freeFunction(_this->data);

	/* Return			*/
	return;
}

void IRVM_translateIRMethodSignatureToBackendSignature (IRVM_t *_this, ir_signature_t *ir_signature, t_jit_function *backendFunction){
	jit_type_t signature;
	jit_type_t result_type;
	jit_type_t *jit_params;
	JITUINT32 parameters_number;
	JITUINT32 jit_parameter_index;
	JITUINT32 count;
	t_jit_function_internal *internalFunction;

	/* Assertions                           */
	assert(ir_signature != NULL);
	assert(backendFunction != NULL);

	/* Initialize the local variables       */
	jit_params = NULL;
	result_type = NULL;
	jit_parameter_index = 0;
	count = 0;

	/* Fetch the number of parameters	*/
	parameters_number = ir_signature->parametersNumber;
	assert(parameters_number >= 0);

	/* Cache some pointers			*/
	if (backendFunction->data == NULL) {
		backendFunction->data = allocFunction(sizeof(t_jit_function_internal));
	}
	internalFunction = backendFunction->data;
	assert(internalFunction != NULL);

	/* initialize the array of parameters   */
	if (parameters_number > 0) {
		jit_params = (jit_type_t *) allocMemory(sizeof(jit_type_t) * parameters_number);
		assert(jit_params != NULL);
	}

	/* Retrieve the jit return type */
	if (ir_signature->resultType == IRVALUETYPE) {
		TypeDescriptor  *valueType;

		/* We have to create the new libjit type. In order to do this we have		*
		 * to query the layout manager.							*
		 * Retrieve the ILType of the valuetype                                         */
		valueType = ir_signature->ilResultType;
		assert(valueType != NULL);

		/* retrieve the layout informations for the current valueType */
		result_type = (_this->lookupValueType(valueType))->type;
	} else{
		result_type = internal_fromIRTypeToJITType(_this, ir_signature->resultType, NULL);
	}
	assert(result_type != NULL);

	if (jit_params != NULL) {
		JITUINT32 counter;

		/* Initialize the internal types of the         *
		 * parameters					*/
		jit_parameter_index = 0;
		for (counter = 0; counter < parameters_number; counter++, jit_parameter_index++) {

			/* Initialize the current parameter     */
			if ((ir_signature->parameterTypes[counter]) != IRVALUETYPE){

				/* the parameter is not a valuetype */
				jit_params[jit_parameter_index] = internal_fromIRTypeToJITType(_this, ir_signature->parameterTypes[counter], NULL);

			} else {
				TypeDescriptor  *current_Valuetype;

				/* initialize the local variables */
				current_Valuetype = NULL;

				/* retrieve the valuetype metadata informations */
				current_Valuetype = ir_signature->ilParamsTypes[counter];
				assert(current_Valuetype != NULL);

				/* retrieve the jit type associated with the valuetype */
				jit_params[jit_parameter_index] = (_this->lookupValueType(current_Valuetype))->type;
				assert(jit_params[jit_parameter_index] != NULL);
			}
			assert(jit_params[jit_parameter_index] != NULL);
		}
	}

	/* once created the parameters and the return type we have to   *
	 * correctly create the JIT-SIGNATURE.                          *
	 * Create the JIT signature. We do not need to synchronize	*
	 * with libjit because the jit_type_create_signature            *
	 * function is already thread safe.				*/
	internal_lockLibjit(_this->data);
	if (result_type == jit_type_void) {
		signature = jit_type_create_signature(jit_abi_cdecl, result_type, jit_params, parameters_number, 0);
	} else{
		signature = jit_type_create_signature(jit_abi_cdecl, result_type, jit_params, parameters_number, 1);
	}
	internal_unlockLibjit(_this->data);
	internalFunction->signature = signature;

	/* Free the memory		*/
	if (jit_params != NULL) {
		freeMemory(jit_params);
	}

	/* Return			*/
	return;
}

JITINT32 IRVM_isANativeFunction (IRVM_t *self, t_jit_function *function){
	t_jit_function_internal *internalFunction;

	/* Assertions				*/
	assert(function != NULL);

	/* Cache some pointers			*/
	internalFunction = function->data;
	assert(internalFunction != NULL);

	/* Return				*/
	return internalFunction->nativeFunction != NULL;
}

void * IRVM_getEntryPoint (IRVM_t *self, t_jit_function *function){
	t_jit_function_internal *internalFunction;

	/* Assertions				*/
	assert(function != NULL);

	/* Cache some pointers			*/
	internalFunction = function->data;
	assert(internalFunction != NULL);

	/* Check the function			*/
	if (internalFunction->nativeFunction != NULL) {

		/* Return				*/
		return internalFunction->nativeFunction;
	}

	/* Return				*/
	return internalFunction->entryPoint;
}

JITINT32 IRVM_hasTheSignatureBeenTranslated (IRVM_t *self, t_jit_function *backendFunction){
	t_jit_function_internal *internalFunction;

	/* Allocate the necessary memory	*/
	if (backendFunction->data == NULL) {
		backendFunction->data = allocFunction(sizeof(t_jit_function_internal));
	}

	/* Cache some pointers			*/
	internalFunction = backendFunction->data;
	assert(internalFunction != NULL);

	/* Return				*/
	return internalFunction->signature != NULL;
}

void IRVM_setNativeEntryPoint (IRVM_t *self, t_jit_function *function, void *nativeEntryPoint){
	t_jit_function_internal *internalFunction;

	/* Assertions				*/
	assert(function != NULL);

	/* Cache some pointers			*/
	internalFunction = function->data;
	assert(internalFunction != NULL);

	/* Set the native function		*/
	internalFunction->nativeFunction = nativeEntryPoint;

	/* Return				*/
	return;
}

JITINT32 IRVM_isCompilationAvailable (IRVM_t *self){
	return !(jit_uses_interpreter());
}

void IRVM_startEntryPointExecution (IRVM_t *self){

}
